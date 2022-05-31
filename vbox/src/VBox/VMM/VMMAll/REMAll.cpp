/* $Id: REMAll.cpp 20869 2009-06-24 00:27:17Z vboxsync $ */
/** @file
 * REM - Recompiled Execution Monitor, all Contexts part.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_REM
#include <VBox/rem.h>
#include <VBox/em.h>
#include <VBox/vmm.h>
#include "REMInternal.h"
#include <VBox/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/assert.h>


#ifndef IN_RING3

/**
 * Records a invlpg instruction for replaying upon REM entry.
 *
 * @returns VINF_SUCCESS on success.
 * @param   pVM         The VM handle.
 * @param   GCPtrPage   The
 */
VMMDECL(int) REMNotifyInvalidatePage(PVM pVM, RTGCPTR GCPtrPage)
{
    /*
     * Try take the REM lock and push the address onto the array.
     */
    if (   pVM->rem.s.cInvalidatedPages < RT_ELEMENTS(pVM->rem.s.aGCPtrInvalidatedPages)
        && EMTryEnterRemLock(pVM) == VINF_SUCCESS)
    {
        uint32_t iPage = pVM->rem.s.cInvalidatedPages;
        if (iPage < RT_ELEMENTS(pVM->rem.s.aGCPtrInvalidatedPages))
        {
            ASMAtomicWriteU32(&pVM->rem.s.cInvalidatedPages, iPage + 1);
            pVM->rem.s.aGCPtrInvalidatedPages[iPage] = GCPtrPage;

            EMRemUnlock(pVM);
            return VINF_SUCCESS;
        }

        CPUMSetChangedFlags(VMMGetCpu(pVM), CPUM_CHANGED_GLOBAL_TLB_FLUSH); /** @todo this should be flagged globally, not locally! ... this array should be per-cpu technically speaking. */
        ASMAtomicWriteU32(&pVM->rem.s.cInvalidatedPages, 0); /** @todo leave this alone? Optimize this code? */

        EMRemUnlock(pVM);
    }
    else
    {
        /* Fallback: Simply tell the recompiler to flush its TLB. */
        CPUMSetChangedFlags(VMMGetCpu(pVM), CPUM_CHANGED_GLOBAL_TLB_FLUSH);
        ASMAtomicWriteU32(&pVM->rem.s.cInvalidatedPages, 0); /** @todo leave this alone?! Optimize this code? */
    }

    return VINF_SUCCESS;
}


/**
 * Flushes the handler notifications by calling the host.
 *
 * @param   pVM     The VM handle.
 */
static void remFlushHandlerNotifications(PVM pVM)
{
#ifdef IN_RC
    VMMGCCallHost(pVM, VMMCALLHOST_REM_REPLAY_HANDLER_NOTIFICATIONS, 0);
#elif defined(IN_RING0)
    /** @todo necessary? */
    VMMR0CallHost(pVM, VMMCALLHOST_REM_REPLAY_HANDLER_NOTIFICATIONS, 0);
#else
    AssertReleaseMsgFailed(("Ring 3 call????.\n"));
#endif
}


/**
 * Insert pending notification
 *
 * @param   pVM             VM Handle.
 * @param   pRec            Notification record to insert
 */
static void remNotifyHandlerInsert(PVM pVM, PREMHANDLERNOTIFICATION pRec)
{
    /*
     * Fetch a free record.
     */
    uint32_t                cFlushes = 0;
    uint32_t                idxFree;
    PREMHANDLERNOTIFICATION pFree;
    do
    {
        idxFree = ASMAtomicUoReadU32(&pVM->rem.s.idxFreeList);
        if (idxFree == (uint32_t)-1)
        {
            do
            {
                Assert(cFlushes++ != 128);
                AssertFatal(cFlushes < _1M);
                remFlushHandlerNotifications(pVM);
                idxFree = ASMAtomicUoReadU32(&pVM->rem.s.idxFreeList);
            } while (idxFree == (uint32_t)-1);
        }
        pFree = &pVM->rem.s.aHandlerNotifications[idxFree];
    } while (!ASMAtomicCmpXchgU32(&pVM->rem.s.idxFreeList, pFree->idxNext, idxFree));

    /*
     * Copy the record.
     */
    pFree->enmKind = pRec->enmKind;
    pFree->u = pRec->u;

    /*
     * Insert it into the pending list.
     */
    uint32_t idxNext;
    do
    {
        idxNext = ASMAtomicUoReadU32(&pVM->rem.s.idxPendingList);
        ASMAtomicWriteU32(&pFree->idxNext, idxNext);
        ASMCompilerBarrier();
    } while (!ASMAtomicCmpXchgU32(&pVM->rem.s.idxPendingList, idxFree, idxNext));

    VM_FF_SET(pVM, VM_FF_REM_HANDLER_NOTIFY);

#if 0 /* Enable this to trigger odd flush bugs. */
    remFlushHandlerNotifications(pVM);
#endif
}


/**
 * Notification about a successful PGMR3HandlerPhysicalRegister() call.
 *
 * @param   pVM             VM Handle.
 * @param   enmType         Handler type.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler have a HC callback function.
 */
VMMDECL(void) REMNotifyHandlerPhysicalRegister(PVM pVM, PGMPHYSHANDLERTYPE enmType, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler)
{
    REMHANDLERNOTIFICATION Rec;
    Rec.enmKind = REMHANDLERNOTIFICATIONKIND_PHYSICAL_REGISTER;
    Rec.u.PhysicalRegister.enmType = enmType;
    Rec.u.PhysicalRegister.GCPhys = GCPhys;
    Rec.u.PhysicalRegister.cb = cb;
    Rec.u.PhysicalRegister.fHasHCHandler = fHasHCHandler;
    remNotifyHandlerInsert(pVM, &Rec);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalDeregister() operation.
 *
 * @param   pVM             VM Handle.
 * @param   enmType         Handler type.
 * @param   GCPhys          Handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler have a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
VMMDECL(void) REMNotifyHandlerPhysicalDeregister(PVM pVM, PGMPHYSHANDLERTYPE enmType, RTGCPHYS GCPhys, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    REMHANDLERNOTIFICATION Rec;
    Rec.enmKind = REMHANDLERNOTIFICATIONKIND_PHYSICAL_DEREGISTER;
    Rec.u.PhysicalDeregister.enmType = enmType;
    Rec.u.PhysicalDeregister.GCPhys = GCPhys;
    Rec.u.PhysicalDeregister.cb = cb;
    Rec.u.PhysicalDeregister.fHasHCHandler = fHasHCHandler;
    Rec.u.PhysicalDeregister.fRestoreAsRAM = fRestoreAsRAM;
    remNotifyHandlerInsert(pVM, &Rec);
}


/**
 * Notification about a successful PGMR3HandlerPhysicalModify() call.
 *
 * @param   pVM             VM Handle.
 * @param   enmType         Handler type.
 * @param   GCPhysOld       Old handler range address.
 * @param   GCPhysNew       New handler range address.
 * @param   cb              Size of the handler range.
 * @param   fHasHCHandler   Set if the handler have a HC callback function.
 * @param   fRestoreAsRAM   Whether the to restore it as normal RAM or as unassigned memory.
 */
VMMDECL(void) REMNotifyHandlerPhysicalModify(PVM pVM, PGMPHYSHANDLERTYPE enmType, RTGCPHYS GCPhysOld, RTGCPHYS GCPhysNew, RTGCPHYS cb, bool fHasHCHandler, bool fRestoreAsRAM)
{
    REMHANDLERNOTIFICATION Rec;
    Rec.enmKind = REMHANDLERNOTIFICATIONKIND_PHYSICAL_MODIFY;
    Rec.u.PhysicalModify.enmType = enmType;
    Rec.u.PhysicalModify.GCPhysOld = GCPhysOld;
    Rec.u.PhysicalModify.GCPhysNew = GCPhysNew;
    Rec.u.PhysicalModify.cb = cb;
    Rec.u.PhysicalModify.fHasHCHandler = fHasHCHandler;
    Rec.u.PhysicalModify.fRestoreAsRAM = fRestoreAsRAM;
    remNotifyHandlerInsert(pVM, &Rec);
}

#endif /* !IN_RING3 */

/**
 * Make REM flush all translation block upon the next call to REMR3State().
 *
 * @param   pVM             Pointer to the shared VM structure.
 */
VMMDECL(void) REMFlushTBs(PVM pVM)
{
    LogFlow(("REMFlushTBs: fFlushTBs=%RTbool fInREM=%RTbool fInStateSync=%RTbool\n",
             pVM->rem.s.fFlushTBs, pVM->rem.s.fInREM, pVM->rem.s.fInStateSync));
    pVM->rem.s.fFlushTBs = true;
}


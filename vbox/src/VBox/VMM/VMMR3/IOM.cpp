/* $Id: IOM.cpp 82311 2019-12-01 01:45:02Z vboxsync $ */
/** @file
 * IOM - Input / Output Monitor.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/** @page pg_iom        IOM - The Input / Output Monitor
 *
 * The input/output monitor will handle I/O exceptions routing them to the
 * appropriate device. It implements an API to register and deregister virtual
 * I/0 port handlers and memory mapped I/O handlers. A handler is PDM devices
 * and a set of callback functions.
 *
 * @see grp_iom
 *
 *
 * @section sec_iom_rawmode     Raw-Mode
 *
 * In raw-mode I/O port access is trapped (\#GP(0)) by ensuring that the actual
 * IOPL is 0 regardless of what the guest IOPL is. The \#GP handler use the
 * disassembler (DIS) to figure which instruction caused it (there are a number
 * of instructions in addition to the I/O ones) and if it's an I/O port access
 * it will hand it to IOMRCIOPortHandler (via EMInterpretPortIO).
 * IOMRCIOPortHandler will lookup the port in the AVL tree of registered
 * handlers. If found, the handler will be called otherwise default action is
 * taken. (Default action is to write into the void and read all set bits.)
 *
 * Memory Mapped I/O (MMIO) is implemented as a slightly special case of PGM
 * access handlers. An MMIO range is registered with IOM which then registers it
 * with the PGM access handler sub-system. The access handler catches all
 * access and will be called in the context of a \#PF handler. In RC and R0 this
 * handler is iomMmioPfHandler while in ring-3 it's iomR3MmioHandler (although
 * in ring-3 there can be alternative ways). iomMmioPfHandler will attempt to
 * emulate the instruction that is doing the access and pass the corresponding
 * reads / writes to the device.
 *
 * Emulating I/O port access is less complex and should be slightly faster than
 * emulating MMIO, so in most cases we should encourage the OS to use port I/O.
 * Devices which are frequently accessed should register GC handlers to speed up
 * execution.
 *
 *
 * @section sec_iom_hm     Hardware Assisted Virtualization Mode
 *
 * When running in hardware assisted virtualization mode we'll be doing much the
 * same things as in raw-mode. The main difference is that we're running in the
 * host ring-0 context and that we don't get faults (\#GP(0) and \#PG) but
 * exits.
 *
 *
 * @section sec_iom_rem         Recompiled Execution Mode
 *
 * When running in the recompiler things are different. I/O port access is
 * handled by calling IOMIOPortRead and IOMIOPortWrite directly. While MMIO can
 * be handled in one of two ways. The normal way is that we have a registered a
 * special RAM range with the recompiler and in the three callbacks (for byte,
 * word and dword access) we call IOMMMIORead and IOMMMIOWrite directly. The
 * alternative ways that the physical memory access which goes via PGM will take
 * care of it by calling iomR3MmioHandler via the PGM access handler machinery
 * - this shouldn't happen but it is an alternative...
 *
 *
 * @section sec_iom_other       Other Accesses
 *
 * I/O ports aren't really exposed in any other way, unless you count the
 * instruction interpreter in EM, but that's just what we're doing in the
 * raw-mode \#GP(0) case really. Now, it's possible to call IOMIOPortRead and
 * IOMIOPortWrite directly to talk to a device, but this is really bad behavior
 * and should only be done as temporary hacks (the PC BIOS device used to setup
 * the CMOS this way back in the dark ages).
 *
 * MMIO has similar direct routes as the I/O ports and these shouldn't be used
 * for the same reasons and with the same restrictions. OTOH since MMIO is
 * mapped into the physical memory address space, it can be accessed in a number
 * of ways thru PGM.
 *
 *
 * @section sec_iom_logging     Logging Levels
 *
 * Following assignments:
 *      - Level 5 is used for defering I/O port and MMIO writes to ring-3.
 *
 */

/** @todo MMIO - simplifying the device end.
 * - Add a return status for doing DBGFSTOP on access where there are no known
 *   registers.
 * -
 *
 *   */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_IOM
#include <VBox/vmm/iom.h>
#include <VBox/vmm/cpum.h>
#include <VBox/vmm/pgm.h>
#include <VBox/sup.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/stam.h>
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/vmm/pdmdev.h>
#include "IOMInternal.h"
#include <VBox/vmm/vm.h>

#include <VBox/param.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/string.h>
#include <VBox/log.h>
#include <VBox/err.h>

#include "IOMInline.h"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void iomR3FlushCache(PVM pVM);

#ifdef VBOX_WITH_STATISTICS
static const char *iomR3IOPortGetStandardName(RTIOPORT Port);
#endif


/**
 * Initializes the IOM.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) IOMR3Init(PVM pVM)
{
    LogFlow(("IOMR3Init:\n"));

    /*
     * Assert alignment and sizes.
     */
    AssertCompileMemberAlignment(VM, iom.s, 32);
    AssertCompile(sizeof(pVM->iom.s) <= sizeof(pVM->iom.padding));
    AssertCompileMemberAlignment(IOM, CritSect, sizeof(uintptr_t));

    /*
     * Initialize the REM critical section.
     */
#ifdef IOM_WITH_CRIT_SECT_RW
    int rc = PDMR3CritSectRwInit(pVM, &pVM->iom.s.CritSect, RT_SRC_POS, "IOM Lock");
#else
    int rc = PDMR3CritSectInit(pVM, &pVM->iom.s.CritSect, RT_SRC_POS, "IOM Lock");
#endif
    AssertRCReturn(rc, rc);

    /*
     * Allocate the trees structure.
     */
    rc = MMHyperAlloc(pVM, sizeof(*pVM->iom.s.pTreesR3), 0, MM_TAG_IOM, (void **)&pVM->iom.s.pTreesR3);
    AssertRCReturn(rc, rc);
    pVM->iom.s.pTreesR0 = MMHyperR3ToR0(pVM, pVM->iom.s.pTreesR3);

    /*
     * Register the MMIO access handler type.
     */
    rc = PGMR3HandlerPhysicalTypeRegister(pVM, PGMPHYSHANDLERKIND_MMIO,
                                          iomMmioHandler,
                                          NULL, "iomMmioHandler", "iomMmioPfHandler",
                                          NULL, "iomMmioHandler", "iomMmioPfHandler",
                                          "MMIO", &pVM->iom.s.hMmioHandlerType);
    AssertRCReturn(rc, rc);

    rc = PGMR3HandlerPhysicalTypeRegister(pVM, PGMPHYSHANDLERKIND_MMIO,
                                          iomMmioHandlerNew,
                                          NULL, "iomMmioHandlerNew", "iomMmioPfHandlerNew",
                                          NULL, "iomMmioHandlerNew", "iomMmioPfHandlerNew",
                                          "MMIO New", &pVM->iom.s.hNewMmioHandlerType);
    AssertRCReturn(rc, rc);

    /*
     * Info.
     */
    DBGFR3InfoRegisterInternal(pVM, "ioport", "Dumps all IOPort ranges. No arguments.", &iomR3IoPortInfo);
    DBGFR3InfoRegisterInternal(pVM, "mmio", "Dumps all MMIO ranges. No arguments.", &iomR3MmioInfo);

    /*
     * Statistics.
     */
    STAM_REG(pVM, &pVM->iom.s.StatIoPortCommits,      STAMTYPE_COUNTER, "/IOM/IoPortCommits",                       STAMUNIT_OCCURENCES,     "Number of ring-3 I/O port commits.");

    STAM_REL_REG(pVM, &pVM->iom.s.StatMMIOStaleMappings, STAMTYPE_PROFILE, "/IOM/MMIOStaleMappings",                STAMUNIT_TICKS_PER_CALL, "Number of times iomMmioHandlerNew got a call for a remapped range at the old mapping.");
    STAM_REG(pVM, &pVM->iom.s.StatRZMMIOHandler,      STAMTYPE_PROFILE, "/IOM/RZ-MMIOHandler",                      STAMUNIT_TICKS_PER_CALL, "Profiling of the iomMmioPfHandler() body, only success calls.");
    STAM_REG(pVM, &pVM->iom.s.StatRZMMIOReadsToR3,    STAMTYPE_COUNTER, "/IOM/RZ-MMIOHandler/ReadsToR3",            STAMUNIT_OCCURENCES,     "Number of read deferred to ring-3.");
    STAM_REG(pVM, &pVM->iom.s.StatRZMMIOWritesToR3,   STAMTYPE_COUNTER, "/IOM/RZ-MMIOHandler/WritesToR3",           STAMUNIT_OCCURENCES,     "Number of writes deferred to ring-3.");
    STAM_REG(pVM, &pVM->iom.s.StatRZMMIOCommitsToR3,  STAMTYPE_COUNTER, "/IOM/RZ-MMIOHandler/CommitsToR3",          STAMUNIT_OCCURENCES,     "Number of commits deferred to ring-3.");
    STAM_REG(pVM, &pVM->iom.s.StatRZMMIODevLockContention, STAMTYPE_COUNTER, "/IOM/RZ-MMIOHandler/DevLockContention", STAMUNIT_OCCURENCES,   "Number of device lock contention force return to ring-3.");
    STAM_REG(pVM, &pVM->iom.s.StatR3MMIOHandler,      STAMTYPE_COUNTER, "/IOM/R3-MMIOHandler",                      STAMUNIT_OCCURENCES,     "Number of calls to iomMmioHandler.");

    STAM_REG(pVM, &pVM->iom.s.StatMmioHandlerR3,      STAMTYPE_COUNTER, "/IOM/OldMmioHandlerR3",                    STAMUNIT_OCCURENCES,     "Number of calls to old iomMmioHandler from ring-3.");
    STAM_REG(pVM, &pVM->iom.s.StatMmioHandlerR0,      STAMTYPE_COUNTER, "/IOM/OldMmioHandlerR0",                    STAMUNIT_OCCURENCES,     "Number of calls to old iomMmioHandler from ring-0.");

    STAM_REG(pVM, &pVM->iom.s.StatMmioHandlerNewR3,   STAMTYPE_COUNTER, "/IOM/MmioHandlerNewR3",                    STAMUNIT_OCCURENCES,     "Number of calls to iomMmioHandlerNew from ring-3.");
    STAM_REG(pVM, &pVM->iom.s.StatMmioHandlerNewR0,   STAMTYPE_COUNTER, "/IOM/MmioHandlerNewR0",                    STAMUNIT_OCCURENCES,     "Number of calls to iomMmioHandlerNew from ring-0.");
    STAM_REG(pVM, &pVM->iom.s.StatMmioPfHandlerNew,   STAMTYPE_COUNTER, "/IOM/MmioPfHandlerNew",                    STAMUNIT_OCCURENCES,     "Number of calls to iomMmioPfHandlerNew.");
    STAM_REG(pVM, &pVM->iom.s.StatMmioPhysHandlerNew, STAMTYPE_COUNTER, "/IOM/MmioPhysHandlerNew",                  STAMUNIT_OCCURENCES,     "Number of calls to IOMR0MmioPhysHandler.");
    STAM_REG(pVM, &pVM->iom.s.StatMmioCommitsDirect,  STAMTYPE_COUNTER, "/IOM/MmioCommitsDirect",                   STAMUNIT_OCCURENCES,     "Number of ring-3 MMIO commits direct to handler via handle hint.");
    STAM_REG(pVM, &pVM->iom.s.StatMmioCommitsPgm,     STAMTYPE_COUNTER, "/IOM/MmioCommitsPgm",                      STAMUNIT_OCCURENCES,     "Number of ring-3 MMIO commits via PGM.");

    /* Redundant, but just in case we change something in the future */
    iomR3FlushCache(pVM);

    LogFlow(("IOMR3Init: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Called when a VM initialization stage is completed.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   enmWhat         The initialization state that was completed.
 */
VMMR3_INT_DECL(int) IOMR3InitCompleted(PVM pVM, VMINITCOMPLETED enmWhat)
{
#ifdef VBOX_WITH_STATISTICS
    if (enmWhat == VMINITCOMPLETED_RING0)
    {
        /*
         * Synchronize the ring-3 I/O port and MMIO statistics indices into the
         * ring-0 tables to simplify ring-0 code.  This also make sure that any
         * later calls to grow the statistics tables will fail.
         */
        int rc = VMMR3CallR0Emt(pVM, pVM->apCpusR3[0], VMMR0_DO_IOM_SYNC_STATS_INDICES, 0, NULL);
        AssertLogRelRCReturn(rc, rc);

        /*
         * Register I/O port and MMIO stats now that we're done registering MMIO
         * regions and won't grow the table again.
         */
        for (uint32_t i = 0; i < pVM->iom.s.cIoPortRegs; i++)
        {
            PIOMIOPORTENTRYR3 pRegEntry = &pVM->iom.s.paIoPortRegs[i];
            if (   pRegEntry->fMapped
                && pRegEntry->idxStats != UINT16_MAX)
                iomR3IoPortRegStats(pVM, pRegEntry);
        }

        for (uint32_t i = 0; i < pVM->iom.s.cMmioRegs; i++)
        {
            PIOMMMIOENTRYR3 pRegEntry = &pVM->iom.s.paMmioRegs[i];
            if (   pRegEntry->fMapped
                && pRegEntry->idxStats != UINT16_MAX)
                iomR3MmioRegStats(pVM, pRegEntry);
        }
    }
#else
    RT_NOREF(pVM, enmWhat);
#endif
    return VINF_SUCCESS;
}


/**
 * Flushes the IOM port & statistics lookup cache
 *
 * @param   pVM     The cross context VM structure.
 */
static void iomR3FlushCache(PVM pVM)
{
    /*
     * Since all relevant (1) cache use requires at least read access to the
     * critical section, we can exclude all other EMTs by grabbing exclusive
     * access to the critical section and then safely update the caches of
     * other EMTs.
     * (1) The irrelvant access not holding the lock is in assertion code.
     */
    IOM_LOCK_EXCL(pVM);
    VMCPUID idCpu = pVM->cCpus;
    while (idCpu-- > 0)
    {
        PVMCPU pVCpu = pVM->apCpusR3[idCpu];
        pVCpu->iom.s.pMMIORangeLastR0  = NIL_RTR0PTR;
        pVCpu->iom.s.pMMIOStatsLastR0  = NIL_RTR0PTR;

        pVCpu->iom.s.pMMIORangeLastR3  = NULL;
        pVCpu->iom.s.pMMIOStatsLastR3  = NULL;
    }

    IOM_UNLOCK_EXCL(pVM);
}


/**
 * The VM is being reset.
 *
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(void) IOMR3Reset(PVM pVM)
{
    iomR3FlushCache(pVM);
}


/**
 * Applies relocations to data and code managed by this
 * component. This function will be called at init and
 * whenever the VMM need to relocate it self inside the GC.
 *
 * The IOM will update the addresses used by the switcher.
 *
 * @param   pVM     The cross context VM structure.
 * @param   offDelta    Relocation delta relative to old location.
 */
VMMR3_INT_DECL(void) IOMR3Relocate(PVM pVM, RTGCINTPTR offDelta)
{
    RT_NOREF(pVM, offDelta);
}

/**
 * Terminates the IOM.
 *
 * Termination means cleaning up and freeing all resources,
 * the VM it self is at this point powered off or suspended.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) IOMR3Term(PVM pVM)
{
    /*
     * IOM is not owning anything but automatically freed resources,
     * so there's nothing to do here.
     */
    NOREF(pVM);
    return VINF_SUCCESS;
}


#ifdef VBOX_WITH_STATISTICS

/**
 * Create the statistics node for an MMIO address.
 *
 * @returns Pointer to new stats node.
 *
 * @param   pVM         The cross context VM structure.
 * @param   GCPhys      The address.
 * @param   pszDesc     Description.
 */
PIOMMMIOSTATS iomR3MMIOStatsCreate(PVM pVM, RTGCPHYS GCPhys, const char *pszDesc)
{
    IOM_LOCK_EXCL(pVM);

    /* check if it already exists. */
    PIOMMMIOSTATS pStats = (PIOMMMIOSTATS)RTAvloGCPhysGet(&pVM->iom.s.pTreesR3->MmioStatTree, GCPhys);
    if (pStats)
    {
        IOM_UNLOCK_EXCL(pVM);
        return pStats;
    }

    /* allocate stats node. */
    int rc = MMHyperAlloc(pVM, sizeof(*pStats), 0, MM_TAG_IOM_STATS, (void **)&pStats);
    AssertRC(rc);
    if (RT_SUCCESS(rc))
    {
        /* insert into the tree. */
        pStats->Core.Key = GCPhys;
        if (RTAvloGCPhysInsert(&pVM->iom.s.pTreesR3->MmioStatTree, &pStats->Core))
        {
            IOM_UNLOCK_EXCL(pVM);

            rc = STAMR3RegisterF(pVM, &pStats->Accesses,    STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,     pszDesc, "/IOM/MMIO/%RGp",              GCPhys); AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pStats->ProfReadR3,  STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL, pszDesc, "/IOM/MMIO/%RGp/Read-R3",      GCPhys); AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pStats->ProfWriteR3, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL, pszDesc, "/IOM/MMIO/%RGp/Write-R3",     GCPhys); AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pStats->ProfReadRZ,  STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL, pszDesc, "/IOM/MMIO/%RGp/Read-RZ",      GCPhys); AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pStats->ProfWriteRZ, STAMTYPE_PROFILE, STAMVISIBILITY_USED, STAMUNIT_TICKS_PER_CALL, pszDesc, "/IOM/MMIO/%RGp/Write-RZ",     GCPhys); AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pStats->ReadRZToR3,  STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,     pszDesc, "/IOM/MMIO/%RGp/Read-RZtoR3",  GCPhys); AssertRC(rc);
            rc = STAMR3RegisterF(pVM, &pStats->WriteRZToR3, STAMTYPE_COUNTER, STAMVISIBILITY_USED, STAMUNIT_OCCURENCES,     pszDesc, "/IOM/MMIO/%RGp/Write-RZtoR3", GCPhys); AssertRC(rc);

            return pStats;
        }
        AssertMsgFailed(("what! GCPhys=%RGp\n", GCPhys));
        MMHyperFree(pVM, pStats);
    }
    IOM_UNLOCK_EXCL(pVM);
    return NULL;
}

#endif /* VBOX_WITH_STATISTICS */

/**
 * Registers a Memory Mapped I/O R3 handler.
 *
 * This API is called by PDM on behalf of a device. Devices must register ring-3 ranges
 * before any GC and R0 ranges can be registered using IOMR3MMIORegisterRC() and IOMR3MMIORegisterR0().
 *
 * @returns VBox status code.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pDevIns             PDM device instance owning the MMIO range.
 * @param   GCPhysStart         First physical address in the range.
 * @param   cbRange             The size of the range (in bytes).
 * @param   pvUser              User argument for the callbacks.
 * @param   pfnWriteCallback    Pointer to function which is gonna handle Write operations.
 * @param   pfnReadCallback     Pointer to function which is gonna handle Read operations.
 * @param   pfnFillCallback     Pointer to function which is gonna handle Fill/memset operations.
 * @param   fFlags              Flags, see IOMMMIO_FLAGS_XXX.
 * @param   pszDesc             Pointer to description string. This must not be freed.
 */
VMMR3_INT_DECL(int)
IOMR3MmioRegisterR3(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTHCPTR pvUser,
                    R3PTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallback, R3PTRTYPE(PFNIOMMMIOREAD) pfnReadCallback,
                    R3PTRTYPE(PFNIOMMMIOFILL) pfnFillCallback, uint32_t fFlags, const char *pszDesc)
{
    LogFlow(("IOMR3MmioRegisterR3: pDevIns=%p GCPhysStart=%RGp cbRange=%RGp pvUser=%RHv pfnWriteCallback=%#x pfnReadCallback=%#x pfnFillCallback=%#x fFlags=%#x pszDesc=%s\n",
             pDevIns, GCPhysStart, cbRange, pvUser, pfnWriteCallback, pfnReadCallback, pfnFillCallback, fFlags, pszDesc));
    int rc;

    /*
     * Validate input.
     */
    AssertMsgReturn(GCPhysStart + (cbRange - 1) >= GCPhysStart,("Wrapped! %RGp LB %RGp\n", GCPhysStart, cbRange),
                    VERR_IOM_INVALID_MMIO_RANGE);
    AssertMsgReturn(   !(fFlags & ~(IOMMMIO_FLAGS_VALID_MASK & ~IOMMMIO_FLAGS_ABS))
                    && (fFlags & IOMMMIO_FLAGS_READ_MODE)  <= IOMMMIO_FLAGS_READ_DWORD_QWORD
                    && (fFlags & IOMMMIO_FLAGS_WRITE_MODE) <= IOMMMIO_FLAGS_WRITE_ONLY_DWORD_QWORD,
                    ("%#x\n", fFlags),
                    VERR_INVALID_PARAMETER);

    /*
     * Allocate new range record and initialize it.
     */
    PIOMMMIORANGE pRange;
    rc = MMHyperAlloc(pVM, sizeof(*pRange), 0, MM_TAG_IOM, (void **)&pRange);
    if (RT_SUCCESS(rc))
    {
        pRange->Core.Key            = GCPhysStart;
        pRange->Core.KeyLast        = GCPhysStart + (cbRange - 1);
        pRange->GCPhys              = GCPhysStart;
        pRange->cb                  = cbRange;
        pRange->cRefs               = 1; /* The tree reference. */
        pRange->pszDesc             = pszDesc;

        //pRange->pvUserR0            = NIL_RTR0PTR;
        //pRange->pDevInsR0           = NIL_RTR0PTR;
        //pRange->pfnReadCallbackR0   = NIL_RTR0PTR;
        //pRange->pfnWriteCallbackR0  = NIL_RTR0PTR;
        //pRange->pfnFillCallbackR0   = NIL_RTR0PTR;

        //pRange->pvUserRC            = NIL_RTRCPTR;
        //pRange->pDevInsRC           = NIL_RTRCPTR;
        //pRange->pfnReadCallbackRC   = NIL_RTRCPTR;
        //pRange->pfnWriteCallbackRC  = NIL_RTRCPTR;
        //pRange->pfnFillCallbackRC   = NIL_RTRCPTR;

        pRange->fFlags              = fFlags;

        pRange->pvUserR3            = pvUser;
        pRange->pDevInsR3           = pDevIns;
        pRange->pfnReadCallbackR3   = pfnReadCallback;
        pRange->pfnWriteCallbackR3  = pfnWriteCallback;
        pRange->pfnFillCallbackR3   = pfnFillCallback;

        /*
         * Try register it with PGM and then insert it into the tree.
         */
        rc = PGMR3PhysMMIORegister(pVM, GCPhysStart, cbRange, pVM->iom.s.hMmioHandlerType,
                                   pRange, MMHyperR3ToR0(pVM, pRange), MMHyperR3ToRC(pVM, pRange), pszDesc);
        if (RT_SUCCESS(rc))
        {
            IOM_LOCK_EXCL(pVM);
            if (RTAvlroGCPhysInsert(&pVM->iom.s.pTreesR3->MMIOTree, &pRange->Core))
            {
                iomR3FlushCache(pVM);
                IOM_UNLOCK_EXCL(pVM);
                return VINF_SUCCESS;
            }

            /* bail out */
            IOM_UNLOCK_EXCL(pVM);
            DBGFR3Info(pVM->pUVM, "mmio", NULL, NULL);
            AssertMsgFailed(("This cannot happen!\n"));
            rc = VERR_IOM_IOPORT_IPE_3;
        }

        MMHyperFree(pVM, pRange);
    }
    if (pDevIns->iInstance > 0)
        MMR3HeapFree((void *)pszDesc);
    return rc;
}


#if 0
/**
 * Registers a Memory Mapped I/O RC handler range.
 *
 * This API is called by PDM on behalf of a device. Devices must first register ring-3 ranges
 * using IOMMMIORegisterR3() before calling this function.
 *
 *
 * @returns VBox status code.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pDevIns             PDM device instance owning the MMIO range.
 * @param   GCPhysStart         First physical address in the range.
 * @param   cbRange             The size of the range (in bytes).
 * @param   pvUser              User argument for the callbacks.
 * @param   pfnWriteCallback    Pointer to function which is gonna handle Write operations.
 * @param   pfnReadCallback     Pointer to function which is gonna handle Read operations.
 * @param   pfnFillCallback     Pointer to function which is gonna handle Fill/memset operations.
 * @thread  EMT
 */
VMMR3_INT_DECL(int)
IOMR3MmioRegisterRC(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTGCPTR pvUser,
                    RCPTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallback, RCPTRTYPE(PFNIOMMMIOREAD) pfnReadCallback,
                    RCPTRTYPE(PFNIOMMMIOFILL) pfnFillCallback)
{
    LogFlow(("IOMR3MmioRegisterRC: pDevIns=%p GCPhysStart=%RGp cbRange=%RGp pvUser=%RGv pfnWriteCallback=%#x pfnReadCallback=%#x pfnFillCallback=%#x\n",
             pDevIns, GCPhysStart, cbRange, pvUser, pfnWriteCallback, pfnReadCallback, pfnFillCallback));
    AssertReturn(VM_IS_RAW_MODE_ENABLED(pVM), VERR_IOM_HM_IPE);

    /*
     * Validate input.
     */
    if (!pfnWriteCallback && !pfnReadCallback)
    {
        AssertMsgFailed(("No callbacks! %RGp LB %RGp\n", GCPhysStart, cbRange));
        return VERR_INVALID_PARAMETER;
    }
    PVMCPU pVCpu = VMMGetCpu(pVM); Assert(pVCpu);

    /*
     * Find the MMIO range and check that the input matches.
     */
    IOM_LOCK_EXCL(pVM);
    PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhysStart);
    AssertReturnStmt(pRange, IOM_UNLOCK_EXCL(pVM), VERR_IOM_MMIO_RANGE_NOT_FOUND);
    AssertReturnStmt(pRange->pDevInsR3 == pDevIns, IOM_UNLOCK_EXCL(pVM), VERR_IOM_NOT_MMIO_RANGE_OWNER);
    AssertReturnStmt(pRange->GCPhys == GCPhysStart, IOM_UNLOCK_EXCL(pVM), VERR_IOM_INVALID_MMIO_RANGE);
    AssertReturnStmt(pRange->cb == cbRange, IOM_UNLOCK_EXCL(pVM), VERR_IOM_INVALID_MMIO_RANGE);

    pRange->pvUserRC          = pvUser;
    pRange->pfnReadCallbackRC = pfnReadCallback;
    pRange->pfnWriteCallbackRC= pfnWriteCallback;
    pRange->pfnFillCallbackRC = pfnFillCallback;
    pRange->pDevInsRC         = pDevIns->pDevInsForRC;
    IOM_UNLOCK_EXCL(pVM);

    return VINF_SUCCESS;
}
#endif


/**
 * Registers a Memory Mapped I/O R0 handler range.
 *
 * This API is called by PDM on behalf of a device. Devices must first register ring-3 ranges
 * using IOMMR3MIORegisterHC() before calling this function.
 *
 *
 * @returns VBox status code.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pDevIns             PDM device instance owning the MMIO range.
 * @param   GCPhysStart         First physical address in the range.
 * @param   cbRange             The size of the range (in bytes).
 * @param   pvUser              User argument for the callbacks.
 * @param   pfnWriteCallback    Pointer to function which is gonna handle Write operations.
 * @param   pfnReadCallback     Pointer to function which is gonna handle Read operations.
 * @param   pfnFillCallback     Pointer to function which is gonna handle Fill/memset operations.
 * @thread  EMT
 */
VMMR3_INT_DECL(int)
IOMR3MmioRegisterR0(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange, RTR0PTR pvUser,
                    R0PTRTYPE(PFNIOMMMIOWRITE) pfnWriteCallback,
                    R0PTRTYPE(PFNIOMMMIOREAD) pfnReadCallback,
                    R0PTRTYPE(PFNIOMMMIOFILL) pfnFillCallback)
{
    LogFlow(("IOMR3MmioRegisterR0: pDevIns=%p GCPhysStart=%RGp cbRange=%RGp pvUser=%RHv pfnWriteCallback=%#x pfnReadCallback=%#x pfnFillCallback=%#x\n",
             pDevIns, GCPhysStart, cbRange, pvUser, pfnWriteCallback, pfnReadCallback, pfnFillCallback));

    /*
     * Validate input.
     */
    if (!pfnWriteCallback && !pfnReadCallback)
    {
        AssertMsgFailed(("No callbacks! %RGp LB %RGp\n", GCPhysStart, cbRange));
        return VERR_INVALID_PARAMETER;
    }
    PVMCPU pVCpu = VMMGetCpu(pVM); Assert(pVCpu);

    /*
     * Find the MMIO range and check that the input matches.
     */
    IOM_LOCK_EXCL(pVM);
    PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhysStart);
    AssertReturnStmt(pRange, IOM_UNLOCK_EXCL(pVM), VERR_IOM_MMIO_RANGE_NOT_FOUND);
    AssertReturnStmt(pRange->pDevInsR3 == pDevIns, IOM_UNLOCK_EXCL(pVM), VERR_IOM_NOT_MMIO_RANGE_OWNER);
    AssertReturnStmt(pRange->GCPhys == GCPhysStart, IOM_UNLOCK_EXCL(pVM), VERR_IOM_INVALID_MMIO_RANGE);
    AssertReturnStmt(pRange->cb == cbRange, IOM_UNLOCK_EXCL(pVM), VERR_IOM_INVALID_MMIO_RANGE);

    pRange->pvUserR0          = pvUser;
    pRange->pfnReadCallbackR0 = pfnReadCallback;
    pRange->pfnWriteCallbackR0= pfnWriteCallback;
    pRange->pfnFillCallbackR0 = pfnFillCallback;
    pRange->pDevInsR0         = pDevIns->pDevInsR0RemoveMe;
    IOM_UNLOCK_EXCL(pVM);

    return VINF_SUCCESS;
}


/**
 * Deregisters a Memory Mapped I/O handler range.
 *
 * Registered GC, R0, and R3 ranges are affected.
 *
 * @returns VBox status code.
 *
 * @param   pVM                 The cross context VM structure.
 * @param   pDevIns             Device instance which the MMIO region is registered.
 * @param   GCPhysStart         First physical address (GC) in the range.
 * @param   cbRange             Number of bytes to deregister.
 *
 * @remark  This function mainly for PCI PnP Config and will not do
 *          all the checks you might expect it to do.
 */
VMMR3_INT_DECL(int) IOMR3MmioDeregister(PVM pVM, PPDMDEVINS pDevIns, RTGCPHYS GCPhysStart, RTGCPHYS cbRange)
{
    LogFlow(("IOMR3MmioDeregister: pDevIns=%p GCPhysStart=%RGp cbRange=%RGp\n", pDevIns, GCPhysStart, cbRange));

    /*
     * Validate input.
     */
    RTGCPHYS GCPhysLast = GCPhysStart + (cbRange - 1);
    if (GCPhysLast < GCPhysStart)
    {
        AssertMsgFailed(("Wrapped! %#x LB %RGp\n", GCPhysStart, cbRange));
        return VERR_IOM_INVALID_MMIO_RANGE;
    }
    PVMCPU pVCpu = VMMGetCpu(pVM); Assert(pVCpu);

    IOM_LOCK_EXCL(pVM);

    /*
     * Check ownership and such for the entire area.
     */
    RTGCPHYS GCPhys = GCPhysStart;
    while (GCPhys <= GCPhysLast && GCPhys >= GCPhysStart)
    {
        PIOMMMIORANGE pRange = iomMmioGetRange(pVM, pVCpu, GCPhys);
        if (!pRange)
        {
            IOM_UNLOCK_EXCL(pVM);
            return VERR_IOM_MMIO_RANGE_NOT_FOUND;
        }
        AssertMsgReturnStmt(pRange->pDevInsR3 == pDevIns,
                            ("Not owner! GCPhys=%RGp %RGp LB %RGp %s\n", GCPhys, GCPhysStart, cbRange, pRange->pszDesc),
                            IOM_UNLOCK_EXCL(pVM),
                            VERR_IOM_NOT_MMIO_RANGE_OWNER);
        AssertMsgReturnStmt(pRange->Core.KeyLast <= GCPhysLast,
                            ("Incomplete R3 range! GCPhys=%RGp %RGp LB %RGp %s\n", GCPhys, GCPhysStart, cbRange, pRange->pszDesc),
                            IOM_UNLOCK_EXCL(pVM),
                            VERR_IOM_INCOMPLETE_MMIO_RANGE);

        /* next */
        Assert(GCPhys <= pRange->Core.KeyLast);
        GCPhys = pRange->Core.KeyLast + 1;
    }

    /*
     * Do the actual removing of the MMIO ranges.
     */
    GCPhys = GCPhysStart;
    while (GCPhys <= GCPhysLast && GCPhys >= GCPhysStart)
    {
        iomR3FlushCache(pVM);

        PIOMMMIORANGE pRange = (PIOMMMIORANGE)RTAvlroGCPhysRemove(&pVM->iom.s.pTreesR3->MMIOTree, GCPhys);
        Assert(pRange);
        Assert(pRange->Core.Key == GCPhys && pRange->Core.KeyLast <= GCPhysLast);
        IOM_UNLOCK_EXCL(pVM); /* Lock order fun. */

        /* remove it from PGM */
        int rc = PGMR3PhysMMIODeregister(pVM, GCPhys, pRange->cb);
        AssertRC(rc);

        IOM_LOCK_EXCL(pVM);

        /* advance and free. */
        GCPhys = pRange->Core.KeyLast + 1;
        if (pDevIns->iInstance > 0)
        {
            void *pvDesc = ASMAtomicXchgPtr((void * volatile *)&pRange->pszDesc, NULL);
            MMR3HeapFree(pvDesc);
        }
        iomMmioReleaseRange(pVM, pRange);
    }

    IOM_UNLOCK_EXCL(pVM);
    return VINF_SUCCESS;
}


/**
 * Notfication from PGM that the pre-registered MMIO region has been mapped into
 * user address space.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the cross context VM structure.
 * @param   pvUser          The pvUserR3 argument of PGMR3PhysMMIOExPreRegister.
 * @param   GCPhys          The mapping address.
 * @remarks Called while owning the PGM lock.
 */
VMMR3_INT_DECL(int) IOMR3MmioExNotifyMapped(PVM pVM, void *pvUser, RTGCPHYS GCPhys)
{
    PIOMMMIORANGE pRange = (PIOMMMIORANGE)pvUser;
    AssertReturn(pRange->GCPhys == NIL_RTGCPHYS, VERR_IOM_MMIO_IPE_1);

    IOM_LOCK_EXCL(pVM);
    Assert(pRange->GCPhys == NIL_RTGCPHYS);
    pRange->GCPhys       = GCPhys;
    pRange->Core.Key     = GCPhys;
    pRange->Core.KeyLast = GCPhys + pRange->cb - 1;
    if (RTAvlroGCPhysInsert(&pVM->iom.s.pTreesR3->MMIOTree, &pRange->Core))
    {
        iomR3FlushCache(pVM);
        IOM_UNLOCK_EXCL(pVM);
        return VINF_SUCCESS;
    }
    IOM_UNLOCK_EXCL(pVM);

    AssertLogRelMsgFailed(("RTAvlroGCPhysInsert failed on %RGp..%RGp - %s\n", pRange->Core.Key, pRange->Core.KeyLast, pRange->pszDesc));
    pRange->GCPhys       = NIL_RTGCPHYS;
    pRange->Core.Key     = NIL_RTGCPHYS;
    pRange->Core.KeyLast = NIL_RTGCPHYS;
    return VERR_IOM_MMIO_IPE_2;
}


/**
 * Notfication from PGM that the pre-registered MMIO region has been unmapped
 * from user address space.
 *
 * @param   pVM             Pointer to the cross context VM structure.
 * @param   pvUser          The pvUserR3 argument of PGMR3PhysMMIOExPreRegister.
 * @param   GCPhys          The mapping address.
 * @remarks Called while owning the PGM lock.
 */
VMMR3_INT_DECL(void) IOMR3MmioExNotifyUnmapped(PVM pVM, void *pvUser, RTGCPHYS GCPhys)
{
    PIOMMMIORANGE pRange = (PIOMMMIORANGE)pvUser;
    AssertLogRelReturnVoid(pRange->GCPhys == GCPhys);

    IOM_LOCK_EXCL(pVM);
    Assert(pRange->GCPhys == GCPhys);
    PIOMMMIORANGE pRemoved = (PIOMMMIORANGE)RTAvlroGCPhysRemove(&pVM->iom.s.pTreesR3->MMIOTree, GCPhys);
    if (pRemoved == pRange)
    {
        pRange->GCPhys       = NIL_RTGCPHYS;
        pRange->Core.Key     = NIL_RTGCPHYS;
        pRange->Core.KeyLast = NIL_RTGCPHYS;
        iomR3FlushCache(pVM);
        IOM_UNLOCK_EXCL(pVM);
    }
    else
    {
        if (pRemoved)
            RTAvlroGCPhysInsert(&pVM->iom.s.pTreesR3->MMIOTree, &pRemoved->Core);
        IOM_UNLOCK_EXCL(pVM);
        AssertLogRelMsgFailed(("RTAvlroGCPhysRemove returned %p instead of %p for %RGp (%s)\n",
                               pRemoved, pRange, GCPhys, pRange->pszDesc));
    }
}


/**
 * Notfication from PGM that the pre-registered MMIO region has been mapped into
 * user address space.
 *
 * @param   pVM             Pointer to the cross context VM structure.
 * @param   pvUser          The pvUserR3 argument of PGMR3PhysMMIOExPreRegister.
 * @remarks Called while owning the PGM lock.
 */
VMMR3_INT_DECL(void) IOMR3MmioExNotifyDeregistered(PVM pVM, void *pvUser)
{
    PIOMMMIORANGE pRange = (PIOMMMIORANGE)pvUser;
    AssertLogRelReturnVoid(pRange->GCPhys == NIL_RTGCPHYS);
    iomMmioReleaseRange(pVM, pRange);
}


/**
 * Handles the unlikely and probably fatal merge cases.
 *
 * @returns Merged status code.
 * @param   rcStrict        Current EM status code.
 * @param   rcStrictCommit  The IOM I/O or MMIO write commit status to merge
 *                          with @a rcStrict.
 * @param   rcIom           For logging purposes only.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling EMT.  For logging purposes.
 */
DECL_NO_INLINE(static, VBOXSTRICTRC) iomR3MergeStatusSlow(VBOXSTRICTRC rcStrict, VBOXSTRICTRC rcStrictCommit,
                                                          int rcIom, PVMCPU pVCpu)
{
    if (RT_FAILURE_NP(rcStrict))
        return rcStrict;

    if (RT_FAILURE_NP(rcStrictCommit))
        return rcStrictCommit;

    if (rcStrict == rcStrictCommit)
        return rcStrictCommit;

    AssertLogRelMsgFailed(("rcStrictCommit=%Rrc rcStrict=%Rrc IOPort={%#06x<-%#xx/%u} MMIO={%RGp<-%.*Rhxs} (rcIom=%Rrc)\n",
                           VBOXSTRICTRC_VAL(rcStrictCommit), VBOXSTRICTRC_VAL(rcStrict),
                           pVCpu->iom.s.PendingIOPortWrite.IOPort,
                           pVCpu->iom.s.PendingIOPortWrite.u32Value, pVCpu->iom.s.PendingIOPortWrite.cbValue,
                           pVCpu->iom.s.PendingMmioWrite.GCPhys,
                           pVCpu->iom.s.PendingMmioWrite.cbValue, &pVCpu->iom.s.PendingMmioWrite.abValue[0], rcIom));
    return VERR_IOM_FF_STATUS_IPE;
}


/**
 * Helper for IOMR3ProcessForceFlag.
 *
 * @returns Merged status code.
 * @param   rcStrict        Current EM status code.
 * @param   rcStrictCommit  The IOM I/O or MMIO write commit status to merge
 *                          with @a rcStrict.
 * @param   rcIom           Either VINF_IOM_R3_IOPORT_COMMIT_WRITE or
 *                          VINF_IOM_R3_MMIO_COMMIT_WRITE.
 * @param   pVCpu           The cross context virtual CPU structure of the
 *                          calling EMT.
 */
DECLINLINE(VBOXSTRICTRC) iomR3MergeStatus(VBOXSTRICTRC rcStrict, VBOXSTRICTRC rcStrictCommit, int rcIom, PVMCPU pVCpu)
{
    /* Simple. */
    if (RT_LIKELY(rcStrict == rcIom || rcStrict == VINF_EM_RAW_TO_R3 || rcStrict == VINF_SUCCESS))
        return rcStrictCommit;

    if (RT_LIKELY(rcStrictCommit == VINF_SUCCESS))
        return rcStrict;

    /* EM scheduling status codes. */
    if (RT_LIKELY(   rcStrict >= VINF_EM_FIRST
                  && rcStrict <= VINF_EM_LAST))
    {
        if (RT_LIKELY(   rcStrictCommit >= VINF_EM_FIRST
                      && rcStrictCommit <= VINF_EM_LAST))
            return rcStrict < rcStrictCommit ? rcStrict : rcStrictCommit;
    }

    /* Unlikely */
    return iomR3MergeStatusSlow(rcStrict, rcStrictCommit, rcIom, pVCpu);
}


/**
 * Called by force-flag handling code when VMCPU_FF_IOM is set.
 *
 * @returns Merge between @a rcStrict and what the commit operation returned.
 * @param   pVM         The cross context VM structure.
 * @param   pVCpu       The cross context virtual CPU structure of the calling EMT.
 * @param   rcStrict    The status code returned by ring-0 or raw-mode.
 * @thread  EMT(pVCpu)
 *
 * @remarks The VMCPU_FF_IOM flag is handled before the status codes by EM, so
 *          we're very likely to see @a rcStrict set to
 *          VINF_IOM_R3_IOPORT_COMMIT_WRITE and VINF_IOM_R3_MMIO_COMMIT_WRITE
 *          here.
 */
VMMR3_INT_DECL(VBOXSTRICTRC) IOMR3ProcessForceFlag(PVM pVM, PVMCPU pVCpu, VBOXSTRICTRC rcStrict)
{
    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_IOM);
    Assert(pVCpu->iom.s.PendingIOPortWrite.cbValue || pVCpu->iom.s.PendingMmioWrite.cbValue);

    if (pVCpu->iom.s.PendingIOPortWrite.cbValue)
    {
        Log5(("IOM: Dispatching pending I/O port write: %#x LB %u -> %RTiop\n", pVCpu->iom.s.PendingIOPortWrite.u32Value,
              pVCpu->iom.s.PendingIOPortWrite.cbValue, pVCpu->iom.s.PendingIOPortWrite.IOPort));
        STAM_COUNTER_INC(&pVM->iom.s.StatIoPortCommits);
        VBOXSTRICTRC rcStrictCommit = IOMIOPortWrite(pVM, pVCpu, pVCpu->iom.s.PendingIOPortWrite.IOPort,
                                                     pVCpu->iom.s.PendingIOPortWrite.u32Value,
                                                     pVCpu->iom.s.PendingIOPortWrite.cbValue);
        pVCpu->iom.s.PendingIOPortWrite.cbValue = 0;
        rcStrict = iomR3MergeStatus(rcStrict, rcStrictCommit, VINF_IOM_R3_IOPORT_COMMIT_WRITE, pVCpu);
    }


    if (pVCpu->iom.s.PendingMmioWrite.cbValue)
    {
        Log5(("IOM: Dispatching pending MMIO write: %RGp LB %#x\n",
              pVCpu->iom.s.PendingMmioWrite.GCPhys, pVCpu->iom.s.PendingMmioWrite.cbValue));

        /* Use new MMIO handle hint and bypass PGM if it still looks right. */
        size_t idxMmioRegionHint = pVCpu->iom.s.PendingMmioWrite.idxMmioRegionHint;
        if (idxMmioRegionHint < pVM->iom.s.cMmioRegs)
        {
            PIOMMMIOENTRYR3 pRegEntry    = &pVM->iom.s.paMmioRegs[idxMmioRegionHint];
            RTGCPHYS const GCPhysMapping = pRegEntry->GCPhysMapping;
            RTGCPHYS const offRegion     = pVCpu->iom.s.PendingMmioWrite.GCPhys - GCPhysMapping;
            if (offRegion < pRegEntry->cbRegion && GCPhysMapping != NIL_RTGCPHYS)
            {
                STAM_COUNTER_INC(&pVM->iom.s.StatMmioCommitsDirect);
                VBOXSTRICTRC rcStrictCommit = iomR3MmioCommitWorker(pVM, pVCpu, pRegEntry, offRegion);
                pVCpu->iom.s.PendingMmioWrite.cbValue = 0;
                return iomR3MergeStatus(rcStrict, rcStrictCommit, VINF_IOM_R3_MMIO_COMMIT_WRITE, pVCpu);
            }
        }

        /* Fall back on PGM. */
        STAM_COUNTER_INC(&pVM->iom.s.StatMmioCommitsPgm);
        VBOXSTRICTRC rcStrictCommit = PGMPhysWrite(pVM, pVCpu->iom.s.PendingMmioWrite.GCPhys,
                                                   pVCpu->iom.s.PendingMmioWrite.abValue, pVCpu->iom.s.PendingMmioWrite.cbValue,
                                                   PGMACCESSORIGIN_IOM);
        pVCpu->iom.s.PendingMmioWrite.cbValue = 0;
        rcStrict = iomR3MergeStatus(rcStrict, rcStrictCommit, VINF_IOM_R3_MMIO_COMMIT_WRITE, pVCpu);
    }

    return rcStrict;
}


/**
 * Notification from DBGF that the number of active I/O port or MMIO
 * breakpoints has change.
 *
 * For performance reasons, IOM will only call DBGF before doing I/O and MMIO
 * accesses where there are armed breakpoints.
 *
 * @param   pVM         The cross context VM structure.
 * @param   fPortIo     True if there are armed I/O port breakpoints.
 * @param   fMmio       True if there are armed MMIO breakpoints.
 */
VMMR3_INT_DECL(void) IOMR3NotifyBreakpointCountChange(PVM pVM, bool fPortIo, bool fMmio)
{
    /** @todo I/O breakpoints. */
    RT_NOREF3(pVM, fPortIo, fMmio);
}


/**
 * Notification from DBGF that an event has been enabled or disabled.
 *
 * For performance reasons, IOM may cache the state of events it implements.
 *
 * @param   pVM         The cross context VM structure.
 * @param   enmEvent    The event.
 * @param   fEnabled    The new state.
 */
VMMR3_INT_DECL(void) IOMR3NotifyDebugEventChange(PVM pVM, DBGFEVENT enmEvent, bool fEnabled)
{
    /** @todo IOM debug events. */
    RT_NOREF3(pVM, enmEvent, fEnabled);
}


#ifdef VBOX_WITH_STATISTICS
/**
 * Tries to come up with the standard name for a port.
 *
 * @returns Pointer to readonly string if known.
 * @returns NULL if unknown port number.
 *
 * @param   Port    The port to name.
 */
static const char *iomR3IOPortGetStandardName(RTIOPORT Port)
{
    switch (Port)
    {
        case 0x00: case 0x10: case 0x20: case 0x30: case 0x40: case 0x50:            case 0x70:
        case 0x01: case 0x11: case 0x21: case 0x31: case 0x41: case 0x51: case 0x61: case 0x71:
        case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52: case 0x62: case 0x72:
        case 0x03: case 0x13: case 0x23: case 0x33: case 0x43: case 0x53: case 0x63: case 0x73:
        case 0x04: case 0x14: case 0x24: case 0x34: case 0x44: case 0x54:            case 0x74:
        case 0x05: case 0x15: case 0x25: case 0x35: case 0x45: case 0x55: case 0x65: case 0x75:
        case 0x06: case 0x16: case 0x26: case 0x36: case 0x46: case 0x56: case 0x66: case 0x76:
        case 0x07: case 0x17: case 0x27: case 0x37: case 0x47: case 0x57: case 0x67: case 0x77:
        case 0x08: case 0x18: case 0x28: case 0x38: case 0x48: case 0x58: case 0x68: case 0x78:
        case 0x09: case 0x19: case 0x29: case 0x39: case 0x49: case 0x59: case 0x69: case 0x79:
        case 0x0a: case 0x1a: case 0x2a: case 0x3a: case 0x4a: case 0x5a: case 0x6a: case 0x7a:
        case 0x0b: case 0x1b: case 0x2b: case 0x3b: case 0x4b: case 0x5b: case 0x6b: case 0x7b:
        case 0x0c: case 0x1c: case 0x2c: case 0x3c: case 0x4c: case 0x5c: case 0x6c: case 0x7c:
        case 0x0d: case 0x1d: case 0x2d: case 0x3d: case 0x4d: case 0x5d: case 0x6d: case 0x7d:
        case 0x0e: case 0x1e: case 0x2e: case 0x3e: case 0x4e: case 0x5e: case 0x6e: case 0x7e:
        case 0x0f: case 0x1f: case 0x2f: case 0x3f: case 0x4f: case 0x5f: case 0x6f: case 0x7f:

        case 0x80: case 0x90: case 0xa0: case 0xb0: case 0xc0: case 0xd0: case 0xe0: case 0xf0:
        case 0x81: case 0x91: case 0xa1: case 0xb1: case 0xc1: case 0xd1: case 0xe1: case 0xf1:
        case 0x82: case 0x92: case 0xa2: case 0xb2: case 0xc2: case 0xd2: case 0xe2: case 0xf2:
        case 0x83: case 0x93: case 0xa3: case 0xb3: case 0xc3: case 0xd3: case 0xe3: case 0xf3:
        case 0x84: case 0x94: case 0xa4: case 0xb4: case 0xc4: case 0xd4: case 0xe4: case 0xf4:
        case 0x85: case 0x95: case 0xa5: case 0xb5: case 0xc5: case 0xd5: case 0xe5: case 0xf5:
        case 0x86: case 0x96: case 0xa6: case 0xb6: case 0xc6: case 0xd6: case 0xe6: case 0xf6:
        case 0x87: case 0x97: case 0xa7: case 0xb7: case 0xc7: case 0xd7: case 0xe7: case 0xf7:
        case 0x88: case 0x98: case 0xa8: case 0xb8: case 0xc8: case 0xd8: case 0xe8: case 0xf8:
        case 0x89: case 0x99: case 0xa9: case 0xb9: case 0xc9: case 0xd9: case 0xe9: case 0xf9:
        case 0x8a: case 0x9a: case 0xaa: case 0xba: case 0xca: case 0xda: case 0xea: case 0xfa:
        case 0x8b: case 0x9b: case 0xab: case 0xbb: case 0xcb: case 0xdb: case 0xeb: case 0xfb:
        case 0x8c: case 0x9c: case 0xac: case 0xbc: case 0xcc: case 0xdc: case 0xec: case 0xfc:
        case 0x8d: case 0x9d: case 0xad: case 0xbd: case 0xcd: case 0xdd: case 0xed: case 0xfd:
        case 0x8e: case 0x9e: case 0xae: case 0xbe: case 0xce: case 0xde: case 0xee: case 0xfe:
        case 0x8f: case 0x9f: case 0xaf: case 0xbf: case 0xcf: case 0xdf: case 0xef: case 0xff:
            return "System Reserved";

        case 0x60:
        case 0x64:
            return "Keyboard & Mouse";

        case 0x378:
        case 0x379:
        case 0x37a:
        case 0x37b:
        case 0x37c:
        case 0x37d:
        case 0x37e:
        case 0x37f:
        case 0x3bc:
        case 0x3bd:
        case 0x3be:
        case 0x3bf:
        case 0x278:
        case 0x279:
        case 0x27a:
        case 0x27b:
        case 0x27c:
        case 0x27d:
        case 0x27e:
        case 0x27f:
            return "LPT1/2/3";

        case 0x3f8:
        case 0x3f9:
        case 0x3fa:
        case 0x3fb:
        case 0x3fc:
        case 0x3fd:
        case 0x3fe:
        case 0x3ff:
            return "COM1";

        case 0x2f8:
        case 0x2f9:
        case 0x2fa:
        case 0x2fb:
        case 0x2fc:
        case 0x2fd:
        case 0x2fe:
        case 0x2ff:
            return "COM2";

        case 0x3e8:
        case 0x3e9:
        case 0x3ea:
        case 0x3eb:
        case 0x3ec:
        case 0x3ed:
        case 0x3ee:
        case 0x3ef:
            return "COM3";

        case 0x2e8:
        case 0x2e9:
        case 0x2ea:
        case 0x2eb:
        case 0x2ec:
        case 0x2ed:
        case 0x2ee:
        case 0x2ef:
            return "COM4";

        case 0x200:
        case 0x201:
        case 0x202:
        case 0x203:
        case 0x204:
        case 0x205:
        case 0x206:
        case 0x207:
            return "Joystick";

        case 0x3f0:
        case 0x3f1:
        case 0x3f2:
        case 0x3f3:
        case 0x3f4:
        case 0x3f5:
        case 0x3f6:
        case 0x3f7:
            return "Floppy";

        case 0x1f0:
        case 0x1f1:
        case 0x1f2:
        case 0x1f3:
        case 0x1f4:
        case 0x1f5:
        case 0x1f6:
        case 0x1f7:
        //case 0x3f6:
        //case 0x3f7:
            return "IDE 1st";

        case 0x170:
        case 0x171:
        case 0x172:
        case 0x173:
        case 0x174:
        case 0x175:
        case 0x176:
        case 0x177:
        case 0x376:
        case 0x377:
            return "IDE 2nd";

        case 0x1e0:
        case 0x1e1:
        case 0x1e2:
        case 0x1e3:
        case 0x1e4:
        case 0x1e5:
        case 0x1e6:
        case 0x1e7:
        case 0x3e6:
        case 0x3e7:
            return "IDE 3rd";

        case 0x160:
        case 0x161:
        case 0x162:
        case 0x163:
        case 0x164:
        case 0x165:
        case 0x166:
        case 0x167:
        case 0x366:
        case 0x367:
            return "IDE 4th";

        case 0x130: case 0x140: case 0x150:
        case 0x131: case 0x141: case 0x151:
        case 0x132: case 0x142: case 0x152:
        case 0x133: case 0x143: case 0x153:
        case 0x134: case 0x144: case 0x154:
        case 0x135: case 0x145: case 0x155:
        case 0x136: case 0x146: case 0x156:
        case 0x137: case 0x147: case 0x157:
        case 0x138: case 0x148: case 0x158:
        case 0x139: case 0x149: case 0x159:
        case 0x13a: case 0x14a: case 0x15a:
        case 0x13b: case 0x14b: case 0x15b:
        case 0x13c: case 0x14c: case 0x15c:
        case 0x13d: case 0x14d: case 0x15d:
        case 0x13e: case 0x14e: case 0x15e:
        case 0x13f: case 0x14f: case 0x15f:
        case 0x220: case 0x230:
        case 0x221: case 0x231:
        case 0x222: case 0x232:
        case 0x223: case 0x233:
        case 0x224: case 0x234:
        case 0x225: case 0x235:
        case 0x226: case 0x236:
        case 0x227: case 0x237:
        case 0x228: case 0x238:
        case 0x229: case 0x239:
        case 0x22a: case 0x23a:
        case 0x22b: case 0x23b:
        case 0x22c: case 0x23c:
        case 0x22d: case 0x23d:
        case 0x22e: case 0x23e:
        case 0x22f: case 0x23f:
        case 0x330: case 0x340: case 0x350:
        case 0x331: case 0x341: case 0x351:
        case 0x332: case 0x342: case 0x352:
        case 0x333: case 0x343: case 0x353:
        case 0x334: case 0x344: case 0x354:
        case 0x335: case 0x345: case 0x355:
        case 0x336: case 0x346: case 0x356:
        case 0x337: case 0x347: case 0x357:
        case 0x338: case 0x348: case 0x358:
        case 0x339: case 0x349: case 0x359:
        case 0x33a: case 0x34a: case 0x35a:
        case 0x33b: case 0x34b: case 0x35b:
        case 0x33c: case 0x34c: case 0x35c:
        case 0x33d: case 0x34d: case 0x35d:
        case 0x33e: case 0x34e: case 0x35e:
        case 0x33f: case 0x34f: case 0x35f:
            return "SCSI (typically)";

        case 0x320:
        case 0x321:
        case 0x322:
        case 0x323:
        case 0x324:
        case 0x325:
        case 0x326:
        case 0x327:
            return "XT HD";

        case 0x3b0:
        case 0x3b1:
        case 0x3b2:
        case 0x3b3:
        case 0x3b4:
        case 0x3b5:
        case 0x3b6:
        case 0x3b7:
        case 0x3b8:
        case 0x3b9:
        case 0x3ba:
        case 0x3bb:
            return "VGA";

        case 0x3c0: case 0x3d0:
        case 0x3c1: case 0x3d1:
        case 0x3c2: case 0x3d2:
        case 0x3c3: case 0x3d3:
        case 0x3c4: case 0x3d4:
        case 0x3c5: case 0x3d5:
        case 0x3c6: case 0x3d6:
        case 0x3c7: case 0x3d7:
        case 0x3c8: case 0x3d8:
        case 0x3c9: case 0x3d9:
        case 0x3ca: case 0x3da:
        case 0x3cb: case 0x3db:
        case 0x3cc: case 0x3dc:
        case 0x3cd: case 0x3dd:
        case 0x3ce: case 0x3de:
        case 0x3cf: case 0x3df:
            return "VGA/EGA";

        case 0x240: case 0x260: case 0x280:
        case 0x241: case 0x261: case 0x281:
        case 0x242: case 0x262: case 0x282:
        case 0x243: case 0x263: case 0x283:
        case 0x244: case 0x264: case 0x284:
        case 0x245: case 0x265: case 0x285:
        case 0x246: case 0x266: case 0x286:
        case 0x247: case 0x267: case 0x287:
        case 0x248: case 0x268: case 0x288:
        case 0x249: case 0x269: case 0x289:
        case 0x24a: case 0x26a: case 0x28a:
        case 0x24b: case 0x26b: case 0x28b:
        case 0x24c: case 0x26c: case 0x28c:
        case 0x24d: case 0x26d: case 0x28d:
        case 0x24e: case 0x26e: case 0x28e:
        case 0x24f: case 0x26f: case 0x28f:
        case 0x300:
        case 0x301:
        case 0x388:
        case 0x389:
        case 0x38a:
        case 0x38b:
            return "Sound Card (typically)";

        default:
            return NULL;
    }
}
#endif /* VBOX_WITH_STATISTICS */


/* $Id: lockvalidator.cpp 25602 2009-12-31 01:18:00Z vboxsync $ */
/** @file
 * IPRT - Lock Validator.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/lockvalidator.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/once.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/thread.h>

#include "internal/lockvalidator.h"
#include "internal/magics.h"
#include "internal/thread.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Deadlock detection stack entry.
 */
typedef struct RTLOCKVALIDATORDDENTRY
{
    /** The current record. */
    PRTLOCKVALIDATORRECUNION    pRec;
    /** The current entry number if pRec is a shared one. */
    uint32_t                    iEntry;
    /** The thread state of the thread we followed to get to pFirstSibling.
     * This is only used for validating a deadlock stack.  */
    RTTHREADSTATE               enmState;
    /** The thread we followed to get to pFirstSibling.
     * This is only used for validating a deadlock stack. */
    PRTTHREADINT                pThread;
    /** What pThread is waiting on, i.e. where we entered the circular list of
     * siblings.  This is used for validating a deadlock stack as well as
     * terminating the sibling walk. */
    PRTLOCKVALIDATORRECUNION    pFirstSibling;
} RTLOCKVALIDATORDDENTRY;


/**
 * Deadlock detection stack.
 */
typedef struct RTLOCKVALIDATORDDSTACK
{
    /** The number stack entries. */
    uint32_t                    c;
    /** The stack entries. */
    RTLOCKVALIDATORDDENTRY      a[32];
} RTLOCKVALIDATORDDSTACK;
/** Pointer to a deadlock detction stack. */
typedef RTLOCKVALIDATORDDSTACK *PRTLOCKVALIDATORDDSTACK;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Serializing object destruction and deadlock detection.
 * NS: RTLOCKVALIDATORREC and RTTHREADINT destruction.
 * EW: Deadlock detection.
 */
static RTSEMXROADS      g_hLockValidatorXRoads   = NIL_RTSEMXROADS;
/** Whether the lock validator is enabled or disabled.
 * Only applies to new locks.  */
static bool volatile    g_fLockValidatorEnabled  = true;
/** Set if the lock validator is quiet. */
#ifdef RT_STRICT
static bool volatile    g_fLockValidatorQuiet    = false;
#else
static bool volatile    g_fLockValidatorQuiet    = true;
#endif
/** Set if the lock validator may panic. */
#ifdef RT_STRICT
static bool volatile    g_fLockValidatorMayPanic = true;
#else
static bool volatile    g_fLockValidatorMayPanic = false;
#endif


/** Wrapper around ASMAtomicReadPtr. */
DECL_FORCE_INLINE(PRTLOCKVALIDATORRECUNION) rtLockValidatorReadRecUnionPtr(PRTLOCKVALIDATORRECUNION volatile *ppRec)
{
    return (PRTLOCKVALIDATORRECUNION)ASMAtomicReadPtr((void * volatile *)ppRec);
}


/** Wrapper around ASMAtomicWritePtr. */
DECL_FORCE_INLINE(void) rtLockValidatorWriteRecUnionPtr(PRTLOCKVALIDATORRECUNION volatile *ppRec, PRTLOCKVALIDATORRECUNION pRecNew)
{
    ASMAtomicWritePtr((void * volatile *)ppRec, pRecNew);
}


/** Wrapper around ASMAtomicReadPtr. */
DECL_FORCE_INLINE(PRTTHREADINT) rtLockValidatorReadThreadHandle(RTTHREAD volatile *phThread)
{
    return (PRTTHREADINT)ASMAtomicReadPtr((void * volatile *)phThread);
}


/** Wrapper around ASMAtomicUoReadPtr. */
DECL_FORCE_INLINE(PRTLOCKVALIDATORSHAREDONE) rtLockValidatorUoReadSharedOwner(PRTLOCKVALIDATORSHAREDONE volatile *ppOwner)
{
    return (PRTLOCKVALIDATORSHAREDONE)ASMAtomicUoReadPtr((void * volatile *)ppOwner);
}


/**
 * Reads a volatile thread handle field and returns the thread name.
 *
 * @returns Thread name (read only).
 * @param   phThread            The thread handle field.
 */
static const char *rtLockValidatorNameThreadHandle(RTTHREAD volatile *phThread)
{
    PRTTHREADINT pThread = rtLockValidatorReadThreadHandle(phThread);
    if (!pThread)
        return "<NIL>";
    if (!VALID_PTR(pThread))
        return "<INVALID>";
    if (pThread->u32Magic != RTTHREADINT_MAGIC)
        return "<BAD-THREAD-MAGIC>";
    return pThread->szName;
}


/**
 * Launch a simple assertion like complaint w/ panic.
 *
 * @param   RT_SRC_POS_DECL     Where from.
 * @param   pszWhat             What we're complaining about.
 * @param   ...                 Format arguments.
 */
static void rtLockValidatorComplain(RT_SRC_POS_DECL, const char *pszWhat, ...)
{
    if (!ASMAtomicUoReadBool(&g_fLockValidatorQuiet))
    {
        RTAssertMsg1Weak("RTLockValidator", iLine, pszFile, pszFunction);
        va_list va;
        va_start(va, pszWhat);
        RTAssertMsg2WeakV(pszWhat, va);
        va_end(va);
    }
    if (!ASMAtomicUoReadBool(&g_fLockValidatorQuiet))
        RTAssertPanic();
}


/**
 * Describes the lock.
 *
 * @param   pszPrefix           Message prefix.
 * @param   Rec                 The lock record we're working on.
 * @param   pszPrefix           Message suffix.
 */
static void rtLockValidatorComplainAboutLock(const char *pszPrefix, PRTLOCKVALIDATORRECUNION pRec, const char *pszSuffix)
{
    if (    VALID_PTR(pRec)
        &&  !ASMAtomicUoReadBool(&g_fLockValidatorQuiet))
    {
        switch (pRec->Core.u32Magic)
        {
            case RTLOCKVALIDATORREC_MAGIC:
                RTAssertMsg2AddWeak("%s%p %s xrec=%p own=%s nest=%u pos={%Rbn(%u) %Rfn %p}%s", pszPrefix,
                                    pRec->Excl.hLock, pRec->Excl.pszName, pRec,
                                    rtLockValidatorNameThreadHandle(&pRec->Excl.hThread), pRec->Excl.cRecursion,
                                    pRec->Excl.SrcPos.pszFile, pRec->Excl.SrcPos.uLine, pRec->Excl.SrcPos.pszFunction, pRec->Excl.SrcPos.uId,
                                    pszSuffix);
                break;

            case RTLOCKVALIDATORSHARED_MAGIC:
                RTAssertMsg2AddWeak("%s%p %s srec=%p%s", pszPrefix,
                                    pRec->Shared.hLock, pRec->Shared.pszName, pRec,
                                    pszSuffix);
                break;

            case RTLOCKVALIDATORSHAREDONE_MAGIC:
            {
                PRTLOCKVALIDATORSHARED pShared = pRec->SharedOne.pSharedRec;
                if (    VALID_PTR(pShared)
                    &&  pShared->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC)
                    RTAssertMsg2AddWeak("%s%p %s srec=%p trec=%p thr=%s nest=%u pos={%Rbn(%u) %Rfn %p}%s", pszPrefix,
                                        pShared->hLock, pShared->pszName, pShared,
                                        pRec, rtLockValidatorNameThreadHandle(&pRec->SharedOne.hThread), pRec->SharedOne.cRecursion,
                                        pRec->SharedOne.SrcPos.pszFile, pRec->SharedOne.SrcPos.uLine, pRec->SharedOne.SrcPos.pszFunction, pRec->SharedOne.SrcPos.uId,
                                        pszSuffix);
                else
                    RTAssertMsg2AddWeak("%sbad srec=%p trec=%p thr=%s nest=%u pos={%Rbn(%u) %Rfn %p}%s", pszPrefix,
                                        pShared,
                                        pRec, rtLockValidatorNameThreadHandle(&pRec->SharedOne.hThread), pRec->SharedOne.cRecursion,
                                        pRec->SharedOne.SrcPos.pszFile, pRec->SharedOne.SrcPos.uLine, pRec->SharedOne.SrcPos.pszFunction, pRec->SharedOne.SrcPos.uId,
                                        pszSuffix);
                break;
            }

            default:
                RTAssertMsg2AddWeak("%spRec=%p u32Magic=%#x (bad)%s", pszPrefix, pRec, pRec->Core.u32Magic, pszSuffix);
                break;
        }
    }
}


/**
 * Launch the initial complaint.
 *
 * @param   pszWhat             What we're complaining about.
 * @param   pSrcPos             Where we are complaining from, as it were.
 * @param   pThreadSelf         The calling thread.
 * @param   pRec                The main lock involved. Can be NULL.
 */
static void rtLockValidatorComplainFirst(const char *pszWhat, PCRTLOCKVALIDATORSRCPOS pSrcPos, PRTTHREADINT pThreadSelf, PRTLOCKVALIDATORRECUNION pRec)
{
    if (!ASMAtomicUoReadBool(&g_fLockValidatorQuiet))
    {
        RTAssertMsg1Weak("RTLockValidator", pSrcPos->uLine, pSrcPos->pszFile, pSrcPos->pszFunction);
        if (pSrcPos->uId)
            RTAssertMsg2Weak("%s  [uId=%p thrd=%s]\n", pszWhat, pSrcPos->uId, VALID_PTR(pThreadSelf) ? pThreadSelf->szName : "<NIL>");
        else
            RTAssertMsg2Weak("%s\n", pszWhat, pSrcPos->uId);
        rtLockValidatorComplainAboutLock("Lock: ", pRec, "\n");
    }
}


/**
 * Continue bitching.
 *
 * @param   pszFormat           Format string.
 * @param   ...                 Format arguments.
 */
static void rtLockValidatorComplainMore(const char *pszFormat, ...)
{
    if (!ASMAtomicUoReadBool(&g_fLockValidatorQuiet))
    {
        va_list va;
        va_start(va, pszFormat);
        RTAssertMsg2AddWeakV(pszFormat, va);
        va_end(va);
    }
}


/**
 * Raise a panic if enabled.
 */
static void rtLockValidatorComplainPanic(void)
{
    if (ASMAtomicUoReadBool(&g_fLockValidatorMayPanic))
        RTAssertPanic();
}


/**
 * Copy a source position record.
 *
 * @param   pDst                The destination.
 * @param   pSrc                The source.
 */
DECL_FORCE_INLINE(void) rtLockValidatorCopySrcPos(PRTLOCKVALIDATORSRCPOS pDst, PCRTLOCKVALIDATORSRCPOS pSrc)
{
    ASMAtomicUoWriteU32(&pDst->uLine,                           pSrc->uLine);
    ASMAtomicUoWritePtr((void * volatile *)&pDst->pszFile,      pSrc->pszFile);
    ASMAtomicUoWritePtr((void * volatile *)&pDst->pszFunction,  pSrc->pszFunction);
    ASMAtomicUoWritePtr((void * volatile *)&pDst->uId,          (void *)pSrc->uId);
}


/**
 * Init a source position record.
 *
 * @param   pSrcPos             The source position record.
 */
DECL_FORCE_INLINE(void) rtLockValidatorInitSrcPos(PRTLOCKVALIDATORSRCPOS pSrcPos)
{
    pSrcPos->pszFile        = NULL;
    pSrcPos->pszFunction    = NULL;
    pSrcPos->uId            = 0;
    pSrcPos->uLine          = 0;
#if HC_ARCH_BITS == 64
    pSrcPos->u32Padding     = 0;
#endif
}


/**
 * Serializes destruction of RTLOCKVALIDATORREC and RTTHREADINT structures.
 */
DECLHIDDEN(void) rtLockValidatorSerializeDestructEnter(void)
{
    RTSEMXROADS hXRoads = g_hLockValidatorXRoads;
    if (hXRoads != NIL_RTSEMXROADS)
        RTSemXRoadsNSEnter(hXRoads);
}


/**
 * Call after rtLockValidatorSerializeDestructEnter.
 */
DECLHIDDEN(void) rtLockValidatorSerializeDestructLeave(void)
{
    RTSEMXROADS hXRoads = g_hLockValidatorXRoads;
    if (hXRoads != NIL_RTSEMXROADS)
        RTSemXRoadsNSLeave(hXRoads);
}


/**
 * Serializes deadlock detection against destruction of the objects being
 * inspected.
 */
DECLINLINE(void) rtLockValidatorSerializeDetectionEnter(void)
{
    RTSEMXROADS hXRoads = g_hLockValidatorXRoads;
    if (hXRoads != NIL_RTSEMXROADS)
        RTSemXRoadsEWEnter(hXRoads);
}


/**
 * Call after rtLockValidatorSerializeDetectionEnter.
 */
DECLHIDDEN(void) rtLockValidatorSerializeDetectionLeave(void)
{
    RTSEMXROADS hXRoads = g_hLockValidatorXRoads;
    if (hXRoads != NIL_RTSEMXROADS)
        RTSemXRoadsEWLeave(hXRoads);
}


RTDECL(void) RTLockValidatorRecInit(PRTLOCKVALIDATORREC pRec, RTLOCKVALIDATORCLASS hClass,
                                    uint32_t uSubClass, const char *pszName, void *hLock)
{
    pRec->Core.u32Magic = RTLOCKVALIDATORREC_MAGIC;
    pRec->fEnabled      = RTLockValidatorIsEnabled();
    pRec->afReserved[0] = 0;
    pRec->afReserved[1] = 0;
    pRec->afReserved[2] = 0;
    rtLockValidatorInitSrcPos(&pRec->SrcPos);
    pRec->hThread       = NIL_RTTHREAD;
    pRec->pDown         = NULL;
    pRec->hClass        = hClass;
    pRec->uSubClass     = uSubClass;
    pRec->cRecursion    = 0;
    pRec->hLock         = hLock;
    pRec->pszName       = pszName;
    pRec->pSibling      = NULL;

    /* Lazily initialize the crossroads semaphore. */
    static uint32_t volatile s_fInitializing = false;
    if (RT_UNLIKELY(    g_hLockValidatorXRoads == NIL_RTSEMXROADS
                    &&  ASMAtomicCmpXchgU32(&s_fInitializing, true, false)))
    {
        RTSEMXROADS hXRoads;
        int rc = RTSemXRoadsCreate(&hXRoads);
        if (RT_SUCCESS(rc))
            ASMAtomicWriteHandle(&g_hLockValidatorXRoads, hXRoads);
        ASMAtomicWriteU32(&s_fInitializing, false);
    }
}


RTDECL(int)  RTLockValidatorRecCreate(PRTLOCKVALIDATORREC *ppRec, RTLOCKVALIDATORCLASS hClass,
                                      uint32_t uSubClass, const char *pszName, void *pvLock)
{
    PRTLOCKVALIDATORREC pRec;
    *ppRec = pRec = (PRTLOCKVALIDATORREC)RTMemAlloc(sizeof(*pRec));
    if (!pRec)
        return VERR_NO_MEMORY;

    RTLockValidatorRecInit(pRec, hClass, uSubClass, pszName, pvLock);

    return VINF_SUCCESS;
}


/**
 * Unlinks all siblings.
 *
 * This is used during record deletion and assumes no races.
 *
 * @param   pCore               One of the siblings.
 */
static void rtLockValidatorUnlinkAllSiblings(PRTLOCKVALIDATORRECCORE pCore)
{
    /* ASSUMES sibling destruction doesn't involve any races and that all
       related records are to be disposed off now.  */
    PRTLOCKVALIDATORRECUNION pSibling = (PRTLOCKVALIDATORRECUNION)pCore;
    while (pSibling)
    {
        PRTLOCKVALIDATORRECUNION volatile *ppCoreNext;
        switch (pSibling->Core.u32Magic)
        {
            case RTLOCKVALIDATORREC_MAGIC:
            case RTLOCKVALIDATORREC_MAGIC_DEAD:
                ppCoreNext = &pSibling->Excl.pSibling;
                break;

            case RTLOCKVALIDATORSHARED_MAGIC:
            case RTLOCKVALIDATORSHARED_MAGIC_DEAD:
                ppCoreNext = &pSibling->Shared.pSibling;
                break;

            default:
                AssertFailed();
                ppCoreNext = NULL;
                break;
        }
        if (RT_UNLIKELY(ppCoreNext))
            break;
        pSibling = (PRTLOCKVALIDATORRECUNION)ASMAtomicXchgPtr((void * volatile *)ppCoreNext, NULL);
    }
}


RTDECL(void) RTLockValidatorRecDelete(PRTLOCKVALIDATORREC pRec)
{
    Assert(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC);

    rtLockValidatorSerializeDestructEnter();

    ASMAtomicWriteU32(&pRec->Core.u32Magic, RTLOCKVALIDATORREC_MAGIC_DEAD);
    ASMAtomicWriteHandle(&pRec->hThread, NIL_RTTHREAD);
    ASMAtomicWriteHandle(&pRec->hClass, NIL_RTLOCKVALIDATORCLASS);
    if (pRec->pSibling)
        rtLockValidatorUnlinkAllSiblings(&pRec->Core);
    rtLockValidatorSerializeDestructLeave();
}


RTDECL(void) RTLockValidatorRecDestroy(PRTLOCKVALIDATORREC *ppRec)
{
    PRTLOCKVALIDATORREC pRec = *ppRec;
    *ppRec = NULL;
    if (pRec)
    {
        RTLockValidatorRecDelete(pRec);
        RTMemFree(pRec);
    }
}


RTDECL(void) RTLockValidatorSharedRecInit(PRTLOCKVALIDATORSHARED pRec, RTLOCKVALIDATORCLASS hClass,
                                          uint32_t uSubClass, const char *pszName, void *hLock)
{
    pRec->Core.u32Magic = RTLOCKVALIDATORSHARED_MAGIC;
    pRec->uSubClass     = uSubClass;
    pRec->hClass        = hClass;
    pRec->hLock         = hLock;
    pRec->pszName       = pszName;
    pRec->fEnabled      = RTLockValidatorIsEnabled();
    pRec->pSibling      = NULL;

    /* the table */
    pRec->cEntries      = 0;
    pRec->iLastEntry    = 0;
    pRec->cAllocated    = 0;
    pRec->fReallocating = false;
    pRec->afPadding[0]  = false;
    pRec->afPadding[1]  = false;
    pRec->papOwners     = NULL;
#if HC_ARCH_BITS == 32
    pRec->u32Alignment  = UINT32_MAX;
#endif
}


RTDECL(void) RTLockValidatorSharedRecDelete(PRTLOCKVALIDATORSHARED pRec)
{
    Assert(pRec->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC);

    /*
     * Flip it into table realloc mode and take the destruction lock.
     */
    rtLockValidatorSerializeDestructEnter();
    while (!ASMAtomicCmpXchgBool(&pRec->fReallocating, true, false))
    {
        rtLockValidatorSerializeDestructLeave();

        rtLockValidatorSerializeDetectionEnter();
        rtLockValidatorSerializeDetectionLeave();

        rtLockValidatorSerializeDestructEnter();
    }

    ASMAtomicWriteU32(&pRec->Core.u32Magic, RTLOCKVALIDATORSHARED_MAGIC_DEAD);
    ASMAtomicUoWriteHandle(&pRec->hClass, NIL_RTLOCKVALIDATORCLASS);
    if (pRec->papOwners)
    {
        PRTLOCKVALIDATORSHAREDONE volatile *papOwners = pRec->papOwners;
        ASMAtomicUoWritePtr((void * volatile *)&pRec->papOwners, NULL);
        ASMAtomicUoWriteU32(&pRec->cAllocated, 0);

        RTMemFree((void *)pRec->papOwners);
    }
    if (pRec->pSibling)
        rtLockValidatorUnlinkAllSiblings(&pRec->Core);
    ASMAtomicWriteBool(&pRec->fReallocating, false);

    rtLockValidatorSerializeDestructLeave();
}


/**
 * Locates a thread in a shared lock record.
 *
 * @returns Pointer to the thread record on success, NULL on failure..
 * @param   pShared             The shared lock record.
 * @param   hThread             The thread to find.
 * @param   piEntry             Where to optionally return the table in index.
 */
DECLINLINE(PRTLOCKVALIDATORSHAREDONE)
rtLockValidatorSharedRecFindThread(PRTLOCKVALIDATORSHARED pShared, RTTHREAD hThread, uint32_t *piEntry)
{
    rtLockValidatorSerializeDetectionEnter();
    if (pShared->papOwners)
    {
        PRTLOCKVALIDATORSHAREDONE volatile *papOwners = pShared->papOwners;
        uint32_t const                      cMax      = pShared->cAllocated;
        for (uint32_t iEntry = 0; iEntry < cMax; iEntry++)
        {
            PRTLOCKVALIDATORSHAREDONE pEntry = rtLockValidatorUoReadSharedOwner(&papOwners[iEntry]);
            if (pEntry && pEntry->hThread == hThread)
            {
                rtLockValidatorSerializeDetectionLeave();
                if (piEntry)
                    *piEntry = iEntry;
                return pEntry;
            }
        }
    }
    rtLockValidatorSerializeDetectionLeave();
    return NULL;
}


/**
 * Allocates and initializes a thread entry for the shared lock record.
 *
 * @returns The new thread entry.
 * @param   pShared             The shared lock record.
 * @param   hThread             The thread handle.
 * @param   pSrcPos             The source position.
 */
DECLINLINE(PRTLOCKVALIDATORSHAREDONE)
rtLockValidatorSharedRecAllocThread(PRTLOCKVALIDATORSHARED pRead, RTTHREAD hThread, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    PRTLOCKVALIDATORSHAREDONE pEntry;

    pEntry = (PRTLOCKVALIDATORSHAREDONE)RTMemAlloc(sizeof(RTLOCKVALIDATORSHAREDONE));
    if (pEntry)
    {
        pEntry->Core.u32Magic   = RTLOCKVALIDATORSHAREDONE_MAGIC;
        pEntry->cRecursion      = 1;
        pEntry->hThread         = hThread;
        pEntry->pDown           = NULL;
        pEntry->pSharedRec      = pRead;
#if HC_ARCH_BITS == 32
        pEntry->pvReserved      = NULL;
#endif
        if (pSrcPos)
            pEntry->SrcPos      = *pSrcPos;
        else
            rtLockValidatorInitSrcPos(&pEntry->SrcPos);
    }

    return pEntry;
}

/**
 * Frees a thread entry allocated by rtLockValidatorSharedRecAllocThread.
 *
 * @param   pEntry              The thread entry.
 */
DECLINLINE(void) rtLockValidatorSharedRecFreeThread(PRTLOCKVALIDATORSHAREDONE pEntry)
{
    if (pEntry)
    {
        rtLockValidatorSerializeDestructEnter();
        ASMAtomicWriteU32(&pEntry->Core.u32Magic, RTLOCKVALIDATORSHAREDONE_MAGIC_DEAD);
        ASMAtomicWriteHandle(&pEntry->hThread, NIL_RTTHREAD);
        rtLockValidatorSerializeDestructLeave();

        RTMemFree(pEntry);
    }
}


/**
 * Make more room in the table.
 *
 * @retval  true on success
 * @retval  false if we're out of memory or running into a bad race condition
 *          (probably a bug somewhere).  No longer holding the lock.
 *
 * @param   pShared             The shared lock record.
 */
static bool rtLockValidatorSharedRecMakeRoom(PRTLOCKVALIDATORSHARED pShared)
{
    for (unsigned i = 0; i < 1000; i++)
    {
        /*
         * Switch to the other data access direction.
         */
        rtLockValidatorSerializeDetectionLeave();
        if (i >= 10)
        {
            Assert(i != 10 && i != 100);
            RTThreadSleep(i >= 100);
        }
        rtLockValidatorSerializeDestructEnter();

        /*
         * Try grab the privilege to reallocating the table.
         */
        if (    pShared->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC
            &&  ASMAtomicCmpXchgBool(&pShared->fReallocating, true, false))
        {
            uint32_t cAllocated = pShared->cAllocated;
            if (cAllocated < pShared->cEntries)
            {
                /*
                 * Ok, still not enough space.  Reallocate the table.
                 */
#if 0  /** @todo enable this after making sure growing works flawlessly. */
                uint32_t                    cInc = RT_ALIGN_32(pShared->cEntries - cAllocated, 16);
#else
                uint32_t                    cInc = RT_ALIGN_32(pShared->cEntries - cAllocated, 1);
#endif
                PRTLOCKVALIDATORSHAREDONE  *papOwners;
                papOwners = (PRTLOCKVALIDATORSHAREDONE *)RTMemRealloc((void *)pShared->papOwners,
                                                                      (cAllocated + cInc) * sizeof(void *));
                if (!papOwners)
                {
                    ASMAtomicWriteBool(&pShared->fReallocating, false);
                    rtLockValidatorSerializeDestructLeave();
                    /* RTMemRealloc will assert */
                    return false;
                }

                while (cInc-- > 0)
                {
                    papOwners[cAllocated] = NULL;
                    cAllocated++;
                }

                ASMAtomicWritePtr((void * volatile *)&pShared->papOwners, papOwners);
                ASMAtomicWriteU32(&pShared->cAllocated, cAllocated);
            }
            ASMAtomicWriteBool(&pShared->fReallocating, false);
        }
        rtLockValidatorSerializeDestructLeave();

        rtLockValidatorSerializeDetectionEnter();
        if (RT_UNLIKELY(pShared->Core.u32Magic != RTLOCKVALIDATORSHARED_MAGIC))
            break;

        if (pShared->cAllocated >= pShared->cEntries)
            return true;
    }

    rtLockValidatorSerializeDetectionLeave();
    AssertFailed(); /* too many iterations or destroyed while racing. */
    return false;
}


/**
 * Adds a thread entry to a shared lock record.
 *
 * @returns true on success, false on serious race or we're if out of memory.
 * @param   pShared             The shared lock record.
 * @param   pEntry              The thread entry.
 */
DECLINLINE(bool) rtLockValidatorSharedRecAddThread(PRTLOCKVALIDATORSHARED pShared, PRTLOCKVALIDATORSHAREDONE pEntry)
{
    rtLockValidatorSerializeDetectionEnter();
    if (RT_LIKELY(pShared->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC)) /* paranoia */
    {
        if (   ASMAtomicIncU32(&pShared->cEntries) > pShared->cAllocated /** @todo add fudge */
            && !rtLockValidatorSharedRecMakeRoom(pShared))
            return false; /* the worker leave the lock */

        PRTLOCKVALIDATORSHAREDONE volatile *papOwners = pShared->papOwners;
        uint32_t const                      cMax      = pShared->cAllocated;
        for (unsigned i = 0; i < 100; i++)
        {
            for (uint32_t iEntry = 0; iEntry < cMax; iEntry++)
            {
                if (ASMAtomicCmpXchgPtr((void * volatile *)&papOwners[iEntry], pEntry, NULL))
                {
                    rtLockValidatorSerializeDetectionLeave();
                    return true;
                }
            }
            Assert(i != 25);
        }
        AssertFailed();
    }
    rtLockValidatorSerializeDetectionLeave();
    return false;
}


/**
 * Remove a thread entry from a shared lock record and free it.
 *
 * @param   pShared             The shared lock record.
 * @param   pEntry              The thread entry to remove.
 * @param   iEntry              The last known index.
 */
DECLINLINE(void) rtLockValidatorSharedRecRemoveAndFree(PRTLOCKVALIDATORSHARED pShared, PRTLOCKVALIDATORSHAREDONE pEntry,
                                                       uint32_t iEntry)
{
    /*
     * Remove it from the table.
     */
    rtLockValidatorSerializeDetectionEnter();
    AssertReturnVoidStmt(pShared->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC, rtLockValidatorSerializeDetectionLeave());
    if (RT_UNLIKELY(   iEntry >= pShared->cAllocated
                    || !ASMAtomicCmpXchgPtr((void * volatile *)&pShared->papOwners[iEntry], NULL, pEntry)))
    {
        /* this shouldn't happen yet... */
        AssertFailed();
        PRTLOCKVALIDATORSHAREDONE volatile *papOwners = pShared->papOwners;
        uint32_t const                      cMax      = pShared->cAllocated;
        for (iEntry = 0; iEntry < cMax; iEntry++)
            if (ASMAtomicCmpXchgPtr((void * volatile *)&papOwners[iEntry], NULL, pEntry))
               break;
        AssertReturnVoidStmt(iEntry < cMax, rtLockValidatorSerializeDetectionLeave());
    }
    uint32_t cNow = ASMAtomicDecU32(&pShared->cEntries);
    Assert(!(cNow & RT_BIT_32(31))); NOREF(cNow);
    rtLockValidatorSerializeDetectionLeave();

    /*
     * Successfully removed, now free it.
     */
    rtLockValidatorSharedRecFreeThread(pEntry);
}


RTDECL(int) RTLockValidatorMakeSiblings(PRTLOCKVALIDATORRECCORE pRec1, PRTLOCKVALIDATORRECCORE pRec2)
{
    /*
     * Validate input.
     */
    PRTLOCKVALIDATORRECUNION p1 = (PRTLOCKVALIDATORRECUNION)pRec1;
    PRTLOCKVALIDATORRECUNION p2 = (PRTLOCKVALIDATORRECUNION)pRec2;

    AssertPtrReturn(p1, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(   p1->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC
                 || p1->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC
                 , VERR_SEM_LV_INVALID_PARAMETER);

    AssertPtrReturn(p2, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(   p2->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC
                 || p2->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC
                 , VERR_SEM_LV_INVALID_PARAMETER);

    /*
     * Link them.
     */
    if (    p1->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC
        &&  p2->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC)
    {
        p1->Excl.pSibling   = p2;
        p2->Shared.pSibling = p1;
    }
    else if (   p1->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC
             && p2->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC)
    {
        p1->Shared.pSibling = p2;
        p2->Excl.pSibling   = p1;
    }
    else
        AssertFailedReturn(VERR_SEM_LV_INVALID_PARAMETER); /* unsupported mix */

    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorCheckOrder(PRTLOCKVALIDATORREC pRec, RTTHREAD hThread, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    AssertReturn(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRec->fEnabled)
        return VINF_SUCCESS;

    /*
     * Check it locks we're currently holding.
     */
    /** @todo later */

    /*
     * If missing order rules, add them.
     */

    return VINF_SUCCESS;
}


RTDECL(int)  RTLockValidatorCheckAndRelease(PRTLOCKVALIDATORREC pRec)
{
    AssertReturn(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRec->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(pRec->hThread != NIL_RTTHREAD, VERR_SEM_LV_INVALID_PARAMETER);

    RTLockValidatorUnsetOwner(pRec);
    return VINF_SUCCESS;
}


RTDECL(int)  RTLockValidatorCheckAndReleaseReadOwner(PRTLOCKVALIDATORSHARED pRead, RTTHREAD hThread)
{
    AssertReturn(pRead->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRead->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(hThread != NIL_RTTHREAD, VERR_SEM_LV_INVALID_PARAMETER);

    /*
     * Locate the entry for this thread in the table.
     */
    uint32_t                    iEntry = 0;
    PRTLOCKVALIDATORSHAREDONE   pEntry = rtLockValidatorSharedRecFindThread(pRead, hThread, &iEntry);
    AssertReturn(pEntry, VERR_SEM_LV_NOT_OWNER);

    /*
     * Check the release order.
     */
    if (pRead->hClass != NIL_RTLOCKVALIDATORCLASS)
    {
        /** @todo order validation */
    }

    /*
     * Release the ownership or unwind a level of recursion.
     */
    Assert(pEntry->cRecursion > 0);
    if (pEntry->cRecursion > 1)
        pEntry->cRecursion--;
    else
        rtLockValidatorSharedRecRemoveAndFree(pRead, pEntry, iEntry);

    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorRecordRecursion(PRTLOCKVALIDATORREC pRec, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    AssertReturn(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRec->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(pRec->hThread != NIL_RTTHREAD, VERR_SEM_LV_INVALID_PARAMETER);

    Assert(pRec->cRecursion < _1M);
    pRec->cRecursion++;

    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorUnwindRecursion(PRTLOCKVALIDATORREC pRec)
{
    AssertReturn(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRec->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(pRec->hThread != NIL_RTTHREAD, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRec->cRecursion > 0, VERR_SEM_LV_INVALID_PARAMETER);

    Assert(pRec->cRecursion);
    pRec->cRecursion--;
    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorRecordReadWriteRecursion(PRTLOCKVALIDATORREC pWrite, PRTLOCKVALIDATORSHARED pRead, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    AssertReturn(pWrite->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRead->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRead->fEnabled == pWrite->fEnabled, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pWrite->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(pWrite->hThread != NIL_RTTHREAD, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pWrite->cRecursion > 0, VERR_SEM_LV_INVALID_PARAMETER);

    Assert(pWrite->cRecursion < _1M);
    pWrite->cRecursion++;

    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorUnwindReadWriteRecursion(PRTLOCKVALIDATORREC pWrite, PRTLOCKVALIDATORSHARED pRead)
{
    AssertReturn(pWrite->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRead->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRead->fEnabled == pWrite->fEnabled, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pWrite->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(pWrite->hThread != NIL_RTTHREAD, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pWrite->cRecursion > 0, VERR_SEM_LV_INVALID_PARAMETER);

    Assert(pWrite->cRecursion);
    pWrite->cRecursion--;
    return VINF_SUCCESS;
}


RTDECL(RTTHREAD) RTLockValidatorSetOwner(PRTLOCKVALIDATORREC pRec, RTTHREAD hThread, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    AssertReturn(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, NIL_RTTHREAD);
    if (!pRec->fEnabled)
        return VINF_SUCCESS;
    if (hThread == NIL_RTTHREAD)
    {
        hThread = RTThreadSelfAutoAdopt();
        AssertReturn(hThread != NIL_RTTHREAD, hThread);
    }

    ASMAtomicIncS32(&hThread->LockValidator.cWriteLocks);

    if (pRec->hThread == hThread)
        pRec->cRecursion++;
    else
    {
        Assert(pRec->hThread == NIL_RTTHREAD);

        /*
         * Update the record.
         */
        rtLockValidatorCopySrcPos(&pRec->SrcPos, pSrcPos);
        ASMAtomicUoWriteU32(&pRec->cRecursion, 1);
        ASMAtomicWriteHandle(&pRec->hThread, hThread);

        /*
         * Push the lock onto the lock stack.
         */
        /** @todo push it onto the per-thread lock stack. */
    }

    return hThread;
}


RTDECL(RTTHREAD) RTLockValidatorUnsetOwner(PRTLOCKVALIDATORREC pRec)
{
    AssertReturn(pRec->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, NIL_RTTHREAD);
    if (!pRec->fEnabled)
        return VINF_SUCCESS;
    RTTHREADINT *pThread = pRec->hThread;
    AssertReturn(pThread != NIL_RTTHREAD, pThread);

    ASMAtomicDecS32(&pThread->LockValidator.cWriteLocks);

    if (ASMAtomicDecU32(&pRec->cRecursion) == 0)
    {
        /*
         * Pop (remove) the lock.
         */
        /** @todo remove it from the per-thread stack/whatever. */

        /*
         * Update the record.
         */
        ASMAtomicWriteHandle(&pRec->hThread, NIL_RTTHREAD);
    }

    return pThread;
}


RTDECL(void) RTLockValidatorAddReadOwner(PRTLOCKVALIDATORSHARED pRead, RTTHREAD hThread, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    AssertReturnVoid(pRead->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC);
    if (!pRead->fEnabled)
        return;
    AssertReturnVoid(hThread != NIL_RTTHREAD);

    /*
     * Recursive?
     *
     * Note! This code can be optimized to try avoid scanning the table on
     *       insert. However, that's annoying work that makes the code big,
     *       so it can wait til later sometime.
     */
    PRTLOCKVALIDATORSHAREDONE pEntry = rtLockValidatorSharedRecFindThread(pRead, hThread, NULL);
    if (pEntry)
    {
        pEntry->cRecursion++;
        return;
    }

    /*
     * Allocate a new thread entry and insert it into the table.
     */
    pEntry = rtLockValidatorSharedRecAllocThread(pRead, hThread, pSrcPos);
    if (    pEntry
        &&  !rtLockValidatorSharedRecAddThread(pRead, pEntry))
        rtLockValidatorSharedRecFreeThread(pEntry);
}


RTDECL(void) RTLockValidatorRemoveReadOwner(PRTLOCKVALIDATORSHARED pRead, RTTHREAD hThread)
{
    AssertReturnVoid(pRead->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC);
    if (!pRead->fEnabled)
        return;
    AssertReturnVoid(hThread != NIL_RTTHREAD);

    /*
     * Find the entry hope it's a recursive one.
     */
    uint32_t iEntry;
    PRTLOCKVALIDATORSHAREDONE pEntry = rtLockValidatorSharedRecFindThread(pRead, hThread, &iEntry);
    AssertReturnVoid(pEntry);
    if (pEntry->cRecursion > 1)
        pEntry->cRecursion--;
    else
        rtLockValidatorSharedRecRemoveAndFree(pRead, pEntry, iEntry);
}


RTDECL(int32_t) RTLockValidatorWriteLockGetCount(RTTHREAD Thread)
{
    if (Thread == NIL_RTTHREAD)
        return 0;

    PRTTHREADINT pThread = rtThreadGet(Thread);
    if (!pThread)
        return VERR_INVALID_HANDLE;
    int32_t cWriteLocks = ASMAtomicReadS32(&pThread->LockValidator.cWriteLocks);
    rtThreadRelease(pThread);
    return cWriteLocks;
}
RT_EXPORT_SYMBOL(RTLockValidatorWriteLockGetCount);


RTDECL(void) RTLockValidatorWriteLockInc(RTTHREAD Thread)
{
    PRTTHREADINT pThread = rtThreadGet(Thread);
    AssertReturnVoid(pThread);
    ASMAtomicIncS32(&pThread->LockValidator.cWriteLocks);
    rtThreadRelease(pThread);
}
RT_EXPORT_SYMBOL(RTLockValidatorWriteLockInc);


RTDECL(void) RTLockValidatorWriteLockDec(RTTHREAD Thread)
{
    PRTTHREADINT pThread = rtThreadGet(Thread);
    AssertReturnVoid(pThread);
    ASMAtomicDecS32(&pThread->LockValidator.cWriteLocks);
    rtThreadRelease(pThread);
}
RT_EXPORT_SYMBOL(RTLockValidatorWriteLockDec);


RTDECL(int32_t) RTLockValidatorReadLockGetCount(RTTHREAD Thread)
{
    if (Thread == NIL_RTTHREAD)
        return 0;

    PRTTHREADINT pThread = rtThreadGet(Thread);
    if (!pThread)
        return VERR_INVALID_HANDLE;
    int32_t cReadLocks = ASMAtomicReadS32(&pThread->LockValidator.cReadLocks);
    rtThreadRelease(pThread);
    return cReadLocks;
}
RT_EXPORT_SYMBOL(RTLockValidatorReadLockGetCount);


RTDECL(void) RTLockValidatorReadLockInc(RTTHREAD Thread)
{
    PRTTHREADINT pThread = rtThreadGet(Thread);
    Assert(pThread);
    ASMAtomicIncS32(&pThread->LockValidator.cReadLocks);
    rtThreadRelease(pThread);
}
RT_EXPORT_SYMBOL(RTLockValidatorReadLockInc);


RTDECL(void) RTLockValidatorReadLockDec(RTTHREAD Thread)
{
    PRTTHREADINT pThread = rtThreadGet(Thread);
    Assert(pThread);
    ASMAtomicDecS32(&pThread->LockValidator.cReadLocks);
    rtThreadRelease(pThread);
}
RT_EXPORT_SYMBOL(RTLockValidatorReadLockDec);


/**
 * Checks for stack cycles caused by another deadlock before returning.
 *
 * @retval  VINF_SUCCESS if the stack is simply too small.
 * @retval  VERR_SEM_LV_EXISTING_DEADLOCK if a cycle was detected.
 *
 * @param   pStack              The deadlock detection stack.
 */
static int rtLockValidatorDdHandleStackOverflow(PRTLOCKVALIDATORDDSTACK pStack)
{
    for (size_t i = 0; i < RT_ELEMENTS(pStack->a) - 1; i++)
    {
        PRTTHREADINT pThread = pStack->a[i].pThread;
        for (size_t j = i + 1; j < RT_ELEMENTS(pStack->a); j++)
            if (pStack->a[j].pThread == pThread)
                return VERR_SEM_LV_EXISTING_DEADLOCK;
    }
    static bool volatile s_fComplained = false;
    if (!s_fComplained)
    {
        s_fComplained = true;
        rtLockValidatorComplain(RT_SRC_POS, "lock validator stack is too small! (%zu entries)\n", RT_ELEMENTS(pStack->a));
    }
    return VINF_SUCCESS;
}


/**
 * Worker for rtLockValidatorDoDeadlockCheck that checks if there is more work
 * to be done during unwind.
 *
 * @returns true if there is more work left for this lock, false if not.
 * @param   pRec            The current record.
 * @param   iEntry          The current index.
 * @param   pFirstSibling   The first record we examined.
 */
DECL_FORCE_INLINE(bool) rtLockValidatorDdMoreWorkLeft(PRTLOCKVALIDATORRECUNION pRec, uint32_t iEntry, PRTLOCKVALIDATORRECUNION pFirstSibling)
{
    PRTLOCKVALIDATORRECUNION pSibling;

    switch (pRec->Core.u32Magic)
    {
        case RTLOCKVALIDATORREC_MAGIC:
            pSibling = pRec->Excl.pSibling;
            break;

        case RTLOCKVALIDATORSHARED_MAGIC:
            if (iEntry + 1 < pRec->Shared.cAllocated)
                return true;
            pSibling = pRec->Excl.pSibling;
            break;

        default:
            return false;
    }
    return pSibling != NULL
        && pSibling != pFirstSibling;
}


/**
 * Worker for rtLockValidatorDeadlockDetection that does the actual deadlock
 * detection.
 *
 * @returns Same as rtLockValidatorDeadlockDetection.
 * @param   pStack          The stack to use.
 * @param   pOriginalRec    The original record.
 * @param   pThreadSelf     The calling thread.
 */
static int rtLockValidatorDdDoDetection(PRTLOCKVALIDATORDDSTACK pStack, PRTLOCKVALIDATORRECUNION const pOriginalRec,
                                        PRTTHREADINT const pThreadSelf)
{
    pStack->c = 0;

    /* We could use a single RTLOCKVALIDATORDDENTRY variable here, but the
       compiler may make a better job of it when using individual variables. */
    PRTLOCKVALIDATORRECUNION    pRec            = pOriginalRec;
    PRTLOCKVALIDATORRECUNION    pFirstSibling   = pOriginalRec;
    uint32_t                    iEntry          = UINT32_MAX;
    PRTTHREADINT                pThread         = NIL_RTTHREAD;
    RTTHREADSTATE               enmState        = RTTHREADSTATE_RUNNING;
    for (;;)
    {
        /*
         * Process the current record.
         */
        /* Extract the (next) thread. */
        PRTTHREADINT pNextThread;
        switch (pRec->Core.u32Magic)
        {
            case RTLOCKVALIDATORREC_MAGIC:
                Assert(iEntry == UINT32_MAX);
                pNextThread = rtLockValidatorReadThreadHandle(&pRec->Excl.hThread);
                if (    pNextThread
                    &&  !RTTHREAD_IS_SLEEPING(pNextThread->enmState)
                    &&  pNextThread != pThreadSelf)
                    pNextThread = NIL_RTTHREAD;
                if (    pNextThread == NIL_RTTHREAD
                    &&  pRec->Excl.pSibling
                    &&  pRec->Excl.pSibling != pFirstSibling)
                {
                    pRec = pRec->Excl.pSibling;
                    continue;
                }
                break;

            case RTLOCKVALIDATORSHARED_MAGIC:
                /* Skip to the next sibling if same side.  ASSUMES reader priority. */
                /** @todo The read side of a read-write lock is problematic if
                 * the implementation prioritizes writers over readers because
                 * that means we should could deadlock against current readers
                 * if a writer showed up.  If the RW sem implementation is
                 * wrapping some native API, it's not so easy to detect when we
                 * should do this and when we shouldn't.  Checking when we
                 * shouldn't is subject to wakeup scheduling and cannot easily
                 * be made reliable.
                 *
                 * At the moment we circumvent all this mess by declaring that
                 * readers has priority.  This is TRUE on linux, but probably
                 * isn't on Solaris and FreeBSD. */
                if (   pRec == pFirstSibling
                    && pRec->Shared.pSibling != NULL
                    && pRec->Shared.pSibling != pFirstSibling)
                {
                    pRec = pRec->Shared.pSibling;
                    Assert(iEntry == UINT32_MAX);
                    continue;
                }

                /* Scan the owner table for blocked owners. */
                if (ASMAtomicUoReadU32(&pRec->Shared.cEntries) > 0)
                {
                    uint32_t                            cAllocated = ASMAtomicUoReadU32(&pRec->Shared.cAllocated);
                    PRTLOCKVALIDATORSHAREDONE volatile *papOwners  = pRec->Shared.papOwners;
                    while (++iEntry < cAllocated)
                    {
                        PRTLOCKVALIDATORSHAREDONE pEntry = rtLockValidatorUoReadSharedOwner(&papOwners[iEntry]);
                        if (   pEntry
                            && pEntry->Core.u32Magic == RTLOCKVALIDATORSHAREDONE_MAGIC)
                        {
                            pNextThread = rtLockValidatorReadThreadHandle(&pEntry->hThread);
                            if (    pNextThread
                                &&  !RTTHREAD_IS_SLEEPING(pNextThread->enmState)
                                &&  pNextThread != pThreadSelf)
                                pNextThread = NIL_RTTHREAD;
                        }
                        else
                            Assert(!pEntry || pEntry->Core.u32Magic == RTLOCKVALIDATORSHAREDONE_MAGIC_DEAD);
                    }
                }

                /* Advance to the next sibling, if any. */
                Assert(pNextThread == NIL_RTTHREAD);
                if (   pRec->Shared.pSibling != NULL
                    && pRec->Shared.pSibling != pFirstSibling)
                {
                    pRec = pRec->Shared.pSibling;
                    iEntry = UINT32_MAX;
                    continue;
                }
                break;

            case RTLOCKVALIDATORREC_MAGIC_DEAD:
            case RTLOCKVALIDATORSHARED_MAGIC_DEAD:
                pNextThread = NIL_RTTHREAD;
                break;

            case RTLOCKVALIDATORSHAREDONE_MAGIC:
            case RTLOCKVALIDATORSHAREDONE_MAGIC_DEAD:
            default:
                AssertMsgFailed(("%p: %#x\n", pRec, pRec->Core));
                pNextThread = NIL_RTTHREAD;
                break;
        }

        /* Is that thread waiting for something? */
        RTTHREADSTATE               enmNextState = RTTHREADSTATE_RUNNING;
        PRTLOCKVALIDATORRECUNION    pNextRec     = NULL;
        if (   pNextThread != NIL_RTTHREAD
            && RT_LIKELY(pNextThread->u32Magic == RTTHREADINT_MAGIC))
        {
            do
            {
                enmNextState = rtThreadGetState(pNextThread);
                if (    !RTTHREAD_IS_SLEEPING(enmNextState)
                    &&  pNextThread != pThreadSelf)
                    break;
                pNextRec = rtLockValidatorReadRecUnionPtr(&pNextThread->LockValidator.pRec);
                if (RT_LIKELY(   !pNextRec
                              || enmNextState == rtThreadGetState(pNextThread)))
                    break;
                pNextRec = NULL;
            } while (pNextThread->u32Magic == RTTHREADINT_MAGIC);
        }
        if (pNextRec)
        {
            /*
             * Recurse and check for deadlock.
             */
            uint32_t i = pStack->c;
            if (RT_UNLIKELY(i >= RT_ELEMENTS(pStack->a)))
                return rtLockValidatorDdHandleStackOverflow(pStack);

            pStack->c++;
            pStack->a[i].pRec           = pRec;
            pStack->a[i].iEntry         = iEntry;
            pStack->a[i].enmState       = enmState;
            pStack->a[i].pThread        = pThread;
            pStack->a[i].pFirstSibling  = pFirstSibling;

            if (RT_UNLIKELY(pNextThread == pThreadSelf))
                return VERR_SEM_LV_DEADLOCK;

            pRec            = pNextRec;
            pFirstSibling   = pNextRec;
            iEntry          = UINT32_MAX;
            enmState        = enmNextState;
            pThread         = pNextThread;
        }
        else if (RT_LIKELY(!pNextThread))
        {
            /*
             * No deadlock here, unwind the stack and deal with any unfinished
             * business there.
             */
            uint32_t i = pStack->c;
            for (;;)
            {
                /* pop */
                if (i == 0)
                    return VINF_SUCCESS;
                i--;

                /* examine it. */
                pRec            = pStack->a[i].pRec;
                pFirstSibling   = pStack->a[i].pFirstSibling;
                iEntry          = pStack->a[i].iEntry;
                if (rtLockValidatorDdMoreWorkLeft(pRec, iEntry, pFirstSibling))
                {
                    enmState    = pStack->a[i].enmState;
                    pThread     = pStack->a[i].pThread;
                    break;
                }
            }
        }
        /* else: see if there is another thread to check for this lock. */
    }
}


/**
 * Check for the simple no-deadlock case.
 *
 * @returns true if no deadlock, false if further investigation is required.
 *
 * @param   pOriginalRec    The original record.
 */
DECLINLINE(int) rtLockValidatorIsSimpleNoDeadlockCase(PRTLOCKVALIDATORRECUNION pOriginalRec)
{
    if (    pOriginalRec->Excl.Core.u32Magic == RTLOCKVALIDATORREC_MAGIC
        &&  !pOriginalRec->Excl.pSibling)
    {
        PRTTHREADINT pThread = rtLockValidatorReadThreadHandle(&pOriginalRec->Excl.hThread);
        if (   !pThread
            || pThread->u32Magic != RTTHREADINT_MAGIC)
            return true;
        RTTHREADSTATE enmState = rtThreadGetState(pThread);
        if (!RTTHREAD_IS_SLEEPING(enmState))
            return true;
    }
    return false;
}


/**
 * Worker for rtLockValidatorDeadlockDetection that bitches about a deadlock.
 *
 * @param   pStack          The chain of locks causing the deadlock.
 * @param   pThreadSelf     This thread.
 * @param   pSrcPos         Where we are going to deadlock.
 * @param   rc              The return code.
 */
static void rcLockValidatorDoDeadlockComplaining(PRTLOCKVALIDATORDDSTACK pStack, PRTTHREADINT pThreadSelf,
                                                 PCRTLOCKVALIDATORSRCPOS pSrcPos, int rc)
{
    if (!ASMAtomicUoReadBool(&g_fLockValidatorQuiet))
    {
        rtLockValidatorComplainFirst(  rc == VERR_SEM_LV_DEADLOCK
                                     ? "deadlock"
                                     : rc == VERR_SEM_LV_EXISTING_DEADLOCK
                                     ? "existing-deadlock"
                                     : "!unexpected rc!",
                                     pSrcPos, pThreadSelf, NULL);
        rtLockValidatorComplainMore("---- start of %u entry deadlock chain ----\n", pStack->c);
        for (uint32_t i = 0; i < pStack->c; i++)
        {
            char szPrefix[24];
            RTStrPrintf(szPrefix, sizeof(szPrefix), "#%u: ", i);
            PRTLOCKVALIDATORSHAREDONE pSharedOne = NULL;
            if (pStack->a[i].pRec->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC)
                pSharedOne = pStack->a[i].pRec->Shared.papOwners[pStack->a[i].iEntry];
            if (VALID_PTR(pSharedOne) && pSharedOne->Core.u32Magic == RTLOCKVALIDATORSHAREDONE_MAGIC)
                rtLockValidatorComplainAboutLock(szPrefix, (PRTLOCKVALIDATORRECUNION)pSharedOne, "\n");
            else
                rtLockValidatorComplainAboutLock(szPrefix, pStack->a[i].pRec, "\n");
        }
        rtLockValidatorComplainMore("---- end of deadlock chain ----\n");
    }

    rtLockValidatorComplainPanic();
}


/**
 * Perform deadlock detection.
 *
 * @retval  VINF_SUCCESS
 * @retval  VERR_SEM_LV_DEADLOCK
 * @retval  VERR_SEM_LV_EXISTING_DEADLOCK
 *
 * @param   pRec            The record relating to the current thread's lock
 *                          operation.
 * @param   pThreadSelf     The current thread.
 * @param   pSrcPos         The position of the current lock operation.
 */
static int rtLockValidatorDeadlockDetection(PRTLOCKVALIDATORRECUNION pRec, PRTTHREADINT pThreadSelf, PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
#ifdef DEBUG_bird
    RTLOCKVALIDATORDDSTACK Stack;
    int rc = rtLockValidatorDdDoDetection(&Stack, pRec, pThreadSelf);
    if (RT_FAILURE(rc))
        rcLockValidatorDoDeadlockComplaining(&Stack, pThreadSelf, pSrcPos, rc);
    return rc;
#else
    return VINF_SUCCESS;
#endif
}



RTDECL(int) RTLockValidatorCheckWriteOrderBlocking(PRTLOCKVALIDATORREC pWrite, PRTLOCKVALIDATORSHARED pRead,
                                                   RTTHREAD hThread, RTTHREADSTATE enmState, bool fRecursiveOk,
                                                   PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    /*
     * Fend off wild life.
     */
    PRTLOCKVALIDATORRECUNION pWriteU = (PRTLOCKVALIDATORRECUNION)pWrite; /* (avoid break aliasing rules) */
    AssertPtrReturn(pWriteU, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pWriteU->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    PRTLOCKVALIDATORRECUNION pReadU = (PRTLOCKVALIDATORRECUNION)pRead;
    AssertPtrReturn(pRead, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pReadU->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pReadU->Shared.fEnabled == pWriteU->Excl.fEnabled, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pWriteU->Excl.fEnabled)
        return VINF_SUCCESS;
    AssertReturn(RTTHREAD_IS_SLEEPING(enmState), VERR_SEM_LV_INVALID_PARAMETER);
    PRTTHREADINT pThread = hThread;
    AssertPtrReturn(pThread, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pThread->u32Magic == RTTHREADINT_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    RTTHREADSTATE enmThreadState = rtThreadGetState(pThread);
    AssertReturn(   enmThreadState == RTTHREADSTATE_RUNNING
                 || enmThreadState == RTTHREADSTATE_TERMINATED   /* rtThreadRemove uses locks too */
                 || enmThreadState == RTTHREADSTATE_INITIALIZING /* rtThreadInsert uses locks too */
                 , VERR_SEM_LV_INVALID_PARAMETER);

    /*
     * Check for attempts at doing a read upgrade.
     */
    PRTLOCKVALIDATORSHAREDONE pEntry = rtLockValidatorSharedRecFindThread(&pReadU->Shared, hThread, NULL);
    if (pEntry)
    {
        rtLockValidatorComplainFirst("Read lock upgrade", pSrcPos, pThread, (PRTLOCKVALIDATORRECUNION)pEntry);
        rtLockValidatorComplainPanic();
        return VERR_SEM_LV_UPGRADE;
    }



    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorCheckReadOrderBlocking(PRTLOCKVALIDATORSHARED pRead, PRTLOCKVALIDATORREC pWrite,
                                                  RTTHREAD hThread, RTTHREADSTATE enmState, bool fRecursiveOk,
                                                  PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    /*
     * Fend off wild life.
     */
    AssertPtrReturn(pRead, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRead->Core.u32Magic == RTLOCKVALIDATORSHARED_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertPtrReturn(pWrite, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pWrite->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRead->fEnabled == pWrite->fEnabled, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRead->fEnabled)
        return VINF_SUCCESS;
    AssertReturn(RTTHREAD_IS_SLEEPING(enmState), VERR_SEM_LV_INVALID_PARAMETER);
    PRTTHREADINT pThread = hThread;
    AssertPtrReturn(pThread, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pThread->u32Magic == RTTHREADINT_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    RTTHREADSTATE enmThreadState = rtThreadGetState(pThread);
    AssertReturn(   enmThreadState == RTTHREADSTATE_RUNNING
                 || enmThreadState == RTTHREADSTATE_TERMINATED   /* rtThreadRemove uses locks too */
                 || enmThreadState == RTTHREADSTATE_INITIALIZING /* rtThreadInsert uses locks too */
                 , VERR_SEM_LV_INVALID_PARAMETER);
    Assert(pWrite->hThread != pThread);


    return VINF_SUCCESS;
}


RTDECL(int) RTLockValidatorCheckBlocking(PRTLOCKVALIDATORREC pRec, RTTHREAD hThread,
                                         RTTHREADSTATE enmState, bool fRecursiveOk,
                                         PCRTLOCKVALIDATORSRCPOS pSrcPos)
{
    /*
     * Fend off wild life.
     */
    PRTLOCKVALIDATORRECUNION pRecU = (PRTLOCKVALIDATORRECUNION)pRec;
    AssertPtrReturn(pRecU, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pRecU->Core.u32Magic == RTLOCKVALIDATORREC_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    if (!pRecU->Excl.fEnabled)
        return VINF_SUCCESS;
    AssertReturn(RTTHREAD_IS_SLEEPING(enmState), VERR_SEM_LV_INVALID_PARAMETER);
    PRTTHREADINT pThreadSelf = hThread;
    AssertPtrReturn(pThreadSelf, VERR_SEM_LV_INVALID_PARAMETER);
    AssertReturn(pThreadSelf->u32Magic == RTTHREADINT_MAGIC, VERR_SEM_LV_INVALID_PARAMETER);
    RTTHREADSTATE enmThreadState = rtThreadGetState(pThreadSelf);
    AssertReturn(   enmThreadState == RTTHREADSTATE_RUNNING
                 || enmThreadState == RTTHREADSTATE_TERMINATED   /* rtThreadRemove uses locks too */
                 || enmThreadState == RTTHREADSTATE_INITIALIZING /* rtThreadInsert uses locks too */
                 , VERR_SEM_LV_INVALID_PARAMETER);

    /*
     * Record the location and everything before changing the state and
     * performing deadlock detection.
     */
    rtLockValidatorWriteRecUnionPtr(&pThreadSelf->LockValidator.pRec, pRecU);
    rtLockValidatorCopySrcPos(&pThreadSelf->LockValidator.SrcPos, pSrcPos);

    /*
     * Don't do deadlock detection if we're recursing.
     *
     * On some hosts we don't do recursion accounting our selves and there
     * isn't any other place to check for this.  semmutex-win.cpp for instance.
     */
    if (rtLockValidatorReadThreadHandle(&pRecU->Excl.hThread) == pThreadSelf)
    {
        if (fRecursiveOk)
            return VINF_SUCCESS;
        rtLockValidatorComplainFirst("Recursion not allowed", pSrcPos, pThreadSelf, pRecU);
        rtLockValidatorComplainPanic();
        return VERR_SEM_LV_NESTED;
    }

    /*
     * Perform deadlock detection.
     */
    if (rtLockValidatorIsSimpleNoDeadlockCase(pRecU))
        return VINF_SUCCESS;
    return rtLockValidatorDeadlockDetection(pRecU, pThreadSelf, pSrcPos);
}
RT_EXPORT_SYMBOL(RTLockValidatorCheckBlocking);


RTDECL(bool) RTLockValidatorSetEnabled(bool fEnabled)
{
    return ASMAtomicXchgBool(&g_fLockValidatorEnabled, fEnabled);
}
RT_EXPORT_SYMBOL(RTLockValidatorSetEnabled);


RTDECL(bool) RTLockValidatorIsEnabled(void)
{
    return ASMAtomicUoReadBool(&g_fLockValidatorEnabled);
}
RT_EXPORT_SYMBOL(RTLockValidatorIsEnabled);


RTDECL(bool) RTLockValidatorSetQuiet(bool fQuiet)
{
    return ASMAtomicXchgBool(&g_fLockValidatorQuiet, fQuiet);
}
RT_EXPORT_SYMBOL(RTLockValidatorSetQuiet);


RTDECL(bool) RTLockValidatorAreQuiet(void)
{
    return ASMAtomicUoReadBool(&g_fLockValidatorQuiet);
}
RT_EXPORT_SYMBOL(RTLockValidatorAreQuiet);


RTDECL(bool) RTLockValidatorSetMayPanic(bool fMayPanic)
{
    return ASMAtomicXchgBool(&g_fLockValidatorMayPanic, fMayPanic);
}
RT_EXPORT_SYMBOL(RTLockValidatorSetMayPanic);


RTDECL(bool) RTLockValidatorMayPanic(void)
{
    return ASMAtomicUoReadBool(&g_fLockValidatorMayPanic);
}
RT_EXPORT_SYMBOL(RTLockValidatorMayPanic);


/* $Id: TMInline.h 87792 2021-02-18 18:38:24Z vboxsync $ */
/** @file
 * TM - Common Inlined functions.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_include_TMInline_h
#define VMM_INCLUDED_SRC_include_TMInline_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


DECLINLINE(PTMTIMER) tmTimerQueueGetHead(PTMTIMERQUEUECC pQueueCC, PTMTIMERQUEUE pQueueShared)
{
#ifdef IN_RING3
    RT_NOREF(pQueueShared);
    uint32_t const idx = pQueueCC->idxActive;
#else
    uint32_t const idx = pQueueShared->idxActive;
#endif
    if (idx < pQueueCC->cTimersAlloc)
        return &pQueueCC->paTimers[idx];
    return NULL;
}


DECLINLINE(void) tmTimerQueueSetHead(PTMTIMERQUEUECC pQueueCC, PTMTIMERQUEUE pQueueShared, PTMTIMER pHead)
{
    uint32_t idx;
    if (pHead)
    {
        idx = (uint32_t)(pHead - &pQueueCC->paTimers[0]);
        AssertMsgStmt(idx < pQueueCC->cTimersAlloc,
                      ("idx=%u (%s) cTimersAlloc=%u\n", idx, pHead->szName, pQueueCC->cTimersAlloc),
                      idx = UINT32_MAX);
    }
    else
        idx = UINT32_MAX;
#ifndef IN_RING3
    pQueueShared->idxActive = idx;
#else
    pQueueCC->idxActive     = idx;
    RT_NOREF(pQueueShared);
#endif
}


/**
 * Get the previous timer - translates TMTIMER::idxPrev.
 */
DECLINLINE(PTMTIMER) tmTimerGetPrev(PTMTIMERQUEUECC pQueueCC, PTMTIMER pTimer)
{
    uint32_t const idxPrev = pTimer->idxPrev;
    Assert(idxPrev);
    if (idxPrev < pQueueCC->cTimersAlloc)
        return &pQueueCC->paTimers[idxPrev];
    Assert(idxPrev == UINT32_MAX);
    return NULL;
}


/**
 * Get the next timer - translates TMTIMER::idxNext.
 */
DECLINLINE(PTMTIMER) tmTimerGetNext(PTMTIMERQUEUECC pQueueCC, PTMTIMER pTimer)
{
    uint32_t const idxNext = pTimer->idxNext;
    Assert(idxNext);
    if (idxNext < pQueueCC->cTimersAlloc)
        return &pQueueCC->paTimers[idxNext];
    Assert(idxNext == UINT32_MAX);
    return NULL;
}


/**
 * Set the previous timer link (TMTIMER::idxPrev).
 */
DECLINLINE(void) tmTimerSetPrev(PTMTIMERQUEUECC pQueueCC, PTMTIMER pTimer, PTMTIMER pPrev)
{
    uint32_t idxPrev;
    if (pPrev)
    {
        idxPrev = (uint32_t)(pPrev - &pQueueCC->paTimers[0]);
        Assert(idxPrev);
        AssertMsgStmt(idxPrev < pQueueCC->cTimersAlloc,
                      ("idxPrev=%u (%s) cTimersAlloc=%u\n", idxPrev, pPrev->szName, pQueueCC->cTimersAlloc),
                      idxPrev = UINT32_MAX);
    }
    else
        idxPrev = UINT32_MAX;
    pTimer->idxPrev = idxPrev;
}


/**
 * Set the next timer link (TMTIMER::idxNext).
 */
DECLINLINE(void) tmTimerSetNext(PTMTIMERQUEUECC pQueueCC, PTMTIMER pTimer, PTMTIMER pNext)
{
    uint32_t idxNext;
    if (pNext)
    {
        idxNext = (uint32_t)(pNext - &pQueueCC->paTimers[0]);
        Assert(idxNext);
        AssertMsgStmt(idxNext < pQueueCC->cTimersAlloc,
                      ("idxNext=%u (%s) cTimersAlloc=%u\n", idxNext, pNext->szName, pQueueCC->cTimersAlloc),
                      idxNext = UINT32_MAX);
    }
    else
        idxNext = UINT32_MAX;
    pTimer->idxNext = idxNext;
}


/**
 * Used to unlink a timer from the active list.
 *
 * @param   pVM         The cross context VM structure.
 * @param   pQueueCC    The context specific queue data (same as @a pQueue for
 *                      ring-3).
 * @param   pQueue      The shared timer queue data.
 * @param   pTimer      The timer that needs linking.
 *
 * @remarks Called while owning the relevant queue lock.
 */
DECL_FORCE_INLINE(void) tmTimerQueueUnlinkActive(PVMCC pVM, PTMTIMERQUEUECC pQueueCC, PTMTIMERQUEUE pQueue, PTMTIMER pTimer)
{
#ifdef VBOX_STRICT
    TMTIMERSTATE const enmState = pTimer->enmState;
    Assert(  pTimer->enmClock == TMCLOCK_VIRTUAL_SYNC
           ? enmState == TMTIMERSTATE_ACTIVE
           : enmState == TMTIMERSTATE_PENDING_SCHEDULE || enmState == TMTIMERSTATE_PENDING_STOP_SCHEDULE);
#endif
    RT_NOREF(pVM);

    const PTMTIMER pPrev = tmTimerGetPrev(pQueueCC, pTimer);
    const PTMTIMER pNext = tmTimerGetNext(pQueueCC, pTimer);
    if (pPrev)
        tmTimerSetNext(pQueueCC, pPrev, pNext);
    else
    {
        tmTimerQueueSetHead(pQueueCC, pQueue, pNext);
        pQueue->u64Expire = pNext ? pNext->u64Expire : INT64_MAX;
        DBGFTRACE_U64_TAG(pVM, pQueue->u64Expire, "tmTimerQueueUnlinkActive");
    }
    if (pNext)
        tmTimerSetPrev(pQueueCC, pNext, pPrev);
    pTimer->idxNext = UINT32_MAX;
    pTimer->idxPrev = UINT32_MAX;
}


/** @def TMTIMER_HANDLE_TO_PTR_RETURN_EX
 * Converts a timer handle to a timer pointer, returning @a a_rcRet if the
 * handle is invalid.
 *
 * @param   a_pVM       The cross context VM structure.
 * @param   a_hTimer    The timer handle to translate.
 * @param   a_rcRet     What to return on failure.
 * @param   a_pTimerVar The timer variable to assign the resulting pointer to.
 */
#ifdef IN_RING3
# define TMTIMER_HANDLE_TO_PTR_RETURN_EX(a_pVM, a_hTimer, a_rcRet, a_pTimerVar) do { \
        uintptr_t const idxQueue = (uintptr_t)((a_hTimer) >> TMTIMERHANDLE_QUEUE_IDX_SHIFT) \
                                 & (uintptr_t)TMTIMERHANDLE_QUEUE_IDX_SMASK; \
        AssertReturn(idxQueue < RT_ELEMENTS((a_pVM)->tm.s.aTimerQueues), a_rcRet); \
        \
        uintptr_t const idxTimer = (uintptr_t)((a_hTimer) & TMTIMERHANDLE_TIMER_IDX_MASK); \
        AssertReturn(idxQueue < (a_pVM)->tm.s.aTimerQueues[idxQueue].cTimersAlloc, a_rcRet); \
        \
        (a_pTimerVar) = &(a_pVM)->tm.s.aTimerQueues[idxQueue].paTimers[idxTimer]; \
        AssertReturn((a_pTimerVar)->hSelf == a_hTimer, a_rcRet); \
    } while (0)
#else
# define TMTIMER_HANDLE_TO_PTR_RETURN_EX(a_pVM, a_hTimer, a_rcRet, a_pTimerVar) do { \
        uintptr_t const idxQueue = (uintptr_t)((a_hTimer) >> TMTIMERHANDLE_QUEUE_IDX_SHIFT) \
                                 & (uintptr_t)TMTIMERHANDLE_QUEUE_IDX_SMASK; \
        AssertReturn(idxQueue < RT_ELEMENTS((a_pVM)->tm.s.aTimerQueues), a_rcRet); \
        AssertCompile(RT_ELEMENTS((a_pVM)->tm.s.aTimerQueues) == RT_ELEMENTS((a_pVM)->tmr0.s.aTimerQueues)); \
        \
        uintptr_t const idxTimer = (uintptr_t)((a_hTimer) & TMTIMERHANDLE_TIMER_IDX_MASK); \
        AssertReturn(idxQueue < (a_pVM)->tmr0.s.aTimerQueues[idxQueue].cTimersAlloc, a_rcRet); \
        \
        (a_pTimerVar) = &(a_pVM)->tmr0.s.aTimerQueues[idxQueue].paTimers[idxTimer]; \
        AssertReturn((a_pTimerVar)->hSelf == a_hTimer, a_rcRet); \
        Assert((a_pTimerVar)->fFlags & TMTIMER_FLAGS_RING0); \
        Assert(VM_IS_EMT(pVM)); \
    } while (0)
#endif

/** @def TMTIMER_HANDLE_TO_PTR_RETURN
 * Converts a timer handle to a timer pointer, returning VERR_INVALID_HANDLE if
 * invalid.
 *
 * @param   a_pVM       The cross context VM structure.
 * @param   a_hTimer    The timer handle to translate.
 * @param   a_pTimerVar The timer variable to assign the resulting pointer to.
 */
#define TMTIMER_HANDLE_TO_PTR_RETURN(a_pVM, a_hTimer, a_pTimerVar) \
        TMTIMER_HANDLE_TO_PTR_RETURN_EX(a_pVM, a_hTimer, VERR_INVALID_HANDLE, a_pTimerVar)

/** @def TMTIMER_HANDLE_TO_PTR_RETURN_VOID
 * Converts a timer handle to a timer pointer, returning VERR_INVALID_HANDLE if
 * invalid.
 *
 * @param   a_pVM       The cross context VM structure.
 * @param   a_hTimer    The timer handle to translate.
 * @param   a_pTimerVar The timer variable to assign the resulting pointer to.
 */
#ifdef IN_RING3
# define TMTIMER_HANDLE_TO_PTR_RETURN_VOID(a_pVM, a_hTimer, a_pTimerVar) do { \
        uintptr_t const idxQueue = (uintptr_t)((a_hTimer) >> TMTIMERHANDLE_QUEUE_IDX_SHIFT) \
                                 & (uintptr_t)TMTIMERHANDLE_QUEUE_IDX_SMASK; \
        AssertReturnVoid(idxQueue < RT_ELEMENTS((a_pVM)->tm.s.aTimerQueues)); \
        \
        uintptr_t const idxTimer = (uintptr_t)((a_hTimer) & TMTIMERHANDLE_TIMER_IDX_MASK); \
        AssertReturnVoid(idxQueue < (a_pVM)->tm.s.aTimerQueues[idxQueue].cTimersAlloc); \
        \
        (a_pTimerVar) = &(a_pVM)->tm.s.aTimerQueues[idxQueue].paTimers[idxTimer]; \
        AssertReturnVoid((a_pTimerVar)->hSelf == a_hTimer); \
    } while (0)
#else
# define TMTIMER_HANDLE_TO_PTR_RETURN_VOID(a_pVM, a_hTimer, a_pTimerVar) do { \
        uintptr_t const idxQueue = (uintptr_t)((a_hTimer) >> TMTIMERHANDLE_QUEUE_IDX_SHIFT) \
                                 & (uintptr_t)TMTIMERHANDLE_QUEUE_IDX_SMASK; \
        AssertReturnVoid(idxQueue < RT_ELEMENTS((a_pVM)->tm.s.aTimerQueues)); \
        AssertCompile(RT_ELEMENTS((a_pVM)->tm.s.aTimerQueues) == RT_ELEMENTS((a_pVM)->tmr0.s.aTimerQueues)); \
        \
        uintptr_t const idxTimer = (uintptr_t)((a_hTimer) & TMTIMERHANDLE_TIMER_IDX_MASK); \
        AssertReturnVoid(idxQueue < (a_pVM)->tmr0.s.aTimerQueues[idxQueue].cTimersAlloc); \
        \
        (a_pTimerVar) = &(a_pVM)->tmr0.s.aTimerQueues[idxQueue].paTimers[idxTimer]; \
        AssertReturnVoid((a_pTimerVar)->hSelf == a_hTimer); \
        Assert((a_pTimerVar)->fFlags & TMTIMER_FLAGS_RING0); \
        Assert(VM_IS_EMT(pVM)); \
    } while (0)
#endif


#endif /* !VMM_INCLUDED_SRC_include_TMInline_h */


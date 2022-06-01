/* $Id: semmutex-r0drv-solaris.c 28532 2010-04-20 16:54:51Z vboxsync $ */
/** @file
 * IPRT - Mutex Semaphores, Ring-0 Driver, Solaris.
 */

/*
 * Copyright (C) 2006-2010 Sun Microsystems, Inc.
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
#include "the-solaris-kernel.h"
#include "internal/iprt.h"
#include <iprt/semaphore.h>

#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/err.h>
#include <iprt/list.h>
#include <iprt/thread.h>

#include "internal/magics.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Wrapper for the solaris semaphore structure.
 */
typedef struct RTSEMMUTEXINTERNAL
{
    /** Magic value (RTSEMMUTEX_MAGIC). */
    uint32_t                    u32Magic;
    /** The number of recursions. */
    uint32_t                    cRecursions;
    /** Whether we are being woken up by a release. */
    uint8_t volatile            fSignaled;
    /** The number of waiting threads. */
    uint32_t volatile           cWaiters;
    /** The number of threads in the process of waking up. */
    uint32_t volatile           cWaking;
    /** The owner thread, NIL_RTNATIVETHREAD if none. */
    RTNATIVETHREAD              hOwnerThread;
    /** The mutex object for synchronization. */
    kmutex_t                    Mtx;
    /** The condition variable for synchronization. */
    kcondvar_t                  Cnd;
} RTSEMMUTEXINTERNAL, *PRTSEMMUTEXINTERNAL;


RTDECL(int) RTSemMutexCreate(PRTSEMMUTEX phMtx)
{
    /*
     * Allocate.
     */
    PRTSEMMUTEXINTERNAL pThis = (PRTSEMMUTEXINTERNAL)RTMemAlloc(sizeof(*pThis));
    if (RT_UNLIKELY(!pThis))
        return VERR_NO_MEMORY;

    /*
     * Initialize.
     */
    pThis->u32Magic     = RTSEMMUTEX_MAGIC;
    pThis->cRecursions  = 0;
    pThis->fSignaled    = false;
    pThis->cWaiters     = 0;
    pThis->cWaking      = 0;
    pThis->hOwnerThread   = NIL_RTNATIVETHREAD;
    mutex_init(&pThis->Mtx, "IPRT Mutex", MUTEX_DRIVER, (void *)ipltospl(DISP_LEVEL));
    cv_init(&pThis->Cnd, "IPRT CVM", CV_DRIVER, NULL);
    *phMtx = pThis;
    return VINF_SUCCESS;
}


RTDECL(int) RTSemMutexDestroy(RTSEMMUTEX hMtx)
{
    PRTSEMMUTEXINTERNAL     pThis = hMtx;

    /*
     * Validate.
     */
    if (pThis == NIL_RTSEMMUTEX)
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMMUTEX_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);

    mutex_enter(&pThis->Mtx);

    /*
     * Invalidate the magic to indicate the mutex is being destroyed.
     */
    ASMAtomicIncU32(&pThis->u32Magic);
    if (pThis->cWaiters > 0)
    {
        /*
         * Wake up all waiters, last waiter thread cleans up.
         */
        ASMAtomicXchgU32(&pThis->cWaking, pThis->cWaking + pThis->cWaiters);
        cv_broadcast(&pThis->Cnd);
        mutex_exit(&pThis->Mtx);
    }
    else if (pThis->cWaking)
    {
        /*
         * We're not the last waiting thread to be woken up. Just relinquish & bail.
         */
        mutex_exit(&pThis->Mtx);
    }
    else
    {
        /*
         * We're the last waiter, destroy.
         */
        mutex_exit(&pThis->Mtx);
        cv_destroy(&pThis->Cnd);
        mutex_destroy(&pThis->Mtx);
        RTMemFree(pThis);
    }

    return VINF_SUCCESS;
}


/**
 * Worker for rtSemMutexSolarisRequest that handles the case where we go to sleep.
 *
 * @returns VINF_SUCCESS, VERR_INTERRUPTED, or VERR_SEM_DESTROYED.
 *          Returns without owning the mutex.
 * @param   pThis           The mutex instance.
 * @param   cMillies        The timeout, must be > 0 or RT_INDEFINITE_WAIT.
 * @param   fInterruptible  The wait type.
 *
 * @remarks This needs to be called with the mutex object held!
 */
static int rtSemMutexSolarisRequestSleep(PRTSEMMUTEXINTERNAL pThis, RTMSINTERVAL cMillies,
                                       bool fInterruptible)
{
    int rc = VERR_GENERAL_FAILURE;
    Assert(cMillies > 0);

    /*
     * Now we wait (sleep; although might spin and then sleep).
     */
    ASMAtomicIncU32(&pThis->cWaiters);

    if (cMillies != RT_INDEFINITE_WAIT)
    {
        clock_t cTicks   = drv_usectohz((clock_t)(cMillies * 1000L));
        clock_t cTimeout = ddi_get_lbolt();
        cTimeout        += cTicks;
        if (fInterruptible)
            rc = cv_timedwait_sig(&pThis->Cnd, &pThis->Mtx, cTimeout);
        else
            rc = cv_timedwait(&pThis->Cnd, &pThis->Mtx, cTimeout);
    }
    else
    {
        if (fInterruptible)
            rc = cv_wait_sig(&pThis->Cnd, &pThis->Mtx);
        else
        {
            cv_wait(&pThis->Cnd, &pThis->Mtx);
            rc = 1;
        }
    }

    if (rc > 0)
    {
        if (pThis->u32Magic == RTSEMMUTEX_MAGIC)
        {
            if (pThis->fSignaled)
            {
                /*
                 * Woken up by a release from another thread.
                 */
                ASMAtomicDecU32(&pThis->cWaking);
                ASMAtomicXchgU8(&pThis->fSignaled, false);
                pThis->cRecursions++;
                pThis->hOwnerThread = RTThreadNativeSelf();
                rc = VINF_SUCCESS;
            }
            else
            {
                /*
                 * Interrupted by some signal.
                 */
                rc= VERR_INTERRUPTED;
                ASMAtomicDecU32(&pThis->cWaiters);
            }
        }
        else
        {
            /*
             * Awakened due to the destruction-in-progress broadcast.
             * We will cleanup if we're the last waiter.
             */
            rc = VERR_SEM_DESTROYED;
            if (!ASMAtomicDecU32(&pThis->cWaking))
            {
                mutex_exit(&pThis->Mtx);
                cv_destroy(&pThis->Cnd);
                mutex_destroy(&pThis->Mtx);
                RTMemFree(pThis);
                return rc;
            }
        }
    }
    else if (rc == -1)
    {
        /*
         * Timed out.
         */
        rc = VERR_TIMEOUT;
        ASMAtomicDecU32(&pThis->cWaiters);
    }
    else
    {
        /*
         * Condition may not have been met, returned due to pending signal.
         */
        rc = VERR_INTERRUPTED;
        ASMAtomicDecU32(&pThis->cWaiters);
    }

    return rc;
}


/**
 * Internal worker.
 */
DECLINLINE(int) rtSemMutexSolarisRequest(RTSEMMUTEX hMutexSem, RTMSINTERVAL cMillies, bool fInterruptible)
{
    PRTSEMMUTEXINTERNAL pThis = hMutexSem;
    int rc = VERR_GENERAL_FAILURE;

    /*
     * Validate.
     */
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMMUTEX_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);

    /*
     * Lock it and check if it's a recursion.
     */
    mutex_enter(&pThis->Mtx);
    if (pThis->hOwnerThread == RTThreadNativeSelf())
    {
        pThis->cRecursions++;
        Assert(pThis->cRecursions > 1);
        Assert(pThis->cRecursions < 256);
        rc = VINF_SUCCESS;
    }
    /*
     * Not a recursion, make sure we don't sneak in when another thread
     * is being woken up (fSignaled).
     */
    else if (   pThis->hOwnerThread == NIL_RTNATIVETHREAD
             && pThis->cWaiters == 0
             && pThis->fSignaled == false)
    {
        pThis->cRecursions  = 1;
        pThis->hOwnerThread = RTThreadNativeSelf();
        rc = VINF_SUCCESS;
    }
    /*
     * A polling call?
     */
    else if (cMillies == 0)
        rc = VERR_TIMEOUT;
    /*
     * No, we really need to get to sleep.
     */
    else
        rc = rtSemMutexSolarisRequestSleep(pThis, cMillies, fInterruptible);

    mutex_exit(&pThis->Mtx);
    return rc;
}


#undef RTSemMutexRequest
RTDECL(int) RTSemMutexRequest(RTSEMMUTEX hMutexSem, RTMSINTERVAL cMillies)
{
    return rtSemMutexSolarisRequest(hMutexSem, cMillies, false /*fInterruptible*/);
}


RTDECL(int) RTSemMutexRequestDebug(RTSEMMUTEX hMutexSem, RTMSINTERVAL cMillies, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    return RTSemMutexRequest(hMutexSem, cMillies);
}


#undef RTSemMutexRequestNoResume
RTDECL(int) RTSemMutexRequestNoResume(RTSEMMUTEX hMutexSem, RTMSINTERVAL cMillies)
{
    return rtSemMutexSolarisRequest(hMutexSem, cMillies, true /*fInterruptible*/);
}


RTDECL(int) RTSemMutexRequestNoResumeDebug(RTSEMMUTEX hMutexSem, RTMSINTERVAL cMillies, RTHCUINTPTR uId, RT_SRC_POS_DECL)
{
    return RTSemMutexRequestNoResume(hMutexSem, cMillies);
}


RTDECL(int) RTSemMutexRelease(RTSEMMUTEX hMtx)
{
    PRTSEMMUTEXINTERNAL pThis = hMtx;
    int                 rc;

    /*
     * Validate.
     */
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertMsgReturn(pThis->u32Magic == RTSEMMUTEX_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), VERR_INVALID_HANDLE);

    /*
     * Take the lock and release one recursion.
     */
    mutex_enter(&pThis->Mtx);
    if (pThis->hOwnerThread == RTThreadNativeSelf())
    {
        Assert(pThis->cRecursions > 0);
        if (--pThis->cRecursions == 0)
        {
            pThis->hOwnerThread = NIL_RTNATIVETHREAD;

            /*
             * If there are any waiters, signal them.
             */
            if (pThis->cWaiters > 0)
            {
                ASMAtomicXchgU8(&pThis->fSignaled, true);
                ASMAtomicDecU32(&pThis->cWaiters);
                ASMAtomicIncU32(&pThis->cWaking);
                cv_signal(&pThis->Cnd);
            }
        }
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_NOT_OWNER;

    mutex_exit(&pThis->Mtx);
    return rc;
}


RTDECL(bool) RTSemMutexIsOwned(RTSEMMUTEX hMutexSem)
{
    PRTSEMMUTEXINTERNAL pThis = hMutexSem;
    bool fOwned = false;

    /*
     * Validate.
     */
    AssertPtrReturn(pThis, false);
    AssertMsgReturn(pThis->u32Magic == RTSEMMUTEX_MAGIC, ("u32Magic=%RX32 pThis=%p\n", pThis->u32Magic, pThis), false);

    /*
     * Check if this is the owner.
     */
    mutex_enter(&pThis->Mtx);
    fOwned = pThis->hOwnerThread != NIL_RTNATIVETHREAD;
    mutex_exit(&pThis->Mtx);

    return fOwned;
}



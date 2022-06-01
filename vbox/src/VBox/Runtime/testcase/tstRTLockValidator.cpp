/* $Id: tstRTLockValidator.cpp 25648 2010-01-05 14:32:58Z vboxsync $ */
/** @file
 * IPRT Testcase - RTLockValidator.
 */

/*
 * Copyright (C) 2006-2009 Sun Microsystems, Inc.
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

#include <iprt/asm.h>                   /* for return addresses */
#include <iprt/critsect.h>
#include <iprt/err.h>
#include <iprt/semaphore.h>
#include <iprt/test.h>
#include <iprt/thread.h>
#include <iprt/time.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define SECS_SIMPLE_TEST    1
#define SECS_RACE_TEST      3
#define TEST_SMALL_TIMEOUT  (  10*1000)
#define TEST_LARGE_TIMEOUT  (  60*1000)
#define TEST_DEBUG_TIMEOUT  (3600*1000)


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The testcase handle. */
static RTTEST               g_hTest;
/** Flip this in the debugger to get some peace to single step wild code. */
bool volatile               g_fDoNotSpin = false;

/** Set when the main thread wishes to terminate the test. */
bool volatile               g_fShutdown = false;
/** The number of threads. */
static uint32_t             g_cThreads;
static uint32_t             g_iDeadlockThread;
static RTTHREAD             g_ahThreads[32];
static RTCRITSECT           g_aCritSects[32];
static RTSEMRW              g_ahSemRWs[32];
static RTSEMMUTEX           g_ahSemMtxes[32];
static RTSEMEVENT           g_hSemEvt;
static RTSEMEVENTMULTI      g_hSemEvtMulti;

/** Multiple release event semaphore that is signalled by the main thread after
 * it has started all the threads. */
static RTSEMEVENTMULTI      g_hThreadsStartedEvt;

/** The number of threads that have called testThreadBlocking */
static uint32_t volatile    g_cThreadsBlocking;
/** Multiple release event semaphore that is signalled by the last thread to
 * call testThreadBlocking.  testWaitForAllOtherThreadsToSleep waits on this. */
static RTSEMEVENTMULTI      g_hThreadsBlockingEvt;

/** When to stop testing. */
static uint64_t             g_NanoTSStop;
/** The number of deadlocks. */
static uint32_t volatile    g_cDeadlocks;
/** The number of loops. */
static uint32_t volatile    g_cLoops;


/**
 * Spin until the callback stops returning VERR_TRY_AGAIN.
 *
 * @returns Callback result. VERR_TIMEOUT if too much time elapses.
 * @param   pfnCallback     Callback for checking the state.
 * @param   pvWhat          Callback parameter.
 */
static int testWaitForSomethingToBeOwned(int (*pfnCallback)(void *), void *pvWhat)
{
    RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
    RTTEST_CHECK_RC_OK(g_hTest, RTSemEventMultiWait(g_hThreadsStartedEvt, TEST_SMALL_TIMEOUT));

    uint64_t u64StartMS = RTTimeMilliTS();
    for (unsigned iLoop = 0; ; iLoop++)
    {
        RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);

        int rc = pfnCallback(pvWhat);
        if (rc != VERR_TRY_AGAIN/* && !g_fDoNotSpin*/)
        {
            RTTEST_CHECK_RC_OK(g_hTest, rc);
            return rc;
        }

        uint64_t cMsElapsed = RTTimeMilliTS() - u64StartMS;
        if (!g_fDoNotSpin)
            RTTEST_CHECK_RET(g_hTest, cMsElapsed <= TEST_SMALL_TIMEOUT, VERR_TIMEOUT);

        RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
        RTThreadSleep(/*g_fDoNotSpin ? TEST_DEBUG_TIMEOUT :*/ iLoop > 256 ? 1 : 0);
    }
}


static int testCheckIfCritSectIsOwned(void *pvWhat)
{
    PRTCRITSECT pCritSect = (PRTCRITSECT)pvWhat;
    if (!RTCritSectIsInitialized(pCritSect))
        return VERR_SEM_DESTROYED;
    if (RTCritSectIsOwned(pCritSect))
        return VINF_SUCCESS;
    return VERR_TRY_AGAIN;
}


static int testWaitForCritSectToBeOwned(PRTCRITSECT pCritSect)
{
    return testWaitForSomethingToBeOwned(testCheckIfCritSectIsOwned, pCritSect);
}


static int testCheckIfSemRWIsOwned(void *pvWhat)
{
    RTSEMRW hSemRW = (RTSEMRW)pvWhat;
    if (RTSemRWGetWriteRecursion(hSemRW) > 0)
        return VINF_SUCCESS;
    if (RTSemRWGetReadCount(hSemRW) > 0)
        return VINF_SUCCESS;
    return VERR_TRY_AGAIN;
}

static int testWaitForSemRWToBeOwned(RTSEMRW hSemRW)
{
    return testWaitForSomethingToBeOwned(testCheckIfSemRWIsOwned, hSemRW);
}


static int testCheckIfSemMutexIsOwned(void *pvWhat)
{
    RTSEMMUTEX hSemRW = (RTSEMMUTEX)pvWhat;
    if (RTSemMutexIsOwned(hSemRW))
        return VINF_SUCCESS;
    return VERR_TRY_AGAIN;
}

static int testWaitForSemMutexToBeOwned(RTSEMMUTEX hSemMutex)
{
    return testWaitForSomethingToBeOwned(testCheckIfSemMutexIsOwned, hSemMutex);
}


/**
 * For reducing spin in testWaitForAllOtherThreadsToSleep.
 */
static void testThreadBlocking(void)
{
    if (ASMAtomicIncU32(&g_cThreadsBlocking) == g_cThreads)
        RTTEST_CHECK_RC_OK(g_hTest, RTSemEventMultiSignal(g_hThreadsBlockingEvt));
}


/**
 * Waits for all the other threads to enter sleeping states.
 *
 * @returns VINF_SUCCESS on success, VERR_INTERNAL_ERROR on failure.
 * @param   enmDesiredState     The desired thread sleep state.
 * @param   cWaitOn             The distance to the lock they'll be waiting on,
 *                              the lock type is derived from the desired state.
 *                              UINT32_MAX means no special lock.
 */
static int testWaitForAllOtherThreadsToSleep(RTTHREADSTATE enmDesiredState, uint32_t cWaitOn)
{
    testThreadBlocking();
    RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
    RTTEST_CHECK_RC_OK(g_hTest, RTSemEventMultiWait(g_hThreadsBlockingEvt, TEST_SMALL_TIMEOUT));

    RTTHREAD hThreadSelf = RTThreadSelf();
    for (uint32_t iOuterLoop = 0; ; iOuterLoop++)
    {
        uint32_t cMissing  = 0;
        uint32_t cWaitedOn = 0;
        for (uint32_t i = 0; i < g_cThreads; i++)
        {
            RTTHREAD hThread = g_ahThreads[i];
            if (hThread == NIL_RTTHREAD)
                cMissing++;
            else if (hThread != hThreadSelf)
            {
                /*
                 * Figure out which lock to wait for.
                 */
                void *pvLock = NULL;
                if (cWaitOn != UINT32_MAX)
                {
                    uint32_t j = (i + cWaitOn) % g_cThreads;
                    switch (enmDesiredState)
                    {
                        case RTTHREADSTATE_CRITSECT:    pvLock = &g_aCritSects[j]; break;
                        case RTTHREADSTATE_RW_WRITE:
                        case RTTHREADSTATE_RW_READ:     pvLock = g_ahSemRWs[j]; break;
                        case RTTHREADSTATE_MUTEX:       pvLock = g_ahSemMtxes[j]; break;
                        default: break;
                    }
                }

                /*
                 * Wait for this thread.
                 */
                for (unsigned iLoop = 0; ; iLoop++)
                {
                    RTTHREADSTATE enmState = RTThreadGetReallySleeping(hThread);
                    if (RTTHREAD_IS_SLEEPING(enmState))
                    {
                        if (   enmState == enmDesiredState
                            && (   !pvLock
                                || (   pvLock == RTLockValidatorQueryBlocking(hThread)
                                    && !RTLockValidatorIsBlockedThreadInValidator(hThread) )
                               )
                            && RTThreadGetNativeState(hThread) != RTTHREADNATIVESTATE_RUNNING
                           )
                            break;
                    }
                    else if (   enmState != RTTHREADSTATE_RUNNING
                             && enmState != RTTHREADSTATE_INITIALIZING)
                        return VERR_INTERNAL_ERROR;
                    RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
                    RTThreadSleep(g_fDoNotSpin ? TEST_DEBUG_TIMEOUT : iOuterLoop + iLoop > 256 ? 1 : 0);
                    RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
                    cWaitedOn++;
                }
            }
            RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
        }

        if (!cMissing && !cWaitedOn)
            break;
        RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
        RTThreadSleep(g_fDoNotSpin ? TEST_DEBUG_TIMEOUT : iOuterLoop > 256 ? 1 : 0);
        RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
    }

    RTThreadSleep(0);                   /* fudge factor */
    RTTEST_CHECK_RET(g_hTest, !g_fShutdown, VERR_INTERNAL_ERROR);
    return VINF_SUCCESS;
}


/**
 * Worker that starts the threads.
 *
 * @returns Same as RTThreadCreate.
 * @param   cThreads            The number of threads to start.
 * @param   pfnThread           Thread function.
 */
static int testStartThreads(uint32_t cThreads, PFNRTTHREAD pfnThread)
{
    RTSemEventMultiReset(g_hThreadsStartedEvt);

    for (uint32_t i = 0; i < RT_ELEMENTS(g_ahThreads); i++)
        g_ahThreads[i] = NIL_RTTHREAD;

    int rc = VINF_SUCCESS;
    for (uint32_t i = 0; i < cThreads; i++)
    {
        rc = RTThreadCreateF(&g_ahThreads[i], pfnThread, (void *)(uintptr_t)i, 0,
                             RTTHREADTYPE_DEFAULT, RTTHREADFLAGS_WAITABLE, "thread-%02u", i);
        RTTEST_CHECK_RC_OK(g_hTest, rc);
        if (RT_FAILURE(rc))
            break;
    }

    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemEventMultiSignal(g_hThreadsStartedEvt), rcCheck);
    return rc;
}


/**
 * Worker that waits for the threads to complete.
 *
 * @param   cMillies            How long to wait for each.
 * @param   fStopOnError        Whether to stop on error and heed the thread
 *                              return status.
 */
static void testWaitForThreads(uint32_t cMillies, bool fStopOnError)
{
    uint32_t i = RT_ELEMENTS(g_ahThreads);
    while (i-- > 0)
        if (g_ahThreads[i] != NIL_RTTHREAD)
        {
            int rcThread;
            int rc2;
            RTTEST_CHECK_RC_OK(g_hTest, rc2 = RTThreadWait(g_ahThreads[i], cMillies, &rcThread));
            if (RT_SUCCESS(rc2))
                g_ahThreads[i] = NIL_RTTHREAD;
            if (fStopOnError && (RT_FAILURE(rc2) || RT_FAILURE(rcThread)))
                return;
        }
}


static void testIt(uint32_t cThreads, uint32_t cSecs, bool fLoops, PFNRTTHREAD pfnThread, const char *pszName)
{
    /*
     * Init test.
     */
    if (cSecs > 0)
        RTTestSubF(g_hTest, "%s, %u threads, %u secs", pszName, cThreads, cSecs);
    else
        RTTestSubF(g_hTest, "%s, %u threads, single pass", pszName, cThreads);

    RTTEST_CHECK_RETV(g_hTest, RT_ELEMENTS(g_ahThreads) >= cThreads);
    RTTEST_CHECK_RETV(g_hTest, RT_ELEMENTS(g_aCritSects) >= cThreads);

    g_cThreads = cThreads;
    g_fShutdown = false;

    for (uint32_t i = 0; i < cThreads; i++)
    {
        RTTEST_CHECK_RC_RETV(g_hTest, RTCritSectInit(&g_aCritSects[i]), VINF_SUCCESS);
        RTTEST_CHECK_RC_RETV(g_hTest, RTSemRWCreate(&g_ahSemRWs[i]), VINF_SUCCESS);
        RTTEST_CHECK_RC_RETV(g_hTest, RTSemMutexCreate(&g_ahSemMtxes[i]), VINF_SUCCESS);
    }
    RTTEST_CHECK_RC_RETV(g_hTest, RTSemEventCreate(&g_hSemEvt), VINF_SUCCESS);
    RTTEST_CHECK_RC_RETV(g_hTest, RTSemEventMultiCreate(&g_hSemEvtMulti), VINF_SUCCESS);
    RTTEST_CHECK_RC_RETV(g_hTest, RTSemEventMultiCreate(&g_hThreadsStartedEvt), VINF_SUCCESS);
    RTTEST_CHECK_RC_RETV(g_hTest, RTSemEventMultiCreate(&g_hThreadsBlockingEvt), VINF_SUCCESS);

    /*
     * The test loop.
     */
    uint32_t cPasses    = 0;
    uint32_t cLoops     = 0;
    uint32_t cDeadlocks = 0;
    uint32_t cErrors    = RTTestErrorCount(g_hTest);
    uint64_t uStartNS   = RTTimeNanoTS();
    g_NanoTSStop        = uStartNS + cSecs * UINT64_C(1000000000);
    do
    {
        g_iDeadlockThread  = (cThreads - 1 + cPasses) % cThreads;
        g_cLoops           = 0;
        g_cDeadlocks       = 0;
        g_cThreadsBlocking = 0;
        RTTEST_CHECK_RC(g_hTest, RTSemEventMultiReset(g_hThreadsBlockingEvt), VINF_SUCCESS);

        int rc = testStartThreads(cThreads, pfnThread);
        if (RT_SUCCESS(rc))
        {
            testWaitForThreads(TEST_LARGE_TIMEOUT + cSecs*1000, true);
            if (g_fDoNotSpin && RTTestErrorCount(g_hTest) != cErrors)
                testWaitForThreads(TEST_DEBUG_TIMEOUT, true);
        }

        RTTEST_CHECK(g_hTest, !fLoops || g_cLoops > 0);
        cLoops += g_cLoops;
        RTTEST_CHECK(g_hTest, !fLoops || g_cDeadlocks > 0);
        cDeadlocks += g_cDeadlocks;
        cPasses++;
    } while (   RTTestErrorCount(g_hTest) == cErrors
             && !fLoops
             && RTTimeNanoTS() < g_NanoTSStop);

    /*
     * Cleanup.
     */
    ASMAtomicWriteBool(&g_fShutdown, true);
    RTTEST_CHECK_RC(g_hTest, RTSemEventMultiSignal(g_hThreadsBlockingEvt), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTSemEventMultiSignal(g_hThreadsStartedEvt), VINF_SUCCESS);
    RTThreadSleep(RTTestErrorCount(g_hTest) == cErrors ? 0 : 50);

    for (uint32_t i = 0; i < cThreads; i++)
    {
        RTTEST_CHECK_RC(g_hTest, RTCritSectDelete(&g_aCritSects[i]), VINF_SUCCESS);
        RTTEST_CHECK_RC(g_hTest, RTSemRWDestroy(g_ahSemRWs[i]), VINF_SUCCESS);
        RTTEST_CHECK_RC(g_hTest, RTSemMutexDestroy(g_ahSemMtxes[i]), VINF_SUCCESS);
    }
    RTTEST_CHECK_RC(g_hTest, RTSemEventDestroy(g_hSemEvt), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTSemEventMultiDestroy(g_hSemEvtMulti), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTSemEventMultiDestroy(g_hThreadsStartedEvt), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTSemEventMultiDestroy(g_hThreadsBlockingEvt), VINF_SUCCESS);

    testWaitForThreads(TEST_SMALL_TIMEOUT, false);

    /*
     * Print results if applicable.
     */
    if (cSecs)
    {
        if (fLoops)
            RTTestPrintf(g_hTest,  RTTESTLVL_ALWAYS, "cLoops=%u cDeadlocks=%u (%u%%)\n",
                         cLoops, cDeadlocks, cLoops ? cDeadlocks * 100 / cLoops : 0);
        else
            RTTestPrintf(g_hTest,  RTTESTLVL_ALWAYS, "cPasses=%u\n", cPasses);
    }
}


static DECLCALLBACK(int) test1Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    PRTCRITSECT     pMine = &g_aCritSects[i];
    PRTCRITSECT     pNext = &g_aCritSects[(i + 1) % g_cThreads];

    RTTEST_CHECK_RC_RET(g_hTest, RTCritSectEnter(pMine), VINF_SUCCESS, rcCheck);
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTCritSectEnter(pMine), VINF_SUCCESS);
    if (RT_SUCCESS(testWaitForCritSectToBeOwned(pNext)))
    {
        int rc;
        if (i != g_iDeadlockThread)
        {
            testThreadBlocking();
            RTTEST_CHECK_RC(g_hTest, rc = RTCritSectEnter(pNext), VINF_SUCCESS);
        }
        else
        {
            RTTEST_CHECK_RC_OK(g_hTest, rc = testWaitForAllOtherThreadsToSleep(RTTHREADSTATE_CRITSECT, 1));
            if (RT_SUCCESS(rc))
                RTTEST_CHECK_RC(g_hTest, rc = RTCritSectEnter(pNext), VERR_SEM_LV_DEADLOCK);
        }
        RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
        if (RT_SUCCESS(rc))
            RTTEST_CHECK_RC(g_hTest, rc = RTCritSectLeave(pNext), VINF_SUCCESS);
    }
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTCritSectLeave(pMine), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTCritSectLeave(pMine), VINF_SUCCESS);
    return VINF_SUCCESS;
}


static void test1(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, false, test1Thread, "critsect");
}


static DECLCALLBACK(int) test2Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    RTSEMRW         hMine = g_ahSemRWs[i];
    RTSEMRW         hNext = g_ahSemRWs[(i + 1) % g_cThreads];
    int             rc;

    if (i & 1)
    {
        RTTEST_CHECK_RC_RET(g_hTest, RTSemRWRequestWrite(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS, rcCheck);
        if ((i & 3) == 3)
            RTTEST_CHECK_RC(g_hTest, RTSemRWRequestWrite(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS);
    }
    else
        RTTEST_CHECK_RC_RET(g_hTest, RTSemRWRequestRead(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS, rcCheck);
    if (RT_SUCCESS(testWaitForSemRWToBeOwned(hNext)))
    {
        if (i != g_iDeadlockThread)
        {
            testThreadBlocking();
            RTTEST_CHECK_RC(g_hTest, rc = RTSemRWRequestWrite(hNext, RT_INDEFINITE_WAIT), VINF_SUCCESS);
        }
        else
        {
            RTTEST_CHECK_RC_OK(g_hTest, rc = testWaitForAllOtherThreadsToSleep(RTTHREADSTATE_RW_WRITE, 1));
            if (RT_SUCCESS(rc))
            {
                if (g_cThreads > 1)
                    RTTEST_CHECK_RC(g_hTest, rc = RTSemRWRequestWrite(hNext, RT_INDEFINITE_WAIT), VERR_SEM_LV_DEADLOCK);
                else
                    RTTEST_CHECK_RC(g_hTest, rc = RTSemRWRequestWrite(hNext, RT_INDEFINITE_WAIT), VERR_SEM_LV_ILLEGAL_UPGRADE);
            }
        }
        RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
        if (RT_SUCCESS(rc))
            RTTEST_CHECK_RC(g_hTest, RTSemRWReleaseWrite(hNext), VINF_SUCCESS);
    }
    if (i & 1)
    {
        if ((i & 3) == 3)
            RTTEST_CHECK_RC(g_hTest, RTSemRWReleaseWrite(hMine), VINF_SUCCESS);
        RTTEST_CHECK_RC(g_hTest, RTSemRWReleaseWrite(hMine), VINF_SUCCESS);
    }
    else
        RTTEST_CHECK_RC(g_hTest, RTSemRWReleaseRead(hMine), VINF_SUCCESS);
    RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
    return VINF_SUCCESS;
}


static void test2(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, false, test2Thread, "read-write");
}


static DECLCALLBACK(int) test3Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    RTSEMRW         hMine = g_ahSemRWs[i];
    RTSEMRW         hNext = g_ahSemRWs[(i + 1) % g_cThreads];
    int             rc;

    if (i & 1)
        RTTEST_CHECK_RC_RET(g_hTest, RTSemRWRequestWrite(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS, rcCheck);
    else
        RTTEST_CHECK_RC_RET(g_hTest, RTSemRWRequestRead(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS, rcCheck);
    if (RT_SUCCESS(testWaitForSemRWToBeOwned(hNext)))
    {
        do
        {
            rc = RTSemRWRequestWrite(hNext, TEST_SMALL_TIMEOUT);
            if (rc != VINF_SUCCESS && rc != VERR_SEM_LV_DEADLOCK && rc != VERR_SEM_LV_ILLEGAL_UPGRADE)
            {
                RTTestFailed(g_hTest, "#%u: RTSemRWRequestWrite -> %Rrc\n", i, rc);
                break;
            }
            if (RT_SUCCESS(rc))
            {
                RTTEST_CHECK_RC(g_hTest, rc = RTSemRWReleaseWrite(hNext), VINF_SUCCESS);
                if (RT_FAILURE(rc))
                    break;
            }
            else
                ASMAtomicIncU32(&g_cDeadlocks);
            ASMAtomicIncU32(&g_cLoops);
        } while (RTTimeNanoTS() < g_NanoTSStop);
    }
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTSemRWReleaseWrite(hMine), VINF_SUCCESS);
    else
        RTTEST_CHECK_RC(g_hTest, RTSemRWReleaseRead(hMine), VINF_SUCCESS);
    RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
    return VINF_SUCCESS;
}


static void test3(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, true, test3Thread, "read-write race");
}


static DECLCALLBACK(int) test4Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    RTSEMRW         hMine = g_ahSemRWs[i];
    RTSEMRW         hNext = g_ahSemRWs[(i + 1) % g_cThreads];

    do
    {
        int rc1 = (i & 1 ? RTSemRWRequestWrite : RTSemRWRequestRead)(hMine, TEST_SMALL_TIMEOUT); /* ugly ;-) */
        RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
        if (rc1 != VINF_SUCCESS && rc1 != VERR_SEM_LV_DEADLOCK && rc1 != VERR_SEM_LV_ILLEGAL_UPGRADE)
        {
            RTTestFailed(g_hTest, "#%u: RTSemRWRequest%s(hMine,) -> %Rrc\n", i, i & 1 ? "Write" : "read", rc1);
            break;
        }
        if (RT_SUCCESS(rc1))
        {
            for (unsigned iInner = 0; iInner < 4; iInner++)
            {
                int rc2 = RTSemRWRequestWrite(hNext, TEST_SMALL_TIMEOUT);
                if (rc2 != VINF_SUCCESS && rc2 != VERR_SEM_LV_DEADLOCK && rc2 != VERR_SEM_LV_ILLEGAL_UPGRADE)
                {
                    RTTestFailed(g_hTest, "#%u: RTSemRWRequestWrite -> %Rrc\n", i, rc2);
                    break;
                }
                if (RT_SUCCESS(rc2))
                {
                    RTTEST_CHECK_RC(g_hTest, rc2 = RTSemRWReleaseWrite(hNext), VINF_SUCCESS);
                    if (RT_FAILURE(rc2))
                        break;
                }
                else
                    ASMAtomicIncU32(&g_cDeadlocks);
                ASMAtomicIncU32(&g_cLoops);
            }

            RTTEST_CHECK_RC(g_hTest, rc1 = (i & 1 ? RTSemRWReleaseWrite : RTSemRWReleaseRead)(hMine), VINF_SUCCESS);
            RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
            if (RT_FAILURE(rc1))
                break;
        }
        else
            ASMAtomicIncU32(&g_cDeadlocks);
        ASMAtomicIncU32(&g_cLoops);
    } while (RTTimeNanoTS() < g_NanoTSStop);

    return VINF_SUCCESS;
}


static void test4(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, true, test4Thread, "read-write race v2");
}


static DECLCALLBACK(int) test5Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    RTSEMMUTEX      hMine = g_ahSemMtxes[i];
    RTSEMMUTEX      hNext = g_ahSemMtxes[(i + 1) % g_cThreads];

    RTTEST_CHECK_RC_RET(g_hTest, RTSemMutexRequest(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS, rcCheck);
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTSemMutexRequest(hMine, RT_INDEFINITE_WAIT), VINF_SUCCESS);
    if (RT_SUCCESS(testWaitForSemMutexToBeOwned(hNext)))
    {
        int rc;
        if (i != g_iDeadlockThread)
        {
            testThreadBlocking();
            RTTEST_CHECK_RC(g_hTest, rc = RTSemMutexRequest(hNext, RT_INDEFINITE_WAIT), VINF_SUCCESS);
        }
        else
        {
            RTTEST_CHECK_RC_OK(g_hTest, rc = testWaitForAllOtherThreadsToSleep(RTTHREADSTATE_MUTEX, 1));
            if (RT_SUCCESS(rc))
                RTTEST_CHECK_RC(g_hTest, rc = RTSemMutexRequest(hNext, RT_INDEFINITE_WAIT), VERR_SEM_LV_DEADLOCK);
        }
        RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
        if (RT_SUCCESS(rc))
            RTTEST_CHECK_RC(g_hTest, rc = RTSemMutexRelease(hNext), VINF_SUCCESS);
    }
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTSemMutexRelease(hMine), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTSemMutexRelease(hMine), VINF_SUCCESS);
    return VINF_SUCCESS;
}


static void test5(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, false, test5Thread, "mutex");
}


static DECLCALLBACK(int) test6Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    PRTCRITSECT     pMine = &g_aCritSects[i];
    PRTCRITSECT     pNext = &g_aCritSects[(i + 1) % g_cThreads];

    RTTEST_CHECK_RC_RET(g_hTest, RTCritSectEnter(pMine), VINF_SUCCESS, rcCheck);
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTCritSectEnter(pMine), VINF_SUCCESS);
    if (RT_SUCCESS(testWaitForCritSectToBeOwned(pNext)))
    {
        int rc;
        if (i != g_iDeadlockThread)
        {
            testThreadBlocking();
            RTTEST_CHECK_RC(g_hTest, rc = RTCritSectEnter(pNext), VINF_SUCCESS);
            RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
            if (RT_SUCCESS(rc))
                RTTEST_CHECK_RC(g_hTest, rc = RTCritSectLeave(pNext), VINF_SUCCESS);
        }
        else
        {
            RTTEST_CHECK_RC_OK(g_hTest, rc = testWaitForAllOtherThreadsToSleep(RTTHREADSTATE_CRITSECT, 1));
            if (RT_SUCCESS(rc))
            {
                RTSemEventSetSignaller(g_hSemEvt, g_ahThreads[0]);
                for (uint32_t iThread = 1; iThread < g_cThreads; iThread++)
                    RTSemEventAddSignaller(g_hSemEvt, g_ahThreads[iThread]);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTTEST_CHECK_RC(g_hTest, RTSemEventWait(g_hSemEvt, TEST_SMALL_TIMEOUT), VERR_SEM_LV_DEADLOCK);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTTEST_CHECK_RC(g_hTest, RTSemEventSignal(g_hSemEvt), VINF_SUCCESS);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTTEST_CHECK_RC(g_hTest, RTSemEventWait(g_hSemEvt, TEST_SMALL_TIMEOUT), VINF_SUCCESS);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTSemEventSetSignaller(g_hSemEvt, NIL_RTTHREAD);
            }
        }
        RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
    }
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTCritSectLeave(pMine), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTCritSectLeave(pMine), VINF_SUCCESS);
    return VINF_SUCCESS;
}


static void test6(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, false, test6Thread, "event");
}


static DECLCALLBACK(int) test7Thread(RTTHREAD ThreadSelf, void *pvUser)
{
    uintptr_t       i     = (uintptr_t)pvUser;
    PRTCRITSECT     pMine = &g_aCritSects[i];
    PRTCRITSECT     pNext = &g_aCritSects[(i + 1) % g_cThreads];

    RTTEST_CHECK_RC_RET(g_hTest, RTCritSectEnter(pMine), VINF_SUCCESS, rcCheck);
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTCritSectEnter(pMine), VINF_SUCCESS);
    if (RT_SUCCESS(testWaitForCritSectToBeOwned(pNext)))
    {
        int rc;
        if (i != g_iDeadlockThread)
        {
            testThreadBlocking();
            RTTEST_CHECK_RC(g_hTest, rc = RTCritSectEnter(pNext), VINF_SUCCESS);
            RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
            if (RT_SUCCESS(rc))
                RTTEST_CHECK_RC(g_hTest, rc = RTCritSectLeave(pNext), VINF_SUCCESS);
        }
        else
        {
            RTTEST_CHECK_RC_OK(g_hTest, rc = testWaitForAllOtherThreadsToSleep(RTTHREADSTATE_CRITSECT, 1));
            if (RT_SUCCESS(rc))
            {
                RTSemEventMultiSetSignaller(g_hSemEvtMulti, g_ahThreads[0]);
                for (uint32_t iThread = 1; iThread < g_cThreads; iThread++)
                    RTSemEventMultiAddSignaller(g_hSemEvtMulti, g_ahThreads[iThread]);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTTEST_CHECK_RC(g_hTest, RTSemEventMultiReset(g_hSemEvtMulti), VINF_SUCCESS);
                RTTEST_CHECK_RC(g_hTest, RTSemEventMultiWait(g_hSemEvtMulti, TEST_SMALL_TIMEOUT), VERR_SEM_LV_DEADLOCK);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTTEST_CHECK_RC(g_hTest, RTSemEventMultiSignal(g_hSemEvtMulti), VINF_SUCCESS);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTTEST_CHECK_RC(g_hTest, RTSemEventMultiWait(g_hSemEvtMulti, TEST_SMALL_TIMEOUT), VINF_SUCCESS);
                RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
                RTSemEventMultiSetSignaller(g_hSemEvtMulti, NIL_RTTHREAD);
            }
        }
        RTTEST_CHECK(g_hTest, RTThreadGetState(RTThreadSelf()) == RTTHREADSTATE_RUNNING);
    }
    if (i & 1)
        RTTEST_CHECK_RC(g_hTest, RTCritSectLeave(pMine), VINF_SUCCESS);
    RTTEST_CHECK_RC(g_hTest, RTCritSectLeave(pMine), VINF_SUCCESS);
    return VINF_SUCCESS;
}


static void test7(uint32_t cThreads, uint32_t cSecs)
{
    testIt(cThreads, cSecs, false, test7Thread, "event multi");
}


static bool testIsLockValidationCompiledIn(void)
{
    RTCRITSECT CritSect;
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTCritSectInit(&CritSect), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTCritSectEnter(&CritSect), false);
    bool fRet = CritSect.pValidatorRec
             && CritSect.pValidatorRec->hThread == RTThreadSelf();
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTCritSectLeave(&CritSect), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTCritSectDelete(&CritSect), false);

    RTSEMRW hSemRW;
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemRWCreate(&hSemRW), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemRWRequestRead(hSemRW, 50), false);
    int rc = RTSemRWRequestWrite(hSemRW, 1);
    if (rc != VERR_SEM_LV_ILLEGAL_UPGRADE)
        fRet = false;
    RTTEST_CHECK_RET(g_hTest, RT_FAILURE_NP(rc), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemRWReleaseRead(hSemRW), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemRWDestroy(hSemRW), false);

#if 0 /** @todo detect it on RTSemMutex... */
    RTSEMMUTEX hSemMtx;
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemMutexCreate(&hSemRW), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemMutexRequest(hSemRW, 50), false);
    /*??*/
    RTTEST_CHECK_RET(g_hTest, RT_FAILURE_NP(rc), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemRWRelease(hSemRW), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemRWDestroy(hSemRW), false);
#endif

    RTSEMEVENT hSemEvt;
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemEventCreate(&hSemEvt), false);
    RTSemEventSetSignaller(hSemEvt, RTThreadSelf());
    RTSemEventSetSignaller(hSemEvt, NIL_RTTHREAD);
    rc = RTSemEventSignal(hSemEvt);
    if (rc != VERR_SEM_LV_NOT_SIGNALLER)
        fRet = false;
    RTTEST_CHECK_RET(g_hTest, RT_FAILURE_NP(rc), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemEventDestroy(hSemEvt), false);

    RTSEMEVENTMULTI hSemEvtMulti;
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemEventMultiCreate(&hSemEvtMulti), false);
    RTSemEventMultiSetSignaller(hSemEvtMulti, RTThreadSelf());
    RTSemEventMultiSetSignaller(hSemEvtMulti, NIL_RTTHREAD);
    rc = RTSemEventMultiSignal(hSemEvtMulti);
    if (rc != VERR_SEM_LV_NOT_SIGNALLER)
        fRet = false;
    RTTEST_CHECK_RET(g_hTest, RT_FAILURE_NP(rc), false);
    RTTEST_CHECK_RC_OK_RET(g_hTest, RTSemEventMultiDestroy(hSemEvtMulti), false);

    return fRet;
}


int main()
{
    /*
     * Init.
     */
    int rc = RTTestInitAndCreate("tstRTLockValidator", &g_hTest);
    if (rc)
        return rc;
    RTTestBanner(g_hTest);

    RTLockValidatorSetEnabled(true);
    RTLockValidatorSetMayPanic(false);
    RTLockValidatorSetQuiet(true);
    if (!testIsLockValidationCompiledIn())
        return RTTestErrorCount(g_hTest) > 0
            ? RTTestSummaryAndDestroy(g_hTest)
            : RTTestSkipAndDestroy(g_hTest, "deadlock detection is not compiled in");
    RTLockValidatorSetQuiet(false);

    /*
     * Some initial tests with verbose output (all single pass).
     */
    test1(3, 0);
    test2(1, 0);
    test2(3, 0);
    test5(3, 0);
    test6(3, 0);
    test7(3, 0);

    /*
     * If successful, perform more thorough testing without noisy output.
     */
    if (RTTestErrorCount(g_hTest) == 0)
    {
        RTLockValidatorSetQuiet(true);

        test1( 2, SECS_SIMPLE_TEST);
        test1( 3, SECS_SIMPLE_TEST);
        test1( 7, SECS_SIMPLE_TEST);
        test1(10, SECS_SIMPLE_TEST);
        test1(15, SECS_SIMPLE_TEST);
        test1(30, SECS_SIMPLE_TEST);

        test2( 1, SECS_SIMPLE_TEST);
        test2( 2, SECS_SIMPLE_TEST);
        test2( 3, SECS_SIMPLE_TEST);
        test2( 7, SECS_SIMPLE_TEST);
        test2(10, SECS_SIMPLE_TEST);
        test2(15, SECS_SIMPLE_TEST);
        test2(30, SECS_SIMPLE_TEST);

        test3( 2, SECS_SIMPLE_TEST);
        test3(10, SECS_SIMPLE_TEST);

        test4( 2, SECS_RACE_TEST);
        test4( 6, SECS_RACE_TEST);
        test4(10, SECS_RACE_TEST);
        test4(30, SECS_RACE_TEST);

        test5( 2, SECS_RACE_TEST);
        test5( 3, SECS_RACE_TEST);
        test5( 7, SECS_RACE_TEST);
        test5(10, SECS_RACE_TEST);
        test5(15, SECS_RACE_TEST);
        test5(30, SECS_RACE_TEST);

        test6( 2, SECS_SIMPLE_TEST);
        test6( 3, SECS_SIMPLE_TEST);
        test6( 7, SECS_SIMPLE_TEST);
        test6(10, SECS_SIMPLE_TEST);
        test6(15, SECS_SIMPLE_TEST);
        test6(30, SECS_SIMPLE_TEST);

        test7( 2, SECS_SIMPLE_TEST);
        test7( 3, SECS_SIMPLE_TEST);
        test7( 7, SECS_SIMPLE_TEST);
        test7(10, SECS_SIMPLE_TEST);
        test7(15, SECS_SIMPLE_TEST);
        test7(30, SECS_SIMPLE_TEST);
    }

    return RTTestSummaryAndDestroy(g_hTest);
}


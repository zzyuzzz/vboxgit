/* $Id: semeventmulti-win.cpp 25831 2010-01-14 15:12:53Z vboxsync $ */
/** @file
 * IPRT - Multiple Release Event Semaphore, Windows.
 *
 * @remarks This file is identical to semevent-win.cpp except for the 2nd
 *          CreateEvent parameter, the reset function and the "Multi" infix.
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
#define LOG_GROUP RTLOGGROUP_SEMAPHORE
#include <Windows.h>

#include <iprt/semaphore.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/lockvalidator.h>
#include <iprt/mem.h>
#include <iprt/thread.h>
#include "internal/magics.h"
#include "internal/strict.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
struct RTSEMEVENTMULTIINTERNAL
{
    /** Magic value (RTSEMEVENTMULTI_MAGIC). */
    uint32_t            u32Magic;
    /** The event handle. */
    HANDLE              hev;
#ifdef RTSEMEVENT_STRICT
    /** Signallers. */
    RTLOCKVALRECSHRD    Signallers;
    /** Indicates that lock validation should be performed. */
    bool volatile       fEverHadSignallers;
#endif
};



RTDECL(int)  RTSemEventMultiCreate(PRTSEMEVENTMULTI phEventMultiSem)
{
    return RTSemEventMultiCreateEx(phEventMultiSem, 0 /*fFlags*/, NIL_RTLOCKVALCLASS, NULL);
}


RTDECL(int)  RTSemEventMultiCreateEx(PRTSEMEVENTMULTI phEventMultiSem, uint32_t fFlags, RTLOCKVALCLASS hClass,
                                     const char *pszNameFmt, ...)
{
    AssertReturn(!(fFlags & ~RTSEMEVENTMULTI_FLAGS_NO_LOCK_VAL), VERR_INVALID_PARAMETER);

    struct RTSEMEVENTMULTIINTERNAL *pThis = (struct RTSEMEVENTMULTIINTERNAL *)RTMemAlloc(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;

    /*
     * Create the semaphore.
     * (Manual reset, not signaled, private event object.)
     */
    pThis->hev = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pThis->hev != NULL) /* not INVALID_HANDLE_VALUE */
    {
        pThis->u32Magic = RTSEMEVENTMULTI_MAGIC;
#ifdef RTSEMEVENT_STRICT
        if (!pszNameFmt)
        {
            static uint32_t volatile s_iSemEventMultiAnon = 0;
            RTLockValidatorRecSharedInit(&pThis->Signallers, hClass, RTLOCKVAL_SUB_CLASS_ANY, pThis,
                                         true /*fSignaller*/, !(fFlags & RTSEMEVENTMULTI_FLAGS_NO_LOCK_VAL),
                                         "RTSemEventMulti-%u", ASMAtomicIncU32(&s_iSemEventMultiAnon) - 1);
        }
        else
        {
            va_list va;
            va_start(va, pszNameFmt);
            RTLockValidatorRecSharedInitV(&pThis->Signallers, hClass, RTLOCKVAL_SUB_CLASS_ANY, pThis,
                                          true /*fSignaller*/, !(fFlags & RTSEMEVENTMULTI_FLAGS_NO_LOCK_VAL),
                                          pszNameFmt, va);
            va_end(va);
        }
        pThis->fEverHadSignallers = false;
#endif

        *phEventMultiSem = pThis;
        return VINF_SUCCESS;
    }

    DWORD dwErr = GetLastError();
    RTMemFree(pThis);
    return RTErrConvertFromWin32(dwErr);
}


RTDECL(int)  RTSemEventMultiDestroy(RTSEMEVENTMULTI hEventMultiSem)
{
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    if (pThis == NIL_RTSEMEVENT)        /* don't bitch */
        return VINF_SUCCESS;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Invalidate the handle and close the semaphore.
     */
    int rc = VINF_SUCCESS;
    AssertReturn(ASMAtomicCmpXchgU32(&pThis->u32Magic, ~RTSEMEVENTMULTI_MAGIC, RTSEMEVENTMULTI_MAGIC), VERR_INVALID_HANDLE);
    if (CloseHandle(pThis->hev))
    {
#ifdef RTSEMEVENT_STRICT
        RTLockValidatorRecSharedDelete(&pThis->Signallers);
#endif
        RTMemFree(pThis);
    }
    else
    {
        DWORD dwErr = GetLastError();
        rc = RTErrConvertFromWin32(dwErr);
        AssertMsgFailed(("Destroy hEventMultiSem %p failed, lasterr=%u (%Rrc)\n", pThis, dwErr, rc));
        /* Leak it. */
    }

    return rc;
}


RTDECL(int)  RTSemEventMultiSignal(RTSEMEVENTMULTI hEventMultiSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC, VERR_INVALID_HANDLE);

#ifdef RTSEMEVENT_STRICT
    if (pThis->fEverHadSignallers)
    {
        int rc9 = RTLockValidatorRecSharedCheckSignaller(&pThis->Signallers, NIL_RTTHREAD);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#endif

    /*
     * Signal the object.
     */
    if (SetEvent(pThis->hev))
        return VINF_SUCCESS;
    DWORD dwErr = GetLastError();
    AssertMsgFailed(("Signaling hEventMultiSem %p failed, lasterr=%d\n", pThis, dwErr));
    return RTErrConvertFromWin32(dwErr);
}


RTDECL(int)  RTSemEventMultiReset(RTSEMEVENTMULTI hEventMultiSem)
{
    /*
     * Validate input.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Reset the object.
     */
    if (ResetEvent(pThis->hev))
        return VINF_SUCCESS;
    DWORD dwErr = GetLastError();
    AssertMsgFailed(("Resetting hEventMultiSem %p failed, lasterr=%d\n", pThis, dwErr));
    return RTErrConvertFromWin32(dwErr);
}


/** Goto avoidance. */
DECL_FORCE_INLINE(int) rtSemEventWaitHandleStatus(struct RTSEMEVENTMULTIINTERNAL *pThis, DWORD rc)
{
    switch (rc)
    {
        case WAIT_OBJECT_0:         return VINF_SUCCESS;
        case WAIT_TIMEOUT:          return VERR_TIMEOUT;
        case WAIT_IO_COMPLETION:    return VERR_INTERRUPTED;
        case WAIT_ABANDONED:        return VERR_SEM_OWNER_DIED;
        default:
            AssertMsgFailed(("%u\n", rc));
        case WAIT_FAILED:
        {
            int rc2 = RTErrConvertFromWin32(GetLastError());
            AssertMsgFailed(("Wait on hEventMultiSem %p failed, rc=%d lasterr=%d\n", pThis, rc, GetLastError()));
            if (rc2)
                return rc2;

            AssertMsgFailed(("WaitForSingleObject(event) -> rc=%d while converted lasterr=%d\n", rc, rc2));
            return VERR_INTERNAL_ERROR;
        }
    }
}


#undef RTSemEventMultiWaitNoResume
RTDECL(int)  RTSemEventMultiWaitNoResume(RTSEMEVENTMULTI hEventMultiSem, RTMSINTERVAL cMillies)
{
    PCRTLOCKVALSRCPOS pSrcPos = NULL;

    /*
     * Validate input.
     */
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC, VERR_INVALID_HANDLE);

    /*
     * Wait for condition.
     */
#ifdef RTSEMEVENT_STRICT
    RTTHREAD hThreadSelf = RTThreadSelfAutoAdopt();
    if (pThis->fEverHadSignallers)
    {
        DWORD rc = WaitForSingleObjectEx(pThis->hev,
                                         0 /*Timeout*/,
                                         TRUE /*fAlertable*/);
        if (rc != WAIT_TIMEOUT || cMillies == 0)
            return rtSemEventWaitHandleStatus(pThis, rc);
        int rc9 = RTLockValidatorRecSharedCheckBlocking(&pThis->Signallers, hThreadSelf, pSrcPos, false,
                                                        cMillies, RTTHREADSTATE_EVENT_MULTI, true);
        if (RT_FAILURE(rc9))
            return rc9;
    }
#else
    RTTHREAD hThreadSelf = RTThreadSelf();
#endif
    RTThreadBlocking(hThreadSelf, RTTHREADSTATE_EVENT_MULTI, true);
    DWORD rc = WaitForSingleObjectEx(pThis->hev,
                                     cMillies == RT_INDEFINITE_WAIT ? INFINITE : cMillies,
                                     TRUE /*fAlertable*/);
    RTThreadUnblocked(hThreadSelf, RTTHREADSTATE_EVENT_MULTI);
    return rtSemEventWaitHandleStatus(pThis, rc);
}


RTDECL(void) RTSemEventMultiSetSignaller(RTSEMEVENTMULTI hEventMultiSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedResetOwner(&pThis->Signallers, hThread, NULL);
#endif
}


RTDECL(void) RTSemEventMultiAddSignaller(RTSEMEVENTMULTI hEventMultiSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC);

    ASMAtomicWriteBool(&pThis->fEverHadSignallers, true);
    RTLockValidatorRecSharedAddOwner(&pThis->Signallers, hThread, NULL);
#endif
}


RTDECL(void) RTSemEventMultiRemoveSignaller(RTSEMEVENTMULTI hEventMultiSem, RTTHREAD hThread)
{
#ifdef RTSEMEVENT_STRICT
    struct RTSEMEVENTMULTIINTERNAL *pThis = hEventMultiSem;
    AssertPtrReturnVoid(pThis);
    AssertReturnVoid(pThis->u32Magic == RTSEMEVENTMULTI_MAGIC);

    RTLockValidatorRecSharedRemoveOwner(&pThis->Signallers, hThread);
#endif
}


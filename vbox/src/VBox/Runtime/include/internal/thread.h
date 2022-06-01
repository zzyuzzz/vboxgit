/* $Id: thread.h 28800 2010-04-27 08:22:32Z vboxsync $ */
/** @file
 * IPRT - Internal RTThread header.
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
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
 */

#ifndef ___thread_h
#define ___thread_h

#include <iprt/types.h>
#include <iprt/thread.h>
#include <iprt/avl.h>
#ifdef IN_RING3
# include <iprt/process.h>
# include <iprt/critsect.h>
#endif
#include "internal/lockvalidator.h"
#include "internal/magics.h"

RT_C_DECLS_BEGIN




/** Max thread name length. */
#define RTTHREAD_NAME_LEN       16
#ifdef IPRT_WITH_GENERIC_TLS
/** The number of TLS entries for the generic implementation. */
# define RTTHREAD_TLS_ENTRIES   64
#endif

/**
 * Internal represenation of a thread.
 */
typedef struct RTTHREADINT
{
    /** Avl node core - the key is the native thread id. */
    AVLPVNODECORE           Core;
    /** Magic value (RTTHREADINT_MAGIC). */
    uint32_t                u32Magic;
    /** Reference counter. */
    uint32_t volatile       cRefs;
    /** The current thread state. */
    RTTHREADSTATE volatile  enmState;
    /** Set when really sleeping. */
    bool volatile           fReallySleeping;
#if defined(RT_OS_WINDOWS) && defined(IN_RING3)
    /** The thread handle
     * This is not valid until the create function has returned! */
    uintptr_t               hThread;
#endif
#if defined(RT_OS_LINUX) && defined(IN_RING3)
    /** The thread ID.
     * This is not valid before rtThreadMain has been called by the new thread.  */
    pid_t                   tid;
#endif
    /** The user event semaphore. */
    RTSEMEVENTMULTI         EventUser;
    /** The terminated event semaphore. */
    RTSEMEVENTMULTI         EventTerminated;
    /** The thread type. */
    RTTHREADTYPE            enmType;
    /** The thread creation flags. (RTTHREADFLAGS) */
    unsigned                fFlags;
    /** Internal flags. (RTTHREADINT_FLAGS_ *) */
    uint32_t                fIntFlags;
    /** The result code. */
    int                     rc;
    /** Thread function. */
    PFNRTTHREAD             pfnThread;
    /** Thread function argument. */
    void                   *pvUser;
    /** Actual stack size. */
    size_t                  cbStack;
#ifdef IN_RING3
    /** The lock validator data. */
    RTLOCKVALPERTHREAD      LockValidator;
#endif /* IN_RING3 */
#ifdef IPRT_WITH_GENERIC_TLS
    /** The TLS entries for this thread. */
    void                   *apvTlsEntries[RTTHREAD_TLS_ENTRIES];
#endif
    /** Thread name. */
    char                    szName[RTTHREAD_NAME_LEN];
} RTTHREADINT;
/** Pointer to the internal representation of a thread. */
typedef RTTHREADINT *PRTTHREADINT;


/** @name RTTHREADINT::fIntFlags Masks and Bits.
 * @{ */
/** Set if the thread is an alien thread.
 * Clear if the thread was created by IPRT. */
#define RTTHREADINT_FLAGS_ALIEN      RT_BIT(0)
/** Set if the thread has terminated.
 * Clear if the thread is running. */
#define RTTHREADINT_FLAGS_TERMINATED RT_BIT(1)
/** This bit is set if the thread is in the AVL tree. */
#define RTTHREADINT_FLAG_IN_TREE_BIT 2
/** @copydoc RTTHREADINT_FLAG_IN_TREE_BIT */
#define RTTHREADINT_FLAG_IN_TREE     RT_BIT(RTTHREADINT_FLAG_IN_TREE_BIT)
/** Set if it's the main thread. */
#define RTTHREADINT_FLAGS_MAIN       RT_BIT(3)
/** @} */


/**
 * Initialize the native part of the thread management.
 *
 * Generally a TLS entry will be allocated at this point (Ring-3).
 *
 * @returns iprt status code.
 */
int rtThreadNativeInit(void);

/**
 * Create a native thread.
 * This creates the thread as described in pThreadInt and stores the thread id in *pThread.
 *
 * @returns iprt status code.
 * @param   pThreadInt      The thread data structure for the thread.
 * @param   pNativeThread   Where to store the native thread identifier.
 */
int rtThreadNativeCreate(PRTTHREADINT pThreadInt, PRTNATIVETHREAD pNativeThread);

/**
 * Adopts a thread, this is called immediately after allocating the
 * thread structure.
 *
 * @param   pThread     Pointer to the thread structure.
 */
int rtThreadNativeAdopt(PRTTHREADINT pThread);

/**
 * Sets the priority of the thread according to the thread type
 * and current process priority.
 *
 * The RTTHREADINT::enmType member has not yet been updated and will be updated by
 * the caller on a successful return.
 *
 * @returns iprt status code.
 * @param   pThread     The thread in question.
 * @param   enmType     The thread type.
 * @remark  Located in sched.
 */
int rtThreadNativeSetPriority(PRTTHREADINT pThread, RTTHREADTYPE enmType);

#ifdef IN_RING3
# ifdef RT_OS_WINDOWS
/**
 * Callback for when a native thread is detaching.
 *
 * It give the Win32/64 backend a chance to terminate alien
 * threads properly.
 */
void rtThreadNativeDetach(void);
# endif
#endif /* !IN_RING0 */


/* thread.cpp */
int rtThreadMain(PRTTHREADINT pThread, RTNATIVETHREAD NativeThread, const char *pszThreadName);
void rtThreadBlocking(PRTTHREADINT pThread, RTTHREADSTATE enmState, uint64_t u64Block,
                      const char *pszFile, unsigned uLine, RTUINTPTR uId);
void rtThreadUnblocked(PRTTHREADINT pThread, RTTHREADSTATE enmCurState);
uint32_t rtThreadRelease(PRTTHREADINT pThread);
void rtThreadTerminate(PRTTHREADINT pThread, int rc);
PRTTHREADINT rtThreadGetByNative(RTNATIVETHREAD NativeThread);
PRTTHREADINT rtThreadGet(RTTHREAD Thread);
int rtThreadInit(void);
void rtThreadTerm(void);
void rtThreadInsert(PRTTHREADINT pThread, RTNATIVETHREAD NativeThread);
#ifdef IN_RING3
int rtThreadDoSetProcPriority(RTPROCPRIORITY enmPriority);
#endif /* !IN_RING0 */
#ifdef IPRT_WITH_GENERIC_TLS
void rtThreadClearTlsEntry(RTTLS iTls);
void rtThreadTlsDestruction(PRTTHREADINT pThread); /* in tls-generic.cpp */
#endif

#ifdef ___iprt_asm_h

/**
 * Gets the thread state.
 *
 * @returns The thread state.
 * @param   pThread             The thread.
 */
DECLINLINE(RTTHREADSTATE) rtThreadGetState(PRTTHREADINT pThread)
{
    return pThread->enmState;
}

/**
 * Sets the thread state.
 *
 * @param   pThread             The thread.
 * @param   enmNewState         The new thread state.
 */
DECLINLINE(void) rtThreadSetState(PRTTHREADINT pThread, RTTHREADSTATE enmNewState)
{
    AssertCompile(sizeof(pThread->enmState) == sizeof(uint32_t));
    ASMAtomicWriteU32((uint32_t volatile *)&pThread->enmState, enmNewState);
}

#endif

RT_C_DECLS_END

#endif

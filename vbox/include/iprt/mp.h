/** @file
 * IPRT - Multiprocessor.
 */

/*
 * Copyright (C) 2008 Sun Microsystems, Inc.
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

#ifndef ___iprt_mp_h
#define ___iprt_mp_h

#include <iprt/cdefs.h>
#include <iprt/types.h>


__BEGIN_DECLS

/** @defgroup grp_rt_mp RTMp - Multiprocessor
 * @ingroup grp_rt
 * @{
 */

/**
 * Gets the identifier of the CPU executing the call.
 *
 * When called from a system mode where scheduling is active, like ring-3 or
 * kernel mode with interrupts enabled on some systems, no assumptions should
 * be made about the current CPU when the call returns.
 *
 * @returns CPU Id.
 */
RTDECL(RTCPUID) RTMpCpuId(void);

/**
 * Converts a CPU identifier to a CPU set index.
 *
 * This may or may not validate the precense of the CPU.
 *
 * @returns The CPU set index on success, -1 on failure.
 * @param   idCpu       The identifier of the CPU.
 */
RTDECL(int) RTMpCpuIdToSetIndex(RTCPUID idCpu);

/**
 * Converts a CPU set index to a a CPU identifier.
 *
 * This may or may not validate the precense of the CPU, so, use
 * RTMpIsCpuPossible for that.
 *
 * @returns The corresponding CPU identifier, NIL_RTCPUID on failure.
 * @param   iCpu    The CPU set index.
 */
RTDECL(RTCPUID) RTMpCpuIdFromSetIndex(int iCpu);

/**
 * Gets the max CPU identifier (inclusive).
 *
 * Inteded for brute force enumerations, but use with
 * care as it may be expensive.
 *
 * @returns The current higest CPU identifier value.
 */
RTDECL(RTCPUID) RTMpGetMaxCpuId(void);

/**
 * Checks if a CPU exists in the system or may possibly be hotplugged later.
 *
 * @returns true/false accordingly.
 * @param   idCpu       The identifier of the CPU.
 */
RTDECL(bool) RTMpIsCpuPossible(RTCPUID idCpu);

/**
 * Gets set of the CPUs present in the system pluss any that may
 * possibly be hotplugged later.
 *
 * @returns pSet.
 * @param   pSet    Where to put the set.
 */
RTDECL(PRTCPUSET) RTMpGetSet(PRTCPUSET pSet);

/**
 * Get the count of CPUs present in the system plus any that may
 * possibly be hotplugged later.
 *
 * @return The count.
 */
RTDECL(RTCPUID) RTMpGetCount(void);

/**
 * Gets set of the CPUs present that are currently online.
 *
 * @returns pSet.
 * @param   pSet    Where to put the set.
 */
RTDECL(PRTCPUSET) RTMpGetOnlineSet(PRTCPUSET pSet);

/**
 * Get the count of CPUs that are currently online.
 *
 * @return The count.
 */
RTDECL(RTCPUID) RTMpGetOnlineCount(void);

/**
 * Checks if a CPU is online or not.
 *
 * @returns true/false accordingly.
 * @param   idCpu       The identifier of the CPU.
 */
RTDECL(bool) RTMpIsCpuOnline(RTCPUID idCpu);


#ifdef IN_RING0

/**
 * Worker function passed to RTMpOnAll, RTMpOnOthers and RTMpOnSpecific that
 * is to be called on the target cpus.
 *
 * @param   idCpu       The identifier for the CPU the function is called on.
 * @param   pvUser1     The 1st user argument.
 * @param   pvUser2     The 2nd user argument.
 */
typedef DECLCALLBACK(void) FNRTMPWORKER(RTCPUID idCpu, void *pvUser1, void *pvUser2);
/** Pointer to a FNRTMPWORKER. */
typedef FNRTMPWORKER *PFNRTMPWORKER;

/**
 * Executes a function on each (online) CPU in the system.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_SUPPORTED if this kind of operation isn't supported by the system.
 *
 * @param   pfnWorker       The worker function.
 * @param   pvUser1         The first user argument for the worker.
 * @param   pvUser2         The second user argument for the worker.
 *
 * @remarks The execution isn't in any way guaranteed to be simultaneous,
 *          it might even be serial (cpu by cpu).
 */
RTDECL(int) RTMpOnAll(PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2);

/**
 * Executes a function on a all other (online) CPUs in the system.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_SUPPORTED if this kind of operation isn't supported by the system.
 *
 * @param   pfnWorker       The worker function.
 * @param   pvUser1         The first user argument for the worker.
 * @param   pvUser2         The second user argument for the worker.
 *
 * @remarks The execution isn't in any way guaranteed to be simultaneous,
 *          it might even be serial (cpu by cpu).
 */
RTDECL(int) RTMpOnOthers(PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2);

/**
 * Executes a function on a specific CPU in the system.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_SUPPORTED if this kind of operation isn't supported by the system.
 * @retval  VERR_CPU_OFFLINE if the CPU is offline.
 * @retval  VERR_CPU_NOT_FOUND if the CPU wasn't found.
 *
 * @param   idCpu           The id of the CPU.
 * @param   pfnWorker       The worker function.
 * @param   pvUser1         The first user argument for the worker.
 * @param   pvUser2         The second user argument for the worker.
 */
RTDECL(int) RTMpOnSpecific(RTCPUID idCpu, PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2);


/**
 * MP event, see FNRTMPNOTIFICATION.
 */
typedef enum RTMPEVENT
{
    /** The CPU goes online. */
    RTMPEVENT_ONLINE = 1,
    /** The CPU goes offline. */
    RTMPEVENT_OFFLINE
} RTMPEVENT;

/**
 * Notification callback.
 *
 * The context this is called in differs a bit from platform to
 * platform, so be careful while in here.
 *
 * @param   idCpu       The CPU this applies to.
 * @param   enmEvent    The event.
 * @param   pvUser      The user argument.
 */
typedef DECLCALLBACK(void) FNRTMPNOTIFICATION(RTMPEVENT enmEvent, RTCPUID idCpu, void *pvUser);
/** Pointer to a FNRTMPNOTIFICATION(). */
typedef FNRTMPNOTIFICATION *PFNRTMPNOTIFICATION;

/**
 * Registers a notification callback for cpu events.
 *
 * On platforms which doesn't do cpu offline/online events this API
 * will just be a no-op that pretends to work.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NO_MEMORY if a registration record cannot be allocated.
 * @retval  VERR_ALREADY_EXISTS if the pfnCallback and pvUser already exist
 *          in the callback list.
 *
 * @param   pfnCallback     The callback.
 * @param   pvUser          The user argument to the callback function.
 */
RTDECL(int) RTMpNotificationRegister(PFNRTMPNOTIFICATION pfnCallback, void *pvUser);

/**
 * This deregisters a notification callback registered via RTMpNotificationRegister().
 *
 * The pfnCallback and pvUser arguments must be identical to the registration call
 * of we won't find the right entry.
 *
 * @returns IPRT status code.
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_NOT_FOUND if no matching entry was found.
 *
 * @param   pfnCallback     The callback.
 * @param   pvUser          The user argument to the callback function.
 */
RTDECL(int) RTMpNotificationDeregister(PFNRTMPNOTIFICATION pfnCallback, void *pvUser);

/**
 * Initializes the multiprocessor event notifcations.
 *
 * This must be called before calling RTMpNotificationRegister(). This is an
 * inconvenice caused Visual C++ not implmenting weak externals.
 *
 * @returns IPRT status code.
 * @param   pvOS            Reserved, pass NULL.
 */
RTR0DECL(int) RTR0MpNotificationInit(void *pvOS);


/**
 * Terminates the multiprocessor event notifcations.
 *
 * The number of RTR0MpNotificationInit calls must match the calls to this
 * function exactly.
 *
 * @returns IPRT status code.
 * @param   pvOS            Reserved, pass NULL.
 */
RTR0DECL(void) RTR0MpNotificationTerm(void *pvOS);

#endif /* IN_RING0 */

/** @} */

__END_DECLS

#endif


/* $Id: mp-darwin.cpp 11810 2008-08-29 11:52:39Z vboxsync $ */
/** @file
 * IPRT - Multiprocessor, Darwin.
 */

/*
 * Copyright (C) 2006-2008 Sun Microsystems, Inc.
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
#define LOG_GROUP RTLOGGROUP_SYSTEM
#include <iprt/types.h>

#include <unistd.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <mach/mach.h>

#include <iprt/mp.h>
#include <iprt/cpuset.h>
#include <iprt/assert.h>
#include <iprt/string.h>


/**
 * Internal worker that determins the max possible CPU count.
 *
 * @returns Max cpus.
 */
static RTCPUID rtMpDarwinMaxCpus(void)
{
    int aiMib[2];
    aiMib[0] = CTL_HW;
    aiMib[1] = HW_NCPU;
    int cCpus = -1;
    size_t cb = sizeof(cCpus);
    int rc = sysctl(aiMib, ELEMENTS(aiMib), &cCpus, &cb, NULL, 0);
    if (rc != -1 && cCpus >= 1)
        return cCpus;
    AssertFailed();
    return 1;
}


/** @todo RTmpCpuId(). */

RTDECL(int) RTMpCpuIdToSetIndex(RTCPUID idCpu)
{
    return idCpu < rtMpDarwinMaxCpus() ? idCpu : -1;
}


RTDECL(RTCPUID) RTMpCpuIdFromSetIndex(int iCpu)
{
    return (unsigned)iCpu < rtMpDarwinMaxCpus() ? iCpu : NIL_RTCPUID;
}


RTDECL(RTCPUID) RTMpGetMaxCpuId(void)
{
    return rtMpDarwinMaxCpus() - 1;
}


RTDECL(bool) RTMpIsCpuOnline(RTCPUID idCpu)
{
#if 0
    return RTMpIsCpuPossible(idCpu);
#else
    /** @todo proper ring-3 support on darwin, see #3014. */
    natural_t nCpus;
    processor_basic_info_t pinfo;
    mach_msg_type_number_t count;
    kern_return_t krc = host_processor_info(mach_host_self(),
        PROCESSOR_BASIC_INFO, &nCpus, (processor_info_array_t*)&pinfo, &count);
    AssertReturn (krc == KERN_SUCCESS, true);
    bool isOnline = idCpu < nCpus ? pinfo[idCpu].running : true;
    vm_deallocate(mach_task_self(), (vm_address_t)pinfo, count * sizeof(*pinfo));
    return isOnline;
#endif
}


RTDECL(bool) RTMpIsCpuPossible(RTCPUID idCpu)
{
    return idCpu != NIL_RTCPUID
        && idCpu < rtMpDarwinMaxCpus();
}


RTDECL(PRTCPUSET) RTMpGetSet(PRTCPUSET pSet)
{
#if 0
    RTCPUID cCpus = rtMpDarwinMaxCpus();
    return RTCpuSetFromU64(RT_BIT_64(cCpus) - 1);

#else
    RTCpuSetEmpty(pSet);
    RTCPUID cMax = rtMpDarwinMaxCpus();
    for (RTCPUID idCpu = 0; idCpu < cMax; idCpu++)
        if (RTMpIsCpuPossible(idCpu))
            RTCpuSetAdd(pSet, idCpu);
    return pSet;
#endif
}


RTDECL(RTCPUID) RTMpGetCount(void)
{
    return rtMpDarwinMaxCpus();
}


RTDECL(PRTCPUSET) RTMpGetOnlineSet(PRTCPUSET pSet)
{
#if 0
    return RTMpGetSet(pSet);
#else
    RTCpuSetEmpty(pSet);
    RTCPUID cMax = rtMpDarwinMaxCpus();
    for (RTCPUID idCpu = 0; idCpu < cMax; idCpu++)
        if (RTMpIsCpuOnline(idCpu))
            RTCpuSetAdd(pSet, idCpu);
    return pSet;
#endif
}


RTDECL(RTCPUID) RTMpGetOnlineCount(void)
{
    RTCPUSET Set;
    RTMpGetOnlineSet(&Set);
    return RTCpuSetCount(&Set);
}


RTDECL(uint32_t) RTMpGetCurFrequency(RTCPUID idCpu)
{
    /** @todo figure out how to get the current cpu speed on darwin. Have to check what powermanagement does. */
    return 0;
}


RTDECL(uint32_t) RTMpGetMaxFrequency(RTCPUID idCpu)
{
    if (!RTMpIsCpuOnline(idCpu))
        return 0;

    /*
     * Try the 'hw.cpufrequency_max' one.
     */
    uint64_t CpuFrequencyMax = 0;
    size_t cb = sizeof(CpuFrequencyMax);
    int rc = sysctlbyname("hw.cpufrequency_max", &CpuFrequencyMax, &cb, NULL, 0);
    if (!rc)
        return (CpuFrequencyMax + 999999) / 1000000;

    /*
     * Use the depricated one.
     */
    int aiMib[2];
    aiMib[0] = CTL_HW;
    aiMib[1] = HW_CPU_FREQ;
    int cCpus = -1;
    cb = sizeof(cCpus);
    rc = sysctl(aiMib, ELEMENTS(aiMib), &cCpus, &cb, NULL, 0);
    if (rc != -1 && cCpus >= 1)
        return cCpus;
    AssertFailed();
    return 0;
}

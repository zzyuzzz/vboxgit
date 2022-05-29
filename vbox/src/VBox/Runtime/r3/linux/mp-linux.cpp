/* $Id: mp-linux.cpp 10446 2008-07-09 18:39:22Z vboxsync $ */
/** @file
 * IPRT - Multiprocessor, Linux.
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
#include <unistd.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>

#include <iprt/mp.h>
#include <iprt/cpuset.h>
#include <iprt/assert.h>
#include <iprt/string.h>


/** @todo move the rtLinuxSysFs* bits into sysfs.cpp and sysfs.h. */

/**
 * Checks if a sysfs file (or directory, device, symlink, whatever) exists.
 *
 * @returns true / false, errno is preserved.
 * @param   pszFormat   The name format, without "/sys/".
 * @param   va          The format args.
 */
bool rtLinuxSysFsExistsV(const char *pszFormat, va_list va)
{
    int iSavedErrno = errno;

    /*
     * Construct the filename and call stat.
     */
    char szFilename[128];
    static const size_t cchPrefix = sizeof("/sys/") - 1;
    strcpy(szFilename, "/sys/");
    size_t cch = RTStrPrintfV(&szFilename[cchPrefix], sizeof(szFilename) - cchPrefix, pszFormat, va);
    Assert(cch < sizeof(szFilename) - cchPrefix - 1);

    struct stat st;
    bool fRet = stat(szFilename, &st) == 0;

    errno = iSavedErrno;
    return fRet;
}

/**
 * Checks if a sysfs file (or directory, device, symlink, whatever) exists.
 *
 * @returns true / false, errno is preserved.
 * @param   pszFormat   The name format, without "/sys/".
 * @param   ...         The format args.
 */
bool rtLinuxSysFsExists(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    bool fRet = rtLinuxSysFsExistsV(pszFormat, va);
    va_end(va);
    return fRet;
}


/**
 * Opens a sysfs file.
 *
 * @returns The file descriptor. -1 and errno on failure.
 * @param   pszFormat   The name format, without "/sys/".
 * @param   va          The format args.
 */
int rtLinuxSysFsOpenV(const char *pszFormat, va_list va)
{
    /*
     * Construct the filename and call open.
     */
    char szFilename[128];
    static const size_t cchPrefix = sizeof("/sys/") - 1;
    strcpy(szFilename, "/sys/");
    size_t cch = RTStrPrintfV(&szFilename[cchPrefix], sizeof(szFilename) - cchPrefix, pszFormat, va);
    Assert(cch < sizeof(szFilename) - cchPrefix - 1);

    return open(szFilename, O_RDONLY, 0);
}


/**
 * Opens a sysfs file.
 *
 * @returns The file descriptor. -1 and errno on failure.
 * @param   pszFormat   The name format, without "/sys/".
 * @param   ...         The format args.
 */
int rtLinuxSysFsOpen(const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    int fd = rtLinuxSysFsOpenV(pszFormat, va);
    va_end(va);
    return fd;
}


/**
 * Closes a file opened with rtLinuxSysFsOpen or rtLinuxSysFsOpenV.
 *
 * @param   fd
 */
void rtLinuxSysFsClose(int fd)
{
    int iSavedErrno = errno;
    close(fd);
    errno = iSavedErrno;
}


/**
 * Closes a file opened with rtLinuxSysFsOpen or rtLinuxSysFsOpenV.
 *
 * @returns The number of bytes read. -1 and errno on failure.
 * @param   fd          The file descriptor returned by rtLinuxSysFsOpen or rtLinuxSysFsOpenV.
 * @param   pszBuf      Where to store the string.
 * @param   cchBuf      The size of the buffer. Must be at least 2 bytes.
 */
ssize_t rtLinuxSysFsReadStr(int fd, char *pszBuf, size_t cchBuf)
{
    Assert(cchBuf > 1);
    ssize_t cchRead = read(fd, pszBuf, cchBuf - 1);
    pszBuf[cchRead >= 0 ? cchRead : 0] = '\0';
    return cchRead;
}


/**
 * Reads a sysfs file.
 *
 * @returns 64-bit signed value on success, -1 and errno on failure.
 * @param   uBase       The number base, 0 for autodetect.
 * @param   pszFormat   The filename format, without "/sys/".
 * @param   va          Format args.
 */
int64_t rtLinuxSysFsReadIntFileV(unsigned uBase, const char *pszFormat, va_list va)
{
    int fd = rtLinuxSysFsOpenV(pszFormat, va);
    if (fd == -1)
        return -1;

    int64_t i64Ret = -1;
    char szNum[128];
    ssize_t cchNum = rtLinuxSysFsReadStr(fd, szNum, sizeof(szNum));
    if (cchNum > 0)
    {
        int rc = RTStrToInt64Ex(szNum, NULL, uBase, &i64Ret);
        if (RT_FAILURE(rc))
        {
            i64Ret = -1;
            errno = -ETXTBSY; /* just something that won't happen at read / open. */
        }
    }
    else if (cchNum == 0)
        errno = -ETXTBSY; /* just something that won't happen at read / open. */

    rtLinuxSysFsClose(fd);
    return i64Ret;
}


/**
 * Reads a sysfs file.
 *
 * @returns 64-bit signed value on success, -1 and errno on failure.
 * @param   uBase       The number base, 0 for autodetect.
 * @param   pszFormat   The filename format, without "/sys/".
 * @param   ...         Format args.
 */
static int64_t rtLinuxSysFsReadIntFile(unsigned uBase, const char *pszFormat, ...)
{
    va_list va;
    va_start(va, pszFormat);
    int64_t i64Ret = rtLinuxSysFsReadIntFileV(uBase, pszFormat, va);
    va_end(va);
    return i64Ret;
}


/**
 * Internal worker that determins the max possible CPU count.
 *
 * @returns Max cpus.
 */
static RTCPUID rtMpLinuxMaxCpus(void)
{
#if 0 /* this doesn't do the right thing :-/ */
    int cMax = sysconf(_SC_NPROCESSORS_CONF);
    Assert(cMax >= 1);
    return cMax;
#else
    static uint32_t s_cMax = 0;
    if (!s_cMax)
    {
        int cMax = 1;
        for (unsigned iCpu = 0; iCpu < RTCPUSET_MAX_CPUS; iCpu++)
            if (rtLinuxSysFsExists("devices/system/cpu/cpu%d", iCpu))
                cMax = iCpu + 1;
        ASMAtomicUoWriteU32((uint32_t volatile *)&s_cMax, cMax);
        return cMax;
    }
    return s_cMax;
#endif
}

/**
 * Internal worker that picks the processor speed in MHz from /proc/cpuinfo.
 *
 * @returns CPU frequency.
 */
static uint32_t rtMpLinuxGetFrequency(RTCPUID idCpu)
{
    FILE *f = fopen("/proc/cpuinfo", "r");
    if (!f)
        return 0;

    char sz[256];
    char *psz = NULL;
    RTCPUID idCpuFound = NIL_RTCPUID;
    uint32_t freq = 0;
    while (fgets(sz, sizeof(sz), f))
    {
        if (   !strncmp(sz, "processor", 9)
            && (sz[10] == ' ' || sz[10] == '\t' || sz[10] == ':')
            && (psz = strchr(sz, ':')))
        {
            psz += 2;
            int64_t iCpu;
            int rc = RTStrToInt64Ex(psz, NULL, 0, &iCpu);
            if (RT_SUCCESS(rc))
                idCpuFound = iCpu;
        }
        else if (   idCpu == idCpuFound
                 && !strncmp(sz, "cpu MHz", 7)
                 && (sz[10] == ' ' || sz[10] == '\t' || sz[10] == ':')
                 && (psz = strchr(sz, ':')))
        {
            psz += 2;
            int64_t v;
            int rc = RTStrToInt64Ex(psz, &psz, 0, &v);
            if (RT_SUCCESS(rc))
            {
                freq = v;
                break;
            }
        }
    }
    fclose(f);
    return freq;
}


/** @todo RTmpCpuId(). */

RTDECL(int) RTMpCpuIdToSetIndex(RTCPUID idCpu)
{
    return idCpu < rtMpLinuxMaxCpus() ? idCpu : -1;
}


RTDECL(RTCPUID) RTMpCpuIdFromSetIndex(int iCpu)
{
    return (unsigned)iCpu < rtMpLinuxMaxCpus() ? iCpu : NIL_RTCPUID;
}


RTDECL(RTCPUID) RTMpGetMaxCpuId(void)
{
    return rtMpLinuxMaxCpus() - 1;
}


RTDECL(bool) RTMpIsCpuOnline(RTCPUID idCpu)
{
    /** @todo check if there is a simpler interface than this... */
    int i = rtLinuxSysFsReadIntFile(0, "devices/system/cpu/cpu%d/online", (int)idCpu);
    if (    i == -1
        &&  rtLinuxSysFsExists("devices/system/cpu/cpu%d", (int)idCpu))
    {
        Assert(!rtLinuxSysFsExists("devices/system/cpu/cpu%d/online", (int)idCpu));
        i = 1;
    }

    Assert(i == 0 || i == -1 || i == 1);
    return i != 0 && i != -1;
}


RTDECL(bool) RTMpIsCpuPossible(RTCPUID idCpu)
{
    /** @todo check this up with hotplugging! */
    return rtLinuxSysFsExists("devices/system/cpu/cpu%d", (int)idCpu);
}


RTDECL(PRTCPUSET) RTMpGetSet(PRTCPUSET pSet)
{
    RTCpuSetEmpty(pSet);
    RTCPUID cMax = rtMpLinuxMaxCpus();
    for (RTCPUID idCpu = 0; idCpu < cMax; idCpu++)
        if (RTMpIsCpuPossible(idCpu))
            RTCpuSetAdd(pSet, idCpu);
    return pSet;
}


RTDECL(RTCPUID) RTMpGetCount(void)
{
    RTCPUSET Set;
    RTMpGetSet(&Set);
    return RTCpuSetCount(&Set);
}


RTDECL(PRTCPUSET) RTMpGetOnlineSet(PRTCPUSET pSet)
{
    RTCpuSetEmpty(pSet);
    RTCPUID cMax = rtMpLinuxMaxCpus();
    for (RTCPUID idCpu = 0; idCpu < cMax; idCpu++)
        if (RTMpIsCpuOnline(idCpu))
            RTCpuSetAdd(pSet, idCpu);
    return pSet;
}


RTDECL(RTCPUID) RTMpGetOnlineCount(void)
{
    RTCPUSET Set;
    RTMpGetOnlineSet(&Set);
    return RTCpuSetCount(&Set);
}


RTDECL(uint32_t) RTMpGetCurFrequency(RTCPUID idCpu)
{
    int64_t kHz = rtLinuxSysFsReadIntFile(0, "devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq", (int)idCpu);
    if (kHz == -1)
    {
        /* The file may be just unreadable - in that case use plan B, i.e.
         * /proc/cpuinfo to get the data we want. The assumption is that if
         * cpuinfo_cur_freq doesn't exist then the speed won't change, and
         * thus cur == max. If it does exist then cpuinfo contains the
         * current frequency. */
        kHz = rtMpLinuxGetFrequency(idCpu) * 1000;
    }
    return (kHz + 999) / 1000;
}


RTDECL(uint32_t) RTMpGetMaxFrequency(RTCPUID idCpu)
{
    int64_t kHz = rtLinuxSysFsReadIntFile(0, "devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", (int)idCpu);
    if (kHz == -1)
    {
        /* Check if the file isn't there - if it is there, then /proc/cpuinfo
         * would provide current frequency information, which is wrong. */
        if (!rtLinuxSysFsExists("devices/system/cpu/cpu%d/cpufreq/cpuinfo_max_freq", (int)idCpu))
            kHz = rtMpLinuxGetFrequency(idCpu) * 1000;
        else
            kHz = 0;
    }
    return (kHz + 999) / 1000;
}

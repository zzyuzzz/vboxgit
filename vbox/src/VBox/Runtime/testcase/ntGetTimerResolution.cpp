/* $Id: ntGetTimerResolution.cpp 9950 2008-06-26 11:56:57Z vboxsync $ */
/** @file
 * IPRT - Win32 (NT) testcase for getting the timer resolution.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
#define _WIN32_WINNT 0x0500
#include <Windows.h>
#include <stdio.h>

extern "C" {
/* from sysinternals. */
NTSYSAPI LONG NTAPI NtQueryTimerResolution(OUT PULONG MaximumResolution, OUT PULONG MinimumResolution, OUT PULONG CurrentResolution);
}


int main()
{
    ULONG Min = ~0;
    ULONG Max = ~0;
    ULONG Cur = ~0;
    NtQueryTimerResolution(&Max, &Min, &Cur);
    printf("NtQueryTimerResolution -> Max=%lu00ns Min=%lu00ns Cur=%lu00ns\n", Min, Max, Cur);

#if 0
    /* figure out the 100ns relative to the 1970 epoc. */
    SYSTEMTIME st;
    st.wYear = 1970;
    st.wMonth = 1;
    st.wDayOfWeek = 4; /* Thor's day. */
    st.wDay = 1;
    st.wHour = 0;
    st.wMinute = 0;
    st.wSecond = 0;
    st.wMilliseconds = 0;

    FILETIME ft;
    if (SystemTimeToFileTime(&st, &ft))
    {
        printf("epoc is %I64u (0x%08x%08x)\n", ft, ft.dwHighDateTime, ft.dwLowDateTime);
        if (FileTimeToSystemTime(&ft, &st))
            printf("unix epoc: %d-%02d-%02d %02d:%02d:%02d.%03d (week day %d)\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, st.wDayOfWeek);
        else
            printf("FileTimeToSystemTime failed, lasterr=%d\n", GetLastError());
    }
    else
        printf("SystemTimeToFileTime failed, lasterr=%d\n", GetLastError());

    ft.dwHighDateTime = 0;
    ft.dwLowDateTime = 0;
    if (FileTimeToSystemTime(&ft, &st))
        printf("nt time start: %d-%02d-%02d %02d:%02d:%02d.%03d (week day %d)\n",
            st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, st.wDayOfWeek);
    else
        printf("FileTimeToSystemTime failed, lasterr=%d\n", GetLastError());
#endif
    return 0;
}


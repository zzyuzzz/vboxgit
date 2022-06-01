/* $Id: RTTimeSet-posix.cpp 26210 2010-02-03 16:54:53Z vboxsync $ */
/** @file
 * IPRT - RTTimeSet, POSIX.
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
#define LOG_GROUP RTLOGGROUP_TIME
#define RTTIME_INCL_TIMEVAL
#include <sys/time.h>
#include <time.h>
#include <errno.h>

#include <iprt/time.h>
#include "internal/iprt.h"

#include <iprt/err.h>


RTDECL(int) RTTimeSet(PCRTTIMESPEC pTime)
{
    struct timeval tv;
    if (settimeofday(RTTimeSpecGetTimeval(pTime, &tv), NULL) == 0)
        return VINF_SUCCESS;
    return RTErrConvertFromErrno(errno);
}


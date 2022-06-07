/* $Id: rtProcInitExePath-os2.cpp 57358 2015-08-14 15:16:38Z vboxsync $ */
/** @file
 * IPRT - rtProcInitName, OS/2.
 */

/*
 * Copyright (C) 2006-2015 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP RTLOGGROUP_PROCESS
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/path.h>
#include "internal/process.h"
#include "internal/path.h"


DECLHIDDEN(int) rtProcInitExePath(char *pszPath, size_t cchPath)
{
    /*
     * Query the image name from the dynamic linker, convert and return it.
     */
    _execname(pszPath, cchPath);

    char const *pszTmp;
    int rc = rtPathFromNative(&pszTmp, pszPath, NULL);
    AssertMsgRCReturn(rc, ("rc=%Rrc pszLink=\"%s\"\nhex: %.*Rhxs\n", rc, pszPath, cchPath, pszPath), rc);
    if (pszTmp != pszPath)
    {
        rc = RTStrCopy(pszPath, cchPath, pszTmp);
        rtPathFreeIprt(pszTmp, pszPath);
    }
    return rc;
}


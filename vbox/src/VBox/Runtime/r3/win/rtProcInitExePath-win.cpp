/* $Id: rtProcInitExePath-win.cpp 11838 2008-08-29 17:14:02Z vboxsync $ */
/** @file
 * IPRT - rtProcInitName, Windows.
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
#define LOG_GROUP RTLOGGROUP_PROCESS
#include <Windows.h>

#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include "internal/process.h"


DECLHIDDEN(int) rtProcInitExePath(char *pszPath, size_t cchPath)
{
    /*
     * Query the image name from the dynamic linker, convert and return it.
     */
    WCHAR  wsz[RTPATH_MAX];
    HMODULE hExe = GetModuleHandle(NULL);
    if (GetModuleFileNameW(hExe, wsz, RTPATH_MAX))
    {
        int rc = RTUtf16ToUtf8Ex(wsz, RTSTR_MAX, &pszPath, cchPath, NULL);
        AssertRCReturn(rc, rc);
        return VINF_SUCCESS;
    }

    DWORD err = GetLastError();
    int rc = RTErrConvertFromWin32(err);
    AssertMsgFailed(("%Rrc %d\n", rc, err));
    return rc;
}


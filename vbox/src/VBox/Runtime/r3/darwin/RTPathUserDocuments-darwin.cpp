/* $Id: RTPathUserDocuments-darwin.cpp 85163 2020-07-10 09:53:41Z vboxsync $ */
/** @file
 * IPRT - RTPathUserDocuments, darwin ring-3.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
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
#include <iprt/path.h>
#include "internal/iprt.h"

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/err.h>

#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
# include <sysdir.h>
#else
# include <NSSystemDirectories.h>
#endif
#include <sys/syslimits.h>
#ifdef IPRT_USE_CORE_SERVICE_FOR_USER_DOCUMENTS
# include <CoreServices/CoreServices.h>
#endif


RTDECL(int) RTPathUserDocuments(char *pszPath, size_t cchPath)
{
    /*
     * Validate input
     */
    AssertPtrReturn(pszPath, VERR_INVALID_POINTER);
    AssertReturn(cchPath, VERR_INVALID_PARAMETER);

    /*
     * Try NSSystemDirectories first since that works for directories that doesn't exist.
     * The NSSystemDirectories API was renamed in 10.12 to sysdir.
     */
    int rc = VERR_PATH_NOT_FOUND;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
    sysdir_search_path_enumeration_state EnmState = sysdir_start_search_path_enumeration(SYSDIR_DIRECTORY_DOCUMENT,
                                                                                         SYSDIR_DOMAIN_MASK_USER);
#else
    NSSearchPathEnumerationState EnmState = NSStartSearchPathEnumeration(NSDocumentDirectory, NSUserDomainMask);
#endif
    if (EnmState != 0)
    {
        char szTmp[PATH_MAX];
        szTmp[0] = szTmp[PATH_MAX - 1] = '\0';
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 101200
        EnmState = sysdir_get_next_search_path_enumeration(EnmState, szTmp);
#else
        EnmState = NSGetNextSearchPathEnumeration(EnmState, szTmp);
#endif
        if (EnmState != 0)
        {
            size_t cchTmp = strlen(szTmp);
            if (cchTmp >= cchPath)
                return VERR_BUFFER_OVERFLOW;

            if (szTmp[0] == '~' && szTmp[1] == '/')
            {
                /* Expand tilde. */
                rc = RTPathUserHome(pszPath, cchPath - cchTmp + 2);
                if (RT_FAILURE(rc))
                    return rc;
                rc = RTPathAppend(pszPath, cchPath, &szTmp[2]);
            }
            else
                rc = RTStrCopy(pszPath, cchPath, szTmp);
            return rc;
        }
    }

#ifdef IPRT_USE_CORE_SERVICE_FOR_USER_DOCUMENTS
    /*
     * Fall back on FSFindFolder in case the above should fail...
     */
    FSRef ref;
    OSErr err = FSFindFolder(kOnAppropriateDisk, kDocumentsFolderType, false /* createFolder */, &ref);
    if (err == noErr)
    {
        err = FSRefMakePath(&ref, (UInt8*)pszPath, cchPath);
        if (err == noErr)
            return VINF_SUCCESS;
    }
#endif
    Assert(RT_FAILURE_NP(rc));
    return rc;
}


/* $Id: ClipboardProvider-HostService.cpp 78942 2019-06-03 19:10:19Z vboxsync $ */
/** @file
 * Shared Clipboard - Provider implementation for host service (host side).
 */

/*
 * Copyright (C) 2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SHARED_CLIPBOARD
#include <VBox/GuestHost/SharedClipboard-uri.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/errcore.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/string.h>

#include <VBox/log.h>


SharedClipboardProviderHostService::SharedClipboardProviderHostService(void)
{
    LogFlowFuncEnter();
}

SharedClipboardProviderHostService::~SharedClipboardProviderHostService(void)
{
}

int SharedClipboardProviderHostService::ReadMetaData(uint32_t fFlags /* = 0 */)
{
    RT_NOREF(fFlags);
    return VERR_NOT_IMPLEMENTED;
}

int SharedClipboardProviderHostService::WriteMetaData(const void *pvBuf, size_t cbBuf, size_t *pcbWritten, uint32_t fFlags /* = 0 */)
{
    RT_NOREF(pvBuf, cbBuf, pcbWritten, fFlags);
    return VERR_NOT_IMPLEMENTED;
}

int SharedClipboardProviderVbglR3::ReadDirectory(PVBOXCLIPBOARDDIRDATA pDirData)
{
    RT_NOREF(pDirData);

    LogFlowFuncEnter();

    int rc = VERR_NOT_IMPLEMENTED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteDirectory(const PVBOXCLIPBOARDDIRDATA pDirData)
{
    RT_NOREF(pDirData);

    LogFlowFuncEnter();

    int rc = VERR_NOT_IMPLEMENTED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::ReadFileHdr(PVBOXCLIPBOARDFILEHDR pFileHdr)
{
    RT_NOREF(pFileHdr);

    LogFlowFuncEnter();

    int rc = VERR_NOT_IMPLEMENTED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteFileHdr(const PVBOXCLIPBOARDFILEHDR pFileHdr)
{
    RT_NOREF(pFileHdr);

    LogFlowFuncEnter();

    int rc = VERR_NOT_IMPLEMENTED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::ReadFileData(PVBOXCLIPBOARDFILEDATA pFileData)
{
    RT_NOREF(pFileData);

    LogFlowFuncEnter();

    int rc = VERR_NOT_IMPLEMENTED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteFileData(const PVBOXCLIPBOARDFILEDATA pFileData)
{
    RT_NOREF(pFileData);

    LogFlowFuncEnter();

    int rc = VERR_NOT_IMPLEMENTED;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

void SharedClipboardProviderHostService::Reset(void)
{
}


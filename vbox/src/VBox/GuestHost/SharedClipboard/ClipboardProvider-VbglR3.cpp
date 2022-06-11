/* $Id: ClipboardProvider-VbglR3.cpp 79120 2019-06-13 10:08:33Z vboxsync $ */
/** @file
 * Shared Clipboard - Provider implementation for VbglR3 (guest side).
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
#include <VBox/VBoxGuestLib.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/dir.h>
#include <iprt/errcore.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/string.h>

#include <VBox/log.h>


SharedClipboardProviderVbglR3::SharedClipboardProviderVbglR3(uint32_t uClientID)
    : m_uClientID(uClientID)
{
    LogFlowFunc(("m_uClientID=%RU32\n", m_uClientID));
}

SharedClipboardProviderVbglR3::~SharedClipboardProviderVbglR3(void)
{
}

int SharedClipboardProviderVbglR3::ReadDataHdr(PVBOXCLIPBOARDDATAHDR pDataHdr)
{
    LogFlowFuncEnter();

    int rc = VbglR3ClipboardReadDataHdr(m_uClientID, pDataHdr);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteDataHdr(const PVBOXCLIPBOARDDATAHDR pDataHdr)
{
    LogFlowFuncEnter();

    int rc = VbglR3ClipboardWriteDataHdr(m_uClientID, pDataHdr);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::ReadDataChunkk(const PVBOXCLIPBOARDDATAHDR pDataHdr, void *pvChunk, uint32_t cbChunk,
                                                  uint32_t *pcbRead, uint32_t fFlags /* = 0 */)
{
    RT_NOREF(fFlags);

    LogFlowFuncEnter();

    int rc = VINF_SUCCESS;

    uint32_t cbReadTotal = 0;
    uint32_t cbToRead = RT_MIN(pDataHdr->cbMeta, cbChunk);

    while (cbToRead)
    {
        uint32_t cbRead;
        rc = VbglR3ClipboardReadMetaData(m_uClientID, pDataHdr, (uint8_t *)pvChunk + cbReadTotal, cbToRead, &cbRead);
        if (RT_FAILURE(rc))
            break;

        cbReadTotal += cbRead;
        Assert(cbToRead >= cbRead);
        cbToRead    -= cbRead;
    }

    if (RT_SUCCESS(rc))
    {
        if (pcbRead)
            *pcbRead = cbReadTotal;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteDataChunk(const PVBOXCLIPBOARDDATAHDR pDataHdr, const void *pvChunk, uint32_t cbChunk,
                                                  uint32_t *pcbWritten, uint32_t fFlags /* = 0 */)
{
    RT_NOREF(fFlags);

    LogFlowFuncEnter();

    int rc = VINF_SUCCESS;

    uint32_t cbWrittenTotal = 0;
    uint32_t cbToWrite      = RT_MIN(pDataHdr->cbMeta, cbChunk);

    while (cbToWrite)
    {
        uint32_t cbWritten;
        rc = VbglR3ClipboardWriteMetaData(m_uClientID, pDataHdr, (uint8_t *)pvChunk + cbWrittenTotal, cbToWrite, &cbWritten);
        if (RT_FAILURE(rc))
            break;

        cbWrittenTotal += cbWritten;
        Assert(cbToWrite >= cbWritten);
        cbToWrite      -= cbWritten;
    }

    if (RT_SUCCESS(rc))
    {
        if (pcbWritten)
            *pcbWritten = cbWrittenTotal;
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::ReadDirectory(PVBOXCLIPBOARDDIRDATA pDirData)
{
    LogFlowFuncEnter();

    int rc = VbglR3ClipboardReadDir(m_uClientID, pDirData->pszPath, pDirData->cbPath, &pDirData->cbPath, &pDirData->fMode);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteDirectory(const PVBOXCLIPBOARDDIRDATA pDirData)
{
    LogFlowFuncEnter();

    int rc = VbglR3ClipboardWriteDir(m_uClientID, pDirData->pszPath, pDirData->cbPath, pDirData->fMode);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::ReadFileHdr(PVBOXCLIPBOARDFILEHDR pFileHdr)
{
    LogFlowFuncEnter();

    int rc = VbglR3ClipboardReadFileHdr(m_uClientID, pFileHdr->pszFilePath, pFileHdr->cbFilePath,
                                        &pFileHdr->fFlags, &pFileHdr->fMode, &pFileHdr->cbSize);
    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteFileHdr(const PVBOXCLIPBOARDFILEHDR pFileHdr)
{
    LogFlowFuncEnter();

    int rc = VbglR3ClipboardWriteFileHdr(m_uClientID, pFileHdr->pszFilePath, pFileHdr->cbFilePath,
                                         pFileHdr->fFlags, pFileHdr->fMode, pFileHdr->cbSize);
    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::ReadFileData(PVBOXCLIPBOARDFILEDATA pFileData, uint32_t *pcbRead)
{
    AssertPtrReturn(pcbRead, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    int rc = VbglR3ClipboardReadFileData(m_uClientID, pFileData->pvData, pFileData->cbData, pcbRead);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int SharedClipboardProviderVbglR3::WriteFileData(const PVBOXCLIPBOARDFILEDATA pFileData, uint32_t *pcbWritten)
{
    AssertPtrReturn(pcbWritten, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    int rc = VbglR3ClipboardWriteFileData(m_uClientID, pFileData->pvData, pFileData->cbData, pcbWritten);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

void SharedClipboardProviderVbglR3::Reset(void)
{
    LogFlowFuncEnter();

    /* Don't clear the refcount here. */
}


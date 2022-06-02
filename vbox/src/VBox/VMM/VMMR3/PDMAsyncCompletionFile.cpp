/* $Id: PDMAsyncCompletionFile.cpp 35696 2011-01-24 18:03:33Z vboxsync $ */
/** @file
 * PDM Async I/O - Transport data asynchronous in R3 using EMT.
 */

/*
 * Copyright (C) 2006-2009 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_PDM_ASYNC_COMPLETION
#include "PDMInternal.h"
#include <VBox/vmm/pdm.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/dbg.h>
#include <VBox/vmm/uvm.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/critsect.h>
#include <iprt/env.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/thread.h>
#include <iprt/path.h>

#if defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS) || defined(RT_OS_FREEBSD)
# include <errno.h>
# include <sys/ioctl.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#endif

#ifdef RT_OS_WINDOWS
# define _WIN32_WINNT 0x0500
# include <windows.h>
# include <winioctl.h>
#endif
#ifdef RT_OS_DARWIN
# include <sys/disk.h>
#endif /* RT_OS_DARWIN */
#ifdef RT_OS_SOLARIS
# include <stropts.h>
# include <sys/dkio.h>
# include <sys/vtoc.h>
#endif /* RT_OS_SOLARIS */
#ifdef RT_OS_FREEBSD
# include <sys/disk.h>
#endif /* RT_OS_FREEBSD */

#include "PDMAsyncCompletionFileInternal.h"


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#ifdef VBOX_WITH_DEBUGGER
static DECLCALLBACK(int) pdmacEpFileErrorInject(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR pArgs, unsigned cArgs);
#endif

/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
#ifdef VBOX_WITH_DEBUGGER
static const DBGCVARDESC g_aInjectErrorArgs[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "direction",    "write/read." },
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "filename",     "Filename." },
    {  1,           1,          DBGCVAR_CAT_NUMBER,     0,                              "errcode",      "VBox status code." },
};

/** Command descriptors. */
static const DBGCCMD g_aCmds[] =
{
    /* pszCmd,       cArgsMin, cArgsMax, paArgDesc,                cArgDescs, fFlags, pfnHandler              pszSyntax, ....pszDescription */
    { "injecterror",        3, 3,        &g_aInjectErrorArgs[0],           3,      0, pdmacEpFileErrorInject, "",        "Inject error into I/O subsystem." },
};
#endif

/**
 * Frees a task.
 *
 * @returns nothing.
 * @param   pEndpoint    Pointer to the endpoint the segment was for.
 * @param   pTask        The task to free.
 */
void pdmacFileTaskFree(PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint,
                       PPDMACTASKFILE pTask)
{
    PPDMASYNCCOMPLETIONEPCLASSFILE pEpClass = (PPDMASYNCCOMPLETIONEPCLASSFILE)pEndpoint->Core.pEpClass;

    LogFlowFunc((": pEndpoint=%p pTask=%p\n", pEndpoint, pTask));

    /* Try the per endpoint cache first. */
    if (pEndpoint->cTasksCached < pEpClass->cTasksCacheMax)
    {
        /* Add it to the list. */
        pEndpoint->pTasksFreeTail->pNext = pTask;
        pEndpoint->pTasksFreeTail        = pTask;
        ASMAtomicIncU32(&pEndpoint->cTasksCached);
    }
    else
    {
        Log(("Freeing task %p because all caches are full\n", pTask));
        MMR3HeapFree(pTask);
    }
}

/**
 * Allocates a task segment
 *
 * @returns Pointer to the new task segment or NULL
 * @param   pEndpoint    Pointer to the endpoint
 */
PPDMACTASKFILE pdmacFileTaskAlloc(PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint)
{
    PPDMACTASKFILE pTask = NULL;

    /* Try the small per endpoint cache first. */
    if (pEndpoint->pTasksFreeHead == pEndpoint->pTasksFreeTail)
    {
        /* Try the bigger endpoint class cache. */
        PPDMASYNCCOMPLETIONEPCLASSFILE pEndpointClass = (PPDMASYNCCOMPLETIONEPCLASSFILE)pEndpoint->Core.pEpClass;

        /*
         * Allocate completely new.
         * If this fails we return NULL.
         */
        int rc = MMR3HeapAllocZEx(pEndpointClass->Core.pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                                  sizeof(PDMACTASKFILE),
                                  (void **)&pTask);
        if (RT_FAILURE(rc))
            pTask = NULL;

        LogFlow(("Allocated task %p\n", pTask));
    }
    else
    {
        /* Grab a free task from the head. */
        AssertMsg(pEndpoint->cTasksCached > 0, ("No tasks cached but list contains more than one element\n"));

        pTask = pEndpoint->pTasksFreeHead;
        pEndpoint->pTasksFreeHead = pTask->pNext;
        ASMAtomicDecU32(&pEndpoint->cTasksCached);
    }

    pTask->pNext = NULL;

    return pTask;
}

PPDMACTASKFILE pdmacFileEpGetNewTasks(PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint)
{
    PPDMACTASKFILE pTasks = NULL;

    /*
     * Get pending tasks.
     */
    pTasks = ASMAtomicXchgPtrT(&pEndpoint->pTasksNewHead, NULL, PPDMACTASKFILE);

    /* Reverse the list to process in FIFO order. */
    if (pTasks)
    {
        PPDMACTASKFILE pTask = pTasks;

        pTasks = NULL;

        while (pTask)
        {
            PPDMACTASKFILE pCur = pTask;
            pTask = pTask->pNext;
            pCur->pNext = pTasks;
            pTasks = pCur;
        }
    }

    return pTasks;
}

static void pdmacFileAioMgrWakeup(PPDMACEPFILEMGR pAioMgr)
{
    bool fWokenUp = ASMAtomicXchgBool(&pAioMgr->fWokenUp, true);

    if (!fWokenUp)
    {
        int rc = VINF_SUCCESS;
        bool fWaitingEventSem = ASMAtomicReadBool(&pAioMgr->fWaitingEventSem);

        if (fWaitingEventSem)
            rc = RTSemEventSignal(pAioMgr->EventSem);

        AssertRC(rc);
    }
}

static int pdmacFileAioMgrWaitForBlockingEvent(PPDMACEPFILEMGR pAioMgr, PDMACEPFILEAIOMGRBLOCKINGEVENT enmEvent)
{
    int rc = VINF_SUCCESS;

    ASMAtomicWriteU32((volatile uint32_t *)&pAioMgr->enmBlockingEvent, enmEvent);
    Assert(!pAioMgr->fBlockingEventPending);
    ASMAtomicXchgBool(&pAioMgr->fBlockingEventPending, true);

    /* Wakeup the async I/O manager */
    pdmacFileAioMgrWakeup(pAioMgr);

    /* Wait for completion. */
    rc = RTSemEventWait(pAioMgr->EventSemBlock, RT_INDEFINITE_WAIT);
    AssertRC(rc);

    ASMAtomicXchgBool(&pAioMgr->fBlockingEventPending, false);
    ASMAtomicWriteU32((volatile uint32_t *)&pAioMgr->enmBlockingEvent, PDMACEPFILEAIOMGRBLOCKINGEVENT_INVALID);

    return rc;
}

int pdmacFileAioMgrAddEndpoint(PPDMACEPFILEMGR pAioMgr, PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint)
{
    int rc;

    LogFlowFunc(("pAioMgr=%#p pEndpoint=%#p{%s}\n", pAioMgr, pEndpoint, pEndpoint->Core.pszUri));

    /* Update the assigned I/O manager. */
    ASMAtomicWritePtr(&pEndpoint->pAioMgr, pAioMgr);

    rc = RTCritSectEnter(&pAioMgr->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    ASMAtomicWritePtr(&pAioMgr->BlockingEventData.AddEndpoint.pEndpoint, pEndpoint);
    rc = pdmacFileAioMgrWaitForBlockingEvent(pAioMgr, PDMACEPFILEAIOMGRBLOCKINGEVENT_ADD_ENDPOINT);
    ASMAtomicWriteNullPtr(&pAioMgr->BlockingEventData.AddEndpoint.pEndpoint);

    RTCritSectLeave(&pAioMgr->CritSectBlockingEvent);

    return rc;
}

static int pdmacFileAioMgrRemoveEndpoint(PPDMACEPFILEMGR pAioMgr, PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint)
{
    int rc;

    rc = RTCritSectEnter(&pAioMgr->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    ASMAtomicWritePtr(&pAioMgr->BlockingEventData.RemoveEndpoint.pEndpoint, pEndpoint);
    rc = pdmacFileAioMgrWaitForBlockingEvent(pAioMgr, PDMACEPFILEAIOMGRBLOCKINGEVENT_REMOVE_ENDPOINT);
    ASMAtomicWriteNullPtr(&pAioMgr->BlockingEventData.RemoveEndpoint.pEndpoint);

    RTCritSectLeave(&pAioMgr->CritSectBlockingEvent);

    return rc;
}

static int pdmacFileAioMgrCloseEndpoint(PPDMACEPFILEMGR pAioMgr, PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint)
{
    int rc;

    rc = RTCritSectEnter(&pAioMgr->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    ASMAtomicWritePtr(&pAioMgr->BlockingEventData.CloseEndpoint.pEndpoint, pEndpoint);
    rc = pdmacFileAioMgrWaitForBlockingEvent(pAioMgr, PDMACEPFILEAIOMGRBLOCKINGEVENT_CLOSE_ENDPOINT);
    ASMAtomicWriteNullPtr(&pAioMgr->BlockingEventData.CloseEndpoint.pEndpoint);

    RTCritSectLeave(&pAioMgr->CritSectBlockingEvent);

    return rc;
}

static int pdmacFileAioMgrShutdown(PPDMACEPFILEMGR pAioMgr)
{
    int rc;

    rc = RTCritSectEnter(&pAioMgr->CritSectBlockingEvent);
    AssertRCReturn(rc, rc);

    rc = pdmacFileAioMgrWaitForBlockingEvent(pAioMgr, PDMACEPFILEAIOMGRBLOCKINGEVENT_SHUTDOWN);

    RTCritSectLeave(&pAioMgr->CritSectBlockingEvent);

    return rc;
}

int pdmacFileEpAddTask(PPDMASYNCCOMPLETIONENDPOINTFILE pEndpoint, PPDMACTASKFILE pTask)
{
    PPDMACTASKFILE pNext;
    do
    {
        pNext = pEndpoint->pTasksNewHead;
        pTask->pNext = pNext;
    } while (!ASMAtomicCmpXchgPtr(&pEndpoint->pTasksNewHead, pTask, pNext));

    pdmacFileAioMgrWakeup(ASMAtomicReadPtrT(&pEndpoint->pAioMgr, PPDMACEPFILEMGR));

    return VINF_SUCCESS;
}

void pdmacFileEpTaskCompleted(PPDMACTASKFILE pTask, void *pvUser, int rc)
{
    PPDMASYNCCOMPLETIONTASKFILE pTaskFile = (PPDMASYNCCOMPLETIONTASKFILE)pvUser;

    LogFlowFunc(("pTask=%#p pvUser=%#p rc=%Rrc\n", pTask, pvUser, rc));

    if (pTask->enmTransferType == PDMACTASKFILETRANSFER_FLUSH)
    {
        pdmR3AsyncCompletionCompleteTask(&pTaskFile->Core, rc, true);
    }
    else
    {
        Assert((uint32_t)pTask->DataSeg.cbSeg == pTask->DataSeg.cbSeg && (int32_t)pTask->DataSeg.cbSeg >= 0);
        uint32_t uOld = ASMAtomicSubS32(&pTaskFile->cbTransferLeft, (int32_t)pTask->DataSeg.cbSeg);

        /* The first error will be returned. */
        if (RT_FAILURE(rc))
            ASMAtomicCmpXchgS32(&pTaskFile->rc, rc, VINF_SUCCESS);
#ifdef VBOX_WITH_DEBUGGER
        else
        {
            PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pTaskFile->Core.pEndpoint;

            /* Overwrite with injected error code. */
            if (pTask->enmTransferType == PDMACTASKFILETRANSFER_READ)
                rc = ASMAtomicXchgS32(&pEpFile->rcReqRead, VINF_SUCCESS);
            else
                rc = ASMAtomicXchgS32(&pEpFile->rcReqWrite, VINF_SUCCESS);

            if (RT_FAILURE(rc))
                ASMAtomicCmpXchgS32(&pTaskFile->rc, rc, VINF_SUCCESS);
        }
#endif

        if (!(uOld - pTask->DataSeg.cbSeg)
            && !ASMAtomicXchgBool(&pTaskFile->fCompleted, true))
            pdmR3AsyncCompletionCompleteTask(&pTaskFile->Core, pTaskFile->rc, true);
    }
}

DECLINLINE(void) pdmacFileEpTaskInit(PPDMASYNCCOMPLETIONTASK pTask, size_t cbTransfer)
{
    PPDMASYNCCOMPLETIONTASKFILE pTaskFile = (PPDMASYNCCOMPLETIONTASKFILE)pTask;

    Assert((uint32_t)cbTransfer == cbTransfer && (int32_t)cbTransfer >= 0);
    ASMAtomicWriteS32(&pTaskFile->cbTransferLeft, (int32_t)cbTransfer);
    ASMAtomicWriteBool(&pTaskFile->fCompleted, false);
    ASMAtomicWriteS32(&pTaskFile->rc, VINF_SUCCESS);
}

int pdmacFileEpTaskInitiate(PPDMASYNCCOMPLETIONTASK pTask,
                            PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                            PCRTSGSEG paSegments, size_t cSegments,
                            size_t cbTransfer, PDMACTASKFILETRANSFER enmTransfer)
{
    int rc = VINF_SUCCESS;
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;
    PPDMASYNCCOMPLETIONTASKFILE pTaskFile = (PPDMASYNCCOMPLETIONTASKFILE)pTask;
    PPDMACEPFILEMGR pAioMgr = pEpFile->pAioMgr;

    Assert(   (enmTransfer == PDMACTASKFILETRANSFER_READ)
           || (enmTransfer == PDMACTASKFILETRANSFER_WRITE));

    for (unsigned i = 0; i < cSegments; i++)
    {
        PPDMACTASKFILE pIoTask = pdmacFileTaskAlloc(pEpFile);
        AssertPtr(pIoTask);

        pIoTask->pEndpoint       = pEpFile;
        pIoTask->enmTransferType = enmTransfer;
        pIoTask->Off             = off;
        pIoTask->DataSeg.cbSeg   = paSegments[i].cbSeg;
        pIoTask->DataSeg.pvSeg   = paSegments[i].pvSeg;
        pIoTask->pvUser          = pTaskFile;
        pIoTask->pfnCompleted    = pdmacFileEpTaskCompleted;

        /* Send it off to the I/O manager. */
        pdmacFileEpAddTask(pEpFile, pIoTask);
        off        += paSegments[i].cbSeg;
        cbTransfer -= paSegments[i].cbSeg;
    }

    AssertMsg(!cbTransfer, ("Incomplete transfer %u bytes left\n", cbTransfer));

    if (ASMAtomicReadS32(&pTaskFile->cbTransferLeft) == 0
        && !ASMAtomicXchgBool(&pTaskFile->fCompleted, true))
        pdmR3AsyncCompletionCompleteTask(pTask, pTaskFile->rc, false);
    else
        rc = VINF_AIO_TASK_PENDING;

    return rc;
}

/**
 * Creates a new async I/O manager.
 *
 * @returns VBox status code.
 * @param   pEpClass    Pointer to the endpoint class data.
 * @param   ppAioMgr    Where to store the pointer to the new async I/O manager on success.
 * @param   enmMgrType  Wanted manager type - can be overwritten by the global override.
 */
int pdmacFileAioMgrCreate(PPDMASYNCCOMPLETIONEPCLASSFILE pEpClass, PPPDMACEPFILEMGR ppAioMgr,
                          PDMACEPFILEMGRTYPE enmMgrType)
{
    int rc = VINF_SUCCESS;
    PPDMACEPFILEMGR pAioMgrNew;

    LogFlowFunc((": Entered\n"));

    rc = MMR3HeapAllocZEx(pEpClass->Core.pVM, MM_TAG_PDM_ASYNC_COMPLETION, sizeof(PDMACEPFILEMGR), (void **)&pAioMgrNew);
    if (RT_SUCCESS(rc))
    {
        if (enmMgrType < pEpClass->enmMgrTypeOverride)
            pAioMgrNew->enmMgrType = enmMgrType;
        else
            pAioMgrNew->enmMgrType = pEpClass->enmMgrTypeOverride;

        rc = RTSemEventCreate(&pAioMgrNew->EventSem);
        if (RT_SUCCESS(rc))
        {
            rc = RTSemEventCreate(&pAioMgrNew->EventSemBlock);
            if (RT_SUCCESS(rc))
            {
                rc = RTCritSectInit(&pAioMgrNew->CritSectBlockingEvent);
                if (RT_SUCCESS(rc))
                {
                    /* Init the rest of the manager. */
                    if (pAioMgrNew->enmMgrType != PDMACEPFILEMGRTYPE_SIMPLE)
                        rc = pdmacFileAioMgrNormalInit(pAioMgrNew);

                    if (RT_SUCCESS(rc))
                    {
                        pAioMgrNew->enmState = PDMACEPFILEMGRSTATE_RUNNING;

                        rc = RTThreadCreateF(&pAioMgrNew->Thread,
                                             pAioMgrNew->enmMgrType == PDMACEPFILEMGRTYPE_SIMPLE
                                             ? pdmacFileAioMgrFailsafe
                                             : pdmacFileAioMgrNormal,
                                             pAioMgrNew,
                                             0,
                                             RTTHREADTYPE_IO,
                                             0,
                                             "AioMgr%d-%s", pEpClass->cAioMgrs,
                                             pAioMgrNew->enmMgrType == PDMACEPFILEMGRTYPE_SIMPLE
                                             ? "F"
                                             : "N");
                        if (RT_SUCCESS(rc))
                        {
                            /* Link it into the list. */
                            RTCritSectEnter(&pEpClass->CritSect);
                            pAioMgrNew->pNext = pEpClass->pAioMgrHead;
                            if (pEpClass->pAioMgrHead)
                                pEpClass->pAioMgrHead->pPrev = pAioMgrNew;
                            pEpClass->pAioMgrHead = pAioMgrNew;
                            pEpClass->cAioMgrs++;
                            RTCritSectLeave(&pEpClass->CritSect);

                            *ppAioMgr = pAioMgrNew;

                            Log(("PDMAC: Successfully created new file AIO Mgr {%s}\n", RTThreadGetName(pAioMgrNew->Thread)));
                            return VINF_SUCCESS;
                        }
                        pdmacFileAioMgrNormalDestroy(pAioMgrNew);
                    }
                    RTCritSectDelete(&pAioMgrNew->CritSectBlockingEvent);
                }
                RTSemEventDestroy(pAioMgrNew->EventSem);
            }
            RTSemEventDestroy(pAioMgrNew->EventSemBlock);
        }
        MMR3HeapFree(pAioMgrNew);
    }

    LogFlowFunc((": Leave rc=%Rrc\n", rc));

    return rc;
}

/**
 * Destroys a async I/O manager.
 *
 * @returns nothing.
 * @param   pAioMgr    The async I/O manager to destroy.
 */
static void pdmacFileAioMgrDestroy(PPDMASYNCCOMPLETIONEPCLASSFILE pEpClassFile, PPDMACEPFILEMGR pAioMgr)
{
    int rc = pdmacFileAioMgrShutdown(pAioMgr);
    AssertRC(rc);

    /* Unlink from the list. */
    rc = RTCritSectEnter(&pEpClassFile->CritSect);
    AssertRC(rc);

    PPDMACEPFILEMGR pPrev = pAioMgr->pPrev;
    PPDMACEPFILEMGR pNext = pAioMgr->pNext;

    if (pPrev)
        pPrev->pNext = pNext;
    else
        pEpClassFile->pAioMgrHead = pNext;

    if (pNext)
        pNext->pPrev = pPrev;

    pEpClassFile->cAioMgrs--;
    rc = RTCritSectLeave(&pEpClassFile->CritSect);
    AssertRC(rc);

    /* Free the resources. */
    RTCritSectDelete(&pAioMgr->CritSectBlockingEvent);
    RTSemEventDestroy(pAioMgr->EventSem);
    if (pAioMgr->enmMgrType != PDMACEPFILEMGRTYPE_SIMPLE)
        pdmacFileAioMgrNormalDestroy(pAioMgr);

    MMR3HeapFree(pAioMgr);
}

static int pdmacFileMgrTypeFromName(const char *pszVal, PPDMACEPFILEMGRTYPE penmMgrType)
{
    int rc = VINF_SUCCESS;

    if (!RTStrCmp(pszVal, "Simple"))
        *penmMgrType = PDMACEPFILEMGRTYPE_SIMPLE;
    else if (!RTStrCmp(pszVal, "Async"))
        *penmMgrType = PDMACEPFILEMGRTYPE_ASYNC;
    else
        rc = VERR_CFGM_CONFIG_UNKNOWN_VALUE;

    return rc;
}

static const char *pdmacFileMgrTypeToName(PDMACEPFILEMGRTYPE enmMgrType)
{
    if (enmMgrType == PDMACEPFILEMGRTYPE_SIMPLE)
        return "Simple";
    if (enmMgrType == PDMACEPFILEMGRTYPE_ASYNC)
        return "Async";

    return NULL;
}

static int pdmacFileBackendTypeFromName(const char *pszVal, PPDMACFILEEPBACKEND penmBackendType)
{
    int rc = VINF_SUCCESS;

    if (!RTStrCmp(pszVal, "Buffered"))
        *penmBackendType = PDMACFILEEPBACKEND_BUFFERED;
    else if (!RTStrCmp(pszVal, "NonBuffered"))
        *penmBackendType = PDMACFILEEPBACKEND_NON_BUFFERED;
    else
        rc = VERR_CFGM_CONFIG_UNKNOWN_VALUE;

    return rc;
}

static const char *pdmacFileBackendTypeToName(PDMACFILEEPBACKEND enmBackendType)
{
    if (enmBackendType == PDMACFILEEPBACKEND_BUFFERED)
        return "Buffered";
    if (enmBackendType == PDMACFILEEPBACKEND_NON_BUFFERED)
        return "NonBuffered";

    return NULL;
}

/**
 * Get the size of the given file.
 * Works for block devices too.
 *
 * @returns VBox status code.
 * @param   hFile    The file handle.
 * @param   pcbSize  Where to store the size of the file on success.
 */
static int pdmacFileEpNativeGetSize(RTFILE hFile, uint64_t *pcbSize)
{
    int rc = VINF_SUCCESS;
    uint64_t cbSize = 0;

    rc = RTFileGetSize(hFile, &cbSize);
    if (RT_SUCCESS(rc) && (cbSize != 0))
        *pcbSize = cbSize;
    else
    {
#ifdef RT_OS_WINDOWS
        DISK_GEOMETRY DriveGeo;
        DWORD cbDriveGeo;
        if (DeviceIoControl((HANDLE)hFile,
                            IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                            &DriveGeo, sizeof(DriveGeo), &cbDriveGeo, NULL))
        {
            if (   DriveGeo.MediaType == FixedMedia
                || DriveGeo.MediaType == RemovableMedia)
            {
                cbSize =     DriveGeo.Cylinders.QuadPart
                         *   DriveGeo.TracksPerCylinder
                         *   DriveGeo.SectorsPerTrack
                         *   DriveGeo.BytesPerSector;

                GET_LENGTH_INFORMATION DiskLenInfo;
                DWORD junk;
                if (DeviceIoControl((HANDLE)hFile,
                                    IOCTL_DISK_GET_LENGTH_INFO, NULL, 0,
                                    &DiskLenInfo, sizeof(DiskLenInfo), &junk, (LPOVERLAPPED)NULL))
                {
                    /* IOCTL_DISK_GET_LENGTH_INFO is supported -- override cbSize. */
                    cbSize = DiskLenInfo.Length.QuadPart;
                }

                rc = VINF_SUCCESS;
            }
            else
            {
                rc = VERR_INVALID_PARAMETER;
            }
        }
        else
        {
            rc = RTErrConvertFromWin32(GetLastError());
        }
#elif defined(RT_OS_DARWIN)
        struct stat DevStat;
        if (!fstat(hFile, &DevStat) && S_ISBLK(DevStat.st_mode))
        {
            uint64_t cBlocks;
            uint32_t cbBlock;
            if (!ioctl(hFile, DKIOCGETBLOCKCOUNT, &cBlocks))
            {
                if (!ioctl(hFile, DKIOCGETBLOCKSIZE, &cbBlock))
                    cbSize = cBlocks * cbBlock;
                else
                    rc = RTErrConvertFromErrno(errno);
            }
            else
                rc = RTErrConvertFromErrno(errno);
        }
        else
            rc = VERR_INVALID_PARAMETER;
#elif defined(RT_OS_SOLARIS)
        struct stat DevStat;
        if (!fstat(hFile, &DevStat) && (   S_ISBLK(DevStat.st_mode)
                                        || S_ISCHR(DevStat.st_mode)))
        {
            struct dk_minfo mediainfo;
            if (!ioctl(hFile, DKIOCGMEDIAINFO, &mediainfo))
                cbSize = mediainfo.dki_capacity * mediainfo.dki_lbsize;
            else
                rc = RTErrConvertFromErrno(errno);
        }
        else
            rc = VERR_INVALID_PARAMETER;
#elif defined(RT_OS_FREEBSD)
        struct stat DevStat;
        if (!fstat(hFile, &DevStat) && S_ISCHR(DevStat.st_mode))
        {
            off_t cbMedia = 0;
            if (!ioctl(hFile, DIOCGMEDIASIZE, &cbMedia))
            {
                cbSize = cbMedia;
            }
            else
                rc = RTErrConvertFromErrno(errno);
        }
        else
            rc = VERR_INVALID_PARAMETER;
#else
        /* Could be a block device */
        rc = RTFileSeek(hFile, 0, RTFILE_SEEK_END, &cbSize);
#endif

        if (RT_SUCCESS(rc) && (cbSize != 0))
            *pcbSize = cbSize;
        else if (RT_SUCCESS(rc))
            rc = VERR_NOT_SUPPORTED;
    }

    return rc;
}

#ifdef VBOX_WITH_DEBUGGER
/**
 * Error inject callback.
 */
static DECLCALLBACK(int) pdmacEpFileErrorInject(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR pArgs, unsigned cArgs)
{
    /*
     * Validate input.
     */
    DBGC_CMDHLP_REQ_VM_RET(pCmdHlp, pCmd, pVM);
    DBGC_CMDHLP_ASSERT_PARSER_RET(pCmdHlp, pCmd, -1, cArgs == 3);
    DBGC_CMDHLP_ASSERT_PARSER_RET(pCmdHlp, pCmd, 0, pArgs[0].enmType == DBGCVAR_TYPE_STRING);
    DBGC_CMDHLP_ASSERT_PARSER_RET(pCmdHlp, pCmd, 1, pArgs[1].enmType == DBGCVAR_TYPE_STRING);
    DBGC_CMDHLP_ASSERT_PARSER_RET(pCmdHlp, pCmd, 2, pArgs[2].enmType == DBGCVAR_TYPE_NUMBER);

    PPDMASYNCCOMPLETIONEPCLASSFILE pEpClassFile;
    pEpClassFile = (PPDMASYNCCOMPLETIONEPCLASSFILE)pVM->pUVM->pdm.s.apAsyncCompletionEndpointClass[PDMASYNCCOMPLETIONEPCLASSTYPE_FILE];

    /* Syntax is "read|write <filename> <status code>" */
    bool fWrite;
    if (!RTStrCmp(pArgs[0].u.pszString, "read"))
        fWrite = false;
    else if (!RTStrCmp(pArgs[0].u.pszString, "write"))
        fWrite = true;
    else
        return DBGCCmdHlpFail(pCmdHlp, pCmd, "invalid transfer direction '%s'", pArgs[0].u.pszString);

    int32_t rcToInject = (int32_t)pArgs[2].u.u64Number;
    if ((uint64_t)rcToInject != pArgs[2].u.u64Number)
        return DBGCCmdHlpFail(pCmdHlp, pCmd, "The status code '%lld' is out of range", pArgs[0].u.u64Number);


    /*
     * Search for the matching endpoint.
     */
    RTCritSectEnter(&pEpClassFile->Core.CritSect);

    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEpClassFile->Core.pEndpointsHead;
    while (pEpFile)
    {
        if (!RTStrCmp(pArgs[1].u.pszString, RTPathFilename(pEpFile->Core.pszUri)))
            break;
        pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEpFile->Core.pNext;
    }
    if (pEpFile)
    {
        /*
         * Do the job.
         */
        if (fWrite)
            ASMAtomicXchgS32(&pEpFile->rcReqWrite, rcToInject);
        else
            ASMAtomicXchgS32(&pEpFile->rcReqRead,  rcToInject);

        DBGCCmdHlpPrintf(pCmdHlp, "Injected %Rrc into '%s' for %s\n",
                         (int)rcToInject, pArgs[1].u.pszString, pArgs[0].u.pszString);
    }

    RTCritSectLeave(&pEpClassFile->Core.CritSect);

    if (!pEpFile)
        return DBGCCmdHlpFail(pCmdHlp, pCmd, "No file with name '%s' found", pArgs[1].u.pszString);
    return VINF_SUCCESS;
}
#endif /* VBOX_WITH_DEBUGGER */

static int pdmacFileInitialize(PPDMASYNCCOMPLETIONEPCLASS pClassGlobals, PCFGMNODE pCfgNode)
{
    int rc = VINF_SUCCESS;
    RTFILEAIOLIMITS AioLimits; /** < Async I/O limitations. */

    PPDMASYNCCOMPLETIONEPCLASSFILE pEpClassFile = (PPDMASYNCCOMPLETIONEPCLASSFILE)pClassGlobals;

    rc = RTFileAioGetLimits(&AioLimits);
#ifdef DEBUG
    if (RT_SUCCESS(rc) && RTEnvExist("VBOX_ASYNC_IO_FAILBACK"))
        rc = VERR_ENV_VAR_NOT_FOUND;
#endif
    if (RT_FAILURE(rc))
    {
        LogRel(("AIO: Async I/O manager not supported (rc=%Rrc). Falling back to simple manager\n",
                rc));
        pEpClassFile->enmMgrTypeOverride = PDMACEPFILEMGRTYPE_SIMPLE;
        pEpClassFile->enmEpBackendDefault = PDMACFILEEPBACKEND_BUFFERED;
    }
    else
    {
        pEpClassFile->uBitmaskAlignment   = AioLimits.cbBufferAlignment ? ~((RTR3UINTPTR)AioLimits.cbBufferAlignment - 1) : RTR3UINTPTR_MAX;
        pEpClassFile->cReqsOutstandingMax = AioLimits.cReqsOutstandingMax;

        if (pCfgNode)
        {
            /* Query the default manager type */
            char *pszVal = NULL;
            rc = CFGMR3QueryStringAllocDef(pCfgNode, "IoMgr", &pszVal, "Async");
            AssertLogRelRCReturn(rc, rc);

            rc = pdmacFileMgrTypeFromName(pszVal, &pEpClassFile->enmMgrTypeOverride);
            MMR3HeapFree(pszVal);
            if (RT_FAILURE(rc))
                return rc;

            LogRel(("AIOMgr: Default manager type is \"%s\"\n", pdmacFileMgrTypeToName(pEpClassFile->enmMgrTypeOverride)));

            /* Query default backend type */
            rc = CFGMR3QueryStringAllocDef(pCfgNode, "FileBackend", &pszVal, "NonBuffered");
            AssertLogRelRCReturn(rc, rc);

            rc = pdmacFileBackendTypeFromName(pszVal, &pEpClassFile->enmEpBackendDefault);
            MMR3HeapFree(pszVal);
            if (RT_FAILURE(rc))
                return rc;

            LogRel(("AIOMgr: Default file backend is \"%s\"\n", pdmacFileBackendTypeToName(pEpClassFile->enmEpBackendDefault)));

#ifdef RT_OS_LINUX
            if (   pEpClassFile->enmMgrTypeOverride == PDMACEPFILEMGRTYPE_ASYNC
                && pEpClassFile->enmEpBackendDefault == PDMACFILEEPBACKEND_BUFFERED)
            {
                LogRel(("AIOMgr: Linux does not support buffered async I/O, changing to non buffered\n"));
                pEpClassFile->enmEpBackendDefault = PDMACFILEEPBACKEND_NON_BUFFERED;
            }
#endif
        }
        else
        {
            /* No configuration supplied, set defaults */
            pEpClassFile->enmEpBackendDefault = PDMACFILEEPBACKEND_NON_BUFFERED;
            pEpClassFile->enmMgrTypeOverride  = PDMACEPFILEMGRTYPE_ASYNC;
        }
    }

    /* Init critical section. */
    rc = RTCritSectInit(&pEpClassFile->CritSect);

#ifdef VBOX_WITH_DEBUGGER
    /* Install the error injection handler. */
    if (RT_SUCCESS(rc))
    {
        rc = DBGCRegisterCommands(&g_aCmds[0], RT_ELEMENTS(g_aCmds));
        AssertRC(rc);
    }
#endif

    return rc;
}

static void pdmacFileTerminate(PPDMASYNCCOMPLETIONEPCLASS pClassGlobals)
{
    PPDMASYNCCOMPLETIONEPCLASSFILE pEpClassFile = (PPDMASYNCCOMPLETIONEPCLASSFILE)pClassGlobals;

    /* All endpoints should be closed at this point. */
    AssertMsg(!pEpClassFile->Core.pEndpointsHead, ("There are still endpoints left\n"));

    /* Destroy all left async I/O managers. */
    while (pEpClassFile->pAioMgrHead)
        pdmacFileAioMgrDestroy(pEpClassFile, pEpClassFile->pAioMgrHead);

    RTCritSectDelete(&pEpClassFile->CritSect);
}

static int pdmacFileEpInitialize(PPDMASYNCCOMPLETIONENDPOINT pEndpoint,
                                 const char *pszUri, uint32_t fFlags)
{
    int rc = VINF_SUCCESS;
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;
    PPDMASYNCCOMPLETIONEPCLASSFILE pEpClassFile = (PPDMASYNCCOMPLETIONEPCLASSFILE)pEndpoint->pEpClass;
    PDMACEPFILEMGRTYPE enmMgrType = pEpClassFile->enmMgrTypeOverride;
    PDMACFILEEPBACKEND enmEpBackend = pEpClassFile->enmEpBackendDefault;

    AssertMsgReturn((fFlags & ~(PDMACEP_FILE_FLAGS_READ_ONLY | PDMACEP_FILE_FLAGS_DONT_LOCK)) == 0,
                    ("PDMAsyncCompletion: Invalid flag specified\n"), VERR_INVALID_PARAMETER);

    unsigned fFileFlags = RTFILE_O_OPEN;

    if (fFlags & PDMACEP_FILE_FLAGS_READ_ONLY)
        fFileFlags |= RTFILE_O_READ | RTFILE_O_DENY_NONE;
    else
    {
        fFileFlags |= RTFILE_O_READWRITE;

        /*
         * Opened in read/write mode. Check whether the caller wants to
         * avoid the lock. Return an error in case caching is enabled
         * because this can lead to data corruption.
         */
        if (fFlags & PDMACEP_FILE_FLAGS_DONT_LOCK)
            fFileFlags |= RTFILE_O_DENY_NONE;
        else
            fFileFlags |= RTFILE_O_DENY_WRITE;
    }

    if (enmMgrType == PDMACEPFILEMGRTYPE_ASYNC)
        fFileFlags |= RTFILE_O_ASYNC_IO;

    if (enmEpBackend == PDMACFILEEPBACKEND_NON_BUFFERED)
    {
        /*
         * We only disable the cache if the size of the file is a multiple of 512.
         * Certain hosts like Windows, Linux and Solaris require that transfer sizes
         * are aligned to the volume sector size.
         * If not we just make sure that the data is written to disk with RTFILE_O_WRITE_THROUGH
         * which will trash the host cache but ensures that the host cache will not
         * contain dirty buffers.
         */
        RTFILE File = NIL_RTFILE;

        rc = RTFileOpen(&File, pszUri, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
        if (RT_SUCCESS(rc))
        {
            uint64_t cbSize;

            rc = pdmacFileEpNativeGetSize(File, &cbSize);
            Assert(RT_FAILURE(rc) || cbSize != 0);

            if (RT_SUCCESS(rc) && ((cbSize % 512) == 0))
                fFileFlags |= RTFILE_O_NO_CACHE;
            else
            {
                /* Downgrade to the buffered backend */
                enmEpBackend = PDMACFILEEPBACKEND_BUFFERED;

#ifdef RT_OS_LINUX
                fFileFlags &= ~RTFILE_O_ASYNC_IO;
                enmMgrType   = PDMACEPFILEMGRTYPE_SIMPLE;
#endif
            }
            RTFileClose(File);
        }
    }

    /* Open with final flags. */
    rc = RTFileOpen(&pEpFile->File, pszUri, fFileFlags);
    if ((rc == VERR_INVALID_FUNCTION) || (rc == VERR_INVALID_PARAMETER))
    {
        LogRel(("pdmacFileEpInitialize: RTFileOpen %s / %08x failed with %Rrc\n",
               pszUri, fFileFlags, rc));
        /*
         * Solaris doesn't support directio on ZFS so far. :-\
         * Trying to enable it returns VERR_INVALID_FUNCTION
         * (ENOTTY). Remove it and hope for the best.
         * ZFS supports write throttling in case applications
         * write more data than can be synced to the disk
         * without blocking the whole application.
         *
         * On Linux we have the same problem with cifs.
         * Have to disable async I/O here too because it requires O_DIRECT.
         */
        fFileFlags &= ~RTFILE_O_NO_CACHE;
        enmEpBackend = PDMACFILEEPBACKEND_BUFFERED;

#ifdef RT_OS_LINUX
        fFileFlags &= ~RTFILE_O_ASYNC_IO;
        enmMgrType   = PDMACEPFILEMGRTYPE_SIMPLE;
#endif

        /* Open again. */
        rc = RTFileOpen(&pEpFile->File, pszUri, fFileFlags);

        if (RT_FAILURE(rc))
        {
            LogRel(("pdmacFileEpInitialize: RTFileOpen %s / %08x failed AGAIN(!) with %Rrc\n",
                        pszUri, fFileFlags, rc));
        }
    }

    if (RT_SUCCESS(rc))
    {
        pEpFile->fFlags = fFileFlags;

        rc = pdmacFileEpNativeGetSize(pEpFile->File, (uint64_t *)&pEpFile->cbFile);
        Assert(RT_FAILURE(rc) || pEpFile->cbFile != 0);

        if (RT_SUCCESS(rc))
        {
            /* Initialize the segment cache */
            rc = MMR3HeapAllocZEx(pEpClassFile->Core.pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                                  sizeof(PDMACTASKFILE),
                                  (void **)&pEpFile->pTasksFreeHead);
            if (RT_SUCCESS(rc))
            {
                PPDMACEPFILEMGR pAioMgr = NULL;

                pEpFile->pTasksFreeTail = pEpFile->pTasksFreeHead;
                pEpFile->cTasksCached   = 0;
                pEpFile->enmBackendType = enmEpBackend;
                /*
                 * Disable async flushes on Solaris for now.
                 * They cause weird hangs which needs more investigations.
                 */
#ifndef RT_OS_SOLARIS
                pEpFile->fAsyncFlushSupported = true;
#else
                pEpFile->fAsyncFlushSupported = false;
#endif

                if (enmMgrType == PDMACEPFILEMGRTYPE_SIMPLE)
                {
                    /* Simple mode. Every file has its own async I/O manager. */
                    rc = pdmacFileAioMgrCreate(pEpClassFile, &pAioMgr, PDMACEPFILEMGRTYPE_SIMPLE);
                    AssertRC(rc);
                }
                else
                {
                    pAioMgr = pEpClassFile->pAioMgrHead;

                    /* Check for an idling manager of the same type */
                    while (pAioMgr)
                    {
                        if (pAioMgr->enmMgrType == enmMgrType)
                            break;
                        pAioMgr = pAioMgr->pNext;
                    }

                    if (!pAioMgr)
                    {
                        rc = pdmacFileAioMgrCreate(pEpClassFile, &pAioMgr, enmMgrType);
                        AssertRC(rc);
                    }
                }

                pEpFile->AioMgr.pTreeRangesLocked = (PAVLRFOFFTREE)RTMemAllocZ(sizeof(AVLRFOFFTREE));
                if (!pEpFile->AioMgr.pTreeRangesLocked)
                    rc = VERR_NO_MEMORY;
                else
                {
                    pEpFile->enmState = PDMASYNCCOMPLETIONENDPOINTFILESTATE_ACTIVE;

                    /* Assign the endpoint to the thread. */
                    rc = pdmacFileAioMgrAddEndpoint(pAioMgr, pEpFile);
                    if (RT_FAILURE(rc))
                    {
                        RTMemFree(pEpFile->AioMgr.pTreeRangesLocked);
                        MMR3HeapFree(pEpFile->pTasksFreeHead);
                    }
                }
            }
        }

        if (RT_FAILURE(rc))
            RTFileClose(pEpFile->File);
    }

#ifdef VBOX_WITH_STATISTICS
    if (RT_SUCCESS(rc))
    {
        STAMR3RegisterF(pEpClassFile->Core.pVM, &pEpFile->StatRead,
                       STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS,
                       STAMUNIT_TICKS_PER_CALL, "Time taken to read from the endpoint",
                       "/PDM/AsyncCompletion/File/%s/Read", RTPathFilename(pEpFile->Core.pszUri));

        STAMR3RegisterF(pEpClassFile->Core.pVM, &pEpFile->StatWrite,
                       STAMTYPE_PROFILE_ADV, STAMVISIBILITY_ALWAYS,
                       STAMUNIT_TICKS_PER_CALL, "Time taken to write to the endpoint",
                       "/PDM/AsyncCompletion/File/%s/Write", RTPathFilename(pEpFile->Core.pszUri));
    }
#endif

    if (RT_SUCCESS(rc))
        LogRel(("AIOMgr: Endpoint for file '%s' (flags %08x) created successfully\n", pszUri, pEpFile->fFlags));

    return rc;
}

static int pdmacFileEpRangesLockedDestroy(PAVLRFOFFNODECORE pNode, void *pvUser)
{
    AssertMsgFailed(("The locked ranges tree should be empty at that point\n"));
    return VINF_SUCCESS;
}

static int pdmacFileEpClose(PPDMASYNCCOMPLETIONENDPOINT pEndpoint)
{
    int rc = VINF_SUCCESS;
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;
    PPDMASYNCCOMPLETIONEPCLASSFILE pEpClassFile = (PPDMASYNCCOMPLETIONEPCLASSFILE)pEndpoint->pEpClass;

    /* Make sure that all tasks finished for this endpoint. */
    rc = pdmacFileAioMgrCloseEndpoint(pEpFile->pAioMgr, pEpFile);
    AssertRC(rc);

    /*
     * If the async I/O manager is in failsafe mode this is the only endpoint
     * he processes and thus can be destroyed now.
     */
    if (pEpFile->pAioMgr->enmMgrType == PDMACEPFILEMGRTYPE_SIMPLE)
        pdmacFileAioMgrDestroy(pEpClassFile, pEpFile->pAioMgr);

    /* Free cached tasks. */
    PPDMACTASKFILE pTask = pEpFile->pTasksFreeHead;

    while (pTask)
    {
        PPDMACTASKFILE pTaskFree = pTask;
        pTask = pTask->pNext;
        MMR3HeapFree(pTaskFree);
    }

    /* Destroy the locked ranges tree now. */
    RTAvlrFileOffsetDestroy(pEpFile->AioMgr.pTreeRangesLocked, pdmacFileEpRangesLockedDestroy, NULL);

    RTFileClose(pEpFile->File);

#ifdef VBOX_WITH_STATISTICS
    STAMR3Deregister(pEpClassFile->Core.pVM, &pEpFile->StatRead);
    STAMR3Deregister(pEpClassFile->Core.pVM, &pEpFile->StatWrite);
#endif

    return VINF_SUCCESS;
}

static int pdmacFileEpRead(PPDMASYNCCOMPLETIONTASK pTask,
                           PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                           PCRTSGSEG paSegments, size_t cSegments,
                           size_t cbRead)
{
    int rc = VINF_SUCCESS;
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;

    LogFlowFunc(("pTask=%#p pEndpoint=%#p off=%RTfoff paSegments=%#p cSegments=%zu cbRead=%zu\n",
                 pTask, pEndpoint, off, paSegments, cSegments, cbRead));

    STAM_PROFILE_ADV_START(&pEpFile->StatRead, Read);

    pdmacFileEpTaskInit(pTask, cbRead);

    rc = pdmacFileEpTaskInitiate(pTask, pEndpoint, off, paSegments, cSegments, cbRead,
                                 PDMACTASKFILETRANSFER_READ);

    STAM_PROFILE_ADV_STOP(&pEpFile->StatRead, Read);

    return rc;
}

static int pdmacFileEpWrite(PPDMASYNCCOMPLETIONTASK pTask,
                            PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                            PCRTSGSEG paSegments, size_t cSegments,
                            size_t cbWrite)
{
    int rc = VINF_SUCCESS;
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;

    if (RT_UNLIKELY(pEpFile->fReadonly))
        return VERR_NOT_SUPPORTED;

    STAM_PROFILE_ADV_START(&pEpFile->StatWrite, Write);

    pdmacFileEpTaskInit(pTask, cbWrite);

    rc = pdmacFileEpTaskInitiate(pTask, pEndpoint, off, paSegments, cSegments, cbWrite,
                                 PDMACTASKFILETRANSFER_WRITE);

    STAM_PROFILE_ADV_STOP(&pEpFile->StatWrite, Write);

    return rc;
}

static int pdmacFileEpFlush(PPDMASYNCCOMPLETIONTASK pTask,
                            PPDMASYNCCOMPLETIONENDPOINT pEndpoint)
{
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;
    PPDMASYNCCOMPLETIONTASKFILE pTaskFile = (PPDMASYNCCOMPLETIONTASKFILE)pTask;

    if (RT_UNLIKELY(pEpFile->fReadonly))
        return VERR_NOT_SUPPORTED;

    pdmacFileEpTaskInit(pTask, 0);

    PPDMACTASKFILE pIoTask = pdmacFileTaskAlloc(pEpFile);
    if (RT_UNLIKELY(!pIoTask))
        return VERR_NO_MEMORY;

    pIoTask->pEndpoint       = pEpFile;
    pIoTask->enmTransferType = PDMACTASKFILETRANSFER_FLUSH;
    pIoTask->pvUser          = pTaskFile;
    pIoTask->pfnCompleted    = pdmacFileEpTaskCompleted;
    pdmacFileEpAddTask(pEpFile, pIoTask);

    return VINF_AIO_TASK_PENDING;
}

static int pdmacFileEpGetSize(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t *pcbSize)
{
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;

    *pcbSize = ASMAtomicReadU64(&pEpFile->cbFile);

    return VINF_SUCCESS;
}

static int pdmacFileEpSetSize(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, uint64_t cbSize)
{
    PPDMASYNCCOMPLETIONENDPOINTFILE pEpFile = (PPDMASYNCCOMPLETIONENDPOINTFILE)pEndpoint;

    ASMAtomicWriteU64(&pEpFile->cbFile, cbSize);
    return RTFileSetSize(pEpFile->File, cbSize);
}

const PDMASYNCCOMPLETIONEPCLASSOPS g_PDMAsyncCompletionEndpointClassFile =
{
    /* u32Version */
    PDMAC_EPCLASS_OPS_VERSION,
    /* pcszName */
    "File",
    /* enmClassType */
    PDMASYNCCOMPLETIONEPCLASSTYPE_FILE,
    /* cbEndpointClassGlobal */
    sizeof(PDMASYNCCOMPLETIONEPCLASSFILE),
    /* cbEndpoint */
    sizeof(PDMASYNCCOMPLETIONENDPOINTFILE),
    /* cbTask */
    sizeof(PDMASYNCCOMPLETIONTASKFILE),
    /* pfnInitialize */
    pdmacFileInitialize,
    /* pfnTerminate */
    pdmacFileTerminate,
    /* pfnEpInitialize. */
    pdmacFileEpInitialize,
    /* pfnEpClose */
    pdmacFileEpClose,
    /* pfnEpRead */
    pdmacFileEpRead,
    /* pfnEpWrite */
    pdmacFileEpWrite,
    /* pfnEpFlush */
    pdmacFileEpFlush,
    /* pfnEpGetSize */
    pdmacFileEpGetSize,
    /* pfnEpSetSize */
    pdmacFileEpSetSize,
    /* u32VersionEnd */
    PDMAC_EPCLASS_OPS_VERSION
};


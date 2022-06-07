/* $Id: vfsreadahead.cpp 59729 2016-02-19 00:31:35Z vboxsync $ */
/** @file
 * IPRT - Virtual File System, Read-Ahead Thread.
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
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
#include "internal/iprt.h"
#include <iprt/vfs.h>

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/list.h>
#include <iprt/poll.h>
#include <iprt/string.h>
#include <iprt/vfslowlevel.h>



/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include "internal/iprt.h"
#include <iprt/vfs.h>

#include <iprt/critsect.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/thread.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Buffer descriptor.
 */
typedef struct RTVFSREADAHEADBUFDESC
{
    /** List entry. */
    RTLISTNODE          ListEntry;
    /** The offset of this extent within the file. */
    uint64_t            off;
    /** The amount of the buffer that has been filled.
     * (Buffer size is RTVFSREADAHEAD::cbBuffer.)  */
    uint32_t            cbFilled;
    /** */
    uint32_t volatile   fReserved;
    /** Pointer to the buffer. */
    uint8_t            *pbBuffer;
} RTVFSREADAHEADBUFDESC;
/** Pointer to a memory file extent. */
typedef RTVFSREADAHEADBUFDESC *PRTVFSREADAHEADBUFDESC;

/**
 * Read ahead file or I/O stream.
 */
typedef struct RTVFSREADAHEAD
{
    /** The I/O critical section (protects offActual).
     * The thread doing I/O or seeking always need to own this. */
    RTCRITSECT              IoCritSect;

    /** The critical section protecting the buffer lists and offConsumer.
     *
     * This can be taken while holding IoCritSect as that eliminates a race
     * condition between the read ahead thread inserting into ConsumerList and
     * a consumer thread deciding to do a direct read. */
    RTCRITSECT              BufferCritSect;
    /** List of buffers available for consumption.
     * The producer thread (hThread) puts buffers into this list once it's done
     * reading into them.   The consumer moves them to the FreeList once the
     * current position has passed beyond each buffer. */
    RTLISTANCHOR            ConsumerList;
    /** List of buffers available for the producer. */
    RTLISTANCHOR            FreeList;

    /** The current file position from the consumer point of view. */
    uint64_t                offConsumer;

    /** The read ahead thread. */
    RTTHREAD                hThread;
    /** Set when we want the thread to terminate. */
    bool volatile           fTerminateThread;
    /** Creation flags. */
    uint32_t                fFlags;

    /** The I/O stream we read from. */
    RTVFSIOSTREAM           hIos;
    /** The file face of hIos, if we're fronting for an actual file. */
    RTVFSFILE               hFile;
    /** The buffer size. */
    uint32_t                cbBuffer;
    /** The number of buffers. */
    uint32_t                cBuffers;
    /** Single big buffer allocation, cBuffers * cbBuffer in size.  */
    uint8_t                *pbAllBuffers;
    /** Array of buffer descriptors (cBuffers in size). */
    RTVFSREADAHEADBUFDESC   aBufDescs[1];
} RTVFSREADAHEAD;
/** Pointer to a memory file. */
typedef RTVFSREADAHEAD *PRTVFSREADAHEAD;



/**
 * @interface_method_impl{RTVFSOBJOPS,pfnClose}
 */
static DECLCALLBACK(int) rtVfsReadAhead_Close(void *pvThis)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    int rc;

    /*
     * Stop the read-ahead thread.
     */
    if (pThis->hThread != NIL_RTTHREAD)
    {
        ASMAtomicWriteBool(&pThis->fTerminateThread, true);
        rc = RTThreadUserSignal(pThis->hThread);
        AssertRC(rc);
        rc = RTThreadWait(pThis->hThread, RT_INDEFINITE_WAIT, NULL);
        AssertRCReturn(rc, rc);
        pThis->hThread = NIL_RTTHREAD;
    }

    /*
     * Release the upstream objects.
     */
    RTCritSectEnter(&pThis->IoCritSect);

    RTVfsIoStrmRelease(pThis->hIos);
    pThis->hIos  = NIL_RTVFSIOSTREAM;
    RTVfsFileRelease(pThis->hFile);
    pThis->hFile = NIL_RTVFSFILE;

    RTCritSectLeave(&pThis->IoCritSect);

    /*
     * Free the buffers.
     */
    RTCritSectEnter(&pThis->BufferCritSect);
    if (pThis->pbAllBuffers)
    {
        RTMemPageFree(pThis->pbAllBuffers, pThis->cBuffers * pThis->cbBuffer);
        pThis->pbAllBuffers = NULL;
    }
    RTCritSectLeave(&pThis->BufferCritSect);

    /*
     * Destroy the critical sections.
     */
    RTCritSectDelete(&pThis->BufferCritSect);
    RTCritSectDelete(&pThis->IoCritSect);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJOPS,pfnQueryInfo}
 */
static DECLCALLBACK(int) rtVfsReadAhead_QueryInfo(void *pvThis, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    return RTVfsIoStrmQueryInfo(pThis->hIos, pObjInfo, enmAddAttr);
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnRead}
 */
static DECLCALLBACK(int) rtVfsReadAhead_Read(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbRead)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;

    Assert(pSgBuf->cSegs == 1);
    Assert(off < 0);
    NOREF(fBlocking);


    /*
     * We loop here to repeat the buffer search after entering the I/O critical
     * section, just in case a buffer got inserted while we were waiting for it.
     */
    int    rc = VINF_SUCCESS;
    size_t cbTotalRead     = 0;
    bool   fPokeReader     = false;
    bool   fOwnsIoCritSect = false;
    RTCritSectEnter(&pThis->BufferCritSect);
    for (;;)
    {
        /*
         * Try satisfy the read from the buffer.
         */
        uint64_t offCur = pThis->offConsumer;
        if (off != -1)
        {
            offCur = (uint64_t)off;
            pThis->offConsumer = offCur;
        }

        PRTVFSREADAHEADBUFDESC pCurBufDesc;
        RTListForEach(&pThis->ConsumerList, pCurBufDesc, RTVFSREADAHEADBUFDESC, ListEntry)
        {
            if (pThis->offConsumer)
            {
            }
        }
/** @todo EOF handling! */
        if (pSgBuf->cbSegLeft == 0)
            break;

        /*
         * First time around we don't own the I/O critsect and need to take it
         * and repeat the above buffer reading code.
         */
        if (!fOwnsIoCritSect)
        {
            RTCritSectLeave(&pThis->BufferCritSect);
            RTCritSectEnter(&pThis->IoCritSect);
            RTCritSectEnter(&pThis->BufferCritSect);
            fOwnsIoCritSect = true;
            continue;
        }

        /*
         * Do a direct read of the remaining data.
         */
        //size_t cbDirectRead;


    }
    RTCritSectLeave(&pThis->BufferCritSect);
    if (fOwnsIoCritSect)
        RTCritSectLeave(&pThis->IoCritSect);
    if (fPokeReader)
        RTThreadUserSignal(pThis->hThread);

    if (pcbRead)
        *pcbRead = cbTotalRead;

    return rc;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnWrite}
 */
static DECLCALLBACK(int) rtVfsReadAhead_Write(void *pvThis, RTFOFF off, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbWritten)
{
    AssertFailed();
    return VERR_ACCESS_DENIED;
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnFlush}
 */
static DECLCALLBACK(int) rtVfsReadAhead_Flush(void *pvThis)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    return RTVfsIoStrmFlush(pThis->hIos);
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnPollOne}
 */
static DECLCALLBACK(int) rtVfsReadAhead_PollOne(void *pvThis, uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr,
                                                uint32_t *pfRetEvents)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    if (pThis->hThread != NIL_RTTHREAD)
    {
        /** @todo poll one with read-ahead thread. */
        return VERR_NOT_IMPLEMENTED;
    }
    return RTVfsIoStrmPoll(pThis->hIos, fEvents, cMillies, fIntr, pfRetEvents);
}


/**
 * @interface_method_impl{RTVFSIOSTREAMOPS,pfnTell}
 */
static DECLCALLBACK(int) rtVfsReadAhead_Tell(void *pvThis, PRTFOFF poffActual)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;

    RTCritSectEnter(&pThis->BufferCritSect);
    *poffActual = pThis->offConsumer;
    RTCritSectLeave(&pThis->BufferCritSect);

    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnMode}
 */
static DECLCALLBACK(int) rtVfsReadAhead_SetMode(void *pvThis, RTFMODE fMode, RTFMODE fMask)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    AssertReturn(pThis->hFile != NIL_RTVFSFILE, VERR_NOT_SUPPORTED);

    RTCritSectEnter(&pThis->IoCritSect);
    /// @todo int rc = RTVfsFileSetMode(pThis->hFile, fMode, fMask);
    int rc = VERR_NOT_SUPPORTED;
    RTCritSectLeave(&pThis->IoCritSect);

    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetTimes}
 */
static DECLCALLBACK(int) rtVfsReadAhead_SetTimes(void *pvThis, PCRTTIMESPEC pAccessTime, PCRTTIMESPEC pModificationTime,
                                               PCRTTIMESPEC pChangeTime, PCRTTIMESPEC pBirthTime)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    AssertReturn(pThis->hFile != NIL_RTVFSFILE, VERR_NOT_SUPPORTED);

    RTCritSectEnter(&pThis->IoCritSect);
    /// @todo int rc = RTVfsFileSetTimes(pThis->hFile, pAccessTime, pModificationTime, pChangeTime, pBirthTime);
    int rc = VERR_NOT_SUPPORTED;
    RTCritSectLeave(&pThis->IoCritSect);

    return rc;
}


/**
 * @interface_method_impl{RTVFSOBJSETOPS,pfnSetOwner}
 */
static DECLCALLBACK(int) rtVfsReadAhead_SetOwner(void *pvThis, RTUID uid, RTGID gid)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    AssertReturn(pThis->hFile != NIL_RTVFSFILE, VERR_NOT_SUPPORTED);

    RTCritSectEnter(&pThis->IoCritSect);
    /// @todo int rc = RTVfsFileSetOwner(pThis->hFile, uid, gid);
    int rc = VERR_NOT_SUPPORTED;
    RTCritSectLeave(&pThis->IoCritSect);

    return rc;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnSeek}
 */
static DECLCALLBACK(int) rtVfsReadAhead_Seek(void *pvThis, RTFOFF offSeek, unsigned uMethod, PRTFOFF poffActual)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    AssertReturn(pThis->hFile != NIL_RTVFSFILE, VERR_NOT_SUPPORTED);

    RTCritSectEnter(&pThis->IoCritSect);        /* protects against concurrent I/O using the offset. */
    RTCritSectEnter(&pThis->BufferCritSect);    /* protects offConsumer */

    uint64_t offActual = UINT64_MAX;
    int rc = RTVfsFileSeek(pThis->hFile, offSeek, uMethod, &offActual);
    if (RT_SUCCESS(rc))
        pThis->offConsumer = offActual;

    RTCritSectLeave(&pThis->BufferCritSect);
    RTCritSectLeave(&pThis->IoCritSect);

    return rc;
}


/**
 * @interface_method_impl{RTVFSFILEOPS,pfnQuerySize}
 */
static DECLCALLBACK(int) rtVfsReadAhead_QuerySize(void *pvThis, uint64_t *pcbFile)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvThis;
    AssertReturn(pThis->hFile != NIL_RTVFSFILE, VERR_NOT_SUPPORTED);

    RTCritSectEnter(&pThis->IoCritSect); /* paranoia */
    int rc = RTVfsFileGetSize(pThis->hFile, pcbFile);
    RTCritSectLeave(&pThis->IoCritSect);

    return rc;
}


/**
 * Read ahead I/O stream operations.
 */
DECL_HIDDEN_CONST(const RTVFSIOSTREAMOPS) g_VfsReadAheadIosOps =
{ /* Stream */
    { /* Obj */
        RTVFSOBJOPS_VERSION,
        RTVFSOBJTYPE_IO_STREAM,
        "Read ahead I/O stream",
        rtVfsReadAhead_Close,
        rtVfsReadAhead_QueryInfo,
        RTVFSOBJOPS_VERSION
    },
    RTVFSIOSTREAMOPS_VERSION,
    RTVFSIOSTREAMOPS_FEAT_NO_SG,
    rtVfsReadAhead_Read,
    rtVfsReadAhead_Write,
    rtVfsReadAhead_Flush,
    rtVfsReadAhead_PollOne,
    rtVfsReadAhead_Tell,
    NULL /*Skip*/,
    NULL /*ZeroFill*/,
    RTVFSIOSTREAMOPS_VERSION,
};


/**
 * Read ahead file operations.
 */
DECL_HIDDEN_CONST(const RTVFSFILEOPS) g_VfsReadAheadFileOps =
{
    { /* Stream */
        { /* Obj */
            RTVFSOBJOPS_VERSION,
            RTVFSOBJTYPE_FILE,
            "Read ahead file",
            rtVfsReadAhead_Close,
            rtVfsReadAhead_QueryInfo,
            RTVFSOBJOPS_VERSION
        },
        RTVFSIOSTREAMOPS_VERSION,
        RTVFSIOSTREAMOPS_FEAT_NO_SG,
        rtVfsReadAhead_Read,
        rtVfsReadAhead_Write,
        rtVfsReadAhead_Flush,
        rtVfsReadAhead_PollOne,
        rtVfsReadAhead_Tell,
        NULL /*Skip*/,
        NULL /*ZeroFill*/,
        RTVFSIOSTREAMOPS_VERSION,
    },
    RTVFSFILEOPS_VERSION,
    /*RTVFSIOFILEOPS_FEAT_NO_AT_OFFSET*/ 0,
    { /* ObjSet */
        RTVFSOBJSETOPS_VERSION,
        RT_OFFSETOF(RTVFSFILEOPS, Stream.Obj) - RT_OFFSETOF(RTVFSFILEOPS, ObjSet),
        rtVfsReadAhead_SetMode,
        rtVfsReadAhead_SetTimes,
        rtVfsReadAhead_SetOwner,
        RTVFSOBJSETOPS_VERSION
    },
    rtVfsReadAhead_Seek,
    rtVfsReadAhead_QuerySize,
    RTVFSFILEOPS_VERSION
};


/**
 * @callback_method_impl{PFNRTTHREAD, Read ahead thread procedure}
 */
static DECLCALLBACK(int) rtVfsReadAheadThreadProc(RTTHREAD hThreadSelf, void *pvUser)
{
    PRTVFSREADAHEAD pThis = (PRTVFSREADAHEAD)pvUser;
    Assert(pThis);

    while (pThis->fTerminateThread)
    {
        int rc;

/** @todo EOF handling.   */
        /*
         * Is there a buffer handy for reading ahead.
         */
        PRTVFSREADAHEADBUFDESC pBufDesc = NULL;
        RTCritSectEnter(&pThis->BufferCritSect);
        if (!pThis->fTerminateThread)
            pBufDesc = RTListGetFirst(&pThis->FreeList, RTVFSREADAHEADBUFDESC, ListEntry);
        RTCritSectLeave(&pThis->BufferCritSect);

        if (pBufDesc)
        {
            /*
             * Got a buffer, take the I/O lock and read into it.
             */
            RTCritSectEnter(&pThis->IoCritSect);
            if (!pThis->fTerminateThread)
            {

                pBufDesc->off      = RTVfsIoStrmTell(pThis->hIos);
                size_t cbRead      = 0;
                size_t cbToRead    = pThis->cbBuffer;
                rc = RTVfsIoStrmRead(pThis->hIos, pBufDesc->pbBuffer, pThis->cbBuffer, true /*fBlocking*/, &cbRead);
                if (RT_SUCCESS(rc))
                {
                    pBufDesc->cbFilled = (uint32_t)cbRead;

                    /*
                     * Put back the buffer.  The consumer list is sorted by offset, but
                     * we should usually end up appending the buffer.
                     */
                    RTCritSectEnter(&pThis->BufferCritSect);
                    PRTVFSREADAHEADBUFDESC pAfter = RTListGetLast(&pThis->ConsumerList, RTVFSREADAHEADBUFDESC, ListEntry);
                    if (!pAfter || pAfter->off <= pBufDesc->off)
                        RTListAppend(&pThis->ConsumerList, &pBufDesc->ListEntry);
                    else
                    {
                        do
                            pAfter = RTListGetPrev(&pThis->ConsumerList, pAfter, RTVFSREADAHEADBUFDESC, ListEntry);
                        while (pAfter && pAfter->off > pBufDesc->off);
                        if (!pAfter)
                            RTListPrepend(&pThis->ConsumerList, &pBufDesc->ListEntry);
                        else
                            RTListNodeInsertAfter(&pAfter->ListEntry, &pBufDesc->ListEntry);
                    }
                    RTCritSectLeave(&pThis->BufferCritSect);
                    pBufDesc = NULL;
                }
            }
            RTCritSectLeave(&pThis->IoCritSect);

            /*
             * If we succeeded, loop without delay to start processing the next buffer.
             */
            if (RT_LIKELY(!pBufDesc))
                continue;

            /* On failure or termination, put the buffer back in the free list and wait. */
            RTCritSectEnter(&pThis->BufferCritSect);
            RTListPrepend(&pThis->FreeList, &pBufDesc->ListEntry);
            RTCritSectLeave(&pThis->BufferCritSect);
            if (pThis->fTerminateThread)
                break;
        }

        /*
         * Wait for more to do.
         */
        rc = RTThreadUserWait(hThreadSelf, RT_MS_1MIN);
        if (RT_SUCCESS(rc))
            rc = RTThreadUserReset(hThreadSelf);
    }

    return VINF_SUCCESS;
}


static int rtVfsCreateReadAheadInstance(RTVFSIOSTREAM hVfsIosSrc, RTVFSFILE hVfsFileSrc, uint32_t fFlags,
                                        uint32_t cBuffers, uint32_t cbBuffer, PRTVFSIOSTREAM phVfsIos, PRTVFSFILE phVfsFile)
{
    /*
     * Validate input a little.
     */
    int rc = VINF_SUCCESS;
    AssertStmt(cBuffers < _4K, rc = VERR_OUT_OF_RANGE);
    if (cBuffers == 0)
        cBuffers = 4;
    AssertStmt(cbBuffer <= _512K, rc = VERR_OUT_OF_RANGE);
    if (cbBuffer == 0)
        cbBuffer = _256K / cBuffers;
    AssertStmt(!fFlags, rc = VERR_INVALID_FLAGS);

    if (RT_SUCCESS(rc))
    {
        /*
         * Create a file or I/O stream instance.
         */
        RTVFSFILE       hVfsFileReadAhead = NIL_RTVFSFILE;
        RTVFSIOSTREAM   hVfsIosReadAhead  = NIL_RTVFSIOSTREAM;
        PRTVFSREADAHEAD pThis;
        size_t          cbThis = RT_OFFSETOF(RTVFSREADAHEAD, aBufDescs[cBuffers]);
        if (hVfsFileSrc != NIL_RTVFSFILE)
            rc = RTVfsNewFile(&g_VfsReadAheadFileOps, cbThis, RTFILE_O_READ, NIL_RTVFS, NIL_RTVFSLOCK,
                              &hVfsFileReadAhead, (void **)&pThis);
        else
            rc = RTVfsNewIoStream(&g_VfsReadAheadIosOps, cbThis, RTFILE_O_READ, NIL_RTVFS, NIL_RTVFSLOCK,
                                  &hVfsIosReadAhead, (void **)&pThis);
        if (RT_SUCCESS(rc))
        {
            RTListInit(&pThis->ConsumerList);
            RTListInit(&pThis->FreeList);
            pThis->hThread          = NIL_RTTHREAD;
            pThis->fTerminateThread = false;
            pThis->fFlags           = fFlags;
            pThis->hFile            = hVfsFileSrc;
            pThis->hIos             = hVfsIosSrc;
            pThis->cBuffers         = cBuffers;
            pThis->cbBuffer         = cbBuffer;
            pThis->offConsumer      = RTVfsIoStrmTell(hVfsIosSrc);
            if ((RTFOFF)pThis->offConsumer >= 0)
            {
                rc = RTCritSectInit(&pThis->IoCritSect);
                if (RT_SUCCESS(rc))
                    rc = RTCritSectInit(&pThis->BufferCritSect);
                if (RT_SUCCESS(rc))
                {
                    pThis->pbAllBuffers = (uint8_t *)RTMemPageAlloc(pThis->cbBuffer * pThis->cBuffers);
                    if (pThis->pbAllBuffers)
                    {
                        for (uint32_t i = 0; i < cBuffers; i++)
                        {
                            pThis->aBufDescs[i].cbFilled = 0;
                            pThis->aBufDescs[i].off      = UINT64_MAX / 2;
                            pThis->aBufDescs[i].pbBuffer = &pThis->pbAllBuffers[cbBuffer * i];
                            RTListAppend(&pThis->FreeList, &pThis->aBufDescs[i].ListEntry);
                        }

                        /*
                         * Create thread.
                         */
                        rc = RTThreadCreate(&pThis->hThread, rtVfsReadAheadThreadProc, pThis, 0, RTTHREADTYPE_DEFAULT,
                                            RTTHREADFLAGS_WAITABLE, "vfsreadahead");
                        if (RT_SUCCESS(rc))
                        {
                            /*
                             * We're good.
                             */
                            if (phVfsFile)
                                *phVfsFile = hVfsFileReadAhead;
                            else if (hVfsFileReadAhead == NIL_RTVFSFILE)
                                *phVfsIos = hVfsIosReadAhead;
                            else
                            {
                                *phVfsIos = RTVfsFileToIoStream(hVfsFileReadAhead);
                                RTVfsFileRelease(hVfsFileReadAhead);
                                AssertReturn(*phVfsIos != NIL_RTVFSIOSTREAM, VERR_INTERNAL_ERROR_5);
                            }
                            return VINF_SUCCESS;
                        }
                    }
                }
            }
            else
                rc = (int)pThis->offConsumer;
        }
    }

    RTVfsFileRelease(hVfsFileSrc);
    RTVfsIoStrmRelease(hVfsIosSrc);
    return rc;
}


RTDECL(int) RTVfsCreateReadAheadForIoStream(RTVFSIOSTREAM hVfsIos, uint32_t fFlags, uint32_t cBuffers, uint32_t cbBuffer,
                                            PRTVFSIOSTREAM phVfsIos)
{
    AssertPtrReturn(phVfsIos, VERR_INVALID_POINTER);
    *phVfsIos = NIL_RTVFSIOSTREAM;

    /*
     * Retain the input stream, trying to obtain a file handle too so we can
     * fully mirror it.
     */
    uint32_t cRefs = RTVfsIoStrmRetain(hVfsIos);
    AssertReturn(cRefs != UINT32_MAX, VERR_INVALID_HANDLE);
    RTVFSFILE hVfsFile = RTVfsIoStrmToFile(hVfsIos);

    /*
     * Do the job. (This always consumes the above retained references.)
     */
    return rtVfsCreateReadAheadInstance(hVfsIos, hVfsFile, fFlags, cBuffers, cbBuffer, phVfsIos, NULL);
}


RTDECL(int) RTVfsCreateReadAheadForFile(RTVFSFILE hVfsFile, uint32_t fFlags, uint32_t cBuffers, uint32_t cbBuffer,
                                        PRTVFSFILE phVfsFile)
{
    AssertPtrReturn(phVfsFile, VERR_INVALID_POINTER);
    *phVfsFile = NIL_RTVFSFILE;

    /*
     * Retain the input file and cast it o an I/O stream.
     */
    RTVFSIOSTREAM hVfsIos = RTVfsFileToIoStream(hVfsFile);
    AssertReturn(hVfsIos != NIL_RTVFSIOSTREAM, VERR_INVALID_HANDLE);
    uint32_t cRefs = RTVfsFileRetain(hVfsFile);
    AssertReturnStmt(cRefs != UINT32_MAX, RTVfsIoStrmRelease(hVfsIos), VERR_INVALID_HANDLE);

    /*
     * Do the job. (This always consumes the above retained references.)
     */
    return rtVfsCreateReadAheadInstance(hVfsIos, hVfsFile, fFlags, cBuffers, cbBuffer, NULL, phVfsFile);
}


/* $Id: DrvSCSI.cpp 64132 2016-10-03 16:23:11Z vboxsync $ */
/** @file
 * VBox storage drivers: Generic SCSI command parser and execution driver
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
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
//#define DEBUG
#define LOG_GROUP LOG_GROUP_DRV_SCSI
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmifs.h>
#include <VBox/vmm/pdmqueue.h>
#include <VBox/vmm/pdmstorageifs.h>
#include <VBox/vmm/pdmthread.h>
#include <VBox/vscsi.h>
#include <VBox/scsi.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/mem.h>
#include <iprt/req.h>
#include <iprt/semaphore.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"

/** The maximum number of release log entries per device. */
#define MAX_LOG_REL_ERRORS  1024

/**
 * Eject state.
 */
typedef struct DRVSCSIEJECTSTATE
{
    /** The item core for the PDM queue. */
    PDMQUEUEITEMCORE Core;
    /** Event semaphore to signal when complete. */
    RTSEMEVENT       hSemEvt;
} DRVSCSIEJECTSTATE;
typedef DRVSCSIEJECTSTATE *PDRVSCSIEJECTSTATE;

/**
 * SCSI driver private per request data.
 */
typedef struct DRVSCSIREQ
{
    /** Size of the guest buffer. */
    size_t                   cbBuf;
    /** Temporary buffer holding the data. */
    void                     *pvBuf;
    /** Data segment. */
    RTSGSEG                  Seg;
    /** Transfer direction. */
    PDMMEDIAEXIOREQSCSITXDIR enmXferDir;
    /** The VSCSI request handle. */
    VSCSIREQ                 hVScsiReq;
    /** Where to store the SCSI status code. */
    uint8_t                  *pu8ScsiSts;
    /** Start of the request data for the device above us. */
    uint8_t                  abAlloc[1];
} DRVSCSIREQ;
/** Pointer to the driver private per request data. */
typedef DRVSCSIREQ *PDRVSCSIREQ;

/**
 * SCSI driver instance data.
 *
 * @implements  PDMISCSICONNECTOR
 * @implements  PDMIMEDIAEXPORT
 * @implements  PDMIMEDIAEX
 * @implements  PDMIMOUNTNOTIFY
 */
typedef struct DRVSCSI
{
    /** Pointer driver instance. */
    PPDMDRVINS              pDrvIns;

    /** Pointer to the attached driver's base interface. */
    PPDMIBASE               pDrvBase;
    /** Pointer to the attached driver's block interface. */
    PPDMIMEDIA              pDrvMedia;
    /** Pointer to the attached driver's extended media interface. */
    PPDMIMEDIAEX            pDrvMediaEx;
    /** Pointer to the attached driver's mount interface. */
    PPDMIMOUNT              pDrvMount;
    /** Pointer to the SCSI port interface of the device above. */
    PPDMISCSIPORT           pDevScsiPort;
    /** Pointer to the extended media port interface of the device above. */
    PPDMIMEDIAEXPORT        pDevMediaExPort;
    /** Pointer to the media port interface of the device above. */
    PPDMIMEDIAPORT          pDevMediaPort;
    /** pointer to the Led port interface of the dveice above. */
    PPDMILEDPORTS           pLedPort;
    /** The scsi connector interface .*/
    PDMISCSICONNECTOR       ISCSIConnector;
    /** The media interface for the device above. */
    PDMIMEDIA               IMedia;
    /** The extended media interface for the device above. */
    PDMIMEDIAEX             IMediaEx;
    /** The media port interface. */
    PDMIMEDIAPORT           IPort;
    /** The optional extended media port interface. */
    PDMIMEDIAEXPORT         IPortEx;
    /** The mount notify interface. */
    PDMIMOUNTNOTIFY         IMountNotify;
    /** Fallback status LED state for this drive.
     * This is used in case the device doesn't has a LED interface. */
    PDMLED                  Led;
    /** Pointer to the status LED for this drive. */
    PPDMLED                 pLed;

    /** VSCSI device handle. */
    VSCSIDEVICE             hVScsiDevice;
    /** VSCSI LUN handle. */
    VSCSILUN                hVScsiLun;
    /** I/O callbacks. */
    VSCSILUNIOCALLBACKS     VScsiIoCallbacks;

    /** Indicates whether PDMDrvHlpAsyncNotificationCompleted should be called by
     * any of the dummy functions. */
    bool volatile           fDummySignal;
    /** Release statistics: number of bytes written. */
    STAMCOUNTER             StatBytesWritten;
    /** Release statistics: number of bytes read. */
    STAMCOUNTER             StatBytesRead;
    /** Release statistics: Current I/O depth. */
    volatile uint32_t       StatIoDepth;
    /** Errors printed in the release log. */
    unsigned                cErrors;

    /** Size of the I/O request to allocate. */
    size_t                  cbIoReqAlloc;
    /** Size of a VSCSI I/O request. */
    size_t                  cbVScsiIoReqAlloc;
    /** Queue to defer unmounting to EMT. */
    PPDMQUEUE               pQueue;
} DRVSCSI, *PDRVSCSI;

/** Convert a VSCSI I/O request handle to the associated PDMIMEDIAEX I/O request. */
#define DRVSCSI_VSCSIIOREQ_2_PDMMEDIAEXIOREQ(a_hVScsiIoReq) (*(PPDMMEDIAEXIOREQ)((uint8_t *)(a_hVScsiIoReq) - sizeof(PDMMEDIAEXIOREQ)))
/** Convert a PDMIMEDIAEX I/O additional request memory to a VSCSI I/O request. */
#define DRVSCSI_PDMMEDIAEXIOREQ_2_VSCSIIOREQ(a_pvIoReqAlloc) ((VSCSIIOREQ)((uint8_t *)(a_pvIoReqAlloc) + sizeof(PDMMEDIAEXIOREQ)))

/**
 * Returns whether the given status code indicates a non fatal error.
 *
 * @returns True if the error can be fixed by the user after the VM was suspended.
 *          False if not and the error should be reported to the guest.
 * @param   rc          The status code to check.
 */
DECLINLINE(bool) drvscsiIsRedoPossible(int rc)
{
    if (   rc == VERR_DISK_FULL
        || rc == VERR_FILE_TOO_BIG
        || rc == VERR_BROKEN_PIPE
        || rc == VERR_NET_CONNECTION_REFUSED
        || rc == VERR_VD_DEK_MISSING)
        return true;

    return false;
}

/* -=-=-=-=- VScsiIoCallbacks -=-=-=-=- */

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunReqAllocSizeSet}
 */
static DECLCALLBACK(int) drvscsiReqAllocSizeSet(VSCSILUN hVScsiLun, void *pvScsiLunUser, size_t cbVScsiIoReqAlloc)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;

    /* We need to store the I/O request handle so we can get it when VSCSI queues an I/O request. */
    int rc = pThis->pDrvMediaEx->pfnIoReqAllocSizeSet(pThis->pDrvMediaEx, cbVScsiIoReqAlloc + sizeof(PDMMEDIAEXIOREQ));
    if (RT_SUCCESS(rc))
        pThis->cbVScsiIoReqAlloc = cbVScsiIoReqAlloc + sizeof(PDMMEDIAEXIOREQ);

    return rc;
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunReqAlloc}
 */
static DECLCALLBACK(int) drvscsiReqAlloc(VSCSILUN hVScsiLun, void *pvScsiLunUser,
                                         uint64_t u64Tag, PVSCSIIOREQ phVScsiIoReq)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;
    PDMMEDIAEXIOREQ hIoReq;
    void *pvIoReqAlloc;
    int rc = pThis->pDrvMediaEx->pfnIoReqAlloc(pThis->pDrvMediaEx, &hIoReq, &pvIoReqAlloc, u64Tag,
                                               PDMIMEDIAEX_F_DEFAULT);
    if (RT_SUCCESS(rc))
    {
        PPDMMEDIAEXIOREQ phIoReq = (PPDMMEDIAEXIOREQ)pvIoReqAlloc;

        *phIoReq = hIoReq;
        *phVScsiIoReq = (VSCSIIOREQ)(phIoReq + 1);
    }

    return rc;
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunReqFree}
 */
static DECLCALLBACK(int) drvscsiReqFree(VSCSILUN hVScsiLun, void *pvScsiLunUser, VSCSIIOREQ hVScsiIoReq)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;
    PDMMEDIAEXIOREQ hIoReq = DRVSCSI_VSCSIIOREQ_2_PDMMEDIAEXIOREQ(hVScsiIoReq);

    return pThis->pDrvMediaEx->pfnIoReqFree(pThis->pDrvMediaEx, hIoReq);
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunMediumGetSize}
 */
static DECLCALLBACK(int) drvscsiGetSize(VSCSILUN hVScsiLun, void *pvScsiLunUser, uint64_t *pcbSize)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;

    *pcbSize = pThis->pDrvMedia->pfnGetSize(pThis->pDrvMedia);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunMediumGetSectorSize}
 */
static DECLCALLBACK(int) drvscsiGetSectorSize(VSCSILUN hVScsiLun, void *pvScsiLunUser, uint32_t *pcbSectorSize)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;

    *pcbSectorSize = pThis->pDrvMedia->pfnGetSectorSize(pThis->pDrvMedia);

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunMediumSetLock}
 */
static DECLCALLBACK(int) drvscsiSetLock(VSCSILUN hVScsiLun, void *pvScsiLunUser, bool fLocked)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;

    if (fLocked)
        pThis->pDrvMount->pfnLock(pThis->pDrvMount);
    else
        pThis->pDrvMount->pfnUnlock(pThis->pDrvMount);

    return VINF_SUCCESS;
}

/** @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunMediumEject} */
static DECLCALLBACK(int) drvscsiEject(VSCSILUN hVScsiLun, void *pvScsiLunUser)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;
    int rc = VINF_SUCCESS;
    RTSEMEVENT hSemEvt = NIL_RTSEMEVENT;

    /* This must be done from EMT. */
    rc = RTSemEventCreate(&hSemEvt);
    if (RT_SUCCESS(rc))
    {
        PDRVSCSIEJECTSTATE pEjectState = (PDRVSCSIEJECTSTATE)PDMQueueAlloc(pThis->pQueue);
        if (pEjectState)
        {
            pEjectState->hSemEvt = hSemEvt;
            PDMQueueInsert(pThis->pQueue, &pEjectState->Core);

            /* Wait for completion. */
            rc = RTSemEventWait(pEjectState->hSemEvt, RT_INDEFINITE_WAIT);
        }
        else
            rc = VERR_NO_MEMORY;

        RTSemEventDestroy(pEjectState->hSemEvt);
    }

    return rc;
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunReqTransferEnqueue}
 */
static DECLCALLBACK(int) drvscsiReqTransferEnqueue(VSCSILUN hVScsiLun, void *pvScsiLunUser, VSCSIIOREQ hVScsiIoReq)
{
    RT_NOREF(hVScsiLun);
    int rc = VINF_SUCCESS;
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;
    PDMMEDIAEXIOREQ hIoReq = DRVSCSI_VSCSIIOREQ_2_PDMMEDIAEXIOREQ(hVScsiIoReq);

    LogFlowFunc(("Enqueuing hVScsiIoReq=%#p\n", hVScsiIoReq));

    VSCSIIOREQTXDIR enmTxDir = VSCSIIoReqTxDirGet(hVScsiIoReq);
    switch (enmTxDir)
    {
        case VSCSIIOREQTXDIR_FLUSH:
        {
            rc = pThis->pDrvMediaEx->pfnIoReqFlush(pThis->pDrvMediaEx, hIoReq);
            if (   RT_FAILURE(rc)
                && pThis->cErrors++ < MAX_LOG_REL_ERRORS)
                LogRel(("SCSI#%u: Flush returned rc=%Rrc\n",
                        pThis->pDrvIns->iInstance, rc));
            break;
        }
        case VSCSIIOREQTXDIR_UNMAP:
        {
            PCRTRANGE paRanges;
            unsigned cRanges;

            rc = VSCSIIoReqUnmapParamsGet(hVScsiIoReq, &paRanges, &cRanges);
            AssertRC(rc);

            pThis->pLed->Asserted.s.fWriting = pThis->pLed->Actual.s.fWriting = 1;
            rc = pThis->pDrvMediaEx->pfnIoReqDiscard(pThis->pDrvMediaEx, hIoReq, cRanges);
            if (   RT_FAILURE(rc)
                && pThis->cErrors++ < MAX_LOG_REL_ERRORS)
                LogRel(("SCSI#%u: Discard returned rc=%Rrc\n",
                        pThis->pDrvIns->iInstance, rc));
            break;
        }
        case VSCSIIOREQTXDIR_READ:
        case VSCSIIOREQTXDIR_WRITE:
        {
            uint64_t  uOffset    = 0;
            size_t    cbTransfer = 0;
            size_t    cbSeg      = 0;
            PCRTSGSEG paSeg      = NULL;
            unsigned  cSeg       = 0;

            rc = VSCSIIoReqParamsGet(hVScsiIoReq, &uOffset, &cbTransfer,
                                     &cSeg, &cbSeg, &paSeg);
            AssertRC(rc);

            if (enmTxDir == VSCSIIOREQTXDIR_READ)
            {
                pThis->pLed->Asserted.s.fReading = pThis->pLed->Actual.s.fReading = 1;
                rc = pThis->pDrvMediaEx->pfnIoReqRead(pThis->pDrvMediaEx, hIoReq, uOffset, cbTransfer);
                STAM_REL_COUNTER_ADD(&pThis->StatBytesRead, cbTransfer);
            }
            else
            {
                pThis->pLed->Asserted.s.fWriting = pThis->pLed->Actual.s.fWriting = 1;
                rc = pThis->pDrvMediaEx->pfnIoReqWrite(pThis->pDrvMediaEx, hIoReq, uOffset, cbTransfer);
                STAM_REL_COUNTER_ADD(&pThis->StatBytesWritten, cbTransfer);
            }

            if (   RT_FAILURE(rc)
                && pThis->cErrors++ < MAX_LOG_REL_ERRORS)
                LogRel(("SCSI#%u: %s at offset %llu (%u bytes left) returned rc=%Rrc\n",
                        pThis->pDrvIns->iInstance,
                        enmTxDir == VSCSIIOREQTXDIR_READ
                        ? "Read"
                        : "Write",
                        uOffset,
                        cbTransfer, rc));
            break;
        }
        default:
            AssertMsgFailed(("Invalid transfer direction %u\n", enmTxDir));
    }

    if (rc == VINF_SUCCESS)
    {
        if (enmTxDir == VSCSIIOREQTXDIR_READ)
            pThis->pLed->Actual.s.fReading = 0;
        else if (enmTxDir == VSCSIIOREQTXDIR_WRITE)
            pThis->pLed->Actual.s.fWriting = 0;
        else
            AssertMsg(enmTxDir == VSCSIIOREQTXDIR_FLUSH, ("Invalid transfer direction %u\n", enmTxDir));

        VSCSIIoReqCompleted(hVScsiIoReq, VINF_SUCCESS, false);
        rc = VINF_SUCCESS;
    }
    else if (rc == VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS)
        rc = VINF_SUCCESS;
    else if (RT_FAILURE(rc))
    {
        if (enmTxDir == VSCSIIOREQTXDIR_READ)
            pThis->pLed->Actual.s.fReading = 0;
        else if (enmTxDir == VSCSIIOREQTXDIR_WRITE)
            pThis->pLed->Actual.s.fWriting = 0;
        else
            AssertMsg(enmTxDir == VSCSIIOREQTXDIR_FLUSH, ("Invalid transfer direction %u\n", enmTxDir));

        VSCSIIoReqCompleted(hVScsiIoReq, rc, drvscsiIsRedoPossible(rc));
        rc = VINF_SUCCESS;
    }
    else
        AssertMsgFailed(("Invalid return code rc=%Rrc\n", rc));

    return rc;
}

/**
 * @interface_method_impl{VSCSILUNIOCALLBACKS,pfnVScsiLunGetFeatureFlags}
 */
static DECLCALLBACK(int) drvscsiGetFeatureFlags(VSCSILUN hVScsiLun, void *pvScsiLunUser, uint64_t *pfFeatures)
{
    RT_NOREF(hVScsiLun);
    PDRVSCSI pThis = (PDRVSCSI)pvScsiLunUser;

    *pfFeatures = 0;

    uint32_t fFeatures = 0;
    int rc = pThis->pDrvMediaEx->pfnQueryFeatures(pThis->pDrvMediaEx, &fFeatures);
    if (RT_SUCCESS(rc) && (fFeatures & PDMIMEDIAEX_FEATURE_F_DISCARD))
        *pfFeatures |= VSCSI_LUN_FEATURE_UNMAP;

    if (   pThis->pDrvMedia
        && pThis->pDrvMedia->pfnIsNonRotational(pThis->pDrvMedia))
        *pfFeatures |= VSCSI_LUN_FEATURE_NON_ROTATIONAL;

    if (pThis->pDrvMedia->pfnIsReadOnly(pThis->pDrvMedia))
        *pfFeatures |= VSCSI_LUN_FEATURE_READONLY;

    return VINF_SUCCESS;
}


/* -=-=-=-=- IPortEx -=-=-=-=- */

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCompleteNotify}
 */
static DECLCALLBACK(int) drvscsiIoReqCompleteNotify(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                    void *pvIoReqAlloc, int rcReq)
{
    RT_NOREF1(hIoReq);

    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IPortEx);
    VSCSIIOREQ hVScsiIoReq = (VSCSIIOREQ)((uint8_t *)pvIoReqAlloc + sizeof(PDMMEDIAEXIOREQ));
    VSCSIIOREQTXDIR enmTxDir = VSCSIIoReqTxDirGet(hVScsiIoReq);

    LogFlowFunc(("Request hVScsiIoReq=%#p completed\n", hVScsiIoReq));

    if (enmTxDir == VSCSIIOREQTXDIR_READ)
        pThis->pLed->Actual.s.fReading = 0;
    else if (   enmTxDir == VSCSIIOREQTXDIR_WRITE
             || enmTxDir == VSCSIIOREQTXDIR_UNMAP)
        pThis->pLed->Actual.s.fWriting = 0;
    else
        AssertMsg(enmTxDir == VSCSIIOREQTXDIR_FLUSH, ("Invalid transfer direction %u\n", enmTxDir));

    if (RT_SUCCESS(rcReq))
        VSCSIIoReqCompleted(hVScsiIoReq, rcReq, false /* fRedoPossible */);
    else
    {
        pThis->cErrors++;
        if (pThis->cErrors < MAX_LOG_REL_ERRORS)
        {
            if (enmTxDir == VSCSIIOREQTXDIR_FLUSH)
                LogRel(("SCSI#%u: Flush returned rc=%Rrc\n",
                        pThis->pDrvIns->iInstance, rcReq));
            else if (enmTxDir == VSCSIIOREQTXDIR_UNMAP)
                LogRel(("SCSI#%u: Unmap returned rc=%Rrc\n",
                        pThis->pDrvIns->iInstance, rcReq));
            else
            {
                uint64_t  uOffset    = 0;
                size_t    cbTransfer = 0;
                size_t    cbSeg      = 0;
                PCRTSGSEG paSeg      = NULL;
                unsigned  cSeg       = 0;

                VSCSIIoReqParamsGet(hVScsiIoReq, &uOffset, &cbTransfer,
                                    &cSeg, &cbSeg, &paSeg);

                LogRel(("SCSI#%u: %s at offset %llu (%u bytes left) returned rc=%Rrc\n",
                        pThis->pDrvIns->iInstance,
                        enmTxDir == VSCSIIOREQTXDIR_READ
                        ? "Read"
                        : "Write",
                        uOffset,
                        cbTransfer, rcReq));
            }
        }

        VSCSIIoReqCompleted(hVScsiIoReq, rcReq, drvscsiIsRedoPossible(rcReq));
    }

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCopyFromBuf}
 */
static DECLCALLBACK(int) drvscsiIoReqCopyFromBuf(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                 void *pvIoReqAlloc, uint32_t offDst, PRTSGBUF pSgBuf,
                                                 size_t cbCopy)
{
    RT_NOREF2(pInterface, hIoReq);

    VSCSIIOREQ hVScsiIoReq = DRVSCSI_PDMMEDIAEXIOREQ_2_VSCSIIOREQ(pvIoReqAlloc);
    uint64_t  uOffset    = 0;
    size_t    cbTransfer = 0;
    size_t    cbSeg      = 0;
    PCRTSGSEG paSeg      = NULL;
    unsigned  cSeg       = 0;
    size_t    cbCopied   = 0;

    int rc = VSCSIIoReqParamsGet(hVScsiIoReq, &uOffset, &cbTransfer, &cSeg, &cbSeg, &paSeg);
    if (RT_SUCCESS(rc))
    {
        RTSGBUF SgBuf;
        RTSgBufInit(&SgBuf, paSeg, cSeg);

        RTSgBufAdvance(&SgBuf, offDst);
        cbCopied = RTSgBufCopy(&SgBuf, pSgBuf, cbCopy);
    }

    return cbCopied == cbCopy ? VINF_SUCCESS : VERR_PDM_MEDIAEX_IOBUF_OVERFLOW;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCopyToBuf}
 */
static DECLCALLBACK(int) drvscsiIoReqCopyToBuf(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                               void *pvIoReqAlloc, uint32_t offSrc, PRTSGBUF pSgBuf,
                                               size_t cbCopy)
{
    RT_NOREF2(pInterface, hIoReq);

    VSCSIIOREQ hVScsiIoReq = DRVSCSI_PDMMEDIAEXIOREQ_2_VSCSIIOREQ(pvIoReqAlloc);
    uint64_t  uOffset    = 0;
    size_t    cbTransfer = 0;
    size_t    cbSeg      = 0;
    PCRTSGSEG paSeg      = NULL;
    unsigned  cSeg       = 0;
    size_t    cbCopied   = 0;

    int rc = VSCSIIoReqParamsGet(hVScsiIoReq, &uOffset, &cbTransfer, &cSeg, &cbSeg, &paSeg);
    if (RT_SUCCESS(rc))
    {
        RTSGBUF SgBuf;
        RTSgBufInit(&SgBuf, paSeg, cSeg);

        RTSgBufAdvance(&SgBuf, offSrc);
        cbCopied = RTSgBufCopy(pSgBuf, &SgBuf, cbCopy);
    }

    return cbCopied == cbCopy ? VINF_SUCCESS : VERR_PDM_MEDIAEX_IOBUF_UNDERRUN;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqQueryDiscardRanges}
 */
static DECLCALLBACK(int) drvscsiIoReqQueryDiscardRanges(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                        void *pvIoReqAlloc, uint32_t idxRangeStart,
                                                        uint32_t cRanges, PRTRANGE paRanges,
                                                        uint32_t *pcRanges)
{
    RT_NOREF2(pInterface, hIoReq);

    VSCSIIOREQ hVScsiIoReq = DRVSCSI_PDMMEDIAEXIOREQ_2_VSCSIIOREQ(pvIoReqAlloc);
    PCRTRANGE paRangesVScsi;
    unsigned cRangesVScsi;

    int rc = VSCSIIoReqUnmapParamsGet(hVScsiIoReq, &paRangesVScsi, &cRangesVScsi);
    if (RT_SUCCESS(rc))
    {
        uint32_t cRangesCopy = RT_MIN(cRangesVScsi - idxRangeStart, cRanges);
        Assert(   idxRangeStart < cRangesVScsi
               && (idxRangeStart + cRanges) <= cRangesVScsi);

        memcpy(paRanges, &paRangesVScsi[idxRangeStart], cRangesCopy * sizeof(RTRANGE));
        *pcRanges = cRangesCopy;
    }
    return rc;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqStateChanged}
 */
static DECLCALLBACK(void) drvscsiIoReqStateChanged(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                   void *pvIoReqAlloc, PDMMEDIAEXIOREQSTATE enmState)
{
    RT_NOREF4(pInterface, hIoReq, pvIoReqAlloc, enmState);
    AssertLogRelMsgFailed(("This should not be hit because I/O requests should not be suspended\n"));
}

static DECLCALLBACK(void) drvscsiVScsiReqCompleted(VSCSIDEVICE hVScsiDevice, void *pVScsiDeviceUser,
                                                   void *pVScsiReqUser, int rcScsiCode, bool fRedoPossible, int rcReq)
{
    RT_NOREF(hVScsiDevice);
    PDRVSCSI pThis = (PDRVSCSI)pVScsiDeviceUser;

    ASMAtomicDecU32(&pThis->StatIoDepth);

    pThis->pDevScsiPort->pfnSCSIRequestCompleted(pThis->pDevScsiPort, (PPDMSCSIREQUEST)pVScsiReqUser,
                                                 rcScsiCode, fRedoPossible, rcReq);

    if (RT_UNLIKELY(pThis->fDummySignal) && !pThis->StatIoDepth)
        PDMDrvHlpAsyncNotificationCompleted(pThis->pDrvIns);
}

/* -=-=-=-=- ISCSIConnector -=-=-=-=- */

#ifdef DEBUG
/**
 * Dumps a SCSI request structure for debugging purposes.
 *
 * @returns nothing.
 * @param   pRequest    Pointer to the request to dump.
 */
static void drvscsiDumpScsiRequest(PPDMSCSIREQUEST pRequest)
{
    Log(("Dump for pRequest=%#p Command: %s\n", pRequest, SCSICmdText(pRequest->pbCDB[0])));
    Log(("cbCDB=%u\n", pRequest->cbCDB));
    for (uint32_t i = 0; i < pRequest->cbCDB; i++)
        Log(("pbCDB[%u]=%#x\n", i, pRequest->pbCDB[i]));
    Log(("cbScatterGather=%u\n", pRequest->cbScatterGather));
    Log(("cScatterGatherEntries=%u\n", pRequest->cScatterGatherEntries));
    /* Print all scatter gather entries. */
    for (uint32_t i = 0; i < pRequest->cScatterGatherEntries; i++)
    {
        Log(("ScatterGatherEntry[%u].cbSeg=%u\n", i, pRequest->paScatterGatherHead[i].cbSeg));
        Log(("ScatterGatherEntry[%u].pvSeg=%#p\n", i, pRequest->paScatterGatherHead[i].pvSeg));
    }
    Log(("pvUser=%#p\n", pRequest->pvUser));
}
#endif

/** @interface_method_impl{PDMISCSICONNECTOR,pfnSCSIRequestSend} */
static DECLCALLBACK(int) drvscsiRequestSend(PPDMISCSICONNECTOR pInterface, PPDMSCSIREQUEST pSCSIRequest)
{
    int rc;
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, ISCSIConnector);
    VSCSIREQ hVScsiReq;

#ifdef DEBUG
    drvscsiDumpScsiRequest(pSCSIRequest);
#endif

    rc = VSCSIDeviceReqCreate(pThis->hVScsiDevice, &hVScsiReq,
                              pSCSIRequest->uLogicalUnit,
                              pSCSIRequest->pbCDB,
                              pSCSIRequest->cbCDB,
                              pSCSIRequest->cbScatterGather,
                              pSCSIRequest->cScatterGatherEntries,
                              pSCSIRequest->paScatterGatherHead,
                              pSCSIRequest->pbSenseBuffer,
                              pSCSIRequest->cbSenseBuffer,
                              pSCSIRequest);
    if (RT_FAILURE(rc))
        return rc;

    ASMAtomicIncU32(&pThis->StatIoDepth);
    rc = VSCSIDeviceReqEnqueue(pThis->hVScsiDevice, hVScsiReq);

    return rc;
}

/** @interface_method_impl{PDMISCSICONNECTOR,pfnQueryLUNType} */
static DECLCALLBACK(int) drvscsiQueryLUNType(PPDMISCSICONNECTOR pInterface, uint32_t iLun, PPDMSCSILUNTYPE pLunType)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, ISCSIConnector);
    VSCSILUNTYPE enmLunType;

    int rc = VSCSIDeviceLunQueryType(pThis->hVScsiDevice, iLun, &enmLunType);
    if (RT_FAILURE(rc))
        return rc;

    switch (enmLunType)
    {
    case VSCSILUNTYPE_SBC:  *pLunType = PDMSCSILUNTYPE_SBC; break;
    case VSCSILUNTYPE_MMC:  *pLunType = PDMSCSILUNTYPE_MMC; break;
    case VSCSILUNTYPE_SSC:  *pLunType = PDMSCSILUNTYPE_SSC; break;
    default:                *pLunType = PDMSCSILUNTYPE_INVALID;
    }

    return rc;
}

/* -=-=-=-=- IMedia -=-=-=-=- */

/** @interface_method_impl{PDMIMEDIA,pfnGetType} */
static DECLCALLBACK(PDMMEDIATYPE) drvscsiGetType(PPDMIMEDIA pInterface)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMedia);
    VSCSILUNTYPE enmLunType;

    int rc = VSCSIDeviceLunQueryType(pThis->hVScsiDevice, 0, &enmLunType);
    if (RT_FAILURE(rc))
        return PDMMEDIATYPE_ERROR;

    switch (enmLunType)
    {
        case VSCSILUNTYPE_SBC:
            return PDMMEDIATYPE_HARD_DISK;
        case VSCSILUNTYPE_MMC:
            return PDMMEDIATYPE_CDROM;
        default:
            return PDMMEDIATYPE_ERROR;
    }

    return PDMMEDIATYPE_ERROR;
}

/** @interface_method_impl{PDMIMEDIA,pfnGetUuid} */
static DECLCALLBACK(int) drvscsiGetUuid(PPDMIMEDIA pInterface, PRTUUID pUuid)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMedia);

    int rc = VINF_SUCCESS;
    if (pThis->pDrvMedia)
        rc = pThis->pDrvMedia->pfnGetUuid(pThis->pDrvMedia, pUuid);
    else
        RTUuidClear(pUuid);

    return rc;
}

/* -=-=-=-=- IMediaEx -=-=-=-=- */

/** @interface_method_impl{PDMIMEDIAEX,pfnQueryFeatures} */
static DECLCALLBACK(int) drvscsiQueryFeatures(PPDMIMEDIAEX pInterface, uint32_t *pfFeatures)
{
    RT_NOREF1(pInterface);

    *pfFeatures = PDMIMEDIAEX_FEATURE_F_RAWSCSICMD;
    return VINF_SUCCESS;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqAllocSizeSet} */
static DECLCALLBACK(int) drvscsiIoReqAllocSizeSet(PPDMIMEDIAEX pInterface, size_t cbIoReqAlloc)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMediaEx);

    pThis->cbIoReqAlloc = RT_OFFSETOF(DRVSCSIREQ, abAlloc[cbIoReqAlloc]);
    return VINF_SUCCESS;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqAlloc} */
static DECLCALLBACK(int) drvscsiIoReqAlloc(PPDMIMEDIAEX pInterface, PPDMMEDIAEXIOREQ phIoReq, void **ppvIoReqAlloc,
                                           PDMMEDIAEXIOREQID uIoReqId, uint32_t fFlags)
{
    RT_NOREF2(uIoReqId, fFlags);

    int rc = VINF_SUCCESS;
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMediaEx);
    PDRVSCSIREQ pReq = (PDRVSCSIREQ)RTMemAllocZ(pThis->cbIoReqAlloc);
    if (RT_LIKELY(pReq))
    {
        *phIoReq = (PDMMEDIAEXIOREQ)pReq;
        *ppvIoReqAlloc = &pReq->abAlloc[0];
    }
    else
        rc = VERR_NO_MEMORY;

    return rc;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqFree} */
static DECLCALLBACK(int) drvscsiIoReqFree(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq)
{
    RT_NOREF1(pInterface);
    PDRVSCSIREQ pReq = (PDRVSCSIREQ)hIoReq;

    if (pReq->pvBuf)
        RTMemFree(pReq->pvBuf);

    RTMemFree(pReq);
    return VINF_SUCCESS;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqQueryResidual} */
static DECLCALLBACK(int) drvscsiIoReqQueryResidual(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, size_t *pcbResidual)
{
    RT_NOREF2(pInterface, hIoReq);

    *pcbResidual = 0; /** @todo: Implement. */
    return VINF_SUCCESS;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqCancelAll} */
static DECLCALLBACK(int) drvscsiIoReqCancelAll(PPDMIMEDIAEX pInterface)
{
    RT_NOREF1(pInterface);
    return VINF_SUCCESS;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqCancel} */
static DECLCALLBACK(int) drvscsiIoReqCancel(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQID uIoReqId)
{
    RT_NOREF2(pInterface, uIoReqId);
    return VERR_PDM_MEDIAEX_IOREQID_NOT_FOUND;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqRead} */
static DECLCALLBACK(int) drvscsiIoReqRead(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, uint64_t off, size_t cbRead)
{
    RT_NOREF4(pInterface, hIoReq, off, cbRead);
    return VERR_NOT_SUPPORTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqWrite} */
static DECLCALLBACK(int) drvscsiIoReqWrite(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, uint64_t off, size_t cbWrite)
{
    RT_NOREF4(pInterface, hIoReq, off, cbWrite);
    return VERR_NOT_SUPPORTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqFlush} */
static DECLCALLBACK(int) drvscsiIoReqFlush(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq)
{
    RT_NOREF2(pInterface, hIoReq);
    return VERR_NOT_SUPPORTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqDiscard} */
static DECLCALLBACK(int) drvscsiIoReqDiscard(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, unsigned cRangesMax)
{
    RT_NOREF3(pInterface, hIoReq, cRangesMax);
    return VERR_NOT_SUPPORTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqSendScsiCmd} */
static DECLCALLBACK(int) drvscsiIoReqSendScsiCmd(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq, uint32_t uLun,
                                                 const uint8_t *pbCdb, size_t cbCdb, PDMMEDIAEXIOREQSCSITXDIR enmTxDir,
                                                 size_t cbBuf, uint8_t *pabSense, size_t cbSense, uint8_t *pu8ScsiSts,
                                                 uint32_t cTimeoutMillies)
{
    RT_NOREF1(cTimeoutMillies);

    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMediaEx);
    PDRVSCSIREQ pReq = (PDRVSCSIREQ)hIoReq;
    int rc = VINF_SUCCESS;

    Log(("Dump for pReq=%#p Command: %s\n", pReq, SCSICmdText(pbCdb[0])));
    Log(("cbCdb=%u\n", cbCdb));
    for (uint32_t i = 0; i < cbCdb; i++)
        Log(("pbCdb[%u]=%#x\n", i, pbCdb[i]));
    Log(("cbBuf=%zu\n", cbBuf));

    pReq->enmXferDir = enmTxDir;
    pReq->cbBuf      = cbBuf;
    pReq->pu8ScsiSts = pu8ScsiSts;

    /* Allocate and sync buffers if a data transfer is indicated. */
    if (cbBuf)
    {
        pReq->pvBuf = RTMemAlloc(cbBuf);
        if (RT_UNLIKELY(!pReq->pvBuf))
            rc = VERR_NO_MEMORY;
    }

    if (RT_SUCCESS(rc))
    {
        pReq->Seg.pvSeg = pReq->pvBuf;
        pReq->Seg.cbSeg = cbBuf;

        if (   cbBuf
            && (   enmTxDir == PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN
                || enmTxDir == PDMMEDIAEXIOREQSCSITXDIR_TO_DEVICE))
        {
            RTSGBUF SgBuf;
            RTSgBufInit(&SgBuf, &pReq->Seg, 1);
            rc = pThis->pDevMediaExPort->pfnIoReqCopyToBuf(pThis->pDevMediaExPort, hIoReq, &pReq->abAlloc[0],
                                                           0, &SgBuf, cbBuf);
        }

        if (RT_SUCCESS(rc))
        {
            rc = VSCSIDeviceReqCreate(pThis->hVScsiDevice, &pReq->hVScsiReq,
                                      uLun, (uint8_t *)pbCdb, cbCdb, cbBuf, 1, &pReq->Seg,
                                      pabSense, cbSense, pReq);
            if (RT_SUCCESS(rc))
            {
                ASMAtomicIncU32(&pThis->StatIoDepth);
                rc = VSCSIDeviceReqEnqueue(pThis->hVScsiDevice, pReq->hVScsiReq);
                if (RT_SUCCESS(rc))
                    rc = VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS;
            }
        }
    }

    return rc;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqGetActiveCount} */
static DECLCALLBACK(uint32_t) drvscsiIoReqGetActiveCount(PPDMIMEDIAEX pInterface)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMediaEx);
    return pThis->StatIoDepth;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqGetSuspendedCount} */
static DECLCALLBACK(uint32_t) drvscsiIoReqGetSuspendedCount(PPDMIMEDIAEX pInterface)
{
    RT_NOREF1(pInterface);
    return 0;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqQuerySuspendedStart} */
static DECLCALLBACK(int) drvscsiIoReqQuerySuspendedStart(PPDMIMEDIAEX pInterface, PPDMMEDIAEXIOREQ phIoReq, void **ppvIoReqAlloc)
{
    RT_NOREF3(pInterface, phIoReq, ppvIoReqAlloc);
    return VERR_NOT_IMPLEMENTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqQuerySuspendedNext} */
static DECLCALLBACK(int) drvscsiIoReqQuerySuspendedNext(PPDMIMEDIAEX pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                        PPDMMEDIAEXIOREQ phIoReqNext, void **ppvIoReqAllocNext)
{
    RT_NOREF4(pInterface, hIoReq, phIoReqNext, ppvIoReqAllocNext);
    return VERR_NOT_IMPLEMENTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqSuspendedSave} */
static DECLCALLBACK(int) drvscsiIoReqSuspendedSave(PPDMIMEDIAEX pInterface, PSSMHANDLE pSSM, PDMMEDIAEXIOREQ hIoReq)
{
    RT_NOREF3(pInterface, pSSM, hIoReq);
    return VERR_NOT_IMPLEMENTED;
}

/** @interface_method_impl{PDMIMEDIAEX,pfnIoReqSuspendedLoad} */
static DECLCALLBACK(int) drvscsiIoReqSuspendedLoad(PPDMIMEDIAEX pInterface, PSSMHANDLE pSSM, PDMMEDIAEXIOREQ hIoReq)
{
    RT_NOREF3(pInterface, pSSM, hIoReq);
    return VERR_NOT_IMPLEMENTED;
}


static DECLCALLBACK(void) drvscsiIoReqVScsiReqCompleted(VSCSIDEVICE hVScsiDevice, void *pVScsiDeviceUser,
                                                        void *pVScsiReqUser, int rcScsiCode, bool fRedoPossible, int rcReq)
{
    RT_NOREF2(hVScsiDevice, fRedoPossible);
    PDRVSCSI pThis = (PDRVSCSI)pVScsiDeviceUser;
    PDRVSCSIREQ pReq = (PDRVSCSIREQ)pVScsiReqUser;

    ASMAtomicDecU32(&pThis->StatIoDepth);

    /* Sync buffers. */
    if (   pReq->cbBuf
        && (   pReq->enmXferDir == PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN
            || pReq->enmXferDir == PDMMEDIAEXIOREQSCSITXDIR_FROM_DEVICE))
    {
        RTSGBUF SgBuf;
        RTSgBufInit(&SgBuf, &pReq->Seg, 1);
        int rcCopy = pThis->pDevMediaExPort->pfnIoReqCopyFromBuf(pThis->pDevMediaExPort, (PDMMEDIAEXIOREQ)pReq,
                                                                 &pReq->abAlloc[0], 0, &SgBuf, pReq->cbBuf);
        if (RT_FAILURE(rcCopy))
            rcReq = rcCopy;
    }

    *pReq->pu8ScsiSts = (uint8_t)rcScsiCode;
    int rc = pThis->pDevMediaExPort->pfnIoReqCompleteNotify(pThis->pDevMediaExPort, (PDMMEDIAEXIOREQ)pReq,
                                                            &pReq->abAlloc[0], rcReq);
    AssertRC(rc); RT_NOREF(rc);

    if (RT_UNLIKELY(pThis->fDummySignal) && !pThis->StatIoDepth)
        PDMDrvHlpAsyncNotificationCompleted(pThis->pDrvIns);
}

/**
 * Consumer for the queue
 *
 * @returns Success indicator.
 *          If false the item will not be removed and the flushing will stop.
 * @param   pDrvIns     The driver instance.
 * @param   pItem       The item to consume. Upon return this item will be freed.
 */
static DECLCALLBACK(bool) drvscsiR3NotifyQueueConsumer(PPDMDRVINS pDrvIns, PPDMQUEUEITEMCORE pItem)
{
    PDRVSCSIEJECTSTATE pEjectState = (PDRVSCSIEJECTSTATE)pItem;
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    int rc = pThis->pDrvMount->pfnUnmount(pThis->pDrvMount, false/*=fForce*/, true/*=fEject*/);
    Assert(RT_SUCCESS(rc) || rc == VERR_PDM_MEDIA_LOCKED || rc == VERR_PDM_MEDIA_NOT_MOUNTED);
    if (RT_SUCCESS(rc))
    {
        if (pThis->pDevMediaExPort)
            pThis->pDevMediaExPort->pfnMediumEjected(pThis->pDevMediaExPort);
    }

    RTSemEventSignal(pEjectState->hSemEvt);
    return true;
}

/* -=-=-=-=- IBase -=-=-=-=- */

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *)  drvscsiQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PPDMDRVINS  pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVSCSI    pThis   = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMOUNT, pThis->pDrvMount);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMISCSICONNECTOR, &pThis->ISCSIConnector);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAEX, pThis->pDevMediaExPort ? &pThis->IMediaEx : NULL);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIA, pThis->pDevMediaPort ? &pThis->IMedia : NULL);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAPORT, &pThis->IPort);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMOUNTNOTIFY, &pThis->IMountNotify);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAEXPORT, &pThis->IPortEx);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIA, pThis->pDrvMedia);
    return NULL;
}

static DECLCALLBACK(int) drvscsiQueryDeviceLocation(PPDMIMEDIAPORT pInterface, const char **ppcszController,
                                                    uint32_t *piInstance, uint32_t *piLUN)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IPort);

    if (pThis->pDevScsiPort)
        return pThis->pDevScsiPort->pfnQueryDeviceLocation(pThis->pDevScsiPort, ppcszController,
                                                           piInstance, piLUN);
    if (pThis->pDevMediaPort)
        return pThis->pDevMediaPort->pfnQueryDeviceLocation(pThis->pDevMediaPort, ppcszController,
                                                            piInstance, piLUN);

    return VERR_NOT_SUPPORTED;
}

/**
 * Called when media is mounted.
 *
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 */
static DECLCALLBACK(void) drvscsiMountNotify(PPDMIMOUNTNOTIFY pInterface)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMountNotify);
    LogFlowFunc(("mounting LUN#%p\n", pThis->hVScsiLun));

    /* Ignore the call if we're called while being attached. */
    if (!pThis->pDrvMedia)
        return;

    /* Let the LUN know that a medium was mounted. */
    VSCSILunMountNotify(pThis->hVScsiLun);
}

/**
 * Called when media is unmounted
 *
 * @param   pInterface      Pointer to the interface structure containing the called function pointer.
 */
static DECLCALLBACK(void) drvscsiUnmountNotify(PPDMIMOUNTNOTIFY pInterface)
{
    PDRVSCSI pThis = RT_FROM_MEMBER(pInterface, DRVSCSI, IMountNotify);
    LogFlowFunc(("unmounting LUN#%p\n", pThis->hVScsiLun));

    /* Let the LUN know that the medium was unmounted. */
    VSCSILunUnmountNotify(pThis->hVScsiLun);
}

/**
 * Worker for drvscsiReset, drvscsiSuspend and drvscsiPowerOff.
 *
 * @param   pDrvIns         The driver instance.
 * @param   pfnAsyncNotify  The async callback.
 */
static void drvscsiR3ResetOrSuspendOrPowerOff(PPDMDRVINS pDrvIns, PFNPDMDRVASYNCNOTIFY pfnAsyncNotify)
{
    RT_NOREF1(pfnAsyncNotify);

    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    if (pThis->StatIoDepth > 0)
        ASMAtomicWriteBool(&pThis->fDummySignal, true);
}

/**
 * Callback employed by drvscsiSuspend and drvscsiPowerOff.
 *
 * @returns true if we've quiesced, false if we're still working.
 * @param   pDrvIns     The driver instance.
 */
static DECLCALLBACK(bool) drvscsiIsAsyncSuspendOrPowerOffDone(PPDMDRVINS pDrvIns)
{
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    if (pThis->StatIoDepth > 0)
        return false;
    else
        return true;
}

/**
 * @copydoc FNPDMDRVPOWEROFF
 */
static DECLCALLBACK(void) drvscsiPowerOff(PPDMDRVINS pDrvIns)
{
    drvscsiR3ResetOrSuspendOrPowerOff(pDrvIns, drvscsiIsAsyncSuspendOrPowerOffDone);
}

/**
 * @copydoc FNPDMDRVSUSPEND
 */
static DECLCALLBACK(void) drvscsiSuspend(PPDMDRVINS pDrvIns)
{
    drvscsiR3ResetOrSuspendOrPowerOff(pDrvIns, drvscsiIsAsyncSuspendOrPowerOffDone);
}

/**
 * Callback employed by drvscsiReset.
 *
 * @returns true if we've quiesced, false if we're still working.
 * @param   pDrvIns     The driver instance.
 */
static DECLCALLBACK(bool) drvscsiIsAsyncResetDone(PPDMDRVINS pDrvIns)
{
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    if (pThis->StatIoDepth > 0)
        return false;
    else
        return true;
}

/** @copydoc FNPDMDRVATTACH */
static DECLCALLBACK(int) drvscsiAttach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    LogFlowFunc(("pDrvIns=%#p fFlags=%#x\n", pDrvIns, fFlags));

    AssertMsgReturn((fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG),
                    ("SCSI: Hotplugging is not supported\n"),
                    VERR_INVALID_PARAMETER);

    /*
     * Try attach driver below and query it's media interface.
     */
    int rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pThis->pDrvBase);
    AssertMsgReturn(RT_SUCCESS(rc), ("Attaching driver below failed rc=%Rrc\n", rc), rc);

    /*
     * Query the media interface.
     */
    pThis->pDrvMedia = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIMEDIA);
    AssertMsgReturn(VALID_PTR(pThis->pDrvMedia), ("VSCSI configuration error: No media interface!\n"),
                    VERR_PDM_MISSING_INTERFACE);

    /* Query the extended media interface. */
    pThis->pDrvMediaEx = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIMEDIAEX);
    AssertMsgReturn(VALID_PTR(pThis->pDrvMediaEx), ("VSCSI configuration error: No extended media interface!\n"),
                    VERR_PDM_MISSING_INTERFACE);

    pThis->pDrvMount = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIMOUNT);

    if (pThis->cbVScsiIoReqAlloc)
    {
        rc = pThis->pDrvMediaEx->pfnIoReqAllocSizeSet(pThis->pDrvMediaEx, pThis->cbVScsiIoReqAlloc);
        AssertMsgReturn(RT_SUCCESS(rc), ("Setting the I/O request allocation size failed with rc=%Rrc\n", rc), rc);
    }

    if (pThis->pDrvMount)
    {
        if (pThis->pDrvMedia->pfnGetSize(pThis->pDrvMedia))
        {
            rc = VINF_SUCCESS; VSCSILunMountNotify(pThis->hVScsiLun);
            AssertMsgReturn(RT_SUCCESS(rc), ("Failed to notify the LUN of media being mounted\n"), rc);
        }
        else
        {
            rc = VINF_SUCCESS; VSCSILunUnmountNotify(pThis->hVScsiLun);
            AssertMsgReturn(RT_SUCCESS(rc), ("Failed to notify the LUN of media being unmounted\n"), rc);
        }
    }

    return rc;
}

/** @copydoc FNPDMDRVDETACH */
static DECLCALLBACK(void) drvscsiDetach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);

    LogFlowFunc(("pDrvIns=%#p fFlags=%#x\n", pDrvIns, fFlags));

    AssertMsgReturnVoid((fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG),
                        ("SCSI: Hotplugging is not supported\n"));

    /*
     * Zero some important members.
     */
    pThis->pDrvBase = NULL;
    pThis->pDrvMedia = NULL;
    pThis->pDrvMediaEx = NULL;
    pThis->pDrvMount = NULL;
}

/**
 * @copydoc FNPDMDRVRESET
 */
static DECLCALLBACK(void) drvscsiReset(PPDMDRVINS pDrvIns)
{
    drvscsiR3ResetOrSuspendOrPowerOff(pDrvIns, drvscsiIsAsyncResetDone);
}

/**
 * Destruct a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvscsiDestruct(PPDMDRVINS pDrvIns)
{
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);

    /* Free the VSCSI device and LUN handle. */
    if (pThis->hVScsiDevice)
    {
        VSCSILUN hVScsiLun;
        int rc = VSCSIDeviceLunDetach(pThis->hVScsiDevice, 0, &hVScsiLun);
        AssertRC(rc);

        Assert(hVScsiLun == pThis->hVScsiLun);
        rc = VSCSILunDestroy(hVScsiLun);
        AssertRC(rc);
        rc = VSCSIDeviceDestroy(pThis->hVScsiDevice);
        AssertRC(rc);

        pThis->hVScsiDevice = NULL;
        pThis->hVScsiLun    = NULL;
    }

    PDMDrvHlpSTAMDeregister(pDrvIns, &pThis->StatBytesRead);
    PDMDrvHlpSTAMDeregister(pDrvIns, &pThis->StatBytesWritten);
    PDMDrvHlpSTAMDeregister(pDrvIns, (void *)&pThis->StatIoDepth);
}

/**
 * Construct a block driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvscsiConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, uint32_t fFlags)
{
    int rc = VINF_SUCCESS;
    PDRVSCSI pThis = PDMINS_2_DATA(pDrvIns, PDRVSCSI);
    LogFlowFunc(("pDrvIns=%#p pCfg=%#p\n", pDrvIns, pCfg));
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    /*
     * Initialize the instance data.
     */
    pThis->pDrvIns                              = pDrvIns;
    pThis->ISCSIConnector.pfnSCSIRequestSend    = drvscsiRequestSend;
    pThis->ISCSIConnector.pfnQueryLUNType       = drvscsiQueryLUNType;

    pDrvIns->IBase.pfnQueryInterface            = drvscsiQueryInterface;

    /* IMedia */
    pThis->IMedia.pfnRead                       = NULL;
    pThis->IMedia.pfnReadPcBios                 = NULL;
    pThis->IMedia.pfnWrite                      = NULL;
    pThis->IMedia.pfnFlush                      = NULL;
    pThis->IMedia.pfnSendCmd                    = NULL;
    pThis->IMedia.pfnMerge                      = NULL;
    pThis->IMedia.pfnSetSecKeyIf                = NULL;
    pThis->IMedia.pfnGetSize                    = NULL;
    pThis->IMedia.pfnGetSectorSize              = NULL;
    pThis->IMedia.pfnIsReadOnly                 = NULL;
    pThis->IMedia.pfnBiosGetPCHSGeometry        = NULL;
    pThis->IMedia.pfnBiosSetPCHSGeometry        = NULL;
    pThis->IMedia.pfnBiosGetLCHSGeometry        = NULL;
    pThis->IMedia.pfnBiosSetLCHSGeometry        = NULL;
    pThis->IMedia.pfnBiosIsVisible              = NULL;
    pThis->IMedia.pfnGetType                    = drvscsiGetType;
    pThis->IMedia.pfnGetUuid                    = drvscsiGetUuid;
    pThis->IMedia.pfnDiscard                    = NULL;

    /* IMediaEx */
    pThis->IMediaEx.pfnQueryFeatures            = drvscsiQueryFeatures;
    pThis->IMediaEx.pfnIoReqAllocSizeSet        = drvscsiIoReqAllocSizeSet;
    pThis->IMediaEx.pfnIoReqAlloc               = drvscsiIoReqAlloc;
    pThis->IMediaEx.pfnIoReqFree                = drvscsiIoReqFree;
    pThis->IMediaEx.pfnIoReqQueryResidual       = drvscsiIoReqQueryResidual;
    pThis->IMediaEx.pfnIoReqCancelAll           = drvscsiIoReqCancelAll;
    pThis->IMediaEx.pfnIoReqCancel              = drvscsiIoReqCancel;
    pThis->IMediaEx.pfnIoReqRead                = drvscsiIoReqRead;
    pThis->IMediaEx.pfnIoReqWrite               = drvscsiIoReqWrite;
    pThis->IMediaEx.pfnIoReqFlush               = drvscsiIoReqFlush;
    pThis->IMediaEx.pfnIoReqDiscard             = drvscsiIoReqDiscard;
    pThis->IMediaEx.pfnIoReqSendScsiCmd         = drvscsiIoReqSendScsiCmd;
    pThis->IMediaEx.pfnIoReqGetActiveCount      = drvscsiIoReqGetActiveCount;
    pThis->IMediaEx.pfnIoReqGetSuspendedCount   = drvscsiIoReqGetSuspendedCount;
    pThis->IMediaEx.pfnIoReqQuerySuspendedStart = drvscsiIoReqQuerySuspendedStart;
    pThis->IMediaEx.pfnIoReqQuerySuspendedNext  = drvscsiIoReqQuerySuspendedNext;
    pThis->IMediaEx.pfnIoReqSuspendedSave       = drvscsiIoReqSuspendedSave;
    pThis->IMediaEx.pfnIoReqSuspendedLoad       = drvscsiIoReqSuspendedLoad;

    pThis->IMountNotify.pfnMountNotify          = drvscsiMountNotify;
    pThis->IMountNotify.pfnUnmountNotify        = drvscsiUnmountNotify;
    pThis->IPort.pfnQueryDeviceLocation         = drvscsiQueryDeviceLocation;
    pThis->IPortEx.pfnIoReqCompleteNotify       = drvscsiIoReqCompleteNotify;
    pThis->IPortEx.pfnIoReqCopyFromBuf          = drvscsiIoReqCopyFromBuf;
    pThis->IPortEx.pfnIoReqCopyToBuf            = drvscsiIoReqCopyToBuf;
    pThis->IPortEx.pfnIoReqQueryDiscardRanges   = drvscsiIoReqQueryDiscardRanges;
    pThis->IPortEx.pfnIoReqStateChanged         = drvscsiIoReqStateChanged;

    /* Query the optional SCSI port interface above. */
    pThis->pDevScsiPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMISCSIPORT);

    /* Query the optional media port interface above. */
    pThis->pDevMediaPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIMEDIAPORT);

    /* Query the optional extended media port interface above. */
    pThis->pDevMediaExPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIMEDIAEXPORT);

    AssertMsgReturn(pThis->pDevScsiPort || pThis->pDevMediaExPort,
                    ("Missing SCSI or extended media port interface above\n"), VERR_PDM_MISSING_INTERFACE);

    /* Query the optional LED interface above. */
    pThis->pLedPort = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMILEDPORTS);
    if (pThis->pLedPort != NULL)
    {
        /* Get The Led. */
        rc = pThis->pLedPort->pfnQueryStatusLed(pThis->pLedPort, 0, &pThis->pLed);
        if (RT_FAILURE(rc))
            pThis->pLed = &pThis->Led;
    }
    else
        pThis->pLed = &pThis->Led;

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg, ""))
        return PDMDRV_SET_ERROR(pDrvIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("SCSI configuration error: unknown option specified"));

    /*
     * Try attach driver below and query it's media interface.
     */
    rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pThis->pDrvBase);
    AssertMsgReturn(RT_SUCCESS(rc), ("Attaching driver below failed rc=%Rrc\n", rc), rc);

    /*
     * Query the media interface.
     */
    pThis->pDrvMedia = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIMEDIA);
    AssertMsgReturn(VALID_PTR(pThis->pDrvMedia), ("VSCSI configuration error: No media interface!\n"),
                    VERR_PDM_MISSING_INTERFACE);

    /* Query the extended media interface. */
    pThis->pDrvMediaEx = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIMEDIAEX);
    AssertMsgReturn(VALID_PTR(pThis->pDrvMediaEx), ("VSCSI configuration error: No extended media interface!\n"),
                    VERR_PDM_MISSING_INTERFACE);

    pThis->pDrvMount = PDMIBASE_QUERY_INTERFACE(pThis->pDrvBase, PDMIMOUNT);

    PDMMEDIATYPE enmType = pThis->pDrvMedia->pfnGetType(pThis->pDrvMedia);
    VSCSILUNTYPE enmLunType;
    switch (enmType)
    {
    case PDMMEDIATYPE_HARD_DISK:
        enmLunType = VSCSILUNTYPE_SBC;
        break;
    case PDMMEDIATYPE_CDROM:
    case PDMMEDIATYPE_DVD:
        enmLunType = VSCSILUNTYPE_MMC;
        break;
    default:
        return PDMDrvHlpVMSetError(pDrvIns, VERR_PDM_UNSUPPORTED_BLOCK_TYPE, RT_SRC_POS,
                                   N_("Only hard disks and CD/DVD-ROMs are currently supported as SCSI devices (enmType=%d)"),
                                   enmType);
    }
    if (    (   enmType == PDMMEDIATYPE_DVD
             || enmType == PDMMEDIATYPE_CDROM)
        &&  !pThis->pDrvMount)
    {
        AssertMsgFailed(("Internal error: cdrom without a mountable interface\n"));
        return VERR_INTERNAL_ERROR;
    }

    /* Create VSCSI device and LUN. */
    pThis->VScsiIoCallbacks.pfnVScsiLunReqAllocSizeSet     = drvscsiReqAllocSizeSet;
    pThis->VScsiIoCallbacks.pfnVScsiLunReqAlloc            = drvscsiReqAlloc;
    pThis->VScsiIoCallbacks.pfnVScsiLunReqFree             = drvscsiReqFree;
    pThis->VScsiIoCallbacks.pfnVScsiLunMediumGetSize       = drvscsiGetSize;
    pThis->VScsiIoCallbacks.pfnVScsiLunMediumGetSectorSize = drvscsiGetSectorSize;
    pThis->VScsiIoCallbacks.pfnVScsiLunMediumEject         = drvscsiEject;
    pThis->VScsiIoCallbacks.pfnVScsiLunReqTransferEnqueue  = drvscsiReqTransferEnqueue;
    pThis->VScsiIoCallbacks.pfnVScsiLunGetFeatureFlags     = drvscsiGetFeatureFlags;
    pThis->VScsiIoCallbacks.pfnVScsiLunMediumSetLock       = drvscsiSetLock;

    rc = VSCSIDeviceCreate(&pThis->hVScsiDevice,
                             pThis->pDevMediaExPort
                           ? drvscsiIoReqVScsiReqCompleted
                           : drvscsiVScsiReqCompleted,
                           pThis);
    AssertMsgReturn(RT_SUCCESS(rc), ("Failed to create VSCSI device rc=%Rrc\n", rc), rc);
    rc = VSCSILunCreate(&pThis->hVScsiLun, enmLunType, &pThis->VScsiIoCallbacks,
                        pThis);
    AssertMsgReturn(RT_SUCCESS(rc), ("Failed to create VSCSI LUN rc=%Rrc\n", rc), rc);
    rc = VSCSIDeviceLunAttach(pThis->hVScsiDevice, pThis->hVScsiLun, 0);
    AssertMsgReturn(RT_SUCCESS(rc), ("Failed to attached the LUN to the SCSI device\n"), rc);

    /// @todo This is a very hacky way of telling the LUN whether a medium was mounted.
    // The mount/unmount interface doesn't work in a very sensible manner!
    if (pThis->pDrvMount)
    {
        if (pThis->pDrvMedia->pfnGetSize(pThis->pDrvMedia))
        {
            rc = VINF_SUCCESS; VSCSILunMountNotify(pThis->hVScsiLun);
            AssertMsgReturn(RT_SUCCESS(rc), ("Failed to notify the LUN of media being mounted\n"), rc);
        }
        else
        {
            rc = VINF_SUCCESS; VSCSILunUnmountNotify(pThis->hVScsiLun);
            AssertMsgReturn(RT_SUCCESS(rc), ("Failed to notify the LUN of media being unmounted\n"), rc);
        }
    }

    const char *pszCtrl = NULL;
    uint32_t iCtrlInstance = 0;
    uint32_t iCtrlLun = 0;

    if (pThis->pDevScsiPort)
        rc = pThis->pDevScsiPort->pfnQueryDeviceLocation(pThis->pDevScsiPort, &pszCtrl, &iCtrlInstance, &iCtrlLun);
    if (pThis->pDevMediaPort)
        rc = pThis->pDevMediaPort->pfnQueryDeviceLocation(pThis->pDevMediaPort, &pszCtrl, &iCtrlInstance, &iCtrlLun);
    if (RT_SUCCESS(rc))
    {
        const char *pszCtrlId =   strcmp(pszCtrl, "Msd") == 0 ? "USB"
                                : strcmp(pszCtrl, "lsilogicsas") == 0 ? "SAS"
                                : "SCSI";
        /* Register statistics counter. */
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatBytesRead, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                                "Amount of data read.", "/Devices/%s%u/%u/ReadBytes", pszCtrlId, iCtrlInstance, iCtrlLun);
        PDMDrvHlpSTAMRegisterF(pDrvIns, &pThis->StatBytesWritten, STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_BYTES,
                                "Amount of data written.", "/Devices/%s%u/%u/WrittenBytes", pszCtrlId, iCtrlInstance, iCtrlLun);
        PDMDrvHlpSTAMRegisterF(pDrvIns, (void *)&pThis->StatIoDepth, STAMTYPE_U32, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                                "Number of active tasks.", "/Devices/%s%u/%u/IoDepth", pszCtrlId, iCtrlInstance, iCtrlLun);
    }

    pThis->StatIoDepth = 0;

    uint32_t fFeatures = 0;
    rc = pThis->pDrvMediaEx->pfnQueryFeatures(pThis->pDrvMediaEx, &fFeatures);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("VSCSI configuration error: Failed to query features of device"));
    if (fFeatures & PDMIMEDIAEX_FEATURE_F_DISCARD)
        LogRel(("SCSI#%d: Enabled UNMAP support\n", pDrvIns->iInstance));

    rc = PDMDrvHlpQueueCreate(pDrvIns, sizeof(DRVSCSIEJECTSTATE), 1, 0, drvscsiR3NotifyQueueConsumer,
                              "SCSI-Eject", &pThis->pQueue);
    if (RT_FAILURE(rc))
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("VSCSI configuration error: Failed to create notification queue"));

    return VINF_SUCCESS;
}

/**
 * SCSI driver registration record.
 */
const PDMDRVREG g_DrvSCSI =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "SCSI",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Generic SCSI driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_SCSI,
    /* cMaxInstances */
    ~0U,
    /* cbInstance */
    sizeof(DRVSCSI),
    /* pfnConstruct */
    drvscsiConstruct,
    /* pfnDestruct */
    drvscsiDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    drvscsiReset,
    /* pfnSuspend */
    drvscsiSuspend,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    drvscsiAttach,
    /* pfnDetach */
    drvscsiDetach,
    /* pfnPowerOff */
    drvscsiPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};

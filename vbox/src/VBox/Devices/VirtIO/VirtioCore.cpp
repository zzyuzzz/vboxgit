/* $Id: VirtioCore.cpp 84819 2020-06-12 20:00:27Z vboxsync $ */

/** @file
 * VirtioCore - Virtio Core (PCI, feature & config mgt, queue mgt & proxy, notification mgt)
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_VIRTIO

#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/mem.h>
#include <iprt/sg.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/param.h>
#include <iprt/types.h>
#include <VBox/log.h>
#include <VBox/msi.h>
#include <iprt/types.h>
#include <VBox/AssertGuest.h>
#include <VBox/vmm/pdmdev.h>
#include "VirtioCore.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
#define INSTANCE(a_pVirtio)                 ((a_pVirtio)->szInstance)
#define VIRTQNAME(a_pVirtio, a_uVirtqNbr)    ((a_pVirtio)->aVirtqState[(a_uVirtqNbr)].szVirtqName)
#define IS_DRIVER_OK(a_pVirtio)             ((a_pVirtio)->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK)
#define IS_VIRTQ_EMPTY(pDevIns, pVirtio, pVirtqState) \
            (virtioCoreVirtqAvailCount(pDevIns, pVirtio, pVirtqState) == 0)

/**
 * This macro returns true if the @a a_offAccess and access length (@a
 * a_cbAccess) are within the range of the mapped capability struct described by
 * @a a_LocCapData.
 *
 * @param[in]  a_offAccess      The offset into the MMIO bar of the access.
 * @param[in]  a_cbAccess       The access size.
 * @param[out] a_offsetInMbr    The variable to return the intra-capability
 *                              offset into.  ASSUMES this is uint32_t.
 * @param[in]  a_LocCapData     The capability location info.
 */
#define MATCHES_VIRTIO_CAP_STRUCT(a_offAccess, a_cbAccess, a_offsetInMbr, a_LocCapData) \
    (   ((a_offsetInMbr) = (uint32_t)((a_offAccess) - (a_LocCapData).offMmio)) < (uint32_t)(a_LocCapData).cbMmio \
     && (a_offsetInMbr) + (uint32_t)(a_cbAccess) <= (uint32_t)(a_LocCapData).cbMmio )


/** Marks the start of the virtio saved state (just for sanity). */
#define VIRTIO_SAVEDSTATE_MARKER                        UINT64_C(0x1133557799bbddff)
/** The current saved state version for the virtio core. */
#define VIRTIO_SAVEDSTATE_VERSION                       UINT32_C(1)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/


/** @name virtq related flags
 * @{ */
#define VIRTQ_DESC_F_NEXT                               1        /**< Indicates this descriptor chains to next  */
#define VIRTQ_DESC_F_WRITE                              2        /**< Marks buffer as write-only (default ro)   */
#define VIRTQ_DESC_F_INDIRECT                           4        /**< Buffer is list of buffer descriptors      */

#define VIRTQ_USED_F_NO_NOTIFY                          1        /**< Dev to Drv: Don't notify when buf added   */
#define VIRTQ_AVAIL_F_NO_INTERRUPT                      1        /**< Drv to Dev: Don't notify when buf eaten   */
/** @} */

/**
 * virtq related structs
 * (struct names follow VirtIO 1.0 spec, typedef use VBox style)
 */
typedef struct virtq_desc
{
    uint64_t  GCPhysBuf;                                         /**< addr       GC Phys. address of buffer     */
    uint32_t  cb;                                                /**< len        Buffer length                  */
    uint16_t  fFlags;                                            /**< flags      Buffer specific flags          */
    uint16_t  uDescIdxNext;                                      /**< next       Idx set if VIRTIO_DESC_F_NEXT  */
} VIRTQ_DESC_T, *PVIRTQ_DESC_T;

typedef struct virtq_avail
{
    uint16_t  fFlags;                                            /**< flags      avail ring guest-to-host flags */
    uint16_t  uIdx;                                              /**< idx        Index of next free ring slot   */
    RT_FLEXIBLE_ARRAY_EXTENSION
    uint16_t  auRing[RT_FLEXIBLE_ARRAY];                         /**< ring       Ring: avail drv to dev bufs    */
    /* uint16_t  uUsedEventIdx;                                     - used_event (if VIRTQ_USED_F_EVENT_IDX)    */
} VIRTQ_AVAIL_T, *PVIRTQ_AVAIL_T;

typedef struct virtq_used_elem
{
    uint32_t  uDescIdx;                                          /**< idx         Start of used desc chain      */
    uint32_t  cbElem;                                            /**< len         Total len of used desc chain  */
} VIRTQ_USED_ELEM_T;

typedef struct virt_used
{
    uint16_t  fFlags;                                            /**< flags       used ring host-to-guest flags */
    uint16_t  uIdx;                                              /**< idx         Index of next ring slot       */
    RT_FLEXIBLE_ARRAY_EXTENSION
    VIRTQ_USED_ELEM_T aRing[RT_FLEXIBLE_ARRAY];                  /**< ring        Ring: used dev to drv bufs    */
    /* uint16_t  uAvailEventIdx;                                    - avail_event if (VIRTQ_USED_F_EVENT_IDX)   */
} VIRTQ_USED_T, *PVIRTQ_USED_T;


const char *virtioCoreGetStateChangeText(VIRTIOVMSTATECHANGED enmState)
{
    switch (enmState)
    {
        case kvirtIoVmStateChangedReset:                return "VM RESET";
        case kvirtIoVmStateChangedSuspend:              return "VM SUSPEND";
        case kvirtIoVmStateChangedPowerOff:             return "VM POWER OFF";
        case kvirtIoVmStateChangedResume:               return "VM RESUME";
        default:                                        return "<BAD ENUM>";
    }
}

/* Internal Functions */

static void virtioCoreNotifyGuestDriver(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr);
static int  virtioKick(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint8_t uCause, uint16_t uVec);

/** @name Internal queue operations
 * @{ */

/**
 * Accessor for virtq descriptor
 */
#ifdef IN_RING3
DECLINLINE(void) virtioReadDesc(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                                uint32_t idxDesc, PVIRTQ_DESC_T pDesc)
{
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    uint16_t const cVirtqItems = RT_MAX(pVirtio->uVirtqSize[uVirtqNbr], 1); /* Make sure to avoid div-by-zero. */
    PDMDevHlpPCIPhysRead(pDevIns,
                      pVirtio->aGCPhysVirtqDesc[uVirtqNbr] + sizeof(VIRTQ_DESC_T) * (idxDesc % cVirtqItems),
                      pDesc, sizeof(VIRTQ_DESC_T));
}
#endif

/**
 * Accessors for virtq avail ring
 */
#ifdef IN_RING3
DECLINLINE(uint16_t) virtioReadAvailDescIdx(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, uint32_t availIdx)
{
    uint16_t uDescIdx;
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    uint16_t const cVirtqItems = RT_MAX(pVirtio->uVirtqSize[uVirtqNbr], 1); /* Make sure to avoid div-by-zero. */
    PDMDevHlpPCIPhysRead(pDevIns,
                        pVirtio->aGCPhysVirtqAvail[uVirtqNbr]
                      + RT_UOFFSETOF_DYN(VIRTQ_AVAIL_T, auRing[availIdx % cVirtqItems]),
                      &uDescIdx, sizeof(uDescIdx));
    return uDescIdx;
}

DECLINLINE(uint16_t) virtioReadAvailUsedEvent(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    uint16_t uUsedEventIdx;
    /* VirtIO 1.0 uUsedEventIdx (used_event) immediately follows ring */
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysRead(pDevIns,
                      pVirtio->aGCPhysVirtqAvail[uVirtqNbr] + RT_UOFFSETOF_DYN(VIRTQ_AVAIL_T, auRing[pVirtio->uVirtqSize[uVirtqNbr]]),
                      &uUsedEventIdx, sizeof(uUsedEventIdx));
    return uUsedEventIdx;
}
#endif

DECLINLINE(uint16_t) virtioReadAvailRingIdx(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    uint16_t uIdx = 0;
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysRead(pDevIns,
                      pVirtio->aGCPhysVirtqAvail[uVirtqNbr] + RT_UOFFSETOF(VIRTQ_AVAIL_T, uIdx),
                      &uIdx, sizeof(uIdx));
    return uIdx;
}

DECLINLINE(uint16_t) virtioReadAvailRingFlags(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    uint16_t fFlags = 0;
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysRead(pDevIns,
                      pVirtio->aGCPhysVirtqAvail[uVirtqNbr] + RT_UOFFSETOF(VIRTQ_AVAIL_T, fFlags),
                      &fFlags, sizeof(fFlags));
    return fFlags;
}

/** @} */

/** @name Accessors for virtq used ring
 * @{
 */

#ifdef IN_RING3
DECLINLINE(void) virtioWriteUsedElem(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                                     uint32_t usedIdx, uint32_t uDescIdx, uint32_t uLen)
{
    VIRTQ_USED_ELEM_T elem = { uDescIdx,  uLen };
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    uint16_t const cVirtqItems = RT_MAX(pVirtio->uVirtqSize[uVirtqNbr], 1); /* Make sure to avoid div-by-zero. */
    PDMDevHlpPCIPhysWrite(pDevIns,
                          pVirtio->aGCPhysVirtqUsed[uVirtqNbr] + RT_UOFFSETOF_DYN(VIRTQ_USED_T, aRing[usedIdx % cVirtqItems]),
                          &elem, sizeof(elem));
}

DECLINLINE(void) virtioWriteUsedRingFlags(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, uint16_t fFlags)
{
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    RT_UNTRUSTED_VALIDATED_FENCE(); /* VirtIO 1.0, Section 3.2.1.4.1 */
    PDMDevHlpPCIPhysWrite(pDevIns,
                          pVirtio->aGCPhysVirtqUsed[uVirtqNbr] + RT_UOFFSETOF(VIRTQ_USED_T, fFlags),
                          &fFlags, sizeof(fFlags));
}
#endif

DECLINLINE(void) virtioWriteUsedRingIdx(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, uint16_t uIdx)
{
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysWrite(pDevIns,
                          pVirtio->aGCPhysVirtqUsed[uVirtqNbr] + RT_UOFFSETOF(VIRTQ_USED_T, uIdx),
                          &uIdx, sizeof(uIdx));
}


#ifdef IN_RING3

DECLINLINE(uint16_t) virtioReadUsedRingIdx(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    uint16_t uIdx = 0;
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysRead(pDevIns,
                      pVirtio->aGCPhysVirtqUsed[uVirtqNbr] + RT_UOFFSETOF(VIRTQ_USED_T, uIdx),
                      &uIdx, sizeof(uIdx));
    return uIdx;
}

DECLINLINE(uint16_t) virtioReadUsedRingFlags(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    uint16_t fFlags = 0;
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysRead(pDevIns,
                      pVirtio->aGCPhysVirtqUsed[uVirtqNbr] + RT_UOFFSETOF(VIRTQ_USED_T, fFlags),
                      &fFlags, sizeof(fFlags));
    return fFlags;
}

DECLINLINE(void) virtioWriteUsedAvailEvent(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, uint32_t uAvailEventIdx)
{
    /** VirtIO 1.0 uAvailEventIdx (avail_event) immediately follows ring */
    AssertMsg(pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK, ("Called with guest driver not ready\n"));
    PDMDevHlpPCIPhysWrite(pDevIns,
                          pVirtio->aGCPhysVirtqUsed[uVirtqNbr] + RT_UOFFSETOF_DYN(VIRTQ_USED_T, aRing[pVirtio->uVirtqSize[uVirtqNbr]]),
                          &uAvailEventIdx, sizeof(uAvailEventIdx));
}


#endif

DECLINLINE(uint16_t) virtioCoreVirtqAvailCount(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTQSTATE pVirtqState)
{
    uint16_t uIdx    = virtioReadAvailRingIdx(pDevIns, pVirtio, pVirtqState->uVirtqNbr);
    uint16_t uShadow = pVirtqState->uAvailIdxShadow;

    uint16_t uDelta;
    if (uIdx < uShadow)
        uDelta = (uIdx + VIRTQ_MAX_ENTRIES) - uShadow;
    else
        uDelta = uIdx - uShadow;

    LogFunc(("%s has %u %s (idx=%u shadow=%u)\n",
        VIRTQNAME(pVirtio, pVirtqState->uVirtqNbr), uDelta, uDelta == 1 ? "entry" : "entries",
        uIdx, uShadow));

    return uDelta;
}
/**
 * Get count of new (e.g. pending) elements in available ring.
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr    Virtq number
 *
 * @returns how many entries have been added to ring as a delta of the consumer's
 *          avail index and the queue's guest-side current avail index.
 */
uint16_t virtioCoreVirtqAvailCount(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    if (!IS_DRIVER_OK(pVirtio) || !pVirtio->uVirtqEnable[uVirtqNbr])
    {
        LogRelFunc(("Driver not ready or queue not enabled\n"));
        return 0;
    }
    return virtioCoreVirtqAvailCount(pDevIns, pVirtio, &pVirtio->aVirtqState[uVirtqNbr]);
}


/** @} */

void virtioCoreSgBufInit(PVIRTIOSGBUF pGcSgBuf, PVIRTIOSGSEG paSegs, size_t cSegs)
{
    AssertPtr(pGcSgBuf);
    Assert(   (cSegs > 0 && VALID_PTR(paSegs)) || (!cSegs && !paSegs));
    Assert(cSegs < (~(unsigned)0 >> 1));

    pGcSgBuf->paSegs = paSegs;
    pGcSgBuf->cSegs  = (unsigned)cSegs;
    pGcSgBuf->idxSeg = 0;
    if (cSegs && paSegs)
    {
        pGcSgBuf->GCPhysCur = paSegs[0].GCPhys;
        pGcSgBuf->cbSegLeft = paSegs[0].cbSeg;
    }
    else
    {
        pGcSgBuf->GCPhysCur = 0;
        pGcSgBuf->cbSegLeft = 0;
    }
}

static RTGCPHYS virtioCoreSgBufGet(PVIRTIOSGBUF pGcSgBuf, size_t *pcbData)
{
    size_t cbData;
    RTGCPHYS pGcBuf;

    /* Check that the S/G buffer has memory left. */
    if (RT_LIKELY(pGcSgBuf->idxSeg < pGcSgBuf->cSegs && pGcSgBuf->cbSegLeft))
    { /* likely */ }
    else
    {
        *pcbData = 0;
        return 0;
    }

    AssertMsg(    pGcSgBuf->cbSegLeft <= 128 * _1M
              && (RTGCPHYS)pGcSgBuf->GCPhysCur >= (RTGCPHYS)pGcSgBuf->paSegs[pGcSgBuf->idxSeg].GCPhys
              && (RTGCPHYS)pGcSgBuf->GCPhysCur + pGcSgBuf->cbSegLeft <=
                   (RTGCPHYS)pGcSgBuf->paSegs[pGcSgBuf->idxSeg].GCPhys + pGcSgBuf->paSegs[pGcSgBuf->idxSeg].cbSeg,
                 ("pGcSgBuf->idxSeg=%d pGcSgBuf->cSegs=%d pGcSgBuf->GCPhysCur=%p pGcSgBuf->cbSegLeft=%zd "
                  "pGcSgBuf->paSegs[%d].GCPhys=%p pGcSgBuf->paSegs[%d].cbSeg=%zd\n",
                  pGcSgBuf->idxSeg, pGcSgBuf->cSegs, pGcSgBuf->GCPhysCur, pGcSgBuf->cbSegLeft,
                  pGcSgBuf->idxSeg, pGcSgBuf->paSegs[pGcSgBuf->idxSeg].GCPhys, pGcSgBuf->idxSeg,
                  pGcSgBuf->paSegs[pGcSgBuf->idxSeg].cbSeg));

    cbData = RT_MIN(*pcbData, pGcSgBuf->cbSegLeft);
    pGcBuf = pGcSgBuf->GCPhysCur;
    pGcSgBuf->cbSegLeft -= cbData;
    if (!pGcSgBuf->cbSegLeft)
    {
        pGcSgBuf->idxSeg++;

        if (pGcSgBuf->idxSeg < pGcSgBuf->cSegs)
        {
            pGcSgBuf->GCPhysCur = pGcSgBuf->paSegs[pGcSgBuf->idxSeg].GCPhys;
            pGcSgBuf->cbSegLeft = pGcSgBuf->paSegs[pGcSgBuf->idxSeg].cbSeg;
        }
        *pcbData = cbData;
    }
    else
        pGcSgBuf->GCPhysCur = pGcSgBuf->GCPhysCur + cbData;

    return pGcBuf;
}

void virtioCoreSgBufReset(PVIRTIOSGBUF pGcSgBuf)
{
    AssertPtrReturnVoid(pGcSgBuf);

    pGcSgBuf->idxSeg = 0;
    if (pGcSgBuf->cSegs)
    {
        pGcSgBuf->GCPhysCur = pGcSgBuf->paSegs[0].GCPhys;
        pGcSgBuf->cbSegLeft = pGcSgBuf->paSegs[0].cbSeg;
    }
    else
    {
        pGcSgBuf->GCPhysCur = 0;
        pGcSgBuf->cbSegLeft = 0;
    }
}

RTGCPHYS virtioCoreSgBufAdvance(PVIRTIOSGBUF pGcSgBuf, size_t cbAdvance)
{
    AssertReturn(pGcSgBuf, 0);

    size_t cbLeft = cbAdvance;
    while (cbLeft)
    {
        size_t cbThisAdvance = cbLeft;
        virtioCoreSgBufGet(pGcSgBuf, &cbThisAdvance);
        if (!cbThisAdvance)
            break;

        cbLeft -= cbThisAdvance;
    }
    return cbAdvance - cbLeft;
}

RTGCPHYS virtioCoreSgBufGetNextSegment(PVIRTIOSGBUF pGcSgBuf, size_t *pcbSeg)
{
    AssertReturn(pGcSgBuf, 0);
    AssertPtrReturn(pcbSeg, 0);

    if (!*pcbSeg)
        *pcbSeg = pGcSgBuf->cbSegLeft;

    return virtioCoreSgBufGet(pGcSgBuf, pcbSeg);
}

size_t virtioCoreSgBufCalcTotalLength(PVIRTIOSGBUF pGcSgBuf)
{
    size_t   cb = 0;
    unsigned i  = pGcSgBuf->cSegs;
     while (i-- > 0)
         cb += pGcSgBuf->paSegs[i].cbSeg;
     return cb;
 }

#ifdef IN_RING3

/** API Function: See header file*/
void virtioCorePrintFeatures(VIRTIOCORE *pVirtio, PCDBGFINFOHLP pHlp)
{
    static struct
    {
        uint64_t fFeatureBit;
        const char *pcszDesc;
    } const s_aFeatures[] =
    {
        { VIRTIO_F_RING_INDIRECT_DESC,      "   RING_INDIRECT_DESC   Driver can use descriptors with VIRTQ_DESC_F_INDIRECT flag set\n" },
        { VIRTIO_F_RING_EVENT_IDX,          "   RING_EVENT_IDX       Enables use_event and avail_event fields described in 2.4.7, 2.4.8\n" },
        { VIRTIO_F_VERSION_1,               "   VERSION              Used to detect legacy drivers.\n" },
    };

#define MAXLINE 80
    /* Display as a single buf to prevent interceding log messages */
    uint16_t cbBuf = RT_ELEMENTS(s_aFeatures) * 132;
    char *pszBuf = (char *)RTMemAllocZ(cbBuf);
    Assert(pszBuf);
    char *cp = pszBuf;
    for (unsigned i = 0; i < RT_ELEMENTS(s_aFeatures); ++i)
    {
        bool isOffered    = RT_BOOL(pVirtio->uDeviceFeatures & s_aFeatures[i].fFeatureBit);
        bool isNegotiated = RT_BOOL(pVirtio->uDriverFeatures & s_aFeatures[i].fFeatureBit);
        cp += RTStrPrintf(cp, cbBuf - (cp - pszBuf), "        %s       %s   %s",
                          isOffered ? "+" : "-", isNegotiated ? "x" : " ", s_aFeatures[i].pcszDesc);
    }
    if (pHlp)
        pHlp->pfnPrintf(pHlp, "VirtIO Core Features Configuration\n\n"
              "    Offered  Accepted  Feature              Description\n"
              "    -------  --------  -------              -----------\n"
              "%s\n", pszBuf);
#ifdef LOG_ENABLED
    else
        Log3(("VirtIO Core Features Configuration\n\n"
              "    Offered  Accepted  Feature              Description\n"
              "    -------  --------  -------              -----------\n"
              "%s\n", pszBuf));
#endif
    RTMemFree(pszBuf);
}
#endif

#ifdef LOG_ENABLED

/** API Function: See header file */
void virtioCoreHexDump(uint8_t *pv, uint32_t cb, uint32_t uBase, const char *pszTitle)
{
#define ADJCURSOR(cb) pszOut += cb; cbRemain -= cb;
    size_t cbPrint = 0, cbRemain = ((cb / 16) + 1) * 80;
    char *pszBuf = (char *)RTMemAllocZ(cbRemain), *pszOut = pszBuf;
    AssertMsgReturnVoid(pszBuf, ("Out of Memory"));
    if (pszTitle)
    {
        cbPrint = RTStrPrintf(pszOut, cbRemain, "%s [%d bytes]:\n", pszTitle, cb);
        ADJCURSOR(cbPrint);
    }
    for (uint32_t row = 0; row < RT_MAX(1, (cb / 16) + 1) && row * 16 < cb; row++)
    {
        cbPrint = RTStrPrintf(pszOut, cbRemain, "%04x: ", row * 16 + uBase); /* line address */
        ADJCURSOR(cbPrint);
        for (uint8_t col = 0; col < 16; col++)
        {
           uint32_t idx = row * 16 + col;
           if (idx >= cb)
               cbPrint = RTStrPrintf(pszOut, cbRemain, "-- %s", (col + 1) % 8 ? "" : "  ");
           else
               cbPrint = RTStrPrintf(pszOut, cbRemain, "%02x %s", pv[idx], (col + 1) % 8 ? "" : "  ");
            ADJCURSOR(cbPrint);
        }
        for (uint32_t idx = row * 16; idx < row * 16 + 16; idx++)
        {
           cbPrint = RTStrPrintf(pszOut, cbRemain, "%c", (idx >= cb) ? ' ' : (pv[idx] >= 0x20 && pv[idx] <= 0x7e ? pv[idx] : '.'));
           ADJCURSOR(cbPrint);
        }
        *pszOut++ = '\n';
        --cbRemain;
    }
    Log(("%s\n", pszBuf));
    RTMemFree(pszBuf);
    RT_NOREF2(uBase, pv);
#undef ADJCURSOR
}

/* API FUnction: See header file */
void virtioCoreGCPhysHexDump(PPDMDEVINS pDevIns, RTGCPHYS GCPhys, uint16_t cb, uint32_t uBase, const char *pszTitle)
{
#define ADJCURSOR(cb) pszOut += cb; cbRemain -= cb;
    size_t cbPrint = 0, cbRemain = ((cb / 16) + 1) * 80;
    char *pszBuf = (char *)RTMemAllocZ(cbRemain), *pszOut = pszBuf;
    AssertMsgReturnVoid(pszBuf, ("Out of Memory"));
    if (pszTitle)
    {
        cbPrint = RTStrPrintf(pszOut, cbRemain, "%s [%d bytes]:\n", pszTitle, cb);
        ADJCURSOR(cbPrint);
    }
    for (uint16_t row = 0; row < (uint16_t)RT_MAX(1, (cb / 16) + 1) && row * 16 < cb; row++)
    {
        uint8_t c;
        cbPrint = RTStrPrintf(pszOut, cbRemain, "%04x: ", row * 16 + uBase); /* line address */
        ADJCURSOR(cbPrint);
        for (uint8_t col = 0; col < 16; col++)
        {
           uint32_t idx = row * 16 + col;
           PDMDevHlpPCIPhysRead(pDevIns, GCPhys + idx, &c, 1);
           if (idx >= cb)
               cbPrint = RTStrPrintf(pszOut, cbRemain, "-- %s", (col + 1) % 8 ? "" : "  ");
           else
               cbPrint = RTStrPrintf(pszOut, cbRemain, "%02x %s", c, (col + 1) % 8 ? "" : "  ");
            ADJCURSOR(cbPrint);
        }
        for (uint16_t idx = row * 16; idx < row * 16 + 16; idx++)
        {
           PDMDevHlpPCIPhysRead(pDevIns, GCPhys + idx, &c, 1);
           cbPrint = RTStrPrintf(pszOut, cbRemain, "%c", (idx >= cb) ? ' ' : (c >= 0x20 && c <= 0x7e ? c : '.'));
           ADJCURSOR(cbPrint);
        }
        *pszOut++ = '\n';
        --cbRemain;
     }
    Log(("%s\n", pszBuf));
    RTMemFree(pszBuf);
    RT_NOREF(uBase);
#undef ADJCURSOR
}
#endif /* LOG_ENABLED */

/** API function: See header file */
void virtioCoreLogMappedIoValue(const char *pszFunc, const char *pszMember, uint32_t uMemberSize,
                                const void *pv, uint32_t cb, uint32_t uOffset, int fWrite,
                                int fHasIndex, uint32_t idx)
{
    if (!LogIs6Enabled())
        return;

    char szIdx[16];
    if (fHasIndex)
        RTStrPrintf(szIdx, sizeof(szIdx), "[%d]", idx);
    else
        szIdx[0] = '\0';

    if (cb == 1 || cb == 2 || cb == 4 || cb == 8)
    {
        char szDepiction[64];
        size_t cchDepiction;
        if (uOffset != 0 || cb != uMemberSize) /* display bounds if partial member access */
            cchDepiction = RTStrPrintf(szDepiction, sizeof(szDepiction), "%s%s[%d:%d]",
                                       pszMember, szIdx, uOffset, uOffset + cb - 1);
        else
            cchDepiction = RTStrPrintf(szDepiction, sizeof(szDepiction), "%s%s", pszMember, szIdx);

        /* padding */
        if (cchDepiction < 30)
            szDepiction[cchDepiction++] = ' ';
        while (cchDepiction < 30)
            szDepiction[cchDepiction++] = '.';
        szDepiction[cchDepiction] = '\0';

        RTUINT64U uValue;
        uValue.u = 0;
        memcpy(uValue.au8, pv, cb);
        Log6(("%s: Guest %s %s %#0*RX64\n",
                  pszFunc, fWrite ? "wrote" : "read ", szDepiction, 2 + cb * 2, uValue.u));
    }
    else /* odd number or oversized access, ... log inline hex-dump style */
    {
        Log6(("%s: Guest %s %s%s[%d:%d]: %.*Rhxs\n",
                  pszFunc, fWrite ? "wrote" : "read ", pszMember,
                  szIdx, uOffset, uOffset + cb, cb, pv));
    }
    RT_NOREF2(fWrite, pszFunc);
}


/**
 * Makes the MMIO-mapped Virtio uDeviceStatus registers non-cryptic
 */
DECLINLINE(void) virtioLogDeviceStatus(uint8_t bStatus)
{
    if (bStatus == 0)
        Log6(("RESET"));
    else
    {
        int primed = 0;
        if (bStatus & VIRTIO_STATUS_ACKNOWLEDGE)
            Log6(("%sACKNOWLEDGE", primed++ ? "" : ""));
        if (bStatus & VIRTIO_STATUS_DRIVER)
            Log6(("%sDRIVER",      primed++ ? " | " : ""));
        if (bStatus & VIRTIO_STATUS_FEATURES_OK)
            Log6(("%sFEATURES_OK", primed++ ? " | " : ""));
        if (bStatus & VIRTIO_STATUS_DRIVER_OK)
            Log6(("%sDRIVER_OK",   primed++ ? " | " : ""));
        if (bStatus & VIRTIO_STATUS_FAILED)
            Log6(("%sFAILED",      primed++ ? " | " : ""));
        if (bStatus & VIRTIO_STATUS_DEVICE_NEEDS_RESET)
            Log6(("%sNEEDS_RESET", primed++ ? " | " : ""));
        (void)primed;
    }
}

#ifdef IN_RING3

/** API Fuunction: See header file */
void virtioCoreR3VirtqInfo(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs, int uVirtqNbr)
{
    RT_NOREF(pszArgs);
    PVIRTIOCORE pVirtio = PDMDEVINS_2_DATA(pDevIns, PVIRTIOCORE);
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];

    /** @todo add ability to dump physical contents of any descriptor (using existing VirtIO core API function) */
//    bool fDump      = pszArgs && (*pszArgs == 'd' || *pszArgs == 'D'); /* "dump" (avail phys descriptor)"

    uint16_t uAvailIdx       = virtioReadAvailRingIdx(pDevIns, pVirtio, uVirtqNbr);
    uint16_t uAvailIdxShadow = pVirtqState->uAvailIdxShadow;

    uint16_t uUsedIdx        = virtioReadUsedRingIdx(pDevIns, pVirtio, uVirtqNbr);
    uint16_t uUsedIdxShadow  = pVirtqState->uUsedIdxShadow;

    PVIRTQBUF pVirtqBuf = NULL;

    bool fEmpty = IS_VIRTQ_EMPTY(pDevIns, pVirtio, pVirtqState);

    LogFunc(("%s, empty = %s\n", VIRTQNAME(pVirtio, uVirtqNbr), fEmpty ? "true" : "false"));

    int cSendSegs = 0, cReturnSegs = 0;
    if (!fEmpty)
    {
        virtioCoreR3VirtqBufPeek(pDevIns,  pVirtio, uVirtqNbr, &pVirtqBuf);
        cSendSegs   = pVirtqBuf->pSgPhysSend ? pVirtqBuf->pSgPhysSend->cSegs : 0;
        cReturnSegs = pVirtqBuf->pSgPhysReturn ? pVirtqBuf->pSgPhysReturn->cSegs : 0;
    }

    bool fAvailNoInterrupt   = virtioReadAvailRingFlags(pDevIns, pVirtio, uVirtqNbr) & VIRTQ_AVAIL_F_NO_INTERRUPT;
    bool fUsedNoNotify       = virtioReadUsedRingFlags(pDevIns, pVirtio, uVirtqNbr) & VIRTQ_USED_F_NO_NOTIFY;


    pHlp->pfnPrintf(pHlp, "       queue enabled: ........... %s\n", pVirtio->uVirtqEnable[uVirtqNbr] ? "true" : "false");
    pHlp->pfnPrintf(pHlp, "       size: .................... %d\n", pVirtio->uVirtqSize[uVirtqNbr]);
    pHlp->pfnPrintf(pHlp, "       notify offset: ........... %d\n", pVirtio->uVirtqNotifyOff[uVirtqNbr]);
    if (pVirtio->fMsiSupport)
        pHlp->pfnPrintf(pHlp, "       MSIX vector: ....... %4.4x\n", pVirtio->uVirtqMsixVector[uVirtqNbr]);
    pHlp->pfnPrintf(pHlp, "\n");
    pHlp->pfnPrintf(pHlp, "       avail ring (%d entries):\n", uAvailIdx - uAvailIdxShadow);
    pHlp->pfnPrintf(pHlp, "          index: ................ %d\n", uAvailIdx);
    pHlp->pfnPrintf(pHlp, "          shadow: ............... %d\n", uAvailIdxShadow);
    pHlp->pfnPrintf(pHlp, "          flags: ................ %s\n", fAvailNoInterrupt ? "NO_INTERRUPT" : "");
    pHlp->pfnPrintf(pHlp, "\n");
    pHlp->pfnPrintf(pHlp, "       used ring (%d entries):\n",  uUsedIdx - uUsedIdxShadow);
    pHlp->pfnPrintf(pHlp, "          index: ................ %d\n", uUsedIdx);
    pHlp->pfnPrintf(pHlp, "          shadow: ............... %d\n", uUsedIdxShadow);
    pHlp->pfnPrintf(pHlp, "          flags: ................ %s\n", fUsedNoNotify ? "NO_NOTIFY" : "");
    pHlp->pfnPrintf(pHlp, "\n");
    if (!fEmpty)
    {
        pHlp->pfnPrintf(pHlp, "       desc chain:\n");
        pHlp->pfnPrintf(pHlp, "          head idx: ............. %d\n", uUsedIdx);
        pHlp->pfnPrintf(pHlp, "          segs: ................. %d\n", cSendSegs + cReturnSegs);
        pHlp->pfnPrintf(pHlp, "          refCnt ................ %d\n", pVirtqBuf->cRefs);
        pHlp->pfnPrintf(pHlp, "\n");
        pHlp->pfnPrintf(pHlp, "          host-to-guest (%d bytes):\n",      pVirtqBuf->cbPhysSend);
        pHlp->pfnPrintf(pHlp,     "             segs: .............. %d\n", cSendSegs);
        if (cSendSegs)
        {
            pHlp->pfnPrintf(pHlp, "             index: ............. %d\n", pVirtqBuf->pSgPhysSend->idxSeg);
            pHlp->pfnPrintf(pHlp, "             unsent ............. %d\n", pVirtqBuf->pSgPhysSend->cbSegLeft);
        }
        pHlp->pfnPrintf(pHlp, "\n");
        pHlp->pfnPrintf(pHlp,     "          guest-to-host (%d bytes)\n",   pVirtqBuf->cbPhysReturn);
        pHlp->pfnPrintf(pHlp,     "             segs: .............. %d\n", cReturnSegs);
        if (cReturnSegs)
        {
            pHlp->pfnPrintf(pHlp, "             index: ............. %d\n", pVirtqBuf->pSgPhysReturn->idxSeg);
            pHlp->pfnPrintf(pHlp, "             unsent ............. %d\n", pVirtqBuf->pSgPhysReturn->cbSegLeft);
        }
    } else
        pHlp->pfnPrintf(pHlp,     "      No desc chains available\n");
    pHlp->pfnPrintf(pHlp, "\n");

}

int virtioCoreR3VirtqAttach(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, const char *pcszName)
{
    LogFunc(("%s\n", pcszName));
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];
    pVirtqState->uVirtqNbr = uVirtqNbr;
    pVirtqState->uAvailIdxShadow = 0;
    pVirtqState->uUsedIdxShadow  = 0;
    pVirtqState->fVirtqRingEventThreshold = false;
    RTStrCopy(pVirtqState->szVirtqName, sizeof(pVirtqState->szVirtqName), pcszName);
    return VINF_SUCCESS;
}
#endif /* IN_RING3 */


#ifdef IN_RING3

/** API Function: See header file */
int virtioCoreR3VirtqBufGet(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                             uint16_t uHeadIdx, PPVIRTQBUF ppVirtqBuf)
{
    AssertReturn(ppVirtqBuf, VERR_INVALID_POINTER);
    *ppVirtqBuf = NULL;

    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));

    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];

    AssertMsgReturn(IS_DRIVER_OK(pVirtio) && pVirtio->uVirtqEnable[uVirtqNbr],
                    ("Guest driver not in ready state.\n"), VERR_INVALID_STATE);

    uint16_t uDescIdx = uHeadIdx;

    Log6Func(("%s DESC CHAIN: (head) desc_idx=%u\n", pVirtqState->szVirtqName, uHeadIdx));
    RT_NOREF(pVirtqState);

    /*
     * Allocate and initialize the descriptor chain structure.
     */
    PVIRTQBUF pVirtqBuf = (PVIRTQBUF)RTMemAllocZ(sizeof(VIRTQBUF_T));
    AssertReturn(pVirtqBuf, VERR_NO_MEMORY);
    pVirtqBuf->u32Magic = VIRTQBUF_MAGIC;
    pVirtqBuf->cRefs    = 1;
    pVirtqBuf->uHeadIdx = uHeadIdx;
    *ppVirtqBuf = pVirtqBuf;

    /*
     * Gather segments.
     */
    VIRTQ_DESC_T desc;

    uint32_t cbIn = 0;
    uint32_t cbOut = 0;
    uint32_t cSegsIn = 0;
    uint32_t cSegsOut = 0;
    PVIRTIOSGSEG paSegsIn  = pVirtqBuf->aSegsIn;
    PVIRTIOSGSEG paSegsOut = pVirtqBuf->aSegsOut;

    do
    {
        PVIRTIOSGSEG pSeg;

        /*
         * Malicious guests may go beyond paSegsIn or paSegsOut boundaries by linking
         * several descriptors into a loop. Since there is no legitimate way to get a sequences of
         * linked descriptors exceeding the total number of descriptors in the ring (see @bugref{8620}),
         * the following aborts I/O if breach and employs a simple log throttling algorithm to notify.
         */
        if (cSegsIn + cSegsOut >= VIRTQ_MAX_ENTRIES)
        {
            static volatile uint32_t s_cMessages  = 0;
            static volatile uint32_t s_cThreshold = 1;
            if (ASMAtomicIncU32(&s_cMessages) == ASMAtomicReadU32(&s_cThreshold))
            {
                LogRelMax(64, ("Too many linked descriptors; check if the guest arranges descriptors in a loop.\n"));
                if (ASMAtomicReadU32(&s_cMessages) != 1)
                    LogRelMax(64, ("(the above error has occured %u times so far)\n", ASMAtomicReadU32(&s_cMessages)));
                ASMAtomicWriteU32(&s_cThreshold, ASMAtomicReadU32(&s_cThreshold) * 10);
            }
            break;
        }
        RT_UNTRUSTED_VALIDATED_FENCE();

        virtioReadDesc(pDevIns, pVirtio, uVirtqNbr, uDescIdx, &desc);

        if (desc.fFlags & VIRTQ_DESC_F_WRITE)
        {
            Log6Func(("%s IN  desc_idx=%u seg=%u addr=%RGp cb=%u\n", VIRTQNAME(pVirtio, uVirtqNbr), uDescIdx, cSegsIn, desc.GCPhysBuf, desc.cb));
            cbIn += desc.cb;
            pSeg = &paSegsIn[cSegsIn++];
        }
        else
        {
            Log6Func(("%s OUT desc_idx=%u seg=%u addr=%RGp cb=%u\n", VIRTQNAME(pVirtio, uVirtqNbr), uDescIdx, cSegsOut, desc.GCPhysBuf, desc.cb));
            cbOut += desc.cb;
            pSeg = &paSegsOut[cSegsOut++];
            if (LogIs11Enabled())
            {
                virtioCoreGCPhysHexDump(pDevIns, desc.GCPhysBuf, desc.cb, 0, NULL);
                Log(("\n"));
            }
        }

        pSeg->GCPhys = desc.GCPhysBuf;
        pSeg->cbSeg = desc.cb;

        uDescIdx = desc.uDescIdxNext;
    } while (desc.fFlags & VIRTQ_DESC_F_NEXT);

    /*
     * Add segments to the descriptor chain structure.
     */
    if (cSegsIn)
    {
        virtioCoreSgBufInit(&pVirtqBuf->SgBufIn, paSegsIn, cSegsIn);
        pVirtqBuf->pSgPhysReturn = &pVirtqBuf->SgBufIn;
        pVirtqBuf->cbPhysReturn  = cbIn;
        STAM_REL_COUNTER_ADD(&pVirtio->StatDescChainsSegsIn, cSegsIn);
    }

    if (cSegsOut)
    {
        virtioCoreSgBufInit(&pVirtqBuf->SgBufOut, paSegsOut, cSegsOut);
        pVirtqBuf->pSgPhysSend   = &pVirtqBuf->SgBufOut;
        pVirtqBuf->cbPhysSend    = cbOut;
        STAM_REL_COUNTER_ADD(&pVirtio->StatDescChainsSegsOut, cSegsOut);
    }

    STAM_REL_COUNTER_INC(&pVirtio->StatDescChainsAllocated);
    Log6Func(("%s -- segs OUT: %u (%u bytes)   IN: %u (%u bytes) --\n", pVirtqState->szVirtqName, cSegsOut, cbOut, cSegsIn, cbIn));

    return VINF_SUCCESS;
}

/** API Function: See header file */
uint32_t virtioCoreR3VirtqBufRetain(PVIRTQBUF pVirtqBuf)
{
    AssertReturn(pVirtqBuf, UINT32_MAX);
    AssertReturn(pVirtqBuf->u32Magic == VIRTQBUF_MAGIC, UINT32_MAX);
    uint32_t cRefs = ASMAtomicIncU32(&pVirtqBuf->cRefs);
    Assert(cRefs > 1);
    Assert(cRefs < 16);
    return cRefs;
}


/** API Function: See header file */
uint32_t virtioCoreR3VirtqBufRelease(PVIRTIOCORE pVirtio, PVIRTQBUF pVirtqBuf)
{
    if (!pVirtqBuf)
        return 0;
    AssertReturn(pVirtqBuf, 0);
    AssertReturn(pVirtqBuf->u32Magic == VIRTQBUF_MAGIC, 0);
    uint32_t cRefs = ASMAtomicDecU32(&pVirtqBuf->cRefs);
    Assert(cRefs < 16);
    if (cRefs == 0)
    {
        pVirtqBuf->u32Magic = ~VIRTQBUF_MAGIC;
        RTMemFree(pVirtqBuf);
        STAM_REL_COUNTER_INC(&pVirtio->StatDescChainsFreed);
    }
    return cRefs;
}

/** API Function: See header file */
void virtioCoreNotifyConfigChanged(PVIRTIOCORE pVirtio)
{
    virtioKick(pVirtio->pDevInsR3, pVirtio, VIRTIO_ISR_DEVICE_CONFIG, pVirtio->uMsixConfig);
}

void virtioCoreVirtqEnableNotify(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, bool fEnable)
{
    if (pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK)
    {
        uint16_t fFlags = virtioReadUsedRingFlags(pVirtio->pDevInsR3, pVirtio, uVirtqNbr);

        if (fEnable)
            fFlags &= ~ VIRTQ_USED_F_NO_NOTIFY;
        else
            fFlags |= VIRTQ_USED_F_NO_NOTIFY;

        virtioWriteUsedRingFlags(pVirtio->pDevInsR3, pVirtio, uVirtqNbr, fFlags);
    }
}

/** API function: See Header file  */
void virtioCoreResetAll(PVIRTIOCORE pVirtio)
{
    LogFunc(("\n"));
    pVirtio->uDeviceStatus |= VIRTIO_STATUS_DEVICE_NEEDS_RESET;
    if (pVirtio->uDeviceStatus & VIRTIO_STATUS_DRIVER_OK)
    {
        pVirtio->fGenUpdatePending = true;
        virtioKick(pVirtio->pDevInsR3, pVirtio, VIRTIO_ISR_DEVICE_CONFIG, pVirtio->uMsixConfig);
    }
}

/** API function: See Header file  */
int virtioCoreR3VirtqBufPeek(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                         PPVIRTQBUF ppVirtqBuf)
{
    return virtioCoreR3VirtqBufGet(pDevIns, pVirtio,  uVirtqNbr, ppVirtqBuf, false);
}

/** API function: See Header file  */
int virtioCoreR3VirtqBufSkip(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];

    AssertMsgReturn(IS_DRIVER_OK(pVirtio) && pVirtio->uVirtqEnable[uVirtqNbr],
                    ("Guest driver not in ready state.\n"), VERR_INVALID_STATE);

    if (IS_VIRTQ_EMPTY(pVirtio->pDevInsR3, pVirtio, pVirtqState))
        return VERR_NOT_AVAILABLE;

    Log6Func(("%s avail shadow idx: %u\n", pVirtqState->szVirtqName, pVirtqState->uAvailIdxShadow));
    pVirtqState->uAvailIdxShadow++;

    return VINF_SUCCESS;
}

/** API function: See Header file  */
int virtioCoreR3VirtqBufGet(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr,
                         PPVIRTQBUF ppVirtqBuf, bool fRemove)
{
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];

    if (IS_VIRTQ_EMPTY(pDevIns, pVirtio, pVirtqState))
        return VERR_NOT_AVAILABLE;

    uint16_t uHeadIdx = virtioReadAvailDescIdx(pDevIns, pVirtio, uVirtqNbr, pVirtqState->uAvailIdxShadow);

    if (pVirtio->uDriverFeatures & VIRTIO_F_EVENT_IDX)
        virtioWriteUsedAvailEvent(pDevIns,pVirtio, uVirtqNbr, pVirtqState->uAvailIdxShadow + 1);

    if (fRemove)
        pVirtqState->uAvailIdxShadow++;

    int rc = virtioCoreR3VirtqBufGet(pDevIns, pVirtio, uVirtqNbr, uHeadIdx, ppVirtqBuf);
    return rc;
}

/** API function: See Header file  */
int virtioCoreR3VirtqBufPut(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, PRTSGBUF pSgVirtReturn,
                            PVIRTQBUF pVirtqBuf, bool fFence)
{
    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];
    PVIRTIOSGBUF pSgPhysReturn = pVirtqBuf->pSgPhysReturn;

    Assert(pVirtqBuf->u32Magic == VIRTQBUF_MAGIC);
    Assert(pVirtqBuf->cRefs > 0);

    AssertMsgReturn(IS_DRIVER_OK(pVirtio), ("Guest driver not in ready state.\n"), VERR_INVALID_STATE);

    Log6Func(("Copying client data to %s, desc chain (head desc_idx %d)\n",
              VIRTQNAME(pVirtio, uVirtqNbr), virtioReadUsedRingIdx(pDevIns, pVirtio, uVirtqNbr)));

    /* Copy s/g buf (virtual memory) to guest phys mem (IN direction). */

    size_t cbCopy = 0, cbTotal = 0, cbRemain = 0;

    if (pSgVirtReturn)
    {
        size_t cbTarget = virtioCoreSgBufCalcTotalLength(pSgPhysReturn);
        cbRemain = cbTotal = RTSgBufCalcTotalLength(pSgVirtReturn);
        AssertMsgReturn(cbTarget >= cbRemain, ("No space to write data to phys memory"), VERR_BUFFER_OVERFLOW);
        virtioCoreSgBufReset(pSgPhysReturn); /* Reset ptr because req data may have already been written */
        while (cbRemain)
        {
            cbCopy = RT_MIN(pSgVirtReturn->cbSegLeft,  pSgPhysReturn->cbSegLeft);
            Assert(cbCopy > 0);
            PDMDevHlpPhysWrite(pDevIns, (RTGCPHYS)pSgPhysReturn->GCPhysCur, pSgVirtReturn->pvSegCur, cbCopy);
            RTSgBufAdvance(pSgVirtReturn, cbCopy);
            virtioCoreSgBufAdvance(pSgPhysReturn, cbCopy);
            cbRemain -= cbCopy;
        }

        if (fFence)
            RT_UNTRUSTED_NONVOLATILE_COPY_FENCE(); /* needed? */

        Assert(!(cbCopy >> 32));
    }

    /* If this write-ahead crosses threshold where the driver wants to get an event flag it */
    if (pVirtio->uDriverFeatures & VIRTIO_F_EVENT_IDX)
        if (pVirtqState->uUsedIdxShadow == virtioReadAvailUsedEvent(pDevIns, pVirtio, uVirtqNbr))
            pVirtqState->fVirtqRingEventThreshold = true;

    /*
     * Place used buffer's descriptor in used ring but don't update used ring's slot index.
     * That will be done with a subsequent client call to virtioCoreVirtqSync() */
    virtioWriteUsedElem(pDevIns, pVirtio, uVirtqNbr, pVirtqState->uUsedIdxShadow++, pVirtqBuf->uHeadIdx, (uint32_t)cbTotal);

    if (pSgVirtReturn)
        Log6Func((".... Copied %zu bytes in %d segs to %u byte buffer, residual=%zu\n",
                  cbTotal - cbRemain, pSgVirtReturn->cSegs, pVirtqBuf->cbPhysReturn, pVirtqBuf->cbPhysReturn - cbTotal));

    Log6Func(("Write ahead used_idx=%u, %s used_idx=%u\n",
              pVirtqState->uUsedIdxShadow, VIRTQNAME(pVirtio, uVirtqNbr), virtioReadUsedRingIdx(pDevIns, pVirtio, uVirtqNbr)));

    return VINF_SUCCESS;
}

#endif /* IN_RING3 */

/** API function: See Header file  */
int virtioCoreVirtqSync(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];

    AssertMsgReturn(IS_DRIVER_OK(pVirtio) && pVirtio->uVirtqEnable[uVirtqNbr],
                    ("Guest driver not in ready state.\n"), VERR_INVALID_STATE);

    Log6Func(("Updating %s used_idx to %u\n",
              VIRTQNAME(pVirtio, uVirtqNbr), pVirtqState->uUsedIdxShadow));

    virtioWriteUsedRingIdx(pDevIns, pVirtio, uVirtqNbr, pVirtqState->uUsedIdxShadow);
    virtioCoreNotifyGuestDriver(pDevIns, pVirtio, uVirtqNbr);

    return VINF_SUCCESS;
}


/**
 */
static void virtioCoreVirtqNotified(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr, uint16_t uNotifyIdx)
{

    PVIRTIOCORECC pVirtioCC = PDMDEVINS_2_DATA_CC(pDevIns, PVIRTIOCORECC);

    /* See VirtIO 1.0, section 4.1.5.2 It implies that uVirtqNbr and uNotifyIdx should match.
     * Disregarding this notification may cause throughput to stop, however there's no way to know
     * which was queue was intended for wake-up if the two parameters disagree. */

    AssertMsg(uNotifyIdx == uVirtqNbr,
                    ("Guest kicked virtq %d's notify addr w/non-corresponding virtq idx %d\n",
                     uVirtqNbr, uNotifyIdx));
    RT_NOREF(uNotifyIdx);

    AssertReturnVoid(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    Log6Func(("%s (desc chains: %u)\n",
        pVirtio->aVirtqState[uVirtqNbr].szVirtqName,
        virtioCoreVirtqAvailCount(pDevIns, pVirtio, uVirtqNbr)));

    /* Inform client */
    pVirtioCC->pfnVirtqNotified(pDevIns, pVirtio, uVirtqNbr);
}

/**
 * Trigger MSI-X or INT# interrupt to notify guest of data added to used ring of
 * the specified virtq, depending on the interrupt configuration of the device
 * and depending on negotiated and realtime constraints flagged by the guest driver.
 *
 * See VirtIO 1.0 specification (section 2.4.7).
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uVirtqNbr    Virtq to check for guest interrupt handling preference
 */
static void virtioCoreNotifyGuestDriver(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{

    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];

    if (!IS_DRIVER_OK(pVirtio))
    {
        LogFunc(("Guest driver not in ready state.\n"));
        return;
    }

    if (pVirtio->uDriverFeatures & VIRTIO_F_EVENT_IDX)
    {
        if (pVirtqState->fVirtqRingEventThreshold)
        {
#ifdef IN_RING3
            Log6Func(("...kicking guest %s, VIRTIO_F_EVENT_IDX set and threshold (%d) reached\n",
                   VIRTQNAME(pVirtio, uVirtqNbr), (uint16_t)virtioReadAvailUsedEvent(pDevIns, pVirtio, uVirtqNbr)));
#endif
            virtioKick(pDevIns, pVirtio, VIRTIO_ISR_VIRTQ_INTERRUPT, pVirtio->uVirtqMsixVector[uVirtqNbr]);
            pVirtqState->fVirtqRingEventThreshold = false;
            return;
        }
#ifdef IN_RING3
        Log6Func(("...skip interrupt %s, VIRTIO_F_EVENT_IDX set but threshold (%d) not reached (%d)\n",
                   VIRTQNAME(pVirtio, uVirtqNbr),(uint16_t)virtioReadAvailUsedEvent(pDevIns, pVirtio, uVirtqNbr), pVirtqState->uUsedIdxShadow));
#endif
    }
    else
    {
        /** If guest driver hasn't suppressed interrupts, interrupt  */
        if (!(virtioReadAvailRingFlags(pDevIns, pVirtio, uVirtqNbr) & VIRTQ_AVAIL_F_NO_INTERRUPT))
        {
            virtioKick(pDevIns, pVirtio, VIRTIO_ISR_VIRTQ_INTERRUPT, pVirtio->uVirtqMsixVector[uVirtqNbr]);
            return;
        }
        Log6Func(("...skipping interrupt for %s (guest set VIRTQ_AVAIL_F_NO_INTERRUPT)\n",
                     VIRTQNAME(pVirtio, uVirtqNbr)));
    }
}

/**
 * Raise interrupt or MSI-X
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   uCause      Interrupt cause bit mask to set in PCI ISR port.
 * @param   uVec        MSI-X vector, if enabled
 */
static int virtioKick(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, uint8_t uCause, uint16_t uMsixVector)
{
    if (uCause == VIRTIO_ISR_VIRTQ_INTERRUPT)
        Log6Func(("reason: buffer added to 'used' ring.\n"));
    else
    if (uCause == VIRTIO_ISR_DEVICE_CONFIG)
       Log6Func(("reason: device config change\n"));

    if (!pVirtio->fMsiSupport)
    {
        pVirtio->uISR |= uCause;
        PDMDevHlpPCISetIrq(pDevIns, 0, PDM_IRQ_LEVEL_HIGH);
    }
    else if (uMsixVector != VIRTIO_MSI_NO_VECTOR)
        PDMDevHlpPCISetIrq(pDevIns, uMsixVector, 1);
    return VINF_SUCCESS;
}

/**
 * Lower interrupt (Called when guest reads ISR and when resetting)
 *
 * @param   pDevIns     The device instance.
 */
static void virtioLowerInterrupt(PPDMDEVINS pDevIns, uint16_t uMsixVector)
{
    PVIRTIOCORE pVirtio = PDMINS_2_DATA(pDevIns, PVIRTIOCORE);
    if (!pVirtio->fMsiSupport)
        PDMDevHlpPCISetIrq(pDevIns, 0, PDM_IRQ_LEVEL_LOW);
    else if (uMsixVector != VIRTIO_MSI_NO_VECTOR)
        PDMDevHlpPCISetIrq(pDevIns, pVirtio->uMsixConfig, PDM_IRQ_LEVEL_LOW);
}

#ifdef IN_RING3
static void virtioResetVirtq(PVIRTIOCORE pVirtio, uint16_t uVirtqNbr)
{
    Assert(uVirtqNbr < RT_ELEMENTS(pVirtio->aVirtqState));
    PVIRTQSTATE pVirtqState = &pVirtio->aVirtqState[uVirtqNbr];
    pVirtqState->uAvailIdxShadow = 0;
    pVirtqState->uUsedIdxShadow  = 0;
    pVirtqState->fVirtqRingEventThreshold = false;
    pVirtio->uVirtqEnable[uVirtqNbr] = false;
    pVirtio->uVirtqSize[uVirtqNbr] = VIRTQ_MAX_ENTRIES;
    pVirtio->uVirtqNotifyOff[uVirtqNbr] = uVirtqNbr;
    pVirtio->uVirtqMsixVector[uVirtqNbr] = uVirtqNbr + 2;
    if (!pVirtio->fMsiSupport) /* VirtIO 1.0, 4.1.4.3 and 4.1.5.1.2 */
        pVirtio->uVirtqMsixVector[uVirtqNbr] = VIRTIO_MSI_NO_VECTOR;

    virtioLowerInterrupt(pVirtio->pDevInsR3, pVirtio->uVirtqMsixVector[uVirtqNbr]);
}

static void virtioResetDevice(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio)
{
    Log2Func(("\n"));
    pVirtio->uDeviceFeaturesSelect  = 0;
    pVirtio->uDriverFeaturesSelect  = 0;
    pVirtio->uConfigGeneration      = 0;
    pVirtio->uDeviceStatus          = 0;
    pVirtio->uISR                   = 0;

    if (!pVirtio->fMsiSupport)
        virtioLowerInterrupt(pDevIns, 0);
    else
    {
        virtioLowerInterrupt(pDevIns, pVirtio->uMsixConfig);
        for (int i = 0; i < VIRTQ_MAX_CNT; i++)
        {
            virtioLowerInterrupt(pDevIns, pVirtio->uVirtqMsixVector[i]);
            pVirtio->uVirtqMsixVector[i];
        }
    }

    if (!pVirtio->fMsiSupport)  /* VirtIO 1.0, 4.1.4.3 and 4.1.5.1.2 */
        pVirtio->uMsixConfig = VIRTIO_MSI_NO_VECTOR;

    for (uint16_t uVirtqNbr = 0; uVirtqNbr < VIRTQ_MAX_CNT; uVirtqNbr++)
        virtioResetVirtq(pVirtio, uVirtqNbr);
}

/**
 * Invoked by this implementation when guest driver resets the device.
 * The driver itself will not  until the device has read the status change.
 */
static void virtioGuestR3WasReset(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC)
{
    LogFunc(("Guest reset the device\n"));

    /* Let the client know */
    pVirtioCC->pfnStatusChanged(pVirtio, pVirtioCC, 0);
    virtioResetDevice(pDevIns, pVirtio);
}
#endif /* IN_RING3 */

/**
 * Handle accesses to Common Configuration capability
 *
 * @returns VBox status code
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   pVirtioCC   Pointer to the current context virtio state.
 * @param   fWrite      Set if write access, clear if read access.
 * @param   offCfg      The common configuration capability offset.
 * @param   cb          Number of bytes to read or write
 * @param   pv          Pointer to location to write to or read from
 */
static int virtioCommonCfgAccessed(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC,
                                   int fWrite, uint32_t offCfg, unsigned cb, void *pv)
{
/**
 * This macro resolves to boolean true if the implied parameters, offCfg and cb,
 * match the field offset and size of a field in the Common Cfg struct, (or if
 * it is a 64-bit field, if it accesses either 32-bit part as a 32-bit access)
 * This is mandated by section 4.1.3.1 of the VirtIO 1.0 specification)
 *
 * @param   member  Member of VIRTIO_PCI_COMMON_CFG_T
 * @param   offCfg  Implied parameter: Offset into VIRTIO_PCI_COMMON_CFG_T
 * @param   cb      Implied parameter: Number of bytes to access
 * @result true or false
 */
#define MATCH_COMMON_CFG(member) \
    (   (   RT_SIZEOFMEMB(VIRTIO_PCI_COMMON_CFG_T, member) == 8 \
         && (   offCfg == RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member) \
             || offCfg == RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member) + sizeof(uint32_t)) \
         && cb == sizeof(uint32_t)) \
     || (   offCfg == RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member) \
         && cb == RT_SIZEOFMEMB(VIRTIO_PCI_COMMON_CFG_T, member)) )

#ifdef LOG_ENABLED
# define LOG_COMMON_CFG_ACCESS(member, uOffset) \
    if (LogIs7Enabled()) { \
        virtioCoreLogMappedIoValue(__FUNCTION__, #member, RT_SIZEOFMEMB(VIRTIO_PCI_COMMON_CFG_T, member), \
                                   pv, cb, uOffset, fWrite, false, 0); \
    }
# define LOG_COMMON_CFG_ACCESS_INDEXED(member, idx, uOffset) \
    if (LogIs7Enabled()) { \
        virtioCoreLogMappedIoValue(__FUNCTION__, #member, RT_SIZEOFMEMB(VIRTIO_PCI_COMMON_CFG_T, member), \
                                   pv, cb, uOffset, fWrite, true, idx); \
    }
#else
# define LOG_COMMON_CFG_ACCESS(member, uOffset)              do { } while (0)
# define LOG_COMMON_CFG_ACCESS_INDEXED(member, idx, uOffset) do { } while (0)
#endif

#define COMMON_CFG_ACCESSOR(member) \
    do \
    { \
        uint32_t uOffset = offCfg - RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member); \
        if (fWrite) \
            memcpy((char *)&pVirtio->member + uOffset, (const char *)pv, cb); \
        else \
            memcpy(pv, (const char *)&pVirtio->member + uOffset, cb); \
        LOG_COMMON_CFG_ACCESS(member, uOffset); \
    } while(0)

#define COMMON_CFG_ACCESSOR_INDEXED(member, idx) \
    do \
    { \
        uint32_t uOffset = offCfg - RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member); \
        if (fWrite) \
            memcpy((char *)&pVirtio->member[idx] + uOffset, pv, cb); \
        else \
            memcpy(pv, (const char *)&pVirtio->member[idx] + uOffset, cb); \
        LOG_COMMON_CFG_ACCESS_INDEXED(member, idx, uOffset); \
    } while(0)

#define COMMON_CFG_ACCESSOR_READONLY(member) \
    do \
    { \
        uint32_t uOffset = offCfg - RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member); \
        if (fWrite) \
            LogFunc(("Guest attempted to write readonly virtio_pci_common_cfg.%s\n", #member)); \
        else \
        { \
            memcpy(pv, (const char *)&pVirtio->member + uOffset, cb); \
            LOG_COMMON_CFG_ACCESS(member, uOffset); \
        } \
    } while(0)

#define COMMON_CFG_ACCESSOR_INDEXED_READONLY(member, idx) \
    do \
    { \
        uint32_t uOffset = offCfg - RT_OFFSETOF(VIRTIO_PCI_COMMON_CFG_T, member); \
        if (fWrite) \
            LogFunc(("Guest attempted to write readonly virtio_pci_common_cfg.%s[%d]\n", #member, idx)); \
        else \
        { \
            memcpy(pv, (char const *)&pVirtio->member[idx] + uOffset, cb); \
            LOG_COMMON_CFG_ACCESS_INDEXED(member, idx, uOffset); \
        } \
    } while(0)


    int rc = VINF_SUCCESS;
    uint64_t val;
    if (MATCH_COMMON_CFG(uDeviceFeatures))
    {
        if (fWrite) /* Guest WRITE pCommonCfg>uDeviceFeatures */
        {
            LogFunc(("Guest attempted to write readonly virtio_pci_common_cfg.device_feature\n"));
            return VINF_SUCCESS;
        }
        else /* Guest READ pCommonCfg->uDeviceFeatures */
        {
            switch (pVirtio->uDeviceFeaturesSelect)
            {
                case 0:
                    val = pVirtio->uDeviceFeatures & UINT32_C(0xffffffff);
                    memcpy(pv, &val, cb);
                    LOG_COMMON_CFG_ACCESS(uDeviceFeatures, offCfg - RT_UOFFSETOF(VIRTIO_PCI_COMMON_CFG_T, uDeviceFeatures));
                    break;
                case 1:
                    val = pVirtio->uDeviceFeatures >> 32;
                    memcpy(pv, &val, cb);
                    LOG_COMMON_CFG_ACCESS(uDeviceFeatures, offCfg - RT_UOFFSETOF(VIRTIO_PCI_COMMON_CFG_T, uDeviceFeatures) + 4);
                    break;
                default:
                    LogFunc(("Guest read uDeviceFeatures with out of range selector (%#x), returning 0\n",
                             pVirtio->uDeviceFeaturesSelect));
                    return VINF_IOM_MMIO_UNUSED_00;
            }
        }
    }
    else if (MATCH_COMMON_CFG(uDriverFeatures))
    {
        if (fWrite) /* Guest WRITE pCommonCfg->udriverFeatures */
        {
            switch (pVirtio->uDriverFeaturesSelect)
            {
                case 0:
                    memcpy(&pVirtio->uDriverFeatures, pv, cb);
                    LOG_COMMON_CFG_ACCESS(uDriverFeatures, offCfg - RT_UOFFSETOF(VIRTIO_PCI_COMMON_CFG_T, uDriverFeatures));
                    break;
                case 1:
                    memcpy((char *)&pVirtio->uDriverFeatures + sizeof(uint32_t), pv, cb);
                    LOG_COMMON_CFG_ACCESS(uDriverFeatures, offCfg - RT_UOFFSETOF(VIRTIO_PCI_COMMON_CFG_T, uDriverFeatures) + 4);
                    break;
                default:
                    LogFunc(("Guest wrote uDriverFeatures with out of range selector (%#x), returning 0\n",
                             pVirtio->uDriverFeaturesSelect));
                    return VINF_SUCCESS;
            }
        }
        else /* Guest READ pCommonCfg->udriverFeatures */
        {
            switch (pVirtio->uDriverFeaturesSelect)
            {
                case 0:
                    val = pVirtio->uDriverFeatures & 0xffffffff;
                    memcpy(pv, &val, cb);
                    LOG_COMMON_CFG_ACCESS(uDriverFeatures, offCfg - RT_UOFFSETOF(VIRTIO_PCI_COMMON_CFG_T, uDriverFeatures));
                    break;
                case 1:
                    val = (pVirtio->uDriverFeatures >> 32) & 0xffffffff;
                    memcpy(pv, &val, cb);
                    LOG_COMMON_CFG_ACCESS(uDriverFeatures, offCfg - RT_UOFFSETOF(VIRTIO_PCI_COMMON_CFG_T, uDriverFeatures) + 4);
                    break;
                default:
                    LogFunc(("Guest read uDriverFeatures with out of range selector (%#x), returning 0\n",
                             pVirtio->uDriverFeaturesSelect));
                    return VINF_IOM_MMIO_UNUSED_00;
            }
        }
    }
    else if (MATCH_COMMON_CFG(uNumVirtqs))
    {
        if (fWrite)
        {
            Log2Func(("Guest attempted to write readonly virtio_pci_common_cfg.num_queues\n"));
            return VINF_SUCCESS;
        }
        else
        {
            *(uint16_t *)pv = VIRTQ_MAX_CNT;
            LOG_COMMON_CFG_ACCESS(uNumVirtqs, 0);
        }
    }
    else if (MATCH_COMMON_CFG(uDeviceStatus))
    {
        if (fWrite) /* Guest WRITE pCommonCfg->uDeviceStatus */
        {
            uint8_t const fNewStatus = *(uint8_t *)pv;
            Log7Func(("Guest wrote uDeviceStatus ................ ("));
            if (LogIs7Enabled())
                virtioLogDeviceStatus(fNewStatus ^ pVirtio->uDeviceStatus);
            Log7((")\n"));

            /* If the status changed or we were reset, we need to go to ring-3 as
               it requires notifying the parent device. */
            bool const fStatusChanged =    (fNewStatus & VIRTIO_STATUS_DRIVER_OK)
                                        != (pVirtio->uPrevDeviceStatus & VIRTIO_STATUS_DRIVER_OK);
#ifndef IN_RING3
            if (fStatusChanged || fNewStatus == 0)
            {
                Log6Func(("=>ring3\n"));
                return VINF_IOM_R3_MMIO_WRITE;
            }
#endif
            pVirtio->uDeviceStatus = fNewStatus;

#ifdef IN_RING3
            /*
             * Notify client only if status actually changed from last time and when we're reset.
             */
            if (pVirtio->uDeviceStatus == 0)
                virtioGuestR3WasReset(pDevIns, pVirtio, pVirtioCC);
            if (fStatusChanged)
                pVirtioCC->pfnStatusChanged(pVirtio, pVirtioCC, fNewStatus & VIRTIO_STATUS_DRIVER_OK);
#endif
            /*
             * Save the current status for the next write so we can see what changed.
             */
            pVirtio->uPrevDeviceStatus = pVirtio->uDeviceStatus;
        }
        else /* Guest READ pCommonCfg->uDeviceStatus */
        {
            Log7Func(("Guest read  uDeviceStatus ................ ("));
            *(uint8_t *)pv = pVirtio->uDeviceStatus;
            if (LogIs7Enabled())
                virtioLogDeviceStatus(pVirtio->uDeviceStatus);
            Log7((")\n"));
        }
    }
    else
    if (MATCH_COMMON_CFG(uMsixConfig))
        COMMON_CFG_ACCESSOR(uMsixConfig);
    else
    if (MATCH_COMMON_CFG(uDeviceFeaturesSelect))
        COMMON_CFG_ACCESSOR(uDeviceFeaturesSelect);
    else
    if (MATCH_COMMON_CFG(uDriverFeaturesSelect))
        COMMON_CFG_ACCESSOR(uDriverFeaturesSelect);
    else
    if (MATCH_COMMON_CFG(uConfigGeneration))
        COMMON_CFG_ACCESSOR_READONLY(uConfigGeneration);
    else
    if (MATCH_COMMON_CFG(uVirtqSelect))
        COMMON_CFG_ACCESSOR(uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(uVirtqSize))
        COMMON_CFG_ACCESSOR_INDEXED(uVirtqSize, pVirtio->uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(uVirtqMsixVector))
        COMMON_CFG_ACCESSOR_INDEXED(uVirtqMsixVector, pVirtio->uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(uVirtqEnable))
        COMMON_CFG_ACCESSOR_INDEXED(uVirtqEnable, pVirtio->uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(uVirtqNotifyOff))
        COMMON_CFG_ACCESSOR_INDEXED_READONLY(uVirtqNotifyOff, pVirtio->uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(aGCPhysVirtqDesc))
        COMMON_CFG_ACCESSOR_INDEXED(aGCPhysVirtqDesc, pVirtio->uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(aGCPhysVirtqAvail))
        COMMON_CFG_ACCESSOR_INDEXED(aGCPhysVirtqAvail, pVirtio->uVirtqSelect);
    else
    if (MATCH_COMMON_CFG(aGCPhysVirtqUsed))
        COMMON_CFG_ACCESSOR_INDEXED(aGCPhysVirtqUsed, pVirtio->uVirtqSelect);
    else
    {
        Log2Func(("Bad guest %s access to virtio_pci_common_cfg: offCfg=%#x (%d), cb=%d\n",
                  fWrite ? "write" : "read ", offCfg, offCfg, cb));
        return fWrite ? VINF_SUCCESS : VINF_IOM_MMIO_UNUSED_00;
    }

#undef COMMON_CFG_ACCESSOR_READONLY
#undef COMMON_CFG_ACCESSOR_INDEXED_READONLY
#undef COMMON_CFG_ACCESSOR_INDEXED
#undef COMMON_CFG_ACCESSOR
#undef LOG_COMMON_CFG_ACCESS_INDEXED
#undef LOG_COMMON_CFG_ACCESS
#undef MATCH_COMMON_CFG
#ifndef IN_RING3
    RT_NOREF(pDevIns, pVirtioCC);
#endif
    return rc;
}

/**
 * @callback_method_impl{FNIOMMMIONEWREAD,
 * Memory mapped I/O Handler for PCI Capabilities read operations.}
 *
 * This MMIO handler specifically supports the VIRTIO_PCI_CAP_PCI_CFG capability defined
 * in the VirtIO 1.0 specification, section 4.1.4.7, and as such is restricted to reads
 * of 1, 2 or 4 bytes, only.
 *
 */
static DECLCALLBACK(VBOXSTRICTRC) virtioMmioRead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void *pv, unsigned cb)
{
    PVIRTIOCORE   pVirtio   = PDMINS_2_DATA(pDevIns, PVIRTIOCORE);
    PVIRTIOCORECC pVirtioCC = PDMINS_2_DATA_CC(pDevIns, PVIRTIOCORECC);
    AssertReturn(cb == 1 || cb == 2 || cb == 4, VERR_INVALID_PARAMETER);
    Assert(pVirtio == (PVIRTIOCORE)pvUser); RT_NOREF(pvUser);


    uint32_t uOffset;
    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocDeviceCap))
    {
#ifdef IN_RING3
        /*
         * Callback to client to manage device-specific configuration.
         */
        VBOXSTRICTRC rcStrict = pVirtioCC->pfnDevCapRead(pDevIns, uOffset, pv, cb);

        /*
         * Additionally, anytime any part of the device-specific configuration (which our client maintains)
         * is READ it needs to be checked to see if it changed since the last time any part was read, in
         * order to maintain the config generation (see VirtIO 1.0 spec, section 4.1.4.3.1)
         */
        bool fDevSpecificFieldChanged = RT_BOOL(memcmp(pVirtioCC->pbDevSpecificCfg + uOffset,
                                                 pVirtioCC->pbPrevDevSpecificCfg + uOffset,
                                                 RT_MIN(cb, pVirtioCC->cbDevSpecificCfg - uOffset)));

        memcpy(pVirtioCC->pbPrevDevSpecificCfg, pVirtioCC->pbDevSpecificCfg, pVirtioCC->cbDevSpecificCfg);

        if (pVirtio->fGenUpdatePending || fDevSpecificFieldChanged)
        {
            ++pVirtio->uConfigGeneration;
            Log6Func(("Bumped cfg. generation to %d because %s%s\n",
                      pVirtio->uConfigGeneration,
                      fDevSpecificFieldChanged ? "<dev cfg changed> " : "",
                      pVirtio->fGenUpdatePending ? "<update was pending>" : ""));
            pVirtio->fGenUpdatePending = false;
        }

        virtioLowerInterrupt(pDevIns, 0);
        return rcStrict;
#else
        return VINF_IOM_R3_MMIO_READ;
#endif
    }

    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocCommonCfgCap))
        return virtioCommonCfgAccessed(pDevIns, pVirtio, pVirtioCC, false /* fWrite */, uOffset, cb, pv);

    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocIsrCap) && cb == sizeof(uint8_t))
    {
        *(uint8_t *)pv = pVirtio->uISR;
        Log6Func(("Read and clear ISR\n"));
        pVirtio->uISR = 0; /* VirtIO specification requires reads of ISR to clear it */
        virtioLowerInterrupt(pDevIns, 0);
        return VINF_SUCCESS;
    }

    ASSERT_GUEST_MSG_FAILED(("Bad read access to mapped capabilities region: off=%RGp cb=%u\n", off, cb));
    return VINF_IOM_MMIO_UNUSED_00;
}

/**
 * @callback_method_impl{FNIOMMMIONEWREAD,
 * Memory mapped I/O Handler for PCI Capabilities write operations.}
 *
 * This MMIO handler specifically supports the VIRTIO_PCI_CAP_PCI_CFG capability defined
 * in the VirtIO 1.0 specification, section 4.1.4.7, and as such is restricted to writes
 * of 1, 2 or 4 bytes, only.
 */
static DECLCALLBACK(VBOXSTRICTRC) virtioMmioWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS off, void const *pv, unsigned cb)
{
    PVIRTIOCORE   pVirtio   = PDMINS_2_DATA(pDevIns, PVIRTIOCORE);
    PVIRTIOCORECC pVirtioCC = PDMINS_2_DATA_CC(pDevIns, PVIRTIOCORECC);

    AssertReturn(cb == 1 || cb == 2 || cb == 4, VERR_INVALID_PARAMETER);

    Assert(pVirtio == (PVIRTIOCORE)pvUser); RT_NOREF(pvUser);
    uint32_t uOffset;
    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocDeviceCap))
    {
#ifdef IN_RING3
        /*
         * Pass this MMIO write access back to the client to handle
         */
        return pVirtioCC->pfnDevCapWrite(pDevIns, uOffset, pv, cb);
#else
        return VINF_IOM_R3_MMIO_WRITE;
#endif
    }

    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocCommonCfgCap))
        return virtioCommonCfgAccessed(pDevIns, pVirtio, pVirtioCC, true /* fWrite */, uOffset, cb, (void *)pv);

    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocIsrCap) && cb == sizeof(uint8_t))
    {
        pVirtio->uISR = *(uint8_t *)pv;
        Log6Func(("Setting uISR = 0x%02x (virtq interrupt: %d, dev confg interrupt: %d)\n",
                  pVirtio->uISR & 0xff,
                  pVirtio->uISR & VIRTIO_ISR_VIRTQ_INTERRUPT,
                  RT_BOOL(pVirtio->uISR & VIRTIO_ISR_DEVICE_CONFIG)));
        return VINF_SUCCESS;
    }

    /* This *should* be guest driver dropping index of a new descriptor in avail ring */
    if (MATCHES_VIRTIO_CAP_STRUCT(off, cb, uOffset, pVirtio->LocNotifyCap) && cb == sizeof(uint16_t))
    {
        virtioCoreVirtqNotified(pDevIns, pVirtio, uOffset / VIRTIO_NOTIFY_OFFSET_MULTIPLIER, *(uint16_t *)pv);
        return VINF_SUCCESS;
    }

    ASSERT_GUEST_MSG_FAILED(("Bad write access to mapped capabilities region: off=%RGp pv=%#p{%.*Rhxs} cb=%u\n", off, pv, cb, pv, cb));
    return VINF_SUCCESS;
}

#ifdef IN_RING3

/**
 * @callback_method_impl{FNPCICONFIGREAD}
 */
static DECLCALLBACK(VBOXSTRICTRC) virtioR3PciConfigRead(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev,
                                                        uint32_t uAddress, unsigned cb, uint32_t *pu32Value)
{
    PVIRTIOCORE   pVirtio   = PDMINS_2_DATA(pDevIns, PVIRTIOCORE);
    PVIRTIOCORECC pVirtioCC = PDMINS_2_DATA_CC(pDevIns, PVIRTIOCORECC);
    RT_NOREF(pPciDev);

    Log7Func(("pDevIns=%p pPciDev=%p uAddress=%#x cb=%u pu32Value=%p\n",
                 pDevIns, pPciDev, uAddress, cb, pu32Value));
    if (uAddress == pVirtio->uPciCfgDataOff)
    {
        /*
         * VirtIO 1.0 spec section 4.1.4.7 describes a required alternative access capability
         * whereby the guest driver can specify a bar, offset, and length via the PCI configuration space
         * (the virtio_pci_cfg_cap capability), and access data items.
         */
        struct virtio_pci_cap *pPciCap = &pVirtioCC->pPciCfgCap->pciCap;
        uint32_t uLength = pPciCap->uLength;

        if (   (uLength != 1 && uLength != 2 && uLength != 4)
            || cb != uLength
            ||  pPciCap->uBar != VIRTIO_REGION_PCI_CAP)
        {
            ASSERT_GUEST_MSG_FAILED(("Guest read virtio_pci_cfg_cap.pci_cfg_data using mismatching config. Ignoring\n"));
            *pu32Value = UINT32_MAX;
            return VINF_SUCCESS;
        }

        VBOXSTRICTRC rcStrict = virtioMmioRead(pDevIns, pVirtio, pPciCap->uOffset, pu32Value, cb);
        Log2Func(("virtio: Guest read  virtio_pci_cfg_cap.pci_cfg_data, bar=%d, offset=%d, length=%d, result=%d -> %Rrc\n",
                   pPciCap->uBar, pPciCap->uOffset, uLength, *pu32Value, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }
    return VINF_PDM_PCI_DO_DEFAULT;
}

/**
 * @callback_method_impl{FNPCICONFIGWRITE}
 */
static DECLCALLBACK(VBOXSTRICTRC) virtioR3PciConfigWrite(PPDMDEVINS pDevIns, PPDMPCIDEV pPciDev,
                                                         uint32_t uAddress, unsigned cb, uint32_t u32Value)
{
    PVIRTIOCORE   pVirtio   = PDMINS_2_DATA(pDevIns, PVIRTIOCORE);
    PVIRTIOCORECC pVirtioCC = PDMINS_2_DATA_CC(pDevIns, PVIRTIOCORECC);
    RT_NOREF(pPciDev);

    Log7Func(("pDevIns=%p pPciDev=%p uAddress=%#x cb=%u u32Value=%#x\n", pDevIns, pPciDev, uAddress, cb, u32Value));
    if (uAddress == pVirtio->uPciCfgDataOff)
    {
        /* VirtIO 1.0 spec section 4.1.4.7 describes a required alternative access capability
         * whereby the guest driver can specify a bar, offset, and length via the PCI configuration space
         * (the virtio_pci_cfg_cap capability), and access data items. */

        struct virtio_pci_cap *pPciCap = &pVirtioCC->pPciCfgCap->pciCap;
        uint32_t uLength = pPciCap->uLength;

        if (   (uLength != 1 && uLength != 2 && uLength != 4)
            || cb != uLength
            ||  pPciCap->uBar != VIRTIO_REGION_PCI_CAP)
        {
            ASSERT_GUEST_MSG_FAILED(("Guest write virtio_pci_cfg_cap.pci_cfg_data using mismatching config. Ignoring\n"));
            return VINF_SUCCESS;
        }

        VBOXSTRICTRC rcStrict = virtioMmioWrite(pDevIns, pVirtio, pPciCap->uOffset, &u32Value, cb);
        Log2Func(("Guest wrote  virtio_pci_cfg_cap.pci_cfg_data, bar=%d, offset=%x, length=%x, value=%d -> %Rrc\n",
                   pPciCap->uBar, pPciCap->uOffset, uLength, u32Value, VBOXSTRICTRC_VAL(rcStrict)));
        return rcStrict;
    }
    return VINF_PDM_PCI_DO_DEFAULT;
}


/*********************************************************************************************************************************
*   Saved state.                                                                                                                 *
*********************************************************************************************************************************/

/**
 * Called from the FNSSMDEVSAVEEXEC function of the device.
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   pHlp        The ring-3 device helpers.
 * @param   pSSM        The saved state handle.
 * @returns VBox status code.
 */
int virtioCoreR3SaveExec(PVIRTIOCORE pVirtio, PCPDMDEVHLPR3 pHlp, PSSMHANDLE pSSM)
{
    LogFunc(("\n"));
    pHlp->pfnSSMPutU64(pSSM, VIRTIO_SAVEDSTATE_MARKER);
    pHlp->pfnSSMPutU32(pSSM, VIRTIO_SAVEDSTATE_VERSION);

    pHlp->pfnSSMPutBool(pSSM,   pVirtio->fGenUpdatePending);
    pHlp->pfnSSMPutU8(pSSM,     pVirtio->uDeviceStatus);
    pHlp->pfnSSMPutU8(pSSM,     pVirtio->uConfigGeneration);
    pHlp->pfnSSMPutU8(pSSM,     pVirtio->uPciCfgDataOff);
    pHlp->pfnSSMPutU8(pSSM,     pVirtio->uISR);
    pHlp->pfnSSMPutU16(pSSM,    pVirtio->uVirtqSelect);
    pHlp->pfnSSMPutU32(pSSM,    pVirtio->uDeviceFeaturesSelect);
    pHlp->pfnSSMPutU32(pSSM,    pVirtio->uDriverFeaturesSelect);
    pHlp->pfnSSMPutU64(pSSM,    pVirtio->uDriverFeatures);

    for (uint32_t i = 0; i < VIRTQ_MAX_CNT; i++)
    {
        pHlp->pfnSSMPutGCPhys64(pSSM, pVirtio->aGCPhysVirtqDesc[i]);
        pHlp->pfnSSMPutGCPhys64(pSSM, pVirtio->aGCPhysVirtqAvail[i]);
        pHlp->pfnSSMPutGCPhys64(pSSM, pVirtio->aGCPhysVirtqUsed[i]);
        pHlp->pfnSSMPutU16(pSSM,      pVirtio->uVirtqNotifyOff[i]);
        pHlp->pfnSSMPutU16(pSSM,      pVirtio->uVirtqMsixVector[i]);
        pHlp->pfnSSMPutU16(pSSM,      pVirtio->uVirtqEnable[i]);
        pHlp->pfnSSMPutU16(pSSM,      pVirtio->uVirtqSize[i]);
        pHlp->pfnSSMPutU16(pSSM,      pVirtio->aVirtqState[i].uAvailIdxShadow);
        pHlp->pfnSSMPutU16(pSSM,      pVirtio->aVirtqState[i].uUsedIdxShadow);
        int rc = pHlp->pfnSSMPutMem(pSSM, pVirtio->aVirtqState[i].szVirtqName, 32);
        AssertRCReturn(rc, rc);
    }

    return VINF_SUCCESS;
}

/**
 * Called from the FNSSMDEVLOADEXEC function of the device.
 *
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   pHlp        The ring-3 device helpers.
 * @param   pSSM        The saved state handle.
 * @returns VBox status code.
 */
int virtioCoreR3LoadExec(PVIRTIOCORE pVirtio, PCPDMDEVHLPR3 pHlp, PSSMHANDLE pSSM)
{
    LogFunc(("\n"));
    /*
     * Check the marker and (embedded) version number.
     */
    uint64_t uMarker = 0;
    int rc = pHlp->pfnSSMGetU64(pSSM, &uMarker);
    AssertRCReturn(rc, rc);
    if (uMarker != VIRTIO_SAVEDSTATE_MARKER)
        return pHlp->pfnSSMSetLoadError(pSSM, VERR_SSM_DATA_UNIT_FORMAT_CHANGED, RT_SRC_POS,
                                        N_("Expected marker value %#RX64 found %#RX64 instead"),
                                        VIRTIO_SAVEDSTATE_MARKER, uMarker);
    uint32_t uVersion = 0;
    rc = pHlp->pfnSSMGetU32(pSSM, &uVersion);
    AssertRCReturn(rc, rc);
    if (uVersion != VIRTIO_SAVEDSTATE_VERSION)
        return pHlp->pfnSSMSetLoadError(pSSM, VERR_SSM_DATA_UNIT_FORMAT_CHANGED, RT_SRC_POS,
                                        N_("Unsupported virtio version: %u"), uVersion);
    /*
     * Load the state.
     */
    pHlp->pfnSSMGetBool(pSSM, &pVirtio->fGenUpdatePending);
    pHlp->pfnSSMGetU8(pSSM,   &pVirtio->uDeviceStatus);
    pHlp->pfnSSMGetU8(pSSM,   &pVirtio->uConfigGeneration);
    pHlp->pfnSSMGetU8(pSSM,   &pVirtio->uPciCfgDataOff);
    pHlp->pfnSSMGetU8(pSSM,   &pVirtio->uISR);
    pHlp->pfnSSMGetU16(pSSM,  &pVirtio->uVirtqSelect);
    pHlp->pfnSSMGetU32(pSSM,  &pVirtio->uDeviceFeaturesSelect);
    pHlp->pfnSSMGetU32(pSSM,  &pVirtio->uDriverFeaturesSelect);
    pHlp->pfnSSMGetU64(pSSM,  &pVirtio->uDriverFeatures);

    for (uint32_t i = 0; i < VIRTQ_MAX_CNT; i++)
    {
        pHlp->pfnSSMGetGCPhys64(pSSM, &pVirtio->aGCPhysVirtqDesc[i]);
        pHlp->pfnSSMGetGCPhys64(pSSM, &pVirtio->aGCPhysVirtqAvail[i]);
        pHlp->pfnSSMGetGCPhys64(pSSM, &pVirtio->aGCPhysVirtqUsed[i]);
        pHlp->pfnSSMGetU16(pSSM, &pVirtio->uVirtqNotifyOff[i]);
        pHlp->pfnSSMGetU16(pSSM, &pVirtio->uVirtqMsixVector[i]);
        pHlp->pfnSSMGetU16(pSSM, &pVirtio->uVirtqEnable[i]);
        pHlp->pfnSSMGetU16(pSSM, &pVirtio->uVirtqSize[i]);
        pHlp->pfnSSMGetU16(pSSM, &pVirtio->aVirtqState[i].uAvailIdxShadow);
        pHlp->pfnSSMGetU16(pSSM, &pVirtio->aVirtqState[i].uUsedIdxShadow);
        rc = pHlp->pfnSSMGetMem(pSSM, pVirtio->aVirtqState[i].szVirtqName,
                                sizeof(pVirtio->aVirtqState[i].szVirtqName));
        AssertRCReturn(rc, rc);
    }

    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Device Level                                                                                                                 *
*********************************************************************************************************************************/

/**
 * This must be called by the client to handle VM state changes
 * after the client takes care of its device-specific tasks for the state change.
 * (i.e. Reset, suspend, power-off, resume)
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 */
void virtioCoreR3VmStateChanged(PVIRTIOCORE pVirtio, VIRTIOVMSTATECHANGED enmState)
{
    LogFunc(("State changing to %s\n",
        virtioCoreGetStateChangeText(enmState)));

    switch(enmState)
    {
        case kvirtIoVmStateChangedReset:
            virtioCoreResetAll(pVirtio);
            break;
        case kvirtIoVmStateChangedSuspend:
            break;
        case kvirtIoVmStateChangedPowerOff:
            break;
        case kvirtIoVmStateChangedResume:
            virtioCoreNotifyGuestDriver(pVirtio->pDevInsR3, pVirtio, 0 /* uVirtqNbr */);
            break;
        default:
            LogRelFunc(("Bad enum value"));
            return;
    }
}

/**
 * This should be called from PDMDEVREGR3::pfnDestruct.
 *
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.
 * @param   pVirtioCC   Pointer to the ring-3 virtio state.
 */
void virtioCoreR3Term(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC)
{
    if (pVirtioCC->pbPrevDevSpecificCfg)
    {
        RTMemFree(pVirtioCC->pbPrevDevSpecificCfg);
        pVirtioCC->pbPrevDevSpecificCfg = NULL;
    }
    RT_NOREF(pDevIns, pVirtio);
}


/**rr
 * Setup PCI device controller and Virtio state
 *
 * This should be called from PDMDEVREGR3::pfnConstruct.
 *
 * @param   pDevIns                 The device instance.
 * @param   pVirtio                 Pointer to the shared virtio state.  This
 *                                  must be the first member in the shared
 *                                  device instance data!
 * @param   pVirtioCC               Pointer to the ring-3 virtio state.  This
 *                                  must be the first member in the ring-3
 *                                  device instance data!
 * @param   pPciParams              Values to populate industry standard PCI Configuration Space data structure
 * @param   pcszInstance            Device instance name (format-specifier)
 * @param   fDevSpecificFeatures    VirtIO device-specific features offered by
 *                                  client
 * @param   cbDevSpecificCfg        Size of virtio_pci_device_cap device-specific struct
 * @param   pvDevSpecificCfg        Address of client's dev-specific
 *                                  configuration struct.
 */
int virtioCoreR3Init(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio, PVIRTIOCORECC pVirtioCC, PVIRTIOPCIPARAMS pPciParams,
                     const char *pcszInstance, uint64_t fDevSpecificFeatures, void *pvDevSpecificCfg, uint16_t cbDevSpecificCfg)
{
    /*
     * The pVirtio state must be the first member of the shared device instance
     * data, otherwise we cannot get our bearings in the PCI configuration callbacks.
     */
    AssertLogRelReturn(pVirtio == PDMINS_2_DATA(pDevIns, PVIRTIOCORE), VERR_STATE_CHANGED);
    AssertLogRelReturn(pVirtioCC == PDMINS_2_DATA_CC(pDevIns, PVIRTIOCORECC), VERR_STATE_CHANGED);

    pVirtio->pDevInsR3 = pDevIns;

    /*
     * Caller must initialize these.
     */
    AssertReturn(pVirtioCC->pfnStatusChanged, VERR_INVALID_POINTER);
    AssertReturn(pVirtioCC->pfnVirtqNotified, VERR_INVALID_POINTER);

#if 0 /* Until pdmR3DvHlp_PCISetIrq() impl is fixed and Assert that limits vec to 0 is removed */
# ifdef VBOX_WITH_MSI_DEVICES
    pVirtio->fMsiSupport = true;
# endif
#endif

    /*
     * The host features offered include both device-specific features
     * and reserved feature bits (device independent)
     */
    pVirtio->uDeviceFeatures = VIRTIO_F_VERSION_1
                             | VIRTIO_DEV_INDEPENDENT_FEATURES_OFFERED
                             | fDevSpecificFeatures;

    RTStrCopy(pVirtio->szInstance, sizeof(pVirtio->szInstance), pcszInstance);

    pVirtio->uDeviceStatus = 0;
    pVirtioCC->cbDevSpecificCfg = cbDevSpecificCfg;
    pVirtioCC->pbDevSpecificCfg = (uint8_t *)pvDevSpecificCfg;
    pVirtioCC->pbPrevDevSpecificCfg = (uint8_t *)RTMemDup(pvDevSpecificCfg, cbDevSpecificCfg);
    AssertLogRelReturn(pVirtioCC->pbPrevDevSpecificCfg, VERR_NO_MEMORY);

    /* Set PCI config registers (assume 32-bit mode) */
    PPDMPCIDEV pPciDev = pDevIns->apPciDevs[0];
    PDMPCIDEV_ASSERT_VALID(pDevIns, pPciDev);

    PDMPciDevSetRevisionId(pPciDev,         DEVICE_PCI_REVISION_ID_VIRTIO);
    PDMPciDevSetVendorId(pPciDev,           DEVICE_PCI_VENDOR_ID_VIRTIO);
    PDMPciDevSetSubSystemVendorId(pPciDev,  DEVICE_PCI_VENDOR_ID_VIRTIO);
    PDMPciDevSetDeviceId(pPciDev,           pPciParams->uDeviceId);
    PDMPciDevSetClassBase(pPciDev,          pPciParams->uClassBase);
    PDMPciDevSetClassSub(pPciDev,           pPciParams->uClassSub);
    PDMPciDevSetClassProg(pPciDev,          pPciParams->uClassProg);
    PDMPciDevSetSubSystemId(pPciDev,        pPciParams->uSubsystemId);
    PDMPciDevSetInterruptLine(pPciDev,      pPciParams->uInterruptLine);
    PDMPciDevSetInterruptPin(pPciDev,       pPciParams->uInterruptPin);

    /* Register PCI device */
    int rc = PDMDevHlpPCIRegister(pDevIns, pPciDev);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("virtio: cannot register PCI Device")); /* can we put params in this error? */

    rc = PDMDevHlpPCIInterceptConfigAccesses(pDevIns, pPciDev, virtioR3PciConfigRead, virtioR3PciConfigWrite);
    AssertRCReturn(rc, rc);


    /* Construct & map PCI vendor-specific capabilities for virtio host negotiation with guest driver */

    /* The following capability mapped via VirtIO 1.0: struct virtio_pci_cfg_cap (VIRTIO_PCI_CFG_CAP_T)
     * as a mandatory but suboptimal alternative interface to host device capabilities, facilitating
     * access the memory of any BAR. If the guest uses it (the VirtIO driver on Linux doesn't),
     * Unlike Common, Notify, ISR and Device capabilities, it is accessed directly via PCI Config region.
     * therefore does not contribute to the capabilities region (BAR) the other capabilities use.
     */
#define CFG_ADDR_2_IDX(addr) ((uint8_t)(((uintptr_t)(addr) - (uintptr_t)&pPciDev->abConfig[0])))
#define SET_PCI_CAP_LOC(a_pPciDev, a_pCfg, a_LocCap, a_uMmioLengthAlign) \
        do { \
            (a_LocCap).offMmio = (a_pCfg)->uOffset; \
            (a_LocCap).cbMmio  = RT_ALIGN_T((a_pCfg)->uLength, a_uMmioLengthAlign, uint16_t); \
            (a_LocCap).offPci  = (uint16_t)(uintptr_t)((uint8_t *)(a_pCfg) - &(a_pPciDev)->abConfig[0]); \
            (a_LocCap).cbPci   = (a_pCfg)->uCapLen; \
        } while (0)

    PVIRTIO_PCI_CAP_T pCfg;
    uint32_t cbRegion = 0;

    /* Common capability (VirtIO 1.0 spec, section 4.1.4.3) */
    pCfg = (PVIRTIO_PCI_CAP_T)&pPciDev->abConfig[0x40];
    pCfg->uCfgType = VIRTIO_PCI_CAP_COMMON_CFG;
    pCfg->uCapVndr = VIRTIO_PCI_CAP_ID_VENDOR;
    pCfg->uCapLen  = sizeof(VIRTIO_PCI_CAP_T);
    pCfg->uCapNext = CFG_ADDR_2_IDX(pCfg) + pCfg->uCapLen;
    pCfg->uBar     = VIRTIO_REGION_PCI_CAP;
    pCfg->uOffset  = RT_ALIGN_32(0, 4); /* reminder, in case someone changes offset */
    pCfg->uLength  = sizeof(VIRTIO_PCI_COMMON_CFG_T);
    cbRegion += pCfg->uLength;
    SET_PCI_CAP_LOC(pPciDev, pCfg, pVirtio->LocCommonCfgCap, 2);
    pVirtioCC->pCommonCfgCap = pCfg;

    /*
     * Notify capability (VirtIO 1.0 spec, section 4.1.4.4). Note: uLength is based on the choice
     * of this implementation to make each queue's uVirtqNotifyOff equal to (VirtqSelect) ordinal
     * value of the queue (different strategies are possible according to spec).
     */
    pCfg = (PVIRTIO_PCI_CAP_T)&pPciDev->abConfig[pCfg->uCapNext];
    pCfg->uCfgType = VIRTIO_PCI_CAP_NOTIFY_CFG;
    pCfg->uCapVndr = VIRTIO_PCI_CAP_ID_VENDOR;
    pCfg->uCapLen  = sizeof(VIRTIO_PCI_NOTIFY_CAP_T);
    pCfg->uCapNext = CFG_ADDR_2_IDX(pCfg) + pCfg->uCapLen;
    pCfg->uBar     = VIRTIO_REGION_PCI_CAP;
    pCfg->uOffset  = pVirtioCC->pCommonCfgCap->uOffset + pVirtioCC->pCommonCfgCap->uLength;
    pCfg->uOffset  = RT_ALIGN_32(pCfg->uOffset, 4);


    pCfg->uLength  = VIRTQ_MAX_CNT * VIRTIO_NOTIFY_OFFSET_MULTIPLIER + 2;  /* will change in VirtIO 1.1 */
    cbRegion += pCfg->uLength;
    SET_PCI_CAP_LOC(pPciDev, pCfg, pVirtio->LocNotifyCap, 1);
    pVirtioCC->pNotifyCap = (PVIRTIO_PCI_NOTIFY_CAP_T)pCfg;
    pVirtioCC->pNotifyCap->uNotifyOffMultiplier = VIRTIO_NOTIFY_OFFSET_MULTIPLIER;

    /* ISR capability (VirtIO 1.0 spec, section 4.1.4.5)
     *
     * VirtIO 1.0 spec says 8-bit, unaligned in MMIO space. Example/diagram
     * of spec shows it as a 32-bit field with upper bits 'reserved'
     * Will take spec's words more literally than the diagram for now.
     */
    pCfg = (PVIRTIO_PCI_CAP_T)&pPciDev->abConfig[pCfg->uCapNext];
    pCfg->uCfgType = VIRTIO_PCI_CAP_ISR_CFG;
    pCfg->uCapVndr = VIRTIO_PCI_CAP_ID_VENDOR;
    pCfg->uCapLen  = sizeof(VIRTIO_PCI_CAP_T);
    pCfg->uCapNext = CFG_ADDR_2_IDX(pCfg) + pCfg->uCapLen;
    pCfg->uBar     = VIRTIO_REGION_PCI_CAP;
    pCfg->uOffset  = pVirtioCC->pNotifyCap->pciCap.uOffset + pVirtioCC->pNotifyCap->pciCap.uLength;
    pCfg->uOffset  = RT_ALIGN_32(pCfg->uOffset, 4);
    pCfg->uLength  = sizeof(uint8_t);
    cbRegion += pCfg->uLength;
    SET_PCI_CAP_LOC(pPciDev, pCfg, pVirtio->LocIsrCap, 4);
    pVirtioCC->pIsrCap = pCfg;

    /*  PCI Cfg capability (VirtIO 1.0 spec, section 4.1.4.7)
     *  This capability doesn't get page-MMIO mapped. Instead uBar, uOffset and uLength are intercepted
     *  by trapping PCI configuration I/O and get modulated by consumers to locate fetch and read/write
     *  values from any region. NOTE: The linux driver not only doesn't use this feature, it will not
     *  even list it as present if uLength isn't non-zero and also 4-byte-aligned as the linux driver is
     *  initializing.
     */
    pVirtio->uPciCfgDataOff = pCfg->uCapNext + RT_OFFSETOF(VIRTIO_PCI_CFG_CAP_T, uPciCfgData);
    pCfg = (PVIRTIO_PCI_CAP_T)&pPciDev->abConfig[pCfg->uCapNext];
    pCfg->uCfgType = VIRTIO_PCI_CAP_PCI_CFG;
    pCfg->uCapVndr = VIRTIO_PCI_CAP_ID_VENDOR;
    pCfg->uCapLen  = sizeof(VIRTIO_PCI_CFG_CAP_T);
    pCfg->uCapNext = (pVirtio->fMsiSupport || pVirtioCC->pbDevSpecificCfg) ? CFG_ADDR_2_IDX(pCfg) + pCfg->uCapLen : 0;
    pCfg->uBar     = 0;
    pCfg->uOffset  = 0;
    pCfg->uLength  = 0;
    cbRegion += pCfg->uLength;
    SET_PCI_CAP_LOC(pPciDev, pCfg, pVirtio->LocPciCfgCap, 1);
    pVirtioCC->pPciCfgCap = (PVIRTIO_PCI_CFG_CAP_T)pCfg;

    if (pVirtioCC->pbDevSpecificCfg)
    {
        /* Following capability (via VirtIO 1.0, section 4.1.4.6). Client defines the
         * device-specific config fields struct and passes size to this constructor */
        pCfg = (PVIRTIO_PCI_CAP_T)&pPciDev->abConfig[pCfg->uCapNext];
        pCfg->uCfgType = VIRTIO_PCI_CAP_DEVICE_CFG;
        pCfg->uCapVndr = VIRTIO_PCI_CAP_ID_VENDOR;
        pCfg->uCapLen  = sizeof(VIRTIO_PCI_CAP_T);
        pCfg->uCapNext = pVirtio->fMsiSupport ? CFG_ADDR_2_IDX(pCfg) + pCfg->uCapLen : 0;
        pCfg->uBar     = VIRTIO_REGION_PCI_CAP;
        pCfg->uOffset  = pVirtioCC->pIsrCap->uOffset + pVirtioCC->pIsrCap->uLength;
        pCfg->uOffset  = RT_ALIGN_32(pCfg->uOffset, 4);
        pCfg->uLength  = cbDevSpecificCfg;
        cbRegion += pCfg->uLength;
        SET_PCI_CAP_LOC(pPciDev, pCfg, pVirtio->LocDeviceCap, 4);
        pVirtioCC->pDeviceCap = pCfg;
    }
    else
        Assert(pVirtio->LocDeviceCap.cbMmio == 0 && pVirtio->LocDeviceCap.cbPci == 0);

    if (pVirtio->fMsiSupport)
    {
        PDMMSIREG aMsiReg;
        RT_ZERO(aMsiReg);
        aMsiReg.iMsixCapOffset  = pCfg->uCapNext;
        aMsiReg.iMsixNextOffset = 0;
        aMsiReg.iMsixBar        = VIRTIO_REGION_MSIX_CAP;
        aMsiReg.cMsixVectors    = VBOX_MSIX_MAX_ENTRIES;
        rc = PDMDevHlpPCIRegisterMsi(pDevIns, &aMsiReg); /* see MsixR3init() */
        if (RT_FAILURE(rc))
        {
            /* See PDMDevHlp.cpp:pdmR3DevHlp_PCIRegisterMsi */
            LogFunc(("Failed to configure MSI-X (%Rrc). Reverting to INTx\n", rc));
            pVirtio->fMsiSupport = false;
        }
        else
            Log2Func(("Using MSI-X for guest driver notification\n"));
    }
    else
        LogFunc(("MSI-X not available for VBox, using INTx notification\n"));

    /* Set offset to first capability and enable PCI dev capabilities */
    PDMPciDevSetCapabilityList(pPciDev, 0x40);
    PDMPciDevSetStatus(pPciDev, VBOX_PCI_STATUS_CAP_LIST);

    /* Linux drivers/virtio/virtio_pci_modern.c tries to map at least a page for the
     * 'unknown' device-specific capability without querying the capability to figure
     *  out size, so pad with an extra page
     */
    size_t cbSize = RTStrPrintf(pVirtioCC->pcszMmioName, sizeof(pVirtioCC->pcszMmioName), "%s MMIO", pcszInstance);
    if (cbSize <= 0)
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("virtio: out of memory allocating string")); /* can we put params in this error? */

    rc = PDMDevHlpPCIIORegionCreateMmio(pDevIns, VIRTIO_REGION_PCI_CAP, RT_ALIGN_32(cbRegion + PAGE_SIZE, PAGE_SIZE),
                                        PCI_ADDRESS_SPACE_MEM, virtioMmioWrite, virtioMmioRead, pVirtio,
                                        IOMMMIO_FLAGS_READ_PASSTHRU | IOMMMIO_FLAGS_WRITE_PASSTHRU,
                                        pVirtioCC->pcszMmioName,
                                        &pVirtio->hMmioPciCap);
    AssertLogRelRCReturn(rc, PDMDEV_SET_ERROR(pDevIns, rc, N_("virtio: cannot register PCI Capabilities address space")));
    /*
     * Statistics.
     */
    PDMDevHlpSTAMRegisterF(pDevIns, &pVirtio->StatDescChainsAllocated,  STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                           "Total number of allocated descriptor chains",   "DescChainsAllocated");
    PDMDevHlpSTAMRegisterF(pDevIns, &pVirtio->StatDescChainsFreed,      STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                           "Total number of freed descriptor chains",       "DescChainsFreed");
    PDMDevHlpSTAMRegisterF(pDevIns, &pVirtio->StatDescChainsSegsIn,     STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                           "Total number of inbound segments",              "DescChainsSegsIn");
    PDMDevHlpSTAMRegisterF(pDevIns, &pVirtio->StatDescChainsSegsOut,    STAMTYPE_COUNTER, STAMVISIBILITY_ALWAYS, STAMUNIT_COUNT,
                           "Total number of outbound segments",             "DescChainsSegsOut");

    return VINF_SUCCESS;
}

#else  /* !IN_RING3 */

/**
 * Sets up the core ring-0/raw-mode virtio bits.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pVirtio     Pointer to the shared virtio state.  This must be the first
 *                      member in the shared device instance data!
 */
int virtioCoreRZInit(PPDMDEVINS pDevIns, PVIRTIOCORE pVirtio)
{
    AssertLogRelReturn(pVirtio == PDMINS_2_DATA(pDevIns, PVIRTIOCORE), VERR_STATE_CHANGED);

#ifdef FUTURE_OPTIMIZATION
    int rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);
#endif
    int rc = PDMDevHlpMmioSetUpContext(pDevIns, pVirtio->hMmioPciCap, virtioMmioWrite, virtioMmioRead, pVirtio);
    AssertRCReturn(rc, rc);
    return rc;
}

#endif /* !IN_RING3 */

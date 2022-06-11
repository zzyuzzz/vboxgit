/* $Id: DevVirtioSCSI.cpp 81660 2019-11-04 21:46:54Z vboxsync $ $Revision: 81660 $ $Date: 2019-11-05 05:46:54 +0800 (Tue, 05 Nov 2019) $ $Author: vboxsync $ */
/** @file
 * VBox storage devices - Virtio SCSI Driver
 *
 * Log-levels used:
 *    - Level 1:   The most important (but usually rare) things to note
 *    - Level 2:   SCSI command logging
 *    - Level 3:   Vector and I/O transfer summary (shows what client sent an expects and fulfillment)
 *    - Level 6:   Device ⟷ Guest Driver negotation, traffic, notifications and state handling
 *    - Level 12:  Brief formatted hex dumps of I/O data
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
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
//#define LOG_GROUP LOG_GROUP_DRV_SCSI
#define LOG_GROUP LOG_GROUP_DEV_VIRTIO

#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmstorageifs.h>
#include <VBox/vmm/pdmcritsect.h>
#include <VBox/msi.h>
#include <VBox/version.h>
#include <VBox/log.h>
#include <iprt/errcore.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <VBox/sup.h>
#include "../build/VBoxDD.h"
#include <VBox/scsi.h>
#ifdef IN_RING3
# include <iprt/alloc.h>
# include <iprt/memcache.h>
# include <iprt/semaphore.h>
# include <iprt/sg.h>
# include <iprt/param.h>
# include <iprt/uuid.h>
#endif
#include "../VirtIO/Virtio_1_0.h"

#include "VBoxSCSI.h"
#include "VBoxDD.h"


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/** The current saved state version. */
#define VIRTIOSCSI_SAVED_STATE_VERSION          UINT32_C(1)


#define LUN0    0
/** @name VirtIO 1.0 SCSI Host feature bits (See VirtIO 1.0 specification, Section 5.6.3)
 * @{  */
#define VIRTIO_SCSI_F_INOUT                RT_BIT_64(0)         /** Request is device readable AND writeable         */
#define VIRTIO_SCSI_F_HOTPLUG              RT_BIT_64(1)         /** Host allows hotplugging SCSI LUNs & targets      */
#define VIRTIO_SCSI_F_CHANGE               RT_BIT_64(2)         /** Host LUNs chgs via VIRTIOSCSI_T_PARAM_CHANGE evt */
#define VIRTIO_SCSI_F_T10_PI               RT_BIT_64(3)         /** Add T10 port info (DIF/DIX) in SCSI req hdr      */
/** @} */


#define VIRTIOSCSI_HOST_SCSI_FEATURES_ALL \
    (VIRTIO_SCSI_F_INOUT | VIRTIO_SCSI_F_HOTPLUG | VIRTIO_SCSI_F_CHANGE | VIRTIO_SCSI_F_T10_PI)

#define VIRTIOSCSI_HOST_SCSI_FEATURES_NONE          0

#define VIRTIOSCSI_HOST_SCSI_FEATURES_OFFERED       VIRTIOSCSI_HOST_SCSI_FEATURES_NONE

#define VIRTIOSCSI_REQ_QUEUE_CNT                    1           /**< T.B.D. Consider increasing                      */
#define VIRTIOSCSI_QUEUE_CNT                        (VIRTIOSCSI_REQ_QUEUE_CNT + 2)
#define VIRTIOSCSI_MAX_LUN                          256         /**< VirtIO specification, section 5.6.4             */
#define VIRTIOSCSI_MAX_COMMANDS_PER_LUN             128         /**< T.B.D. What is a good value for this?           */
#define VIRTIOSCSI_MAX_SEG_COUNT                    126         /**< T.B.D. What is a good value for this?           */
#define VIRTIOSCSI_MAX_SECTORS_HINT                 0x10000     /**< VirtIO specification, section 5.6.4             */
#define VIRTIOSCSI_MAX_CHANNEL_HINT                 0           /**< VirtIO specification, section 5.6.4 should be 0 */
#define VIRTIOSCSI_SAVED_STATE_MINOR_VERSION        0x01        /**< SSM version #                                   */

#define PCI_DEVICE_ID_VIRTIOSCSI_HOST               0x1048      /**< Informs guest driver of type of VirtIO device   */
#define PCI_CLASS_BASE_MASS_STORAGE                 0x01        /**< PCI Mass Storage device class                   */
#define PCI_CLASS_SUB_SCSI_STORAGE_CONTROLLER       0x00        /**< PCI SCSI Controller subclass                    */
#define PCI_CLASS_PROG_UNSPECIFIED                  0x00        /**< Programming interface. N/A.                     */
#define VIRTIOSCSI_PCI_CLASS                        0x01        /**< Base class Mass Storage?                        */

#define VIRTIOSCSI_SENSE_SIZE_DEFAULT               96          /**< VirtIO 1.0: 96 on reset, guest can change       */
#define VIRTIOSCSI_CDB_SIZE_DEFAULT                 32          /**< VirtIO 1.0: 32 on reset, guest can change       */
#define VIRTIOSCSI_PI_BYTES_IN                      1           /**< Value TBD (see section 5.6.6.1)                 */
#define VIRTIOSCSI_PI_BYTES_OUT                     1           /**< Value TBD (see section 5.6.6.1)                 */
#define VIRTIOSCSI_DATA_OUT                         512         /**< Value TBD (see section 5.6.6.1)                 */

/**
 * VirtIO SCSI Host Device device-specific queue indicies.
 * (Note: # of request queues is determined by virtio_scsi_config.num_queues. VirtIO 1.0, 5.6.4)
 */
#define CONTROLQ_IDX                                0           /**< Spec-defined Index of control queue             */
#define EVENTQ_IDX                                  1           /**< Spec-defined Index of event queue               */
#define VIRTQ_REQ_BASE                              2           /**< Spec-defined base index of request queues       */

#define QUEUENAME(qIdx) (pThis->aszQueueNames[qIdx])            /**< Macro to get queue name from its index          */
#define CBQUEUENAME(qIdx) RTStrNLen(QUEUENAME(qIdx), sizeof(QUEUENAME(qIdx)))

#define IS_REQ_QUEUE(qIdx) (qIdx >= VIRTQ_REQ_BASE && qIdx < VIRTIOSCSI_QUEUE_CNT)

#define VIRTIO_IS_IN_DIRECTION(pMediaExTxDirEnumValue) \
    ((pMediaExTxDirEnumValue) == PDMMEDIAEXIOREQSCSITXDIR_FROM_DEVICE)

#define VIRTIO_IS_OUT_DIRECTION(pMediaExTxDirEnumValue) \
    ((pMediaExTxDirEnumValue) == PDMMEDIAEXIOREQSCSITXDIR_TO_DEVICE)


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * VirtIO SCSI Host Device device-specific configuration (see VirtIO 1.0, section 5.6.4)
 * VBox VirtIO framework issues callback to this client (device) to handle MMIO accesses
 * to the device-specific configuration parameters.
 */
typedef struct virtio_scsi_config
{
    uint32_t uNumQueues;                                        /**< num_queues       \# of req q's exposed by dev   */
    uint32_t uSegMax;                                           /**< seg_max          Max \# of segs allowed in cmd  */
    uint32_t uMaxSectors;                                       /**< max_sectors      Hint to guest max xfer to use  */
    uint32_t uCmdPerLun;                                        /**< cmd_per_lun      Max \# of link cmd sent per lun */
    uint32_t uEventInfoSize;                                    /**< event_info_size  Fill max, evtq bufs            */
    uint32_t uSenseSize;                                        /**< sense_size       Max sense data size dev writes */
    uint32_t uCdbSize;                                          /**< cdb_size         Max CDB size driver writes     */
    uint16_t uMaxChannel;                                       /**< max_channel      Hint to guest driver           */
    uint16_t uMaxTarget;                                        /**< max_target       Hint to guest driver           */
    uint32_t uMaxLun;                                           /**< max_lun          Hint to guest driver           */
} VIRTIOSCSI_CONFIG_T, PVIRTIOSCSI_CONFIG_T;

/** @name VirtIO 1.0 SCSI Host Device device specific control types
 * @{  */
#define VIRTIOSCSI_T_NO_EVENT                       0
#define VIRTIOSCSI_T_TRANSPORT_RESET                1
#define VIRTIOSCSI_T_ASYNC_NOTIFY                   2           /**< Asynchronous notification                       */
#define VIRTIOSCSI_T_PARAM_CHANGE                   3
/** @} */

/**
 * Device operation: eventq
 */
#define VIRTIOSCSI_T_EVENTS_MISSED                  UINT32_C(0x80000000)
typedef struct virtio_scsi_event
{
    // Device-writable part
    uint32_t uEvent;                                            /**< event:                                          */
    uint8_t  abVirtioLun[8];                                    /**< lun                                             */
    uint32_t uReason;                                           /**< reason                                          */
} VIRTIOSCSI_EVENT_T, *PVIRTIOSCSI_EVENT_T;

/** @name VirtIO 1.0 SCSI Host Device device specific event types
 * @{  */
#define VIRTIOSCSI_EVT_RESET_HARD                   0           /**<                                                 */
#define VIRTIOSCSI_EVT_RESET_RESCAN                 1           /**<                                                 */
#define VIRTIOSCSI_EVT_RESET_REMOVED                2           /**<                                                 */
/** @} */



/**
 * Device operation: reqestq
 */
#pragma pack(1)
typedef struct REQ_CMD_HDR_T
{
    uint8_t  abVirtioLun[8];                                    /**< lun                                          */
    uint64_t uId;                                               /**< id                                           */
    uint8_t  uTaskAttr;                                         /**< task_attr                                    */
    uint8_t  uPrio;                                             /**< prio                                         */
    uint8_t  uCrn;                                              /**< crn                                          */
} REQ_CMD_HDR_T;
#pragma pack()
AssertCompileSize(REQ_CMD_HDR_T, 19);

typedef struct REQ_CMD_PI_T
{
    uint32_t uPiBytesOut;                                       /**< pi_bytesout                                  */
    uint32_t uPiBytesIn;                                        /**< pi_bytesin                                   */
} REQ_CMD_PI_T;
AssertCompileSize(REQ_CMD_PI_T, 8);

typedef struct REQ_RESP_HDR_T
{
    uint32_t cbSenseLen;                                        /**< sense_len                                    */
    uint32_t uResidual;                                         /**< residual                                     */
    uint16_t uStatusQualifier;                                  /**< status_qualifier                             */
    uint8_t  uStatus;                                           /**< status            SCSI status code           */
    uint8_t  uResponse;                                         /**< response                                     */
} REQ_RESP_HDR_T;
AssertCompileSize(REQ_RESP_HDR_T, 12);

#pragma pack(1)
typedef struct VIRTIOSCSI_REQ_CMD_T
{
    /** Device-readable section
     * @{ */
    REQ_CMD_HDR_T  ReqHdr;
    uint8_t  uCdb[1];                                           /**< cdb                                          */

    REQ_CMD_PI_T piHdr;                                         /** T10 Pi block integrity (optional feature)     */
    uint8_t  uPiOut[1];                                         /**< pi_out[]          T10 pi block integrity     */
    uint8_t  uDataOut[1];                                       /**< dataout                                      */
    /** @} */

    /** @name Device writable section
     * @{ */
    REQ_RESP_HDR_T respHdr;
    uint8_t  uSense[1];                                         /**< sense                                        */
    uint8_t  uPiIn[1];                                          /**< pi_in[]           T10 Pi block integrity     */
    uint8_t  uDataIn[1];                                        /**< detain;                                      */
    /** @} */
} VIRTIOSCSI_REQ_CMD_T, *PVIRTIOSCSI_REQ_CMD_T;
#pragma pack()
AssertCompileSize(VIRTIOSCSI_REQ_CMD_T, 19+8+12+6);

/** @name VirtIO 1.0 SCSI Host Device Req command-specific response values
 * @{  */
#define VIRTIOSCSI_S_OK                             0           /**< control, command                                 */
#define VIRTIOSCSI_S_OVERRUN                        1           /**< control                                          */
#define VIRTIOSCSI_S_ABORTED                        2           /**< control                                          */
#define VIRTIOSCSI_S_BAD_TARGET                     3           /**< control, command                                 */
#define VIRTIOSCSI_S_RESET                          4           /**< control                                          */
#define VIRTIOSCSI_S_BUSY                           5           /**< control, command                                 */
#define VIRTIOSCSI_S_TRANSPORT_FAILURE              6           /**< control, command                                 */
#define VIRTIOSCSI_S_TARGET_FAILURE                 7           /**< control, command                                 */
#define VIRTIOSCSI_S_NEXUS_FAILURE                  8           /**< control, command                                 */
#define VIRTIOSCSI_S_FAILURE                        9           /**< control, command                                 */
#define VIRTIOSCSI_S_INCORRECT_LUN                  12          /**< command                                          */
/** @} */

/** @name VirtIO 1.0 SCSI Host Device command-specific task_attr values
 * @{  */
#define VIRTIOSCSI_S_SIMPLE                         0           /**<                                                  */
#define VIRTIOSCSI_S_ORDERED                        1           /**<                                                  */
#define VIRTIOSCSI_S_HEAD                           2           /**<                                                  */
#define VIRTIOSCSI_S_ACA                            3           /**<                                                  */
/** @} */

/**
 * VirtIO 1.0 SCSI Host Device Control command before we know type (5.6.6.2)
 */
typedef struct VIRTIOSCSI_CTRL_T
{
    uint32_t uType;
} VIRTIOSCSI_CTRL_T, *PVIRTIOSCSI_CTRL_T;

/** @name VirtIO 1.0 SCSI Host Device command-specific TMF values
 * @{  */
#define VIRTIOSCSI_T_TMF                            0           /**<                                                  */
#define VIRTIOSCSI_T_TMF_ABORT_TASK                 0           /**<                                                  */
#define VIRTIOSCSI_T_TMF_ABORT_TASK_SET             1           /**<                                                  */
#define VIRTIOSCSI_T_TMF_CLEAR_ACA                  2           /**<                                                  */
#define VIRTIOSCSI_T_TMF_CLEAR_TASK_SET             3           /**<                                                  */
#define VIRTIOSCSI_T_TMF_I_T_NEXUS_RESET            4           /**<                                                  */
#define VIRTIOSCSI_T_TMF_LOGICAL_UNIT_RESET         5           /**<                                                  */
#define VIRTIOSCSI_T_TMF_QUERY_TASK                 6           /**<                                                  */
#define VIRTIOSCSI_T_TMF_QUERY_TASK_SET             7           /**<                                                  */
/** @} */

#pragma pack(1)
typedef struct VIRTIOSCSI_CTRL_TMF_T
{
     // Device-readable part
    uint32_t uType;                                             /**< type                                             */
    uint32_t uSubtype;                                          /**< subtype                                          */
    uint8_t  abScsiLun[8];                                      /**< lun                                              */
    uint64_t uId;                                               /**< id                                               */
    // Device-writable part
    uint8_t  uResponse;                                         /**< response                                         */
} VIRTIOSCSI_CTRL_TMF_T, *PVIRTIOSCSI_CTRL_TMF_T;
#pragma pack()
AssertCompileSize(VIRTIOSCSI_CTRL_TMF_T, 25);

/** @name VirtIO 1.0 SCSI Host Device device specific tmf control response values
 * @{  */
#define VIRTIOSCSI_S_FUNCTION_COMPLETE              0           /**<                                                   */
#define VIRTIOSCSI_S_FUNCTION_SUCCEEDED             10          /**<                                                   */
#define VIRTIOSCSI_S_FUNCTION_REJECTED              11          /**<                                                   */
/** @} */

#define VIRTIOSCSI_T_AN_QUERY                       1           /**<Asynchronous notification query                    */
#define VIRTIOSCSI_T_AN_SUBSCRIBE                   2           /**<Asynchronous notification subscription             */

#pragma pack(1)
typedef struct VIRTIOSCSI_CTRL_AN_T
{
    // Device-readable part
    uint32_t  uType;                                            /**< type                                              */
    uint8_t   abScsiLun[8];                                     /**< lun                                               */
    uint32_t  fEventsRequested;                                 /**< event_requested                                   */
    // Device-writable part
    uint32_t  uEventActual;                                     /**< event_actual                                      */
    uint8_t   uResponse;                                        /**< response                                          */
}  VIRTIOSCSI_CTRL_AN_T, *PVIRTIOSCSI_CTRL_AN_T;
#pragma pack()
AssertCompileSize(VIRTIOSCSI_CTRL_AN_T, 21);

typedef union VIRTIO_SCSI_CTRL_UNION_T
{
    VIRTIOSCSI_CTRL_T       scsiCtrl;
    VIRTIOSCSI_CTRL_TMF_T   scsiCtrlTmf;
    VIRTIOSCSI_CTRL_AN_T    scsiCtrlAsyncNotify;
    uint8_t                 ab[25];
} VIRTIO_SCSI_CTRL_UNION_T, *PVIRTIO_SCSI_CTRL_UNION_T;
AssertCompile(sizeof(VIRTIO_SCSI_CTRL_UNION_T) == 28); /* VIRTIOSCSI_CTRL_T forces 4 byte alignment, the other two are byte packed. */

/** @name VirtIO 1.0 SCSI Host Device device specific tmf control response values
 * @{  */
#define VIRTIOSCSI_EVT_ASYNC_OPERATIONAL_CHANGE  2              /**<                                                   */
#define VIRTIOSCSI_EVT_ASYNC_POWER_MGMT          4              /**<                                                   */
#define VIRTIOSCSI_EVT_ASYNC_EXTERNAL_REQUEST    8              /**<                                                   */
#define VIRTIOSCSI_EVT_ASYNC_MEDIA_CHANGE        16             /**<                                                   */
#define VIRTIOSCSI_EVT_ASYNC_MULTI_HOST          32             /**<                                                   */
#define VIRTIOSCSI_EVT_ASYNC_DEVICE_BUSY         64             /**<                                                   */
/** @} */

#define SUBSCRIBABLE_EVENTS \
    (  VIRTIOSCSI_EVT_ASYNC_OPERATIONAL_CHANGE \
     | VIRTIOSCSI_EVT_ASYNC_POWER_MGMT \
     | VIRTIOSCSI_EVT_ASYNC_EXTERNAL_REQUEST \
     | VIRTIOSCSI_EVT_ASYNC_MEDIA_CHANGE \
     | VIRTIOSCSI_EVT_ASYNC_MULTI_HOST \
     | VIRTIOSCSI_EVT_ASYNC_DEVICE_BUSY )

/**
 * Worker thread context
 */
typedef struct VIRTIOSCSIWORKER
{
    R3PTRTYPE(PPDMTHREAD)           pThread;                    /**< pointer to worker thread's handle                 */
    SUPSEMEVENT                     hEvtProcess;                /**< handle of associated sleep/wake-up semaphore      */
    bool                            fSleeping;                  /**< Flags whether worker thread is sleeping or not    */
    bool                            fNotified;                  /**< Flags whether worker thread notified              */
} VIRTIOSCSIWORKER;
typedef VIRTIOSCSIWORKER *PVIRTIOSCSIWORKER;

/**
 * State of a target attached to the VirtIO SCSI Host
 */
typedef struct VIRTIOSCSITARGET
{
    /** Pointer to PCI device that owns this target instance. - R3 pointer */
    R3PTRTYPE(struct VIRTIOSCSI *)  pVirtioScsi;

    /** Pointer to attached driver's base interface. */
    R3PTRTYPE(PPDMIBASE)            pDrvBase;

    /** Target number (PDM LUN) */
    RTUINT                          iTarget;

    /** Target Description */
    char *                          pszTargetName;

    /** Target base interface. */
    PDMIBASE                        IBase;

    /** Flag whether device is present. */
    bool                            fPresent;

    /** Media port interface. */
    PDMIMEDIAPORT                   IMediaPort;

    /** Pointer to the attached driver's media interface. */
    R3PTRTYPE(PPDMIMEDIA)           pDrvMedia;

    /** Extended media port interface. */
    PDMIMEDIAEXPORT                 IMediaExPort;

    PPDMIMEDIANOTIFY                pMediaNotify;

    /** Pointer to the attached driver's extended media interface. */
    R3PTRTYPE(PPDMIMEDIAEX)         pDrvMediaEx;

    /** Status LED interface */
    PDMILEDPORTS                    ILed;

    /** The status LED state for this device. */
    PDMLED                          led;

} VIRTIOSCSITARGET, *PVIRTIOSCSITARGET;


/** Why we're quiescing. */
typedef enum VIRTIOSCSIQUIESCINGFOR
{
    kvirtIoScsiQuiescingForInvalid = 0,
    kvirtIoScsiQuiescingForReset,
    kvirtIoScsiQuiescingForSuspend,
    kvirtIoScsiQuiescingForPowerOff,
    kvirtIoScsiQuiescingFor32BitHack = 0x7fffffff
} VIRTIOSCSIQUIESCINGFOR;


/**
 * PDM instance data (state) for VirtIO Host SCSI device
 *
 * @extends     PDMPCIDEV
 */
typedef struct VIRTIOSCSI
{
    /** The virtio state.   */
    VIRTIOSTATE                     Virtio;

    /** Number of targets detected */
    uint64_t                        cTargets;

    R3PTRTYPE(PVIRTIOSCSITARGET)    paTargetInstances;
#if HC_ARCH_BITS == 32
    RTR3PTR                         R3PtrPadding0;
#endif

    /** Per device-bound virtq worker-thread contexts (eventq slot unused) */
    VIRTIOSCSIWORKER                aWorkers[VIRTIOSCSI_QUEUE_CNT];

    bool                            fBootable;
    bool                            fRCEnabled;
    bool                            fR0Enabled;
    /** Instance name */
    char                            szInstance[16];

    /** Device-specific spec-based VirtIO queuenames */
    char                            aszQueueNames[VIRTIOSCSI_QUEUE_CNT][VIRTIO_MAX_QUEUE_NAME_SIZE];

    /** Track which VirtIO queues we've attached to */
    bool                            afQueueAttached[VIRTIOSCSI_QUEUE_CNT];

    /** Device base interface. */
    PDMIBASE                        IBase;

    /** Pointer to the device instance. - R3 ptr. */
    PPDMDEVINSR3                    pDevInsR3;

    /** Pointer to the device instance. - R0 ptr. */
    PPDMDEVINSR0                    pDevInsR0;

    /** Pointer to the device instance. - RC ptr. */
    PPDMDEVINSRC                    pDevInsRC;

    /** Status Target: LEDs port interface. */
    PDMILEDPORTS                    ILeds;

    /** Status Target: Partner of ILeds. */
    R3PTRTYPE(PPDMILEDCONNECTORS)   pLedsConnector;

    /** IMediaExPort: Media ejection notification */
    R3PTRTYPE(PPDMIMEDIANOTIFY)     pMediaNotify;

    /** Queue to send tasks to R3. - HC ptr */
    R3PTRTYPE(PPDMQUEUE)            pNotifierQueueR3;

    /** The support driver session handle. */
    R3R0PTRTYPE(PSUPDRVSESSION)     pSupDrvSession;

    /** Mask of VirtIO Async Event types this device will deliver */
    uint32_t                        fAsyncEvtsEnabled;

    /** Total number of requests active across all targets */
    volatile uint32_t               cActiveReqs;

    /** True if PDMDevHlpAsyncNotificationCompleted should be called when port goes idle */
    bool volatile                   fSignalIdle;

    /** Events the guest has subscribed to get notifications of */
    uint32_t                        fSubscribedEvents;

    /** Set if events missed due to lack of bufs avail on eventq */
    bool                            fEventsMissed;

    /** VirtIO Host SCSI device runtime configuration parameters */
    VIRTIOSCSI_CONFIG_T             virtioScsiConfig;

    /** True if the guest/driver and VirtIO framework are in the ready state */
    uint32_t                        fVirtioReady;

    /** True if VIRTIO_SCSI_F_T10_PI was negotiated */
    uint32_t                        fHasT10pi;

    /** True if VIRTIO_SCSI_F_T10_PI was negotiated */
    uint32_t                        fHasHotplug;

    /** True if VIRTIO_SCSI_F_T10_PI was negotiated */
    uint32_t                        fHasInOutBufs;

    /** True if VIRTIO_SCSI_F_T10_PI was negotiated */
    uint32_t                        fHasLunChange;

    /** True if in the process of resetting */
    uint32_t                        fResetting;

    /** True if in the process of quiescing I/O */
    uint32_t                        fQuiescing;
    /** For which purpose we're quiescing. */
    VIRTIOSCSIQUIESCINGFOR          enmQuiescingFor;

} VIRTIOSCSI, *PVIRTIOSCSI;

/**
 * Request structure for IMediaEx (Associated Interfaces implemented by DrvSCSI)
 * @note cbIn, cbOUt, cbDataOut mostly for debugging
 */
typedef struct VIRTIOSCSIREQ
{
    PDMMEDIAEXIOREQ                hIoReq;                      /**< Handle of I/O request                             */
    PVIRTIOSCSITARGET              pTarget;                     /**< Target                                            */
    uint16_t                       qIdx;                        /**< Index of queue this request arrived on            */
    PVIRTIO_DESC_CHAIN_T           pDescChain;                  /**< Prepared desc chain pulled from virtq avail ring  */
    uint32_t                       cbDataIn;                    /**< size of dataout buffer                            */
    uint32_t                       cbDataOut;                   /**< size of dataout buffer                            */
    uint16_t                       uDataInOff;                  /**< Fixed size of respHdr + sense (precede datain)    */
    uint16_t                       uDataOutOff;                 /**< Fixed size of respHdr + sense (precede datain)    */
    uint32_t                       cbSenseAlloc;                /**< Size of sense buffer                              */
    size_t                         cbSenseLen;                  /**< Receives \# bytes written into sense buffer       */
    uint8_t                       *pbSense;                     /**< Pointer to R3 sense buffer                        */
    PDMMEDIAEXIOREQSCSITXDIR       enmTxDir;                    /**< Receives transfer direction of I/O req            */
    uint8_t                        uStatus;                     /**< SCSI status code                                  */
} VIRTIOSCSIREQ;
typedef VIRTIOSCSIREQ *PVIRTIOSCSIREQ;


#ifdef LOG_ENABLED

DECLINLINE(const char *) virtioGetTxDirText(uint32_t enmTxDir)
{
    switch (enmTxDir)
    {
        case PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN:          return "<UNKNOWN>";
        case PDMMEDIAEXIOREQSCSITXDIR_FROM_DEVICE:      return "<DEV-TO-GUEST>";
        case PDMMEDIAEXIOREQSCSITXDIR_TO_DEVICE:        return "<GUEST-TO-DEV>";
        case PDMMEDIAEXIOREQSCSITXDIR_NONE:             return "<NONE>";
        default:                                        return "<BAD ENUM>";
    }
}

DECLINLINE(const char *) virtioGetTMFTypeText(uint32_t uSubType)
{
    switch (uSubType)
    {
        case VIRTIOSCSI_T_TMF_ABORT_TASK:               return "ABORT TASK";
        case VIRTIOSCSI_T_TMF_ABORT_TASK_SET:           return "ABORT TASK SET";
        case VIRTIOSCSI_T_TMF_CLEAR_ACA:                return "CLEAR ACA";
        case VIRTIOSCSI_T_TMF_CLEAR_TASK_SET:           return "CLEAR TASK SET";
        case VIRTIOSCSI_T_TMF_I_T_NEXUS_RESET:          return "I T NEXUS RESET";
        case VIRTIOSCSI_T_TMF_LOGICAL_UNIT_RESET:       return "LOGICAL UNIT RESET";
        case VIRTIOSCSI_T_TMF_QUERY_TASK:               return "QUERY TASK";
        case VIRTIOSCSI_T_TMF_QUERY_TASK_SET:           return "QUERY TASK SET";
        default:                                        return "<unknown>";
    }
}

DECLINLINE(const char *) virtioGetReqRespText(uint32_t vboxRc)
{
    switch (vboxRc)
    {
        case VIRTIOSCSI_S_OK:                           return "OK/COMPLETE";
        case VIRTIOSCSI_S_OVERRUN:                      return "OVERRRUN";
        case VIRTIOSCSI_S_ABORTED:                      return "ABORTED";
        case VIRTIOSCSI_S_BAD_TARGET:                   return "BAD TARGET";
        case VIRTIOSCSI_S_RESET:                        return "RESET";
        case VIRTIOSCSI_S_TRANSPORT_FAILURE:            return "TRANSPORT FAILURE";
        case VIRTIOSCSI_S_TARGET_FAILURE:               return "TARGET FAILURE";
        case VIRTIOSCSI_S_NEXUS_FAILURE:                return "NEXUS FAILURE";
        case VIRTIOSCSI_S_BUSY:                         return "BUSY";
        case VIRTIOSCSI_S_FAILURE:                      return "FAILURE";
        case VIRTIOSCSI_S_INCORRECT_LUN:                return "INCORRECT LUN";
        case VIRTIOSCSI_S_FUNCTION_SUCCEEDED:           return "FUNCTION SUCCEEDED";
        case VIRTIOSCSI_S_FUNCTION_REJECTED:            return "FUNCTION REJECTED";
        default:                                        return "<unknown>";
    }
}

DECLINLINE(void) virtioGetControlAsyncMaskText(char *pszOutput, uint32_t cbOutput, uint32_t fAsyncTypesMask)
{
    RTStrPrintf(pszOutput, cbOutput, "%s%s%s%s%s%s",
                fAsyncTypesMask & VIRTIOSCSI_EVT_ASYNC_OPERATIONAL_CHANGE ? "CHANGE_OPERATION  "   : "",
                fAsyncTypesMask & VIRTIOSCSI_EVT_ASYNC_POWER_MGMT         ? "POWER_MGMT  "         : "",
                fAsyncTypesMask & VIRTIOSCSI_EVT_ASYNC_EXTERNAL_REQUEST   ? "EXTERNAL_REQ  "       : "",
                fAsyncTypesMask & VIRTIOSCSI_EVT_ASYNC_MEDIA_CHANGE       ? "MEDIA_CHANGE  "       : "",
                fAsyncTypesMask & VIRTIOSCSI_EVT_ASYNC_MULTI_HOST         ? "MULTI_HOST  "         : "",
                fAsyncTypesMask & VIRTIOSCSI_EVT_ASYNC_DEVICE_BUSY        ? "DEVICE_BUSY  "        : "");
}

static uint8_t virtioScsiEstimateCdbLen(uint8_t uCmd, uint8_t cbMax)
{
    if (uCmd < 0x1f)
        return 6;
    if (uCmd >= 0x20 && uCmd < 0x60)
        return 10;
    if (uCmd >= 0x60 && uCmd < 0x80)
        return cbMax;
    if (uCmd >= 0x80 && uCmd < 0xa0)
        return 16;
    if (uCmd >= 0xa0 && uCmd < 0xc0)
        return 12;
    return cbMax;
}

#endif /* LOG_ENABLED */

static int virtioScsiR3SendEvent(PVIRTIOSCSI pThis, uint16_t uTarget, uint32_t uEventType, uint32_t uReason)
{

    VIRTIOSCSI_EVENT_T event;
    event.uEvent = uEventType;
    event.uReason = uReason;
    event.abVirtioLun[0] = 1;
    event.abVirtioLun[1] = uTarget;
    event.abVirtioLun[2] = (LUN0 >> 8) & 0x40;
    event.abVirtioLun[3] = LUN0 & 0xff;
    event.abVirtioLun[4] = event.abVirtioLun[5] = event.abVirtioLun[6] = event.abVirtioLun[7] = 0;

    /** @todo r=bird: This switch is missing some masking, right?  Because 'VIRTIOSCSI_T_NO_EVENT | VIRTIOSCSI_T_EVENTS_MISSED'
     * will never end up here but be disregarded in the 'default' case.  Given that the only caller of this function
     * is virtioScsiR3ReportEventsMissed(), I find this a bit confusing.
     *
     * For the time being I've added a VIRTIOSCSI_T_NO_EVENT | VIRTIOSCSI_T_EVENTS_MISSED case to make the code make sense,
     * but it migth not be what you had in mind.   I've also changed uEventType to fEventType since that's more appropriate. */
    switch (uEventType)
    {
        case VIRTIOSCSI_T_NO_EVENT:
            Log6Func(("(target=%d, LUN=%d): Warning event info guest queued is shorter than configured\n", uTarget, LUN0));
            break;
        case VIRTIOSCSI_T_NO_EVENT | VIRTIOSCSI_T_EVENTS_MISSED:
            Log6Func(("(target=%d, LUN=%d): Warning driver that events were missed\n", uTarget, LUN0));
            break;
        case VIRTIOSCSI_T_TRANSPORT_RESET:
            switch (uReason)
            {
                case VIRTIOSCSI_EVT_RESET_REMOVED:
                    Log6Func(("(target=%d, LUN=%d): Target or LUN removed\n", uTarget, LUN0));
                    break;
                case VIRTIOSCSI_EVT_RESET_RESCAN:
                    Log6Func(("(target=%d, LUN=%d): Target or LUN added\n", uTarget, LUN0));
                    break;
                case VIRTIOSCSI_EVT_RESET_HARD:
                    Log6Func(("(target=%d, LUN=%d): Target was reset\n", uTarget, LUN0));
                    break;
            }
            break;
        case VIRTIOSCSI_T_ASYNC_NOTIFY:
        {
#ifdef LOG_ENABLED
            char szTypeText[128];
            virtioGetControlAsyncMaskText(szTypeText, sizeof(szTypeText), uReason);
            Log6Func(("(target=%d, LUN=%d): Delivering subscribed async notification %s\n", uTarget, LUN0, szTypeText));
#endif
            break;
        }
        case VIRTIOSCSI_T_PARAM_CHANGE:
            LogFunc(("(target=%d, LUN=%d): PARAM_CHANGE sense code: 0x%x sense qualifier: 0x%x\n",
                     uTarget, LUN0, uReason & 0xff, (uReason >> 8) & 0xff));
            break;
        default:
            Log6Func(("(target=%d, LUN=%d): Unknown event type: %d, ignoring\n", uTarget, LUN0, uEventType));
            return VINF_SUCCESS;
    }

    if (virtioQueueIsEmpty(&pThis->Virtio, EVENTQ_IDX))
    {
        LogFunc(("eventq is empty, events missed (driver didn't preload queue)!\n"));
        ASMAtomicWriteBool(&pThis->fEventsMissed, true);
        return VINF_SUCCESS;
    }

    PVIRTIO_DESC_CHAIN_T pDescChain;
    virtioR3QueueGet(&pThis->Virtio, EVENTQ_IDX, &pDescChain, true);

    RTSGBUF reqSegBuf;
    RTSGSEG aReqSegs[] = { { &event, sizeof(event) } };
    RTSgBufInit(&reqSegBuf, aReqSegs, RT_ELEMENTS(aReqSegs));

    virtioR3QueuePut( &pThis->Virtio, EVENTQ_IDX, &reqSegBuf, pDescChain, true);
    virtioQueueSync(&pThis->Virtio, EVENTQ_IDX);

    return VINF_SUCCESS;
}

/** Internal worker. */
static void virtioScsiR3FreeReq(PVIRTIOSCSITARGET pTarget, PVIRTIOSCSIREQ pReq)
{
    RTMemFree(pReq->pbSense);
    pReq->pbSense = NULL;
    pTarget->pDrvMediaEx->pfnIoReqFree(pTarget->pDrvMediaEx, pReq->hIoReq);
}

/**
 * This is called to complete a request immediately
 *
 * @param   pThis       PDM driver instance state
 * @param   qIdx        Queue index
 * @param   pDescChain  Pointer to pre-processed descriptor chain pulled from virtq
 * @param   pRespHdr    Response header
 * @param   pbSense     Pointer to sense buffer or NULL if none.
 *
 * @returns VBox status code.
 */
static int virtioScsiR3ReqErr(PVIRTIOSCSI pThis, uint16_t qIdx, PVIRTIO_DESC_CHAIN_T pDescChain,
                              REQ_RESP_HDR_T *pRespHdr, uint8_t *pbSense)
{
    uint8_t *pabSenseBuf = (uint8_t *)RTMemAllocZ(pThis->virtioScsiConfig.uSenseSize);
    AssertReturn(pabSenseBuf, VERR_NO_MEMORY);

    Log2Func(("   status: %s    response: %s\n", SCSIStatusText(pRespHdr->uStatus), virtioGetReqRespText(pRespHdr->uResponse)));

    RTSGSEG aReqSegs[2];
    aReqSegs[0].cbSeg = sizeof(pRespHdr);
    aReqSegs[0].pvSeg = pRespHdr;
    aReqSegs[1].cbSeg = pThis->virtioScsiConfig.uSenseSize;
    aReqSegs[1].pvSeg = pabSenseBuf;

    if (pbSense && pRespHdr->cbSenseLen)
        memcpy(pabSenseBuf, pbSense, pRespHdr->cbSenseLen);
    else
        pRespHdr->cbSenseLen = 0;

    RTSGBUF reqSegBuf;
    RTSgBufInit(&reqSegBuf, aReqSegs, RT_ELEMENTS(aReqSegs));

    if (pThis->fResetting)
        pRespHdr->uResponse = VIRTIOSCSI_S_RESET;

    virtioR3QueuePut(&pThis->Virtio, qIdx, &reqSegBuf, pDescChain, true /* fFence */);
    virtioQueueSync(&pThis->Virtio, qIdx);

    RTMemFree(pabSenseBuf);

    if (!ASMAtomicDecU32(&pThis->cActiveReqs) && pThis->fQuiescing)
        PDMDevHlpAsyncNotificationCompleted(pThis->CTX_SUFF(pDevIns));

    Log2(("---------------------------------------------------------------------------------\n"));

    return VINF_SUCCESS;
}

static void virtioScsiR3SenseKeyToVirtioResp(REQ_RESP_HDR_T *respHdr, uint8_t uSenseKey)
{
    switch (uSenseKey)
    {
        case SCSI_SENSE_ABORTED_COMMAND:
            respHdr->uResponse = VIRTIOSCSI_S_ABORTED;
            break;
        case SCSI_SENSE_COPY_ABORTED:
            respHdr->uResponse = VIRTIOSCSI_S_ABORTED;
            break;
        case SCSI_SENSE_UNIT_ATTENTION:
            respHdr->uResponse = VIRTIOSCSI_S_TARGET_FAILURE;
            break;
        case SCSI_SENSE_HARDWARE_ERROR:
            respHdr->uResponse = VIRTIOSCSI_S_TARGET_FAILURE;
            break;
        case SCSI_SENSE_NOT_READY:
            respHdr->uResponse = VIRTIOSCSI_S_BUSY; /* e.g. re-tryable */
            break;
        default:
            respHdr->uResponse = VIRTIOSCSI_S_FAILURE;
            break;
    }
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCompleteNotify}
 */
static DECLCALLBACK(int) virtioScsiR3IoReqFinish(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                 void *pvIoReqAlloc, int rcReq)
{
    RT_NOREF(pInterface);

    PVIRTIOSCSIREQ    pReq      = (PVIRTIOSCSIREQ)pvIoReqAlloc;
    PVIRTIOSCSITARGET pTarget   = pReq->pTarget;
    PVIRTIOSCSI       pThis     = pTarget->pVirtioScsi;
    PPDMIMEDIAEX      pIMediaEx = pTarget->pDrvMediaEx;

    size_t cbResidual = 0;
    int rc = pIMediaEx->pfnIoReqQueryResidual(pIMediaEx, hIoReq, &cbResidual);
    AssertRC(rc);

    size_t cbXfer = 0;
    rc = pIMediaEx->pfnIoReqQueryXferSize(pIMediaEx, hIoReq, &cbXfer);
    AssertRC(rc);

    /* Masking used to deal with datatype size differences between APIs (Windows complains otherwise) */
    Assert(!(cbXfer & 0xffffffff00000000));
    uint32_t cbXfer32 = cbXfer & 0xffffffff;
    REQ_RESP_HDR_T respHdr = { 0 };
    respHdr.cbSenseLen = pReq->pbSense[2] == SCSI_SENSE_NONE ? 0 : (uint32_t)pReq->cbSenseLen;
    AssertMsg(!(cbResidual & 0xffffffff00000000),
            ("WARNING: Residual size larger than sizeof(uint32_t), truncating"));
    respHdr.uResidual = (uint32_t)(cbResidual & 0xffffffff);
    respHdr.uStatus   = pReq->uStatus;

    /*  VirtIO 1.0 spec 5.6.6.1.1 says device MUST return a VirtIO response byte value.
     *  Some are returned during the submit phase, and a few are not mapped at all,
     *  wherein anything that can't map specifically gets mapped to VIRTIOSCSI_S_FAILURE
     */
    if (pThis->fResetting)
        respHdr.uResponse = VIRTIOSCSI_S_RESET;
    else
    {
        switch (rcReq)
        {
            case SCSI_STATUS_OK:
            {
                if (pReq->uStatus != SCSI_STATUS_CHECK_CONDITION)
                    respHdr.uResponse = VIRTIOSCSI_S_OK;
                else
                    virtioScsiR3SenseKeyToVirtioResp(&respHdr, pReq->pbSense[2]);
                break;
            }
            case SCSI_STATUS_CHECK_CONDITION:
                virtioScsiR3SenseKeyToVirtioResp(&respHdr, pReq->pbSense[2]);
                break;

            default:
                respHdr.uResponse = VIRTIOSCSI_S_FAILURE;
                break;
        }
    }

    Log2Func(("status: (%d) %s,   response: (%d) %s\n", pReq->uStatus, SCSIStatusText(pReq->uStatus),
              respHdr.uResponse, virtioGetReqRespText(respHdr.uResponse)));

    if (RT_FAILURE(rcReq))
        Log2Func(("rcReq:  %s\n", RTErrGetDefine(rcReq)));

    if (LogIs3Enabled())
    {
        LogFunc(("cbDataIn = %u, cbDataOut = %u (cbIn = %u, cbOut = %u)\n",
                  pReq->cbDataIn, pReq->cbDataOut, pReq->pDescChain->cbPhysReturn, pReq->pDescChain->cbPhysSend));
        LogFunc(("xfer = %lu, residual = %u\n", cbXfer, cbResidual));
        LogFunc(("xfer direction: %s, sense written = %d, sense size = %d\n",
                 virtioGetTxDirText(pReq->enmTxDir), respHdr.cbSenseLen, pThis->virtioScsiConfig.uSenseSize));
    }

    if (respHdr.cbSenseLen && LogIs2Enabled())
    {
        LogFunc(("Sense: %s\n", SCSISenseText(pReq->pbSense[2])));
        LogFunc(("Sense Ext3: %s\n", SCSISenseExtText(pReq->pbSense[12], pReq->pbSense[13])));
    }

    int cSegs = 0;

    if (   (VIRTIO_IS_IN_DIRECTION(pReq->enmTxDir)  && cbXfer32 > pReq->cbDataIn)
        || (VIRTIO_IS_OUT_DIRECTION(pReq->enmTxDir) && cbXfer32 > pReq->cbDataOut))
    {
        Log2Func((" * * * * Data overrun, returning sense\n"));
        uint8_t abSense[] = { RT_BIT(7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED,
                              0, SCSI_SENSE_ILLEGAL_REQUEST, 0, 0, 0, 0, 10, 0, 0, 0 };
        respHdr.cbSenseLen = sizeof(abSense);
        respHdr.uStatus    = SCSI_STATUS_CHECK_CONDITION;
        respHdr.uResponse  = VIRTIOSCSI_S_OVERRUN;
        respHdr.uResidual  = pReq->cbDataIn;

        virtioScsiR3ReqErr(pThis, pReq->qIdx, pReq->pDescChain, &respHdr, abSense);
    }
    else
    {
        Assert(pReq->pbSense != NULL);

        /* req datain bytes already in guest phys mem. via virtioScsiIoReqCopyFromBuf() */

        RTSGSEG aReqSegs[4];
        aReqSegs[cSegs].pvSeg = &respHdr;
        aReqSegs[cSegs++].cbSeg = sizeof(respHdr);

        aReqSegs[cSegs].pvSeg = pReq->pbSense;
        aReqSegs[cSegs++].cbSeg = pReq->cbSenseAlloc; /* VirtIO 1.0 spec 5.6.4/5.6.6.1 */

        RTSGBUF reqSegBuf;
        RTSgBufInit(&reqSegBuf, aReqSegs, cSegs);

        size_t cbReqSgBuf = RTSgBufCalcTotalLength(&reqSegBuf);
        AssertMsgReturn(cbReqSgBuf <= pReq->pDescChain->cbPhysReturn,
                       ("Guest expected less req data (space needed: %d, avail: %d)\n",
                         cbReqSgBuf, pReq->pDescChain->cbPhysReturn),
                       VERR_BUFFER_OVERFLOW);


        virtioR3QueuePut(&pThis->Virtio, pReq->qIdx, &reqSegBuf, pReq->pDescChain, true /* fFence TBD */);
        virtioQueueSync(&pThis->Virtio, pReq->qIdx);


        Log2(("-----------------------------------------------------------------------------------------\n"));
    }

    virtioScsiR3FreeReq(pTarget, pReq);

    if (!ASMAtomicDecU32(&pThis->cActiveReqs) && pThis->fQuiescing)
        PDMDevHlpAsyncNotificationCompleted(pThis->CTX_SUFF(pDevIns));

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCopyFromBuf}
 *
 * Copy virtual memory from VSCSI layer to guest physical memory
 */
static DECLCALLBACK(int) virtioScsiR3IoReqCopyFromBuf(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                      void *pvIoReqAlloc, uint32_t offDst, PRTSGBUF pSgBuf, size_t cbCopy)
{
    RT_NOREF(hIoReq, cbCopy);

    PVIRTIOSCSIREQ pReq = (PVIRTIOSCSIREQ)pvIoReqAlloc;

    if (!pReq->cbDataIn)
        return VINF_SUCCESS;

    PVIRTIOSCSITARGET pTarget = RT_FROM_MEMBER(pInterface, VIRTIOSCSITARGET, IMediaExPort);
    PVIRTIOSCSI pThis = pTarget->pVirtioScsi;

    AssertReturn(pReq->pDescChain, VERR_INVALID_PARAMETER);

    PRTSGBUF pSgPhysReturn = pReq->pDescChain->pSgPhysReturn;
    RTSgBufAdvance(pSgPhysReturn, offDst);

    size_t cbCopied = 0;
    size_t cbRemain = pReq->cbDataIn;

    if (!pSgPhysReturn->idxSeg && pSgPhysReturn->cbSegLeft == pSgPhysReturn->paSegs[0].cbSeg)
        RTSgBufAdvance(pSgPhysReturn, pReq->uDataInOff);

    while (cbRemain)
    {
        PCRTSGSEG paSeg = &pSgPhysReturn->paSegs[pSgPhysReturn->idxSeg];
        uint64_t dstSgStart = (uint64_t)paSeg->pvSeg;
        uint64_t dstSgLen   = (uint64_t)paSeg->cbSeg;
        uint64_t dstSgCur   = (uint64_t)pSgPhysReturn->pvSegCur;
        cbCopied = RT_MIN((uint64_t)pSgBuf->cbSegLeft, dstSgLen - (dstSgCur - dstSgStart));
        PDMDevHlpPhysWrite(pThis->CTX_SUFF(pDevIns),
                          (RTGCPHYS)pSgPhysReturn->pvSegCur, pSgBuf->pvSegCur, cbCopied);
        RTSgBufAdvance(pSgBuf, cbCopied);
        RTSgBufAdvance(pSgPhysReturn, cbCopied);
        cbRemain -= cbCopied;
    }
    RT_UNTRUSTED_NONVOLATILE_COPY_FENCE(); /* needed? */

    Log2Func((".... Copied %lu bytes from %lu byte guest buffer, residual=%lu\n",
         cbCopy, pReq->pDescChain->cbPhysReturn, pReq->pDescChain->cbPhysReturn - cbCopy));

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqCopyToBuf}
 *
 * Copy guest physical memory to VSCSI layer virtual memory
 */
static DECLCALLBACK(int) virtioScsiR3IoReqCopyToBuf(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                    void *pvIoReqAlloc, uint32_t offSrc, PRTSGBUF pSgBuf, size_t cbCopy)
{
    RT_NOREF(hIoReq, cbCopy);

    PVIRTIOSCSIREQ pReq = (PVIRTIOSCSIREQ)pvIoReqAlloc;

    if (!pReq->cbDataOut)
        return VINF_SUCCESS;

    PVIRTIOSCSITARGET pTarget = RT_FROM_MEMBER(pInterface, VIRTIOSCSITARGET, IMediaExPort);
    PVIRTIOSCSI pThis = pTarget->pVirtioScsi;

    PRTSGBUF pSgPhysSend = pReq->pDescChain->pSgPhysSend;
    RTSgBufAdvance(pSgPhysSend, offSrc);

    size_t cbCopied = 0;
    size_t cbRemain = pReq->cbDataOut;
    while (cbRemain)
    {
        PCRTSGSEG paSeg     = &pSgPhysSend->paSegs[pSgPhysSend->idxSeg];
        uint64_t srcSgStart = (uint64_t)paSeg->pvSeg;
        uint64_t srcSgLen   = (uint64_t)paSeg->cbSeg;
        uint64_t srcSgCur   = (uint64_t)pSgPhysSend->pvSegCur;
        cbCopied = RT_MIN((uint64_t)pSgBuf->cbSegLeft, srcSgLen - (srcSgCur - srcSgStart));
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns),
                          (RTGCPHYS)pSgPhysSend->pvSegCur, pSgBuf->pvSegCur, cbCopied);
        RTSgBufAdvance(pSgBuf, cbCopied);
        RTSgBufAdvance(pSgPhysSend, cbCopied);
        cbRemain -= cbCopied;
    }

    Log2Func((".... Copied %lu bytes to %lu byte guest buffer, residual=%lu\n",
         cbCopy, pReq->pDescChain->cbPhysReturn, pReq->pDescChain->cbPhysReturn - cbCopy));

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnMediumEjected}
 */
static DECLCALLBACK(void) virtioScsiR3MediumEjected(PPDMIMEDIAEXPORT pInterface)
{
    PVIRTIOSCSITARGET pTarget = RT_FROM_MEMBER(pInterface, VIRTIOSCSITARGET, IMediaExPort);
    PVIRTIOSCSI pThis = pTarget->pVirtioScsi;
    LogFunc(("LUN %d Ejected!\n", pTarget->iTarget));
    if (pThis->pMediaNotify)
    {
        int rc = VMR3ReqCallNoWait(PDMDevHlpGetVM(pThis->CTX_SUFF(pDevIns)), VMCPUID_ANY,
                                   (PFNRT)pThis->pMediaNotify->pfnEjected, 2,
                                   pThis->pMediaNotify, pTarget->iTarget);
        AssertRC(rc);
    }
}

/**
 * @interface_method_impl{PDMIMEDIAEXPORT,pfnIoReqStateChanged}
 */
static DECLCALLBACK(void) virtioScsiR3IoReqStateChanged(PPDMIMEDIAEXPORT pInterface, PDMMEDIAEXIOREQ hIoReq,
                                                        void *pvIoReqAlloc, PDMMEDIAEXIOREQSTATE enmState)
{
    RT_NOREF4(pInterface, hIoReq, pvIoReqAlloc, enmState);
    PVIRTIOSCSI pThis = RT_FROM_MEMBER(pInterface, VIRTIOSCSI, IBase);

    switch (enmState)
    {
        case PDMMEDIAEXIOREQSTATE_SUSPENDED:
        {
            /* Stop considering this request active */
            if (!ASMAtomicDecU32(&pThis->cActiveReqs) && pThis->fQuiescing)
                PDMDevHlpAsyncNotificationCompleted(pThis->CTX_SUFF(pDevIns));
            break;
        }
        case PDMMEDIAEXIOREQSTATE_ACTIVE:
            ASMAtomicIncU32(&pThis->cActiveReqs);
            break;
        default:
            AssertMsgFailed(("Invalid request state given %u\n", enmState));
    }
}


/*********************************************************************************************************************************
*   Worker Thread                                                                                                                *
*********************************************************************************************************************************/

/**
 * Handles request queues for/on a worker thread.
 *
 * @returns VBox status code (logged by caller).
 */
static int virtioScsiR3ReqSubmit(PVIRTIOSCSI pThis, uint16_t qIdx, PVIRTIO_DESC_CHAIN_T pDescChain)
{
    AssertReturn(pDescChain->cbPhysSend, VERR_INVALID_PARAMETER);

    ASMAtomicIncU32(&pThis->cActiveReqs);

    /*
     * Extract command header and CDB from guest physical memory
     */
    size_t cbReqHdr = sizeof(REQ_CMD_HDR_T) + pThis->virtioScsiConfig.uCdbSize;
    PVIRTIOSCSI_REQ_CMD_T pVirtqReq = (PVIRTIOSCSI_REQ_CMD_T)RTMemAllocZ(cbReqHdr);
    AssertReturn(pVirtqReq, VERR_NO_MEMORY);
    uint8_t *pb = (uint8_t *)pVirtqReq;
    for (size_t cb = RT_MIN(pDescChain->cbPhysSend, cbReqHdr); cb; )
    {
        size_t cbSeg = cb;
        RTGCPHYS GCPhys = (RTGCPHYS)RTSgBufGetNextSegment(pDescChain->pSgPhysSend, &cbSeg);
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), GCPhys, pb, cbSeg);
        pb += cbSeg;
        cb -= cbSeg;
    }

    uint8_t  uTarget  = pVirtqReq->ReqHdr.abVirtioLun[1];
    uint32_t uScsiLun = (pVirtqReq->ReqHdr.abVirtioLun[2] << 8 | pVirtqReq->ReqHdr.abVirtioLun[3]) & 0x3fff;
    PVIRTIOSCSITARGET pTarget = &pThis->paTargetInstances[uTarget];

    LogFunc(("[%s] (Target: %d LUN: %d)  CDB: %.*Rhxs\n",
             SCSICmdText(pVirtqReq->uCdb[0]), uTarget, uScsiLun,
             virtioScsiEstimateCdbLen(pVirtqReq->uCdb[0], pThis->virtioScsiConfig.uCdbSize), pVirtqReq->uCdb));

    Log3Func(("cmd id: %RX64, attr: %x, prio: %d, crn: %x\n",
              pVirtqReq->ReqHdr.uId, pVirtqReq->ReqHdr.uTaskAttr, pVirtqReq->ReqHdr.uPrio, pVirtqReq->ReqHdr.uCrn));

    /*
     * Calculate request offsets
     */
    off_t uDataOutOff = sizeof(REQ_CMD_HDR_T)  + pThis->virtioScsiConfig.uCdbSize;
    off_t uDataInOff  = sizeof(REQ_RESP_HDR_T) + pThis->virtioScsiConfig.uSenseSize;
    uint32_t cbDataOut = pDescChain->cbPhysSend - uDataOutOff;
    uint32_t cbDataIn  = pDescChain->cbPhysReturn - uDataInOff;
    /*
     * Handle submission errors
     */

    if (RT_UNLIKELY(pThis->fResetting))
    {
        Log2Func(("Aborting req submission because reset is in progress\n"));
        REQ_RESP_HDR_T respHdr = { 0 };
        respHdr.cbSenseLen = 0;
        respHdr.uStatus    = SCSI_STATUS_OK;
        respHdr.uResponse  = VIRTIOSCSI_S_RESET;
        respHdr.uResidual  = cbDataIn + cbDataOut;
        virtioScsiR3ReqErr(pThis, qIdx, pDescChain, &respHdr, NULL);
        RTMemFree(pVirtqReq);
        return VINF_SUCCESS;
    }
    if (RT_UNLIKELY(uScsiLun != 0))
    {
        Log2Func(("Error submitting request to bad target (%d) or bad LUN (%d)\n", uTarget, uScsiLun));
        uint8_t abSense[] = { RT_BIT(7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED,
                              0, SCSI_SENSE_ILLEGAL_REQUEST,
                              0, 0, 0, 0, 10, SCSI_ASC_LOGICAL_UNIT_NOT_SUPPORTED, 0, 0 };
        REQ_RESP_HDR_T respHdr = { 0 };
        respHdr.cbSenseLen = sizeof(abSense);
        respHdr.uStatus    = SCSI_STATUS_CHECK_CONDITION;
        respHdr.uResponse  = VIRTIOSCSI_S_OK;
        respHdr.uResidual  = cbDataOut + cbDataIn;
        virtioScsiR3ReqErr(pThis, qIdx, pDescChain, &respHdr, abSense);
        RTMemFree(pVirtqReq);
        return VINF_SUCCESS;
    }
    if (RT_UNLIKELY(uTarget >= pThis->cTargets || !pTarget->fPresent))
    {
        Log2Func(("Error submitting request, target not present!!\n"));
        uint8_t abSense[] = { RT_BIT(7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED,
                              0, SCSI_SENSE_NOT_READY, 0, 0, 0, 0, 10, 0, 0, 0 };
        REQ_RESP_HDR_T respHdr = { 0 };
        respHdr.cbSenseLen = sizeof(abSense);
        respHdr.uStatus    = SCSI_STATUS_CHECK_CONDITION;
        respHdr.uResponse  = VIRTIOSCSI_S_BAD_TARGET;
        respHdr.uResidual  = cbDataIn + cbDataOut;
        virtioScsiR3ReqErr(pThis, qIdx, pDescChain, &respHdr , abSense);
        RTMemFree(pVirtqReq);
        return VINF_SUCCESS;
    }
    if (RT_UNLIKELY(cbDataIn && cbDataOut && !pThis->fHasInOutBufs)) /* VirtIO 1.0, 5.6.6.1.1 */
    {
        Log2Func(("Error submitting request, got datain & dataout bufs w/o INOUT feature negotated\n"));
        uint8_t abSense[] = { RT_BIT(7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED,
                              0, SCSI_SENSE_ILLEGAL_REQUEST, 0, 0, 0, 0, 10, 0, 0, 0 };
        REQ_RESP_HDR_T respHdr = { 0 };
        respHdr.cbSenseLen = sizeof(abSense);
        respHdr.uStatus    = SCSI_STATUS_CHECK_CONDITION;
        respHdr.uResponse  = VIRTIOSCSI_S_FAILURE;
        respHdr.uResidual  = cbDataIn + cbDataOut;
        virtioScsiR3ReqErr(pThis, qIdx, pDescChain, &respHdr , abSense);
        RTMemFree(pVirtqReq);
        return VINF_SUCCESS;
    }

    /*
     * Have underlying driver allocate a req of size set during initialization of this device.
     */
    PDMMEDIAEXIOREQ hIoReq = NULL;
    PVIRTIOSCSIREQ  pReq;
    PPDMIMEDIAEX    pIMediaEx = pTarget->pDrvMediaEx;

    int rc = pIMediaEx->pfnIoReqAlloc(pIMediaEx, &hIoReq, (void **)&pReq, 0 /* uIoReqId */,
                                      PDMIMEDIAEX_F_SUSPEND_ON_RECOVERABLE_ERR);

    if (RT_FAILURE(rc))
    {
        RTMemFree(pVirtqReq);
        /** @todo r=bird: This *will* crash: virtioScsiR3FreeReq(pTarget, NULL); */
        AssertMsgRCReturn(rc, ("Failed to allocate I/O request, rc=%Rrc\n", rc), rc);
    }

    pReq->hIoReq      = hIoReq;
    pReq->pTarget     = pTarget;
    pReq->qIdx        = qIdx;
    pReq->cbDataIn    = cbDataIn;
    pReq->cbDataOut   = cbDataOut;
    pReq->pDescChain  = pDescChain;
    pReq->uDataInOff  = uDataInOff;
    pReq->uDataOutOff = uDataOutOff;

    pReq->cbSenseAlloc = pThis->virtioScsiConfig.uSenseSize;
    pReq->pbSense      = (uint8_t *)RTMemAllocZ(pReq->cbSenseAlloc);
    AssertMsgReturnStmt(pReq->pbSense, ("Out of memory allocating sense buffer"),
                        virtioScsiR3FreeReq(pTarget, pReq); RTMemFree(pVirtqReq), VERR_NO_MEMORY);

    /* Note: DrvSCSI allocates one virtual memory buffer for input and output phases of the request */
    rc = pIMediaEx->pfnIoReqSendScsiCmd(pIMediaEx, pReq->hIoReq, uScsiLun,
                                        pVirtqReq->uCdb, (size_t)pThis->virtioScsiConfig.uCdbSize,
                                        PDMMEDIAEXIOREQSCSITXDIR_UNKNOWN, &pReq->enmTxDir,
                                        RT_MAX(cbDataIn, cbDataOut),
                                        pReq->pbSense, pReq->cbSenseAlloc, &pReq->cbSenseLen,
                                        &pReq->uStatus, RT_MS_30SEC);

    if (rc != VINF_PDM_MEDIAEX_IOREQ_IN_PROGRESS)
    {
        /*
         * Getting here means the request failed in early in the submission to the lower level driver,
         * and there will be no callback to the finished/completion function for this request
         */
        Assert(RT_FAILURE_NP(rc));
        Log2Func(("Request submission error from lower-level driver\n"));
        uint8_t uASC, uASCQ = 0;
        switch (rc)
        {
            case VERR_NO_MEMORY:
                uASC = SCSI_ASC_SYSTEM_RESOURCE_FAILURE;
                break;
            default:
                uASC = SCSI_ASC_INTERNAL_TARGET_FAILURE;
                break;
        }
        uint8_t abSense[] = { RT_BIT(7) | SCSI_SENSE_RESPONSE_CODE_CURR_FIXED,
                              0, SCSI_SENSE_VENDOR_SPECIFIC,
                              0, 0, 0, 0, 10, uASC, uASCQ, 0 };
        REQ_RESP_HDR_T respHdr = { 0 };
        respHdr.cbSenseLen = sizeof(abSense);
        respHdr.uStatus    = SCSI_STATUS_CHECK_CONDITION;
        respHdr.uResponse  = VIRTIOSCSI_S_FAILURE;
        respHdr.uResidual  = cbDataIn + cbDataOut;
        virtioScsiR3ReqErr(pThis, qIdx, pDescChain, &respHdr, abSense);
        virtioScsiR3FreeReq(pTarget, pReq);
    }

    RTMemFree(pVirtqReq);
    return VINF_SUCCESS;
}

/**
 * Handles control transfers for/on a worker thread.
 *
 * @returns VBox status code (ignored by the caller).
 * @param   pThis       The device instance data.
 * @param   qIdx        CONTROLQ_IDX
 * @param   pDescChain  Descriptor chain to process.
 */
static int virtioScsiR3Ctrl(PVIRTIOSCSI pThis, uint16_t qIdx, PVIRTIO_DESC_CHAIN_T pDescChain)
{
    uint8_t bResponse = VIRTIOSCSI_S_OK;

    /*
     * Allocate buffer and read in the control request/whatever.
     */
    /** @todo r=bird: The following may misbehave if the guest is not feeding you
     *        sufficient data.  There are no size checks below or with the caller
     *        that I can see, and more importantly you're using RTMemAlloc rather
     *        than RTMemAllocZ here, so you'll get random heap bytes.
     *
     *        I've changed it to RTMemAllocZ so the memory is all zeroed, but you
     *        need to consider how to deal with incorrectly sized input.
     *
     *        Ditto in virtioScsiR3ReqSubmit.
     */
    PVIRTIO_SCSI_CTRL_UNION_T pScsiCtrlUnion = (PVIRTIO_SCSI_CTRL_UNION_T)RTMemAllocZ(sizeof(VIRTIO_SCSI_CTRL_UNION_T));
    AssertPtrReturn(pScsiCtrlUnion, VERR_NO_MEMORY /*ignored*/);

    uint8_t *pb = pScsiCtrlUnion->ab;
    for (size_t cb = RT_MIN(pDescChain->cbPhysSend, sizeof(VIRTIO_SCSI_CTRL_UNION_T)); cb; )
    {
        size_t cbSeg = cb;
        /** @todo r=bird: This is ABUSING the RTSgBuf, interchanging host context
         * pointers with RTGCPHYS.  If we hadn't just dropped 32-bit host
         * support, this would have been a serious problem.  Now it is just UGLY! */
        AssertCompile(sizeof(RTGCPHYS) == sizeof(void *)); /* ASSUMING RTGCPHYS and host pointers are interchangable. (horrible!) */
        RTGCPHYS GCPhys = (RTGCPHYS)RTSgBufGetNextSegment(pDescChain->pSgPhysSend, &cbSeg);
        PDMDevHlpPhysRead(pThis->CTX_SUFF(pDevIns), GCPhys, pb, cbSeg);
        pb += cbSeg;
        cb -= cbSeg;
    }

    /*
     * Mask of events to tell guest driver this device supports
     * See VirtIO 1.0 specification section 5.6.6.2
     */
    uint32_t fSubscribedEvents  = VIRTIOSCSI_EVT_ASYNC_POWER_MGMT
                                | VIRTIOSCSI_EVT_ASYNC_EXTERNAL_REQUEST
                                | VIRTIOSCSI_EVT_ASYNC_MEDIA_CHANGE
                                | VIRTIOSCSI_EVT_ASYNC_DEVICE_BUSY;

    RTSGBUF reqSegBuf;


    switch (pScsiCtrlUnion->scsiCtrl.uType)
    {
        case VIRTIOSCSI_T_TMF: /* Task Management Functions */
        {
            uint8_t  uTarget  = pScsiCtrlUnion->scsiCtrlTmf.abScsiLun[1];
            uint32_t uScsiLun = (pScsiCtrlUnion->scsiCtrlTmf.abScsiLun[2] << 8
                               | pScsiCtrlUnion->scsiCtrlTmf.abScsiLun[3]) & 0x3fff;
            Log2Func(("[%s] (Target: %d LUN: %d)  Task Mgt Function: %s\n",
                      QUEUENAME(qIdx), uTarget, uScsiLun, virtioGetTMFTypeText(pScsiCtrlUnion->scsiCtrlTmf.uSubtype)));

            PVIRTIOSCSITARGET pTarget = NULL;
            if (uTarget < pThis->cTargets)
                pTarget = &pThis->paTargetInstances[uTarget];

            if (uTarget >= pThis->cTargets || !pTarget->fPresent)
                bResponse = VIRTIOSCSI_S_BAD_TARGET;
            else
            if (uScsiLun != 0)
                bResponse = VIRTIOSCSI_S_INCORRECT_LUN;
            else
                switch (pScsiCtrlUnion->scsiCtrlTmf.uSubtype)
                {
                    case VIRTIOSCSI_T_TMF_ABORT_TASK:
                        bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                        break;
                    case VIRTIOSCSI_T_TMF_ABORT_TASK_SET:
                        bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                        break;
                    case VIRTIOSCSI_T_TMF_CLEAR_ACA:
                        bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                        break;
                    case VIRTIOSCSI_T_TMF_CLEAR_TASK_SET:
                        bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                        break;
                    case VIRTIOSCSI_T_TMF_I_T_NEXUS_RESET:
                        bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                        break;
                    case VIRTIOSCSI_T_TMF_LOGICAL_UNIT_RESET:
                        bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                        break;
                    case VIRTIOSCSI_T_TMF_QUERY_TASK:
                        bResponse = VIRTIOSCSI_S_FUNCTION_REJECTED;
                        break;
                    case VIRTIOSCSI_T_TMF_QUERY_TASK_SET:
                        bResponse = VIRTIOSCSI_S_FUNCTION_REJECTED;
                        break;
                    default:
                        LogFunc(("Unknown TMF type\n"));
                        bResponse = VIRTIOSCSI_S_FAILURE;
                }

            RTSGSEG aReqSegs[] = { { &bResponse,  sizeof(bResponse) } };
            RTSgBufInit(&reqSegBuf, aReqSegs, RT_ELEMENTS(aReqSegs));

            break;
        }
        case VIRTIOSCSI_T_AN_QUERY: /* Guest SCSI driver is querying supported async event notifications */
        {

            PVIRTIOSCSI_CTRL_AN_T pScsiCtrlAnQuery = &pScsiCtrlUnion->scsiCtrlAsyncNotify;

            fSubscribedEvents &= pScsiCtrlAnQuery->fEventsRequested;

            uint8_t  uTarget  = pScsiCtrlAnQuery->abScsiLun[1];
            uint32_t uScsiLun = (pScsiCtrlAnQuery->abScsiLun[2] << 8 | pScsiCtrlAnQuery->abScsiLun[3]) & 0x3fff;

            PVIRTIOSCSITARGET pTarget = NULL;
            if (uTarget < pThis->cTargets)
                pTarget = &pThis->paTargetInstances[uTarget];

            if (uTarget >= pThis->cTargets || !pTarget->fPresent)
                bResponse = VIRTIOSCSI_S_BAD_TARGET;
            else
            if (uScsiLun != 0)
                bResponse = VIRTIOSCSI_S_INCORRECT_LUN;
            else
                bResponse = VIRTIOSCSI_S_FUNCTION_COMPLETE;

#ifdef LOG_ENABLED
            if (LogIs2Enabled())
            {
                char szTypeText[128];
                virtioGetControlAsyncMaskText(szTypeText, sizeof(szTypeText), pScsiCtrlAnQuery->fEventsRequested);
                Log2Func(("[%s] (Target: %d LUN: %d)  Async. Notification Query: %s\n",
                          QUEUENAME(qIdx), uTarget, uScsiLun, szTypeText));
            }
#endif
            RTSGSEG aReqSegs[] = { { &fSubscribedEvents, sizeof(fSubscribedEvents) },  { &bResponse, sizeof(bResponse)  } };
            RTSgBufInit(&reqSegBuf, aReqSegs, RT_ELEMENTS(aReqSegs));

            break;
        }
        case VIRTIOSCSI_T_AN_SUBSCRIBE: /* Guest SCSI driver is subscribing to async event notification(s) */
        {

            PVIRTIOSCSI_CTRL_AN_T pScsiCtrlAnSubscribe = &pScsiCtrlUnion->scsiCtrlAsyncNotify;

            if (pScsiCtrlAnSubscribe->fEventsRequested & ~SUBSCRIBABLE_EVENTS)
                LogFunc(("Unsupported bits in event subscription event mask: %#x\n", pScsiCtrlAnSubscribe->fEventsRequested));

            fSubscribedEvents &= pScsiCtrlAnSubscribe->fEventsRequested;
            pThis->fAsyncEvtsEnabled = fSubscribedEvents;

            uint8_t  uTarget  = pScsiCtrlAnSubscribe->abScsiLun[1];
            uint32_t uScsiLun = (pScsiCtrlAnSubscribe->abScsiLun[2] << 8
                               | pScsiCtrlAnSubscribe->abScsiLun[3]) & 0x3fff;

#ifdef LOG_ENABLED
            if (LogIs2Enabled())
            {
                char szTypeText[128];
                virtioGetControlAsyncMaskText(szTypeText, sizeof(szTypeText), pScsiCtrlAnSubscribe->fEventsRequested);
                Log2Func(("[%s] (Target: %d LUN: %d)  Async. Notification Subscribe: %s\n",
                          QUEUENAME(qIdx), uTarget, uScsiLun, szTypeText));
            }
#endif

            PVIRTIOSCSITARGET pTarget = NULL;
            if (uTarget < pThis->cTargets)
                pTarget = &pThis->paTargetInstances[uTarget];

            if (uTarget >= pThis->cTargets || !pTarget->fPresent)
                bResponse = VIRTIOSCSI_S_BAD_TARGET;
            else
            if (uScsiLun != 0)
                bResponse = VIRTIOSCSI_S_INCORRECT_LUN;
            else
            {
                /*
                 * TBD: Verify correct status code if request mask is only partially fulfillable
                 *      and confirm when to use 'complete' vs. 'succeeded' See VirtIO 1.0 spec section 5.6.6.2
                 *      and read SAM docs*/
                if (fSubscribedEvents == pScsiCtrlAnSubscribe->fEventsRequested)
                    bResponse = VIRTIOSCSI_S_FUNCTION_SUCCEEDED;
                else
                    bResponse = VIRTIOSCSI_S_FUNCTION_COMPLETE;
            }
            RTSGSEG aReqSegs[] = { { &fSubscribedEvents, sizeof(fSubscribedEvents) },  { &bResponse, sizeof(bResponse)  } };
            RTSgBufInit(&reqSegBuf, aReqSegs, RT_ELEMENTS(aReqSegs));

            break;
        }
        default:
        {
            LogFunc(("Unknown control type extracted from %s: %u\n", QUEUENAME(qIdx), pScsiCtrlUnion->scsiCtrl.uType));

            bResponse = VIRTIOSCSI_S_FAILURE;

            RTSGSEG aReqSegs[] = { { &bResponse, sizeof(bResponse) } };
            RTSgBufInit(&reqSegBuf, aReqSegs, RT_ELEMENTS(aReqSegs));
        }
    }

    LogFunc(("Response code: %s\n", virtioGetReqRespText(bResponse)));
    virtioR3QueuePut( &pThis->Virtio, qIdx, &reqSegBuf, pDescChain, true);
    virtioQueueSync(&pThis->Virtio, qIdx);

    return VINF_SUCCESS;
}

/**
 * @callback_method_impl{FNPDMTHREADWAKEUPDEV}
 */
static DECLCALLBACK(int) virtioScsiR3WorkerWakeUp(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    RT_NOREF(pThread);
    uint16_t qIdx = ((uint64_t)pThread->pvUser) & 0xffff;
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    return SUPSemEventSignal(pThis->pSupDrvSession, pThis->aWorkers[qIdx].hEvtProcess);
}

/**
 * @callback_method_impl{FNPDMTHREADDEV}
 */
static DECLCALLBACK(int) virtioScsiR3WorkerThread(PPDMDEVINS pDevIns, PPDMTHREAD pThread)
{
    uint16_t const qIdx = (uint16_t)(uintptr_t)pThread->pvUser;
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    PVIRTIOSCSIWORKER pWorker = &pThis->aWorkers[qIdx];

    if (pThread->enmState == PDMTHREADSTATE_INITIALIZING)
        return VINF_SUCCESS;

    while (pThread->enmState == PDMTHREADSTATE_RUNNING)
    {
        if (virtioQueueIsEmpty(&pThis->Virtio, qIdx))
        {
            /* Atomic interlocks avoid missing alarm while going to sleep & notifier waking the awoken */
            ASMAtomicWriteBool(&pWorker->fSleeping, true);
            bool fNotificationSent = ASMAtomicXchgBool(&pWorker->fNotified, false);
            if (!fNotificationSent)
            {
                Log6Func(("%s worker sleeping...\n", QUEUENAME(qIdx)));
                Assert(ASMAtomicReadBool(&pWorker->fSleeping));
                int rc = SUPSemEventWaitNoResume(pThis->pSupDrvSession, pWorker->hEvtProcess, RT_INDEFINITE_WAIT);
                AssertLogRelMsgReturn(RT_SUCCESS(rc) || rc == VERR_INTERRUPTED, ("%Rrc\n", rc), rc);
                if (RT_UNLIKELY(pThread->enmState != PDMTHREADSTATE_RUNNING))
                    return VINF_SUCCESS;
                Log6Func(("%s worker woken\n", QUEUENAME(qIdx)));
                ASMAtomicWriteBool(&pWorker->fNotified, false);
            }
            ASMAtomicWriteBool(&pWorker->fSleeping, false);
        }

        if (!pThis->afQueueAttached[qIdx])
        {
            LogFunc(("%s queue not attached, worker aborting...\n", QUEUENAME(qIdx)));
            break;
        }
        if (!pThis->fQuiescing)
        {
             Log6Func(("fetching next descriptor chain from %s\n", QUEUENAME(qIdx)));
             PVIRTIO_DESC_CHAIN_T pDescChain;
             int rc = virtioR3QueueGet(&pThis->Virtio, qIdx, &pDescChain, true);
             if (rc == VERR_NOT_AVAILABLE)
             {
                Log6Func(("Nothing found in %s\n", QUEUENAME(qIdx)));
                continue;
             }

             AssertRC(rc);
             if (qIdx == CONTROLQ_IDX)
                 virtioScsiR3Ctrl(pThis, qIdx, pDescChain);
             else /* request queue index */
             {
                  rc = virtioScsiR3ReqSubmit(pThis, qIdx, pDescChain);
                  if (RT_FAILURE(rc))
                  {
                     LogRel(("Error submitting req packet, resetting %Rrc", rc));
                  }
             }
        }
    }
    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Sending evnets
*********************************************************************************************************************************/

DECLINLINE(void) virtioScsiR3ReportEventsMissed(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_NO_EVENT | VIRTIOSCSI_T_EVENTS_MISSED, 0);
}

#if 0

/** Only invoke this if VIRTIOSCSI_F_HOTPLUG is negotiated during intiailization
 * This effectively removes the SCSI Target/LUN on the guest side
 */
DECLINLINE(void) virtioScsiR3ReportTargetRemoved(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fHasHotplug)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_TRANSPORT_RESET, VIRTIOSCSI_EVT_RESET_REMOVED);
}

/** Only invoke this if VIRTIOSCSI_F_HOTPLUG is negotiated during intiailization
 * This effectively adds the SCSI Target/LUN on the guest side
 */
DECLINLINE(void) virtioScsiR3ReportTargetAdded(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fHasHotplug)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_TRANSPORT_RESET, VIRTIOSCSI_EVT_RESET_RESCAN);
}

DECLINLINE(void) virtioScsiR3ReportTargetReset(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_TRANSPORT_RESET, VIRTIOSCSI_EVT_RESET_HARD);
}

DECLINLINE(void) virtioScsiR3ReportOperChange(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fSubscribedEvents & VIRTIOSCSI_EVT_ASYNC_OPERATIONAL_CHANGE)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_ASYNC_NOTIFY, VIRTIOSCSI_EVT_ASYNC_OPERATIONAL_CHANGE);
}

DECLINLINE(void) virtioScsiR3ReportPowerMsg(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fSubscribedEvents & VIRTIOSCSI_EVT_ASYNC_POWER_MGMT)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_ASYNC_NOTIFY, VIRTIOSCSI_EVT_ASYNC_POWER_MGMT);
}

DECLINLINE(void) virtioScsiR3ReportExtReq(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fSubscribedEvents & VIRTIOSCSI_EVT_ASYNC_EXTERNAL_REQUEST)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_ASYNC_NOTIFY, VIRTIOSCSI_EVT_ASYNC_EXTERNAL_REQUEST);
}

DECLINLINE(void) virtioScsiR3ReportMediaChange(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fSubscribedEvents & VIRTIOSCSI_EVT_ASYNC_MEDIA_CHANGE)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_ASYNC_NOTIFY, VIRTIOSCSI_EVT_ASYNC_MEDIA_CHANGE);
}

DECLINLINE(void) virtioScsiR3ReportMultiHost(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fSubscribedEvents & VIRTIOSCSI_EVT_ASYNC_MULTI_HOST)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_ASYNC_NOTIFY, VIRTIOSCSI_EVT_ASYNC_MULTI_HOST);
}

DECLINLINE(void) virtioScsiR3ReportDeviceBusy(PVIRTIOSCSI pThis, uint16_t uTarget)
{
    if (pThis->fSubscribedEvents & VIRTIOSCSI_EVT_ASYNC_DEVICE_BUSY)
        virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_ASYNC_NOTIFY, VIRTIOSCSI_EVT_ASYNC_DEVICE_BUSY);
}

DECLINLINE(void) virtioScsiR3ReportParamChange(PVIRTIOSCSI pThis, uint16_t uTarget, uint32_t uSenseCode, uint32_t uSenseQualifier)
{
    uint32_t uReason = uSenseQualifier << 8 | uSenseCode;
    virtioScsiR3SendEvent(pThis, uTarget, VIRTIOSCSI_T_PARAM_CHANGE, uReason);

}

#endif

/**
 * @callback_method_impl{FNVIRTIOQUEUENOTIFIED}
 */
static DECLCALLBACK(void) virtioScsiR3Notified(PVIRTIOSTATE pVirtio, uint16_t qIdx)
{
    AssertReturnVoid(qIdx < VIRTIOSCSI_QUEUE_CNT);
    PVIRTIOSCSI pThis = RT_FROM_MEMBER(pVirtio, VIRTIOSCSI, Virtio);
    PVIRTIOSCSIWORKER pWorker = &pThis->aWorkers[qIdx];

    RTLogFlush(RTLogDefaultInstanceEx(RT_MAKE_U32(0, UINT16_MAX)));

    if (qIdx == CONTROLQ_IDX || IS_REQ_QUEUE(qIdx))
    {
        Log6Func(("%s has available data\n", QUEUENAME(qIdx)));
        /* Wake queue's worker thread up if sleeping */
        if (!ASMAtomicXchgBool(&pWorker->fNotified, true))
        {
            if (ASMAtomicReadBool(&pWorker->fSleeping))
            {
                Log6Func(("waking %s worker.\n", QUEUENAME(qIdx)));
                int rc = SUPSemEventSignal(pThis->pSupDrvSession, pWorker->hEvtProcess);
                AssertRC(rc);
            }
        }
    }
    else if (qIdx == EVENTQ_IDX)
    {
        Log3Func(("Driver queued buffer(s) to %s\n", QUEUENAME(qIdx)));
        if (ASMAtomicXchgBool(&pThis->fEventsMissed, false))
            virtioScsiR3ReportEventsMissed(pThis, 0);
    }
    else
        LogFunc(("Unexpected queue idx (ignoring): %d\n", qIdx));
}

/**
 * @callback_method_impl{FNVIRTIOSTATUSCHANGED}
 */
static DECLCALLBACK(void) virtioScsiR3StatusChanged(PVIRTIOSTATE pVirtio,  uint32_t fVirtioReady)
{
    PVIRTIOSCSI pThis = RT_FROM_MEMBER(pVirtio, VIRTIOSCSI, Virtio);

    pThis->fVirtioReady = fVirtioReady;

    if (fVirtioReady)
    {
        LogFunc(("VirtIO ready\n-----------------------------------------------------------------------------------------\n"));
        uint64_t fFeatures   = virtioGetNegotiatedFeatures(&pThis->Virtio);
        pThis->fHasT10pi     = fFeatures & VIRTIO_SCSI_F_T10_PI;
        pThis->fHasHotplug   = fFeatures & VIRTIO_SCSI_F_HOTPLUG;
        pThis->fHasInOutBufs = fFeatures & VIRTIO_SCSI_F_INOUT;
        pThis->fHasLunChange = fFeatures & VIRTIO_SCSI_F_CHANGE;
        pThis->fQuiescing    = false;
        pThis->fResetting    = false;

        for (int i = 0; i < VIRTIOSCSI_QUEUE_CNT; i++)
            pThis->afQueueAttached[i] = true;
    }
    else
    {
        LogFunc(("VirtIO is resetting\n"));
        for (int i = 0; i < VIRTIOSCSI_QUEUE_CNT; i++)
            pThis->afQueueAttached[i] = false;
    }
}


/*********************************************************************************************************************************
*   LEDs                                                                                                                         *
*********************************************************************************************************************************/

#if 0 /** @todo r=bird: LEDs are not being set it seems. */

/**
 * Turns on/off the write status LED.
 *
 * @param   pTarget         Pointer to the target device
 * @param   fOn             New LED state.
 */
static void virtioScsiR3SetWriteLed(PVIRTIOSCSITARGET pTarget, bool fOn)
{
    LogFlow(("%s virtioSetWriteLed: %s\n", pTarget->pszTargetName, fOn ? "on" : "off"));
    if (fOn)
        pTarget->led.Asserted.s.fWriting = pTarget->led.Actual.s.fWriting = 1;
    else
        pTarget->led.Actual.s.fWriting = 0;
}

/**
 * Turns on/off the read status LED.
 *
 * @param   pTarget         Pointer to the device state structure.
 * @param   fOn             New LED state.
 */
static void virtioScsiR3SetReadLed(PVIRTIOSCSITARGET pTarget, bool fOn)
{
    LogFlow(("%s virtioSetReadLed: %s\n", pTarget->pszTargetName, fOn ? "on" : "off"));
    if (fOn)
        pTarget->led.Asserted.s.fReading = pTarget->led.Actual.s.fReading = 1;
    else
        pTarget->led.Actual.s.fReading = 0;
}

#endif /* unused*/

/**
 * @interface_method_impl{PDMILEDPORTS,pfnQueryStatusLed, Target level.}
 */
static DECLCALLBACK(int) virtioScsiR3TargetQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iTarget, PPDMLED *ppLed)
{
    PVIRTIOSCSITARGET pTarget = RT_FROM_MEMBER(pInterface, VIRTIOSCSITARGET, ILed);
    if (iTarget == 0)
    {
        *ppLed = &pTarget->led;
        Assert((*ppLed)->u32Magic == PDMLED_MAGIC);
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}

/**
 * @interface_method_impl{PDMILEDPORTS,pfnQueryStatusLed, Device level.}
 */
static DECLCALLBACK(int) virtioScsiR3DeviceQueryStatusLed(PPDMILEDPORTS pInterface, unsigned iTarget, PPDMLED *ppLed)
{
    PVIRTIOSCSI pThis = RT_FROM_MEMBER(pInterface, VIRTIOSCSI, ILeds);
    if (iTarget < pThis->cTargets)
    {
        *ppLed = &pThis->paTargetInstances[iTarget].led;
        Assert((*ppLed)->u32Magic == PDMLED_MAGIC);
        return VINF_SUCCESS;
    }
    return VERR_PDM_LUN_NOT_FOUND;
}


/*********************************************************************************************************************************
*   PDMIMEDIAPORT (target)                                                                                                       *
*********************************************************************************************************************************/

/**
 * @interface_method_impl{PDMIMEDIAPORT,pfnQueryDeviceLocation, Target level.}
 */
static DECLCALLBACK(int) virtioScsiR3QueryDeviceLocation(PPDMIMEDIAPORT pInterface, const char **ppcszController,
                                                         uint32_t *piInstance, uint32_t *piTarget)
{
    PVIRTIOSCSITARGET pTarget = RT_FROM_MEMBER(pInterface, VIRTIOSCSITARGET, IMediaPort);
    PPDMDEVINS pDevIns = pTarget->pVirtioScsi->CTX_SUFF(pDevIns);

    AssertPtrReturn(ppcszController, VERR_INVALID_POINTER);
    AssertPtrReturn(piInstance, VERR_INVALID_POINTER);
    AssertPtrReturn(piTarget, VERR_INVALID_POINTER);

    *ppcszController = pDevIns->pReg->szName;
    *piInstance = pDevIns->iInstance;
    *piTarget = pTarget->iTarget;

    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Virtio config.                                                                                                               *
*********************************************************************************************************************************/

/**
 * Worker for virtioScsiR3DevCapWrite and virtioScsiR3DevCapRead.
 */
static int virtioScsiR3CfgAccessed(PVIRTIOSCSI pThis, uint32_t offConfig, void *pv, uint32_t cb, bool fWrite)
{
    AssertReturn(pv && cb <= sizeof(uint32_t), fWrite ? VINF_SUCCESS : VINF_IOM_MMIO_UNUSED_00);

/**
 * Resolves to boolean true if uOffset matches a field offset and size exactly,
 * (or if 64-bit field, if it accesses either 32-bit part as a 32-bit access)
 * Assumption is this critereon is mandated by VirtIO 1.0, Section 4.1.3.1)
 * (Easily re-written to allow unaligned bounded access to a field).
 *
 * @param   member   - Member of VIRTIO_PCI_COMMON_CFG_T
 * @result           - true or false
 */
#define MATCH_SCSI_CONFIG(member) \
        (   (   RT_SIZEOFMEMB(VIRTIOSCSI_CONFIG_T, member) == 8 \
             && (   offConfig == RT_UOFFSETOF(VIRTIOSCSI_CONFIG_T, member) \
                 || offConfig == RT_UOFFSETOF(VIRTIOSCSI_CONFIG_T, member) + sizeof(uint32_t)) \
             && cb == sizeof(uint32_t)) \
         || (   offConfig == RT_UOFFSETOF(VIRTIOSCSI_CONFIG_T, member) \
             && cb == RT_SIZEOFMEMB(VIRTIOSCSI_CONFIG_T, member)) )

#define LOG_SCSI_CONFIG_ACCESSOR(member) \
        virtioLogMappedIoValue(__FUNCTION__, #member, RT_SIZEOFMEMB(VIRTIOSCSI_CONFIG_T, member), \
                               pv, cb, offIntra, fWrite, false, 0);

#define SCSI_CONFIG_ACCESSOR(member) \
    do \
    { \
        uint32_t offIntra = offConfig - RT_UOFFSETOF(VIRTIOSCSI_CONFIG_T, member); \
        if (fWrite) \
            memcpy((char *)&pThis->virtioScsiConfig.member + offIntra, pv, cb); \
        else \
            memcpy(pv, (const char *)&pThis->virtioScsiConfig.member + offIntra, cb); \
        LOG_SCSI_CONFIG_ACCESSOR(member); \
    } while(0)

#define SCSI_CONFIG_ACCESSOR_READONLY(member) \
    do \
    { \
        uint32_t offIntra = offConfig - RT_UOFFSETOF(VIRTIOSCSI_CONFIG_T, member); \
        if (fWrite) \
            LogFunc(("Guest attempted to write readonly virtio_pci_common_cfg.%s\n", #member)); \
        else \
        { \
            memcpy(pv, (const char *)&pThis->virtioScsiConfig.member + offIntra, cb); \
            LOG_SCSI_CONFIG_ACCESSOR(member); \
        } \
    } while(0)

    if (MATCH_SCSI_CONFIG(            uNumQueues))
        SCSI_CONFIG_ACCESSOR_READONLY(uNumQueues);
    else if (MATCH_SCSI_CONFIG(       uSegMax))
        SCSI_CONFIG_ACCESSOR_READONLY(uSegMax);
    else if (MATCH_SCSI_CONFIG(       uMaxSectors))
        SCSI_CONFIG_ACCESSOR_READONLY(uMaxSectors);
    else if (MATCH_SCSI_CONFIG(       uCmdPerLun))
        SCSI_CONFIG_ACCESSOR_READONLY(uCmdPerLun);
    else if (MATCH_SCSI_CONFIG(       uEventInfoSize))
        SCSI_CONFIG_ACCESSOR_READONLY(uEventInfoSize);
    else if (MATCH_SCSI_CONFIG(       uSenseSize))
        SCSI_CONFIG_ACCESSOR(         uSenseSize);
    else if (MATCH_SCSI_CONFIG(       uCdbSize))
        SCSI_CONFIG_ACCESSOR(         uCdbSize);
    else if (MATCH_SCSI_CONFIG(       uMaxChannel))
        SCSI_CONFIG_ACCESSOR_READONLY(uMaxChannel);
    else if (MATCH_SCSI_CONFIG(       uMaxTarget))
        SCSI_CONFIG_ACCESSOR_READONLY(uMaxTarget);
    else if (MATCH_SCSI_CONFIG(       uMaxLun))
        SCSI_CONFIG_ACCESSOR_READONLY(uMaxLun);
    else
    {
        LogFunc(("Bad access by guest to virtio_scsi_config: off=%u (%#x), cb=%u\n", offConfig, offConfig, cb));
        return fWrite ? VINF_SUCCESS : VINF_IOM_MMIO_UNUSED_00;
    }
    return VINF_SUCCESS;
#undef SCSI_CONFIG_ACCESSOR_READONLY
#undef SCSI_CONFIG_ACCESSOR
#undef LOG_ACCESSOR
#undef MATCH_SCSI_CONFIG
}

/**
 * @callback_method_impl{FNVIRTIODEVCAPREAD}
 */
static DECLCALLBACK(int) virtioScsiR3DevCapRead(PPDMDEVINS pDevIns, uint32_t uOffset, void *pv, uint32_t cb)
{
    return virtioScsiR3CfgAccessed(PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI), uOffset, pv, cb, false /*fRead*/);
}

/**
 * @callback_method_impl{FNVIRTIODEVCAPWRITE}
 */
static DECLCALLBACK(int) virtioScsiR3DevCapWrite(PPDMDEVINS pDevIns, uint32_t uOffset, const void *pv, uint32_t cb)
{
    return virtioScsiR3CfgAccessed(PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI), uOffset, (void *)pv, cb, true /*fWrite*/);
}


/*********************************************************************************************************************************
*   IBase for device and targets                                                                                                 *
*********************************************************************************************************************************/

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface, Target level.}
 */
static DECLCALLBACK(void *) virtioScsiR3TargetQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
     PVIRTIOSCSITARGET pTarget = RT_FROM_MEMBER(pInterface, VIRTIOSCSITARGET, IBase);
     PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE,        &pTarget->IBase);
     PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAPORT,   &pTarget->IMediaPort);
     PDMIBASE_RETURN_INTERFACE(pszIID, PDMIMEDIAEXPORT, &pTarget->IMediaExPort);
     PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS,    &pTarget->ILed);
     return NULL;
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface, Device level.}
 */
static DECLCALLBACK(void *) virtioScsiR3DeviceQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    PVIRTIOSCSI pThis = RT_FROM_MEMBER(pInterface, VIRTIOSCSI, IBase);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE,         &pThis->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMILEDPORTS,     &pThis->ILeds);

    return NULL;
}


/*********************************************************************************************************************************
*   Misc                                                                                                                         *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNDBGFHANDLERDEV, virtio-scsi debugger info callback.}
 */
static DECLCALLBACK(void) virtioScsiR3Info(PPDMDEVINS pDevIns, PCDBGFINFOHLP pHlp, const char *pszArgs)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);

    /* Parse arguments. */
    RT_NOREF(pszArgs); //bool fVerbose = pszArgs && strstr(pszArgs, "verbose") != NULL;

    /* Show basic information. */
    pHlp->pfnPrintf(pHlp, "%s#%d: virtio-scsci ",
                    pDevIns->pReg->szName,
                    pDevIns->iInstance);
    pHlp->pfnPrintf(pHlp, "numTargets=%lu", pThis->cTargets);
}


/*********************************************************************************************************************************
*   Saved state                                                                                                                  *
*********************************************************************************************************************************/

/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) virtioScsiR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    LogFunc(("LOAD EXEC!!\n"));

    AssertReturn(uPass == SSM_PASS_FINAL, VERR_SSM_UNEXPECTED_PASS);
    AssertLogRelMsgReturn(uVersion == VIRTIOSCSI_SAVED_STATE_VERSION,
                          ("uVersion=%u\n", uVersion), VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);

    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uNumQueues);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uSegMax);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uMaxSectors);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uCmdPerLun);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uEventInfoSize);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uSenseSize);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uCdbSize);
    SSMR3GetU16(pSSM, &pThis->virtioScsiConfig.uMaxChannel);
    SSMR3GetU16(pSSM, &pThis->virtioScsiConfig.uMaxTarget);
    SSMR3GetU32(pSSM, &pThis->virtioScsiConfig.uMaxLun);
    SSMR3GetU32(pSSM, &pThis->fAsyncEvtsEnabled);
    SSMR3GetU32(pSSM, (uint32_t *)&pThis->cActiveReqs);
    SSMR3GetBool(pSSM, &pThis->fEventsMissed);
    SSMR3GetU32(pSSM, &pThis->fVirtioReady);
    SSMR3GetU32(pSSM, &pThis->fHasT10pi);
    SSMR3GetU32(pSSM, &pThis->fHasHotplug);
    SSMR3GetU32(pSSM, &pThis->fHasInOutBufs);
    SSMR3GetU32(pSSM, &pThis->fHasLunChange);
    SSMR3GetU32(pSSM, &pThis->fResetting);
    int rc = SSMR3GetU32(pSSM, &pThis->fQuiescing);
    AssertRCReturn(rc, rc);

    /*
     * Call the virtio core to let it load its state.
     */
    return virtioR3LoadExec(&pThis->Virtio, pDevIns->pHlpR3, pSSM);
}

/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) virtioScsiR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    LogFunc(("SAVE EXEC!!\n"));

    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uNumQueues);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uSegMax);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uMaxSectors);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uCmdPerLun);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uEventInfoSize);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uSenseSize);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uCdbSize);
    SSMR3PutU16(pSSM, pThis->virtioScsiConfig.uMaxChannel);
    SSMR3PutU16(pSSM, pThis->virtioScsiConfig.uMaxTarget);
    SSMR3PutU32(pSSM, pThis->virtioScsiConfig.uMaxLun);
    SSMR3PutU32(pSSM, pThis->fAsyncEvtsEnabled);
    SSMR3PutU32(pSSM, (uint32_t)pThis->cActiveReqs); /** @todo r=bird: Shouldn't this be zero?  I don't think we can have
                                                      * outstanding requests when the VM is suspended (which we are when this
                                                      * function is called), and more importantely, I don't understand how they
                                                      * would be restored by virtioScsiR3LoadExec. */
    SSMR3PutBool(pSSM, pThis->fEventsMissed);
    SSMR3PutU32(pSSM, pThis->fVirtioReady);
    SSMR3PutU32(pSSM, pThis->fHasT10pi);
    SSMR3PutU32(pSSM, pThis->fHasHotplug);
    SSMR3PutU32(pSSM, pThis->fHasInOutBufs);
    SSMR3PutU32(pSSM, pThis->fHasLunChange);
    SSMR3PutU32(pSSM, pThis->fResetting);
    SSMR3PutU32(pSSM, pThis->fQuiescing); /** @todo r=bird: This shall always be false, as the VM is suspended when saving, so no need to save this */

    /*
     * Call the virtio core to let it save its state.
     */
    return virtioR3SaveExec(&pThis->Virtio, pDevIns->pHlpR3, pSSM);
}


/*********************************************************************************************************************************
*   Device interface.                                                                                                            *
*********************************************************************************************************************************/

/**
 * @interface_method_impl{PDMDEVREGR3,pfnDetach}
 *
 * One harddisk at one port has been unplugged.
 * The VM is suspended at this point.
 */
static DECLCALLBACK(void) virtioScsiR3Detach(PPDMDEVINS pDevIns, unsigned iTarget, uint32_t fFlags)
{
    PVIRTIOSCSI       pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    PVIRTIOSCSITARGET pTarget = &pThis->paTargetInstances[iTarget];

    LogFunc((""));

    AssertMsg(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
              ("virtio-scsi: Device does not support hotplugging\n"));
    RT_NOREF(fFlags);

    /*
     * Zero some important members.
     */
    pTarget->fPresent    = false;
    pTarget->pDrvBase    = NULL;
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnAttach}
 *
 * This is called when we change block driver.
 */
static DECLCALLBACK(int) virtioScsiR3Attach(PPDMDEVINS pDevIns, unsigned iTarget, uint32_t fFlags)
{
    PVIRTIOSCSI       pThis   = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    PVIRTIOSCSITARGET pTarget = &pThis->paTargetInstances[iTarget];
    int rc;

    pThis->pDevInsR3    = pDevIns;
    pThis->pDevInsR0    = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC    = PDMDEVINS_2_RCPTR(pDevIns);

    AssertMsgReturn(fFlags & PDM_TACH_FLAGS_NOT_HOT_PLUG,
                    ("virtio-scsi: Device does not support hotplugging\n"),
                    VERR_INVALID_PARAMETER);

    /* the usual paranoia */
    AssertRelease(!pTarget->pDrvBase);
    Assert(pTarget->iTarget == iTarget);

    /*
     * Try attach the SCSI driver and get the interfaces,
     * required as well as optional.
     */
    rc = PDMDevHlpDriverAttach(pDevIns, pTarget->iTarget, &pDevIns->IBase,
                               &pTarget->pDrvBase, (const char *)&pTarget->pszTargetName);
    if (RT_SUCCESS(rc))
        pTarget->fPresent = true;
    else
        AssertMsgFailed(("Failed to attach %s. rc=%Rrc\n", pTarget->pszTargetName, rc));

    if (RT_FAILURE(rc))
    {
        pTarget->fPresent = false;
        pTarget->pDrvBase = NULL;
    }
    return rc;
}

/**
 * @callback_method_impl{FNPDMDEVASYNCNOTIFY}
 */
static DECLCALLBACK(bool) virtioScsiR3DeviceQuiesced(PPDMDEVINS pDevIns)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    LogFunc(("Device I/O activity quiesced: enmQuiescingFor=%d\n", pThis->enmQuiescingFor));

    if (pThis->enmQuiescingFor == kvirtIoScsiQuiescingForReset)
        virtioR3PropagateResetNotification(&pThis->Virtio);
    /** @todo r=bird: Do we need other notifications here for suspend and/or poweroff? */

    pThis->enmQuiescingFor = kvirtIoScsiQuiescingForInvalid;
    pThis->fQuiescing = false;
    return true;
}

/**
 * Worker for virtioScsiR3Reset() and virtioScsiR3SuspendOrPowerOff().
 */
static void virtioScsiR3QuiesceDevice(PPDMDEVINS pDevIns, VIRTIOSCSIQUIESCINGFOR enmQuiscingFor)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);

    /* Prevent worker threads from removing/processing elements from virtq's */
    pThis->fQuiescing = true;
    pThis->enmQuiescingFor = enmQuiscingFor;

    PDMDevHlpSetAsyncNotification(pDevIns, virtioScsiR3DeviceQuiesced);

    /* If already quiesced invoke async callback.  */
    if (!ASMAtomicReadU32(&pThis->cActiveReqs))
        PDMDevHlpAsyncNotificationCompleted(pDevIns);
}

/**
 * Common worker for suspend and power off.
 */
static void virtioScsiR3SuspendOrPowerOff(PPDMDEVINS pDevIns, VIRTIOSCSIQUIESCINGFOR enmQuiscingFor)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);

    virtioScsiR3QuiesceDevice(pDevIns, enmQuiscingFor);


    /* VM is halted, thus no new I/O being dumped into queues by the guest.
     * Workers have been flagged to stop pulling stuff already queued-up by the guest.
     * Now tell lower-level to to suspend reqs (for example, DrvVD suspends all reqs
     * on its wait queue, and we will get a callback as the state changes to
     * suspended (and later, resumed) for each).
     */
    for (uint32_t i = 0; i < pThis->cTargets; i++)
    {
        PVIRTIOSCSITARGET pTarget = &pThis->paTargetInstances[i];
        if (pTarget->pDrvBase)
            if (pTarget->pDrvMediaEx)
                pTarget->pDrvMediaEx->pfnNotifySuspend(pTarget->pDrvMediaEx);
    }
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnPowerOff}
 */
static DECLCALLBACK(void) virtioScsiR3PowerOff(PPDMDEVINS pDevIns)
{
    LogFunc(("\n"));
    virtioScsiR3SuspendOrPowerOff(pDevIns, kvirtIoScsiQuiescingForPowerOff);
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnSuspend}
 */
static DECLCALLBACK(void) virtioScsiR3Suspend(PPDMDEVINS pDevIns)
{
    LogFunc(("\n"));
    virtioScsiR3SuspendOrPowerOff(pDevIns, kvirtIoScsiQuiescingForSuspend);
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnResume}
 */
static DECLCALLBACK(void) virtioScsiR3Resume(PPDMDEVINS pDevIns)
{
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    LogFunc(("\n"));

    pThis->fQuiescing = false;

    /* Wake worker threads flagged to skip pulling queue entries during quiesce
     * to ensure they re-check their queues. Active request queues may already
     * be awake due to new reqs coming in.
     */
    for (uint16_t qIdx = 0; qIdx < VIRTIOSCSI_REQ_QUEUE_CNT; qIdx++)
    {
        PVIRTIOSCSIWORKER pWorker = &pThis->aWorkers[qIdx];

        if (ASMAtomicReadBool(&pWorker->fSleeping))
        {
            Log6Func(("waking %s worker.\n", QUEUENAME(qIdx)));
            int rc = SUPSemEventSignal(pThis->pSupDrvSession, pWorker->hEvtProcess);
            AssertRC(rc);
        }
    }

    /* Ensure guest is working the queues too. */
    virtioR3PropagateResumeNotification(&pThis->Virtio);
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnReset}
 */
static DECLCALLBACK(void) virtioScsiR3Reset(PPDMDEVINS pDevIns)
{
    LogFunc(("\n"));
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    pThis->fResetting = true;
    virtioScsiR3QuiesceDevice(pDevIns, kvirtIoScsiQuiescingForReset);

    /** @todo r=bird: Shouldn't you reset the device here?!? */
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnDestruct}
 */
static DECLCALLBACK(int) virtioScsiR3Destruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);
    PVIRTIOSCSI pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);

    RTMemFree(pThis->paTargetInstances);
    pThis->paTargetInstances = NULL;
    for (unsigned qIdx = 0; qIdx < VIRTIOSCSI_QUEUE_CNT; qIdx++)
    {
        PVIRTIOSCSIWORKER pWorker = &pThis->aWorkers[qIdx];
        if (pWorker->hEvtProcess != NIL_SUPSEMEVENT)
        {
            SUPSemEventClose(pThis->pSupDrvSession, pWorker->hEvtProcess);
            pWorker->hEvtProcess = NIL_SUPSEMEVENT;
        }
    }

    virtioR3Term(&pThis->Virtio, pDevIns);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREGR3,pfnConstruct}
 */
static DECLCALLBACK(int) virtioScsiR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    PVIRTIOSCSI  pThis = PDMDEVINS_2_DATA(pDevIns, PVIRTIOSCSI);
    int  rc = VINF_SUCCESS;

    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);
    pThis->pSupDrvSession = PDMDevHlpGetSupDrvSession(pDevIns);

    LogFunc(("PDM device instance: %d\n", iInstance));
    RTStrPrintf((char *)pThis->szInstance, sizeof(pThis->szInstance), "VIRTIOSCSI%d", iInstance);

    /* Usable defaults */
    pThis->cTargets = 1;

    /*
     * Validate and read configuration.
     */
    if (!CFGMR3AreValuesValid(pCfg,"NumTargets\0"
                                   "Bootable\0"
                                /* "GCEnabled\0"    TBD */
                                /* "R0Enabled\0"    TBD */
    ))
    return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("virtio-scsi configuration error: unknown option specified"));

    rc = CFGMR3QueryIntegerDef(pCfg, "NumTargets", &pThis->cTargets, true);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("virtio-scsi configuration error: failed to read NumTargets as integer"));
    LogFunc(("NumTargets=%d\n", pThis->cTargets));

    rc = CFGMR3QueryBoolDef(pCfg, "Bootable", &pThis->fBootable, true);
    if (RT_FAILURE(rc))
         return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("virtio-scsi configuration error: failed to read Bootable as boolean"));
    LogFunc(("Bootable=%RTbool (unimplemented)\n", pThis->fBootable));

    rc = CFGMR3QueryBoolDef(pCfg, "R0Enabled", &pThis->fR0Enabled, false);
    if (RT_FAILURE(rc))
         return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("virtio-scsi configuration error: failed to read R0Enabled as boolean"));

    rc = CFGMR3QueryBoolDef(pCfg, "RCEnabled", &pThis->fRCEnabled, false);
    if (RT_FAILURE(rc))
         return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("virtio-scsi configuration error: failed to read RCEnabled as boolean"));

    VIRTIOPCIPARAMS virtioPciParams, *pVirtioPciParams = &virtioPciParams;
    pVirtioPciParams->uDeviceId      = PCI_DEVICE_ID_VIRTIOSCSI_HOST;
    pVirtioPciParams->uClassBase     = PCI_CLASS_BASE_MASS_STORAGE;
    pVirtioPciParams->uClassSub      = PCI_CLASS_SUB_SCSI_STORAGE_CONTROLLER;
    pVirtioPciParams->uClassProg     = PCI_CLASS_PROG_UNSPECIFIED;
    pVirtioPciParams->uSubsystemId   = PCI_DEVICE_ID_VIRTIOSCSI_HOST;  /* Virtio 1.0 spec allows PCI Device ID here */
    pVirtioPciParams->uInterruptLine = 0x00;
    pVirtioPciParams->uInterruptPin  = 0x01;

    pThis->IBase.pfnQueryInterface = virtioScsiR3DeviceQueryInterface;

    /* Configure virtio_scsi_config that transacts via VirtIO implementation's Dev. Specific Cap callbacks */
    pThis->virtioScsiConfig.uNumQueues      = VIRTIOSCSI_REQ_QUEUE_CNT;
    pThis->virtioScsiConfig.uSegMax         = VIRTIOSCSI_MAX_SEG_COUNT;
    pThis->virtioScsiConfig.uMaxSectors     = VIRTIOSCSI_MAX_SECTORS_HINT;
    pThis->virtioScsiConfig.uCmdPerLun      = VIRTIOSCSI_MAX_COMMANDS_PER_LUN;
    pThis->virtioScsiConfig.uEventInfoSize  = sizeof(VIRTIOSCSI_EVENT_T); /* Spec says at least this size! */
    pThis->virtioScsiConfig.uSenseSize      = VIRTIOSCSI_SENSE_SIZE_DEFAULT;
    pThis->virtioScsiConfig.uCdbSize        = VIRTIOSCSI_CDB_SIZE_DEFAULT;
    pThis->virtioScsiConfig.uMaxChannel     = VIRTIOSCSI_MAX_CHANNEL_HINT;
    pThis->virtioScsiConfig.uMaxTarget      = pThis->cTargets;
    pThis->virtioScsiConfig.uMaxLun         = VIRTIOSCSI_MAX_LUN;

    /* Initialize the generic Virtio core: */
    pThis->Virtio.Callbacks.pfnStatusChanged  = virtioScsiR3StatusChanged;
    pThis->Virtio.Callbacks.pfnQueueNotified  = virtioScsiR3Notified;
    pThis->Virtio.Callbacks.pfnDevCapRead     = virtioScsiR3DevCapRead;
    pThis->Virtio.Callbacks.pfnDevCapWrite    = virtioScsiR3DevCapWrite;

    rc = virtioR3Init(&pThis->Virtio, pDevIns, pVirtioPciParams, pThis->szInstance, VIRTIOSCSI_HOST_SCSI_FEATURES_OFFERED,
                      &pThis->virtioScsiConfig /*pvDevSpecificCap*/, sizeof(pThis->virtioScsiConfig));

    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("virtio-scsi: failed to initialize VirtIO"));

    /* Name the queues: */
    RTStrCopy(pThis->aszQueueNames[CONTROLQ_IDX], VIRTIO_MAX_QUEUE_NAME_SIZE, "controlq");
    RTStrCopy(pThis->aszQueueNames[EVENTQ_IDX],   VIRTIO_MAX_QUEUE_NAME_SIZE, "eventq");
    for (uint16_t qIdx = VIRTQ_REQ_BASE; qIdx < VIRTQ_REQ_BASE + VIRTIOSCSI_REQ_QUEUE_CNT; qIdx++)
        RTStrPrintf(pThis->aszQueueNames[qIdx], VIRTIO_MAX_QUEUE_NAME_SIZE,
                    "requestq<%d>", qIdx - VIRTQ_REQ_BASE);

    /* Attach the queues and create worker threads for them: */
    for (uint16_t qIdx = 0; qIdx < VIRTIOSCSI_QUEUE_CNT; qIdx++)
    {
        rc = virtioR3QueueAttach(&pThis->Virtio, qIdx, QUEUENAME(qIdx));
        pThis->afQueueAttached[qIdx] = (rc == VINF_SUCCESS); /** @todo r=bird: This looks a bit fishy, esp. giving the following. */

        if (qIdx == CONTROLQ_IDX || IS_REQ_QUEUE(qIdx))
        {
            rc = PDMDevHlpThreadCreate(pDevIns, &pThis->aWorkers[qIdx].pThread,
                                       (void *)(uintptr_t)qIdx, virtioScsiR3WorkerThread,
                                       virtioScsiR3WorkerWakeUp, 0, RTTHREADTYPE_IO, QUEUENAME(qIdx));
            if (rc != VINF_SUCCESS)
            {
                LogRel(("Error creating thread for Virtual Queue %s: %Rrc\n", QUEUENAME(qIdx), rc));
                return rc;
            }

            rc = SUPSemEventCreate(pThis->pSupDrvSession, &pThis->aWorkers[qIdx].hEvtProcess);
            if (RT_FAILURE(rc))
                return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                           N_("DevVirtioSCSI: Failed to create SUP event semaphore"));
         }
    }

    /* Initialize per device instance. */

    Log2Func(("Found %d targets attached to controller\n", pThis->cTargets));

    pThis->paTargetInstances = (PVIRTIOSCSITARGET)RTMemAllocZ(sizeof(VIRTIOSCSITARGET) * pThis->cTargets);
    if (!pThis->paTargetInstances)
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to allocate memory for target states"));

    for (RTUINT iTarget = 0; iTarget < pThis->cTargets; iTarget++)
    {
        PVIRTIOSCSITARGET pTarget = &pThis->paTargetInstances[iTarget];

        if (RTStrAPrintf(&pTarget->pszTargetName, "VSCSI%u", iTarget) < 0)
            AssertLogRelFailedReturn(VERR_NO_MEMORY);

        /* Initialize static parts of the device. */
        pTarget->iTarget = iTarget;
        pTarget->pVirtioScsi = pThis;
        pTarget->led.u32Magic = PDMLED_MAGIC;

        pTarget->IBase.pfnQueryInterface                 = virtioScsiR3TargetQueryInterface;

        /* IMediaPort and IMediaExPort interfaces provide callbacks for VD media and downstream driver access */
        pTarget->IMediaPort.pfnQueryDeviceLocation       = virtioScsiR3QueryDeviceLocation;
        pTarget->IMediaPort.pfnQueryScsiInqStrings       = NULL;
        pTarget->IMediaExPort.pfnIoReqCompleteNotify     = virtioScsiR3IoReqFinish;
        pTarget->IMediaExPort.pfnIoReqCopyFromBuf        = virtioScsiR3IoReqCopyFromBuf;
        pTarget->IMediaExPort.pfnIoReqCopyToBuf          = virtioScsiR3IoReqCopyToBuf;
        pTarget->IMediaExPort.pfnIoReqStateChanged       = virtioScsiR3IoReqStateChanged;
        pTarget->IMediaExPort.pfnMediumEjected           = virtioScsiR3MediumEjected;
        pTarget->IMediaExPort.pfnIoReqQueryBuf           = NULL; /* When used avoids copyFromBuf CopyToBuf*/
        pTarget->IMediaExPort.pfnIoReqQueryDiscardRanges = NULL;


        pTarget->IBase.pfnQueryInterface                 = virtioScsiR3TargetQueryInterface;
        pTarget->ILed.pfnQueryStatusLed                  = virtioScsiR3TargetQueryStatusLed;
        pThis->ILeds.pfnQueryStatusLed                   = virtioScsiR3DeviceQueryStatusLed;
        pTarget->led.u32Magic                            = PDMLED_MAGIC;

        LogFunc(("Attaching LUN: %s\n", pTarget->pszTargetName));

        AssertReturn(iTarget < pThis->cTargets, VERR_PDM_NO_SUCH_LUN);
        rc = PDMDevHlpDriverAttach(pDevIns, iTarget, &pTarget->IBase, &pTarget->pDrvBase, pTarget->pszTargetName);
        if (RT_SUCCESS(rc))
        {
            pTarget->fPresent = true;

            pTarget->pDrvMedia = PDMIBASE_QUERY_INTERFACE(pTarget->pDrvBase, PDMIMEDIA);
            AssertMsgReturn(VALID_PTR(pTarget->pDrvMedia),
                            ("virtio-scsi configuration error: LUN#%d missing basic media interface!\n", iTarget),
                            VERR_PDM_MISSING_INTERFACE);

            /* Get the extended media interface. */
            pTarget->pDrvMediaEx = PDMIBASE_QUERY_INTERFACE(pTarget->pDrvBase, PDMIMEDIAEX);
            AssertMsgReturn(VALID_PTR(pTarget->pDrvMediaEx),
                            ("virtio-scsi configuration error: LUN#%d missing extended media interface!\n", iTarget),
                            VERR_PDM_MISSING_INTERFACE);

            rc = pTarget->pDrvMediaEx->pfnIoReqAllocSizeSet(pTarget->pDrvMediaEx, sizeof(VIRTIOSCSIREQ));
            AssertMsgReturn(VALID_PTR(pTarget->pDrvMediaEx),
                            ("virtio-scsi configuration error: LUN#%u: Failed to set I/O request size!\n", iTarget),
                            rc);

            pTarget->pMediaNotify = PDMIBASE_QUERY_INTERFACE(pTarget->pDrvBase, PDMIMEDIANOTIFY);
            AssertMsgReturn(VALID_PTR(pTarget->pDrvMediaEx),
                            ("virtio-scsi configuration error: LUN#%u: Failed to get set Media notify obj!\n", iTarget),
                            VERR_PDM_MISSING_INTERFACE);

        }
        else if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
        {
            pTarget->fPresent = false;
            pTarget->pDrvBase = NULL;
            Log(("virtio-scsi: no driver attached to device %s\n", pTarget->pszTargetName));
            rc = VINF_SUCCESS;
        }
        else
        {
            AssertLogRelMsgFailed(("virtio-scsi: Failed to attach %s: %Rrc\n", pTarget->pszTargetName, rc));
            return rc;
        }
    }

    /* Status driver */
    PPDMIBASE pUpBase;
    rc = PDMDevHlpDriverAttach(pDevIns, PDM_STATUS_LUN, &pThis->IBase, &pUpBase, "Status Port");
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Failed to attach the status LUN"));


    /*
     * Register saved state.
     */
    rc = PDMDevHlpSSMRegister(pDevIns, VIRTIOSCSI_SAVED_STATE_VERSION, sizeof(*pThis),
                              virtioScsiR3SaveExec, virtioScsiR3LoadExec);
    AssertRCReturn(rc, rc);

    /*
     * Register the debugger info callback (ignore errors).
     */
    char szTmp[128];
    RTStrPrintf(szTmp, sizeof(szTmp), "%s%d", pDevIns->pReg->szName, pDevIns->iInstance);
    PDMDevHlpDBGFInfoRegister(pDevIns, szTmp, "virtio-scsi info", virtioScsiR3Info);

    return rc;
}

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceVirtioSCSI =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "virtio-scsi",

#ifdef VIRTIOSCSI_GC_SUPPORT
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RZ
                                    | PDM_DEVREG_FLAGS_FIRST_SUSPEND_NOTIFICATION
                                    | PDM_DEVREG_FLAGS_FIRST_POWEROFF_NOTIFICATION,
#else
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS
                                    | PDM_DEVREG_FLAGS_FIRST_SUSPEND_NOTIFICATION
                                    | PDM_DEVREG_FLAGS_FIRST_POWEROFF_NOTIFICATION,
#endif
    /* .fClass = */                 PDM_DEVREG_CLASS_MISC,
    /* .cMaxInstances = */          ~0U,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(VIRTIOSCSI),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         1,
    /* .cMaxMsixVectors = */        VBOX_MSIX_MAX_ENTRIES,
    /* .pszDescription = */         "Virtio Host SCSI.\n",
#if defined(IN_RING3)
    /* .pszRCMod = */               "VBoxDDRC.rc",
    /* .pszR0Mod = */               "VBoxDDR0.r0",
    /* .pfnConstruct = */           virtioScsiR3Construct,
    /* .pfnDestruct = */            virtioScsiR3Destruct,
    /* .pfnRelocate = */            NULL,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               virtioScsiR3Reset,
    /* .pfnSuspend = */             virtioScsiR3Suspend,
    /* .pfnResume = */              virtioScsiR3Resume,
    /* .pfnAttach = */              virtioScsiR3Attach,
    /* .pfnDetach = */              virtioScsiR3Detach,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
    /* .pfnPowerOff = */            virtioScsiR3PowerOff,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           NULL,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};


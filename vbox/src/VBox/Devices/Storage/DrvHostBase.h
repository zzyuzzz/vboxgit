/* $Id: DrvHostBase.h 64316 2016-10-19 11:59:42Z vboxsync $ */
/** @file
 * DrvHostBase - Host base drive access driver.
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

#ifndef __HostDrvBase_h__
#define __HostDrvBase_h__

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/critsect.h>
#include <iprt/log.h>
#include <iprt/semaphore.h>
#include <VBox/cdefs.h>
#include <VBox/vmm/pdmdrv.h>
#include <VBox/vmm/pdmstorageifs.h>

RT_C_DECLS_BEGIN


/** Pointer to host base drive access driver instance data. */
typedef struct DRVHOSTBASE *PDRVHOSTBASE;
/**
 * Host base drive access driver instance data.
 *
 * @implements PDMIMOUNT
 * @implements PDMIMEDIA
 */
typedef struct DRVHOSTBASE
{
    /** Critical section used to serialize access to the handle and other
     * members of this struct. */
    RTCRITSECT              CritSect;
    /** Pointer driver instance. */
    PPDMDRVINS              pDrvIns;
    /** Drive type. */
    PDMMEDIATYPE            enmType;
    /** Visible to the BIOS. */
    bool                    fBiosVisible;
    /** The configuration readonly value. */
    bool                    fReadOnlyConfig;
    /** The current readonly status. */
    bool                    fReadOnly;
    /** Flag whether failure to attach is an error or not. */
    bool                    fAttachFailError;
    /** Flag whether to keep instance working (as unmounted though). */
    bool                    fKeepInstance;
    /** Device name (MMHeap). */
    char                   *pszDevice;
    /** Device name to open (RTStrFree). */
    char                   *pszDeviceOpen;
    /** Uuid of the drive. */
    RTUUID                  Uuid;

    /** Pointer to the block port interface above us. */
    PPDMIMEDIAPORT          pDrvMediaPort;
    /** Pointer to the mount notify interface above us. */
    PPDMIMOUNTNOTIFY        pDrvMountNotify;
    /** Our media interface. */
    PDMIMEDIA               IMedia;
    /** Our mountable interface. */
    PDMIMOUNT               IMount;

    /** Media present indicator. */
    bool volatile           fMediaPresent;
    /** Locked indicator. */
    bool                    fLocked;
    /** The size of the media currently in the drive.
     * This is invalid if no drive is in the drive. */
    uint64_t volatile       cbSize;

    /** Handle of the poller thread. */
    RTTHREAD                ThreadPoller;
    /** Event semaphore the thread will wait on. */
    RTSEMEVENT              EventPoller;
    /** The poller interval. */
    RTMSINTERVAL            cMilliesPoller;
    /** The shutdown indicator. */
    bool volatile           fShutdownPoller;

    /** BIOS PCHS geometry. */
    PDMMEDIAGEOMETRY        PCHSGeometry;
    /** BIOS LCHS geometry. */
    PDMMEDIAGEOMETRY        LCHSGeometry;

    /**
     * Performs the locking / unlocking of the device.
     *
     * This callback pointer should be set to NULL if the device doesn't support this action.
     *
     * @returns VBox status code.
     * @param   pThis       Pointer to the instance data.
     * @param   fLock       Set if locking, clear if unlocking.
     */
    DECLCALLBACKMEMBER(int, pfnDoLock)(PDRVHOSTBASE pThis, bool fLock);

    union
    {
#ifdef DRVHOSTBASE_OS_INT_DECLARED
        DRVHOSTBASEOS       Os;
#endif
        uint8_t             abPadding[64];
    };
} DRVHOSTBASE;


DECLHIDDEN(int) DRVHostBaseInit(PPDMDRVINS pDrvIns, PCFGMNODE pCfg, const char *pszCfgValid, PDMMEDIATYPE enmType);
DECLHIDDEN(int) DRVHostBaseMediaPresent(PDRVHOSTBASE pThis);
DECLHIDDEN(void) DRVHostBaseMediaNotPresent(PDRVHOSTBASE pThis);
DECLCALLBACK(void) DRVHostBaseDestruct(PPDMDRVINS pDrvIns);

DECLHIDDEN(int) drvHostBaseScsiCmdOs(PDRVHOSTBASE pThis, const uint8_t *pbCmd, size_t cbCmd, PDMMEDIATXDIR enmTxDir,
                                     void *pvBuf, uint32_t *pcbBuf, uint8_t *pbSense, size_t cbSense, uint32_t cTimeoutMillies);
DECLHIDDEN(int) drvHostBaseGetMediaSizeOs(PDRVHOSTBASE pThis, uint64_t *pcb);
DECLHIDDEN(int) drvHostBaseReadOs(PDRVHOSTBASE pThis, uint64_t off, void *pvBuf, size_t cbRead);
DECLHIDDEN(int) drvHostBaseWriteOs(PDRVHOSTBASE pThis, uint64_t off, const void *pvBuf, size_t cbWrite);
DECLHIDDEN(int) drvHostBaseFlushOs(PDRVHOSTBASE pThis);
DECLHIDDEN(int) drvHostBaseDoLockOs(PDRVHOSTBASE pThis, bool fLock);
DECLHIDDEN(int) drvHostBaseEjectOs(PDRVHOSTBASE pThis);

DECLHIDDEN(void) drvHostBaseInitOs(PDRVHOSTBASE pThis);
DECLHIDDEN(int) drvHostBaseOpenOs(PDRVHOSTBASE pThis, bool fReadOnly);
DECLHIDDEN(int) drvHostBaseMediaRefreshOs(PDRVHOSTBASE pThis);
DECLHIDDEN(int) drvHostBaseQueryMediaStatusOs(PDRVHOSTBASE pThis, bool *pfMediaChanged, bool *pfMediaPresent);
DECLHIDDEN(bool) drvHostBaseIsMediaPollingRequiredOs(PDRVHOSTBASE pThis);
DECLHIDDEN(void) drvHostBaseDestructOs(PDRVHOSTBASE pThis);

RT_C_DECLS_END

#endif

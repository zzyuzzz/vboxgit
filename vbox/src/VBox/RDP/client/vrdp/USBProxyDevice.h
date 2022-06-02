/* $Id: USBProxyDevice.h 31247 2010-07-30 13:37:49Z vboxsync $ */
/** @file
 * USBPROXY - USB proxy header
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
 *
 * Oracle Corporation confidential
 * All rights reserved
 */

#ifndef ___USBProxyDevice_h
#define ___USBProxyDevice_h

#ifndef RDESKTOP
# include <VBox/cdefs.h>
# include <VBox/vusb.h>
#else
# include "runtime.h"
# include "vusb.h"
#endif

RT_C_DECLS_BEGIN


/**
 * Arguments passed to the USB proxy device constructor.
 */
typedef struct USBPROXYDEVARGS
{
    /** Whether this is a remote (VRDP) or local (Host) device. */
    bool        fRemote;
    /** Host specific USB device address. */
    const char *pszAddress;
    /** Pointer to backend specific data. */
    void       *pvBackend;
} USBPROXYDEVARGS;
/** Pointer to proxy device creation structure. */
typedef USBPROXYDEVARGS *PUSBPROXYDEVARGS;


/** Pointer to a USB proxy device. */
typedef struct USBPROXYDEV *PUSBPROXYDEV;

/**
 * USB Proxy Device Backend
 */
typedef struct USBPROXYBACK
{
    /** Name of the backend. */
    const char *pszName;

    /**
     * Opens the USB device specfied by pszAddress.
     *
     * This method will initialize backend private data. If the backend has
     * already selected a configuration for the device, this must be indicated
     * in USBPROXYDEV::iActiveCfg.
     *
     * @returns VBox status code.
     * @param   pProxyDev   The USB Proxy Device instance.
     * @param   pszAddress  Host specific USB device address.
     * @param   pvBackend   Pointer to backend specific data.
     */
    int  (* pfnOpen)(PUSBPROXYDEV pProxyDev, const char *pszAddress, void *pvBackend);

    /**
     * Optional callback for initalizing the device after the configuration
     * has been established.
     *
     * @returns VBox status code.
     * @param   pProxyDev   The USB Proxy Device instance.
     */
    int  (* pfnInit)(PUSBPROXYDEV pProxyDev);

    /**         Closes handle to the host USB device.
     *
     * @param   pDev        The USB Proxy Device instance.
     */
    void (* pfnClose)(PUSBPROXYDEV pProxyDev);

    /**
     * Reset a device.
     *
     * The backend must update iActualCfg and fIgnoreEqualSetConfig.
     *
     * @returns VBox status code.
     * @param   pDev            The device to reset.
     * @param   fResetOnLinux   It's safe to do reset on linux, we can deal with devices
     *                          being logically reconnected.
     */
    int  (* pfnReset)(PUSBPROXYDEV pProxyDev, bool fResetOnLinux);

    /** @todo make it return a VBox status code! */
    int  (* pfnSetConfig)(PUSBPROXYDEV pProxyDev, int iCfg);

    /** @todo make it return a VBox status code! */
    int  (* pfnClaimInterface)(PUSBPROXYDEV pProxyDev, int iIf);

    /** @todo make it return a VBox status code! */
    int  (* pfnReleaseInterface)(PUSBPROXYDEV pProxyDev, int iIf);

    /** @todo make it return a VBox status code! */
    int  (* pfnSetInterface)(PUSBPROXYDEV pProxyDev, int iIf, int setting);

    /** @todo make it return a VBox status code! */
    bool (* pfnClearHaltedEndpoint)(PUSBPROXYDEV  pDev, unsigned int iEp);

    /** @todo make it return a VBox status code! Add pDev. */
    int  (* pfnUrbQueue)(PVUSBURB pUrb);

    /**
     * Cancel an in-flight URB.
     *
     * @param   pUrb        The URB to cancel.
     * @todo make it return a VBox status code! Add pDev.
     */
    void (* pfnUrbCancel)(PVUSBURB pUrb);

    /**
     * Reap URBs in-flight on a device.
     *
     * @returns Pointer to a completed URB.
     * @returns NULL if no URB was completed.
     * @param   pDev        The device.
     * @param   cMillies    Number of milliseconds to wait. Use 0 to not
     *                      wait at all.
     */
    PVUSBURB (* pfnUrbReap)(PUSBPROXYDEV pProxyDev, RTMSINTERVAL cMillies);

    /** Dummy entry for making sure we've got all members initialized. */
    uint32_t uDummy;
} USBPROXYBACK;
/** Pointer to a USB Proxy Device Backend. */
typedef USBPROXYBACK *PUSBPROXYBACK;
/** Pointer to a const USB Proxy Device Backend. */
typedef const USBPROXYBACK *PCUSBPROXYBACK;

/** The Host backend. */
extern const USBPROXYBACK g_USBProxyDeviceHost;
#ifdef VBOX_WITH_VRDP
/** The VRDP backend. */
extern const USBPROXYBACK g_USBProxyDeviceVRDP;
#endif /* VBOX_WITH_VRDP */

#ifdef RDESKTOP
typedef struct VUSBDEV
{
    char* pszName;
} VUSBDEV, *PVUSBDEV;
#endif

/**
 * USB Proxy device.
 */
typedef struct USBPROXYDEV
{
#ifdef RDESKTOP
    /** The VUSB device structure - must be the first structure member. */
    VUSBDEV             Dev;
    /** The next device in rdesktop-vrdp's linked list */
    PUSBPROXYDEV        pNext;
    /** The previous device in rdesktop-vrdp's linked list */
    PUSBPROXYDEV        pPrev;
    /** The vrdp device ID */
    uint32_t devid;
    /** Linked list of in-flight URBs */
    PVUSBURB            pUrbs;
#endif
    /** The device descriptor. */
    VUSBDESCDEVICE      DevDesc;
    /** The configuration descriptor array. */
    PVUSBDESCCONFIGEX   paCfgDescs;
#ifndef RDESKTOP
    /** The descriptor cache.
     * Contains &DevDesc and paConfigDescs. */
    PDMUSBDESCCACHE     DescCache;
    /** Pointer to the PDM USB device instance. */
    PPDMUSBINS          pUsbIns;
#endif

    /** Pointer to the backend. */
    PCUSBPROXYBACK      pOps;
    /** The currently active configration.
     * It's -1 if no configuration is active. This is set to -1 before open and reset,
     * the backend will change it if open or reset implies SET_CONFIGURATION. */
    int                 iActiveCfg;
    /** Ignore one or two SET_CONFIGURATION operation.
     * See usbProxyDevSetCfg for details. */
    int                 cIgnoreSetConfigs;
    /** Mask of the interfaces that the guest shall doesn't see.
     * This is experimental!
     */
    uint32_t            fMaskedIfs;
    /** Whether we've opened the device or not.
     * For dealing with failed construction (the destruct method is always called). */
    bool                fOpened;
    /** Whether we've called pfnInit or not.
     * For dealing with failed construction (the destruct method is always called). */
    bool                fInited;
    /** Whether the device has been detached.
     * This is hack for making PDMUSBREG::pfnUsbQueue return the right status code. */
    bool                fDetached;
    /** Backend specific data */
    union USBPROXYBACKENDDATA
    {
        /** Pointer to some backend data.
         * The Linux and Darwin backends are making use of this. */
        void *pv;
        RTFILE File;
        int fd;
#ifdef VBOX_WITH_VRDP
        struct vrdp_priv
        {
            void *pCallback;
            void *pDevice;
        } vrdp;
#endif /* VBOX_WITH_VRDP */
    } Backend;
} USBPROXYDEV;

static inline char *usbProxyGetName(PUSBPROXYDEV pProxyDev)
{
#ifndef RDESKTOP
    return pProxyDev->pUsbIns->pszName;
#else
    return pProxyDev->Dev.pszName;
#endif
}

#ifdef RDESKTOP
static inline PUSBPROXYDEV usbProxyFromVusbDev(PVUSBDEV pDev)
{
    return (PUSBPROXYDEV)pDev;
}
#endif

#ifdef RT_OS_LINUX
RTDECL(int) USBProxyDeviceLinuxGetFD(PUSBPROXYDEV pProxyDev);
#endif

RT_C_DECLS_END

#endif


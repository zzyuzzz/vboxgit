/* $Id: VBoxUSBMon-solaris.c 31898 2010-08-24 09:28:43Z vboxsync $ */
/** @file
 * VirtualBox USB Monitor Driver, Solaris Hosts.
 */

/*
 * Copyright (C) 2008 Oracle Corporation
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
#define LOG_GROUP  LOG_GROUP_USB_DRV
#include "VBoxUSBFilterMgr.h"
#include <VBox/usblib-solaris.h>
#include <VBox/version.h>
#include <VBox/log.h>
#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/version.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/initterm.h>
#include <iprt/process.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
# include <iprt/path.h>
#endif

#define USBDRV_MAJOR_VER    2
#define USBDRV_MINOR_VER    0
#include <sys/usb/usba.h>
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/mutex.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>
#include <sys/open.h>
#include <sys/cmn_err.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** The module name. */
#define DEVICE_NAME              "vboxusbmon"
/** The module description as seen in 'modinfo'. */
#define DEVICE_DESC_DRV          "VirtualBox USBMon"

#if defined(DEBUG_ramshankar)
# undef LogFlowFunc
# define LogFlowFunc    LogRel
# undef Log
# define Log            LogRel
# undef LogFlow
# define LogFlow        LogRel
#endif

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int VBoxUSBMonSolarisOpen(dev_t *pDev, int fFlag, int fType, cred_t *pCred);
static int VBoxUSBMonSolarisClose(dev_t Dev, int fFlag, int fType, cred_t *pCred);
static int VBoxUSBMonSolarisRead(dev_t Dev, struct uio *pUio, cred_t *pCred);
static int VBoxUSBMonSolarisWrite(dev_t Dev, struct uio *pUio, cred_t *pCred);
static int VBoxUSBMonSolarisIOCtl(dev_t Dev, int Cmd, intptr_t pArg, int Mode, cred_t *pCred, int *pVal);
static int VBoxUSBMonSolarisGetInfo(dev_info_t *pDip, ddi_info_cmd_t enmCmd, void *pArg, void **ppResult);
static int VBoxUSBMonSolarisAttach(dev_info_t *pDip, ddi_attach_cmd_t enmCmd);
static int VBoxUSBMonSolarisDetach(dev_info_t *pDip, ddi_detach_cmd_t enmCmd);

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * cb_ops: for drivers that support char/block entry points
 */
static struct cb_ops g_VBoxUSBMonSolarisCbOps =
{
    VBoxUSBMonSolarisOpen,
    VBoxUSBMonSolarisClose,
    nodev,                      /* b strategy */
    nodev,                      /* b dump */
    nodev,                      /* b print */
    VBoxUSBMonSolarisRead,
    VBoxUSBMonSolarisWrite,
    VBoxUSBMonSolarisIOCtl,
    nodev,                      /* c devmap */
    nodev,                      /* c mmap */
    nodev,                      /* c segmap */
    nochpoll,                   /* c poll */
    ddi_prop_op,                /* property ops */
    NULL,                       /* streamtab  */
    D_NEW | D_MP,               /* compat. flag */
    CB_REV                      /* revision */
};

/**
 * dev_ops: for driver device operations
 */
static struct dev_ops g_VBoxUSBMonSolarisDevOps =
{
    DEVO_REV,                   /* driver build revision */
    0,                          /* ref count */
    VBoxUSBMonSolarisGetInfo,
    nulldev,                    /* identify */
    nulldev,                    /* probe */
    VBoxUSBMonSolarisAttach,
    VBoxUSBMonSolarisDetach,
    nodev,                      /* reset */
    &g_VBoxUSBMonSolarisCbOps,
    (struct bus_ops *)0,
    nodev                       /* power */
};

/**
 * modldrv: export driver specifics to the kernel
 */
static struct modldrv g_VBoxUSBMonSolarisModule =
{
    &mod_driverops,             /* extern from kernel */
    DEVICE_DESC_DRV " " VBOX_VERSION_STRING "r" RT_XSTR(VBOX_SVN_REV),
    &g_VBoxUSBMonSolarisDevOps
};

/**
 * modlinkage: export install/remove/info to the kernel
 */
static struct modlinkage g_VBoxUSBMonSolarisModLinkage =
{
    MODREV_1,
    &g_VBoxUSBMonSolarisModule,
    NULL,
};

#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
/**
 * Client driver info.
 */
typedef struct vboxusbmon_client_t
{
    dev_info_t                 *pDip;                       /* Client device info. pointer */
    VBOXUSB_CLIENT_INFO         Info;                       /* Client registeration data. */
    struct vboxusbmon_client_t *pNext;                      /* Pointer to next client */
} vboxusbmon_client_t;
#endif

/**
 * Device state info.
 */
typedef struct
{
    RTPROCESS                   Process;                    /* The process (id) of the session */
} vboxusbmon_state_t;


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Global Device handle we only support one instance. */
static dev_info_t *g_pDip;
/** Global Mutex. */
static kmutex_t g_VBoxUSBMonSolarisMtx;

#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
/** Number of userland clients that have kept us open. */
static uint64_t g_cVBoxUSBMonSolarisClient = 0;
/** Global list of client drivers registered with us. */
vboxusbmon_client_t *g_pVBoxUSBMonSolarisClients = 0;
#endif

/** Opaque pointer to list of soft states. */
static void *g_pVBoxUSBMonSolarisState;

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int vboxUSBMonSolarisProcessIOCtl(int iFunction, void *pvState, void *pvData, size_t cbData, size_t *pcbReturnedData);
static int vboxUSBMonSolarisResetDevice(char *pszDevicePath, bool fReattach);
#ifndef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
static int vboxUSBMonSolarisGetDeviceInstance(char *pszDevicePath, int32_t *pInstance);
#endif


/*******************************************************************************
*   Monitor Global Hooks                                                       *
*******************************************************************************/
#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
static int vboxUSBMonSolarisClientInfo(vboxusbmon_state_t *pState, PVBOXUSB_CLIENT_INFO pClientInfo);
int VBoxUSBMonSolarisRegisterClient(dev_info_t *pClientDip, PVBOXUSB_CLIENT_INFO pClientInfo);
int VBoxUSBMonSolarisUnregisterClient(dev_info_t *pClientDip);
int VBoxUSBMonSolarisElectDriver(usb_dev_descr_t *pDevDesc, usb_dev_str_t *pDevStrings, char *pszDevicePath, int Bus, int Port,
                                char **ppszDrv, void *pvReserved);
#endif


/**
 * Kernel entry points
 */
int _init(void)
{
    int rc;

    LogFlowFunc((DEVICE_NAME ":_init\n"));

    g_pDip = NULL;

    /*
     * Prevent module autounloading.
     */
    modctl_t *pModCtl = mod_getctl(&g_VBoxUSBMonSolarisModLinkage);
    if (pModCtl)
        pModCtl->mod_loadflags |= MOD_NOAUTOUNLOAD;
    else
        LogRel((DEVICE_NAME ":failed to disable autounloading!\n"));

    /*
     * Initialize IPRT R0 driver, which internally calls OS-specific r0 init.
     */
    rc = RTR0Init(0);
    if (RT_SUCCESS(rc))
    {
        /*
         * Initialize global mutex.
         */
        mutex_init(&g_VBoxUSBMonSolarisMtx, NULL, MUTEX_DRIVER, NULL);
        rc = ddi_soft_state_init(&g_pVBoxUSBMonSolarisState, sizeof(vboxusbmon_state_t), 1);
        if (!rc)
        {
            rc = mod_install(&g_VBoxUSBMonSolarisModLinkage);
            if (!rc)
                return rc;

            LogRel((DEVICE_NAME ":mod_install failed! rc=%d\n", rc));
            ddi_soft_state_fini(&g_pVBoxUSBMonSolarisState);
        }
        else
            LogRel((DEVICE_NAME ":ddi_soft_state_init failed! rc=%d\n", rc));

        mutex_destroy(&g_VBoxUSBMonSolarisMtx);
        RTR0Term();
    }
    else
        LogRel((DEVICE_NAME ":RTR0Init failed! rc=%d\n", rc));

    return -1;
}


int _fini(void)
{
    int rc;

    LogFlowFunc((DEVICE_NAME ":_fini\n"));

    rc = mod_remove(&g_VBoxUSBMonSolarisModLinkage);
    if (!rc)
    {
        ddi_soft_state_fini(&g_pVBoxUSBMonSolarisState);
        mutex_destroy(&g_VBoxUSBMonSolarisMtx);

        RTR0Term();
    }
    return rc;
}


int _info(struct modinfo *pModInfo)
{
    LogFlowFunc((DEVICE_NAME ":_info\n"));

    return mod_info(&g_VBoxUSBMonSolarisModLinkage, pModInfo);
}


/**
 * Attach entry point, to attach a device to the system or resume it.
 *
 * @param   pDip            The module structure instance.
 * @param   enmCmd          Attach type (ddi_attach_cmd_t)
 *
 * @returns corresponding solaris error code.
 */
static int VBoxUSBMonSolarisAttach(dev_info_t *pDip, ddi_attach_cmd_t enmCmd)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisAttach pDip=%p enmCmd=%d\n", pDip, enmCmd));
    switch (enmCmd)
    {
        case DDI_ATTACH:
        {
            vboxusbmon_state_t *pState = NULL;
            int instance = ddi_get_instance(pDip);
            int rc;

            pState = RTMemAllocZ(sizeof(*pState));
            if (pState)
            {
                g_pDip = pDip;
                rc = ddi_create_priv_minor_node(pDip, DEVICE_NAME, S_IFCHR, instance, DDI_PSEUDO, 0,
                                                            "none", "none", 0666);
                if (rc == DDI_SUCCESS)
                {
                    ddi_set_driver_private(pDip, pState);
                    ddi_report_dev(pDip);
                    return rc;
                }
                else
                    LogRel((DEVICE_NAME ":ddi_create_minor_node failed! rc=%d\n", rc));
                RTMemFree(pState);
            }
            else
                LogRel((DEVICE_NAME ":RTMemAllocZ failed to allocated %d bytes for pState\n", sizeof(*pState)));
            return DDI_FAILURE;
        }

        case DDI_RESUME:
        {
            /* We don't have to bother about power management. */
            return DDI_SUCCESS;
        }

        default:
            return DDI_FAILURE;
    }
}


/**
 * Detach entry point, to detach a device to the system or suspend it.
 *
 * @param   pDip            The module structure instance.
 * @param   enmCmd          Attach type (ddi_attach_cmd_t)
 *
 * @returns corresponding solaris error code.
 */
static int VBoxUSBMonSolarisDetach(dev_info_t *pDip, ddi_detach_cmd_t enmCmd)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisDetach\n"));

    switch (enmCmd)
    {
        case DDI_DETACH:
        {
#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
            /*
             * Free all registered clients' info.
             */
            mutex_enter(&g_VBoxUSBMonSolarisMtx);
            vboxusbmon_client_t *pCur = g_pVBoxUSBMonSolarisClients;
            while (pCur)
            {
                vboxusbmon_client_t *pNext = pCur->pNext;
                RTMemFree(pCur);
                pCur = pNext;
            }
            mutex_exit(&g_VBoxUSBMonSolarisMtx);
#endif

            vboxusbmon_state_t *pState = ddi_get_driver_private(g_pDip);
            if (pState)
            {
                ddi_remove_minor_node(pDip, NULL);
                RTMemFree(pState);
                return DDI_SUCCESS;
            }
            else
                LogRel((DEVICE_NAME ":failed to get soft state on detach.\n"));
            break;
        }

        case DDI_SUSPEND:
        {
            /* We don't have to bother about power management. */
            return DDI_SUCCESS;
        }

        default:
            return DDI_FAILURE;
    }
}


/**
 * Info entry point, called by solaris kernel for obtaining driver info.
 *
 * @param   pDip            The module structure instance (do not use).
 * @param   enmCmd          Information request type.
 * @param   pvArg           Type specific argument.
 * @param   ppvResult       Where to store the requested info.
 *
 * @returns corresponding solaris error code.
 */
static int VBoxUSBMonSolarisGetInfo(dev_info_t *pDip, ddi_info_cmd_t enmCmd, void *pvArg, void **ppvResult)
{
    int rc = DDI_SUCCESS;

    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisGetInfo\n"));

    switch (enmCmd)
    {
        case DDI_INFO_DEVT2DEVINFO:
            *ppvResult = (void *)g_pDip;
            break;

        case DDI_INFO_DEVT2INSTANCE:
            *ppvResult = (void *)(uintptr_t)ddi_get_instance(g_pDip);
            break;

        default:
            rc = DDI_FAILURE;
            break;
    }
    return rc;
}


static int VBoxUSBMonSolarisOpen(dev_t *pDev, int fFlag, int fType, cred_t *pCred)
{
    vboxusbmon_state_t *pState = NULL;
    unsigned iOpenInstance;

    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisOpen\n"));

    /*
     * Verify we are being opened as a character device
     */
    if (fType != OTYP_CHR)
        return EINVAL;

    /*
     * Verify that we're called after attach
     */
    if (!g_pDip)
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisOpen invalid state for opening.\n"));
        return ENXIO;
    }

#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
    mutex_enter(&g_VBoxUSBMonSolarisMtx);
    if (!g_cVBoxUSBMonSolarisClient)
    {
        mutex_exit(&g_VBoxUSBMonSolarisMtx);
        int rc = usb_register_dev_driver(g_pDip, VBoxUSBMonSolarisElectDriver);
        if (RT_UNLIKELY(rc != DDI_SUCCESS))
        {
            LogRel((DEVICE_NAME ":Failed to register driver election callback with USBA rc=%d\n", rc));
            return EINVAL;
        }
        LogFlow((DEVICE_NAME ":Successfully registered election callback with USBA\n"));
        mutex_enter(&g_VBoxUSBMonSolarisMtx);
    }
    g_cVBoxUSBMonSolarisClient++;
    mutex_exit(&g_VBoxUSBMonSolarisMtx);
#endif

    for (iOpenInstance = 0; iOpenInstance < 4096; iOpenInstance++)
    {
        if (    !ddi_get_soft_state(g_pVBoxUSBMonSolarisState, iOpenInstance) /* faster */
            &&  ddi_soft_state_zalloc(g_pVBoxUSBMonSolarisState, iOpenInstance) == DDI_SUCCESS)
        {
            pState = ddi_get_soft_state(g_pVBoxUSBMonSolarisState, iOpenInstance);
            break;
        }
    }
    if (!pState)
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisOpen: too many open instances."));
#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
        mutex_enter(&g_VBoxUSBMonSolarisMtx);
        g_cVBoxUSBMonSolarisClient--;
        mutex_exit(&g_VBoxUSBMonSolarisMtx);
#endif
        return ENXIO;
    }

    pState->Process = RTProcSelf();
    *pDev = makedevice(getmajor(*pDev), iOpenInstance);

    NOREF(fFlag);
    NOREF(pCred);

    return 0;
}


static int VBoxUSBMonSolarisClose(dev_t Dev, int fFlag, int fType, cred_t *pCred)
{
    vboxusbmon_state_t *pState = NULL;

    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisClose\n"));

    pState = ddi_get_soft_state(g_pVBoxUSBMonSolarisState, getminor(Dev));
    if (!pState)
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisClose: failed to get pState.\n"));
        return EFAULT;
    }

#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
    mutex_enter(&g_VBoxUSBMonSolarisMtx);
    g_cVBoxUSBMonSolarisClient--;
    if (!g_cVBoxUSBMonSolarisClient)
    {
        if (RT_LIKELY(g_pDip))
        {
            mutex_exit(&g_VBoxUSBMonSolarisMtx);
            usb_unregister_dev_driver(g_pDip);
            LogFlow((DEVICE_NAME ":Successfully deregistered driver election callback\n"));
        }
        else
        {
            mutex_exit(&g_VBoxUSBMonSolarisMtx);
            LogRel((DEVICE_NAME ":Extreme error! Missing device info during close.\n"));
        }
    }
    else
        mutex_exit(&g_VBoxUSBMonSolarisMtx);
#endif

    /*
     * Remove all filters for this client process.
     */
    VBoxUSBFilterRemoveOwner(pState->Process);

    ddi_soft_state_free(g_pVBoxUSBMonSolarisState, getminor(Dev));
    pState = NULL;

    NOREF(fFlag);
    NOREF(fType);
    NOREF(pCred);

    return 0;
}


static int VBoxUSBMonSolarisRead(dev_t Dev, struct uio *pUio, cred_t *pCred)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisRead\n"));
    return 0;
}


static int VBoxUSBMonSolarisWrite(dev_t Dev, struct uio *pUio, cred_t *pCred)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisWrite\n"));
    return 0;
}


/** @def IOCPARM_LEN
 * Gets the length from the ioctl number.
 * This is normally defined by sys/ioccom.h on BSD systems...
 */
#ifndef IOCPARM_LEN
# define IOCPARM_LEN(Code)                      (((Code) >> 16) & IOCPARM_MASK)
#endif

static int VBoxUSBMonSolarisIOCtl(dev_t Dev, int Cmd, intptr_t pArg, int Mode, cred_t *pCred, int *pVal)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl Dev=%d Cmd=%d pArg=%p Mode=%d\n", Dev, Cmd, pArg));

    /*
     * Get the session from the soft state item.
     */
    vboxusbmon_state_t *pState = ddi_get_soft_state(g_pVBoxUSBMonSolarisState, getminor(Dev));
    if (!pState)
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: no state data for %d\n", getminor(Dev)));
        return EINVAL;
    }

    /*
     * Read the request wrapper. Though We don't really need wrapper struct. now
     * it's room for the future as Solaris isn't generous regarding the size.
     */
    VBOXUSBREQ ReqWrap;
    if (IOCPARM_LEN(Cmd) != sizeof(ReqWrap))
    {
        LogRel((DEVICE_NAME ": VBoxUSBMonSolarisIOCtl: bad request %#x size=%d expected=%d\n", Cmd, IOCPARM_LEN(Cmd), sizeof(ReqWrap)));
        return ENOTTY;
    }

    int rc = ddi_copyin((void *)pArg, &ReqWrap, sizeof(ReqWrap), Mode);
    if (RT_UNLIKELY(rc))
    {
        LogRel((DEVICE_NAME ": VBoxUSBMonSolarisIOCtl: ddi_copyin failed to read header pArg=%p Cmd=%d. rc=%d.\n", pArg, Cmd, rc));
        return EINVAL;
    }

    if (ReqWrap.u32Magic != VBOXUSBMON_MAGIC)
    {
        LogRel((DEVICE_NAME ": VBoxUSBMonSolarisIOCtl: bad magic %#x; pArg=%p Cmd=%d.\n", ReqWrap.u32Magic, pArg, Cmd));
        return EINVAL;
    }
    if (RT_UNLIKELY(   ReqWrap.cbData == 0
                    || ReqWrap.cbData > _1M*16))
    {
        LogRel((DEVICE_NAME ": VBoxUSBMonSolarisIOCtl: bad size %#x; pArg=%p Cmd=%d.\n", ReqWrap.cbData, pArg, Cmd));
        return EINVAL;
    }

    /*
     * Read the request.
     */
    void *pvBuf = RTMemTmpAlloc(ReqWrap.cbData);
    if (RT_UNLIKELY(!pvBuf))
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: RTMemTmpAlloc failed to alloc %d bytes.\n", ReqWrap.cbData));
        return ENOMEM;
    }

    rc = ddi_copyin((void *)(uintptr_t)ReqWrap.pvDataR3, pvBuf, ReqWrap.cbData, Mode);
    if (RT_UNLIKELY(rc))
    {
        RTMemTmpFree(pvBuf);
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: ddi_copyin failed; pvBuf=%p pArg=%p Cmd=%d. rc=%d\n", pvBuf, pArg, Cmd, rc));
        return EFAULT;
    }
    if (RT_UNLIKELY(   ReqWrap.cbData != 0
                    && !VALID_PTR(pvBuf)))
    {
        RTMemTmpFree(pvBuf);
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: pvBuf invalid pointer %p\n", pvBuf));
        return EINVAL;
    }
    LogFlow((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: pid=%d.\n", (int)RTProcSelf()));

    /*
     * Process the IOCtl.
     */
    size_t cbDataReturned;
    rc = vboxUSBMonSolarisProcessIOCtl(Cmd, pState, pvBuf, ReqWrap.cbData, &cbDataReturned);
    ReqWrap.rc = rc;
    rc = 0;

    if (RT_UNLIKELY(cbDataReturned > ReqWrap.cbData))
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: too much output data %d expected %d\n", cbDataReturned, ReqWrap.cbData));
        cbDataReturned = ReqWrap.cbData;
    }

    ReqWrap.cbData = cbDataReturned;

    /*
     * Copy the request back to user space.
     */
    rc = ddi_copyout(&ReqWrap, (void *)pArg, sizeof(ReqWrap), Mode);
    if (RT_LIKELY(!rc))
    {
        /*
         * Copy the payload (if any) back to user space.
         */
        if (cbDataReturned > 0)
        {
            rc = ddi_copyout(pvBuf, (void *)(uintptr_t)ReqWrap.pvDataR3, cbDataReturned, Mode);
            if (RT_UNLIKELY(rc))
            {
                LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: ddi_copyout failed; pvBuf=%p pArg=%p Cmd=%d. rc=%d\n", pvBuf, pArg, Cmd, rc));
                rc = EFAULT;
            }
        }
    }
    else
    {
        LogRel((DEVICE_NAME ":VBoxUSBMonSolarisIOCtl: ddi_copyout(1) failed pArg=%p Cmd=%d\n", pArg, Cmd));
        rc = EFAULT;
    }

    *pVal = rc;
    RTMemTmpFree(pvBuf);
    return rc;
}


/**
 * IOCtl processor for user to kernel and kernel to kernel communcation.
 *
 * @returns  VBox status code.
 *
 * @param   iFunction           The requested function.
 * @param   pvState             Opaque pointer to driver state used for getting ring-3 process (Id).
 * @param   pvData              The input/output data buffer. Can be NULL depending on the function.
 * @param   cbData              The max size of the data buffer.
 * @param   pcbDataReturned     Where to store the amount of returned data. Can be NULL.
 */
static int vboxUSBMonSolarisProcessIOCtl(int iFunction, void *pvState, void *pvData, size_t cbData, size_t *pcbReturnedData)
{
    LogFlowFunc((DEVICE_NAME ":solarisUSBProcessIOCtl iFunction=%d pvBuf=%p cbBuf=%zu\n", iFunction, pvData, cbData));

    AssertPtrReturn(pvState, VERR_INVALID_POINTER);
    vboxusbmon_state_t *pState = (vboxusbmon_state_t *)pvState;
    int rc;

#define CHECKRET_MIN_SIZE(mnemonic, cbMin) \
    do { \
        if (cbData < (cbMin)) \
        { \
            LogRel(("vboxUSBSolarisProcessIOCtl: " mnemonic ": cbData=%#zx (%zu) min is %#zx (%zu)\n", \
                 cbData, cbData, (size_t)(cbMin), (size_t)(cbMin))); \
            return VERR_BUFFER_OVERFLOW; \
        } \
        if ((cbMin) != 0 && !VALID_PTR(pvData)) \
        { \
            LogRel(("vboxUSBSolarisProcessIOCtl: " mnemonic ": Invalid pointer %p\n", pvData)); \
            return VERR_INVALID_POINTER; \
        } \
    } while (0)

    switch (iFunction)
    {
        case VBOXUSBMON_IOCTL_ADD_FILTER:
        {
            CHECKRET_MIN_SIZE("ADD_FILTER", sizeof(VBOXUSBREQ_ADD_FILTER));

            VBOXUSBREQ_ADD_FILTER *pReq = (VBOXUSBREQ_ADD_FILTER *)pvData;
            PUSBFILTER pFilter = (PUSBFILTER)&pReq->Filter;

            LogFlow(("vboxUSBMonSolarisProcessIOCtl: idVendor=%#x idProduct=%#x bcdDevice=%#x bDeviceClass=%#x bDeviceSubClass=%#x bDeviceProtocol=%#x bBus=%#x bPort=%#x\n",
                      USBFilterGetNum(pFilter, USBFILTERIDX_VENDOR_ID),
                      USBFilterGetNum(pFilter, USBFILTERIDX_PRODUCT_ID),
                      USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_REV),
                      USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_CLASS),
                      USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_SUB_CLASS),
                      USBFilterGetNum(pFilter, USBFILTERIDX_DEVICE_PROTOCOL),
                      USBFilterGetNum(pFilter, USBFILTERIDX_BUS),
                      USBFilterGetNum(pFilter, USBFILTERIDX_PORT)));
            LogFlow(("vboxUSBMonSolarisProcessIOCtl: Manufacturer=%s Product=%s Serial=%s\n",
                      USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  ? USBFilterGetString(pFilter, USBFILTERIDX_MANUFACTURER_STR)  : "<null>",
                      USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       ? USBFilterGetString(pFilter, USBFILTERIDX_PRODUCT_STR)       : "<null>",
                      USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) ? USBFilterGetString(pFilter, USBFILTERIDX_SERIAL_NUMBER_STR) : "<null>"));

            rc = USBFilterSetMustBePresent(pFilter, USBFILTERIDX_BUS, false /* fMustBePresent */);      AssertRC(rc);

            rc = VBoxUSBFilterAdd(pFilter, pState->Process, &pReq->uId);
            *pcbReturnedData = cbData;
            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: ADD_FILTER (Process:%d) returned %d\n", pState->Process, rc));
            break;
        }

        case VBOXUSBMON_IOCTL_REMOVE_FILTER:
        {
            CHECKRET_MIN_SIZE("REMOVE_FILTER", sizeof(VBOXUSBREQ_REMOVE_FILTER));

            VBOXUSBREQ_REMOVE_FILTER *pReq = (VBOXUSBREQ_REMOVE_FILTER *)pvData;
            rc = VBoxUSBFilterRemove(pState->Process, (uintptr_t)pReq->uId);
            *pcbReturnedData = 0;
            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: REMOVE_FILTER (Process:%d) returned %d\n", pState->Process, rc));
            break;
        }

        case VBOXUSBMON_IOCTL_RESET_DEVICE:
        {
            CHECKRET_MIN_SIZE("RESET_DEVICE", sizeof(VBOXUSBREQ_RESET_DEVICE));

            VBOXUSBREQ_RESET_DEVICE *pReq = (VBOXUSBREQ_RESET_DEVICE *)pvData;
            rc = vboxUSBMonSolarisResetDevice(pReq->szDevicePath, pReq->fReattach);
            *pcbReturnedData = 0;
            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: RESET_DEVICE (Process:%d) returned %d\n", pState->Process, rc));
            break;
        }

#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
        case VBOXUSBMON_IOCTL_CLIENT_INFO:
        {
            CHECKRET_MIN_SIZE("CLIENT_INFO", sizeof(VBOXUSBREQ_CLIENT_INFO));

            VBOXUSBREQ_CLIENT_INFO *pReq = (VBOXUSBREQ_CLIENT_INFO *)pvData;
            rc = vboxUSBMonSolarisClientInfo(pState, pReq);
            *pcbReturnedData = cbData;
            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: CLIENT_INFO (Process:%d) returned %d\n", pState->Process, rc));
            break;
        }
#else
        case VBOXUSBMON_IOCTL_DEVICE_INSTANCE:
        {
            CHECKRET_MIN_SIZE("DEVICE_INSTANCE", sizeof(VBOXUSBREQ_DEVICE_INSTANCE));

            VBOXUSBREQ_DEVICE_INSTANCE *pReq = (VBOXUSBREQ_DEVICE_INSTANCE *)pvData;
            rc = vboxUSBMonSolarisGetDeviceInstance(pReq->szDevicePath, (int32_t *)pReq->pInstance);
            *pcbReturnedData = cbData;
            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: DEVICE_INSTANCE returned %d\n", rc));
            break;
        }
#endif

        case VBOXUSBMON_IOCTL_GET_VERSION:
        {
            CHECKRET_MIN_SIZE("GET_VERSION", sizeof(VBOXUSBREQ_GET_VERSION));

            PVBOXUSBREQ_GET_VERSION pGetVersionReq = (PVBOXUSBREQ_GET_VERSION)pvData;
            pGetVersionReq->u32Major = VBOXUSBMON_VERSION_MAJOR;
            pGetVersionReq->u32Minor = VBOXUSBMON_VERSION_MINOR;
            *pcbReturnedData = sizeof(VBOXUSBREQ_GET_VERSION);
            rc = VINF_SUCCESS;
            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: GET_VERSION returned %d\n", rc));
            break;
        }

        default:
        {
            LogRel((DEVICE_NAME ":vboxUSBMonSolarisProcessIOCtl: Unknown request (Process:%d) %#x\n", pState->Process, iFunction));
            rc = VERR_NOT_SUPPORTED;
            break;
        }
    }
    return rc;
}


static int vboxUSBMonSolarisResetDevice(char *pszDevicePath, bool fReattach)
{
    int rc = VERR_GENERAL_FAILURE;

    LogFlowFunc((DEVICE_NAME ":vboxUSBMonSolarisResetDevice pszDevicePath=%s fReattach=%d\n", pszDevicePath, fReattach));

#ifdef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS
    /*
     * Try grabbing the dev_info_t.
     */
    dev_info_t *pDeviceInfo = e_ddi_hold_devi_by_path(pszDevicePath, 0);
    if (pDeviceInfo)
    {
        ddi_release_devi(pDeviceInfo);

        /*
         * Grab the root device node from the parent hub for resetting.
         */
        dev_info_t *pTmpDeviceInfo = NULL;
        for (;;)
        {
            pTmpDeviceInfo = ddi_get_parent(pDeviceInfo);
            if (!pTmpDeviceInfo)
            {
                LogRel((DEVICE_NAME ":vboxUSBMonSolarisResetDevice failed to get parent device info for %s\n", pszDevicePath));
                return VERR_GENERAL_FAILURE;
            }

            if (ddi_prop_exists(DDI_DEV_T_ANY, pTmpDeviceInfo, DDI_PROP_DONTPASS, "usb-port-count"))   /* parent hub */
                break;

            pDeviceInfo = pTmpDeviceInfo;
        }

        /*
         * Try re-enumerating the device.
         */
        rc = usb_reset_device(pDeviceInfo, fReattach ? USB_RESET_LVL_REATTACH : USB_RESET_LVL_DEFAULT);
        LogFlow((DEVICE_NAME ":usb_reset_device for %s level=%s returned %d\n", pszDevicePath, fReattach ? "ReAttach" : "Default", rc));
        switch (rc)
        {
            case USB_SUCCESS:         rc = VINF_SUCCESS;                break;
            case USB_INVALID_PERM:    rc = VERR_PERMISSION_DENIED;      break;
            case USB_INVALID_ARGS:    rc = VERR_INVALID_PARAMETER;      break;

            /* @todo find better codes for these (especially USB_BUSY) */
            case USB_BUSY:
            case USB_INVALID_CONTEXT:
            case USB_FAILURE:         rc = VERR_GENERAL_FAILURE;        break;

            default:                  rc = VERR_UNRESOLVED_ERROR;       break;
        }
    }
    else
    {
        rc = VERR_INVALID_HANDLE;
        LogRel((DEVICE_NAME ":vboxUSBMonSolarisResetDevice Cannot obtain device info for %s\n", pszDevicePath));
    }

#else
    /*
     * Try grabbing the dev_info_t.
     */
    dev_info_t *pDeviceInfo = e_ddi_hold_devi_by_path(pszDevicePath, 0);
    if (pDeviceInfo)
    {
        /*
         * Try re-enumerating the device.
         */
        rc = usb_reset_device(pDeviceInfo, fReattach ? USB_RESET_LVL_REATTACH : USB_RESET_LVL_DEFAULT);
        LogFlow((DEVICE_NAME ":usb_reset_device for %s level=%s returned %d\n", pszDevicePath, fReattach ? "ReAttach" : "Default", rc));

        ddi_release_devi(pDeviceInfo);
        switch (rc)
        {
            case USB_SUCCESS:         rc = VINF_SUCCESS;                break;
            case USB_INVALID_PERM:    rc = VERR_PERMISSION_DENIED;      break;
            case USB_INVALID_ARGS:    rc = VERR_INVALID_PARAMETER;      break;

            /* @todo find better codes for these (especially USB_BUSY) */
            case USB_BUSY:
            case USB_INVALID_CONTEXT:
            case USB_FAILURE:         rc = VERR_GENERAL_FAILURE;        break;

            default:                  rc = VERR_UNRESOLVED_ERROR;       break;
        }
    }
    else
    {
        rc = VERR_INVALID_HANDLE;
        LogRel(("vboxUSBMonSolarisResetDevice: Cannot obtain dev_info_t for Device %s\n", pszDevicePath));
    }
#endif

    return rc;
}

#ifndef VBOX_WITH_NEW_USB_CODE_ON_SOLARIS

static int vboxUSBMonSolarisGetDeviceInstance(char *pszDevicePath, int32_t *pInstance)
{
    LogFlowFunc((DEVICE_NAME ":vboxUSBMonSolarisGetDeviceInstance pszDevicePath=%s pInstance=%p\n", pszDevicePath, pInstance));

    AssertPtrReturn(pInstance, VERR_INVALID_POINTER);

    /*
     * Try grabbing the dev_info_t.
     */
    dev_info_t *pDeviceInfo = e_ddi_hold_devi_by_path(pszDevicePath, 0);
    if (pDeviceInfo)
    {
        /*
         * Get device instance.
         */
        *pInstance = ddi_get_instance(pDeviceInfo);
        ddi_release_devi(pDeviceInfo);
        LogFlow(("vboxUSBMonSolarisGetDeviceInstance: instance %d\n", *pInstance));
        return VINF_SUCCESS;
    }
    else
        LogRel(("vboxUSBMonSolarisGetDeviceInstance: Cannot obtain dev_info_t for Device %s\n", pszDevicePath));
    return VERR_INVALID_HANDLE;
}

#else /* VBOX_WITH_NEW_USB_CODE_ON_SOLARIS */

static int vboxUSBMonSolarisClientInfo(vboxusbmon_state_t *pState, PVBOXUSB_CLIENT_INFO pClientInfo)
{
    LogFlowFunc((DEVICE_NAME ":vboxUSBMonSolarisClientInfo pState=%p pClientInfo=%p\n", pState, pClientInfo));

    AssertPtrReturn(pState, VERR_INVALID_POINTER);
    AssertPtrReturn(pClientInfo, VERR_INVALID_POINTER);

    mutex_enter(&g_VBoxUSBMonSolarisMtx);
    vboxusbmon_client_t *pCur = g_pVBoxUSBMonSolarisClients;
    vboxusbmon_client_t *pPrev = NULL;
    while (pCur)
    {
        if (strncmp(pClientInfo->achDeviceIdent, pCur->Info.achDeviceIdent, sizeof(pCur->Info.achDeviceIdent) - 1) == 0)
        {
            pClientInfo->Instance = pCur->Info.Instance;
            RTStrPrintf(pClientInfo->achClientPath, sizeof(pClientInfo->achClientPath), "%s", pCur->Info.achClientPath);

            mutex_exit(&g_VBoxUSBMonSolarisMtx);

            LogFlow((DEVICE_NAME ":vboxUSBMonSolarisClientInfo found. %s\n", pClientInfo->achDeviceIdent));
            return VINF_SUCCESS;
        }
        pPrev = pCur;
        pCur = pCur->pNext;
    }

    mutex_exit(&g_VBoxUSBMonSolarisMtx);

    LogRel((DEVICE_NAME ":vboxUSBMonSolarisClientInfo Failed to find client %s\n", pClientInfo->achDeviceIdent));
    return VERR_NOT_FOUND;
}


/**
 * Registers client driver.
 *
 * @returns VBox status code.
 * @param   pszDevicePath       The device path of the client driver.
 * @param   Instance            The client driver instance.
 */
int VBoxUSBMonSolarisRegisterClient(dev_info_t *pClientDip, PVBOXUSB_CLIENT_INFO pClientInfo)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisRegisterClient pClientDip=%p pClientInfo=%p\n", pClientDip, pClientInfo));
    AssertPtrReturn(pClientInfo, VERR_INVALID_PARAMETER);

    if (RT_LIKELY(g_pDip))
    {
        vboxusbmon_state_t *pState = ddi_get_driver_private(g_pDip);

        if (RT_LIKELY(pState))
        {
            vboxusbmon_client_t *pClient = RTMemAlloc(sizeof(vboxusbmon_client_t));
            if (RT_LIKELY(pClient))
            {
                bcopy(pClientInfo, &pClient->Info, sizeof(pClient->Info));
                pClient->pDip = pClientDip;

                mutex_enter(&g_VBoxUSBMonSolarisMtx);
                pClient->pNext = g_pVBoxUSBMonSolarisClients;
                g_pVBoxUSBMonSolarisClients = pClient;
                mutex_exit(&g_VBoxUSBMonSolarisMtx);

                LogFlow((DEVICE_NAME ":VBoxUSBMonSolarisRegisterClient registered. %d %s %s\n",
                            pClient->Info.Instance, pClient->Info.achClientPath, pClient->Info.achDeviceIdent));

                return VINF_SUCCESS;
            }
            else
                return VERR_NO_MEMORY;
        }
        else
            return VERR_INTERNAL_ERROR;
    }
    else
        return VERR_INTERNAL_ERROR_2;
}


/**
 * Deregisters client driver.
 *
 * @returns VBox status code.
 * @param   pszDevicePath       The device path of the client driver.
 * @param   Instance            The client driver instance.
 */
int VBoxUSBMonSolarisUnregisterClient(dev_info_t *pClientDip)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisUnregisterClient pClientDip=%p\n", pClientDip));

    if (RT_LIKELY(g_pDip))
    {
        vboxusbmon_state_t *pState = ddi_get_driver_private(g_pDip);

        if (RT_LIKELY(pState))
        {
            mutex_enter(&g_VBoxUSBMonSolarisMtx);

            vboxusbmon_client_t *pCur = g_pVBoxUSBMonSolarisClients;
            vboxusbmon_client_t *pPrev = NULL;
            while (pCur)
            {
                if (pCur->pDip == pClientDip)
                {
                    if (pPrev)
                        pPrev->pNext = pCur->pNext;
                    else
                        g_pVBoxUSBMonSolarisClients = pCur->pNext;

                    mutex_exit(&g_VBoxUSBMonSolarisMtx);

                    LogFlow((DEVICE_NAME ":VBoxUSBMonSolarisUnregisterClient unregistered. %d %s %s\n",
                                pCur->Info.Instance, pCur->Info.achClientPath, pCur->Info.achDeviceIdent));
                    RTMemFree(pCur);
                    pCur = NULL;
                    return VINF_SUCCESS;
                }
                pPrev = pCur;
                pCur = pCur->pNext;
            }

            mutex_exit(&g_VBoxUSBMonSolarisMtx);

            LogRel((DEVICE_NAME ":VBoxUSBMonSolarisUnregisterClient Failed to find registered client %p\n", pClientDip));
            return VERR_SEARCH_ERROR;
        }
        else
            return VERR_INTERNAL_ERROR;
    }
    else
        return VERR_INTERNAL_ERROR_2;
}


/**
 * USBA driver election callback.
 *
 * @returns USB_SUCCESS if we want to capture the device, USB_FAILURE otherwise.
 * @param   pDevDesc        The parsed device descriptor (does not include subconfigs).
 * @param   pDevStrings     Device strings: Manufacturer, Product, Serial Number.
 * @param   pszDevicePath   The physical path of the device being attached.
 * @param   Bus             The Bus number on which the device is on.
 * @param   Port            The Port number on the bus.
 * @param   ppszDrv         The name of the driver we wish to capture the device with.
 * @param   pvReserved      Reserved for future use.
 */
int VBoxUSBMonSolarisElectDriver(usb_dev_descr_t *pDevDesc, usb_dev_str_t *pDevStrings, char *pszDevicePath, int Bus, int Port,
                                char **ppszDrv, void *pvReserved)
{
    LogFlowFunc((DEVICE_NAME ":VBoxUSBMonSolarisElectDriver pDevDesc=%p pDevStrings=%p pszDevicePath=%s Bus=%d Port=%d\n", pDevDesc,
            pDevStrings, pszDevicePath, Bus, Port));

    AssertPtrReturn(pDevDesc, USB_FAILURE);
    AssertPtrReturn(pDevStrings, USB_FAILURE);

    /*
     * Create a filter from the device being attached.
     */
    USBFILTER Filter;
    USBFilterInit(&Filter, USBFILTERTYPE_CAPTURE);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_VENDOR_ID, pDevDesc->idVendor, true);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_PRODUCT_ID, pDevDesc->idProduct, true);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_DEVICE_REV, pDevDesc->bcdDevice, true);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_DEVICE_CLASS, pDevDesc->bDeviceClass, true);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_DEVICE_SUB_CLASS, pDevDesc->bDeviceSubClass, true);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_DEVICE_PROTOCOL, pDevDesc->bDeviceProtocol, true);
    USBFilterSetNumExact(&Filter, USBFILTERIDX_BUS, 0x0 /* Bus */, true); /* Use 0x0 as userland initFilterFromDevice function in Main: see comment on "SetMustBePresent" below */
    USBFilterSetNumExact(&Filter, USBFILTERIDX_PORT, Port, true);
    USBFilterSetStringExact(&Filter, USBFILTERIDX_MANUFACTURER_STR, pDevStrings->usb_mfg, true);
    USBFilterSetStringExact(&Filter, USBFILTERIDX_PRODUCT_STR, pDevStrings->usb_product, true);
    USBFilterSetStringExact(&Filter, USBFILTERIDX_SERIAL_NUMBER_STR, pDevStrings->usb_serialno, true);

    /* This doesn't work like it should (USBFilterMatch fails on matching field (6) i.e. Bus despite this. Investigate later. */
    USBFilterSetMustBePresent(&Filter, USBFILTERIDX_BUS, false /* fMustBePresent */);

    LogFlow((DEVICE_NAME ":VBoxUSBMonSolarisElectDriver: idVendor=%#x idProduct=%#x bcdDevice=%#x bDeviceClass=%#x bDeviceSubClass=%#x bDeviceProtocol=%#x bBus=%#x bPort=%#x\n",
              USBFilterGetNum(&Filter, USBFILTERIDX_VENDOR_ID),
              USBFilterGetNum(&Filter, USBFILTERIDX_PRODUCT_ID),
              USBFilterGetNum(&Filter, USBFILTERIDX_DEVICE_REV),
              USBFilterGetNum(&Filter, USBFILTERIDX_DEVICE_CLASS),
              USBFilterGetNum(&Filter, USBFILTERIDX_DEVICE_SUB_CLASS),
              USBFilterGetNum(&Filter, USBFILTERIDX_DEVICE_PROTOCOL),
              USBFilterGetNum(&Filter, USBFILTERIDX_BUS),
              USBFilterGetNum(&Filter, USBFILTERIDX_PORT)));
    LogFlow((DEVICE_NAME ":VBoxUSBMonSolarisElectDriver: Manufacturer=%s Product=%s Serial=%s\n",
              USBFilterGetString(&Filter, USBFILTERIDX_MANUFACTURER_STR)  ? USBFilterGetString(&Filter, USBFILTERIDX_MANUFACTURER_STR)  : "<null>",
              USBFilterGetString(&Filter, USBFILTERIDX_PRODUCT_STR)       ? USBFilterGetString(&Filter, USBFILTERIDX_PRODUCT_STR)       : "<null>",
              USBFilterGetString(&Filter, USBFILTERIDX_SERIAL_NUMBER_STR) ? USBFilterGetString(&Filter, USBFILTERIDX_SERIAL_NUMBER_STR) : "<null>"));

    /*
     * Run through user filters and try to see if it has a match.
     */
    uintptr_t uId = 0;
    RTPROCESS Owner = VBoxUSBFilterMatch(&Filter, &uId);
    USBFilterDelete(&Filter);
    if (Owner == NIL_RTPROCESS)
    {
        LogFlow((DEVICE_NAME ":No matching filters, device %#x:%#x uninteresting.\n", pDevDesc->idVendor, pDevDesc->idProduct));
        return USB_FAILURE;
    }

    *ppszDrv = ddi_strdup(VBOXUSB_DRIVER_NAME, KM_SLEEP);
    LogRel((DEVICE_NAME ": Capturing %s %#x:%#x:%s\n", pDevStrings->usb_product ? pDevStrings->usb_product : "<Unnamed USB device>",
                    pDevDesc->idVendor, pDevDesc->idProduct, pszDevicePath));
    return USB_SUCCESS;
}

#endif


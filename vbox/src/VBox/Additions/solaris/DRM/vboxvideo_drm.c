/* $Id: vboxvideo_drm.c 18057 2009-03-18 12:31:39Z vboxsync $ */
/** @file
 * vboxvideo_drm - Direct Rendering Module, Solaris Specific Code.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <sys/conf.h>
#include <sys/modctl.h>
#include <sys/mutex.h>
#include <sys/pci.h>
#include <sys/stat.h>
#include <sys/ddi.h>
#include <sys/ddi_intr.h>
#include <sys/sunddi.h>
#include <sys/open.h>

#include <VBox/log.h>
#include <VBox/version.h>

#include "include/drmP.h"
#include "include/drm.h"

#undef u /* /usr/include/sys/user.h:249:1 is where this is defined to (curproc->p_user). very cool. */


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define VBOXSOLQUOTE2(x)                #x
#define VBOXSOLQUOTE(x)                 VBOXSOLQUOTE2(x)
/** The module name. */
#define DEVICE_NAME                     "vboxvideo"
/** The module description as seen in 'modinfo'. */
#define DEVICE_DESC_DRV                 "VirtualBox DRM"

/** DRM Specific defines */
#define DRIVER_AUTHOR                   "Sun Microsystems Inc."
#define DRIVER_NAME                     DEVICE_NAME
#define DRIVER_DESC                     DEVICE_DESC_DRV
#define DRIVER_DATE                     "20090317"
#define DRIVER_MAJOR                    1
#define DRIVER_MINOR                    0
#define DRIVER_PATCHLEVEL               0
#define vboxvideo_PCI_IDS               { 0x80ee, 0xbeef, 0, "VirtualBox Video" }, \
                                        { 0, 0, 0, NULL }


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static int VBoxVideoSolarisAttach(dev_info_t *pDip, ddi_attach_cmd_t enmCmd);
static int VBoxVideoSolarisDetach(dev_info_t *pDip, ddi_detach_cmd_t enmCmd);
static int VBoxVideoSolarisGetInfo(dev_info_t *pDip, ddi_info_cmd_t enmCmd, void *pvArg, void **ppvResult);

static void vboxVideoSolarisConfigure(drm_driver_t *pDriver);


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * cb_ops: for drivers that support char/block entry points
 */
static struct cb_ops g_VBoxVideoSolarisCbOps =
{
    nodev,                  /* c open */
    nodev,                  /* c close */
    nodev,                  /* b strategy */
    nodev,                  /* b dump */
    nodev,                  /* b print */
    nodev,                  /* c read */
    nodev,                  /* c write*/
    nodev,                  /* ioctl */
    nodev,                  /* c devmap */
    nodev,                  /* c mmap */
    nodev,                  /* c segmap */
    nochpoll,               /* c poll */
    ddi_prop_op,            /* property ops */
    NULL,                   /* streamtab  */
    D_NEW | D_MP,           /* compat. flag */
    CB_REV                  /* revision */
};

/**
 * dev_ops: for driver device operations
 */
static struct dev_ops g_VBoxVideoSolarisDevOps =
{
    DEVO_REV,               /* driver build revision */
    0,                      /* ref count */
    VBoxVideoSolarisGetInfo,
    nulldev,                /* identify */
    nulldev,                /* probe */
    VBoxVideoSolarisAttach,
    VBoxVideoSolarisDetach,
    nodev,                  /* reset */
    &g_VBoxVideoSolarisCbOps,
    (struct bus_ops *)0,
    nodev                   /* power */
};

/**
 * modldrv: export driver specifics to the kernel
 */
static struct modldrv g_VBoxVideoSolarisModule =
{
    &mod_driverops,         /* extern from kernel */
    DEVICE_DESC_DRV " " VBOX_VERSION_STRING "r" VBOXSOLQUOTE(VBOX_SVN_REV),
    &g_VBoxVideoSolarisDevOps
};

/**
 * modlinkage: export install/remove/info to the kernel
 */
static struct modlinkage g_VBoxVideoSolarisModLinkage =
{
    MODREV_1,               /* loadable module system revision */
    &g_VBoxVideoSolarisModule,
    NULL                    /* terminate array of linkage structures */
};

/* VBoxVideo device PCI ID */
static drm_pci_id_list_t vboxvideo_pciidlist[] = {
    vboxvideo_PCI_IDS
};


/** DRM Driver */
static drm_driver_t	g_VBoxVideoSolarisDRMDriver = { 0 };


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** Device handle (we support only one instance). */
static dev_info_t *g_pDip;

/** Soft state. */
static void *g_pVBoxVideoSolarisState;

/** GCC C++ hack. */
unsigned __gxx_personality_v0 = 0xdecea5ed;


/**
 * Kernel entry points
 */
int _init(void)
{
    LogFlow((DEVICE_NAME ":_init\n"));
    int rc = ddi_soft_state_init(&g_pVBoxVideoSolarisState, sizeof(drm_device_t), DRM_MAX_INSTANCES);
    if (!rc)
        return mod_install(&g_VBoxVideoSolarisModLinkage);
    else
        LogRel((DEVICE_NAME ":_init: ddi_soft_state_init failed. rc=%d\n", rc));
}


int _fini(void)
{
    LogFlow((DEVICE_NAME ":_fini\n"));
    int rc = mod_remove(&g_VBoxVideoSolarisModLinkage);
    ddi_soft_state_fini(&g_pVBoxVideoSolarisState);
    return rc;
}


int _info(struct modinfo *pModInfo)
{
    LogFlow((DEVICE_NAME ":_info\n"));
    return mod_info(&g_VBoxVideoSolarisModLinkage, pModInfo);
}


/**
 * Attach entry point, to attach a device to the system or resume it.
 *
 * @param   pDip            The module structure instance.
 * @param   enmCmd          Operation type (attach/resume).
 *
 * @returns corresponding solaris error code.
 */
static int VBoxVideoSolarisAttach(dev_info_t *pDip, ddi_attach_cmd_t enmCmd)
{
    LogFlow((DEVICE_NAME ":VBoxVideoSolarisAttach pDip=%p enmCmd=%d\n", pDip, enmCmd));

    int rc = -1;
    switch (enmCmd)
    {
        case DDI_ATTACH:
        {
            drm_device_t *pState;
            int Instance = ddi_get_instance(pDip);
            int rc = ddi_soft_state_zalloc(g_pVBoxVideoSolarisState, Instance);
            if (rc == DDI_SUCCESS)
            {
                pState = ddi_get_soft_state(g_pVBoxVideoSolarisState, Instance);
                pState->dip = pDip;
                pState->driver = &g_VBoxVideoSolarisDRMDriver;
                
                /*
                 * Register using the DRM module which will create the minor nodes
                 */
                void *pDRMHandle = drm_supp_register(pDip, pState);
                if (pDRMHandle)
                {
                    pState->drm_handle = pDRMHandle;
                    
                    /*
                     * Probe with our pci-id.
                     * -XXX- is probing really required???
                     */
                    pState->drm_supported = DRM_UNSUPPORT;
                    rc = drm_probe(pState, vboxvideo_pciidlist);
                    if (rc == DDI_SUCCESS)
                    {
                        pState->drm_supported = DRM_SUPPORT;

                        /*
                         * Call the common attach DRM routine.
                         */
                        rc = drm_attach(pState);
                        if (rc == DDI_SUCCESS)
                        {
                            return DDI_SUCCESS;
                        }
                        else
                            LogRel((DEVICE_NAME ":VBoxVideoSolarisAttach drm_attach failed.rc=%d\n", rc));
                    }
                    else
                        LogRel((DEVICE_NAME ":VBoxVideoSolarisAttach drm_probe failed.rc=%d\n", rc));

                    drm_supp_unregister(pDRMHandle);
                }
                else
                    LogRel((DEVICE_NAME ":VBoxVideoSolarisAttach drm_supp_register failed.\n"));

                ddi_soft_state_free(g_pVBoxVideoSolarisState, Instance);
            }
            else
                LogRel((DEVICE_NAME ":VBoxVideoSolarisAttach failed to alloc memory for soft state.rc=%d\n", rc));
            return DDI_FAILURE;
        }

        case DDI_RESUME:
        {
            /* Nothing to do here... */
            return DDI_SUCCESS;
        }
    }
    return DDI_FAILURE;
}


/**
 * Detach entry point, to detach a device to the system or suspend it.
 *
 * @param   pDip            The module structure instance.
 * @param   enmCmd          Operation type (detach/suspend).
 *
 * @returns corresponding solaris error code.
 */
static int VBoxVideoSolarisDetach(dev_info_t *pDip, ddi_detach_cmd_t enmCmd)
{
    LogFlow((DEVICE_NAME ":VBoxVideoSolarisDetach pDip=%p enmCmd=%d\n", pDip, enmCmd));

    switch (enmCmd)
    {
        case DDI_DETACH:
        {
            int Instance = ddi_get_instance(pDip);
            drm_device_t *pState = ddi_get_soft_state(g_pVBoxVideoSolarisState, Instance);
            if (pState)
            {
                drm_detach(pState);
                drm_supp_unregister(pState->drm_handle);
                ddi_soft_state_free(g_pVBoxVideoSolarisState, Instance);
                return DDI_SUCCESS;
            }
            else
                LogRel((DEVICE_NAME ":VBoxVideoSolarisDetach failed to get soft state.\n"));

            return DDI_FAILURE;
        }

        case DDI_RESUME:
        {
            /* Nothing to do here... */
            return DDI_SUCCESS;
        }
    }
    return DDI_FAILURE;
}


/*
 * Info entry point, called by solaris kernel for obtaining driver info.
 *
 * @param   pDip            The module structure instance (do not use).
 * @param   enmCmd          Information request type.
 * @param   pvArg           Type specific argument.
 * @param   ppvResult       Where to store the requested info.
 *
 * @return  corresponding solaris error code.
 */
static int VBoxVideoSolarisGetInfo(dev_info_t *pDip, ddi_info_cmd_t enmCmd, void *pvArg, void **ppvResult)
{
    LogFlow((DEVICE_NAME ":VBoxGuestSolarisGetInfo\n"));

    int rc = DDI_FAILURE;
    int Instance = drm_dev_to_instance((dev_t)pvArg);
    drm_device_t *pState = NULL;
    switch (enmCmd)
    {
        case DDI_INFO_DEVT2DEVINFO:
        {
            pState = ddi_get_soft_state(g_pVBoxVideoSolarisState, Instance);
            if (   pState
                && pState->dip)
            {
                *ppvResult = (void *)pState->dip;
                rc = DDI_SUCCESS;
            }
            else
            {            
                LogRel((DEVICE_NAME ":VBoxGuestSolarisGetInfo state or state's devinfo invalid.\n"));
                rc = DDI_FAILURE;
            }
            break;
        }

        case DDI_INFO_DEVT2INSTANCE:
        {
            *ppvResult = (void *)(uintptr_t)Instance;
            rc = DDI_SUCCESS;
            break;
        }

        default:
        {
            rc = DDI_FAILURE;
            break;
        }
    }

    return rc;
}


static int vboxVideoSolarisLoad(drm_driver_t *pDriver, unsigned long fFlag)
{
    return 0;
}

static int vboxVideoSolarisUnload(drm_driver_t *pDriver)
{
    return 0;
}

static int vboxVideoSolarisPreClose(drm_driver_t *pDriver, drm_file_t *pFile)
{
    return 0;
}


static void vboxVideoSolarisConfigure(drm_driver_t *pDriver)
{
    /*
     * DRM entry points, use the common DRM extension wherever possible.
     */
    pDriver->buf_priv_size      = 1;
    pDriver->load               = vboxVideoSolarisLoad;
    pDriver->unload             = vboxVideoSolarisUnload;
    pDriver->preclose           = vboxVideoSolarisPreClose;
    pDriver->lastclose          = vboxVideoSolarisUnload;
    pDriver->device_is_agp      = drm_device_is_agp;
#if 0
    pDriver->get_vblank_counter = drm_vblank_count;
    pDriver->enable_vblank      = NULL;
    pDriver->disable_vblank     = NULL;
    pDriver->irq_install        = drm_driver_irq_install;
    pDriver->irq_preinstall     = drm_driver_irq_preinstall;
    pDriver->irq_postinstall    = drm_driver_irq_postinstall;
    pDirver->irq_uninstall      = drm_driver_irq_uninstall;
    pDriver->irq_handler        = drm_driver_irq_handler;

    pDriver->driver_ioctls      = 
    pDriver->max_driver_ioctls  = 
#endif

    pDriver->driver_name        = DRIVER_NAME;
    pDriver->driver_desc        = DRIVER_DESC;
    pDriver->driver_date        = DRIVER_DATE;
    pDriver->driver_major       = DRIVER_MAJOR;
    pDriver->driver_minor       = DRIVER_MINOR;
    pDriver->driver_patchlevel  = DRIVER_PATCHLEVEL;

    pDriver->use_agp            = 1;
    pDriver->require_agp        = 1;
    pDriver->use_irq            = 1;
}


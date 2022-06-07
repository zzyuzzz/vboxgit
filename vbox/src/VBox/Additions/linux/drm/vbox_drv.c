/*  $Id: vbox_drv.c 59597 2016-02-05 14:36:27Z vboxsync $ */
/** @file
 * VirtualBox Additions Linux kernel video driver
 */

/*
 * Copyright (C) 2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 *
 * This code is based on
 * ast_drv.c
 * with the following copyright and permission notice:
 *
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 */
/*
 * Authors: Dave Airlie <airlied@redhat.com>
 */
#include "vbox_drv.h"

#include <VBox/VBoxGuest.h>

#include <linux/module.h>
#include <linux/console.h>

#include <drm/drmP.h>
#include <drm/drm_crtc_helper.h>

int vbox_modeset = -1;

MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, vbox_modeset, int, 0400);

static struct drm_driver driver;

static DEFINE_PCI_DEVICE_TABLE(pciidlist) =
{
    {0x80ee, 0xbeef, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},
    {0, 0, 0},
};

MODULE_DEVICE_TABLE(pci, pciidlist);

static int vbox_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    return drm_get_pci_dev(pdev, ent, &driver);
}


static void vbox_pci_remove(struct pci_dev *pdev)
{
    struct drm_device *dev = pci_get_drvdata(pdev);

    drm_put_dev(dev);
}



static int vbox_drm_freeze(struct drm_device *dev)
{
    drm_kms_helper_poll_disable(dev);

    pci_save_state(dev->pdev);

    console_lock();
    vbox_fbdev_set_suspend(dev, 1);
    console_unlock();
    return 0;
}

static int vbox_drm_thaw(struct drm_device *dev)
{
    int error = 0;

    drm_mode_config_reset(dev);
    drm_helper_resume_force_mode(dev);

    console_lock();
    vbox_fbdev_set_suspend(dev, 0);
    console_unlock();
    return error;
}

static int vbox_drm_resume(struct drm_device *dev)
{
    int ret;

    if (pci_enable_device(dev->pdev))
        return -EIO;

       ret = vbox_drm_thaw(dev);
    if (ret)
       return ret;

    drm_kms_helper_poll_enable(dev);
    return 0;
}

static int vbox_pm_suspend(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *ddev = pci_get_drvdata(pdev);
    int error;

    error = vbox_drm_freeze(ddev);
    if (error)
        return error;

    pci_disable_device(pdev);
    pci_set_power_state(pdev, PCI_D3hot);
    return 0;
}

static int vbox_pm_resume(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *ddev = pci_get_drvdata(pdev);
    return vbox_drm_resume(ddev);
}

static int vbox_pm_freeze(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *ddev = pci_get_drvdata(pdev);

    if (!ddev || !ddev->dev_private)
        return -ENODEV;
    return vbox_drm_freeze(ddev);

}

static int vbox_pm_thaw(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *ddev = pci_get_drvdata(pdev);
    return vbox_drm_thaw(ddev);
}

static int vbox_pm_poweroff(struct device *dev)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct drm_device *ddev = pci_get_drvdata(pdev);

    return vbox_drm_freeze(ddev);
}

static const struct dev_pm_ops vbox_pm_ops = {
    .suspend = vbox_pm_suspend,
    .resume = vbox_pm_resume,
    .freeze = vbox_pm_freeze,
    .thaw = vbox_pm_thaw,
    .poweroff = vbox_pm_poweroff,
    .restore = vbox_pm_resume,
};

static struct pci_driver vbox_pci_driver =
{
    .name = DRIVER_NAME,
    .id_table = pciidlist,
    .probe = vbox_pci_probe,
    .remove = vbox_pci_remove,
    .driver.pm = &vbox_pm_ops,
};


static const struct file_operations vbox_fops =
{
    .owner = THIS_MODULE,
    .open = drm_open,
    .release = drm_release,
    .unlocked_ioctl = drm_ioctl,
    .mmap = vbox_mmap,
    .poll = drm_poll,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
    .fasync = drm_fasync,
#endif
#ifdef CONFIG_COMPAT
    .compat_ioctl = drm_compat_ioctl,
#endif
    .read = drm_read,
};

static struct drm_driver driver =
{
    .driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_HAVE_IRQ,
    .dev_priv_size = 0,

    .load = vbox_driver_load,
    .unload = vbox_driver_unload,
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 17, 0)
    .set_busid = drm_pci_set_busid,
#endif

    .fops = &vbox_fops,
    .irq_handler = vbox_irq_handler,
    .name = DRIVER_NAME,
    .desc = DRIVER_DESC,
    .date = DRIVER_DATE,
    .major = DRIVER_MAJOR,
    .minor = DRIVER_MINOR,
    .patchlevel = DRIVER_PATCHLEVEL,

    .gem_free_object = vbox_gem_free_object,
    .dumb_create = vbox_dumb_create,
    .dumb_map_offset = vbox_dumb_mmap_offset,
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
    .dumb_destroy = vbox_dumb_destroy,
#else
    .dumb_destroy = drm_gem_dumb_destroy,
#endif

};

static int __init vbox_init(void)
{
#ifdef CONFIG_VGA_CONSOLE
    if (vgacon_text_force() && vbox_modeset == -1)
        return -EINVAL;
#endif

    if (vbox_modeset == 0)
        return -EINVAL;
    return drm_pci_init(&driver, &vbox_pci_driver);
}
static void __exit vbox_exit(void)
{
    drm_pci_exit(&driver, &vbox_pci_driver);
}

module_init(vbox_init);
module_exit(vbox_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");


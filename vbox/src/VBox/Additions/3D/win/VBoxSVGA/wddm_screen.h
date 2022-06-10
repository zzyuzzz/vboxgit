/* $Id: wddm_screen.h 76539 2018-12-30 06:22:29Z vboxsync $ */
/** @file
 * VirtualBox Windows Guest Mesa3D - VMSVGA hardware driver.
 */

/*
 * Copyright (C) 2016-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_VBoxSVGA_wddm_screen_h
#define VBOX_INCLUDED_VBoxSVGA_wddm_screen_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBoxGaDriver.h>

#include "vmw_screen.h"

struct vmw_winsys_screen_wddm
{
    struct vmw_winsys_screen base;

    const WDDMGalliumDriverEnv *pEnv;
    VBOXGAHWINFOSVGA HwInfo;
};

#endif


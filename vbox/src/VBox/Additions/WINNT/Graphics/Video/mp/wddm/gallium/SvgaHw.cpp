/* $Id: SvgaHw.cpp 75443 2018-11-14 10:17:08Z vboxsync $ */
/** @file
 * VirtualBox Windows Guest Mesa3D - VMSVGA hardware access.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "SvgaHw.h"

int SvgaEnable(PVBOXWDDM_EXT_VMSVGA pSvga)
{
    NOREF(pSvga);
    return 0;
}

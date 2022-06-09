/* $Id: DevVGAModes.h 69052 2017-10-11 19:04:39Z vboxsync $ */
/** @file
 * DevVGA - VBox VGA/VESA device, VBE modes.
 *
 * List of static mode information, containing all "supported" VBE
 * modes and their 'settings'.
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

#include <VBoxVideoVBE.h>
#include <VBoxVideoVBEPrivate.h>

#include "vbetables.h"

#define MODE_INFO_SIZE ( sizeof(mode_info_list) / sizeof(ModeInfoListItem) )

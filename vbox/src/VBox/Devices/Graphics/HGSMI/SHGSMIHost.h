/* $Id: SHGSMIHost.h 71590 2018-03-31 18:34:28Z vboxsync $ */
/** @file
 * Missing description
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___SHGSMIHost_h___
#define ___SHGSMIHost_h___

#include "HGSMIHost.h"

int  VBoxSHGSMICommandComplete(PHGSMIINSTANCE pIns, void RT_UNTRUSTED_VOLATILE_GUEST *pvData);
void VBoxSHGSMICommandMarkAsynchCompletion(void RT_UNTRUSTED_VOLATILE_GUEST *pvData);

#endif


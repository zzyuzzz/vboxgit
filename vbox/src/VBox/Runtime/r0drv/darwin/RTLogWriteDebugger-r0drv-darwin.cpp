/* $Id: RTLogWriteDebugger-r0drv-darwin.cpp 1  vboxsync $ */
/** @file
 * InnoTek Portable Runtime - Log To Debugger, Ring-0 Driver, Darwin.
 */

/*
 * Copyright (C) 2006 InnoTek Systemberatung GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "the-darwin-kernel.h"
#include <iprt/log.h>
#include <iprt/assert.h>


RTDECL(void) RTLogWriteDebugger(const char *pch, size_t cb)
{
    kprintf("%.*s", cb, pch);
    return;
}


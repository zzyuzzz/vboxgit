/* $Id: RTLogWriteDebugger-r0drv-nt.cpp 4071 2007-08-07 17:07:59Z vboxsync $ */
/** @file
 * innotek Portable Runtime - Log To Debugger, Ring-0 Driver, NT.
 */

/*
 * Copyright (C) 2006-2007 innotek GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "the-nt-kernel.h"
#include <iprt/log.h>
#include <iprt/assert.h>


RTDECL(void) RTLogWriteDebugger(const char *pch, size_t cb)
{
    if (pch[cb] != '\0')
        AssertBreakpoint();
    DbgPrint("%s", pch);
    return;
}


/* $Id: RTLogWriteDebugger-win32.cpp 2981 2007-06-01 16:01:28Z vboxsync $ */
/** @file
 * innotek Portable Runtime - Log To Debugger, Win32.
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
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <Windows.h>

#include <iprt/log.h>
#include <iprt/assert.h>


RTDECL(void) RTLogWriteDebugger(const char *pch, size_t cb)
{
    if (pch[cb] != '\0')
        AssertBreakpoint();
    OutputDebugStringA(pch);
    return;
}


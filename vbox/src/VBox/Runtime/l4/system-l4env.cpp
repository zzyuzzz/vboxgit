/* $Id: system-l4env.cpp 620 2007-02-05 09:27:22Z vboxsync $ */
/** @file
 * InnoTek Portable Runtime - System, l4env.
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
#include <iprt/system.h>
#include <iprt/assert.h>



RTDECL(unsigned) RTSystemProcessorGetCount(void)
{
    return 1;
}


RTDECL(uint64_t) RTSystemProcessorGetActiveMask(void)
{
    return 1;
}



/* $Id: CPUMStack.cpp 23 2007-01-15 14:08:28Z vboxsync $ */
/** @file
 * CPUM - CPU Monitor(/Manager) - Stack manipulation.
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
#include <VBox/cpum.h>
#include "CPUMInternal.h"
#include <VBox/vm.h>
#include <VBox/mm.h>

/** Disable stack frame pointer generation here. */
#if defined(_MSC_VER) && !defined(DEBUG)
# pragma optimize("y", "off");
#endif


CPUMDECL(void) CPUMPushHyper(PVM pVM, uint32_t u32)
{
    /* ASSUME always on flat stack within hypervisor memory for now */
    pVM->cpum.s.Hyper.esp -= sizeof(u32);
    *(uint32_t *)MMHyperGC2HC(pVM, (RTGCPTR)pVM->cpum.s.Hyper.esp) = u32;
}


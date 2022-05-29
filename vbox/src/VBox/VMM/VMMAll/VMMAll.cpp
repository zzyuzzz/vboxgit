/* $Id: VMMAll.cpp 10450 2008-07-09 21:55:45Z vboxsync $ */
/** @file
 * VMM All Contexts.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_VMM
#include <VBox/vmm.h>
#include "VMMInternal.h"
#include <VBox/vm.h>
#include <VBox/param.h>


#ifndef IN_RING0

/**
 * Gets the bottom of the hypervisor stack - GC Ptr.
 * I.e. the returned address is not actually writable.
 *
 * @returns bottom of the stack.
 * @param   pVM         The VM handle.
 */
RTGCPTR VMMGetStackGC(PVM pVM)
{
    return (RTGCPTR)pVM->vmm.s.pbGCStackBottom;
}


/**
 * Gets the bottom of the hypervisor stack - HC Ptr.
 * I.e. the returned address is not actually writable.
 *
 * @returns bottom of the stack.
 * @param   pVM         The VM handle.
 */
RTHCPTR VMMGetHCStack(PVM pVM)
{
    return pVM->vmm.s.pbHCStack + VMM_STACK_SIZE;
}

#endif /* !IN_RING0 */


/**
 * Gets the VBOX_SVN_REV.
 *
 * This is just to avoid having to compile a bunch of big files
 * and requires less Makefile mess.
 *
 * @returns VBOX_SVN_REV.
 */
VMMDECL(uint32_t) VMMGetSvnRev(void)
{
    return VBOX_SVN_REV;
}

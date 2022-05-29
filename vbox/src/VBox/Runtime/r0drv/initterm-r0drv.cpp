/* $Id: initterm-r0drv.cpp 8245 2008-04-21 17:24:28Z vboxsync $ */
/** @file
 * IPRT - Initialization & Termination, R0 Driver, Common.
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/initterm.h>
#include <iprt/assert.h>
#include <iprt/err.h>

#include "internal/initterm.h"
#include "internal/thread.h"


/**
 * Initalizes the ring-0 driver runtime library.
 *
 * @returns iprt status code.
 * @param   fReserved       Flags reserved for the future.
 */
RTR0DECL(int) RTR0Init(unsigned fReserved)
{
    int rc;
    Assert(fReserved == 0);
    rc = rtR0InitNative();
    if (RT_SUCCESS(rc))
    {
#if !defined(RT_OS_LINUX) && !defined(RT_OS_WINDOWS)
        rc = rtThreadInit();
#endif
        if (RT_SUCCESS(rc))
            return rc;

        rtR0TermNative();
    }
    return rc;
}


/**
 * Terminates the ring-0 driver runtime library.
 */
RTR0DECL(void) RTR0Term(void)
{
#if !defined(RT_OS_LINUX) && !defined(RT_OS_WINDOWS)
    rtThreadTerm();
#endif
    rtR0TermNative();
}


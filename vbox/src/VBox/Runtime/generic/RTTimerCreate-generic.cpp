/** $Id: RTTimerCreate-generic.cpp 8155 2008-04-18 15:16:47Z vboxsync $ */
/** @file
 * innotek Portable Runtime - Timers, Generic RTTimerCreate() Implementation.
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
#include <iprt/timer.h>
#include <iprt/err.h>
#include <iprt/assert.h>


RTDECL(int) RTTimerCreate(PRTTIMER *ppTimer, unsigned uMilliesInterval, PFNRTTIMER pfnTimer, void *pvUser)
{
    int rc = RTTimerCreateEx(ppTimer, uMilliesInterval * UINT64_C(1000000), 0, pfnTimer, pvUser);
    if (RT_SUCCESS(rc))
    {
        rc = RTTimerStart(*ppTimer, 0);
        if (RT_SUCCESS(rc))
            return rc;
        int rc2 = RTTimerDestroy(*ppTimer); AssertRC(rc2);
        *ppTimer = NULL;
    }
    return rc;
}


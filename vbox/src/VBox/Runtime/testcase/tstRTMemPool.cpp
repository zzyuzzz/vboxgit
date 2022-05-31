/* $Id: tstRTMemPool.cpp 20562 2009-06-14 18:38:25Z vboxsync $ */
/** @file
 * IPRT Testcase - MemPool.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
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
#include <iprt/mempool.h>

#include <iprt/err.h>
#include <iprt/initterm.h>
#include <iprt/stdarg.h>
#include <iprt/string.h>
#include <iprt/test.h>



int main()
{
    int rc = RTR3Init();
    if (RT_FAILURE(rc))
        return 1;
    RTTEST hTest;
    rc = RTTestCreate("tstRTMemPool", &hTest);
    if (RT_FAILURE(rc))
        return 1;
    RTTestBanner(hTest);

    /*
     * Smoke tests using first the default and then a custom pool.
     */



    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}



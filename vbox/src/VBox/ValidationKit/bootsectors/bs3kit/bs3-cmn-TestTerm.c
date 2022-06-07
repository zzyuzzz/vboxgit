/* $Id: bs3-cmn-TestTerm.c 59863 2016-02-26 20:59:52Z vboxsync $ */
/** @file
 * BS3Kit - Bs3TestTerm
 */

/*
 * Copyright (C) 2007-2016 Oracle Corporation
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
 */


#include "bs3kit-template-header.h"
#include "bs3-cmn-test.h"


/**
 * Equivalent to RTTestSummaryAndDestroy.
 */
BS3_DECL(void) Bs3TestTerm(void)
{
    /*
     * Close current sub-test.
     */
    if (BS3_CMN_NM(g_pszBs3SubTest))
    {

    }


    /*
     * Report summary.
     */
    if (BS3_CMN_NM(g_pszBs3Test))
    {
        Bs3PrintStr(BS3_CMN_NM(g_pszBs3Test));
        if (g_cusBs3TestErrors == 0)
            Bs3Printf(": SUCCESS (%u tests)\n", g_cusBs3SubTests);
        else
            Bs3Printf(": FAILURE - %u (%u of %u tests)\n", g_cusBs3TestErrors, g_cusBs3SubTestsFailed, g_cusBs3SubTests);
    }

    /*
     * Tell VMMDev.
     */
    bs3TestSendCmdWithU32(VMMDEV_TESTING_CMD_TERM, g_cusBs3TestErrors);
    for (;;) { }

    BS3_CMN_NM(g_pszBs3Test) = NULL;
}


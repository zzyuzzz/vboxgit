/* $Id: bs3-timing-1-exe.c 93115 2022-01-01 11:31:46Z vboxsync $ */
/** @file
 * BS3Kit - bs3-timing-1, regular executable version of the TSC test.
 */

/*
 * Copyright (C) 2007-2022 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/assert.h>
#include <iprt/errcore.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/time.h>

/* Fake bs3kit.h bits:  */
#define Bs3TestNow      RTTimeNanoTS
#define Bs3TestPrintf   RTPrintf
#define Bs3TestFailedF  RTMsgError
#define Bs3MemZero      RT_BZERO
#define BS3_DECL(t)     t

#define STANDALONE_EXECUTABLE
#include "bs3-timing-1-32.c32"


int main(int argc, char **argv)
{
    /*
     * Parse arguments.
     */
    static const RTGETOPTDEF s_aOptions[] =
    {
        { "--loops",            'l', RTGETOPT_REQ_UINT32 },
        { "--seconds",          's', RTGETOPT_REQ_UINT32 },
        { "--min-history",      'm', RTGETOPT_REQ_UINT32 },
        { "--quiet",            'q', RTGETOPT_REQ_NOTHING },
        { "--verbose",          'v', RTGETOPT_REQ_NOTHING },
    };
    uint32_t        cLoops      = 5;
    uint32_t        cSecs       = 10;
    unsigned        uVerbosity  = 1;
    unsigned        iMinHistory = 17;

    RTGETOPTSTATE   GetState;
    RTGETOPTUNION   ValueUnion;

    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    rc = RTGetOptInit(&GetState, argc, argv, s_aOptions, RT_ELEMENTS(s_aOptions), 1, RTGETOPTINIT_FLAGS_OPTS_FIRST);
    AssertRC(rc);
    while ((rc = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (rc)
        {
            case 'l':
                cLoops = ValueUnion.u32;
                break;
            case 'm':
                iMinHistory = ValueUnion.u32;
                break;
            case 's':
                cSecs = ValueUnion.u32;
                if (cSecs == 0 || cSecs > RT_SEC_1DAY / 2)
                    return RTMsgErrorExitFailure("Seconds value is out of range: %u (min 1, max %u)", cSecs, RT_SEC_1DAY / 2);
                break;
            case 'q':
                uVerbosity = 0;
                break;
            case 'v':
                uVerbosity++;
                break;
            case 'V':
                RTPrintf("v0.1.0\n");
                return RTEXITCODE_SUCCESS;
            case 'h':
                RTPrintf("usage: %Rbn [-q|-v] [-l <iterations>] [-s <seconds-per-iteration>] [-m <log2-big-gap>]\n", argv[0]);
                return RTEXITCODE_SUCCESS;
            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }

    /*
     * Run the test.
     */
    bs3Timing1_Tsc_Driver(cLoops, cSecs, uVerbosity, iMinHistory);
    return RTEXITCODE_SUCCESS;
}


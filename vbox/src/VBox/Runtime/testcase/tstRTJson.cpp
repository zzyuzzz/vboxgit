/* $Id: tstRTJson.cpp 61721 2016-06-15 14:26:43Z vboxsync $ */
/** @file
 * IPRT Testcase - JSON API.
 */

/*
 * Copyright (C) 2016 Oracle Corporation
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
#include <iprt/json.h>

#include <iprt/test.h>

/**
 * Some basic tests to detect malformed JSON.
 */
static void tstBasic(RTTEST hTest)
{
    RTTestSub(hTest, "Basic valid/malformed tests");
    static struct
    {
        const char *pszJson;
        int         iRcResult;
    } const aTests[] =
    {
        { "",              VERR_JSON_MALFORMED },
        { ",",             VERR_JSON_MALFORMED },
        { ":",             VERR_JSON_MALFORMED },
        { "   \n\t{",      VERR_JSON_MALFORMED },
        { "}",             VERR_JSON_MALFORMED },
        { "[",             VERR_JSON_MALFORMED },
        { "]",             VERR_JSON_MALFORMED },
        { "[ \"test\" : ", VERR_JSON_MALFORMED },
        { "null",          VINF_SUCCESS },
        { "true",          VINF_SUCCESS },
        { "false",         VINF_SUCCESS },
        { "100",           VINF_SUCCESS },
        { "\"test\"",      VINF_SUCCESS },
        { "{ }",           VINF_SUCCESS },
        { "[ ]",           VINF_SUCCESS },
        { "[ 100, 200 ]",  VINF_SUCCESS },
        { "{ \"1\": 1 }",  VINF_SUCCESS },
        { "{ \"1\": 1, \"2\": 2 }", VINF_SUCCESS }
    };
    for (unsigned iTest = 0; iTest < RT_ELEMENTS(aTests); iTest++)
    {
        RTJSONVAL hJsonVal = NIL_RTJSONVAL;
        int rc = RTJsonParseFromString(&hJsonVal, aTests[iTest].pszJson, NULL);
        if (rc != aTests[iTest].iRcResult)
            RTTestIFailed("RTJsonParseFromString() for \"%s\" failed, expected %Rrc got %Rrc\n",
                          aTests[iTest].pszJson, aTests[iTest].iRcResult, rc);
        if (RT_SUCCESS(rc))
        {
            if (hJsonVal != NIL_RTJSONVAL)
                RTJsonValueRelease(hJsonVal);
            else
                RTTestIFailed("RTJsonParseFromString() returned success but no value\n");
        }
        else if (hJsonVal != NIL_RTJSONVAL)
            RTTestIFailed("RTJsonParseFromString() failed but a JSON value was returned\n");
    }
}

/**
 * Test that the parser returns the correct values for a valid JSON.
 */
static void tstCorrectness(RTTEST hTest)
{

}

int main()
{
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstRTJson", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    tstBasic(hTest);
    tstCorrectness(hTest);

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}


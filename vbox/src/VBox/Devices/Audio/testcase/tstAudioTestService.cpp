/* $Id: tstAudioTestService.cpp 90918 2021-08-26 15:29:25Z vboxsync $ */
/** @file
 * Audio testcase - Tests for the Audio Test Service (ATS).
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/

#include <iprt/errcore.h>
#include <iprt/file.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>

#include "../AudioTestService.h"
#include "../AudioTestServiceClient.h"


static size_t g_cToRead = _1M;
static size_t g_cbRead  = 0;


/** @copydoc ATSCALLBACKS::pfnTestSetSendRead */
static DECLCALLBACK(int) tstTestSetSendReadCallback(void const *pvUser,
                                                    const char *pszTag, void *pvBuf, size_t cbBuf, size_t *pcbRead)
{
    RT_NOREF(pvUser, pszTag);

    size_t cbToRead = RT_MIN(g_cToRead - g_cbRead, cbBuf);
    if (cbToRead)
    {
        memset(pvBuf, 0x42, cbToRead);
        g_cbRead += cbToRead;
    }

    *pcbRead = cbToRead;

    return VINF_SUCCESS;
}

int main(int argc, char **argv)
{
    RTR3InitExe(argc, &argv, 0);

    /*
     * Initialize IPRT and create the test.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstAudioTestService", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    ATSCALLBACKS Callbacks;
    RT_ZERO(Callbacks);
    Callbacks.pfnTestSetSendRead  = tstTestSetSendReadCallback;

    ATSCLIENT Client;

    ATSSERVER Srv;
    rc = AudioTestSvcCreate(&Srv);
    RTTEST_CHECK_RC_OK(hTest, rc);
    if (RT_SUCCESS(rc))
    {
        RTGETOPTUNION Val;
        RT_ZERO(Val);

        Val.psz = "server";
        rc = AudioTestSvcHandleOption(&Srv, ATSTCPOPT_CONN_MODE, &Val);
        RTTEST_CHECK_RC_OK(hTest, rc);

        rc = AudioTestSvcInit(&Srv, &Callbacks);
        RTTEST_CHECK_RC_OK(hTest, rc);
        if (RT_SUCCESS(rc))
        {
            uint16_t uPort = ATS_TCP_DEF_BIND_PORT_HOST;

            for (unsigned i = 0; i < 64; i++)
            {
                Val.u16 = uPort;
                rc = AudioTestSvcHandleOption(&Srv, ATSTCPOPT_BIND_PORT, &Val);
                RTTEST_CHECK_RC_OK(hTest, rc);

                rc = AudioTestSvcStart(&Srv);
                if (RT_SUCCESS(rc))
                    break;

                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Port %RU32 already used\n", uPort);

                /* Use a different port base in case VBox already is running
                 * with the same service using ATS_TCP_DEF_BIND_PORT_HOST. */
                uPort = ATS_TCP_DEF_BIND_PORT_HOST + RTRandU32Ex(0, 4242);
            }

            if (RT_SUCCESS(rc))
            {
                RTTestPrintf(hTest, RTTESTLVL_ALWAYS, "Using port %RU32\n", uPort);

                rc = AudioTestSvcClientCreate(&Client);
                RTTEST_CHECK_RC_OK(hTest, rc);

                Val.psz = "client";
                rc = AudioTestSvcClientHandleOption(&Client, ATSTCPOPT_CONN_MODE, &Val);
                RTTEST_CHECK_RC_OK(hTest, rc);

                Val.psz = ATS_TCP_DEF_CONNECT_HOST_ADDR_STR;
                rc = AudioTestSvcClientHandleOption(&Client, ATSTCPOPT_CONNECT_ADDRESS, &Val);
                RTTEST_CHECK_RC_OK(hTest, rc);

                Val.u16 = uPort;
                rc = AudioTestSvcClientHandleOption(&Client, ATSTCPOPT_CONNECT_PORT, &Val);
                RTTEST_CHECK_RC_OK(hTest, rc);

                rc = AudioTestSvcClientConnect(&Client);
                RTTEST_CHECK_RC_OK(hTest, rc);
            }
        }
    }

    if (RT_SUCCESS(rc))
    {
        char szTemp[RTPATH_MAX];
        rc = RTPathTemp(szTemp, sizeof(szTemp));
        RTTEST_CHECK_RC_OK(hTest, rc);
        if (RT_SUCCESS(rc))
        {
            char szName[RTPATH_MAX];
            RTTESTI_CHECK_RC(rc = RTStrCopy(szName, sizeof(szName), szTemp), VINF_SUCCESS);
            RTTESTI_CHECK_RC(rc = RTPathAppend(szName, sizeof(szName), "tstAudioTestService-XXXXX"), VINF_SUCCESS);
            if (RT_SUCCESS(rc))
            {
                rc = AudioTestSvcClientTestSetDownload(&Client, "ignored", szName);
                RTTEST_CHECK_RC_OK(hTest, rc);

                /* ignore rc */ RTFileDelete(szName);
            }
        }
    }

    rc = AudioTestSvcClientClose(&Client);
    RTTEST_CHECK_RC_OK(hTest, rc);

    rc = AudioTestSvcStop(&Srv);
    RTTEST_CHECK_RC_OK(hTest, rc);

    rc = AudioTestSvcDestroy(&Srv);
    RTTEST_CHECK_RC_OK(hTest, rc);

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}

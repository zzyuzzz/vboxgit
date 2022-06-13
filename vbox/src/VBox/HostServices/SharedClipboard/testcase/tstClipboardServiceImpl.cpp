/* $Id: tstClipboardServiceImpl.cpp 93401 2022-01-21 23:41:27Z vboxsync $ */
/** @file
 * Shared Clipboard host service implementation (backend) test case.
 */

/*
 * Copyright (C) 2020-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "../VBoxSharedClipboardSvc-internal.h"

#include <VBox/HostServices/VBoxClipboardSvc.h>
#ifdef RT_OS_WINDOWS
# include <VBox/GuestHost/SharedClipboard-win.h>
#endif

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/test.h>

extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable);

static SHCLCLIENT g_Client;
static VBOXHGCMSVCHELPERS g_Helpers = { NULL };

/** Simple call handle structure for the guest call completion callback */
struct VBOXHGCMCALLHANDLE_TYPEDEF
{
    /** Where to store the result code */
    int32_t rc;
};

/** Call completion callback for guest calls. */
static DECLCALLBACK(int) callComplete(VBOXHGCMCALLHANDLE callHandle, int32_t rc)
{
    callHandle->rc = rc;
    return VINF_SUCCESS;
}

static int setupTable(VBOXHGCMSVCFNTABLE *pTable)
{
    pTable->cbSize = sizeof(*pTable);
    pTable->u32Version = VBOX_HGCM_SVC_VERSION;
    g_Helpers.pfnCallComplete = callComplete;
    pTable->pHelpers = &g_Helpers;
    return VBoxHGCMSvcLoad(pTable);
}

int ShClBackendInit(VBOXHGCMSVCFNTABLE *) { return VINF_SUCCESS; }
void ShClBackendDestroy(void) { }
int ShClBackendDisconnect(PSHCLCLIENT) { return VINF_SUCCESS; }
int ShClBackendConnect(PSHCLCLIENT, bool) { return VINF_SUCCESS; }
int ShClBackendFormatAnnounce(PSHCLCLIENT, SHCLFORMATS) { AssertFailed(); return VINF_SUCCESS; }
int ShClBackendReadData(PSHCLCLIENT, PSHCLCLIENTCMDCTX, SHCLFORMAT, void *, uint32_t, unsigned int *) { AssertFailed(); return VERR_WRONG_ORDER; }
int ShClBackendWriteData(PSHCLCLIENT, PSHCLCLIENTCMDCTX, SHCLFORMAT, void *, uint32_t) { AssertFailed(); return VINF_SUCCESS; }
int ShClBackendSync(PSHCLCLIENT) { return VINF_SUCCESS; }

static void testAnnounceAndReadData(void)
{
    struct VBOXHGCMSVCPARM parms[2];
    VBOXHGCMSVCFNTABLE table;
    int rc;

    RTTestISub("Setting up client ...");
    RTTestIDisableAssertions();

    rc = setupTable(&table);
    RTTESTI_CHECK_MSG_RETV(RT_SUCCESS(rc), ("rc=%Rrc\n", rc));
    /* Unless we are bidirectional the host message requests will be dropped. */
    HGCMSvcSetU32(&parms[0], VBOX_SHCL_MODE_BIDIRECTIONAL);
    rc = table.pfnHostCall(NULL, VBOX_SHCL_HOST_FN_SET_MODE, 1, parms);
    RTTESTI_CHECK_RC_OK(rc);
    rc = shClSvcClientInit(&g_Client, 1 /* clientId */);
    RTTESTI_CHECK_RC_OK(rc);

    RTTestIRestoreAssertions();
}

#ifdef RT_OS_WINDOWS
# include "VBoxOrgCfHtml1.h"    /* From chrome 97.0.4692.71 */
# include "VBoxOrgMimeHtml1.h"

static void testHtmlCf(void)
{
    RTTestISub("CF_HTML");

    char    *pszOutput = NULL;
    uint32_t cbOutput  = UINT32_MAX/2;
    RTTestIDisableAssertions();
    RTTESTI_CHECK_RC(SharedClipboardWinConvertCFHTMLToMIME("", 0, &pszOutput, &cbOutput), VERR_INVALID_PARAMETER);
    RTTestIRestoreAssertions();

    pszOutput = NULL;
    cbOutput  = UINT32_MAX/2;
    RTTESTI_CHECK_RC(SharedClipboardWinConvertCFHTMLToMIME((char *)&g_abVBoxOrgCfHtml1[0], g_cbVBoxOrgCfHtml1,
                                                           &pszOutput, &cbOutput), VINF_SUCCESS);
    RTTESTI_CHECK(cbOutput == g_cbVBoxOrgMimeHtml1);
    RTTESTI_CHECK(memcmp(pszOutput, g_abVBoxOrgMimeHtml1, cbOutput) == 0);
    RTMemFree(pszOutput);


    static RTSTRTUPLE const s_aRoundTrips[] =
    {
        { RT_STR_TUPLE("") },
        { RT_STR_TUPLE("1") },
        { RT_STR_TUPLE("12") },
        { RT_STR_TUPLE("123") },
        { RT_STR_TUPLE("1234") },
        { RT_STR_TUPLE("12345") },
        { RT_STR_TUPLE("123456") },
        { RT_STR_TUPLE("1234567") },
        { RT_STR_TUPLE("12345678") },
        { RT_STR_TUPLE("123456789") },
        { RT_STR_TUPLE("1234567890") },
        { RT_STR_TUPLE("<h2>asdfkjhasdflhj</h2>") },
        { RT_STR_TUPLE("<h2>asdfkjhasdflhj</h2>\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0") },
        { (const char *)g_abVBoxOrgMimeHtml1, sizeof(g_abVBoxOrgMimeHtml1) },
    };

    for (size_t i = 0; i < RT_ELEMENTS(s_aRoundTrips); i++)
    {
        int      rc;
        char    *pszCfHtml = NULL;
        uint32_t cbCfHtml  = UINT32_MAX/2;
        rc = SharedClipboardWinConvertMIMEToCFHTML(s_aRoundTrips[i].psz, s_aRoundTrips[i].cch + 1, &pszCfHtml, &cbCfHtml);
        if (rc == VINF_SUCCESS)
        {
            if (strlen(pszCfHtml) + 1 != cbCfHtml)
                RTTestIFailed("#%u: SharedClipboardWinConvertMIMEToCFHTML(%s, %#zx,,) returned incorrect length: %#x, actual %#zx",
                              i, s_aRoundTrips[i].psz, s_aRoundTrips[i].cch, cbCfHtml, strlen(pszCfHtml) + 1);

            char     *pszHtml = NULL;
            uint32_t  cbHtml  = UINT32_MAX/4;
            rc = SharedClipboardWinConvertCFHTMLToMIME(pszCfHtml, (uint32_t)strlen(pszCfHtml), &pszHtml, &cbHtml);
            if (rc == VINF_SUCCESS)
            {
                if (strlen(pszHtml) + 1 != cbHtml)
                    RTTestIFailed("#%u: SharedClipboardWinConvertCFHTMLToMIME(%s, %#zx,,) returned incorrect length: %#x, actual %#zx",
                                  i, pszHtml, strlen(pszHtml), cbHtml, strlen(pszHtml) + 1);
                if (strcmp(pszHtml, s_aRoundTrips[i].psz) != 0)
                    RTTestIFailed("#%u: roundtrip for '%s' LB %#zx failed, ended up with '%s'",
                                  i, s_aRoundTrips[i].psz, s_aRoundTrips[i].cch, pszHtml);
                RTMemFree(pszHtml);
            }
            else
                RTTestIFailed("#%u: SharedClipboardWinConvertCFHTMLToMIME(%s, %#zx,,) returned %Rrc, expected VINF_SUCCESS",
                              i, pszCfHtml, strlen(pszCfHtml), rc);
            RTMemFree(pszCfHtml);
        }
        else
            RTTestIFailed("#%u: SharedClipboardWinConvertMIMEToCFHTML(%s, %#zx,,) returned %Rrc, expected VINF_SUCCESS",
                          i, s_aRoundTrips[i].psz, s_aRoundTrips[i].cch, rc);
    }
}

#endif /* RT_OS_WINDOWS */


int main(int argc, char *argv[])
{
    /*
     * Init the runtime, test and say hello.
     */
    const char *pcszExecName;
    NOREF(argc);
    pcszExecName = strrchr(argv[0], '/');
    pcszExecName = pcszExecName ? pcszExecName + 1 : argv[0];
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate(pcszExecName, &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    /*
     * Run the tests.
     */
    testAnnounceAndReadData();
#ifdef RT_OS_WINDOWS
    testHtmlCf();
#endif

    /*
     * Summary
     */
    return RTTestSummaryAndDestroy(hTest);
}


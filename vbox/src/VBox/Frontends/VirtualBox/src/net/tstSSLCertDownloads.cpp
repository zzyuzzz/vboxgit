/* $Id: tstSSLCertDownloads.cpp 57671 2015-09-09 15:42:22Z vboxsync $ */
/** @file
 * IPRT Testcase - Simple cURL testcase.
 */

/*
 * Copyright (C) 2012-2015 Oracle Corporation
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
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/string.h>
#include <iprt/vfslowlevel.h>
#include <iprt/zip.h>
#include <iprt/test.h>


class UINetworkReplyPrivateThreadTestcase
{
public:
    UINetworkReplyPrivateThreadTestcase()
    {
    }

    static void testIt(RTTEST hTest);
};


/*
 * It's a private class we're testing, so we must include the source.
 * Better than emedding the testcase as public function in the class.
 */
#include "UINetworkReply.cpp"


/*static*/ void UINetworkReplyPrivateThreadTestcase::testIt(RTTEST hTest)
{
    QNetworkRequest Dummy;
    UINetworkReplyPrivateThread TestObj(Dummy);

    /*
     * Do the first setup things that UINetworkReplyPrivateThread::run.
     */
    RTTESTI_CHECK_RC(RTHttpCreate(&TestObj.m_hHttp), VINF_SUCCESS);
    RTTESTI_CHECK_RC(TestObj.applyProxyRules(), VINF_SUCCESS);

    /*
     * Duplicate much of downloadMissingCertificates, but making sure we
     * can both get the certificate from the ZIP file(s) and the first
     * PEM URL (as the rest are currently busted).
     */
    RTCRSTORE hStore;
    RTTESTI_CHECK_RC(RTCrStoreCreateInMem(&hStore, RT_ELEMENTS(TestObj.s_aCerts) * 2), VINF_SUCCESS);

    int rc;

    /* ZIP files: */
    for (uint32_t iUrl = 0; iUrl < RT_ELEMENTS(TestObj.s_apszRootsZipUrls); iUrl++)
    {
        RTTestIPrintf(RTTESTLVL_ALWAYS, "URL: %s\n", TestObj.s_apszRootsZipUrls[iUrl]);
        void   *pvRootsZip;
        size_t  cbRootsZip;
        rc = RTHttpGetBinary(TestObj.m_hHttp, TestObj.s_apszRootsZipUrls[iUrl], &pvRootsZip, &cbRootsZip);
        if (RT_SUCCESS(rc))
        {
            for (uint32_t i = 0; i < RT_ELEMENTS(TestObj.s_aCerts); i++)
            {
                UINetworkReplyPrivateThread::CERTINFO const *pInfo;
                pInfo = (UINetworkReplyPrivateThread::CERTINFO const *)TestObj.s_aCerts[i].pvUser;
                if (pInfo->pszZipFile)
                {
                    void  *pvFile;
                    size_t cbFile;
                    RTTESTI_CHECK_RC_BREAK(RTZipPkzipMemDecompress(&pvFile, &cbFile, pvRootsZip, cbRootsZip, pInfo->pszZipFile),
                                           VINF_SUCCESS);
                    RTTESTI_CHECK_RC(TestObj.convertVerifyAndAddPemCertificateToStore(hStore, pvFile, cbFile,
                                                                                      &TestObj.s_aCerts[i]), VINF_SUCCESS);
                    RTMemFree(pvFile);
                }
            }
            RTHttpFreeResponse(pvRootsZip);
        }
        else if (   rc != VERR_HTTP_PROXY_NOT_FOUND
                 && rc != VERR_HTTP_COULDNT_CONNECT)
            RTTestIFailed("%Rrc on '%s'", rc, TestObj.s_apszRootsZipUrls[iUrl]); /* code or link broken */
    }

    /* PEM files: */
    for (uint32_t i = 0; i < RT_ELEMENTS(TestObj.s_aCerts); i++)
    {
        uint32_t iUrl = 0; /* First URL must always work. */
        UINetworkReplyPrivateThread::CERTINFO const *pInfo;
        pInfo = (UINetworkReplyPrivateThread::CERTINFO const *)TestObj.s_aCerts[i].pvUser;
        if (pInfo->apszUrls[iUrl])
        {
            RTTestIPrintf(RTTESTLVL_ALWAYS, "URL: %s\n", pInfo->apszUrls[iUrl]);
            void  *pvResponse;
            size_t cbResponse;
            rc = RTHttpGetBinary(TestObj.m_hHttp, pInfo->apszUrls[iUrl], &pvResponse, &cbResponse);
            if (RT_SUCCESS(rc))
            {
                RTTESTI_CHECK_RC_OK(TestObj.convertVerifyAndAddPemCertificateToStore(hStore, pvResponse, cbResponse,
                                                                                     &TestObj.s_aCerts[i]));
                RTHttpFreeResponse(pvResponse);
            }
            else if (   rc != VERR_HTTP_PROXY_NOT_FOUND
                     && rc != VERR_HTTP_COULDNT_CONNECT)
                RTTestIFailed("%Rrc on '%s'", rc, pInfo->apszUrls[iUrl]); /* code or link broken */
        }
    }

    RTTESTI_CHECK(RTCrStoreRelease(hStore) == 0);

    /*
     * We're done.
     */
    //RTTESTI_CHECK_RC(RTHttpDestroy(TestObj.m_hHttp), VINF_SUCCESS);
    RTHttpDestroy(TestObj.m_hHttp);
    TestObj.m_hHttp = NIL_RTHTTP;
}


int main()
{
    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstCertDownloads", &hTest);
    if (rcExit != RTEXITCODE_SUCCESS)
        return rcExit;
    RTTestBanner(hTest);

    UINetworkReplyPrivateThreadTestcase::testIt(hTest);

    return RTTestSummaryAndDestroy(hTest);
}



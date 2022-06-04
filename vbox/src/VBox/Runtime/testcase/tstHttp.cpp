/* $Id: tstHttp.cpp 45381 2013-04-05 15:00:45Z vboxsync $ */
/** @file
 * IPRT Testcase - Simple cURL testcase.
 */

/*
 * Copyright (C) 2012-2013 Oracle Corporation
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/err.h>
#include <iprt/http.h>
#include <iprt/mem.h>
#include <iprt/file.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/initterm.h>

#define CAFILE_NAME "tstHttp-tempcafile.crt"

int main()
{
    unsigned cErrors = 0;

    RTR3InitExeNoArguments(0);

    RTHTTP hHttp;
    char *pszBuf = NULL;
    PRTSTREAM CAFile = NULL;

    int rc = RTHttpCreate(&hHttp);

    // create certificate file
    if (RT_SUCCESS(rc))
        rc = RTStrmOpen(CAFILE_NAME, "w+b", &CAFile);

    // fetch root CA certificate (new one, often avoided in cert chains by
    // using an intermediate cert which is signed by old root)
    if (RT_SUCCESS(rc))
        rc = RTHttpGet(hHttp,
                       "http://www.verisign.com/repository/roots/root-certificates/PCA-3G5.pem",
                       &pszBuf);
    if (RT_SUCCESS(rc) && pszBuf)
    {
        uint8_t *abSha1;
        size_t  cbSha1;
        uint8_t *abSha512;
        size_t  cbSha512;
        const uint8_t abSha1PCA3G5[] =
        {
            0x4e, 0xb6, 0xd5, 0x78, 0x49, 0x9b, 0x1c, 0xcf, 0x5f, 0x58,
            0x1e, 0xad, 0x56, 0xbe, 0x3d, 0x9b, 0x67, 0x44, 0xa5, 0xe5
        };
        const uint8_t abSha512PCA3G5[] =
        {
            0xd4, 0xf8, 0x10, 0x54, 0x72, 0x77, 0x0a, 0x2d,
            0xe3, 0x17, 0xb3, 0xcf, 0xed, 0x61, 0xae, 0x5c,
            0x5d, 0x3e, 0xde, 0xa1, 0x41, 0x35, 0xb2, 0xdf,
            0x60, 0xe2, 0x61, 0xfe, 0x3a, 0xc1, 0x66, 0xa3,
            0x3c, 0x88, 0x54, 0x04, 0x4f, 0x1d, 0x13, 0x46,
            0xe3, 0x8c, 0x06, 0x92, 0x9d, 0x70, 0x54, 0xc3,
            0x44, 0xeb, 0x2c, 0x74, 0x25, 0x9e, 0x5d, 0xfb,
            0xd2, 0x6b, 0xa8, 0x9a, 0xf0, 0xb3, 0x6a, 0x01
        };
        rc = RTHttpCertDigest(hHttp, pszBuf, strlen(pszBuf),
                              &abSha1, &cbSha1, &abSha512, &cbSha512);
        if (RT_SUCCESS(rc))
        {
            if (cbSha1 != sizeof(abSha1PCA3G5))
            {
                RTPrintf("Wrong SHA1 digest size of PCA-3G5\n");
                rc = VERR_INTERNAL_ERROR;
            }
            else if (memcmp(abSha1PCA3G5, abSha1, cbSha1))
            {
                RTPrintf("Wrong SHA1 digest for PCA-3G5:\n"
                        "Got:      %.*Rhxs\n"
                        "Expected: %.*Rhxs\n",
                        cbSha1, abSha1, sizeof(abSha1PCA3G5), abSha1PCA3G5);
                rc = VERR_INTERNAL_ERROR;
            }
            if (cbSha512 != sizeof(abSha512PCA3G5))
            {
                RTPrintf("Wrong SHA512 digest size of PCA-3G5\n");
                rc = VERR_INTERNAL_ERROR;
            }
            else if (memcmp(abSha512PCA3G5, abSha512, cbSha512))
            {
                RTPrintf("Wrong SHA512 digest for PCA-3G5:\n"
                        "Got:      %.*Rhxs\n"
                        "Expected: %.*Rhxs\n",
                        cbSha512, abSha512, sizeof(abSha512PCA3G5), abSha512PCA3G5);
                rc = VERR_INTERNAL_ERROR;
            }
            RTMemFree(abSha1);
            RTMemFree(abSha512);
            if (RT_SUCCESS(rc))
                rc = RTStrmWrite(CAFile, pszBuf, strlen(pszBuf));
            if (RT_SUCCESS(rc))
                rc = RTStrmWrite(CAFile, RTFILE_LINEFEED, strlen(RTFILE_LINEFEED));
        }
    }
    if (pszBuf)
    {
        RTMemFree(pszBuf);
        pszBuf = NULL;
    }

    // fetch root CA certificate (old one, but still very widely used)
    if (RT_SUCCESS(rc))
        rc = RTHttpGet(hHttp,
                       "http://www.verisign.com/repository/roots/root-certificates/PCA-3.pem",
                       &pszBuf);
    if (RT_SUCCESS(rc) && pszBuf)
    {
        uint8_t *abSha1;
        size_t  cbSha1;
        uint8_t *abSha512;
        size_t  cbSha512;
        const uint8_t abSha1PCA3[] =
        {
            0xa1, 0xdb, 0x63, 0x93, 0x91, 0x6f, 0x17, 0xe4, 0x18, 0x55,
            0x09, 0x40, 0x04, 0x15, 0xc7, 0x02, 0x40, 0xb0, 0xae, 0x6b
        };
        const uint8_t abSha512PCA3[] =
        {
            0xbb, 0xf7, 0x8a, 0x19, 0x9f, 0x37, 0xee, 0xa2,
            0xce, 0xc8, 0xaf, 0xe3, 0xd6, 0x22, 0x54, 0x20,
            0x74, 0x67, 0x6e, 0xa5, 0x19, 0xb7, 0x62, 0x1e,
            0xc1, 0x2f, 0xd5, 0x08, 0xf4, 0x64, 0xc4, 0xc6,
            0xbb, 0xc2, 0xf2, 0x35, 0xe7, 0xbe, 0x32, 0x0b,
            0xde, 0xb2, 0xfc, 0x44, 0x92, 0x5b, 0x8b, 0x9b,
            0x77, 0xa5, 0x40, 0x22, 0x18, 0x12, 0xcb, 0x3d,
            0x0a, 0x67, 0x83, 0x87, 0xc5, 0x45, 0xc4, 0x99
        };
        rc = RTHttpCertDigest(hHttp, pszBuf, strlen(pszBuf),
                              &abSha1, &cbSha1, &abSha512, &cbSha512);
        if (RT_SUCCESS(rc))
        {
            if (cbSha1 != sizeof(abSha1PCA3))
            {
                RTPrintf("Wrong SHA1 digest size of PCA-3\n");
                rc = VERR_INTERNAL_ERROR;
            }
            else if (memcmp(abSha1PCA3, abSha1, cbSha1))
            {
                RTPrintf("Wrong SHA1 digest for PCA-3:\n"
                        "Got:      %.*Rhxs\n"
                        "Expected: %.*Rhxs\n",
                        cbSha1, abSha1, sizeof(abSha1PCA3), abSha1PCA3);
                rc = VERR_INTERNAL_ERROR;
            }
            if (cbSha512 != sizeof(abSha512PCA3))
            {
                RTPrintf("Wrong SHA512 digest size of PCA-3\n");
                rc = VERR_INTERNAL_ERROR;
            }
            else if (memcmp(abSha512PCA3, abSha512, cbSha512))
            {
                RTPrintf("Wrong SHA512 digest for PCA-3:\n"
                        "Got:      %.*Rhxs\n"
                        "Expected: %.*Rhxs\n",
                        cbSha512, abSha512, sizeof(abSha512PCA3), abSha512PCA3);
                rc = VERR_INTERNAL_ERROR;
            }
            RTMemFree(abSha1);
            RTMemFree(abSha512);
            if (RT_SUCCESS(rc))
                rc = RTStrmWrite(CAFile, pszBuf, strlen(pszBuf));
            if (RT_SUCCESS(rc))
                rc = RTStrmWrite(CAFile, RTFILE_LINEFEED, strlen(RTFILE_LINEFEED));
        }
    }
    if (pszBuf)
    {
        RTMemFree(pszBuf);
        pszBuf = NULL;
    }

    // close certificate file
    if (CAFile)
    {
        RTStrmClose(CAFile);
        CAFile = NULL;
    }

    if (RT_SUCCESS(rc))
        rc = RTHttpSetCAFile(hHttp, CAFILE_NAME);

    if (RT_SUCCESS(rc))
        rc = RTHttpGet(hHttp,
                       "https://update.virtualbox.org/query.php?platform=LINUX_32BITS_UBUNTU_12_04&version=4.1.18",
                       &pszBuf);

    if (   RT_FAILURE(rc)
        && rc != VERR_HTTP_COULDNT_CONNECT)
        cErrors++;

    if (RT_FAILURE(rc))
        RTPrintf("Error code: %Rrc\n", rc);
    else
        RTPrintf("Success!\n");
    RTPrintf("Got: %s\n", pszBuf);
    if (pszBuf)
    {
        RTMemFree(pszBuf);
        pszBuf = NULL;
    }

    RTHttpDestroy(hHttp);

//    RTFileDelete(CAFILE_NAME);

    return !!cErrors;
}

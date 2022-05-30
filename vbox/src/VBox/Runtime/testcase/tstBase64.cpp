/* $Id: tstBase64.cpp 16765 2009-02-14 08:49:30Z vboxsync $ */
/** @file
 * IPRT Testcase - Base64.
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
#include <iprt/base64.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/err.h>
#include <iprt/initterm.h>


int main()
{
    int     cErrors = 0;
    char    szOut[0x10000];
    size_t  cchOut = 0;
    RTR3Init();


    /*
     * Test 0.
     */
    static const struct
    {
        const char *pszText;
        size_t      cchText;
        const char *pszEnc;
        size_t      cchEnc;
    } g_aTests[] =
    {
#define TEST_ENTRY(szText, szEnc) { szText, sizeof(szText) - 1, szEnc, sizeof(szEnc) - 1 }
        TEST_ENTRY("Hey", "SGV5"),
        TEST_ENTRY("Base64", "QmFzZTY0"),
        TEST_ENTRY("Call me Ishmael.", "Q2FsbCBtZSBJc2htYWVsLg==")
#undef TEST_ENTRY
    };

    for (unsigned i = 0; i < RT_ELEMENTS(g_aTests); i++)
    {
        int rc = RTBase64Decode(g_aTests[i].pszEnc, szOut, g_aTests[i].cchText, &cchOut, NULL);
        if (RT_FAILURE(rc))
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64Decode -> %Rrc\n", i, rc);
            cErrors++;
        }
        else if (cchOut != g_aTests[i].cchText)
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64Decode returned %zu bytes, expected %zu.\n",
                     i, cchOut, g_aTests[i].cchText);
            cErrors++;
        }
        else if (memcmp(szOut, g_aTests[i].pszText, cchOut))
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64Decode returned:\n%.*s\nexpected:\n%s\n",
                     i, (int)cchOut, szOut, g_aTests[i].pszText);
            cErrors++;
        }

        cchOut = RTBase64DecodedSize(g_aTests[i].pszEnc, NULL);
        if (cchOut != g_aTests[i].cchText)
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64DecodedSize returned %zu bytes, expected %zu.\n",
                     i, cchOut, g_aTests[i].cchText);
            cErrors++;
        }

        rc = RTBase64Encode(g_aTests[i].pszText, g_aTests[i].cchText, szOut, g_aTests[i].cchEnc + 1, &cchOut);
        if (RT_FAILURE(rc))
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64Encode -> %Rrc\n", i, rc);
            cErrors++;
        }
        else if (cchOut != g_aTests[i].cchEnc)
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64Encode returned %zu bytes, expected %zu.\n",
                     i, cchOut, g_aTests[i].cchEnc);
            cErrors++;
        }
        else if (memcmp(szOut, g_aTests[i].pszEnc, cchOut + 1))
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64Encode returned:\n%*s\nexpected:\n%s\n",
                     i, szOut, g_aTests[i].pszText);
            cErrors++;
        }

        cchOut = RTBase64EncodedLength(g_aTests[i].cchText);
        if (cchOut != g_aTests[i].cchEnc)
        {
            RTPrintf("tstBase64: FAILURE - #%u: RTBase64EncodedLength returned %zu bytes, expected %zu.\n",
                     i, cchOut, g_aTests[i].cchEnc);
            cErrors++;
        }

    }

    /*
     * Bigger Test.
     */
    static const char s_szText2[] =
        "Man is distinguished, not only by his reason, but by this singular passion "
        "from other animals, which is a lust of the mind, that by a perseverance of "
        "delight in the continued and indefatigable generation of knowledge, exceeds "
        "the short vehemence of any carnal pleasure."; /* Thomas Hobbes's Leviathan */

    static const char s_szEnc2[] =
        " TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz\n"
        " IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg\n\r"
        " dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu\n"
        " dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo\n\r"
        " ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=\n \n";

    int rc = RTBase64Decode(&s_szEnc2[0], szOut, sizeof(s_szText2), &cchOut, NULL);
    if (RT_FAILURE(rc))
    {
        RTPrintf("tstBase64: FAILURE - RTBase64Decode s_szEnc2 -> %Rrc\n", rc);
        cErrors++;
    }
    else if (cchOut != sizeof(s_szText2) - 1)
    {
        RTPrintf("tstBase64: FAILURE - RTBase64Decode returned %zu bytes, expected %zu.\n",
                 cchOut, sizeof(s_szText2) - 1);
        cErrors++;
    }
    else if (memcmp(szOut, s_szText2, cchOut))
    {
        RTPrintf("tstBase64: FAILURE - RTBase64Decode returned:\n%.*s\nexpected:\n%s\n",
                 (int)cchOut, szOut, s_szText2);
        cErrors++;
    }

    cchOut = RTBase64DecodedSize(s_szEnc2, NULL);
    if (cchOut != sizeof(s_szText2) - 1)
    {
        RTPrintf("tstBase64: FAILURE - RTBase64DecodedSize returned %zu bytes, expected %zu.\n",
                 cchOut, sizeof(s_szText2) - 1);
        cErrors++;
    }

    /*
     * Summary.
     */
    if (!cErrors)
        RTPrintf("tstBase64: SUCCESS\n");
    else
        RTPrintf("tstBase64: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}


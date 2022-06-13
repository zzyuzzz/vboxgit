/* $Id: tstDnDTransferObject.cpp 93115 2022-01-01 11:31:46Z vboxsync $ */
/** @file
 * DnD URI object (DNDTRANSFEROBJECT) tests.
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

#include <iprt/assert.h>
#include <iprt/env.h>
#include <iprt/errcore.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/test.h>

#include <VBox/GuestHost/DragAndDrop.h>


static void tstPaths(RTTEST hTest)
{
    RTTestSub(hTest, "Testing path handling");

    char szBuf[64];

    DNDTRANSFEROBJECT Obj;
    RT_ZERO(Obj);

    /*
     * Initialization handling.
     */
    RTTEST_CHECK(hTest, DnDTransferObjectGetSourcePath(&Obj) == NULL);
    RTTEST_CHECK_RC(hTest, DnDTransferObjectGetDestPathEx(&Obj, DNDTRANSFEROBJPATHSTYLE_TRANSPORT, szBuf, sizeof(szBuf)), VERR_NOT_FOUND);
    RTTEST_CHECK(hTest, DnDTransferObjectGetMode(&Obj) == 0);
    RTTEST_CHECK(hTest, DnDTransferObjectGetSize(&Obj) == 0);
    RTTEST_CHECK(hTest, DnDTransferObjectGetProcessed(&Obj) == 0);
    RTTEST_CHECK(hTest, DnDTransferObjectGetType(&Obj) == DNDTRANSFEROBJTYPE_UNKNOWN);

    /*
     * Paths handling.
     */
    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_FILE, "", "/rel/path/to/dst"));
    RTTestDisableAssertions(hTest);
    RTTEST_CHECK_RC   (hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_FILE, "", "/rel/path/to/dst"), VERR_WRONG_ORDER);
    RTTestRestoreAssertions(hTest);
    DnDTransferObjectReset(&Obj);

    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_FILE, "/src/path1", "dst/path2"));
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetSourcePath(&Obj), "/src/path1/dst/path2") == 0);
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetDestPath(&Obj), "dst/path2") == 0);
    RTTEST_CHECK(hTest,    DnDTransferObjectGetDestPathEx(&Obj, DNDTRANSFEROBJPATHSTYLE_DOS, szBuf, sizeof(szBuf)) == VINF_SUCCESS
                        && RTStrCmp(szBuf, "dst\\path2") == 0);
    DnDTransferObjectReset(&Obj);
    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_FILE, "", "dst/with/ending/slash/"));
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetDestPath(&Obj), "dst/with/ending/slash/") == 0);
    RTTEST_CHECK(hTest,    DnDTransferObjectGetDestPathEx(&Obj, DNDTRANSFEROBJPATHSTYLE_TRANSPORT, szBuf, sizeof(szBuf)) == VINF_SUCCESS
                        && RTStrCmp(szBuf, "dst/with/ending/slash/") == 0);
    DnDTransferObjectReset(&Obj);
    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_DIRECTORY, "", "dst/path2"));
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetSourcePath(&Obj), "dst/path2/") == 0);
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetDestPath(&Obj), "dst/path2/") == 0);
    DnDTransferObjectReset(&Obj);
    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_DIRECTORY, "", "dst\\to\\path2"));
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetSourcePath(&Obj), "dst/to/path2/") == 0);
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetDestPath(&Obj), "dst/to/path2/") == 0);
    DnDTransferObjectReset(&Obj);
    /* Test that the destination does not have a beginning slash. */
    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_DIRECTORY, "/src/path2", "/dst/to/path2/"));
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetSourcePath(&Obj), "/src/path2/dst/to/path2/") == 0);
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetDestPath(&Obj), "dst/to/path2/") == 0);
    DnDTransferObjectReset(&Obj);
    RTTEST_CHECK_RC_OK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_DIRECTORY, "/src/path2", "//////dst/to/path2/"));
    RTTEST_CHECK(hTest, RTStrCmp(DnDTransferObjectGetDestPath(&Obj), "dst/to/path2/") == 0);

    /*
     * Invalid stuff.
     */
    DnDTransferObjectReset(&Obj);
    RTTestDisableAssertions(hTest);
    RTTEST_CHECK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_DIRECTORY, "/src/path3", "../../dst/path3") == VERR_INVALID_PARAMETER);
    RTTEST_CHECK(hTest, DnDTransferObjectInitEx(&Obj, DNDTRANSFEROBJTYPE_DIRECTORY, "/src/../../path3", "dst/path3") == VERR_INVALID_PARAMETER);
    RTTestRestoreAssertions(hTest);

    /*
     * Reset handling.
     */
    DnDTransferObjectReset(&Obj);
    RTTEST_CHECK(hTest, DnDTransferObjectGetSourcePath(&Obj) == NULL);
    RTTEST_CHECK(hTest, DnDTransferObjectGetDestPath(&Obj) == NULL);

    DnDTransferObjectDestroy(&Obj);
    DnDTransferObjectDestroy(&Obj); /* Doing this twice here is intentional. */
}

int main()
{
    /*
     * Init the runtime, test and say hello.
     */
    RTTEST hTest;
    int rc = RTTestInitAndCreate("tstDnDTransferObject", &hTest);
    if (rc)
        return rc;
    RTTestBanner(hTest);

    tstPaths(hTest);

    return RTTestSummaryAndDestroy(hTest);
}


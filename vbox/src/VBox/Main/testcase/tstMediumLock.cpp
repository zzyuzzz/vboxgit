/* $Id: tstMediumLock.cpp 47779 2013-08-15 17:59:50Z vboxsync $ */

/** @file
 *
 * Medium lock test cases.
 */

/*
 * Copyright (C) 2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_ENABLED
#define LOG_GROUP LOG_GROUP_MAIN
#define LOG_INSTANCE NULL
#include <VBox/log.h>

#include <VBox/com/com.h>
#include <VBox/com/ptr.h>
#include <VBox/com/defs.h>
#include <VBox/com/array.h>
#include <VBox/com/string.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/initterm.h>
#include <iprt/test.h>

using namespace com;


#define TEST_RT_SUCCESS(x,y,z) \
    do \
    { \
        int rc = (y); \
        if (RT_FAILURE(rc)) \
            RTTestFailed((x), "%s %Rrc", (z), rc); \
    } while (0)

#define TEST_COM_SUCCESS(x,y,z) \
    do \
    { \
        HRESULT hrc = (y); \
        if (FAILED(hrc)) \
            RTTestFailed((x), "%s %Rhrc", (z), hrc); \
    } while (0)

#define TEST_COM_FAILURE(x,y,z) \
    do \
    { \
        HRESULT hrc = (y); \
        if (SUCCEEDED(hrc)) \
            RTTestFailed((x), "%s", (z)); \
    } while (0)

int main(int argc, char *argv[])
{
    /* Init the runtime without loading the support driver. */
    RTR3InitExe(argc, &argv, 0);

    RTTEST hTest;
    RTEXITCODE rcExit = RTTestInitAndCreate("tstMediumLock", &hTest);
    if (rcExit)
        return rcExit;
    RTTestBanner(hTest);

    bool fComInit = false;
    ComPtr<IVirtualBox> pVirtualBox;
    char szPathTemp[RTPATH_MAX] = "";
    ComPtr<IMedium> pMedium;

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Constructing temp image name");
        TEST_RT_SUCCESS(hTest, RTPathTemp(szPathTemp, sizeof(szPathTemp)), "temp directory");
        RTUUID uuid;
        RTUuidCreate(&uuid);
        char szFile[50];
        RTStrPrintf(szFile, sizeof(szFile), "%RTuuid.vdi", &uuid);
        TEST_RT_SUCCESS(hTest, RTPathAppend(szPathTemp, sizeof(szPathTemp), szFile), "concatenate image name");
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Initializing COM");
        TEST_COM_SUCCESS(hTest, Initialize(), "init");
    }

    if (!RTTestSubErrorCount(hTest))
    {
        fComInit = true;

        RTTestSub(hTest, "Getting VirtualBox reference");
        TEST_COM_SUCCESS(hTest, pVirtualBox.createLocalObject(CLSID_VirtualBox), "vbox reference");
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Creating temp hard disk medium");
        TEST_COM_SUCCESS(hTest, pVirtualBox->CreateHardDisk(Bstr("VDI").raw(), Bstr(szPathTemp).raw(), pMedium.asOutParam()), "create medium");
        if (!pMedium.isNull())
        {
            ComPtr<IProgress> pProgress;
            SafeArray<MediumVariant_T> variant;
            variant.push_back(MediumVariant_Standard);
            TEST_COM_SUCCESS(hTest, pMedium->CreateBaseStorage(_1M, ComSafeArrayAsInParam(variant), pProgress.asOutParam()), "create base storage");
            if (!pProgress.isNull())
                TEST_COM_SUCCESS(hTest, pProgress->WaitForCompletion(30000), "waiting for completion of create");
        }
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Write locks");

        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockWrite(&mediumState), "write lock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting lock write state");
        if (mediumState != MediumState_LockedWrite)
            RTTestFailed(hTest, "wrong lock write medium state %d", mediumState);

        TEST_COM_FAILURE(hTest, pMedium->LockWrite(&mediumState), "nested write lock succeeded");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after nested lock write state");
        if (mediumState != MediumState_LockedWrite)
            RTTestFailed(hTest, "wrong after nested lock write medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockWrite(&mediumState), "write unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting unlock write state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong unlock write medium state %d", mediumState);
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Read locks");

        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockRead(&mediumState), "read lock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting lock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong lock read medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockRead(&mediumState), "nested read lock failed");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after nested lock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong after nested lock read medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockRead(&mediumState), "read nested unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after nested lock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong after nested lock read medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockRead(&mediumState), "read unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting unlock read state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong unlock read medium state %d", mediumState);
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Mixing write and read locks");

        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockWrite(&mediumState), "write lock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting lock write state");
        if (mediumState != MediumState_LockedWrite)
            RTTestFailed(hTest, "wrong lock write medium state %d", mediumState);

        TEST_COM_FAILURE(hTest, pMedium->LockRead(&mediumState), "write+read lock succeeded");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after nested lock write state");
        if (mediumState != MediumState_LockedWrite)
            RTTestFailed(hTest, "wrong after nested lock write medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockWrite(&mediumState), "write unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting unlock write state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong unlock write medium state %d", mediumState);
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Mixing read and write locks");

        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockRead(&mediumState), "read lock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting lock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong lock read medium state %d", mediumState);

        TEST_COM_FAILURE(hTest, pMedium->LockWrite(&mediumState), "read+write lock succeeded");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after nested lock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong after nested lock read medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockRead(&mediumState), "read unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting unlock read state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong unlock read medium state %d", mediumState);
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Write locks, read unlocks");

        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockWrite(&mediumState), "write lock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting lock write state");
        if (mediumState != MediumState_LockedWrite)
            RTTestFailed(hTest, "wrong lock write medium state %d", mediumState);

        TEST_COM_FAILURE(hTest, pMedium->UnlockRead(&mediumState), "read unlocking a write lock succeeded");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after read unlock write state");
        if (mediumState != MediumState_LockedWrite)
            RTTestFailed(hTest, "wrong after read lock write medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockWrite(&mediumState), "write unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting unlock write state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong unlock write medium state %d", mediumState);
    }

    if (!RTTestSubErrorCount(hTest))
    {
        RTTestSub(hTest, "Read locks, write unlocks");

        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->LockRead(&mediumState), "read lock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting lock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong lock write medium state %d", mediumState);

        TEST_COM_FAILURE(hTest, pMedium->UnlockWrite(&mediumState), "write unlocking a read lock succeeded");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting after write unlock read state");
        if (mediumState != MediumState_LockedRead)
            RTTestFailed(hTest, "wrong after write lock read medium state %d", mediumState);

        TEST_COM_SUCCESS(hTest, pMedium->UnlockRead(&mediumState), "read unlock");

        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting unlock read state");
        if (mediumState != MediumState_Created)
            RTTestFailed(hTest, "wrong unlock read medium state %d", mediumState);
    }

    /* Cleanup, also part of the testcase */

    if (!pMedium.isNull())
    {
        RTTestSub(hTest, "Closing medium");
        MediumState_T mediumState = MediumState_NotCreated;
        TEST_COM_SUCCESS(hTest, pMedium->COMGETTER(State)(&mediumState), "getting state");
        if (mediumState == MediumState_Created)
        {
            ComPtr<IProgress> pProgress;
            TEST_COM_SUCCESS(hTest, pMedium->DeleteStorage(pProgress.asOutParam()), "deleting storage");
            if (!pProgress.isNull())
                TEST_COM_SUCCESS(hTest, pProgress->WaitForCompletion(30000), "waiting for completion of delete");
        }
        TEST_COM_SUCCESS(hTest, pMedium->Close(), "closing");
        pMedium.setNull();
    }

    pVirtualBox.setNull();

    /* Make sure that there are no object references alive here, XPCOM does
     * a very bad job at cleaning up such leftovers, spitting out warning
     * messages in a debug build. */

    if (fComInit)
    {
        RTTestIPrintf(RTTESTLVL_DEBUG, "Shutting down COM...\n");
        Shutdown();
    }

    /*
     * Summary.
     */
    return RTTestSummaryAndDestroy(hTest);
}

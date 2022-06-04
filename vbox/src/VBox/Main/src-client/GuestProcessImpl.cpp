
/* $Id: GuestProcessImpl.cpp 42216 2012-07-18 18:09:54Z vboxsync $ */
/** @file
 * VirtualBox Main - XXX.
 */

/*
 * Copyright (C) 2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "GuestProcessImpl.h"
#include "GuestSessionImpl.h"
#include "ConsoleImpl.h"

#include "Global.h"
#include "AutoCaller.h"
#include "Logging.h"
#include "VMMDev.h"

#include <memory> /* For auto_ptr. */

#include <iprt/asm.h>
#include <iprt/getopt.h>
#include <VBox/VMMDev.h>
#include <VBox/com/array.h>


struct GuestProcessTask
{
    GuestProcessTask(GuestProcess *pProcess)
        : mProcess(pProcess) { }

    ~GuestProcessTask(void) { }

    int rc() const { return mRC; }
    bool isOk() const { return RT_SUCCESS(rc()); }

    const ComObjPtr<GuestProcess>    mProcess;

private:
    int                              mRC;
};

struct GuestProcessStartTask : public GuestProcessTask
{
    GuestProcessStartTask(GuestProcess *pProcess)
        : GuestProcessTask(pProcess) { }
};


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(GuestProcess)

HRESULT GuestProcess::FinalConstruct(void)
{
    LogFlowThisFunc(("\n"));

    mData.mExitCode = 0;
    mData.mNextContextID = 0;
    mData.mPID = 0;
    mData.mProcessID = 0;
    mData.mStatus = ProcessStatus_Undefined;
    mData.mStarted = false;

    return BaseFinalConstruct();
}

void GuestProcess::FinalRelease(void)
{
    LogFlowThisFuncEnter();
    uninit();
    BaseFinalRelease();
    LogFlowThisFuncLeave();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

int GuestProcess::init(Console *aConsole, GuestSession *aSession, ULONG aProcessID, const GuestProcessInfo &aProcInfo)
{
    AssertPtrReturn(aSession, VERR_INVALID_POINTER);

    /* Enclose the state transition NotReady->InInit->Ready. */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), VERR_OBJECT_DESTROYED);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    mData.mConsole = aConsole;
    mData.mParent = aSession;
    mData.mPID = 0; /* Will be assigned by the guest OS later. */
    mData.mProcessID = aProcessID;
    mData.mStatus = ProcessStatus_Starting;
    /* Everything else will be set by the actual starting routine. */

    /* Asynchronously start the process on the guest by kicking off a
     * worker thread. */
    std::auto_ptr<GuestProcessStartTask> pTask(new GuestProcessStartTask(this));
    AssertReturn(pTask->isOk(), pTask->rc());

    int rc = RTThreadCreate(NULL, GuestProcess::startProcessThread,
                            (void *)pTask.get(), 0,
                            RTTHREADTYPE_MAIN_WORKER, 0,
                            "gctlPrcStart");
    if (RT_SUCCESS(rc))
    {
        /* task is now owned by startProcessThread(), so release it. */
        pTask.release();

        /* Confirm a successful initialization when it's the case. */
        autoInitSpan.setSucceeded();
    }

    return rc;
}

/**
 * Uninitializes the instance.
 * Called from FinalRelease().
 */
void GuestProcess::uninit(void)
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady. */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

// implementation of public getters/setters for attributes
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP GuestProcess::COMGETTER(Arguments)(ComSafeArrayOut(BSTR, aArguments))
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    CheckComArgOutSafeArrayPointerValid(aArguments);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    com::SafeArray<BSTR> collection(mData.mProcess.mArguments.size());
    size_t s = 0;
    for (ProcessArguments::const_iterator it = mData.mProcess.mArguments.begin();
         it != mData.mProcess.mArguments.end();
         it++, s++)
    {
        Bstr tmp = *it;
        tmp.cloneTo(&collection[s]);
    }

    collection.detachTo(ComSafeArrayOutArg(aArguments));

    LogFlowFuncLeaveRC(S_OK);
    return S_OK;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::COMGETTER(Environment)(ComSafeArrayOut(BSTR, aEnvironment))
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    CheckComArgOutSafeArrayPointerValid(aEnvironment);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    com::SafeArray<BSTR> arguments(mData.mProcess.mEnvironment.Size());
    for (size_t i = 0; i < arguments.size(); i++)
    {
        Bstr tmp = mData.mProcess.mEnvironment.Get(i);
        tmp.cloneTo(&arguments[i]);
    }
    arguments.detachTo(ComSafeArrayOutArg(aEnvironment));

    LogFlowFuncLeaveRC(S_OK);
    return S_OK;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::COMGETTER(ExecutablePath)(BSTR *aExecutablePath)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    mData.mProcess.mCommand.cloneTo(aExecutablePath);

    LogFlowFuncLeaveRC(S_OK);
    return S_OK;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::COMGETTER(ExitCode)(LONG *aExitCode)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aExitCode = mData.mExitCode;

    LogFlowFuncLeaveRC(S_OK);
    return S_OK;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::COMGETTER(Pid)(ULONG *aPID)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aPID = mData.mPID;

    LogFlowFuncLeaveRC(S_OK);
    return S_OK;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::COMGETTER(Status)(ProcessStatus_T *aStatus)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aStatus = mData.mStatus;

    LogFlowFuncLeaveRC(S_OK);
    return S_OK;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

// private methods
/////////////////////////////////////////////////////////////////////////////

/*

    SYNC TO ASK:
    Everything which involves HGCM communication (start, read/write/status(?)/...)
    either can be called synchronously or asynchronously by running in a Main worker
    thread.

    Rules:
        - Only one async operation per process a time can be around.

*/

int GuestProcess::callbackAdd(GuestCtrlCallback *pCallback, ULONG *puContextID)
{
    const ComObjPtr<GuestSession> pSession(mData.mParent);
    Assert(!pSession.isNull());
    ULONG uSessionID = 0;
    HRESULT hr = pSession->COMGETTER(Id)(&uSessionID);
    ComAssertComRC(hr);

    /* Create a new context ID and assign it. */
    int rc = VERR_NOT_FOUND;
    ULONG uNewContextID = 0;
    ULONG uTries = 0;
    for (;;)
    {
        /* Create a new context ID ... */
        uNewContextID = VBOX_GUESTCTRL_CONTEXTID_MAKE(uSessionID,
                                                      mData.mProcessID, mData.mNextContextID++);
        if (uNewContextID == UINT32_MAX)
            mData.mNextContextID = 0;
        /* Is the context ID already used?  Try next ID ... */
        if (!callbackExists(uNewContextID))
        {
            /* Callback with context ID was not found. This means
             * we can use this context ID for our new callback we want
             * to add below. */
            rc = VINF_SUCCESS;
            break;
        }

        if (++uTries == UINT32_MAX)
            break; /* Don't try too hard. */
    }

    if (RT_SUCCESS(rc))
    {
        /* Add callback with new context ID to our callback map. */
        mData.mCallbacks[uNewContextID] = pCallback;
        Assert(mData.mCallbacks.size());

        /* Report back new context ID. */
        if (puContextID)
            *puContextID = uNewContextID;
    }

    return rc;
}

int GuestProcess::callbackDispatcher(uint32_t uContextID, uint32_t uFunction, void *pvData, size_t cbData)
{
    AssertPtrReturn(pvData, VERR_INVALID_POINTER);
    AssertReturn(cbData, VERR_INVALID_PARAMETER);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    GuestCtrlCallbacks::const_iterator it
        = mData.mCallbacks.find(VBOX_GUESTCTRL_CONTEXTID_GET_COUNT(uContextID));
    if (it != mData.mCallbacks.end())
    {
        GuestCtrlCallback *pCallback = it->second;
        AssertPtr(pCallback);

        int rc;

        switch (uFunction)
        {
            case GUEST_DISCONNECTED:
            {
                PCALLBACKDATACLIENTDISCONNECTED pCBData = reinterpret_cast<PCALLBACKDATACLIENTDISCONNECTED>(pvData);
                AssertPtr(pCBData);
                AssertReturn(sizeof(CALLBACKDATACLIENTDISCONNECTED) == cbData, VERR_INVALID_PARAMETER);
                AssertReturn(CALLBACKDATAMAGIC_CLIENT_DISCONNECTED == pCBData->hdr.u32Magic, VERR_INVALID_PARAMETER);

                rc = onGuestDisconnected(); /* Affects all callbacks. */
                break;
            }

            case GUEST_EXEC_SEND_STATUS:
            {
                PCALLBACKDATAEXECSTATUS pCBData = reinterpret_cast<PCALLBACKDATAEXECSTATUS>(pvData);
                AssertPtr(pCBData);
                AssertReturn(sizeof(CALLBACKDATAEXECSTATUS) == cbData, VERR_INVALID_PARAMETER);
                AssertReturn(CALLBACKDATAMAGIC_EXEC_STATUS == pCBData->hdr.u32Magic, VERR_INVALID_PARAMETER);

                rc = onProcessStatusChange(pCBData->u32Status, pCBData->u32Flags, pCBData->u32PID);
                break;
            }

            case GUEST_EXEC_SEND_OUTPUT:
            {
                PCALLBACKDATAEXECOUT pCBData = reinterpret_cast<PCALLBACKDATAEXECOUT>(pvData);
                AssertPtr(pCBData);
                AssertReturn(sizeof(CALLBACKDATAEXECOUT) == cbData, VERR_INVALID_PARAMETER);
                AssertReturn(CALLBACKDATAMAGIC_EXEC_OUT == pCBData->hdr.u32Magic, VERR_INVALID_PARAMETER);

                Assert(mData.mPID == pCBData->u32PID);
                rc = onProcessOutput(pCBData->u32HandleId, pCBData->u32Flags, pCBData->pvData, pCBData->cbData);
                break;
            }

            case GUEST_EXEC_SEND_INPUT_STATUS:
            {
                PCALLBACKDATAEXECINSTATUS pCBData = reinterpret_cast<PCALLBACKDATAEXECINSTATUS>(pvData);
                AssertPtr(pCBData);
                AssertReturn(sizeof(CALLBACKDATAEXECINSTATUS) == cbData, VERR_INVALID_PARAMETER);
                AssertReturn(CALLBACKDATAMAGIC_EXEC_IN_STATUS == pCBData->hdr.u32Magic, VERR_INVALID_PARAMETER);

                Assert(mData.mPID == pCBData->u32PID);
                rc = onProcessInputStatus(pCBData->u32Status, pCBData->u32Flags, pCBData->cbProcessed);
                break;
            }

            default:
                /* Silently ignore not implemented functions. */
                rc = VERR_NOT_IMPLEMENTED;
                break;
        }

        return rc;
    }

    return VERR_NOT_FOUND;
}

bool GuestProcess::callbackExists(ULONG uContextID)
{
    AssertReturn(uContextID, false);

    GuestCtrlCallbacks::const_iterator it = mData.mCallbacks.find(uContextID);
    return (it == mData.mCallbacks.end()) ? false : true;
}

bool GuestProcess::isReady(void)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mData.mStatus == ProcessStatus_Started)
    {
        Assert(mData.mPID); /* PID must not be 0. */
        return true;
    }

    return false;
}

int GuestProcess::onGuestDisconnected(void)
{
    LogFlowFunc(("uPID=%RU32", mData.mPID));

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::onProcessInputStatus(uint32_t uStatus, uint32_t uFlags, uint32_t cbDataProcessed)
{
    LogFlowFunc(("uPID=%RU32, uStatus=%RU32, uFlags=%RU32, cbDataProcessed=%RU32\n",
                 mData.mPID, uStatus, uFlags, cbDataProcessed));

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::onProcessStatusChange(uint32_t uStatus, uint32_t uFlags, uint32_t uPID)
{
    LogFlowFunc(("uPID=%RU32, uStatus=%RU32, uFlags=%RU32\n",
                 uPID, uStatus, uFlags));

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::onProcessOutput(uint32_t uHandle, uint32_t uFlags, void *pvData, uint32_t cbData)
{
    LogFlowFunc(("uPID=%RU32, uHandle=%RU32, uFlags=%RU32, pvData=%p, cbData=%RU32\n",
                 mData.mPID, uHandle, uFlags, pvData, cbData));

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::readData(ULONG uHandle, ULONG uSize, ULONG uTimeoutMS, BYTE *pbData, size_t cbData)
{
    LogFlowFunc(("uPID=%RU32, uHandle=%RU32, uSize=%RU32, uTimeoutMS=%RU32, pbData=%p, cbData=%z\n",
                 mData.mPID, uHandle, uSize, uTimeoutMS, pbData, cbData));
    AssertReturn(uSize, VERR_INVALID_PARAMETER);
    AssertPtrReturn(pbData, VERR_INVALID_POINTER);
    AssertReturn(cbData >= uSize, VERR_INVALID_PARAMETER);

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::startProcess(void)
{
    LogFlowFunc(("aCmd=%s, aTimeoutMS=%RU32, fFlags=%x\n",
                 mData.mProcess.mCommand.c_str(), mData.mProcess.mTimeoutMS, mData.mProcess.mFlags));

    AssertReturn(!mData.mStarted, VERR_ALREADY_EXISTS);

    int rc;
    ULONG uContextID;
    GuestCtrlCallback *pCallbackStart = (GuestCtrlCallback *)RTMemAlloc(sizeof(GuestCtrlCallback));
    if (!pCallbackStart)
        return VERR_NO_MEMORY;

    {
        /* Wait until the caller function (if kicked off by a thread)
         * has returned and continue operation. */
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

        /* Create callback and add it to the map. */
        rc = pCallbackStart->Init(VBOXGUESTCTRLCALLBACKTYPE_EXEC_START);
        if (RT_SUCCESS(rc))
            rc = callbackAdd(pCallbackStart, &uContextID);
    }

    if (RT_SUCCESS(rc))
    {
        Assert(uContextID);

        ComObjPtr<GuestSession> pSession(mData.mParent);
        Assert(!pSession.isNull());

        const GuestCredentials &sessionCreds = pSession->getCredentials();

        /* Prepare arguments. */
        char *pszArgs = NULL;
        size_t cArgs = mData.mProcess.mArguments.size();
        if (cArgs)
        {
            char **papszArgv = (char**)RTMemAlloc(sizeof(char*) * (cArgs + 1));
            AssertReturn(papszArgv, VERR_NO_MEMORY);
            for (size_t i = 0; RT_SUCCESS(rc) && i < cArgs; i++)
                rc = RTStrDupEx(&papszArgv[i], mData.mProcess.mArguments[i].c_str());
            papszArgv[cArgs] = NULL;

            if (RT_SUCCESS(rc))
                rc = RTGetOptArgvToString(&pszArgs, papszArgv, RTGETOPTARGV_CNV_QUOTE_MS_CRT);
        }
        size_t cbArgs = pszArgs ? strlen(pszArgs) + 1 : 0; /* Include terminating zero. */

        /* Prepare environment. */
        void *pvEnv = NULL;
        size_t cbEnv = 0;
        rc = mData.mProcess.mEnvironment.BuildEnvironmentBlock(&pvEnv, &cbEnv, NULL /* cEnv */);

        if (RT_SUCCESS(rc))
        {
            /* Prepare HGCM call. */
            VBOXHGCMSVCPARM paParms[15];
            int i = 0;
            paParms[i++].setUInt32(uContextID);
            paParms[i++].setPointer((void*)mData.mProcess.mCommand.c_str(),
                                    (ULONG)mData.mProcess.mCommand.length() + 1);
            paParms[i++].setUInt32(mData.mProcess.mFlags);
            paParms[i++].setUInt32(mData.mProcess.mArguments.size());
            paParms[i++].setPointer((void*)pszArgs, cbArgs);
            paParms[i++].setUInt32(mData.mProcess.mEnvironment.Size());
            paParms[i++].setUInt32(cbEnv);
            paParms[i++].setPointer((void*)pvEnv, cbEnv);
            paParms[i++].setPointer((void*)sessionCreds.mUser.c_str(), (ULONG)sessionCreds.mUser.length() + 1);
            paParms[i++].setPointer((void*)sessionCreds.mPassword.c_str(), (ULONG)sessionCreds.mPassword.length() + 1);

            /*
             * If the WaitForProcessStartOnly flag is set, we only want to define and wait for a timeout
             * until the process was started - the process itself then gets an infinite timeout for execution.
             * This is handy when we want to start a process inside a worker thread within a certain timeout
             * but let the started process perform lengthly operations then.
             */
            if (mData.mProcess.mFlags & ProcessCreateFlag_WaitForProcessStartOnly)
                paParms[i++].setUInt32(UINT32_MAX /* Infinite timeout */);
            else
                paParms[i++].setUInt32(mData.mProcess.mTimeoutMS);

            const ComObjPtr<Console> pConsole(mData.mConsole);
            Assert(!pConsole.isNull());

            VMMDev *pVMMDev = NULL;
            {
                /* Make sure mParent is valid, so set the read lock while using.
                 * Do not keep this lock while doing the actual call, because in the meanwhile
                 * another thread could request a write lock which would be a bad idea ... */
                AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

                /* Forward the information to the VMM device. */
                pVMMDev = pConsole->getVMMDev();
            }

            LogFlowFunc(("hgcmHostCall numParms=%d, CID=%RU32\n", i, uContextID));
            rc = pVMMDev->hgcmHostCall("VBoxGuestControlSvc", HOST_EXEC_CMD,
                                       i, paParms);
        }

        GuestEnvironment::FreeEnvironmentBlock(pvEnv);
        if (pszArgs)
            RTStrFree(pszArgs);

        if (RT_SUCCESS(rc))
        {
            /*
             * Let's wait for the process being started.
             * Note: Be sure not keeping a AutoRead/WriteLock here.
             */
            rc = pCallbackStart->Wait(mData.mProcess.mTimeoutMS);
        }
    }

    LogFlowFuncLeave();
    return rc;
}

DECLCALLBACK(int) GuestProcess::startProcessThread(RTTHREAD Thread, void *pvUser)
{
    LogFlowFuncEnter();

    std::auto_ptr<GuestProcessStartTask> pTask(static_cast<GuestProcessStartTask*>(pvUser));
    AssertPtr(pTask.get());

    const ComObjPtr<GuestProcess> pProcess(pTask->mProcess);
    Assert(!pProcess.isNull());

    int rc = pProcess->startProcess();

    LogFlowFuncLeave();
    return rc;
}

int GuestProcess::terminateProcess(void)
{
    LogFlowFuncEnter();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::waitFor(uint32_t fFlags, ULONG uTimeoutMS, ProcessWaitReason_T *penmReason)
{
    LogFlowFunc(("fFlags=%x, uTimeoutMS=%RU32, penmReason=%p\n",
                 fFlags, uTimeoutMS, penmReason));

    LogFlowFuncLeave();
    return 0;
}

int GuestProcess::writeData(ULONG uHandle, const BYTE *pbData, size_t cbData, ULONG uTimeoutMS, ULONG *puWritten)
{
    LogFlowFunc(("uPID=%RU32, uHandle=%RU32, pbData=%p, cbData=%z, uTimeoutMS=%RU32, puWritten=%p\n",
                 mData.mPID, uHandle, pbData, cbData, uTimeoutMS, puWritten));
    AssertPtrReturn(pbData, VERR_INVALID_POINTER);
    AssertReturn(pbData, VERR_INVALID_PARAMETER);
    /* Rest is optional. */

    LogFlowFuncLeave();
    return 0;
}

// implementation of public methods
/////////////////////////////////////////////////////////////////////////////

STDMETHODIMP GuestProcess::Read(ULONG aHandle, ULONG aSize, ULONG aTimeoutMS, ComSafeArrayOut(BYTE, aData))
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    if (aSize == 0)
        return setError(E_INVALIDARG, tr("Invalid size to read specified"));
    CheckComArgOutSafeArrayPointerValid(aData);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    com::SafeArray<BYTE> data(aSize);
    int rc = readData(aHandle, aSize, aTimeoutMS, data.raw(), aSize);
    if (RT_SUCCESS(rc))
        data.detachTo(ComSafeArrayOutArg(aData));

    /** @todo Do setError() here. */
    HRESULT hr = RT_SUCCESS(rc) ? S_OK : VBOX_E_IPRT_ERROR;
    LogFlowFuncLeaveRC(hr);

    return hr;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::Terminate(void)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    int rc = terminateProcess();
    /** @todo Do setError() here. */
    HRESULT hr = RT_SUCCESS(rc) ? S_OK : VBOX_E_IPRT_ERROR;
    LogFlowFuncLeaveRC(hr);

    return hr;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::WaitFor(ComSafeArrayIn(ProcessWaitForFlag_T, aFlags), ULONG aTimeoutMS, ProcessWaitReason_T *aReason)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aReason);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    uint32_t fFlags = ProcessWaitForFlag_None;
    com::SafeArray<ProcessWaitForFlag_T> flags(ComSafeArrayInArg(aFlags));
    for (size_t i = 0; i < flags.size(); i++)
        fFlags |= flags[i];

    int rc = waitFor(fFlags, aTimeoutMS, aReason);
    /** @todo Do setError() here. */
    HRESULT hr = RT_SUCCESS(rc) ? S_OK : VBOX_E_IPRT_ERROR;
    LogFlowFuncLeaveRC(hr);

    return hr;
#endif /* VBOX_WITH_GUEST_CONTROL */
}

STDMETHODIMP GuestProcess::Write(ULONG aHandle, ComSafeArrayIn(BYTE, aData), ULONG aTimeoutMS, ULONG *aWritten)
{
#ifndef VBOX_WITH_GUEST_CONTROL
    ReturnComNotImplemented();
#else
    LogFlowThisFuncEnter();

    CheckComArgOutPointerValid(aWritten);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    com::SafeArray<BYTE> data(ComSafeArrayInArg(aData));
    int rc = writeData(aHandle, data.raw(), data.size(), aTimeoutMS, aWritten);
    /** @todo Do setError() here. */
    HRESULT hr = RT_SUCCESS(rc) ? S_OK : VBOX_E_IPRT_ERROR;
    LogFlowFuncLeaveRC(hr);

    return hr;
#endif /* VBOX_WITH_GUEST_CONTROL */
}


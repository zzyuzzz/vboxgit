/* $Id: VirtualBoxClientImpl.cpp 34421 2010-11-26 17:44:43Z vboxsync $ */
/** @file
 * VirtualBox COM class implementation
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VirtualBoxClientImpl.h"

#include "AutoCaller.h"
#include "VBoxEvents.h"
#include "Logging.h"

#include <iprt/thread.h>
#include <iprt/critsect.h>
#include <iprt/semaphore.h>
#include <iprt/cpp/utils.h>


/** Waiting time between probing whether VBoxSVC is alive. */
#define VBOXCLIENT_DEFAULT_INTERVAL 30000


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

DEFINE_EMPTY_CTOR_DTOR(VirtualBoxClient)

HRESULT VirtualBoxClient::FinalConstruct()
{
    return init();
}

void VirtualBoxClient::FinalRelease()
{
    uninit();
}

// public initializer/uninitializer for internal purposes only
/////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the VirtualBoxClient object.
 *
 * @returns COM result indicator
 */
HRESULT VirtualBoxClient::init()
{
    LogFlowThisFunc(("\n"));

    HRESULT rc;
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    rc = unconst(mData.m_pVirtualBox).createLocalObject(CLSID_VirtualBox);
    AssertComRCReturnRC(rc);

    rc = unconst(mData.m_pEventSource).createObject();
    AssertComRCReturnRC(rc);
    rc = mData.m_pEventSource->init(static_cast<IVirtualBoxClient *>(this));
    AssertComRCReturnRC(rc);

    /* Setting up the VBoxSVC watcher thread. If anything goes wrong here it
     * is not considered important enough to cause any sort of visible
     * failure. The monitoring will not be done, but that's all. */
    mData.m_ThreadWatcher = NIL_RTTHREAD;
    int vrc = RTSemEventCreate(&mData.m_SemEvWatcher);
    AssertRC(vrc);
    if (RT_SUCCESS(vrc))
    {
        vrc = RTThreadCreate(&mData.m_ThreadWatcher, SVCWatcherThread,
                             this, 0, RTTHREADTYPE_INFREQUENT_POLLER,
                             RTTHREADFLAGS_WAITABLE, "VBoxSVCWatcher");
        AssertRC(vrc);
    }
    else
    {
        RTSemEventDestroy(mData.m_SemEvWatcher);
        mData.m_SemEvWatcher = NIL_RTSEMEVENT;
    }

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return rc;
}

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void VirtualBoxClient::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    unconst(mData.m_pVirtualBox).setNull();

    if (mData.m_ThreadWatcher != NIL_RTTHREAD)
    {
        /* Signal the event semaphore and wait for the thread to terminate.
         * if it hangs for some reason exit anyway, this can cause a crash
         * though as the object will no longer be available. */
        RTSemEventSignal(mData.m_SemEvWatcher);
        RTThreadWait(mData.m_ThreadWatcher, 30000, NULL);
        mData.m_ThreadWatcher = NIL_RTTHREAD;
        RTSemEventDestroy(mData.m_SemEvWatcher);
        mData.m_SemEvWatcher = NIL_RTSEMEVENT;
    }
}

// IVirtualBoxClient properties
/////////////////////////////////////////////////////////////////////////////

/**
 * Returns a reference to the VirtualBox object.
 *
 * @returns COM status code
 * @param   aVirtualBox Address of result variable.
 */
STDMETHODIMP VirtualBoxClient::COMGETTER(VirtualBox)(IVirtualBox **aVirtualBox)
{
    CheckComArgOutPointerValid(aVirtualBox);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* this is const, no need to lock */
    mData.m_pVirtualBox.queryInterfaceTo(aVirtualBox);
    return S_OK;
}

/**
 * Create a new Session object and return a reference to it.
 *
 * @returns COM status code
 * @param   aSession    Address of result variable.
 */
STDMETHODIMP VirtualBoxClient::COMGETTER(Session)(ISession **aSession)
{
    HRESULT rc;
    CheckComArgOutPointerValid(aSession);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* this is not stored in this object, no need to lock */
    ComPtr<ISession> pSession;
    rc = pSession.createInprocObject(CLSID_Session);
    if (SUCCEEDED(rc))
        pSession.queryInterfaceTo(aSession);

    return rc;
}

/**
 * Return reference to the EventSource associated with this object.
 *
 * @returns COM status code
 * @param   aEventSource    Address of result variable.
 */
STDMETHODIMP VirtualBoxClient::COMGETTER(EventSource)(IEventSource **aEventSource)
{
    CheckComArgOutPointerValid(aEventSource);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* this is const, no need to lock */
    mData.m_pEventSource.queryInterfaceTo(aEventSource);

    return mData.m_pEventSource.isNull() ? E_FAIL : S_OK;
}

// private methods
/////////////////////////////////////////////////////////////////////////////

/*static*/
DECLCALLBACK(int) VirtualBoxClient::SVCWatcherThread(RTTHREAD ThreadSelf,
                                                     void *pvUser)
{
    NOREF(ThreadSelf);
    Assert(pvUser);
    VirtualBoxClient *pThis = (VirtualBoxClient *)pvUser;
    RTSEMEVENT sem = pThis->mData.m_SemEvWatcher;
    RTMSINTERVAL cMillies = VBOXCLIENT_DEFAULT_INTERVAL;
    int vrc;

    /* The likelihood of early crashes are high, so start with a short wait. */
    vrc = RTSemEventWait(sem, cMillies / 2);

    /* As long as the waiting times out keep retrying the wait. */
    while (RT_FAILURE(vrc))
    {
        {
            HRESULT rc = S_OK;
            ComPtr<IVirtualBox> pV = pThis->mData.m_pVirtualBox;
            if (!pV.isNull())
            {
                ULONG rev;
                rc = pV->COMGETTER(Revision)(&rev);
                if (FAILED_DEAD_INTERFACE(rc))
                {
                    LogRel(("VirtualBoxClient: detected unresponsive VBoxSVC (rc=%Rhrc)", rc));
                    fireVBoxSVCUnavailableEvent(pThis->mData.m_pEventSource);

                    /* Throw away the VirtualBox reference, it's no longer
                     * usable as VBoxSVC terminated in the mean time. */
                    unconst(pThis->mData.m_pVirtualBox).setNull();
                }
            }
            else
            {
                /* Try to get a new VirtualBox reference straight away, and if
                 * this fails use an increased waiting time as very frequent
                 * restart attempts in some wedged config can cause high CPU
                 * and disk load. */
                rc = unconst(pThis->mData.m_pVirtualBox).createLocalObject(CLSID_VirtualBox);
                if (FAILED(rc))
                    cMillies = 3 * VBOXCLIENT_DEFAULT_INTERVAL;
                else
                    cMillies = VBOXCLIENT_DEFAULT_INTERVAL;
            }
        }
        vrc = RTSemEventWait(sem, cMillies);
    }
    return 0;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */

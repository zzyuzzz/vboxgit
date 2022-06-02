/* $Id: UIConsoleEventHandler.cpp 31816 2010-08-20 12:51:55Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIConsoleEventHandler class implementation
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

/* Local includes */
#include "UIConsoleEventHandler.h"
#include "UIMainEventListener.h"
#include "VBoxGlobal.h"
#include "UISession.h"

#ifdef Q_WS_MAC
# include "VBoxUtils.h"
#endif /* Q_WS_MAC */

/* Global includes */
//#include <iprt/thread.h>
//#include <iprt/stream.h>

/* static */
UIConsoleEventHandler *UIConsoleEventHandler::m_pInstance = 0;

/* static */
UIConsoleEventHandler* UIConsoleEventHandler::instance(UISession *pSession /* = 0 */)
{
    if (!m_pInstance)
        m_pInstance = new UIConsoleEventHandler(pSession);
    return m_pInstance;
}

/* static */
void UIConsoleEventHandler::destroy()
{
    if (m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = 0;
    }
}

UIConsoleEventHandler::UIConsoleEventHandler(UISession *pSession)
  : m_pSession(pSession)
{
    Assert(pSession);

//    RTPrintf("Self add: %RTthrd\n", RTThreadSelf());
    UIMainEventListener *pListener = new UIMainEventListener(this);
    m_mainEventListener = CEventListener(pListener);
    QVector<KVBoxEventType> events;
    events
        << KVBoxEventType_OnMousePointerShapeChanged
        << KVBoxEventType_OnMouseCapabilityChanged
        << KVBoxEventType_OnKeyboardLedsChanged
        << KVBoxEventType_OnStateChanged
        << KVBoxEventType_OnAdditionsStateChanged
        << KVBoxEventType_OnNetworkAdapterChanged
        << KVBoxEventType_OnMediumChanged
        << KVBoxEventType_OnUSBControllerChanged
        << KVBoxEventType_OnUSBDeviceStateChanged
        << KVBoxEventType_OnSharedFolderChanged
        << KVBoxEventType_OnRuntimeError
        << KVBoxEventType_OnCanShowWindow
        << KVBoxEventType_OnShowWindow;

    const CConsole &console = m_pSession->session().GetConsole();
    console.GetEventSource().RegisterListener(m_mainEventListener, events, TRUE);
    AssertWrapperOk(console);

    connect(pListener, SIGNAL(sigMousePointerShapeChange(bool, bool, QPoint, QSize, QVector<uint8_t>)),
            this, SIGNAL(sigMousePointerShapeChange(bool, bool, QPoint, QSize, QVector<uint8_t>)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigMouseCapabilityChange(bool, bool, bool)),
            this, SIGNAL(sigMouseCapabilityChange(bool, bool, bool)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigKeyboardLedsChangeEvent(bool, bool, bool)),
            this, SIGNAL(sigKeyboardLedsChangeEvent(bool, bool, bool)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigStateChange(KMachineState)),
            this, SIGNAL(sigStateChange(KMachineState)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigAdditionsChange()),
            this, SIGNAL(sigAdditionsChange()),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigNetworkAdapterChange(CNetworkAdapter)),
            this, SIGNAL(sigNetworkAdapterChange(CNetworkAdapter)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigMediumChange(CMediumAttachment)),
            this, SIGNAL(sigMediumChange(CMediumAttachment)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigUSBControllerChange()),
            this, SIGNAL(sigUSBControllerChange()),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigUSBDeviceStateChange(CUSBDevice, bool, CVirtualBoxErrorInfo)),
            this, SIGNAL(sigUSBDeviceStateChange(CUSBDevice, bool, CVirtualBoxErrorInfo)),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigSharedFolderChange()),
            this, SIGNAL(sigSharedFolderChange()),
            Qt::QueuedConnection);

    connect(pListener, SIGNAL(sigRuntimeError(bool, QString, QString)),
            this, SIGNAL(sigRuntimeError(bool, QString, QString)),
            Qt::QueuedConnection);

    /* This is a vetoable event, so we have to respond to the event and have to
     * use a direct connection therefor. */
    connect(pListener, SIGNAL(sigCanShowWindow(bool&, QString&)),
            this, SLOT(sltCanShowWindow(bool&, QString&)),
            Qt::DirectConnection);

    /* This returns a winId, so we have to respond to the event and have to use
     * a direct connection therefor. */
    connect(pListener, SIGNAL(sigShowWindow(LONG64&)),
            this, SLOT(sltShowWindow(LONG64&)),
            Qt::DirectConnection);
}

UIConsoleEventHandler::~UIConsoleEventHandler()
{
    const CConsole &console = m_pSession->session().GetConsole();
    console.GetEventSource().UnregisterListener(m_mainEventListener);
    AssertWrapperOk(console);
}

void UIConsoleEventHandler::sltCanShowWindow(bool & /* fVeto */, QString & /* strReason */)
{
    /* No veto, so nothing for us to do. */
}

void UIConsoleEventHandler::sltShowWindow(LONG64 &winId)
{
#ifdef Q_WS_MAC
        /* Let's try the simple approach first - grab the focus.
         * Getting a window out of the dock (minimized or whatever it's called)
         * needs to be done on the GUI thread, so post it a note: */
        winId = 0;
        if (::darwinSetFrontMostProcess())
            emit sigShowWindow();
        else
        {
            /* It failed for some reason, send the other process our PSN so it can try.
             * (This is just a precaution should Mac OS X start imposing the same sensible
             * focus stealing restrictions that other window managers implement). */
            winId = ::darwinGetCurrentProcessId();
        }
#else /* Q_WS_MAC */
        /* Return the ID of the top-level console window. */
        winId = (ULONG64)m_pSession->winId();
#endif /* !Q_WS_MAC */
}


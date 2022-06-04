/* $Id: UIExtraDataEventHandler.cpp 44453 2013-01-30 10:21:33Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIExtraDataEventHandler class implementation
 */

/*
 * Copyright (C) 2010-2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QMutex>

/* GUI includes: */
#include "UIExtraDataEventHandler.h"
#include "UIMainEventListener.h"
#include "VBoxGlobal.h"
#include "VBoxGlobalSettings.h"
#include "UIActionPool.h"

/* COM includes: */
#include "COMEnums.h"
#include "CEventSource.h"

class UIExtraDataEventHandlerPrivate: public QObject
{
    Q_OBJECT;

public:

    UIExtraDataEventHandlerPrivate(QObject *pParent = 0)
        : QObject(pParent)
#ifdef VBOX_GUI_WITH_SYSTRAY
        , m_fIsTrayIconOwner(false)
#endif /* VBOX_GUI_WITH_SYSTRAY */
    {}

public slots:

    void sltExtraDataCanChange(QString strId, QString strKey, QString strValue, bool &fVeto, QString &strVetoReason)
    {
        if (QUuid(strId).isNull())
        {
            /* it's a global extra data key someone wants to change */
            if (strKey.startsWith("GUI/"))
            {
#ifdef VBOX_GUI_WITH_SYSTRAY
                if (strKey == GUI_TrayIconWinID)
                {
                    if (m_fIsTrayIconOwner)
                    {
                        if (!(strValue.isEmpty() ||
                              strValue == QString("%1")
                              .arg((qulonglong)vboxGlobal().mainWindow()->winId())))
                            fVeto = true;
                    }
                    return;
                }
#endif /* VBOX_GUI_WITH_SYSTRAY */
                /* Try to set the global setting to check its syntax */
                VBoxGlobalSettings gs(false /* non-null */);
                if (gs.setPublicProperty (strKey, strValue))
                {
                    /* this is a known GUI property key */
                    if (!gs)
                    {
                        strVetoReason = gs.lastError();
                        /* disallow the change when there is an error*/
                        fVeto = true;
                    }
                    return;
                }
            }
        }
    }

    void sltExtraDataChange(QString strId, QString strKey, QString strValue)
    {
        if (QUuid(strId).isNull())
        {
            if (strKey.startsWith ("GUI/"))
            {
                if (strKey == GUI_LanguageId)
                    emit sigGUILanguageChange(strValue);
                if (strKey == GUI_Input_SelectorShortcuts && gActionPool->type() == UIActionPoolType_Selector)
                    emit sigSelectorShortcutsChanged();
                if (strKey == GUI_Input_MachineShortcuts && gActionPool->type() == UIActionPoolType_Runtime)
                    emit sigMachineShortcutsChanged();
#ifdef VBOX_GUI_WITH_SYSTRAY
                if (strKey == GUI_MainWindowCount)
                    emit sigMainWindowCountChange(strValue.toInt());
                if (strKey == GUI_TrayIconWinID)
                {
                    if (strValue.isEmpty())
                    {
                        m_fIsTrayIconOwner = false;
                        emit sigCanShowTrayIcon(true);
                    }
                    else if (strValue == QString("%1")
                             .arg((qulonglong)vboxGlobal().mainWindow()->winId()))
                    {
                        m_fIsTrayIconOwner = true;
                        emit sigCanShowTrayIcon(true);
                    }
                    else
                        emit sigCanShowTrayIcon(false);
                }
                if (strKey == GUI_TrayIconEnabled)
                    emit sigTrayIconChange((strValue.toLower() == "true") ? true : false);
#endif /* VBOX_GUI_WITH_SYSTRAY */
#ifdef Q_WS_MAC
                if (strKey == GUI_PresentationModeEnabled)
                {
                    /* Default to true if it is an empty value */
                    QString testStr = strValue.toLower();
                    bool f = (testStr.isEmpty() || testStr == "false");
                    emit sigPresentationModeChange(f);
                }
#endif /* Q_WS_MAC */

                m_mutex.lock();
                vboxGlobal().settings().setPublicProperty(strKey, strValue);
                m_mutex.unlock();
                Assert(!!vboxGlobal().settings());
            }
        }
#ifdef Q_WS_MAC
        else if (vboxGlobal().isVMConsoleProcess())
        {
            /* Check for the currently running machine */
            if (strId == vboxGlobal().managedVMUuid())
            {
                if (   strKey == GUI_RealtimeDockIconUpdateEnabled
                    || strKey == GUI_RealtimeDockIconUpdateMonitor)
                {
                    bool f = strValue.toLower() == "false" ? false : true;
                    emit sigDockIconAppearanceChange(f);
                }
            }
        }
#endif /* Q_WS_MAC */
    }

signals:

    void sigGUILanguageChange(QString strLang);
    void sigSelectorShortcutsChanged();
    void sigMachineShortcutsChanged();
#ifdef VBOX_GUI_WITH_SYSTRAY
    void sigMainWindowCountChange(int count);
    void sigCanShowTrayIcon(bool fEnabled);
    void sigTrayIconChange(bool fEnabled);
#endif /* VBOX_GUI_WITH_SYSTRAY */
#ifdef RT_OS_DARWIN
    void sigPresentationModeChange(bool fEnabled);
    void sigDockIconAppearanceChange(bool fEnabled);
#endif /* RT_OS_DARWIN */

private:

    /** protects #OnExtraDataChange() */
    QMutex m_mutex;

    /* Private member vars */
#ifdef VBOX_GUI_WITH_SYSTRAY
    bool m_fIsTrayIconOwner;
#endif /* VBOX_GUI_WITH_SYSTRAY */
};

/* static */
UIExtraDataEventHandler *UIExtraDataEventHandler::m_pInstance = 0;

/* static */
UIExtraDataEventHandler* UIExtraDataEventHandler::instance()
{
    if (!m_pInstance)
        m_pInstance = new UIExtraDataEventHandler();
    return m_pInstance;
}

/* static */
void UIExtraDataEventHandler::destroy()
{
    if (m_pInstance)
    {
        delete m_pInstance;
        m_pInstance = 0;
    }
}

UIExtraDataEventHandler::UIExtraDataEventHandler()
  : m_pHandler(new UIExtraDataEventHandlerPrivate(this))
{
//    RTPrintf("Self add: %RTthrd\n", RTThreadSelf());
    const CVirtualBox &vbox = vboxGlobal().virtualBox();
    ComObjPtr<UIMainEventListenerImpl> pListener;
    pListener.createObject();
    pListener->init(new UIMainEventListener(), this);
    m_mainEventListener = CEventListener(pListener);
    QVector<KVBoxEventType> events;
    events
        << KVBoxEventType_OnExtraDataCanChange
        << KVBoxEventType_OnExtraDataChanged;

    vbox.GetEventSource().RegisterListener(m_mainEventListener, events, TRUE);
    AssertWrapperOk(vbox);

    /* This is a vetoable event, so we have to respond to the event and have to
     * use a direct connection therefor. */
    connect(pListener->getWrapped(), SIGNAL(sigExtraDataCanChange(QString, QString, QString, bool&, QString&)),
            m_pHandler, SLOT(sltExtraDataCanChange(QString, QString, QString, bool&, QString&)),
            Qt::DirectConnection);

    /* Use a direct connection to the helper class. */
    connect(pListener->getWrapped(), SIGNAL(sigExtraDataChange(QString, QString, QString)),
            m_pHandler, SLOT(sltExtraDataChange(QString, QString, QString)),
            Qt::DirectConnection);

    /* UI signals */
    connect(m_pHandler, SIGNAL(sigGUILanguageChange(QString)),
            this, SIGNAL(sigGUILanguageChange(QString)),
            Qt::QueuedConnection);

    connect(m_pHandler, SIGNAL(sigSelectorShortcutsChanged()),
            this, SIGNAL(sigSelectorShortcutsChanged()),
            Qt::QueuedConnection);

    connect(m_pHandler, SIGNAL(sigMachineShortcutsChanged()),
            this, SIGNAL(sigMachineShortcutsChanged()),
            Qt::QueuedConnection);

#ifdef VBOX_GUI_WITH_SYSTRAY
    connect(m_pHandler, SIGNAL(sigMainWindowCountChange(int)),
            this, SIGNAL(sigMainWindowCountChange(int)),
            Qt::QueuedConnection);

    connect(m_pHandler, SIGNAL(sigCanShowTrayIcon(bool)),
            this, SIGNAL(sigCanShowTrayIcon(bool)),
            Qt::QueuedConnection);

    connect(m_pHandler, SIGNAL(sigTrayIconChange(bool)),
            this, SIGNAL(sigTrayIconChange(bool)),
            Qt::QueuedConnection);
#endif /* VBOX_GUI_WITH_SYSTRAY */

#ifdef Q_WS_MAC
    connect(m_pHandler, SIGNAL(sigPresentationModeChange(bool)),
            this, SIGNAL(sigPresentationModeChange(bool)),
            Qt::QueuedConnection);

    connect(m_pHandler, SIGNAL(sigDockIconAppearanceChange(bool)),
            this, SIGNAL(sigDockIconAppearanceChange(bool)),
            Qt::QueuedConnection);
#endif /* Q_WS_MAC */
}

UIExtraDataEventHandler::~UIExtraDataEventHandler()
{
    const CVirtualBox &vbox = vboxGlobal().virtualBox();
    vbox.GetEventSource().UnregisterListener(m_mainEventListener);
}

#include "UIExtraDataEventHandler.moc"

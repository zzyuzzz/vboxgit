/* $Id: UIMachineLogicScale.cpp 52130 2014-07-22 15:52:00Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineLogicScale class implementation.
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

#ifndef Q_WS_MAC
/* Qt includes: */
# include <QTimer>
#endif /* !Q_WS_MAC */

/* GUI includes: */
#include "VBoxGlobal.h"
#include "UIMessageCenter.h"
#include "UISession.h"
#include "UIActionPoolRuntime.h"
#include "UIMachineLogicScale.h"
#include "UIMachineWindow.h"
#ifndef Q_WS_MAC
# include "QIMenu.h"
#else /* Q_WS_MAC */
# include "VBoxUtils.h"
#endif /* Q_WS_MAC */

UIMachineLogicScale::UIMachineLogicScale(QObject *pParent, UISession *pSession)
    : UIMachineLogic(pParent, pSession, UIVisualStateType_Scale)
#ifndef Q_WS_MAC
    , m_pPopupMenu(0)
#endif /* !Q_WS_MAC */
{
}

bool UIMachineLogicScale::checkAvailability()
{
    /* Take the toggle hot key from the menu item.
     * Since VBoxGlobal::extractKeyFromActionText gets exactly
     * the linked key without the 'Host+' part we are adding it here. */
    QString strHotKey = QString("Host+%1")
        .arg(VBoxGlobal::extractKeyFromActionText(gActionPool->action(UIActionIndexRuntime_Toggle_Scale)->text()));
    Assert(!strHotKey.isEmpty());

    /* Show the info message. */
    if (!msgCenter().confirmGoingScale(strHotKey))
        return false;

    return true;
}

#ifndef Q_WS_MAC
void UIMachineLogicScale::sltInvokePopupMenu()
{
    /* Popup main-menu if present: */
    if (m_pPopupMenu && !m_pPopupMenu->isEmpty())
    {
        m_pPopupMenu->popup(activeMachineWindow()->geometry().center());
        QTimer::singleShot(0, m_pPopupMenu, SLOT(sltHighlightFirstAction()));
    }
}
#endif /* !Q_WS_MAC */

void UIMachineLogicScale::prepareActionGroups()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionGroups();

    /* Take care of view-action toggle state: */
    UIAction *pActionScale = gActionPool->action(UIActionIndexRuntime_Toggle_Scale);
    if (!pActionScale->isChecked())
    {
        pActionScale->blockSignals(true);
        pActionScale->setChecked(true);
        pActionScale->blockSignals(false);
        pActionScale->update();
    }
}

void UIMachineLogicScale::prepareActionConnections()
{
    /* Call to base-class: */
    UIMachineLogic::prepareActionConnections();

    /* "View" actions connections: */
    connect(gActionPool->action(UIActionIndexRuntime_Toggle_Scale), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToNormal()));
    connect(gActionPool->action(UIActionIndexRuntime_Toggle_Fullscreen), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToFullscreen()));
    connect(gActionPool->action(UIActionIndexRuntime_Toggle_Seamless), SIGNAL(triggered(bool)),
            this, SLOT(sltChangeVisualStateToSeamless()));
}

void UIMachineLogicScale::prepareMachineWindows()
{
    /* Do not create machine-window(s) if they created already: */
    if (isMachineWindowsCreated())
        return;

#ifdef Q_WS_MAC // TODO: Is that really need here?
    /* We have to make sure that we are getting the front most process.
     * This is necessary for Qt versions > 4.3.3: */
    ::darwinSetFrontMostProcess();
#endif /* Q_WS_MAC */

    /* Get monitors count: */
    ulong uMonitorCount = session().GetMachine().GetMonitorCount();
    /* Create machine window(s): */
    for (ulong uScreenId = 0; uScreenId < uMonitorCount; ++ uScreenId)
        addMachineWindow(UIMachineWindow::create(this, uScreenId));
    /* Order machine window(s): */
    for (ulong uScreenId = uMonitorCount; uScreenId > 0; -- uScreenId)
        machineWindows()[uScreenId - 1]->raise();

    /* Mark machine-window(s) created: */
    setMachineWindowsCreated(true);
}

#ifndef Q_WS_MAC
void UIMachineLogicScale::prepareMenu()
{
    /* Call to base-class: */
    UIMachineLogic::prepareMenu();

    /* Prepare popup-menu: */
    m_pPopupMenu = new QIMenu;
    AssertPtrReturnVoid(m_pPopupMenu);
    {
        /* Prepare popup-menu: */
        foreach (QMenu *pMenu, menus())
            m_pPopupMenu->addMenu(pMenu);
    }
}
#endif /* !Q_WS_MAC */

#ifndef Q_WS_MAC
void UIMachineLogicScale::cleanupMenu()
{
    /* Cleanup popup-menu: */
    delete m_pPopupMenu;
    m_pPopupMenu = 0;

    /* Call to base-class: */
    UIMachineLogic::cleanupMenu();
}
#endif /* !Q_WS_MAC */

void UIMachineLogicScale::cleanupMachineWindows()
{
    /* Do not destroy machine-window(s) if they destroyed already: */
    if (!isMachineWindowsCreated())
        return;

    /* Mark machine-window(s) destroyed: */
    setMachineWindowsCreated(false);

    /* Cleanup machine-window(s): */
    foreach (UIMachineWindow *pMachineWindow, machineWindows())
        UIMachineWindow::destroy(pMachineWindow);
}

void UIMachineLogicScale::cleanupActionConnections()
{
    /* "View" actions disconnections: */
    disconnect(gActionPool->action(UIActionIndexRuntime_Toggle_Scale), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToNormal()));
    disconnect(gActionPool->action(UIActionIndexRuntime_Toggle_Fullscreen), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToFullscreen()));
    disconnect(gActionPool->action(UIActionIndexRuntime_Toggle_Seamless), SIGNAL(triggered(bool)),
               this, SLOT(sltChangeVisualStateToSeamless()));

    /* Call to base-class: */
    UIMachineLogic::cleanupActionConnections();

}

void UIMachineLogicScale::cleanupActionGroups()
{
    /* Take care of view-action toggle state: */
    UIAction *pActionScale = gActionPool->action(UIActionIndexRuntime_Toggle_Scale);
    if (pActionScale->isChecked())
    {
        pActionScale->blockSignals(true);
        pActionScale->setChecked(false);
        pActionScale->blockSignals(false);
        pActionScale->update();
    }

    /* Call to base-class: */
    UIMachineLogic::cleanupActionGroups();
}


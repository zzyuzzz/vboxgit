/* $Id: UIVirtualMachineItem.cpp 82878 2020-01-27 17:30:46Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVirtualMachineItem class implementation.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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
#include <QApplication>
#include <QFileInfo>
#include <QIcon>

/* GUI includes: */
#include "UIVirtualMachineItem.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIExtraDataManager.h"
#ifdef VBOX_WS_MAC
# include <ApplicationServices/ApplicationServices.h>
#endif /* VBOX_WS_MAC */

/* COM includes: */
#include "CSnapshot.h"


/*********************************************************************************************************************************
*   Class UIVirtualMachineItem implementation.                                                                                   *
*********************************************************************************************************************************/

UIVirtualMachineItem::UIVirtualMachineItem(const CMachine &comMachine)
    : m_comMachine(comMachine)
    , m_fAccessible(false)
    , m_fHasDetails(false)
    , m_cSnaphot(0)
    , m_enmMachineState(KMachineState_Null)
    , m_enmSessionState(KSessionState_Null)
    , m_enmConfigurationAccessLevel(ConfigurationAccessLevel_Null)
{
    recache();
}

UIVirtualMachineItem::~UIVirtualMachineItem()
{
}

QPixmap UIVirtualMachineItem::osPixmap(QSize *pLogicalSize /* = 0 */) const
{
    if (pLogicalSize)
        *pLogicalSize = m_logicalPixmapSize;
    return m_pixmap;
}

QString UIVirtualMachineItem::machineStateName() const
{
    return   m_fAccessible
           ? gpConverter->toString(m_enmMachineState)
           : QApplication::translate("UIVMListView", "Inaccessible");
}

QString UIVirtualMachineItem::sessionStateName() const
{
    return   m_fAccessible
           ? gpConverter->toString(m_enmSessionState)
           : QApplication::translate("UIVMListView", "Inaccessible");
}

QIcon UIVirtualMachineItem::machineStateIcon() const
{
    return   m_fAccessible
           ? gpConverter->toIcon(m_enmMachineState)
           : gpConverter->toIcon(KMachineState_Aborted);
}

QString UIVirtualMachineItem::toolTipText() const
{
    QString strToolTip;

    const QString strDateTime = (m_lastStateChange.date() == QDate::currentDate())
                              ? m_lastStateChange.time().toString(Qt::LocalDate)
                              : m_lastStateChange.toString(Qt::LocalDate);

    if (m_fAccessible)
    {
        strToolTip = QString("<b>%1</b>").arg(m_strName);
        if (!m_strSnapshotName.isNull())
            strToolTip += QString(" (%1)").arg(m_strSnapshotName);
        strToolTip = QApplication::translate("UIVMListView",
                                             "<nobr>%1<br></nobr>"
                                             "<nobr>%2 since %3</nobr><br>"
                                             "<nobr>Session %4</nobr>",
                                             "VM tooltip (name, last state change, session state)")
                                             .arg(strToolTip)
                                             .arg(gpConverter->toString(m_enmMachineState))
                                             .arg(strDateTime)
                                             .arg(gpConverter->toString(m_enmSessionState).toLower());
    }
    else
    {
        strToolTip = QApplication::translate("UIVMListView",
                                             "<nobr><b>%1</b><br></nobr>"
                                             "<nobr>Inaccessible since %2</nobr>",
                                             "Inaccessible VM tooltip (name, last state change)")
                                             .arg(m_strSettingsFile)
                                             .arg(strDateTime);
    }

    return strToolTip;
}

void UIVirtualMachineItem::recache()
{
    /* Determine attributes which are always available: */
    m_uId = m_comMachine.GetId();
    m_strSettingsFile = m_comMachine.GetSettingsFilePath();

    /* Now determine whether VM is accessible: */
    m_fAccessible = m_comMachine.GetAccessible();
    if (m_fAccessible)
    {
        /* Reset last access error information: */
        m_comAccessError = CVirtualBoxErrorInfo();

        /* Determine own VM attributes: */
        m_strName = m_comMachine.GetName();
        m_strOSTypeId = m_comMachine.GetOSTypeId();
        m_groups = m_comMachine.GetGroups().toList();

        /* Determine snapshot attributes: */
        CSnapshot comSnapshot = m_comMachine.GetCurrentSnapshot();
        m_strSnapshotName = comSnapshot.isNull() ? QString() : comSnapshot.GetName();
        m_lastStateChange.setTime_t(m_comMachine.GetLastStateChange() / 1000);
        m_cSnaphot = m_comMachine.GetSnapshotCount();

        /* Determine VM states: */
        m_enmMachineState = m_comMachine.GetState();
        m_enmSessionState = m_comMachine.GetSessionState();

        /* Determine configuration access level: */
        m_enmConfigurationAccessLevel = ::configurationAccessLevel(m_enmSessionState, m_enmMachineState);
        /* Also take restrictions into account: */
        if (   m_enmConfigurationAccessLevel != ConfigurationAccessLevel_Null
            && !gEDataManager->machineReconfigurationEnabled(m_uId))
            m_enmConfigurationAccessLevel = ConfigurationAccessLevel_Null;

        /* Determine PID finally: */
        if (   m_enmMachineState == KMachineState_PoweredOff
            || m_enmMachineState == KMachineState_Saved
            || m_enmMachineState == KMachineState_Teleported
            || m_enmMachineState == KMachineState_Aborted
           )
        {
            m_pid = (ULONG) ~0;
        }
        else
        {
            m_pid = m_comMachine.GetSessionPID();
        }

        /* Determine whether we should show this VM details: */
        m_fHasDetails = gEDataManager->showMachineInVirtualBoxManagerDetails(m_uId);
    }
    else
    {
        /* Update last access error information: */
        m_comAccessError = m_comMachine.GetAccessError();

        /* Determine machine name on the basis of settings file only: */
        QFileInfo fi(m_strSettingsFile);
        m_strName = UICommon::hasAllowedExtension(fi.completeSuffix(), VBoxFileExts)
                  ? fi.completeBaseName()
                  : fi.fileName();
        /* Reset other VM attributes: */
        m_strOSTypeId = QString();
        m_groups.clear();

        /* Reset snapshot attributes: */
        m_strSnapshotName = QString();
        m_lastStateChange = QDateTime::currentDateTime();
        m_cSnaphot = 0;

        /* Reset VM states: */
        m_enmMachineState = KMachineState_Null;
        m_enmSessionState = KSessionState_Null;

        /* Reset configuration access level: */
        m_enmConfigurationAccessLevel = ConfigurationAccessLevel_Null;

        /* Reset PID finally: */
        m_pid = (ULONG) ~0;

        /* Reset whether we should show this VM details: */
        m_fHasDetails = true;
    }

    /* Recache item pixmap: */
    recachePixmap();
}

void UIVirtualMachineItem::recachePixmap()
{
    /* If machine is accessible: */
    if (m_fAccessible)
    {
        /* First, we are trying to acquire personal machine guest OS type icon: */
        m_pixmap = uiCommon().vmUserPixmapDefault(m_comMachine, &m_logicalPixmapSize);
        /* If there is nothing, we are using icon corresponding to cached guest OS type: */
        if (m_pixmap.isNull())
            m_pixmap = uiCommon().vmGuestOSTypePixmapDefault(m_strOSTypeId, &m_logicalPixmapSize);
    }
    /* Otherwise: */
    else
    {
        /* We are using "Other" guest OS type icon: */
        m_pixmap = uiCommon().vmGuestOSTypePixmapDefault("Other", &m_logicalPixmapSize);
    }
}

bool UIVirtualMachineItem::canSwitchTo() const
{
    return const_cast <CMachine &>(m_comMachine).CanShowConsoleWindow();
}

bool UIVirtualMachineItem::switchTo()
{
#ifdef VBOX_WS_MAC
    ULONG64 id = m_comMachine.ShowConsoleWindow();
#else
    WId id = (WId) m_comMachine.ShowConsoleWindow();
#endif
    AssertWrapperOk(m_comMachine);
    if (!m_comMachine.isOk())
        return false;

    /* winId = 0 it means the console window has already done everything
     * necessary to implement the "show window" semantics. */
    if (id == 0)
        return true;

#if defined (VBOX_WS_WIN) || defined (VBOX_WS_X11)

    return uiCommon().activateWindow(id, true);

#elif defined (VBOX_WS_MAC)

    // WORKAROUND:
    // This is just for the case were the other process cannot steal
    // the focus from us. It will send us a PSN so we can try.
    ProcessSerialNumber psn;
    psn.highLongOfPSN = id >> 32;
    psn.lowLongOfPSN = (UInt32)id;
    OSErr rc = ::SetFrontProcess(&psn);
    if (!rc)
        Log(("GUI: %#RX64 couldn't do SetFrontProcess on itself, the selector (we) had to do it...\n", id));
    else
        Log(("GUI: Failed to bring %#RX64 to front. rc=%#x\n", id, rc));
    return !rc;

#else

    return false;

#endif
}

/* static */
bool UIVirtualMachineItem::isItemEditable(UIVirtualMachineItem *pItem)
{
    return    pItem
           && pItem->accessible()
           && pItem->sessionState() == KSessionState_Unlocked;
}

/* static */
bool UIVirtualMachineItem::isItemSaved(UIVirtualMachineItem *pItem)
{
    return    pItem
           && pItem->accessible()
           && pItem->machineState() == KMachineState_Saved;
}

/* static */
bool UIVirtualMachineItem::isItemPoweredOff(UIVirtualMachineItem *pItem)
{
    return    pItem
           && pItem->accessible()
           && (   pItem->machineState() == KMachineState_PoweredOff
               || pItem->machineState() == KMachineState_Saved
               || pItem->machineState() == KMachineState_Teleported
               || pItem->machineState() == KMachineState_Aborted);
}

/* static */
bool UIVirtualMachineItem::isItemStarted(UIVirtualMachineItem *pItem)
{
    return    isItemRunning(pItem)
           || isItemPaused(pItem);
}

/* static */
bool UIVirtualMachineItem::isItemRunning(UIVirtualMachineItem *pItem)
{
    return    pItem
           && pItem->accessible()
           && (   pItem->machineState() == KMachineState_Running
               || pItem->machineState() == KMachineState_Teleporting
               || pItem->machineState() == KMachineState_LiveSnapshotting);
}

/* static */
bool UIVirtualMachineItem::isItemRunningHeadless(UIVirtualMachineItem *pItem)
{
    if (isItemRunning(pItem))
    {
        /* Open session to determine which frontend VM is started with: */
        CSession comSession = uiCommon().openExistingSession(pItem->id());
        if (!comSession.isNull())
        {
            /* Acquire the session name: */
            const QString strSessionName = comSession.GetMachine().GetSessionName();
            /* Close the session early: */
            comSession.UnlockMachine();
            /* Check whether we are in 'headless' session: */
            return strSessionName == "headless";
        }
    }
    return false;
}

/* static */
bool UIVirtualMachineItem::isItemPaused(UIVirtualMachineItem *pItem)
{
    return    pItem
           && pItem->accessible()
           && (   pItem->machineState() == KMachineState_Paused
               || pItem->machineState() == KMachineState_TeleportingPausedVM);
}

/* static */
bool UIVirtualMachineItem::isItemStuck(UIVirtualMachineItem *pItem)
{
    return    pItem
           && pItem->accessible()
           && pItem->machineState() == KMachineState_Stuck;
}


/*********************************************************************************************************************************
*   Class UIVirtualMachineItemMimeData implementation.                                                                           *
*********************************************************************************************************************************/

QString UIVirtualMachineItemMimeData::m_type = "application/org.virtualbox.gui.vmselector.UIVirtualMachineItem";

UIVirtualMachineItemMimeData::UIVirtualMachineItemMimeData(UIVirtualMachineItem *pItem)
    : m_pItem(pItem)
{
}

QStringList UIVirtualMachineItemMimeData::formats() const
{
    QStringList types;
    types << type();
    return types;
}

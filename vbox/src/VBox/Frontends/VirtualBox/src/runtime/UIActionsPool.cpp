/* $Id: UIActionsPool.cpp 27335 2010-03-12 16:20:29Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIActionsPool class implementation
 */

/*
 * Copyright (C) 2010 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

/* Local includes */
#include "UIActionsPool.h"
#include "VBoxGlobal.h"

/* Global includes */
#include <QtGlobal>

/* Action activation event */
class ActivateActionEvent : public QEvent
{
public:

    ActivateActionEvent(QAction *pAction)
        : QEvent((QEvent::Type)VBoxDefs::ActivateActionEventType)
        , m_pAction(pAction) {}
    QAction* action() const { return m_pAction; }

private:

    QAction *m_pAction;
};

UIAction::UIAction(QObject *pParent, UIActionType type)
    : QIWithRetranslateUI3<QAction>(pParent)
    , m_type(type)
{
}

UIActionType UIAction::type() const
{
    return m_type;
}

class UISeparatorAction : public UIAction
{
    Q_OBJECT;

public:

    UISeparatorAction(QObject *pParent)
        : UIAction(pParent, UIActionType_Separator)
    {
        setSeparator(true);
    }
};

class UISimpleAction : public UIAction
{
    Q_OBJECT;

public:

    UISimpleAction(QObject *pParent,
                   const QString &strIcon = QString(), const QString &strIconDis = QString())
        : UIAction(pParent, UIActionType_Simple)
    {
        if (!strIcon.isNull())
            setIcon(VBoxGlobal::iconSet(strIcon.toLatin1().data(), strIconDis.toLatin1().data()));
    }
};

class UIToggleAction : public UIAction
{
    Q_OBJECT;

public:

    UIToggleAction(QObject *pParent,
                   const QString &strIcon = QString(), const QString &strIconDis = QString())
        : UIAction(pParent, UIActionType_Toggle)
    {
        if (!strIcon.isNull())
            setIcon(VBoxGlobal::iconSet(strIcon.toLatin1().data(), strIconDis.toLatin1().data()));
        init();
    }

    UIToggleAction(QObject *pParent,
                   const QString &strIconOn, const QString &strIconOff,
                   const QString &strIconOnDis, const QString &strIconOffDis)
        : UIAction(pParent, UIActionType_Toggle)
    {
        setIcon(VBoxGlobal::iconSetOnOff(strIconOn.toLatin1().data(), strIconOff.toLatin1().data(),
                                         strIconOnDis.toLatin1().data(), strIconOffDis.toLatin1().data()));
        init();
    }

private slots:

    void sltUpdateAppearance()
    {
        retranslateUi();
    }

private:

    void init()
    {
        setCheckable(true);
        connect(this, SIGNAL(triggered(bool)), this, SLOT(sltUpdateAppearance()));
    }
};

class UIMenuAction : public UIAction
{
    Q_OBJECT;

public:

    UIMenuAction(QObject *pParent,
                 const QString &strIcon = QString(), const QString &strIconDis = QString())
        : UIAction(pParent, UIActionType_Menu)
    {
        if (!strIcon.isNull())
            setIcon(VBoxGlobal::iconSet(strIcon, strIconDis));
        setMenu(new QMenu);
    }
};

class SeparatorAction : public UISeparatorAction
{
    Q_OBJECT;

public:

    SeparatorAction(QObject *pParent)
        : UISeparatorAction(pParent)
    {
    }

protected:

    void retranslateUi()
    {
    }
};

class MenuMachineAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuMachineAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("&Machine"));
    }
};

class ToggleFullscreenModeAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleFullscreenModeAction(QObject *pParent)
        : UIToggleAction(pParent,
                         ":/fullscreen_on_16px.png", ":/fullscreen_16px.png",
                         ":/fullscreen_on_disabled_16px.png", ":/fullscreen_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Fullscreen Mode"), "F"));
        setStatusTip(UIActionsPool::tr("Switch to fullscreen mode"));
    }
};

class ToggleSeamlessModeAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleSeamlessModeAction(QObject *pParent)
        : UIToggleAction(pParent,
                         ":/seamless_on_16px.png", ":/seamless_16px.png",
                         ":/seamless_on_disabled_16px.png", ":/seamless_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("Seam&less Mode"), "L"));
        setStatusTip(UIActionsPool::tr("Switch to seamless desktop integration mode"));
    }
};

class ToggleGuestAutoresizeAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleGuestAutoresizeAction(QObject *pParent)
        : UIToggleAction(pParent,
                         ":/auto_resize_on_on_16px.png", ":/auto_resize_on_16px.png",
                         ":/auto_resize_on_on_disabled_16px.png", ":/auto_resize_on_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("Auto-resize &Guest Display"), "G"));
        setStatusTip(UIActionsPool::tr("Automatically resize the guest display when the window is resized (requires Guest Additions)"));
    }
};

class PerformWindowAdjustAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformWindowAdjustAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/adjust_win_size_16px.png", ":/adjust_win_size_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Adjust Window Size"), "A"));
        setStatusTip(UIActionsPool::tr("Adjust window size and position to best fit the guest display"));
    }
};

class MenuMouseIntegrationAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuMouseIntegrationAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
    }
};

class ToggleMouseIntegrationAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleMouseIntegrationAction(QObject *pParent)
        : UIToggleAction(pParent,
                         ":/mouse_can_seamless_on_16px.png", ":/mouse_can_seamless_16px.png",
                         ":/mouse_can_seamless_on_disabled_16px.png", ":/mouse_can_seamless_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        if (!isChecked())
        {
            setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("Disable &Mouse Integration"), "I"));
            setStatusTip(UIActionsPool::tr("Temporarily disable host mouse pointer integration"));
        }
        else
        {
            setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("Enable &Mouse Integration"), "I"));
            setStatusTip(UIActionsPool::tr("Enable temporarily disabled host mouse pointer integration"));
        }
    }
};

class PerformTypeCADAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformTypeCADAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/hostkey_16px.png", ":/hostkey_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Insert Ctrl-Alt-Del"), "Del"));
        setStatusTip(UIActionsPool::tr("Send the Ctrl-Alt-Del sequence to the virtual machine"));
    }
};

#ifdef Q_WS_X11
class PerformTypeCABSAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformTypeCABSAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/hostkey_16px.png", ":/hostkey_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Insert Ctrl-Alt-Backspace"), "Backspace"));
        setStatusTip(UIActionsPool::tr("Send the Ctrl-Alt-Backspace sequence to the virtual machine"));
    }
};
#endif

class PerformTakeSnapshotAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformTakeSnapshotAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/take_snapshot_16px.png", ":/take_snapshot_dis_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("Take &Snapshot..."), "S"));
        setStatusTip(UIActionsPool::tr("Take a snapshot of the virtual machine"));
    }
};

class ShowInformationDialogAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowInformationDialogAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/session_info_16px.png", ":/session_info_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("Session I&nformation Dialog"), "N"));
        setStatusTip(UIActionsPool::tr("Show Session Information Dialog"));
    }
};

class TogglePauseAction : public UIToggleAction
{
    Q_OBJECT;

public:

    TogglePauseAction(QObject *pParent)
        : UIToggleAction(pParent,
                         ":/pause_16px.png", ":/pause_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        if (!isChecked())
        {
            setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Pause"), "P"));
            setStatusTip(UIActionsPool::tr("Suspend the execution of the virtual machine"));
        }
        else
        {
            setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("R&esume"), "P"));
            setStatusTip(UIActionsPool::tr("Resume the execution of the virtual machine"));
        }
    }
};

class PerformResetAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformResetAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/reset_16px.png", ":/reset_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Reset"), "R"));
        setStatusTip(UIActionsPool::tr("Reset the virtual machine"));
    }
};

class PerformShutdownAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformShutdownAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/acpi_16px.png", ":/acpi_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
#ifdef Q_WS_MAC
        /* Host+H is Hide on the mac */
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("ACPI Sh&utdown"), "U"));
#else /* Q_WS_MAC */
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("ACPI S&hutdown"), "H"));
#endif /* !Q_WS_MAC */
        setStatusTip(UIActionsPool::tr("Send the ACPI Power Button press event to the virtual machine"));
    }
};

class PerformCloseAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformCloseAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/exit_16px.png")
    {
        setMenuRole(QAction::QuitRole);
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Close..." ), "Q"));
        setStatusTip(UIActionsPool::tr("Close the virtual machine"));
    }
};

class MenuViewAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuViewAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("&View"));
    }
};

class MenuDevicesAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuDevicesAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("&Devices"));
    }
};

class MenuOpticalDevicesAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuOpticalDevicesAction(QObject *pParent)
        : UIMenuAction(pParent,
                       ":/cd_16px.png", ":/cd_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("&CD/DVD Devices"));
    }
};

class MenuFloppyDevicesAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuFloppyDevicesAction(QObject *pParent)
        : UIMenuAction(pParent,
                       ":/fd_16px.png", ":/fd_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("&Floppy Devices"));
    }
};

class MenuUSBDevicesAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuUSBDevicesAction(QObject *pParent)
        : UIMenuAction(pParent,
                       ":/usb_16px.png", ":/usb_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("&USB Devices"));
    }
};

class MenuNetworkAdaptersAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuNetworkAdaptersAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
    }
};

class ShowNetworkAdaptersDialogAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowNetworkAdaptersDialogAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/nw_16px.png", ":/nw_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Network Adapters..."));
        setStatusTip(UIActionsPool::tr("Change the settings of network adapters"));
    }
};

class MenuSharedFoldersAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuSharedFoldersAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
    }
};

class ShowSharedFoldersDialogAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowSharedFoldersDialogAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/shared_folder_16px.png", ":/shared_folder_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Shared Folders..."));
        setStatusTip(UIActionsPool::tr("Create or modify shared folders"));
    }
};

class ToggleVRDPAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleVRDPAction(QObject *pParent)
        : UIToggleAction(pParent,
                         ":/vrdp_on_16px.png", ":/vrdp_16px.png",
                         ":/vrdp_on_disabled_16px.png", ":/vrdp_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Remote Display"));
        setStatusTip(UIActionsPool::tr("Enable or disable remote desktop (RDP) connections to this machine"));
    }
};

class PerformInstallGuestToolsAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformInstallGuestToolsAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/guesttools_16px.png", ":/guesttools_disabled_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(VBoxGlobal::insertKeyToActionText(UIActionsPool::tr("&Install Guest Additions..."), "D"));
        setStatusTip(UIActionsPool::tr("Mount the Guest Additions installation image"));
    }
};

#ifdef VBOX_WITH_DEBUGGER_GUI
class MenuDebugAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuDebugAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        menu()->setTitle(UIActionsPool::tr("De&bug"));
    }
};

class ShowStatisticsAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowStatisticsAction(QObject *pParent)
        : UISimpleAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Statistics...", "debug action"));
    }
};

class ShowCommandLineAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowCommandLineAction(QObject *pParent)
        : UISimpleAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Command Line...", "debug action"));
    }
};

class ToggleLoggingAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleLoggingAction(QObject *pParent)
        : UIToggleAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Logging...", "debug action"));
    }
};
#endif

class MenuHelpAction : public UIMenuAction
{
    Q_OBJECT;

public:

    MenuHelpAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Help"));
    }
};

class ShowHelpAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowHelpAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/help_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setShortcut(QKeySequence::HelpContents);
        setText(UIActionsPool::tr("&Contents..."));
        setStatusTip(UIActionsPool::tr("Show the online help contents"));
    }
};

class ShowWebAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowWebAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/site_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&VirtualBox Web Site..."));
        setStatusTip(UIActionsPool::tr("Open the browser and go to the VirtualBox product web site"));
    }
};

class PerformResetWarningsAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformResetWarningsAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/reset_16px.png")
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&Reset All Warnings"));
        setStatusTip(UIActionsPool::tr("Go back to showing all suppressed warnings and messages"));
    }
};

class PerformRegisterAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformRegisterAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/register_16px.png", ":/register_disabled_16px.png")
    {
        setEnabled(vboxGlobal().virtualBox().
                   GetExtraData(VBoxDefs::GUI_RegistrationDlgWinID).isEmpty());
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("R&egister VirtualBox..."));
        setStatusTip(UIActionsPool::tr("Open VirtualBox registration form"));
    }
};

class PerformUpdateAction : public UISimpleAction
{
    Q_OBJECT;

public:

    PerformUpdateAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/refresh_16px.png", ":/refresh_disabled_16px.png")
    {
        setMenuRole(QAction::ApplicationSpecificRole);
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("C&heck for Updates..."));
        setStatusTip(UIActionsPool::tr("Check for a new VirtualBox version"));
    }
};

class ShowAboutAction : public UISimpleAction
{
    Q_OBJECT;

public:

    ShowAboutAction(QObject *pParent)
        : UISimpleAction(pParent,
                         ":/about_16px.png")
    {
        setMenuRole(QAction::AboutRole);
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("&About VirtualBox..."));
        setStatusTip(UIActionsPool::tr("Show a dialog with product information"));
    }
};

#ifdef Q_WS_MAC
class DockMenuAction : public UIMenuAction
{
    Q_OBJECT;

public:

    DockMenuAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi() {}
};

class DockSettingsMenuAction : public UIMenuAction
{
    Q_OBJECT;

public:

    DockSettingsMenuAction(QObject *pParent)
        : UIMenuAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("Dock Icon"));
    }
};

class ToggleDockPreviewMonitorAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleDockPreviewMonitorAction(QObject *pParent)
        : UIToggleAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("Show Monitor Preview"));
    }
};

class ToggleDockDisableMonitorAction : public UIToggleAction
{
    Q_OBJECT;

public:

    ToggleDockDisableMonitorAction(QObject *pParent)
        : UIToggleAction(pParent)
    {
        retranslateUi();
    }

protected:

    void retranslateUi()
    {
        setText(UIActionsPool::tr("Show Application Icon"));
    }
};
#endif /* Q_WS_MAC */

UIActionsPool::UIActionsPool(QObject *pParent)
    : QObject(pParent)
    , m_actionsPool(UIActionIndex_End, 0)
{
    /* Common actions: */
    m_actionsPool[UIActionIndex_Separator] = new SeparatorAction(this);

    /* "Machine" menu actions: */
    m_actionsPool[UIActionIndex_Toggle_Fullscreen] = new ToggleFullscreenModeAction(this);
    m_actionsPool[UIActionIndex_Toggle_Seamless] = new ToggleSeamlessModeAction(this);
    m_actionsPool[UIActionIndex_Toggle_GuestAutoresize] = new ToggleGuestAutoresizeAction(this);
    m_actionsPool[UIActionIndex_Simple_AdjustWindow] = new PerformWindowAdjustAction(this);
    m_actionsPool[UIActionIndex_Toggle_MouseIntegration] = new ToggleMouseIntegrationAction(this);
    m_actionsPool[UIActionIndex_Simple_TypeCAD] = new PerformTypeCADAction(this);
#ifdef Q_WS_X11
    m_actionsPool[UIActionIndex_Simple_TypeCABS] = new PerformTypeCABSAction(this);
#endif
    m_actionsPool[UIActionIndex_Simple_TakeSnapshot] = new PerformTakeSnapshotAction(this);
    m_actionsPool[UIActionIndex_Simple_InformationDialog] = new ShowInformationDialogAction(this);
    m_actionsPool[UIActionIndex_Toggle_Pause] = new TogglePauseAction(this);
    m_actionsPool[UIActionIndex_Simple_Reset] = new PerformResetAction(this);
    m_actionsPool[UIActionIndex_Simple_Shutdown] = new PerformShutdownAction(this);
    m_actionsPool[UIActionIndex_Simple_Close] = new PerformCloseAction(this);

    /* "Devices" menu actions: */
    m_actionsPool[UIActionIndex_Simple_NetworkAdaptersDialog] = new ShowNetworkAdaptersDialogAction(this);
    m_actionsPool[UIActionIndex_Simple_SharedFoldersDialog] = new ShowSharedFoldersDialogAction(this);
    m_actionsPool[UIActionIndex_Toggle_VRDP] = new ToggleVRDPAction(this);
    m_actionsPool[UIActionIndex_Simple_InstallGuestTools] = new PerformInstallGuestToolsAction(this);

#ifdef VBOX_WITH_DEBUGGER_GUI
    /* "Debugger" menu actions: */
    m_actionsPool[UIActionIndex_Simple_Statistics] = new ShowStatisticsAction(this);
    m_actionsPool[UIActionIndex_Simple_CommandLine] = new ShowCommandLineAction(this);
    m_actionsPool[UIActionIndex_Toggle_Logging] = new ToggleLoggingAction(this);
#endif

    /* "Help" menu actions: */
    m_actionsPool[UIActionIndex_Simple_Help] = new ShowHelpAction(this);
    m_actionsPool[UIActionIndex_Simple_Web] = new ShowWebAction(this);
    m_actionsPool[UIActionIndex_Simple_ResetWarnings] = new PerformResetWarningsAction(this);
    m_actionsPool[UIActionIndex_Simple_Register] = new PerformRegisterAction(this);
    m_actionsPool[UIActionIndex_Simple_Update] = new PerformUpdateAction(this);
    m_actionsPool[UIActionIndex_Simple_About] = new ShowAboutAction(this);

#ifdef Q_WS_MAC
    /* "Dock" menu actions: */
    m_actionsPool[UIActionIndex_Toggle_DockPreviewMonitor] = new ToggleDockPreviewMonitorAction(this);
    m_actionsPool[UIActionIndex_Toggle_DockDisableMonitor] = new ToggleDockDisableMonitorAction(this);
#endif /* Q_WS_MAC */

    /* Create all menus */
    createMenus();

    /* Test all actions were initialized */
    for (int i = 0; i < m_actionsPool.size(); ++i)
        if (!m_actionsPool.at(i))
            AssertMsgFailed(("Action #%d is not created!\n", i));
}

UIActionsPool::~UIActionsPool()
{
    for (int i = 0; i < m_actionsPool.size(); ++i)
        delete m_actionsPool.at(i);
    m_actionsPool.clear();
}

UIAction* UIActionsPool::action(UIActionIndex index) const
{
    return m_actionsPool.at(index);
}

void UIActionsPool::createMenus()
{
    /* On Mac OS X, all QMenu's are consumed by Qt after they are added to
     * another QMenu or a QMenuBar. This means we have to recreate all QMenus
     * when creating a new QMenuBar. For simplicity we doing this on all
     * platforms right now. */
    if (m_actionsPool[UIActionIndex_Menu_Machine])
        delete m_actionsPool[UIActionIndex_Menu_Machine];
    m_actionsPool[UIActionIndex_Menu_Machine] = new MenuMachineAction(this);
    if (m_actionsPool[UIActionIndex_Menu_View])
        delete m_actionsPool[UIActionIndex_Menu_View];
    m_actionsPool[UIActionIndex_Menu_View] = new MenuViewAction(this);
    if (m_actionsPool[UIActionIndex_Menu_MouseIntegration])
        delete m_actionsPool[UIActionIndex_Menu_MouseIntegration];
    m_actionsPool[UIActionIndex_Menu_MouseIntegration] = new MenuMouseIntegrationAction(this);

    if (m_actionsPool[UIActionIndex_Menu_Devices])
        delete m_actionsPool[UIActionIndex_Menu_Devices];
    m_actionsPool[UIActionIndex_Menu_Devices] = new MenuDevicesAction(this);
    if (m_actionsPool[UIActionIndex_Menu_OpticalDevices])
        delete m_actionsPool[UIActionIndex_Menu_OpticalDevices];
    m_actionsPool[UIActionIndex_Menu_OpticalDevices] = new MenuOpticalDevicesAction(this);
    if (m_actionsPool[UIActionIndex_Menu_FloppyDevices])
        delete m_actionsPool[UIActionIndex_Menu_FloppyDevices];
    m_actionsPool[UIActionIndex_Menu_FloppyDevices] = new MenuFloppyDevicesAction(this);
    if (m_actionsPool[UIActionIndex_Menu_USBDevices])
        delete m_actionsPool[UIActionIndex_Menu_USBDevices];
    m_actionsPool[UIActionIndex_Menu_USBDevices] = new MenuUSBDevicesAction(this);
    if (m_actionsPool[UIActionIndex_Menu_NetworkAdapters])
        delete m_actionsPool[UIActionIndex_Menu_NetworkAdapters];
    m_actionsPool[UIActionIndex_Menu_NetworkAdapters] = new MenuNetworkAdaptersAction(this);

    if (m_actionsPool[UIActionIndex_Menu_SharedFolders])
        delete m_actionsPool[UIActionIndex_Menu_SharedFolders];
    m_actionsPool[UIActionIndex_Menu_SharedFolders] = new MenuSharedFoldersAction(this);

    if (m_actionsPool[UIActionIndex_Menu_Debug])
        delete m_actionsPool[UIActionIndex_Menu_Debug];
    m_actionsPool[UIActionIndex_Menu_Debug] = new MenuDebugAction(this);

    if (m_actionsPool[UIActionIndex_Menu_Help])
        delete m_actionsPool[UIActionIndex_Menu_Help];
    m_actionsPool[UIActionIndex_Menu_Help] = new MenuHelpAction(this);

#ifdef Q_WS_MAC
    if (m_actionsPool[UIActionIndex_Menu_Dock])
        delete m_actionsPool[UIActionIndex_Menu_Dock];
    m_actionsPool[UIActionIndex_Menu_Dock] = new DockMenuAction(this);
    if (m_actionsPool[UIActionIndex_Menu_DockSettings])
        delete m_actionsPool[UIActionIndex_Menu_DockSettings];
    m_actionsPool[UIActionIndex_Menu_DockSettings] = new DockSettingsMenuAction(this);
#endif /* Q_WS_MAC */
}

bool UIActionsPool::processHotKey(const QKeySequence &key)
{
    for (int i = 0; i < m_actionsPool.size(); ++i)
    {
        UIAction *pAction = m_actionsPool.at(i);
        /* Skip menus/separators */
        if (pAction->type() == UIActionType_Menu || pAction->type() == UIActionType_Separator)
            continue;
        QString hotkey = VBoxGlobal::extractKeyFromActionText(pAction->text());
        if (pAction->isEnabled() && pAction->isVisible() && !hotkey.isEmpty())
        {
            if (key.matches(QKeySequence(hotkey)) == QKeySequence::ExactMatch)
            {
                /* We asynchronously post a special event instead of calling
                 * pAction->trigger() directly, to let key presses and
                 * releases be processed correctly by Qt first.
                 * Note: we assume that nobody will delete the menu item
                 * corresponding to the key sequence, so that the pointer to
                 * menu data posted along with the event will remain valid in
                 * the event handler, at least until the main window is closed. */
                QApplication::postEvent(this, new ActivateActionEvent(pAction));
                return true;
            }
        }
    }

    return false;
}

bool UIActionsPool::event(QEvent *pEvent)
{
    switch (pEvent->type())
    {
        case VBoxDefs::ActivateActionEventType:
        {
            ActivateActionEvent *pActionEvent = static_cast<ActivateActionEvent*>(pEvent);
            pActionEvent->action()->trigger();

            // TODO_NEW_CORE
            /* The main window and its children can be destroyed at this point (if, for example, the activated menu
             * item closes the main window). Detect this situation to prevent calls to destroyed widgets: */
//            QWidgetList list = QApplication::topLevelWidgets();
//            bool destroyed = list.indexOf(machineWindowWrapper()->machineWindow()) < 0;
//            if (!destroyed && machineWindowWrapper()->machineWindow()->statusBar())
//                machineWindowWrapper()->machineWindow()->statusBar()->clearMessage();

            pEvent->accept();
            return true;
        }
        default:
            break;
    }

    return QObject::event(pEvent);
}

#include "UIActionsPool.moc"


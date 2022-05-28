/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * VBoxConsoleWnd class declaration
 */

/*
 * Copyright (C) 2006 InnoTek Systemberatung GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */

#ifndef __VBoxConsoleWnd_h__
#define __VBoxConsoleWnd_h__

#include "COMDefs.h"

#include <qmainwindow.h>

#include <qmap.h>
#include <qobjectlist.h>
#include <qcolor.h>

#ifdef VBOX_WITH_DEBUGGER_GUI
#include <VBox/dbggui.h>
#endif

class QAction;
class QActionGroup;
class QHBox;
class QLabel;

class VBoxConsoleView;
class QIStateIndicator;

class VBoxUSBMenu;
class VBoxSwitchMenu;
class VBoxUSBLedTip;

class VBoxConsoleWnd : public QMainWindow
{
    Q_OBJECT

public:

    VBoxConsoleWnd (VBoxConsoleWnd **aSelf,
                     QWidget* aParent = 0, const char* aName = 0,
                     WFlags aFlags = WType_TopLevel);
    virtual ~VBoxConsoleWnd();

    bool openView (const CSession &session);
    void closeView();

    void refreshView();

    bool isTrueFullscreen() const { return full_screen; }

public slots:

protected:

    // events
    bool event (QEvent *e);
    void closeEvent (QCloseEvent *e);
#if defined(Q_WS_X11)
    bool x11Event (XEvent *event);
#endif
#ifdef VBOX_WITH_DEBUGGER_GUI
    bool dbgCreated();
    void dbgDestroy();
    void dbgAdjustRelativePos();
#endif

protected slots:

private:

    enum /* Stuff */
    {
        FloppyStuff                 = 0x01,
        DVDStuff                    = 0x02,
        HardDiskStuff               = 0x04,
        PauseAction                 = 0x08,
        NetworkStuff                = 0x10,
        DisableMouseIntegrAction    = 0x20,
        Caption                     = 0x40,
        USBStuff                    = 0x80,
        VRDPStuff                   = 0x100,
        AllStuff                    = 0xFF,
    };

    void languageChange();

    void updateAppearanceOf (int element);

private slots:

    void finalizeOpenView();

    void vmFullscreen (bool on);
    void vmAutoresizeGuest (bool on);
    void vmAdjustWindow();

    void vmTypeCAD();
    void vmTypeCABS();
    void vmReset();
    void vmPause(bool);
    void vmACPIShutdown();
    void vmClose();
    void vmTakeSnapshot();
    void vmDisableMouseIntegr (bool);

    void devicesMountFloppyImage();
    void devicesUnmountFloppy();
    void devicesMountDVDImage();
    void devicesUnmountDVD();
    void devicesSwitchVrdp (bool);
    void devicesInstallGuestAdditions();

    void prepareFloppyMenu();
    void prepareDVDMenu();

    void captureFloppy (int id);
    void captureDVD (int id);
    void switchUSB (int id);

    void showIndicatorContextMenu (QIStateIndicator *ind, QContextMenuEvent *e);

    void updateDeviceLights();
    void updateMachineState (CEnums::MachineState state);

    void updateMouseState (int state);

    void tryClose();

    void processGlobalSettingChange (const char *publicName, const char *name);

    void dbgShowStatistics();
    void dbgShowCommandLine();

private:

    QActionGroup *runningActions;

    // VM actions
    QAction *vmFullscreenAction;
    QAction *vmAutoresizeGuestAction;
    QAction *vmAdjustWindowAction;
    QAction *vmTypeCADAction;
#if defined(Q_WS_X11)
    QAction *vmTypeCABSAction;
#endif
    QAction *vmResetAction;
    QAction *vmPauseAction;
    QAction *vmACPIShutdownAction;
    QAction *vmCloseAction;
    QAction *vmTakeSnapshotAction;
    QAction *vmDisableMouseIntegrAction;

    // VM popup menus
    VBoxSwitchMenu *vmAutoresizeMenu;
    VBoxSwitchMenu *vmDisMouseIntegrMenu;

    // Devices actions
    QAction *devicesMountFloppyImageAction;
    QAction *devicesUnmountFloppyAction;
    QAction *devicesMountDVDImageAction;
    QAction *devicesUnmountDVDAction;
    QAction *devicesSwitchVrdpAction;
    QAction *devicesInstallGuestToolsAction;

#ifdef VBOX_WITH_DEBUGGER_GUI
    // Debugger actions
    QAction *dbgStatisticsAction;
    QAction *dbgCommandLineAction;
#endif

    // Help actions
    QAction *helpWebAction;
    QAction *helpAboutAction;
    QAction *helpResetMessagesAction;

    // Devices popup menus
    QPopupMenu *devicesMenu;
    QPopupMenu *devicesMountFloppyMenu;
    QPopupMenu *devicesMountDVDMenu;
    VBoxUSBMenu *devicesUSBMenu;
    VBoxSwitchMenu *devicesVRDPMenu;

    int devicesUSBMenuSeparatorId;
    int devicesVRDPMenuSeparatorId;

#ifdef VBOX_WITH_DEBUGGER_GUI
    // Debugger popup menu
    QPopupMenu *dbgMenu;
#endif

    // Menu identifiers
    enum {
        vmMenuId = 1,
        devicesMenuId,
        devicesMountFloppyMenuId,
        devicesMountDVDMenuId,
        devicesUSBMenuId,
#ifdef VBOX_WITH_DEBUGGER_GUI
        dbgMenuId,
#endif
        helpMenuId,
    };

    CSession csession;

    // widgets
    VBoxConsoleView *console;
    QIStateIndicator *hd_light, *cd_light, *fd_light, *net_light, *usb_light;
    QIStateIndicator *mouse_state, *hostkey_state;
    QIStateIndicator *autoresize_state;
    QIStateIndicator *vrdp_state;
    QHBox *hostkey_hbox;
    QLabel *hostkey_name;

    VBoxUSBLedTip *mUsbLedTip;

    QTimer *idle_timer;
    CEnums::MachineState machine_state;
    QString caption_prefix;

    bool no_auto_close : 1;

    QMap <int, CHostDVDDrive> hostDVDMap;
    QMap <int, CHostFloppyDrive> hostFloppyMap;

    QPoint normal_pos;
    QSize normal_size;

    // variables for dealing with true fullscreen
    bool full_screen : 1;
    int normal_wflags;
    bool was_max : 1;
    QObjectList hidden_children;
    int console_style;
    QColor erase_color;

#ifdef VBOX_WITH_DEBUGGER_GUI
    // Debugger GUI
    PDBGGUI dbg_gui;
#endif
};

#endif // __VBoxConsoleWnd_h__

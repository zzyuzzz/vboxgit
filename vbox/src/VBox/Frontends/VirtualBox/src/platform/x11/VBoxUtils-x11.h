/* $Id: VBoxUtils-x11.h 91057 2021-09-01 11:57:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - Declarations of utility classes and functions for handling X11 specific tasks.
 */

/*
 * Copyright (C) 2006-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_platform_x11_VBoxUtils_x11_h
#define FEQT_INCLUDED_SRC_platform_x11_VBoxUtils_x11_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QString>
#include <QVector>

/* GUI includes: */
#include "UILibraryDefs.h"

/* Forward declarations: */
class QDBusConnection;
class QDomElement;

/** X11: Known Window Manager types. */
enum X11WMType
{
    X11WMType_Unknown,
    X11WMType_Compiz,
    X11WMType_GNOMEShell,
    X11WMType_KWin,
    X11WMType_Metacity,
    X11WMType_Mutter,
    X11WMType_Xfwm4,
};

/** X11: Screen-saver inhibit methods. */
struct SHARED_LIBRARY_STUFF X11ScreenSaverInhibitMethod
{
    QString  m_strServiceName;
    QString  m_strInterface;
    QString  m_strPath;
    uint     m_iCookie;
};

/* Namespace for native window sub-system functions: */
namespace NativeWindowSubsystem
{
    /** X11: Determines and returns whether the compositing manager is running. */
    SHARED_LIBRARY_STUFF bool X11IsCompositingManagerRunning();

    /** X11: Determines and returns current Window Manager type. */
    SHARED_LIBRARY_STUFF X11WMType X11WindowManagerType();

    /** X11: Inits the screen saver save/restore mechanism. */
    SHARED_LIBRARY_STUFF void X11ScreenSaverSettingsInit();
    /** X11: Saves screen saver settings. */
    SHARED_LIBRARY_STUFF void X11ScreenSaverSettingsSave();
    /** X11: Restores previously saved screen saver settings. */
    SHARED_LIBRARY_STUFF void X11ScreenSaverSettingsRestore();
    /** X11: Returns true if XLib extension with name @p extensionName is avaible, false otherwise. */
    SHARED_LIBRARY_STUFF bool X11CheckExtension(const char *extensionName);

    /** X11: Returns whether DBus @a connection is alive. */
    bool X11CheckDBusConnection(const QDBusConnection &connection);
    /** X11: Finds a list of DBus @a connection screen-saver services. */
    QStringList X11FindDBusScreenSaverServices(const QDBusConnection &connection);
    /** X11: Returns whether there are any DBus services whose name contains the substring 'screensaver'. */
    bool X11CheckDBusScreenSaverServices();
    /** X11: Does something known only to original creator... */
    void X11IntrospectInterfaceNode(const QDomElement &interface,
                                    const QString &strServiceName,
                                    QVector<X11ScreenSaverInhibitMethod*> &methods);
    /** X11: Does something known only to original creator... */
    void X11IntrospectServices(const QDBusConnection &connection,
                               const QString &strService,
                               const QString &strPath,
                               QVector<X11ScreenSaverInhibitMethod*> &methods);

    /** X11: Returns the list of Inhibit methods found by introspecting DBus services. */
    SHARED_LIBRARY_STUFF QVector<X11ScreenSaverInhibitMethod*> X11FindDBusScrenSaverInhibitMethods();

    /** X11: Disables/enables Screen Saver through QDBus. */
    SHARED_LIBRARY_STUFF void X11InhibitUninhibitScrenSaver(bool fInhibit, QVector<X11ScreenSaverInhibitMethod*> &inOutIhibitMethods);
}

#endif /* !FEQT_INCLUDED_SRC_platform_x11_VBoxUtils_x11_h */

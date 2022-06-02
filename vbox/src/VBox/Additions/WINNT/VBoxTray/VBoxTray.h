/* $Id: VBoxTray.h 33966 2010-11-11 10:32:07Z vboxsync $ */
/** @file
 * VBoxTray - Guest Additions Tray, Internal Header.
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBOXTRAY_H
#define ___VBOXTRAY_H

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <process.h>

#include <iprt/initterm.h>
#include <iprt/string.h>

#include <VBox/version.h>
#include <VBox/Log.h>
#include <VBox/VBoxGuest.h> /** @todo use the VbglR3 interface! */
#include <VBox/VBoxGuestLib.h>
#include <VBoxDisplay.h>

#include "VBoxDispIf.h"

/*
 * Windows messsages.
 */

/**
 * General VBoxTray messages.
 */
#define WM_VBOXTRAY_TRAY_ICON                   WM_APP + 40
#define WM_VBOXTRAY_TRAY_DISPLAY_BALLOON        WM_APP + 41
/**
 * VM/VMMDev related messsages.
 */
#define WM_VBOXTRAY_VM_RESTORED                 WM_APP + 100
/**
 * VRDP messages.
 */
#define WM_VBOXTRAY_VRDP_CHECK                  WM_APP + 301
/**
 * Misc. utility functions.
 */
#define WM_VBOXTRAY_CHECK_HOSTVERSION           WM_APP + 1000


/* The tray icon's ID. */
#define ID_TRAYICON                     2000


/*
 * Timer IDs.
 */
#define TIMERID_VBOXTRAY_CHECK_HOSTVERSION      1000

/* The environment information for services. */
typedef struct _VBOXSERVICEENV
{
    HINSTANCE hInstance;
    HANDLE    hDriver;
    HANDLE    hStopEvent;
    /* display driver interface, XPDM - WDDM abstraction see VBOXDISPIF** definitions above */
    VBOXDISPIF dispIf;
} VBOXSERVICEENV;

/* The service initialization info and runtime variables. */
typedef struct _VBOXSERVICEINFO
{
    char     *pszName;
    int      (* pfnInit)             (const VBOXSERVICEENV *pEnv, void **ppInstance, bool *pfStartThread);
    unsigned (__stdcall * pfnThread) (void *pInstance);
    void     (* pfnDestroy)          (const VBOXSERVICEENV *pEnv, void *pInstance);

    /* Variables. */
    HANDLE hThread;
    void  *pInstance;
    bool   fStarted;

} VBOXSERVICEINFO;


extern HWND         gToolWindow;
extern HINSTANCE    gInstance;

extern void VBoxServiceReloadCursor(void);

#endif /* !___VBOXTRAY_H */


/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * UIMachineMenuBar class declaration
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

#ifndef __UIMachineMenuBar_h__
#define __UIMachineMenuBar_h__

class UISession;
class UIActionsPool;
class QMenuBar;
class QMenu;

class UIMachineMenuBar
{
public:
    UIMachineMenuBar();

    QMenuBar* createMenuBar(UISession *pSession);

protected:

    void prepareMenuMachine(QMenu *pMenu, UIActionsPool *pActionsPool);
    void prepareMenuDevices(QMenu *pMenu, UIActionsPool *pActionsPool);
#ifdef VBOX_WITH_DEBUGGER_GUI
    void prepareMenuDebug(QMenu *pMenu, UIActionsPool *pActionsPool);
#endif
    void prepareMenuHelp(QMenu *pMenu, UIActionsPool *pActionsPool);

    bool m_fIsFirstTime;
};

#endif /* __UIMachineMenuBar_h__ */


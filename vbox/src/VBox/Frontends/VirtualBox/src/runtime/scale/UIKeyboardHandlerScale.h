/* $Id: UIKeyboardHandlerScale.h 55401 2015-04-23 10:03:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIKeyboardHandlerScale class declaration.
 */

/*
 * Copyright (C) 2010-2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIKeyboardHandlerScale_h___
#define ___UIKeyboardHandlerScale_h___

/* GUI includes: */
#include "UIKeyboardHandler.h"

/** UIKeyboardHandler reimplementation
  * providing machine-logic with PopupMenu keyboard handler. */
class UIKeyboardHandlerScale : public UIKeyboardHandler
{
    Q_OBJECT;

protected:

    /** Scale keyboard-handler constructor. */
    UIKeyboardHandlerScale(UIMachineLogic *pMachineLogic);
    /** Scale keyboard-handler destructor. */
    virtual ~UIKeyboardHandlerScale();

private:

#ifndef Q_WS_MAC
    /** General event-filter. */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);
#endif /* !Q_WS_MAC */

    /* Friend class: */
    friend class UIKeyboardHandler;
};

#endif /* !___UIKeyboardHandlerScale_h___ */

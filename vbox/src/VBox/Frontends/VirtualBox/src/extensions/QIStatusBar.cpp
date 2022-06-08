/* $Id: QIStatusBar.cpp 65252 2017-01-12 10:16:05Z vboxsync $ */
/** @file
 * VBox Qt GUI - VirtualBox Qt extensions: QIStatusBar class implementation.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else
/* GUI includes: */
# include "QIStatusBar.h"
#endif


QIStatusBar::QIStatusBar(QWidget *pParent)
    : QStatusBar(pParent)
{
    /* Make sure we remember the last one status message: */
    connect(this, SIGNAL(messageChanged(const QString &)),
            this, SLOT(sltRememberLastMessage(const QString &)));

    /* Remove that ugly border around the status-bar items on every platform: */
    setStyleSheet("QStatusBar::item { border: 0px none black; }");
}


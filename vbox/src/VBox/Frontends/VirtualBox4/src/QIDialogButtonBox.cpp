/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * VirtualBox Qt extensions: QIDialogButtonBox class implementation
 */

/*
 * Copyright (C) 2008 Sun Microsystems, Inc.
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

#include "QIDialogButtonBox.h"

/* Qt includes */
#include <QPushButton>

QIDialogButtonBox::QIDialogButtonBox (StandardButtons aButtons, Qt::Orientation aOrientation, QWidget *aParent)
   : QDialogButtonBox (aButtons, aOrientation, aParent)
{
    initHelpButton ();
}

QPushButton *QIDialogButtonBox::addButton (const QString &aText, ButtonRole aRole)
{
    QPushButton *btn = QDialogButtonBox::addButton (aText, aRole);
    initHelpButton();
    return btn;
}

QPushButton *QIDialogButtonBox::addButton (StandardButton aButton)
{
    QPushButton *btn = QDialogButtonBox::addButton (aButton);
    initHelpButton();
    return btn;
}

void QIDialogButtonBox::setStandardButtons (StandardButtons aButtons)
{
    QDialogButtonBox::setStandardButtons (aButtons);
    initHelpButton();
}

void QIDialogButtonBox::initHelpButton()
{
    QPushButton *btn = button (QDialogButtonBox::Help);
    if (btn)
    {
        if (btn->text() == tr ("Help"))
            btn->setText (tr ("&Help"));
        if (btn->shortcut().isEmpty())
            btn->setShortcut (QKeySequence (tr ("F1"))); 
    }
}


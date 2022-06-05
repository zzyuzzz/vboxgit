/* $Id: QIArrowButtonSwitch.cpp 51873 2014-07-04 16:22:24Z vboxsync $ */
/** @file
 * VBox Qt GUI - QIArrowButtonSwitch class implementation.
 */

/*
 * Copyright (C) 2006-2014 Oracle Corporation
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
#include <QKeyEvent>

/* GUI includes: */
#include "QIArrowButtonSwitch.h"

QIArrowButtonSwitch::QIArrowButtonSwitch(QWidget *pParent /* = 0 */)
    : QIRichToolButton(pParent)
    , m_buttonState(ButtonState_Collapsed)
{
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::setIconForButtonState(QIArrowButtonSwitch::ButtonState buttonState, const QIcon &icon)
{
    /* Assign icon: */
    m_icons[buttonState] = icon;
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::sltButtonClicked()
{
    /* Toggle button-state: */
    m_buttonState = m_buttonState == ButtonState_Collapsed ?
                    ButtonState_Expanded : ButtonState_Collapsed;
    /* Update icon: */
    updateIcon();
}

void QIArrowButtonSwitch::keyPressEvent(QKeyEvent *pEvent)
{
    /* Handle different keys: */
    switch (pEvent->key())
    {
        /* Animate-click for the Space key: */
        case Qt::Key_Minus: if (m_buttonState == ButtonState_Expanded) return animateClick(); break;
        case Qt::Key_Plus: if (m_buttonState == ButtonState_Collapsed) return animateClick(); break;
        default: break;
    }
    /* Call to base-class: */
    QIRichToolButton::keyPressEvent(pEvent);
}

void QIArrowButtonSwitch::updateIcon()
{
    setIcon(m_icons.value(m_buttonState));
}


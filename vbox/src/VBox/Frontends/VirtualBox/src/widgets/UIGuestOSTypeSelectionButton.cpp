/* $Id: UIGuestOSTypeSelectionButton.cpp 94044 2022-03-01 14:56:01Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGuestOSTypeSelectionButton class implementation.
 */

/*
 * Copyright (C) 2009-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes */
#include <QMenu>
#include <QSignalMapper>
#include <QStyle>

/* GUI includes */
#include "UICommon.h"
#include "UIGuestOSTypeSelectionButton.h"
#include "UIIconPool.h"


UIGuestOSTypeSelectionButton::UIGuestOSTypeSelectionButton(QWidget *pParent)
    : QIWithRetranslateUI<QPushButton>(pParent)
{
    /* Determine icon metric: */
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    setIconSize(QSize(iIconMetric, iIconMetric));

    /* We have to make sure that the button has strong focus, otherwise
     * the editing is ended when the menu is shown: */
    setFocusPolicy(Qt::StrongFocus);

    /* Create a signal mapper so that we not have to react to
     * every single menu activation ourself: */
    m_pSignalMapper = new QSignalMapper(this);
    if (m_pSignalMapper)
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
        connect(m_pSignalMapper, static_cast<void(QSignalMapper::*)(const QString &)>(&QSignalMapper::mappedString),
                this, &UIGuestOSTypeSelectionButton::setOSTypeId);
#else
        connect(m_pSignalMapper, static_cast<void(QSignalMapper::*)(const QString &)>(&QSignalMapper::mapped),
                this, &UIGuestOSTypeSelectionButton::setOSTypeId);
#endif

    /* Create main menu: */
    m_pMainMenu = new QMenu(pParent);
    if (m_pMainMenu)
        setMenu(m_pMainMenu);

    /* Apply language settings: */
    retranslateUi();
}

bool UIGuestOSTypeSelectionButton::isMenuShown() const
{
    return m_pMainMenu->isVisible();
}

void UIGuestOSTypeSelectionButton::setOSTypeId(const QString &strOSTypeId)
{
    m_strOSTypeId = strOSTypeId;
    CGuestOSType enmType = uiCommon().vmGuestOSType(strOSTypeId);

#ifndef VBOX_WS_MAC
    /* Looks ugly on the Mac: */
    setIcon(generalIconPool().guestOSTypePixmapDefault(enmType.GetId()));
#endif

    setText(enmType.GetDescription());
}

void UIGuestOSTypeSelectionButton::retranslateUi()
{
    populateMenu();
}

void UIGuestOSTypeSelectionButton::populateMenu()
{
    /* Clea initially: */
    m_pMainMenu->clear();

    /* Create a list of all possible OS types: */
    foreach(const QString &strFamilyId, uiCommon().vmGuestOSFamilyIDs())
    {
        QMenu *pSubMenu = m_pMainMenu->addMenu(uiCommon().vmGuestOSFamilyDescription(strFamilyId));
        foreach (const CGuestOSType &comType, uiCommon().vmGuestOSTypeList(strFamilyId))
        {
            QAction *pAction = pSubMenu->addAction(generalIconPool().guestOSTypePixmapDefault(comType.GetId()),
                                                   comType.GetDescription());
            connect(pAction, &QAction::triggered,
                    m_pSignalMapper, static_cast<void(QSignalMapper::*)(void)>(&QSignalMapper::map));
            m_pSignalMapper->setMapping(pAction, comType.GetId());
        }
    }
}

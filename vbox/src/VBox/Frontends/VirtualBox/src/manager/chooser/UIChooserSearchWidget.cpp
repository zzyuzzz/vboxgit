/* $Id: UIChooserSearchWidget.cpp 77750 2019-03-18 11:51:29Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserSearchWidget class implementation.
 */

/*
 * Copyright (C) 2012-2019 Oracle Corporation
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
#include <QVBoxLayout>

/* GUI includes: */
#include "QILineEdit.h"
#include "QIToolButton.h"
#include "UIChooserDefs.h"
#include "UIChooserSearchWidget.h"
#include "UIIconPool.h"
#include "UISearchLineEdit.h"

UIChooserSearchWidget::UIChooserSearchWidget(QWidget *pParent)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pLineEdit(0)
    , m_pMainLayout(0)
    , m_pScrollToNextMatchButton(0)
    , m_pScrollToPreviousMatchButton(0)
{
    prepareWidgets();
    prepareConnections();
}

void UIChooserSearchWidget::setMatchCount(int iMatchCount)
{
    if (!m_pLineEdit)
        return;
    m_pLineEdit->setMatchCount(iMatchCount);
}

void UIChooserSearchWidget::setScroolToIndex(int iScrollToIndex)
{
    if (!m_pLineEdit)
        return;
    m_pLineEdit->setScroolToIndex(iScrollToIndex);
}

void UIChooserSearchWidget::appendToSearchString(const QString &strSearchText)
{
    if (!m_pLineEdit)
        return;
    m_pLineEdit->setText(m_pLineEdit->text().append(strSearchText));
}

void UIChooserSearchWidget::prepareWidgets()
{
    m_pMainLayout = new QHBoxLayout;
    if (!m_pMainLayout)
        return;
    m_pMainLayout->setSpacing(2);
    m_pMainLayout->setContentsMargins(0, 0, 0, 0);
    m_pLineEdit = new UISearchLineEdit;
    if (m_pLineEdit)
    {
        m_pMainLayout->addWidget(m_pLineEdit);
        m_pLineEdit->installEventFilter(this);
    }

    m_pScrollToPreviousMatchButton = new QIToolButton;
    if (m_pScrollToPreviousMatchButton)
    {
        m_pScrollToPreviousMatchButton->setIcon(UIIconPool::iconSet(":/log_viewer_search_backward_16px.png", ":/log_viewer_search_backward_disabled_16px.png"));
        m_pMainLayout->addWidget(m_pScrollToPreviousMatchButton);
    }
    m_pScrollToNextMatchButton = new QIToolButton;
    if (m_pScrollToNextMatchButton)
    {
        m_pScrollToNextMatchButton->setIcon(UIIconPool::iconSet(":/log_viewer_search_forward_16px.png", ":/log_viewer_search_forward_disabled_16px.png"));
        m_pMainLayout->addWidget(m_pScrollToNextMatchButton);
    }

    setLayout(m_pMainLayout);
}

void UIChooserSearchWidget::prepareConnections()
{
    if (m_pLineEdit)
    {
        connect(m_pLineEdit, &QILineEdit::textEdited,
                this, &UIChooserSearchWidget::sltHandleSearchTermChange);
    }
    if (m_pScrollToPreviousMatchButton)
        connect(m_pScrollToPreviousMatchButton, &QIToolButton::clicked, this, &UIChooserSearchWidget::sltHandleScroolToButtonClick);
    if (m_pScrollToNextMatchButton)
        connect(m_pScrollToNextMatchButton, &QIToolButton::clicked, this, &UIChooserSearchWidget::sltHandleScroolToButtonClick);
}

void UIChooserSearchWidget::showEvent(QShowEvent *pEvent)
{
    Q_UNUSED(pEvent);
    if (m_pLineEdit)
        m_pLineEdit->setFocus();
}

void UIChooserSearchWidget::hideEvent(QHideEvent *pEvent)
{
    Q_UNUSED(pEvent);
    if (m_pLineEdit)
        m_pLineEdit->clear();
}

void UIChooserSearchWidget::retranslateUi()
{
}

bool UIChooserSearchWidget::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    Q_UNUSED(pWatched);
    if (pEvent->type() == QEvent::KeyPress)
    {
        QKeyEvent *pKeyEvent = dynamic_cast<QKeyEvent*>(pEvent);
        if (pKeyEvent)
        {
            if (pKeyEvent->key() == Qt::Key_Escape)
                emit sigToggleVisibility(false);
        }
    }
    return false;
}

void UIChooserSearchWidget::sltHandleSearchTermChange(const QString &strSearchTerm)
{
    emit sigRedoSearch(strSearchTerm, UIChooserItemSearchFlag_Machine);
}

void UIChooserSearchWidget::sltHandleScroolToButtonClick()
{
    if (sender() == m_pScrollToNextMatchButton)
        emit sigScrollToMatch(true);
    else if (sender() == m_pScrollToPreviousMatchButton)
        emit sigScrollToMatch(false);
}

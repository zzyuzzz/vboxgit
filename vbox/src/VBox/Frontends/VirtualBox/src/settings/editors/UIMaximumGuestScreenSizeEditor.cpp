/* $Id: UIMaximumGuestScreenSizeEditor.cpp 93992 2022-02-28 17:56:58Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMaximumGuestScreenSizeEditor class implementation.
 */

/*
 * Copyright (C) 2019-2022 Oracle Corporation
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
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

/* GUI includes: */
#include "QIComboBox.h"
#include "UICommon.h"
#include "UIConverter.h"
#include "UIMaximumGuestScreenSizeEditor.h"


/*********************************************************************************************************************************
*   Class UIMaximumGuestScreenSizeValue implementation.                                                                          *
*********************************************************************************************************************************/

UIMaximumGuestScreenSizeValue::UIMaximumGuestScreenSizeValue(MaximumGuestScreenSizePolicy enmPolicy /* = MaximumGuestScreenSizePolicy_Any */,
                                                             const QSize &size /* = QSize() */)
    : m_enmPolicy(enmPolicy)
    , m_size(size)
{
}

bool UIMaximumGuestScreenSizeValue::equal(const UIMaximumGuestScreenSizeValue &other) const
{
    return true
           && (m_enmPolicy == other.m_enmPolicy)
           && (m_size == other.m_size)
           ;
}


/*********************************************************************************************************************************
*   Class UIMaximumGuestScreenSizeEditor implementation.                                                                         *
*********************************************************************************************************************************/

UIMaximumGuestScreenSizeEditor::UIMaximumGuestScreenSizeEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_pLayout(0)
    , m_pLabelPolicy(0)
    , m_pComboPolicy(0)
    , m_pLabelMaxWidth(0)
    , m_pSpinboxMaxWidth(0)
    , m_pLabelMaxHeight(0)
    , m_pSpinboxMaxHeight(0)
{
    prepare();
}

QWidget *UIMaximumGuestScreenSizeEditor::focusProxy1() const
{
    return m_pComboPolicy->focusProxy();
}

QWidget *UIMaximumGuestScreenSizeEditor::focusProxy2() const
{
    return m_pSpinboxMaxWidth;
}

QWidget *UIMaximumGuestScreenSizeEditor::focusProxy3() const
{
    return m_pSpinboxMaxHeight;
}

void UIMaximumGuestScreenSizeEditor::setValue(const UIMaximumGuestScreenSizeValue &guiValue)
{
    /* Update cached value if value has changed: */
    if (m_guiValue != guiValue)
        m_guiValue = guiValue;

    /* Look for proper policy index to choose: */
    if (m_pComboPolicy)
    {
        const int iIndex = m_pComboPolicy->findData(QVariant::fromValue(guiValue.m_enmPolicy));
        if (iIndex != -1)
        {
            m_pComboPolicy->setCurrentIndex(iIndex);
            sltHandleCurrentPolicyIndexChanged();
        }
    }
    /* Load size as well: */
    if (m_pSpinboxMaxWidth && m_pSpinboxMaxHeight)
    {
        if (guiValue.m_enmPolicy == MaximumGuestScreenSizePolicy_Fixed)
        {
            m_pSpinboxMaxWidth->setValue(guiValue.m_size.width());
            m_pSpinboxMaxHeight->setValue(guiValue.m_size.height());
        }
    }
}

UIMaximumGuestScreenSizeValue UIMaximumGuestScreenSizeEditor::value() const
{
    return   m_pComboPolicy && m_pSpinboxMaxWidth && m_pSpinboxMaxHeight
           ? UIMaximumGuestScreenSizeValue(m_pComboPolicy->currentData().value<MaximumGuestScreenSizePolicy>(),
                                           QSize(m_pSpinboxMaxWidth->value(), m_pSpinboxMaxHeight->value()))
           : m_guiValue;
}

int UIMaximumGuestScreenSizeEditor::minimumLabelHorizontalHint() const
{
    int iMinimumHint = 0;
    iMinimumHint = qMax(iMinimumHint, m_pLabelPolicy->minimumSizeHint().width());
    iMinimumHint = qMax(iMinimumHint, m_pLabelMaxWidth->minimumSizeHint().width());
    iMinimumHint = qMax(iMinimumHint, m_pLabelMaxHeight->minimumSizeHint().width());
    return iMinimumHint;
}

void UIMaximumGuestScreenSizeEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayout)
        m_pLayout->setColumnMinimumWidth(0, iIndent);
}

void UIMaximumGuestScreenSizeEditor::retranslateUi()
{
    if (m_pLabelPolicy)
        m_pLabelPolicy->setText(tr("Maximum Guest Screen &Size:"));
    if (m_pLabelMaxWidth)
        m_pLabelMaxWidth->setText(tr("&Width:"));
    if (m_pSpinboxMaxWidth)
        m_pSpinboxMaxWidth->setToolTip(tr("Holds the maximum width which we would like the guest to use."));
    if (m_pLabelMaxHeight)
        m_pLabelMaxHeight->setText(tr("&Height:"));
    if (m_pSpinboxMaxHeight)
        m_pSpinboxMaxHeight->setToolTip(tr("Holds the maximum height which we would like the guest to use."));

    if (m_pComboPolicy)
    {
        m_pComboPolicy->setToolTip(tr("Selects maximum guest screen size policy."));
        for (int i = 0; i < m_pComboPolicy->count(); ++i)
        {
            const MaximumGuestScreenSizePolicy enmType = m_pComboPolicy->itemData(i).value<MaximumGuestScreenSizePolicy>();
            m_pComboPolicy->setItemText(i, gpConverter->toString(enmType));
        }
    }
}

void UIMaximumGuestScreenSizeEditor::sltHandleCurrentPolicyIndexChanged()
{
    if (m_pComboPolicy)
    {
        /* Get current size-combo tool-tip data: */
        const QString strCurrentComboItemTip = m_pComboPolicy->currentData(Qt::ToolTipRole).toString();
        m_pComboPolicy->setWhatsThis(strCurrentComboItemTip);

        /* Get current size-combo item data: */
        const MaximumGuestScreenSizePolicy enmPolicy = m_pComboPolicy->currentData().value<MaximumGuestScreenSizePolicy>();
        /* Should be combo-level widgets enabled? */
        const bool fComboLevelWidgetsEnabled = enmPolicy == MaximumGuestScreenSizePolicy_Fixed;
        /* Enable/disable combo-level widgets: */
        if (m_pLabelMaxWidth)
            m_pLabelMaxWidth->setEnabled(fComboLevelWidgetsEnabled);
        if (m_pSpinboxMaxWidth)
            m_pSpinboxMaxWidth->setEnabled(fComboLevelWidgetsEnabled);
        if (m_pLabelMaxHeight)
            m_pLabelMaxHeight->setEnabled(fComboLevelWidgetsEnabled);
        if (m_pSpinboxMaxHeight)
            m_pSpinboxMaxHeight->setEnabled(fComboLevelWidgetsEnabled);

        /* Notify listeners: */
        emit sigValueChanged(value());
    }
}

void UIMaximumGuestScreenSizeEditor::sltHandleSizeChanged()
{
    if (m_pSpinboxMaxWidth && m_pSpinboxMaxHeight)
    {
        /* Notify listeners: */
        emit sigValueChanged(value());
    }
}

void UIMaximumGuestScreenSizeEditor::prepare()
{
    /* Create main layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        m_pLayout->setContentsMargins(0, 0, 0, 0);
        m_pLayout->setColumnStretch(1, 1);

        const int iMinWidth = 640;
        const int iMinHeight = 480;
        const int iMaxSize = 16 * _1K;

        /* Prepare policy label: */
        m_pLabelPolicy = new QLabel(this);
        if (m_pLabelPolicy)
        {
           m_pLabelPolicy->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
           m_pLayout->addWidget(m_pLabelPolicy, 0, 0);
        }
        /* Prepare policy combo: */
        m_pComboPolicy = new QIComboBox(this);
        if (m_pComboPolicy)
        {
            if (m_pLabelPolicy)
                m_pLabelPolicy->setBuddy(m_pComboPolicy);
            connect(m_pComboPolicy, static_cast<void(QIComboBox::*)(int)>(&QIComboBox::activated),
                    this, &UIMaximumGuestScreenSizeEditor::sltHandleCurrentPolicyIndexChanged);

            m_pLayout->addWidget(m_pComboPolicy, 0, 1);
        }

        /* Prepare max width label: */
        m_pLabelMaxWidth = new QLabel(this);
        if (m_pLabelMaxWidth)
        {
            m_pLabelMaxWidth->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabelMaxWidth, 1, 0);
        }
        /* Prepare max width spinbox: */
        m_pSpinboxMaxWidth = new QSpinBox(this);
        if (m_pSpinboxMaxWidth)
        {
            if (m_pLabelMaxWidth)
                m_pLabelMaxWidth->setBuddy(m_pSpinboxMaxWidth);
            m_pSpinboxMaxWidth->setMinimum(iMinWidth);
            m_pSpinboxMaxWidth->setMaximum(iMaxSize);
            connect(m_pSpinboxMaxWidth, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &UIMaximumGuestScreenSizeEditor::sltHandleSizeChanged);

            m_pLayout->addWidget(m_pSpinboxMaxWidth, 1, 1);
        }

        /* Prepare max height label: */
        m_pLabelMaxHeight = new QLabel(this);
        if (m_pLabelMaxHeight)
        {
            m_pLabelMaxHeight->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabelMaxHeight, 2, 0);
        }
        /* Prepare max width spinbox: */
        m_pSpinboxMaxHeight = new QSpinBox(this);
        if (m_pSpinboxMaxHeight)
        {
            if (m_pLabelMaxHeight)
                m_pLabelMaxHeight->setBuddy(m_pSpinboxMaxHeight);
            m_pSpinboxMaxHeight->setMinimum(iMinHeight);
            m_pSpinboxMaxHeight->setMaximum(iMaxSize);
            connect(m_pSpinboxMaxHeight, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
                    this, &UIMaximumGuestScreenSizeEditor::sltHandleSizeChanged);

            m_pLayout->addWidget(m_pSpinboxMaxHeight, 2, 1);
        }
    }

    /* Populate combo: */
    populateCombo();

    /* Apply language settings: */
    retranslateUi();
}

void UIMaximumGuestScreenSizeEditor::populateCombo()
{
    if (m_pComboPolicy)
    {
        /* Clear combo first of all: */
        m_pComboPolicy->clear();

        /* Init currently supported maximum guest size policy types: */
        QList<MaximumGuestScreenSizePolicy> supportedValues = QList<MaximumGuestScreenSizePolicy>()
                                                          << MaximumGuestScreenSizePolicy_Automatic
                                                          << MaximumGuestScreenSizePolicy_Any
                                                          << MaximumGuestScreenSizePolicy_Fixed;

        /* Update combo with all the supported values: */
        foreach (const MaximumGuestScreenSizePolicy &enmType, supportedValues)
            m_pComboPolicy->addItem(QString(), QVariant::fromValue(enmType));

        /* Retranslate finally: */
        retranslateUi();
    }
}

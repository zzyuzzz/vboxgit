/* $Id: UIGraphicsControllerEditor.cpp 94358 2022-03-24 15:24:02Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGraphicsControllerEditor class implementation.
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
#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>

/* GUI includes: */
#include "UICommon.h"
#include "UIConverter.h"
#include "UIGraphicsControllerEditor.h"

/* COM includes: */
#include "CSystemProperties.h"


UIGraphicsControllerEditor::UIGraphicsControllerEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmValue(KGraphicsControllerType_Max)
    , m_pLayout(0)
    , m_pLabel(0)
    , m_pCombo(0)
{
    prepare();
}

void UIGraphicsControllerEditor::setValue(KGraphicsControllerType enmValue)
{
    if (m_pCombo)
    {
        /* Update cached value and
         * combo if value has changed: */
        if (m_enmValue != enmValue)
        {
            m_enmValue = enmValue;
            populateCombo();
        }

        /* Look for proper index to choose: */
        int iIndex = m_pCombo->findData(QVariant::fromValue(m_enmValue));
        if (iIndex != -1)
            m_pCombo->setCurrentIndex(iIndex);
    }
}

KGraphicsControllerType UIGraphicsControllerEditor::value() const
{
    return m_pCombo ? m_pCombo->currentData().value<KGraphicsControllerType>() : m_enmValue;
}

int UIGraphicsControllerEditor::minimumLabelHorizontalHint() const
{
    return m_pLabel->minimumSizeHint().width();
}

void UIGraphicsControllerEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayout)
        m_pLayout->setColumnMinimumWidth(0, iIndent);
}

void UIGraphicsControllerEditor::retranslateUi()
{
    if (m_pLabel)
        m_pLabel->setText(tr("&Graphics Controller:"));
    if (m_pCombo)
    {
        for (int i = 0; i < m_pCombo->count(); ++i)
        {
            const KGraphicsControllerType enmType = m_pCombo->itemData(i).value<KGraphicsControllerType>();
            m_pCombo->setItemText(i, gpConverter->toString(enmType));
        }
        m_pCombo->setToolTip(tr("Selects the graphics adapter type the virtual machine will use."));
    }
}

void UIGraphicsControllerEditor::sltHandleCurrentIndexChanged()
{
    if (m_pCombo)
        emit sigValueChanged(m_pCombo->itemData(m_pCombo->currentIndex()).value<KGraphicsControllerType>());
}

void UIGraphicsControllerEditor::prepare()
{
    /* Create main layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        m_pLayout->setContentsMargins(0, 0, 0, 0);

        /* Create label: */
        m_pLabel = new QLabel(this);
        if (m_pLabel)
        {
            m_pLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabel, 0, 0);
        }

        /* Create combo layout: */
        QHBoxLayout *pLayoutCombo = new QHBoxLayout;
        if (pLayoutCombo)
        {
            /* Create combo: */
            m_pCombo = new QComboBox(this);
            if (m_pCombo)
            {
                /* This is necessary since contents is dynamical now: */
                if (m_pLabel)
                    m_pLabel->setBuddy(m_pCombo);
                m_pCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
                connect(m_pCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
                        this, &UIGraphicsControllerEditor::sltHandleCurrentIndexChanged);

                pLayoutCombo->addWidget(m_pCombo);
            }

            /* Add stretch: */
            pLayoutCombo->addStretch();

            /* Add combo-layout into main-layout: */
            m_pLayout->addLayout(pLayoutCombo, 0, 1);
        }
    }

    /* Populate combo: */
    populateCombo();

    /* Apply language settings: */
    retranslateUi();
}

void UIGraphicsControllerEditor::populateCombo()
{
    if (m_pCombo)
    {
        /* Clear combo first of all: */
        m_pCombo->clear();

        /* Load currently supported graphics controller types: */
        CSystemProperties comProperties = uiCommon().virtualBox().GetSystemProperties();
        m_supportedValues = comProperties.GetSupportedGraphicsControllerTypes();

        /* Make sure requested value if sane is present as well: */
        if (   m_enmValue != KGraphicsControllerType_Max
            && !m_supportedValues.contains(m_enmValue))
            m_supportedValues.prepend(m_enmValue);

        /* Update combo with all the supported values: */
        foreach (const KGraphicsControllerType &enmType, m_supportedValues)
            m_pCombo->addItem(QString(), QVariant::fromValue(enmType));

        /* Retranslate finally: */
        retranslateUi();
    }
}

/* $Id: UIAudioFeaturesEditor.cpp 94709 2022-04-25 16:16:06Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIAudioFeaturesEditor class implementation.
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
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
#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>

/* GUI includes: */
#include "UIAudioFeaturesEditor.h"


UIAudioFeaturesEditor::UIAudioFeaturesEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_fEnableOutput(false)
    , m_fEnableInput(false)
    , m_pLabel(0)
    , m_pCheckBoxEnableOutput(0)
    , m_pCheckBoxEnableInput(0)
{
    prepare();
}

void UIAudioFeaturesEditor::setEnableOutput(bool fOn)
{
    /* Update cached value and
     * check-box if value has changed: */
    if (m_fEnableOutput != fOn)
    {
        m_fEnableOutput = fOn;
        if (m_pCheckBoxEnableOutput)
            m_pCheckBoxEnableOutput->setCheckState(m_fEnableOutput ? Qt::Checked : Qt::Unchecked);
    }
}

bool UIAudioFeaturesEditor::outputEnabled() const
{
    return   m_pCheckBoxEnableOutput
           ? m_pCheckBoxEnableOutput->checkState() == Qt::Checked
           : m_fEnableOutput;
}

void UIAudioFeaturesEditor::setEnableInput(bool fOn)
{
    /* Update cached value and
     * check-box if value has changed: */
    if (m_fEnableInput != fOn)
    {
        m_fEnableInput = fOn;
        if (m_pCheckBoxEnableInput)
            m_pCheckBoxEnableInput->setCheckState(m_fEnableInput ? Qt::Checked : Qt::Unchecked);
    }
}

bool UIAudioFeaturesEditor::inputEnabled() const
{
    return   m_pCheckBoxEnableInput
           ? m_pCheckBoxEnableInput->checkState() == Qt::Checked
           : m_fEnableInput;
}

int UIAudioFeaturesEditor::minimumLabelHorizontalHint() const
{
    return m_pLabel ? m_pLabel->minimumSizeHint().width() : 0;
}

void UIAudioFeaturesEditor::setMinimumLayoutIndent(int iIndent)
{
    if (m_pLayout)
        m_pLayout->setColumnMinimumWidth(0, iIndent);
}

void UIAudioFeaturesEditor::retranslateUi()
{
    if (m_pLabel)
        m_pLabel->setText(tr("Extended Features:"));
    if (m_pCheckBoxEnableOutput)
    {
        m_pCheckBoxEnableOutput->setText(tr("Enable Audio &Output"));
        m_pCheckBoxEnableOutput->setToolTip(tr("When checked, output to the virtual audio device will reach the host. "
                                               "Otherwise the guest is muted."));
    }
    if (m_pCheckBoxEnableInput)
    {
        m_pCheckBoxEnableInput->setText(tr("Enable Audio &Input"));
        m_pCheckBoxEnableInput->setToolTip(tr("When checked, the guest will be able to capture audio input from the host. "
                                              "Otherwise the guest will capture only silence."));
    }
}

void UIAudioFeaturesEditor::prepare()
{
    /* Prepare main layout: */
    m_pLayout = new QGridLayout(this);
    if (m_pLayout)
    {
        m_pLayout->setContentsMargins(0, 0, 0, 0);
        m_pLayout->setColumnStretch(1, 1);

        /* Prepare label: */
        m_pLabel = new QLabel(this);
        if (m_pLabel)
        {
            m_pLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
            m_pLayout->addWidget(m_pLabel, 0, 0);
        }
        /* Prepare 'enable output' check-box: */
        m_pCheckBoxEnableOutput = new QCheckBox(this);
        if (m_pCheckBoxEnableOutput)
            m_pLayout->addWidget(m_pCheckBoxEnableOutput, 0, 1);
        /* Prepare 'enable input' check-box: */
        m_pCheckBoxEnableInput = new QCheckBox(this);
        if (m_pCheckBoxEnableInput)
            m_pLayout->addWidget(m_pCheckBoxEnableInput, 1, 1);
    }

    /* Apply language settings: */
    retranslateUi();
}

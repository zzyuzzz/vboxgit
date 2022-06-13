/* $Id: UIAutoCaptureKeyboardEditor.cpp 94395 2022-03-29 16:29:26Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIAutoCaptureKeyboardEditor class implementation.
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
#include "UIAutoCaptureKeyboardEditor.h"


UIAutoCaptureKeyboardEditor::UIAutoCaptureKeyboardEditor(QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_fValue(false)
    , m_pLabel(0)
    , m_pCheckBox(0)
{
    prepare();
}

void UIAutoCaptureKeyboardEditor::setValue(bool fValue)
{
    /* Update cached value and
     * check-box if value has changed: */
    if (m_fValue != fValue)
    {
        m_fValue = fValue;
        if (m_pCheckBox)
            m_pCheckBox->setCheckState(m_fValue ? Qt::Checked : Qt::Unchecked);
    }
}

bool UIAutoCaptureKeyboardEditor::value() const
{
    return m_pCheckBox ? m_pCheckBox->checkState() == Qt::Checked : m_fValue;
}

void UIAutoCaptureKeyboardEditor::retranslateUi()
{
    if (m_pLabel)
        m_pLabel->setText(tr("Extended Features:"));
    if (m_pCheckBox)
    {
        m_pCheckBox->setText(tr("&Auto Capture Keyboard"));
        m_pCheckBox->setToolTip(tr("When checked, the keyboard is automatically captured every time the VM window is "
                                   "activated. When the keyboard is captured, all keystrokes (including system ones like "
                                   "Alt-Tab) are directed to the VM."));
    }
}

void UIAutoCaptureKeyboardEditor::prepare()
{
    /* Prepare main layout: */
    QGridLayout *pLayout = new QGridLayout(this);
    if (pLayout)
    {
        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->setColumnStretch(1, 1);

        /* Prepare label: */
        m_pLabel = new QLabel(this);
        if (m_pLabel)
            pLayout->addWidget(m_pLabel, 0, 0);
        /* Prepare check-box: */
        m_pCheckBox = new QCheckBox(this);
        if (m_pCheckBox)
            pLayout->addWidget(m_pCheckBox, 0, 1);
    }

    /* Apply language settings: */
    retranslateUi();
}

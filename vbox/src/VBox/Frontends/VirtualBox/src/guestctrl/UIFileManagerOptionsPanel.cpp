/* $Id: UIFileManagerOptionsPanel.cpp 76553 2019-01-01 01:45:53Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class implementation.
 */

/*
 * Copyright (C) 2010-2019 Oracle Corporation
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
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QComboBox>
# include <QHBoxLayout>
# include <QFontDatabase>
# include <QFontDialog>
# include <QCheckBox>
# include <QLabel>
# include <QSpinBox>

/* GUI includes: */
# include "QIToolButton.h"
# include "UIIconPool.h"
# include "UIFileManager.h"
# include "UIFileManagerOptionsPanel.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

UIFileManagerOptionsPanel::UIFileManagerOptionsPanel(UIFileManager *pManagerWidget,
                                                                               QWidget *pParent, UIFileManagerOptions *pFileManagerOptions)
    : UIFileManagerPanel(pManagerWidget, pParent)
    , m_pListDirectoriesOnTopCheckBox(0)
    , m_pDeleteConfirmationCheckBox(0)
    , m_pHumanReabableSizesCheckBox(0)
    , m_pFileManagerOptions(pFileManagerOptions)
{
    prepare();
}

QString UIFileManagerOptionsPanel::panelName() const
{
    return "OptionsPanel";
}

void UIFileManagerOptionsPanel::update()
{
    if (!m_pFileManagerOptions)
        return;

    if (m_pListDirectoriesOnTopCheckBox)
    {
        m_pListDirectoriesOnTopCheckBox->blockSignals(true);
        m_pListDirectoriesOnTopCheckBox->setChecked(m_pFileManagerOptions->bListDirectoriesOnTop);
        m_pListDirectoriesOnTopCheckBox->blockSignals(false);
    }

    if (m_pDeleteConfirmationCheckBox)
    {
        m_pDeleteConfirmationCheckBox->blockSignals(true);
        m_pDeleteConfirmationCheckBox->setChecked(m_pFileManagerOptions->bAskDeleteConfirmation);
        m_pDeleteConfirmationCheckBox->blockSignals(false);
    }

    if (m_pHumanReabableSizesCheckBox)
    {
        m_pHumanReabableSizesCheckBox->blockSignals(true);
        m_pHumanReabableSizesCheckBox->setChecked(m_pFileManagerOptions->bShowHumanReadableSizes);
        m_pHumanReabableSizesCheckBox->blockSignals(false);
    }
}

void UIFileManagerOptionsPanel::prepareWidgets()
{
    if (!mainLayout())
        return;

    m_pListDirectoriesOnTopCheckBox = new QCheckBox;
    if (m_pListDirectoriesOnTopCheckBox)
    {
        mainLayout()->addWidget(m_pListDirectoriesOnTopCheckBox, 0, Qt::AlignLeft);
    }

    m_pDeleteConfirmationCheckBox = new QCheckBox;
    if (m_pDeleteConfirmationCheckBox)
    {
        mainLayout()->addWidget(m_pDeleteConfirmationCheckBox, 0, Qt::AlignLeft);
    }

    m_pHumanReabableSizesCheckBox = new QCheckBox;
    if (m_pHumanReabableSizesCheckBox)
    {
        mainLayout()->addWidget(m_pHumanReabableSizesCheckBox, 0, Qt::AlignLeft);
    }
    /* Set initial checkbox status wrt. options: */
    if (m_pFileManagerOptions)
    {
        if (m_pListDirectoriesOnTopCheckBox)
            m_pListDirectoriesOnTopCheckBox->setChecked(m_pFileManagerOptions->bListDirectoriesOnTop);
        if (m_pDeleteConfirmationCheckBox)
            m_pDeleteConfirmationCheckBox->setChecked(m_pFileManagerOptions->bAskDeleteConfirmation);
        if (m_pHumanReabableSizesCheckBox)
            m_pHumanReabableSizesCheckBox->setChecked(m_pFileManagerOptions->bShowHumanReadableSizes);
    }
    retranslateUi();
    mainLayout()->addStretch(2);
}

void UIFileManagerOptionsPanel::sltListDirectoryCheckBoxToogled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->bListDirectoriesOnTop = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::sltDeleteConfirmationCheckBoxToogled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->bAskDeleteConfirmation = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::sltHumanReabableSizesCheckBoxToogled(bool bChecked)
{
    if (!m_pFileManagerOptions)
        return;
    m_pFileManagerOptions->bShowHumanReadableSizes = bChecked;
    emit sigOptionsChanged();
}

void UIFileManagerOptionsPanel::prepareConnections()
{
    if (m_pListDirectoriesOnTopCheckBox)
        connect(m_pListDirectoriesOnTopCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltListDirectoryCheckBoxToogled);
    if (m_pDeleteConfirmationCheckBox)
        connect(m_pDeleteConfirmationCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltDeleteConfirmationCheckBoxToogled);
    if (m_pHumanReabableSizesCheckBox)
        connect(m_pHumanReabableSizesCheckBox, &QCheckBox::toggled,
                this, &UIFileManagerOptionsPanel::sltHumanReabableSizesCheckBoxToogled);
}

void UIFileManagerOptionsPanel::retranslateUi()
{
    UIFileManagerPanel::retranslateUi();
    if (m_pListDirectoriesOnTopCheckBox)
    {
        m_pListDirectoriesOnTopCheckBox->setText(UIFileManager::tr("List directories on top"));
        m_pListDirectoriesOnTopCheckBox->setToolTip(UIFileManager::tr("List directories before files"));
    }

    if (m_pDeleteConfirmationCheckBox)
    {
        m_pDeleteConfirmationCheckBox->setText(UIFileManager::tr("Ask before delete"));
        m_pDeleteConfirmationCheckBox->setToolTip(UIFileManager::tr("Show a confirmation dialog "
                                                                                "before deleting files and directories"));
    }

    if (m_pHumanReabableSizesCheckBox)
    {
        m_pHumanReabableSizesCheckBox->setText(UIFileManager::tr("Human readable sizes"));
        m_pHumanReabableSizesCheckBox->setToolTip(UIFileManager::tr("Show file/directory sizes in human "
                                                                                "readable format rather than in bytes"));
    }
}

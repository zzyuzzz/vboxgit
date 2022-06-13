/* $Id: UIWizardCloneVMModePage.cpp 93115 2022-01-01 11:31:46Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMModePage class implementation.
 */

/*
 * Copyright (C) 2011-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Global includes: */
#include <QVBoxLayout>

/* Local includes: */
#include "UIWizardCloneVM.h"
#include "UIWizardCloneVMEditors.h"
#include "UIWizardCloneVMModePage.h"
#include "QIRichTextLabel.h"

UIWizardCloneVMModePage::UIWizardCloneVMModePage(bool fShowChildsOption)
    : m_pLabel(0)
    , m_pCloneModeGroupBox(0)
    , m_fShowChildsOption(fShowChildsOption)
{
    prepare();
}

void UIWizardCloneVMModePage::prepare()
{
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    AssertReturnVoid(pMainLayout);
    m_pLabel = new QIRichTextLabel(this);

    if (m_pLabel)
        pMainLayout->addWidget(m_pLabel);

    m_pCloneModeGroupBox = new UICloneVMCloneModeGroupBox(m_fShowChildsOption);
    if (m_pCloneModeGroupBox)
    {
        pMainLayout->addWidget(m_pCloneModeGroupBox);
        m_pCloneModeGroupBox->setFlat(true);
        connect(m_pCloneModeGroupBox, &UICloneVMCloneModeGroupBox::sigCloneModeChanged,
                this, &UIWizardCloneVMModePage::sltCloneModeChanged);
    }
    pMainLayout->addStretch();

    retranslateUi();
}

void UIWizardCloneVMModePage::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVM::tr("Snapshots"));

    /* Translate widgets: */
    const QString strGeneral = UIWizardCloneVM::tr("<p>Please choose which parts of the snapshot tree "
                                                   "should be cloned with the machine.</p>");
    const QString strOpt1    = UIWizardCloneVM::tr("<p>If you choose <b>Current machine state</b>, "
                                                   "the new machine will reflect the current state "
                                                   "of the original machine and will have no snapshots.</p>");
    const QString strOpt2    = UIWizardCloneVM::tr("<p>If you choose <b>Current snapshot tree branch</b>, "
                                                   "the new machine will reflect the current state "
                                                   "of the original machine and will have matching snapshots "
                                                   "for all snapshots in the tree branch "
                                                   "starting at the current state in the original machine.</p>");
    const QString strOpt3    = UIWizardCloneVM::tr("<p>If you choose <b>Everything</b>, "
                                                   "the new machine will reflect the current state "
                                                   "of the original machine and will have matching snapshots "
                                                   "for all snapshots in the original machine.</p>");
    if (m_fShowChildsOption)
        m_pLabel->setText(QString("<p>%1</p><p>%2 %3 %4</p>")
                          .arg(strGeneral)
                          .arg(strOpt1)
                          .arg(strOpt2)
                          .arg(strOpt3));
    else
        m_pLabel->setText(QString("<p>%1</p><p>%2 %3</p>")
                          .arg(strGeneral)
                          .arg(strOpt1)
                          .arg(strOpt3));
}

void UIWizardCloneVMModePage::initializePage()
{

    AssertReturnVoid(wizardWindow<UIWizardCloneVM>());
    if (m_pCloneModeGroupBox && !m_userModifiedParameters.contains("CloneMode"))
        wizardWindow<UIWizardCloneVM>()->setCloneMode(m_pCloneModeGroupBox->cloneMode());

    retranslateUi();
}

bool UIWizardCloneVMModePage::validatePage()
{
    bool fResult = true;

    UIWizardCloneVM *pWizard = wizardWindow<UIWizardCloneVM>();
    AssertReturn(pWizard, false);
    /* Try to clone VM: */
    fResult = pWizard->cloneVM();

    return fResult;
}

void UIWizardCloneVMModePage::sltCloneModeChanged(KCloneMode enmCloneMode)
{
    AssertReturnVoid(wizardWindow<UIWizardCloneVM>());
    m_userModifiedParameters << "CloneMode";
    wizardWindow<UIWizardCloneVM>()->setCloneMode(enmCloneMode);
}

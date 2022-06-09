/* $Id: UIWizardImportAppPageBasic1.cpp 72173 2018-05-09 13:38:13Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic1 class implementation.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
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

/* Global includes: */
# include <QFileInfo>
# include <QVBoxLayout>

/* Local includes: */
# include "QIRichTextLabel.h"
# include "UIEmptyFilePathSelector.h"
# include "UIFilePathSelector.h"
# include "UIWizardImportAppPageBasic1.h"
# include "UIWizardImportAppPageBasic2.h"
# include "UIWizardImportApp.h"
# include "VBoxGlobal.h"

/* COM includes: */
# include "CSystemProperties.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


UIWizardImportAppPage1::UIWizardImportAppPage1()
{
}

UIWizardImportAppPageBasic1::UIWizardImportAppPageBasic1()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pFileSelector = new UIEmptyFilePathSelector(this);
        {
            m_pFileSelector->setHomeDir(vboxGlobal().documentsPath());
            m_pFileSelector->setMode(UIEmptyFilePathSelector::Mode_File_Open);
            m_pFileSelector->setButtonPosition(UIEmptyFilePathSelector::RightPosition);
            m_pFileSelector->setEditable(true);
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pFileSelector);


        m_pDefaultFolderLabel = new QIRichTextLabel(this);
        if (m_pDefaultFolderLabel)
        {
            pMainLayout->addWidget(m_pDefaultFolderLabel);
        }

        m_pDestinationFolderSelector = new UIFilePathSelector(this);
        if (m_pDestinationFolderSelector)
        {
            m_pDestinationFolderSelector->setMode(UIFilePathSelector::Mode_Folder);
            pMainLayout->addWidget(m_pDestinationFolderSelector);

            m_pDestinationFolderSelector->setPath(vboxGlobal().virtualBox().GetSystemProperties().GetDefaultMachineFolder());
            m_pDestinationFolderSelector->setResetEnabled(false);
        }

        pMainLayout->addStretch();
    }

    /* Setup connections: */
    connect(m_pFileSelector, &UIEmptyFilePathSelector::pathChanged, this, &UIWizardImportAppPageBasic1::completeChanged);
}

void UIWizardImportAppPageBasic1::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardImportApp::tr("Appliance to import"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardImportApp::tr("<p>VirtualBox currently supports importing appliances "
                                            "saved in the Open Virtualization Format (OVF). "
                                            "To continue, select the file to import below.</p>"));
    m_pDefaultFolderLabel->setText(UIWizardImportApp::tr("Select a destionation folder for appliance"));
    m_pFileSelector->setChooseButtonToolTip(UIWizardImportApp::tr("Choose a virtual appliance file to import..."));
    m_pFileSelector->setFileDialogTitle(UIWizardImportApp::tr("Please choose a virtual appliance file to import"));
    m_pFileSelector->setFileFilters(UIWizardImportApp::tr("Open Virtualization Format (%1)").arg("*.ova *.ovf"));
}

void UIWizardImportAppPageBasic1::initializePage()
{
    /* Translate page: */
    retranslateUi();
}

bool UIWizardImportAppPageBasic1::isComplete() const
{
    /* Make sure appliance file has allowed extension and exists: */
    return VBoxGlobal::hasAllowedExtension(m_pFileSelector->path().toLower(), OVFFileExts) &&
           QFile::exists(m_pFileSelector->path());
}

bool UIWizardImportAppPageBasic1::validatePage()
{
    /* Get import appliance widget: */
    ImportAppliancePointer pImportApplianceWidget = field("applianceWidget").value<ImportAppliancePointer>();
    AssertMsg(!pImportApplianceWidget.isNull(), ("Appliance Widget is not set!\n"));

    /* If file name was changed: */
    if (m_pFileSelector->isModified())
    {
        /* Check if set file contains valid appliance: */
        if (!pImportApplianceWidget->setFile(m_pFileSelector->path()))
            return false;
        /* Reset the modified bit afterwards: */
        m_pFileSelector->resetModified();
    }

    /* If we have a valid ovf proceed to the appliance settings page: */
    return pImportApplianceWidget->isValid();
}

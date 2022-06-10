/* $Id: UIWizardImportAppPageBasic2.cpp 78128 2019-04-15 18:28:48Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageBasic2 class implementation.
 */

/*
 * Copyright (C) 2009-2019 Oracle Corporation
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
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIRichTextLabel.h"
#include "UIApplianceUnverifiedCertificateViewer.h"
#include "UIWizardImportApp.h"
#include "UIWizardImportAppPageBasic2.h"

/* COM includes: */
#include "CAppliance.h"
#include "CCertificate.h"


/*********************************************************************************************************************************
*   Class UIWizardImportAppPage2 implementation.                                                                                 *
*********************************************************************************************************************************/

UIWizardImportAppPage2::UIWizardImportAppPage2()
{
}


/*********************************************************************************************************************************
*   Class UIWizardImportAppPageBasic2 implementation.                                                                            *
*********************************************************************************************************************************/

UIWizardImportAppPageBasic2::UIWizardImportAppPageBasic2(const QString &strFileName)
    : m_enmCertText(kCertText_Uninitialized)
{
    /* Create main layout: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    if (pMainLayout)
    {
        /* Create label: */
        m_pLabel = new QIRichTextLabel(this);
        if (m_pLabel)
        {
            /* Add into layout: */
            pMainLayout->addWidget(m_pLabel);
        }

        /* Create appliance widget: */
        m_pApplianceWidget = new UIApplianceImportEditorWidget(this);
        {
            m_pApplianceWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
            m_pApplianceWidget->setFile(strFileName);

            /* Add into layout: */
            pMainLayout->addWidget(m_pApplianceWidget);
        }

        /* Create form editor widget: */
        m_pFormEditor = new UIFormEditorWidget(this);
        if (m_pFormEditor)
        {
            m_pFormEditor->hide();

            /* Add into layout: */
            pMainLayout->addWidget(m_pFormEditor);
        }

        /* Create certificate label: */
        m_pCertLabel = new QLabel("<cert label>", this);
        if (m_pCertLabel)
        {
            /* Add into layout: */
            pMainLayout->addWidget(m_pCertLabel);
        }
    }

    /* Register classes: */
    qRegisterMetaType<ImportAppliancePointer>();
    /* Register fields: */
    registerField("applianceWidget", this, "applianceWidget");
}

void UIWizardImportAppPageBasic2::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardImportApp::tr("Appliance settings"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardImportApp::tr("These are the virtual machines contained in the appliance "
                                            "and the suggested settings of the imported VirtualBox machines. "
                                            "You can change many of the properties shown by double-clicking "
                                            "on the items and disable others using the check boxes below."));
    switch (m_enmCertText)
    {
        case kCertText_Unsigned:
            m_pCertLabel->setText(UIWizardImportApp::tr("Appliance is not signed"));
            break;
        case kCertText_IssuedTrusted:
            m_pCertLabel->setText(UIWizardImportApp::tr("Appliance signed by %1 (trusted)").arg(m_strSignedBy));
            break;
        case kCertText_IssuedExpired:
            m_pCertLabel->setText(UIWizardImportApp::tr("Appliance signed by %1 (expired!)").arg(m_strSignedBy));
            break;
        case kCertText_IssuedUnverified:
            m_pCertLabel->setText(UIWizardImportApp::tr("Unverified signature by %1!").arg(m_strSignedBy));
            break;
        case kCertText_SelfSignedTrusted:
            m_pCertLabel->setText(UIWizardImportApp::tr("Self signed by %1 (trusted)").arg(m_strSignedBy));
            break;
        case kCertText_SelfSignedExpired:
            m_pCertLabel->setText(UIWizardImportApp::tr("Self signed by %1 (expired!)").arg(m_strSignedBy));
            break;
        case kCertText_SelfSignedUnverified:
            m_pCertLabel->setText(UIWizardImportApp::tr("Unverified self signed signature by %1!").arg(m_strSignedBy));
            break;
        default:
            AssertFailed();
            RT_FALL_THRU();
        case kCertText_Uninitialized:
            m_pCertLabel->setText("<uninitialized page>");
            break;
    }
}

void UIWizardImportAppPageBasic2::initializePage()
{
    /* Acquire appliance: */
    CAppliance *pAppliance = m_pApplianceWidget->appliance();

    /* Check if pAppliance is alive. If not just return here.
     * This prevents crashes when an invalid ova file is supllied: */
    if (!pAppliance)
    {
        if (wizard())
            wizard()->reject();
        return;
    }

    /* Acquire certificate: */
    CCertificate comCertificate = pAppliance->GetCertificate();
    if (comCertificate.isNull())
        m_enmCertText = kCertText_Unsigned;
    else
    {
        /* Pick a 'signed-by' name: */
        m_strSignedBy = comCertificate.GetFriendlyName();

        /* If trusted, just select the right message: */
        if (comCertificate.GetTrusted())
        {
            if (comCertificate.GetSelfSigned())
                m_enmCertText = !comCertificate.GetExpired() ? kCertText_SelfSignedTrusted : kCertText_SelfSignedExpired;
            else
                m_enmCertText = !comCertificate.GetExpired() ? kCertText_IssuedTrusted     : kCertText_IssuedExpired;
        }
        else
        {
            /* Not trusted!  Must ask the user whether to continue in this case: */
            m_enmCertText = comCertificate.GetSelfSigned() ? kCertText_SelfSignedUnverified : kCertText_IssuedUnverified;

            /* Translate page early: */
            retranslateUi();

            /* Instantiate the dialog: */
            QPointer<UIApplianceUnverifiedCertificateViewer> pDialog = new UIApplianceUnverifiedCertificateViewer(this, comCertificate);
            AssertPtrReturnVoid(pDialog.data());

            /* Show viewer in modal mode: */
            const int iResultCode = pDialog->exec();

            /* Leave if viewer destroyed prematurely: */
            if (!pDialog)
                return;
            /* Delete viewer finally: */
            delete pDialog;

            /* Dismiss the entire import-appliance wizard if user rejects certificate: */
            if (iResultCode == QDialog::Rejected)
                wizard()->reject();
        }
    }

    /* Translate page: */
    retranslateUi();
}

void UIWizardImportAppPageBasic2::cleanupPage()
{
    /* Rollback settings: */
    m_pApplianceWidget->restoreDefaults();
    /* Call to base-class: */
    UIWizardPage::cleanupPage();
}

bool UIWizardImportAppPageBasic2::validatePage()
{
    /* Initial result: */
    bool fResult = true;

    /* Lock finish button: */
    startProcessing();

    /* Try to import appliance: */
    if (fResult)
        fResult = qobject_cast<UIWizardImportApp*>(wizard())->importAppliance();

    /* Unlock finish button: */
    endProcessing();

    /* Return result: */
    return fResult;
}

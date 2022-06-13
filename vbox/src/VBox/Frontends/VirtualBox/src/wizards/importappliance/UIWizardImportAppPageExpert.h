/* $Id: UIWizardImportAppPageExpert.h 91664 2021-10-11 15:43:02Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageExpert class declaration.
 */

/*
 * Copyright (C) 2009-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppPageExpert_h
#define FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppPageExpert_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizardPage.h"

/* Forward declarations: */
class QCheckBox;
class QGridLayout;
class QLabel;
class QListWidget;
class QStackedWidget;
class QIComboBox;
class QIToolButton;
class UIApplianceImportEditorWidget;
class UIEmptyFilePathSelector;
class UIFilePathSelector;
class UIFormEditorWidget;
class UIToolBox;
class UIWizardImportApp;

/** UINativeWizardPage extension for expert page of the Import Appliance wizard,
  * based on UIWizardImportAppPage1 & UIWizardImportAppPage2 namespace functions. */
class UIWizardImportAppPageExpert : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs expert page.
      * @param  fImportFromOCIByDefault  Brings whether we should propose import from OCI by default.
      * @param  strFileName              Brings appliance file name. */
    UIWizardImportAppPageExpert(bool fImportFromOCIByDefault, const QString &strFileName);

protected:

    /** Returns wizard this page belongs to. */
    UIWizardImportApp *wizard() const;

    /** Handles translation event. */
    virtual void retranslateUi() /* override final */;

    /** Performs page initialization. */
    virtual void initializePage() /* override final */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override final */;

    /** Performs page validation. */
    virtual bool validatePage() /* override final */;

private slots:

    /** Inits page async way. */
    void sltAsyncInit();

    /** Handles source combo change. */
    void sltHandleSourceComboChange();

    /** Handles imported file selector change. */
    void sltHandleImportedFileSelectorChange();
    /** Handles profile combo change. */
    void sltHandleProfileComboChange();
    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();
    /** Handles instance list change. */
    void sltHandleInstanceListChange();

    /** Handles import path editor change. */
    void sltHandleImportPathEditorChange();
    /** Handles MAC address import policy combo change. */
    void sltHandleMACImportPolicyComboChange();
    /** Handles import HDs as VDI check-box change. */
    void sltHandleImportHDsAsVDICheckBoxChange();

private:

    /** Holds whether default source should be Import from OCI. */
    bool     m_fImportFromOCIByDefault;
    /** Handles the appliance file name. */
    QString  m_strFileName;

    /** Holds the cached source. */
    QString  m_strSource;
    /** Holds the cached profile name. */
    QString  m_strProfileName;

    /** Holds the tool-box instance. */
    UIToolBox *m_pToolBox;

    /** Holds the source layout instance. */
    QGridLayout *m_pSourceLayout;
    /** Holds the source type label instance. */
    QLabel      *m_pSourceLabel;
    /** Holds the source type combo-box instance. */
    QIComboBox  *m_pSourceComboBox;

    /** Holds the settings widget 1 instance. */
    QStackedWidget *m_pSettingsWidget1;

    /** Holds the local container layout instance. */
    QGridLayout             *m_pLocalContainerLayout;
    /** Holds the file selector instance. */
    UIEmptyFilePathSelector *m_pFileSelector;

    /** Holds the cloud container layout instance. */
    QGridLayout  *m_pCloudContainerLayout;
    /** Holds the profile combo-box instance. */
    QIComboBox   *m_pProfileComboBox;
    /** Holds the profile management tool-button instance. */
    QIToolButton *m_pProfileToolButton;
    /** Holds the profile instance list instance. */
    QListWidget  *m_pProfileInstanceList;

    /** Holds the settings widget 2 instance. */
    QStackedWidget *m_pSettingsWidget2;

    /** Holds the appliance widget instance. */
    UIApplianceImportEditorWidget *m_pApplianceWidget;
    /** Holds the import file-path label instance. */
    QLabel                        *m_pLabelImportFilePath;
    /** Holds the import file-path editor instance. */
    UIFilePathSelector            *m_pEditorImportFilePath;
    /** Holds the MAC address label instance. */
    QLabel                        *m_pLabelMACImportPolicy;
    /** Holds the MAC address combo instance. */
    QIComboBox                    *m_pComboMACImportPolicy;
    /** Holds the additional options label instance. */
    QLabel                        *m_pLabelAdditionalOptions;
    /** Holds the 'import HDs as VDI' checkbox instance. */
    QCheckBox                     *m_pCheckboxImportHDsAsVDI;
    /** Holds the signature/certificate info label instance. */
    QLabel                        *m_pCertLabel;

    /** Holds the Form Editor widget instance. */
    UIFormEditorWidget *m_pFormEditor;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppPageExpert_h */

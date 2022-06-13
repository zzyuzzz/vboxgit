/* $Id: UIWizardImportAppPageExpert.h 91567 2021-10-05 11:46:01Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppPageExpert class declaration.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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
#include "UIWizardImportAppPageBasic1.h"
#include "UIWizardImportAppPageBasic2.h"

/* Forward declarations: */
class QGroupBox;

/** UIWizardPage extension for UIWizardImportAppPage1 and UIWizardImportAppPage2. */
class UIWizardImportAppPageExpert : public UIWizardPage,
                                    public UIWizardImportAppPage1,
                                    public UIWizardImportAppPage2
{
    Q_OBJECT;
    Q_PROPERTY(bool isSourceCloudOne READ isSourceCloudOne);
    Q_PROPERTY(CCloudProfile profile READ profile);
    Q_PROPERTY(CAppliance cloudAppliance READ cloudAppliance);
    Q_PROPERTY(CVirtualSystemDescriptionForm vsdForm READ vsdForm);
    Q_PROPERTY(QString machineId READ machineId);
    Q_PROPERTY(ImportAppliancePointer applianceWidget READ applianceWidget);
    Q_PROPERTY(MACAddressImportPolicy macAddressImportPolicy READ macAddressImportPolicy);
    Q_PROPERTY(bool importHDsAsVDI READ importHDsAsVDI);

public:

    /** Constructs expert page.
      * @param  strFileName  Brings appliance file name. */
    UIWizardImportAppPageExpert(bool fImportFromOCIByDefault, const QString &strFileName);

protected:

    /** Allows access wizard from base part. */
    UIWizard *wizardImp() const { return UIWizardPage::wizard(); }
    /** Allows to access 'field()' from base part. */
    virtual QVariant fieldImp(const QString &strFieldName) const /* override */ { return UIWizardPage::field(strFieldName); }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs page initialization. */
    virtual void initializePage() /* override */;

    /** Returns whether page is complete. */
    virtual bool isComplete() const /* override */;

    /** Performs page validation. */
    virtual bool validatePage() /* override */;

    /** Updates page appearance. */
    virtual void updatePageAppearance() /* override */;

private slots:

    /** Handles import source change. */
    void sltHandleSourceChange();

    /** Handles file-path change. */
    void sltFilePathChangeHandler();

    /** Handles change in profile combo-box. */
    void sltHandleProfileComboChange();
    /** Handles profile tool-button click. */
    void sltHandleProfileButtonClick();

    /** Handles change in instance list. */
    void sltHandleInstanceListChange();

    /** Handles file path being changed to @a strNewPath. */
    void sltHandlePathChanged(const QString &strNewPath);

    /** Handles MAC address import policy changes. */
    void sltHandleMACImportPolicyChange();

private:

    /** Holds the source container instance. */
    QGroupBox *m_pCntSource;
    /** Holds the settings container instance. */
    QGroupBox *m_pSettingsCnt;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppPageExpert_h */

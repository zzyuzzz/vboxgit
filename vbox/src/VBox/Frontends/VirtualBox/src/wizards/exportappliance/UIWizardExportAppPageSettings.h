/* $Id: UIWizardExportAppPageSettings.h 93115 2022-01-01 11:31:46Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppPageSettings class declaration.
 */

/*
 * Copyright (C) 2009-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageSettings_h
#define FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageSettings_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QList>

/* GUI includes: */
#include "UINativeWizardPage.h"

/* COM includes: */
#include "COMEnums.h"
#include "CAppliance.h"

/* Forward declarations: */
class QStackedWidget;
class QIRichTextLabel;
class UIApplianceExportEditorWidget;
class UIFormEditorWidget;
class UIWizardExportApp;

/** Namespace for Settings page of the Export Appliance wizard. */
namespace UIWizardExportAppSettings
{
    /** Refresh stacked widget. */
    void refreshStackedWidget(QStackedWidget *pStackedWidget,
                              bool fIsFormatCloudOne);

    /** Refreshes appliance settings widget. */
    void refreshApplianceSettingsWidget(UIApplianceExportEditorWidget *pApplianceWidget,
                                        const CAppliance &comAppliance,
                                        bool fIsFormatCloudOne);
    /** Refreshes form properties table. */
    void refreshFormPropertiesTable(UIFormEditorWidget *pFormEditor,
                                    const CVirtualSystemDescriptionForm &comVsdForm,
                                    bool fIsFormatCloudOne);
}

/** UINativeWizardPage extension for Settings page of the Export Appliance wizard,
  * based on UIWizardExportAppSettings namespace functions. */
class UIWizardExportAppPageSettings : public UINativeWizardPage
{
    Q_OBJECT;

public:

    /** Constructs Settings page. */
    UIWizardExportAppPageSettings();

protected:

    /** Returns wizard this page belongs to. */
    UIWizardExportApp *wizard() const;

    /** Handles translation event. */
    virtual void retranslateUi() /* override final */;

    /** Performs page initialization. */
    virtual void initializePage() /* override final */;

    /** Performs page validation. */
    virtual bool validatePage() /* override final */;

private:

    /** Holds the label instance. */
    QIRichTextLabel *m_pLabel;

    /** Holds the settings widget 2 instance. */
    QStackedWidget *m_pSettingsWidget2;

    /** Holds the appliance widget reference. */
    UIApplianceExportEditorWidget *m_pApplianceWidget;
    /** Holds the Form Editor widget instance. */
    UIFormEditorWidget            *m_pFormEditor;

    /** Holds whether cloud exporting is at launching stage. */
    bool  m_fLaunching;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppPageSettings_h */

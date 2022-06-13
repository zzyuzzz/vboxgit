/* $Id: UIApplianceImportEditorWidget.h 91547 2021-10-04 15:49:52Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIApplianceImportEditorWidget class declaration.
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

#ifndef FEQT_INCLUDED_SRC_widgets_UIApplianceImportEditorWidget_h
#define FEQT_INCLUDED_SRC_widgets_UIApplianceImportEditorWidget_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIApplianceEditorWidget.h"

/* Forward declarations: */
class UIFilePathSelector;
class QComboBox;

/** MAC address policies. */
enum MACAddressImportPolicy
{
    MACAddressImportPolicy_KeepAllMACs,
    MACAddressImportPolicy_KeepNATMACs,
    MACAddressImportPolicy_StripAllMACs,
    MACAddressImportPolicy_MAX
};
Q_DECLARE_METATYPE(MACAddressImportPolicy);

/** UIApplianceEditorWidget subclass for Import Appliance wizard. */
class UIApplianceImportEditorWidget: public UIApplianceEditorWidget
{
    Q_OBJECT;

public:

    /** Constructs widget passing @a pParent to the base-class. */
    UIApplianceImportEditorWidget(QWidget *pParent);

    /** Defines @a strFaile name. */
    bool setFile(const QString &strFile);

    /** Prepares import by pushing edited data back to appliance. */
    void prepareImport();

    /** Performs import. */
    bool import();

    /** Returns a list of license agreement pairs. */
    QList<QPair<QString, QString> > licenseAgreements() const;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** Handles file path being changed to @a strNewPath. */
    void sltHandlePathChanged(const QString &strNewPath);

    /** Handles MAC address import policy changes. */
    void sltHandleMACAddressImportPolicyChange();

private:

    /** Prepares all. */
    void prepare();

    /** Populates MAC address import policies. */
    void populateMACAddressImportPolicies();

    /** Defines MAC address import @a enmPolicy. */
    void setMACAddressImportPolicy(MACAddressImportPolicy enmPolicy);

    /** Holds the exporting file-path label instance. */
    QLabel             *m_pLabelExportingFilePath;
    /** Holds the exporting file-path editor instance. */
    UIFilePathSelector *m_pEditorExportingFilePath;
    /** Holds the MAC address label instance. */
    QLabel             *m_pLabelMACExportPolicy;
    /** Holds the MAC address combo instance. */
    QComboBox          *m_pComboMACExportPolicy;
    /** Holds the additional options label instance. */
    QLabel             *m_pLabelAdditionalOptions;
    /** Holds the 'import HDs as VDI' checkbox instance. */
    QCheckBox          *m_pCheckboxImportHDsAsVDI;
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIApplianceImportEditorWidget_h */

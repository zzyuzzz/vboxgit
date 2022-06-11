/* $Id: UIWizardCloneVMPageBasic1.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVMPageBasic1 class declaration.
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVMPageBasic1_h
#define FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVMPageBasic1_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Local includes: */
#include "UIWizardPage.h"

/* Forward declarations: */
class QILineEdit;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QIRichTextLabel;
class UIFilePathSelector;

/** MAC address policies. */
enum MACAddressClonePolicy
{
    MACAddressClonePolicy_KeepAllMACs,
    MACAddressClonePolicy_KeepNATMACs,
    MACAddressClonePolicy_StripAllMACs,
    MACAddressClonePolicy_MAX
};
Q_DECLARE_METATYPE(MACAddressClonePolicy);

/* 1st page of the Clone Virtual Machine wizard (base part): */
class UIWizardCloneVMPage1 : public UIWizardPageBase
{
protected:

    UIWizardCloneVMPage1(const QString &strOriginalName, const QString &strDefaultPath, const QString &strGroup);

    QString cloneName() const;
    void    setCloneName(const QString &strName);

    QString clonePath() const;
    void     setClonePath(const QString &strName);

    QString cloneFilePath() const;
    void setCloneFilePath(const QString &path);

    /** calls CVirtualBox::ComposeMachineFilename(...) and sets related member variables */
    void composeCloneFilePath();
    /** Populates MAC address policies. */
    void populateMACAddressClonePolicies();

    /** Updates MAC address policy combo tool-tips. */
    void updateMACAddressClonePolicyComboToolTip();
    /** Returns MAC address clone policy. */
    MACAddressClonePolicy macAddressClonePolicy() const;
    /** Defines @a enmMACAddressClonePolicy. */
    void setMACAddressClonePolicy(MACAddressClonePolicy enmMACAddressClonePolicy);

    bool keepDiskNames() const;
    void setKeepDiskNames(bool fKeepDiskNames);

    bool keepHWUUIDs() const;
    void setKeepHWUUIDs(bool bKeepHWUUIDs);

    QString      m_strOriginalName;
    QString      m_strDefaultPath;
    QString      m_strGroup;
    /** Full, non-native path of the clone machines setting file. Generated by CVirtualBox::ComposeMachineFilename(...) */
    QString      m_strCloneFilePath;
    /** The full path of the folder where clone machine's settings file is located.
     * Generated from the m_strCloneFilePath by removing base file name */
    QString      m_strCloneFolder;
    QILineEdit  *m_pNameLineEdit;
    UIFilePathSelector *m_pPathSelector;
    QLabel      *m_pNameLabel;
    QLabel      *m_pPathLabel;
    QLabel      *m_pMACComboBoxLabel;
    QComboBox   *m_pMACComboBox;
    QCheckBox   *m_pKeepDiskNamesCheckBox;
    QCheckBox   *m_pKeepHWUUIDsCheckBox;
};

/* 1st page of the Clone Virtual Machine wizard (basic extension): */
class UIWizardCloneVMPageBasic1 : public UIWizardPage, public UIWizardCloneVMPage1
{
    Q_OBJECT;
    Q_PROPERTY(QString cloneName READ cloneName WRITE setCloneName);
    Q_PROPERTY(QString cloneFilePath READ cloneFilePath WRITE setCloneFilePath);
    Q_PROPERTY(MACAddressClonePolicy macAddressClonePolicy READ macAddressClonePolicy WRITE setMACAddressClonePolicy);
    Q_PROPERTY(bool keepDiskNames READ keepDiskNames WRITE setKeepDiskNames);
    Q_PROPERTY(bool keepHWUUIDs READ keepHWUUIDs WRITE setKeepHWUUIDs);


public:

    UIWizardCloneVMPageBasic1(const QString &strOriginalName, const QString &strDefaultPath, const QString &strGroup);

private slots:

    void sltNameChanged();
    void sltPathChanged();
    /** Handles change in MAC address policy combo-box. */
    void sltHandleMACAddressClonePolicyComboChange();

private:

    void retranslateUi();
    void initializePage();

    /* Validation stuff: */
    bool isComplete() const;

    QIRichTextLabel *m_pMainLabel;
    QGridLayout     *m_pContainerLayout;
    QLabel          *m_pAdditionalOptionsLabel;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevm_UIWizardCloneVMPageBasic1_h */

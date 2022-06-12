/* $Id: UIWizardNewVMPageBasic5.h 87874 2021-02-25 13:05:21Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardNewVMPageBasic5 class declaration.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageBasic5_h
#define FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageBasic5_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QVariant>

/* GUI includes: */
#include "UIWizardPage.h"
#include "UIWizardNewVDPageBasic1.h"
#include "UIWizardNewVDPageBasic2.h"
#include "UIWizardNewVDPageBasic3.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QRadioButton;
class QCheckBox;
class QIRichTextLabel;
class UIBaseMemoryEditor;
class UIVirtualCPUEditor;

class UIWizardNewVMPageBasic5 : public UIWizardPage,
                                public UIWizardNewVDPage1,
                                public UIWizardNewVDPage2,
                                public UIWizardNewVDPage3

{
    Q_OBJECT;
    Q_PROPERTY(CMediumFormat mediumFormat READ mediumFormat WRITE setMediumFormat);
    Q_PROPERTY(qulonglong mediumVariant READ mediumVariant WRITE setMediumVariant);
    Q_PROPERTY(QString mediumPath READ mediumPath);
    Q_PROPERTY(qulonglong mediumSize READ mediumSize WRITE setMediumSize);

public:

    /** Constructor. */
    UIWizardNewVMPageBasic5();

protected:

    /** Wrapper to access 'wizard' from base part. */
    UIWizard *wizardImp() const { return wizard(); }
    /** Wrapper to access 'this' from base part. */
    UIWizardPage* thisImp() { return this; }
    /** Wrapper to access 'wizard-field' from base part. */
    QVariant fieldImp(const QString &strFieldName) const { return UIWizardPage::field(strFieldName); }

private slots:


private:


    /** Prepare stuff. */
    void prepare();
    void createConnections();
    void retranslateUi();
    void initializePage();
    void cleanupPage();

    bool isComplete() const;


    /** Widgets. */
    QIRichTextLabel *m_pLabel;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newvm_UIWizardNewVMPageBasic5_h */

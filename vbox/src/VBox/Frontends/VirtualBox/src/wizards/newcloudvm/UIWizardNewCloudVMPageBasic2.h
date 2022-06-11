/* $Id: UIWizardNewCloudVMPageBasic2.h 79256 2019-06-20 13:40:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardNewCloudVMPageBasic2 class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageBasic2_h
#define FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageBasic2_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UIFormEditorWidget.h"
#include "UIWizardPage.h"

/* Forward declarations: */
class QIRichTextLabel;

/** UIWizardPageBase extension for 2nd page of the New Cloud VM wizard. */
class UIWizardNewCloudVMPage2 : public UIWizardPageBase
{
protected:

    /** Constructs 2nd page base. */
    UIWizardNewCloudVMPage2();

    /** Refreshes form properties table. */
    void refreshFormPropertiesTable();

    /** Holds the Form Editor widget instance. */
    UIFormEditorWidgetPointer  m_pFormEditor;
};

/** UIWizardPage extension for 2nd page of the New Cloud VM wizard, extends UIWizardNewCloudVMPage2 as well. */
class UIWizardNewCloudVMPageBasic2 : public UIWizardPage, public UIWizardNewCloudVMPage2
{
    Q_OBJECT;

public:

    /** Constructs 2nd basic page. */
    UIWizardNewCloudVMPageBasic2();

protected:

    /** Allows to access 'field()' from base part. */
    virtual QVariant fieldImp(const QString &strFieldName) const /* override */ { return UIWizardPage::field(strFieldName); }

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs page initialization. */
    virtual void initializePage() /* override */;

    /** Performs page validation. */
    virtual bool validatePage() /* override */;

private:

    /** Holds the label instance. */
    QIRichTextLabel *m_pLabel;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_newcloudvm_UIWizardNewCloudVMPageBasic2_h */

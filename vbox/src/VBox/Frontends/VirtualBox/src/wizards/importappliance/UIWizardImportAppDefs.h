/* $Id: UIWizardImportAppDefs.h 78046 2019-04-08 15:41:31Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardImportAppDefs class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppDefs_h
#define FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMetaType>
#include <QPointer>

/* GUI includes: */
#include "UIApplianceImportEditorWidget.h"

/** Safe pointer to import appliance editor widget. */
typedef QPointer<UIApplianceImportEditorWidget> ImportAppliancePointer;
Q_DECLARE_METATYPE(ImportAppliancePointer);

/** Import source types. */
enum ImportSourceType
{
    ImportSourceType_Invalid,
    ImportSourceType_Local,
    ImportSourceType_Cloud
};
Q_DECLARE_METATYPE(ImportSourceType);

#endif /* !FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportAppDefs_h */

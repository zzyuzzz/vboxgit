/* $Id: UIWizardExportAppDefs.h 91260 2021-09-15 18:55:11Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardExportAppDefs class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppDefs_h
#define FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMetaType>
#include <QPointer>

/* GUI includes: */
#include "UIApplianceExportEditorWidget.h"

/* Typedefs: */
typedef QPointer<UIApplianceExportEditorWidget> ExportAppliancePointer;
Q_DECLARE_METATYPE(ExportAppliancePointer);

#endif /* !FEQT_INCLUDED_SRC_wizards_exportappliance_UIWizardExportAppDefs_h */

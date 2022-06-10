/* $Id: UIFDCreationDialog.h 72815 2018-07-03 09:55:30Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIFDCreationDialog class declaration.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIFDCreationDialog_h__
#define __UIFDCreationDialog_h__

/* Qt includes: */
#include <QDialog>


/* GUI Includes */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class UIFilePathSelector;

/* A QDialog extension to get necessary setting from the user for floppy disk creation */
class SHARED_LIBRARY_STUFF UIFDCreationDialog : public QIWithRetranslateUI<QDialog>

{
    Q_OBJECT;


public:

    UIFDCreationDialog(QWidget *pParent = 0,
                       const QString &strMachineName = QString(),
                       const QString &strMachineSettingsFilePath = QString()
                       );

protected:

    void retranslateUi();

private slots:


private:

    void prepare();

    UIFilePathSelector *m_pFilePathselector;
    QString             m_strMachineName;
    QString             m_strMachineSettingsFilePath;
};

#endif // __UIFDCreationDialog_h__

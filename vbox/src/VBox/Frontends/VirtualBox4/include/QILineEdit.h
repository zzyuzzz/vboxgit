/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * QILineEdit class declarations
 */

/*
 * Copyright (C) 2008 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef __QILineEdit_h__
#define __QILineEdit_h__

/* Qt includes */
#include <QLineEdit>

class QILineEdit: public QLineEdit
{
public:
    QILineEdit (QWidget *aParent = 0)
        :QLineEdit (aParent) {}
    QILineEdit (const QString &aContents, QWidget *aParent = 0)
        :QLineEdit (aContents, aParent) {}

    void setFixedWidthByText (const QString &aText);
}; 

#endif /* __QILineEdit_h__ */


/* $Id: UIInformationView.h 59647 2016-02-12 11:13:32Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIInformationView class declaration.
 */

/*
 * Copyright (C) 2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIInformationView_h___
#define ___UIInformationView_h___

/* Qt includes: */
#include <QListView>
#include <QModelIndex>

/** QListView extension
  * providing GUI with information-view in session-information window. */
class UIInformationView : public QListView
{
    Q_OBJECT;

public:
    /** Constructs information-view passing @a pParent to base-class. */
    UIInformationView(QWidget *pParent = 0);

public slots:
    void updateData(const QModelIndex & topLeft, const QModelIndex & bottomRight);

};

#endif /* !___UIInformationView_h___ */


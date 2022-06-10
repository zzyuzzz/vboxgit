/* $Id: UIGuestControlWidget.h 75059 2018-10-25 10:07:18Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGuestControlWidget class declaration.
 */

/*
 * Copyright (C) 2016-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIGuestControlWidget_h___
#define ___UIGuestControlWidget_h___

/* Qt includes: */
#include <QWidget>

/* COM includes: */
#include "COMEnums.h"
#include "CGuest.h"
#include "CEventListener.h"

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QITabWidget;
class UIActionPool;
class UIGuestControlFileManager;
class UIGuestProcessControlWidget;

class UIGuestControlWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    UIGuestControlWidget(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                         const CGuest &comGuest,
                         bool fShowToolbar = true, QWidget *pParent = 0);

protected:

    virtual void retranslateUi() /* override */;

private:

    enum TabIndex
    {
        TabIndex_FileManager,
        TabIndex_ProcessControl,
        TabIndex_Max
    };

    void prepare();
    /** m_enmEmbedding determines if the widget is embedded into a standalone dialog
      * or into the manager UI.*/
    const EmbedTo                m_enmEmbedding;
    UIActionPool                *m_pActionPool;
    CGuest                       m_comGuest;
    QITabWidget                 *m_pTabWidget;
    UIGuestProcessControlWidget *m_pProcessControlWidget;
    UIGuestControlFileManager   *m_pFileManager;
    bool                         m_fShowToolbar;
};

#endif /* !___UIGuestControlWidget_h___ */

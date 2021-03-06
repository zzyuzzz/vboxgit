/* $Id: UIGuestProcessControlDialog.h 93990 2022-02-28 15:34:57Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGuestProcessControlDialog class declaration.
 */

/*
 * Copyright (C) 2010-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_guestctrl_UIGuestProcessControlDialog_h
#define FEQT_INCLUDED_SRC_guestctrl_UIGuestProcessControlDialog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QString>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"
#include "CGuest.h"

/* Forward declarations: */
class UIActionPool;
class UIGuestProcessControlDialog;
class CGuest;

/** QIManagerDialogFactory extension used as a factory for the Guest Control dialog. */
class UIGuestProcessControlDialogFactory : public QIManagerDialogFactory
{
public:

    UIGuestProcessControlDialogFactory(UIActionPool *pActionPool = 0, const CGuest &comGuest = CGuest(), const QString &strMachineName = QString());

protected:

    /** Creates derived @a pDialog instance.
      * @param  pCenterWidget  Passes the widget to center wrt. pCenterWidget. */
    virtual void create(QIManagerDialog *&pDialog, QWidget *pCenterWidget) RT_OVERRIDE;

    UIActionPool *m_pActionPool;
    CGuest        m_comGuest;
    QString       m_strMachineName;
};


/** QIManagerDialog extension providing GUI with the dialog displaying guest control releated logs. */
class UIGuestProcessControlDialog : public QIWithRetranslateUI<QIManagerDialog>
{
    Q_OBJECT;

public:

    /** Constructs Guest Control dialog.
      * @param  pCenterWidget  Passes the widget reference to center according to.
      * @param  pActionPool    Passes the action-pool reference.
      * @param  comGuest       Passes the com-guest reference. */
    UIGuestProcessControlDialog(QWidget *pCenterWidget, UIActionPool *pActionPool, const CGuest &comGuest, const QString &strMachineName = QString());

protected:

    /** @name Event-handling stuff.
      * @{ */
        /** Handles translation event. */
        virtual void retranslateUi() RT_OVERRIDE;
    /** @} */

    /** @name Prepare/cleanup cascade.
     * @{ */
        /** Configures all. */
        virtual void configure() RT_OVERRIDE;
        /** Configures central-widget. */
        virtual void configureCentralWidget() RT_OVERRIDE;
        /** Perform final preparations. */
        virtual void finalize() RT_OVERRIDE;
        /** Loads dialog setting from extradata. */
        virtual void loadSettings() RT_OVERRIDE;

        /** Saves dialog setting into extradata. */
        virtual void saveSettings() RT_OVERRIDE;
    /** @} */

    /** @name Functions related to geometry restoration.
     * @{ */
        /** Returns whether the window should be maximized when geometry being restored. */
        virtual bool shouldBeMaximized() const RT_OVERRIDE;
    /** @} */

private slots:

    void sltSetCloseButtonShortCut(QKeySequence shortcut);

private:

    UIActionPool *m_pActionPool;
    CGuest      m_comGuest;
    QString     m_strMachineName;
};


#endif /* !FEQT_INCLUDED_SRC_guestctrl_UIGuestProcessControlDialog_h */

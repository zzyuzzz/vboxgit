/* $Id: UIUpdateManager.h 71448 2018-03-22 10:58:10Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIUpdateManager class declaration.
 */

/*
 * Copyright (C) 2006-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIUpdateManager_h___
#define ___UIUpdateManager_h___

/* Qt includes: */
#include <QObject>

/* Forward declarations: */
class UIUpdateQueue;

/** Singleton to perform new version checks
  * and update of various VirtualBox parts. */
class UIUpdateManager : public QObject
{
    Q_OBJECT;

    /** Constructs Update Manager. */
    UIUpdateManager();
    /** Destructs Update Manager. */
    ~UIUpdateManager();

public:

    /** Schedules manager. */
    static void schedule();
    /** Shutdowns manager. */
    static void shutdown();
    /** Returns manager instance. */
    static UIUpdateManager *instance() { return s_pInstance; }

    /** Initiates the extension pack installation process.
      * @param  strFilePath      Brings the extension pack file path.
      * @param  strDigest        Brings the extension pack file digest.
      * @param  pParent          Brings the parent dialog reference.
      * @param  pstrExtPackName  Brings the extension pack name. */
    static void doExtPackInstallation(QString const &strFilePath,
                                      QString const &strDigest,
                                      QWidget *pParent,
                                      QString *pstrExtPackName);

public slots:

    /** Performs forced new version check. */
    void sltForceCheck();

private slots:

    /** Checks whether update is necessary. */
    void sltCheckIfUpdateIsNecessary(bool fForceCall = false);

    /** Handles update finishing. */
    void sltHandleUpdateFinishing();

private:

    /** Holds the singleton instance. */
    static UIUpdateManager *s_pInstance;

    /** Holds the update queue instance. */
    UIUpdateQueue *m_pQueue;
    /** Holds whether Update Manager is running. */
    bool           m_fIsRunning;
    /** Holds the refresh period. */
    quint64        m_uTime;
};

/** Singleton Update Manager 'official' name. */
#define gUpdateManager UIUpdateManager::instance()

#endif /* !___UIUpdateManager_h___ */


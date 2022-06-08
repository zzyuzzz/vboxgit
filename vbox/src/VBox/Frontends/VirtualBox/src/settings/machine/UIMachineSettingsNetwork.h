/* $Id: UIMachineSettingsNetwork.h 66183 2017-03-21 16:26:35Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsNetwork class declaration.
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

#ifndef ___UIMachineSettingsNetwork_h___
#define ___UIMachineSettingsNetwork_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsNetwork.gen.h"
#include "UIMachineSettingsPortForwardingDlg.h"

/* Forward declarations: */
class QITabWidget;
class UIMachineSettingsNetworkPage;
struct UIDataSettingsMachineNetwork;
struct UIDataSettingsMachineNetworkAdapter;
typedef UISettingsCache<UIDataSettingsMachineNetworkAdapter> UISettingsCacheMachineNetworkAdapter;
typedef UISettingsCachePool<UIDataSettingsMachineNetwork, UISettingsCacheMachineNetworkAdapter> UISettingsCacheMachineNetwork;


/** Machine settings: Network page. */
class UIMachineSettingsNetworkPage : public UISettingsPageMachine
{
    Q_OBJECT;

public:

    /** Constructs Network settings page. */
    UIMachineSettingsNetworkPage();
    /** Destructs Network settings page. */
    ~UIMachineSettingsNetworkPage();

    /* Bridged adapter list: */
    const QStringList& bridgedAdapterList() const { return m_bridgedAdapterList; }
    /* Internal network list: */
    const QStringList& internalNetworkList() const { return m_internalNetworkList; }
    /* Host-only interface list: */
    const QStringList& hostInterfaceList() const { return m_hostInterfaceList; }
    /* Generic driver list: */
    const QStringList& genericDriverList() const { return m_genericDriverList; }
    /* NAT network list: */
    const QStringList& natNetworkList() const { return m_natNetworkList; }

protected:

    /** Returns whether the page content was changed. */
    bool changed() const /* override */;

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    void loadToCacheFrom(QVariant &data);
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    void getFromCache();

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    void putToCache();
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    void saveFromCacheTo(QVariant &data);

    /** Performs validation, updates @a messages list if something is wrong. */
    bool validate(QList<UIValidationMessage> &messages);

    /** Handles translation event. */
    void retranslateUi();

private slots:

    /* Handles tab updates: */
    void sltHandleUpdatedTab();

    /** Handles advanced button state change to @a fExpanded. */
    void sltHandleAdvancedButtonStateChange(bool fExpanded);

private:

    /* Private helpers: */
    void polishPage();
    void refreshBridgedAdapterList();
    void refreshInternalNetworkList(bool fFullRefresh = false);
    void refreshHostInterfaceList();
    void refreshGenericDriverList(bool fFullRefresh = false);
    void refreshNATNetworkList();

    /* Various static stuff: */
    static QStringList otherInternalNetworkList();
    static QStringList otherGenericDriverList();
    static QString summarizeGenericProperties(const CNetworkAdapter &adapter);
    static void updateGenericProperties(CNetworkAdapter &adapter, const QString &strPropText);

    /* Tab holder: */
    QITabWidget *m_pTwAdapters;

    /* Alternative-name lists: */
    QStringList m_bridgedAdapterList;
    QStringList m_internalNetworkList;
    QStringList m_hostInterfaceList;
    QStringList m_genericDriverList;
    QStringList m_natNetworkList;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineNetwork *m_pCache;
};

#endif /* !___UIMachineSettingsNetwork_h___ */


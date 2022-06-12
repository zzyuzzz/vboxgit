/* $Id: UIGlobalSettingsInput.h 86020 2020-09-03 13:10:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsInput class declaration.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsInput_h
#define FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsInput_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UISettingsPage.h"

/* Forward declartions: */
class QCheckBox;
class QLineEdit;
class QTabWidget;
class UIDataSettingsGlobalInput;
class UIHotKeyTable;
class UIHotKeyTableModel;
typedef UISettingsCache<UIDataSettingsGlobalInput> UISettingsCacheGlobalInput;

/** Global settings: Input page. */
class SHARED_LIBRARY_STUFF UIGlobalSettingsInput : public UISettingsPageGlobal
{
    Q_OBJECT;

    /** Hot-key table indexes. */
    enum { UIHotKeyTableIndex_Selector, UIHotKeyTableIndex_Machine };

public:

    /** Constructs Input settings page. */
    UIGlobalSettingsInput();
    /** Destructs Input settings page. */
    ~UIGlobalSettingsInput();

protected:

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void loadToCacheFrom(QVariant &data) /* override */;
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void getFromCache() /* override */;

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    virtual void putToCache() /* override */;
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    virtual void saveFromCacheTo(QVariant &data) /* overrride */;

    /** Performs validation, updates @a messages list if something is wrong. */
    virtual bool validate(QList<UIValidationMessage> &messages) /* override */;

    /** Defines TAB order for passed @a pWidget. */
    virtual void setOrderAfter(QWidget *pWidget) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private:

    /** Prepares all. */
    void prepare();
    /** Prepares widgets. */
    void prepareWidgets();
    /** Prepares Manager UI tab. */
    void prepareTabManager();
    /** Prepares Runtime UI tab. */
    void prepareTabMachine();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Saves existing input data from the cache. */
    bool saveInputData();

    /** Holds the Manager UI shortcuts model instance. */
    UIHotKeyTableModel *m_pModelManager;
    /** Holds the Runtime UI shortcuts model instance. */
    UIHotKeyTableModel *m_pModelRuntime;

    /** Holds the page data cache instance. */
    UISettingsCacheGlobalInput *m_pCache;

    /** @name Widgets
     * @{ */
        /** Holds the tab-widget instance. */
        QTabWidget    *m_pTabWidget;
        /** Holds the Manager UI shortcuts filter instance. */
        QLineEdit     *m_pEditorManagerFilter;
        /** Holds the Manager UI shortcuts table instance. */
        UIHotKeyTable *m_pTableManager;
        /** Holds the Runtime UI shortcuts filter instance. */
        QLineEdit     *m_pEditorRuntimeFilter;
        /** Holds the Runtime UI shortcuts table instance. */
        UIHotKeyTable *m_pTableRuntime;
        /** Holds the 'enable auto-grab' checkbox instance. */
        QCheckBox     *m_pCheckBoxEnableAutoGrab;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsInput_h */

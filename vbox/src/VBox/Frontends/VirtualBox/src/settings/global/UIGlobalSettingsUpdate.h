/* $Id: UIGlobalSettingsUpdate.h 86095 2020-09-11 14:28:34Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsUpdate class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsUpdate_h
#define FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsUpdate_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIUpdateDefs.h"

/* Forward declarations: */
class QCheckBox;
class QComboBox;
class QLabel;
class QRadioButton;
struct UIDataSettingsGlobalUpdate;
typedef UISettingsCache<UIDataSettingsGlobalUpdate> UISettingsCacheGlobalUpdate;

/** Global settings: Update page. */
class SHARED_LIBRARY_STUFF UIGlobalSettingsUpdate : public UISettingsPageGlobal
{
    Q_OBJECT;

public:

    /** Constructs Update settings page. */
    UIGlobalSettingsUpdate();
    /** Destructs Update settings page. */
    ~UIGlobalSettingsUpdate();

protected:

    /** Loads settings from external object(s) packed inside @a data to cache.
      * @note  This task WILL be performed in other than the GUI thread, no widget interactions! */
    virtual void loadToCacheFrom(QVariant &data) /* override */;
    /** Loads data from cache to corresponding widgets.
      * @note  This task WILL be performed in the GUI thread only, all widget interactions here! */
    virtual void getFromCache() /* override */;

    /** Saves data from corresponding widgets to cache.
      * @note  This task WILL be performed in the GUI thread only, all widget interactions here! */
    virtual void putToCache() /* override */;
    /** Saves settings from cache to external object(s) packed inside @a data.
      * @note  This task WILL be performed in other than the GUI thread, no widget interactions! */
    virtual void saveFromCacheTo(QVariant &data) /* overrride */;

    /** Defines TAB order for passed @a pWidget. */
    virtual void setOrderAfter(QWidget *pWidget) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

private slots:

    /** Handles whether update is @a fEnabled. */
    void sltHandleUpdateToggle(bool fEnabled);
    /** Handles update period change. */
    void sltHandleUpdatePeriodChange();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares widgets. */
    void prepareWidgets();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Returns period type. */
    VBoxUpdateData::PeriodType periodType() const;
    /** Returns branch type. */
    VBoxUpdateData::BranchType branchType() const;

    /** Saves existing update data from the cache. */
    bool saveUpdateData();

    /** Holds the page data cache instance. */
    UISettingsCacheGlobalUpdate *m_pCache;

    /** @name Widgets
     * @{ */
        /** Holds the update check-box instance. */
        QCheckBox    *m_pCheckBoxUpdate;
        /** Holds the update settings widget instance. */
        QWidget      *m_pWidgetUpdateSettings;
        /** Holds the update period label instance. */
        QLabel       *m_pLabelUpdatePeriod;
        /** Holds the update period combo instance. */
        QComboBox    *m_pComboUpdatePeriod;
        /** Holds the update date label instance. */
        QLabel       *m_pLabelUpdateDate;
        /** Holds the update date field instance. */
        QLabel       *m_pFieldUpdateDate;
        /** Holds the update filter label instance. */
        QLabel       *m_pLabelUpdateFilter;
        /** Holds the 'update to stable' radio-button instance. */
        QRadioButton *m_pRadioUpdateFilterStable;
        /** Holds the 'update to every' radio-button instance. */
        QRadioButton *m_pRadioUpdateFilterEvery;
        /** Holds the 'update to betas' radio-button instance. */
        QRadioButton *m_pRadioUpdateFilterBetas;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_settings_global_UIGlobalSettingsUpdate_h */

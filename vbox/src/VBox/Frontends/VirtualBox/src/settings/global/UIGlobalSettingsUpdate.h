/* $Id: UIGlobalSettingsUpdate.h 62493 2016-07-22 18:44:18Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsUpdate class declaration.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIGlobalSettingsUpdate_h__
#define __UIGlobalSettingsUpdate_h__

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIGlobalSettingsUpdate.gen.h"
#include "UIUpdateDefs.h"

/* Global settings / Update page / Cache: */
struct UISettingsCacheGlobalUpdate
{
    bool m_fCheckEnabled;
    VBoxUpdateData::PeriodType m_periodIndex;
    VBoxUpdateData::BranchType m_branchIndex;
    QString m_strDate;
};

/* Global settings / Update page: */
class UIGlobalSettingsUpdate : public UISettingsPageGlobal, public Ui::UIGlobalSettingsUpdate
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIGlobalSettingsUpdate();

protected:

    /* Load data to cache from corresponding external object(s),
     * this task COULD be performed in other than GUI thread: */
    void loadToCacheFrom(QVariant &data);
    /* Load data to corresponding widgets from cache,
     * this task SHOULD be performed in GUI thread only: */
    void getFromCache();

    /* Save data from corresponding widgets to cache,
     * this task SHOULD be performed in GUI thread only: */
    void putToCache();
    /* Save data from cache to corresponding external object(s),
     * this task COULD be performed in other than GUI thread: */
    void saveFromCacheTo(QVariant &data);

    /* Helper: Navigation stuff: */
    void setOrderAfter(QWidget *pWidget);

    /* Helper: Translation stuff: */
    void retranslateUi();

private slots:

    /* Handlers: */
    void sltUpdaterToggled(bool fEnabled);
    void sltPeriodActivated();
    void sltBranchToggled();

private:

    /* Helpers: */
    VBoxUpdateData::PeriodType periodType() const;
    VBoxUpdateData::BranchType branchType() const;

    /* Variables: */
    QRadioButton *m_pLastChosenRadio;
    bool m_fChanged;

    /* Cache: */
    UISettingsCacheGlobalUpdate m_cache;
};

#endif // __UIGlobalSettingsUpdate_h__

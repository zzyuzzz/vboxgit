/* $Id: UIGlobalSettingsLanguage.h 65686 2017-02-08 15:57:18Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsLanguage class declaration.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIGlobalSettingsLanguage_h___
#define ___UIGlobalSettingsLanguage_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIGlobalSettingsLanguage.gen.h"


/** Global settings: Language page data structure. */
struct UIDataSettingsGlobalLanguage
{
    /** Constructs data. */
    UIDataSettingsGlobalLanguage()
        : m_strLanguageId(QString())
    {}

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalLanguage &other) const
    {
        return true
               && (m_strLanguageId == other.m_strLanguageId)
               ;
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalLanguage &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalLanguage &other) const { return !equal(other); }

    /** Holds the current language id. */
    QString m_strLanguageId;
};
typedef UISettingsCache<UIDataSettingsGlobalLanguage> UISettingsCacheGlobalLanguage;


/** Global settings: Language page. */
class UIGlobalSettingsLanguage : public UISettingsPageGlobal, public Ui::UIGlobalSettingsLanguage
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIGlobalSettingsLanguage();

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

    /* Handlers: Event stuff: */
    void showEvent(QShowEvent *pEvent);
    void polishEvent(QShowEvent *pEvent);

private slots:

    /* Handler: List-painting stuff: */
    void sltLanguageItemPainted(QTreeWidgetItem *pItem, QPainter *pPainter);

    /* Handler: Current-changed stuff: */
    void sltCurrentLanguageChanged(QTreeWidgetItem *pItem);

private:

    /* Helper: List-loading stuff: */
    void reload(const QString &strLangId);

    /* Variables: */
    bool m_fPolished;

    /* Cache: */
    UISettingsCacheGlobalLanguage m_cache;
};

#endif /* !___UIGlobalSettingsLanguage_h___ */


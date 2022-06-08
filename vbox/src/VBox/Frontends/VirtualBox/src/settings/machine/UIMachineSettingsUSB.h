/* $Id: UIMachineSettingsUSB.h 66245 2017-03-24 13:43:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsUSB class declaration.
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

#ifndef ___UIMachineSettingsUSB_h___
#define ___UIMachineSettingsUSB_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsUSB.gen.h"

/* Forward declarations: */
class VBoxUSBMenu;
class UIToolBar;
struct UIDataSettingsMachineUSB;
struct UIDataSettingsMachineUSBFilter;
typedef UISettingsCache<UIDataSettingsMachineUSBFilter> UISettingsCacheMachineUSBFilter;
typedef UISettingsCachePool<UIDataSettingsMachineUSB, UISettingsCacheMachineUSBFilter> UISettingsCacheMachineUSB;


/** Machine settings: USB page. */
class UIMachineSettingsUSB : public UISettingsPageMachine,
                             public Ui::UIMachineSettingsUSB
{
    Q_OBJECT;

public:

    enum { ModeAny, ModeOn, ModeOff };

    /** Constructs USB settings page. */
    UIMachineSettingsUSB();
    /** Destructs USB settings page. */
    ~UIMachineSettingsUSB();

    bool isUSBEnabled() const;

protected:

    /** Returns whether the page content was changed. */
    virtual bool changed() const /* override */;

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

    /** Defines TAB order. */
    virtual void setOrderAfter(QWidget *pWidget) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private slots:

    void sltHandleUsbAdapterToggle(bool fEnabled);

    void sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem);
    void sltHandleContextMenuRequest(const QPoint &position);
    void sltHandleActivityStateChange(QTreeWidgetItem *pChangedItem);

    void sltNewFilter();
    void sltAddFilter();
    void sltEditFilter();
    void sltAddFilterConfirmed(QAction *pAction);
    void sltRemoveFilter();
    void sltMoveFilterUp();
    void sltMoveFilterDown();

private:

    void addUSBFilter(const UIDataSettingsMachineUSBFilter &usbFilterData, bool fChoose);

    /* Returns the multi-line description of the given USB filter: */
    static QString toolTipFor(const UIDataSettingsMachineUSBFilter &data);

    /* Other variables: */
    UIToolBar   *m_pToolBar;
    QAction     *m_pActionNew;
    QAction     *m_pActionAdd;
    QAction     *m_pActionEdit;
    QAction     *m_pActionRemove;
    QAction     *m_pActionMoveUp;
    QAction     *m_pActionMoveDown;
    VBoxUSBMenu *m_pMenuUSBDevices;

    QString  m_strTrUSBFilterName;

    QList<UIDataSettingsMachineUSBFilter>  m_filters;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineUSB *m_pCache;
};

#endif /* !___UIMachineSettingsUSB_h___ */


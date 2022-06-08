/* $Id: UIMachineSettingsUSB.h 66345 2017-03-29 18:03:32Z vboxsync $ */
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

    /** Remote mode types. */
    enum { ModeAny, ModeOn, ModeOff };

    /** Constructs USB settings page. */
    UIMachineSettingsUSB();
    /** Destructs USB settings page. */
    ~UIMachineSettingsUSB();

    /** Returns whether the USB is enabled. */
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

    /** Defines TAB order for passed @a pWidget. */
    virtual void setOrderAfter(QWidget *pWidget) /* override */;

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;

    /** Performs final page polishing. */
    virtual void polishPage() /* override */;

private slots:

    /** Handles whether USB adapted is @a fEnabled. */
    void sltHandleUsbAdapterToggle(bool fEnabled);

    /** Handles USB filter tree @a pCurrentItem change. */
    void sltHandleCurrentItemChange(QTreeWidgetItem *pCurrentItem);
    /** Handles context menu request for @a position of USB filter tree. */
    void sltHandleContextMenuRequest(const QPoint &position);
    /** Handles USB filter tree activity state change for @a pChangedItem. */
    void sltHandleActivityStateChange(QTreeWidgetItem *pChangedItem);

    /** Handles command to add new filter. */
    void sltNewFilter();
    /** Handles command to add existing filter. */
    void sltAddFilter();
    /** Handles command to edit filter. */
    void sltEditFilter();
    /** Handles command to confirm add of existing filter defined by @a pAction. */
    void sltAddFilterConfirmed(QAction *pAction);
    /** Handles command to remove chosen filter. */
    void sltRemoveFilter();
    /** Handles command to move chosen filter up. */
    void sltMoveFilterUp();
    /** Handles command to move chosen filter down. */
    void sltMoveFilterDown();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares USB filters tree. */
    void prepareFiltersTree();
    /** Prepares USB filters toolbar. */
    void prepareFiltersToolbar();
    /** Prepares connections. */
    void prepareConnections();
    /** Cleanups all. */
    void cleanup();

    /** Adds USB filter item based on a given @a usbFilterData, fChoose if requested. */
    void addUSBFilter(const UIDataSettingsMachineUSBFilter &usbFilterData, bool fChoose);

    /** Returns the multi-line description of the given USB filter. */
    static QString toolTipFor(const UIDataSettingsMachineUSBFilter &data);

    /** Holds the toolbar instance. */
    UIToolBar   *m_pToolBar;
    /** Holds the New action instance. */
    QAction     *m_pActionNew;
    /** Holds the Add action instance. */
    QAction     *m_pActionAdd;
    /** Holds the Edit action instance. */
    QAction     *m_pActionEdit;
    /** Holds the Remove action instance. */
    QAction     *m_pActionRemove;
    /** Holds the Move Up action instance. */
    QAction     *m_pActionMoveUp;
    /** Holds the Move Down action instance. */
    QAction     *m_pActionMoveDown;
    /** Holds the USB devices menu instance. */
    VBoxUSBMenu *m_pMenuUSBDevices;

    /** Holds the "New Filter %1" translation tag. */
    QString  m_strTrUSBFilterName;

    /** Holds the list of all USB filters. */
    QList<UIDataSettingsMachineUSBFilter>  m_filters;

    /** Holds the page data cache instance. */
    UISettingsCacheMachineUSB *m_pCache;
};

#endif /* !___UIMachineSettingsUSB_h___ */


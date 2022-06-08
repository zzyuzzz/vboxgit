/* $Id: UIMachineSettingsSF.h 66183 2017-03-21 16:26:35Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineSettingsSF class declaration.
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

#ifndef ___UIMachineSettingsSF_h___
#define ___UIMachineSettingsSF_h___

/* GUI includes: */
#include "UISettingsPage.h"
#include "UIMachineSettingsSF.gen.h"

/* COM includes: */
#include "CSharedFolder.h"

/* Forward declarations: */
class SFTreeViewItem;
struct UIDataSettingsSharedFolder;
struct UIDataSettingsSharedFolders;
enum UISharedFolderType { MachineType, ConsoleType };
typedef UISettingsCache<UIDataSettingsSharedFolder> UISettingsCacheSharedFolder;
typedef UISettingsCachePool<UIDataSettingsSharedFolders, UISettingsCacheSharedFolder> UISettingsCacheSharedFolders;
typedef QPair<QString, UISharedFolderType> SFolderName;
typedef QList<SFolderName> SFoldersNameList;


/** Machine settings: Shared Folders page. */
class UIMachineSettingsSF : public UISettingsPageMachine,
                            public Ui::UIMachineSettingsSF
{
    Q_OBJECT;

public:

    /** Constructs Shared Folders settings page. */
    UIMachineSettingsSF();
    /** Destructs Shared Folders settings page. */
    ~UIMachineSettingsSF();

protected:

    /** Returns whether the page content was changed. */
    bool changed() const /* override */;

    /** Loads data into the cache from corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    void loadToCacheFrom(QVariant &data);
    void loadToCacheFrom(UISharedFolderType sharedFoldersType);
    /** Loads data into corresponding widgets from the cache,
      * this task SHOULD be performed in the GUI thread only. */
    void getFromCache();

    /** Saves data from corresponding widgets to the cache,
      * this task SHOULD be performed in the GUI thread only. */
    void putToCache();
    /** Saves data from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    void saveFromCacheTo(QVariant &data);
    /** Saves data of @a sharedFoldersType from the cache to corresponding external object(s),
      * this task COULD be performed in other than the GUI thread. */
    void saveFromCacheTo(UISharedFolderType sharedFoldersType);

    /** Defines TAB order. */
    void setOrderAfter(QWidget *pWidget);

    /** Handles translation event. */
    void retranslateUi();

private slots:

    void addTriggered();
    void edtTriggered();
    void delTriggered();

    void processCurrentChanged (QTreeWidgetItem *aCurrentItem);
    void processDoubleClick (QTreeWidgetItem *aItem);
    void showContextMenu (const QPoint &aPos);

    void adjustList();
    void adjustFields();

private:

    void resizeEvent (QResizeEvent *aEvent);

    void showEvent (QShowEvent *aEvent);

    SFTreeViewItem* root(UISharedFolderType type);
    SFoldersNameList usedList (bool aIncludeSelected);

    bool isSharedFolderTypeSupported(UISharedFolderType sharedFolderType) const;
    void updateRootItemsVisibility();
    void setRootItemVisible(UISharedFolderType sharedFolderType, bool fVisible);

    void polishPage();

    CSharedFolderVector getSharedFolders(UISharedFolderType sharedFoldersType);

    bool removeSharedFolder(const UISettingsCacheSharedFolder &folderCache);
    bool createSharedFolder(const UISettingsCacheSharedFolder &folderCache);

    QAction  *mNewAction;
    QAction  *mEdtAction;
    QAction  *mDelAction;
    QString   mTrFull;
    QString   mTrReadOnly;
    QString   mTrYes;

    /** Holds the page data cache instance. */
    UISettingsCacheSharedFolders *m_pCache;
};

#endif /* !___UIMachineSettingsSF_h___ */


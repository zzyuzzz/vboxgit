/* $Id: UIActionPool.h 73694 2018-08-15 15:07:32Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIActionPool class declaration.
 */

/*
 * Copyright (C) 2010-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIActionPool_h___
#define ___UIActionPool_h___

/* Qt includes: */
#include <QAction>
#include <QMenu>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UIExtraDataDefs.h"
#include "UILibraryDefs.h"

/* Forward declarations: */
class QKeySequence;
class QString;
class UIActionPolymorphic;
class UIActionPolymorphicMenu;
class UIActionPool;
class UIActionPoolRuntime;
class UIActionPoolSelector;


/** Action-pool types. */
enum UIActionPoolType
{
    UIActionPoolType_Selector,
    UIActionPoolType_Runtime
};

/** Action types. */
enum UIActionType
{
    UIActionType_Menu,
    UIActionType_Simple,
    UIActionType_Toggle,
    UIActionType_Polymorphic,
    UIActionType_PolymorphicMenu
};

/** Action indexes. */
enum UIActionIndex
{
    /* 'Application' menu actions: */
    UIActionIndex_M_Application,
#ifdef VBOX_WS_MAC
    UIActionIndex_M_Application_S_About,
#endif
    UIActionIndex_M_Application_S_Preferences,
#ifdef VBOX_GUI_WITH_NETWORK_MANAGER
    UIActionIndex_M_Application_S_NetworkAccessManager,
    UIActionIndex_M_Application_S_CheckForUpdates,
#endif /* VBOX_GUI_WITH_NETWORK_MANAGER */
    UIActionIndex_M_Application_S_ResetWarnings,
    UIActionIndex_M_Application_S_Close,

#ifdef VBOX_WS_MAC
    /* 'Window' menu actions: */
    UIActionIndex_M_Window,
    UIActionIndex_M_Window_S_Minimize,
#endif

    /* 'Help' menu actions: */
    UIActionIndex_Menu_Help,
    UIActionIndex_Simple_Contents,
    UIActionIndex_Simple_WebSite,
    UIActionIndex_Simple_BugTracker,
    UIActionIndex_Simple_Forums,
    UIActionIndex_Simple_Oracle,
#ifndef VBOX_WS_MAC
    UIActionIndex_Simple_About,
#endif

    /* 'Log Viewer' menu actions: */
    UIActionIndex_M_LogViewer,
    UIActionIndex_M_LogViewer_T_Find,
    UIActionIndex_M_LogViewer_T_Filter,
    UIActionIndex_M_LogViewer_T_Bookmark,
    UIActionIndex_M_LogViewer_T_Settings,
    UIActionIndex_M_LogViewer_S_Refresh,
    UIActionIndex_M_LogViewer_S_Save,

    /* Maximum index: */
    UIActionIndex_Max
};

/** Restriction levels. */
enum UIActionRestrictionLevel
{
    UIActionRestrictionLevel_Base,
    UIActionRestrictionLevel_Session,
    UIActionRestrictionLevel_Logic
};


/** QMenu extension. */
class SHARED_LIBRARY_STUFF UIMenu : public QMenu
{
    Q_OBJECT;

public:

    /** Constructs menu. */
    UIMenu();

    /** Defines whether tool-tip should be shown. */
    void setShowToolTip(bool fShowToolTips) { m_fShowToolTip = fShowToolTips; }

#ifdef VBOX_WS_MAC
    /** Mac OS X: Returns whether this menu is consumable by the menu-bar. */
    bool isConsumable() const { return m_fConsumable; }
    /** Mac OS X: Defines whether this menu is @a fConsumable by the menu-bar. */
    void setConsumable(bool fConsumable) { m_fConsumable = fConsumable; }

    /** Mac OS X: Returns whether this menu is consumed by the menu-bar. */
    bool isConsumed() const { return m_fConsumed; }
    /** Mac OS X: Defines whether this menu is @a fConsumed by the menu-bar. */
    void setConsumed(bool fConsumed) { m_fConsumed = fConsumed; }
#endif /* VBOX_WS_MAC */

protected:

    /** Handles any Qt @a pEvent. */
    virtual bool event(QEvent *pEvent);

private:

    /** Holds whether tool-tip should be shown. */
    bool m_fShowToolTip;

#ifdef VBOX_WS_MAC
    /** Mac OS X: Holds whether this menu can be consumed by the menu-bar. */
    bool m_fConsumable;
    /** Mac OS X: Holds whether this menu is consumed by the menu-bar. */
    bool m_fConsumed;
#endif /* VBOX_WS_MAC */
};


/** Abstract QAction extension. */
class SHARED_LIBRARY_STUFF UIAction : public QAction
{
    Q_OBJECT;

public:

    /** Returns action type. */
    UIActionType type() const { return m_enmType; }

    /** Returns menu contained by this action. */
    UIMenu *menu() const;

    /** Returns action-pool this action belongs to. */
    UIActionPool *actionPool() const { return m_pActionPool; }

    /** Casts action to polymorphic-action. */
    UIActionPolymorphic *toActionPolymorphic();
    /** Casts action to polymorphic-menu-action. */
    UIActionPolymorphicMenu *toActionPolymorphicMenu();

    /** Returns current action name. */
    const QString &name() const { return m_strName; }
    /** Defines current action name. */
    void setName(const QString &strName);

    /** Returns action extra-data ID. */
    virtual int extraDataID() const { return 0; }
    /** Returns action extra-data key. */
    virtual QString extraDataKey() const { return QString(); }
    /** Returns whether action is allowed. */
    virtual bool isAllowed() const { return true; }

    /** Returns extra-data ID to save keyboard shortcut under. */
    virtual QString shortcutExtraDataID() const { return QString(); }
    /** Returns default keyboard shortcut for this action. */
    virtual QKeySequence defaultShortcut(UIActionPoolType) const { return QKeySequence(); }

    /** Defines current keyboard shortcut for this action. */
    void setShortcut(const QKeySequence &shortcut);
    /** Make action show keyboard shortcut. */
    void showShortcut();
    /** Make action hide keyboard shortcut. */
    void hideShortcut();

    /** Retranslates action. */
    virtual void retranslateUi() = 0;
    virtual ~UIAction() /* override */ { delete menu(); }

protected:

    /** Constructs action passing @a pParent to the base-class.
      * @param  enmType  Brings the action type. */
    UIAction(UIActionPool *pParent, UIActionType enmType);

    /** Returns current action name in menu. */
    QString nameInMenu() const;

    /** Updates action text accordingly. */
    virtual void updateText();

private:

    /** Holds the action type. */
    UIActionType  m_enmType;

    /** Holds the reference to the action-pool this action belongs to. */
    UIActionPool     *m_pActionPool;
    /** Holds the type of the action-pool this action belongs to. */
    UIActionPoolType  m_enmActionPoolType;

    /** Holds the action name. */
    QString       m_strName;
    /** Holds the action shortcut. */
    QKeySequence  m_shortcut;
    /** Holds whether action shortcut hidden. */
    bool          m_fShortcutHidden;
};


/** Abstract UIAction extension for 'Menu' action type. */
class SHARED_LIBRARY_STUFF UIActionMenu : public UIAction
{
    Q_OBJECT;

protected:

    /** Constructs menu action passing @a pParent to the base-class.
      * @param  strIcon          Brings the normal-icon name.
      * @param  strIconDisabled  Brings the disabled-icon name. */
    UIActionMenu(UIActionPool *pParent,
                 const QString &strIcon = QString(),
                 const QString &strIconDisabled = QString());
    /** Constructs menu action passing @a pParent to the base-class.
      * @param  icon  Brings the icon. */
    UIActionMenu(UIActionPool *pParent,
                 const QIcon &icon);

    /** Defines whether tool-tip should be shown. */
    void setShowToolTip(bool fShowToolTip);

private:

    /** Prepare routine. */
    void prepare();

    /** Updates action text accordingly. */
    virtual void updateText();
};


/** Abstract UIAction extension for 'Simple' action type. */
class SHARED_LIBRARY_STUFF UIActionSimple : public UIAction
{
    Q_OBJECT;

protected:

    /** Constructs simple action passing @a pParent to the base-class.
      * @param  strIcon          Brings the normal-icon name.
      * @param  strIconDisabled  Brings the disabled-icon name. */
    UIActionSimple(UIActionPool *pParent,
                   const QString &strIcon = QString(),
                   const QString &strIconDisabled = QString());
    /** Constructs simple action passing @a pParent to the base-class.
      * @param  strIconNormal          Brings the normal-icon name.
      * @param  strIconSmall           Brings the small-icon name.
      * @param  strIconNormalDisabled  Brings the normal-disabled-icon name.
      * @param  strIconSmallDisabled   Brings the small-disabled-icon name. */
    UIActionSimple(UIActionPool *pParent,
                   const QString &strIconNormal, const QString &strIconSmall,
                   const QString &strIconNormalDisabled, const QString &strIconSmallDisabled);
    /** Constructs simple action passing @a pParent to the base-class.
      * @param  icon  Brings the icon. */
    UIActionSimple(UIActionPool *pParent,
                   const QIcon& icon);
};


/** Abstract UIAction extension for 'Toggle' action type. */
class SHARED_LIBRARY_STUFF UIActionToggle : public UIAction
{
    Q_OBJECT;

protected:

    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  strIcon          Brings the normal-icon name.
      * @param  strIconDisabled  Brings the disabled-icon name. */
    UIActionToggle(UIActionPool *pParent,
                   const QString &strIcon = QString(), const QString &strIconDisabled = QString());
    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  strIconOn           Brings the on-icon name.
      * @param  strIconOff          Brings the off-icon name.
      * @param  strIconOnDisabled   Brings the on-disabled-icon name.
      * @param  strIconOffDisabled  Brings the off-disabled-icon name. */
    UIActionToggle(UIActionPool *pParent,
                   const QString &strIconOn, const QString &strIconOff,
                   const QString &strIconOnDisabled, const QString &strIconOffDisabled);
    /** Constructs toggle action passing @a pParent to the base-class.
      * @param  icon  Brings the icon. */
    UIActionToggle(UIActionPool *pParent,
                   const QIcon &icon);

private:

    /** Prepare routine. */
    void prepare();
};


/** Abstract UIAction extension for 'Polymorphic' action type. */
class SHARED_LIBRARY_STUFF UIActionPolymorphic : public UIAction
{
    Q_OBJECT;

public:

    /** Returns current action state. */
    int state() const { return m_iState; }
    /** Defines current action state. */
    void setState(int iState) { m_iState = iState; retranslateUi(); }

protected:

    /** Constructs polymorphic action passing @a pParent to the base-class.
      * @param  strIcon          Brings the normal-icon name.
      * @param  strIconDisabled  Brings the disabled-icon name. */
    UIActionPolymorphic(UIActionPool *pParent,
                        const QString &strIcon = QString(), const QString &strIconDisabled = QString());
    /** Constructs polymorphic action passing @a pParent to the base-class.
      * @param  strIconNormal          Brings the normal-icon name.
      * @param  strIconSmall           Brings the small-icon name.
      * @param  strIconNormalDisabled  Brings the normal-disabled-icon name.
      * @param  strIconSmallDisabled   Brings the small-disabled-icon name. */
    UIActionPolymorphic(UIActionPool *pParent,
                        const QString &strIconNormal, const QString &strIconSmall,
                        const QString &strIconNormalDisabled, const QString &strIconSmallDisabled);
    /** Constructs polymorphic action passing @a pParent to the base-class.
      * @param  icon  Brings the icon. */
    UIActionPolymorphic(UIActionPool *pParent,
                        const QIcon& icon);

private:

    /** Holds current action state. */
    int m_iState;
};


/** Abstract UIAction extension for 'Polymorphic Menu' action type. */
class SHARED_LIBRARY_STUFF UIActionPolymorphicMenu : public UIAction
{
    Q_OBJECT;

public:

    /** Returns current action state. */
    int state() const { return m_iState; }
    /** Defines current action state. */
    void setState(int iState) { m_iState = iState; retranslateUi(); }

protected:

    /** Constructs polymorphic menu action passing @a pParent to the base-class.
      * @param  strIcon          Brings the normal-icon name.
      * @param  strIconDisabled  Brings the disabled-icon name. */
    UIActionPolymorphicMenu(UIActionPool *pParent,
                            const QString &strIcon = QString(), const QString &strIconDisabled = QString());
    /** Constructs polymorphic menu action passing @a pParent to the base-class.
      * @param  strIconNormal          Brings the normal-icon name.
      * @param  strIconSmall           Brings the small-icon name.
      * @param  strIconNormalDisabled  Brings the normal-disabled-icon name.
      * @param  strIconSmallDisabled   Brings the small-disabled-icon name. */
    UIActionPolymorphicMenu(UIActionPool *pParent,
                            const QString &strIconNormal, const QString &strIconSmall,
                            const QString &strIconNormalDisabled, const QString &strIconSmallDisabled);
    /** Constructs polymorphic menu action passing @a pParent to the base-class.
      * @param  icon  Brings the icon. */
    UIActionPolymorphicMenu(UIActionPool *pParent,
                            const QIcon &icon);

    /** Destructs polymorphic menu action. */
    ~UIActionPolymorphicMenu();

    /** Defines whether tool-tip should be shown. */
    void setShowToolTip(bool fShowToolTip);

    /** Show menu. */
    void showMenu();
    /** Hide menu. */
    void hideMenu();

private:

    /** Prepare routine. */
    void prepare();

    /** Updates action text accordingly. */
    virtual void updateText();

private:

    /** Holds the menu instance. */
    UIMenu *m_pMenu;
    /** Holds current action state. */
    int     m_iState;
};


/** Abstract QObject extension
  * representing action-pool interface and factory. */
class SHARED_LIBRARY_STUFF UIActionPool : public QIWithRetranslateUI3<QObject>
{
    Q_OBJECT;

    /** Pointer to menu update-handler for this class. */
    typedef void (UIActionPool::*PTFActionPool)();
    /** Pointer to menu update-handler for Runtime sub-class. */
    typedef void (UIActionPoolRuntime::*PTFActionPoolRuntime)();
    /** Union for two defines above. */
    union PointerToFunction
    {
        PTFActionPool ptf;
        PTFActionPoolRuntime ptfr;
    };

signals:

    /** Notifies about menu prepare. */
    void sigNotifyAboutMenuPrepare(int iIndex, QMenu *pMenu);

#ifdef VBOX_WS_MAC
    /** Notifies about @a pAction hovered. */
    void sigActionHovered(UIAction *pAction);
#endif

public:

    /** Creates singleton instance. */
    static UIActionPool *create(UIActionPoolType enmType);
    /** Destroys singleton instance. */
    static void destroy(UIActionPool *pActionPool);

    /** Creates temporary singleton instance,
      * used to initialize shortcuts-pool from action-pool of passed @a enmType. */
    static void createTemporary(UIActionPoolType enmType);

    /** Cast action-pool to Runtime one. */
    UIActionPoolRuntime *toRuntime();
    /** Cast action-pool to Selector one. */
    UIActionPoolSelector *toSelector();

    /** Returns action-pool type. */
    UIActionPoolType type() const { return m_enmType; }

    /** Returns the action for the passed @a iIndex. */
    UIAction *action(int iIndex) const { return m_pool.value(iIndex); }
    /** Returns all the actions action-pool contains. */
    QList<UIAction*> actions() const { return m_pool.values(); }

    /** Returns whether the menu with passed @a type is allowed in menu-bar. */
    bool isAllowedInMenuBar(UIExtraDataMetaDefs::MenuType type) const;
    /** Defines menu-bar @a enmRestriction for passed @a level. */
    void setRestrictionForMenuBar(UIActionRestrictionLevel level, UIExtraDataMetaDefs::MenuType enmRestriction);

    /** Returns whether the action with passed @a type is allowed in the 'Application' menu. */
    bool isAllowedInMenuApplication(UIExtraDataMetaDefs::MenuApplicationActionType type) const;
    /** Defines 'Application' menu @a enmRestriction for passed @a level. */
    void setRestrictionForMenuApplication(UIActionRestrictionLevel level, UIExtraDataMetaDefs::MenuApplicationActionType enmRestriction);

#ifdef VBOX_WS_MAC
    /** Mac OS X: Returns whether the action with passed @a type is allowed in the 'Window' menu. */
    bool isAllowedInMenuWindow(UIExtraDataMetaDefs::MenuWindowActionType type) const;
    /** Mac OS X: Defines 'Window' menu @a enmRestriction for passed @a level. */
    void setRestrictionForMenuWindow(UIActionRestrictionLevel level, UIExtraDataMetaDefs::MenuWindowActionType enmRestriction);
#endif /* VBOX_WS_MAC */

    /** Returns whether the action with passed @a type is allowed in the 'Help' menu. */
    bool isAllowedInMenuHelp(UIExtraDataMetaDefs::MenuHelpActionType enmType) const;
    /** Defines 'Help' menu @a enmRestriction for passed @a level. */
    void setRestrictionForMenuHelp(UIActionRestrictionLevel enmLevel, UIExtraDataMetaDefs::MenuHelpActionType enmRestriction);

    /** Hot-key processing delegate. */
    bool processHotKey(const QKeySequence &key);

    /** Returns extra-data ID to save keyboard shortcuts under. */
    virtual QString shortcutsExtraDataID() const = 0;

    /** Returns the list of main menus. */
    virtual QList<QMenu*> menus() const = 0;

public slots:

    /** Handles menu prepare. */
    void sltHandleMenuPrepare();

#ifdef VBOX_WS_MAC
    /** Handles action hovered signal. */
    void sltActionHovered();
#endif

protected slots:

    /** Loads keyboard shortcuts of action-pool into shortcuts-pool. */
    void sltApplyShortcuts() { updateShortcuts(); }

protected:

    /** Constructs probably @a fTemporary action-pool of passed @a enmType. */
    UIActionPool(UIActionPoolType enmType, bool fTemporary = false);

    /** Prepares all. */
    void prepare();
    /** Prepares pool. */
    virtual void preparePool();
    /** Prepares connections. */
    virtual void prepareConnections();
    /** Cleanups connections. */
    virtual void cleanupConnections() {}
    /** Cleanups pool. */
    virtual void cleanupPool();
    /** Cleanups all. */
    void cleanup();

    /** Updates configuration. */
    virtual void updateConfiguration();

    /** Updates menu with certain @a iIndex. */
    virtual void updateMenu(int iIndex);
    /** Updates menus. */
    virtual void updateMenus() = 0;
    /** Updates 'Application' menu. */
    virtual void updateMenuApplication();
#ifdef VBOX_WS_MAC
    /** Mac OS X: Updates 'Window' menu. */
    virtual void updateMenuWindow();
#endif
    /** Updates 'Help' menu. */
    virtual void updateMenuHelp();
    /** Updates 'Log Viewer' menu. */
    virtual void updateMenuLogViewer();

    /** Updates shortcuts. */
    virtual void updateShortcuts();

    /** Hadles translation event. */
    virtual void retranslateUi() /* override */;

    /** Handles any Qt @a pEvent */
    virtual bool event(QEvent *pEvent) /* override */;

    /** Adds action into corresponding menu. */
    bool addAction(UIMenu *pMenu, UIAction *pAction, bool fReallyAdd = true);
    /** Adds action's menu into corresponding menu list. */
    bool addMenu(QList<QMenu*> &menuList, UIAction *pAction, bool fReallyAdd = true);

    /** Holds the action-pool type. */
    const UIActionPoolType  m_enmType;
    /** Holds whether this action-pool is temporary. */
    const bool              m_fTemporary;

    /** Holds the map of actions. */
    QMap<int, UIAction*>          m_pool;
    /** Holds the map of validation handlers. */
    QMap<int, PointerToFunction>  m_menuUpdateHandlers;
    /** Holds the set of invalidated action indexes. */
    QSet<int>                     m_invalidations;

    /** Holds restricted menu types. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuType>                   m_restrictedMenus;
    /** Holds restricted action types of the 'Application' menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuApplicationActionType>  m_restrictedActionsMenuApplication;
#ifdef VBOX_WS_MAC
    /** Mac OS X: Holds restricted action types of the 'Window' menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuWindowActionType>       m_restrictedActionsMenuWindow;
#endif
    /** Holds restricted action types of the Help menu. */
    QMap<UIActionRestrictionLevel, UIExtraDataMetaDefs::MenuHelpActionType>         m_restrictedActionsMenuHelp;
};


#endif /* !___UIActionPool_h___ */


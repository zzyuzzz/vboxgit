/* $Id: UIChooserItemGroup.h 73927 2018-08-28 11:40:48Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserItemGroup class declaration.
 */

/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIChooserItemGroup_h__
#define __UIChooserItemGroup_h__

/* Qt includes: */
#include <QWidget>
#include <QPixmap>

/* GUI includes: */
#include "UIChooserItem.h"

/* Forward declarations: */
class QGraphicsScene;
class QGraphicsProxyWidget;
class QLineEdit;
class UIGraphicsButton;
class UIGraphicsRotatorButton;
class UIEditorGroupRename;

/* Graphics group-item
 * for graphics selector model/view architecture: */
class UIChooserItemGroup : public UIChooserItem
{
    Q_OBJECT;
    Q_PROPERTY(int additionalHeight READ additionalHeight WRITE setAdditionalHeight);

signals:

    /* Notifiers: Toggle stuff: */
    void sigToggleStarted();
    void sigToggleFinished();

public:

    /* Class-name used for drag&drop mime-data format: */
    static QString className();

    /* Graphics-item type: */
    enum { Type = UIChooserItemType_Group };
    int type() const { return Type; }

    /* Constructor (main-root-item): */
    UIChooserItemGroup(QGraphicsScene *pScene);
    /* Constructor (temporary main-root-item/root-item copy): */
    UIChooserItemGroup(QGraphicsScene *pScene, UIChooserItemGroup *pCopyFrom, bool fMainRoot);
    /* Constructor (new non-root-item): */
    UIChooserItemGroup(UIChooserItem *pParent, const QString &strName, bool fOpened = false, int iPosition  = -1);
    /* Constructor (new non-root-item copy): */
    UIChooserItemGroup(UIChooserItem *pParent, UIChooserItemGroup *pCopyFrom, int iPosition = -1);
    /* Destructor: */
    ~UIChooserItemGroup();

    /* API: Basic stuff: */
    QString name() const;
    QString description() const;
    QString fullName() const;
    QString definition() const;
    void setName(const QString &strName);
    bool isClosed() const;
    bool isOpened() const;
    void close(bool fAnimated = true);
    void open(bool fAnimated = true);

    /* API: Children stuff: */
    bool isContainsMachine(const QString &strId) const;
    bool isContainsLockedMachine();

private slots:

    /** Handles top-level window remaps. */
    void sltHandleWindowRemapped();

    /* Handler: Name editing stuff: */
    void sltNameEditingFinished();

    /* Handler: Toggle stuff: */
    void sltGroupToggleStart();
    void sltGroupToggleFinish(bool fToggled);

    /* Handlers: Indent root stuff: */
    void sltIndentRoot();
    void sltUnindentRoot();

private:

    /* Data enumerator: */
    enum GroupItemData
    {
        /* Layout hints: */
        GroupItemData_HorizonalMargin,
        GroupItemData_VerticalMargin,
        GroupItemData_MajorSpacing,
        GroupItemData_MinorSpacing,
        GroupItemData_RootIndent,
    };

    /* Data provider: */
    QVariant data(int iKey) const;

    /* Helpers: Prepare stuff: */
    void prepare();
    static void copyContent(UIChooserItemGroup *pFrom, UIChooserItemGroup *pTo);

    /* Helpers: Update stuff: */
    void handleRootStatusChange();
    void updateVisibleName();
    void updatePixmaps();
    void updateItemCountInfo();
    void updateMinimumHeaderSize();
    void updateToolTip();
    void updateToggleButtonToolTip();

    /* Helper: Translate stuff: */
    void retranslateUi();

    /* Helpers: Basic stuff: */
    void show();
    void hide();
    void startEditing();
    bool isMainRoot() const { return m_fMainRoot; }

    /* Helpers: Children stuff: */
    void addItem(UIChooserItem *pItem, int iPosition);
    void removeItem(UIChooserItem *pItem);
    void setItems(const QList<UIChooserItem*> &items, UIChooserItemType type);
    QList<UIChooserItem*> items(UIChooserItemType type = UIChooserItemType_Any) const;
    bool hasItems(UIChooserItemType type = UIChooserItemType_Any) const;
    void clearItems(UIChooserItemType type = UIChooserItemType_Any);
    void updateAllItems(const QString &strId);
    void removeAllItems(const QString &strId);
    UIChooserItem *searchForItem(const QString &strSearchTag, int iItemSearchFlags);
    UIChooserItem *firstMachineItem();
    void sortItems();

    /* Helpers: Layout stuff: */
    void updateLayout();
    int minimumWidthHint(bool fOpenedGroup) const;
    int minimumHeightHint(bool fOpenedGroup) const;
    int minimumWidthHint() const;
    int minimumHeightHint() const;
#ifdef VBOX_WS_MAC
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif
    QSizeF minimumSizeHint(bool fOpenedGroup) const;
#ifdef VBOX_WS_MAC
# pragma clang diagnostic pop
#endif
    QSizeF sizeHint(Qt::SizeHint which, const QSizeF &constraint = QSizeF()) const;

    /* Helpers: Drag&drop stuff: */
    QPixmap toPixmap();
    bool isDropAllowed(QGraphicsSceneDragDropEvent *pEvent, DragToken where) const;
    void processDrop(QGraphicsSceneDragDropEvent *pEvent, UIChooserItem *pFromWho, DragToken where);
    void resetDragToken();
    QMimeData* createMimeData();

    /** Handles show @a pEvent. */
    virtual void showEvent(QShowEvent *pEvent) /* override */;

    /* Handler: Resize handling stuff: */
    void resizeEvent(QGraphicsSceneResizeEvent *pEvent);

    /* Handlers: Hover handling stuff: */
    void hoverMoveEvent(QGraphicsSceneHoverEvent *pEvent);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *pEvent);

    /* Helpers: Paint stuff: */
    void paint(QPainter *pPainter, const QStyleOptionGraphicsItem *pOption, QWidget *pWidget = 0);
    void paintBackground(QPainter *pPainter, const QRect &rect);
    void paintHeader(QPainter *pPainter, const QRect &rect);

    /* Helpers: Animation stuff: */
    void updateAnimationParameters();
    void setAdditionalHeight(int iAdditionalHeight);
    int additionalHeight() const;

    /* Helper: Color stuff: */
    int blackoutDarkness() const { return m_iBlackoutDarkness; }

    /* Variables: */
    bool m_fClosed;
    UIGraphicsRotatorButton *m_pToggleButton;
    UIGraphicsButton *m_pEnterButton;
    UIGraphicsButton *m_pExitButton;
    UIEditorGroupRename *m_pNameEditorWidget;
    QGraphicsProxyWidget *m_pNameEditor;
    QList<UIChooserItem*> m_groupItems;
    QList<UIChooserItem*> m_globalItems;
    QList<UIChooserItem*> m_machineItems;
    int m_iAdditionalHeight;
    int m_iCornerRadius;
    bool m_fMainRoot;
    int m_iBlackoutDarkness;
    /* Cached values: */
    QString m_strName;
    QString m_strDescription;
    QString m_strVisibleName;
    QString m_strInfoGroups;
    QString m_strInfoMachines;
    QSize m_visibleNameSize;
    QSize m_infoSizeGroups;
    QSize m_infoSizeMachines;
    QSize m_pixmapSizeGroups;
    QSize m_pixmapSizeMachines;
    QSize m_minimumHeaderSize;
    QSize m_toggleButtonSize;
    QSize m_enterButtonSize;
    QSize m_exitButtonSize;
    QFont m_nameFont;
    QFont m_infoFont;
    QPixmap m_groupsPixmap;
    QPixmap m_machinesPixmap;
};

class UIEditorGroupRename : public QWidget
{
    Q_OBJECT;

signals:

    /* Notifier: Editing stuff: */
    void sigEditingFinished();

public:

    /* Constructor: */
    UIEditorGroupRename(const QString &strName, UIChooserItem *pParent);

    /* API: Text stuff: */
    QString text() const;
    void setText(const QString &strText);

    /* API: Font stuff: */
    void setFont(const QFont &font);

public slots:

    /* API/Handler: Focus stuff: */
    void setFocus();

private:

    /* Handler: Event-filter: */
    bool eventFilter(QObject *pWatched, QEvent *pEvent);

    /* Helper: Context-menu stuff: */
    void handleContextMenuEvent(QContextMenuEvent *pContextMenuEvent);

    /* Variables: */
    UIChooserItem *m_pParent;
    QLineEdit *m_pLineEdit;
    QMenu *m_pTemporaryMenu;
};

#endif /* __UIChooserItemGroup_h__ */


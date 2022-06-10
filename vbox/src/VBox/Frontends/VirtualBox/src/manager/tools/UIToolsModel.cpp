/* $Id: UIToolsModel.cpp 74249 2018-09-13 16:24:26Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIToolsModel class implementation.
 */

/*
 * Copyright (C) 2012-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
# include <QGraphicsScene>
# include <QGraphicsSceneContextMenuEvent>
# include <QGraphicsSceneMouseEvent>
# include <QGraphicsView>
# include <QPropertyAnimation>
# include <QScrollBar>
# include <QRegExp>
# include <QTimer>

/* GUI includes: */
# include "QIMessageBox.h"
# include "VBoxGlobal.h"
# include "UIActionPoolSelector.h"
# include "UIIconPool.h"
# include "UITools.h"
# include "UIToolsHandlerMouse.h"
# include "UIToolsHandlerKeyboard.h"
# include "UIToolsModel.h"
# include "UIExtraDataDefs.h"
# include "UIExtraDataManager.h"
# include "UIMessageCenter.h"
# include "UIModalWindowManager.h"
# include "UIVirtualBoxManagerWidget.h"
# include "UIVirtualBoxEventHandler.h"
# include "UIWizardNewVM.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
#include <QParallelAnimationGroup>

/* Type defs: */
typedef QSet<QString> UIStringSet;


UIToolsModel:: UIToolsModel(UITools *pParent)
    : QObject(pParent)
    , m_pTools(pParent)
    , m_pScene(0)
    , m_pMouseHandler(0)
    , m_pKeyboardHandler(0)
    , m_enmCurrentClass(UIToolsClass_Global)
{
    /* Prepare: */
    prepare();
}

UIToolsModel::~UIToolsModel()
{
    /* Cleanup: */
    cleanup();
}

void UIToolsModel::init()
{
    /* Update navigation: */
    updateNavigation();

    /* Update layout: */
    updateLayout();

    /* Load last selected item: */
    loadLastSelectedItem();
}

void UIToolsModel::deinit()
{
    /* Save last selected item: */
    saveLastSelectedItem();
}

UITools *UIToolsModel::tools() const
{
    return m_pTools;
}

UIActionPool *UIToolsModel::actionPool() const
{
    return tools()->actionPool();
}

QGraphicsScene *UIToolsModel::scene() const
{
    return m_pScene;
}

QPaintDevice *UIToolsModel::paintDevice() const
{
    if (scene() && !scene()->views().isEmpty())
        return scene()->views().first();
    return 0;
}

QGraphicsItem *UIToolsModel::itemAt(const QPointF &position, const QTransform &deviceTransform /* = QTransform() */) const
{
    return scene()->itemAt(position, deviceTransform);
}

void UIToolsModel::setToolsClass(UIToolsClass enmClass)
{
    /* Update linked values: */
    if (m_enmCurrentClass != enmClass)
    {
        m_enmCurrentClass = enmClass;
        updateLayout();
        updateNavigation();
    }
}

UIToolsClass UIToolsModel::toolsClass() const
{
    return m_enmCurrentClass;
}

UIToolsType UIToolsModel::toolsType() const
{
    return currentItem()->itemType();
}

void UIToolsModel::setCurrentItem(UIToolsItem *pItem)
{
    /* Is there something changed? */
    if (m_pCurrentItem == pItem)
        return;

    /* Remember old current-item: */
    UIToolsItem *pOldCurrentItem = m_pCurrentItem;

    /* If there is item: */
    if (pItem)
    {
        /* Set this item to current if navigation list contains it: */
        if (navigationList().contains(pItem))
            m_pCurrentItem = pItem;
        /* Otherwise it's error: */
        else
            AssertMsgFailed(("Passed item is not in navigation list!"));
    }
    /* Otherwise reset current item: */
    else
        m_pCurrentItem = 0;

    /* Update old item (if any): */
    if (pOldCurrentItem)
        pOldCurrentItem->update();
    /* Update new item (if any): */
    if (m_pCurrentItem)
        m_pCurrentItem->update();

    /* Notify about selection change: */
    emit sigSelectionChanged();

    /* Move focus to current-item: */
    setFocusItem(currentItem());
}

UIToolsItem *UIToolsModel::currentItem() const
{
    return m_pCurrentItem;
}

void UIToolsModel::setFocusItem(UIToolsItem *pItem)
{
    /* Always make sure real focus unset: */
    scene()->setFocusItem(0);

    /* Is there something changed? */
    if (m_pFocusItem == pItem)
        return;

    /* Remember old focus-item: */
    UIToolsItem *pOldFocusItem = m_pFocusItem;

    /* If there is item: */
    if (pItem)
    {
        /* Set this item to focus if navigation list contains it: */
        if (navigationList().contains(pItem))
            m_pFocusItem = pItem;
        /* Otherwise it's error: */
        else
            AssertMsgFailed(("Passed item is not in navigation list!"));
    }
    /* Otherwise reset focus item: */
    else
        m_pFocusItem = 0;

    /* Disconnect old focus-item (if any): */
    if (pOldFocusItem)
        disconnect(pOldFocusItem, SIGNAL(destroyed(QObject*)), this, SLOT(sltFocusItemDestroyed()));
    /* Connect new focus-item (if any): */
    if (m_pFocusItem)
        connect(m_pFocusItem, SIGNAL(destroyed(QObject*)), this, SLOT(sltFocusItemDestroyed()));

    /* Notify about focus change: */
    emit sigFocusChanged();
}

UIToolsItem *UIToolsModel::focusItem() const
{
    return m_pFocusItem;
}

void UIToolsModel::makeSureSomeItemIsSelected()
{
    /* Make sure selection item list is never empty
     * if at least one item (for example 'focus') present: */
    if (!currentItem() && focusItem())
        setCurrentItem(focusItem());
}

const QList<UIToolsItem*> &UIToolsModel::navigationList() const
{
    return m_navigationList;
}

void UIToolsModel::removeFromNavigationList(UIToolsItem *pItem)
{
    AssertMsg(pItem, ("Passed item is invalid!"));
    m_navigationList.removeAll(pItem);
}

void UIToolsModel::updateNavigation()
{
    /* Clear list initially: */
    m_navigationList.clear();

    /* Enumerate the children: */
    foreach (UIToolsItem *pItem, items())
        if (pItem->isVisible())
            m_navigationList << pItem;
}

QList<UIToolsItem*> UIToolsModel::items() const
{
    return m_items;
}

void UIToolsModel::updateLayout()
{
    /* Initialize variables: */
    const QSize viewportSize = scene()->views()[0]->viewport()->size();
    const int iViewportWidth = viewportSize.width();
    int iVerticalIndent = 0;

    /* Layout the children: */
    foreach (UIToolsItem *pItem, items())
    {
        /* Hide/skip unrelated items: */
        if (pItem->itemClass() != m_enmCurrentClass)
        {
            pItem->hide();
            continue;
        }

        /* Set item position: */
        pItem->setPos(0, iVerticalIndent);
        /* Set root-item size: */
        pItem->resize(iViewportWidth, pItem->minimumHeightHint());
        /* Make sure item is shown: */
        pItem->show();
        /* Advance vertical indent: */
        iVerticalIndent += pItem->minimumHeightHint();
    }
}

void UIToolsModel::sltHandleViewResized()
{
    /* Relayout: */
    updateLayout();
}

void UIToolsModel::sltItemMinimumWidthHintChanged()
{
    UIToolsItem *pSender = qobject_cast<UIToolsItem*>(sender());
    AssertPtrReturnVoid(pSender);
    /// @todo handle!
}

void UIToolsModel::sltItemMinimumHeightHintChanged()
{
    UIToolsItem *pSender = qobject_cast<UIToolsItem*>(sender());
    AssertPtrReturnVoid(pSender);
    /// @todo handle!
}

bool UIToolsModel::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    /* Process only scene events: */
    if (pWatched != scene())
        return QObject::eventFilter(pWatched, pEvent);

    /* Process only item focused by model: */
    if (scene()->focusItem())
        return QObject::eventFilter(pWatched, pEvent);

    /* Checking event-type: */
    switch (pEvent->type())
    {
        /* Keyboard handler: */
        case QEvent::KeyPress:
            return m_pKeyboardHandler->handle(static_cast<QKeyEvent*>(pEvent), UIKeyboardEventType_Press);
        case QEvent::KeyRelease:
            return m_pKeyboardHandler->handle(static_cast<QKeyEvent*>(pEvent), UIKeyboardEventType_Release);
        /* Mouse handler: */
        case QEvent::GraphicsSceneMousePress:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_Press);
        case QEvent::GraphicsSceneMouseRelease:
            return m_pMouseHandler->handle(static_cast<QGraphicsSceneMouseEvent*>(pEvent), UIMouseEventType_Release);
        /* Shut up MSC: */
        default: break;
    }

    /* Call to base-class: */
    return QObject::eventFilter(pWatched, pEvent);
}

void UIToolsModel::sltFocusItemDestroyed()
{
    AssertMsgFailed(("Focus item destroyed!"));
}

void UIToolsModel::prepare()
{
    /* Prepare scene: */
    prepareScene();
    /* Prepare items: */
    prepareItems();
    /* Prepare handlers: */
    prepareHandlers();
    /* Prepare connections: */
    prepareConnections();
}

void UIToolsModel::prepareScene()
{
    m_pScene = new QGraphicsScene(this);
    if (m_pScene)
        m_pScene->installEventFilter(this);
}

void UIToolsModel::prepareItems()
{
    /* Prepare classes: */
    QList<UIToolsClass> classes;
    classes << UIToolsClass_Global;
    classes << UIToolsClass_Global;
    classes << UIToolsClass_Machine;
    classes << UIToolsClass_Machine;

    /* Prepare types: */
    QList<UIToolsType> types;
    types << UIToolsType_Media;
    types << UIToolsType_Network;
    types << UIToolsType_Details;
    types << UIToolsType_Snapshots;

    /* Prepare icons: */
    QList<QIcon> icons;
    icons << UIIconPool::iconSet(":/media_manager_22px.png");
    icons << UIIconPool::iconSet(":/host_iface_manager_22px.png");
    icons << UIIconPool::iconSet(":/machine_details_manager_22px.png");
    icons << UIIconPool::iconSet(":/snapshot_manager_22px.png");

    /* Prepare names: */
    QList<QString> names;
    names << tr("Media");
    names << tr("Network");
    names << tr("Details");
    names << tr("Snapshots");

    /* Populate the items: */
    for (int i = 0; i < names.size(); ++i)
        m_items << new UIToolsItem(scene(),
                                   classes.value(i),
                                   types.value(i),
                                   icons.value(i),
                                   names.value(i));
}

void UIToolsModel::prepareHandlers()
{
    m_pMouseHandler = new UIToolsHandlerMouse(this);
    m_pKeyboardHandler = new UIToolsHandlerKeyboard(this);
}

void UIToolsModel::prepareConnections()
{
    /* Setup parent connections: */
    connect(this, SIGNAL(sigSelectionChanged()),
            parent(), SIGNAL(sigSelectionChanged()));
    connect(this, SIGNAL(sigExpandingStarted()),
            parent(), SIGNAL(sigExpandingStarted()));
    connect(this, SIGNAL(sigExpandingFinished()),
            parent(), SIGNAL(sigExpandingFinished()));
}

void UIToolsModel::loadLastSelectedItem()
{
//    /* Load last selected item (choose first if unable to load): */
//    setCurrentItem(gEDataManager->toolsPaneLastItemChosen());
    if (!currentItem() && !navigationList().isEmpty())
        setCurrentItem(navigationList().first());
}

void UIToolsModel::saveLastSelectedItem()
{
//    /* Save last selected item: */
//    gEDataManager->setToolsPaneLastItemChosen(currentItem() ? currentItem()->definition() : QString());
}

void UIToolsModel::cleanupHandlers()
{
    delete m_pKeyboardHandler;
    m_pKeyboardHandler = 0;
    delete m_pMouseHandler;
    m_pMouseHandler = 0;
}

void UIToolsModel::cleanupItems()
{
    foreach (UIToolsItem *pItem, m_items)
        delete pItem;
    m_items.clear();
}

void UIToolsModel::cleanupScene()
{
    delete m_pScene;
    m_pScene = 0;
}

void UIToolsModel::cleanup()
{
    /* Cleanup handlers: */
    cleanupHandlers();
    /* Cleanup items: */
    cleanupItems();
    /* Cleanup scene: */
    cleanupScene();
}

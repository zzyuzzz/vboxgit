/* $Id: UICloudProfileManager.cpp 74853 2018-10-15 18:36:00Z vboxsync $ */
/** @file
 * VBox Qt GUI - UICloudProfileManager class implementation.
 */

/*
 * Copyright (C) 2009-2018 Oracle Corporation
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
# include <QPushButton>
# include <QVBoxLayout>

/* GUI includes: */
# include "QIDialogButtonBox.h"
# include "QITreeWidget.h"
# include "VBoxGlobal.h"
# include "UIActionPoolSelector.h"
# include "UIExtraDataManager.h"
# include "UIIconPool.h"
# include "UICloudProfileDetailsWidget.h"
# include "UICloudProfileManager.h"
# include "UIToolBar.h"
# ifdef VBOX_WS_MAC
#  include "UIWindowMenuManager.h"
# endif

/* COM includes: */
#include "CCloudProfile.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


/** Tree-widget column tags. */
enum
{
    Column_Name,
    /// @todo rest of columns?
    Column_Max,
};


/** Cloud Profile Manager tree-widget item. */
class UIItemCloudProfile : public QITreeWidgetItem, public UIDataCloudProfile
{
public:

    /** Updates item fields from data. */
    void updateFields();

    /** Returns item name. */
    QString name() const { return m_strName; }
};


/*********************************************************************************************************************************
*   Class UIItemCloudProfile implementation.                                                                                     *
*********************************************************************************************************************************/

void UIItemCloudProfile::updateFields()
{
    /* Compose item fields: */
    setText(Column_Name, m_strName);
    /// @todo assign rest of field values!

    /* Compose item tool-tip: */
    /// @todo assign tool-tips!
}


/*********************************************************************************************************************************
*   Class UICloudProfileManagerWidget implementation.                                                                            *
*********************************************************************************************************************************/

UICloudProfileManagerWidget::UICloudProfileManagerWidget(EmbedTo enmEmbedding, UIActionPool *pActionPool,
                                                         bool fShowToolbar /* = true */, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_pActionPool(pActionPool)
    , m_fShowToolbar(fShowToolbar)
    , m_pToolBar(0)
    , m_pTreeWidget(0)
    , m_pDetailsWidget(0)
{
    /* Prepare: */
    prepare();
}

QMenu *UICloudProfileManagerWidget::menu() const
{
    /// @todo implement menu action!
    return 0;
}

void UICloudProfileManagerWidget::retranslateUi()
{
    /* Adjust toolbar: */
#ifdef VBOX_WS_MAC
    // WORKAROUND:
    // There is a bug in Qt Cocoa which result in showing a "more arrow" when
    // the necessary size of the toolbar is increased. Also for some languages
    // the with doesn't match if the text increase. So manually adjust the size
    // after changing the text.
    if (m_pToolBar)
        m_pToolBar->updateLayout();
#endif

    /* Translate tree-widget: */
    /// @todo assign tree-widget column names!
}

void UICloudProfileManagerWidget::resizeEvent(QResizeEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::resizeEvent(pEvent);

    /* Adjust tree-widget: */
    sltAdjustTreeWidget();
}

void UICloudProfileManagerWidget::showEvent(QShowEvent *pEvent)
{
    /* Call to base-class: */
    QIWithRetranslateUI<QWidget>::showEvent(pEvent);

    /* Adjust tree-widget: */
    sltAdjustTreeWidget();
}

void UICloudProfileManagerWidget::sltResetCloudProfileDetailsChanges()
{
    /* Just push the current item data there again: */
    sltHandleCurrentItemChange();
}

void UICloudProfileManagerWidget::sltApplyCloudProfileDetailsChanges()
{
    /* Get profile item: */
    UIItemCloudProfile *pItem = static_cast<UIItemCloudProfile*>(m_pTreeWidget->currentItem());
    AssertMsgReturnVoid(pItem, ("Current item must not be null!\n"));

    /// @todo apply cloud profile details!
}

void UICloudProfileManagerWidget::sltCreateCloudProfile()
{
    /// @todo create cloud profile!
}

void UICloudProfileManagerWidget::sltRemoveCloudProfile()
{
    /// @todo remove cloud profile!
}

void UICloudProfileManagerWidget::sltToggleCloudProfileDetailsVisibility(bool fVisible)
{
    /* Save the setting: */
    /// @todo implement extra-data setter!
    /* Show/hide details area and Apply button: */
    m_pDetailsWidget->setVisible(fVisible);
    /* Notify external lsiteners: */
    emit sigCloudProfileDetailsVisibilityChanged(fVisible);
}

void UICloudProfileManagerWidget::sltRefreshCloudProfiles()
{
    // Not implemented.
    AssertMsgFailed(("Not implemented!"));
}

void UICloudProfileManagerWidget::sltAdjustTreeWidget()
{
    /* Calculate the total tree-widget width: */
    const int iTotal = m_pTreeWidget->viewport()->width();

    /// @todo calculate proposed column widths!

    /* Apply the proposal: */
    m_pTreeWidget->setColumnWidth(Column_Name, iTotal /*- iWidth1 - iWidth2 - iWidth3*/);
}

void UICloudProfileManagerWidget::sltHandleItemChange(QTreeWidgetItem *pItem)
{
    /* Get profile item: */
    UIItemCloudProfile *pChangedItem = static_cast<UIItemCloudProfile*>(pItem);
    AssertMsgReturnVoid(pChangedItem, ("Changed item must not be null!\n"));

    /// @todo handle item change!
}

void UICloudProfileManagerWidget::sltHandleCurrentItemChange()
{
    /* Get profile item: */
    UIItemCloudProfile *pItem = static_cast<UIItemCloudProfile*>(m_pTreeWidget->currentItem());

    /* Update actions availability: */
    /// @todo implement % enable/disable actions!

    /* If there is an item => update details data: */
    if (pItem)
        m_pDetailsWidget->setData(*pItem);
    else
    {
        /* Otherwise => clear details and close the area: */
        m_pDetailsWidget->setData(UIDataCloudProfile());
        sltToggleCloudProfileDetailsVisibility(false);
    }
}

void UICloudProfileManagerWidget::sltHandleContextMenuRequest(const QPoint &position)
{
    /* Compose temporary context-menu: */
    QMenu menu;
    /// @todo implement & insert actions!

    /* And show it: */
    menu.exec(m_pTreeWidget->mapToGlobal(position));
}

void UICloudProfileManagerWidget::prepare()
{
    /* Prepare actions: */
    prepareActions();
    /* Prepare widgets: */
    prepareWidgets();

    /* Load settings: */
    loadSettings();

    /* Apply language settings: */
    retranslateUi();

    /* Load cloud profiles: */
    loadCloudProfiles();
}

void UICloudProfileManagerWidget::prepareActions()
{
    /* Connect actions: */
    /// @todo implement & connect actions!
}

void UICloudProfileManagerWidget::prepareWidgets()
{
    /* Create main-layout: */
    new QVBoxLayout(this);
    if (layout())
    {
        /* Configure layout: */
        layout()->setContentsMargins(0, 0, 0, 0);
#ifdef VBOX_WS_MAC
        layout()->setSpacing(10);
#else
        layout()->setSpacing(qApp->style()->pixelMetric(QStyle::PM_LayoutVerticalSpacing) / 2);
#endif

        /* Prepare toolbar, if requested: */
        if (m_fShowToolbar)
            prepareToolBar();
        /* Prepare tree-widget: */
        prepareTreeWidget();
        /* Prepare details-widget: */
        prepareDetailsWidget();
    }
}

void UICloudProfileManagerWidget::prepareToolBar()
{
    /* Create toolbar: */
    m_pToolBar = new UIToolBar(parentWidget());
    if (m_pToolBar)
    {
        /* Configure toolbar: */
        const int iIconMetric = (int)(QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) * 1.375);
        m_pToolBar->setIconSize(QSize(iIconMetric, iIconMetric));
        m_pToolBar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

        /* Add toolbar actions: */
        /// @todo implement & add actions!

#ifdef VBOX_WS_MAC
        /* Check whether we are embedded into a stack: */
        if (m_enmEmbedding == EmbedTo_Stack)
        {
            /* Add into layout: */
            layout()->addWidget(m_pToolBar);
        }
#else
        /* Add into layout: */
        layout()->addWidget(m_pToolBar);
#endif
    }
}

void UICloudProfileManagerWidget::prepareTreeWidget()
{
    /* Create tree-widget: */
    m_pTreeWidget = new QITreeWidget;
    if (m_pTreeWidget)
    {
        /* Configure tree-widget: */
        m_pTreeWidget->setRootIsDecorated(false);
        m_pTreeWidget->setAlternatingRowColors(true);
        m_pTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        m_pTreeWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        m_pTreeWidget->setColumnCount(Column_Max);
        m_pTreeWidget->setSortingEnabled(true);
        m_pTreeWidget->sortByColumn(Column_Name, Qt::AscendingOrder);
        m_pTreeWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
        connect(m_pTreeWidget, &QITreeWidget::currentItemChanged,
                this, &UICloudProfileManagerWidget::sltHandleCurrentItemChange);
        connect(m_pTreeWidget, &QITreeWidget::customContextMenuRequested,
                this, &UICloudProfileManagerWidget::sltHandleContextMenuRequest);
        connect(m_pTreeWidget, &QITreeWidget::itemChanged,
                this, &UICloudProfileManagerWidget::sltHandleItemChange);
        /// @todo implement & connect actions!

        /* Add into layout: */
        layout()->addWidget(m_pTreeWidget);
    }
}

void UICloudProfileManagerWidget::prepareDetailsWidget()
{
    /* Create details-widget: */
    m_pDetailsWidget = new UICloudProfileDetailsWidget(m_enmEmbedding);
    if (m_pDetailsWidget)
    {
        /* Configure details-widget: */
        m_pDetailsWidget->setVisible(false);
        m_pDetailsWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        connect(m_pDetailsWidget, &UICloudProfileDetailsWidget::sigDataChanged,
                this, &UICloudProfileManagerWidget::sigCloudProfileDetailsDataChanged);
        connect(m_pDetailsWidget, &UICloudProfileDetailsWidget::sigDataChangeRejected,
                this, &UICloudProfileManagerWidget::sltResetCloudProfileDetailsChanges);
        connect(m_pDetailsWidget, &UICloudProfileDetailsWidget::sigDataChangeAccepted,
                this, &UICloudProfileManagerWidget::sltApplyCloudProfileDetailsChanges);

        /* Add into layout: */
        layout()->addWidget(m_pDetailsWidget);
    }
}

void UICloudProfileManagerWidget::loadSettings()
{
    /* Details action/widget: */
    /// @todo implement extra-data getter!
}

void UICloudProfileManagerWidget::loadCloudProfiles()
{
    /* Clear tree first of all: */
    m_pTreeWidget->clear();

    /// @todo load cloud profiles!
}

void UICloudProfileManagerWidget::loadCloudProfile(const CCloudProfile &comProfile, UIDataCloudProfile &data)
{
    Q_UNUSED(comProfile);
    Q_UNUSED(data);

    /// @todo load cloud profile!
}

void UICloudProfileManagerWidget::createItemForCloudProfile(const UIDataCloudProfile &data, bool fChooseItem)
{
    /* Create new item: */
    UIItemCloudProfile *pItem = new UIItemCloudProfile;
    if (pItem)
    {
        /* Configure item: */
        pItem->UIDataCloudProfile::operator=(data);
        pItem->updateFields();
        /* Add item to the tree: */
        m_pTreeWidget->addTopLevelItem(pItem);
        /* And choose it as current if necessary: */
        if (fChooseItem)
            m_pTreeWidget->setCurrentItem(pItem);
    }
}

void UICloudProfileManagerWidget::updateItemForCloudProfile(const UIDataCloudProfile &data, bool fChooseItem, UIItemCloudProfile *pItem)
{
    /* Update passed item: */
    if (pItem)
    {
        /* Configure item: */
        pItem->UIDataCloudProfile::operator=(data);
        pItem->updateFields();
        /* And choose it as current if necessary: */
        if (fChooseItem)
            m_pTreeWidget->setCurrentItem(pItem);
    }
}


/*********************************************************************************************************************************
*   Class UICloudProfileManagerFactory implementation.                                                                            *
*********************************************************************************************************************************/

UICloudProfileManagerFactory::UICloudProfileManagerFactory(UIActionPool *pActionPool /* = 0 */)
    : m_pActionPool(pActionPool)
{
}

void UICloudProfileManagerFactory::create(QIManagerDialog *&pDialog, QWidget *pCenterWidget)
{
    pDialog = new UICloudProfileManager(pCenterWidget, m_pActionPool);
}


/*********************************************************************************************************************************
*   Class UICloudProfileManager implementation.                                                                                   *
*********************************************************************************************************************************/

UICloudProfileManager::UICloudProfileManager(QWidget *pCenterWidget, UIActionPool *pActionPool)
    : QIWithRetranslateUI<QIManagerDialog>(pCenterWidget)
    , m_pActionPool(pActionPool)
{
}

void UICloudProfileManager::sltHandleButtonBoxClick(QAbstractButton *pButton)
{
    /* Disable buttons first of all: */
    button(ButtonType_Reset)->setEnabled(false);
    button(ButtonType_Apply)->setEnabled(false);

    /* Compare with known buttons: */
    if (pButton == button(ButtonType_Reset))
        emit sigDataChangeRejected();
    else
    if (pButton == button(ButtonType_Apply))
        emit sigDataChangeAccepted();
}

void UICloudProfileManager::retranslateUi()
{
    /* Translate window title: */
    setWindowTitle(tr("Cloud Profile Manager"));

    /* Translate buttons: */
    button(ButtonType_Reset)->setText(tr("Reset"));
    button(ButtonType_Apply)->setText(tr("Apply"));
    button(ButtonType_Close)->setText(tr("Close"));
    button(ButtonType_Reset)->setStatusTip(tr("Reset changes in current cloud profile details"));
    button(ButtonType_Apply)->setStatusTip(tr("Apply changes in current cloud profile details"));
    button(ButtonType_Close)->setStatusTip(tr("Close dialog without saving"));
    button(ButtonType_Reset)->setShortcut(QString("Ctrl+Backspace"));
    button(ButtonType_Apply)->setShortcut(QString("Ctrl+Return"));
    button(ButtonType_Close)->setShortcut(Qt::Key_Escape);
    button(ButtonType_Reset)->setToolTip(tr("Reset Changes (%1)").arg(button(ButtonType_Reset)->shortcut().toString()));
    button(ButtonType_Apply)->setToolTip(tr("Apply Changes (%1)").arg(button(ButtonType_Apply)->shortcut().toString()));
    button(ButtonType_Close)->setToolTip(tr("Close Window (%1)").arg(button(ButtonType_Close)->shortcut().toString()));
}

void UICloudProfileManager::configure()
{
    /* Apply window icons: */
    /// @todo apply proper cloud profile manager icons!
    setWindowIcon(UIIconPool::iconSetFull(":/host_iface_manager_32px.png", ":/host_iface_manager_16px.png"));
}

void UICloudProfileManager::configureCentralWidget()
{
    /* Create widget: */
    UICloudProfileManagerWidget *pWidget = new UICloudProfileManagerWidget(EmbedTo_Dialog, m_pActionPool, true, this);
    if (pWidget)
    {
        /* Configure widget: */
        setWidget(pWidget);
        setWidgetMenu(pWidget->menu());
#ifdef VBOX_WS_MAC
        setWidgetToolbar(pWidget->toolbar());
#endif
        connect(this, &UICloudProfileManager::sigDataChangeRejected,
                pWidget, &UICloudProfileManagerWidget::sltResetCloudProfileDetailsChanges);
        connect(this, &UICloudProfileManager::sigDataChangeAccepted,
                pWidget, &UICloudProfileManagerWidget::sltApplyCloudProfileDetailsChanges);

        /* Add into layout: */
        centralWidget()->layout()->addWidget(pWidget);
    }
}

void UICloudProfileManager::configureButtonBox()
{
    /* Configure button-box: */
    connect(widget(), &UICloudProfileManagerWidget::sigCloudProfileDetailsVisibilityChanged,
            button(ButtonType_Apply), &QPushButton::setVisible);
    connect(widget(), &UICloudProfileManagerWidget::sigCloudProfileDetailsVisibilityChanged,
            button(ButtonType_Reset), &QPushButton::setVisible);
    connect(widget(), &UICloudProfileManagerWidget::sigCloudProfileDetailsDataChanged,
            button(ButtonType_Apply), &QPushButton::setEnabled);
    connect(widget(), &UICloudProfileManagerWidget::sigCloudProfileDetailsDataChanged,
            button(ButtonType_Reset), &QPushButton::setEnabled);
    connect(buttonBox(), &QIDialogButtonBox::clicked,
            this, &UICloudProfileManager::sltHandleButtonBoxClick);
    // WORKAROUND:
    // Since we connected signals later than extra-data loaded
    // for signals above, we should handle that stuff here again:
    /// @todo implement extra-data getter!
}

void UICloudProfileManager::finalize()
{
    /* Apply language settings: */
    retranslateUi();
}

UICloudProfileManagerWidget *UICloudProfileManager::widget()
{
    return qobject_cast<UICloudProfileManagerWidget*>(QIManagerDialog::widget());
}

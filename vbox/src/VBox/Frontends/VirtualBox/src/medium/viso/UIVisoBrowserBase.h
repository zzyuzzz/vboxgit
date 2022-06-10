/* $Id: UIVisoBrowserBase.h 76730 2019-01-09 11:09:17Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVisoBrowserBase class declaration.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoBrowserBase_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoBrowserBase_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QModelIndex>
#include <QWidget>

/* Forward declarations: */
class QItemSelection;
class QGridLayout;
class QLabel;
class QSplitter;
class QVBoxLayout;
class QTableView;
class QTreeView;
class UIToolBar;

class UIVisoBrowserBase : public QWidget
{
    Q_OBJECT;

public:

    UIVisoBrowserBase(QWidget *pParent = 0);
    ~UIVisoBrowserBase();
    virtual void showHideHiddenObjects(bool bShow) = 0;

protected:

    void prepareObjects();
    void prepareConnections();

    virtual void tableViewItemDoubleClick(const QModelIndex &index) = 0;
    virtual void treeSelectionChanged(const QModelIndex &selectedTreeIndex) = 0;
    virtual void setTableRootIndex(QModelIndex index = QModelIndex()) = 0;
    virtual void setTreeCurrentIndex(QModelIndex index = QModelIndex()) = 0;


    QTreeView          *m_pTreeView;
    QTableView         *m_pTableView;
    QLabel             *m_pTitleLabel;
    QWidget            *m_pRightContainerWidget;
    QGridLayout        *m_pRightContainerLayout;
    UIToolBar          *m_pVerticalToolBar;

private:
    QGridLayout    *m_pMainLayout;
    QSplitter      *m_pHorizontalSplitter;

private slots:
    void sltHandleTableViewItemDoubleClick(const QModelIndex &index);
    void sltHandleTreeSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    void sltHandleTreeItemClicked(const QModelIndex &modelIndex);

private:

};

#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoBrowserBase_h */

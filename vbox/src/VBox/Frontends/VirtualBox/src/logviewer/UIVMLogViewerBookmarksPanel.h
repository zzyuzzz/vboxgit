/* $Id: UIVMLogViewerBookmarksPanel.h 70617 2018-01-18 07:50:33Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVMLogViewer class declaration.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIVMLogViewerBookmarksPanel_h___
#define ___UIVMLogViewerBookmarksPanel_h___

/* GUI includes: */
#include "UIVMLogViewerPanel.h"

/* Forward declarations: */
class QComboBox;
class QIToolButton;
class UIRoundRectSegmentedButton;

/** UIVMLogViewerPanel extension providing GUI for bookmark management. Show a list of bookmarks currently set
  for displayed log page. It has controls to navigate and clear bookmarks. */
class UIVMLogViewerBookmarksPanel : public UIVMLogViewerPanel
{
    Q_OBJECT;

signals:

    void sigDeleteBookmark(int bookmarkIndex);
    void sigDeleteAllBookmarks();
    void sigBookmarkSelected(int index);

public:

    UIVMLogViewerBookmarksPanel(QWidget *pParent, UIVMLogViewerWidget *pViewer);

    /* Adds a single bookmark to an existing list of bookmarks. Possibly called
       by UIVMLogViewerWidget when user adds a bookmark thru context menu etc. */
    void addBookmark(const QPair<int, QString> &newBookmark);
    /* Clear the bookmark list and show this list instead. Probably done after
       user switches to another log page tab etc. */
    void setBookmarksList(const QVector<QPair<int, QString> > &bookmarkList);
    void updateBookmarkList(const QVector<QPair<int, QString> > &bookmarkVector);

public slots:

protected:

    virtual void prepareWidgets() /* override */;
    virtual void prepareConnections() /* override */;

    /** Handles the translation event. */
    void retranslateUi();

private slots:

    void sltDeleteCurrentBookmark();
    void sltBookmarkSelected(int index);
    void sltGotoNextPreviousBookmark(int buttonIndex);
    void sltGotoSelectedBookmark();

private:

    /* @a index is the index of the curent bookmark. */
    void setBookmarkIndex(int index);

    const int     m_iMaxBookmarkTextLength;
    QComboBox    *m_pBookmarksComboBox;
    QIToolButton *m_pGotoSelectedBookmark;
    QIToolButton *m_pDeleteAllButton;
    QIToolButton *m_pDeleteCurrentButton;
    UIRoundRectSegmentedButton *m_pNextPrevButtons;
};

#endif /* !___UIVMLogViewerBookmarksPanel_h___ */

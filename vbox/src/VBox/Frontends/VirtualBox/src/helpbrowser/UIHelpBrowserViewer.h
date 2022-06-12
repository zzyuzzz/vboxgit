/* $Id: UIHelpBrowserViewer.h 87157 2021-01-04 10:59:51Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIHelpBrowserWidget class declaration.
 */

/*
 * Copyright (C) 2010-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_helpbrowser_UIHelpBrowserViewer_h
#define FEQT_INCLUDED_SRC_helpbrowser_UIHelpBrowserViewer_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */

#include <QKeySequence>
#include <QPair>
#include <QTextBrowser>
#include <QWidget>

/* GUI includes: */
#include "QIManagerDialog.h"
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QHBoxLayout;
class QItemSelection;
class QVBoxLayout;
class QHelpEngine;
class QHelpContentModel;
class QHelpContentWidget;
class QHelpIndexWidget;
class QHelpSearchEngine;
class QHelpSearchQueryWidget;
class QHelpSearchResultWidget;
class QListWidget;
class QPlainTextEdit;
class QSplitter;
class QITabWidget;
class QIToolBar;
class UIActionPool;
class UIBookmarksListContainer;
class UIDialogPanel;
class UIFindInPageWidget;
class UIHelpBrowserTabManager;

#if defined(RT_OS_LINUX) && defined(VBOX_WITH_DOCS_QHELP) && (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
class UIHelpBrowserViewer : public QIWithRetranslateUI<QTextBrowser>
{
    Q_OBJECT;

signals:

    void sigOpenLinkInNewTab(const QUrl &url);
    void sigCloseFindInPageWidget();
    void sigFontPointSizeChanged(int iFontPointSize);
    void sigGoBackward();
    void sigGoForward();
    void sigGoHome();
    void sigAddBookmark();

public:

    UIHelpBrowserViewer(const QHelpEngine *pHelpEngine, QWidget *pParent = 0);
    virtual QVariant loadResource(int type, const QUrl &name) /* override */;
    void emitHistoryChangedSignal();
    void setSource(const QUrl &url) /* override */;
    void toggleFindInPageWidget(bool fVisible);
    int initialFontPointSize() const;
    void setFont(const QFont &);

public slots:


protected:

    virtual void contextMenuEvent(QContextMenuEvent *event) /* override */;
    virtual void resizeEvent(QResizeEvent *pEvent) /* override */;
    virtual void wheelEvent(QWheelEvent *pEvent) /* override */;

private slots:

    void sltHandleOpenLinkInNewTab();
    void sltHandleOpenLink();
    void sltHandleCopyLink();
    void sltHandleFindWidgetDrag(const QPoint &delta);
    void sltHandleFindInPageSearchTextChange(const QString &strSearchText);
    void sltSelectPreviousMatch();
    void sltSelectNextMatch();

private:

    void retranslateUi();
    bool isRectInside(const QRect &rect, int iMargin) const;
    void moveFindWidgetIn(int iMargin);
    void findAllMatches(const QString &searchString);
    void highlightFinds(int iSearchTermLength);
    void selectMatch(int iMatchIndex, int iSearchStringLength);
    const QHelpEngine* m_pHelpEngine;
    UIFindInPageWidget *m_pFindInPageWidget;
    /* Initilized as false and set to true once the user drag moves the find widget. */
    bool m_fFindWidgetDragged;
    const int m_iMarginForFindWidget;
    /** Document positions of the cursors within the document for all matches. */
    QVector<int>   m_matchedCursorPosition;
    int m_iSelectedMatchIndex;
    int m_iSearchTermLength;
    int m_iInitialFontPointSize;
};

#endif /* #if defined(RT_OS_LINUX) && defined(VBOX_WITH_DOCS_QHELP) && (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0)) */
#endif /* !FEQT_INCLUDED_SRC_helpbrowser_UIHelpBrowserViewer_h */

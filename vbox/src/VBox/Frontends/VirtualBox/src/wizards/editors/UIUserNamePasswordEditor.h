/* $Id: UIUserNamePasswordEditor.h 93990 2022-02-28 15:34:57Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIUserNamePasswordEditor class declaration.
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_editors_UIUserNamePasswordEditor_h
#define FEQT_INCLUDED_SRC_wizards_editors_UIUserNamePasswordEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QIcon>
#include <QLineEdit>
#include <QWidget>

/* Local includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QGridLayout;
class QLabel;
class QILineEdit;
class QIToolButton;
class UIPasswordLineEdit;

class SHARED_LIBRARY_STUFF UIPasswordLineEdit : public QLineEdit
{
    Q_OBJECT;

signals:

    void sigTextVisibilityToggled(bool fTextVisible);

public:

    UIPasswordLineEdit(QWidget *pParent = 0);
    void toggleTextVisibility(bool fTextVisible);
    void mark(bool fError, const QString &strErrorToolTip);

protected:

    virtual void resizeEvent(QResizeEvent *pEvent) RT_OVERRIDE;

private slots:

    void sltHandleTextVisibilityChange();

private:

    void prepare();
    void adjustTextVisibilityButtonGeometry();

    QIToolButton *m_pTextVisibilityButton;
    QIcon m_markIcon;
    QLabel *m_pErrorIconLabel;
    QString m_strErrorToolTip;
    /** When true the line edit is marked with some icon to indicate some error. */
    bool m_fMarkForError;
};

class SHARED_LIBRARY_STUFF UIUserNamePasswordEditor : public QIWithRetranslateUI<QWidget>
{

    Q_OBJECT;

signals:

    void sigUserNameChanged(const QString &strUserName);
    void sigPasswordChanged(const QString &strPassword);

public:

    UIUserNamePasswordEditor(QWidget *pParent = 0);

    QString userName() const;
    void setUserName(const QString &strUserName);

    QString password() const;
    void setPassword(const QString &strPassword);

    /** Returns false if username or password fields are empty, or password fields do not match. */
    bool isComplete();

    /** When fEnabled true place holder texts for the line edits are shown. */
    void setPlaceholderTextEnabled(bool fEnabled);
    void setLabelsVisible(bool fVisible);

protected:

    void retranslateUi();

private slots:

    void sltHandlePasswordVisibility(bool fPasswordVisible);
    void sltUserNameChanged();
    void sltPasswordChanged();

private:

    void prepare();
    template <class T>
    void addLineEdit(int &iRow, QLabel *&pLabel, T *&pLineEdit, QGridLayout *pLayout);

    bool isUserNameComplete();
    bool isPasswordComplete();

    QILineEdit         *m_pUserNameLineEdit;
    UIPasswordLineEdit *m_pPasswordLineEdit;
    UIPasswordLineEdit *m_pPasswordRepeatLineEdit;

    QLabel *m_pUserNameLabel;
    QLabel *m_pPasswordLabel;
    QLabel *m_pPasswordRepeatLabel;

    bool m_fShowPlaceholderText;
    bool m_fLabelsVisible;

    QString m_strPasswordError;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_editors_UIUserNamePasswordEditor_h */

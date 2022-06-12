/* $Id: UIUserNamePasswordEditor.h 84969 2020-06-26 13:56:07Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIUserNamePasswordEditor class declaration.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_widgets_UIUserNamePasswordEditor_h
#define FEQT_INCLUDED_SRC_widgets_UIUserNamePasswordEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QLineEdit>
#include <QWidget>

/* Local includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QGridLayout;
class QLabel;

class UIPasswordLineEdit : public QLineEdit
{
    Q_OBJECT;

signals:

    void sigTextVisibilityToggled(bool fTextVisible);

public:

    UIPasswordLineEdit(QWidget *pParent = 0);
    void toggleTextVisibility(bool fTextVisible);

protected:

    virtual void paintEvent(QPaintEvent *pevent) /* override */;

private:

    void prepare();

    QToolButton *m_pTextVisibilityButton;

private slots:

    void sltHandleTextVisibilityChange();
};

class UIUserNamePasswordEditor : public QIWithRetranslateUI<QWidget>
{

    Q_OBJECT;

signals:

    /** this is emitted whenever the content of one of the line edits is changed. */
    void sigSomeTextChanged();

public:

    UIUserNamePasswordEditor(QWidget *pParent = 0);

    QString userName() const;
    void setUserName(const QString &strUserName);

    QString password() const;
    void setPassword(const QString &strPassword);

    /** Returns false if username or password fields are empty, or password fields do not match. */
    bool isComplete();

protected:

    void retranslateUi();

private slots:

    void sltHandlePasswordVisibility(bool fPasswordVisible);

private:

    void prepare();
    template <class T>
    void addLineEdit(int &iRow, QLabel *&pLabel, T *&pLineEdit, QGridLayout *pLayout);
    /** Changes @p pLineEdit's base color to indicate an error or reverts it to the original color. */
    void markLineEdit(QLineEdit *pLineEdit, bool fError);

    QLineEdit          *m_pUserNameLineEdit;
    UIPasswordLineEdit *m_pPasswordLineEdit;
    UIPasswordLineEdit *m_pPasswordRepeatLineEdit;

    QLabel *m_pUserNameLabel;
    QLabel *m_pPasswordLabel;
    QLabel *m_pPasswordRepeatLabel;
    QColor m_orginalLineEditBaseColor;
};

#endif /* !FEQT_INCLUDED_SRC_widgets_UIUserNamePasswordEditor_h */
/* $Id: UIAutoCaptureKeyboardEditor.h 94395 2022-03-29 16:29:26Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIAutoCaptureKeyboardEditor class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_editors_UIAutoCaptureKeyboardEditor_h
#define FEQT_INCLUDED_SRC_settings_editors_UIAutoCaptureKeyboardEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QCheckBox;
class QLabel;

/** QWidget subclass used as an auto capture keyboard editor. */
class SHARED_LIBRARY_STUFF UIAutoCaptureKeyboardEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs editor passing @a pParent to the base-class. */
    UIAutoCaptureKeyboardEditor(QWidget *pParent = 0);

    /** Defines editor @a fValue. */
    void setValue(bool fValue);
    /** Returns editor value. */
    bool value() const;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the value to be set. */
    bool  m_fValue;

    /** Holds the label instance. */
    QLabel    *m_pLabel;
    /** Holds the check-box instance. */
    QCheckBox *m_pCheckBox;
};

#endif /* !FEQT_INCLUDED_SRC_settings_editors_UIAutoCaptureKeyboardEditor_h */

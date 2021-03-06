/* $Id: UIMachineDescriptionEditor.h 94445 2022-04-01 17:19:37Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIMachineDescriptionEditor class declaration.
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

#ifndef FEQT_INCLUDED_SRC_settings_editors_UIMachineDescriptionEditor_h
#define FEQT_INCLUDED_SRC_settings_editors_UIMachineDescriptionEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* Forward declarations: */
class QTextEdit;

/** QWidget subclass used as machine description editor. */
class SHARED_LIBRARY_STUFF UIMachineDescriptionEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs editor passing @a pParent to the base-class. */
    UIMachineDescriptionEditor(QWidget *pParent = 0);

    /** Defines editor @a strValue. */
    void setValue(const QString &strValue);
    /** Returns editor value. */
    QString value() const;

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private:

    /** Prepares all. */
    void prepare();

    /** Holds the value to be set. */
    QString  m_strValue;

    /** Holds the check-box instance. */
    QTextEdit *m_pTextEdit;
};

#endif /* !FEQT_INCLUDED_SRC_settings_editors_UIMachineDescriptionEditor_h */

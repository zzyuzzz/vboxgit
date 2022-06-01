/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * Declarations of utility classes and functions
 * VirtualBox Qt extensions: QIWithRetranslateUI class declaration
 */

/*
 * Copyright (C) 2008-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __QIWithRetranslateUI_h
#define __QIWithRetranslateUI_h

/* Global includes */
#include <QObject>
#include <QEvent>

template <class Base>
class QIWithRetranslateUI: public Base
{
public:

    QIWithRetranslateUI(QWidget *pParent = 0) : Base(pParent) {}

protected:

    virtual void changeEvent(QEvent *pEvent)
    {
        Base::changeEvent (pEvent);
        switch (pEvent->type())
        {
            case QEvent::LanguageChange:
            {
                retranslateUi();
                pEvent->accept();
                break;
            }
            default:
                break;
        }
    }

    virtual void retranslateUi() = 0;
};

template <class Base>
class QIWithRetranslateUI2: public Base
{
public:

    QIWithRetranslateUI2(QWidget *pParent = 0, Qt::WindowFlags fFlags = 0) : Base(pParent, fFlags) {}

protected:

    virtual void changeEvent(QEvent *pEvent)
    {
        Base::changeEvent (pEvent);
        switch (pEvent->type())
        {
            case QEvent::LanguageChange:
            {
                retranslateUi();
                pEvent->accept();
                break;
            }
            default:
                break;
        }
    }

    virtual void retranslateUi() = 0;
};

template <class Base>
class QIWithRetranslateUI3: public Base
{
public:

    QIWithRetranslateUI3(QObject *pParent = 0) : Base(pParent) {}

protected:

    virtual bool event(QEvent *pEvent)
    {
        bool bResult = Base::event(pEvent);
        switch (pEvent->type())
        {
            case QEvent::LanguageChange:
            {
                retranslateUi();
                pEvent->accept();
                bResult = true;
                break;
            }
            default:
                break;
        }
        return bResult;
    }

    virtual void retranslateUi() = 0;
};

#endif /* __QIWithRetranslateUI_h */


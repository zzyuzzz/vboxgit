/* $Id: QILineEdit.cpp 52730 2014-09-12 16:19:53Z vboxsync $ */
/** @file
 * VirtualBox Qt GUI - QILineEdit class implementation.
 */

/*
 * Copyright (C) 2008-2013 Oracle Corporation
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

# include "QILineEdit.h"

/* Qt includes */
# include <QStyleOptionFrame>

# ifdef Q_WS_WIN32
#  include <QWindowsVistaStyle>
#  include <QLibrary>

#  include <Windows.h>
#  include "iprt/ldr.h"
# endif

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


void QILineEdit::setMinimumWidthByText (const QString &aText)
{
    setMinimumWidth (featTextWidth (aText).width());
}

void QILineEdit::setFixedWidthByText (const QString &aText)
{
    setFixedWidth (featTextWidth (aText).width());
}

QSize QILineEdit::featTextWidth (const QString &aText) const
{
    QStyleOptionFrame sof;
    sof.initFrom (this);
    sof.rect = contentsRect();
    sof.lineWidth = hasFrame() ? style()->pixelMetric (QStyle::PM_DefaultFrameWidth) : 0;
    sof.midLineWidth = 0;
    sof.state |= QStyle::State_Sunken;

    /* The margins are based on qlineedit.cpp of Qt. Maybe they where changed
     * at some time in the future. */
    QSize sc (fontMetrics().width (aText) + 2*2,
              fontMetrics().xHeight()     + 2*1);
    QSize sa = style()->sizeFromContents (QStyle::CT_LineEdit, &sof, sc, this);

#if defined (Q_WS_WIN32)
    /* Vista l&f style has a bug where the last parameter of sizeFromContents
     * function ('widget' what corresponds to 'this' in our class) is ignored.
     * Due to it QLineEdit processed as QComboBox and size calculation includes
     * non-existing combo-box button of 23 pix in width. So fixing it here: */
    if (qobject_cast <QWindowsVistaStyle*> (style()))
    {
        /* Check if l&f style theme is really active else painting performed by
         * Windows Classic theme and there is no such shifting error. */
        typedef BOOL (WINAPI *PFNISAPPTHEMED)(VOID);
        static PFNISAPPTHEMED s_pfnIsAppThemed = (PFNISAPPTHEMED)~(uintptr_t)0;
        if (s_pfnIsAppThemed == (PFNISAPPTHEMED)~(uintptr_t)0 )
            s_pfnIsAppThemed = (PFNISAPPTHEMED)RTLdrGetSystemSymbol("uxtheme.dll", "IsAppThemed");

        if (s_pfnIsAppThemed && s_pfnIsAppThemed())
            sa -= QSize(23, 0);
    }
#endif

    return sa;
}


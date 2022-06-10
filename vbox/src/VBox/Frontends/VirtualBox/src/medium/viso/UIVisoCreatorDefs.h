/* $Id: UIVisoCreatorDefs.h 76671 2019-01-07 12:15:00Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIVisoCreatorDefs class declaration.
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

#ifndef FEQT_INCLUDED_SRC_medium_viso_UIVisoCreatorDefs_h
#define FEQT_INCLUDED_SRC_medium_viso_UIVisoCreatorDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


struct VisoOptions
{
    QString m_strVisoName = "ad-hoc-viso";
    bool operator==(const VisoOptions &otherOptions) const
    {
        return m_strVisoName == otherOptions.m_strVisoName;
    }
};

struct BrowserOptions
{
    bool m_bShowHiddenObjects = true;
    bool operator==(const BrowserOptions &otherOptions) const
    {
        return m_bShowHiddenObjects == otherOptions.m_bShowHiddenObjects;
    }
};



#endif /* !FEQT_INCLUDED_SRC_medium_viso_UIVisoCreatorDefs_h */

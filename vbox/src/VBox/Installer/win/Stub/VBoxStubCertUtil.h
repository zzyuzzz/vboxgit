/* $Id: VBoxStubCertUtil.h 76553 2019-01-01 01:45:53Z vboxsync $ */
/** @file
 * VBoxStub - VirtualBox's Windows installer stub (certificate manipulations).
 */

/*
 * Copyright (C) 2012-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_win_Stub_VBoxStubCertUtil_h
#define VBOX_INCLUDED_win_Stub_VBoxStubCertUtil_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

extern bool addCertToStore(DWORD dwDst, const char *pszStoreNm, const unsigned char kpCertBuf[], DWORD cbCertBuf);

#endif


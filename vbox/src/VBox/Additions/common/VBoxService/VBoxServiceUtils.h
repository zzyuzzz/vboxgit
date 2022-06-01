/* $Id: VBoxServiceUtils.h 26136 2010-02-01 17:45:40Z vboxsync $ */
/** @file
 * VBoxServiceUtils - Guest Additions Services (Utilities).
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___VBoxServiceUtils_h
#define ___VBoxServiceUtils_h

#include "VBoxServiceInternal.h"

#ifdef VBOX_WITH_GUEST_PROPS
int VBoxServiceReadProp(uint32_t u32ClientId, const char *pszPropName, char **ppszValue, char **ppszFlags, uint64_t *puTimestamp);
int VBoxServiceReadPropUInt32(uint32_t u32ClientId, const char *pszPropName, uint32_t *pu32, uint32_t u32Min, uint32_t u32Max);
int VBoxServiceWritePropF(uint32_t u32ClientId, const char *pszName, const char *pszValueFormat, ...);
#endif

#ifdef RT_OS_WINDOWS
int VBoxServiceGetFileVersionString(const char *pszPath, const char *pszFileName, char *pszVersion, size_t cbVersion);
#endif

#endif


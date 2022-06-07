/* $Id: bs3-cmn-StrCpy.c 58666 2015-11-12 00:04:31Z vboxsync $ */
/** @file
 * BS3Kit - Bs3StrCpy
 */

/*
 * Copyright (C) 2007-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#include "bs3kit-template-header.h"

#undef Bs3StrCpy
BS3_DECL(char BS3_FAR *) BS3_CMN_NM(Bs3StrCpy)(char BS3_FAR *pszDst, const char BS3_FAR *pszSrc)
{
    char BS3_FAR *pszRet = pszDst;
    char ch;
    do
    {
        ch = *pszSrc++;
        *pszDst++ = ch;
    } while (ch != '\0');
    return pszRet;
}


/* $Id: bs3-cmn-Printf.c 58812 2015-11-22 02:56:17Z vboxsync $ */
/** @file
 * BS3Kit - Bs3Printf, Bs3PrintfV
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
#include <iprt/ctype.h>


static BS3_DECL_CALLBACK(size_t) bs3PrintFmtOutput(char ch, void BS3_FAR *pvUser)
{
    if (ch != '\0')
    {
        if (ch == '\n')
            Bs3PrintChr('\r');
        Bs3PrintChr(ch);
        return 1;
    }
    NOREF(pvUser);
    return 0;
}


BS3_DECL(size_t) Bs3PrintfV(const char BS3_FAR *pszFormat, va_list va)
{
    return Bs3StrFormatV(pszFormat, va, bs3PrintFmtOutput, NULL);
}


BS3_DECL(size_t) Bs3Printf(const char BS3_FAR *pszFormat, ...)
{
    size_t cchRet;
    va_list va;
    va_start(va, pszFormat);
    cchRet = Bs3StrFormatV(pszFormat, va, bs3PrintFmtOutput, NULL);
    va_end(va);
    return cchRet;
}


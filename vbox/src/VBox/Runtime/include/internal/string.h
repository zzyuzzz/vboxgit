/* $Id: string.h 28903 2010-04-29 14:58:12Z vboxsync $ */
/** @file
 * IPRT - Internal RTStr header.
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
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

#ifndef ___internal_string_h
#define ___internal_string_h

#include <iprt/string.h>

RT_C_DECLS_BEGIN

/** @def RTSTR_STRICT
 * Enables strict assertions on bad string encodings.
 */
#ifdef DOXYGEN_RUNNING
# define RTSTR_STRICT
#endif
/*#define RTSTR_STRICT*/

#ifdef RTSTR_STRICT
# define RTStrAssertMsgFailed(msg)              AssertMsgFailed(msg)
# define RTStrAssertMsgReturn(expr, msg, rc)    AssertMsgReturn(expr, msg, rc)
#else
# define RTStrAssertMsgFailed(msg)              do { } while (0)
# define RTStrAssertMsgReturn(expr, msg, rc)    do { if (!(expr)) return rc; } while (0)
#endif

size_t rtstrFormatRt(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput, const char **ppszFormat, va_list *pArgs,
                     int cchWidth, int cchPrecision, unsigned fFlags, char chArgSize);
size_t rtstrFormatType(PFNRTSTROUTPUT pfnOutput, void *pvArgOutput, const char **ppszFormat, va_list *pArgs,
                       int cchWidth, int cchPrecision, unsigned fFlags, char chArgSize);

#ifdef RT_WITH_ICONV_CACHE
void rtStrIconvCacheInit(struct RTTHREADINT *pThread);
void rtStrIconvCacheDestroy(struct RTTHREADINT *pThread);
#endif

/**
 * Indexes into RTTHREADINT::ahIconvs
 */
typedef enum RTSTRICONV
{
    /** UTF-8 to the locale codeset (LC_CTYPE). */
    RTSTRICONV_UTF8_TO_LOCALE = 0,
    /** The locale codeset (LC_CTYPE) to UTF-8. */
    RTSTRICONV_LOCALE_TO_UTF8,
    /** UTF-8 to the filesystem codeset - if different from the locale codeset. */
    RTSTRICONV_UTF8_TO_FS,
    /** The filesystem codeset to UTF-8. */
    RTSTRICONV_FS_TO_UTF8,
    /** The end of the valid indexes. */
    RTSTRICONV_END
} RTSTRICONV;

int rtStrConvert(const char *pchInput, size_t cchInput, const char *pszInputCS,
                 char **ppszOutput, size_t cbOutput, const char *pszOutputCS,
                 unsigned cFactor, RTSTRICONV enmCacheIdx);
const char *rtStrGetLocaleCodeset(void);
int rtUtf8Length(const char *psz, size_t cch, size_t *pcuc, size_t *pcchActual);

RT_C_DECLS_END

#endif


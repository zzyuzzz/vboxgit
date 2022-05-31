/* $Id: RTPathAppend.cpp 21676 2009-07-17 12:21:49Z vboxsync $ */
/** @file
 * IPRT - RTPathAppend
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
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "internal/iprt.h"
#include <iprt/path.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/err.h>
#include <iprt/string.h>


/**
 * Figures the length of the root part of the path.
 *
 * @returns length of the root specifier.
 * @retval  0 if none.
 *
 * @param   pszPath         The path to investigate.
 *
 * @remarks Unnecessary root slashes will not be counted. The caller will have
 *          to deal with it where it matters.
 */
static size_t rtPathRootSpecLen(const char *pszPath)
{
    /* fend of wildlife. */
    if (!pszPath)
        return 0;

    /* Root slash? */
    if (RTPATH_IS_SLASH(pszPath[0]))
    {
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
        /* UNC? */
        if (    RTPATH_IS_SLASH(pszPath[1])
            &&  pszPath[2] != '\0'
            &&  !RTPATH_IS_SLASH(pszPath[2]))
        {
            /* Find the end of the server name. */
            const char *pszEnd = pszPath + 2;
            pszEnd += 2;
            while (   *pszEnd != '\0'
                   && !RTPATH_IS_SLASH(*pszEnd))
                pszEnd++;
            if (RTPATH_IS_SLASH(*pszEnd))
            {
                pszEnd++;
                while (RTPATH_IS_SLASH(*pszEnd))
                    pszEnd++;

                /* Find the end of the share name */
                while (   *pszEnd != '\0'
                       && !RTPATH_IS_SLASH(*pszEnd))
                    pszEnd++;
                if (RTPATH_IS_SLASH(*pszEnd))
                    pszEnd++;
                return pszPath - pszEnd;
            }
        }
#endif
        return 1;
    }

#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
    /* Drive specifier? */
    if (   pszPath[0] != '\0'
        && pszPath[1] == ':'
        && RT_C_IS_ALPHA(pszPath[0]))
    {
        if (RTPATH_IS_SLASH(pszPath[2]))
            return 3;
        return 2;
    }
#endif
    return 0;
}


RTDECL(int) RTPathAppend(char *pszPath, size_t cbPathDst, const char *pszAppend)
{
    char *pszPathEnd = (char *)memchr(pszPath, '\0', cbPathDst);
    AssertReturn(pszPathEnd, VERR_INVALID_PARAMETER);

    /*
     * Special cases.
     */
    if (!pszAppend)
        return VINF_SUCCESS;
    size_t cchAppend = strlen(pszAppend);
    if (!cchAppend)
        return VINF_SUCCESS;
    if (pszPathEnd == pszPath)
    {
        if (cchAppend >= cbPathDst)
            return VERR_BUFFER_OVERFLOW;
        memcpy(pszPath, pszAppend, cchAppend + 1);
        return VINF_SUCCESS;
    }

    /*
     * Balance slashes and check for buffer overflow.
     */
    bool fAddSlash = false;
    if (!RTPATH_IS_SLASH(pszPathEnd[-1]))
    {
        if (!RTPATH_IS_SLASH(pszAppend[0]))
        {
#if defined (RT_OS_OS2) || defined (RT_OS_WINDOWS)
            if (    (size_t)(pszPathEnd - pszPath) == 2
                &&  pszPath[1] == ':'
                &&  RT_C_IS_ALPHA(pszPath[0]))
            {
                if ((size_t)(pszPathEnd - pszPath) + cchAppend >= cbPathDst)
                    return VERR_BUFFER_OVERFLOW;
            }
            else
#endif
            {
                if ((size_t)(pszPathEnd - pszPath) + 1 + cchAppend >= cbPathDst)
                    return VERR_BUFFER_OVERFLOW;
                *pszPathEnd++ = '/';
            }
        }
        else
        {
            /* One slash is sufficient at this point. */
            while (RTPATH_IS_SLASH(pszAppend[1]))
                pszAppend++, cchAppend--;

            if ((size_t)(pszPathEnd - pszPath) + cchAppend >= cbPathDst)
                return VERR_BUFFER_OVERFLOW;
        }
    }
    else
    {
        /* No slashes needed in the appended bit. */
        while (RTPATH_IS_SLASH(*pszAppend))
            pszAppend++, cchAppend--;

        /* In the leading path we can skip unnecessary trailing slashes, but
           be sure to leave one. */
        size_t const cchRoot = rtPathRootSpecLen(pszPath);
        while (     (size_t)(pszPathEnd - pszPath) > RT_MAX(1, cchRoot)
               &&   RTPATH_IS_SLASH(pszPathEnd[-2]))
            pszPathEnd--;

        if ((size_t)(pszPathEnd - pszPath) + cchAppend >= cbPathDst)
            return VERR_BUFFER_OVERFLOW;
    }

    /*
     * What remains now is the just the copying.
     */
    memcpy(pszPathEnd, pszAppend, cchAppend + 1);
    return VINF_SUCCESS;
}


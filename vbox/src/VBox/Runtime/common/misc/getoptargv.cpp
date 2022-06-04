/* $Id: getoptargv.cpp 44528 2013-02-04 14:27:54Z vboxsync $ */
/** @file
 * IPRT - Command Line Parsing, Argument Vector.
 */

/*
 * Copyright (C) 2012 Oracle Corporation
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/getopt.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/mem.h>
#include <iprt/string.h>


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
/**
 * Array indexed by the quoting type and 7-bit ASCII character.
 *
 * We include some extra stuff here that the corresponding shell would normally
 * require quoting of.
 */
static uint8_t const g_abmQuoteChars[RTGETOPTARGV_CNV_QUOTE_MASK + 1][128/8] =
{
    { 0xfe, 0xff, 0x0f, 0x00, 0x65, 0x00, 0x00, 0x50 },
    { 0xfe, 0xff, 0x0f, 0x00, 0xd7, 0x07, 0x00, 0xd8 },
};


#if 0   /* To re-generate the bitmaps. */
#include <stdio.h>
int main()
{
    RT_ZERO(g_abmQuoteChars);

# define SET_ALL(ch) \
        do { \
            for (size_t iType = 0; iType <= RTGETOPTARGV_CNV_QUOTE_MASK; iType++) \
                ASMBitSet(&g_abmQuoteChars[iType], (ch)); \
        } while (0)
# define SET(ConstSuffix, ch) \
        ASMBitSet(&g_abmQuoteChars[RTGETOPTARGV_CNV_QUOTE_##ConstSuffix], (ch));

    /* just flag all the control chars as in need of quoting. */
    for (char ch = 1; ch < 0x20; ch++)
        SET_ALL(ch);

    /* ... and space of course */
    SET_ALL(' ');

    /* MS CRT / CMD.EXE: */
    SET(MS_CRT, '"')
    SET(MS_CRT, '&')
    SET(MS_CRT, '>')
    SET(MS_CRT, '<')
    SET(MS_CRT, '|')
    SET(MS_CRT, '%')

    /* Bourne shell: */
    SET(BOURNE_SH, '!');
    SET(BOURNE_SH, '"');
    SET(BOURNE_SH, '$');
    SET(BOURNE_SH, '&');
    SET(BOURNE_SH, '(');
    SET(BOURNE_SH, ')');
    SET(BOURNE_SH, '*');
    SET(BOURNE_SH, ';');
    SET(BOURNE_SH, '<');
    SET(BOURNE_SH, '>');
    SET(BOURNE_SH, '?');
    SET(BOURNE_SH, '[');
    SET(BOURNE_SH, '\'');
    SET(BOURNE_SH, '\\');
    SET(BOURNE_SH, '`');
    SET(BOURNE_SH, '|');
    SET(BOURNE_SH, '~');

    for (size_t iType = 0; iType <= RTGETOPTARGV_CNV_QUOTE_MASK; iType++)
    {
        printf("    {");
        for (size_t iByte = 0; iByte < 8; iByte++)
            printf(iByte == 0 ? " 0x%02x" : ", 0x%02x", g_abmQuoteChars[iType][iByte]);
        printf(" },\n");
    }
    return 0;
}
#endif /* To re-generate the bitmaps. */


/**
 * Look for an unicode code point in the separator string.
 *
 * @returns true if it's a separator, false if it isn't.
 * @param   Cp              The code point.
 * @param   pszSeparators   The separators.
 */
static bool rtGetOptIsUniCpInString(RTUNICP Cp, const char *pszSeparators)
{
    /* This could be done in a more optimal fashion.  Probably worth a
       separate RTStr function at some point. */
    for (;;)
    {
        RTUNICP CpSep;
        int rc = RTStrGetCpEx(&pszSeparators, &CpSep);
        AssertRCReturn(rc, false);
        if (CpSep == Cp)
            return true;
        if (!CpSep)
            return false;
    }
}


/**
 * Look for an 7-bit ASCII character in the separator string.
 *
 * @returns true if it's a separator, false if it isn't.
 * @param   ch              The character.
 * @param   pszSeparators   The separators.
 * @param   cchSeparators   The number of separators chars.
 */
DECLINLINE(bool) rtGetOptIsAsciiInSet(char ch, const char *pszSeparators, size_t cchSeparators)
{
    switch (cchSeparators)
    {
        case 8: if (ch == pszSeparators[7]) return true;
        case 7: if (ch == pszSeparators[6]) return true;
        case 6: if (ch == pszSeparators[5]) return true;
        case 5: if (ch == pszSeparators[4]) return true;
        case 4: if (ch == pszSeparators[3]) return true;
        case 3: if (ch == pszSeparators[2]) return true;
        case 2: if (ch == pszSeparators[1]) return true;
        case 1: if (ch == pszSeparators[0]) return true;
            return false;
        default:
            return memchr(pszSeparators, ch, cchSeparators) != NULL;
    }
}


/**
 * Checks if the character is in the set of separators
 *
 * @returns true if it is, false if it isn't.
 *
 * @param   Cp              The code point.
 * @param   pszSeparators   The separators.
 * @param   cchSeparators   The length of @a pszSeparators.
 */
DECL_FORCE_INLINE(bool) rtGetOptIsCpInSet(RTUNICP Cp, const char *pszSeparators, size_t cchSeparators)
{
    if (RT_LIKELY(Cp <= 127))
        return rtGetOptIsAsciiInSet((char)Cp, pszSeparators, cchSeparators);
    return rtGetOptIsUniCpInString(Cp, pszSeparators);
}


/**
 * Skips any delimiters at the start of the string that is pointed to.
 *
 * @returns VINF_SUCCESS or RTStrGetCpEx status code.
 * @param   ppszSrc         Where to get and return the string pointer.
 * @param   pszSeparators   The separators.
 * @param   cchSeparators   The length of @a pszSeparators.
 */
static int rtGetOptSkipDelimiters(const char **ppszSrc, const char *pszSeparators, size_t cchSeparators)
{
    const char *pszSrc = *ppszSrc;
    const char *pszRet;
    for (;;)
    {
        pszRet = pszSrc;
        RTUNICP Cp;
        int rc = RTStrGetCpEx(&pszSrc, &Cp);
        if (RT_FAILURE(rc))
        {
            *ppszSrc = pszRet;
            return rc;
        }
        if (   !Cp
            || !rtGetOptIsCpInSet(Cp, pszSeparators, cchSeparators))
            break;
    }

    *ppszSrc = pszRet;
    return VINF_SUCCESS;
}


RTDECL(int) RTGetOptArgvFromString(char ***ppapszArgv, int *pcArgs, const char *pszCmdLine, const char *pszSeparators)
{
    /*
     * Some input validation.
     */
    AssertPtr(pszCmdLine);
    AssertPtr(pcArgs);
    AssertPtr(ppapszArgv);
    if (!pszSeparators)
        pszSeparators = " \t\n\r";
    else
        AssertPtr(pszSeparators);
    size_t const cchSeparators = strlen(pszSeparators);
    AssertReturn(cchSeparators > 0, VERR_INVALID_PARAMETER);

    /*
     * Parse the command line and chop off it into argv individual argv strings.
     */
    int         rc        = VINF_SUCCESS;
    const char *pszSrc    = pszCmdLine;
    char       *pszDup    = (char *)RTMemAlloc(strlen(pszSrc) + 1);
    char       *pszDst    = pszDup;
    if (!pszDup)
        return VERR_NO_STR_MEMORY;
    char      **papszArgs = NULL;
    unsigned    iArg      = 0;
    while (*pszSrc)
    {
        /* Skip stuff */
        rc = rtGetOptSkipDelimiters(&pszSrc, pszSeparators, cchSeparators);
        if (RT_FAILURE(rc))
            break;
        if (!*pszSrc)
            break;

        /* Start a new entry. */
        if ((iArg % 32) == 0)
        {
            void *pvNew = RTMemRealloc(papszArgs, (iArg + 33) * sizeof(char *));
            if (!pvNew)
            {
                rc = VERR_NO_MEMORY;
                break;
            }
            papszArgs = (char **)pvNew;
        }
        papszArgs[iArg++] = pszDst;

        /* Parse and copy the string over. */
        RTUNICP CpQuote = 0;
        RTUNICP Cp;
        for (;;)
        {
            rc = RTStrGetCpEx(&pszSrc, &Cp);
            if (RT_FAILURE(rc) || !Cp)
                break;
            if (!CpQuote)
            {
                if (Cp == '"' || Cp == '\'')
                    CpQuote = Cp;
                else if (rtGetOptIsCpInSet(Cp, pszSeparators, cchSeparators))
                    break;
                else
                    pszDst = RTStrPutCp(pszDst, Cp);
            }
            else if (CpQuote != Cp)
                pszDst = RTStrPutCp(pszDst, Cp);
            else
                CpQuote = 0;
        }
        *pszDst++ = '\0';
        if (RT_FAILURE(rc) || !Cp)
            break;
    }

    if (RT_FAILURE(rc))
    {
        RTMemFree(pszDup);
        RTMemFree(papszArgs);
        return rc;
    }

    /*
     * Terminate the array.
     * Check for empty string to make sure we've got an array.
     */
    if (iArg == 0)
    {
        RTMemFree(pszDup);
        papszArgs = (char **)RTMemAlloc(1 * sizeof(char *));
        if (!papszArgs)
            return VERR_NO_MEMORY;
    }
    papszArgs[iArg] = NULL;

    *pcArgs     = iArg;
    *ppapszArgv = papszArgs;
    return VINF_SUCCESS;
}


RTDECL(void) RTGetOptArgvFree(char **papszArgv)
{
    if (papszArgv)
    {
        /*
         * We've really only _two_ allocations here. Check the code in
         * RTGetOptArgvFromString for the particulars.
         */
        RTMemFree(papszArgv[0]);
        RTMemFree(papszArgv);
    }
}


/**
 * Checks if the argument needs quoting or not.
 *
 * @returns true if it needs, false if it don't.
 * @param   pszArg              The argument.
 * @param   fFlags              Quoting style.
 * @param   pcch                Where to store the argument length when quoting
 *                              is not required.  (optimization)
 */
DECLINLINE(bool) rtGetOpArgvRequiresQuoting(const char *pszArg, uint32_t fFlags, size_t *pcch)
{
    char const *psz = pszArg;
    unsigned char ch;
    while ((ch = (unsigned char)*psz))
    {
        if (   ch < 128
            && ASMBitTest(&g_abmQuoteChars[fFlags & RTGETOPTARGV_CNV_QUOTE_MASK], ch))
            return true;
        psz++;
    }

    *pcch = psz - pszArg;
    return false;
}


/**
 * Grows the command line string buffer.
 *
 * @returns VINF_SUCCESS or VERR_NO_STR_MEMORY.
 * @param   ppszCmdLine     Pointer to the command line string pointer.
 * @param   pcbCmdLineAlloc Pointer to the allocation length variable.
 * @param   cchMin          The minimum size to grow with, kind of.
 */
static int rtGetOptArgvToStringGrow(char **ppszCmdLine, size_t *pcbCmdLineAlloc, size_t cchMin)
{
    size_t cb = *pcbCmdLineAlloc;
    while (cb < cchMin)
        cb *= 2;
    cb *= 2;
    *pcbCmdLineAlloc = cb;
    return RTStrRealloc(ppszCmdLine, cb);
}

/**
 * Checks if we have a sequence of DOS slashes followed by a double quote char.
 *
 * @returns true / false accordingly.
 * @param   psz             The string.
 */
DECLINLINE(bool) rtGetOptArgvMsCrtIsSlashQuote(const char *psz)
{
    while (*psz == '\\')
        psz++;
    return *psz == '"' || *psz == '\0';
}


RTDECL(int) RTGetOptArgvToString(char **ppszCmdLine, const char * const *papszArgv, uint32_t fFlags)
{
    AssertReturn(!(fFlags & ~RTGETOPTARGV_CNV_QUOTE_MASK), VERR_INVALID_PARAMETER);

#define PUT_CH(ch) \
        if (RT_UNLIKELY(off + 1 >= cbCmdLineAlloc)) { \
            rc = rtGetOptArgvToStringGrow(&pszCmdLine, &cbCmdLineAlloc, 1); \
            if (RT_FAILURE(rc)) \
                break; \
        } \
        pszCmdLine[off++] = (ch)

#define PUT_PSZ(psz, cch) \
        if (RT_UNLIKELY(off + (cch) >= cbCmdLineAlloc)) { \
            rc = rtGetOptArgvToStringGrow(&pszCmdLine, &cbCmdLineAlloc, (cch)); \
            if (RT_FAILURE(rc)) \
                break; \
        } \
        memcpy(&pszCmdLine[off], (psz), (cch)); \
        off += (cch);
#define PUT_SZ(sz)  PUT_PSZ(sz, sizeof(sz) - 1)

    /*
     * Take the realloc approach, it requires less code and is probably more
     * efficient than figuring out the size first.
     */
    int     rc              = VINF_SUCCESS;
    size_t  off             = 0;
    size_t  cbCmdLineAlloc  = 256;
    char   *pszCmdLine      = RTStrAlloc(256);
    if (!pszCmdLine)
        return VERR_NO_STR_MEMORY;

    for (size_t i = 0; papszArgv[i]; i++)
    {
        if (i > 0)
        {
            PUT_CH(' ');
        }

        /* does it need quoting? */
        const char *pszArg = papszArgv[i];
        size_t      cchArg;
        if (!rtGetOpArgvRequiresQuoting(pszArg, fFlags, &cchArg))
        {
            /* No quoting needed, just append the argument. */
            PUT_PSZ(pszArg, cchArg);
        }
        else if ((fFlags & RTGETOPTARGV_CNV_QUOTE_MASK) == RTGETOPTARGV_CNV_QUOTE_MS_CRT)
        {
            /*
             * Microsoft CRT quoting.  Quote the whole argument in double
             * quotes to make it easier to read and code.
             */
            PUT_CH('"');
            char ch;
            while ((ch = *pszArg++))
            {
                if (   ch == '\\'
                    && rtGetOptArgvMsCrtIsSlashQuote(pszArg))
                {
                    PUT_SZ("\\\\");
                }
                else if (ch == '"')
                {
                    PUT_SZ("\\\"");
                }
                else
                {
                    PUT_CH(ch);
                }
            }
            PUT_CH('"');
        }
        else
        {
            /*
             * Bourne Shell quoting.  Quote the whole thing in single quotes
             * and use double quotes for any single quote chars.
             */
            PUT_CH('\'');
            char ch;
            while ((ch = *pszArg++))
            {
                if (ch == '\'')
                {
                    PUT_SZ("'\"'\"'");
                }
                else
                {
                    PUT_CH(ch);
                }
            }
            PUT_CH('\'');
        }
    }

    /* Set return value / cleanup. */
    if (RT_SUCCESS(rc))
    {
        pszCmdLine[off] = '\0';
        *ppszCmdLine    = pszCmdLine;
    }
    else
        RTStrFree(pszCmdLine);
#undef PUT_SZ
#undef PUT_PSZ
#undef PUT_CH
    return rc;
}


RTDECL(int) RTGetOptArgvToUtf16String(PRTUTF16 *ppwszCmdLine, const char * const *papszArgv, uint32_t fFlags)
{
    char *pszCmdLine;
    int rc = RTGetOptArgvToString(&pszCmdLine, papszArgv, fFlags);
    if (RT_SUCCESS(rc))
    {
        rc = RTStrToUtf16(pszCmdLine, ppwszCmdLine);
        RTStrFree(pszCmdLine);
    }
    return rc;
}


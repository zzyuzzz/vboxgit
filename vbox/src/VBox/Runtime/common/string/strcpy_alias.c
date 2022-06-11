/* $Id: strcpy_alias.c 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * IPRT - No-CRT strcpy() alias for gcc.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/nocrt/string.h>
#undef strcpy

#if defined(RT_OS_DARWIN) || defined(RT_OS_WINDOWS)
# ifndef __MINGW32__
#  pragma weak strcpy
# endif

/* No alias support here (yet in the ming case). */
extern char * (strcpy)(char *psz1, const char *psz2)
{
    return RT_NOCRT(strcpy)(psz1, psz2);
}

#elif __GNUC__ >= 4
/* create a weak alias. */
__asm__(".weak strcpy\t\n"
        " .set strcpy," RT_NOCRT_STR(strcpy) "\t\n"
        ".global strcpy\t\n"
        );
#else
/* create a weak alias. */
extern __typeof(RT_NOCRT(strcpy)) strcpy __attribute__((weak, alias(RT_NOCRT_STR(strcpy))));
#endif


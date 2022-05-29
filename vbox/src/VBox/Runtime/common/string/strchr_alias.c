/* $Id: strchr_alias.c 9503 2008-06-08 03:04:08Z vboxsync $ */
/** @file
 * IPRT - No-CRT strchr() alias for gcc.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
#include <iprt/nocrt/string.h>
#undef strchr

#if defined(RT_OS_DARWIN) || defined(RT_OS_WINDOWS)
# ifndef __MINGW32__
#  pragma weak strchr
# endif

/* No alias support here (yet in the ming case). */
extern char *(strchr)(const char *psz, int ch)
{
    return RT_NOCRT(strchr)(psz, ch);
}

#elif __GNUC__ >= 4
/* create a weak alias. */
__asm__(".weak strchr\t\n"
        " .set strchr," RT_NOCRT_STR(strchr) "\t\n");
#else
/* create a weak alias. */
extern __typeof(RT_NOCRT(strchr)) strchr __attribute__((weak, alias(RT_NOCRT_STR(strchr))));
#endif


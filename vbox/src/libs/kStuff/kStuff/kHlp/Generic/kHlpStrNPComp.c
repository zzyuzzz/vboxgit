/* $Id: kHlpStrNPComp.c 3608 2007-10-29 00:49:05Z bird $ */
/** @file
 * kHlpString - kHlpStrNPComp.
 */

/*
 * Copyright (c) 2006-2007 knut st. osmundsen <bird-kStuff-spam@anduin.net>
 *
 * This file is part of kStuff.
 *
 * kStuff is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * In addition to the permissions in the GNU Lesser General Public
 * License, you are granted unlimited permission to link the compiled
 * version of this file into combinations with other programs, and to
 * distribute those combinations without any restriction coming from
 * the use of this file.
 *
 * kStuff is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with kStuff; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpString.h>


KHLP_DECL(char *) kHlpStrNPComp(const char *psz1, const char *psz2, KSIZE cb)
{
    while (cb-- > 0)
    {
        char ch1 = *psz1;
        char ch2 = *psz2;
        if (ch1 != ch2)
            return (char *)psz1;
        if (!ch1)
            break;
        psz1++;
        psz2++;
    }
    return NULL;
}



/* $Id: kHlpBareThread.c 3606 2007-10-29 00:48:44Z bird $ */
/** @file
 * kHlpBare - Thread Manipulation.
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
#include <k/kHlpThread.h>

#if K_OS == K_OS_DARWIN

#elif K_OS == K_OS_LINUX
# include <k/kHlpSys.h>

#elif K_OS == K_OS_OS2
# define INCL_BASE
# define INCL_ERRORS
# include <os2.h>
#elif  K_OS == K_OS_WINDOWS
# include <Windows.h>
#else
# error "port me"
#endif


/**
 * Sleep for a number of milliseconds.
 * @param   cMillies    Number of milliseconds to sleep.
 */
void kHlpSleep(unsigned cMillies)
{
#if K_OS == K_OS_DARWIN
    /** @todo mach_wait_until, see gen/nanosleep.c. */
    usleep(cMillies * 1000);

#elif K_OS == K_OS_LINUX
    /** @todo find the right syscall... */

#elif K_OS == K_OS_OS2
    DosSleep(cMillies);
#elif  K_OS == K_OS_WINDOWS
    Sleep(cMillies);
#else
    usleep(cMillies * 1000);
#endif
}


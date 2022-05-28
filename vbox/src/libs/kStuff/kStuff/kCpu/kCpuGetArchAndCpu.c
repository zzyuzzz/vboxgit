/* $Id: kCpuGetArchAndCpu.c 3605 2007-10-29 00:48:14Z bird $ */
/** @file
 * kCpu - kCpuGetArchAndCpu.
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
#include <k/kCpu.h>


/**
 * Gets the arch+cpu of the calling cpu.
 *
 * @param   penmArch    Where to store the cpu architecture.
 * @param   penmCpu     Where to store the cpu brand/model.
 */
KCPU_DECL(void) kCpuGetArchAndCpu(PKCPUARCH penmArch, PKCPU penmCpu)
{
#if K_ARCH == K_ARCH_AMD64
    *penmArch = KCPUARCH_AMD64;
    *penmCpu = KCPU_AMD64_BLEND; /** @todo check it using cpu. */

#elif K_ARCH == K_ARCH_X86_32
    *penmArch = KCPUARCH_X86_32;
    *penmCpu = KCPU_X86_32_BLEND; /** @todo check it using cpu. */

#else
# error "Port me"
#endif
}


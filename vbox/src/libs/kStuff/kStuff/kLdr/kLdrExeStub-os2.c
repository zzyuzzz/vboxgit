/* $Id: kLdrExeStub-os2.c 3609 2007-10-29 01:11:39Z bird $ */
/** @file
 * kLdr - OS/2 C Loader Stub.
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
#include <k/kLdr.h>
#include <os2.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/** The stub arguments. */
static const KLDREXEARGS g_Args =
{
    /* .fFlags       = */ KLDRDYLD_LOAD_FLAGS_RECURSIVE_INIT,
    /* .enmSearch    = */ KLDRDYLD_SEARCH_OS2,
    /* .szExecutable = */ "tst-0", /* just while testing */
    /* .szDefPrefix  = */ "",
    /* .szDefSuffix  = */ ".dll",
    /* .szLibPath    = */ ""
};

/**
 * OS/2 'main'.
 */
int _System OS2Main(HMODULE hmod, ULONG ulReserved, PCH pszzEnv, PCH pszzCmdLine)
{
    return kLdrDyldLoadExe(&g_Args, &hmod);
}


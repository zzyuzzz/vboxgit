/* $Id: dllmain.cpp 62679 2016-07-29 12:52:10Z vboxsync $ */
/** @file
 * VBoxHook -- Global windows hook dll
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


#include <iprt/win/windows.h>


/**
 * Dll entrypoint
 *
 * @returns type size or 0 if unknown type
 * @param   hDLLInst        Dll instance handle
 * @param   fdwReason       Callback reason
 * @param   lpvReserved     Reserved
 */
BOOL WINAPI DllMain(HINSTANCE hDLLInst, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            return TRUE;

        case DLL_PROCESS_DETACH:
            return TRUE;

        case DLL_THREAD_ATTACH:
            return TRUE;

        case DLL_THREAD_DETACH:
            return TRUE;

        default:
            return TRUE;
    }
}


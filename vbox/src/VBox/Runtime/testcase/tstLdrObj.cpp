/* $Id: tstLdrObj.cpp 8245 2008-04-21 17:24:28Z vboxsync $ */
/** @file
 * IPRT - RTLdr test object.
 *
 * We use precompiled versions of this object for testing all the loaders.
 *
 * This is not supposed to be pretty or usable code, just something which
 * make life difficult for the loader.
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
#ifndef IN_GC
# error "not IN_GC!"
#endif
#include <VBox/dis.h>
#include <VBox/vm.h>
#include <iprt/string.h>


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
static const char szStr1[] = "some readonly string";
static char szStr2[6000] = "some read/write string";
static char achBss[8192];

#ifdef VBOX_SOME_IMPORT_FUNCTION
extern "C" DECLIMPORT(int) SomeImportFunction(void);
#endif


extern "C" DECLEXPORT(int) Entrypoint(void)
{
    g_VM.fRawR0Enabled = true;
    g_VM.fRawR3Enabled = true;
    g_VM.fForcedActions = 0;
    strcpy(achBss, szStr2);
    memcpy(achBss, szStr1, sizeof(szStr1));
    memcpy(achBss, &g_VM, RT_MIN(sizeof(g_VM), sizeof(achBss)));
    memcpy(achBss, (void *)(uintptr_t)&Entrypoint, 32);
#ifdef VBOX_SOME_IMPORT_FUNCTION
    memcpy(achBss, (void *)(uintptr_t)&SomeImportFunction, 32);
    return SomeImportFunction();
#else
    return 0;
#endif
}


extern "C" DECLEXPORT(uint32_t) SomeExportFunction1(void *pvBuf)
{
    memcpy(pvBuf, &g_VM, sizeof(g_VM));
    return g_VM.fForcedActions;
}


extern "C" DECLEXPORT(char *) SomeExportFunction2(void *pvBuf)
{
    return (char *)memcpy(achBss, szStr1, sizeof(szStr1));
}


extern "C" DECLEXPORT(char *) SomeExportFunction3(void *pvBuf)
{
    return (char *)memcpy(achBss, szStr2, strlen(szStr2));
}


extern "C" DECLEXPORT(void *) SomeExportFunction4(void)
{
    static unsigned cb;
    DISCPUSTATE Cpu;
    memset(&Cpu, 0, sizeof(Cpu));
    Cpu.mode = CPUMODE_32BIT;
    DISCoreOne(&Cpu, (RTGCUINTPTR)SomeExportFunction3, &cb);
    return (void *)(uintptr_t)&SomeExportFunction1;
}


extern "C" DECLEXPORT(uintptr_t) SomeExportFunction5(void)
{
    return (uintptr_t)SomeExportFunction3(NULL) + (uintptr_t)SomeExportFunction2(NULL)
         + (uintptr_t)SomeExportFunction1(NULL) + (uintptr_t)&SomeExportFunction4;
}



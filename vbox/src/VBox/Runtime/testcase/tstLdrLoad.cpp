/* $Id: tstLdrLoad.cpp 8245 2008-04-21 17:24:28Z vboxsync $ */
/** @file
 * IPRT Testcase - Native Loader.
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


#include <iprt/ldr.h>
#include <iprt/stream.h>
#include <iprt/runtime.h>
#include <iprt/err.h>

int main(int argc, const char * const *argv)
{
    int rcRet = 0;
    RTR3Init();

    /*
     * If no args, display usage.
     */
    if (argc <= 1)
    {
        RTPrintf("Syntax: %s [so/dll [so/dll [..]]\n", argv[0]);
        return 1;
    }

    /*
     * Iterate the arguments and treat all of them as so/dll paths.
     */
    for (int i = 1; i < argc; i++)
    {
        RTLDRMOD hLdrMod = (RTLDRMOD)0xbaadffaa;
        int rc = RTLdrLoad(argv[i], &hLdrMod);
        if (RT_SUCCESS(rc))
        {
            RTPrintf("tstLdrLoad: %d - %s\n", i, argv[i]);
            rc = RTLdrClose(hLdrMod);
            if (RT_FAILURE(rc))
            {
                RTPrintf("tstLdrLoad: rc=%Rrc RTLdrClose()\n", rc);
                rcRet++;
            }
        }
        else
        {
            RTPrintf("tstLdrLoad: rc=%Rrc RTLdrOpen('%s')\n", rc, argv[i]);
            rcRet++;
        }
    }

    /*
     * Summary.
     */
    if (!rcRet)
        RTPrintf("tstLdrLoad: SUCCESS\n");
    else
        RTPrintf("tstLdrLoad: FAILURE - %d errors\n", rcRet);

    return !!rcRet;
}

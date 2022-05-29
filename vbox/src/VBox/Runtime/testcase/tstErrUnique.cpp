/* $Id: tstErrUnique.cpp 8245 2008-04-21 17:24:28Z vboxsync $ */
/** @file
 * IPRT Testcase - Error Messages.
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
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include <iprt/runtime.h>
#include <VBox/err.h>


/********************************************************************************   Global Variables                                                           ********************************************************************************//** Array of messages.
 * The data is generated by a sed script.
 */
static const RTSTATUSMSG  g_aErrorMessages[] =
{
#include "errmsgdata.h"
};

static bool strIsPermissibleDuplicate(const RTSTATUSMSG *msg)
{
    const char *pszMsgShort = msg->pszMsgShort;
    const char *pszDefine = msg->pszDefine;
    size_t cbDefine = strlen(pszDefine);

    return    (strstr(pszMsgShort, "(mapped to") != 0)
           || (strstr(pszDefine, "FIRST") == pszDefine + (cbDefine - 5))
           || (strstr(pszDefine, "LAST") == pszDefine + (cbDefine - 4));
}


int main()
{
    int         cErrors = 0;
    RTPrintf("tstErrUnique: TESTING\n");
    RTR3Init();

    for (uint32_t i = 0; i < ELEMENTS(g_aErrorMessages) - 1; i++)
    {
        if (strIsPermissibleDuplicate(&g_aErrorMessages[i]))
            continue;

        for (uint32_t j = i + 1; j < ELEMENTS(g_aErrorMessages); j++)
        {
            if (strIsPermissibleDuplicate(&g_aErrorMessages[j]))
                continue;

            if (g_aErrorMessages[i].iCode == g_aErrorMessages[j].iCode)
            {
                RTPrintf("tstErrUnique: status code %d can mean '%s' or '%s'\n", g_aErrorMessages[i].iCode, g_aErrorMessages[i].pszMsgShort, g_aErrorMessages[j]);
                cErrors++;
            }
        }
    }

    /*
     * Summary
     */
    if (cErrors == 0)
        RTPrintf("tstErrUnique: SUCCESS\n");
    else
        RTPrintf("tstErrUnique: FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}


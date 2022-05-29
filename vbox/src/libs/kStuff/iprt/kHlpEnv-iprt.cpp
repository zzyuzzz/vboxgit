/* $Id: kHlpEnv-iprt.cpp 8155 2008-04-18 15:16:47Z vboxsync $ */
/** @file
 * kHlpEnv - Environment Manipulation, IPRT based implementation.
 */

/*
 * Copyright (C) 2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <k/kHlpEnv.h>
#include <k/kErrors.h>
#include <iprt/env.h>
#include <iprt/assert.h>
#include <iprt/err.h>


KHLP_DECL(int) kHlpGetEnv(const char *pszVar, char *pszVal, KSIZE cchVal)
{
    int rc = RTEnvGetEx(RTENV_DEFAULT, pszVar, pszVal, cchVal, NULL);
    switch (rc)
    {
        case VINF_SUCCESS:              return 0;
        case VERR_ENV_VAR_NOT_FOUND:    return KERR_ENVVAR_NOT_FOUND;
        case VERR_BUFFER_OVERFLOW:      return KERR_BUFFER_OVERFLOW;
        default:                        AssertMsgFailedReturn(("%Rrc\n", rc), rc);
    }
}



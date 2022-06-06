/* $Id: threadctxhooks-r0drv-generic.cpp 55863 2015-05-14 18:29:34Z vboxsync $ */
/** @file
 * IPRT - Thread Context Switching Hook, Ring-0 Driver, Generic.
 */

/*
 * Copyright (C) 2013-2015 Oracle Corporation
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
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/thread.h>
#include <iprt/err.h>

#include "internal/iprt.h"

RTDECL(int) RTThreadCtxHookCreate(PRTTHREADCTXHOOK phCtxHook, uint32_t fFlags, PFNRTTHREADCTXHOOK pfnCallback, void *pvUser)
{
    NOREF(phCtxHook);
    NOREF(pfnCallback);
    NOREF(pvUser);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTThreadCtxHookCreate);


RTDECL(int) RTThreadCtxHookDestroy(RTTHREADCTXHOOK hCtxHook)
{
    return hCtxHook == NIL_RTTHREADCTXHOOK ? VINF_SUCCESS : VERR_INVALID_HANDLE;
}
RT_EXPORT_SYMBOL(RTThreadCtxHookDestroy);


RTDECL(int) RTThreadCtxHookEnable(RTTHREADCTXHOOK hCtxHook)
{
    NOREF(hCtxHook);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTThreadCtxHookEnable);


RTDECL(int) RTThreadCtxHookDisable(RTTHREADCTXHOOK hCtxHook)
{
    NOREF(hCtxHook);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTThreadCtxHookDisable);


RTDECL(bool) RTThreadCtxHookIsEnabled(RTTHREADCTXHOOK hCtxHook)
{
    NOREF(hCtxHook);
    return false;
}
RT_EXPORT_SYMBOL(RTThreadCtxHookIsEnabled);


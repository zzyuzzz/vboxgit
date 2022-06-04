/* $Id: RTMpOn-r0drv-generic.cpp 44529 2013-02-04 15:54:15Z vboxsync $ */
/** @file
 * IPRT - Multiprocessor, Ring-0 Driver, Generic Stubs.
 */

/*
 * Copyright (C) 2008-2010 Oracle Corporation
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
#include <iprt/mp.h>
#include "internal/iprt.h"

#include <iprt/err.h>


RTDECL(int) RTMpOnAll(PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnAll);


RTDECL(int) RTMpOnOthers(PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnOthers);


RTDECL(int) RTMpOnSpecific(RTCPUID idCpu, PFNRTMPWORKER pfnWorker, void *pvUser1, void *pvUser2)
{
    NOREF(idCpu);
    NOREF(pfnWorker);
    NOREF(pvUser1);
    NOREF(pvUser2);
    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTMpOnSpecific);


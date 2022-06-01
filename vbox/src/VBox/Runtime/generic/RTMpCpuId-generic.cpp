/* $Id: RTMpCpuId-generic.cpp 28800 2010-04-27 08:22:32Z vboxsync $ */
/** @file
 * IPRT - Multiprocessor, Generic RTMpCpuId.
 */

/*
 * Copyright (C) 2008 Oracle Corporation
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

#include <iprt/asm.h>


RTDECL(RTCPUID) RTMpCpuId(void)
{
#if defined(RT_ARCH_X86) || defined(RT_ARCH_AMD64) || defined(RT_ARCH_SPARC) || defined(RT_ARCH_SPARC64)
    return ASMGetApicId();
#else
# error "Not ported to this architecture."
    return NIL_RTAPICID;
#endif
}
RT_EXPORT_SYMBOL(RTMpCpuId);


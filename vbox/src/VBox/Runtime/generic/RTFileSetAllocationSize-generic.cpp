/* $Id: RTFileSetAllocationSize-generic.cpp 59459 2016-01-25 13:33:52Z vboxsync $ */
/** @file
 * IPRT - RTFileSetAllocationSize, generic implementation.
 */

/*
 * Copyright (C) 2016 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/file.h>

#include "internal/iprt.h"


RTDECL(int) RTFileSetAllocationSize(RTFILE hFile, uint64_t cbSize, uint32_t fFlags)
{
    /*
     * Quick validation.
     */
    AssertReturn(!(fFlags & ~RTFILE_ALLOC_SIZE_F_VALID), VERR_INVALID_PARAMETER);

    NOREF(hFile);
    NOREF(cbSize);

    return VERR_NOT_SUPPORTED;
}
RT_EXPORT_SYMBOL(RTFileSetAllocationSize);


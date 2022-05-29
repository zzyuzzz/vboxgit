/* $Id: semfastmutex-r0drv-linux.c 5999 2007-12-07 15:05:06Z vboxsync $ */
/** @file
 * innotek Portable Runtime - Fast Mutex Semaphores, Ring-0 Driver, Linux.
 */

/*
 * Copyright (C) 2006-2007 innotek GmbH
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
#include "the-linux-kernel.h"
#include <iprt/semaphore.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/err.h>

#include "internal/magics.h"


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Wrapper for the linux semaphore structure.
 */
typedef struct RTSEMFASTMUTEXINTERNAL
{
    /** Magic value (RTSEMFASTMUTEX_MAGIC). */
    uint32_t            u32Magic;
    /** the linux semaphore. */
    struct semaphore    Semaphore;
} RTSEMFASTMUTEXINTERNAL, *PRTSEMFASTMUTEXINTERNAL;


RTDECL(int)  RTSemFastMutexCreate(PRTSEMFASTMUTEX pMutexSem)
{
    /*
     * Allocate.
     */
    PRTSEMFASTMUTEXINTERNAL pFastInt;
    pFastInt = (PRTSEMFASTMUTEXINTERNAL)RTMemAlloc(sizeof(*pFastInt));
    if (!pFastInt)
        return VERR_NO_MEMORY;

    /*
     * Initialize.
     */
    pFastInt->u32Magic = RTSEMFASTMUTEX_MAGIC;
    sema_init(&pFastInt->Semaphore, 1);
    *pMutexSem = pFastInt;
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexDestroy(RTSEMFASTMUTEX MutexSem)
{
    /*
     * Validate.
     */
    PRTSEMFASTMUTEXINTERNAL pFastInt = (PRTSEMFASTMUTEXINTERNAL)MutexSem;
    if (!pFastInt)
        return VERR_INVALID_PARAMETER;
    if (pFastInt->u32Magic != RTSEMFASTMUTEX_MAGIC)
    {
        AssertMsgFailed(("pFastInt->u32Magic=%RX32 pMutexInt=%p\n", pFastInt->u32Magic, pFastInt));
        return VERR_INVALID_PARAMETER;
    }

    ASMAtomicIncU32(&pFastInt->u32Magic);
    RTMemFree(pFastInt);
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexRequest(RTSEMFASTMUTEX MutexSem)
{
    /*
     * Validate.
     */
    PRTSEMFASTMUTEXINTERNAL pFastInt = (PRTSEMFASTMUTEXINTERNAL)MutexSem;
    if (    !pFastInt
        ||  pFastInt->u32Magic != RTSEMFASTMUTEX_MAGIC)
    {
        AssertMsgFailed(("pFastInt->u32Magic=%RX32 pMutexInt=%p\n", pFastInt ? pFastInt->u32Magic : 0, pFastInt));
        return VERR_INVALID_PARAMETER;
    }

    down(&pFastInt->Semaphore);
    return VINF_SUCCESS;
}


RTDECL(int)  RTSemFastMutexRelease(RTSEMFASTMUTEX MutexSem)
{
    /*
     * Validate.
     */
    PRTSEMFASTMUTEXINTERNAL pFastInt = (PRTSEMFASTMUTEXINTERNAL)MutexSem;
    if (    !pFastInt
        ||  pFastInt->u32Magic != RTSEMFASTMUTEX_MAGIC)
    {
        AssertMsgFailed(("pFastInt->u32Magic=%RX32 pMutexInt=%p\n", pFastInt ? pFastInt->u32Magic : 0, pFastInt));
        return VERR_INVALID_PARAMETER;
    }

    up(&pFastInt->Semaphore);
    return VINF_SUCCESS;
}


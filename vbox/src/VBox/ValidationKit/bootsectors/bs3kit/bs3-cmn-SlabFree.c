/* $Id: bs3-cmn-SlabFree.c 59286 2016-01-08 00:23:32Z vboxsync $ */
/** @file
 * BS3Kit - Bs3SlabFree
 */

/*
 * Copyright (C) 2007-2015 Oracle Corporation
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
#include "bs3kit-template-header.h"
#include <iprt/asm.h>


BS3_DECL(uint16_t) Bs3SlabFree(PBS3SLABCTL pSlabCtl, uint32_t uFlatChunkPtr, uint16_t cChunks)
{
    uint16_t cFreed = 0;
    BS3_ASSERT(cChunks > 0);
    if (cChunks > 0)
    {
        uint16_t iChunk = (uint16_t)((uFlatChunkPtr - BS3_XPTR_GET_FLAT(uint8_t, pSlabCtl->pbStart)) >> pSlabCtl->cChunkShift);
        BS3_ASSERT(iChunk < pSlabCtl->cChunks);
        BS3_ASSERT(iChunk + cChunks <= pSlabCtl->cChunks);

        do
        {
            if (ASMBitTestAndClear(&pSlabCtl->bmAllocated, iChunk))
                cFreed++;
            else
                BS3_ASSERT(0);
            iChunk++;
        } while (--cChunks > 0);

        pSlabCtl->cFreeChunks += cFreed;
    }
    return cFreed;
}


/* $Id: VBoxGuestR3LibMisc.cpp 6470 2008-01-24 07:09:56Z vboxsync $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, Misc.
 */

/*
 * Copyright (C) 2007 innotek GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/assert.h>
#include "VBGLR3Internal.h"


/**
 * Cause any pending WaitEvent calls (VBOXGUEST_IOCTL_WAITEVENT) to return
 * with a VERR_INTERRUPTED status.
 *
 * Can be used in combination with a termination flag variable for interrupting
 * event loops. Avoiding race conditions is the responsibility of the caller.
 *
 * @returns IPRT status code
 */
VBGLR3DECL(int) VbglR3InterruptEventWaits(void)
{
    return vbglR3DoIOCtl(VBOXGUEST_IOCTL_CANCEL_ALL_WAITEVENTS, 0, 0);
}


/**
 * Write to the backdoor logger from ring 3 guest code.
 *
 * @returns IPRT status code
 *
 * @remarks This currently does not accept more than 255 bytes of data at
 *          one time. It should probably be rewritten to use pass a pointer
 *          in the IOCtl.
 */
VBGLR3DECL(int) VbglR3WriteLog(const char *pch, size_t cb)
{
    /*
     * *BSD does not accept more than 4KB per ioctl request,
     * so, split it up into 2KB chunks.
     */
#define STEP 2048
    int rc = VINF_SUCCESS;
    for (size_t off = 0; off < cb && RT_SUCCESS(rc); off += STEP)
    {
        size_t cbStep = RT_MIN(cb - off, STEP);
        rc = vbglR3DoIOCtl(VBOXGUEST_IOCTL_LOG(cbStep), (char *)pch + off, cbStep);
    }
#undef STEP
    return rc;
}


/**
 * Change the IRQ filter mask.
 *
 * @returns IPRT status code
 * @param   fOr     The OR mask.
 * @param   fNo     The NOT mask.
 */
VBGLR3DECL(int) VbglR3CtlFilterMask(uint32_t fOr, uint32_t fNot)
{
    VBoxGuestFilterMaskInfo Info;
    Info.u32OrMask = fOr;
    Info.u32NotMask = fNot;
    return vbglR3DoIOCtl(VBOXGUEST_IOCTL_CTL_FILTER_MASK, &Info, sizeof(Info));
}

/**
 * Query the last display change request.
 *
 * @returns iprt status value
 * @param   pcx         Where to store the horizontal pixel resolution (0 = do not change).
 * @param   pcy         Where to store the vertical pixel resolution (0 = do not change).
 * @param   pcBits      Where to store the bits per pixel (0 = do not change).
 * @param   fFventAck   Flag that the request is an acknowlegement for the
 *                      VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST.
 *                      Values:
 *                          0                                   - just querying,
 *                          VMMDEV_EVENT_DISPLAY_CHANGE_REQUEST - event acknowledged.
 * @param   iDisplay    0 for primary display, 1 for the first secondary, etc.
 */
VBGLR3DECL(int) VbglR3GetDisplayChangeRequest(uint32_t *pcx, uint32_t *pcy, uint32_t *pcBits,
                                              uint32_t fEventAck, uint32_t iDisplay)
{
    VMMDevDisplayChangeRequest2 Req;
    vmmdevInitRequest(&Req.header, VMMDevReq_GetDisplayChangeRequest2);
    Req.xres = 0;
    Req.yres = 0;
    Req.bpp = 0;
    Req.eventAck = fEventAck;
    Req.display = iDisplay;
    int rc = vbglR3GRPerform(&Req.header);
    if (RT_SUCCESS(rc))
    {
        *pcx = Req.xres;
        *pcy = Req.yres;
        *pcBits = Req.bpp;
    }
    return rc;
}

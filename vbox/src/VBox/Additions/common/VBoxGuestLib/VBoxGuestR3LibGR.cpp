/* $Id: VBoxGuestR3LibGR.cpp 26425 2010-02-11 11:37:08Z vboxsync $ */
/** @file
 * VBoxGuestR3Lib - Ring-3 Support Library for VirtualBox guest additions, GR.
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
#ifdef VBOX_VBGLR3_XFREE86
/* Rather than try to resolve all the header file conflicts, I will just
   prototype what we need here. */
# define xalloc(size) Xalloc((unsigned long)(size))
# define xfree(ptr) Xfree((pointer)(ptr))
typedef void *pointer;
extern "C" pointer Xalloc(unsigned long /*amount*/);
extern "C" void Xfree(pointer /*ptr*/);
#else
# include <iprt/mem.h>
# include <iprt/assert.h>
# include <iprt/string.h>
#endif
#include <iprt/err.h>
#include "VBGLR3Internal.h"


int vbglR3GRAlloc(VMMDevRequestHeader **ppReq, uint32_t cb, VMMDevRequestType enmReqType)
{
    VMMDevRequestHeader *pReq;

#ifdef VBOX_VBGLR3_XFREE86
    pReq = (VMMDevRequestHeader *)xalloc(cb);
#else
    AssertPtrReturn(ppReq, VERR_INVALID_PARAMETER);
    AssertMsgReturn(cb >= sizeof(VMMDevRequestHeader), ("%#x vs %#zx\n", cb, sizeof(VMMDevRequestHeader)),
                    VERR_INVALID_PARAMETER);

    pReq = (VMMDevRequestHeader *)RTMemTmpAlloc(cb);
#endif
    if (RT_UNLIKELY(!pReq))
        return VERR_NO_MEMORY;

    pReq->size        = cb;
    pReq->version     = VMMDEV_REQUEST_HEADER_VERSION;
    pReq->requestType = enmReqType;
    pReq->rc          = VERR_GENERAL_FAILURE;
    pReq->reserved1   = 0;
    pReq->reserved2   = 0;

    *ppReq = pReq;

    return VINF_SUCCESS;
}


VBGLR3DECL(int) vbglR3GRPerform(VMMDevRequestHeader *pReq)
{
    return vbglR3DoIOCtl(VBOXGUEST_IOCTL_VMMREQUEST(pReq->size), pReq, pReq->size);
}


void vbglR3GRFree(VMMDevRequestHeader *pReq)
{
#ifdef VBOX_VBGLR3_XFREE86
    xfree(pReq);
#else
    RTMemTmpFree(pReq);
#endif
}


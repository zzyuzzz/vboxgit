/* $Rev: 21408 $ */
/** @file
 * VBoxGuest - Inter Driver Communcation, unix implementation.
 *
 * This file is included by the platform specific source file.
 */

/*
 * Copyright (C) 2006-2009 Sun Microsystems, Inc.
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
 * Some lines of code to disable the local APIC on x86_64 machines taken
 * from a Mandriva patch by Gwenole Beauchesne <gbeauchesne@mandriva.com>.
 */


/**
 * Open a new IDC connection.
 *
 * @returns Opaque pointer to session object.
 * @param   pu32Version         Where to store VMMDev version.
 */
DECLVBGL(void *) VBoxGuestIDCOpen(uint32_t *pu32Version)
{
    PVBOXGUESTSESSION   pSession;
    int                 rc;
    LogFlow(("VBoxGuestIDCOpen: Version=%#x\n", pu32Version ? *pu32Version : 0));

    AssertPtrReturn(pu32Version, NULL);
    rc = VBoxGuestCreateKernelSession(&g_DevExt, &pSession);
    if (RT_SUCCESS(rc))
    {
        *pu32Version = VMMDEV_VERSION;
        return pSession;
    }
    LogRel(("VBoxGuestIDCOpen: VBoxGuestCreateKernelSession failed. rc=%d\n", rc));
    return NULL;
}


/**
 * Close an IDC connection.
 *
 * @returns VBox error code.
 * @param   pvState             Opaque pointer to the session object.
 */
DECLVBGL(int) VBoxGuestIDCClose(void *pvSession)
{
    PVBOXGUESTSESSION pSession = (PVBOXGUESTSESSION)pvSession;
    LogFlow(("VBoxGuestIDCClose:\n"));

    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    VBoxGuestCloseSession(&g_DevExt, pSession);
    return VINF_SUCCESS;
}


/**
 * Perform an IDC call.
 *
 * @returns VBox error code.
 * @param   pvSession           Opaque pointer to the session.
 * @param   iCmd                Requested function.
 * @param   pvData              IO data buffer.
 * @param   cbData              Size of the data buffer.
 * @param   pcbDataReturned     Where to store the amount of returned data.
 */
DECLVBGL(int) VBoxGuestIDCCall(void *pvSession, unsigned iCmd, void *pvData, size_t cbData, size_t *pcbDataReturned)
{
    PVBOXGUESTSESSION pSession = (PVBOXGUESTSESSION)pvSession;
    LogFlow(("VBoxGuestIDCCall: %pvSesssion=%p Cmd=%u pvData=%p cbData=%d\n", pvSession, iCmd, pvData, cbData));

    AssertPtrReturn(pSession, VERR_INVALID_POINTER);
    AssertMsgReturn(pSession->pDevExt == &g_DevExt,
                    ("SC: %p != %p\n", pSession->pDevExt, &g_DevExt), VERR_INVALID_HANDLE);

    return VBoxGuestCommonIOCtl(iCmd, &g_DevExt, pSession, pvData, cbData, pcbDataReturned);
}



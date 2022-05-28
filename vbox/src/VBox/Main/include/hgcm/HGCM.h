/** @file
 *
 * HGCM - Host-Guest Communication Manager
 */

/*
 * Copyright (C) 2006 InnoTek Systemberatung GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */

#ifndef __HGCM_h__
#define __HGCM_h__

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/pdm.h>

#include <VBox/VBoxGuest.h>
#include <VBox/hgcmsvc.h>

__BEGIN_DECLS
int hgcmInit (void);

int hgcmLoadInternal (const char *pszServiceName, const char *pszServiceLibrary);

int hgcmConnectInternal (PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmdPtr, PHGCMSERVICELOCATION pLoc, uint32_t *pClientID, bool fBlock);
int hgcmDisconnectInternal (PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmdPtr, uint32_t clientID, bool fBlock);
int hgcmGuestCallInternal (PPDMIHGCMPORT pHGCMPort, PVBOXHGCMCMD pCmdPtr, uint32_t clientID, uint32_t function, uint32_t cParms, VBOXHGCMSVCPARM aParms[], bool fBlock);
int hgcmHostCallInternal (const char *pszServiceName, uint32_t function, uint32_t cParms, VBOXHGCMSVCPARM aParms[]);
__END_DECLS

#endif /* __HGCM_h__ */

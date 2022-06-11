/* $Id: VBoxSharedClipboardSvc-uri.h 80858 2019-09-17 13:03:39Z vboxsync $ */
/** @file
 * Shared Clipboard Service - Internal header for transfer (list) handling.
 */

/*
 * Copyright (C) 2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_SharedClipboard_VBoxSharedClipboardSvc_uri_h
#define VBOX_INCLUDED_SRC_SharedClipboard_VBoxSharedClipboardSvc_uri_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

int sharedClipboardSvcTransferHandler(PSHCLCLIENT pClient, VBOXHGCMCALLHANDLE callHandle, uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[], uint64_t tsArrival);
int sharedClipboardSvcTransferHostHandler(uint32_t u32Function, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);

int sharedClipboardSvcTransferAreaRegister(PSHCLCLIENTSTATE pClientState, PSHCLTRANSFER pTransfer);
int sharedClipboardSvcTransferAreaUnregister(PSHCLCLIENTSTATE pClientState, PSHCLTRANSFER pTransfer);
int sharedClipboardSvcTransferAreaAttach(PSHCLCLIENTSTATE pClientState, PSHCLTRANSFER pTransfer, SHCLAREAID uID);
int sharedClipboardSvcTransferAreaDetach(PSHCLCLIENTSTATE pClientState, PSHCLTRANSFER pTransfer);

#endif /* !VBOX_INCLUDED_SRC_SharedClipboard_VBoxSharedClipboardSvc_uri_h */


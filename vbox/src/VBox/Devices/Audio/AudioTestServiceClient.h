/* $Id: AudioTestServiceClient.h 89205 2021-05-20 16:37:01Z vboxsync $ */
/** @file
 * AudioTestServiceClient - Audio test execution server, Public Header.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_Audio_AudioTestServiceClient_h
#define VBOX_INCLUDED_SRC_Audio_AudioTestServiceClient_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

typedef struct ATSCLIENT
{
    uint8_t  abHdr[16];
    uint16_t cbHdr;
    RTSOCKET hSock;
} ATSCLIENT;
typedef ATSCLIENT *PATSCLIENT;

int AudioTestSvcClientConnect(PATSCLIENT pClient);
int AudioTestSvcClientClose(PATSCLIENT pClient);

#endif /* !VBOX_INCLUDED_SRC_Audio_AudioTestServiceClient_h */


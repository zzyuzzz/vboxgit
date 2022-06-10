/* $Id: HDAStreamChannel.h 76520 2018-12-30 05:39:55Z vboxsync $ */
/** @file
 * HDAStreamChannel.h - Stream channel functions for HD Audio.
 */

/*
 * Copyright (C) 2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef HDA_STREAMCHANNEL_H
#define HDA_STREAMCHANNEL_H
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

int  hdaR3StreamChannelDataInit(PPDMAUDIOSTREAMCHANNELDATA pChanData, uint32_t fFlags);
void hdaR3StreamChannelDataDestroy(PPDMAUDIOSTREAMCHANNELDATA pChanData);
int  hdaR3StreamChannelAcquireData(PPDMAUDIOSTREAMCHANNELDATA pChanData, void *ppvData, size_t *pcbData);
int  hdaR3StreamChannelReleaseData(PPDMAUDIOSTREAMCHANNELDATA pChanData);

#endif /* !HDA_STREAMCHANNEL_H */


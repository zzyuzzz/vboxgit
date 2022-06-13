/* $Id: SvgaFifo.h 94622 2022-04-16 14:32:45Z vboxsync $ */
/** @file
 * VirtualBox Windows Guest Mesa3D - Gallium driver VMSVGA FIFO operations.
 */

/*
 * Copyright (C) 2016-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_wddm_gallium_SvgaFifo_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_wddm_gallium_SvgaFifo_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "Svga.h"

NTSTATUS SvgaFifoInit(PVBOXWDDM_EXT_VMSVGA pSvga);

void *SvgaFifoReserve(PVBOXWDDM_EXT_VMSVGA pSvga, uint32_t cbReserve);
void SvgaFifoCommit(PVBOXWDDM_EXT_VMSVGA pSvga, uint32_t cbActual);

NTSTATUS SvgaCmdBufInit(PVBOXWDDM_EXT_VMSVGA pSvga);
NTSTATUS SvgaCmdBufDestroy(PVBOXWDDM_EXT_VMSVGA pSvga);

NTSTATUS SvgaCmdBufDeviceCommand(PVBOXWDDM_EXT_VMSVGA pSvga, void const *pvCmd, uint32_t cbCmd);

void *SvgaCmdBuf3dCmdReserve(PVBOXWDDM_EXT_VMSVGA pSvga, SVGAFifo3dCmdId enmCmd, uint32_t cbReserve, uint32_t idDXContext);
void *SvgaCmdBufFifoCmdReserve(PVBOXWDDM_EXT_VMSVGA pSvga, SVGAFifoCmdId enmCmd, uint32_t cbReserve);
void *SvgaCmdBufReserve(PVBOXWDDM_EXT_VMSVGA pSvga, uint32_t cbReserve, uint32_t idDXContext);
void  SvgaCmdBufCommit(PVBOXWDDM_EXT_VMSVGA pSvga, uint32_t cbActual);
void  SvgaCmdBufFlush(PVBOXWDDM_EXT_VMSVGA pSvga);
void  SvgaCmdBufProcess(PVBOXWDDM_EXT_VMSVGA pSvga);

#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_mp_wddm_gallium_SvgaFifo_h */

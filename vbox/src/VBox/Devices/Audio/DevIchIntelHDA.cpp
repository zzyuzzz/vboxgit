/* $Id: DevIchIntelHDA.cpp 31028 2010-07-23 03:33:34Z vboxsync $ */
/** @file
 * DevIchIntelHD - VBox ICH Intel HD Audio Controller.
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DEV_AUDIO
#include <VBox/pdmdev.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/asm.h>

#include "../Builtins.h"

extern "C" {
#include "audio.h"
}
#include "DevCodec.h"

#undef LOG_VOICES
#ifndef VBOX
//#define USE_MIXER
#else
#define USE_MIXER
#endif

#define INTELHD_SSM_VERSION 1
PDMBOTHCBDECL(int) hdaMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb);
PDMBOTHCBDECL(int) hdaMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb);
static DECLCALLBACK(void)  hdaReset (PPDMDEVINS pDevIns);

/* Registers */
#define HDA_REG_IND_NAME(x) ICH6_HDA_REG_##x
#define HDA_REG_FIELD_NAME(reg, x) ICH6_HDA_##reg##_##x
#define HDA_REG_FIELD_MASK(reg, x) ICH6_HDA_##reg##_##x##_MASK
#define HDA_REG_FIELD_FLAG_MASK(reg, x) RT_BIT(ICH6_HDA_##reg##_##x##_SHIFT)
#define HDA_REG_FIELD_SHIFT(reg, x) ICH6_HDA_##reg##_##x##_SHIFT
#define HDA_REG_IND(pState, x) ((pState)->au32Regs[(x)])
#define HDA_REG(pState, x) (HDA_REG_IND((pState), HDA_REG_IND_NAME(x)))
#define HDA_REG_VALUE(pState, reg, val) (HDA_REG((pState),reg) & (((HDA_REG_FIELD_MASK(reg, val))) << (HDA_REG_FIELD_SHIFT(reg, val))))
#define HDA_REG_FLAG_VALUE(pState, reg, val) (HDA_REG((pState),reg) & (((HDA_REG_FIELD_FLAG_MASK(reg, val)))))
#define HDA_REG_SVALUE(pState, reg, val) (HDA_REG_VALUE(pState, reg, val) >> (HDA_REG_FIELD_SHIFT(reg, val)))

#define ICH6_HDA_REG_GCAP 0 /* range 0x00-0x01*/
#define GCAP(pState) (HDA_REG((pState), GCAP))

#define ICH6_HDA_REG_VMIN 1 /* range 0x02 */
#define VMIN(pState) (HDA_REG((pState), VMIN))

#define ICH6_HDA_REG_VMAJ 2 /* range 0x03 */
#define VMAJ(pState) (HDA_REG((pState), VMAJ))

#define ICH6_HDA_REG_OUTPAY 3 /* range 0x04-0x05 */
#define OUTPAY(pState) (HDA_REG((pState), OUTPAY))

#define ICH6_HDA_REG_INPAY 4 /* range 0x06-0x07 */
#define INPAY(pState) (HDA_REG((pState), INPAY))

#define ICH6_HDA_REG_GCTL (5)
#define ICH6_HDA_GCTL_RST_SHIFT (0)
#define ICH6_HDA_GCTL_FSH_SHIFT (1)
#define GCTL(pState) (HDA_REG((pState), GCTL))

#define ICH6_HDA_REG_STATES 7 /* range 0x0E */
#define STATES(pState) (HDA_REG((pState), STATES))
#define ICH6_HDA_STATES_SCSF 0x1

#define ICH6_HDA_REG_GSTS 8 /* range 0x10-0x11*/
#define ICH6_HDA_GSTS_FSH_SHIFT (1)
#define GSTS(pState) (HDA_REG(pState, GSTS))

#define ICH6_HDA_REG_INTCTL 9 /* 0x20 */
#define ICH6_HDA_INTCTL_GIE_SHIFT 31
#define ICH6_HDA_INTCTL_CIE_SHIFT 30
#define ICH6_HDA_INTCTL_S0_SHIFT (0)
#define ICH6_HDA_INTCTL_S1_SHIFT (1)
#define ICH6_HDA_INTCTL_S2_SHIFT (2)
#define ICH6_HDA_INTCTL_S3_SHIFT (3)
#define ICH6_HDA_INTCTL_S4_SHIFT (4)
#define ICH6_HDA_INTCTL_S5_SHIFT (5)
#define ICH6_HDA_INTCTL_S6_SHIFT (6)
#define ICH6_HDA_INTCTL_S7_SHIFT (7)
#define INTCTL(pState) (HDA_REG((pState), INTCTL))
#define INTCTL_GIE(pState) (HDA_REG_FLAG_VALUE(pState, INTCTL, GIE))
#define INTCTL_CIE(pState) (HDA_REG_FLAG_VALUE(pState, INTCTL, CIE))
#define INTCTL_SX(pState, X) (HDA_REG_FLAG_VALUE((pState), INTCTL, S##X))
#define INTCTL_SALL(pState) (INTCTL((pState)) & 0xFF)

/* Note: The HDA specification defines a SSYNC register at offset 0x38. The
 * ICH6/ICH9 datahseet defines SSYNC at offset 0x34. The Linux HDA driver matches
 * the datasheet.
 */
#define ICH6_HDA_REG_SSYNC  12 /* 0x34 */
#define SSYNC(pState) (HDA_REG((pState), SSYNC))

#define ICH6_HDA_REG_INTSTS 10 /* 0x24 */
#define ICH6_HDA_INTSTS_GIS_SHIFT (31)
#define ICH6_HDA_INTSTS_CIS_SHIFT (30)
#define ICH6_HDA_INTSTS_S0_SHIFT (0)
#define ICH6_HDA_INTSTS_S1_SHIFT (1)
#define ICH6_HDA_INTSTS_S2_SHIFT (2)
#define ICH6_HDA_INTSTS_S3_SHIFT (3)
#define ICH6_HDA_INTSTS_S4_SHIFT (4)
#define ICH6_HDA_INTSTS_S5_SHIFT (5)
#define ICH6_HDA_INTSTS_S6_SHIFT (6)
#define ICH6_HDA_INTSTS_S7_SHIFT (7)
#define ICH6_HDA_INTSTS_S_MASK(num) RT_BIT(HDA_REG_FIELD_SHIFT(S##num))
#define INTSTS(pState) (HDA_REG((pState), INTSTS))
#define INTSTS_GIS(pState) (HDA_REG_FLAG_VALUE((pState), INTSTS, GIS)
#define INTSTS_CIS(pState) (HDA_REG_FLAG_VALUE((pState), INTSTS, CIS)
#define INTSTS_SX(pState, X) (HDA_REG_FLAG_VALUE(pState), INTSTS, S##X)
#define INTSTS_SANY(pState) (INTSTS((pState)) & 0xFF)

#define ICH6_HDA_REG_CORBLBASE  13 /* 0x40 */
#define ICH6_HDA_REG_CORBUBASE  14 /* 0x44 */
#define ICH6_HDA_REG_CORBWP  15 /* 48 */
#define ICH6_HDA_REG_CORBRP  16 /* 4A */
#define ICH6_HDA_CORBRP_RST_SHIFT  15
#define ICH6_HDA_CORBRP_WP_SHIFT  0
#define ICH6_HDA_CORBRP_WP_MASK   0xFF

#define CORBRP(pState) (HDA_REG(pState, CORBRP))
#define CORBWP(pState) (HDA_REG(pState, CORBWP))

#define ICH6_HDA_REG_CORBCTL  17 /* 0x4C */
#define ICH6_HDA_COBCTL_RUN_SHIFT (1)
#define ICH6_HDA_COBCTL_CMEIE_SHIFT (0)

#define ICH6_HDA_REG_CORBSTS  18 /* 0x4D */
#define CORBSTS(pState) (HDA_REG(pState, CORBSTS))
#define ICH6_HDA_CORBSTS_CMEI_SHIFT  (0)

#define ICH6_HDA_REG_CORBSIZE  19 /* 0x4E */
#define ICH6_HDA_CORBSIZE_SZ_CAP 0xF0
#define ICH6_HDA_CORBSIZE_SZ 0x3
#define CORBSIZE_SZ(pState) (HDA_REG(pState, ICH6_HDA_REG_CORBSIZE) & ICH6_HDA_CORBSIZE_SZ)
#define CORBSIZE_SZ_CAP(pState) (HDA_REG(pState, ICH6_HDA_REG_CORBSIZE) & ICH6_HDA_CORBSIZE_SZ_CAP)
/* till ich 10 sizes of CORB and RIRB are harcoded to 256 in real hw */
#define CORBSIZE(pState) (255)

#define ICH6_HDA_REG_RIRLBASE  20 /* 0x50 */
#define ICH6_HDA_REG_RIRUBASE  21 /* 0x54 */

#define ICH6_HDA_REG_RIRBWP  22 /* 0x58 */
#define ICH6_HDA_RIRBWP_RST_SHIFT  (15)
#define ICH6_HDA_RIRBWP_WP_MASK   0xFF
#define RIRBWP(pState) (HDA_REG(pState, RIRBWP))

#define ICH6_HDA_REG_RINTCNT  23 /* 0x5A */
#define RINTCNT(pState) (HDA_REG((pState), RINTCNT))
#define RINTCNT_N(pState) (RINTCNT((pState)) & 0xff)

#define ICH6_HDA_REG_RIRBCTL  24 /* 0x5C */
#define ICH6_HDA_RIRBCTL_RIC_SHIFT    (0)
#define ICH6_HDA_RIRBCTL_DMA_SHIFT    (1)
#define ICH6_HDA_ROI_DMA_SHIFT        (2)
#define RIRBCTL(pState)                 (HDA_REG((pState), RIRBCTL))
#define RIRBCTL_RIRB_RIC(pState)        (HDA_REG_FLAG_VALUE(pState, RIRBCTL, RIC))
#define RIRBCTL_RIRB_DMA(pState)        (HDA_REG_FLAG_VALUE((pState), RIRBCTL, DMA)
#define RIRBCTL_ROI(pState)             (HDA_REG_FLAG_VALUE((pState), RIRBCTL, ROI))

#define ICH6_HDA_REG_RIRBSTS  25 /* 0x5D */
#define ICH6_HDA_RIRBSTS_RINTFL_SHIFT (0)
#define ICH6_HDA_RIRBSTS_RIRBOIS_SHIFT (2)
#define RIRBSTS(pState)         (HDA_REG(pState, RIRBSTS))
#define RIRBSTS_RINTFL(pState)  (HDA_REG_FLAG_VALUE(pState, RIRBSTS, RINTFL))
#define RIRBSTS_RIRBOIS(pState) (HDA_REG_FLAG_VALUE(pState, RIRBSTS, RIRBOIS))

#define ICH6_HDA_REG_RIRBSIZE  26 /* 0x5E */
#define ICH6_HDA_RIRBSIZE_SZ_CAP 0xF0
#define ICH6_HDA_RIRBSIZE_SZ 0x3

#define RIRBSIZE_SZ(pState)     (HDA_REG(pState, ICH6_HDA_REG_RIRBSIZE) & ICH6_HDA_RIRBSIZE_SZ)
#define RIRBSIZE_SZ_CAP(pState) (HDA_REG(pState, ICH6_HDA_REG_RIRBSIZE) & ICH6_HDA_RIRBSIZE_SZ_CAP)
#define RIRBSIZE(pState)        (255)


#define ICH6_HDA_REG_IC   27 /* 0x60 */
#define IC(pState) (HDA_REG(pState, IC))
#define ICH6_HDA_REG_IR   28 /* 0x64 */
#define IR(pState) (HDA_REG(pState, IR))
#define ICH6_HDA_REG_IRS  29 /* 0x68 */
#define ICH6_HDA_IRS_ICB_SHIFT   (0)
#define ICH6_HDA_IRS_IRV_SHIFT   (1)
#define IRS(pState)     (HDA_REG(pState, IRS))
#define IRS_ICB(pState) (HDA_REG_FLAG_VALUE(pState, IRS, ICB))
#define IRS_IRV(pState) (HDA_REG_FLAG_VALUE(pState, IRS, IRV))

#define ICH6_HDA_REG_DPLBASE  30 /* 0x70 */
#define DPLBASE(pState) (HDA_REG((pState), DPLBASE))
#define ICH6_HDA_REG_DPUBASE  31 /* 0x74 */
#define DPUBASE(pState) (HDA_REG((pState), DPUBASE))

#define HDA_STREAM_REG_DEF(name, num) (ICH6_HDA_REG_SD##num##name)
#define HDA_STREAM_REG(pState, name, num) (HDA_REG((pState), N_(HDA_STREAM_REG_DEF(name, num))))

#define ICH6_HDA_REG_SD0CTL   32 /* 0x80 */
#define ICH6_HDA_REG_SD1CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 10) /* 0xA0 */
#define ICH6_HDA_REG_SD2CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 20) /* 0xC0 */
#define ICH6_HDA_REG_SD3CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 30) /* 0xE0 */
#define ICH6_HDA_REG_SD4CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 40) /* 0x100 */
#define ICH6_HDA_REG_SD5CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 50) /* 0x120 */
#define ICH6_HDA_REG_SD6CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 60) /* 0x140 */
#define ICH6_HDA_REG_SD7CTL   (HDA_STREAM_REG_DEF(CTL, 0) + 70) /* 0x160 */

#define SD(func, num) SD##num##func
#define SDCTL(pState, num) HDA_REG((pState), SD(CTL, num))
#define SDCTL_NUM(pState, num) ((SDCTL((pState), num) & HDA_REG_FIELD_MASK(SDCTL,NUM)) >> HDA_REG_FIELD_SHIFT(SDCTL, NUM))
#define ICH6_HDA_SDCTL_NUM_MASK   (0xF)
#define ICH6_HDA_SDCTL_NUM_SHIFT  (20)
#define ICH6_HDA_SDCTL_DEIE_SHIFT (4)
#define ICH6_HDA_SDCTL_FEIE_SHIFT (3)
#define ICH6_HDA_SDCTL_ICE_SHIFT  (2)
#define ICH6_HDA_SDCTL_RUN_SHIFT  (1)
#define ICH6_HDA_SDCTL_SRST_SHIFT (0)

#define ICH6_HDA_REG_SD0STS   33 /* 0x83 */
#define ICH6_HDA_REG_SD1STS   (HDA_STREAM_REG_DEF(STS, 0) + 10) /* 0xA3 */
#define ICH6_HDA_REG_SD2STS   (HDA_STREAM_REG_DEF(STS, 0) + 20) /* 0xC3 */
#define ICH6_HDA_REG_SD3STS   (HDA_STREAM_REG_DEF(STS, 0) + 30) /* 0xE3 */
#define ICH6_HDA_REG_SD4STS   (HDA_STREAM_REG_DEF(STS, 0) + 40) /* 0x103 */
#define ICH6_HDA_REG_SD5STS   (HDA_STREAM_REG_DEF(STS, 0) + 50) /* 0x123 */
#define ICH6_HDA_REG_SD6STS   (HDA_STREAM_REG_DEF(STS, 0) + 60) /* 0x143 */
#define ICH6_HDA_REG_SD7STS   (HDA_STREAM_REG_DEF(STS, 0) + 70) /* 0x163 */

#define SDSTS(pState, num) HDA_REG((pState), SD(STS, num))
#define ICH6_HDA_SDSTS_FIFORDY_SHIFT (5)
#define ICH6_HDA_SDSTS_DE_SHIFT (4)
#define ICH6_HDA_SDSTS_FE_SHIFT (3)
#define ICH6_HDA_SDSTS_BCIS_SHIFT  (2)

#define ICH6_HDA_REG_SD0LPIB   34 /* 0x84 */
#define ICH6_HDA_REG_SD1LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 10) /* 0xA4 */
#define ICH6_HDA_REG_SD2LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 20) /* 0xC4 */
#define ICH6_HDA_REG_SD3LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 30) /* 0xE4 */
#define ICH6_HDA_REG_SD4LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 40) /* 0x104 */
#define ICH6_HDA_REG_SD5LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 50) /* 0x124 */
#define ICH6_HDA_REG_SD6LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 60) /* 0x144 */
#define ICH6_HDA_REG_SD7LPIB   (HDA_STREAM_REG_DEF(LPIB, 0) + 70) /* 0x164 */

#define SDLPIB(pState, num) HDA_REG((pState), SD(LPIB, num))

#define ICH6_HDA_REG_SD0CBL   35 /* 0x88 */
#define ICH6_HDA_REG_SD1CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 10) /* 0xA8 */
#define ICH6_HDA_REG_SD2CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 20) /* 0xC8 */
#define ICH6_HDA_REG_SD3CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 30) /* 0xE8 */
#define ICH6_HDA_REG_SD4CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 40) /* 0x108 */
#define ICH6_HDA_REG_SD5CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 50) /* 0x128 */
#define ICH6_HDA_REG_SD6CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 60) /* 0x148 */
#define ICH6_HDA_REG_SD7CBL   (HDA_STREAM_REG_DEF(CBL, 0) + 70) /* 0x168 */

#define SDLCBL(pState, num) HDA_REG((pState), SD(CBL, num))

#define ICH6_HDA_REG_SD0LVI   36 /* 0x8C */
#define ICH6_HDA_REG_SD1LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 10) /* 0xAC */
#define ICH6_HDA_REG_SD2LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 20) /* 0xCC */
#define ICH6_HDA_REG_SD3LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 30) /* 0xEC */
#define ICH6_HDA_REG_SD4LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 40) /* 0x10C */
#define ICH6_HDA_REG_SD5LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 50) /* 0x12C */
#define ICH6_HDA_REG_SD6LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 60) /* 0x14C */
#define ICH6_HDA_REG_SD7LVI   (HDA_STREAM_REG_DEF(LVI, 0) + 70) /* 0x16C */

#define SDLVI(pState, num) HDA_REG((pState), SD(LVI, num))

#define ICH6_HDA_REG_SD0FIFOW   37 /* 0x8E */
#define ICH6_HDA_REG_SD1FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 10) /* 0xAE */
#define ICH6_HDA_REG_SD2FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 20) /* 0xCE */
#define ICH6_HDA_REG_SD3FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 30) /* 0xEE */
#define ICH6_HDA_REG_SD4FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 40) /* 0x10E */
#define ICH6_HDA_REG_SD5FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 50) /* 0x12E */
#define ICH6_HDA_REG_SD6FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 60) /* 0x14E */
#define ICH6_HDA_REG_SD7FIFOW   (HDA_STREAM_REG_DEF(FIFOW, 0) + 70) /* 0x16E */

#define ICH6_HDA_REG_SD0FIFOS   38 /* 0x90 */
#define ICH6_HDA_REG_SD1FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 10) /* 0xB0 */
#define ICH6_HDA_REG_SD2FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 20) /* 0xD0 */
#define ICH6_HDA_REG_SD3FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 30) /* 0xF0 */
#define ICH6_HDA_REG_SD4FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 40) /* 0x110 */
#define ICH6_HDA_REG_SD5FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 50) /* 0x130 */
#define ICH6_HDA_REG_SD6FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 60) /* 0x150 */
#define ICH6_HDA_REG_SD7FIFOS   (HDA_STREAM_REG_DEF(FIFOS, 0) + 70) /* 0x170 */

#define SDFIFOS(pState, num) HDA_REG((pState), SD(FIFOS, num))

#define ICH6_HDA_REG_SD0FMT     39 /* 0x92 */
#define ICH6_HDA_REG_SD1FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 10) /* 0xB2 */
#define ICH6_HDA_REG_SD2FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 20) /* 0xD2 */
#define ICH6_HDA_REG_SD3FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 30) /* 0xF2 */
#define ICH6_HDA_REG_SD4FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 40) /* 0x112 */
#define ICH6_HDA_REG_SD5FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 50) /* 0x132 */
#define ICH6_HDA_REG_SD6FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 60) /* 0x152 */
#define ICH6_HDA_REG_SD7FMT     (HDA_STREAM_REG_DEF(FMT, 0) + 70) /* 0x172 */

#define ICH6_HDA_REG_SD0BDPL     40 /* 0x98 */
#define ICH6_HDA_REG_SD1BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 10) /* 0xB8 */
#define ICH6_HDA_REG_SD2BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 20) /* 0xD8 */
#define ICH6_HDA_REG_SD3BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 30) /* 0xF8 */
#define ICH6_HDA_REG_SD4BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 40) /* 0x118 */
#define ICH6_HDA_REG_SD5BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 50) /* 0x138 */
#define ICH6_HDA_REG_SD6BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 60) /* 0x158 */
#define ICH6_HDA_REG_SD7BDPL     (HDA_STREAM_REG_DEF(BDPL, 0) + 70) /* 0x178 */

#define SDBDPL(pState, num) HDA_REG((pState), SD(BDPL, num))

#define ICH6_HDA_REG_SD0BDPU     41 /* 0x9C */
#define ICH6_HDA_REG_SD1BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 10) /* 0xBC */
#define ICH6_HDA_REG_SD2BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 20) /* 0xDC */
#define ICH6_HDA_REG_SD3BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 30) /* 0xFC */
#define ICH6_HDA_REG_SD4BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 40) /* 0x11C */
#define ICH6_HDA_REG_SD5BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 50) /* 0x13C */
#define ICH6_HDA_REG_SD6BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 60) /* 0x15C */
#define ICH6_HDA_REG_SD7BDPU     (HDA_STREAM_REG_DEF(BDPU, 0) + 70) /* 0x17C */

#define SDBDPU(pState, num) HDA_REG((pState), SD(BDPU, num))

/* Predicates */


typedef struct INTELHDLinkState
{
    /** Pointer to the device instance. */
    PPDMDEVINSR3            pDevIns;
    /** Pointer to the connector of the attached audio driver. */
    PPDMIAUDIOCONNECTOR     pDrv;
    /** Pointer to the attached audio driver. */
    PPDMIBASE               pDrvBase;
    /** The base interface for LUN\#0. */
    PDMIBASE                IBase;
    RTGCPHYS    addrMMReg;
    uint32_t     au32Regs[113];
    /* Current BD index  */
    uint32_t    u32Cvi; 
    uint64_t    u64CviAddr; 
    /* Length of current BD entry */
    uint32_t    u32CviLen;
    uint32_t    u32CviPos;
    uint32_t    u32Cbp;
    /* Interrupt on completition */
    bool        fCviIoc;
    uint64_t    u64CORBBase;
    uint64_t    u64RIRBBase;
    uint64_t    u64DPBase;
    uint8_t     u8CORBRP;
    uint8_t     cResponse;
    /* pointer on CORB buf */
    uint32_t    *pu32CorbBuf;
    /* size in bytes of CORB buf */
    uint32_t    cbCorbBuf;
    /* size in double words of CORB buf */
    uint8_t     cdwCorbBuf;
    /* pointer on RIRB buf */
    uint64_t    *pu64RirbBuf;
    /* size in bytes of RIRB buf */
    uint32_t    cbRirbBuf;
    /* size in quad words of RIRB buf */
    uint8_t     cdqRirbBuf;
    CODECState  Codec;
} INTELHDLinkState;

#define ICH6_HDASTATE_2_DEVINS(pINTELHD)   ((pINTELHD)->pDevIns)
#define PCIDEV_2_ICH6_HDASTATE(pPciDev) ((PCIINTELHDLinkState *)(pPciDev))




typedef struct PCIINTELHDLinkState
{
    PCIDevice dev;
    INTELHDLinkState hda;
} PCIINTELHDLinkState;

DECLCALLBACK(int)hdaRegReadUnimplemented(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteUnimplemented(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegReadGCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteGCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegReadSTATESTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteSTATESTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegReadGCAP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegReadINTSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteINTSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegWriteCORBWP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegWriteCORBRP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteCORBSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteRIRBWP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegWriteRIRBSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteIRS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteSDCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegReadSDCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);

DECLCALLBACK(int)hdaRegWriteSDSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteSDLVI(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteSDBDPL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteSDBDPU(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegWriteBase(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
DECLCALLBACK(int)hdaRegReadU32(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteU32(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegReadU24(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteU24(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegReadU16(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteU16(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
DECLCALLBACK(int)hdaRegReadU8(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
DECLCALLBACK(int)hdaRegWriteU8(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t pu32Value);
static int hdaLookup(INTELHDLinkState* pState, uint32_t u32Offset);

/* see 302349 p 6.2*/
const static struct stIchIntelHDRegMap
{
    /** Register offset in the register space. */
    uint32_t   offset;
    /** Size in bytes. Registers of size > 4 are in fact tables. */
    uint32_t   size;
    /** Readable bits. */
    uint32_t readable;
    /** Writable bits. */
    uint32_t writable;
    /** Read callback. */
    int       (*pfnRead)(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value);
    /** Write callback. */
    int       (*pfnWrite)(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value);
    /** Abbreviated name. */
    const char *abbrev;
    /** Full name. */
    const char *name;
} s_ichIntelHDRegMap[] =
{
    /* offset  size     read mask   write mask         read callback         write callback         abbrev      full name                     */
    /*-------  -------  ----------  ----------  -----------------------  ------------------------ ----------    ------------------------------*/
    { 0x00000, 0x00002, 0x0000FFFB, 0x00000000, hdaRegReadGCAP         , hdaRegWriteUnimplemented, "GCAP"      , "Global Capabilities" },
    { 0x00002, 0x00001, 0x000000FF, 0x00000000, hdaRegReadU8           , hdaRegWriteUnimplemented, "VMIN"      , "Minor Version" },
    { 0x00003, 0x00001, 0x000000FF, 0x00000000, hdaRegReadU8           , hdaRegWriteUnimplemented, "VMAJ"      , "Major Version" },
    { 0x00004, 0x00002, 0x0000FFFF, 0x00000000, hdaRegReadU16          , hdaRegWriteUnimplemented, "OUTPAY"    , "Output Payload Capabilities" },
    { 0x00006, 0x00002, 0x0000FFFF, 0x00000000, hdaRegReadU16          , hdaRegWriteUnimplemented, "INPAY"     , "Input Payload Capabilities" },
    { 0x00008, 0x00004, 0x00000103, 0x00000103, hdaRegReadGCTL         , hdaRegWriteGCTL         , "GCTL"      , "Global Control" },
    { 0x0000c, 0x00002, 0xFFFFFFFF, 0x00000000, hdaRegReadUnimplemented, hdaRegWriteUnimplemented, "WAKEEN"    , "Wake Enable" },
    { 0x0000e, 0x00002, 0x00000007, 0x00000007, hdaRegReadU8           , hdaRegWriteSTATESTS     , "STATESTS"  , "State Change Status" },
    { 0x00010, 0x00002, 0xFFFFFFFF, 0x00000000, hdaRegReadUnimplemented, hdaRegWriteUnimplemented, "GSTS"      , "Global Status" },
    { 0x00020, 0x00004, 0xC00000FF, 0xC00000FF, hdaRegReadU32          , hdaRegWriteU32          , "INTCTL"    , "Interrupt Control" },
    { 0x00024, 0x00004, 0xC00000FF, 0x400000FF, hdaRegReadINTSTS       , hdaRegWriteINTSTS       , "INTSTS"    , "Interrupt Status" },
    //** @todo r=michaln: Are guests really not reading the WALCLK register at all?
    { 0x00030, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadUnimplemented, hdaRegWriteUnimplemented, "WALCLK"    , "Wall Clock Counter" },
    //** @todo r=michaln: Doesn't the SSYNC register need to actually stop the stream(s)?
    { 0x00034, 0x00004, 0x000000FF, 0x000000FF, hdaRegReadU32          , hdaRegWriteU32          , "SSYNC"     , "Stream Synchronization" },
    { 0x00040, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteBase         , "CORBLBASE" , "CORB Lower Base Address" },
    { 0x00044, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteBase         , "CORBUBASE" , "CORB Upper Base Address" },
    { 0x00048, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteCORBWP       , "CORBWP"    , "CORB Write Pointer" },
    { 0x0004A, 0x00002, 0x000000FF, 0x000080FF, hdaRegReadU8           , hdaRegWriteCORBRP       , "CORBRP"    , "CORB Read Pointer" },
    { 0x0004C, 0x00001, 0x00000003, 0x00000003, hdaRegReadU8           , hdaRegWriteU8           , "CORBCTL"   , "CORB Control" },
    { 0x0004D, 0x00001, 0x00000001, 0x00000001, hdaRegReadU8           , hdaRegWriteCORBSTS      , "CORBSTS"   , "CORB Status" },
    { 0x0004E, 0x00001, 0x000000F3, 0x00000000, hdaRegReadU8           , hdaRegWriteUnimplemented, "CORBSIZE"  , "CORB Size" },
    { 0x00050, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteBase         , "RIRBLBASE" , "RIRB Lower Base Address" },
    { 0x00054, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteBase         , "RIRBUBASE" , "RIRB Upper Base Address" },
    { 0x00058, 0x00002, 0x000000FF, 0x00008000, hdaRegReadU8,            hdaRegWriteRIRBWP       , "RIRBWP"    , "RIRB Write Pointer" },
    { 0x0005A, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "RINTCNT"   , "Response Interrupt Count" },
    { 0x0005C, 0x00001, 0x00000007, 0x00000007, hdaRegReadU8           , hdaRegWriteU8           , "RIRBCTL"   , "RIRB Control" },
    { 0x0005D, 0x00001, 0x00000005, 0x00000005, hdaRegReadU8           , hdaRegWriteRIRBSTS      , "RIRBSTS"   , "RIRB Status" },
    { 0x0005E, 0x00001, 0x000000F3, 0x00000000, hdaRegReadU8           , hdaRegWriteUnimplemented, "RIRBSIZE"  , "RIRB Size" },
    { 0x00060, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "IC"        , "Immediate Command" },
    { 0x00064, 0x00004, 0x00000000, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteUnimplemented, "IR"        , "Immediate Response" },
    { 0x00068, 0x00004, 0x00000002, 0x00000002, hdaRegReadU16          , hdaRegWriteIRS          , "IRS"       , "Immediate Command Status" },
    { 0x00070, 0x00004, 0xFFFFFFFF, 0xFFFFFF81, hdaRegReadU32          , hdaRegWriteBase         , "DPLBASE"   , "DMA Position Lower Base" },
    { 0x00074, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteBase         , "DPUBASE"   , "DMA Position Upper Base" },

    { 0x00080, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "ISD0CTL"  , "Input Stream Descriptor 0 (ICD0) Control" },
    { 0x00083, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "ISD0STS"  , "ISD0 Status" },
    { 0x00084, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "ISD0LPIB" , "ISD0 Link Position In Buffer" },
    { 0x00088, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "ISD0CBL"  , "ISD0 Cyclic Buffer Length" },
    { 0x0008C, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "ISD0LVI"  , "ISD0 Last Valid Index" },
    { 0x0008E, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "ISD0FIFOW", "ISD0 FIFO Watermark" },
    { 0x00090, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "ISD0FIFOS", "ISD0 FIFO Size" },
    { 0x00092, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "ISD0FMT"  , "ISD0 Format" },
    { 0x00098, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "ISD0BDPL" , "ISD0 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x0009C, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "ISD0BDPU" , "ISD0 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x000A0, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "ISD1CTL"  , "Input Stream Descriptor 1 (ISD1) Control" },
    { 0x000A3, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "ISD1STS"  , "ISD1 Status" },
    { 0x000A4, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "ISD1LPIB" , "ISD1 Link Position In Buffer" },
    { 0x000A8, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "ISD1CBL"  , "ISD1 Cyclic Buffer Length" },
    { 0x000AC, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "ISD1LVI"  , "ISD1 Last Valid Index" },
    { 0x000AE, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "ISD1FIFOW", "ISD1 FIFO Watermark" },
    { 0x000B0, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "ISD1FIFOS", "ISD1 FIFO Size" },
    { 0x000B2, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "ISD1FMT"  , "ISD1 Format" },
    { 0x000B8, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "ISD1BDPL" , "ISD1 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x000BC, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "ISD1BDPU" , "ISD1 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x000C0, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "ISD2CTL"  , "Input Stream Descriptor 2 (ISD2) Control" },
    { 0x000C3, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "ISD2STS"  , "ISD2 Status" },
    { 0x000C4, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "ISD2LPIB" , "ISD2 Link Position In Buffer" },
    { 0x000C8, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "ISD2CBL"  , "ISD2 Cyclic Buffer Length" },
    { 0x000CC, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "ISD2LVI"  , "ISD2 Last Valid Index" },
    { 0x000CE, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "ISD2FIFOW", "ISD2 FIFO Watermark" },
    { 0x000D0, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "ISD2FIFOS", "ISD2 FIFO Size" },
    { 0x000D2, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "ISD2FMT"  , "ISD2 Format" },
    { 0x000D8, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "ISD2BDPL" , "ISD2 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x000DC, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "ISD2BDPU" , "ISD2 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x000E0, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "ISD3CTL"  , "Input Stream Descriptor 3 (ISD3) Control" },
    { 0x000E3, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "ISD3STS"  , "ISD3 Status" },
    { 0x000E4, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "ISD3LPIB" , "ISD3 Link Position In Buffer" },
    { 0x000E8, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "ISD3CBL"  , "ISD3 Cyclic Buffer Length" },
    { 0x000EC, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "ISD3LVI"  , "ISD3 Last Valid Index" },
    { 0x000EE, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "ISD3FIFOW", "ISD3 FIFO Watermark" },
    { 0x000F0, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "ISD3FIFOS", "ISD3 FIFO Size" },
    { 0x000F2, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "ISD3FMT"  , "ISD3 Format" },
    { 0x000F8, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "ISD3BDPL" , "ISD3 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x000FC, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "ISD3BDPU" , "ISD3 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x00100, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadSDCTL        , hdaRegWriteSDCTL        , "OSD0CTL"  , "Input Stream Descriptor 0 (OSD0) Control" },
    { 0x00103, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "OSD0STS"  , "OSD0 Status" },
    { 0x00104, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "OSD0LPIB" , "OSD0 Link Position In Buffer" },
    { 0x00108, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "OSD0CBL"  , "OSD0 Cyclic Buffer Length" },
    { 0x0010C, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "OSD0LVI"  , "OSD0 Last Valid Index" },
    { 0x0010E, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "OSD0FIFOW", "OSD0 FIFO Watermark" },
    { 0x00110, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "OSD0FIFOS", "OSD0 FIFO Size" },
    { 0x00112, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "OSD0FMT"  , "OSD0 Format" },
    { 0x00118, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "OSD0BDPL" , "OSD0 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x0011C, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "OSD0BDPU" , "OSD0 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x00120, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "OSD1CTL"  , "Input Stream Descriptor 0 (OSD1) Control" },
    { 0x00123, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "OSD1STS"  , "OSD1 Status" },
    { 0x00124, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "OSD1LPIB" , "OSD1 Link Position In Buffer" },
    { 0x00128, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "OSD1CBL"  , "OSD1 Cyclic Buffer Length" },
    { 0x0012C, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "OSD1LVI"  , "OSD1 Last Valid Index" },
    { 0x0012E, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "OSD1FIFOW", "OSD1 FIFO Watermark" },
    { 0x00130, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "OSD1FIFOS", "OSD1 FIFO Size" },
    { 0x00132, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "OSD1FMT"  , "OSD1 Format" },
    { 0x00138, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "OSD1BDPL" , "OSD1 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x0013C, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "OSD1BDPU" , "OSD1 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x00140, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "OSD2CTL"  , "Input Stream Descriptor 0 (OSD2) Control" },
    { 0x00143, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "OSD2STS"  , "OSD2 Status" },
    { 0x00144, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "OSD2LPIB" , "OSD2 Link Position In Buffer" },
    { 0x00148, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "OSD2CBL"  , "OSD2 Cyclic Buffer Length" },
    { 0x0014C, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "OSD2LVI"  , "OSD2 Last Valid Index" },
    { 0x0014E, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "OSD2FIFOW", "OSD2 FIFO Watermark" },
    { 0x00150, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "OSD2FIFOS", "OSD2 FIFO Size" },
    { 0x00152, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "OSD2FMT"  , "OSD2 Format" },
    { 0x00158, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "OSD2BDPL" , "OSD2 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x0015C, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "OSD2BDPU" , "OSD2 Buffer Descriptor List Pointer-Upper Base Address" },

    { 0x00160, 0x00003, 0x00FF001F, 0x00F0001F, hdaRegReadU24          , hdaRegWriteSDCTL        , "OSD3CTL"  , "Input Stream Descriptor 0 (OSD3) Control" },
    { 0x00163, 0x00001, 0x0000001C, 0x0000003C, hdaRegReadU8           , hdaRegWriteSDSTS        , "OSD3STS"  , "OSD3 Status" },
    { 0x00164, 0x00004, 0xFFFFFFFF, 0x00000000, hdaRegReadU32          , hdaRegWriteU32          , "OSD3LPIB" , "OSD3 Link Position In Buffer" },
    { 0x00168, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteU32          , "OSD3CBL"  , "OSD3 Cyclic Buffer Length" },
    { 0x0016C, 0x00002, 0x0000FFFF, 0x0000FFFF, hdaRegReadU16          , hdaRegWriteSDLVI        , "OSD3LVI"  , "OSD3 Last Valid Index" },
    { 0x0016E, 0x00002, 0x00000005, 0x00000005, hdaRegReadU16          , hdaRegWriteU16          , "OSD3FIFOW", "OSD3 FIFO Watermark" },
    { 0x00170, 0x00002, 0x000000FF, 0x000000FF, hdaRegReadU16          , hdaRegWriteU16          , "OSD3FIFOS", "OSD3 FIFO Size" },
    { 0x00172, 0x00002, 0x00007F7F, 0x00007F7F, hdaRegReadU16          , hdaRegWriteU16          , "OSD3FMT"  , "OSD3 Format" },
    { 0x00178, 0x00004, 0xFFFFFF80, 0xFFFFFF80, hdaRegReadU32          , hdaRegWriteSDBDPL       , "OSD3BDPL" , "OSD3 Buffer Descriptor List Pointer-Lower Base Address" },
    { 0x0017C, 0x00004, 0xFFFFFFFF, 0xFFFFFFFF, hdaRegReadU32          , hdaRegWriteSDBDPU       , "OSD3BDPU" , "OSD3 Buffer Descriptor List Pointer-Upper Base Address" },
};

static int hdaProcessInterrupt(INTELHDLinkState* pState)
{
    bool fIrq = false;
   /* @todo add state change */    
    if(   INTCTL_CIE(pState)
       && (   RIRBSTS_RINTFL(pState)
           ||  RIRBSTS_RIRBOIS(pState)))
    {
        INTSTS(pState) |= HDA_REG_FIELD_FLAG_MASK(INTSTS, CIS);
        fIrq = true;
    }
    if (   INTCTL_SX(pState, 4) 
        && SDSTS(pState, 4) && HDA_REG_FIELD_FLAG_MASK(SDSTS, BCIS))
    {
        INTSTS(pState) |= HDA_REG_FIELD_FLAG_MASK(INTSTS, S4);
        fIrq = true;
    }
    if (INTCTL_GIE(pState))
    {
        Log(("hda: irq %s\n", fIrq ? "asserted" : "deasserted"));
        PDMDevHlpPCISetIrq(ICH6_HDASTATE_2_DEVINS(pState), 0 , fIrq);
    }
    return VINF_SUCCESS;
}

static int hdaLookup(INTELHDLinkState* pState, uint32_t u32Offset)
{
    int index = 0;
    //** @todo r=michaln: A linear search of an array with over 100 elements is very inefficient.
    for (;index < (int)(sizeof(s_ichIntelHDRegMap)/sizeof(s_ichIntelHDRegMap[0])); ++index)
    {
        if (   u32Offset >= s_ichIntelHDRegMap[index].offset
            && u32Offset < s_ichIntelHDRegMap[index].offset + s_ichIntelHDRegMap[index].size)
        {
            return index;
        }
    }
    return -1;
}

static int hdaCmdSync(INTELHDLinkState *pState, bool fLocal)
{
    int rc = VINF_SUCCESS;
    if (fLocal)
    {
        rc = PDMDevHlpPhysRead(ICH6_HDASTATE_2_DEVINS(pState), pState->u64CORBBase, pState->pu32CorbBuf, pState->cbCorbBuf);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
        uint8_t i = 0;
        do
        {
            Log(("hda: corb%02x: ", i));
            uint8_t j = 0;
            do
            {
                const char *prefix;
                if ((i + j) == CORBRP(pState))
                    prefix = "[R]";
                else if ((i + j) == CORBWP(pState))
                    prefix = "[W]";
                else
                    prefix = "   "; /* three spaces */
                Log(("%s%08x", prefix, pState->pu32CorbBuf[i + j]));
                j++;
            } while (j < 8);
            Log(("\n"));
            i += 8;
        } while(i != 0);
    }
    else
    {
        rc = PDMDevHlpPhysWrite(ICH6_HDASTATE_2_DEVINS(pState), pState->u64RIRBBase, pState->pu64RirbBuf, pState->cbRirbBuf);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
        uint8_t i = 0;
        do {
            Log(("hda: rirb%02x: ", i));
            uint8_t j = 0;
            do {
                const char *prefix;
                if ((i + j) == RIRBWP(pState))
                    prefix = "[W]";
                else
                    prefix = "   ";
                Log((" %s%016x", prefix, pState->pu64RirbBuf[i + j]));
            } while (++j < 8);
            Log(("\n"));
            i += 8;
        } while (i != 0);
    }
    return rc;
}
static int hdaCORBCmdProcess(INTELHDLinkState *pState)
{
    int rc;
    uint8_t corbRp;
    uint8_t corbWp;
    uint8_t rirbWp;

    PFNCODECVERBPROCESSOR pfn = (PFNCODECVERBPROCESSOR)NULL;
    
    rc = hdaCmdSync(pState, true);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);
    corbRp = CORBRP(pState);
    corbWp = CORBWP(pState);
    rirbWp = RIRBWP(pState);
    Assert((corbWp != corbRp));
    Log(("hda: CORB(RP:%x, WP:%x) RIRBWP:%x\n", CORBRP(pState), CORBWP(pState), RIRBWP(pState)));
    while (corbRp != corbWp)
    {
        uint32_t cmd;
        corbRp++;
        cmd = pState->pu32CorbBuf[corbRp];
        rc = (pState)->Codec.pfnLookup(&pState->Codec, cmd, &pfn);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
        (rirbWp)++;
        rc = pfn(&pState->Codec, cmd, &pState->pu64RirbBuf[rirbWp]);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
        pState->cResponse++;
    }
    pState->au32Regs[ICH6_HDA_REG_CORBRP] = corbRp;
    pState->au32Regs[ICH6_HDA_REG_RIRBWP] = rirbWp;
    rc = hdaCmdSync(pState, false);
    Log(("hda: CORB(RP:%x, WP:%x) RIRBWP:%x\n", CORBRP(pState), CORBWP(pState), RIRBWP(pState)));
    if (   RIRBCTL_RIRB_RIC(pState)
        && (pState)->cResponse == RINTCNT_N(pState))
    {
        RIRBSTS((pState)) |= HDA_REG_FIELD_FLAG_MASK(RIRBSTS,RINTFL);
        rc = hdaProcessInterrupt(pState);
        (pState)->cResponse = 0;
    }
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);
    return rc;
}

static void hdaStreamReset(INTELHDLinkState *pState, uint32_t u32Offset)
{
    Log(("hda: reset of stream (%x) started\n", u32Offset));
    Log(("hda: reset of stream (%x) finished\n", u32Offset));
}


DECLCALLBACK(int)hdaRegReadUnimplemented(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    *pu32Value = 0;
    return VINF_SUCCESS;
}
DECLCALLBACK(int)hdaRegWriteUnimplemented(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    return VINF_SUCCESS;
}
/* U8 */
DECLCALLBACK(int)hdaRegReadU8(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    Assert(((pState->au32Regs[index] & s_ichIntelHDRegMap[index].readable) & 0xffffff00) == 0);
    return hdaRegReadU32(pState, offset, index, pu32Value);
}

DECLCALLBACK(int)hdaRegWriteU8(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    Assert(((u32Value & 0xffffff00) == 0));
    return hdaRegWriteU32(pState, offset, index, u32Value);
}
/* U16 */
DECLCALLBACK(int)hdaRegReadU16(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    Assert(((pState->au32Regs[index] & s_ichIntelHDRegMap[index].readable) & 0xffff0000) == 0);
    return hdaRegReadU32(pState, offset, index, pu32Value);
}

DECLCALLBACK(int)hdaRegWriteU16(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    Assert(((u32Value & 0xffff0000) == 0));
    return hdaRegWriteU32(pState, offset, index, u32Value);
}

/* U24 */
DECLCALLBACK(int)hdaRegReadU24(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    Assert(((pState->au32Regs[index] & s_ichIntelHDRegMap[index].readable) & 0xff000000) == 0);
    return hdaRegReadU32(pState, offset, index, pu32Value);
}

DECLCALLBACK(int)hdaRegWriteU24(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    Assert(((u32Value & 0xff000000) == 0));
    return hdaRegWriteU32(pState, offset, index, u32Value);
}
/* U32 */
DECLCALLBACK(int)hdaRegReadU32(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    *pu32Value = pState->au32Regs[index] & s_ichIntelHDRegMap[index].readable;
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegWriteU32(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    pState->au32Regs[index]  = (u32Value & s_ichIntelHDRegMap[index].writable)
                                  | (pState->au32Regs[index] & ~s_ichIntelHDRegMap[index].writable);
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegReadGCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    return hdaRegReadU32(pState, offset, index, pu32Value);
}

DECLCALLBACK(int)hdaRegWriteGCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    if (u32Value & HDA_REG_FIELD_FLAG_MASK(GCTL, RST))
    {
        /* exit reset state */
        GCTL(pState) |= HDA_REG_FIELD_FLAG_MASK(GCTL, RST);
    }
    else
    {
        /* enter reset state*/
        hdaReset(ICH6_HDASTATE_2_DEVINS(pState));
        GCTL(pState) &= ~HDA_REG_FIELD_FLAG_MASK(GCTL, RST);
        //** @todo r=michaln: The device isn't supposed to respond to any writes except to this bit now.
    }
    if (u32Value & HDA_REG_FIELD_FLAG_MASK(GCTL, FSH))
    {
        /* Flush: GSTS:1 set,  see 6.2.6*/
        GSTS(pState) |= HDA_REG_FIELD_FLAG_MASK(GSTS, FSH); /* set the flush state */
        /* DPLBASE and DPUBASE, should be initialized with initial value (see 6.2.6)*/
    }
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegWriteSTATESTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    uint32_t v = pState->au32Regs[index];
    uint32_t nv = u32Value & ICH6_HDA_STATES_SCSF;
    pState->au32Regs[index] = (v ^ nv) & v; /* write of 1 clears corresponding bit */
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegReadINTSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    uint32_t v = INTSTS(pState);
    v &= ~HDA_REG_FIELD_FLAG_MASK(INTSTS, GIS);
    v |= (v ? HDA_REG_FIELD_FLAG_MASK(INTSTS, GIS) : 0);
    *pu32Value = v;
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegWriteINTSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    uint32_t v = INTSTS(pState);
    INTSTS(pState) = (v ^ u32Value) & v;
    return hdaProcessInterrupt(pState);
}

DECLCALLBACK(int)hdaRegReadGCAP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    return hdaRegReadU16(pState, offset, index, pu32Value);
}

DECLCALLBACK(int)hdaRegWriteCORBRP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    if (u32Value & HDA_REG_FIELD_FLAG_MASK(CORBRP, RST))
    {
        pState->u8CORBRP = 0;
        CORBRP(pState) = 0;
    }
    else
        return hdaRegWriteU8(pState, offset, index, u32Value);
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegWriteCORBSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    uint32_t v = CORBSTS(pState);
    v = (v ^ u32Value) & v;
    CORBSTS(pState) = v;
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegWriteCORBWP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    int rc;
    rc = hdaRegWriteU16(pState, offset, index, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);
    if (CORBWP(pState) == CORBRP(pState))
        return VINF_SUCCESS;
    rc = hdaCORBCmdProcess(pState);
    return rc;
}

DECLCALLBACK(int)hdaRegReadSDCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t *pu32Value)
{
    return hdaRegReadU24(pState, offset, index, pu32Value);
}

DECLCALLBACK(int)hdaRegWriteSDCTL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    if((u32Value & HDA_REG_FIELD_FLAG_MASK(SDCTL, SRST)))
    {
        hdaStreamReset(pState, offset);
    }
    /* @todo: use right offsets for right streams */
    if (u32Value & HDA_REG_FIELD_FLAG_MASK(SDCTL, RUN))
    {
        Log(("hda: DMA(%x) switched on\n", offset));
        AUD_set_active_in(pState->Codec.voice_pi, 1);
        AUD_set_active_in(pState->Codec.voice_mc, 1);
        if (offset == 0x100)
        {
            AUD_set_active_out(pState->Codec.voice_po, 1);
            //SDSTS(pState, 4) |= (1<<5);
        }
    }
    else
    {
        Log(("hda: DMA(%x) switched off\n", offset));
        AUD_set_active_in(pState->Codec.voice_pi, 0);
        AUD_set_active_in(pState->Codec.voice_mc, 0);
        if (offset == 0x100)
        {
            SDSTS(pState, 4) &= ~(1<<5);
            AUD_set_active_out(pState->Codec.voice_po, 0);
        }
        SSYNC(pState) &= ~(1<< (offset - 0x80));
    }
    int rc = hdaRegWriteU24(pState, offset, index, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, VINF_SUCCESS);
    return rc;
}

DECLCALLBACK(int)hdaRegWriteSDSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    uint32_t v = HDA_REG_IND(pState, index);
    //int rc = hdaRegWriteU8(pState, offset, index, u32Value);
    switch (offset)
    {
        case 0x83:
            SDSTS(pState, 0) ^= u32Value;
            break;
        case 0xA3:
            SDSTS(pState, 1) ^= u32Value;
            break;
        case 0xC3:
            SDSTS(pState, 2) ^= u32Value;
            break;
        case 0xE3:
            SDSTS(pState, 3) ^= u32Value;
            break;
        case 0x103:
            SDSTS(pState, 4) ^= u32Value;
            break;
        case 0x123:
            SDSTS(pState, 5) ^= u32Value;
            break;
        case 0x143:
            SDSTS(pState, 6) ^= u32Value;
            break;
        case 0x163:
            SDSTS(pState, 7) ^= u32Value;
            break;
    }
    hdaProcessInterrupt(pState);
#if 0
    if (   v != u32Value
        && (INTCTL_SALL(pState) & (1 << ((offset - 0x83) >> 5))))
    {
        int rc;
        rc = hdaProcessInterrupt(pState);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
    }
#endif
    return VINF_SUCCESS; 
}
DECLCALLBACK(int)hdaRegWriteSDLVI(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    int rc = hdaRegWriteU32(pState, offset, index, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, VINF_SUCCESS);
    return rc;
}

DECLCALLBACK(int)hdaRegWriteSDBDPL(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    int rc = hdaRegWriteU32(pState, offset, index, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, VINF_SUCCESS);
    return rc;
}

DECLCALLBACK(int)hdaRegWriteSDBDPU(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    int rc = hdaRegWriteU32(pState, offset, index, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, VINF_SUCCESS);
    return rc;
}

DECLCALLBACK(int)hdaRegWriteIRS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    int rc = VINF_SUCCESS;
    uint64_t resp;
    PFNCODECVERBPROCESSOR pfn = (PFNCODECVERBPROCESSOR)NULL;
    if (   u32Value & HDA_REG_FIELD_FLAG_MASK(IRS, ICB)
        && !IRS_ICB(pState))
    {
        uint32_t cmd = IC(pState);
        IRS(pState) = HDA_REG_FIELD_FLAG_MASK(IRS, ICB);  /* busy */
        Log(("hda: IC:%x\n", cmd));
        rc = pState->Codec.pfnLookup(&pState->Codec, cmd, &pfn);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
        rc = pfn(&pState->Codec, cmd, &resp);
        if (RT_FAILURE(rc))
            AssertRCReturn(rc, rc);
        IR(pState) = (uint32_t)resp;
        Log(("hda: IR:%x\n", IR(pState)));
        IRS(pState) = HDA_REG_FIELD_FLAG_MASK(IRS, IRV);  /* clear busy, result is ready  */
        return rc;
    }
    if (   u32Value & HDA_REG_FIELD_FLAG_MASK(IRS, IRV)
        && IRS_IRV(pState))
        IRS(pState) ^= HDA_REG_FIELD_FLAG_MASK(IRS, IRV);
    return rc;
}

DECLCALLBACK(int)hdaRegWriteRIRBWP(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    if (u32Value & HDA_REG_FIELD_FLAG_MASK(RIRBWP, RST))
    {
        RIRBWP(pState) = 0;
    }
    /*The rest of bits are O, see 6.2.22 */
    return VINF_SUCCESS;
}

DECLCALLBACK(int)hdaRegWriteBase(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    int rc = hdaRegWriteU32(pState, offset, index, u32Value);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);
    switch(index)
    {
        case ICH6_HDA_REG_CORBLBASE:
            pState->u64CORBBase &= 0xFFFFFFFF00000000ULL;
            pState->u64CORBBase |= pState->au32Regs[index];
        break;
        case ICH6_HDA_REG_CORBUBASE:
            pState->u64CORBBase &= 0x00000000FFFFFFFFULL;
            pState->u64CORBBase |= ((uint64_t)pState->au32Regs[index] << 32);
        break;
        case ICH6_HDA_REG_RIRLBASE:
            pState->u64RIRBBase &= 0xFFFFFFFF00000000ULL;
            pState->u64RIRBBase |= pState->au32Regs[index];
        break;
        case ICH6_HDA_REG_RIRUBASE:
            pState->u64RIRBBase &= 0x00000000FFFFFFFFULL;
            pState->u64RIRBBase |= ((uint64_t)pState->au32Regs[index] << 32);
        break;
        case ICH6_HDA_REG_DPLBASE:
            /* @todo: first bit has special meaning */
            pState->u64DPBase &= 0xFFFFFFFF00000000ULL;
            pState->u64DPBase |= pState->au32Regs[index];
        break;
        case ICH6_HDA_REG_DPUBASE:
            pState->u64DPBase &= 0x00000000FFFFFFFFULL;
            pState->u64DPBase |= ((uint64_t)pState->au32Regs[index] << 32);
        break;
        default:
            AssertMsgFailed(("Invalid index"));
    }
    Log(("hda: CORB base:%llx RIRB base: %llx DP base: %llx\n", pState->u64CORBBase, pState->u64RIRBBase, pState->u64DPBase));
    return rc;
}

DECLCALLBACK(int)hdaRegWriteRIRBSTS(INTELHDLinkState* pState, uint32_t offset, uint32_t index, uint32_t u32Value)
{
    uint8_t nv = u32Value;
    uint8_t v = RIRBSTS(pState);
    RIRBSTS(pState) = (v ^ nv) & v;
    
    return hdaProcessInterrupt(pState);
}

static void dump_bd(INTELHDLinkState *pState)
{
    uint64_t addr;
    uint32_t len;
    uint32_t ioc;
    uint8_t  bdle[16];
    uint32_t counter;
    uint32_t i;
    uint32_t sum = 0;
    for (i = 0; i <= SDLVI(pState, 4); ++i)
    {
        PDMDevHlpPhysRead(ICH6_HDASTATE_2_DEVINS(pState), SDBDPL(pState, 4) + i*16, bdle, 16);
        addr = *(uint64_t *)bdle;
        len = *(uint32_t *)&bdle[8];
        ioc = *(uint32_t *)&bdle[12];
        Log(("hda: %s bdle[%d] a:%x, len:%x, ios:%d\n",  (i == pState->u32Cvi? "[C]": "   "), i, addr, len, ioc));
        sum += len;
    }
    Log(("hda: sum: %d\n", sum));
    for (i = 0; i < 8; ++i)
    {
        PDMDevHlpPhysRead(ICH6_HDASTATE_2_DEVINS(pState), pState->u64DPBase + i*8, &counter, 4);
        Log(("hda: %s stream[%d] counter=%x\n", (i) == SDCTL_NUM(pState, 4)? "[C]": "   ", i , counter));
    }
}
static void fetch_bd(INTELHDLinkState *pState)
{
    dump_bd(pState); 
    pState->u32Cvi;
    uint8_t  bdle[16];
    PDMDevHlpPhysRead(ICH6_HDASTATE_2_DEVINS(pState), SDBDPL(pState, 4) + pState->u32Cvi*16, bdle, 16);
    pState->u64CviAddr = *(uint64_t *)bdle;
    pState->u32CviLen = *(uint32_t *)&bdle[8];
    pState->fCviIoc = (*(uint32_t *)&bdle[12]) & 0x1;
}

static uint32_t write_audio(INTELHDLinkState *pState, int avail, bool *fStop)
{
    uint8_t tmpbuf[4096];
    uint32_t temp;
    uint32_t u32Rest;
    uint32_t written = 0;
    int to_copy = 0;
    u32Rest = pState->u32CviLen - pState->u32CviPos;
    temp = audio_MIN(u32Rest, (uint32_t)avail);
    if (!temp)
    {
        *fStop = true;
        return written;
    }
    while (temp)
    {
        int copied;
        to_copy = audio_MIN(temp, 4096U);
        PDMDevHlpPhysRead(ICH6_HDASTATE_2_DEVINS(pState), pState->u64CviAddr + pState->u32CviPos, tmpbuf, to_copy);
        copied = AUD_write (pState->Codec.voice_po, tmpbuf, to_copy);
        Log (("hda: write_audio max=%x to_copy=%x copied=%x\n",
              avail, to_copy, copied));
        Assert((copied));
        if (!copied)
        {
            *fStop = true;
            break;
        }
        temp    -= copied;
        written += copied;
        pState->u32CviPos += written;
    }
    return written;
}

DECLCALLBACK(void) hdaTransfer(CODECState *pCodecState, ENMSOUNDSOURCE src, int avail)
{
    bool fStop = false;
    INTELHDLinkState *pState = (INTELHDLinkState *)pCodecState->pHDAState;
    switch(src)
    {
        case PO_INDEX:
        {
            uint32_t written;
            uint32_t u32Counter;
            if (   !(SDCTL(pState, 4) & HDA_REG_FIELD_FLAG_MASK(SDCTL, RUN))
                || avail == 0)
                return;
            SDCTL(pState, 4) |= ((pState->Codec.pNodes[2].adc.u32F06_param & (0x5 << 4)) >> 4) << 20;
            fetch_bd(pState);
            while(   avail
                  && !fStop)
            {
                PDMDevHlpPhysRead(ICH6_HDASTATE_2_DEVINS(pState), (pState->u64DPBase & ~0x1) + 4*8, &u32Counter, 4);
                written = write_audio(pState, avail, &fStop);
                if (fStop)
                    break;
                SDLPIB(pState, 4) += written; /* bytes ? */
                avail -= written;
                u32Counter += written;
                PDMDevHlpPhysWrite(ICH6_HDASTATE_2_DEVINS(pState), (pState->u64DPBase & ~0x1) + 4*8, &u32Counter, 4);
                if (pState->u32CviPos == pState->u32CviLen
                    || SDLPIB(pState, 4) == SDLCBL(pState, 4))
                {
                    if (   SDCTL(pState, 4) & HDA_REG_FIELD_FLAG_MASK(SDCTL, ICE)
                        && (   pState->u32CviPos == pState->u32CviLen
                            || SDLPIB(pState, 4) == SDLCBL(pState, 4)))
                    {
                        SDSTS(pState,4) |= HDA_REG_FIELD_FLAG_MASK(SDSTS, BCIS);
                        INTSTS(pState) |= HDA_REG_FIELD_FLAG_MASK(INTSTS, S4);
                        hdaProcessInterrupt(pState);
                        if (SDLPIB(pState, 4) == SDLCBL(pState, 4))
                        {
                            SDLPIB(pState, 4) = 0;
                            u32Counter = 0;
                            PDMDevHlpPhysWrite(ICH6_HDASTATE_2_DEVINS(pState), (pState->u64DPBase & ~0x1)  + 4*8, &u32Counter, 4);
                        }
                        if (pState->u32CviPos == pState->u32CviLen)
                        { 
                            pState->u32CviPos = 0;
                            pState->u32Cvi++;
                            if (pState->u32Cvi == SDLVI(pState, 4) + 1)
                                pState->u32Cvi = 0;
                        }
                     }
                }
                fetch_bd(pState);
            }
        }
        break;
        AssertMsgFailed(("Unexpected index: %x\n", src));
        default:
            break;
    }
}

/**
 * Handle register read operation.
 *
 * Looks up and calls appropriate handler.
 *
 * @note: while implementation was detected so called "forgotten" or "hole" registers
 * which description is missed in RPM, datasheet or spec. 
 *
 * @returns VBox status code.
 *
 * @param   pState      The device state structure.
 * @param   uOffset     Register offset in memory-mapped frame.
 * @param   pv          Where to fetch the value.
 * @param   cb          Number of bytes to write.
 * @thread  EMT
 */
PDMBOTHCBDECL(int) hdaMMIORead(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    int rc = VINF_SUCCESS;
    PCIINTELHDLinkState *pThis = PDMINS_2_DATA(pDevIns, PCIINTELHDLinkState *);
    uint32_t  u32Offset = GCPhysAddr - pThis->hda.addrMMReg;
    int index = hdaLookup(&pThis->hda, u32Offset);
    Assert(   index != -1 
           && u32Offset == s_ichIntelHDRegMap[index].offset 
           && cb <= 4);
    if (index != -1)
    {
        uint32_t mask = 0;
        uint32_t v = 0;
        switch(cb)
        {
            case 1: mask = 0x000000ff; break; 
            case 2: mask = 0x0000ffff; break; 
            case 3: mask = 0x00ffffff; break; 
            case 4: mask = 0xffffffff; break; 
        }
        Assert(u32Offset == s_ichIntelHDRegMap[index].offset);
        rc = s_ichIntelHDRegMap[index].pfnRead(&pThis->hda, u32Offset, index, &v);
        *(uint32_t *)pv = v & mask;
        Log(("hda: read %s[%x/%x]\n", s_ichIntelHDRegMap[index].abbrev, v, *(uint32_t *)pv));
        return rc;
    }
    *(uint32_t *)pv = 0xFF;
    Log(("hda: hole at %X is accessed for read\n", u32Offset));
    return rc;
}

/**
 * Handle register write operation.
 *
 * Looks up and calls appropriate handler.
 *
 * @returns VBox status code.
 *
 * @param   pState      The device state structure.
 * @param   uOffset     Register offset in memory-mapped frame.
 * @param   pv          Where to fetch the value.
 * @param   cb          Number of bytes to write.
 * @thread  EMT
 */
PDMBOTHCBDECL(int) hdaMMIOWrite(PPDMDEVINS pDevIns, void *pvUser, RTGCPHYS GCPhysAddr, void *pv, unsigned cb)
{
    int rc = VINF_SUCCESS;
    PCIINTELHDLinkState *pThis = PDMINS_2_DATA(pDevIns, PCIINTELHDLinkState *);
    uint32_t  u32Offset = GCPhysAddr - pThis->hda.addrMMReg;
    int index = hdaLookup(&pThis->hda, u32Offset);
    Assert(   index != -1 
           && u32Offset == s_ichIntelHDRegMap[index].offset 
           && cb <= 4);
    if (index != -1)
    {
        Assert(u32Offset == s_ichIntelHDRegMap[index].offset);
        uint32_t v = pThis->hda.au32Regs[index];
        uint32_t mask = 0;
        switch(cb)
        {
            case 1: mask = 0xffffff00; break; 
            case 2: mask = 0xffff0000; break; 
            case 3: mask = 0xff000000; break; 
            case 4: mask = 0x00000000; break; 
        }
        *(uint32_t *)pv = (v & mask) | (*(uint32_t *)pv & ~mask); 
        rc = s_ichIntelHDRegMap[index].pfnWrite(&pThis->hda, u32Offset, index, *(uint32_t *)pv);
        Log(("hda: write %s:(%x) %x => %x\n", s_ichIntelHDRegMap[index].abbrev, *(uint32_t *)pv, v, pThis->hda.au32Regs[index]));
        return rc;
    }
    Log(("hda: hole at %X is accessed for write\n", u32Offset));
    return rc;
}

/**
 * Callback function for mapping a PCI I/O region.
 *
 * @return VBox status code.
 * @param   pPciDev         Pointer to PCI device.
 *                          Use pPciDev->pDevIns to get the device instance.
 * @param   iRegion         The region number.
 * @param   GCPhysAddress   Physical address of the region.
 *                          If iType is PCI_ADDRESS_SPACE_IO, this is an
 *                          I/O port, else it's a physical address.
 *                          This address is *NOT* relative
 *                          to pci_mem_base like earlier!
 * @param   enmType         One of the PCI_ADDRESS_SPACE_* values.
 */
static DECLCALLBACK(int) hdaMap (PPCIDEVICE pPciDev, int iRegion,
                                           RTGCPHYS GCPhysAddress, uint32_t cb,
                                           PCIADDRESSSPACE enmType)
{
    int         rc;
    PPDMDEVINS  pDevIns = pPciDev->pDevIns;
    RTIOPORT    Port = (RTIOPORT)GCPhysAddress;
    PCIINTELHDLinkState *pThis = PCIDEV_2_ICH6_HDASTATE(pPciDev);

    Assert(enmType == PCI_ADDRESS_SPACE_MEM);
    rc = PDMDevHlpMMIORegister(pPciDev->pDevIns, GCPhysAddress, cb, 0,
                               hdaMMIOWrite, hdaMMIORead, NULL, "ICH6_HDA");

    if (RT_FAILURE(rc))
        return rc;

    pThis->hda.addrMMReg = GCPhysAddress;
    return VINF_SUCCESS;
}


/**
 * Reset notification.
 *
 * @returns VBox status.
 * @param   pDevIns     The device instance data.
 *
 * @remark  The original sources didn't install a reset handler, but it seems to
 *          make sense to me so we'll do it.
 */
static DECLCALLBACK(void)  hdaReset (PPDMDEVINS pDevIns)
{
    PCIINTELHDLinkState *pThis = PDMINS_2_DATA(pDevIns, PCIINTELHDLinkState *);
    GCAP(&pThis->hda) = 0x4401; /* see 6.2.1 */
    VMIN(&pThis->hda) = 0x00;       /* see 6.2.2 */
    VMAJ(&pThis->hda) = 0x01;       /* see 6.2.3 */
    VMAJ(&pThis->hda) = 0x01;       /* see 6.2.3 */
    OUTPAY(&pThis->hda) = 0x003C;   /* see 6.2.4 */
    INPAY(&pThis->hda)  = 0x001D;   /* see 6.2.5 */
    pThis->hda.au32Regs[ICH6_HDA_REG_CORBSIZE] = 0x42; /* see 6.2.1 */
    pThis->hda.au32Regs[ICH6_HDA_REG_RIRBSIZE] = 0x42; /* see 6.2.1 */
    STATES(&pThis->hda) = 0x5;
    CORBRP(&pThis->hda) = 0x0;
    RIRBWP(&pThis->hda) = 0x0;

    LogRel(("hda: inter HDA reset.\n"));
    //** @todo r=michaln: There should be LogRel statements when the guest initializes
    // or resets the HDA chip, and possibly also when opening the PCM streams.
    pThis->hda.cdwCorbBuf = CORBSIZE(&pThis->hda);
    pThis->hda.cbCorbBuf = CORBSIZE(&pThis->hda) * sizeof(uint32_t);

    if (pThis->hda.pu32CorbBuf)
        memset(pThis->hda.pu32CorbBuf, 0, pThis->hda.cbCorbBuf);
    else
        pThis->hda.pu32CorbBuf = (uint32_t *)RTMemAllocZ(pThis->hda.cbCorbBuf);

    pThis->hda.cdqRirbBuf = RIRBSIZE(&pThis->hda);
    pThis->hda.cbRirbBuf = RIRBSIZE(&pThis->hda) * sizeof(uint64_t);
    if (pThis->hda.pu64RirbBuf)
        memset(pThis->hda.pu64RirbBuf, 0, pThis->hda.cbRirbBuf);
    else
        pThis->hda.pu64RirbBuf = (uint64_t *)RTMemAllocZ(pThis->hda.cbRirbBuf);

    /* Accoding to ICH6 datasheet, 0x40000 is default value for stream descriptor register 23:20 
     * bits are reserved for stream number 18.2.33 */
    SDCTL(&pThis->hda, 0) = 0x40000;
    SDCTL(&pThis->hda, 1) = 0x40000;
    SDCTL(&pThis->hda, 2) = 0x40000;
    SDCTL(&pThis->hda, 3) = 0x40000;
    SDCTL(&pThis->hda, 4) = 0x40000;
    SDCTL(&pThis->hda, 5) = 0x40000;
    SDCTL(&pThis->hda, 6) = 0x40000;
    SDCTL(&pThis->hda, 7) = 0x40000;

    /* ICH6 defines default values (0x77 for input and 0xBF for output descriptors) of FIFO size. 18.2.39 */
    SDFIFOS(&pThis->hda, 0) = 0x77;
    SDFIFOS(&pThis->hda, 1) = 0x77;
    SDFIFOS(&pThis->hda, 2) = 0x77;
    SDFIFOS(&pThis->hda, 3) = 0x77;
    SDFIFOS(&pThis->hda, 4) = 0xBF;
    SDFIFOS(&pThis->hda, 5) = 0xBF;
    SDFIFOS(&pThis->hda, 6) = 0xBF;
    SDFIFOS(&pThis->hda, 7) = 0xBF;
    

    Log(("hda: reset finished\n"));
}

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) hdaQueryInterface (struct PDMIBASE *pInterface,
                                                   const char *pszIID)
{
    PCIINTELHDLinkState *pThis = RT_FROM_MEMBER(pInterface, PCIINTELHDLinkState, hda.IBase);
    Assert(&pThis->hda.IBase == pInterface);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pThis->hda.IBase);
    return NULL;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) hdaConstruct (PPDMDEVINS pDevIns, int iInstance,
                                           PCFGMNODE pCfgHandle)
{
    PCIINTELHDLinkState *pThis = PDMINS_2_DATA(pDevIns, PCIINTELHDLinkState *);
    INTELHDLinkState    *s     = &pThis->hda;
    int               rc;

    Assert(iInstance == 0);
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);

    /*
     * Validations.
     */
    if (!CFGMR3AreValuesValid (pCfgHandle, "\0"))
        return PDMDEV_SET_ERROR (pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                 N_ ("Invalid configuration for the INTELHD device"));

    // ** @todo r=michaln: This device may need R0/RC enabling, especially if guests
    // poll some register(s).

    /*
     * Initialize data (most of it anyway).
     */
    s->pDevIns                  = pDevIns;
    /* IBase */
    s->IBase.pfnQueryInterface  = hdaQueryInterface;

    /* PCI Device (the assertions will be removed later) */
    PCIDevSetVendorId           (&pThis->dev, 0x8086); /* 00 ro - intel. */
    PCIDevSetDeviceId           (&pThis->dev, 0x2668); /* 02 ro - 82801 / 82801aa(?). */
    PCIDevSetCommand            (&pThis->dev, 0x0000); /* 04 rw,ro - pcicmd. */
    PCIDevSetStatus             (&pThis->dev, 0x0010); /* 06 rwc?,ro? - pcists. */
    PCIDevSetRevisionId         (&pThis->dev, 0x01);   /* 08 ro - rid. */
    PCIDevSetClassProg          (&pThis->dev, 0x00);   /* 09 ro - pi. */
    PCIDevSetClassSub           (&pThis->dev, 0x03);   /* 0a ro - scc; 01 == Audio. */
    PCIDevSetClassBase          (&pThis->dev, 0x04);   /* 0b ro - bcc; 04 == multimedia. */
    PCIDevSetHeaderType         (&pThis->dev, 0x00);   /* 0e ro - headtyp. */
    PCIDevSetBaseAddress        (&pThis->dev, 0,       /* 10 rw - MMIO */
                                 false /* fIoSpace */, false /* fPrefetchable */, true /* f64Bit */, 0x00000000);
    /* ICH6 datasheet defines 0 values for SVID and SID (18.1.14-15), which together with values returned for 
       verb F20 should provide device/codec recognition. */
    PCIDevSetSubSystemVendorId  (&pThis->dev, 0x0000); /* 2c ro - intel.) */
    PCIDevSetSubSystemId        (&pThis->dev, 0x0000); /* 2e ro. */
    PCIDevSetInterruptLine      (&pThis->dev, 0x00);   /* 3c rw. */
    PCIDevSetInterruptPin       (&pThis->dev, 0x01);   /* 3d ro - INTA#. */             Assert (pThis->dev.config[0x3d] == 0x01);
    PCIDevSetCapabilityList(&pThis->dev, 0x50); /* ICH6 datasheet 18.1.16 */

    //** @todo r=michaln: If there are really no PCIDevSetXx for these, the meaning
    // of these values needs to be properly documented! 
    /* HDCTL off 0x40 bit 0 selects signaling mode (1-HDA, 0 - Ac97) 18.1.19 */
    pThis->dev.config[0x40] = 0x01;

    pThis->dev.config[0x50] = 0x01;
    pThis->dev.config[0x51] = 0x60; /* next */
    pThis->dev.config[0x52] = 0x22;
    pThis->dev.config[0x53] = 0x00; /* PM - disabled,  */

    pThis->dev.config[0x60] = 0x05;
    pThis->dev.config[0x61] = 0x70; /* next */
    pThis->dev.config[0x62] = 0x00;
    pThis->dev.config[0x63] = 0x80;

    /*
     * Register the PCI device.
     */
    rc = PDMDevHlpPCIRegister (pDevIns, &pThis->dev);
    if (RT_FAILURE (rc))
        return rc;

    rc = PDMDevHlpPCIIORegionRegister (pDevIns, 0, 0x4000, PCI_ADDRESS_SPACE_MEM,
                                       hdaMap);
    if (RT_FAILURE (rc))
        return rc;

    /*
     * Attach driver.
     */
    rc = PDMDevHlpDriverAttach (pDevIns, 0, &s->IBase,
                                &s->pDrvBase, "Audio Driver Port");
    if (rc == VERR_PDM_NO_ATTACHED_DRIVER)
        Log (("hda: No attached driver!\n"));
    else if (RT_FAILURE (rc))
    {
        AssertMsgFailed (("Failed to attach INTELHD LUN #0! rc=%Rrc\n", rc));
        return rc;
    }



    pThis->hda.Codec.pHDAState = (void *)&pThis->hda;
    rc = stac9220Construct(&pThis->hda.Codec);
    if (RT_FAILURE(rc))
        AssertRCReturn(rc, rc);
    hdaReset (pDevIns);
    pThis->hda.Codec.pfnTransfer = hdaTransfer;

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) hdaDestruct (PPDMDEVINS pDevIns)
{
    PCIINTELHDLinkState *pThis = PDMINS_2_DATA(pDevIns, PCIINTELHDLinkState *);
    
    int rc = stac9220Destruct(&pThis->hda.Codec);
    AssertRC(rc);
    if (pThis->hda.pu32CorbBuf)
        RTMemFree(pThis->hda.pu32CorbBuf);
    if (pThis->hda.pu32CorbBuf)
        RTMemFree(pThis->hda.pu32CorbBuf);
    return VINF_SUCCESS;
}

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceICH6_HDA =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szName */
    "hda",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "ICH IntelHD Audio Controller",
    /* fFlags */
    PDM_DEVREG_FLAGS_DEFAULT_BITS,
    /* fClass */
    PDM_DEVREG_CLASS_AUDIO,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(PCIINTELHDLinkState),
    /* pfnConstruct */
    hdaConstruct,
    /* pfnDestruct */
    hdaDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    hdaReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete */
    NULL,
    /* pfnPowerOff */
    NULL,
    /* pfnSoftReset */
    NULL,
    /* u32VersionEnd */
    PDM_DEVREG_VERSION
};

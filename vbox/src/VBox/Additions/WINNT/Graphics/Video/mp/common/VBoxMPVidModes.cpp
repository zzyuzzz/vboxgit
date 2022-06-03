/* $Id: VBoxMPVidModes.cpp 37165 2011-05-20 13:22:01Z vboxsync $ */

/** @file
 * VBox Miniport video modes related functions
 */

/*
 * Copyright (C) 2011 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include "VBoxMPCommon.h"

#if _MSC_VER >= 1400 /* bird: MS fixed swprintf to be standard-conforming... */
#define _INC_SWPRINTF_INL_
extern "C" int __cdecl swprintf(wchar_t *, const wchar_t *, ...);
#endif
#include <wchar.h>

/* Custom video modes which are being read from registry at driver startup. */
static VIDEO_MODE_INFORMATION g_CustomVideoModes[64] = { 0 };

/* Standart video modes list.
 * Additional space is reserved for custom video modes for 64 guest monitors.
 * The custom video mode index is alternating and 2 indexes are reserved for the last custom mode.
 */
static VIDEO_MODE_INFORMATION g_VideoModes[VBOXMP_MAX_VIDEO_MODES + 64 + 2] = { 0 };

/* Number of available video modes, set by VBoxMPCmnBuildVideoModesTable. */
static uint32_t g_NumVideoModes = 0;

/* Fills given video mode BPP related fields */
static void 
VBoxFillVidModeBPP(VIDEO_MODE_INFORMATION *pMode, ULONG bitsR, ULONG bitsG, ULONG bitsB, 
                   ULONG maskR, ULONG maskG, ULONG maskB)
{
    pMode->NumberRedBits   = bitsR;
    pMode->NumberGreenBits = bitsG;
    pMode->NumberBlueBits  = bitsB;
    pMode->RedMask         = maskR;
    pMode->GreenMask       = maskG;
    pMode->BlueMask        = maskB;
}

/* Fills given video mode structure */
static void 
VBoxFillVidModeInfo(VIDEO_MODE_INFORMATION *pMode, ULONG xres, ULONG yres, ULONG bpp, ULONG index, ULONG yoffset)
{
    LOGF(("%dx%d:%d (idx=%d, yoffset=%d)", xres, yres, bpp, index, yoffset));

    memset(pMode, 0, sizeof(VIDEO_MODE_INFORMATION));

    /*Common entries*/
    pMode->Length                       = sizeof(VIDEO_MODE_INFORMATION);
    pMode->ModeIndex                    = index;
    pMode->VisScreenWidth               = xres;
    pMode->VisScreenHeight              = yres - yoffset;
    pMode->ScreenStride                 = xres * ((bpp + 7) / 8);
    pMode->NumberOfPlanes               = 1;
    pMode->BitsPerPlane                 = bpp;
    pMode->Frequency                    = 60;
    pMode->XMillimeter                  = 320;
    pMode->YMillimeter                  = 240;
    pMode->VideoMemoryBitmapWidth       = xres;
    pMode->VideoMemoryBitmapHeight      = yres - yoffset;
    pMode->DriverSpecificAttributeFlags = 0;
    pMode->AttributeFlags               = VIDEO_MODE_GRAPHICS | VIDEO_MODE_COLOR | VIDEO_MODE_NO_OFF_SCREEN;

    /*BPP related entries*/
    switch (bpp)
    {
#ifdef VBOX_WITH_8BPP_MODES
        case 8:
            VBoxFillVidModeBPP(pMode, 6, 6, 6, 0, 0, 0);

            pMode->AttributeFlags |= VIDEO_MODE_PALETTE_DRIVEN | VIDEO_MODE_MANAGED_PALETTE;
            break;
#endif
        case 16:
            VBoxFillVidModeBPP(pMode, 5, 6, 5, 0xF800, 0x7E0, 0x1F);
            break;
        case 24:
        case 32:
            VBoxFillVidModeBPP(pMode, 8, 8, 8, 0xFF0000, 0xFF00, 0xFF);
            break;
        default:
            Assert(0);
            break;
    }
}

void VBoxMPCmnInitCustomVideoModes(PVBOXMP_DEVEXT pExt)
{
    VBOXMPCMNREGISTRY Registry;
    VP_STATUS rc;
    int iMode;

    LOGF_ENTER();

    rc = VBoxMPCmnRegInit(pExt, &Registry);
    VBOXMP_WARN_VPS(rc);

    /* Initialize all custom modes to the 800x600x32 */
    VBoxFillVidModeInfo(&g_CustomVideoModes[0], 800, 600, 32, 0, 0);
    for (iMode=1; iMode<RT_ELEMENTS(g_CustomVideoModes); ++iMode)
    {
        g_CustomVideoModes[iMode] = g_CustomVideoModes[0];
    }

    /* Read stored custom resolution info from registry */
    for (iMode=0; iMode<VBoxCommonFromDeviceExt(pExt)->cDisplays; ++iMode)
    {
        uint32_t CustomXRes = 0, CustomYRes = 0, CustomBPP = 0;

        if (iMode==0)
        {
            /*First name without a suffix*/
            rc = VBoxMPCmnRegQueryDword(Registry, L"CustomXRes", &CustomXRes);
            VBOXMP_WARN_VPS(rc);
            rc = VBoxMPCmnRegQueryDword(Registry, L"CustomYRes", &CustomYRes);
            VBOXMP_WARN_VPS(rc);
            rc = VBoxMPCmnRegQueryDword(Registry, L"CustomBPP", &CustomBPP);
            VBOXMP_WARN_VPS(rc);
        }
        else
        {
            wchar_t keyname[32];
            swprintf(keyname, L"CustomXRes%d", iMode);
            rc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomXRes);
            VBOXMP_WARN_VPS(rc);
            swprintf(keyname, L"CustomYRes%d", iMode);
            rc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomYRes);
            VBOXMP_WARN_VPS(rc);
            swprintf(keyname, L"CustomBPP%d", iMode);
            rc = VBoxMPCmnRegQueryDword(Registry, keyname, &CustomBPP);
            VBOXMP_WARN_VPS(rc);
        }

        LOG(("got stored custom resolution[%d] %dx%dx%d", iMode, CustomXRes, CustomYRes, CustomBPP));

        if (CustomXRes || CustomYRes || CustomBPP)
        {
            if (CustomXRes == 0)
            {
                CustomXRes = g_CustomVideoModes[iMode].VisScreenWidth;
            }
            if (CustomYRes == 0)
            {
                CustomYRes = g_CustomVideoModes[iMode].VisScreenHeight;
            }
            if (CustomBPP == 0)
            {
                CustomBPP = g_CustomVideoModes[iMode].BitsPerPlane;
            }

            VBoxFillVidModeInfo(&g_CustomVideoModes[iMode], CustomXRes, CustomYRes, CustomBPP, 0, 0);
        }
    }

    rc = VBoxMPCmnRegFini(Registry);
    VBOXMP_WARN_VPS(rc);
    LOGF_LEAVE();
}

VIDEO_MODE_INFORMATION *VBoxMPCmnGetCustomVideoModeInfo(ULONG ulIndex)
{
    return (ulIndex<RT_ELEMENTS(g_CustomVideoModes)) ? &g_CustomVideoModes[ulIndex] : NULL;
}

VIDEO_MODE_INFORMATION* VBoxMPCmnGetVideoModeInfo(ULONG ulIndex)
{
    return (ulIndex<RT_ELEMENTS(g_VideoModes)) ? &g_VideoModes[ulIndex] : NULL;
}

static bool VBoxMPVideoModesMatch(const PVIDEO_MODE_INFORMATION pMode1, const PVIDEO_MODE_INFORMATION pMode2)
{
    return pMode1->VisScreenHeight == pMode2->VisScreenHeight
           && pMode1->VisScreenWidth == pMode2->VisScreenWidth
           && pMode1->BitsPerPlane == pMode2->BitsPerPlane;
}

static int
VBoxMPFindVideoMode(const PVIDEO_MODE_INFORMATION pModesTable, int cModes, const PVIDEO_MODE_INFORMATION pMode)
{
    for (int i = 0; i < cModes; ++i)
    {
        if (VBoxMPVideoModesMatch(pMode, &pModesTable[i]))
        {
            return i;
        }
    }
    return -1;
}

/* Helper function to dynamically build our table of standard video
 * modes. We take the amount of VRAM and create modes with standard
 * geometries until we've either reached the maximum number of modes
 * or the available VRAM does not allow for additional modes.
 * We also check registry for manually added video modes.
 * Returns number of modes added to the table.
 */
static uint32_t
VBoxMPFillModesTable(PVBOXMP_DEVEXT pExt, int iDisplay, PVIDEO_MODE_INFORMATION pModesTable, size_t tableSize,
                     int32_t *pPrefModeIdx)
{
    /* the resolution matrix */
    struct
    {
        uint16_t xRes;
        uint16_t yRes;
    } resolutionMatrix[] =
    {
        /* standard modes */
        { 640,   480 },
        { 800,   600 },
        { 1024,  768 },
        { 1152,  864 },
        { 1280,  960 },
        { 1280, 1024 },
        { 1400, 1050 },
        { 1600, 1200 },
        { 1920, 1440 },
#ifndef VBOX_WITH_WDDM
        /* multi screen modes with 1280x1024 */
        { 2560, 1024 },
        { 3840, 1024 },
        { 5120, 1024 },
        /* multi screen modes with 1600x1200 */
        { 3200, 1200 },
        { 4800, 1200 },
        { 6400, 1200 },
#endif
    };

#ifdef VBOX_XPDM_MINIPORT
    ULONG vramSize = pExt->pPrimary->u.primary.ulMaxFrameBufferSize;
#else
    ULONG vramSize = vboxWddmVramCpuVisibleSegmentSize(pExt);
    /* at least two surfaces will be needed: primary & shadow */
    vramSize /= 2 * pExt->u.primary.commonInfo.cDisplays;
#endif

    uint32_t iMode=0, iPrefIdx=0;
    /* there are 4 color depths: 8, 16, 24 and 32bpp and we reserve 50% of the modes for other sources */
    size_t   maxModesPerColorDepth = VBOXMP_MAX_VIDEO_MODES / 2 / 4;

    /* Always add 800x600 video modes. Windows XP+ needs at least 800x600 resolution
     * and fallbacks to 800x600x4bpp VGA mode if the driver did not report suitable modes.
     * This resolution could be rejected by a low resolution host (netbooks, etc).
     */
#ifdef VBOX_WITH_8BPP_MODES
    int bytesPerPixel=1;
#else
    int bytesPerPixel=2;
#endif
    for (; bytesPerPixel<=4; bytesPerPixel++)
    {
        int bitsPerPixel = 8*bytesPerPixel;

        if (800*600*bytesPerPixel > (LONG)vramSize)
        {
            /* we don't have enough VRAM for this mode */
            continue;
        }

        VBoxFillVidModeInfo(&pModesTable[iMode], 800, 600, bitsPerPixel, iMode+1, 0);

        if (32==bitsPerPixel)
        {
            iPrefIdx = iMode;
        }
        ++iMode;
    }

    /* Query yoffset from the host */
    ULONG yOffset = VBoxGetHeightReduction();

    /* Iterate through our static resolution table and add supported video modes for different bpp's */
#ifdef VBOX_WITH_8BPP_MODES
    bytesPerPixel=1;
#else
    bytesPerPixel=2;
#endif
    for (; bytesPerPixel<=4; bytesPerPixel++)
    {
        int bitsPerPixel = 8*bytesPerPixel;
        size_t cAdded, resIndex;

        for (cAdded=0, resIndex=0; resIndex<RT_ELEMENTS(resolutionMatrix) && cAdded<maxModesPerColorDepth; resIndex++)
        {
            if (resolutionMatrix[resIndex].xRes * resolutionMatrix[resIndex].yRes * bytesPerPixel > (LONG)vramSize)
            {
                /* we don't have enough VRAM for this mode */
                continue;
            }

            if (yOffset == 0 && resolutionMatrix[resIndex].xRes == 800 && resolutionMatrix[resIndex].yRes == 600)
            {
                /* this mode was already added */
                continue;
            }

            if (!VBoxLikesVideoMode(iDisplay, resolutionMatrix[resIndex].xRes, resolutionMatrix[resIndex].yRes - yOffset, bitsPerPixel))
            {
                /* host doesn't like this mode */
                continue;
            }

            /* Sanity check, we shouldn't ever get here */
            if (iMode >= tableSize)
            {
                WARN(("video modes table overflow!"));
                break;
            }

            VBoxFillVidModeInfo(&pModesTable[iMode], resolutionMatrix[resIndex].xRes, resolutionMatrix[resIndex].yRes, bitsPerPixel, iMode+1, yOffset);
            ++iMode;
            ++cAdded;
        }
    }

    /* Check registry for manually added modes, up to 128 entries is supported 
     * Give up on the first error encountered.
     */
    VBOXMPCMNREGISTRY Registry;
    int fPrefSet=0;
    VP_STATUS rc;

    rc = VBoxMPCmnRegInit(pExt, &Registry);
    VBOXMP_WARN_VPS(rc);

    for (int curKey=0; curKey<128; curKey++)
    {
        if (iMode>=tableSize)
        {
            WARN(("ignoring possible custom mode(s), table is full!"));
            break;
        }

        wchar_t keyname[24];
        uint32_t xres, yres, bpp = 0;

        swprintf(keyname, L"CustomMode%dWidth", curKey);
        rc = VBoxMPCmnRegQueryDword(Registry, keyname, &xres);
        VBOXMP_CHECK_VPS_BREAK(rc);

        swprintf(keyname, L"CustomMode%dHeight", curKey);
        rc = VBoxMPCmnRegQueryDword(Registry, keyname, &yres);
        VBOXMP_CHECK_VPS_BREAK(rc);

        swprintf(keyname, L"CustomMode%dBPP", curKey);
        rc = VBoxMPCmnRegQueryDword(Registry, keyname, &bpp);
        VBOXMP_CHECK_VPS_BREAK(rc);

        LOG(("got custom mode[%u]=%ux%u:%u", curKey, xres, yres, bpp));

        /* round down width to be a multiple of 8 if necessary */
        if (!pExt->fAnyX)
        {
            xres &= 0xFFF8;
        }

        if (   (xres > (1 << 16))
            || (yres > (1 << 16))
            || (   (bpp != 16)
                && (bpp != 24)
                && (bpp != 32)))
        {
            /* incorrect values */
            break;
        }

        /* does it fit within our VRAM? */
        if (xres * yres * (bpp / 8) > vramSize)
        {
            /* we don't have enough VRAM for this mode */
            break;
        }

        if (!VBoxLikesVideoMode(iDisplay, xres, yres, bpp))
        {
            /* host doesn't like this mode */
            break;
        }

        LOG(("adding video mode from registry."));

        VBoxFillVidModeInfo(&pModesTable[iMode], xres, yres, bpp, iMode+1, yOffset);

        if (!fPrefSet)
        {
            fPrefSet = 1;
            iPrefIdx = iMode;
        }
#ifdef VBOX_WDDM_MINIPORT
        /*check if the same mode has been added to the table already*/
        int foundIdx = VBoxMPFindVideoMode(pModesTable, iMode, &pModesTable[iMode]);

        if (foundIdx>=0)
        {
            if (iPrefIdx==iMode)
            {
                iPrefIdx=foundIdx;
            }
        }
        else
#endif
        {
            ++iMode;
        }
    }

    rc = VBoxMPCmnRegFini(Registry);
    VBOXMP_WARN_VPS(rc);

    if (pPrefModeIdx)
    {
        *pPrefModeIdx = iPrefIdx;
    }

    return iMode;
}

/* Returns if we're in the first mode change, ie doesn't have valid video mode set yet */
static BOOLEAN VBoxMPIsStartingUp(PVBOXMP_DEVEXT pExt)
{
#ifdef VBOX_XPDM_MINIPORT
    return (pExt->CurrentMode == 0);
#else
    return (!VBoxCommonFromDeviceExt(pExt)->cDisplays || !pExt->aSources[0].pPrimaryAllocation);
#endif
}

/* Updates missing video mode params with current values,
 * Checks if resulting mode is liked by the host and fits into VRAM.
 * Returns TRUE if resulting mode could be used.
 */
static BOOLEAN
VBoxMPValidateVideoModeParams(PVBOXMP_DEVEXT pExt, uint32_t iDisplay, uint32_t &xres, uint32_t &yres, uint32_t &bpp)
{
    /* Make sure all important video mode values are set */
    if (VBoxMPIsStartingUp(pExt))
    {
        /* Use stored custom values only if nothing was read from host. */
        xres = xres ? xres:g_CustomVideoModes[iDisplay].VisScreenWidth;
        yres = yres ? yres:g_CustomVideoModes[iDisplay].VisScreenHeight;
        bpp  = bpp  ? bpp :g_CustomVideoModes[iDisplay].BitsPerPlane;
    }
    else
    {
        /* Use current values for field which weren't read from host. */
#ifdef VBOX_XPDM_MINIPORT
        xres = xres ? xres:pExt->CurrentModeWidth;
        yres = yres ? yres:pExt->CurrentModeHeight;
        bpp  = bpp  ? bpp :pExt->CurrentModeBPP;
#else
        xres = xres ? xres:pExt->aSources[0].pPrimaryAllocation->SurfDesc.width;
        yres = yres ? yres:pExt->aSources[0].pPrimaryAllocation->SurfDesc.height;
        bpp  = bpp  ? bpp :pExt->aSources[0].pPrimaryAllocation->SurfDesc.bpp;
#endif
    }

    /* Round down width to be a multiple of 8 if necessary */
    if (!pExt->fAnyX)
    {
        xres &= 0xFFF8;
    }

    /* We always need bpp to be set */
    if (!bpp)
    {
        bpp=32;
    }

    /* Check if host likes this mode */
    if (!VBoxLikesVideoMode(iDisplay, xres, yres, bpp))
    {
        WARN(("host does not like special mode %dx%d:%d for display %d", xres, yres, bpp, iDisplay));
        return FALSE;
    }

#ifdef VBOX_XPDM_MINIPORT
    ULONG vramSize = pExt->pPrimary->u.primary.ulMaxFrameBufferSize;
#else
    ULONG vramSize = vboxWddmVramCpuVisibleSegmentSize(pExt);
    /* at least two surfaces will be needed: primary & shadow */
    vramSize /= 2 * pExt->u.primary.commonInfo.cDisplays;
#endif

    /* Check that values are valid and mode fits into VRAM */
    if (!xres || !yres
        || !((bpp == 16)
#ifdef VBOX_WITH_8BPP_MODES
             || (bpp == 8)
#endif
             || (bpp == 24)
             || (bpp == 32)))
    {
        LOG(("invalid params for special mode %dx%d:%d", xres, yres, bpp));
        return FALSE;
    }



    if ((xres * yres * (bpp / 8) >= vramSize))
    {
        /* Store values of last reported release log message to avoid log flooding. */
        static uint32_t s_xresNoVRAM=0, s_yresNoVRAM=0, s_bppNoVRAM=0;

        LOG(("not enough VRAM for video mode %dx%dx%dbpp. Available: %d bytes. Required: more than %d bytes.",
             xres, yres, bpp, vramSize, xres * yres * (bpp / 8)));

        s_xresNoVRAM = xres;
        s_yresNoVRAM = yres;
        s_bppNoVRAM = bpp;

        return FALSE;
    }

    return TRUE;
}

/* Checks if there's a pending video mode change hint,
 * and fills pPendingMode with associated info.
 * returns TRUE if there's a pending change. Otherwise returns FALSE.
 */
static BOOLEAN
VBoxMPCheckPendingVideoMode(PVBOXMP_DEVEXT pExt, PVIDEO_MODE_INFORMATION pPendingMode)
{
    uint32_t xres=0, yres=0, bpp=0, display=0;

    /* Check if there's a pending display change request for this display */    
    if (VBoxQueryDisplayRequest(&xres, &yres, &bpp, &display) && (xres || yres || bpp))
    {
        if (display>RT_ELEMENTS(g_CustomVideoModes))
        {
            /*display = RT_ELEMENTS(g_CustomVideoModes) - 1;*/
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    /* Correct video mode params and check if host likes it */
    if (VBoxMPValidateVideoModeParams(pExt, display, xres, yres, bpp))
    {
        VBoxFillVidModeInfo(pPendingMode, xres, yres, bpp, display, 0);
        return TRUE;
    }

    return FALSE;
}

/* Save custom mode info to registry */
static void VBoxMPRegSaveModeInfo(PVBOXMP_DEVEXT pExt, uint32_t iDisplay, PVIDEO_MODE_INFORMATION pMode)
{
    VBOXMPCMNREGISTRY Registry;
    VP_STATUS rc;

    rc = VBoxMPCmnRegInit(pExt, &Registry);
    VBOXMP_WARN_VPS(rc);

    if (iDisplay==0)
    {
        /*First name without a suffix*/
        rc = VBoxMPCmnRegSetDword(Registry, L"CustomXRes", pMode->VisScreenWidth);
        VBOXMP_WARN_VPS(rc);
        rc = VBoxMPCmnRegSetDword(Registry, L"CustomYRes", pMode->VisScreenHeight);
        VBOXMP_WARN_VPS(rc);
        rc = VBoxMPCmnRegSetDword(Registry, L"CustomBPP", pMode->BitsPerPlane);
        VBOXMP_WARN_VPS(rc);
    }
    else
    {
        wchar_t keyname[32];
        swprintf(keyname, L"CustomXRes%d", iDisplay);
        rc = VBoxMPCmnRegSetDword(Registry, keyname, pMode->VisScreenWidth);
        VBOXMP_WARN_VPS(rc);
        swprintf(keyname, L"CustomYRes%d", iDisplay);
        rc = VBoxMPCmnRegSetDword(Registry, keyname, pMode->VisScreenHeight);
        VBOXMP_WARN_VPS(rc);
        swprintf(keyname, L"CustomBPP%d", iDisplay);
        rc = VBoxMPCmnRegSetDword(Registry, keyname, pMode->BitsPerPlane);
        VBOXMP_WARN_VPS(rc);
    }

    rc = VBoxMPCmnRegFini(Registry);
    VBOXMP_WARN_VPS(rc);
}

#ifdef VBOX_XPDM_MINIPORT
VIDEO_MODE_INFORMATION* VBoxMPXpdmCurrentVideoMode(PVBOXMP_DEVEXT pExt)
{
    return VBoxMPCmnGetVideoModeInfo(pExt->CurrentMode - 1);
}

ULONG VBoxMPXpdmGetVideoModesCount()
{
    return g_NumVideoModes;
}

/* Makes a table of video modes consisting of:
 * Default modes
 * Custom modes manually added to registry
 * Custom modes for all displays (either from a display change hint or stored in registry)
 * 2 special modes, for a pending display change for this adapter. See comments below.
 */
void VBoxMPXpdmBuildVideoModesTable(PVBOXMP_DEVEXT pExt)
{
    uint32_t cStandartModes, cCustomModes;
    BOOLEAN bPending, bHaveSpecial;
    VIDEO_MODE_INFORMATION specialMode;

    /* Fill table with standart modes and ones manually added to registry */
    cStandartModes = VBoxMPFillModesTable(pExt, pExt->iDevice, g_VideoModes, RT_ELEMENTS(g_VideoModes), NULL);

    /* Add custom modes for all displays to the table */
    cCustomModes = VBoxCommonFromDeviceExt(pExt)->cDisplays;
    for (uint32_t i=0; i<cCustomModes; i++)
    {
        memcpy(&g_VideoModes[cStandartModes+i], &g_CustomVideoModes[i], sizeof(VIDEO_MODE_INFORMATION));
        g_VideoModes[cStandartModes+i].ModeIndex = cStandartModes+i+1;
    }

    /* Check if host wants us to switch video mode and it's for this adapter */
    bPending = VBoxMPCheckPendingVideoMode(pExt, &specialMode);
    bHaveSpecial = bPending && (pExt->iDevice == specialMode.ModeIndex);

    /* Check the startup case */
    if (!bHaveSpecial && VBoxMPIsStartingUp(pExt))
    {
        uint32_t xres=0, yres=0, bpp=0;
        /* Check if we could make valid mode from values stored to registry */
        if (VBoxMPValidateVideoModeParams(pExt, pExt->iDevice, xres, yres, bpp))
        {
            VBoxFillVidModeInfo(&specialMode, xres, yres, bpp, 0, 0);
            bHaveSpecial = TRUE;
        }
    }

    /* Update number of modes */
    g_NumVideoModes = cStandartModes + cCustomModes;

    if (!bHaveSpecial)
    {
        /* Just add 2 dummy modes to maintain table size. */
        memcpy(&g_VideoModes[g_NumVideoModes], &g_VideoModes[2], sizeof(VIDEO_MODE_INFORMATION));
        g_VideoModes[g_NumVideoModes].ModeIndex = g_NumVideoModes+1;
        g_NumVideoModes++;
        memcpy(&g_VideoModes[g_NumVideoModes], &g_VideoModes[2], sizeof(VIDEO_MODE_INFORMATION));
        g_VideoModes[g_NumVideoModes].ModeIndex = g_NumVideoModes+1;
        g_NumVideoModes++;
    }
    else
    {
        /* We need to alternate mode index entry for a pending mode change,
         * else windows will ignore actual mode change call.
         * Only alternate index if one of mode parameters changed and
         * regardless of conditions always add 2 entries to the table.
         */
        static int s_InvocationCounter=0;
        BOOLEAN bAlternativeIndex = FALSE;

        static uint32_t s_Prev_xres=0;
        static uint32_t s_Prev_yres=0;
        static uint32_t s_Prev_bpp=0;
        BOOLEAN bChanged = (s_Prev_xres!=specialMode.VisScreenWidth
                            || s_Prev_yres!=specialMode.VisScreenHeight
                            || s_Prev_bpp!=specialMode.BitsPerPlane);
        if (bChanged)
        {
            s_Prev_xres = specialMode.VisScreenWidth;
            s_Prev_yres = specialMode.VisScreenHeight;
            s_Prev_bpp = specialMode.BitsPerPlane;
        }

        /* Make sure there's no other mode in the table with same parameters,
         * because we need windows to pick up a new video mode index otherwise
         * actual mode change wouldn't happen.
         */
        int iFoundIdx;
        uint32_t uiStart=0;

        while (0 <= (iFoundIdx = VBoxMPFindVideoMode(&g_VideoModes[uiStart], g_NumVideoModes-uiStart, &specialMode)))
        {
            memcpy(&g_VideoModes[uiStart+iFoundIdx], &g_VideoModes[2], sizeof(VIDEO_MODE_INFORMATION));
            g_VideoModes[uiStart+iFoundIdx].ModeIndex = uiStart+iFoundIdx+1;
            uiStart += iFoundIdx+1;
        }

        /* Check if we need to alternate the index */
        if (!VBoxMPIsStartingUp(pExt))
        {
            if (bChanged)
            {
                s_InvocationCounter++;
            }

            if (s_InvocationCounter % 2)
            {
                bAlternativeIndex = TRUE;
                memcpy(&g_VideoModes[g_NumVideoModes], &g_VideoModes[2], sizeof(VIDEO_MODE_INFORMATION));
                g_VideoModes[g_NumVideoModes].ModeIndex = g_NumVideoModes+1;
                ++g_NumVideoModes;
            }
        }

        LOG(("add special mode[%d] %dx%d:%d for display %d (bChanged=%d, bAlretnativeIndex=%d)",
             g_NumVideoModes, specialMode.VisScreenWidth, specialMode.VisScreenHeight, specialMode.BitsPerPlane,
             pExt->iDevice, bChanged, bAlternativeIndex));

        /* Add special mode to the table
         * Note: Y offset isn't used for a host-supplied modes 
         */
        specialMode.ModeIndex = g_NumVideoModes+1;
        memcpy(&g_VideoModes[g_NumVideoModes], &specialMode, sizeof(VIDEO_MODE_INFORMATION));
        ++g_NumVideoModes;

        /* Save special mode in the custom modes table */
        memcpy(&g_CustomVideoModes[pExt->iDevice], &specialMode, sizeof(VIDEO_MODE_INFORMATION));
                    
        
        /* Make sure we've added 2nd mode if necessary to maintain table size */
        if (VBoxMPIsStartingUp(pExt))
        {
            memcpy(&g_VideoModes[g_NumVideoModes], &g_VideoModes[g_NumVideoModes-1], sizeof(VIDEO_MODE_INFORMATION));
            g_VideoModes[g_NumVideoModes].ModeIndex = g_NumVideoModes+1;
            ++g_NumVideoModes;
        }
        else if (!bAlternativeIndex)
        {
            memcpy(&g_VideoModes[g_NumVideoModes], &g_VideoModes[2], sizeof(VIDEO_MODE_INFORMATION));
            g_VideoModes[g_NumVideoModes].ModeIndex = g_NumVideoModes+1;
            ++g_NumVideoModes;
        }

        /* Save special mode info to registry */
        VBoxMPRegSaveModeInfo(pExt, pExt->iDevice, &specialMode);
    }

#if defined(LOG_ENABLED)
    do
    {
        LOG(("Filled %d modes", g_NumVideoModes));

        for (uint32_t i=0; i<g_NumVideoModes; ++i)
        {
            LOG(("Mode[%2d]: %4dx%4d:%2d (idx=%d)",
                i, g_VideoModes[i].VisScreenWidth, g_VideoModes[i].VisScreenHeight,
                g_VideoModes[i].BitsPerPlane, g_VideoModes[i].ModeIndex));
        }
    } while (0);
#endif
}
#endif /*VBOX_XPDM_MINIPORT*/

#ifdef VBOX_WDDM_MINIPORT
static VBOXWDDM_VIDEOMODES_INFO g_aVBoxVideoModeInfos[VBOX_VIDEO_MAX_SCREENS] = {0};

bool VBoxWddmFillMode(VIDEO_MODE_INFORMATION *pInfo, D3DDDIFORMAT enmFormat, ULONG w, ULONG h)
{
    switch (enmFormat)
    {
        case D3DDDIFMT_A8R8G8B8:
            VBoxFillVidModeInfo(pInfo, w, h, 32, 0, 0);
            return true;
        case D3DDDIFMT_R8G8B8:
            VBoxFillVidModeInfo(pInfo, w, h, 24, 0, 0);
            return true;
        case D3DDDIFMT_R5G6B5:
            VBoxFillVidModeInfo(pInfo, w, h, 16, 0, 0);
            return true;
        case D3DDDIFMT_P8:
            VBoxFillVidModeInfo(pInfo, w, h, 8, 0, 0);
            return true;
        default:
            WARN(("unsupported enmFormat(%d)", enmFormat));
            AssertBreakpoint();
            break;
    }

    return false;
}

static void
VBoxWddmBuildResolutionTable(PVIDEO_MODE_INFORMATION pModesTable, size_t tableSize, 
                             SIZE *pResolutions, uint32_t * pcResolutions)
{
    uint32_t cResolutionsArray = *pcResolutions;
    uint32_t cResolutions = 0;

    for (uint32_t i=0; i<tableSize; ++i)
    {
        PVIDEO_MODE_INFORMATION pMode = &pModesTable[i];
        BOOLEAN bFound = FALSE;

        for (uint32_t j=0; j<cResolutions; ++j)
        {
            if (pResolutions[j].cx == pMode->VisScreenWidth
                && pResolutions[j].cy == pMode->VisScreenHeight)
            {
                bFound = TRUE;
                break;
            }
        }

        if (!bFound)
        {
            if (cResolutions == cResolutionsArray)
            {
                WARN(("table overflow!"));
                break;
            }

            pResolutions[cResolutions].cx = pMode->VisScreenWidth;
            pResolutions[cResolutions].cy = pMode->VisScreenHeight;
            ++cResolutions;
        }
    }

    *pcResolutions = cResolutions;
}

AssertCompile(sizeof (SIZE) == sizeof (D3DKMDT_2DREGION));
AssertCompile(RT_OFFSETOF(SIZE, cx) == RT_OFFSETOF(D3DKMDT_2DREGION, cx));
AssertCompile(RT_OFFSETOF(SIZE, cy) == RT_OFFSETOF(D3DKMDT_2DREGION, cy));
static void
VBoxWddmBuildVideoModesInfo(PVBOXMP_DEVEXT pExt, D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId,
                            PVBOXWDDM_VIDEOMODES_INFO pModes, VIDEO_MODE_INFORMATION *paAddlModes, 
                            UINT cAddlModes)
{
    pModes->cResolutions = RT_ELEMENTS(pModes->aResolutions);

    /* Add default modes and ones read from registry. */
    pModes->cModes = VBoxMPFillModesTable(pExt, VidPnTargetId, pModes->aModes, RT_ELEMENTS(pModes->aModes), &pModes->iPreferredMode);
    Assert(pModes->cModes<=RT_ELEMENTS(pModes->aModes));

    /* Check if there's a pending display change request for this adapter */
    VIDEO_MODE_INFORMATION specialMode;
    if (VBoxMPCheckPendingVideoMode(pExt, &specialMode) && (specialMode.ModeIndex==VidPnTargetId))
    {
        /*Minor hack, ModeIndex!=0 Means this mode has been validated already and not just read from registry */
        specialMode.ModeIndex = 1;
        memcpy(&g_CustomVideoModes[VidPnTargetId], &specialMode, sizeof(VIDEO_MODE_INFORMATION));

        /* Save mode to registry */
        VBoxMPRegSaveModeInfo(pExt, VidPnTargetId, &specialMode);
    }

    /* Validate the mode which has been read from registry */
    if (!g_CustomVideoModes[VidPnTargetId].ModeIndex)
    {
        uint32_t xres, yres, bpp;

        xres = g_CustomVideoModes[VidPnTargetId].VisScreenWidth;
        yres = g_CustomVideoModes[VidPnTargetId].VisScreenHeight;
        bpp = g_CustomVideoModes[VidPnTargetId].BitsPerPlane;

        if (VBoxMPValidateVideoModeParams(pExt, VidPnTargetId, xres, yres, bpp))
        {
            VBoxFillVidModeInfo(&g_CustomVideoModes[VidPnTargetId], xres, yres, bpp, 1/*index*/, 0);
            Assert(g_CustomVideoModes[VidPnTargetId].ModeIndex == 1);
        }
    }

    /* Add custom mode to the table */
    if (g_CustomVideoModes[VidPnTargetId].ModeIndex)
    {
        if (RT_ELEMENTS(pModes->aModes) > pModes->cModes)
        {
            g_CustomVideoModes[VidPnTargetId].ModeIndex = pModes->cModes;
            pModes->aModes[pModes->cModes] = g_CustomVideoModes[VidPnTargetId];

            /* Check if we already have this mode in the table */
            int foundIdx;
            if ((foundIdx=VBoxMPFindVideoMode(pModes->aModes, pModes->cModes, &pModes->aModes[pModes->cModes]))>=0)
            {
                pModes->iPreferredMode = foundIdx;
            }
            else
            {
                pModes->iPreferredMode = pModes->cModes;
                ++pModes->cModes;
            }

            /* Add other bpp modes for this custom resolution */
#ifdef VBOX_WITH_8BPP_MODES
        UINT bpp=8;
#else
        UINT bpp=16;    
#endif
            for (; bpp<=32; bpp+=8)
            {
                if (RT_ELEMENTS(pModes->aModes) == pModes->cModes)
                {
                    WARN(("table full, can't add other bpp for specail mode!"));
#ifdef DEBUG_misha
                    /* this is definitely something we do not expect */
                    AssertFailed();
#endif
                    break;
                }

                AssertRelease(RT_ELEMENTS(pModes->aModes) > pModes->cModes); /* if not - the driver state is screwed up, @todo: better do KeBugCheckEx here */

                if (pModes->aModes[pModes->iPreferredMode].BitsPerPlane != bpp)
                {
                    VBoxFillVidModeInfo(&pModes->aModes[pModes->cModes],
                                        pModes->aModes[pModes->iPreferredMode].VisScreenWidth,
                                        pModes->aModes[pModes->iPreferredMode].VisScreenHeight,
                                        bpp, pModes->cModes, 0);
                    if (VBoxMPFindVideoMode(pModes->aModes, pModes->cModes, &pModes->aModes[pModes->cModes]) < 0)
                    {
                        ++pModes->cModes;
                    }
                }
            }
        }
        else
        {
            AssertRelease(RT_ELEMENTS(pModes->aModes) == pModes->cModes); /* if not - the driver state is screwed up, @todo: better do KeBugCheckEx here */
            WARN(("table full, can't add video mode for a host request!"));
#ifdef DEBUG_misha
            /* this is definitely something we do not expect */
            AssertFailed();
#endif
        }
    }

    /* Check and Add additional modes passed in paAddlModes */
    for (UINT i=0; i<cAddlModes; ++i)
    {
        if (RT_ELEMENTS(pModes->aModes) == pModes->cModes)
        {
           WARN(("table full, can't add addl modes!"));
#ifdef DEBUG_misha
            /* this is definitely something we do not expect */
            AssertFailed();
#endif
           break;
        }

        AssertRelease(RT_ELEMENTS(pModes->aModes) > pModes->cModes); /* if not - the driver state is screwed up, @todo: better do KeBugCheckEx here */

        if (!pExt->fAnyX)
        {
            paAddlModes[i].VisScreenWidth &= 0xFFF8;
        }

        if (VBoxLikesVideoMode(VidPnTargetId, paAddlModes[i].VisScreenWidth, paAddlModes[i].VisScreenHeight, paAddlModes[i].BitsPerPlane))
        {
            int foundIdx;
            if ((foundIdx=VBoxMPFindVideoMode(pModes->aModes, pModes->cModes, &paAddlModes[i]))>=0)
            {
                pModes->iPreferredMode = foundIdx;
            }
            else
            {
                memcpy(&pModes->aModes[pModes->cModes], &paAddlModes[i], sizeof(VIDEO_MODE_INFORMATION));
                pModes->aModes[pModes->cModes].ModeIndex = pModes->cModes;
                ++pModes->cModes;
            }
        }
    }

    pModes->cPrevModes = pModes->cModes;

    /* Build resolution table */
    VBoxWddmBuildResolutionTable(pModes->aModes, pModes->cModes, (SIZE*)((void*)pModes->aResolutions), &pModes->cResolutions);
}

void VBoxWddmInvalidateVideoModesInfo(PVBOXMP_DEVEXT pExt)
{
    for (UINT i = 0; i < RT_ELEMENTS(g_aVBoxVideoModeInfos); ++i)
    {
        g_aVBoxVideoModeInfos[i].cModes = 0;
    }
}

PVBOXWDDM_VIDEOMODES_INFO VBoxWddmUpdateVideoModesInfo(PVBOXMP_DEVEXT pExt, PVBOXWDDM_RECOMMENDVIDPN pVidPnInfo)
{
    VBoxWddmInvalidateVideoModesInfo(pExt);

    if (pVidPnInfo)
    {
        for (UINT i = 0; i < pVidPnInfo->cScreenInfos; ++i)
        {
            PVBOXWDDM_RECOMMENDVIDPN_SCREEN_INFO pScreenInfo = &pVidPnInfo->aScreenInfos[i];
            Assert(pScreenInfo->Id < (DWORD)VBoxCommonFromDeviceExt(pExt)->cDisplays);
            if (pScreenInfo->Id < (DWORD)VBoxCommonFromDeviceExt(pExt)->cDisplays)
            {
                PVBOXWDDM_VIDEOMODES_INFO pInfo = &g_aVBoxVideoModeInfos[pScreenInfo->Id];
                VIDEO_MODE_INFORMATION ModeInfo = {0};
                D3DDDIFORMAT enmFormat;
                switch (pScreenInfo->BitsPerPixel)
                {
                    case 32:
                        enmFormat = D3DDDIFMT_A8R8G8B8;
                        break;
                    case 24:
                        enmFormat = D3DDDIFMT_R8G8B8;
                        break;
                    case 16:
                        enmFormat = D3DDDIFMT_R5G6B5;
                        break;
                    case 8:
                        enmFormat = D3DDDIFMT_P8;
                        break;
                    default:
                        Assert(0);
                        enmFormat = D3DDDIFMT_UNKNOWN;
                        break;
                }
                if (enmFormat != D3DDDIFMT_UNKNOWN)
                {
                    if (VBoxWddmFillMode(&ModeInfo, enmFormat, pScreenInfo->Width, pScreenInfo->Height))
                    {
                        VBoxWddmBuildVideoModesInfo(pExt, pScreenInfo->Id, pInfo, &ModeInfo, 1);
                    }
                    else
                    {
                        Assert(0);
                    }
                }
            }
        }
    }

    /* ensure we have all the rest populated */
    VBoxWddmGetAllVideoModesInfos(pExt);
    return g_aVBoxVideoModeInfos;
}

NTSTATUS VBoxWddmGetModesForResolution(VIDEO_MODE_INFORMATION *pAllModes, uint32_t cAllModes, int iSearchPreferredMode,
        const D3DKMDT_2DREGION *pResolution, VIDEO_MODE_INFORMATION * pModes, uint32_t cModes, uint32_t *pcModes, int32_t *piPreferrableMode)
{
    NTSTATUS Status = STATUS_SUCCESS;
    uint32_t cFound = 0;
    int iFoundPreferrableMode = -1;
    for (uint32_t i = 0; i < cAllModes; ++i)
    {
        VIDEO_MODE_INFORMATION *pCur = &pAllModes[i];
        if (pResolution->cx == pCur->VisScreenWidth
                        && pResolution->cy == pCur->VisScreenHeight)
        {
            if (pModes && cModes > cFound)
                memcpy(&pModes[cFound], pCur, sizeof(VIDEO_MODE_INFORMATION));
            else
                Status = STATUS_BUFFER_TOO_SMALL;

            if (i == iSearchPreferredMode)
                iFoundPreferrableMode = cFound;

            ++cFound;
        }
    }

    Assert(iFoundPreferrableMode < 0 || cFound > (uint32_t)iFoundPreferrableMode);

    *pcModes = cFound;
    if (piPreferrableMode)
        *piPreferrableMode = iFoundPreferrableMode;

    return Status;
}

PVBOXWDDM_VIDEOMODES_INFO VBoxWddmGetAllVideoModesInfos(PVBOXMP_DEVEXT pExt)
{
    /* ensure all modes are initialized */
    for (int i = 0; i < VBoxCommonFromDeviceExt(pExt)->cDisplays; ++i)
    {
        VBoxWddmGetVideoModesInfo(pExt, (D3DDDI_VIDEO_PRESENT_TARGET_ID)i);
    }

    return g_aVBoxVideoModeInfos;
}

PVBOXWDDM_VIDEOMODES_INFO VBoxWddmGetVideoModesInfo(PVBOXMP_DEVEXT pExt, D3DDDI_VIDEO_PRESENT_TARGET_ID VidPnTargetId)
{
    Assert(VidPnTargetId < (D3DDDI_VIDEO_PRESENT_TARGET_ID)VBoxCommonFromDeviceExt(pExt)->cDisplays);
    if (VidPnTargetId >= (D3DDDI_VIDEO_PRESENT_TARGET_ID)VBoxCommonFromDeviceExt(pExt)->cDisplays)
    {
        return NULL;
    }

    PVBOXWDDM_VIDEOMODES_INFO pInfo = &g_aVBoxVideoModeInfos[VidPnTargetId];

    if (!pInfo->cModes)
    {
        VBoxWddmBuildVideoModesInfo(pExt, VidPnTargetId, pInfo, NULL, 0);
        Assert(pInfo->cModes);
    }

    return pInfo;
}

#endif /*VBOX_WDDM_MINIPORT*/

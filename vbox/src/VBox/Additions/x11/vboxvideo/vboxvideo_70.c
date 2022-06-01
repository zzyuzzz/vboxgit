/** @file
 *
 * Linux Additions X11 graphics driver
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
 * --------------------------------------------------------------------
 *
 * This code is based on:
 *
 * X11 VESA driver
 *
 * Copyright (c) 2000 by Conectiva S.A. (http://www.conectiva.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * CONECTIVA LINUX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Conectiva Linux shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from
 * Conectiva Linux.
 *
 * Authors: Paulo César Pereira de Andrade <pcpa@conectiva.com.br>
 */

#ifdef XORG_7X
# include "xorg-server.h"
#endif
#include "vboxvideo.h"
#include "version-generated.h"
#include <xf86.h>

/* All drivers initialising the SW cursor need this */
#include "mipointer.h"

/* All drivers implementing backing store need this */
#include "mibstore.h"

/* Colormap handling */
#include "micmap.h"
#include "xf86cmap.h"

/* DPMS */
/* #define DPMS_SERVER
#include "extensions/dpms.h" */

/* Mandatory functions */

static const OptionInfoRec * VBOXAvailableOptions(int chipid, int busid);
static void VBOXIdentify(int flags);
static Bool VBOXProbe(DriverPtr drv, int flags);
static Bool VBOXPreInit(ScrnInfoPtr pScrn, int flags);
static Bool VBOXScreenInit(int Index, ScreenPtr pScreen, int argc,
                           char **argv);
static Bool VBOXEnterVT(int scrnIndex, int flags);
static void VBOXLeaveVT(int scrnIndex, int flags);
static Bool VBOXCloseScreen(int scrnIndex, ScreenPtr pScreen);
static Bool VBOXSaveScreen(ScreenPtr pScreen, int mode);
static Bool VBOXSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags);
static Bool VBOXSetMode(ScrnInfoPtr pScrn, DisplayModePtr pMode);
static void VBOXAdjustFrame(int scrnIndex, int x, int y, int flags);
static void VBOXFreeScreen(int scrnIndex, int flags);
static void VBOXFreeRec(ScrnInfoPtr pScrn);
static void VBOXDisplayPowerManagementSet(ScrnInfoPtr pScrn, int mode,
                                          int flags);

/* locally used functions */
static Bool VBOXMapVidMem(ScrnInfoPtr pScrn);
static void VBOXUnmapVidMem(ScrnInfoPtr pScrn);
static void VBOXLoadPalette(ScrnInfoPtr pScrn, int numColors,
                            int *indices,
                            LOCO *colors, VisualPtr pVisual);
static void SaveFonts(ScrnInfoPtr pScrn);
static void RestoreFonts(ScrnInfoPtr pScrn);
static Bool VBOXSaveRestore(ScrnInfoPtr pScrn,
                            vbeSaveRestoreFunction function);
static void vboxWriteHostModes(ScrnInfoPtr pScrn, DisplayModePtr pCurrent);
static bool VBOXAdjustScreenPixmap(ScrnInfoPtr pScrn, DisplayModePtr pMode);

/*
 * This contains the functions needed by the server after loading the
 * driver module.  It must be supplied, and gets added the driver list by
 * the Module Setup funtion in the dynamic case.  In the static case a
 * reference to this is compiled in, and this requires that the name of
 * this DriverRec be an upper-case version of the driver name.
 */

#ifdef XORG_7X
_X_EXPORT
#endif
DriverRec VBOXDRV = {
    VBOX_VERSION,
    VBOX_DRIVER_NAME,
    VBOXIdentify,
    VBOXProbe,
    VBOXAvailableOptions,
    NULL,
    0,
#ifdef XORG_7X
    NULL
#endif
};

/* Supported chipsets */
static SymTabRec VBOXChipsets[] =
{
    {VBOX_VESA_DEVICEID, "vbox"},
    {-1,	 NULL}
};

static PciChipsets VBOXPCIchipsets[] = {
  { VBOX_DEVICEID, VBOX_DEVICEID, RES_SHARED_VGA },
  { -1,		-1,		    RES_UNDEFINED },
};

typedef enum {
    OPTION_SHADOW_FB,
    OPTION_DFLT_REFRESH,
    OPTION_MODESET_CLEAR_SCREEN
} VBOXOpts;

/* No options for now */
static const OptionInfoRec VBOXOptions[] = {
    { -1,		NULL,		OPTV_NONE,	{0},	FALSE }
};

/*
 * List of symbols from other modules that this module references.  This
 * list is used to tell the loader that it is OK for symbols here to be
 * unresolved providing that it hasn't been told that they haven't been
 * told that they are essential via a call to xf86LoaderReqSymbols() or
 * xf86LoaderReqSymLists().  The purpose is this is to avoid warnings about
 * unresolved symbols that are not required.
 */
static const char *fbSymbols[] = {
    "fbPictureInit",
    "fbScreenInit",
    NULL
};

static const char *shadowfbSymbols[] = {
  "ShadowFBInit2",
  NULL
};

static const char *vbeSymbols[] = {
    "VBEExtendedInit",
    "VBEFindSupportedDepths",
    "VBEGetModeInfo",
    "VBEGetVBEInfo",
    "VBEGetVBEMode",
    "VBEPrintModes",
    "VBESaveRestore",
    "VBESetDisplayStart",
    "VBESetGetDACPaletteFormat",
    "VBESetGetLogicalScanlineLength",
    "VBESetGetPaletteData",
    "VBESetModeNames",
    "VBESetModeParameters",
    "VBESetVBEMode",
    "VBEValidateModes",
    "vbeDoEDID",
    "vbeFree",
    NULL
};

static const char *ramdacSymbols[] = {
    "xf86InitCursor",
    "xf86CreateCursorInfoRec",
    NULL
};

#ifdef XFree86LOADER
/* Module loader interface */
static MODULESETUPPROTO(vboxSetup);

static XF86ModuleVersionInfo vboxVersionRec =
{
    VBOX_DRIVER_NAME,
    "Sun Microsystems, Inc.",
    MODINFOSTRING1,
    MODINFOSTRING2,
#ifdef XORG_7X
    XORG_VERSION_CURRENT,
#else
    XF86_VERSION_CURRENT,
#endif
    1,                          /* Module major version. Xorg-specific */
    0,                          /* Module minor version. Xorg-specific */
    1,                          /* Module patchlevel. Xorg-specific */
    ABI_CLASS_VIDEODRV,	        /* This is a video driver */
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

/*
 * This data is accessed by the loader.  The name must be the module name
 * followed by "ModuleData".
 */
#ifdef XORG_7X
_X_EXPORT
#endif
XF86ModuleData vboxvideoModuleData = { &vboxVersionRec, vboxSetup, NULL };

static pointer
vboxSetup(pointer Module, pointer Options, int *ErrorMajor, int *ErrorMinor)
{
    static Bool Initialised = FALSE;

    if (!Initialised)
    {
        Initialised = TRUE;
        xf86AddDriver(&VBOXDRV, Module, 0);
        LoaderRefSymLists(fbSymbols,
                          shadowfbSymbols,
                          vbeSymbols,
                          ramdacSymbols,
                          NULL);
        return (pointer)TRUE;
    }

    if (ErrorMajor)
        *ErrorMajor = LDR_ONCEONLY;
    return (NULL);
}

#endif  /* XFree86Loader defined */

static const OptionInfoRec *
VBOXAvailableOptions(int chipid, int busid)
{
    return (VBOXOptions);
}

static void
VBOXIdentify(int flags)
{
    xf86PrintChipsets(VBOX_NAME, "guest driver for VirtualBox", VBOXChipsets);
}

/*
 * This function is called once, at the start of the first server generation to
 * do a minimal probe for supported hardware.
 */

static Bool
VBOXProbe(DriverPtr drv, int flags)
{
    Bool foundScreen = FALSE;
    int numDevSections, numUsed;
    GDevPtr *devSections;
    int *usedChips;
    int i;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice(VBOX_NAME,
					  &devSections)) <= 0)
	return (FALSE);

    /* PCI BUS */
    if (xf86GetPciVideoInfo()) {
	numUsed = xf86MatchPciInstances(VBOX_NAME, VBOX_VENDORID,
					VBOXChipsets, VBOXPCIchipsets,
					devSections, numDevSections,
					drv, &usedChips);
	if (numUsed > 0) {
	    if (flags & PROBE_DETECT)
		foundScreen = TRUE;
	    else {
		for (i = 0; i < numUsed; i++) {
		    ScrnInfoPtr pScrn = NULL;
		    /* Allocate a ScrnInfoRec  */
		    if ((pScrn = xf86ConfigPciEntity(pScrn,0,usedChips[i],
						     VBOXPCIchipsets,NULL,
						     NULL,NULL,NULL,NULL))) {
			pScrn->driverVersion = VBOX_VERSION;
			pScrn->driverName    = VBOX_DRIVER_NAME;
			pScrn->name	     = VBOX_NAME;
			pScrn->Probe	     = VBOXProbe;
                        pScrn->PreInit       = VBOXPreInit;
			pScrn->ScreenInit    = VBOXScreenInit;
			pScrn->SwitchMode    = VBOXSwitchMode;
			pScrn->AdjustFrame   = VBOXAdjustFrame;
			pScrn->EnterVT       = VBOXEnterVT;
			pScrn->LeaveVT       = VBOXLeaveVT;
			pScrn->FreeScreen    = VBOXFreeScreen;
			foundScreen = TRUE;
		    }
		}
	    }
	    xfree(usedChips);
	}
    }
    xfree(devSections);

    return (foundScreen);
}

static VBOXPtr
VBOXGetRec(ScrnInfoPtr pScrn)
{
    if (!pScrn->driverPrivate)
    {
        pScrn->driverPrivate = xcalloc(sizeof(VBOXRec), 1);
#if 0
        ((VBOXPtr)pScrn->driverPrivate)->vbox_fd = -1;
#endif
    }

    return ((VBOXPtr)pScrn->driverPrivate);
}

static void
VBOXFreeRec(ScrnInfoPtr pScrn)
{
    VBOXPtr pVBox = VBOXGetRec(pScrn);
#if 0
    xfree(pVBox->vbeInfo);
#endif
    xfree(pVBox->savedPal);
    xfree(pVBox->fonts);
    xfree(pScrn->driverPrivate);
    pScrn->driverPrivate = NULL;
}

/**
 * Fills a display mode M with a built-in mode of name pszName and dimensions
 * cx and cy.
 */
static void vboxFillDisplayMode(DisplayModePtr m, const char *pszName,
                                unsigned cx, unsigned cy)
{
    TRACE_LOG("pszName=%s, cx=%u, cy=%u\n", pszName, cx, cy);
    m->status        = MODE_OK;
    m->type          = M_T_BUILTIN;
    /* VBox only supports screen widths which are a multiple of 8 */
    m->HDisplay      = cx & ~7;
    m->HSyncStart    = m->HDisplay + 2;
    m->HSyncEnd      = m->HDisplay + 4;
    m->HTotal        = m->HDisplay + 6;
    m->VDisplay      = cy;
    m->VSyncStart    = m->VDisplay + 2;
    m->VSyncEnd      = m->VDisplay + 4;
    m->VTotal        = m->VDisplay + 6;
    m->Clock         = m->HTotal * m->VTotal * 60 / 1000; /* kHz */
    if (pszName)
    {
        if (m->name)
            xfree(m->name);
        m->name      = xnfstrdup(pszName);
    }
}

/** vboxvideo's list of standard video modes */
struct
{
    /** mode width */
    uint32_t cx;
    /** mode height */
    uint32_t cy;    
} vboxStandardModes[] =
{
    { 1600, 1200 },
    { 1440, 1050 },
    { 1280, 960 },
    { 1024, 768 },
    { 800, 600 },
    { 640, 480 },
    { 0, 0 }
};
enum
{
    vboxNumStdModes = sizeof(vboxStandardModes) / sizeof(vboxStandardModes[0])
};

/**
 * Returns a standard mode which the host likes.  Can be called multiple
 * times with the index returned by the previous call to get a list of modes.
 * @returns  the index of the mode in the list, or 0 if no more modes are
 *           available
 * @param    pScrn   the screen information structure
 * @param    pScrn->bitsPerPixel
 *                   if this is non-null, only modes with this BPP will be
 *                   returned
 * @param    cIndex  the index of the last mode queried, or 0 to query the
 *                   first mode available.  Note: the first index is 1
 * @param    pcx     where to store the mode's width
 * @param    pcy     where to store the mode's height
 * @param    pcBits  where to store the mode's BPP
 */
static unsigned vboxNextStandardMode(ScrnInfoPtr pScrn, unsigned cIndex,
                                     uint32_t *pcx, uint32_t *pcy,
                                     uint32_t *pcBits)
{
    XF86ASSERT(cIndex < vboxNumStdModes,
               ("cIndex = %d, vboxNumStdModes = %d\n", cIndex,
                vboxNumStdModes));
    for (unsigned i = cIndex; i < vboxNumStdModes - 1; ++i)
    {
        uint32_t cBits = pScrn->bitsPerPixel;
        uint32_t cx = vboxStandardModes[i].cx;
        uint32_t cy = vboxStandardModes[i].cy;

        if (cBits != 0 && !vboxHostLikesVideoMode(pScrn, cx, cy, cBits))
            continue;
        if (vboxHostLikesVideoMode(pScrn, cx, cy, 32))
            cBits = 32;
        else if (vboxHostLikesVideoMode(pScrn, cx, cy, 16))
            cBits = 16;
        else
            continue;
        if (pcx)
            *pcx = cx;
        if (pcy)
            *pcy = cy;
        if (pcBits)
            *pcBits = cBits;
        return i + 1;
    }
    return 0;
}

/**
 * Returns the preferred video mode.  The current order of preference is
 * (from highest to least preferred):
 *  - The mode corresponding to the last size hint from the host
 *  - The video mode saved from the last session
 *  - The largest standard mode which the host likes, falling back to
 *    640x480x32 as a worst case
 *  - If the host can't be contacted at all, we return 1024x768x32
 *
 * The return type is void as we guarantee we will return some mode.
 */
static void vboxGetPreferredMode(ScrnInfoPtr pScrn, uint32_t *pcx,
                                 uint32_t *pcy, uint32_t *pcBits)
{
    /* Query the host for the preferred resolution and colour depth */
    uint32_t cx = 0, cy = 0, iDisplay = 0, cBits = 32;
    VBOXPtr pVBox = pScrn->driverPrivate;

    TRACE_ENTRY();
    if (pVBox->useDevice)
    {
        bool found = vboxGetDisplayChangeRequest(pScrn, &cx, &cy, &cBits, &iDisplay);
        if ((cx == 0) || (cy == 0))
            found = false;
        if (!found)
            found = vboxRetrieveVideoMode(pScrn, &cx, &cy, &cBits);
        if ((cx == 0) || (cy == 0))
            found = false;
        if (found)
            /* Adjust to a multiple of eight */
            cx -= cx % 8;
        if (!found)
            found = (vboxNextStandardMode(pScrn, 0, &cx, &cy, &cBits) != 0);
        if (!found)
        {
            /* Last resort */
            cx = 640;
            cy = 480;
            cBits = 32;
        }
    }
    else
    {
        cx = 1024;
        cy = 768;
    }
    if (pcx)
        *pcx = cx;
    if (pcy)
        *pcy = cy;
    if (pcx)
        *pcBits = cBits;
}

/* Move a screen mode found to the end of the list, so that RandR will give
 * it the highest priority when a mode switch is requested.  Returns the mode
 * that was previously before the mode in the list in order to allow the
 * caller to continue walking the list. */
static DisplayModePtr vboxMoveModeToFront(ScrnInfoPtr pScrn,
                                          DisplayModePtr pMode)
{
    DisplayModePtr pPrev = pMode->prev;
    if (pMode != pScrn->modes)
    {
        pMode->prev->next = pMode->next;
        pMode->next->prev = pMode->prev;
        pMode->next = pScrn->modes;
        pMode->prev = pScrn->modes->prev;
        pMode->next->prev = pMode;
        pMode->prev->next = pMode;
        pScrn->modes = pMode;
    }
    return pPrev;
}

/**
 * Rewrites the first dynamic mode found which is not the current screen mode
 * to contain the host's currently preferred screen size, then moves that
 * mode to the front of the screen information structure's mode list. 
 * Additionally, if the current mode is not dynamic, the second dynamic mode
 * will be set to match the current mode and also added to the front.  This
 * ensures that the user can always reset the current size to kick the driver
 * to update its mode list.
 */
static void vboxWriteHostModes(ScrnInfoPtr pScrn, DisplayModePtr pCurrent)
{
    uint32_t cx = 0, cy = 0, iDisplay = 0, cBits = 0;
    DisplayModePtr pMode;
    bool found = false;

    TRACE_ENTRY();
    vboxGetPreferredMode(pScrn, &cx, &cy, &cBits);
#ifdef DEBUG
    /* Count the number of modes for sanity */
    unsigned cModes = 1, cMode = 0;
    DisplayModePtr pCount;
    for (pCount = pScrn->modes; ; pCount = pCount->next, ++cModes)
        if (pCount->next == pScrn->modes)
            break;
#endif
    for (pMode = pScrn->modes; ; pMode = pMode->next)
    {
#ifdef DEBUG
        XF86ASSERT (cMode++ < cModes, (NULL));
#endif
        if (   pMode != pCurrent
            && !strcmp(pMode->name, "VBoxDynamicMode"))
        {
            if (!found)
                vboxFillDisplayMode(pMode, NULL, cx, cy);
            else if (pCurrent)
                vboxFillDisplayMode(pMode, NULL, pCurrent->HDisplay,
                                    pCurrent->VDisplay);
            found = true;
            pMode = vboxMoveModeToFront(pScrn, pMode);
        }
        if (pMode->next == pScrn->modes)
            break;
    }
    XF86ASSERT (found,
                ("vboxvideo: no free dynamic mode found.  Exiting.\n"));
    XF86ASSERT (   (pScrn->modes->HDisplay == (long) cx)
                || (   (pScrn->modes->HDisplay == pCurrent->HDisplay)
                    && (pScrn->modes->next->HDisplay == (long) cx)),
                ("pScrn->modes->HDisplay=%u, pScrn->modes->next->HDisplay=%u\n",
                 pScrn->modes->HDisplay, pScrn->modes->next->HDisplay));
    XF86ASSERT (   (pScrn->modes->VDisplay == (long) cy)
                || (   (pScrn->modes->VDisplay == pCurrent->VDisplay)
                    && (pScrn->modes->next->VDisplay == (long) cy)),
                ("pScrn->modes->VDisplay=%u, pScrn->modes->next->VDisplay=%u\n",
                 pScrn->modes->VDisplay, pScrn->modes->next->VDisplay));
}

/**
 * Allocates an empty display mode and links it into the doubly linked list of
 * modes pointed to by pScrn->modes.  Returns a pointer to the newly allocated
 * memory.
 */
static DisplayModePtr vboxAddEmptyScreenMode(ScrnInfoPtr pScrn)
{
    DisplayModePtr pMode = xnfcalloc(sizeof(DisplayModeRec), 1);

    TRACE_ENTRY();
    if (!pScrn->modes)
    {
        pScrn->modes = pMode;
        pMode->next = pMode;
        pMode->prev = pMode;
    }
    else
    {
        pMode->next = pScrn->modes;
        pMode->prev = pScrn->modes->prev;
        pMode->next->prev = pMode;
        pMode->prev->next = pMode;
    }
    return pMode;
}

/**
 * Create display mode entries in the screen information structure for each
 * of the initial graphics modes that we wish to support.  This includes:
 *  - An initial mode, of the size requested by the caller
 *  - Two dynamic modes, one of which will be updated to match the last size
 *    hint from the host on each mode switch, but initially also of the
 *    requested size
 *  - Several standard modes, if possible ones that the host likes
 *  - Any modes that the user requested in xorg.conf/XFree86Config
 */
static void
vboxAddModes(ScrnInfoPtr pScrn, uint32_t cxInit, uint32_t cyInit)
{
    unsigned cx = 0, cy = 0, cIndex = 0;
    /* For reasons related to the way RandR 1.1 is implemented, we need to
     * make sure that the initial mode (more precisely, a mode equal to the
     * initial virtual resolution) is always present in the mode list.  RandR
     * has the assumption build in that there will either be a mode of that
     * size present at all times, or that the first mode in the list will
     * always be smaller than the initial virtual resolution.  Since our
     * approach to dynamic resizing isn't quite the way RandR was intended to
     * be, and breaks the second assumption, we guarantee the first. */
    DisplayModePtr pMode = vboxAddEmptyScreenMode(pScrn);
    vboxFillDisplayMode(pMode, "VBoxInitialMode", cxInit, cyInit);
    /* Create our two dynamic modes. */
    pMode = vboxAddEmptyScreenMode(pScrn);
    vboxFillDisplayMode(pMode, "VBoxDynamicMode", cxInit, cyInit);
    pMode = vboxAddEmptyScreenMode(pScrn);
    vboxFillDisplayMode(pMode, "VBoxDynamicMode", cxInit, cyInit);
    /* Add standard modes supported by the host */
    for ( ; ; )
    {
        char szName[256];
        cIndex = vboxNextStandardMode(pScrn, cIndex, &cx, &cy, NULL);
        if (cIndex == 0)
            break;
        sprintf(szName, "VBox-%ux%u", cx, cy);
        pMode = vboxAddEmptyScreenMode(pScrn);
        vboxFillDisplayMode(pMode, szName, cx, cy);
    }
    /* And finally any modes specified by the user.  We assume here that
     * the mode names reflect the mode sizes. */
    for (unsigned i = 0;    pScrn->display->modes != NULL
                         && pScrn->display->modes[i] != NULL; i++)
    {
        if (sscanf(pScrn->display->modes[i], "%ux%u", &cx, &cy) == 2)
        {
            pMode = vboxAddEmptyScreenMode(pScrn);
            vboxFillDisplayMode(pMode, pScrn->display->modes[i], cx, cy);
        }
    }
}

/*
 * QUOTE from the XFree86 DESIGN document:
 *
 * The purpose of this function is to find out all the information
 * required to determine if the configuration is usable, and to initialise
 * those parts of the ScrnInfoRec that can be set once at the beginning of
 * the first server generation.
 *
 * (...)
 *
 * This includes probing for video memory, clocks, ramdac, and all other
 * HW info that is needed. It includes determining the depth/bpp/visual
 * and related info. It includes validating and determining the set of
 * video modes that will be used (and anything that is required to
 * determine that).
 *
 * This information should be determined in the least intrusive way
 * possible. The state of the HW must remain unchanged by this function.
 * Although video memory (including MMIO) may be mapped within this
 * function, it must be unmapped before returning.
 *
 * END QUOTE
 */

static Bool
VBOXPreInit(ScrnInfoPtr pScrn, int flags)
{
    VBOXPtr pVBox;
    Gamma gzeros = {0.0, 0.0, 0.0};
    rgb rzeros = {0, 0, 0};
    int i;
    DisplayModePtr pMode;
    enum { MODE_MIN_SIZE = 64 };

    TRACE_ENTRY();
    /* Are we really starting the server, or is this just a dummy run? */
    if (flags & PROBE_DETECT)
        return (FALSE);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
               "VirtualBox guest additions video driver version "
               VBOX_VERSION_STRING "\n");

     /* Get our private data from the ScrnInfoRec structure. */
    pVBox = VBOXGetRec(pScrn);

    /* Initialise the guest library */
    vbox_init(pScrn->scrnIndex, pVBox);

    /* Entity information seems to mean bus information. */
    pVBox->pEnt = xf86GetEntityInfo(pScrn->entityList[0]);
    if (pVBox->pEnt->location.type != BUS_PCI)
        return FALSE;

    /* The ramdac module is needed for the hardware cursor. */
    if (!xf86LoadSubModule(pScrn, "ramdac"))
        return FALSE;
    xf86LoaderReqSymLists(ramdacSymbols, NULL);

    /* We need the vbe module because we use VBE code to save and restore
       text mode, in order to keep our code simple. */
    if (!xf86LoadSubModule(pScrn, "vbe"))
        return (FALSE);
    xf86LoaderReqSymLists(vbeSymbols, NULL);

    /* The framebuffer module. */
    if (xf86LoadSubModule(pScrn, "fb") == NULL)
        return (FALSE);
    xf86LoaderReqSymLists(fbSymbols, NULL);

    if (!xf86LoadSubModule(pScrn, "shadowfb"))
        return FALSE;
    xf86LoaderReqSymLists(shadowfbSymbols, NULL);

    pVBox->pciInfo = xf86GetPciInfoForEntity(pVBox->pEnt->index);
    pVBox->pciTag = pciTag(pVBox->pciInfo->bus,
                           pVBox->pciInfo->device,
                           pVBox->pciInfo->func);

    /* Set up our ScrnInfoRec structure to describe our virtual
       capabilities to X. */

    pScrn->rgbBits = 8;

    /* Let's create a nice, capable virtual monitor. */
    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->monitor->DDC = NULL;
    pScrn->monitor->nHsync = 1;
    pScrn->monitor->hsync[0].lo = 1;
    pScrn->monitor->hsync[0].hi = 10000;
    pScrn->monitor->nVrefresh = 1;
    pScrn->monitor->vrefresh[0].lo = 1;
    pScrn->monitor->vrefresh[0].hi = 100;

    pScrn->chipset = "vbox";
    pScrn->progClock = TRUE;

    /* Determine the size of the VBox video RAM from PCI data*/
#if 0
    pScrn->videoRam = 1 << pVBox->pciInfo->size[0];
#endif
    /* Using the PCI information caused problems with non-powers-of-two
       sized video RAM configurations */
    pScrn->videoRam = inl(VBE_DISPI_IOPORT_DATA) / 1024;

    /* Set up clock information that will support all modes we need. */
    pScrn->clockRanges = xnfcalloc(sizeof(ClockRange), 1);
    pScrn->clockRanges->minClock = 1000;
    pScrn->clockRanges->maxClock = 1000000000;
    pScrn->clockRanges->clockIndex = -1;
    pScrn->clockRanges->ClockMulFactor = 1;
    pScrn->clockRanges->ClockDivFactor = 1;

    /* Determine the preferred size and colour depth and setup video modes */
    {
        uint32_t cx = 0, cy = 0, cBits = 0;

        vboxGetPreferredMode(pScrn, &cx, &cy, &cBits);
        /* We only support 16 and 24 bits depth (i.e. 16 and 32bpp) */
        if (cBits != 16)
            cBits = 24;
        if (!xf86SetDepthBpp(pScrn, cBits, 0, 0, Support32bppFb))
            return FALSE;
        vboxAddModes(pScrn, cx, cy);
    }
    if (pScrn->bitsPerPixel != 32 && pScrn->bitsPerPixel != 16)
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "The VBox additions only support 16 and 32bpp graphics modes\n");
        return FALSE;
    }
    xf86PrintDepthBpp(pScrn);

    /* Colour weight - we always call this, since we are always in
       truecolour. */
    if (!xf86SetWeight(pScrn, rzeros, rzeros))
        return (FALSE);

    /* visual init */
    if (!xf86SetDefaultVisual(pScrn, -1))
        return (FALSE);

    xf86SetGamma(pScrn, gzeros);

    /* We don't validate with xf86ValidateModes and xf86PruneModes as we
     * already know what we like and what we don't. */

    pScrn->currentMode = pScrn->modes;

    /* Set the right virtual resolution. */
    pScrn->virtualX = pScrn->currentMode->HDisplay;
    pScrn->virtualY = pScrn->currentMode->VDisplay;

    pScrn->displayWidth = pScrn->virtualX;

    xf86PrintModes(pScrn);

    /* Set the DPI.  Perhaps we should read this from the host? */
    xf86SetDpi(pScrn, 96, 96);

    /* options */
    xf86CollectOptions(pScrn, NULL);
    if (!(pVBox->Options = xalloc(sizeof(VBOXOptions))))
        return FALSE;
    memcpy(pVBox->Options, VBOXOptions, sizeof(VBOXOptions));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, pVBox->Options);

    /* Framebuffer-related setup */
    pScrn->bitmapBitOrder = BITMAP_BIT_ORDER;
    TRACE_EXIT();
    return (TRUE);
}

/*
 * QUOTE from the XFree86 DESIGN document:
 *
 * This is called at the start of each server generation.
 *
 * (...)
 *
 * Decide which operations need to be placed under resource access
 * control. (...) Map any video memory or other memory regions. (...)
 * Save the video card state. (...) Initialise the initial video
 * mode.
 *
 * End QUOTE.
 */
static Bool
VBOXScreenInit(int scrnIndex, ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    VBOXPtr pVBox = VBOXGetRec(pScrn);
    VisualPtr visual;
    unsigned flags;

    TRACE_ENTRY();
    /* We make use of the X11 VBE code to save and restore text mode, in
       order to keep our code simple. */
    if ((pVBox->pVbe = VBEExtendedInit(NULL, pVBox->pEnt->index,
                                       SET_BIOS_SCRATCH
                                       | RESTORE_BIOS_SCRATCH)) == NULL)
        return (FALSE);

    if (pVBox->mapPhys == 0) {
        pVBox->mapPhys = pVBox->pciInfo->memBase[0];
/*        pVBox->mapSize = 1 << pVBox->pciInfo->size[0]; */
        /* Using the PCI information caused problems with
           non-powers-of-two sized video RAM configurations */
        pVBox->mapSize = inl(VBE_DISPI_IOPORT_DATA);
        pVBox->mapOff = 0;
    }

    if (!VBOXMapVidMem(pScrn))
        return (FALSE);

    /* save current video state */
    VBOXSaveRestore(pScrn, MODE_SAVE);
    pVBox->savedPal = VBESetGetPaletteData(pVBox->pVbe, FALSE, 0, 256,
                                           NULL, FALSE, FALSE);

    /* set the viewport */
    VBOXAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);

    /* Blank the screen for aesthetic reasons. */
    VBOXSaveScreen(pScreen, SCREEN_SAVER_ON);

    /* mi layer - reset the visual list (?)*/
    miClearVisualTypes();
    if (!xf86SetDefaultVisual(pScrn, -1))
        return (FALSE);
    if (!miSetVisualTypes(pScrn->depth, TrueColorMask,
                          pScrn->rgbBits, TrueColor))
        return (FALSE);
    if (!miSetPixmapDepths())
        return (FALSE);

    /* I checked in the sources, and XFree86 4.2 does seem to support
       this function for 32bpp. */
    if (!fbScreenInit(pScreen, pVBox->base,
                      pScrn->virtualX, pScrn->virtualY,
                      pScrn->xDpi, pScrn->yDpi,
                      pScrn->displayWidth, pScrn->bitsPerPixel))
        return (FALSE);

    /* Fixup RGB ordering */
    visual = pScreen->visuals + pScreen->numVisuals;
    while (--visual >= pScreen->visuals) {
        if ((visual->class | DynamicClass) == DirectColor) {
            visual->offsetRed   = pScrn->offset.red;
            visual->offsetGreen = pScrn->offset.green;
            visual->offsetBlue  = pScrn->offset.blue;
            visual->redMask     = pScrn->mask.red;
            visual->greenMask   = pScrn->mask.green;
            visual->blueMask    = pScrn->mask.blue;
        }
    }

    /* must be after RGB ordering fixed */
    fbPictureInit(pScreen, 0, 0);

    xf86SetBlackWhitePixels(pScreen);
    miInitializeBackingStore(pScreen);
    xf86SetBackingStore(pScreen);

    /* software cursor */
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* colourmap code - apparently, we need this even in Truecolour */
    if (!miCreateDefColormap(pScreen))
	return (FALSE);

    flags = CMAP_RELOAD_ON_MODE_SWITCH;

    if(!xf86HandleColormaps(pScreen, 256,
        8 /* DAC is switchable to 8 bits per primary color */,
        VBOXLoadPalette, NULL, flags))
        return (FALSE);

    pVBox->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = VBOXCloseScreen;
    pScreen->SaveScreen = VBOXSaveScreen;

    /* However, we probably do want to support power management -
       even if we just use a dummy function. */
    xf86DPMSInit(pScreen, VBOXDisplayPowerManagementSet, 0);

    /* Report any unused options (only for the first generation) */
    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    /* set first video mode */
    if (!VBOXSetMode(pScrn, pScrn->currentMode))
        return FALSE;
    /* And make sure that a non-current dynamic mode is at the front of the
     * list */
    vboxWriteHostModes(pScrn, pScrn->currentMode);

    if (vbox_open (pScrn, pScreen, pVBox)) {
        if (vbox_cursor_init(pScreen) != TRUE)
            xf86DrvMsg(scrnIndex, X_ERROR,
                       "Unable to start the VirtualBox mouse pointer integration with the host system.\n");
        if (vboxEnableVbva(pScrn) == TRUE)
            xf86DrvMsg(scrnIndex, X_INFO,
                      "The VBox video extensions are now enabled.\n");
    }
    TRACE_EXIT();
    return (TRUE);
}

static Bool
VBOXEnterVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    VBOXPtr pVBox = VBOXGetRec(pScrn);

    if (!VBOXSetMode(pScrn, pScrn->currentMode))
        return FALSE;
    VBOXAdjustFrame(scrnIndex, pScrn->frameX0, pScrn->frameY0, 0);
    if (pVBox->useVbva == TRUE)
        vboxEnableVbva(pScrn);
    return TRUE;
}

static void
VBOXLeaveVT(int scrnIndex, int flags)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    VBOXPtr pVBox = VBOXGetRec(pScrn);

    VBOXSaveRestore(pScrn, MODE_RESTORE);
    if (pVBox->useVbva == TRUE)
        vboxDisableVbva(pScrn);
}

static Bool
VBOXCloseScreen(int scrnIndex, ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];
    VBOXPtr pVBox = VBOXGetRec(pScrn);

    if (pVBox->useVbva == TRUE)
        vboxDisableVbva(pScrn);
    if (pScrn->vtSema) {
	VBOXSaveRestore(xf86Screens[scrnIndex], MODE_RESTORE);
	if (pVBox->savedPal)
	    VBESetGetPaletteData(pVBox->pVbe, TRUE, 0, 256,
				 pVBox->savedPal, FALSE, TRUE);
	VBOXUnmapVidMem(pScrn);
    }
    pScrn->vtSema = FALSE;

    pScreen->CloseScreen = pVBox->CloseScreen;
    return pScreen->CloseScreen(scrnIndex, pScreen);
}

static Bool
VBOXSwitchMode(int scrnIndex, DisplayModePtr pMode, int flags)
{
    ScrnInfoPtr pScrn;
    VBOXPtr pVBox;

    pScrn = xf86Screens[scrnIndex];  /* Why does X have three ways of refering to the screen? */
    pVBox = VBOXGetRec(pScrn);
    if (pVBox->useVbva == TRUE)
        if (vboxDisableVbva(pScrn) != TRUE)  /* This would be bad. */
            return FALSE;
    if (VBOXSetMode(pScrn, pMode) != TRUE)
        return FALSE;
    if (VBOXAdjustScreenPixmap(pScrn, pMode) != TRUE)
        return FALSE;
    vboxSaveVideoMode(pScrn, pMode->HDisplay, pMode->VDisplay,
                      pScrn->bitsPerPixel);
    vboxWriteHostModes(pScrn, pMode);
    xf86PrintModes(pScrn);
    if (pVBox->useVbva == TRUE)
        if (vboxEnableVbva(pScrn) != TRUE)  /* Bad but not fatal */
            pVBox->useVbva = FALSE;
    return TRUE;
}

/* Set a graphics mode */
static Bool
VBOXSetMode(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
    TRACE_ENTRY();
    int bpp = pScrn->depth == 24 ? 32 : 16;
    VBOXPtr pVBox = VBOXGetRec(pScrn);

    if (pScrn->virtualX * pScrn->virtualY * bpp / 8
        >= pScrn->videoRam * 1024)
    {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Unable to set up a virtual screen size of %dx%d with %d Kb of video memory.  Please increase the video memory size.\n",
                   pScrn->virtualX, pScrn->virtualY, pScrn->videoRam);
        return FALSE;
    }
    /* Do not reset the current mode - we use this as a way of kicking
     * the driver */
    if (   (pMode->HDisplay == pVBox->cLastWidth)
        && (pMode->VDisplay == pVBox->cLastHeight))
        return TRUE;

    pScrn->vtSema = TRUE;
    /* Disable linear framebuffer mode before making changes to the resolution. */
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    outw(VBE_DISPI_IOPORT_DATA,
         VBE_DISPI_DISABLED);
    /* Unlike the resolution, the depth is fixed for a given screen
       for the lifetime of the X session. */
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_BPP);
    outw(VBE_DISPI_IOPORT_DATA, bpp);
    /* HDisplay and VDisplay are actually monitor information about
       the display part of the scanlines. */
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_XRES);
    outw(VBE_DISPI_IOPORT_DATA, pMode->HDisplay);
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_YRES);
    outw(VBE_DISPI_IOPORT_DATA, pMode->VDisplay);
    /* Enable linear framebuffer mode. */
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_ENABLE);
    outw(VBE_DISPI_IOPORT_DATA,
         VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);
    /* Set the virtual resolution.  We are still using VESA to control
       the virtual offset. */
    outw(VBE_DISPI_IOPORT_INDEX, VBE_DISPI_INDEX_VIRT_WIDTH);
    outw(VBE_DISPI_IOPORT_DATA, pMode->HDisplay);
    pVBox->cLastWidth = pMode->HDisplay;
    pVBox->cLastHeight = pMode->VDisplay;
    TRACE_EXIT();
    return (TRUE);
}

static bool VBOXAdjustScreenPixmap(ScrnInfoPtr pScrn, DisplayModePtr pMode)
{
    ScreenPtr pScreen = pScrn->pScreen;
    PixmapPtr pPixmap = pScreen->GetScreenPixmap(pScreen);
    VBOXPtr pVBox = VBOXGetRec(pScrn);
    int bpp = pScrn->depth == 24 ? 32 : 16;
    if (!pPixmap) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to get the screen pixmap.\n");
        return FALSE;
    }
    pScreen->ModifyPixmapHeader(pPixmap, pMode->HDisplay, pMode->VDisplay,
                                pScrn->depth, bpp, pMode->HDisplay * bpp / 8,
                                pVBox->base);
    pScrn->EnableDisableFBAccess(pScrn->scrnIndex, FALSE);
    pScrn->EnableDisableFBAccess(pScrn->scrnIndex, TRUE);
    return TRUE;
}

static void
VBOXAdjustFrame(int scrnIndex, int x, int y, int flags)
{
    VBOXPtr pVBox = VBOXGetRec(xf86Screens[scrnIndex]);
    ScrnInfoPtr pScrn = xf86Screens[scrnIndex];

    pVBox->viewportX = x;
    pVBox->viewportY = y;
    /* If VBVA is enabled the graphics card will not notice the change. */
    if (pVBox->useVbva == TRUE)
        vboxDisableVbva(pScrn);
    VBESetDisplayStart(pVBox->pVbe, x, y, TRUE);
    if (pVBox->useVbva == TRUE)
        vboxEnableVbva(pScrn);
}

static void
VBOXFreeScreen(int scrnIndex, int flags)
{
    VBOXFreeRec(xf86Screens[scrnIndex]);
}

static Bool
VBOXMapVidMem(ScrnInfoPtr pScrn)
{
    VBOXPtr pVBox = VBOXGetRec(pScrn);

    if (pVBox->base != NULL)
        return (TRUE);

    pScrn->memPhysBase = pVBox->mapPhys;
    pScrn->fbOffset = pVBox->mapOff;

    pVBox->base = xf86MapPciMem(pScrn->scrnIndex,
                                VIDMEM_FRAMEBUFFER,
                                pVBox->pciTag, pVBox->mapPhys,
                                (unsigned) pVBox->mapSize);

    if (pVBox->base) {
        pScrn->memPhysBase = pVBox->mapPhys;
        pVBox->VGAbase = xf86MapDomainMemory(pScrn->scrnIndex, 0,
                                             pVBox->pciTag,
                                             0xa0000, 0x10000);
    }
    /* We need this for saving/restoring textmode */
    pVBox->ioBase = pScrn->domainIOBase;

    return (pVBox->base != NULL);
}

static void
VBOXUnmapVidMem(ScrnInfoPtr pScrn)
{
    VBOXPtr pVBox = VBOXGetRec(pScrn);

    if (pVBox->base == NULL)
        return;

    xf86UnMapVidMem(pScrn->scrnIndex, pVBox->base,
                    (unsigned) pVBox->mapSize);
    xf86UnMapVidMem(pScrn->scrnIndex, pVBox->VGAbase, 0x10000);
    pVBox->base = NULL;
}

static void
VBOXLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
		LOCO *colors, VisualPtr pVisual)
{
    VBOXPtr pVBox = VBOXGetRec(pScrn);
    int i, idx;
#define VBOXDACDelay()							    \
    do {								    \
	   (void)inb(pVBox->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET); \
	   (void)inb(pVBox->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET); \
    } while (0)

    for (i = 0; i < numColors; i++) {
	   idx = indices[i];
	   outb(pVBox->ioBase + VGA_DAC_WRITE_ADDR, idx);
	   VBOXDACDelay();
	   outb(pVBox->ioBase + VGA_DAC_DATA, colors[idx].red);
	   VBOXDACDelay();
	   outb(pVBox->ioBase + VGA_DAC_DATA, colors[idx].green);
	   VBOXDACDelay();
	   outb(pVBox->ioBase + VGA_DAC_DATA, colors[idx].blue);
	   VBOXDACDelay();
    }
}

/*
 * Just adapted from the std* functions in vgaHW.c
 */
static void
WriteAttr(VBOXPtr pVBox, int index, int value)
{
    (void) inb(pVBox->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET);

    index |= 0x20;
    outb(pVBox->ioBase + VGA_ATTR_INDEX, index);
    outb(pVBox->ioBase + VGA_ATTR_DATA_W, value);
}

static int
ReadAttr(VBOXPtr pVBox, int index)
{
    (void) inb(pVBox->ioBase + VGA_IOBASE_COLOR + VGA_IN_STAT_1_OFFSET);

    index |= 0x20;
    outb(pVBox->ioBase + VGA_ATTR_INDEX, index);
    return (inb(pVBox->ioBase + VGA_ATTR_DATA_R));
}

#define WriteMiscOut(value)	outb(pVBox->ioBase + VGA_MISC_OUT_W, value)
#define ReadMiscOut()		inb(pVBox->ioBase + VGA_MISC_OUT_R)
#define WriteSeq(index, value) \
        outb(pVBox->ioBase + VGA_SEQ_INDEX, (index));\
        outb(pVBox->ioBase + VGA_SEQ_DATA, value)

static int
ReadSeq(VBOXPtr pVBox, int index)
{
    outb(pVBox->ioBase + VGA_SEQ_INDEX, index);

    return (inb(pVBox->ioBase + VGA_SEQ_DATA));
}

#define WriteGr(index, value)				\
    outb(pVBox->ioBase + VGA_GRAPH_INDEX, index);	\
    outb(pVBox->ioBase + VGA_GRAPH_DATA, value)

static int
ReadGr(VBOXPtr pVBox, int index)
{
    outb(pVBox->ioBase + VGA_GRAPH_INDEX, index);

    return (inb(pVBox->ioBase + VGA_GRAPH_DATA));
}

#define WriteCrtc(index, value)						     \
    outb(pVBox->ioBase + (VGA_IOBASE_COLOR + VGA_CRTC_INDEX_OFFSET), index); \
    outb(pVBox->ioBase + (VGA_IOBASE_COLOR + VGA_CRTC_DATA_OFFSET), value)

static void
SeqReset(VBOXPtr pVBox, Bool start)
{
    if (start) {
	   WriteSeq(0x00, 0x01);		/* Synchronous Reset */
    }
    else {
	   WriteSeq(0x00, 0x03);		/* End Reset */
    }
}

static void
SaveFonts(ScrnInfoPtr pScrn)
{
    VBOXPtr pVBox = VBOXGetRec(pScrn);
    unsigned char miscOut, attr10, gr4, gr5, gr6, seq2, seq4, scrn;

    if (pVBox->fonts != NULL)
	return;

    /* If in graphics mode, don't save anything */
    attr10 = ReadAttr(pVBox, 0x10);
    if (attr10 & 0x01)
	return;

    pVBox->fonts = xalloc(16384);

    /* save the registers that are needed here */
    miscOut = ReadMiscOut();
    gr4 = ReadGr(pVBox, 0x04);
    gr5 = ReadGr(pVBox, 0x05);
    gr6 = ReadGr(pVBox, 0x06);
    seq2 = ReadSeq(pVBox, 0x02);
    seq4 = ReadSeq(pVBox, 0x04);

    /* Force into colour mode */
    WriteMiscOut(miscOut | 0x01);

    scrn = ReadSeq(pVBox, 0x01) | 0x20;
    SeqReset(pVBox, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVBox, FALSE);

    WriteAttr(pVBox, 0x10, 0x01);	/* graphics mode */

    /*font1 */
    WriteSeq(0x02, 0x04);	/* write to plane 2 */
    WriteSeq(0x04, 0x06);	/* enable plane graphics */
    WriteGr(0x04, 0x02);	/* read plane 2 */
    WriteGr(0x05, 0x00);	/* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);	/* set graphics */
    slowbcopy_frombus(pVBox->VGAbase, pVBox->fonts, 8192);

    /* font2 */
    WriteSeq(0x02, 0x08);	/* write to plane 3 */
    WriteSeq(0x04, 0x06);	/* enable plane graphics */
    WriteGr(0x04, 0x03);	/* read plane 3 */
    WriteGr(0x05, 0x00);	/* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);	/* set graphics */
    slowbcopy_frombus(pVBox->VGAbase, pVBox->fonts + 8192, 8192);

    scrn = ReadSeq(pVBox, 0x01) & ~0x20;
    SeqReset(pVBox, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVBox, FALSE);

    /* Restore clobbered registers */
    WriteAttr(pVBox, 0x10, attr10);
    WriteSeq(0x02, seq2);
    WriteSeq(0x04, seq4);
    WriteGr(0x04, gr4);
    WriteGr(0x05, gr5);
    WriteGr(0x06, gr6);
    WriteMiscOut(miscOut);
}

static void
RestoreFonts(ScrnInfoPtr pScrn)
{
    VBOXPtr pVBox = VBOXGetRec(pScrn);
    unsigned char miscOut, attr10, gr1, gr3, gr4, gr5, gr6, gr8, seq2, seq4, scrn;

    if (pVBox->fonts == NULL)
        return;

    /* save the registers that are needed here */
    miscOut = ReadMiscOut();
    attr10 = ReadAttr(pVBox, 0x10);
    gr1 = ReadGr(pVBox, 0x01);
    gr3 = ReadGr(pVBox, 0x03);
    gr4 = ReadGr(pVBox, 0x04);
    gr5 = ReadGr(pVBox, 0x05);
    gr6 = ReadGr(pVBox, 0x06);
    gr8 = ReadGr(pVBox, 0x08);
    seq2 = ReadSeq(pVBox, 0x02);
    seq4 = ReadSeq(pVBox, 0x04);

    /* Force into colour mode */
    WriteMiscOut(miscOut | 0x01);

    scrn = ReadSeq(pVBox, 0x01) & ~0x20;
    SeqReset(pVBox, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVBox, FALSE);

    WriteAttr(pVBox, 0x10, 0x01);	/* graphics mode */
    if (pScrn->depth == 4) {
	/* GJA */
	WriteGr(0x03, 0x00);	/* don't rotate, write unmodified */
	WriteGr(0x08, 0xFF);	/* write all bits in a byte */
	WriteGr(0x01, 0x00);	/* all planes come from CPU */
    }

    WriteSeq(0x02, 0x04);   /* write to plane 2 */
    WriteSeq(0x04, 0x06);   /* enable plane graphics */
    WriteGr(0x04, 0x02);    /* read plane 2 */
    WriteGr(0x05, 0x00);    /* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);    /* set graphics */
    slowbcopy_tobus(pVBox->fonts, pVBox->VGAbase, 8192);

    WriteSeq(0x02, 0x08);   /* write to plane 3 */
    WriteSeq(0x04, 0x06);   /* enable plane graphics */
    WriteGr(0x04, 0x03);    /* read plane 3 */
    WriteGr(0x05, 0x00);    /* write mode 0, read mode 0 */
    WriteGr(0x06, 0x05);    /* set graphics */
    slowbcopy_tobus(pVBox->fonts + 8192, pVBox->VGAbase, 8192);

    scrn = ReadSeq(pVBox, 0x01) & ~0x20;
    SeqReset(pVBox, TRUE);
    WriteSeq(0x01, scrn);
    SeqReset(pVBox, FALSE);

    /* restore the registers that were changed */
    WriteMiscOut(miscOut);
    WriteAttr(pVBox, 0x10, attr10);
    WriteGr(0x01, gr1);
    WriteGr(0x03, gr3);
    WriteGr(0x04, gr4);
    WriteGr(0x05, gr5);
    WriteGr(0x06, gr6);
    WriteGr(0x08, gr8);
    WriteSeq(0x02, seq2);
    WriteSeq(0x04, seq4);
}

static Bool
VBOXSaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    VBOXPtr pVBox = VBOXGetRec(pScrn);
    Bool on = xf86IsUnblank(mode);

    if (on)
	SetTimeSinceLastInputEvent();

    if (pScrn->vtSema) {
	unsigned char scrn = ReadSeq(pVBox, 0x01);

	if (on)
	    scrn &= ~0x20;
	else
	    scrn |= 0x20;
	SeqReset(pVBox, TRUE);
 	WriteSeq(0x01, scrn);
	SeqReset(pVBox, FALSE);
    }

    return (TRUE);
}

Bool
VBOXSaveRestore(ScrnInfoPtr pScrn, vbeSaveRestoreFunction function)
{
    VBOXPtr pVBox;

    if (MODE_QUERY < 0 || function > MODE_RESTORE)
	return (FALSE);

    pVBox = VBOXGetRec(pScrn);


    /* Query amount of memory to save state */
    if (function == MODE_QUERY ||
	(function == MODE_SAVE && pVBox->state == NULL))
    {

	/* Make sure we save at least this information in case of failure */
	(void)VBEGetVBEMode(pVBox->pVbe, &pVBox->stateMode);
	SaveFonts(pScrn);

        if (!VBESaveRestore(pVBox->pVbe,function,(pointer)&pVBox->state,
                            &pVBox->stateSize,&pVBox->statePage))
            return FALSE;
    }

    /* Save/Restore Super VGA state */
    if (function != MODE_QUERY) {
        Bool retval = TRUE;

        if (function == MODE_RESTORE)
            memcpy(pVBox->state, pVBox->pstate,
                   (unsigned) pVBox->stateSize);

        if ((retval = VBESaveRestore(pVBox->pVbe,function,
                                     (pointer)&pVBox->state,
                                     &pVBox->stateSize,&pVBox->statePage))
             && function == MODE_SAVE)
        {
            /* don't rely on the memory not being touched */
            if (pVBox->pstate == NULL)
                pVBox->pstate = xalloc(pVBox->stateSize);
            memcpy(pVBox->pstate, pVBox->state,
                   (unsigned) pVBox->stateSize);
        }

        if (function == MODE_RESTORE)
        {
            VBESetVBEMode(pVBox->pVbe, pVBox->stateMode, NULL);
            RestoreFonts(pScrn);
        }

	if (!retval)
	    return (FALSE);

    }

    return (TRUE);
}

static void
VBOXDisplayPowerManagementSet(ScrnInfoPtr pScrn, int mode,
                int flags)
{
    /* VBox is always power efficient... */
}


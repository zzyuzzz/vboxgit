/* $Id: DevVGA-SVGA3d.h 94265 2022-03-16 10:33:19Z vboxsync $ */
/** @file
 * DevVMWare - VMWare SVGA device - 3D part.
 */

/*
 * Copyright (C) 2013-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_Graphics_DevVGA_SVGA3d_h
#define VBOX_INCLUDED_SRC_Graphics_DevVGA_SVGA3d_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "DevVGA-SVGA.h"


/** Arbitrary limit */
#define SVGA3D_MAX_SHADER_IDS                   0x800
/** D3D allows up to 8 texture stages. */
#define SVGA3D_MAX_TEXTURE_STAGES               8
/** Samplers: 16 Pixel Shader + 1 Displacement Map + 4 Vertex Shader */
#define SVGA3D_MAX_SAMPLERS_PS         16
#define SVGA3D_MAX_SAMPLERS_DMAP       1
#define SVGA3D_MAX_SAMPLERS_VS         4
#define SVGA3D_MAX_SAMPLERS            (SVGA3D_MAX_SAMPLERS_PS + SVGA3D_MAX_SAMPLERS_DMAP + SVGA3D_MAX_SAMPLERS_VS)
/** Arbitrary upper limit; seen 8 so far. */
#define SVGA3D_MAX_LIGHTS                       32
/** Arbitrary upper limit; 2GB enough for 32768x16384*4. */
#define SVGA3D_MAX_SURFACE_MEM_SIZE             0x80000000
/** Arbitrary upper limit. [0,15] is enough for 2^15=32768x32768. */
#define SVGA3D_MAX_MIP_LEVELS                   16


/** @todo Use this as a parameter for vmsvga3dSurfaceDefine and a field in VMSVGA3DSURFACE instead of a multiple values. */
/* A surface description provided by the guest. Mostly mirrors SVGA3dCmdDefineGBSurface_v4 */
typedef struct VMSVGA3D_SURFACE_DESC
{
   SVGA3dSurface1Flags surface1Flags;
   SVGA3dSurface2Flags surface2Flags;
   SVGA3dSurfaceFormat format;
   uint32 numMipLevels;
   uint32 multisampleCount;
   SVGA3dMSPattern multisamplePattern;
   SVGA3dMSQualityLevel qualityLevel;
   SVGA3dTextureFilter autogenFilter;
   SVGA3dSize size;
   uint32 numArrayElements; /* "Number of array elements for a 1D/2D texture. For cubemap
                             * texture number of faces * array_size."
                             */
   uint32 bufferByteStride;
} VMSVGA3D_SURFACE_DESC;

typedef enum VMSVGA3D_SURFACE_MAP
{
    VMSVGA3D_SURFACE_MAP_READ,
    VMSVGA3D_SURFACE_MAP_WRITE,
    VMSVGA3D_SURFACE_MAP_READ_WRITE,
    VMSVGA3D_SURFACE_MAP_WRITE_DISCARD,
} VMSVGA3D_SURFACE_MAP;

typedef struct VMSVGA3D_MAPPED_SURFACE
{
    VMSVGA3D_SURFACE_MAP enmMapType;
    SVGA3dSurfaceFormat format;
    SVGA3dBox box;
    uint32_t cbPixel;
    uint32_t cbRowPitch;
    uint32_t cbDepthPitch;
    void *pvData;
} VMSVGA3D_MAPPED_SURFACE;

/* Write render targets to bitmaps. */
//#define DUMP_BITMAPS
void vmsvga3dMapWriteBmpFile(VMSVGA3D_MAPPED_SURFACE const *pMap, char const *pszPrefix);

/* DevVGA-SVGA.cpp: */
void vmsvgaR33dSurfaceUpdateHeapBuffersOnFifoThread(PPDMDEVINS pDevIns, PVGASTATE pThis, PVGASTATECC pThisCC, uint32_t sid);


/* DevVGA-SVGA3d-ogl.cpp & DevVGA-SVGA3d-win.cpp: */
int vmsvga3dLoadExec(PPDMDEVINS pDevIns, PVGASTATE pThis, PVGASTATECC pThisCC, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
int vmsvga3dSaveExec(PPDMDEVINS pDevIns, PVGASTATECC pThisCC, PSSMHANDLE pSSM);
void vmsvga3dUpdateHostScreenViewport(PVGASTATECC pThisCC, uint32_t idScreen, VMSVGAVIEWPORT const *pOldViewport);
int vmsvga3dQueryCaps(PVGASTATECC pThisCC, SVGA3dDevCapIndex idx3dCaps, uint32_t *pu32Val);

int vmsvga3dSurfaceDefine(PVGASTATECC pThisCC, uint32_t sid, SVGA3dSurface1Flags surfaceFlags, SVGA3dSurfaceFormat format,
                          uint32_t multisampleCount, SVGA3dTextureFilter autogenFilter,
                          uint32_t cMipLevels, SVGA3dSize const *pMipLevel0Size, uint32_t arraySize, bool fAllocMipLevels);
int vmsvga3dSurfaceDestroy(PVGASTATECC pThisCC, uint32_t sid);
int vmsvga3dSurfaceCopy(PVGASTATECC pThisCC, SVGA3dSurfaceImageId dest, SVGA3dSurfaceImageId src,
                        uint32_t cCopyBoxes, SVGA3dCopyBox *pBox);
int vmsvga3dSurfaceStretchBlt(PVGASTATE pThis, PVGASTATECC pThisCC,
                              SVGA3dSurfaceImageId const *pDstSfcImg, SVGA3dBox const *pDstBox,
                              SVGA3dSurfaceImageId const *pSrcSfcImg, SVGA3dBox const *pSrcBox, SVGA3dStretchBltMode enmMode);
int vmsvga3dSurfaceDMA(PVGASTATE pThis, PVGASTATECC pThisCC, SVGAGuestImage guest, SVGA3dSurfaceImageId host, SVGA3dTransferType transfer, uint32_t cCopyBoxes, SVGA3dCopyBox *pBoxes);
int vmsvga3dSurfaceBlitToScreen(PVGASTATE pThis, PVGASTATECC pThisCC, uint32_t dest, SVGASignedRect destRect, SVGA3dSurfaceImageId srcImage, SVGASignedRect srcRect, uint32_t cRects, SVGASignedRect *pRect);

int vmsvga3dContextDefine(PVGASTATECC pThisCC, uint32_t cid);
int vmsvga3dContextDestroy(PVGASTATECC pThisCC, uint32_t cid);

int vmsvga3dChangeMode(PVGASTATECC pThisCC);

int vmsvga3dDefineScreen(PVGASTATE pThis, PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen);
int vmsvga3dDestroyScreen(PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen);

int vmsvga3dSetTransform(PVGASTATECC pThisCC, uint32_t cid, SVGA3dTransformType type, float matrix[16]);
int vmsvga3dSetZRange(PVGASTATECC pThisCC, uint32_t cid, SVGA3dZRange zRange);
int vmsvga3dSetRenderState(PVGASTATECC pThisCC, uint32_t cid, uint32_t cRenderStates, SVGA3dRenderState *pRenderState);
int vmsvga3dSetRenderTarget(PVGASTATECC pThisCC, uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId target);
int vmsvga3dSetTextureState(PVGASTATECC pThisCC, uint32_t cid, uint32_t cTextureStates, SVGA3dTextureState *pTextureState);
int vmsvga3dSetMaterial(PVGASTATECC pThisCC, uint32_t cid, SVGA3dFace face, SVGA3dMaterial *pMaterial);
int vmsvga3dSetLightData(PVGASTATECC pThisCC, uint32_t cid, uint32_t index, SVGA3dLightData *pData);
int vmsvga3dSetLightEnabled(PVGASTATECC pThisCC, uint32_t cid, uint32_t index, uint32_t enabled);
int vmsvga3dSetViewPort(PVGASTATECC pThisCC, uint32_t cid, SVGA3dRect *pRect);
int vmsvga3dSetClipPlane(PVGASTATECC pThisCC, uint32_t cid,  uint32_t index, float plane[4]);
int vmsvga3dCommandClear(PVGASTATECC pThisCC, uint32_t cid, SVGA3dClearFlag clearFlag, uint32_t color, float depth, uint32_t stencil, uint32_t cRects, SVGA3dRect *pRect);
int vmsvga3dCommandPresent(PVGASTATE pThis, PVGASTATECC pThisCC, uint32_t sid, uint32_t cRects, SVGA3dCopyRect *pRect);
int vmsvga3dDrawPrimitives(PVGASTATECC pThisCC, uint32_t cid, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl, uint32_t numRanges, SVGA3dPrimitiveRange *pNumRange, uint32_t cVertexDivisor, SVGA3dVertexDivisor *pVertexDivisor);
int vmsvga3dSetScissorRect(PVGASTATECC pThisCC, uint32_t cid, SVGA3dRect *pRect);
int vmsvga3dGenerateMipmaps(PVGASTATECC pThisCC, uint32_t sid, SVGA3dTextureFilter filter);

int vmsvga3dShaderDefine(PVGASTATECC pThisCC, uint32_t cid, uint32_t shid, SVGA3dShaderType type, uint32_t cbData, uint32_t *pShaderData);
int vmsvga3dShaderDestroy(PVGASTATECC pThisCC, uint32_t cid, uint32_t shid, SVGA3dShaderType type);
int vmsvga3dShaderSet(PVGASTATECC pThisCC, struct VMSVGA3DCONTEXT *pContext, uint32_t cid, SVGA3dShaderType type, uint32_t shid);
int vmsvga3dShaderSetConst(PVGASTATECC pThisCC, uint32_t cid, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype, uint32_t cRegisters, uint32_t *pValues);

int vmsvga3dQueryCreate(PVGASTATECC pThisCC, uint32_t cid, SVGA3dQueryType type);
int vmsvga3dQueryBegin(PVGASTATECC pThisCC, uint32_t cid, SVGA3dQueryType type);
int vmsvga3dQueryEnd(PVGASTATECC pThisCC, uint32_t cid, SVGA3dQueryType type);
int vmsvga3dQueryWait(PVGASTATECC pThisCC, uint32_t cid, SVGA3dQueryType type, PVGASTATE pThis, SVGAGuestPtr const *pGuestResult);

int vmsvga3dSurfaceInvalidate(PVGASTATECC pThisCC, uint32_t sid, uint32_t face, uint32_t mipmap);

int vmsvga3dSurfaceMap(PVGASTATECC pThisCC, SVGA3dSurfaceImageId const *pImage, SVGA3dBox const *pBox,
                           VMSVGA3D_SURFACE_MAP enmMapType, VMSVGA3D_MAPPED_SURFACE *pMap);
int vmsvga3dSurfaceUnmap(PVGASTATECC pThisCC, SVGA3dSurfaceImageId const *pImage, VMSVGA3D_MAPPED_SURFACE *pMap, bool fWritten);

uint32_t vmsvga3dCalcSubresourceOffset(PVGASTATECC pThisCC, SVGA3dSurfaceImageId const *pImage);

typedef struct VMSGA3D_BOX_DIMENSIONS
{
    uint32_t offSubresource; /* Offset of the miplevel. */
    uint32_t offBox;         /* Offset of the box in the miplevel. */
    uint32_t cbRow;          /* Bytes per row. */
    int32_t  cbPitch;        /* Bytes between rows. */
    uint32_t cyBlocks;       /* Number of rows. */
    uint32_t cDepth;         /* Number of planes. */
} VMSGA3D_BOX_DIMENSIONS;

int vmsvga3dGetBoxDimensions(PVGASTATECC pThisCC, SVGA3dSurfaceImageId const *pImage, SVGA3dBox const *pBox,
                             VMSGA3D_BOX_DIMENSIONS *pResult);

DECLINLINE(void) vmsvga3dCalcMipmapSize(SVGA3dSize const *pSize0, uint32_t iMipmap, SVGA3dSize *pSize)
{
    pSize->width  = RT_MAX(pSize0->width  >> iMipmap, 1);
    pSize->height = RT_MAX(pSize0->height >> iMipmap, 1);
    pSize->depth  = RT_MAX(pSize0->depth  >> iMipmap, 1);
}

uint32_t vmsvga3dGetArrayElements(PVGASTATECC pThisCC, SVGA3dSurfaceId sid);
uint32_t vmsvga3dGetSubresourceCount(PVGASTATECC pThisCC, SVGA3dSurfaceId sid);

DECLINLINE(uint32_t) vmsvga3dCalcSubresource(uint32_t iMipLevel, uint32_t iArray, uint32_t cMipLevels)
{
    /* Same as in D3D */
    return iMipLevel + iArray * cMipLevels;
}

DECLINLINE(void) vmsvga3dCalcMipmapAndFace(uint32_t cMipLevels, uint32_t iSubresource, uint32_t *piMipmap, uint32_t *piFace)
{
    *piFace = iSubresource / cMipLevels;
    *piMipmap = iSubresource % cMipLevels;
}

int vmsvga3dCalcSurfaceMipmapAndFace(PVGASTATECC pThisCC, uint32_t sid, uint32_t iSubresource, uint32_t *piMipmap, uint32_t *piFace);


/* DevVGA-SVGA3d-shared.h: */
#if defined(RT_OS_WINDOWS) && defined(IN_RING3)
# include <iprt/win/windows.h>

# define WM_VMSVGA3D_WAKEUP                     (WM_APP+1)
# define WM_VMSVGA3D_CREATEWINDOW               (WM_APP+2)
# define WM_VMSVGA3D_DESTROYWINDOW              (WM_APP+3)
# define WM_VMSVGA3D_EXIT                       (WM_APP+5)
# if 0
#  define WM_VMSVGA3D_CREATE_DEVICE             (WM_APP+6)
typedef struct VMSVGA3DCREATEDEVICEPARAMS
{
    struct VMSVGA3DSTATE   *pState;
    struct VMSVGA3DCONTEXT *pContext;
    struct _D3DPRESENT_PARAMETERS_ *pPresParams;
    HRESULT                 hrc;
} VMSVGA3DCREATEDEVICEPARAMS;
# endif

DECLCALLBACK(int) vmsvga3dWindowThread(RTTHREAD ThreadSelf, void *pvUser);
int vmsvga3dSendThreadMessage(RTTHREAD pWindowThread, RTSEMEVENT WndRequestSem, UINT msg, WPARAM wParam, LPARAM lParam);
int vmsvga3dContextWindowCreate(HINSTANCE hInstance, RTTHREAD pWindowThread, RTSEMEVENT WndRequestSem, HWND *pHwnd);

#endif

void vmsvga3dUpdateHeapBuffersForSurfaces(PVGASTATECC pThisCC, uint32_t sid);
void vmsvga3dInfoContextWorker(PVGASTATECC pThisCC, PCDBGFINFOHLP pHlp, uint32_t cid, bool fVerbose);
void vmsvga3dInfoSurfaceWorker(PPDMDEVINS pDevIns, PVGASTATE pThis, PVGASTATECC pThisCC, PCDBGFINFOHLP pHlp, uint32_t sid,
                               bool fVerbose, uint32_t cxAscii, bool fInvY, const char *pszBitmapPath);

/* DevVGA-SVGA3d-shared.cpp: */

/**
 * Structure for use with vmsvga3dInfoU32Flags.
 */
typedef struct VMSVGAINFOFLAGS32
{
    /** The flags. */
    uint32_t    fFlags;
    /** The corresponding mnemonic. */
    const char *pszJohnny;
} VMSVGAINFOFLAGS32;
/** Pointer to a read-only flag translation entry. */
typedef VMSVGAINFOFLAGS32 const *PCVMSVGAINFOFLAGS32;
void vmsvga3dInfoU32Flags(PCDBGFINFOHLP pHlp, uint32_t fFlags, const char *pszPrefix, PCVMSVGAINFOFLAGS32 paFlags, uint32_t cFlags);

/**
 * Structure for use with vmsvgaFormatEnumValueEx and vmsvgaFormatEnumValue.
 */
typedef struct VMSVGAINFOENUM
{
    /** The enum value. */
    int32_t     iValue;
    /** The corresponding value name. */
    const char *pszName;
} VMSVGAINFOENUM;
/** Pointer to a read-only enum value translation entry. */
typedef VMSVGAINFOENUM const *PCVMSVGAINFOENUM;
/**
 * Structure for use with vmsvgaFormatEnumValueEx and vmsvgaFormatEnumValue.
 */
typedef struct VMSVGAINFOENUMMAP
{
    /** Pointer to the value mapping array. */
    PCVMSVGAINFOENUM    paValues;
    /** The number of value mappings. */
    size_t              cValues;
    /** The prefix. */
    const char         *pszPrefix;
#ifdef RT_STRICT
    /** Indicates whether we've checked that it's sorted or not. */
    bool               *pfAsserted;
#endif
} VMSVGAINFOENUMMAP;
typedef VMSVGAINFOENUMMAP const *PCVMSVGAINFOENUMMAP;
/** @def VMSVGAINFOENUMMAP_MAKE
 * Macro for defining a VMSVGAINFOENUMMAP, silently dealing with pfAsserted.
 *
 * @param   a_Scope     The scope. RT_NOTHING or static.
 * @param   a_VarName   The variable name for this map.
 * @param   a_aValues   The variable name of the value mapping array.
 * @param   a_pszPrefix The value name prefix.
 */
#ifdef VBOX_STRICT
# define VMSVGAINFOENUMMAP_MAKE(a_Scope, a_VarName, a_aValues, a_pszPrefix) \
    static bool RT_CONCAT(a_VarName,_AssertedSorted) = false; \
    a_Scope VMSVGAINFOENUMMAP const a_VarName = { \
        a_aValues, RT_ELEMENTS(a_aValues), a_pszPrefix, &RT_CONCAT(a_VarName,_AssertedSorted) \
    }
#else
# define VMSVGAINFOENUMMAP_MAKE(a_Scope, a_VarName, a_aValues, a_pszPrefix) \
    a_Scope VMSVGAINFOENUMMAP const a_VarName = { a_aValues, RT_ELEMENTS(a_aValues), a_pszPrefix }
#endif
extern VMSVGAINFOENUMMAP const g_SVGA3dSurfaceFormat2String;
const char *vmsvgaLookupEnum(int32_t iValue, PCVMSVGAINFOENUMMAP pEnumMap);
char *vmsvgaFormatEnumValueEx(char *pszBuffer, size_t cbBuffer, const char *pszName, int32_t iValue,
                              bool fPrefix, PCVMSVGAINFOENUMMAP pEnumMap);
char *vmsvgaFormatEnumValue(char *pszBuffer, size_t cbBuffer, const char *pszName, uint32_t uValue,
                            const char *pszPrefix, const char * const *papszValues, size_t cValues);

/**
 * ASCII "art" scanline printer callback.
 *
 * @param   pszLine         The line to output.
 * @param   pvUser          The user argument.
 */
typedef DECLCALLBACKTYPE(void, FNVMSVGAASCIIPRINTLN,(const char *pszLine, void *pvUser));
/** Pointer to an ASCII "art" print line callback. */
typedef FNVMSVGAASCIIPRINTLN *PFNVMSVGAASCIIPRINTLN;
void vmsvga3dAsciiPrint(PFNVMSVGAASCIIPRINTLN pfnPrintLine, void *pvUser, void const *pvImage, size_t cbImage,
                        uint32_t cx, uint32_t cy, uint32_t cbScanline, SVGA3dSurfaceFormat enmFormat, bool fInvY,
                        uint32_t cchMaxX, uint32_t cchMaxY);
DECLCALLBACK(void) vmsvga3dAsciiPrintlnInfo(const char *pszLine, void *pvUser);
DECLCALLBACK(void) vmsvga3dAsciiPrintlnLog(const char *pszLine, void *pvUser);

char *vmsvga3dFormatRenderState(char *pszBuffer, size_t cbBuffer, SVGA3dRenderState const *pRenderState);
char *vmsvga3dFormatTextureState(char *pszBuffer, size_t cbBuffer, SVGA3dTextureState const *pTextureState);
void vmsvga3dInfoHostWindow(PCDBGFINFOHLP pHlp, uint64_t idHostWindow);

uint32_t vmsvga3dSurfaceFormatSize(SVGA3dSurfaceFormat format,
                                   uint32_t *pu32BlockWidth,
                                   uint32_t *pu32BlockHeight);

#ifdef LOG_ENABLED
const char *vmsvga3dGetCapString(uint32_t idxCap);
const char *vmsvga3dGet3dFormatString(uint32_t format);
const char *vmsvga3dGetRenderStateName(uint32_t state);
const char *vmsvga3dTextureStateToString(SVGA3dTextureStateName textureState);
const char *vmsvgaTransformToString(SVGA3dTransformType type);
const char *vmsvgaDeclUsage2String(SVGA3dDeclUsage usage);
const char *vmsvgaDeclType2String(SVGA3dDeclType type);
const char *vmsvgaDeclMethod2String(SVGA3dDeclMethod method);
const char *vmsvgaSurfaceType2String(SVGA3dSurfaceFormat format);
const char *vmsvga3dPrimitiveType2String(SVGA3dPrimitiveType PrimitiveType);
#endif


/*
 * Backend interfaces.
 */
bool vmsvga3dIsLegacyBackend(PVGASTATECC pThisCC);

typedef struct VMSVGA3DSURFACE *PVMSVGA3DSURFACE;
typedef struct VMSVGA3DMIPMAPLEVEL *PVMSVGA3DMIPMAPLEVEL;
typedef struct VMSVGA3DCONTEXT *PVMSVGA3DCONTEXT;

/* Essential 3D backend function. */
#define VMSVGA3D_BACKEND_INTERFACE_NAME_3D "3D"
typedef struct
{
    DECLCALLBACKMEMBER(int,  pfnInit,                     (PPDMDEVINS pDevIns, PVGASTATE pThis, PVGASTATECC pThisCC));
    DECLCALLBACKMEMBER(int,  pfnPowerOn,                  (PPDMDEVINS pDevIns, PVGASTATE pThis, PVGASTATECC pThisCC));
    DECLCALLBACKMEMBER(int,  pfnTerminate,                (PVGASTATECC pThisCC));
    DECLCALLBACKMEMBER(int,  pfnReset,                    (PVGASTATECC pThisCC));
    DECLCALLBACKMEMBER(int,  pfnQueryCaps,                (PVGASTATECC pThisCC, SVGA3dDevCapIndex idx3dCaps, uint32_t *pu32Val));
    DECLCALLBACKMEMBER(int,  pfnChangeMode,               (PVGASTATECC pThisCC));
    DECLCALLBACKMEMBER(int,  pfnCreateTexture,            (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext, uint32_t idAssociatedContext, PVMSVGA3DSURFACE pSurface));
    DECLCALLBACKMEMBER(void, pfnSurfaceDestroy,           (PVGASTATECC pThisCC, PVMSVGA3DSURFACE pSurface));
    DECLCALLBACKMEMBER(void, pfnSurfaceInvalidateImage,   (PVGASTATECC pThisCC, PVMSVGA3DSURFACE pSurface, uint32_t uFace, uint32_t uMipmap));
    DECLCALLBACKMEMBER(int,  pfnSurfaceCopy,              (PVGASTATECC pThisCC, SVGA3dSurfaceImageId dest, SVGA3dSurfaceImageId src, uint32_t cCopyBoxes, SVGA3dCopyBox *pBox));
    DECLCALLBACKMEMBER(int,  pfnSurfaceDMACopyBox,        (PVGASTATE pThis, PVGASTATECC pThisCC, PVMSVGA3DSTATE pState, PVMSVGA3DSURFACE pSurface,
                                                           PVMSVGA3DMIPMAPLEVEL pMipLevel, uint32_t uHostFace, uint32_t uHostMipmap,
                                                           SVGAGuestPtr GuestPtr, uint32_t cbGuestPitch, SVGA3dTransferType transfer,
                                                           SVGA3dCopyBox const *pBox, PVMSVGA3DCONTEXT pContext, int rc, int iBox));
    DECLCALLBACKMEMBER(int,  pfnSurfaceStretchBlt,        (PVGASTATE pThis, PVMSVGA3DSTATE pState,
                                                           PVMSVGA3DSURFACE pDstSurface, uint32_t uDstFace, uint32_t uDstMipmap, SVGA3dBox const *pDstBox,
                                                           PVMSVGA3DSURFACE pSrcSurface, uint32_t uSrcFace, uint32_t uSrcMipmap, SVGA3dBox const *pSrcBox,
                                                           SVGA3dStretchBltMode enmMode, PVMSVGA3DCONTEXT pContext));
    DECLCALLBACKMEMBER(void, pfnUpdateHostScreenViewport, (PVGASTATECC pThisCC, uint32_t idScreen, VMSVGAVIEWPORT const *pOldViewport));
    /** @todo HW accelerated screen output probably needs a separate interface. */
    DECLCALLBACKMEMBER(int,  pfnDefineScreen,             (PVGASTATE pThis, PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen));
    DECLCALLBACKMEMBER(int,  pfnDestroyScreen,            (PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen));
    DECLCALLBACKMEMBER(int,  pfnSurfaceBlitToScreen,      (PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen,
                                                           SVGASignedRect destRect, SVGA3dSurfaceImageId srcImage,
                                                           SVGASignedRect srcRect, uint32_t cRects, SVGASignedRect *paRects));
    /* Various helpers. */
    DECLCALLBACKMEMBER(int,  pfnSurfaceUpdateHeapBuffers, (PVGASTATECC pThisCC, PVMSVGA3DSURFACE pSurface));
} VMSVGA3DBACKENDFUNCS3D;

/* VGPU9 3D */
#define VMSVGA3D_BACKEND_INTERFACE_NAME_VGPU9 "VGPU9"
typedef struct
{
    DECLCALLBACKMEMBER(int,  pfnContextDefine,            (PVGASTATECC pThisCC, uint32_t cid));
    DECLCALLBACKMEMBER(int,  pfnContextDestroy,           (PVGASTATECC pThisCC, uint32_t cid));
    DECLCALLBACKMEMBER(int,  pfnSetTransform,             (PVGASTATECC pThisCC, uint32_t cid, SVGA3dTransformType type, float matrix[16]));
    DECLCALLBACKMEMBER(int,  pfnSetZRange,                (PVGASTATECC pThisCC, uint32_t cid, SVGA3dZRange zRange));
    DECLCALLBACKMEMBER(int,  pfnSetRenderState,           (PVGASTATECC pThisCC, uint32_t cid, uint32_t cRenderStates, SVGA3dRenderState *pRenderState));
    DECLCALLBACKMEMBER(int,  pfnSetRenderTarget,          (PVGASTATECC pThisCC, uint32_t cid, SVGA3dRenderTargetType type, SVGA3dSurfaceImageId target));
    DECLCALLBACKMEMBER(int,  pfnSetTextureState,          (PVGASTATECC pThisCC, uint32_t cid, uint32_t cTextureStates, SVGA3dTextureState *pTextureState));
    DECLCALLBACKMEMBER(int,  pfnSetMaterial,              (PVGASTATECC pThisCC, uint32_t cid, SVGA3dFace face, SVGA3dMaterial *pMaterial));
    DECLCALLBACKMEMBER(int,  pfnSetLightData,             (PVGASTATECC pThisCC, uint32_t cid, uint32_t index, SVGA3dLightData *pData));
    DECLCALLBACKMEMBER(int,  pfnSetLightEnabled,          (PVGASTATECC pThisCC, uint32_t cid, uint32_t index, uint32_t enabled));
    DECLCALLBACKMEMBER(int,  pfnSetViewPort,              (PVGASTATECC pThisCC, uint32_t cid, SVGA3dRect *pRect));
    DECLCALLBACKMEMBER(int,  pfnSetClipPlane,             (PVGASTATECC pThisCC, uint32_t cid,  uint32_t index, float plane[4]));
    DECLCALLBACKMEMBER(int,  pfnCommandClear,             (PVGASTATECC pThisCC, uint32_t cid, SVGA3dClearFlag clearFlag, uint32_t color, float depth, uint32_t stencil, uint32_t cRects, SVGA3dRect *pRect));
    DECLCALLBACKMEMBER(int,  pfnDrawPrimitives,           (PVGASTATECC pThisCC, uint32_t cid, uint32_t numVertexDecls, SVGA3dVertexDecl *pVertexDecl, uint32_t numRanges, SVGA3dPrimitiveRange *pNumRange, uint32_t cVertexDivisor, SVGA3dVertexDivisor *pVertexDivisor));
    DECLCALLBACKMEMBER(int,  pfnSetScissorRect,           (PVGASTATECC pThisCC, uint32_t cid, SVGA3dRect *pRect));
    DECLCALLBACKMEMBER(int,  pfnGenerateMipmaps,          (PVGASTATECC pThisCC, uint32_t sid, SVGA3dTextureFilter filter));
    DECLCALLBACKMEMBER(int,  pfnShaderDefine,             (PVGASTATECC pThisCC, uint32_t cid, uint32_t shid, SVGA3dShaderType type, uint32_t cbData, uint32_t *pShaderData));
    DECLCALLBACKMEMBER(int,  pfnShaderDestroy,            (PVGASTATECC pThisCC, uint32_t cid, uint32_t shid, SVGA3dShaderType type));
    DECLCALLBACKMEMBER(int,  pfnShaderSet,                (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext, uint32_t cid, SVGA3dShaderType type, uint32_t shid));
    DECLCALLBACKMEMBER(int,  pfnShaderSetConst,           (PVGASTATECC pThisCC, uint32_t cid, uint32_t reg, SVGA3dShaderType type, SVGA3dShaderConstType ctype, uint32_t cRegisters, uint32_t *pValues));
    DECLCALLBACKMEMBER(int,  pfnOcclusionQueryCreate,     (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext));
    DECLCALLBACKMEMBER(int,  pfnOcclusionQueryDelete,     (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext));
    DECLCALLBACKMEMBER(int,  pfnOcclusionQueryBegin,      (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext));
    DECLCALLBACKMEMBER(int,  pfnOcclusionQueryEnd,        (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext));
    DECLCALLBACKMEMBER(int,  pfnOcclusionQueryGetData,    (PVGASTATECC pThisCC, PVMSVGA3DCONTEXT pContext, uint32_t *pu32Pixels));
} VMSVGA3DBACKENDFUNCSVGPU9;

/* Support for Guest-Backed Objects. */
#define VMSVGA3D_BACKEND_INTERFACE_NAME_GBO "GBO"
typedef struct
{
    DECLCALLBACKMEMBER(int,  pfnScreenTargetBind,   (PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen, uint32_t sid));
    DECLCALLBACKMEMBER(int,  pfnScreenTargetUpdate, (PVGASTATECC pThisCC, VMSVGASCREENOBJECT *pScreen, SVGA3dRect const *pRect));
} VMSVGA3DBACKENDFUNCSGBO;

#define VMSVGA3D_BACKEND_INTERFACE_NAME_MAP "MAP"
typedef struct
{
    DECLCALLBACKMEMBER(int, pfnSurfaceMap,   (PVGASTATECC pThisCC, SVGA3dSurfaceImageId const *pImage, SVGA3dBox const *pBox, VMSVGA3D_SURFACE_MAP enmMapType, VMSVGA3D_MAPPED_SURFACE *pMap));
    DECLCALLBACKMEMBER(int, pfnSurfaceUnmap, (PVGASTATECC pThisCC, SVGA3dSurfaceImageId const *pImage, VMSVGA3D_MAPPED_SURFACE *pMap, bool fWritten));
} VMSVGA3DBACKENDFUNCSMAP;

typedef struct VMSVGA3DSHADER *PVMSVGA3DSHADER;
typedef struct VMSVGA3DDXCONTEXT *PVMSVGA3DDXCONTEXT;
struct DXShaderInfo;
#define VMSVGA3D_BACKEND_INTERFACE_NAME_DX "DX"
typedef struct
{
    DECLCALLBACKMEMBER(int, pfnDXSaveState,                 (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, PCPDMDEVHLPR3 pHlp, PSSMHANDLE pSSM));
    DECLCALLBACKMEMBER(int, pfnDXLoadState,                 (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, PCPDMDEVHLPR3 pHlp, PSSMHANDLE pSSM));
    DECLCALLBACKMEMBER(int, pfnDXDefineContext,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDestroyContext,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXBindContext,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSwitchContext,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXReadbackContext,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXInvalidateContext,         (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetSingleConstantBuffer,   (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t slot, SVGA3dShaderType type, SVGA3dSurfaceId sid, uint32_t offsetInBytes, uint32_t sizeInBytes));
    DECLCALLBACKMEMBER(int, pfnDXSetShaderResources,        (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t startView, SVGA3dShaderType type, uint32_t cShaderResourceViewId, SVGA3dShaderResourceViewId const *paShaderResourceViewId));
    DECLCALLBACKMEMBER(int, pfnDXSetShader,                 (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderId shaderId, SVGA3dShaderType type));
    DECLCALLBACKMEMBER(int, pfnDXSetSamplers,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t startSampler, SVGA3dShaderType type, uint32_t cSamplerId, SVGA3dSamplerId const *paSamplerId));
    DECLCALLBACKMEMBER(int, pfnDXDraw,                      (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t vertexCount, uint32_t startVertexLocation));
    DECLCALLBACKMEMBER(int, pfnDXDrawIndexed,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation));
    DECLCALLBACKMEMBER(int, pfnDXDrawInstanced,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t vertexCountPerInstance, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation));
    DECLCALLBACKMEMBER(int, pfnDXDrawIndexedInstanced,      (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t indexCountPerInstance, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation));
    DECLCALLBACKMEMBER(int, pfnDXDrawAuto,                  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetInputLayout,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dElementLayoutId elementLayoutId));
    DECLCALLBACKMEMBER(int, pfnDXSetVertexBuffers,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t startBuffer, uint32_t cVertexBuffer, SVGA3dVertexBuffer const *paVertexBuffer));
    DECLCALLBACKMEMBER(int, pfnDXSetIndexBuffer,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dSurfaceId sid, SVGA3dSurfaceFormat format, uint32_t offset));
    DECLCALLBACKMEMBER(int, pfnDXSetTopology,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dPrimitiveType topology));
    DECLCALLBACKMEMBER(int, pfnDXSetRenderTargets,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dDepthStencilViewId depthStencilViewId, uint32_t cRenderTargetViewId, SVGA3dRenderTargetViewId const *paRenderTargetViewId));
    DECLCALLBACKMEMBER(int, pfnDXSetBlendState,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dBlendStateId blendId, float const blendFactor[4], uint32_t sampleMask));
    DECLCALLBACKMEMBER(int, pfnDXSetDepthStencilState,      (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dDepthStencilStateId depthStencilId, uint32_t stencilRef));
    DECLCALLBACKMEMBER(int, pfnDXSetRasterizerState,        (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dRasterizerStateId rasterizerId));
    DECLCALLBACKMEMBER(int, pfnDXDefineQuery,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId, SVGACOTableDXQueryEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyQuery,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId));
    DECLCALLBACKMEMBER(int, pfnDXBindQuery,                 (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId));
    DECLCALLBACKMEMBER(int, pfnDXBeginQuery,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId));
    DECLCALLBACKMEMBER(int, pfnDXEndQuery,                  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId));
    DECLCALLBACKMEMBER(int, pfnDXReadbackQuery,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dQueryId queryId, void *pvData, uint32_t cbData, uint32_t *pcbOut));
    DECLCALLBACKMEMBER(int, pfnDXSetPredication,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetSOTargets,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t cSoTarget, SVGA3dSoTarget const *paSoTarget));
    DECLCALLBACKMEMBER(int, pfnDXSetViewports,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t cViewport, SVGA3dViewport const *paViewport));
    DECLCALLBACKMEMBER(int, pfnDXSetScissorRects,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t cRect, SVGASignedRect const *paRect));
    DECLCALLBACKMEMBER(int, pfnDXClearRenderTargetView,     (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dRenderTargetViewId renderTargetViewId, SVGA3dRGBAFloat const *pRGBA));
    DECLCALLBACKMEMBER(int, pfnDXClearDepthStencilView,     (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, uint32_t flags, SVGA3dDepthStencilViewId depthStencilViewId, float depth, uint8_t stencil));
    DECLCALLBACKMEMBER(int, pfnDXPredCopyRegion,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dSurfaceId dstSid, uint32_t dstSubResource, SVGA3dSurfaceId srcSid, uint32_t srcSubResource, SVGA3dCopyBox const *pBox));
    DECLCALLBACKMEMBER(int, pfnDXPredCopy,                  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXPresentBlt,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXGenMips,                   (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderResourceViewId shaderResourceViewId));
    DECLCALLBACKMEMBER(int, pfnDXDefineShaderResourceView,  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderResourceViewId shaderResourceViewId, SVGACOTableDXSRViewEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyShaderResourceView, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderResourceViewId shaderResourceViewId));
    DECLCALLBACKMEMBER(int, pfnDXDefineRenderTargetView,    (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dRenderTargetViewId renderTargetViewId, SVGACOTableDXRTViewEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyRenderTargetView,   (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dRenderTargetViewId renderTargetViewId));
    DECLCALLBACKMEMBER(int, pfnDXDefineDepthStencilView,    (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dDepthStencilViewId depthStencilViewId, SVGACOTableDXDSViewEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyDepthStencilView,   (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dDepthStencilViewId depthStencilViewId));
    DECLCALLBACKMEMBER(int, pfnDXDefineElementLayout,       (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dElementLayoutId elementLayoutId, SVGACOTableDXElementLayoutEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyElementLayout,      (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dElementLayoutId elementLayoutId));
    DECLCALLBACKMEMBER(int, pfnDXDefineBlendState,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dBlendStateId blendId, SVGACOTableDXBlendStateEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyBlendState,         (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dBlendStateId blendId));
    DECLCALLBACKMEMBER(int, pfnDXDefineDepthStencilState,   (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dDepthStencilStateId depthStencilId, SVGACOTableDXDepthStencilEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyDepthStencilState,  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dDepthStencilStateId depthStencilId));
    DECLCALLBACKMEMBER(int, pfnDXDefineRasterizerState,     (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dRasterizerStateId rasterizerId, SVGACOTableDXRasterizerStateEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyRasterizerState,    (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dRasterizerStateId rasterizerId));
    DECLCALLBACKMEMBER(int, pfnDXDefineSamplerState,        (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dSamplerId samplerId, SVGACOTableDXSamplerEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroySamplerState,       (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dSamplerId samplerId));
    DECLCALLBACKMEMBER(int, pfnDXDefineShader,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderId shaderId, SVGACOTableDXShaderEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyShader,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderId shaderId));
    DECLCALLBACKMEMBER(int, pfnDXBindShader,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dShaderId shaderId, DXShaderInfo const *pShaderInfo));
    DECLCALLBACKMEMBER(int, pfnDXDefineStreamOutput,        (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dStreamOutputId soid, SVGACOTableDXStreamOutputEntry const *pEntry));
    DECLCALLBACKMEMBER(int, pfnDXDestroyStreamOutput,       (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dStreamOutputId soid));
    DECLCALLBACKMEMBER(int, pfnDXSetStreamOutput,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGA3dStreamOutputId soid));
    DECLCALLBACKMEMBER(int, pfnDXSetCOTable,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext, SVGACOTableType type, uint32_t cValidEntries));
    DECLCALLBACKMEMBER(int, pfnDXBufferCopy,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSurfaceCopyAndReadback,    (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXMoveQuery,                 (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXMobFence64,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXBindAllShader,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXHint,                      (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXBufferUpdate,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetVSConstantBufferOffset, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetPSConstantBufferOffset, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetGSConstantBufferOffset, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetHSConstantBufferOffset, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetDSConstantBufferOffset, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetCSConstantBufferOffset, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXCondBindAllShader,         (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnScreenCopy,                  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnGrowOTable,                  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXGrowCOTable,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnIntraSurfaceCopy,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDefineGBSurface_v3,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXResolveCopy,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXPredResolveCopy,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXPredConvertRegion,         (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXPredConvert,               (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnWholeSurfaceCopy,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDefineUAView,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDestroyUAView,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXClearUAViewUint,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXClearUAViewFloat,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXCopyStructureCount,        (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetUAViews,                (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDrawIndexedInstancedIndirect, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDrawInstancedIndirect,     (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDispatch,                  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDispatchIndirect,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnWriteZeroSurface,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnHintZeroSurface,             (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXTransferToBuffer,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetStructureCount,         (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnLogicOpsBitBlt,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnLogicOpsTransBlt,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnLogicOpsStretchBlt,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnLogicOpsColorFill,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnLogicOpsAlphaBlend,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnLogicOpsClearTypeBlend,      (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDefineGBSurface_v4,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetCSUAViews,              (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetMinLOD,                 (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXDefineStreamOutputWithMob, (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXSetShaderIface,            (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXBindStreamOutput,          (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnSurfaceStretchBltNonMSToMS,  (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
    DECLCALLBACKMEMBER(int, pfnDXBindShaderIface,           (PVGASTATECC pThisCC, PVMSVGA3DDXCONTEXT pDXContext));
} VMSVGA3DBACKENDFUNCSDX;

typedef struct VMSVGA3DBACKENDDESC
{
    char const *pszName;
    DECLCALLBACKMEMBER(int, pfnQueryInterface, (PVGASTATECC pThisCC, char const *pszInterfaceName, void *pvInterfaceFuncs, size_t cbInterfaceFuncs));
} VMSVGA3DBACKENDDESC;

#ifdef VMSVGA3D_DX
/* Helpers. */
int vmsvga3dDXUnbindContext(PVGASTATECC pThisCC, uint32_t cid, SVGADXContextMobFormat *pSvgaDXContext);
int vmsvga3dDXSwitchContext(PVGASTATECC pThisCC, uint32_t cid);

/* Command handlers. */
int vmsvga3dDXDefineContext(PVGASTATECC pThisCC, uint32_t cid);
int vmsvga3dDXDestroyContext(PVGASTATECC pThisCC, uint32_t cid);
int vmsvga3dDXBindContext(PVGASTATECC pThisCC, uint32_t cid, SVGADXContextMobFormat *pSvgaDXContext);
int vmsvga3dDXReadbackContext(PVGASTATECC pThisCC, uint32_t idDXContext, SVGADXContextMobFormat *pSvgaDXContext);
int vmsvga3dDXInvalidateContext(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetSingleConstantBuffer(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetSingleConstantBuffer const *pCmd);
int vmsvga3dDXSetShaderResources(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetShaderResources const *pCmd, uint32_t cShaderResourceViewId, SVGA3dShaderResourceViewId const *paShaderResourceViewId);
int vmsvga3dDXSetShader(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetShader const *pCmd);
int vmsvga3dDXSetSamplers(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetSamplers const *pCmd, uint32_t cSamplerId, SVGA3dSamplerId const *paSamplerId);
int vmsvga3dDXDraw(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDraw const *pCmd);
int vmsvga3dDXDrawIndexed(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawIndexed const *pCmd);
int vmsvga3dDXDrawInstanced(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawInstanced const *pCmd);
int vmsvga3dDXDrawIndexedInstanced(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDrawIndexedInstanced const *pCmd);
int vmsvga3dDXDrawAuto(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetInputLayout(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dElementLayoutId elementLayoutId);
int vmsvga3dDXSetVertexBuffers(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t startBuffer, uint32_t cVertexBuffer, SVGA3dVertexBuffer const *paVertexBuffer);
int vmsvga3dDXSetIndexBuffer(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetIndexBuffer const *pCmd);
int vmsvga3dDXSetTopology(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dPrimitiveType topology);
int vmsvga3dDXSetRenderTargets(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dDepthStencilViewId depthStencilViewId, uint32_t cRenderTargetViewId, SVGA3dRenderTargetViewId const *paRenderTargetViewId);
int vmsvga3dDXSetBlendState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetBlendState const *pCmd);
int vmsvga3dDXSetDepthStencilState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetDepthStencilState const *pCmd);
int vmsvga3dDXSetRasterizerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dRasterizerStateId rasterizerId);
int vmsvga3dDXDefineQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineQuery const *pCmd);
int vmsvga3dDXDestroyQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyQuery const *pCmd);
int vmsvga3dDXBindQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBindQuery const *pCmd, PVMSVGAMOB pMob);
int vmsvga3dDXSetQueryOffset(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetQueryOffset const *pCmd);
int vmsvga3dDXBeginQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBeginQuery const *pCmd);
int vmsvga3dDXEndQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXEndQuery const *pCmd);
int vmsvga3dDXReadbackQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXReadbackQuery const *pCmd);
int vmsvga3dDXSetPredication(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetSOTargets(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t cSoTarget, SVGA3dSoTarget const *paSoTarget);
int vmsvga3dDXSetViewports(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t cViewport, SVGA3dViewport const *paViewport);
int vmsvga3dDXSetScissorRects(PVGASTATECC pThisCC, uint32_t idDXContext, uint32_t cRect, SVGASignedRect const *paRect);
int vmsvga3dDXClearRenderTargetView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXClearRenderTargetView const *pCmd);
int vmsvga3dDXClearDepthStencilView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXClearDepthStencilView const *pCmd);
int vmsvga3dDXPredCopyRegion(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXPredCopyRegion const *pCmd);
int vmsvga3dDXPredCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXPresentBlt(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXGenMips(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXGenMips const *pCmd);
int vmsvga3dDXDefineShaderResourceView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineShaderResourceView const *pCmd);
int vmsvga3dDXDestroyShaderResourceView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyShaderResourceView const *pCmd);
int vmsvga3dDXDefineRenderTargetView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineRenderTargetView const *pCmd);
int vmsvga3dDXDestroyRenderTargetView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyRenderTargetView const *pCmd);
int vmsvga3dDXDefineDepthStencilView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineDepthStencilView_v2 const *pCmd);
int vmsvga3dDXDestroyDepthStencilView(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyDepthStencilView const *pCmd);
int vmsvga3dDXDefineElementLayout(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dElementLayoutId elementLayoutId, uint32_t cDesc, SVGA3dInputElementDesc const *paDesc);
int vmsvga3dDXDestroyElementLayout(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyElementLayout const *pCmd);
int vmsvga3dDXDefineBlendState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineBlendState const *pCmd);
int vmsvga3dDXDestroyBlendState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyBlendState const *pCmd);
int vmsvga3dDXDefineDepthStencilState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineDepthStencilState const *pCmd);
int vmsvga3dDXDestroyDepthStencilState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyDepthStencilState const *pCmd);
int vmsvga3dDXDefineRasterizerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineRasterizerState const *pCmd);
int vmsvga3dDXDestroyRasterizerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyRasterizerState const *pCmd);
int vmsvga3dDXDefineSamplerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineSamplerState const *pCmd);
int vmsvga3dDXDestroySamplerState(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroySamplerState const *pCmd);
int vmsvga3dDXDefineShader(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineShader const *pCmd);
int vmsvga3dDXDestroyShader(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyShader const *pCmd);
int vmsvga3dDXBindShader(PVGASTATECC pThisCC, SVGA3dCmdDXBindShader const *pCmd, PVMSVGAMOB pMob);
int vmsvga3dDXDefineStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDefineStreamOutput const *pCmd);
int vmsvga3dDXDestroyStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXDestroyStreamOutput const *pCmd);
int vmsvga3dDXSetStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXSetStreamOutput const *pCmd);
int vmsvga3dDXSetCOTable(PVGASTATECC pThisCC, SVGA3dCmdDXSetCOTable const *pCmd, PVMSVGAMOB pMob);
int vmsvga3dDXReadbackCOTable(PVGASTATECC pThisCC, SVGA3dCmdDXReadbackCOTable const *pCmd);
int vmsvga3dDXBufferCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSurfaceCopyAndReadback(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXMoveQuery(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXBindAllQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXBindAllQuery const *pCmd);
int vmsvga3dDXReadbackAllQuery(PVGASTATECC pThisCC, uint32_t idDXContext, SVGA3dCmdDXReadbackAllQuery const *pCmd);
int vmsvga3dDXMobFence64(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXBindAllShader(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXHint(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXBufferUpdate(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetVSConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetPSConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetGSConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetHSConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetDSConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetCSConstantBufferOffset(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXCondBindAllShader(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dScreenCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dGrowOTable(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXGrowCOTable(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dIntraSurfaceCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDefineGBSurface_v3(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXResolveCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXPredResolveCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXPredConvertRegion(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXPredConvert(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dWholeSurfaceCopy(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDefineUAView(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDestroyUAView(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXClearUAViewUint(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXClearUAViewFloat(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXCopyStructureCount(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetUAViews(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDrawIndexedInstancedIndirect(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDrawInstancedIndirect(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDispatch(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDispatchIndirect(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dWriteZeroSurface(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dHintZeroSurface(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXTransferToBuffer(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetStructureCount(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dLogicOpsBitBlt(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dLogicOpsTransBlt(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dLogicOpsStretchBlt(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dLogicOpsColorFill(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dLogicOpsAlphaBlend(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dLogicOpsClearTypeBlend(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDefineGBSurface_v4(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetCSUAViews(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetMinLOD(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXDefineStreamOutputWithMob(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXSetShaderIface(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXBindStreamOutput(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dSurfaceStretchBltNonMSToMS(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXBindShaderIface(PVGASTATECC pThisCC, uint32_t idDXContext);
int vmsvga3dDXLoadExec(PPDMDEVINS pDevIns, PVGASTATE pThis, PVGASTATECC pThisCC, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass);
int vmsvga3dDXSaveExec(PPDMDEVINS pDevIns, PVGASTATECC pThisCC, PSSMHANDLE pSSM);
#endif /* VMSVGA3D_DX */


#endif /* !VBOX_INCLUDED_SRC_Graphics_DevVGA_SVGA3d_h */


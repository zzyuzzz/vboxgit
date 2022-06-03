/** @file
 *
 * VBox frontends: VBoxSDL (simple frontend based on SDL):
 * Declaration of VBoxSDLFB (SDL framebuffer) class
 */

/*
 * Copyright (C) 2006-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __H_FRAMEBUFFER
#define __H_FRAMEBUFFER

#include "VBoxSDL.h"
#include <iprt/thread.h>

#include <iprt/critsect.h>

#ifdef VBOX_SECURELABEL
#include <SDL_ttf.h>
/* function pointers */
extern "C"
{
extern DECLSPEC int (SDLCALL *pTTF_Init)(void);
extern DECLSPEC TTF_Font* (SDLCALL *pTTF_OpenFont)(const char *file, int ptsize);
extern DECLSPEC SDL_Surface* (SDLCALL *pTTF_RenderUTF8_Solid)(TTF_Font *font, const char *text, SDL_Color fg);
extern DECLSPEC SDL_Surface* (SDLCALL *pTTF_RenderUTF8_Shaded)(TTF_Font *font, const char *text, SDL_Color fg, SDL_Color bg);
extern DECLSPEC SDL_Surface* (SDLCALL *pTTF_RenderUTF8_Blended)(TTF_Font *font, const char *text, SDL_Color fg);
extern DECLSPEC void (SDLCALL *pTTF_CloseFont)(TTF_Font *font);
extern DECLSPEC void (SDLCALL *pTTF_Quit)(void);
}
#endif /* VBOX_SECURELABEL && !VBOX_WITH_SDL13 */

class VBoxSDLFBOverlay;

class VBoxSDLFB :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    VBOX_SCRIPTABLE_IMPL(IFramebuffer)
{
public:
    DECLARE_NOT_AGGREGATABLE(VBoxSDLFB)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    VBoxSDLFB() { /* empty */ }
    ~VBoxSDLFB() { /* empty */ }

    HRESULT FinalConstruct();
    void FinalRelease();

    HRESULT init(uint32_t uScreenId,
                 bool fFullscreen = false, bool fResizable = true, bool fShowSDLConfig = false,
                 bool fKeepHostRes = false, uint32_t u32FixedWidth = ~(uint32_t)0,
                 uint32_t u32FixedHeight = ~(uint32_t)0, uint32_t u32FixedBPP = ~(uint32_t)0);
    void uninit();

    static bool initSDL(bool fShowSDLConfig);
    static void uninitSDL();

    BEGIN_COM_MAP(VBoxSDLFB)
        VBOX_MINIMAL_INTERFACE_ENTRIES(IFramebuffer)
    END_COM_MAP()

    STDMETHOD(COMGETTER(Width))(ULONG *width);
    STDMETHOD(COMGETTER(Height))(ULONG *height);
    STDMETHOD(Lock)();
    STDMETHOD(Unlock)();
    STDMETHOD(COMGETTER(Address))(BYTE **address);
    STDMETHOD(COMGETTER(BitsPerPixel))(ULONG *bitsPerPixel);
    STDMETHOD(COMGETTER(BytesPerLine))(ULONG *bytesPerLine);
    STDMETHOD(COMGETTER(PixelFormat)) (ULONG *pixelFormat);
    STDMETHOD(COMGETTER(UsesGuestVRAM)) (BOOL *usesGuestVRAM);
    STDMETHOD(COMGETTER(HeightReduction)) (ULONG *heightReduction);
    STDMETHOD(COMGETTER(Overlay)) (IFramebufferOverlay **aOverlay);
    STDMETHOD(COMGETTER(WinId)) (int64_t *winId);

    STDMETHOD(NotifyUpdate)(ULONG x, ULONG y, ULONG w, ULONG h);
    STDMETHOD(RequestResize)(ULONG aScreenId, ULONG pixelFormat, BYTE *vram,
                             ULONG bitsPerPixel, ULONG bytesPerLine,
                             ULONG w, ULONG h, BOOL *finished);
    STDMETHOD(VideoModeSupported)(ULONG width, ULONG height, ULONG bpp, BOOL *supported);

    STDMETHOD(GetVisibleRegion)(BYTE *aRectangles, ULONG aCount, ULONG *aCountCopied);
    STDMETHOD(SetVisibleRegion)(BYTE *aRectangles, ULONG aCount);

    STDMETHOD(ProcessVHWACommand)(BYTE *pCommand);

    // internal public methods
    bool initialized() { return mfInitialized; }
    void resizeGuest();
    void resizeSDL();
    void update(int x, int y, int w, int h, bool fGuestRelative);
    void repaint();
    void setFullscreen(bool fFullscreen);
    void getFullscreenGeometry(uint32_t *width, uint32_t *height);
    uint32_t getScreenId() { return mScreenId; }
    uint32_t getGuestXRes() { return mGuestXRes; }
    uint32_t getGuestYRes() { return mGuestYRes; }
    int32_t getOriginX() { return mOriginX; }
    int32_t getOriginY() { return mOriginY; }
    int32_t getXOffset() { return mCenterXOffset; }
    int32_t getYOffset() { return mCenterYOffset; }
#ifdef VBOX_WITH_SDL13
    bool hasWindow(SDL_WindowID id) { return mScreen && mWindow == id; }
#endif
#ifdef VBOX_SECURELABEL
    int  initSecureLabel(uint32_t height, char *font, uint32_t pointsize, uint32_t labeloffs);
    void setSecureLabelText(const char *text);
    void setSecureLabelColor(uint32_t colorFG, uint32_t colorBG);
    void paintSecureLabel(int x, int y, int w, int h, bool fForce);
#endif
    void setWinId(int64_t winId) { mWinId = winId; }
    void setOrigin(int32_t axOrigin, int32_t ayOrigin) { mOriginX = axOrigin; mOriginY = ayOrigin; }
    bool getFullscreen() { return mfFullscreen; }

private:
    /** current SDL framebuffer pointer (also includes screen width/height) */
    SDL_Surface *mScreen;
#ifdef VBOX_WITH_SDL13
    /** the SDL window */
    SDL_WindowID mWindow;
    /** the texture */
    SDL_TextureID mTexture;
    /** render info */
    SDL_RendererInfo mRenderInfo;
#endif
    /** false if constructor failed */
    bool mfInitialized;
    /** the screen number of this framebuffer */
    uint32_t mScreenId;
    /** maximum possible screen width in pixels (~0 = no restriction) */
    uint32_t mMaxScreenWidth;
    /** maximum possible screen height in pixels (~0 = no restriction) */
    uint32_t mMaxScreenHeight;
    /** current guest screen width in pixels */
    ULONG mGuestXRes;
    /** current guest screen height in pixels */
    ULONG mGuestYRes;
    int32_t mOriginX;
    int32_t mOriginY;
    /** fixed SDL screen width (~0 = not set) */
    uint32_t mFixedSDLWidth;
    /** fixed SDL screen height (~0 = not set) */
    uint32_t mFixedSDLHeight;
    /** fixed SDL bits per pixel (~0 = not set) */
    uint32_t mFixedSDLBPP;
    /** Y offset in pixels, i.e. guest-nondrawable area at the top */
    uint32_t mTopOffset;
    /** X offset for guest screen centering */
    uint32_t mCenterXOffset;
    /** Y offset for guest screen centering */
    uint32_t mCenterYOffset;
    /** flag whether we're in fullscreen mode */
    bool  mfFullscreen;
    /** flag whether we keep the host screen resolution when switching to
     *  fullscreen or not */
    bool  mfKeepHostRes;
    /** framebuffer update semaphore */
    RTCRITSECT mUpdateLock;
    /** flag whether the SDL window should be resizable */
    bool mfResizable;
    /** flag whether we print out SDL information */
    bool mfShowSDLConfig;
    /** handle to window where framebuffer context is being drawn*/
    int64_t mWinId;
#ifdef VBOX_SECURELABEL
    /** current secure label text */
    Utf8Str mSecureLabelText;
    /** current secure label foreground color (RGB) */
    uint32_t mSecureLabelColorFG;
    /** current secure label background color (RGB) */
    uint32_t mSecureLabelColorBG;
    /** secure label font handle */
    TTF_Font *mLabelFont;
    /** secure label height in pixels */
    uint32_t mLabelHeight;
    /** secure label offset from the top of the secure label */
    uint32_t mLabelOffs;
#endif

    SDL_Surface *mSurfVRAM;

    BYTE *mPtrVRAM;
    ULONG mBitsPerPixel;
    ULONG mBytesPerLine;
    ULONG mPixelFormat;
    BOOL mUsesGuestVRAM;
    BOOL mfSameSizeRequested;
};

class VBoxSDLFBOverlay :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    VBOX_SCRIPTABLE_IMPL(IFramebufferOverlay)
{
public:
    DECLARE_NOT_AGGREGATABLE(VBoxSDLFBOverlay)

    VBoxSDLFBOverlay(ULONG x, ULONG y, ULONG width, ULONG height, BOOL visible,
                     VBoxSDLFB *aParent);
    virtual ~VBoxSDLFBOverlay();

    BEGIN_COM_MAP(VBoxSDLFBOverlay)
        VBOX_MINIMAL_INTERFACE_ENTRIES(IFramebufferOverlay)
    END_COM_MAP()

    STDMETHOD(COMGETTER(X))(ULONG *x);
    STDMETHOD(COMGETTER(Y))(ULONG *y);
    STDMETHOD(COMGETTER(Width))(ULONG *width);
    STDMETHOD(COMGETTER(Height))(ULONG *height);
    STDMETHOD(COMGETTER(Visible))(BOOL *visible);
    STDMETHOD(COMSETTER(Visible))(BOOL visible);
    STDMETHOD(COMGETTER(Alpha))(ULONG *alpha);
    STDMETHOD(COMSETTER(Alpha))(ULONG alpha);
    STDMETHOD(COMGETTER(Address))(ULONG *address);
    STDMETHOD(COMGETTER(BytesPerLine))(ULONG *bytesPerLine);

    /* These are not used, or return standard values. */
    STDMETHOD(COMGETTER(BitsPerPixel))(ULONG *bitsPerPixel);
    STDMETHOD(COMGETTER(PixelFormat)) (ULONG *pixelFormat);
    STDMETHOD(COMGETTER(UsesGuestVRAM)) (BOOL *usesGuestVRAM);
    STDMETHOD(COMGETTER(HeightReduction)) (ULONG *heightReduction);
    STDMETHOD(COMGETTER(Overlay)) (IFramebufferOverlay **aOverlay);
    STDMETHOD(COMGETTER(WinId)) (LONG64 *winId);

    STDMETHOD(Lock)();
    STDMETHOD(Unlock)();
    STDMETHOD(Move)(ULONG x, ULONG y);
    STDMETHOD(NotifyUpdate)(ULONG x, ULONG y, ULONG w, ULONG h);
    STDMETHOD(RequestResize)(ULONG aScreenId, ULONG pixelFormat, ULONG vram,
                             ULONG bitsPerPixel, ULONG bytesPerLine,
                             ULONG w, ULONG h, BOOL *finished);
    STDMETHOD(VideoModeSupported)(ULONG width, ULONG height, ULONG bpp, BOOL *supported);

    // internal public methods
    HRESULT init();

private:
    /** Overlay X offset */
    ULONG mOverlayX;
    /** Overlay Y offset */
    ULONG mOverlayY;
    /** Overlay width */
    ULONG mOverlayWidth;
    /** Overlay height */
    ULONG mOverlayHeight;
    /** Whether the overlay is currently active */
    BOOL mOverlayVisible;
    /** The parent IFramebuffer */
    VBoxSDLFB *mParent;
    /** SDL surface containing the actual framebuffer bits */
    SDL_Surface *mOverlayBits;
    /** Additional SDL surface used for combining the framebuffer and the overlay */
    SDL_Surface *mBlendedBits;
};

#endif // __H_FRAMEBUFFER

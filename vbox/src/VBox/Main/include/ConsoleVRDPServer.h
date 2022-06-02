/* $Id: ConsoleVRDPServer.h 35722 2011-01-26 16:37:16Z vboxsync $ */
/** @file
 * VBox Console VRDE Server Helper class and implementation of IVRDEServerInfo
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

#ifndef ____H_CONSOLEVRDPSERVER
#define ____H_CONSOLEVRDPSERVER

#include "RemoteUSBBackend.h"
#include "HGCM.h"

#include <VBox/VBoxAuth.h>

#include <VBox/HostServices/VBoxClipboardExt.h>

#include "SchemaDefs.h"

// ConsoleVRDPServer
///////////////////////////////////////////////////////////////////////////////

typedef struct _VRDPInputSynch
{
    int cGuestNumLockAdaptions;
    int cGuestCapsLockAdaptions;

    bool fGuestNumLock;
    bool fGuestCapsLock;
    bool fGuestScrollLock;

    bool fClientNumLock;
    bool fClientCapsLock;
    bool fClientScrollLock;
} VRDPInputSynch;

/* Member of Console. Helper class for VRDP server management. Not a COM class. */
class ConsoleVRDPServer
{
public:
    ConsoleVRDPServer (Console *console);
    ~ConsoleVRDPServer ();

    int Launch (void);

    void NotifyAbsoluteMouse (bool fGuestWantsAbsolute)
    {
        m_fGuestWantsAbsolute = fGuestWantsAbsolute;
    }

    void NotifyKeyboardLedsChange (BOOL fNumLock, BOOL fCapsLock, BOOL fScrollLock)
    {
        bool fGuestNumLock    = (fNumLock != FALSE);
        bool fGuestCapsLock   = (fCapsLock != FALSE);
        bool fGuestScrollLock = (fScrollLock != FALSE);

        /* Might need to resync in case the guest itself changed the LED status. */
        if (m_InputSynch.fClientNumLock != fGuestNumLock)
        {
            m_InputSynch.cGuestNumLockAdaptions = 2;
        }

        if (m_InputSynch.fClientCapsLock != fGuestCapsLock)
        {
            m_InputSynch.cGuestCapsLockAdaptions = 2;
        }

        m_InputSynch.fGuestNumLock    = fGuestNumLock;
        m_InputSynch.fGuestCapsLock   = fGuestCapsLock;
        m_InputSynch.fGuestScrollLock = fGuestScrollLock;
    }

    void EnableConnections (void);
    void DisconnectClient (uint32_t u32ClientId, bool fReconnect);
    void MousePointerUpdate (const VRDECOLORPOINTER *pPointer);
    void MousePointerHide (void);

    void Stop (void);

    AuthResult Authenticate (const Guid &uuid, AuthGuestJudgement guestJudgement,
                             const char *pszUser, const char *pszPassword, const char *pszDomain,
                             uint32_t u32ClientId);

    void AuthDisconnect (const Guid &uuid, uint32_t u32ClientId);

    void USBBackendCreate (uint32_t u32ClientId, void **ppvIntercept);
    void USBBackendDelete (uint32_t u32ClientId);

    void *USBBackendRequestPointer (uint32_t u32ClientId, const Guid *pGuid);
    void USBBackendReleasePointer (const Guid *pGuid);

    /* Private interface for the RemoteUSBBackend destructor. */
    void usbBackendRemoveFromList (RemoteUSBBackend *pRemoteUSBBackend);

    /* Private methods for the Remote USB thread. */
    RemoteUSBBackend *usbBackendGetNext (RemoteUSBBackend *pRemoteUSBBackend);

    void notifyRemoteUSBThreadRunning (RTTHREAD thread);
    bool isRemoteUSBThreadRunning (void);
    void waitRemoteUSBThreadEvent (RTMSINTERVAL cMillies);

    void ClipboardCreate (uint32_t u32ClientId);
    void ClipboardDelete (uint32_t u32ClientId);

    /*
     * Forwarders to VRDP server library.
     */
    void SendUpdate (unsigned uScreenId, void *pvUpdate, uint32_t cbUpdate) const;
    void SendResize (void) const;
    void SendUpdateBitmap (unsigned uScreenId, uint32_t x, uint32_t y, uint32_t w, uint32_t h) const;

    void SendAudioSamples (void *pvSamples, uint32_t cSamples, VRDEAUDIOFORMAT format) const;
    void SendAudioVolume (uint16_t left, uint16_t right) const;
    void SendUSBRequest (uint32_t u32ClientId, void *pvParms, uint32_t cbParms) const;

    void QueryInfo (uint32_t index, void *pvBuffer, uint32_t cbBuffer, uint32_t *pcbOut) const;

    int SendAudioInputBegin(void **ppvUserCtx,
                            void *pvContext,
                            uint32_t cSamples,
                            uint32_t iSampleHz,
                            uint32_t cChannels,
                            uint32_t cBits);

    void SendAudioInputEnd(void *pvUserCtx);

private:
    /* Note: This is not a ComObjPtr here, because the ConsoleVRDPServer object
     * is actually just a part of the Console.
     */
    Console *mConsole;

    HVRDESERVER mhServer;
    int mServerInterfaceVersion;

    static int loadVRDPLibrary (const char *pszLibraryName);

    /** Static because will never load this more than once! */
    static RTLDRMOD mVRDPLibrary;

    static PFNVRDECREATESERVER mpfnVRDECreateServer;

    static VRDEENTRYPOINTS_3 mEntryPoints;
    static VRDEENTRYPOINTS_3 *mpEntryPoints;
    static VRDECALLBACKS_3 mCallbacks;

    static DECLCALLBACK(int)  VRDPCallbackQueryProperty     (void *pvCallback, uint32_t index, void *pvBuffer, uint32_t cbBuffer, uint32_t *pcbOut);
    static DECLCALLBACK(int)  VRDPCallbackClientLogon       (void *pvCallback, uint32_t u32ClientId, const char *pszUser, const char *pszPassword, const char *pszDomain);
    static DECLCALLBACK(void) VRDPCallbackClientConnect     (void *pvCallback, uint32_t u32ClientId);
    static DECLCALLBACK(void) VRDPCallbackClientDisconnect  (void *pvCallback, uint32_t u32ClientId, uint32_t fu32Intercepted);
    static DECLCALLBACK(int)  VRDPCallbackIntercept         (void *pvCallback, uint32_t u32ClientId, uint32_t fu32Intercept, void **ppvIntercept);
    static DECLCALLBACK(int)  VRDPCallbackUSB               (void *pvCallback, void *pvIntercept, uint32_t u32ClientId, uint8_t u8Code, const void *pvRet, uint32_t cbRet);
    static DECLCALLBACK(int)  VRDPCallbackClipboard         (void *pvCallback, void *pvIntercept, uint32_t u32ClientId, uint32_t u32Function, uint32_t u32Format, const void *pvData, uint32_t cbData);
    static DECLCALLBACK(bool) VRDPCallbackFramebufferQuery  (void *pvCallback, unsigned uScreenId, VRDEFRAMEBUFFERINFO *pInfo);
    static DECLCALLBACK(void) VRDPCallbackFramebufferLock   (void *pvCallback, unsigned uScreenId);
    static DECLCALLBACK(void) VRDPCallbackFramebufferUnlock (void *pvCallback, unsigned uScreenId);
    static DECLCALLBACK(void) VRDPCallbackInput             (void *pvCallback, int type, const void *pvInput, unsigned cbInput);
    static DECLCALLBACK(void) VRDPCallbackVideoModeHint     (void *pvCallback, unsigned cWidth, unsigned cHeight,  unsigned cBitsPerPixel, unsigned uScreenId);
    static DECLCALLBACK(void) VRDECallbackAudioIn           (void *pvCallback, void *pvCtx, uint32_t u32ClientId, uint32_t u32Event, const void *pvData, uint32_t cbData);

    bool m_fGuestWantsAbsolute;
    int m_mousex;
    int m_mousey;

    IFramebuffer *maFramebuffers[SchemaDefs::MaxGuestMonitors];

    ComPtr<IEventListener> mConsoleListener;

    VRDPInputSynch m_InputSynch;

    int32_t mVRDPBindPort;

    RTCRITSECT mCritSect;

    int lockConsoleVRDPServer (void);
    void unlockConsoleVRDPServer (void);

    int mcClipboardRefs;
    HGCMSVCEXTHANDLE mhClipboard;
    PFNVRDPCLIPBOARDEXTCALLBACK mpfnClipboardCallback;

    static DECLCALLBACK(int) ClipboardCallback (void *pvCallback, uint32_t u32ClientId, uint32_t u32Function, uint32_t u32Format, const void *pvData, uint32_t cbData);
    static DECLCALLBACK(int) ClipboardServiceExtension (void *pvExtension, uint32_t u32Function, void *pvParm, uint32_t cbParms);

#ifdef VBOX_WITH_USB
    RemoteUSBBackend *usbBackendFindByUUID (const Guid *pGuid);
    RemoteUSBBackend *usbBackendFind (uint32_t u32ClientId);

    typedef struct _USBBackends
    {
        RemoteUSBBackend *pHead;
        RemoteUSBBackend *pTail;

        RTTHREAD thread;

        bool fThreadRunning;

        RTSEMEVENT event;
    } USBBackends;

    USBBackends mUSBBackends;

    void remoteUSBThreadStart (void);
    void remoteUSBThreadStop (void);
#endif /* VBOX_WITH_USB */

    /* External authentication library handle. The library is loaded in the
     * Authenticate method and unloaded at the object destructor.
     */
    RTLDRMOD mAuthLibrary;
    PAUTHENTRY mpfnAuthEntry;
    PAUTHENTRY2 mpfnAuthEntry2;
    PAUTHENTRY3 mpfnAuthEntry3;

    uint32_t volatile mu32AudioInputClientId;
};


class Console;

class ATL_NO_VTABLE VRDEServerInfo :
    public VirtualBoxBase,
    VBOX_SCRIPTABLE_IMPL(IVRDEServerInfo)
{
public:

    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(VRDEServerInfo, IVRDEServerInfo)

    DECLARE_NOT_AGGREGATABLE(VRDEServerInfo)

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(VRDEServerInfo)
        VBOX_DEFAULT_INTERFACE_ENTRIES(IVRDEServerInfo)
    END_COM_MAP()

    DECLARE_EMPTY_CTOR_DTOR (VRDEServerInfo)

    HRESULT FinalConstruct();
    void FinalRelease();

    /* Public initializer/uninitializer for internal purposes only. */
    HRESULT init (Console *aParent);
    void uninit();

    /* IVRDEServerInfo properties */
    #define DECL_GETTER(_aType, _aName) STDMETHOD(COMGETTER(_aName)) (_aType *a##_aName)
        DECL_GETTER (BOOL,    Active);
        DECL_GETTER (LONG,    Port);
        DECL_GETTER (ULONG,   NumberOfClients);
        DECL_GETTER (LONG64,  BeginTime);
        DECL_GETTER (LONG64,  EndTime);
        DECL_GETTER (LONG64,  BytesSent);
        DECL_GETTER (LONG64,  BytesSentTotal);
        DECL_GETTER (LONG64,  BytesReceived);
        DECL_GETTER (LONG64,  BytesReceivedTotal);
        DECL_GETTER (BSTR,    User);
        DECL_GETTER (BSTR,    Domain);
        DECL_GETTER (BSTR,    ClientName);
        DECL_GETTER (BSTR,    ClientIP);
        DECL_GETTER (ULONG,   ClientVersion);
        DECL_GETTER (ULONG,   EncryptionStyle);
    #undef DECL_GETTER

private:

    Console * const         mParent;
};

#endif // ____H_CONSOLEVRDPSERVER
/* vi: set tabstop=4 shiftwidth=4 expandtab: */

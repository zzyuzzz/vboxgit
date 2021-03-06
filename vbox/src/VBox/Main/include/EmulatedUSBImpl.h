/* $Id: EmulatedUSBImpl.h 94368 2022-03-25 07:26:37Z vboxsync $ */
/** @file
 * Emulated USB devices manager.
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

#ifndef MAIN_INCLUDED_EmulatedUSBImpl_h
#define MAIN_INCLUDED_EmulatedUSBImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "EmulatedUSBWrap.h"

#include <VBox/vrdpusb.h>

class Console;
class EUSBWEBCAM;

typedef std::map<Utf8Str, EUSBWEBCAM *> WebcamsMap;

class ATL_NO_VTABLE EmulatedUSB :
    public EmulatedUSBWrap
{
public:

    DECLARE_COMMON_CLASS_METHODS(EmulatedUSB)

    HRESULT FinalConstruct();
    void FinalRelease();

    /* Public initializer/uninitializer for internal purposes only. */
    HRESULT init(ComObjPtr<Console> pConsole);
    void uninit();

    /* Public method for internal use. */
    static DECLCALLBACK(int) i_eusbCallback(void *pv, const char *pszId, uint32_t iEvent,
                                            const void *pvData, uint32_t cbData);

    PEMULATEDUSBIF i_getEmulatedUsbIf();

    HRESULT i_webcamAttachInternal(const com::Utf8Str &aPath,
                                   const com::Utf8Str &aSettings,
                                   const char *pszDriver,
                                   void *pvObject);
    HRESULT i_webcamDetachInternal(const com::Utf8Str &aPath);

private:

    static DECLCALLBACK(int) eusbCallbackEMT(EmulatedUSB *pThis, char *pszId, uint32_t iEvent,
                                             void *pvData, uint32_t cbData);

    static DECLCALLBACK(int) i_QueryEmulatedUsbDataById(void *pvUser, const char *pszId, void **ppvEmUsbCb, void **ppvEmUsbCbData, void **ppvObject);

    HRESULT webcamPathFromId(com::Utf8Str *pPath, const char *pszId);

    // wrapped IEmulatedUSB properties
    virtual HRESULT getWebcams(std::vector<com::Utf8Str> &aWebcams);

    // wrapped IEmulatedUSB methods
    virtual HRESULT webcamAttach(const com::Utf8Str &aPath,
                                 const com::Utf8Str &aSettings);
    virtual HRESULT webcamDetach(const com::Utf8Str &aPath);

    /* Data. */
    struct Data
    {
        Data()
        {
        }

        ComObjPtr<Console> pConsole;
        WebcamsMap webcams;
    };

    Data m;
    EMULATEDUSBIF mEmUsbIf;
};

#endif /* !MAIN_INCLUDED_EmulatedUSBImpl_h */

/* vi: set tabstop=4 shiftwidth=4 expandtab: */

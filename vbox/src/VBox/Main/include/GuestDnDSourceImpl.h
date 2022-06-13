/* $Id: GuestDnDSourceImpl.h 90828 2021-08-24 09:44:46Z vboxsync $ */
/** @file
 * VBox Console COM Class implementation - Guest drag'n drop source.
 */

/*
 * Copyright (C) 2014-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_GuestDnDSourceImpl_h
#define MAIN_INCLUDED_GuestDnDSourceImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/GuestHost/DragAndDrop.h>
#include <VBox/HostServices/DragAndDropSvc.h>

using namespace DragAndDropSvc;

#include "GuestDnDSourceWrap.h"
#include "GuestDnDPrivate.h"

class GuestDnDRecvDataTask;
struct GuestDnDRecvCtx;

class ATL_NO_VTABLE GuestDnDSource :
    public GuestDnDSourceWrap,
    public GuestDnDBase
{
public:
    /** @name COM and internal init/term/mapping cruft.
     * @{ */
    DECLARE_COMMON_CLASS_METHODS(GuestDnDSource)

    HRESULT init(const ComObjPtr<Guest>& pGuest);
    void    uninit(void);

    HRESULT FinalConstruct(void);
    void    FinalRelease(void);
    /** @}  */

private:

    /** Private wrapped @name IDnDBase methods.
     * @{ */
    HRESULT isFormatSupported(const com::Utf8Str &aFormat, BOOL *aSupported);
    HRESULT getFormats(GuestDnDMIMEList &aFormats);
    HRESULT addFormats(const GuestDnDMIMEList &aFormats);
    HRESULT removeFormats(const GuestDnDMIMEList &aFormats);
    /** @}  */

    /** Private wrapped @name IDnDSource methods.
     * @{ */
    HRESULT dragIsPending(ULONG uScreenId, GuestDnDMIMEList &aFormats, std::vector<DnDAction_T> &aAllowedActions, DnDAction_T *aDefaultAction);
    HRESULT drop(const com::Utf8Str &aFormat, DnDAction_T aAction, ComPtr<IProgress> &aProgress);
    HRESULT receiveData(std::vector<BYTE> &aData);
    /** @}  */

protected:

    void i_reset(void);

#ifdef VBOX_WITH_DRAG_AND_DROP_GH
    /** @name Dispatch handlers for the HGCM callbacks.
     * @{ */
    int i_onReceiveDataHdr(GuestDnDRecvCtx *pCtx, PVBOXDNDSNDDATAHDR pDataHdr);
    int i_onReceiveData(GuestDnDRecvCtx *pCtx, PVBOXDNDSNDDATA pSndData);
    int i_onReceiveDir(GuestDnDRecvCtx *pCtx, const char *pszPath, uint32_t cbPath, uint32_t fMode);
    int i_onReceiveFileHdr(GuestDnDRecvCtx *pCtx, const char *pszPath, uint32_t cbPath, uint64_t cbSize, uint32_t fMode, uint32_t fFlags);
    int i_onReceiveFileData(GuestDnDRecvCtx *pCtx,const void *pvData, uint32_t cbData);
    /** @}  */
#endif

protected:

    static Utf8Str i_guestErrorToString(int guestRc);
    static Utf8Str i_hostErrorToString(int hostRc);

    /** @name Callbacks for dispatch handler.
     * @{ */
    static DECLCALLBACK(int) i_receiveRawDataCallback(uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser);
    static DECLCALLBACK(int) i_receiveTransferDataCallback(uint32_t uMsg, void *pvParms, size_t cbParms, void *pvUser);
    /** @}  */

protected:

    int i_receiveData(GuestDnDRecvCtx *pCtx, RTMSINTERVAL msTimeout);
    int i_receiveRawData(GuestDnDRecvCtx *pCtx, RTMSINTERVAL msTimeout);
    int i_receiveTransferData(GuestDnDRecvCtx *pCtx, RTMSINTERVAL msTimeout);

protected:

    struct
    {
        /** Maximum data block size (in bytes) the source can handle. */
        uint32_t    mcbBlockSize;
        /** The context for receiving data from the guest.
         *  At the moment only one transfer at a time is supported. */
        GuestDnDRecvCtx mRecvCtx;
    } mData;

    friend class GuestDnDRecvDataTask;
};

#endif /* !MAIN_INCLUDED_GuestDnDSourceImpl_h */


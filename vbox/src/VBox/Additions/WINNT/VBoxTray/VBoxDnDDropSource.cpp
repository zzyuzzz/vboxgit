/* $Id: VBoxDnDDropSource.cpp 85694 2020-08-11 16:30:25Z vboxsync $ */
/** @file
 * VBoxDnDSource.cpp - IDropSource implementation.
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <iprt/win/windows.h>
#include <new> /* For bad_alloc. */

#ifdef LOG_GROUP
# undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>

#include "VBoxTray.h"
#include "VBoxHelpers.h"
#include "VBoxDnD.h"

#include "VBox/HostServices/DragAndDropSvc.h"



VBoxDnDDropSource::VBoxDnDDropSource(VBoxDnDWnd *pParent)
    : m_cRefs(1),
      m_pWndParent(pParent),
      m_dwCurEffect(0),
      m_enmActionCurrent(VBOX_DND_ACTION_IGNORE)
{
    LogFlowFuncEnter();
}

VBoxDnDDropSource::~VBoxDnDDropSource(void)
{
    LogFlowFunc(("mRefCount=%RI32\n", m_cRefs));
}

/*
 * IUnknown methods.
 */

STDMETHODIMP_(ULONG) VBoxDnDDropSource::AddRef(void)
{
    return InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) VBoxDnDDropSource::Release(void)
{
    LONG lCount = InterlockedDecrement(&m_cRefs);
    if (lCount == 0)
    {
        delete this;
        return 0;
    }

    return lCount;
}

STDMETHODIMP VBoxDnDDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
    AssertPtrReturn(ppvObject, E_INVALIDARG);

    if (   iid == IID_IDropSource
        || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }

    *ppvObject = 0;
    return E_NOINTERFACE;
}

/*
 * IDropSource methods.
 */

/**
 * The system informs us about whether we should continue the drag'n drop
 * operation or not, depending on the sent key states.
 *
 * @return  HRESULT
 */
STDMETHODIMP VBoxDnDDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD dwKeyState)
{
#if 1
    LogFlowFunc(("fEscapePressed=%RTbool, dwKeyState=0x%x, mdwCurEffect=%RI32, mDnDActionCurrent=%RU32\n",
                 fEscapePressed, dwKeyState, m_dwCurEffect, m_enmActionCurrent));
#endif

    /* ESC pressed? Bail out. */
    if (fEscapePressed)
    {
        m_dwCurEffect = 0;
        m_enmActionCurrent = VBOX_DND_ACTION_IGNORE;

        LogFlowFunc(("Canceled\n"));
        return DRAGDROP_S_CANCEL;
    }

    /* Left mouse button released? Start "drop" action. */
    if ((dwKeyState & MK_LBUTTON) == 0)
    {
        LogFlowFunc(("Dropping ...\n"));
        return DRAGDROP_S_DROP;
    }

    /* No change, just continue. */
    return S_OK;
}

/**
 * The drop target gives our source feedback about whether
 * it can handle our data or not.
 *
 * @return  HRESULT
 */
STDMETHODIMP VBoxDnDDropSource::GiveFeedback(DWORD dwEffect)
{
    uint32_t uAction = VBOX_DND_ACTION_IGNORE;

#if 1
    LogFlowFunc(("dwEffect=0x%x\n", dwEffect));
#endif
    if (dwEffect)
    {
        if (dwEffect & DROPEFFECT_COPY)
            uAction |= VBOX_DND_ACTION_COPY;
        if (dwEffect & DROPEFFECT_MOVE)
            uAction |= VBOX_DND_ACTION_MOVE;
        if (dwEffect & DROPEFFECT_LINK)
            uAction |= VBOX_DND_ACTION_LINK;
    }

    m_dwCurEffect = dwEffect;
    m_enmActionCurrent = uAction;

    return DRAGDROP_S_USEDEFAULTCURSORS;
}


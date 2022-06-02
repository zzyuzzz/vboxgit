/** @file
 *
 * VBoxVideo Display D3D User mode dll
 *
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#include "VBoxDispD3DCmn.h"
#include "VBoxDispD3D.h"
#include "../../../include/VBoxDisplay.h"

#include <iprt/list.h>


typedef struct VBOXDISPCM_SESSION
{
    HANDLE hEvent;
    CRITICAL_SECTION CritSect;
    RTLISTNODE CtxList;
    bool bQueryMp;
} VBOXDISPCM_SESSION, *PVBOXDISPCM_SESSION;

typedef struct VBOXDISPCM_MGR
{
    VBOXDISPCM_SESSION Session;
} VBOXDISPCM_MGR, *PVBOXDISPCM_MGR;

/* the cm is one per process */
static VBOXDISPCM_MGR g_pVBoxCmMgr;

HRESULT vboxDispCmSessionTerm(PVBOXDISPCM_SESSION pSession)
{
    Assert(RTListIsEmpty(&pSession->CtxList));
    BOOL bRc = CloseHandle(pSession->hEvent);
    Assert(bRc);
    if (bRc)
    {
        DeleteCriticalSection(&pSession->CritSect);
        return S_OK;
    }
    DWORD winEr = GetLastError();
    HRESULT hr = HRESULT_FROM_WIN32(winEr);
    return hr;
}

HRESULT vboxDispCmSessionInit(PVBOXDISPCM_SESSION pSession)
{
    HANDLE hEvent = CreateEvent(NULL,
            FALSE, /* BOOL bManualReset */
            FALSE, /* BOOL bInitialState */
            NULL /* LPCTSTR lpName */
            );
    Assert(hEvent);
    if (hEvent)
    {
        pSession->hEvent = hEvent;
        InitializeCriticalSection(&pSession->CritSect);
        RTListInit(&pSession->CtxList);
        pSession->bQueryMp = false;
        return S_OK;
    }
    DWORD winEr = GetLastError();
    HRESULT hr = HRESULT_FROM_WIN32(winEr);
    return hr;
}

void vboxDispCmSessionCtxAdd(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_CONTEXT pContext)
{
    EnterCriticalSection(&pSession->CritSect);
    RTListAppend(&pSession->CtxList, &pContext->ListNode);
    LeaveCriticalSection(&pSession->CritSect);
}

void vboxDispCmSessionCtxRemoveLocked(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_CONTEXT pContext)
{
    RTListNodeRemove(&pContext->ListNode);
}

void vboxDispCmSessionCtxRemove(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_CONTEXT pContext)
{
    EnterCriticalSection(&pSession->CritSect);
    vboxDispCmSessionCtxRemoveLocked(pSession, pContext);
    LeaveCriticalSection(&pSession->CritSect);
}

HRESULT vboxDispCmInit()
{
    HRESULT hr = vboxDispCmSessionInit(&g_pVBoxCmMgr.Session);
    Assert(hr == S_OK);
    return hr;
}

HRESULT vboxDispCmTerm()
{
    HRESULT hr = vboxDispCmSessionTerm(&g_pVBoxCmMgr.Session);
    Assert(hr == S_OK);
    return hr;
}

HRESULT vboxDispCmCtxCreate(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_CONTEXT pContext)
{
    VBOXWDDM_CREATECONTEXT_INFO Info;
    Info.u32IfVersion = pDevice->u32IfVersion;
    Info.enmType = VBOXDISPMODE_IS_3D(pDevice->pAdapter) ? VBOXWDDM_CONTEXT_TYPE_CUSTOM_3D : VBOXWDDM_CONTEXT_TYPE_CUSTOM_2D;
    Info.hUmEvent = (uint64_t)g_pVBoxCmMgr.Session.hEvent;
    Info.u64UmInfo = (uint64_t)pContext;

    pContext->ContextInfo.NodeOrdinal = 0;
    pContext->ContextInfo.EngineAffinity = 0;
    pContext->ContextInfo.Flags.Value = 0;
    pContext->ContextInfo.pPrivateDriverData = &Info;
    pContext->ContextInfo.PrivateDriverDataSize = sizeof (Info);
    pContext->ContextInfo.hContext = 0;
    pContext->ContextInfo.pCommandBuffer = NULL;
    pContext->ContextInfo.CommandBufferSize = 0;
    pContext->ContextInfo.pAllocationList = NULL;
    pContext->ContextInfo.AllocationListSize = 0;
    pContext->ContextInfo.pPatchLocationList = NULL;
    pContext->ContextInfo.PatchLocationListSize = 0;

    HRESULT hr = S_OK;
    hr = pDevice->RtCallbacks.pfnCreateContextCb(pDevice->hDevice, &pContext->ContextInfo);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        Assert(pContext->ContextInfo.hContext);
        pContext->ContextInfo.pPrivateDriverData = NULL;
        pContext->ContextInfo.PrivateDriverDataSize = 0;
        vboxDispCmSessionCtxAdd(&g_pVBoxCmMgr.Session, pContext);
        pContext->pDevice = pDevice;
    }
    else
    {
        exit(1);
    }

    return hr;
}

HRESULT vboxDispCmSessionCtxDestroy(PVBOXDISPCM_SESSION pSession, PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_CONTEXT pContext)
{
    EnterCriticalSection(&pSession->CritSect);
    Assert(pContext->ContextInfo.hContext);
    D3DDDICB_DESTROYCONTEXT DestroyContext;
    Assert(pDevice->DefaultContext.ContextInfo.hContext);
    DestroyContext.hContext = pDevice->DefaultContext.ContextInfo.hContext;
    HRESULT hr = pDevice->RtCallbacks.pfnDestroyContextCb(pDevice->hDevice, &DestroyContext);
    Assert(hr == S_OK);
    if (hr == S_OK)
    {
        vboxDispCmSessionCtxRemoveLocked(pSession, pContext);
    }
    LeaveCriticalSection(&pSession->CritSect);
    return hr;
}

HRESULT vboxDispCmCtxDestroy(PVBOXWDDMDISP_DEVICE pDevice, PVBOXWDDMDISP_CONTEXT pContext)
{
    return vboxDispCmSessionCtxDestroy(&g_pVBoxCmMgr.Session, pDevice, pContext);
}

static HRESULT vboxDispCmSessionCmdQueryData(PVBOXDISPCM_SESSION pSession, PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD pCmd, uint32_t cbCmd)
{

    HRESULT hr = S_OK;
    D3DDDICB_ESCAPE DdiEscape;
    DdiEscape.Flags.Value = 0;
    DdiEscape.pPrivateDriverData = pCmd;
    DdiEscape.PrivateDriverDataSize = cbCmd;

    pCmd->EscapeHdr.escapeCode = VBOXESC_GETVBOXVIDEOCMCMD;
    /* lock to ensure the context is not destroyed */
    EnterCriticalSection(&pSession->CritSect);
    /* use any context for identifying the kernel CmSession. We're using the first one */
    PVBOXWDDMDISP_CONTEXT pContext = RTListNodeGetFirst(&pSession->CtxList, VBOXWDDMDISP_CONTEXT, ListNode);
    if (pContext)
    {
        PVBOXWDDMDISP_DEVICE pDevice = pContext->pDevice;
        DdiEscape.hDevice = pDevice->hDevice;
        DdiEscape.hContext = pContext->ContextInfo.hContext;
        Assert (DdiEscape.hContext);
        Assert (DdiEscape.hDevice);
        hr = pDevice->RtCallbacks.pfnEscapeCb(pDevice->pAdapter->hAdapter, &DdiEscape);
        LeaveCriticalSection(&pSession->CritSect);
        Assert(hr == S_OK);
        if (hr == S_OK)
        {
            if (!pCmd->Hdr.cbCmdsReturned && !pCmd->Hdr.cbRemainingFirstCmd)
                hr = S_FALSE;
        }
        else
        {
            exit(1);
        }
    }
    else
    {
        LeaveCriticalSection(&pSession->CritSect);
        hr = S_FALSE;
    }

    return hr;
}

HRESULT vboxDispCmSessionCmdGet(PVBOXDISPCM_SESSION pSession, PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD pCmd, uint32_t cbCmd, DWORD dwMilliseconds)
{
    Assert(cbCmd >= sizeof (VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD));
    if (cbCmd < sizeof (VBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD))
        return E_INVALIDARG;

    do
    {

        if (pSession->bQueryMp)
        {
            HRESULT hr = vboxDispCmSessionCmdQueryData(pSession, pCmd, cbCmd);
            Assert(hr == S_OK || hr == S_FALSE);
            if (hr == S_OK || hr != S_FALSE)
                return hr;

            pSession->bQueryMp = false;
        }

        DWORD dwResult = WaitForSingleObject(pSession->hEvent, dwMilliseconds);
        switch(dwResult)
        {
            case WAIT_OBJECT_0:
            {
                pSession->bQueryMp = true;
                break; /* <- query commands */
            }
            case WAIT_TIMEOUT:
            {
                Assert(!pSession->bQueryMp);
                return WAIT_TIMEOUT;
            }
            default:
                Assert(0);
                return E_FAIL;
        }
    } while (1);

    /* should never be here */
    Assert(0);
    return E_FAIL;
}

HRESULT vboxDispCmCmdGet(PVBOXDISPIFESCAPE_GETVBOXVIDEOCMCMD pCmd, uint32_t cbCmd, DWORD dwMilliseconds)
{
    return vboxDispCmSessionCmdGet(&g_pVBoxCmMgr.Session, pCmd, cbCmd, dwMilliseconds);
}

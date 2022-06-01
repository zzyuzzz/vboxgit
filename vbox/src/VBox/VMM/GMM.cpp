/* $Id: GMM.cpp 28974 2010-05-03 13:09:44Z vboxsync $ */
/** @file
 * GMM - Global Memory Manager, ring-3 request wrappers.
 */

/*
 * Copyright (C) 2008 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_GMM
#include <VBox/gmm.h>
#include <VBox/vmm.h>
#include <VBox/vm.h>
#include <VBox/sup.h>
#include <VBox/err.h>
#include <VBox/param.h>

#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/mem.h>
#include <iprt/string.h>


/**
 * @see GMMR0InitialReservation
 */
GMMR3DECL(int)  GMMR3InitialReservation(PVM pVM, uint64_t cBasePages, uint32_t cShadowPages, uint32_t cFixedPages,
                                        GMMOCPOLICY enmPolicy, GMMPRIORITY enmPriority)
{
    GMMINITIALRESERVATIONREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq = sizeof(Req);
    Req.cBasePages = cBasePages;
    Req.cShadowPages = cShadowPages;
    Req.cFixedPages = cFixedPages;
    Req.enmPolicy = enmPolicy;
    Req.enmPriority = enmPriority;
    return VMMR3CallR0(pVM, VMMR0_DO_GMM_INITIAL_RESERVATION, 0, &Req.Hdr);
}


/**
 * @see GMMR0UpdateReservation
 */
GMMR3DECL(int)  GMMR3UpdateReservation(PVM pVM, uint64_t cBasePages, uint32_t cShadowPages, uint32_t cFixedPages)
{
    GMMUPDATERESERVATIONREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq = sizeof(Req);
    Req.cBasePages = cBasePages;
    Req.cShadowPages = cShadowPages;
    Req.cFixedPages = cFixedPages;
    return VMMR3CallR0(pVM, VMMR0_DO_GMM_UPDATE_RESERVATION, 0, &Req.Hdr);
}


/**
 * Prepares a GMMR0AllocatePages request.
 *
 * @returns VINF_SUCCESS or VERR_NO_TMP_MEMORY.
 * @param       pVM         Pointer to the shared VM structure.
 * @param[out]  ppReq       Where to store the pointer to the request packet.
 * @param       cPages      The number of pages that's to be allocated.
 * @param       enmAccount  The account to charge.
 */
GMMR3DECL(int) GMMR3AllocatePagesPrepare(PVM pVM, PGMMALLOCATEPAGESREQ *ppReq, uint32_t cPages, GMMACCOUNT enmAccount)
{
    uint32_t cb = RT_OFFSETOF(GMMALLOCATEPAGESREQ, aPages[cPages]);
    PGMMALLOCATEPAGESREQ pReq = (PGMMALLOCATEPAGESREQ)RTMemTmpAllocZ(cb);
    if (!pReq)
        return VERR_NO_TMP_MEMORY;

    pReq->Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    pReq->Hdr.cbReq = cb;
    pReq->enmAccount = enmAccount;
    pReq->cPages = cPages;
    NOREF(pVM);
    *ppReq = pReq;
    return VINF_SUCCESS;
}


/**
 * Performs a GMMR0AllocatePages request.
 * This will call VMSetError on failure.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the shared VM structure.
 * @param   pReq        Pointer to the request (returned by GMMR3AllocatePagesPrepare).
 */
GMMR3DECL(int) GMMR3AllocatePagesPerform(PVM pVM, PGMMALLOCATEPAGESREQ pReq)
{
    for (unsigned i = 0; ; i++)
    {
        int rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_ALLOCATE_PAGES, 0, &pReq->Hdr);
        if (RT_SUCCESS(rc))
        {
#ifdef LOG_ENABLED
            for (uint32_t iPage = 0; iPage < pReq->cPages; iPage++)
                Log3(("GMMR3AllocatePagesPerform: idPage=%#x HCPhys=%RHp\n",
                      pReq->aPages[iPage].idPage, pReq->aPages[iPage].HCPhysGCPhys));
#endif
            return rc;
        }
        if (rc != VERR_GMM_SEED_ME)
            return VMSetError(pVM, rc, RT_SRC_POS,
                              N_("GMMR0AllocatePages failed to allocate %u pages"),
                              pReq->cPages);
        Assert(i < pReq->cPages);

        /*
         * Seed another chunk.
         */
        void *pvChunk;
        rc = SUPR3PageAlloc(GMM_CHUNK_SIZE >> PAGE_SHIFT, &pvChunk);
        if (RT_FAILURE(rc))
            return VMSetError(pVM, rc, RT_SRC_POS,
                              N_("Out of memory (SUPR3PageAlloc) seeding a %u pages allocation request"),
                              pReq->cPages);

        rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_SEED_CHUNK, (uintptr_t)pvChunk, NULL);
        if (RT_FAILURE(rc))
            return VMSetError(pVM, rc, RT_SRC_POS, N_("GMM seeding failed"));
    }
}


/**
 * Cleans up a GMMR0AllocatePages request.
 * @param   pReq        Pointer to the request (returned by GMMR3AllocatePagesPrepare).
 */
GMMR3DECL(void) GMMR3AllocatePagesCleanup(PGMMALLOCATEPAGESREQ pReq)
{
    RTMemTmpFree(pReq);
}


/**
 * Prepares a GMMR0FreePages request.
 *
 * @returns VINF_SUCCESS or VERR_NO_TMP_MEMORY.
 * @param       pVM         Pointer to the shared VM structure.
 * @param[out]  ppReq       Where to store the pointer to the request packet.
 * @param       cPages      The number of pages that's to be freed.
 * @param       enmAccount  The account to charge.
 */
GMMR3DECL(int) GMMR3FreePagesPrepare(PVM pVM, PGMMFREEPAGESREQ *ppReq, uint32_t cPages, GMMACCOUNT enmAccount)
{
    uint32_t cb = RT_OFFSETOF(GMMFREEPAGESREQ, aPages[cPages]);
    PGMMFREEPAGESREQ pReq = (PGMMFREEPAGESREQ)RTMemTmpAllocZ(cb);
    if (!pReq)
        return VERR_NO_TMP_MEMORY;

    pReq->Hdr.u32Magic  = SUPVMMR0REQHDR_MAGIC;
    pReq->Hdr.cbReq     = cb;
    pReq->enmAccount    = enmAccount;
    pReq->cPages        = cPages;
    NOREF(pVM);
    *ppReq = pReq;
    return VINF_SUCCESS;
}


/**
 * Re-prepares a GMMR0FreePages request.
 *
 * @returns VINF_SUCCESS or VERR_NO_TMP_MEMORY.
 * @param       pVM         Pointer to the shared VM structure.
 * @param       pReq        A request buffer previously returned by
 *                          GMMR3FreePagesPrepare().
 * @param       cPages      The number of pages originally passed to
 *                          GMMR3FreePagesPrepare().
 * @param       enmAccount  The account to charge.
 */
GMMR3DECL(void) GMMR3FreePagesRePrep(PVM pVM, PGMMFREEPAGESREQ pReq, uint32_t cPages, GMMACCOUNT enmAccount)
{
    Assert(pReq->Hdr.u32Magic == SUPVMMR0REQHDR_MAGIC);
    pReq->Hdr.cbReq     = RT_OFFSETOF(GMMFREEPAGESREQ, aPages[cPages]);
    pReq->enmAccount    = enmAccount;
    pReq->cPages        = cPages;
    NOREF(pVM);
}


/**
 * Performs a GMMR0FreePages request.
 * This will call VMSetError on failure.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pReq            Pointer to the request (returned by GMMR3FreePagesPrepare).
 * @param   cActualPages    The number of pages actually freed.
 */
GMMR3DECL(int) GMMR3FreePagesPerform(PVM pVM, PGMMFREEPAGESREQ pReq, uint32_t cActualPages)
{
    /*
     * Adjust the request if we ended up with fewer pages than anticipated.
     */
    if (cActualPages != pReq->cPages)
    {
        AssertReturn(cActualPages < pReq->cPages, VERR_INTERNAL_ERROR);
        if (!cActualPages)
            return VINF_SUCCESS;
        pReq->cPages = cActualPages;
        pReq->Hdr.cbReq = RT_OFFSETOF(GMMFREEPAGESREQ, aPages[cActualPages]);
    }

    /*
     * Do the job.
     */
    int rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_FREE_PAGES, 0, &pReq->Hdr);
    if (RT_SUCCESS(rc))
        return rc;
    AssertRC(rc);
    return VMSetError(pVM, rc, RT_SRC_POS,
                      N_("GMMR0FreePages failed to free %u pages"),
                      pReq->cPages);
}


/**
 * Cleans up a GMMR0FreePages request.
 * @param   pReq        Pointer to the request (returned by GMMR3FreePagesPrepare).
 */
GMMR3DECL(void) GMMR3FreePagesCleanup(PGMMFREEPAGESREQ pReq)
{
    RTMemTmpFree(pReq);
}


/**
 * Frees allocated pages, for bailing out on failure.
 *
 * This will not call VMSetError on failure but will use AssertLogRel instead.
 *
 * @param   pVM         Pointer to the shared VM structure.
 * @param   pAllocReq   The allocation request to undo.
 */
GMMR3DECL(void) GMMR3FreeAllocatedPages(PVM pVM, GMMALLOCATEPAGESREQ const *pAllocReq)
{
    uint32_t cb = RT_OFFSETOF(GMMFREEPAGESREQ, aPages[pAllocReq->cPages]);
    PGMMFREEPAGESREQ pReq = (PGMMFREEPAGESREQ)RTMemTmpAllocZ(cb);
    AssertLogRelReturnVoid(pReq);

    pReq->Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    pReq->Hdr.cbReq = cb;
    pReq->enmAccount = pAllocReq->enmAccount;
    pReq->cPages = pAllocReq->cPages;
    uint32_t iPage = pAllocReq->cPages;
    while (iPage-- > 0)
    {
        Assert(pAllocReq->aPages[iPage].idPage != NIL_GMM_PAGEID);
        pReq->aPages[iPage].idPage = pAllocReq->aPages[iPage].idPage;
    }

    int rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_FREE_PAGES, 0, &pReq->Hdr);
    AssertLogRelRC(rc);

    RTMemTmpFree(pReq);
}


/**
 * @see GMMR0BalloonedPages
 */
GMMR3DECL(int)  GMMR3BalloonedPages(PVM pVM, GMMBALLOONACTION enmAction, uint32_t cBalloonedPages)
{
    GMMBALLOONEDPAGESREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq = sizeof(Req);
    Req.enmAction = enmAction;
    Req.cBalloonedPages = cBalloonedPages;

    return VMMR3CallR0(pVM, VMMR0_DO_GMM_BALLOONED_PAGES, 0, &Req.Hdr);
}

/**
 * @see GMMR0QueryVMMMemoryStatsReq
 */
GMMR3DECL(int)  GMMR3QueryHypervisorMemoryStats(PVM pVM, uint64_t *pcTotalAllocPages, uint64_t *pcTotalFreePages, uint64_t *pcTotalBalloonPages)
{
    GMMMEMSTATSREQ Req;
    Req.Hdr.u32Magic     = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq        = sizeof(Req);
    Req.cAllocPages      = 0;
    Req.cFreePages       = 0;
    Req.cBalloonedPages  = 0;

    *pcTotalAllocPages   = 0;
    *pcTotalFreePages    = 0;
    *pcTotalBalloonPages = 0;

    /* Must be callable from any thread, so can't use VMMR3CallR0. */
    int rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0, VMMR0_DO_GMM_QUERY_HYPERVISOR_MEM_STATS, 0, &Req.Hdr);
    if (rc == VINF_SUCCESS)
    {
        *pcTotalAllocPages   = Req.cAllocPages;
        *pcTotalFreePages    = Req.cFreePages;
        *pcTotalBalloonPages = Req.cBalloonedPages;
    }
    return rc;
}

/**
 * @see GMMR0QueryMemoryStatsReq
 */
GMMR3DECL(int)  GMMR3QueryMemoryStats(PVM pVM, uint64_t *pcAllocPages, uint64_t *pcMaxPages, uint64_t *pcBalloonPages)
{
    GMMMEMSTATSREQ Req;
    Req.Hdr.u32Magic    = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq       = sizeof(Req);
    Req.cAllocPages     = 0;
    Req.cFreePages      = 0;
    Req.cBalloonedPages = 0;

    *pcAllocPages      = 0;
    *pcMaxPages         = 0;
    *pcBalloonPages     = 0;

    /* Must be callable from any thread, so can't use VMMR3CallR0. */
    int rc = SUPR3CallVMMR0Ex(pVM->pVMR0, 0, VMMR0_DO_GMM_QUERY_MEM_STATS, 0, &Req.Hdr);
    if (rc == VINF_SUCCESS)
    {
        *pcAllocPages   = Req.cAllocPages;
        *pcMaxPages     = Req.cMaxPages;
        *pcBalloonPages = Req.cBalloonedPages;
    }
    return rc;
}

/**
 * @see GMMR0MapUnmapChunk
 */
GMMR3DECL(int)  GMMR3MapUnmapChunk(PVM pVM, uint32_t idChunkMap, uint32_t idChunkUnmap, PRTR3PTR ppvR3)
{
    GMMMAPUNMAPCHUNKREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq = sizeof(Req);
    Req.idChunkMap = idChunkMap;
    Req.idChunkUnmap = idChunkUnmap;
    Req.pvR3 = NULL;
    int rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_MAP_UNMAP_CHUNK, 0, &Req.Hdr);
    if (RT_SUCCESS(rc) && ppvR3)
        *ppvR3 = Req.pvR3;
    return rc;
}

/**
 * @see GMMR0FreeLargePage
 */
GMMR3DECL(int)  GMMR3FreeLargePage(PVM pVM,  uint32_t idPage)
{
    GMMFREELARGEPAGEREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq = sizeof(Req);
    Req.idPage = idPage;
    return VMMR3CallR0(pVM, VMMR0_DO_GMM_FREE_LARGE_PAGE, 0, &Req.Hdr);
}

/**
 * @see GMMR0SeedChunk
 */
GMMR3DECL(int)  GMMR3SeedChunk(PVM pVM, RTR3PTR pvR3)
{
    return VMMR3CallR0(pVM, VMMR0_DO_GMM_SEED_CHUNK, (uintptr_t)pvR3, NULL);
}


/**
 * @see GMMR0RegisterSharedModule
 */
GMMR3DECL(int) GMMR3RegisterSharedModule(PVM pVM, char *pszModuleName, char *pszVersion, RTGCPTR GCBaseAddr, uint32_t cbModule,
                                         unsigned cRegions, VMMDEVSHAREDREGIONDESC *pRegions)
{
    PGMMREGISTERSHAREDMODULEREQ pReq;
    int rc;

    /* Sanity check. */
    AssertReturn(cRegions < VMMDEVSHAREDREGIONDESC_MAX, VERR_INVALID_PARAMETER);

    pReq = (PGMMREGISTERSHAREDMODULEREQ)RTMemAllocZ(RT_OFFSETOF(GMMREGISTERSHAREDMODULEREQ, aRegions[cRegions]));
    AssertReturn(pReq, VERR_NO_MEMORY);

    pReq->Hdr.u32Magic  = SUPVMMR0REQHDR_MAGIC;
    pReq->Hdr.cbReq     = sizeof(*pReq);
    pReq->GCBaseAddr    = GCBaseAddr;
    pReq->cbModule      = cbModule;
    pReq->cRegions      = cRegions;
    for (unsigned i = 0; i < cRegions; i++)
        pReq->aRegions[i] = pRegions[i];

    if (    RTStrCopy(pReq->szName, sizeof(pReq->szName), pszModuleName) != VINF_SUCCESS
        ||  RTStrCopy(pReq->szVersion, sizeof(pReq->szVersion), pszVersion) != VINF_SUCCESS)
    {
        rc = VERR_BUFFER_OVERFLOW;
        goto end;
    }

    rc = VMMR3CallR0(pVM, VMMR0_DO_GMM_REGISTER_SHARED_MODULE, 0, &pReq->Hdr);
end:
    RTMemFree(pReq);
    return rc;
}

/**
 * @see GMMR0RegisterSharedModule
 */
GMMR3DECL(int) GMMR3UnregisterSharedModule(PVM pVM, char *pszModuleName, char *pszVersion, RTGCPTR GCBaseAddr, uint32_t cbModule)
{
    GMMUNREGISTERSHAREDMODULEREQ Req;
    Req.Hdr.u32Magic = SUPVMMR0REQHDR_MAGIC;
    Req.Hdr.cbReq = sizeof(Req);

    Req.GCBaseAddr    = GCBaseAddr;
    Req.cbModule      = cbModule;

    if (    RTStrCopy(Req.szName, sizeof(Req.szName), pszModuleName) != VINF_SUCCESS
        ||  RTStrCopy(Req.szVersion, sizeof(Req.szVersion), pszVersion) != VINF_SUCCESS)
    {
        return VERR_BUFFER_OVERFLOW;
    }

    return VMMR3CallR0(pVM, VMMR0_DO_GMM_UNREGISTER_SHARED_MODULE, 0, &Req.Hdr);
}

/**
 * @see GMMR0CheckSharedModules
 */
GMMR3DECL(int) GMMR3CheckSharedModules(PVM pVM)
{
    return VMMR3CallR0(pVM, VMMR0_DO_GMM_CHECK_SHARED_MODULES, 0, NULL);
}

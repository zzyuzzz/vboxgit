/* $Id: MMPagePool.cpp 12814 2008-09-29 18:14:37Z vboxsync $ */
/** @file
 * MM - Memory Monitor(/Manager) - Page Pool.
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
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_MM_POOL
#include <VBox/mm.h>
#include <VBox/pgm.h>
#include <VBox/stam.h>
#include "MMInternal.h"
#include <VBox/vm.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#define USE_INLINE_ASM_BIT_OPS
#ifdef USE_INLINE_ASM_BIT_OPS
# include <iprt/asm.h>
#endif
#include <iprt/string.h>



/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
#ifdef IN_RING3
static void *   mmR3PagePoolAlloc(PMMPAGEPOOL pPool);
static void     mmR3PagePoolFree(PMMPAGEPOOL pPool, void *pv);
#endif


/**
 * Initializes the page pool
 *
 * @return  VBox status.
 * @param   pVM     VM handle.
 * @thread  The Emulation Thread.
 */
int mmR3PagePoolInit(PVM pVM)
{
    AssertMsg(!pVM->mm.s.pPagePool, ("Already initialized!\n"));

    /*
     * Allocate the pool structures.
     */
    AssertRelease(sizeof(*pVM->mm.s.pPagePool) + sizeof(*pVM->mm.s.pPagePoolLow) < PAGE_SIZE);
    int rc = SUPPageAllocLocked(1, (void **)&pVM->mm.s.pPagePool);
    if (VBOX_FAILURE(rc))
        return rc;
    memset(pVM->mm.s.pPagePool, 0, PAGE_SIZE);
    pVM->mm.s.pPagePool->pVM = pVM;
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cPages,         STAMTYPE_U32,     "/MM/Page/Def/cPages",        STAMUNIT_PAGES, "Number of pages in the default pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cFreePages,     STAMTYPE_U32,     "/MM/Page/Def/cFreePages",    STAMUNIT_PAGES, "Number of free pages in the default pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cSubPools,      STAMTYPE_U32,     "/MM/Page/Def/cSubPools",     STAMUNIT_COUNT, "Number of sub pools in the default pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cAllocCalls,    STAMTYPE_COUNTER, "/MM/Page/Def/cAllocCalls",   STAMUNIT_CALLS, "Number of MMR3PageAlloc() calls for the default pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cFreeCalls,     STAMTYPE_COUNTER, "/MM/Page/Def/cFreeCalls",    STAMUNIT_CALLS, "Number of MMR3PageFree()+MMR3PageFreeByPhys() calls for the default pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cToPhysCalls,   STAMTYPE_COUNTER, "/MM/Page/Def/cToPhysCalls",  STAMUNIT_CALLS, "Number of MMR3Page2Phys() calls for this pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cToVirtCalls,   STAMTYPE_COUNTER, "/MM/Page/Def/cToVirtCalls",  STAMUNIT_CALLS, "Number of MMR3PagePhys2Page()+MMR3PageFreeByPhys() calls for the default pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePool->cErrors,        STAMTYPE_COUNTER, "/MM/Page/Def/cErrors",       STAMUNIT_ERRORS,"Number of errors for the default pool.");

    pVM->mm.s.pPagePoolLow = pVM->mm.s.pPagePool + 1;
    pVM->mm.s.pPagePoolLow->pVM = pVM;
    pVM->mm.s.pPagePoolLow->fLow = true;
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cPages,      STAMTYPE_U32,     "/MM/Page/Low/cPages",        STAMUNIT_PAGES, "Number of pages in the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cFreePages,  STAMTYPE_U32,     "/MM/Page/Low/cFreePages",    STAMUNIT_PAGES, "Number of free pages in the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cSubPools,   STAMTYPE_U32,     "/MM/Page/Low/cSubPools",     STAMUNIT_COUNT, "Number of sub pools in the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cAllocCalls, STAMTYPE_COUNTER, "/MM/Page/Low/cAllocCalls",   STAMUNIT_CALLS, "Number of MMR3PageAllocLow() calls for the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cFreeCalls,  STAMTYPE_COUNTER, "/MM/Page/Low/cFreeCalls",    STAMUNIT_CALLS, "Number of MMR3PageFreeLow()+MMR3PageFreeByPhys() calls for the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cToPhysCalls,STAMTYPE_COUNTER, "/MM/Page/Low/cToPhysCalls",  STAMUNIT_CALLS, "Number of MMR3Page2Phys() calls for the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cToVirtCalls,STAMTYPE_COUNTER, "/MM/Page/Low/cToVirtCalls",  STAMUNIT_CALLS, "Number of MMR3PagePhys2Page()+MMR3PageFreeByPhys() calls for the <4GB pool.");
    STAM_REG(pVM, &pVM->mm.s.pPagePoolLow->cErrors,     STAMTYPE_COUNTER, "/MM/Page/Low/cErrors",       STAMUNIT_ERRORS,"Number of errors for the <4GB pool.");

    /** @todo init a mutex? */
    return VINF_SUCCESS;
}


/**
 * Release all locks and free the allocated memory.
 *
 * @param   pVM     VM handle.
 * @thread  The Emulation Thread.
 */
void mmR3PagePoolTerm(PVM pVM)
{
    if (pVM->mm.s.pPagePool)
    {
        /*
         * Unlock all memory held by subpools and free the memory.
         * (The MM Heap will free the memory used for internal stuff.)
         */
        Assert(!pVM->mm.s.pPagePool->fLow);
        PMMPAGESUBPOOL  pSubPool = pVM->mm.s.pPagePool->pHead;
        while (pSubPool)
        {
            int rc = SUPPageUnlock(pSubPool->pvPages);
            AssertMsgRC(rc, ("SUPPageUnlock(%p) failed with rc=%d\n", pSubPool->pvPages, rc));
            rc = SUPPageFree(pSubPool->pvPages, pSubPool->cPages);
            AssertMsgRC(rc, ("SUPPageFree(%p) failed with rc=%d\n", pSubPool->pvPages, rc));
            pSubPool->pvPages = NULL;

            /* next */
            pSubPool = pSubPool->pNext;
        }
        pVM->mm.s.pPagePool = NULL;
    }

    if (pVM->mm.s.pPagePoolLow)
    {
        /*
         * Free the memory.
         */
        Assert(pVM->mm.s.pPagePoolLow->fLow);
        PMMPAGESUBPOOL  pSubPool = pVM->mm.s.pPagePoolLow->pHead;
        while (pSubPool)
        {
            int rc = SUPLowFree(pSubPool->pvPages, pSubPool->cPages);
            AssertMsgRC(rc, ("SUPPageFree(%p) failed with rc=%d\n", pSubPool->pvPages, rc));
            pSubPool->pvPages = NULL;

            /* next */
            pSubPool = pSubPool->pNext;
        }
        pVM->mm.s.pPagePoolLow = NULL;
    }
}


/**
 * Allocates a page from the page pool.
 *
 * @returns Pointer to allocated page(s).
 * @returns NULL on failure.
 * @param   pPool   Pointer to the page pool.
 * @thread  The Emulation Thread.
 */
DECLINLINE(void *) mmR3PagePoolAlloc(PMMPAGEPOOL pPool)
{
    VM_ASSERT_EMT(pPool->pVM);
    STAM_COUNTER_INC(&pPool->cAllocCalls);

    /*
     * Walk free list.
     */
    if (pPool->pHeadFree)
    {
        PMMPAGESUBPOOL  pSub = pPool->pHeadFree;
        /* decrement free count and unlink if no more free entries. */
        if (!--pSub->cPagesFree)
            pPool->pHeadFree = pSub->pNextFree;
#ifdef VBOX_WITH_STATISTICS
        pPool->cFreePages--;
#endif

        /* find free spot in bitmap. */
#ifdef USE_INLINE_ASM_BIT_OPS
        const int iPage = ASMBitFirstClear(pSub->auBitmap, pSub->cPages);
        if (iPage >= 0)
        {
            Assert(!ASMBitTest(pSub->auBitmap, iPage));
            ASMBitSet(pSub->auBitmap, iPage);
            return (uint8_t *)pSub->pvPages + PAGE_SIZE * iPage;
        }
#else
        unsigned   *pu = &pSub->auBitmap[0];
        unsigned   *puEnd = &pSub->auBitmap[pSub->cPages / (sizeof(pSub->auBitmap) * 8)];
        while (pu < puEnd)
        {
            unsigned u;
            if ((u = *pu) != ~0U)
            {
                unsigned iBit = 0;
                unsigned uMask = 1;
                while (iBit < sizeof(pSub->auBitmap[0]) * 8)
                {
                    if (!(u & uMask))
                    {
                        *pu |= uMask;
                        return (uint8_t *)pSub->pvPages
                            + PAGE_SIZE * (iBit + ((uint8_t *)pu - (uint8_t *)&pSub->auBitmap[0]) * 8);
                    }
                    iBit++;
                    uMask <<= 1;
                }
                STAM_COUNTER_INC(&pPool->cErrors);
                AssertMsgFailed(("how odd, expected to find a free bit in %#x, but didn't\n", u));
            }
            /* next */
            pu++;
        }
#endif
        STAM_COUNTER_INC(&pPool->cErrors);
#ifdef VBOX_WITH_STATISTICS
        pPool->cFreePages++;
#endif
        AssertMsgFailed(("how strange, expected to find a free bit in %p, but didn't (%d pages supposed to be free!)\n", pSub, pSub->cPagesFree + 1));
    }

    /*
     * Allocate new subpool.
     */
    unsigned        cPages = !pPool->fLow ? 128 : 32;
    PMMPAGESUBPOOL  pSub;
    int rc = MMHyperAlloc(pPool->pVM,
                          RT_OFFSETOF(MMPAGESUBPOOL, auBitmap[cPages / (sizeof(pSub->auBitmap[0] * 8))])
                          + (sizeof(SUPPAGE) + sizeof(MMPPLOOKUPHCPHYS)) * cPages
                          + sizeof(MMPPLOOKUPHCPTR),
                          0,
                          MM_TAG_MM_PAGE,
                          (void **)&pSub);
    if (VBOX_FAILURE(rc))
        return NULL;

    PSUPPAGE paPhysPages = (PSUPPAGE)&pSub->auBitmap[cPages / (sizeof(pSub->auBitmap[0]) * 8)];
    Assert((uintptr_t)paPhysPages >= (uintptr_t)&pSub->auBitmap[1]);
    if (!pPool->fLow)
    {
        /*
         * Allocate and lock the pages.
         */
        rc = SUPPageAlloc(cPages, &pSub->pvPages);
        if (VBOX_SUCCESS(rc))
        {
            rc = SUPPageLock(pSub->pvPages, cPages, paPhysPages);
            if (VBOX_FAILURE(rc))
            {
                SUPPageFree(pSub->pvPages, cPages);
                rc = VMSetError(pPool->pVM, rc, RT_SRC_POS,
                                N_("Failed to lock host %zd bytes of memory (out of memory)"), (size_t)cPages << PAGE_SHIFT);
            }
        }
    }
    else
        rc = SUPLowAlloc(cPages, &pSub->pvPages, NULL, paPhysPages);
    if (VBOX_SUCCESS(rc))
    {
        /*
         * Setup the sub structure and allocate the requested page.
         */
        pSub->cPages    = cPages;
        pSub->cPagesFree= cPages - 1;
        pSub->paPhysPages = paPhysPages;
        memset(pSub->auBitmap, 0, cPages / 8);
        /* allocate first page. */
        pSub->auBitmap[0] |= 1;
        /* link into free chain. */
        pSub->pNextFree = pPool->pHeadFree;
        pPool->pHeadFree= pSub;
        /* link into main chain. */
        pSub->pNext     = pPool->pHead;
        pPool->pHead    = pSub;
        /* update pool statistics. */
        pPool->cSubPools++;
        pPool->cPages  += cPages;
#ifdef VBOX_WITH_STATISTICS
        pPool->cFreePages += cPages - 1;
#endif

        /*
         * Initialize the physical pages with backpointer to subpool.
         */
        unsigned i = cPages;
        while (i-- > 0)
        {
            AssertMsg(paPhysPages[i].Phys && !(paPhysPages[i].Phys & PAGE_OFFSET_MASK),
                      ("i=%d Phys=%d\n", i, paPhysPages[i].Phys));
            paPhysPages[i].uReserved = (RTHCUINTPTR)pSub;
        }

        /*
         * Initialize the physical lookup record with backpointers to the physical pages.
         */
        PMMPPLOOKUPHCPHYS paLookupPhys = (PMMPPLOOKUPHCPHYS)&paPhysPages[cPages];
        i = cPages;
        while (i-- > 0)
        {
            paLookupPhys[i].pPhysPage = &paPhysPages[i];
            paLookupPhys[i].Core.Key = paPhysPages[i].Phys;
            RTAvlHCPhysInsert(&pPool->pLookupPhys, &paLookupPhys[i].Core);
        }

        /*
         * And the one record for virtual memory lookup.
         */
        PMMPPLOOKUPHCPTR   pLookupVirt = (PMMPPLOOKUPHCPTR)&paLookupPhys[cPages];
        pLookupVirt->pSubPool = pSub;
        pLookupVirt->Core.Key = pSub->pvPages;
        RTAvlPVInsert(&pPool->pLookupVirt, &pLookupVirt->Core);

        /* return allocated page (first). */
        return pSub->pvPages;
    }

    MMR3HeapFree(pSub);
    STAM_COUNTER_INC(&pPool->cErrors);
    if (pPool->fLow)
        VMSetError(pPool->pVM, rc, RT_SRC_POS,
                   N_("Failed to expand page pool for memory below 4GB. current size: %d pages"),
                   pPool->cPages);
    AssertMsgFailed(("Failed to expand pool%s. rc=%Vrc poolsize=%d\n",
                     pPool->fLow ? " (<4GB)" : "", rc, pPool->cPages));
    return NULL;
}


/**
 * Frees a page from the page pool.
 *
 * @param   pPool   Pointer to the page pool.
 * @param   pv      Pointer to the page to free.
 *                  I.e. pointer returned by mmR3PagePoolAlloc().
 * @thread  The Emulation Thread.
 */
DECLINLINE(void) mmR3PagePoolFree(PMMPAGEPOOL pPool, void *pv)
{
    VM_ASSERT_EMT(pPool->pVM);
    STAM_COUNTER_INC(&pPool->cFreeCalls);

    /*
     * Lookup the virtual address.
     */
    PMMPPLOOKUPHCPTR pLookup = (PMMPPLOOKUPHCPTR)RTAvlPVGetBestFit(&pPool->pLookupVirt, pv, false);
    if (    !pLookup
        ||  (uint8_t *)pv >= (uint8_t *)pLookup->pSubPool->pvPages + (pLookup->pSubPool->cPages << PAGE_SHIFT)
        )
    {
        STAM_COUNTER_INC(&pPool->cErrors);
        AssertMsgFailed(("invalid pointer %p\n", pv));
        return;
    }

    /*
     * Free the page.
     */
    PMMPAGESUBPOOL  pSubPool = pLookup->pSubPool;
    /* clear bitmap bit */
    const unsigned  iPage = ((uint8_t *)pv - (uint8_t *)pSubPool->pvPages) >> PAGE_SHIFT;
#ifdef USE_INLINE_ASM_BIT_OPS
    Assert(ASMBitTest(pSubPool->auBitmap, iPage));
    ASMBitClear(pSubPool->auBitmap, iPage);
#else
    unsigned    iBit   = iPage % (sizeof(pSubPool->auBitmap[0]) * 8);
    unsigned    iIndex = iPage / (sizeof(pSubPool->auBitmap[0]) * 8);
    pSubPool->auBitmap[iIndex] &= ~(1 << iBit);
#endif
    /* update stats. */
    pSubPool->cPagesFree++;
#ifdef VBOX_WITH_STATISTICS
    pPool->cFreePages++;
#endif
    if (pSubPool->cPagesFree == 1)
    {
        pSubPool->pNextFree = pPool->pHeadFree;
        pPool->pHeadFree = pSubPool;
    }
}


/**
 * Allocates a page from the page pool.
 *
 * This function may returns pages which has physical addresses any
 * where. If you require a page to be within the first 4GB of physical
 * memory, use MMR3PageAllocLow().
 *
 * @returns Pointer to the allocated page page.
 * @returns NULL on failure.
 * @param   pVM         VM handle.
 * @thread  The Emulation Thread.
 */
MMR3DECL(void *) MMR3PageAlloc(PVM pVM)
{
    return mmR3PagePoolAlloc(pVM->mm.s.pPagePool);
}


/**
 * Allocates a page from the page pool and return its physical address.
 *
 * This function may returns pages which has physical addresses any
 * where. If you require a page to be within the first 4GB of physical
 * memory, use MMR3PageAllocLow().
 *
 * @returns Pointer to the allocated page page.
 * @returns NIL_RTHCPHYS on failure.
 * @param   pVM         VM handle.
 * @thread  The Emulation Thread.
 */
MMR3DECL(RTHCPHYS) MMR3PageAllocPhys(PVM pVM)
{
    /** @todo optimize this, it's the most common case now. */
    void *pv = mmR3PagePoolAlloc(pVM->mm.s.pPagePool);
    if (pv)
        return mmPagePoolPtr2Phys(pVM->mm.s.pPagePool, pv);
    return NIL_RTHCPHYS;
}


/**
 * Frees a page allocated from the page pool by MMR3PageAlloc() or
 * MMR3PageAllocPhys().
 *
 * @param   pVM         VM handle.
 * @param   pvPage      Pointer to the page.
 * @thread  The Emulation Thread.
 */
MMR3DECL(void) MMR3PageFree(PVM pVM, void *pvPage)
{
    mmR3PagePoolFree(pVM->mm.s.pPagePool, pvPage);
}


/**
 * Allocates a page from the low page pool.
 *
 * @returns Pointer to the allocated page.
 * @returns NULL on failure.
 * @param   pVM         VM handle.
 * @thread  The Emulation Thread.
 */
MMR3DECL(void *) MMR3PageAllocLow(PVM pVM)
{
    return mmR3PagePoolAlloc(pVM->mm.s.pPagePoolLow);
}


/**
 * Frees a page allocated from the page pool by MMR3PageAllocLow().
 *
 * @param   pVM         VM handle.
 * @param   pvPage      Pointer to the page.
 * @thread  The Emulation Thread.
 */
MMR3DECL(void) MMR3PageFreeLow(PVM pVM, void *pvPage)
{
    mmR3PagePoolFree(pVM->mm.s.pPagePoolLow, pvPage);
}


/**
 * Free a page allocated from the page pool by physical address.
 * This works for pages allocated by MMR3PageAlloc(), MMR3PageAllocPhys()
 * and MMR3PageAllocLow().
 *
 * @param   pVM         VM handle.
 * @param   HCPhysPage  The physical address of the page to be freed.
 * @thread  The Emulation Thread.
 */
MMR3DECL(void) MMR3PageFreeByPhys(PVM pVM, RTHCPHYS HCPhysPage)
{
    void *pvPage = mmPagePoolPhys2Ptr(pVM->mm.s.pPagePool, HCPhysPage);
    if (!pvPage)
        pvPage = mmPagePoolPhys2Ptr(pVM->mm.s.pPagePoolLow, HCPhysPage);
    if (pvPage)
        mmR3PagePoolFree(pVM->mm.s.pPagePool, pvPage);
    else
        AssertMsgFailed(("Invalid address HCPhysPT=%#x\n", HCPhysPage));
}


/**
 * Gets the HC pointer to the dummy page.
 *
 * The dummy page is used as a place holder to prevent potential bugs
 * from doing really bad things to the system.
 *
 * @returns Pointer to the dummy page.
 * @param   pVM         VM handle.
 * @thread  The Emulation Thread.
 */
MMR3DECL(void *) MMR3PageDummyHCPtr(PVM pVM)
{
    VM_ASSERT_EMT(pVM);
    if (!pVM->mm.s.pvDummyPage)
    {
        pVM->mm.s.pvDummyPage = mmR3PagePoolAlloc(pVM->mm.s.pPagePool);
        AssertRelease(pVM->mm.s.pvDummyPage);
        pVM->mm.s.HCPhysDummyPage = mmPagePoolPtr2Phys(pVM->mm.s.pPagePool, pVM->mm.s.pvDummyPage);
        AssertRelease(!(pVM->mm.s.HCPhysDummyPage & ~X86_PTE_PAE_PG_MASK));
    }
    return pVM->mm.s.pvDummyPage;
}


/**
 * Gets the HC Phys to the dummy page.
 *
 * The dummy page is used as a place holder to prevent potential bugs
 * from doing really bad things to the system.
 *
 * @returns Pointer to the dummy page.
 * @param   pVM         VM handle.
 * @thread  The Emulation Thread.
 */
MMR3DECL(RTHCPHYS) MMR3PageDummyHCPhys(PVM pVM)
{
    VM_ASSERT_EMT(pVM);
    if (!pVM->mm.s.pvDummyPage)
        MMR3PageDummyHCPtr(pVM);
    return pVM->mm.s.HCPhysDummyPage;
}


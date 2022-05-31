/* $Id: PGMShw.h 20374 2009-06-08 00:43:21Z vboxsync $ */
/** @file
 * VBox - Page Manager / Monitor, Shadow Paging Template.
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
*   Defined Constants And Macros                                               *
*******************************************************************************/
#undef SHWPT
#undef PSHWPT
#undef SHWPTE
#undef PSHWPTE
#undef SHWPD
#undef PSHWPD
#undef SHWPDE
#undef PSHWPDE
#undef SHW_PDE_PG_MASK
#undef SHW_PD_SHIFT
#undef SHW_PD_MASK
#undef SHW_PTE_PG_MASK
#undef SHW_PT_SHIFT
#undef SHW_PT_MASK
#undef SHW_TOTAL_PD_ENTRIES
#undef SHW_PDPT_SHIFT
#undef SHW_PDPT_MASK
#undef SHW_PDPE_PG_MASK
#undef SHW_POOL_ROOT_IDX

#if PGM_SHW_TYPE == PGM_TYPE_32BIT
# define SHWPT                  X86PT
# define PSHWPT                 PX86PT
# define SHWPTE                 X86PTE
# define PSHWPTE                PX86PTE
# define SHWPD                  X86PD
# define PSHWPD                 PX86PD
# define SHWPDE                 X86PDE
# define PSHWPDE                PX86PDE
# define SHW_PDE_PG_MASK        X86_PDE_PG_MASK
# define SHW_PD_SHIFT           X86_PD_SHIFT
# define SHW_PD_MASK            X86_PD_MASK
# define SHW_TOTAL_PD_ENTRIES   X86_PG_ENTRIES
# define SHW_PTE_PG_MASK        X86_PTE_PG_MASK
# define SHW_PT_SHIFT           X86_PT_SHIFT
# define SHW_PT_MASK            X86_PT_MASK
# define SHW_POOL_ROOT_IDX      PGMPOOL_IDX_PD

#elif PGM_SHW_TYPE == PGM_TYPE_EPT
# define SHWPT                  EPTPT
# define PSHWPT                 PEPTPT
# define SHWPTE                 EPTPTE
# define PSHWPTE                PEPTPTE
# define SHWPD                  EPTPD
# define PSHWPD                 PEPTPD
# define SHWPDE                 EPTPDE
# define PSHWPDE                PEPTPDE
# define SHW_PDE_PG_MASK        EPT_PDE_PG_MASK
# define SHW_PD_SHIFT           EPT_PD_SHIFT
# define SHW_PD_MASK            EPT_PD_MASK
# define SHW_PTE_PG_MASK        EPT_PTE_PG_MASK
# define SHW_PT_SHIFT           EPT_PT_SHIFT
# define SHW_PT_MASK            EPT_PT_MASK
# define SHW_PDPT_SHIFT         EPT_PDPT_SHIFT
# define SHW_PDPT_MASK          EPT_PDPT_MASK
# define SHW_PDPE_PG_MASK       EPT_PDPE_PG_MASK
# define SHW_TOTAL_PD_ENTRIES   (EPT_PG_AMD64_ENTRIES*EPT_PG_AMD64_PDPE_ENTRIES)
# define SHW_POOL_ROOT_IDX      PGMPOOL_IDX_NESTED_ROOT      /* do not use! exception is real mode & protected mode without paging. */

#else
# define SHWPT                  X86PTPAE
# define PSHWPT                 PX86PTPAE
# define SHWPTE                 X86PTEPAE
# define PSHWPTE                PX86PTEPAE
# define SHWPD                  X86PDPAE
# define PSHWPD                 PX86PDPAE
# define SHWPDE                 X86PDEPAE
# define PSHWPDE                PX86PDEPAE
# define SHW_PDE_PG_MASK        X86_PDE_PAE_PG_MASK
# define SHW_PD_SHIFT           X86_PD_PAE_SHIFT
# define SHW_PD_MASK            X86_PD_PAE_MASK
# define SHW_PTE_PG_MASK        X86_PTE_PAE_PG_MASK
# define SHW_PT_SHIFT           X86_PT_PAE_SHIFT
# define SHW_PT_MASK            X86_PT_PAE_MASK

# if PGM_SHW_TYPE == PGM_TYPE_AMD64
#  define SHW_PDPT_SHIFT        X86_PDPT_SHIFT
#  define SHW_PDPT_MASK         X86_PDPT_MASK_AMD64
#  define SHW_PDPE_PG_MASK      X86_PDPE_PG_MASK
#  define SHW_TOTAL_PD_ENTRIES  (X86_PG_AMD64_ENTRIES*X86_PG_AMD64_PDPE_ENTRIES)
#  define SHW_POOL_ROOT_IDX     PGMPOOL_IDX_AMD64_CR3

# else /* 32 bits PAE mode */
#  define SHW_PDPT_SHIFT        X86_PDPT_SHIFT
#  define SHW_PDPT_MASK         X86_PDPT_MASK_PAE
#  define SHW_PDPE_PG_MASK      X86_PDPE_PG_MASK
#  define SHW_TOTAL_PD_ENTRIES  (X86_PG_PAE_ENTRIES*X86_PG_PAE_PDPE_ENTRIES)
#  define SHW_POOL_ROOT_IDX     PGMPOOL_IDX_PDPT
# endif
#endif


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
RT_C_DECLS_BEGIN
/* r3 */
PGM_SHW_DECL(int, InitData)(PVM pVM, PPGMMODEDATA pModeData, bool fResolveGCAndR0);
PGM_SHW_DECL(int, Enter)(PVMCPU pVCpu);
PGM_SHW_DECL(int, Relocate)(PVMCPU pVCpu, RTGCPTR offDelta);
PGM_SHW_DECL(int, Exit)(PVMCPU pVCpu);

/* all */
PGM_SHW_DECL(int, GetPage)(PVMCPU pVCpu, RTGCPTR GCPtr, uint64_t *pfFlags, PRTHCPHYS pHCPhys);
PGM_SHW_DECL(int, ModifyPage)(PVMCPU pVCpu, RTGCPTR GCPtr, size_t cb, uint64_t fFlags, uint64_t fMask);
RT_C_DECLS_END


/**
 * Initializes the guest bit of the paging mode data.
 *
 * @returns VBox status code.
 * @param   pVM             The VM handle.
 * @param   fResolveGCAndR0 Indicate whether or not GC and Ring-0 symbols can be resolved now.
 *                          This is used early in the init process to avoid trouble with PDM
 *                          not being initialized yet.
 */
PGM_SHW_DECL(int, InitData)(PVM pVM, PPGMMODEDATA pModeData, bool fResolveGCAndR0)
{
    Assert(pModeData->uShwType == PGM_SHW_TYPE || pModeData->uShwType == PGM_TYPE_NESTED);

    /* Ring-3 */
    pModeData->pfnR3ShwRelocate          = PGM_SHW_NAME(Relocate);
    pModeData->pfnR3ShwExit              = PGM_SHW_NAME(Exit);
    pModeData->pfnR3ShwGetPage           = PGM_SHW_NAME(GetPage);
    pModeData->pfnR3ShwModifyPage        = PGM_SHW_NAME(ModifyPage);

    if (fResolveGCAndR0)
    {
        int rc;

#if PGM_SHW_TYPE != PGM_TYPE_AMD64 && PGM_SHW_TYPE != PGM_TYPE_NESTED && PGM_SHW_TYPE != PGM_TYPE_EPT /* No AMD64 for traditional virtualization, only VT-x and AMD-V. */
        /* GC */
        rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_SHW_NAME_RC_STR(GetPage),    &pModeData->pfnRCShwGetPage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_SHW_NAME_RC_STR(GetPage),  rc), rc);
        rc = PDMR3LdrGetSymbolRC(pVM, NULL,       PGM_SHW_NAME_RC_STR(ModifyPage), &pModeData->pfnRCShwModifyPage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_SHW_NAME_RC_STR(ModifyPage), rc), rc);
#endif /* Not AMD64 shadow paging. */

        /* Ring-0 */
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_SHW_NAME_R0_STR(GetPage),    &pModeData->pfnR0ShwGetPage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_SHW_NAME_R0_STR(GetPage),  rc), rc);
        rc = PDMR3LdrGetSymbolR0(pVM, NULL,       PGM_SHW_NAME_R0_STR(ModifyPage), &pModeData->pfnR0ShwModifyPage);
        AssertMsgRCReturn(rc, ("%s -> rc=%Rrc\n", PGM_SHW_NAME_R0_STR(ModifyPage), rc), rc);
    }
    return VINF_SUCCESS;
}

/**
 * Enters the shadow mode.
 *
 * @returns VBox status code.
 * @param   pVCpu       The VMCPU to operate on.
 */
PGM_SHW_DECL(int, Enter)(PVMCPU pVCpu)
{
#if PGM_SHW_TYPE == PGM_TYPE_NESTED || PGM_SHW_TYPE == PGM_TYPE_EPT
    RTGCPHYS     GCPhysCR3 = RT_BIT_64(63);
    PPGMPOOLPAGE pNewShwPageCR3;
    PVM          pVM       = pVCpu->pVMR3;
    PPGMPOOL     pPool     = pVM->pgm.s.CTX_SUFF(pPool);

    Assert(HWACCMIsNestedPagingActive(pVM));
    Assert(!pVCpu->pgm.s.pShwPageCR3R3);

    int rc = pgmPoolAlloc(pVM, GCPhysCR3, PGMPOOLKIND_ROOT_NESTED, PGMPOOL_IDX_NESTED_ROOT, GCPhysCR3 >> PAGE_SHIFT, &pNewShwPageCR3);
    AssertFatalRC(rc);

    /* Mark the page as locked; disallow flushing. */
    pgmPoolLockPage(pPool, pNewShwPageCR3);

    pVCpu->pgm.s.iShwUser      = PGMPOOL_IDX_NESTED_ROOT;
    pVCpu->pgm.s.iShwUserTable = GCPhysCR3 >> PAGE_SHIFT;
    pVCpu->pgm.s.pShwPageCR3R3 = pNewShwPageCR3;

    pVCpu->pgm.s.pShwPageCR3RC = MMHyperCCToRC(pVM, pVCpu->pgm.s.pShwPageCR3R3);
    pVCpu->pgm.s.pShwPageCR3R0 = MMHyperCCToR0(pVM, pVCpu->pgm.s.pShwPageCR3R3);

    Log(("Enter nested shadow paging mode: root %RHv phys %RHp\n", pVCpu->pgm.s.pShwPageCR3R3, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3)->Core.Key));
#endif
    return VINF_SUCCESS;
}


/**
 * Relocate any GC pointers related to shadow mode paging.
 *
 * @returns VBox status code.
 * @param   pVCpu       The VMCPU to operate on.
 * @param   offDelta    The reloation offset.
 */
PGM_SHW_DECL(int, Relocate)(PVMCPU pVCpu, RTGCPTR offDelta)
{
    pVCpu->pgm.s.pShwPageCR3RC += offDelta;
    return VINF_SUCCESS;
}


/**
 * Exits the shadow mode.
 *
 * @returns VBox status code.
 * @param   pVCpu       The VMCPU to operate on.
 */
PGM_SHW_DECL(int, Exit)(PVMCPU pVCpu)
{
    PVM pVM = pVCpu->pVMR3;

    if (    (   pVCpu->pgm.s.enmShadowMode == PGMMODE_NESTED
             || pVCpu->pgm.s.enmShadowMode == PGMMODE_EPT)
        &&  pVCpu->pgm.s.CTX_SUFF(pShwPageCR3))
    {
        PPGMPOOL pPool = pVM->pgm.s.CTX_SUFF(pPool);

        Assert(pVCpu->pgm.s.iShwUser == PGMPOOL_IDX_NESTED_ROOT);

        /* Mark the page as unlocked; allow flushing again. */
        pgmPoolUnlockPage(pPool, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3));

        pgmPoolFreeByPage(pPool, pVCpu->pgm.s.CTX_SUFF(pShwPageCR3), pVCpu->pgm.s.iShwUser, pVCpu->pgm.s.iShwUserTable);
        pVCpu->pgm.s.pShwPageCR3R3 = 0;
        pVCpu->pgm.s.pShwPageCR3R0 = 0;
        pVCpu->pgm.s.pShwPageCR3RC = 0;
        pVCpu->pgm.s.iShwUser      = 0;
        pVCpu->pgm.s.iShwUserTable = 0;

        Log(("Leave nested shadow paging mode\n"));
    }
    return VINF_SUCCESS;
}




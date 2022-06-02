/* $Id: PGMGstDefs.h 31080 2010-07-24 17:25:32Z vboxsync $ */
/** @file
 * VBox - Page Manager, Guest Paging Template - All context code.
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


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#undef GSTPT
#undef PGSTPT
#undef GSTPTE
#undef PGSTPTE
#undef GSTPD
#undef PGSTPD
#undef GSTPDE
#undef PGSTPDE
#undef GSTPTWALK
#undef PGSTPTWALK
#undef PCGSTPTWALK
#undef GST_BIG_PAGE_SIZE
#undef GST_BIG_PAGE_OFFSET_MASK
#undef GST_PDE_PG_MASK
#undef GST_PDE_BIG_PG_MASK
#undef GST_PD_SHIFT
#undef GST_PD_MASK
#undef GST_PTE_PG_MASK
#undef GST_PT_SHIFT
#undef GST_PT_MASK
#undef GST_TOTAL_PD_ENTRIES
#undef GST_CR3_PAGE_MASK
#undef GST_PDPE_ENTRIES
#undef GST_PDPT_SHIFT
#undef GST_PDPT_MASK
#undef GST_PDPE_PG_MASK
#undef GST_GET_PDE_BIG_PG_GCPHYS
#undef GST_IS_PTE_VALID
#undef GST_IS_PDE_VALID
#undef GST_IS_BIG_PDE_VALID
#undef GST_IS_PDPE_VALID
#undef GST_IS_BIG_PDPE_VALID
#undef GST_IS_PML4E_VALID
#undef GST_IS_PSE_ACTIVE
#undef GST_IS_NX_ACTIVE
#undef BTH_IS_NP_ACTIVE

#if PGM_GST_TYPE == PGM_TYPE_REAL \
 || PGM_GST_TYPE == PGM_TYPE_PROT

# if PGM_SHW_TYPE == PGM_TYPE_EPT
#  define GSTPT                                 X86PTPAE
#  define PGSTPT                                PX86PTPAE
#  define GSTPTE                                X86PTEPAE
#  define PGSTPTE                               PX86PTEPAE
#  define GSTPD                                 X86PDPAE
#  define PGSTPD                                PX86PDPAE
#  define GSTPDE                                X86PDEPAE
#  define PGSTPDE                               PX86PDEPAE
#  define GST_PTE_PG_MASK                       X86_PTE_PAE_PG_MASK
#  define GST_IS_NX_ACTIVE(pVCpu)               (true && This_should_perhaps_not_be_used_in_this_context)
#  define BTH_IS_NP_ACTIVE(pVM)                 (true)
# else
#  define GSTPT                                 SHWPT
#  define PGSTPT                                PSHWPT
#  define GSTPTE                                SHWPTE
#  define PGSTPTE                               PSHWPTE
#  define GSTPD                                 SHWPD
#  define PGSTPD                                PSHWPD
#  define GSTPDE                                SHWPDE
#  define PGSTPDE                               PSHWPDE
#  define GST_PTE_PG_MASK                       SHW_PTE_PG_MASK
#  define GST_IS_NX_ACTIVE(pVCpu)               (pgmGstIsNoExecuteActive(pVCpu))
#  if PGM_GST_TYPE == PGM_TYPE_PROT             /* (comment at top of PGMAllBth.h) */
#   define BTH_IS_NP_ACTIVE(pVM)                (pVM->pgm.s.fNestedPaging)
#  else
#   define BTH_IS_NP_ACTIVE(pVM)                (false)
#  endif
# endif
# define GST_IS_PTE_VALID(pVCpu, Pte)           (true)
# define GST_IS_PDE_VALID(pVCpu, Pde)           (true)
# define GST_IS_BIG_PDE_VALID(pVCpu, Pde)       (true)
# define GST_IS_PDPE_VALID(pVCpu, Pdpe)         (true)
# define GST_IS_BIG_PDPE_VALID(pVCpu, Pdpe)     (true)
# define GST_IS_PML4E_VALID(pVCpu, Pml4e)       (true)
# define GST_IS_PSE_ACTIVE(pVCpu)               (false && This_should_not_be_used_in_this_context)

#elif PGM_GST_TYPE == PGM_TYPE_32BIT
# define GSTPT                                  X86PT
# define PGSTPT                                 PX86PT
# define GSTPTE                                 X86PTE
# define PGSTPTE                                PX86PTE
# define GSTPD                                  X86PD
# define PGSTPD                                 PX86PD
# define GSTPDE                                 X86PDE
# define PGSTPDE                                PX86PDE
# define GSTPTWALK                              PGMPTWALKGST32BIT
# define PGSTPTWALK                             PPGMPTWALKGST32BIT
# define PCGSTPTWALK                            PCPGMPTWALKGST32BIT
# define GST_BIG_PAGE_SIZE                      X86_PAGE_4M_SIZE
# define GST_BIG_PAGE_OFFSET_MASK               X86_PAGE_4M_OFFSET_MASK
# define GST_PDE_PG_MASK                        X86_PDE_PG_MASK
# define GST_PDE_BIG_PG_MASK                    X86_PDE4M_PG_MASK
# define GST_GET_PDE_BIG_PG_GCPHYS(pVM, PdeGst) pgmGstGet4MBPhysPage(&(pVM)->pgm.s, PdeGst)
# define GST_PD_SHIFT                           X86_PD_SHIFT
# define GST_PD_MASK                            X86_PD_MASK
# define GST_TOTAL_PD_ENTRIES                   X86_PG_ENTRIES
# define GST_PTE_PG_MASK                        X86_PTE_PG_MASK
# define GST_PT_SHIFT                           X86_PT_SHIFT
# define GST_PT_MASK                            X86_PT_MASK
# define GST_CR3_PAGE_MASK                      X86_CR3_PAGE_MASK
# define GST_IS_PTE_VALID(pVCpu, Pte)           (true)
# define GST_IS_PDE_VALID(pVCpu, Pde)           (true)
# define GST_IS_BIG_PDE_VALID(pVCpu, Pde)       (!( (Pde).u & (pVCpu)->pgm.s.fGst32BitMbzBigPdeMask ))
//# define GST_IS_PDPE_VALID(pVCpu, Pdpe)         (false)
//# define GST_IS_BIG_PDPE_VALID(pVCpu, Pdpe)     (false)
//# define GST_IS_PML4E_VALID(pVCpu, Pml4e)       (false)
# define GST_IS_PSE_ACTIVE(pVCpu)               pgmGst32BitIsPageSizeExtActive(pVCpu)
# define GST_IS_NX_ACTIVE(pVCpu)                (false)
# define BTH_IS_NP_ACTIVE(pVM)                  (false)

#elif   PGM_GST_TYPE == PGM_TYPE_PAE \
     || PGM_GST_TYPE == PGM_TYPE_AMD64
# define GSTPT                                  X86PTPAE
# define PGSTPT                                 PX86PTPAE
# define GSTPTE                                 X86PTEPAE
# define PGSTPTE                                PX86PTEPAE
# define GSTPD                                  X86PDPAE
# define PGSTPD                                 PX86PDPAE
# define GSTPDE                                 X86PDEPAE
# define PGSTPDE                                PX86PDEPAE
# define GST_BIG_PAGE_SIZE                      X86_PAGE_2M_SIZE
# define GST_BIG_PAGE_OFFSET_MASK               X86_PAGE_2M_OFFSET_MASK
# define GST_PDE_PG_MASK                        X86_PDE_PAE_PG_MASK_FULL
# define GST_PDE_BIG_PG_MASK                    X86_PDE2M_PAE_PG_MASK
# define GST_GET_PDE_BIG_PG_GCPHYS(pVM, PdeGst) ((PdeGst).u & GST_PDE_BIG_PG_MASK)
# define GST_PD_SHIFT                           X86_PD_PAE_SHIFT
# define GST_PD_MASK                            X86_PD_PAE_MASK
# if PGM_GST_TYPE == PGM_TYPE_PAE
#  define GSTPTWALK                             PGMPTWALKGSTPAE
#  define PGSTPTWALK                            PPGMPTWALKGSTPAE
#  define PCGSTPTWALK                           PCPGMPTWALKGSTPAE
#  define GST_TOTAL_PD_ENTRIES                  (X86_PG_PAE_ENTRIES * X86_PG_PAE_PDPE_ENTRIES)
#  define GST_PDPE_ENTRIES                      X86_PG_PAE_PDPE_ENTRIES
#  define GST_PDPE_PG_MASK                      X86_PDPE_PG_MASK_FULL
#  define GST_PDPT_SHIFT                        X86_PDPT_SHIFT
#  define GST_PDPT_MASK                         X86_PDPT_MASK_PAE
#  define GST_PTE_PG_MASK                       X86_PTE_PAE_PG_MASK
#  define GST_CR3_PAGE_MASK                     X86_CR3_PAE_PAGE_MASK
#  define GST_IS_PTE_VALID(pVCpu, Pte)          (!( (Pte).u   & (pVCpu)->pgm.s.fGstPaeMbzPteMask ))
#  define GST_IS_PDE_VALID(pVCpu, Pde)          (!( (Pde).u   & (pVCpu)->pgm.s.fGstPaeMbzPdeMask ))
#  define GST_IS_BIG_PDE_VALID(pVCpu, Pde)      (!( (Pde).u   & (pVCpu)->pgm.s.fGstPaeMbzBigPdeMask ))
#  define GST_IS_PDPE_VALID(pVCpu, Pdpe)        (!( (Pdpe).u  & (pVCpu)->pgm.s.fGstPaeMbzPdpeMask ))
//# define GST_IS_BIG_PDPE_VALID(pVCpu, Pdpe)    (false)
//# define GST_IS_PML4E_VALID(pVCpu, Pml4e)      (false)
# else
#  define GSTPTWALK                             PGMPTWALKGSTAMD64
#  define PGSTPTWALK                            PPGMPTWALKGSTAMD64
#  define PCGSTPTWALK                           PCPGMPTWALKGSTAMD64
#  define GST_TOTAL_PD_ENTRIES                  (X86_PG_AMD64_ENTRIES * X86_PG_AMD64_PDPE_ENTRIES)
#  define GST_PDPE_ENTRIES                      X86_PG_AMD64_PDPE_ENTRIES
#  define GST_PDPT_SHIFT                        X86_PDPT_SHIFT
#  define GST_PDPE_PG_MASK                      X86_PDPE_PG_MASK_FULL
#  define GST_PDPT_MASK                         X86_PDPT_MASK_AMD64
#  define GST_PTE_PG_MASK                       X86_PTE_PAE_PG_MASK_FULL
#  define GST_CR3_PAGE_MASK                     X86_CR3_AMD64_PAGE_MASK
#  define GST_IS_PTE_VALID(pVCpu, Pte)          (!( (Pte).u   & (pVCpu)->pgm.s.fGstAmd64MbzPteMask ))
#  define GST_IS_PDE_VALID(pVCpu, Pde)          (!( (Pde).u   & (pVCpu)->pgm.s.fGstAmd64MbzPdeMask ))
#  define GST_IS_BIG_PDE_VALID(pVCpu, Pde)      (!( (Pde).u   & (pVCpu)->pgm.s.fGstAmd64MbzBigPdeMask ))
#  define GST_IS_PDPE_VALID(pVCpu, Pdpe)        (!( (Pdpe).u  & (pVCpu)->pgm.s.fGstAmd64MbzPdpeMask ))
#  define GST_IS_BIG_PDPE_VALID(pVCpu, Pdpe)    (!( (Pdpe).u  & (pVCpu)->pgm.s.fGstAmd64MbzBigPdpeMask ))
#  define GST_IS_PML4E_VALID(pVCpu, Pml4e)      (!( (Pml4e).u & (pVCpu)->pgm.s.fGstAmd64MbzPml4eMask ))
# endif
# define GST_PT_SHIFT                           X86_PT_PAE_SHIFT
# define GST_PT_MASK                            X86_PT_PAE_MASK
# define GST_IS_PSE_ACTIVE(pVCpu)               (true)
# define GST_IS_NX_ACTIVE(pVCpu)                (pgmGstIsNoExecuteActive(pVCpu))
# define BTH_IS_NP_ACTIVE(pVM)                  (false)
#endif



/* $Id: GIMR0Hv.cpp 51643 2014-06-18 11:06:06Z vboxsync $ */
/** @file
 * Guest Interface Manager (GIM), Hyper-V - Host Context Ring-0.
 */

/*
 * Copyright (C) 2014 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_GIM
#include "GIMInternal.h"
#include "GIMHvInternal.h"

#include <VBox/err.h>
#include <VBox/vmm/gim.h>
#include <VBox/vmm/vm.h>

#include <iprt/spinlock.h>


#if 0
/**
 * Allocates and maps one physically contiguous page. The allocated page is
 * zero'd out.
 *
 * @returns IPRT status code.
 * @param   pMemObj         Pointer to the ring-0 memory object.
 * @param   ppVirt          Where to store the virtual address of the
 *                          allocation.
 * @param   pPhys           Where to store the physical address of the
 *                          allocation.
 */
static int gimR0HvPageAllocZ(PRTR0MEMOBJ pMemObj, PRTR0PTR ppVirt, PRTHCPHYS pHCPhys)
{
    AssertPtr(pMemObj);
    AssertPtr(ppVirt);
    AssertPtr(pHCPhys);

    int rc = RTR0MemObjAllocCont(pMemObj, PAGE_SIZE, false /* fExecutable */);
    if (RT_FAILURE(rc))
        return rc;
    *ppVirt  = RTR0MemObjAddress(*pMemObj);
    *pHCPhys = RTR0MemObjGetPagePhysAddr(*pMemObj, 0 /* iPage */);
    ASMMemZero32(*ppVirt, PAGE_SIZE);
    return VINF_SUCCESS;
}


/**
 * Frees and unmaps an allocated physical page.
 *
 * @param   pMemObj         Pointer to the ring-0 memory object.
 * @param   ppVirt          Where to re-initialize the virtual address of
 *                          allocation as 0.
 * @param   pHCPhys         Where to re-initialize the physical address of the
 *                          allocation as 0.
 */
static void gimR0HvPageFree(PRTR0MEMOBJ pMemObj, PRTR0PTR ppVirt, PRTHCPHYS pHCPhys)
{
    AssertPtr(pMemObj);
    AssertPtr(ppVirt);
    AssertPtr(pHCPhys);
    if (*pMemObj != NIL_RTR0MEMOBJ)
    {
        int rc = RTR0MemObjFree(*pMemObj, true /* fFreeMappings */);
        AssertRC(rc);
        *pMemObj = NIL_RTR0MEMOBJ;
        *ppVirt  = 0;
        *pHCPhys = 0;
    }
}
#endif

/**
 * Updates Hyper-V's reference TSC page.
 *
 * @returns VBox status code.
 * @param   pVM         Pointer to the VM.
 * @param   u64Offset   The computed TSC offset.
 * @thread  EMT.
 */
VMM_INT_DECL(int) GIMR0HvUpdateParavirtTsc(PVM pVM, uint64_t u64Offset)
{
    Assert(GIMIsEnabled(pVM));
    bool fHvTscEnabled = MSR_GIM_HV_REF_TSC_IS_ENABLED(pVM->gim.s.u.Hv.u64TscPageMsr);
    if (RT_UNLIKELY(!fHvTscEnabled))
        return VERR_GIM_PVTSC_NOT_ENABLED;

    PCGIMHV          pcHv     = &pVM->gim.s.u.Hv;
    PCGIMMMIO2REGION pcRegion = &pcHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    PGIMHVREFTSC     pRefTsc  = (PGIMHVREFTSC)pcRegion->CTX_SUFF(pvPage);
    Assert(pRefTsc);

    RTSpinlockAcquire(pcHv->hSpinlockR0);
    pRefTsc->i64TscOffset = u64Offset;
    if (pRefTsc->u32TscSequence < UINT32_C(0xfffffffe))
        ASMAtomicIncU32(&pRefTsc->u32TscSequence);
    else
        ASMAtomicWriteU32(&pRefTsc->u32TscSequence, 1);
    RTSpinlockRelease(pcHv->hSpinlockR0);

    Assert(pRefTsc->u32TscSequence != 0);
    Assert(pRefTsc->u32TscSequence != UINT32_C(0xffffffff));
    return VINF_SUCCESS;
}


/**
 * Does ring-0 per-VM GIM Hyper-V initialization.
 *
 * @returns VBox status code.
 * @param   pVM     Pointer to the VM.
 */
VMMR0_INT_DECL(int) GIMR0HvInitVM(PVM pVM)
{
    AssertPtr(pVM);
    Assert(GIMIsEnabled(pVM));

    PGIMHV pHv = &pVM->gim.s.u.Hv;
    Assert(pHv->hSpinlockR0 == NIL_RTSPINLOCK);

    int rc = RTSpinlockCreate(&pHv->hSpinlockR0, RTSPINLOCK_FLAGS_INTERRUPT_UNSAFE, "Hyper-V");
    return rc;
}


/**
 * Does ring-0 per-VM GIM Hyper-V termination.
 *
 * @returns VBox status code.
 * @param   pVM     Pointer to the VM.
 */
VMMR0_INT_DECL(int) GIMR0HvTermVM(PVM pVM)
{
    AssertPtr(pVM);
    Assert(GIMIsEnabled(pVM));

    PGIMHV pHv = &pVM->gim.s.u.Hv;
    RTSpinlockDestroy(pHv->hSpinlockR0);
    pHv->hSpinlockR0 = NIL_RTSPINLOCK;

    return VINF_SUCCESS;
}


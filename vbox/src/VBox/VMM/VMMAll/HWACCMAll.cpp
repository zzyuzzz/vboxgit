/* $Id: HWACCMAll.cpp 19812 2009-05-19 11:54:55Z vboxsync $ */
/** @file
 * HWACCM - All contexts.
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
#define LOG_GROUP LOG_GROUP_HWACCM
#include <VBox/hwaccm.h>
#include "HWACCMInternal.h"
#include <VBox/vm.h>
#include <VBox/x86.h>
#include <VBox/hwacc_vmx.h>
#include <VBox/hwacc_svm.h>
#include <VBox/pgm.h>
#include <VBox/pdm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <VBox/selm.h>
#include <VBox/iom.h>
#include <iprt/param.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/memobj.h>
#include <iprt/cpuset.h>

/**
 * Invalidates a guest page
 *
 * @returns VBox status code.
 * @param   pVCpu       The VMCPU to operate on.
 * @param   GCVirt      Page to invalidate
 */
VMMDECL(int) HWACCMInvalidatePage(PVMCPU pVCpu, RTGCPTR GCVirt)
{
#ifdef IN_RING0
    PVM pVM = pVCpu->CTX_SUFF(pVM);
    if (pVM->hwaccm.s.vmx.fSupported)
        return VMXR0InvalidatePage(pVM, pVCpu, GCVirt);

    Assert(pVM->hwaccm.s.svm.fSupported);
    return SVMR0InvalidatePage(pVM, pVCpu, GCVirt);
#endif

    return VINF_SUCCESS;
}

/**
 * Flushes the guest TLB
 *
 * @returns VBox status code.
 * @param   pVCpu       The VMCPU to operate on.
 */
VMMDECL(int) HWACCMFlushTLB(PVMCPU pVCpu)
{
    LogFlow(("HWACCMFlushTLB\n"));

    pVCpu->hwaccm.s.fForceTLBFlush = true;
    STAM_COUNTER_INC(&pVCpu->hwaccm.s.StatFlushTLBManual);
    return VINF_SUCCESS;
}

#ifndef IN_RC
/**
 * Flush the TLBs of all VCPUs
 *
 * @returns VBox status code.
 * @param   pVM       The VM to operate on.
 */
VMMDECL(int) HWACCMFlushAllTLBs(PVM pVM)
{
    if (pVM->cCPUs == 1)
        return HWACCMFlushTLB(&pVM->aCpus[0]);

    VMCPUID idThisCpu = VMMGetCpuId(pVM);

    for (unsigned idCpu = 0; idCpu < pVM->cCPUs; idCpu++)
    {
        PVMCPU pVCpu = &pVM->aCpus[idCpu];

        VMCPU_FF_SET(pVCpu, VMCPU_FF_TLB_FLUSH);
        if (idThisCpu == idCpu)
            continue;

        if (VMCPU_GET_STATE(pVCpu) == VMCPUSTATE_STARTED_EXEC)
        {
            STAM_COUNTER_INC(&pVCpu->hwaccm.s.StatTlbShootdownFlush);
#ifdef IN_RING0
            RTMpPokeCpu(idCpu);
#else
            VMR3NotifyCpuFFU(pVCpu->pUVCpu, VMNOTIFYFF_FLAGS_POKE);
#endif
        }
        else
            STAM_COUNTER_INC(&pVCpu->hwaccm.s.StatFlushTLBManual);
    }
    return VINF_SUCCESS;
}
#endif

/**
 * Checks if nested paging is enabled
 *
 * @returns boolean
 * @param   pVM         The VM to operate on.
 */
VMMDECL(bool) HWACCMIsNestedPagingActive(PVM pVM)
{
    return HWACCMIsEnabled(pVM) && pVM->hwaccm.s.fNestedPaging;
}

/**
 * Return the shadow paging mode for nested paging/ept
 *
 * @returns shadow paging mode
 * @param   pVM         The VM to operate on.
 */
VMMDECL(PGMMODE) HWACCMGetShwPagingMode(PVM pVM)
{
    Assert(HWACCMIsNestedPagingActive(pVM));
    if (pVM->hwaccm.s.svm.fSupported)
        return PGMMODE_NESTED;

    Assert(pVM->hwaccm.s.vmx.fSupported);
    return PGMMODE_EPT;
}

/**
 * Invalidates a guest page by physical address
 *
 * NOTE: Assumes the current instruction references this physical page though a virtual address!!
 *
 * @returns VBox status code.
 * @param   pVCpu       The VMCPU to operate on.
 * @param   GCPhys      Page to invalidate
 */
VMMDECL(int) HWACCMInvalidatePhysPage(PVMCPU pVCpu, RTGCPHYS GCPhys)
{
    PVM pVM = pVCpu->CTX_SUFF(pVM);

    if (!HWACCMIsNestedPagingActive(pVM))
        return VINF_SUCCESS;

#ifdef IN_RING0
    if (pVM->hwaccm.s.vmx.fSupported)
        return VMXR0InvalidatePhysPage(pVM, pVCpu, GCPhys);

    Assert(pVM->hwaccm.s.svm.fSupported);
    SVMR0InvalidatePhysPage(pVM, pVCpu, GCPhys);
#else
    HWACCMFlushTLB(pVCpu);
#endif
    return VINF_SUCCESS;
}

/**
 * Checks if an interrupt event is currently pending.
 *
 * @returns Interrupt event pending state.
 * @param   pVM         The VM to operate on.
 */
VMMDECL(bool) HWACCMHasPendingIrq(PVM pVM)
{
    PVMCPU pVCpu = VMMGetCpu(pVM);
    return !!pVCpu->hwaccm.s.Event.fPending;
}


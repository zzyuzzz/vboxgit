/* $Id: HMSVMR0.h 93963 2022-02-28 08:39:08Z vboxsync $ */
/** @file
 * HM SVM (AMD-V) - Internal header file.
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VMM_INCLUDED_SRC_VMMR0_HMSVMR0_h
#define VMM_INCLUDED_SRC_VMMR0_HMSVMR0_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/hm_svm.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_svm_int  Internal
 * @ingroup grp_svm
 * @internal
 * @{
 */

#ifdef IN_RING0

VMMR0DECL(int)          SVMR0GlobalInit(void);
VMMR0DECL(void)         SVMR0GlobalTerm(void);
VMMR0DECL(int)          SVMR0Enter(PVMCPUCC pVCpu);
VMMR0DECL(void)         SVMR0ThreadCtxCallback(RTTHREADCTXEVENT enmEvent, PVMCPUCC pVCpu, bool fGlobalInit);
VMMR0DECL(int)          SVMR0AssertionCallback(PVMCPUCC pVCpu);
VMMR0DECL(int)          SVMR0EnableCpu(PHMPHYSCPU pHostCpu, PVMCC pVM, void *pvPageCpu, RTHCPHYS HCPhysCpuPage,
                                       bool fEnabledBySystem, PCSUPHWVIRTMSRS pHwvirtMsrs);
VMMR0DECL(int)          SVMR0DisableCpu(PHMPHYSCPU pHostCpu, void *pvPageCpu, RTHCPHYS pPageCpuPhys);
VMMR0DECL(int)          SVMR0InitVM(PVMCC pVM);
VMMR0DECL(int)          SVMR0TermVM(PVMCC pVM);
VMMR0DECL(int)          SVMR0SetupVM(PVMCC pVM);
VMMR0DECL(VBOXSTRICTRC) SVMR0RunGuestCode(PVMCPUCC pVCpu);
VMMR0DECL(int)          SVMR0ExportHostState(PVMCPUCC pVCpu);
VMMR0DECL(int)          SVMR0ImportStateOnDemand(PVMCPUCC pVCpu, uint64_t fWhat);
VMMR0DECL(int)          SVMR0InvalidatePage(PVMCPUCC pVCpu, RTGCPTR GCVirt);
VMMR0DECL(int)          SVMR0GetExitAuxInfo(PVMCPUCC pVCpu, PSVMEXITAUX pSvmExitAux);

/**
 * Executes INVLPGA.
 *
 * @param   GCVirt          Virtual page to invalidate.
 * @param   u32ASID         Tagged TLB id.
 */
DECLASM(void) SVMR0InvlpgA(RTGCPTR GCVirt, uint32_t u32ASID);

#endif /* IN_RING0 */

/** @} */

RT_C_DECLS_END

#endif /* !VMM_INCLUDED_SRC_VMMR0_HMSVMR0_h */


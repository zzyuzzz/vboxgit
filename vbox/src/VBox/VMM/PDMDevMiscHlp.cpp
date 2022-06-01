/* $Id: PDMDevMiscHlp.cpp 26160 2010-02-02 18:23:29Z vboxsync $ */
/** @file
 * PDM - Pluggable Device and Driver Manager, Misc. Device Helpers.
 */

/*
 * Copyright (C) 2006-2010 Sun Microsystems, Inc.
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
#define LOG_GROUP LOG_GROUP_PDM_DEVICE
#include "PDMInternal.h"
#include <VBox/pdm.h>
#include <VBox/rem.h>
#include <VBox/vm.h>
#include <VBox/vmm.h>

#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/thread.h>



/** @name Ring-3 PIC Helpers
 * @{
 */

/** @interface_method_impl{PDMPICHLPR3,pfnSetInterruptFF} */
static DECLCALLBACK(void) pdmR3PicHlp_SetInterruptFF(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;

    if (pVM->pdm.s.Apic.pfnLocalInterruptR3)
    {
        LogFlow(("pdmR3PicHlp_SetInterruptFF: caller='%s'/%d: Setting local interrupt on LAPIC\n",
                 pDevIns->pReg->szDeviceName, pDevIns->iInstance));
        /* Raise the LAPIC's LINT0 line instead of signaling the CPU directly. */
        pVM->pdm.s.Apic.pfnLocalInterruptR3(pVM->pdm.s.Apic.pDevInsR3, 0, 1);
        return;
    }

    PVMCPU pVCpu = &pVM->aCpus[0];  /* for PIC we always deliver to CPU 0, MP use APIC */

    LogFlow(("pdmR3PicHlp_SetInterruptFF: caller='%s'/%d: VM_FF_INTERRUPT_PIC %d -> 1\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, VMCPU_FF_ISSET(pVCpu, VMCPU_FF_INTERRUPT_PIC)));

    VMCPU_FF_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC);
    REMR3NotifyInterruptSet(pVM, pVCpu);
    VMR3NotifyCpuFFU(pVCpu->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM | VMNOTIFYFF_FLAGS_POKE);
}


/** @interface_method_impl{PDMPICHLPR3,pfnClearInterruptFF} */
static DECLCALLBACK(void) pdmR3PicHlp_ClearInterruptFF(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;
    PVMCPU pVCpu = &pVM->aCpus[0];  /* for PIC we always deliver to CPU 0, MP use APIC */

    if (pVM->pdm.s.Apic.pfnLocalInterruptR3)
    {
        /* Raise the LAPIC's LINT0 line instead of signaling the CPU directly. */
        LogFlow(("pdmR3PicHlp_ClearInterruptFF: caller='%s'/%d: Clearing local interrupt on LAPIC\n",
                 pDevIns->pReg->szDeviceName, pDevIns->iInstance));
        /* Lower the LAPIC's LINT0 line instead of signaling the CPU directly. */
        pVM->pdm.s.Apic.pfnLocalInterruptR3(pVM->pdm.s.Apic.pDevInsR3, 0, 0);
        return;
    }

    LogFlow(("pdmR3PicHlp_ClearInterruptFF: caller='%s'/%d: VM_FF_INTERRUPT_PIC %d -> 0\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, VMCPU_FF_ISSET(pVCpu, VMCPU_FF_INTERRUPT_PIC)));

    VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_PIC);
    REMR3NotifyInterruptClear(pVM, pVCpu);
}


/** @interface_method_impl{PDMPICHLPR3,pfnLock} */
static DECLCALLBACK(int) pdmR3PicHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    return pdmLockEx(pDevIns->Internal.s.pVMR3, rc);
}


/** @interface_method_impl{PDMPICHLPR3,pfnUnlock} */
static DECLCALLBACK(void) pdmR3PicHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    pdmUnlock(pDevIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMPICHLPR3,pfnGetRCHelpers} */
static DECLCALLBACK(PCPDMPICHLPRC) pdmR3PicHlp_GetRCHelpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    RTRCPTR pRCHelpers = 0;
    int rc = PDMR3LdrGetSymbolRC(pDevIns->Internal.s.pVMR3, NULL, "g_pdmRCPicHlp", &pRCHelpers);
    AssertReleaseRC(rc);
    AssertRelease(pRCHelpers);
    LogFlow(("pdmR3PicHlp_GetRCHelpers: caller='%s'/%d: returns %RRv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pRCHelpers));
    return pRCHelpers;
}


/** @interface_method_impl{PDMPICHLPR3,pfnGetR0Helpers} */
static DECLCALLBACK(PCPDMPICHLPR0) pdmR3PicHlp_GetR0Helpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    PCPDMPICHLPR0 pR0Helpers = 0;
    int rc = PDMR3LdrGetSymbolR0(pDevIns->Internal.s.pVMR3, NULL, "g_pdmR0PicHlp", &pR0Helpers);
    AssertReleaseRC(rc);
    AssertRelease(pR0Helpers);
    LogFlow(("pdmR3PicHlp_GetR0Helpers: caller='%s'/%d: returns %RHv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pR0Helpers));
    return pR0Helpers;
}


/**
 * PIC Device Helpers.
 */
const PDMPICHLPR3 g_pdmR3DevPicHlp =
{
    PDM_PICHLPR3_VERSION,
    pdmR3PicHlp_SetInterruptFF,
    pdmR3PicHlp_ClearInterruptFF,
    pdmR3PicHlp_Lock,
    pdmR3PicHlp_Unlock,
    pdmR3PicHlp_GetRCHelpers,
    pdmR3PicHlp_GetR0Helpers,
    PDM_PICHLPR3_VERSION /* the end */
};

/** @} */




/** @name R3 APIC Helpers
 * @{
 */

/** @interface_method_impl{PDMAPICHLPR3,pfnSetInterruptFF} */
static DECLCALLBACK(void) pdmR3ApicHlp_SetInterruptFF(PPDMDEVINS pDevIns, PDMAPICIRQ enmType, VMCPUID idCpu)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;
    PVMCPU pVCpu = &pVM->aCpus[idCpu];

    AssertReturnVoid(idCpu < pVM->cCpus);

    LogFlow(("pdmR3ApicHlp_SetInterruptFF: caller='%s'/%d: VM_FF_INTERRUPT(%d) %d -> 1\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, idCpu, VMCPU_FF_ISSET(pVCpu, VMCPU_FF_INTERRUPT_APIC)));

    switch (enmType)
    {
        case PDMAPICIRQ_HARDWARE:
            VMCPU_FF_SET(pVCpu, VMCPU_FF_INTERRUPT_APIC);
            break;
        case PDMAPICIRQ_NMI:
            VMCPU_FF_SET(pVCpu, VMCPU_FF_INTERRUPT_NMI);
            break;
        case PDMAPICIRQ_SMI:
            VMCPU_FF_SET(pVCpu, VMCPU_FF_INTERRUPT_SMI);
            break;
        case PDMAPICIRQ_EXTINT:
            VMCPU_FF_SET(pVCpu, VMCPU_FF_INTERRUPT_PIC);
            break;
        default:
            AssertMsgFailed(("enmType=%d\n", enmType));
            break;
    }
    REMR3NotifyInterruptSet(pVM, pVCpu);
    VMR3NotifyCpuFFU(pVCpu->pUVCpu, VMNOTIFYFF_FLAGS_DONE_REM | VMNOTIFYFF_FLAGS_POKE);
}


/** @interface_method_impl{PDMAPICHLPR3,pfnClearInterruptFF} */
static DECLCALLBACK(void) pdmR3ApicHlp_ClearInterruptFF(PPDMDEVINS pDevIns, PDMAPICIRQ enmType, VMCPUID idCpu)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;
    PVMCPU pVCpu = &pVM->aCpus[idCpu];

    AssertReturnVoid(idCpu < pVM->cCpus);

    LogFlow(("pdmR3ApicHlp_ClearInterruptFF: caller='%s'/%d: VM_FF_INTERRUPT(%d) %d -> 0\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, idCpu, VMCPU_FF_ISSET(pVCpu, VMCPU_FF_INTERRUPT_APIC)));

    /* Note: NMI/SMI can't be cleared. */
    switch (enmType)
    {
        case PDMAPICIRQ_HARDWARE:
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_APIC);
            break;
        case PDMAPICIRQ_EXTINT:
            VMCPU_FF_CLEAR(pVCpu, VMCPU_FF_INTERRUPT_PIC);
            break;
        default:
            AssertMsgFailed(("enmType=%d\n", enmType));
            break;
    }
    REMR3NotifyInterruptClear(pVM, pVCpu);
}


/** @interface_method_impl{PDMAPICHLPR3,pfnChangeFeature} */
static DECLCALLBACK(void) pdmR3ApicHlp_ChangeFeature(PPDMDEVINS pDevIns, PDMAPICVERSION enmVersion)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3ApicHlp_ChangeFeature: caller='%s'/%d: version=%d\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, (int)enmVersion));
    switch (enmVersion)
    {
        case PDMAPICVERSION_NONE:
            CPUMClearGuestCpuIdFeature(pDevIns->Internal.s.pVMR3, CPUMCPUIDFEATURE_APIC);
            CPUMClearGuestCpuIdFeature(pDevIns->Internal.s.pVMR3, CPUMCPUIDFEATURE_X2APIC);
            break;
        case PDMAPICVERSION_APIC:
            CPUMSetGuestCpuIdFeature(pDevIns->Internal.s.pVMR3, CPUMCPUIDFEATURE_APIC);
            CPUMClearGuestCpuIdFeature(pDevIns->Internal.s.pVMR3, CPUMCPUIDFEATURE_X2APIC);
            break;
        case PDMAPICVERSION_X2APIC:
            CPUMSetGuestCpuIdFeature(pDevIns->Internal.s.pVMR3, CPUMCPUIDFEATURE_X2APIC);
            CPUMSetGuestCpuIdFeature(pDevIns->Internal.s.pVMR3, CPUMCPUIDFEATURE_APIC);
            break;
        default:
            AssertMsgFailed(("Unknown APIC version: %d\n", (int)enmVersion));
    }
}

/** @interface_method_impl{PDMAPICHLPR3,pfnGetCpuId} */
static DECLCALLBACK(VMCPUID) pdmR3ApicHlp_GetCpuId(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    return VMMGetCpuId(pDevIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMAPICHLPR3,pfnSendSipi} */
static DECLCALLBACK(void) pdmR3ApicHlp_SendSipi(PPDMDEVINS pDevIns, VMCPUID idCpu, uint32_t uVector)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    VMMR3SendSipi(pDevIns->Internal.s.pVMR3, idCpu, uVector);
}

/** @interface_method_impl{PDMAPICHLPR3,pfnSendInitIpi} */
static DECLCALLBACK(void) pdmR3ApicHlp_SendInitIpi(PPDMDEVINS pDevIns, VMCPUID idCpu)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    VMMR3SendInitIpi(pDevIns->Internal.s.pVMR3, idCpu);
}

/** @interface_method_impl{PDMAPICHLPR3,pfnGetRCHelpers} */
static DECLCALLBACK(PCPDMAPICHLPRC) pdmR3ApicHlp_GetRCHelpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    RTRCPTR pRCHelpers = 0;
    int rc = PDMR3LdrGetSymbolRC(pDevIns->Internal.s.pVMR3, NULL, "g_pdmRCApicHlp", &pRCHelpers);
    AssertReleaseRC(rc);
    AssertRelease(pRCHelpers);
    LogFlow(("pdmR3ApicHlp_GetRCHelpers: caller='%s'/%d: returns %RRv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pRCHelpers));
    return pRCHelpers;
}


/** @interface_method_impl{PDMAPICHLPR3,pfnGetR0Helpers} */
static DECLCALLBACK(PCPDMAPICHLPR0) pdmR3ApicHlp_GetR0Helpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    PCPDMAPICHLPR0 pR0Helpers = 0;
    int rc = PDMR3LdrGetSymbolR0(pDevIns->Internal.s.pVMR3, NULL, "g_pdmR0ApicHlp", &pR0Helpers);
    AssertReleaseRC(rc);
    AssertRelease(pR0Helpers);
    LogFlow(("pdmR3ApicHlp_GetR0Helpers: caller='%s'/%d: returns %RHv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pR0Helpers));
    return pR0Helpers;
}


/** @interface_method_impl{PDMAPICHLPR3,pfnGetR3CritSect} */
static DECLCALLBACK(R3PTRTYPE(PPDMCRITSECT)) pdmR3ApicHlp_GetR3CritSect(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3ApicHlp_Lock: caller='%s'/%d\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance));
    return &pDevIns->Internal.s.pVMR3->pdm.s.CritSect;
}


/** @interface_method_impl{PDMAPICHLPR3,pfnGetRCCritSect} */
static DECLCALLBACK(RCPTRTYPE(PPDMCRITSECT)) pdmR3ApicHlp_GetRCCritSect(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;
    RTRCPTR RCPtr = MMHyperCCToRC(pVM, &pVM->pdm.s.CritSect);
    LogFlow(("pdmR3ApicHlp_GetR0CritSect: caller='%s'/%d: return %RRv\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance, RCPtr));
    return RCPtr;
}


/** @interface_method_impl{PDMAPICHLPR3,pfnGetR3CritSect} */
static DECLCALLBACK(R0PTRTYPE(PPDMCRITSECT)) pdmR3ApicHlp_GetR0CritSect(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;
    RTR0PTR R0Ptr = MMHyperCCToR0(pVM, &pVM->pdm.s.CritSect);
    LogFlow(("pdmR3ApicHlp_GetR0CritSect: caller='%s'/%d: return %RHv\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance, R0Ptr));
    return R0Ptr;
}



/**
 * APIC Device Helpers.
 */
const PDMAPICHLPR3 g_pdmR3DevApicHlp =
{
    PDM_APICHLPR3_VERSION,
    pdmR3ApicHlp_SetInterruptFF,
    pdmR3ApicHlp_ClearInterruptFF,
    pdmR3ApicHlp_ChangeFeature,
    pdmR3ApicHlp_GetCpuId,
    pdmR3ApicHlp_SendSipi,
    pdmR3ApicHlp_SendInitIpi,
    pdmR3ApicHlp_GetRCHelpers,
    pdmR3ApicHlp_GetR0Helpers,
    pdmR3ApicHlp_GetR3CritSect,
    pdmR3ApicHlp_GetRCCritSect,
    pdmR3ApicHlp_GetR0CritSect,
    PDM_APICHLPR3_VERSION /* the end */
};

/** @} */




/** @name Ring-3 I/O APIC Helpers
 * @{
 */

/** @interface_method_impl{PDMIOAPICHLPR3,pfnApicBusDeliver} */
static DECLCALLBACK(int) pdmR3IoApicHlp_ApicBusDeliver(PPDMDEVINS pDevIns, uint8_t u8Dest, uint8_t u8DestMode, uint8_t u8DeliveryMode,
                                                        uint8_t iVector, uint8_t u8Polarity, uint8_t u8TriggerMode)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    PVM pVM = pDevIns->Internal.s.pVMR3;
    LogFlow(("pdmR3IoApicHlp_ApicBusDeliver: caller='%s'/%d: u8Dest=%RX8 u8DestMode=%RX8 u8DeliveryMode=%RX8 iVector=%RX8 u8Polarity=%RX8 u8TriggerMode=%RX8\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, u8Dest, u8DestMode, u8DeliveryMode, iVector, u8Polarity, u8TriggerMode));
    if (pVM->pdm.s.Apic.pfnBusDeliverR3)
        return pVM->pdm.s.Apic.pfnBusDeliverR3(pVM->pdm.s.Apic.pDevInsR3, u8Dest, u8DestMode, u8DeliveryMode, iVector, u8Polarity, u8TriggerMode);
    return VINF_SUCCESS;
}


/** @interface_method_impl{PDMIOAPICHLPR3,pfnLock} */
static DECLCALLBACK(int) pdmR3IoApicHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3IoApicHlp_Lock: caller='%s'/%d: rc=%Rrc\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance, rc));
    return pdmLockEx(pDevIns->Internal.s.pVMR3, rc);
}


/** @interface_method_impl{PDMIOAPICHLPR3,pfnUnlock} */
static DECLCALLBACK(void) pdmR3IoApicHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3IoApicHlp_Unlock: caller='%s'/%d:\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance));
    pdmUnlock(pDevIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMIOAPICHLPR3,pfnGetRCHelpers} */
static DECLCALLBACK(PCPDMIOAPICHLPRC) pdmR3IoApicHlp_GetRCHelpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    RTRCPTR pRCHelpers = 0;
    int rc = PDMR3LdrGetSymbolRC(pDevIns->Internal.s.pVMR3, NULL, "g_pdmRCIoApicHlp", &pRCHelpers);
    AssertReleaseRC(rc);
    AssertRelease(pRCHelpers);
    LogFlow(("pdmR3IoApicHlp_GetRCHelpers: caller='%s'/%d: returns %RRv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pRCHelpers));
    return pRCHelpers;
}


/** @interface_method_impl{PDMIOAPICHLPR3,pfnGetR0Helpers} */
static DECLCALLBACK(PCPDMIOAPICHLPR0) pdmR3IoApicHlp_GetR0Helpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    PCPDMIOAPICHLPR0 pR0Helpers = 0;
    int rc = PDMR3LdrGetSymbolR0(pDevIns->Internal.s.pVMR3, NULL, "g_pdmR0IoApicHlp", &pR0Helpers);
    AssertReleaseRC(rc);
    AssertRelease(pR0Helpers);
    LogFlow(("pdmR3IoApicHlp_GetR0Helpers: caller='%s'/%d: returns %RHv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pR0Helpers));
    return pR0Helpers;
}


/**
 * I/O APIC Device Helpers.
 */
const PDMIOAPICHLPR3 g_pdmR3DevIoApicHlp =
{
    PDM_IOAPICHLPR3_VERSION,
    pdmR3IoApicHlp_ApicBusDeliver,
    pdmR3IoApicHlp_Lock,
    pdmR3IoApicHlp_Unlock,
    pdmR3IoApicHlp_GetRCHelpers,
    pdmR3IoApicHlp_GetR0Helpers,
    PDM_IOAPICHLPR3_VERSION /* the end */
};

/** @} */




/** @name Ring-3 PCI Bus Helpers
 * @{
 */

/** @interface_method_impl{PDMPCIHLPR3,pfnIsaSetIrq} */
static DECLCALLBACK(void) pdmR3PciHlp_IsaSetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    Log4(("pdmR3PciHlp_IsaSetIrq: iIrq=%d iLevel=%d\n", iIrq, iLevel));
    PDMIsaSetIrq(pDevIns->Internal.s.pVMR3, iIrq, iLevel);
}


/** @interface_method_impl{PDMPCIHLPR3,pfnIoApicSetIrq} */
static DECLCALLBACK(void) pdmR3PciHlp_IoApicSetIrq(PPDMDEVINS pDevIns, int iIrq, int iLevel)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    Log4(("pdmR3PciHlp_IsaSetIrq: iIrq=%d iLevel=%d\n", iIrq, iLevel));
    PDMIoApicSetIrq(pDevIns->Internal.s.pVMR3, iIrq, iLevel);
}


/** @interface_method_impl{PDMPCIHLPR3,pfnIsMMIO2Base} */
static DECLCALLBACK(bool) pdmR3PciHlp_IsMMIO2Base(PPDMDEVINS pDevIns, PPDMDEVINS pOwner, RTGCPHYS GCPhys)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    bool fRc = PGMR3PhysMMIO2IsBase(pDevIns->Internal.s.pVMR3, pOwner, GCPhys);
    Log4(("pdmR3PciHlp_IsMMIO2Base: pOwner=%p GCPhys=%RGp -> %RTbool\n", pOwner, GCPhys, fRc));
    return fRc;
}


/** @interface_method_impl{PDMPCIHLPR3,pfnLock} */
static DECLCALLBACK(int) pdmR3PciHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3PciHlp_Lock: caller='%s'/%d: rc=%Rrc\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance, rc));
    return pdmLockEx(pDevIns->Internal.s.pVMR3, rc);
}


/** @interface_method_impl{PDMPCIHLPR3,pfnUnlock} */
static DECLCALLBACK(void) pdmR3PciHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3PciHlp_Unlock: caller='%s'/%d:\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance));
    pdmUnlock(pDevIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMPCIHLPR3,pfnGetRCHelpers} */
static DECLCALLBACK(PCPDMPCIHLPRC) pdmR3PciHlp_GetRCHelpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    RTRCPTR pRCHelpers = 0;
    int rc = PDMR3LdrGetSymbolRC(pDevIns->Internal.s.pVMR3, NULL, "g_pdmRCPciHlp", &pRCHelpers);
    AssertReleaseRC(rc);
    AssertRelease(pRCHelpers);
    LogFlow(("pdmR3IoApicHlp_GetGCHelpers: caller='%s'/%d: returns %RRv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pRCHelpers));
    return pRCHelpers;
}


/** @interface_method_impl{PDMPCIHLPR3,pfnGetR0Helpers} */
static DECLCALLBACK(PCPDMPCIHLPR0) pdmR3PciHlp_GetR0Helpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    PCPDMPCIHLPR0 pR0Helpers = 0;
    int rc = PDMR3LdrGetSymbolR0(pDevIns->Internal.s.pVMR3, NULL, "g_pdmR0PciHlp", &pR0Helpers);
    AssertReleaseRC(rc);
    AssertRelease(pR0Helpers);
    LogFlow(("pdmR3IoApicHlp_GetR0Helpers: caller='%s'/%d: returns %RHv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pR0Helpers));
    return pR0Helpers;
}


/**
 * PCI Bus Device Helpers.
 */
const PDMPCIHLPR3 g_pdmR3DevPciHlp =
{
    PDM_PCIHLPR3_VERSION,
    pdmR3PciHlp_IsaSetIrq,
    pdmR3PciHlp_IoApicSetIrq,
    pdmR3PciHlp_IsMMIO2Base,
    pdmR3PciHlp_GetRCHelpers,
    pdmR3PciHlp_GetR0Helpers,
    pdmR3PciHlp_Lock,
    pdmR3PciHlp_Unlock,
    PDM_PCIHLPR3_VERSION, /* the end */
};

/** @} */




/** @name Ring-3 HPET Helpers
 * {@
 */

/** @interface_method_impl{PDMHPETHLPR3,pfnSetLegacyMode} */
static DECLCALLBACK(int) pdmR3HpetHlp_SetLegacyMode(PPDMDEVINS pDevIns, bool fActivate)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3HpetHlp_SetLegacyMode: caller='%s'/%d: fActivate=%d\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance, fActivate));
    return 0;
}


/** @interface_method_impl{PDMHPETHLPR3,pfnLock} */
static DECLCALLBACK(int) pdmR3HpetHlp_Lock(PPDMDEVINS pDevIns, int rc)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3HpetHlp_Lock: caller='%s'/%d: rc=%Rrc\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance, rc));
    return pdmLockEx(pDevIns->Internal.s.pVMR3, rc);
}


/** @interface_method_impl{PDMHPETHLPR3,pfnUnlock} */
static DECLCALLBACK(void) pdmR3HpetHlp_Unlock(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    LogFlow(("pdmR3HpetHlp_Unlock: caller='%s'/%d:\n", pDevIns->pReg->szDeviceName, pDevIns->iInstance));
    pdmUnlock(pDevIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMHPETHLPR3,pfnGetRCHelpers} */
static DECLCALLBACK(PCPDMHPETHLPRC) pdmR3HpetHlp_GetRCHelpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    RTRCPTR pRCHelpers = 0;
    int rc = PDMR3LdrGetSymbolRC(pDevIns->Internal.s.pVMR3, NULL, "g_pdmRCHpetHlp", &pRCHelpers);
    AssertReleaseRC(rc);
    AssertRelease(pRCHelpers);
    LogFlow(("pdmR3HpetHlp_GetGCHelpers: caller='%s'/%d: returns %RRv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pRCHelpers));
    return pRCHelpers;
}


/** @interface_method_impl{PDMHPETHLPR3,pfnGetR0Helpers} */
static DECLCALLBACK(PCPDMHPETHLPR0) pdmR3HpetHlp_GetR0Helpers(PPDMDEVINS pDevIns)
{
    PDMDEV_ASSERT_DEVINS(pDevIns);
    VM_ASSERT_EMT(pDevIns->Internal.s.pVMR3);
    PCPDMHPETHLPR0 pR0Helpers = 0;
    int rc = PDMR3LdrGetSymbolR0(pDevIns->Internal.s.pVMR3, NULL, "g_pdmR0HpetHlp", &pR0Helpers);
    AssertReleaseRC(rc);
    AssertRelease(pR0Helpers);
    LogFlow(("pdmR3HpetHlp_GetR0Helpers: caller='%s'/%d: returns %RHv\n",
             pDevIns->pReg->szDeviceName, pDevIns->iInstance, pR0Helpers));
    return pR0Helpers;
}


/**
 * HPET Device Helpers.
 */
const PDMHPETHLPR3 g_pdmR3DevHpetHlp =
{
    PDM_HPETHLPR3_VERSION,
    pdmR3HpetHlp_SetLegacyMode,
    pdmR3HpetHlp_Lock,
    pdmR3HpetHlp_Unlock,
    pdmR3HpetHlp_GetRCHelpers,
    pdmR3HpetHlp_GetR0Helpers,
    PDM_HPETHLPR3_VERSION, /* the end */
};

/** @} */



/* none yet */

/**
 * DMAC Device Helpers.
 */
const PDMDMACHLP g_pdmR3DevDmacHlp =
{
    PDM_DMACHLP_VERSION
};




/* none yet */

/**
 * RTC Device Helpers.
 */
const PDMRTCHLP g_pdmR3DevRtcHlp =
{
    PDM_RTCHLP_VERSION
};

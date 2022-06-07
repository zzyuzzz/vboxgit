/* $Id: GIMHv.cpp 58331 2015-10-20 11:25:21Z vboxsync $ */
/** @file
 * GIM - Guest Interface Manager, Hyper-V implementation.
 */

/*
 * Copyright (C) 2014-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_GIM
#include "GIMInternal.h"

#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/string.h>
#include <iprt/mem.h>
#include <iprt/semaphore.h>
#include <iprt/spinlock.h>

#include <VBox/vmm/cpum.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/ssm.h>
#include <VBox/vmm/vm.h>
#include <VBox/vmm/hm.h>
#include <VBox/vmm/pdmapi.h>
#include <VBox/version.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/**
 * GIM Hyper-V saved-state version.
 */
#define GIM_HV_SAVED_STATE_VERSION                UINT32_C(2)
/** Vanilla saved states, prior to any debug support. */
#define GIM_HV_SAVED_STATE_VERSION_PRE_DEBUG      UINT32_C(1)

#ifdef VBOX_WITH_STATISTICS
# define GIMHV_MSRRANGE(a_uFirst, a_uLast, a_szName) \
    { (a_uFirst), (a_uLast), kCpumMsrRdFn_Gim, kCpumMsrWrFn_Gim, 0, 0, 0, 0, 0, a_szName, { 0 }, { 0 }, { 0 }, { 0 } }
#else
# define GIMHV_MSRRANGE(a_uFirst, a_uLast, a_szName) \
    { (a_uFirst), (a_uLast), kCpumMsrRdFn_Gim, kCpumMsrWrFn_Gim, 0, 0, 0, 0, 0, a_szName }
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Array of MSR ranges supported by Hyper-V.
 */
static CPUMMSRRANGE const g_aMsrRanges_HyperV[] =
{
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE0_START,  MSR_GIM_HV_RANGE0_END,  "Hyper-V range 0"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE1_START,  MSR_GIM_HV_RANGE1_END,  "Hyper-V range 1"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE2_START,  MSR_GIM_HV_RANGE2_END,  "Hyper-V range 2"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE3_START,  MSR_GIM_HV_RANGE3_END,  "Hyper-V range 3"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE4_START,  MSR_GIM_HV_RANGE4_END,  "Hyper-V range 4"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE5_START,  MSR_GIM_HV_RANGE5_END,  "Hyper-V range 5"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE6_START,  MSR_GIM_HV_RANGE6_END,  "Hyper-V range 6"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE7_START,  MSR_GIM_HV_RANGE7_END,  "Hyper-V range 7"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE8_START,  MSR_GIM_HV_RANGE8_END,  "Hyper-V range 8"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE9_START,  MSR_GIM_HV_RANGE9_END,  "Hyper-V range 9"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE10_START, MSR_GIM_HV_RANGE10_END, "Hyper-V range 10"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE11_START, MSR_GIM_HV_RANGE11_END, "Hyper-V range 11"),
    GIMHV_MSRRANGE(MSR_GIM_HV_RANGE12_START, MSR_GIM_HV_RANGE12_END, "Hyper-V range 12")
};
#undef GIMHV_MSRRANGE

/**
 * DHCP OFFER packet response to the guest (client) over the Hyper-V debug
 * transport.
 *
 * - MAC: Destination: broadcast.
 * - MAC: Source: 00:00:00:00:01 (hypervisor). It's important that it's
 *   different from the client's MAC address which is all 0's.
 * - IP: Source: 10.0.5.1 (hypervisor)
 * - IP: Destination: broadcast.
 * - IP: Checksum included.
 * - BOOTP: Client IP address: 10.0.5.5.
 * - BOOTP: Server IP address: 10.0.5.1.
 * - DHCP options: Subnet mask, router, lease-time, DHCP server identifier.
 *   Options are kept to a minimum required for making Windows guests happy.
 */
#define GIMHV_DEBUGCLIENT_IPV4          RT_H2N_U32_C(0x0a000505)    /* 10.0.5.5 */
#define GIMHV_DEBUGSERVER_IPV4          RT_H2N_U32_C(0x0a000501)    /* 10.0.5.1 */
static const uint8_t g_abDhcpOffer[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x45, 0x10,
    0x01, 0x28, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x6a, 0xb5, 0x0a, 0x00, 0x05, 0x01, 0xff, 0xff,
    0xff, 0xff, 0x00, 0x43, 0x00, 0x44, 0x01, 0x14, 0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x05, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82, 0x53, 0x63, 0x35, 0x01, 0x02, 0x01, 0x04, 0xff,
    0xff, 0xff, 0x00, 0x03, 0x04, 0x0a, 0x00, 0x05, 0x01, 0x33, 0x04, 0xff, 0xff, 0xff, 0xff, 0x36,
    0x04, 0x0a, 0x00, 0x05, 0x01, 0xff
};

/**
 * DHCP ACK packet response to the guest (client) over the Hyper-V debug
 * transport.
 *
 * - MAC: Destination: 00:00:00:00:00 (client).
 * - IP: Destination: 10.0.5.5 (client).
 * - Rest are mostly similar to the DHCP offer.
 */
static const uint8_t g_abDhcpAck[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x45, 0x10,
    0x01, 0x28, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x5b, 0xb0, 0x0a, 0x00, 0x05, 0x01, 0x0a, 0x00,
    0x05, 0x05, 0x00, 0x43, 0x00, 0x44, 0x01, 0x14, 0x00, 0x00, 0x02, 0x01, 0x06, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x05, 0x05, 0x0a, 0x00, 0x05, 0x05, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82, 0x53, 0x63, 0x35, 0x01, 0x05, 0x01, 0x04, 0xff,
    0xff, 0xff, 0x00, 0x03, 0x04, 0x0a, 0x00, 0x05, 0x01, 0x33, 0x04, 0xff, 0xff, 0xff, 0xff, 0x36,
    0x04, 0x0a, 0x00, 0x05, 0x01, 0xff
};

/**
 * ARP reply to the guest (client) over the Hyper-V debug transport.
 *
 * - MAC: Destination: 00:00:00:00:00 (client)
 * - MAC: Source: 00:00:00:00:01 (hypervisor)
 * - ARP: Reply: 10.0.5.1 is at Source MAC address.
 */
static const uint8_t g_abArpReply[] =
{
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x01,
    0x08, 0x00, 0x06, 0x04, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x00, 0x05, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x00, 0x05, 0x05
};


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int    gimR3HvInitHypercallSupport(PVM pVM);
static void   gimR3HvTermHypercallSupport(PVM pVM);


/**
 * Initializes the Hyper-V GIM provider.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pGimCfg     The GIM CFGM node.
 */
VMMR3_INT_DECL(int) gimR3HvInit(PVM pVM, PCFGMNODE pGimCfg)
{
    AssertReturn(pVM, VERR_INVALID_PARAMETER);
    AssertReturn(pVM->gim.s.enmProviderId == GIMPROVIDERID_HYPERV, VERR_INTERNAL_ERROR_5);

    int rc;
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    /*
     * Read configuration.
     */
    PCFGMNODE pCfgHv = CFGMR3GetChild(pGimCfg, "HyperV");
    if (pCfgHv)
    {
        /*
         * Validate the Hyper-V settings.
         */
        rc = CFGMR3ValidateConfig(pCfgHv, "/HyperV/",
                                  "VendorID"
                                  "|VSInterface",
                                  "" /* pszValidNodes */, "GIM/HyperV" /* pszWho */, 0 /* uInstance */);
        if (RT_FAILURE(rc))
            return rc;
    }

    /** @cfgm{/GIM/HyperV/VendorID, string, 'VBoxVBoxVBox'}
     * The Hyper-V vendor signature, must be 12 characters. */
    char szVendor[13];
    rc = CFGMR3QueryStringDef(pCfgHv, "VendorID", szVendor, sizeof(szVendor), "VBoxVBoxVBox");
    AssertLogRelRCReturn(rc, rc);

    LogRel(("GIM: HyperV: Reporting vendor as '%s'\n", szVendor));
    if (!RTStrNCmp(szVendor, GIM_HV_VENDOR_MICROSOFT, sizeof(GIM_HV_VENDOR_MICROSOFT) - 1))
    {
        LogRel(("GIM: HyperV: Warning! Posing as the Microsoft vendor, guest behavior may be altered!\n"));
        pHv->fIsVendorMsHv = true;
    }

    if (pHv->fIsVendorMsHv)
    {
        /** @cfgm{/GIM/HyperV/VSInterface, bool, true}
         * The Microsoft virtualization service interface (debugging). */
        rc = CFGMR3QueryBoolDef(pCfgHv, "VSInterface", &pHv->fIsInterfaceVs, true);
        AssertLogRelRCReturn(rc, rc);
    }
    else
        Assert(pHv->fIsInterfaceVs == false);

    /*
     * Determine interface capabilities based on the version.
     */
    if (!pVM->gim.s.u32Version)
    {
        /* Basic features. */
        pHv->uBaseFeat = 0
                       //| GIM_HV_BASE_FEAT_VP_RUNTIME_MSR
                       | GIM_HV_BASE_FEAT_PART_TIME_REF_COUNT_MSR
                       //| GIM_HV_BASE_FEAT_BASIC_SYNTH_IC
                       //| GIM_HV_BASE_FEAT_SYNTH_TIMER_MSRS
                       | GIM_HV_BASE_FEAT_APIC_ACCESS_MSRS
                       | GIM_HV_BASE_FEAT_HYPERCALL_MSRS
                       | GIM_HV_BASE_FEAT_VP_ID_MSR
                       | GIM_HV_BASE_FEAT_VIRT_SYS_RESET_MSR
                       //| GIM_HV_BASE_FEAT_STAT_PAGES_MSR
                       | GIM_HV_BASE_FEAT_PART_REF_TSC_MSR
                       //| GIM_HV_BASE_FEAT_GUEST_IDLE_STATE_MSR
                       | GIM_HV_BASE_FEAT_TIMER_FREQ_MSRS
                       //| GIM_HV_BASE_FEAT_DEBUG_MSRS
                       ;

        /* Miscellaneous features. */
        pHv->uMiscFeat = 0
                       //| GIM_HV_MISC_FEAT_GUEST_DEBUGGING
                       //| GIM_HV_MISC_FEAT_XMM_HYPERCALL_INPUT
                       | GIM_HV_MISC_FEAT_TIMER_FREQ
                       | GIM_HV_MISC_FEAT_GUEST_CRASH_MSRS
                       //| GIM_HV_MISC_FEAT_DEBUG_MSRS
                       ;

        /* Hypervisor recommendations to the guest. */
        pHv->uHyperHints = GIM_HV_HINT_MSR_FOR_SYS_RESET
                         | GIM_HV_HINT_RELAX_TIME_CHECKS;

        /* Expose more if we're posing as Microsoft. We can, if needed, force MSR-based Hv
           debugging by not exposing these bits while exposing the VS interface.*/
        if (pHv->fIsVendorMsHv)
        {
            pHv->uMiscFeat  |= GIM_HV_MISC_FEAT_GUEST_DEBUGGING
                             | GIM_HV_MISC_FEAT_DEBUG_MSRS;

            pHv->uPartFlags |= GIM_HV_PART_FLAGS_DEBUGGING;
        }
    }

    /*
     * Populate the required fields in MMIO2 region records for registering.
     */
    AssertCompile(GIM_HV_PAGE_SIZE == PAGE_SIZE);
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    pRegion->iRegion    = GIM_HV_HYPERCALL_PAGE_REGION_IDX;
    pRegion->fRCMapping = false;
    pRegion->cbRegion   = PAGE_SIZE;  /* Sanity checked in gimR3HvLoad(), gimR3HvEnableTscPage() & gimR3HvEnableHypercallPage() */
    pRegion->GCPhysPage = NIL_RTGCPHYS;
    RTStrCopy(pRegion->szDescription, sizeof(pRegion->szDescription), "Hyper-V hypercall page");

    pRegion = &pHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    pRegion->iRegion    = GIM_HV_REF_TSC_PAGE_REGION_IDX;
    pRegion->fRCMapping = false;
    pRegion->cbRegion   = PAGE_SIZE;  /* Sanity checked in gimR3HvLoad(), gimR3HvEnableTscPage() & gimR3HvEnableHypercallPage() */
    pRegion->GCPhysPage = NIL_RTGCPHYS;
    RTStrCopy(pRegion->szDescription, sizeof(pRegion->szDescription), "Hyper-V TSC page");

    /*
     * Make sure the CPU ID bit are in accordance to the Hyper-V
     * requirement and other paranoia checks.
     * See "Requirements for implementing the Microsoft hypervisor interface" spec.
     */
    Assert(!(pHv->uPartFlags & (  GIM_HV_PART_FLAGS_CREATE_PART
                                | GIM_HV_PART_FLAGS_ACCESS_MEMORY_POOL
                                | GIM_HV_PART_FLAGS_ACCESS_PART_ID
                                | GIM_HV_PART_FLAGS_ADJUST_MSG_BUFFERS
                                | GIM_HV_PART_FLAGS_CREATE_PORT
                                | GIM_HV_PART_FLAGS_ACCESS_STATS
                                | GIM_HV_PART_FLAGS_CPU_MGMT
                                | GIM_HV_PART_FLAGS_CPU_PROFILER)));
    Assert((pHv->uBaseFeat & (GIM_HV_BASE_FEAT_HYPERCALL_MSRS | GIM_HV_BASE_FEAT_VP_ID_MSR))
                          == (GIM_HV_BASE_FEAT_HYPERCALL_MSRS | GIM_HV_BASE_FEAT_VP_ID_MSR));
    for (unsigned i = 0; i < RT_ELEMENTS(pHv->aMmio2Regions); i++)
    {
        PCGIMMMIO2REGION pcCur = &pHv->aMmio2Regions[i];
        Assert(!pcCur->fRCMapping);
        Assert(!pcCur->fMapped);
        Assert(pcCur->GCPhysPage == NIL_RTGCPHYS);
    }

    /*
     * Expose HVP (Hypervisor Present) bit to the guest.
     */
    CPUMSetGuestCpuIdFeature(pVM, CPUMCPUIDFEATURE_HVP);

    /*
     * Modify the standard hypervisor leaves for Hyper-V.
     */
    CPUMCPUIDLEAF HyperLeaf;
    RT_ZERO(HyperLeaf);
    HyperLeaf.uLeaf        = UINT32_C(0x40000000);
    HyperLeaf.uEax         = UINT32_C(0x40000006); /* Minimum value for Hyper-V is 0x40000005. */
    /*
     * Don't report vendor as 'Microsoft Hv'[1] by default, see @bugref{7270#c152}.
     * [1]: ebx=0x7263694d ('rciM') ecx=0x666f736f ('foso') edx=0x76482074 ('vH t')
     */
    {
        uint32_t uVendorEbx;
        uint32_t uVendorEcx;
        uint32_t uVendorEdx;
        uVendorEbx = ((uint32_t)szVendor[ 3]) << 24 | ((uint32_t)szVendor[ 2]) << 16 | ((uint32_t)szVendor[1]) << 8
                    | (uint32_t)szVendor[ 0];
        uVendorEcx = ((uint32_t)szVendor[ 7]) << 24 | ((uint32_t)szVendor[ 6]) << 16 | ((uint32_t)szVendor[5]) << 8
                    | (uint32_t)szVendor[ 4];
        uVendorEdx = ((uint32_t)szVendor[11]) << 24 | ((uint32_t)szVendor[10]) << 16 | ((uint32_t)szVendor[9]) << 8
                    | (uint32_t)szVendor[ 8];
        HyperLeaf.uEbx         = uVendorEbx;
        HyperLeaf.uEcx         = uVendorEcx;
        HyperLeaf.uEdx         = uVendorEdx;
    }
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000001);
    HyperLeaf.uEax         = 0x31237648;           /* 'Hv#1' */
    HyperLeaf.uEbx         = 0;                    /* Reserved */
    HyperLeaf.uEcx         = 0;                    /* Reserved */
    HyperLeaf.uEdx         = 0;                    /* Reserved */
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    /*
     * Add Hyper-V specific leaves.
     */
    HyperLeaf.uLeaf        = UINT32_C(0x40000002); /* MBZ until MSR_GIM_HV_GUEST_OS_ID is set by the guest. */
    HyperLeaf.uEax         = 0;
    HyperLeaf.uEbx         = 0;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000003);
    HyperLeaf.uEax         = pHv->uBaseFeat;
    HyperLeaf.uEbx         = pHv->uPartFlags;
    HyperLeaf.uEcx         = pHv->uPowMgmtFeat;
    HyperLeaf.uEdx         = pHv->uMiscFeat;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    HyperLeaf.uLeaf        = UINT32_C(0x40000004);
    HyperLeaf.uEax         = pHv->uHyperHints;
    HyperLeaf.uEbx         = 0xffffffff;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    if (   pHv->fIsVendorMsHv
        && pHv->fIsInterfaceVs)
    {
        HyperLeaf.uLeaf        = UINT32_C(0x40000080);
        HyperLeaf.uEax         = 0;
        HyperLeaf.uEbx         = 0x7263694d;        /* 'rciM' */
        HyperLeaf.uEcx         = 0x666f736f;        /* 'foso'*/
        HyperLeaf.uEdx         = 0x53562074;        /* 'SV t' */
        rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
        AssertLogRelRCReturn(rc, rc);

        HyperLeaf.uLeaf        = UINT32_C(0x40000081);
        HyperLeaf.uEax         = 0x31235356;        /* '1#SV' */
        HyperLeaf.uEbx         = 0;
        HyperLeaf.uEcx         = 0;
        HyperLeaf.uEdx         = 0;
        rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
        AssertLogRelRCReturn(rc, rc);

        HyperLeaf.uLeaf        = UINT32_C(0x40000082);
        HyperLeaf.uEax         = RT_BIT_32(1);
        HyperLeaf.uEbx         = 0;
        HyperLeaf.uEcx         = 0;
        HyperLeaf.uEdx         = 0;
        rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
        AssertLogRelRCReturn(rc, rc);
    }

    /*
     * Insert all MSR ranges of Hyper-V.
     */
    for (unsigned i = 0; i < RT_ELEMENTS(g_aMsrRanges_HyperV); i++)
    {
        rc = CPUMR3MsrRangesInsert(pVM, &g_aMsrRanges_HyperV[i]);
        AssertLogRelRCReturn(rc, rc);
    }

    /*
     * Setup non-zero MSRs.
     */
    if (pHv->uMiscFeat & GIM_HV_MISC_FEAT_GUEST_CRASH_MSRS)
        pHv->uCrashCtlMsr = MSR_GIM_HV_CRASH_CTL_NOTIFY_BIT;
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
        pVM->aCpus[i].gim.s.u.HvCpu.uSint2Msr = MSR_GIM_HV_SINT_MASKED_BIT;

    /*
     * Setup hypercall support.
     */
    rc = gimR3HvInitHypercallSupport(pVM);
    AssertLogRelRCReturn(rc, rc);

    return VINF_SUCCESS;
}


/**
 * Initializes remaining bits of the Hyper-V provider.
 *
 * This is called after initializing HM and almost all other VMM components.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) gimR3HvInitCompleted(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    pHv->cTscTicksPerSecond = TMCpuTicksPerSecond(pVM);

    /*
     * Determine interface capabilities based on the version.
     */
    if (!pVM->gim.s.u32Version)
    {
        /* Hypervisor capabilities; features used by the hypervisor. */
        pHv->uHyperCaps  = HMIsNestedPagingActive(pVM)   ? GIM_HV_HOST_FEAT_NESTED_PAGING : 0;
        pHv->uHyperCaps |= HMAreMsrBitmapsAvailable(pVM) ? GIM_HV_HOST_FEAT_MSR_BITMAP : 0;
    }

    CPUMCPUIDLEAF HyperLeaf;
    RT_ZERO(HyperLeaf);
    HyperLeaf.uLeaf        = UINT32_C(0x40000006);
    HyperLeaf.uEax         = pHv->uHyperCaps;
    HyperLeaf.uEbx         = 0;
    HyperLeaf.uEcx         = 0;
    HyperLeaf.uEdx         = 0;
    int rc = CPUMR3CpuIdInsert(pVM, &HyperLeaf);
    AssertLogRelRCReturn(rc, rc);

    return rc;
}


#if 0
VMMR3_INT_DECL(int) gimR3HvInitFinalize(PVM pVM)
{
    pVM->gim.s.pfnHypercallR3 = &GIMHvHypercall;
    if (!HMIsEnabled(pVM))
    {
        rc = PDMR3LdrGetSymbolRC(pVM, NULL /* pszModule */, GIMHV_HYPERCALL, &pVM->gim.s.pfnHypercallRC);
        AssertRCReturn(rc, rc);
    }
    rc = PDMR3LdrGetSymbolR0(pVM, NULL /* pszModule */, GIMHV_HYPERCALL, &pVM->gim.s.pfnHypercallR0);
    AssertRCReturn(rc, rc);
}
#endif


/**
 * Terminates the Hyper-V GIM provider.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 */
VMMR3_INT_DECL(int) gimR3HvTerm(PVM pVM)
{
    gimR3HvReset(pVM);
    gimR3HvTermHypercallSupport(pVM);
    return VINF_SUCCESS;
}


/**
 * Applies relocations to data and code managed by this component.
 *
 * This function will be called at init and whenever the VMM need to relocate
 * itself inside the GC.
 *
 * @param   pVM         The cross context VM structure.
 * @param   offDelta    Relocation delta relative to old location.
 */
VMMR3_INT_DECL(void) gimR3HvRelocate(PVM pVM, RTGCINTPTR offDelta)
{
#if 0
    int rc = PDMR3LdrGetSymbolRC(pVM, NULL /* pszModule */, GIMHV_HYPERCALL, &pVM->gim.s.pfnHypercallRC);
    AssertFatalRC(rc);
#endif
}


/**
 * This resets Hyper-V provider MSRs and unmaps whatever Hyper-V regions that
 * the guest may have mapped.
 *
 * This is called when the VM is being reset.
 *
 * @param   pVM     The cross context VM structure.
 *
 * @thread  EMT(0).
 */
VMMR3_INT_DECL(void) gimR3HvReset(PVM pVM)
{
    VM_ASSERT_EMT0(pVM);

    /*
     * Unmap MMIO2 pages that the guest may have setup.
     */
    LogRel(("GIM: HyperV: Resetting MMIO2 regions and MSRs\n"));
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    for (unsigned i = 0; i < RT_ELEMENTS(pHv->aMmio2Regions); i++)
    {
        PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[i];
#if 0
        GIMR3Mmio2Unmap(pVM, pRegion);
#else
        pRegion->fMapped    = false;
        pRegion->GCPhysPage = NIL_RTGCPHYS;
#endif
    }

    /*
     * Reset MSRs.
     */
    pHv->u64GuestOsIdMsr        = 0;
    pHv->u64HypercallMsr        = 0;
    pHv->u64TscPageMsr          = 0;
    pHv->uCrashP0Msr            = 0;
    pHv->uCrashP1Msr            = 0;
    pHv->uCrashP2Msr            = 0;
    pHv->uCrashP3Msr            = 0;
    pHv->uCrashP4Msr            = 0;
    pHv->uDebugStatusMsr        = 0;
    pHv->uDebugPendingBufferMsr = 0;
    pHv->uDebugSendBufferMsr    = 0;
    pHv->uDebugRecvBufferMsr    = 0;
    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PVMCPU pVCpu = &pVM->aCpus[i];
        pVCpu->gim.s.u.HvCpu.uSint2Msr = MSR_GIM_HV_SINT_MASKED_BIT;
        pVCpu->gim.s.u.HvCpu.uSimpMsr  = 0;
    }
}


/**
 * Returns a pointer to the MMIO2 regions supported by Hyper-V.
 *
 * @returns Pointer to an array of MMIO2 regions.
 * @param   pVM         The cross context VM structure.
 * @param   pcRegions   Where to store the number of regions in the array.
 */
VMMR3_INT_DECL(PGIMMMIO2REGION) gimR3HvGetMmio2Regions(PVM pVM, uint32_t *pcRegions)
{
    Assert(GIMIsEnabled(pVM));
    PGIMHV pHv = &pVM->gim.s.u.Hv;

    *pcRegions = RT_ELEMENTS(pHv->aMmio2Regions);
    Assert(*pcRegions <= UINT8_MAX);    /* See PGMR3PhysMMIO2Register(). */
    return pHv->aMmio2Regions;
}


/**
 * Hyper-V state-save operation.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 * @param   pSSM    Pointer to the SSM handle.
 */
VMMR3_INT_DECL(int) gimR3HvSave(PVM pVM, PSSMHANDLE pSSM)
{
    PCGIMHV pcHv = &pVM->gim.s.u.Hv;

    /*
     * Save the Hyper-V SSM version.
     */
    SSMR3PutU32(pSSM, GIM_HV_SAVED_STATE_VERSION);

    /*
     * Save per-VM MSRs.
     */
    SSMR3PutU64(pSSM, pcHv->u64GuestOsIdMsr);
    SSMR3PutU64(pSSM, pcHv->u64HypercallMsr);
    SSMR3PutU64(pSSM, pcHv->u64TscPageMsr);

    /*
     * Save Hyper-V features / capabilities.
     */
    SSMR3PutU32(pSSM, pcHv->uBaseFeat);
    SSMR3PutU32(pSSM, pcHv->uPartFlags);
    SSMR3PutU32(pSSM, pcHv->uPowMgmtFeat);
    SSMR3PutU32(pSSM, pcHv->uMiscFeat);
    SSMR3PutU32(pSSM, pcHv->uHyperHints);
    SSMR3PutU32(pSSM, pcHv->uHyperCaps);

    /*
     * Save the Hypercall region.
     */
    PCGIMMMIO2REGION pcRegion = &pcHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    SSMR3PutU8(pSSM,     pcRegion->iRegion);
    SSMR3PutBool(pSSM,   pcRegion->fRCMapping);
    SSMR3PutU32(pSSM,    pcRegion->cbRegion);
    SSMR3PutGCPhys(pSSM, pcRegion->GCPhysPage);
    SSMR3PutStrZ(pSSM,   pcRegion->szDescription);

    /*
     * Save the reference TSC region.
     */
    pcRegion = &pcHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    SSMR3PutU8(pSSM,     pcRegion->iRegion);
    SSMR3PutBool(pSSM,   pcRegion->fRCMapping);
    SSMR3PutU32(pSSM,    pcRegion->cbRegion);
    SSMR3PutGCPhys(pSSM, pcRegion->GCPhysPage);
    SSMR3PutStrZ(pSSM,   pcRegion->szDescription);
    /* Save the TSC sequence so we can bump it on restore (as the CPU frequency/offset may change). */
    uint32_t uTscSequence = 0;
    if (   pcRegion->fMapped
        && MSR_GIM_HV_REF_TSC_IS_ENABLED(pcHv->u64TscPageMsr))
    {
        PCGIMHVREFTSC pcRefTsc = (PCGIMHVREFTSC)pcRegion->pvPageR3;
        uTscSequence = pcRefTsc->u32TscSequence;
    }
    SSMR3PutU32(pSSM, uTscSequence);

    /*
     * Save debug support data.
     */
    SSMR3PutU64(pSSM, pcHv->uDebugPendingBufferMsr);
    SSMR3PutU64(pSSM, pcHv->uDebugSendBufferMsr);
    SSMR3PutU64(pSSM, pcHv->uDebugRecvBufferMsr);
    SSMR3PutU64(pSSM, pcHv->uDebugStatusMsr);
    SSMR3PutU32(pSSM, pcHv->enmDebugReply);
    SSMR3PutU32(pSSM, pcHv->uBootpXId);
    SSMR3PutU32(pSSM, pcHv->DbgGuestIp4Addr.u);

    for (VMCPUID i = 0; i < pVM->cCpus; i++)
    {
        PGIMHVCPU pHvCpu = &pVM->aCpus[i].gim.s.u.HvCpu;
        SSMR3PutU64(pSSM, pHvCpu->uSimpMsr);
        SSMR3PutU64(pSSM, pHvCpu->uSint2Msr);
    }

    return SSMR3PutU8(pSSM, UINT8_MAX);;
}


/**
 * Hyper-V state-load operation, final pass.
 *
 * @returns VBox status code.
 * @param   pVM             The cross context VM structure.
 * @param   pSSM            Pointer to the SSM handle.
 * @param   uSSMVersion     The GIM saved-state version.
 */
VMMR3_INT_DECL(int) gimR3HvLoad(PVM pVM, PSSMHANDLE pSSM, uint32_t uSSMVersion)
{
    /*
     * Load the Hyper-V SSM version first.
     */
    uint32_t uHvSavedStatVersion;
    int rc = SSMR3GetU32(pSSM, &uHvSavedStatVersion);
    AssertRCReturn(rc, rc);
    if (   uHvSavedStatVersion != GIM_HV_SAVED_STATE_VERSION
        && uHvSavedStatVersion != GIM_HV_SAVED_STATE_VERSION_PRE_DEBUG)
        return SSMR3SetLoadError(pSSM, VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION, RT_SRC_POS,
                                 N_("Unsupported Hyper-V saved-state version %u (current %u)!"), uHvSavedStatVersion,
                                 GIM_HV_SAVED_STATE_VERSION);

    /*
     * Update the TSC frequency from TM.
     */
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    pHv->cTscTicksPerSecond = TMCpuTicksPerSecond(pVM);

    /*
     * Load per-VM MSRs.
     */
    SSMR3GetU64(pSSM, &pHv->u64GuestOsIdMsr);
    SSMR3GetU64(pSSM, &pHv->u64HypercallMsr);
    SSMR3GetU64(pSSM, &pHv->u64TscPageMsr);

    /*
     * Load Hyper-V features / capabilities.
     */
    SSMR3GetU32(pSSM, &pHv->uBaseFeat);
    SSMR3GetU32(pSSM, &pHv->uPartFlags);
    SSMR3GetU32(pSSM, &pHv->uPowMgmtFeat);
    SSMR3GetU32(pSSM, &pHv->uMiscFeat);
    SSMR3GetU32(pSSM, &pHv->uHyperHints);
    SSMR3GetU32(pSSM, &pHv->uHyperCaps);

    /*
     * Load and enable the Hypercall region.
     */
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    SSMR3GetU8(pSSM,     &pRegion->iRegion);
    SSMR3GetBool(pSSM,   &pRegion->fRCMapping);
    SSMR3GetU32(pSSM,    &pRegion->cbRegion);
    SSMR3GetGCPhys(pSSM, &pRegion->GCPhysPage);
    rc = SSMR3GetStrZ(pSSM, pRegion->szDescription, sizeof(pRegion->szDescription));
    AssertRCReturn(rc, rc);

    if (pRegion->cbRegion != PAGE_SIZE)
        return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Hypercall page region size %u invalid, expected %u"),
                                pRegion->cbRegion, PAGE_SIZE);

    if (MSR_GIM_HV_HYPERCALL_PAGE_IS_ENABLED(pHv->u64HypercallMsr))
    {
        Assert(pRegion->GCPhysPage != NIL_RTGCPHYS);
        if (RT_LIKELY(pRegion->fRegistered))
        {
            rc = gimR3HvEnableHypercallPage(pVM, pRegion->GCPhysPage);
            if (RT_FAILURE(rc))
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Failed to enable the hypercall page. GCPhys=%#RGp rc=%Rrc"),
                                        pRegion->GCPhysPage, rc);
        }
        else
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Hypercall MMIO2 region not registered. Missing GIM device?!"));
    }

    /*
     * Load and enable the reference TSC region.
     */
    uint32_t uTscSequence;
    pRegion = &pHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    SSMR3GetU8(pSSM,     &pRegion->iRegion);
    SSMR3GetBool(pSSM,   &pRegion->fRCMapping);
    SSMR3GetU32(pSSM,    &pRegion->cbRegion);
    SSMR3GetGCPhys(pSSM, &pRegion->GCPhysPage);
    SSMR3GetStrZ(pSSM,    pRegion->szDescription, sizeof(pRegion->szDescription));
    rc = SSMR3GetU32(pSSM, &uTscSequence);
    AssertRCReturn(rc, rc);

    if (pRegion->cbRegion != PAGE_SIZE)
        return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("TSC page region size %u invalid, expected %u"),
                                pRegion->cbRegion, PAGE_SIZE);

    if (MSR_GIM_HV_REF_TSC_IS_ENABLED(pHv->u64TscPageMsr))
    {
        Assert(pRegion->GCPhysPage != NIL_RTGCPHYS);
        if (pRegion->fRegistered)
        {
            rc = gimR3HvEnableTscPage(pVM, pRegion->GCPhysPage, true /* fUseThisTscSeq */, uTscSequence);
            if (RT_FAILURE(rc))
                return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("Failed to enable the TSC page. GCPhys=%#RGp rc=%Rrc"),
                                        pRegion->GCPhysPage, rc);
        }
        else
            return SSMR3SetCfgError(pSSM, RT_SRC_POS, N_("TSC-page MMIO2 region not registered. Missing GIM device?!"));
    }

    /*
     * Load the debug support data.
     */
    if (uHvSavedStatVersion > GIM_HV_SAVED_STATE_VERSION_PRE_DEBUG)
    {
        SSMR3GetU64(pSSM, &pHv->uDebugPendingBufferMsr);
        SSMR3GetU64(pSSM, &pHv->uDebugSendBufferMsr);
        SSMR3GetU64(pSSM, &pHv->uDebugRecvBufferMsr);
        SSMR3GetU64(pSSM, &pHv->uDebugStatusMsr);
        SSMR3GetU32(pSSM, (uint32_t *)&pHv->enmDebugReply);
        SSMR3GetU32(pSSM, &pHv->uBootpXId);
        rc = SSMR3GetU32(pSSM, &pHv->DbgGuestIp4Addr.u);
        AssertRCReturn(rc, rc);

        for (VMCPUID i = 0; i < pVM->cCpus; i++)
        {
            PGIMHVCPU pHvCpu = &pVM->aCpus[i].gim.s.u.HvCpu;
            SSMR3GetU64(pSSM, &pHvCpu->uSimpMsr);
            SSMR3GetU64(pSSM, &pHvCpu->uSint2Msr);
        }
    }

    uint8_t bDelim;
    return SSMR3GetU8(pSSM, &bDelim);
}


/**
 * Enables the Hyper-V TSC page.
 *
 * @returns VBox status code.
 * @param   pVM                The cross context VM structure.
 * @param   GCPhysTscPage      Where to map the TSC page.
 * @param   fUseThisTscSeq     Whether to set the TSC sequence number to the one
 *                             specified in @a uTscSeq.
 * @param   uTscSeq            The TSC sequence value to use. Ignored if
 *                             @a fUseThisTscSeq is false.
 */
VMMR3_INT_DECL(int) gimR3HvEnableTscPage(PVM pVM, RTGCPHYS GCPhysTscPage, bool fUseThisTscSeq, uint32_t uTscSeq)
{
    PPDMDEVINSR3    pDevIns = pVM->gim.s.pDevInsR3;
    PGIMMMIO2REGION pRegion = &pVM->gim.s.u.Hv.aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    AssertPtrReturn(pDevIns, VERR_GIM_DEVICE_NOT_REGISTERED);

    int rc;
    if (pRegion->fMapped)
    {
        /*
         * Is it already enabled at the given guest-address?
         */
        if (pRegion->GCPhysPage == GCPhysTscPage)
            return VINF_SUCCESS;

        /*
         * If it's mapped at a different address, unmap the previous address.
         */
        rc = gimR3HvDisableTscPage(pVM);
        AssertRC(rc);
    }

    /*
     * Map the TSC-page at the specified address.
     */
    Assert(!pRegion->fMapped);

    /** @todo this is buggy when large pages are used due to a PGM limitation, see
     *        @bugref{7532}. Instead of the overlay style mapping, we just
     *               rewrite guest memory directly. */
#if 0
    rc = GIMR3Mmio2Map(pVM, pRegion, GCPhysTscPage);
    if (RT_SUCCESS(rc))
    {
        Assert(pRegion->GCPhysPage == GCPhysTscPage);

        /*
         * Update the TSC scale. Windows guests expect a non-zero TSC sequence, otherwise
         * they fallback to using the reference count MSR which is not ideal in terms of VM-exits.
         *
         * Also, Hyper-V normalizes the time in 10 MHz, see:
         * http://technet.microsoft.com/it-it/sysinternals/dn553408%28v=vs.110%29
         */
        PGIMHVREFTSC pRefTsc = (PGIMHVREFTSC)pRegion->pvPageR3;
        Assert(pRefTsc);

        PGIMHV pHv = &pVM->gim.s.u.Hv;
        uint64_t const u64TscKHz = pHv->cTscTicksPerSecond / UINT64_C(1000);
        uint32_t       u32TscSeq = 1;
        if (   fUseThisTscSeq
            && uTscSeq < UINT32_C(0xfffffffe))
            u32TscSeq = uTscSeq + 1;
        pRefTsc->u32TscSequence  = u32TscSeq;
        pRefTsc->u64TscScale     = ((INT64_C(10000) << 32) / u64TscKHz) << 32;
        pRefTsc->i64TscOffset    = 0;

        LogRel(("GIM: HyperV: Enabled TSC page at %#RGp - u64TscScale=%#RX64 u64TscKHz=%#RX64 (%'RU64) Seq=%#RU32\n",
                GCPhysTscPage, pRefTsc->u64TscScale, u64TscKHz, u64TscKHz, pRefTsc->u32TscSequence));

        TMR3CpuTickParavirtEnable(pVM);
        return VINF_SUCCESS;
    }
    else
        LogRelFunc(("GIMR3Mmio2Map failed. rc=%Rrc\n", rc));
    return VERR_GIM_OPERATION_FAILED;
#else
    AssertReturn(pRegion->cbRegion == PAGE_SIZE, VERR_GIM_IPE_2);
    PGIMHVREFTSC pRefTsc = (PGIMHVREFTSC)RTMemAllocZ(PAGE_SIZE);
    if (RT_UNLIKELY(!pRefTsc))
    {
        LogRelFunc(("Failed to alloc %u bytes\n", PAGE_SIZE));
        return VERR_NO_MEMORY;
    }

    PGIMHV pHv = &pVM->gim.s.u.Hv;
    uint64_t const u64TscKHz = pHv->cTscTicksPerSecond / UINT64_C(1000);
    uint32_t       u32TscSeq = 1;
    if (   fUseThisTscSeq
        && uTscSeq < UINT32_C(0xfffffffe))
        u32TscSeq = uTscSeq + 1;
    pRefTsc->u32TscSequence  = u32TscSeq;
    pRefTsc->u64TscScale     = ((INT64_C(10000) << 32) / u64TscKHz) << 32;
    pRefTsc->i64TscOffset    = 0;

    rc = PGMPhysSimpleWriteGCPhys(pVM, GCPhysTscPage, pRefTsc, sizeof(*pRefTsc));
    if (RT_SUCCESS(rc))
    {
        LogRel(("GIM: HyperV: Enabled TSC page at %#RGp - u64TscScale=%#RX64 u64TscKHz=%#RX64 (%'RU64) Seq=%#RU32\n",
                GCPhysTscPage, pRefTsc->u64TscScale, u64TscKHz, u64TscKHz, pRefTsc->u32TscSequence));

        pRegion->GCPhysPage = GCPhysTscPage;
        pRegion->fMapped = true;
        TMR3CpuTickParavirtEnable(pVM);
    }
    else
    {
        LogRelFunc(("GIM: HyperV: PGMPhysSimpleWriteGCPhys failed. rc=%Rrc\n", rc));
        rc = VERR_GIM_OPERATION_FAILED;
    }
    RTMemFree(pRefTsc);
    return rc;
#endif
}


/**
 * Disables the Hyper-V TSC page.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
VMMR3_INT_DECL(int) gimR3HvDisableTscPage(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_REF_TSC_PAGE_REGION_IDX];
    if (pRegion->fMapped)
    {
#if 0
        GIMR3Mmio2Unmap(pVM, pRegion);
        Assert(!pRegion->fMapped);
#else
        pRegion->fMapped = false;
#endif
        LogRel(("GIM: HyperV: Disabled TSC-page\n"));

        TMR3CpuTickParavirtDisable(pVM);
        return VINF_SUCCESS;
    }
    return VERR_GIM_PVTSC_NOT_ENABLED;
}


/**
 * Disables the Hyper-V Hypercall page.
 *
 * @returns VBox status code.
 */
VMMR3_INT_DECL(int) gimR3HvDisableHypercallPage(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    PGIMMMIO2REGION pRegion = &pHv->aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    if (pRegion->fMapped)
    {
#if 0
        GIMR3Mmio2Unmap(pVM, pRegion);
        Assert(!pRegion->fMapped);
#else
        pRegion->fMapped = false;
#endif
        LogRel(("GIM: HyperV: Disabled Hypercall-page\n"));
        return VINF_SUCCESS;
    }
    return VERR_GIM_HYPERCALLS_NOT_ENABLED;
}


/**
 * Enables the Hyper-V Hypercall page.
 *
 * @returns VBox status code.
 * @param   pVM                     The cross context VM structure.
 * @param   GCPhysHypercallPage     Where to map the hypercall page.
 */
VMMR3_INT_DECL(int) gimR3HvEnableHypercallPage(PVM pVM, RTGCPHYS GCPhysHypercallPage)
{
    PPDMDEVINSR3    pDevIns = pVM->gim.s.pDevInsR3;
    PGIMMMIO2REGION pRegion = &pVM->gim.s.u.Hv.aMmio2Regions[GIM_HV_HYPERCALL_PAGE_REGION_IDX];
    AssertPtrReturn(pDevIns, VERR_GIM_DEVICE_NOT_REGISTERED);

    if (pRegion->fMapped)
    {
        /*
         * Is it already enabled at the given guest-address?
         */
        if (pRegion->GCPhysPage == GCPhysHypercallPage)
            return VINF_SUCCESS;

        /*
         * If it's mapped at a different address, unmap the previous address.
         */
        int rc2 = gimR3HvDisableHypercallPage(pVM);
        AssertRC(rc2);
    }

    /*
     * Map the hypercall-page at the specified address.
     */
    Assert(!pRegion->fMapped);

    /** @todo this is buggy when large pages are used due to a PGM limitation, see
     *        @bugref{7532}. Instead of the overlay style mapping, we just
     *               rewrite guest memory directly. */
#if 0
    int rc = GIMR3Mmio2Map(pVM, pRegion, GCPhysHypercallPage);
    if (RT_SUCCESS(rc))
    {
        Assert(pRegion->GCPhysPage == GCPhysHypercallPage);

        /*
         * Patch the hypercall-page.
         */
        size_t cbWritten = 0;
        rc = VMMPatchHypercall(pVM, pRegion->pvPageR3, PAGE_SIZE, &cbWritten);
        if (   RT_SUCCESS(rc)
            && cbWritten < PAGE_SIZE)
        {
            uint8_t *pbLast = (uint8_t *)pRegion->pvPageR3 + cbWritten;
            *pbLast = 0xc3;  /* RET */

            /*
             * Notify VMM that hypercalls are now enabled for all VCPUs.
             */
            for (VMCPUID i = 0; i < pVM->cCpus; i++)
                VMMHypercallsEnable(&pVM->aCpus[i]);

            LogRel(("GIM: HyperV: Enabled hypercall page at %#RGp\n", GCPhysHypercallPage));
            return VINF_SUCCESS;
        }
        else
        {
            if (rc == VINF_SUCCESS)
                rc = VERR_GIM_OPERATION_FAILED;
            LogRel(("GIM: HyperV: VMMPatchHypercall failed. rc=%Rrc cbWritten=%u\n", rc, cbWritten));
        }

        GIMR3Mmio2Unmap(pVM, pRegion);
    }

    LogRel(("GIM: HyperV: GIMR3Mmio2Map failed. rc=%Rrc\n", rc));
    return rc;
#else
    AssertReturn(pRegion->cbRegion == PAGE_SIZE, VERR_GIM_IPE_3);
    void *pvHypercallPage = RTMemAllocZ(PAGE_SIZE);
    if (RT_UNLIKELY(!pvHypercallPage))
    {
        LogRelFunc(("Failed to alloc %u bytes\n", PAGE_SIZE));
        return VERR_NO_MEMORY;
    }

    /*
     * Patch the hypercall-page.
     */
    size_t cbWritten = 0;
    int rc = VMMPatchHypercall(pVM, pvHypercallPage, PAGE_SIZE, &cbWritten);
    if (   RT_SUCCESS(rc)
        && cbWritten < PAGE_SIZE)
    {
        uint8_t *pbLast = (uint8_t *)pvHypercallPage + cbWritten;
        *pbLast = 0xc3;  /* RET */

        rc = PGMPhysSimpleWriteGCPhys(pVM, GCPhysHypercallPage, pvHypercallPage, PAGE_SIZE);
        if (RT_SUCCESS(rc))
        {
            pRegion->GCPhysPage = GCPhysHypercallPage;
            pRegion->fMapped = true;
            LogRel(("GIM: HyperV: Enabled hypercall page at %#RGp\n", GCPhysHypercallPage));
        }
        else
            LogRel(("GIM: HyperV: PGMPhysSimpleWriteGCPhys failed during hypercall page setup. rc=%Rrc\n", rc));
    }
    else
    {
        if (rc == VINF_SUCCESS)
            rc = VERR_GIM_OPERATION_FAILED;
        LogRel(("GIM: HyperV: VMMPatchHypercall failed. rc=%Rrc cbWritten=%u\n", rc, cbWritten));
    }

    RTMemFree(pvHypercallPage);
    return rc;
#endif
}


/**
 * Initializes Hyper-V guest hypercall support.
 *
 * @returns VBox status code.
 * @param   pVM     The cross context VM structure.
 */
static int gimR3HvInitHypercallSupport(PVM pVM)
{
    int rc = VINF_SUCCESS;
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    pHv->pbHypercallIn = (uint8_t *)RTMemAllocZ(GIM_HV_PAGE_SIZE);
    if (RT_LIKELY(pHv->pbHypercallIn))
    {
        pHv->pbHypercallOut = (uint8_t *)RTMemAllocZ(GIM_HV_PAGE_SIZE);
        if (RT_LIKELY(pHv->pbHypercallOut))
            return VINF_SUCCESS;
        RTMemFree(pHv->pbHypercallIn);
    }
    return VERR_NO_MEMORY;
}


/**
 * Terminates Hyper-V guest hypercall support.
 *
 * @param   pVM     The cross context VM structure.
 */
static void gimR3HvTermHypercallSupport(PVM pVM)
{
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    RTMemFree(pHv->pbHypercallIn);
    pHv->pbHypercallIn = NULL;

    RTMemFree(pHv->pbHypercallOut);
    pHv->pbHypercallOut = NULL;
}


/**
 * Reads data from a debugger connection, asynchronous.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pvBuf       Where to read the data.
 * @param   cbBuf       Size of the read buffer @a pvBuf, must be >= @a cbRead.
 * @param   cbRead      Number of bytes to read.
 * @param   pcbRead     Where to store how many bytes were really read.
 * @param   cMsTimeout  Timeout of the read operation in milliseconds.
 * @param   fUdpPkt     Whether the debug data returned in @a pvBuf needs to be
 *                      encapsulated in a UDP frame.
 *
 * @thread  EMT.
 */
VMMR3_INT_DECL(int) gimR3HvDebugRead(PVM pVM, void *pvBuf, uint32_t cbBuf, uint32_t cbRead, uint32_t *pcbRead,
                                     uint32_t cMsTimeout, bool fUdpPkt)
{
    NOREF(cMsTimeout);      /** @todo implement timeout. */
    AssertCompile(sizeof(size_t) >= sizeof(uint32_t));
    AssertReturn(cbBuf >= cbRead, VERR_INVALID_PARAMETER);

    int rc;
    if (!fUdpPkt)
    {
        /*
         * Read the raw debug data.
         */
        size_t cbReallyRead = cbRead;
        rc = GIMR3DebugRead(pVM, pvBuf, &cbReallyRead);
        *pcbRead = (uint32_t)cbReallyRead;
    }
    else
    {
        /*
         * Guest requires UDP encapsulated frames.
         */
        PGIMHV pHv = &pVM->gim.s.u.Hv;
        rc = VERR_GIM_IPE_1;
        switch (pHv->enmDebugReply)
        {
            case GIMHVDEBUGREPLY_UDP:
            {
                size_t cbReallyRead = cbRead;
                rc = GIMR3DebugRead(pVM, pvBuf, &cbReallyRead);
                if (   RT_SUCCESS(rc)
                    && cbReallyRead > 0)
                {
                    uint8_t abFrame[sizeof(RTNETETHERHDR) + RTNETIPV4_MIN_LEN + sizeof(RTNETUDP)];
                    if (cbReallyRead + sizeof(abFrame) <= cbBuf)
                    {
                        /*
                         * Windows guests pumps ethernet frames over the Hyper-V debug connection as
                         * explained in gimR3HvHypercallPostDebugData(). Here, we reconstruct the packet
                         * with the guest's self-chosen IP ARP address we saved in pHv->DbgGuestAddr.
                         *
                         * Note! We really need to pass the minimum IPv4 header length. The Windows 10 guest
                         * is -not- happy if we include the IPv4 options field, i.e. using sizeof(RTNETIPV4)
                         * instead of RTNETIPV4_MIN_LEN.
                         */
                        RT_ZERO(abFrame);
                        PRTNETETHERHDR pEthHdr = (PRTNETETHERHDR)&abFrame[0];
                        PRTNETIPV4     pIpHdr  = (PRTNETIPV4)    (pEthHdr + 1);
                        PRTNETUDP      pUdpHdr = (PRTNETUDP)     ((uint8_t *)pIpHdr + RTNETIPV4_MIN_LEN);

                        /* Ethernet */
                        pEthHdr->EtherType = RT_H2N_U16_C(RTNET_ETHERTYPE_IPV4);
                        /* IPv4 */
                        pIpHdr->ip_v       = 4;
                        pIpHdr->ip_hl      = RTNETIPV4_MIN_LEN / sizeof(uint32_t);
                        pIpHdr->ip_tos     = 0;
                        pIpHdr->ip_len     = RT_H2N_U16((uint16_t)cbReallyRead + sizeof(RTNETUDP) + RTNETIPV4_MIN_LEN);
                        pIpHdr->ip_id      = 0;
                        pIpHdr->ip_off     = 0;
                        pIpHdr->ip_ttl     = 255;
                        pIpHdr->ip_p       = RTNETIPV4_PROT_UDP;
                        pIpHdr->ip_sum     = 0;
                        pIpHdr->ip_src.u   = 0;
                        pIpHdr->ip_dst.u   = pHv->DbgGuestIp4Addr.u;
                        pIpHdr->ip_sum     = RTNetIPv4HdrChecksum(pIpHdr);
                        /* UDP */
                        pUdpHdr->uh_ulen   = RT_H2N_U16_C((uint16_t)cbReallyRead + sizeof(*pUdpHdr));

                        /* Make room by moving the payload and prepending the headers. */
                        uint8_t *pbData = (uint8_t *)pvBuf;
                        memmove(pbData + sizeof(abFrame), pbData, cbReallyRead);
                        memcpy(pbData, &abFrame[0], sizeof(abFrame));

                        /* Update the adjusted sizes. */
                        cbReallyRead += sizeof(abFrame);
                    }
                    else
                        rc = VERR_BUFFER_UNDERFLOW;
                }
                *pcbRead = (uint32_t)cbReallyRead;
                break;
            }

            case GIMHVDEBUGREPLY_ARP_REPLY:
            {
                uint32_t const cbArpReplyPkt =  sizeof(g_abArpReply);
                if (cbBuf >= cbArpReplyPkt)
                {
                    memcpy(pvBuf, g_abArpReply, cbArpReplyPkt);
                    rc = VINF_SUCCESS;
                    *pcbRead = cbArpReplyPkt;
                    pHv->enmDebugReply = GIMHVDEBUGREPLY_ARP_REPLY_SENT;
                }
                else
                {
                    rc = VERR_BUFFER_UNDERFLOW;
                    *pcbRead = 0;
                }
                break;
            }

            case GIMHVDEBUGREPLY_DHCP_OFFER:
            {
                uint32_t const cbDhcpOfferPkt = sizeof(g_abDhcpOffer);
                if (cbBuf >= cbDhcpOfferPkt)
                {
                    memcpy(pvBuf, g_abDhcpOffer, cbDhcpOfferPkt);
                    PRTNETETHERHDR pEthHdr   = (PRTNETETHERHDR)pvBuf;
                    PRTNETIPV4     pIpHdr    = (PRTNETIPV4)    (pEthHdr + 1);
                    PRTNETUDP      pUdpHdr   = (PRTNETUDP)     ((uint8_t *)pIpHdr + RTNETIPV4_MIN_LEN);
                    PRTNETBOOTP    pBootpHdr = (PRTNETBOOTP)   (pUdpHdr + 1);
                    pBootpHdr->bp_xid = pHv->uBootpXId;

                    rc = VINF_SUCCESS;
                    *pcbRead = cbDhcpOfferPkt;
                    pHv->enmDebugReply = GIMHVDEBUGREPLY_DHCP_OFFER_SENT;
                    LogRel(("GIM: HyperV: Debug DHCP offered IP address %RTnaipv4, transaction Id %#x\n", pBootpHdr->bp_yiaddr,
                            RT_N2H_U32(pHv->uBootpXId)));
                }
                else
                {
                    rc = VERR_BUFFER_UNDERFLOW;
                    *pcbRead = 0;
                }
                break;
            }

            case GIMHVDEBUGREPLY_DHCP_ACK:
            {
                uint32_t const cbDhcpAckPkt = sizeof(g_abDhcpAck);
                if (cbBuf >= cbDhcpAckPkt)
                {
                    memcpy(pvBuf, g_abDhcpAck, cbDhcpAckPkt);
                    PRTNETETHERHDR pEthHdr   = (PRTNETETHERHDR)pvBuf;
                    PRTNETIPV4     pIpHdr    = (PRTNETIPV4)    (pEthHdr + 1);
                    PRTNETUDP      pUdpHdr   = (PRTNETUDP)     ((uint8_t *)pIpHdr + RTNETIPV4_MIN_LEN);
                    PRTNETBOOTP    pBootpHdr = (PRTNETBOOTP)   (pUdpHdr + 1);
                    pBootpHdr->bp_xid = pHv->uBootpXId;

                    rc = VINF_SUCCESS;
                    *pcbRead = cbDhcpAckPkt;
                    pHv->enmDebugReply = GIMHVDEBUGREPLY_DHCP_ACK_SENT;
                    LogRel(("GIM: HyperV: Debug DHCP acknowledged IP address %RTnaipv4, transaction Id %#x\n",
                            pBootpHdr->bp_yiaddr, RT_N2H_U32(pHv->uBootpXId)));
                }
                else
                {
                    rc = VERR_BUFFER_UNDERFLOW;
                    *pcbRead = 0;
                }
                break;
            }

            case GIMHVDEBUGREPLY_ARP_REPLY_SENT:
            case GIMHVDEBUGREPLY_DHCP_OFFER_SENT:
            case GIMHVDEBUGREPLY_DHCP_ACK_SENT:
            {
                rc = VINF_SUCCESS;
                *pcbRead = 0;
                break;
            }

            default:
            {
                AssertMsgFailed(("GIM: HyperV: Invalid/unimplemented debug reply type %u\n", pHv->enmDebugReply));
                rc = VERR_INTERNAL_ERROR_2;
            }
        }
        Assert(rc != VERR_GIM_IPE_1);
    }
    return rc;
}


/**
 * Writes data to the debugger connection, asynchronous.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   pvData      Pointer to the data to be written.
 * @param   cbWrite     Size of the write buffer @a pvData.
 * @param   pcbWritten  Where to store the number of bytes written.
 * @param   fUdpPkt     Whether the debug data in @a pvData is encapsulated in a
 *                      UDP frame.
 *
 * @thread  EMT.
 */
VMMR3_INT_DECL(int) gimR3HvDebugWrite(PVM pVM, void *pvData, uint32_t cbWrite, uint32_t *pcbWritten, bool fUdpPkt)
{
    Assert(cbWrite > 0);

    PGIMHV    pHv        = &pVM->gim.s.u.Hv;
    bool      fIgnorePkt = false;
    uint8_t  *pbData     = (uint8_t *)pvData;
    if (fUdpPkt)
    {
        /*
         * Windows guests sends us ethernet frames over the Hyper-V debug connection.
         * It sends DHCP/ARP queries with zero'd out MAC addresses and requires fudging up the
         * packets somewhere.
         *
         * The Microsoft WinDbg debugger talks UDP and thus only expects the actual debug
         * protocol payload.
         *
         * If the guest is configured with the "nodhcp" option it sends ARP queries with
         * a self-chosen IP and after a couple of attempts of receiving no replies, the guest
         * picks its own IP address. After this, the guest starts sending the UDP packets
         * we require. We thus ignore the initial ARP packets until the guest eventually
         * starts talking UDP. Then we can finally feed the UDP payload over the debug
         * connection.
         */
        if (cbWrite > sizeof(RTNETETHERHDR))
        {
            PCRTNETETHERHDR pEtherHdr = (PCRTNETETHERHDR)pbData;
            if (pEtherHdr->EtherType == RT_H2N_U16_C(RTNET_ETHERTYPE_IPV4))
            {
                if (cbWrite > sizeof(RTNETETHERHDR) + RTNETIPV4_MIN_LEN + RTNETUDP_MIN_LEN)
                {
                    size_t const cbMaxIpHdr = cbWrite - sizeof(RTNETETHERHDR) - sizeof(RTNETUDP) - 1;
                    size_t const cbMaxIpPkt = cbWrite - sizeof(RTNETETHERHDR);
                    PCRTNETIPV4  pIp4Hdr    = (PCRTNETIPV4)(pbData + sizeof(RTNETETHERHDR));
                    bool const   fValidIp4  = RTNetIPv4IsHdrValid(pIp4Hdr, cbMaxIpHdr, cbMaxIpPkt, false /*fChecksum*/);
                    if (   fValidIp4
                        && pIp4Hdr->ip_p == RTNETIPV4_PROT_UDP)
                    {
                        uint32_t const cbIpHdr     = pIp4Hdr->ip_hl * 4;
                        uint32_t const cbMaxUdpPkt = cbWrite - sizeof(RTNETETHERHDR) - cbIpHdr;
                        PCRTNETUDP pUdpHdr       = (PCRTNETUDP)((uint8_t *)pIp4Hdr + cbIpHdr);
                        if (   pUdpHdr->uh_ulen >  RT_H2N_U16(sizeof(RTNETUDP))
                            && pUdpHdr->uh_ulen <= RT_H2N_U16((uint16_t)cbMaxUdpPkt))
                        {
                            /*
                             * Check for DHCP.
                             */
                            size_t const cbUdpPkt = cbMaxIpPkt - cbIpHdr;
                            if (   pUdpHdr->uh_dport == RT_N2H_U16_C(RTNETIPV4_PORT_BOOTPS)
                                && pUdpHdr->uh_sport == RT_N2H_U16_C(RTNETIPV4_PORT_BOOTPC)
                                && cbMaxIpPkt >= cbIpHdr + RTNETUDP_MIN_LEN + RTNETBOOTP_DHCP_MIN_LEN)
                            {
                                PCRTNETBOOTP pDhcpPkt = (PCRTNETBOOTP)(pUdpHdr + 1);
                                uint8_t bMsgType;
                                if (RTNetIPv4IsDHCPValid(pUdpHdr, pDhcpPkt, cbUdpPkt - sizeof(*pUdpHdr), &bMsgType))
                                {
                                    switch (bMsgType)
                                    {
                                        case RTNET_DHCP_MT_DISCOVER:
                                            pHv->enmDebugReply = GIMHVDEBUGREPLY_DHCP_OFFER;
                                            pHv->uBootpXId = pDhcpPkt->bp_xid;
                                            break;
                                        case RTNET_DHCP_MT_REQUEST:
                                            pHv->enmDebugReply = GIMHVDEBUGREPLY_DHCP_ACK;
                                            pHv->uBootpXId = pDhcpPkt->bp_xid;
                                            break;
                                        default:
                                            LogRelMax(5, ("GIM: HyperV: Debug DHCP MsgType %#x not implemented! Packet dropped\n",
                                                          bMsgType));
                                            break;
                                    }
                                }
                                fIgnorePkt = true;
                            }
                            else if (   !pUdpHdr->uh_dport
                                     && !pUdpHdr->uh_sport)
                            {
                                /*
                                 * Extract the UDP payload and pass it to the debugger and record the guest IP address.
                                 * Hyper-V sends UDP debugger packets with source and destination port as 0. If we don't
                                 * filter out the ports here, we would receive BOOTP, NETBIOS and other UDP sub-protocol
                                 * packets which the debugger yells as "Bad packet received from...".
                                 */
                                uint32_t const cbFrameHdr = sizeof(RTNETETHERHDR) + cbIpHdr + sizeof(RTNETUDP);
                                pbData  += cbFrameHdr;
                                cbWrite -= cbFrameHdr;
                                pHv->DbgGuestIp4Addr = pIp4Hdr->ip_src;
                                pHv->enmDebugReply   = GIMHVDEBUGREPLY_UDP;
                            }
                            else
                            {
                                LogFlow(("GIM: HyperV: Ignoring UDP packet not src and dst port 0\n"));
                                fIgnorePkt = true;
                            }
                        }
                        else
                        {
                            LogFlow(("GIM: HyperV: Ignoring malformed UDP packet. cbMaxUdpPkt=%u UdpPkt.len=%u\n", cbMaxUdpPkt,
                                     RT_N2H_U16(pUdpHdr->uh_ulen)));
                            fIgnorePkt = true;
                        }
                    }
                    else
                    {
                        LogFlow(("GIM: HyperV: Ignoring non-IP / non-UDP packet. fValidIp4=%RTbool Proto=%u\n", fValidIp4,
                                  pIp4Hdr->ip_p));
                        fIgnorePkt = true;
                    }
                }
                else
                {
                    LogFlow(("GIM: HyperV: Ignoring IPv4 packet; too short to be valid UDP. cbWrite=%u\n", cbWrite));
                    fIgnorePkt = true;
                }
            }
            else if (pEtherHdr->EtherType == RT_H2N_U16_C(RTNET_ETHERTYPE_ARP))
            {
                /*
                 * Check for targetted ARP query.
                 */
                PCRTNETARPHDR pArpHdr = (PCRTNETARPHDR)(pbData + sizeof(RTNETETHERHDR));
                if (   pArpHdr->ar_hlen  == sizeof(RTMAC)
                    && pArpHdr->ar_plen  == sizeof(RTNETADDRIPV4)
                    && pArpHdr->ar_htype == RT_H2N_U16(RTNET_ARP_ETHER)
                    && pArpHdr->ar_ptype == RT_H2N_U16(RTNET_ETHERTYPE_IPV4))
                {
                    uint16_t uArpOp = pArpHdr->ar_oper;
                    if (uArpOp == RT_H2N_U16_C(RTNET_ARPOP_REQUEST))
                    {
                        PCRTNETARPIPV4 pArpPkt = (PCRTNETARPIPV4)pArpHdr;
                        bool fGratuitous = pArpPkt->ar_spa.u == pArpPkt->ar_tpa.u;
                        if (   !fGratuitous
                            &&  pArpPkt->ar_spa.u == GIMHV_DEBUGCLIENT_IPV4
                            &&  pArpPkt->ar_tpa.u == GIMHV_DEBUGSERVER_IPV4)
                        {
                            pHv->enmDebugReply = GIMHVDEBUGREPLY_ARP_REPLY;
                        }
                    }
                }
                fIgnorePkt = true;
            }
            else
            {
                LogFlow(("GIM: HyperV: Ignoring non-IP packet. Ethertype=%#x\n", RT_N2H_U16(pEtherHdr->EtherType)));
                fIgnorePkt = true;
            }
        }
    }

    if (!fIgnorePkt)
    {
        AssertCompile(sizeof(size_t) >= sizeof(uint32_t));
        size_t cbWriteBuf = cbWrite;
        int rc = GIMR3DebugWrite(pVM, pbData, &cbWriteBuf);
        if (   RT_SUCCESS(rc)
            && cbWriteBuf == cbWrite)
            *pcbWritten = (uint32_t)cbWriteBuf;
        else
            *pcbWritten = 0;
    }
    else
        *pcbWritten = cbWrite;

    return VINF_SUCCESS;
}


/**
 * Performs the HvPostDebugData hypercall.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   prcHv       Where to store the result of the hypercall operation.
 *
 * @thread  EMT.
 */
VMMR3_INT_DECL(int) gimR3HvHypercallPostDebugData(PVM pVM, int *prcHv)
{
    AssertPtr(pVM);
    AssertPtr(prcHv);
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    int    rcHv = GIM_HV_STATUS_OPERATION_DENIED;

    /*
     * Grab the parameters.
     */
    PGIMHVDEBUGPOSTIN pIn = (PGIMHVDEBUGPOSTIN)pHv->pbHypercallIn;
    AssertPtrReturn(pIn, VERR_GIM_IPE_1);
    uint32_t   cbWrite = pIn->cbWrite;
    uint32_t   fFlags = pIn->fFlags;
    uint8_t   *pbData = ((uint8_t *)pIn) + sizeof(PGIMHVDEBUGPOSTIN);

    PGIMHVDEBUGPOSTOUT pOut = (PGIMHVDEBUGPOSTOUT)pHv->pbHypercallOut;

    /*
     * Perform the hypercall.
     */
#if 0
    /* Currently disabled as Windows 10 guest passes us undocumented flags. */
    if (fFlags & ~GIM_HV_DEBUG_POST_OPTIONS_MASK))
        rcHv = GIM_HV_STATUS_INVALID_PARAMETER;
#endif
    if (cbWrite > GIM_HV_DEBUG_MAX_DATA_SIZE)
        rcHv = GIM_HV_STATUS_INVALID_PARAMETER;
    else if (!cbWrite)
    {
        rcHv = GIM_HV_STATUS_SUCCESS;
        pOut->cbPending = 0;
    }
    else if (cbWrite > 0)
    {
        uint32_t cbWritten = 0;
        int rc2 = gimR3HvDebugWrite(pVM, pbData, cbWrite, &cbWritten, pHv->fIsVendorMsHv /*fUdpPkt*/);
        if (   RT_SUCCESS(rc2)
            && cbWritten == cbWrite)
        {
            pOut->cbPending = 0;
            rcHv = GIM_HV_STATUS_SUCCESS;
        }
        else
            rcHv = GIM_HV_STATUS_INSUFFICIENT_BUFFER;
    }

    /*
     * Update the guest memory with result.
     */
    int rc = PGMPhysSimpleWriteGCPhys(pVM, pHv->GCPhysHypercallOut, pHv->pbHypercallOut, sizeof(GIMHVDEBUGPOSTOUT));
    if (RT_FAILURE(rc))
    {
        LogRelMax(10, ("GIM: HyperV: HvPostDebugData failed to update guest memory. rc=%Rrc\n", rc));
        rc = VERR_GIM_HYPERCALL_MEMORY_WRITE_FAILED;
    }

    *prcHv = rcHv;
    return rc;
}


/**
 * Performs the HvRetrieveDebugData hypercall.
 *
 * @returns VBox status code.
 * @param   pVM         The cross context VM structure.
 * @param   prcHv       Where to store the result of the hypercall operation.
 *
 * @thread  EMT.
 */
VMMR3_INT_DECL(int) gimR3HvHypercallRetrieveDebugData(PVM pVM, int *prcHv)
{
    AssertPtr(pVM);
    AssertPtr(prcHv);
    PGIMHV pHv = &pVM->gim.s.u.Hv;
    int    rcHv = GIM_HV_STATUS_OPERATION_DENIED;

    /*
     * Grab the parameters.
     */
    PGIMHVDEBUGRETRIEVEIN pIn = (PGIMHVDEBUGRETRIEVEIN)pHv->pbHypercallIn;
    AssertPtrReturn(pIn, VERR_GIM_IPE_1);
    uint32_t   cbRead = pIn->cbRead;
    uint32_t   fFlags = pIn->fFlags;
    uint64_t   uTimeout = pIn->u64Timeout;
    uint32_t   cMsTimeout = (fFlags & GIM_HV_DEBUG_RETREIVE_LOOP) ? (uTimeout * 100) / RT_NS_1MS_64 : 0;

    PGIMHVDEBUGRETRIEVEOUT pOut = (PGIMHVDEBUGRETRIEVEOUT)pHv->pbHypercallOut;
    AssertPtrReturn(pOut, VERR_GIM_IPE_2);
    uint32_t   *pcbReallyRead = &pOut->cbRead;
    uint32_t   *pcbRemainingRead = &pOut->cbRemaining;
    void       *pvData = ((uint8_t *)pOut) + sizeof(GIMHVDEBUGRETRIEVEOUT);

    /*
     * Perform the hypercall.
     */
    *pcbReallyRead    = 0;
    *pcbRemainingRead = cbRead;
#if 0
    /* Currently disabled as Windows 10 guest passes us undocumented flags. */
    if (fFlags & ~GIM_HV_DEBUG_RETREIVE_OPTIONS_MASK)
        rcHv = GIM_HV_STATUS_INVALID_PARAMETER;
#endif
    if (cbRead > GIM_HV_DEBUG_MAX_DATA_SIZE)
        rcHv = GIM_HV_STATUS_INVALID_PARAMETER;
    else if (fFlags & GIM_HV_DEBUG_RETREIVE_TEST_ACTIVITY)
        rcHv = GIM_HV_STATUS_SUCCESS; /** @todo implement this. */
    else if (!cbRead)
        rcHv = GIM_HV_STATUS_SUCCESS;
    else if (cbRead > 0)
    {
        int rc2 = gimR3HvDebugRead(pVM, pvData, GIM_HV_PAGE_SIZE, cbRead, pcbReallyRead, cMsTimeout,
                                   pHv->fIsVendorMsHv /*fUdpPkt*/);
        Assert(*pcbReallyRead <= cbRead);
        if (   RT_SUCCESS(rc2)
            && *pcbReallyRead > 0)
        {
            *pcbRemainingRead = cbRead -  *pcbReallyRead;
            rcHv = GIM_HV_STATUS_SUCCESS;
        }
        else
            rcHv = GIM_HV_STATUS_NO_DATA;
    }

    /*
     * Update the guest memory with result.
     */
    int rc = PGMPhysSimpleWriteGCPhys(pVM, pHv->GCPhysHypercallOut, pHv->pbHypercallOut,
                                      sizeof(GIMHVDEBUGRETRIEVEOUT) + *pcbReallyRead);
    if (RT_FAILURE(rc))
    {
        LogRelMax(10, ("GIM: HyperV: HvRetrieveDebugData failed to update guest memory. rc=%Rrc\n", rc));
        rc = VERR_GIM_HYPERCALL_MEMORY_WRITE_FAILED;
    }

    *prcHv = rcHv;
    return rc;
}


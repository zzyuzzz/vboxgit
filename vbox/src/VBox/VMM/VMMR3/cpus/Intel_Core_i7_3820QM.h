/* $Id: Intel_Core_i7_3820QM.h 61776 2016-06-20 23:25:06Z vboxsync $ */
/** @file
 * CPU database entry "Intel Core i7-3820QM".
 * Generated at 2013-12-04T12:54:32Z by VBoxCpuReport v4.3.51r91071 on darwin.amd64.
 */

/*
 * Copyright (C) 2013-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_CPUDB_Intel_Core_i7_3820QM
#define VBOX_CPUDB_Intel_Core_i7_3820QM


#ifndef CPUM_DB_STANDALONE
/**
 * CPUID leaves for Intel(R) Core(TM) i7-3820QM CPU @ 2.70GHz.
 */
static CPUMCPUIDLEAF const g_aCpuIdLeaves_Intel_Core_i7_3820QM[] =
{
    { 0x00000000, 0x00000000, 0x00000000, 0x0000000d, 0x756e6547, 0x6c65746e, 0x49656e69, 0 },
    { 0x00000001, 0x00000000, 0x00000000, 0x000306a9, 0x02100800, 0x7fbae3ff, 0xbfebfbff, 0 | CPUMCPUIDLEAF_F_CONTAINS_APIC_ID | CPUMCPUIDLEAF_F_CONTAINS_APIC },
    { 0x00000002, 0x00000000, 0x00000000, 0x76035a01, 0x00f0b2ff, 0x00000000, 0x00ca0000, 0 },
    { 0x00000003, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x00000004, 0x00000000, 0x00000000, 0x1c004121, 0x01c0003f, 0x0000003f, 0x00000000, 0 },
    { 0x00000005, 0x00000000, 0x00000000, 0x00000040, 0x00000040, 0x00000003, 0x00021120, 0 },
    { 0x00000006, 0x00000000, 0x00000000, 0x00000077, 0x00000002, 0x00000009, 0x00000000, 0 },
    { 0x00000007, 0x00000000, 0x00000000, 0x00000000, 0x00000281, 0x00000000, 0x00000000, 0 },
    { 0x00000008, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x00000009, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x0000000a, 0x00000000, 0x00000000, 0x07300403, 0x00000000, 0x00000000, 0x00000603, 0 },
    { 0x0000000b, 0x00000000, 0x00000000, 0x00000001, 0x00000002, 0x00000100, 0x00000002, 0 | CPUMCPUIDLEAF_F_INTEL_TOPOLOGY_SUBLEAVES | CPUMCPUIDLEAF_F_CONTAINS_APIC_ID },
    { 0x0000000c, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x0000000d, 0x00000000, 0x00000000, 0x00000007, 0x00000340, 0x00000340, 0x00000000, 0 },
    { 0x80000000, 0x00000000, 0x00000000, 0x80000008, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x28100800, 0 },
    { 0x80000002, 0x00000000, 0x00000000, 0x20202020, 0x6e492020, 0x286c6574, 0x43202952, 0 },
    { 0x80000003, 0x00000000, 0x00000000, 0x2865726f, 0x20294d54, 0x332d3769, 0x51303238, 0 },
    { 0x80000004, 0x00000000, 0x00000000, 0x5043204d, 0x20402055, 0x30372e32, 0x007a4847, 0 },
    { 0x80000005, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0 },
    { 0x80000006, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x01006040, 0x00000000, 0 },
    { 0x80000007, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000100, 0 },
    { 0x80000008, 0x00000000, 0x00000000, 0x00003024, 0x00000000, 0x00000000, 0x00000000, 0 },
};
#endif /* !CPUM_DB_STANDALONE */


#ifndef CPUM_DB_STANDALONE
/**
 * MSR ranges for Intel(R) Core(TM) i7-3820QM CPU @ 2.70GHz.
 */
static CPUMMSRRANGE const g_aMsrRanges_Intel_Core_i7_3820QM[] =
{
    MFX(0x00000000, "IA32_P5_MC_ADDR", Ia32P5McAddr, Ia32P5McAddr, 0, UINT64_C(0xffffffffffffffe0), 0), /* value=0x1f */
    MFX(0x00000001, "IA32_P5_MC_TYPE", Ia32P5McType, Ia32P5McType, 0, 0, UINT64_MAX), /* value=0x0 */
    MFX(0x00000006, "IA32_MONITOR_FILTER_LINE_SIZE", Ia32MonitorFilterLineSize, Ia32MonitorFilterLineSize, 0, 0, UINT64_C(0xffffffffffff0000)), /* value=0x40 */
    MFX(0x00000010, "IA32_TIME_STAMP_COUNTER", Ia32TimestampCounter, Ia32TimestampCounter, 0, 0, 0),
    MFV(0x00000017, "IA32_PLATFORM_ID", Ia32PlatformId, ReadOnly, UINT64_C(0x10000000000000)),
    MFX(0x0000001b, "IA32_APIC_BASE", Ia32ApicBase, Ia32ApicBase, UINT32_C(0xfee00900), 0, UINT64_C(0xfffffff0000002ff)),
    MFX(0x0000002a, "EBL_CR_POWERON", IntelEblCrPowerOn, ReadOnly, 0, 0, 0), /* value=0x0 */
    MVX(0x0000002e, "I7_UNK_0000_002e", 0, 0x400, UINT64_C(0xfffffffffffffbff)),
    MVX(0x00000033, "TEST_CTL", 0, 0, UINT64_C(0xffffffff7fffffff)),
    MVO(0x00000034, "P6_UNK_0000_0034", 0xe),
    MFO(0x00000035, "MSR_CORE_THREAD_COUNT", IntelI7CoreThreadCount), /* value=0x40008*/
    MVO(0x00000036, "I7_UNK_0000_0036", 0x6c405eec),
    MFO(0x0000003a, "IA32_FEATURE_CONTROL", Ia32FeatureControl), /* value=0xff07 */
    MVX(0x0000003e, "I7_UNK_0000_003e", 0, 0, UINT64_C(0xfffffffffffffffe)),
    MFN(0x00000079, "IA32_BIOS_UPDT_TRIG", WriteOnly, Ia32BiosUpdateTrigger),
    MVX(0x0000008b, "BBL_CR_D3|BIOS_SIGN", UINT64_C(0x1500000000), 0x1, UINT32_C(0xfffffffe)),
    MFO(0x0000009b, "IA32_SMM_MONITOR_CTL", Ia32SmmMonitorCtl), /* value=0x0 */
    MFX(0x000000c1, "IA32_PMC0", Ia32PmcN, Ia32PmcN, 0, ~(uint64_t)UINT32_MAX, 0), /* value=0x0 */
    MFX(0x000000c2, "IA32_PMC1", Ia32PmcN, Ia32PmcN, 0, ~(uint64_t)UINT32_MAX, 0), /* value=0x0 */
    MFX(0x000000c3, "IA32_PMC2", Ia32PmcN, Ia32PmcN, 0, ~(uint64_t)UINT32_MAX, 0), /* value=0x0 */
    MFX(0x000000c4, "IA32_PMC3", Ia32PmcN, Ia32PmcN, 0, ~(uint64_t)UINT32_MAX, 0), /* value=0x0 */
    MVO(0x000000ce, "P6_UNK_0000_00ce", UINT64_C(0x80c10f0011b00)),
    MFX(0x000000e2, "MSR_PKG_CST_CONFIG_CONTROL", IntelPkgCStConfigControl, IntelPkgCStConfigControl, 0, 0, UINT64_C(0xffffffffe1fffbf8)), /* value=0x8405 */
    MFX(0x000000e4, "MSR_PMG_IO_CAPTURE_BASE", IntelPmgIoCaptureBase, IntelPmgIoCaptureBase, 0, 0, UINT64_C(0xfffffffffff80000)), /* value=0x20414 */
    MFX(0x000000e7, "IA32_MPERF", Ia32MPerf, Ia32MPerf, 0, 0x47810, 0), /* value=0x6b`5d075e9c */
    MFX(0x000000e8, "IA32_APERF", Ia32APerf, Ia32APerf, 0, 0x1121880, 0), /* value=0x55`2bec768b */
    MFX(0x000000fe, "IA32_MTRRCAP", Ia32MtrrCap, ReadOnly, 0xd0a, 0, 0), /* value=0xd0a */
    MVX(0x00000102, "I7_IVY_UNK_0000_0102", 0, 0, UINT64_C(0xffffffff7fff8000)),
    MVX(0x00000103, "I7_IVY_UNK_0000_0103", 0, 0, UINT64_C(0xffffffffffffff00)),
    MVX(0x00000104, "I7_IVY_UNK_0000_0104", 0, 0, UINT64_C(0xfffffffffffffffe)),
    MVX(0x00000132, "I7_UNK_0000_0132", UINT64_MAX, 0, 0),
    MVX(0x00000133, "I7_UNK_0000_0133", UINT64_MAX, 0, 0),
    MVX(0x00000134, "I7_UNK_0000_0134", UINT64_MAX, 0, 0),
    MVO(0x0000013c, "TODO_0000_013c", 0x1),
    MVX(0x00000140, "I7_IVY_UNK_0000_0140", 0, 0, UINT64_C(0xfffffffffffffffe)),
    MVX(0x00000142, "I7_IVY_UNK_0000_0142", 0, 0, UINT64_C(0xfffffffffffffffc)),
    MFX(0x00000174, "IA32_SYSENTER_CS", Ia32SysEnterCs, Ia32SysEnterCs, 0, ~(uint64_t)UINT32_MAX, 0), /* value=0xb */
    MFN(0x00000175, "IA32_SYSENTER_ESP", Ia32SysEnterEsp, Ia32SysEnterEsp), /* value=0xffffff80`21af5080 */
    MFN(0x00000176, "IA32_SYSENTER_EIP", Ia32SysEnterEip, Ia32SysEnterEip), /* value=0xffffff80`214ce720 */
    MFX(0x00000179, "IA32_MCG_CAP", Ia32McgCap, ReadOnly, 0xc09, 0, 0), /* value=0xc09 */
    MFX(0x0000017a, "IA32_MCG_STATUS", Ia32McgStatus, Ia32McgStatus, 0, 0, UINT64_C(0xfffffffffffffff8)), /* value=0x0 */
    RSN(0x00000186, 0x00000189, "IA32_PERFEVTSELn", Ia32PerfEvtSelN, Ia32PerfEvtSelN, 0, 0, UINT64_C(0xffffffff00080000)),
    MVX(0x00000194, "CLOCK_FLEX_MAX", 0x180000, 0x1e00ff, UINT64_C(0xffffffffffe00000)),
    MFX(0x00000198, "IA32_PERF_STATUS", Ia32PerfStatus, ReadOnly, UINT64_C(0x240700002400), 0, 0), /* value=0x2407`00002400 */
    MFX(0x00000199, "IA32_PERF_CTL", Ia32PerfCtl, Ia32PerfCtl, 0x2500, 0, 0), /* Might bite. value=0x2500 */
    MFX(0x0000019a, "IA32_CLOCK_MODULATION", Ia32ClockModulation, Ia32ClockModulation, 0, 0, UINT64_C(0xffffffffffffffe0)), /* value=0x0 */
    MFX(0x0000019b, "IA32_THERM_INTERRUPT", Ia32ThermInterrupt, Ia32ThermInterrupt, 0x10, 0, UINT64_C(0xfffffffffe0000e8)), /* value=0x10 */
    MFX(0x0000019c, "IA32_THERM_STATUS", Ia32ThermStatus, Ia32ThermStatus, UINT32_C(0x88340000), UINT32_C(0xf87f0fff), UINT64_C(0xffffffff0780f000)), /* value=0x88340000 */
    MFX(0x0000019d, "IA32_THERM2_CTL", Ia32Therm2Ctl, ReadOnly, 0, 0, 0), /* value=0x0 */
    MFX(0x000001a0, "IA32_MISC_ENABLE", Ia32MiscEnable, Ia32MiscEnable, 0x850089, 0x1080, UINT64_C(0xffffffbbff3aef72)), /* value=0x850089 */
    MFX(0x000001a2, "I7_MSR_TEMPERATURE_TARGET", IntelI7TemperatureTarget, IntelI7TemperatureTarget, 0x691200, 0xffff00, UINT64_C(0xfffffffff00000ff)), /* value=0x691200 */
    MVX(0x000001a4, "I7_UNK_0000_01a4", 0, 0, UINT64_C(0xfffffffffffff7f0)),
    RSN(0x000001a6, 0x000001a7, "I7_MSR_OFFCORE_RSP_n", IntelI7MsrOffCoreResponseN, IntelI7MsrOffCoreResponseN, 0, 0, UINT64_C(0xffffffc000007000)), /* XXX: The range ended earlier than expected! */
    MVX(0x000001a8, "I7_UNK_0000_01a8", 0, 0, UINT64_C(0xfffffffffffffffc)),
    MFX(0x000001aa, "MSR_MISC_PWR_MGMT", IntelI7MiscPwrMgmt, IntelI7MiscPwrMgmt, 0, 0, UINT64_C(0xffffffffffbffffe)), /* value=0x400001 */
    MVX(0x000001ad, "TODO_0000_01ad", 0x23232425, UINT32_MAX, ~(uint64_t)UINT32_MAX),
    MVX(0x000001b0, "IA32_ENERGY_PERF_BIAS", 0x4, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x000001b1, "IA32_PACKAGE_THERM_STATUS", UINT32_C(0x88300000), UINT32_C(0xf87f0fff), UINT64_C(0xffffffff0780f000)),
    MVX(0x000001b2, "IA32_PACKAGE_THERM_INTERRUPT", 0, 0, UINT64_C(0xfffffffffe0000e8)),
    MVO(0x000001c6, "TODO_0000_01c6", 0x3),
    MVX(0x000001c8, "TODO_0000_01c8", 0, 0, UINT64_C(0xfffffffffffffe00)),
    MFX(0x000001c9, "MSR_LASTBRANCH_TOS", IntelLastBranchTos, IntelLastBranchTos, 0, 0, UINT64_C(0xfffffffffffffff0)), /* value=0x8 */
    MFX(0x000001d9, "IA32_DEBUGCTL", Ia32DebugCtl, Ia32DebugCtl, 0, 0, UINT64_C(0xffffffffffff803c)), /* value=0x0 */
    MFO(0x000001db, "P6_LAST_BRANCH_FROM_IP", P6LastBranchFromIp), /* value=0x7fffff7f`a38c2298 */
    MFO(0x000001dc, "P6_LAST_BRANCH_TO_IP", P6LastBranchToIp), /* value=0xffffff80`214b24e0 */
    MFN(0x000001dd, "P6_LAST_INT_FROM_IP", P6LastIntFromIp, P6LastIntFromIp), /* value=0x0 */
    MFN(0x000001de, "P6_LAST_INT_TO_IP", P6LastIntToIp, P6LastIntToIp), /* value=0x0 */
    MVO(0x000001f0, "TODO_0000_01f0", 0x74),
    MVO(0x000001f2, "TODO_0000_01f2", UINT32_C(0x8b000006)),
    MVO(0x000001f3, "TODO_0000_01f3", UINT32_C(0xff800800)),
    MVX(0x000001fc, "TODO_0000_01fc", 0x340047, 0x20, UINT64_C(0xffffffffffc20000)),
    MFX(0x00000200, "IA32_MTRR_PHYS_BASE0", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0, 0, UINT64_C(0xfffffff000000ff8)), /* value=0xc0000000 */
    MFX(0x00000201, "IA32_MTRR_PHYS_MASK0", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0, 0, UINT64_C(0xfffffff0000007ff)), /* value=0xf`c0000800 */
    MFX(0x00000202, "IA32_MTRR_PHYS_BASE1", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x1, 0, UINT64_C(0xfffffff000000ff8)), /* value=0xa0000000 */
    MFX(0x00000203, "IA32_MTRR_PHYS_MASK1", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x1, 0, UINT64_C(0xfffffff0000007ff)), /* value=0xf`e0000800 */
    MFX(0x00000204, "IA32_MTRR_PHYS_BASE2", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x2, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x90000000 */
    MFX(0x00000205, "IA32_MTRR_PHYS_MASK2", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x2, 0, UINT64_C(0xfffffff0000007ff)), /* value=0xf`f0000800 */
    MFX(0x00000206, "IA32_MTRR_PHYS_BASE3", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x3, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x8c000000 */
    MFX(0x00000207, "IA32_MTRR_PHYS_MASK3", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x3, 0, UINT64_C(0xfffffff0000007ff)), /* value=0xf`fc000800 */
    MFX(0x00000208, "IA32_MTRR_PHYS_BASE4", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x4, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x8b000000 */
    MFX(0x00000209, "IA32_MTRR_PHYS_MASK4", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x4, 0, UINT64_C(0xfffffff0000007ff)), /* value=0xf`ff000800 */
    MFX(0x0000020a, "IA32_MTRR_PHYS_BASE5", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x5, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x0 */
    MFX(0x0000020b, "IA32_MTRR_PHYS_MASK5", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x5, 0, UINT64_C(0xfffffff0000007ff)), /* value=0x0 */
    MFX(0x0000020c, "IA32_MTRR_PHYS_BASE6", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x6, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x0 */
    MFX(0x0000020d, "IA32_MTRR_PHYS_MASK6", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x6, 0, UINT64_C(0xfffffff0000007ff)), /* value=0x0 */
    MFX(0x0000020e, "IA32_MTRR_PHYS_BASE7", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x7, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x0 */
    MFX(0x0000020f, "IA32_MTRR_PHYS_MASK7", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x7, 0, UINT64_C(0xfffffff0000007ff)), /* value=0x0 */
    MFX(0x00000210, "IA32_MTRR_PHYS_BASE8", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x8, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x0 */
    MFX(0x00000211, "IA32_MTRR_PHYS_MASK8", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x8, 0, UINT64_C(0xfffffff0000007ff)), /* value=0x0 */
    MFX(0x00000212, "IA32_MTRR_PHYS_BASE9", Ia32MtrrPhysBaseN, Ia32MtrrPhysBaseN, 0x9, 0, UINT64_C(0xfffffff000000ff8)), /* value=0x0 */
    MFX(0x00000213, "IA32_MTRR_PHYS_MASK9", Ia32MtrrPhysMaskN, Ia32MtrrPhysMaskN, 0x9, 0, UINT64_C(0xfffffff0000007ff)), /* value=0x0 */
    MFS(0x00000250, "IA32_MTRR_FIX64K_00000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix64K_00000),
    MFS(0x00000258, "IA32_MTRR_FIX16K_80000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix16K_80000),
    MFS(0x00000259, "IA32_MTRR_FIX16K_A0000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix16K_A0000),
    MFS(0x00000268, "IA32_MTRR_FIX4K_C0000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_C0000),
    MFS(0x00000269, "IA32_MTRR_FIX4K_C8000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_C8000),
    MFS(0x0000026a, "IA32_MTRR_FIX4K_D0000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_D0000),
    MFS(0x0000026b, "IA32_MTRR_FIX4K_D8000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_D8000),
    MFS(0x0000026c, "IA32_MTRR_FIX4K_E0000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_E0000),
    MFS(0x0000026d, "IA32_MTRR_FIX4K_E8000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_E8000),
    MFS(0x0000026e, "IA32_MTRR_FIX4K_F0000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_F0000),
    MFS(0x0000026f, "IA32_MTRR_FIX4K_F8000", Ia32MtrrFixed, Ia32MtrrFixed, GuestMsrs.msr.MtrrFix4K_F8000),
    MFS(0x00000277, "IA32_PAT", Ia32Pat, Ia32Pat, Guest.msrPAT),
    MVX(0x00000280, "TODO_0000_0280", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000281, "TODO_0000_0281", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000282, "TODO_0000_0282", 0, 0x40007fff, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000283, "TODO_0000_0283", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000284, "TODO_0000_0284", 0, 0x40007fff, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000285, "TODO_0000_0285", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000286, "TODO_0000_0286", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000287, "TODO_0000_0287", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x00000288, "TODO_0000_0288", 0, 0, UINT64_C(0xffffffffbfff8000)),
    MVX(0x000002e0, "TODO_0000_02e0", 0, 0, UINT64_C(0xfffffffffffffffc)),
    MFN(0x000002e6, "TODO_0000_02e6", WriteOnly, IgnoreWrite),
    MVX(0x000002e7, "TODO_0000_02e7", 0x1, 0x1, UINT64_C(0xfffffffffffffffe)),
    MFZ(0x000002ff, "IA32_MTRR_DEF_TYPE", Ia32MtrrDefType, Ia32MtrrDefType, GuestMsrs.msr.MtrrDefType, 0, UINT64_C(0xfffffffffffff3f8)),
    MVO(0x00000305, "TODO_0000_0305", 0),
    MVX(0x00000309, "TODO_0000_0309", 0, 0, UINT64_C(0xffff000000000000)),
    MVX(0x0000030a, "TODO_0000_030a", 0, 0, UINT64_C(0xffff000000000000)),
    MVX(0x0000030b, "TODO_0000_030b", 0, 0, UINT64_C(0xffff000000000000)),
    MVO(0x00000345, "TODO_0000_0345", 0x31c3),
    MVX(0x0000038d, "TODO_0000_038d", 0, 0, UINT64_C(0xfffffffffffff000)),
    MVO(0x0000038e, "TODO_0000_038e", UINT64_C(0x8000000000000000)),
    MVX(0x0000038f, "TODO_0000_038f", 0xf, 0, UINT64_C(0xfffffff8fffffff0)),
    MVX(0x00000390, "TODO_0000_0390", 0, UINT64_C(0xe00000070000000f), UINT64_C(0x1ffffff8fffffff0)),
    MVX(0x00000391, "TODO_0000_0391", 0, 0, UINT64_C(0xffffffff1fffffe0)),
    MVX(0x00000392, "TODO_0000_0392", 0, 0xf, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000393, "TODO_0000_0393", 0, 0x3, UINT64_C(0xfffffffffffffffc)),
    MVX(0x00000394, "TODO_0000_0394", 0, 0, UINT64_C(0xffffffffffafffff)),
    MVX(0x00000395, "TODO_0000_0395", 0, 0, UINT64_C(0xffff000000000000)),
    MVO(0x00000396, "TODO_0000_0396", 0x5),
    MVX(0x00000397, "TODO_0000_0397", 0, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x000003b0, "TODO_0000_03b0", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x000003b1, "TODO_0000_03b1", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x000003b2, "TODO_0000_03b2", 0, 0, UINT64_C(0xffffffffc0230000)),
    MVX(0x000003b3, "TODO_0000_03b3", 0, 0, UINT64_C(0xffffffffc0230000)),
    MVX(0x000003f1, "TODO_0000_03f1", 0, 0, UINT64_C(0x7ffffff0fffffff0)),
    MVX(0x000003f6, "TODO_0000_03f6", UINT16_MAX, UINT64_C(0xffffffffffff0000), 0),
    MVO(0x000003f8, "TODO_0000_03f8", 0),
    MVO(0x000003f9, "TODO_0000_03f9", UINT64_C(0x27495a818)),
    MVO(0x000003fa, "TODO_0000_03fa", UINT64_C(0x428fa6c6207)),
    MVO(0x000003fc, "TODO_0000_03fc", 0x389bb693),
    MVO(0x000003fd, "TODO_0000_03fd", 0x13323393),
    MVO(0x000003fe, "TODO_0000_03fe", UINT64_C(0x48d7ffc9bd1)),
    RFN(0x00000400, 0x00000423, "IA32_MCi_CTL_STATUS_ADDR_MISC", Ia32McCtlStatusAddrMiscN, Ia32McCtlStatusAddrMiscN),
    MVO(0x00000480, "TODO_0000_0480", UINT64_C(0xda040000000010)),
    MVO(0x00000481, "TODO_0000_0481", UINT64_C(0x7f00000016)),
    MVO(0x00000482, "TODO_0000_0482", UINT64_C(0xfff9fffe0401e172)),
    MVO(0x00000483, "TODO_0000_0483", UINT64_C(0x7fffff00036dff)),
    MVO(0x00000484, "TODO_0000_0484", UINT64_C(0xffff000011ff)),
    MVO(0x00000485, "TODO_0000_0485", 0x100401e5),
    MVO(0x00000486, "TODO_0000_0486", UINT32_C(0x80000021)),
    MVO(0x00000487, "TODO_0000_0487", UINT32_MAX),
    MVO(0x00000488, "TODO_0000_0488", 0x2000),
    MVO(0x00000489, "TODO_0000_0489", 0x1767ff),
    MVO(0x0000048a, "TODO_0000_048a", 0x2a),
    MVO(0x0000048b, "TODO_0000_048b", UINT64_C(0x8ff00000000)),
    MVO(0x0000048c, "TODO_0000_048c", UINT64_C(0xf0106114141)),
    MVO(0x0000048d, "TODO_0000_048d", UINT64_C(0x7f00000016)),
    MVO(0x0000048e, "TODO_0000_048e", UINT64_C(0xfff9fffe04006172)),
    MVO(0x0000048f, "TODO_0000_048f", UINT64_C(0x7fffff00036dfb)),
    MVO(0x00000490, "TODO_0000_0490", UINT64_C(0xffff000011fb)),
    MVX(0x000004c1, "TODO_0000_04c1", 0, 0, UINT64_C(0xffff000000000000)),
    MVX(0x000004c2, "TODO_0000_04c2", 0, 0, UINT64_C(0xffff000000000000)),
    MVX(0x000004c3, "TODO_0000_04c3", 0, 0, UINT64_C(0xffff000000000000)),
    MVX(0x000004c4, "TODO_0000_04c4", 0, 0, UINT64_C(0xffff000000000000)),
    MFN(0x00000600, "IA32_DS_AREA", Ia32DsArea, Ia32DsArea), /* value=0x0 */
    MVX(0x00000601, "TODO_0000_0601", UINT64_C(0x1814149480000380), UINT32_C(0x80001fff), 0x7fffe000),
    MVX(0x00000602, "TODO_0000_0602", UINT64_C(0x1814149480000170), UINT32_C(0x80001fff), 0x7fffe000),
    MVX(0x00000603, "TODO_0000_0603", UINT32_C(0x80303030), UINT32_C(0x80ffffff), UINT64_C(0xffffffff7f000000)),
    MVX(0x00000604, "TODO_0000_0604", UINT32_C(0x80646464), UINT32_C(0x80ffffff), UINT64_C(0xffffffff7f000000)),
    MVO(0x00000606, "TODO_0000_0606", 0xa1003),
    MVX(0x0000060a, "TODO_0000_060a", 0x8894, 0, UINT64_C(0xffffffffffff6000)),
    MVX(0x0000060b, "TODO_0000_060b", 0x88a9, 0, UINT64_C(0xffffffffffff6000)),
    MVX(0x0000060c, "TODO_0000_060c", 0x88c6, 0, UINT64_C(0xffffffffffff6000)),
    MVO(0x0000060d, "TODO_0000_060d", UINT64_C(0xd0fd23dd9)),
    MVX(0x00000610, "TODO_0000_0610", UINT64_C(0x800083e800dd8320), UINT64_C(0x80ffffff00ffffff), UINT64_C(0x7f000000ff000000)),
    MVO(0x00000611, "TODO_0000_0611", 0x2ed06e3b),
    MVO(0x00000614, "TODO_0000_0614", 0x1200168),
    MVX(0x00000638, "TODO_0000_0638", UINT32_C(0x80000000), UINT32_C(0x80ffffff), UINT64_C(0xffffffff7f000000)),
    MVO(0x00000639, "TODO_0000_0639", 0x106344fd),
    MVX(0x0000063a, "TODO_0000_063a", 0, 0, UINT64_C(0xffffffffffffffe0)),
    MVX(0x00000640, "TODO_0000_0640", UINT32_C(0x80000000), UINT32_C(0x80ffffff), UINT64_C(0xffffffff7f000000)),
    MVO(0x00000641, "TODO_0000_0641", 0xb39e93),
    MVX(0x00000642, "TODO_0000_0642", 0x10, 0, UINT64_C(0xffffffffffffffe0)),
    MVO(0x00000648, "TODO_0000_0648", 0x1b),
    MVO(0x00000649, "TODO_0000_0649", UINT64_C(0x120000000000000)),
    MVO(0x0000064a, "TODO_0000_064a", UINT64_C(0x120000000000000)),
    MVO(0x0000064b, "TODO_0000_064b", UINT32_C(0x80000000)),
    MVX(0x0000064c, "TODO_0000_064c", UINT32_C(0x80000000), UINT32_C(0x800000ff), UINT64_C(0xffffffff7fffff00)),
    MVX(0x00000680, "TODO_0000_0680", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000681, "TODO_0000_0681", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000682, "TODO_0000_0682", UINT64_C(0x7fffff7fa38c2289), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000683, "TODO_0000_0683", UINT64_C(0x7fffff80214b24cb), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000684, "TODO_0000_0684", UINT64_C(0x7fffff7fa38c2298), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000685, "TODO_0000_0685", UINT64_C(0x7fffff80214b24ee), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000686, "TODO_0000_0686", UINT64_C(0x7fffff7fa38c2289), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000687, "TODO_0000_0687", UINT64_C(0x7fffff80214b24cb), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000688, "TODO_0000_0688", UINT64_C(0x7fffff7fa38c2298), 0, UINT64_C(0x7fff800000000000)),
    MVX(0x00000689, "TODO_0000_0689", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x0000068a, "TODO_0000_068a", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x0000068b, "TODO_0000_068b", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x0000068c, "TODO_0000_068c", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x0000068d, "TODO_0000_068d", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x0000068e, "TODO_0000_068e", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x0000068f, "TODO_0000_068f", 0, 0, UINT64_C(0x7fff800000000000)),
    MVX(0x000006c0, "TODO_0000_06c0", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c1, "TODO_0000_06c1", UINT64_C(0xffffff7fa38c227f), 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c2, "TODO_0000_06c2", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c3, "TODO_0000_06c3", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c4, "TODO_0000_06c4", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c5, "TODO_0000_06c5", UINT64_C(0xffffff7fa38c227f), 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c6, "TODO_0000_06c6", UINT64_C(0xffffff80214b24c0), 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c7, "TODO_0000_06c7", UINT64_C(0xffffff7fa38c228f), 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c8, "TODO_0000_06c8", UINT64_C(0xffffff80214b24e0), 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006c9, "TODO_0000_06c9", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006ca, "TODO_0000_06ca", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006cb, "TODO_0000_06cb", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006cc, "TODO_0000_06cc", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006cd, "TODO_0000_06cd", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006ce, "TODO_0000_06ce", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006cf, "TODO_0000_06cf", 0, 0, UINT64_C(0xffff800000000000)),
    MVX(0x000006e0, "TODO_0000_06e0", UINT64_C(0x535157ca1ca), 0x80000, 0),
    MVX(0x00000700, "TODO_0000_0700", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000701, "TODO_0000_0701", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000702, "TODO_0000_0702", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000703, "TODO_0000_0703", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000704, "TODO_0000_0704", 0, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000705, "TODO_0000_0705", 0, 0xf, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000706, "TODO_0000_0706", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000707, "TODO_0000_0707", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000708, "TODO_0000_0708", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000709, "TODO_0000_0709", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000710, "TODO_0000_0710", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000711, "TODO_0000_0711", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000712, "TODO_0000_0712", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000713, "TODO_0000_0713", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000714, "TODO_0000_0714", 0, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000715, "TODO_0000_0715", 0, 0xf, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000716, "TODO_0000_0716", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000717, "TODO_0000_0717", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000718, "TODO_0000_0718", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000719, "TODO_0000_0719", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000720, "TODO_0000_0720", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000721, "TODO_0000_0721", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000722, "TODO_0000_0722", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000723, "TODO_0000_0723", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000724, "TODO_0000_0724", 0, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000725, "TODO_0000_0725", 0, 0xf, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000726, "TODO_0000_0726", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000727, "TODO_0000_0727", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000728, "TODO_0000_0728", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000729, "TODO_0000_0729", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000730, "TODO_0000_0730", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000731, "TODO_0000_0731", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000732, "TODO_0000_0732", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000733, "TODO_0000_0733", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000734, "TODO_0000_0734", 0, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000735, "TODO_0000_0735", 0, 0xf, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000736, "TODO_0000_0736", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000737, "TODO_0000_0737", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000738, "TODO_0000_0738", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000739, "TODO_0000_0739", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000740, "TODO_0000_0740", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000741, "TODO_0000_0741", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000742, "TODO_0000_0742", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000743, "TODO_0000_0743", 0, 0, UINT64_C(0xffffffffe0230000)),
    MVX(0x00000744, "TODO_0000_0744", 0, 0, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000745, "TODO_0000_0745", 0, 0xf, UINT64_C(0xfffffffffffffff0)),
    MVX(0x00000746, "TODO_0000_0746", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000747, "TODO_0000_0747", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000748, "TODO_0000_0748", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000749, "TODO_0000_0749", 0, 0, UINT64_C(0xfffff00000000000)),
    MVX(0x00000c80, "TODO_0000_0c80", 0, 0, 0),
    MVX(0x00000c81, "TODO_0000_0c81", 0, 0, 0),
    MVX(0x00000c82, "TODO_0000_0c82", 0, ~(uint64_t)UINT32_MAX, 0),
    MVX(0x00000c83, "TODO_0000_0c83", 0, ~(uint64_t)UINT32_MAX, 0),
    MFX(0xc0000080, "AMD64_EFER", Amd64Efer, Amd64Efer, 0xd01, 0x400, UINT64_C(0xfffffffffffff2fe)),
    MFX(0xc0000081, "AMD64_STAR", Amd64SyscallTarget, Amd64SyscallTarget, 0, 0, 0), /* value=0x1b0008`00000000 */
    MFN(0xc0000082, "AMD64_STAR64", Amd64LongSyscallTarget, Amd64LongSyscallTarget), /* value=0xffffff80`214ce6c0 */
    MFN(0xc0000083, "AMD64_STARCOMPAT", Amd64CompSyscallTarget, Amd64CompSyscallTarget), /* value=0x0 */
    MFX(0xc0000084, "AMD64_SYSCALL_FLAG_MASK", Amd64SyscallFlagMask, Amd64SyscallFlagMask, 0, ~(uint64_t)UINT32_MAX, 0), /* value=0x4700 */
    MFN(0xc0000100, "AMD64_FS_BASE", Amd64FsBase, Amd64FsBase), /* value=0x0 */
    MFN(0xc0000101, "AMD64_GS_BASE", Amd64GsBase, Amd64GsBase), /* value=0xffffff81`e942f000 */
    MFN(0xc0000102, "AMD64_KERNEL_GS_BASE", Amd64KernelGsBase, Amd64KernelGsBase), /* value=0x7fff`7ccad1e0 */
    MFX(0xc0000103, "AMD64_TSC_AUX", Amd64TscAux, Amd64TscAux, 0, 0, ~(uint64_t)UINT32_MAX), /* value=0x0 */
};
#endif /* !CPUM_DB_STANDALONE */


/**
 * Database entry for Intel(R) Core(TM) i7-3820QM CPU @ 2.70GHz.
 */
static CPUMDBENTRY const g_Entry_Intel_Core_i7_3820QM =
{
    /*.pszName          = */ "Intel Core i7-3820QM",
    /*.pszFullName      = */ "Intel(R) Core(TM) i7-3820QM CPU @ 2.70GHz",
    /*.enmVendor        = */ CPUMCPUVENDOR_INTEL,
    /*.uFamily          = */ 6,
    /*.uModel           = */ 58,
    /*.uStepping        = */ 9,
    /*.enmMicroarch     = */ kCpumMicroarch_Intel_Core7_IvyBridge,
    /*.uScalableBusFreq = */ CPUM_SBUSFREQ_UNKNOWN,
    /*.fFlags           = */ 0,
    /*.cMaxPhysAddrWidth= */ 36,
    /*.paCpuIdLeaves    = */ NULL_ALONE(g_aCpuIdLeaves_Intel_Core_i7_3820QM),
    /*.cCpuIdLeaves     = */ ZERO_ALONE(RT_ELEMENTS(g_aCpuIdLeaves_Intel_Core_i7_3820QM)),
    /*.enmUnknownCpuId  = */ CPUMUNKNOWNCPUID_LAST_STD_LEAF_WITH_ECX,
    /*.DefUnknownCpuId  = */ { 0x00000007, 0x00000340, 0x00000340, 0x00000000 },
    /*.fMsrMask         = */ UINT32_MAX,
    /*.apaMsrRanges[]   = */
    {
        NULL_ALONE(g_aMsrRanges_Intel_Core_i7_3820QM),
        NULL,
        NULL,
        NULL,
        NULL,
    }
};

#endif /* !VBOX_DB_Intel_Core_i7_3820QM */


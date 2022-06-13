/* $Id: bs3kit-mangling-code-undef.h 90470 2021-08-02 11:21:43Z vboxsync $ */
/** @file
 * BS3Kit - Undefining function mangling - automatically generated by the bs3kit-mangling-code-undef.h makefile rule.
 */

/*
 * Copyright (C) 2007-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#undef Bs3A20Disable
#undef Bs3A20DisableViaKbd
#undef Bs3A20DisableViaPortA
#undef Bs3A20Enable
#undef Bs3A20EnableViaKbd
#undef Bs3A20EnableViaPortA
#undef Bs3ExtCtxAlloc
#undef Bs3ExtCtxCopy
#undef Bs3ExtCtxFree
#undef Bs3ExtCtxGetSize
#undef Bs3ExtCtxInit
#undef Bs3ExtCtxRestore
#undef Bs3ExtCtxSave
#undef Bs3GetCpuVendor
#undef Bs3GetModeName
#undef Bs3GetModeNameShortLower
#undef Bs3KbdRead
#undef Bs3KbdWait
#undef Bs3KbdWrite
#undef Bs3MemAlloc
#undef Bs3MemAllocZ
#undef Bs3MemChr
#undef Bs3MemCmp
#undef Bs3MemCpy
#undef Bs3MemFree
#undef Bs3MemGuardedTestPageAlloc
#undef Bs3MemGuardedTestPageAllocEx
#undef Bs3MemGuardedTestPageFree
#undef Bs3MemMove
#undef Bs3MemPCpy
#undef Bs3MemPrintInfo
#undef Bs3MemSet
#undef Bs3MemZero
#undef Bs3PagingAlias
#undef bs3PagingGetLegacyPte
#undef bs3PagingGetPaePte
#undef Bs3PagingGetPte
#undef Bs3PagingInitRootForLM
#undef Bs3PagingInitRootForPAE
#undef Bs3PagingInitRootForPP
#undef Bs3PagingProtect
#undef Bs3PagingProtectPtr
#undef Bs3PagingQueryAddressInfo
#undef Bs3PagingSetupCanonicalTraps
#undef Bs3PagingUnalias
#undef Bs3Panic
#undef Bs3PicMaskAll
#undef Bs3PicSetup
#undef Bs3PicUpdateMask
#undef Bs3PitDisable
#undef Bs3PitSetupAndEnablePeriodTimer
#undef Bs3PrintChr
#undef Bs3Printf
#undef Bs3PrintfV
#undef Bs3PrintStr
#undef Bs3PrintStrN
#undef Bs3PrintU32
#undef Bs3PrintX32
#undef Bs3RegCtxConvertToRingX
#undef Bs3RegCtxPrint
#undef Bs3RegCtxRestore
#undef Bs3RegCtxSave
#undef Bs3RegCtxSaveEx
#undef Bs3RegCtxSetGrpDsFromCurPtr
#undef Bs3RegCtxSetGrpSegFromCurPtr
#undef Bs3RegCtxSetGrpSegFromFlat
#undef Bs3RegCtxSetRipCsFromCurPtr
#undef Bs3RegCtxSetRipCsFromFlat
#undef Bs3RegCtxSetRipCsFromLnkPtr
#undef Bs3RegGetCr0
#undef Bs3RegGetCr2
#undef Bs3RegGetCr3
#undef Bs3RegGetCr4
#undef Bs3RegGetDr0
#undef Bs3RegGetDr1
#undef Bs3RegGetDr2
#undef Bs3RegGetDr3
#undef Bs3RegGetDr6
#undef Bs3RegGetDr7
#undef Bs3RegGetDrX
#undef Bs3RegGetLdtr
#undef Bs3RegGetTr
#undef Bs3RegSetCr0
#undef Bs3RegSetCr2
#undef Bs3RegSetCr3
#undef Bs3RegSetCr4
#undef Bs3RegSetDr0
#undef Bs3RegSetDr1
#undef Bs3RegSetDr2
#undef Bs3RegSetDr3
#undef Bs3RegSetDr6
#undef Bs3RegSetDr7
#undef Bs3RegSetDrX
#undef Bs3RegSetLdtr
#undef Bs3RegSetTr
#undef Bs3SelFar32ToFlat32
#undef Bs3SelFar32ToFlat32NoClobber
#undef Bs3SelFlatCodeToProtFar16
#undef Bs3SelFlatCodeToRealMode
#undef Bs3SelFlatDataToProtFar16
#undef Bs3SelFlatDataToRealMode
#undef Bs3SelLnkPtrToCurPtr
#undef Bs3SelProtFar16DataToFlat
#undef Bs3SelProtFar16DataToRealMode
#undef Bs3SelProtFar32ToFlat32
#undef Bs3SelProtModeCodeToRealMode
#undef Bs3SelRealModeCodeToFlat
#undef Bs3SelRealModeCodeToProtMode
#undef Bs3SelRealModeDataToFlat
#undef Bs3SelRealModeDataToProtFar16
#undef Bs3SelSetup16BitCode
#undef Bs3SelSetup16BitData
#undef Bs3Shutdown
#undef Bs3SlabAlloc
#undef Bs3SlabAllocEx
#undef Bs3SlabFree
#undef Bs3SlabInit
#undef Bs3SlabListAdd
#undef Bs3SlabListAlloc
#undef Bs3SlabListAllocEx
#undef Bs3SlabListFree
#undef Bs3SlabListInit
#undef Bs3StrCpy
#undef Bs3StrFormatV
#undef Bs3StrLen
#undef Bs3StrNLen
#undef Bs3StrPrintf
#undef Bs3StrPrintfV
#undef Bs3SwitchFromV86To16BitAndCallC
#undef Bs3TestCheckRegCtxEx
#undef Bs3TestFailed
#undef Bs3TestFailedF
#undef Bs3TestFailedV
#undef Bs3TestHostPrintf
#undef Bs3TestHostPrintfV
#undef Bs3TestInit
#undef Bs3TestNow
#undef Bs3TestPrintf
#undef Bs3TestPrintfV
#undef Bs3TestSkipped
#undef Bs3TestSkippedF
#undef Bs3TestSkippedV
#undef Bs3TestSub
#undef Bs3TestSubDone
#undef Bs3TestSubErrorCount
#undef Bs3TestSubF
#undef Bs3TestSubV
#undef Bs3TestTerm
#undef Bs3TestValue
#undef Bs3Trap16Init
#undef Bs3Trap16InitEx
#undef Bs3Trap16SetGate
#undef Bs3Trap32Init
#undef Bs3Trap32SetGate
#undef Bs3Trap64Init
#undef Bs3Trap64SetGate
#undef Bs3TrapDefaultHandler
#undef Bs3TrapPrintFrame
#undef Bs3TrapReInit
#undef Bs3TrapRmV86Init
#undef Bs3TrapRmV86InitEx
#undef Bs3TrapRmV86SetGate
#undef Bs3TrapSetDpl
#undef Bs3TrapSetHandler
#undef Bs3TrapSetHandlerEx
#undef Bs3TrapSetJmp
#undef Bs3TrapSetJmpAndRestore
#undef Bs3TrapUnsetJmp
#undef Bs3UInt32Div
#undef Bs3UInt64Div
#undef Bs3UtilSetFullGdtr
#undef Bs3UtilSetFullIdtr
#ifndef BS3_CMN_ONLY
# undef Bs3CpuDetect
# undef Bs3SwitchTo32BitAndCallC
# undef Bs3TestDoModes
# undef Bs3TestDoModesByMax
# undef Bs3TestDoModesByOne
# undef Bs3TrapInit
#endif /* !BS3_CMN_ONLY */

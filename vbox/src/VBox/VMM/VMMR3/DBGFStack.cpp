/* $Id: DBGFStack.cpp 73388 2018-07-27 15:20:29Z vboxsync $ */
/** @file
 * DBGF - Debugger Facility, Call Stack Analyser.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_DBGF
#include <VBox/vmm/dbgf.h>
#include <VBox/vmm/selm.h>
#include <VBox/vmm/mm.h>
#include "DBGFInternal.h"
#include <VBox/vmm/vm.h>
#include <VBox/vmm/uvm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/param.h>
#include <iprt/assert.h>
#include <iprt/alloca.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include <iprt/formats/pecoff.h>


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/
/**
 * Unwind context.
 *
 * @note Using a constructor and destructor here for simple+safe cleanup.
 *
 * @todo Generalize and move to IPRT or some such place.
 */
typedef struct DBGFUNWINDCTX
{
    PUVM        m_pUVM;
    VMCPUID     m_idCpu;
    RTDBGAS     m_hAs;
    uint64_t    m_uPc;
    uint64_t    m_aGprs[16];

    RTDBGMOD    m_hCached;
    RTUINTPTR   m_uCachedMapping;
    RTUINTPTR   m_cbCachedMapping;
    uint8_t    *m_pbCachedInfo;
    size_t      m_cbCachedInfo;

    /** Function table for PE/AMD64 (entire m_pbCachedInfo) . */
    PCIMAGE_RUNTIME_FUNCTION_ENTRY  m_paFunctions;
    /** Number functions in m_paFunctions. */
    size_t                          m_cFunctions;

    DBGFUNWINDCTX(PUVM pUVM, VMCPUID idCpu, PCCPUMCTX pInitialCtx, RTDBGAS hAs)
    {
        if (pInitialCtx)
        {
            m_aGprs[X86_GREG_xAX] = pInitialCtx->rax;
            m_aGprs[X86_GREG_xCX] = pInitialCtx->rcx;
            m_aGprs[X86_GREG_xDX] = pInitialCtx->rdx;
            m_aGprs[X86_GREG_xBX] = pInitialCtx->rbx;
            m_aGprs[X86_GREG_xSP] = pInitialCtx->rsp;
            m_aGprs[X86_GREG_xBP] = pInitialCtx->rbp;
            m_aGprs[X86_GREG_xSI] = pInitialCtx->rsi;
            m_aGprs[X86_GREG_xDI] = pInitialCtx->rdi;
            m_aGprs[X86_GREG_x8 ] = pInitialCtx->r8;
            m_aGprs[X86_GREG_x9 ] = pInitialCtx->r9;
            m_aGprs[X86_GREG_x10] = pInitialCtx->r10;
            m_aGprs[X86_GREG_x11] = pInitialCtx->r11;
            m_aGprs[X86_GREG_x12] = pInitialCtx->r12;
            m_aGprs[X86_GREG_x13] = pInitialCtx->r13;
            m_aGprs[X86_GREG_x14] = pInitialCtx->r14;
            m_aGprs[X86_GREG_x15] = pInitialCtx->r15;
            m_uPc                 = pInitialCtx->rip;
        }
        else
        {
            RT_BZERO(m_aGprs, sizeof(m_aGprs));
            m_uPc = 0;
        }
        m_pUVM            = pUVM;
        m_idCpu           = idCpu;
        m_hAs             = hAs;

        m_hCached         = NIL_RTDBGMOD;
        m_uCachedMapping  = 0;
        m_cbCachedMapping = 0;
        m_pbCachedInfo    = NULL;
        m_cbCachedInfo    = 0;
        m_paFunctions     = NULL;
        m_cFunctions      = 0;
    }

    ~DBGFUNWINDCTX();

} DBGFUNWINDCTX;
/** Pointer to unwind context. */
typedef DBGFUNWINDCTX *PDBGFUNWINDCTX;


static void dbgfR3UnwindCtxFlushCache(PDBGFUNWINDCTX pUnwindCtx)
{
    if (pUnwindCtx->m_hCached != NIL_RTDBGMOD)
    {
        RTDbgModRelease(pUnwindCtx->m_hCached);
        pUnwindCtx->m_hCached = NIL_RTDBGMOD;
    }
    if (pUnwindCtx->m_pbCachedInfo)
    {
        RTMemFree(pUnwindCtx->m_pbCachedInfo);
        pUnwindCtx->m_pbCachedInfo = NULL;
    }
    pUnwindCtx->m_cbCachedInfo = 0;
    pUnwindCtx->m_paFunctions  = NULL;
    pUnwindCtx->m_cFunctions   = 0;
}


DBGFUNWINDCTX::~DBGFUNWINDCTX()
{
    dbgfR3UnwindCtxFlushCache(this);
}


static bool dbgfR3UnwindCtxSetPcAndSp(PDBGFUNWINDCTX pUnwindCtx, PCDBGFADDRESS pAddrPC, PCDBGFADDRESS pAddrStack)
{
    pUnwindCtx->m_uPc                 = pAddrPC->FlatPtr;
    pUnwindCtx->m_aGprs[X86_GREG_xSP] = pAddrStack->FlatPtr;
    return true;
}


/**
 * Try read a 64-bit value off the stack.
 *
 * @param   pUnwindCtx      The unwind context.
 * @param   uSrcAddr        The stack address.
 * @param   puDst           The read destination.
 */
static void dbgfR3UnwindCtxLoadU64(PDBGFUNWINDCTX pUnwindCtx, uint64_t uSrcAddr, uint64_t *puDst)
{
    DBGFADDRESS SrcAddr;
    DBGFR3MemRead(pUnwindCtx->m_pUVM, pUnwindCtx->m_idCpu,
                  DBGFR3AddrFromFlat(pUnwindCtx->m_pUVM, &SrcAddr, uSrcAddr),
                  puDst, sizeof(*puDst));
}


/**
 * Binary searches the lookup table.
 *
 * @returns RVA of unwind info on success, UINT32_MAX on failure.
 * @param   paFunctions     The table to lookup @a offFunctionRva in.
 * @param   iEnd            Size of the table.
 * @param   uRva            The RVA of the function we want.
 */
DECLINLINE(PCIMAGE_RUNTIME_FUNCTION_ENTRY)
dbgfR3UnwindCtxLookupUnwindInfoRva(PCIMAGE_RUNTIME_FUNCTION_ENTRY paFunctions, size_t iEnd, uint32_t uRva)
{
    size_t iBegin = 0;
    while (iBegin < iEnd)
    {
        size_t const i = iBegin  + (iEnd - iBegin) / 2;
        PCIMAGE_RUNTIME_FUNCTION_ENTRY pEntry = &paFunctions[i];
        if (uRva < pEntry->BeginAddress)
            iEnd = i;
        else if (uRva > pEntry->EndAddress)
            iBegin = i + 1;
        else
            return pEntry;
    }
    return NULL;
}


static bool dbgfR3UnwindCtxDoOneFrameCached(PDBGFUNWINDCTX pUnwindCtx, uint32_t uRvaRip, PDBGFADDRESS pAddrFrame)
{
    /*
     * Lookup the unwind info RVA and try read it.
     */
    PCIMAGE_RUNTIME_FUNCTION_ENTRY pEntry = dbgfR3UnwindCtxLookupUnwindInfoRva(pUnwindCtx->m_paFunctions,
                                                                               pUnwindCtx->m_cFunctions, uRvaRip);
    if (pEntry)
    {
        unsigned iFrameReg   = ~0U;
        unsigned offFrameReg = 0;

        for (;;)
        {
            /*
             * Get the info.
             */
            union
            {
                uint32_t uRva;
                uint8_t  ab[  RT_OFFSETOF(IMAGE_UNWIND_INFO, aOpcodes)
                            + sizeof(IMAGE_UNWIND_CODE) * 256
                            + sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)];
            } uBuf;

            uBuf.uRva = pEntry->UnwindInfoAddress;
            size_t cbBuf = sizeof(uBuf);
            int rc = RTDbgModImageQueryProp(pUnwindCtx->m_hCached, RTLDRPROP_UNWIND_INFO, &uBuf, cbBuf, &cbBuf);
            if (RT_FAILURE(rc))
                return false;

            /*
             * Check the info.
             */
            ASMCompilerBarrier(); /* we're aliasing */
            PCIMAGE_UNWIND_INFO pInfo = (PCIMAGE_UNWIND_INFO)&uBuf;

            if (pInfo->Version != 1)
                return false;

            /*
             * Execute the opcodes.
             */
            unsigned const cOpcodes = pInfo->CountOfCodes;
            unsigned       iOpcode  = 0;

            /* First, skip opcodes that doesn't apply to us if we're the prolog. */
            uint32_t offPc = uRvaRip - pEntry->BeginAddress;
            if (offPc < pInfo->SizeOfProlog)
                while (iOpcode < cOpcodes && pInfo->aOpcodes[iOpcode].u.CodeOffset > offPc)
                    iOpcode++;

            /* Execute. */
            if (pInfo->FrameRegister != 0)
            {
                iFrameReg   = pInfo->FrameRegister;
                offFrameReg = pInfo->FrameOffset * 16;
            }
            while (iOpcode < cOpcodes)
            {
                Assert(pInfo->aOpcodes[iOpcode].u.CodeOffset <= offPc);
                switch (pInfo->aOpcodes[iOpcode].u.UnwindOp)
                {
                    case IMAGE_AMD64_UWOP_PUSH_NONVOL:
                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8;
                        dbgfR3UnwindCtxLoadU64(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP],
                                               &pUnwindCtx->m_aGprs[pInfo->aOpcodes[iOpcode].u.OpInfo]);
                        iOpcode++;
                        break;

                    case IMAGE_AMD64_UWOP_ALLOC_LARGE:
                        iOpcode += 3;
                        AssertBreak(iOpcode <= cOpcodes);
                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= *(uint32_t const *)&pInfo->aOpcodes[iOpcode - 2];
                        break;

                    case IMAGE_AMD64_UWOP_ALLOC_SMALL:
                        AssertBreak(iOpcode <= cOpcodes);
                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= pInfo->aOpcodes[iOpcode].u.OpInfo * 8 + 8;
                        iOpcode++;
                        break;

                    case IMAGE_AMD64_UWOP_SET_FPREG:
                        iFrameReg = pInfo->aOpcodes[iOpcode].u.OpInfo;
                        offFrameReg = pInfo->FrameOffset * 16;
                        break;

                    case IMAGE_AMD64_UWOP_SAVE_NONVOL:
                    case IMAGE_AMD64_UWOP_SAVE_NONVOL_FAR:
                    {
                        bool const     fFar  = pInfo->aOpcodes[iOpcode].u.UnwindOp == IMAGE_AMD64_UWOP_SAVE_NONVOL_FAR;
                        unsigned const iGreg = pInfo->aOpcodes[iOpcode].u.OpInfo;
                        uint32_t       off   = 0;
                        iOpcode++;
                        if (iOpcode < cOpcodes)
                        {
                            off = pInfo->aOpcodes[iOpcode].FrameOffset;
                            iOpcode++;
                            if (fFar && iOpcode < cOpcodes)
                            {
                                off |= (uint32_t)pInfo->aOpcodes[iOpcode].FrameOffset << 16;
                                iOpcode++;
                            }
                        }
                        off *= 8;
                        dbgfR3UnwindCtxLoadU64(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP] + off, &pUnwindCtx->m_aGprs[iGreg]);
                        break;
                    }

                    case IMAGE_AMD64_UWOP_SAVE_XMM128:
                        iOpcode += 2;
                        break;

                    case IMAGE_AMD64_UWOP_SAVE_XMM128_FAR:
                        iOpcode += 3;
                        break;

                    case IMAGE_AMD64_UWOP_PUSH_MACHFRAME:
                    {
                        Assert(pInfo->aOpcodes[iOpcode].u.OpInfo <= 1);
                        if (pInfo->aOpcodes[iOpcode].u.OpInfo)
                            pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8; /* error code */

                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8; /* RIP */
                        dbgfR3UnwindCtxLoadU64(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP], &pUnwindCtx->m_uPc);

                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8; /* CS */
                        //dbgfR3UnwindCtxLoadU16(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP], &pUnwindCtx->m_uCs);

                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8; /* EFLAGS */
                        //dbgfR3UnwindCtxLoadU64(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP], &pUnwindCtx->m_uRFlags);

                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8; /* RSP */
                        uint64_t uNewRsp = (pUnwindCtx->m_aGprs[X86_GREG_xSP] - 8) & ~(uint64_t)15;
                        dbgfR3UnwindCtxLoadU64(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP], &uNewRsp);

                        pUnwindCtx->m_aGprs[X86_GREG_xSP] -= 8; /* SS */
                        //dbgfR3UnwindCtxLoadU16(pUnwindCtx, pUnwindCtx->m_aGprs[X86_GREG_xSP], &pUnwindCtx->m_uSs);

                        pUnwindCtx->m_aGprs[X86_GREG_xSP] = uNewRsp;
                        return true;
                    }

                    case IMAGE_AMD64_UWOP_RESERVED_6:
                        AssertFailedReturn(false);

                    case IMAGE_AMD64_UWOP_RESERVED_7:
                        AssertFailedReturn(false);

                    default:
                        AssertMsgFailedReturn(("%u\n", pInfo->aOpcodes[iOpcode].u.UnwindOp), false);
                }
            }

            /*
             * Chained stuff?
             */
            if (!(pInfo->Flags & IMAGE_UNW_FLAGS_CHAININFO))
                break;
            /** @todo impl chains.   */
            break;
        }

        /** @todo do post processing. */
    }

    RT_NOREF_PV(pAddrFrame);
    return false;
}


static bool dbgfR3UnwindCtxDoOneFrame(PDBGFUNWINDCTX pUnwindCtx, PDBGFADDRESS pAddrFrame)
{
    /*
     * Hope for the same module as last time around.
     */
    RTUINTPTR offCache = pUnwindCtx->m_uPc - pUnwindCtx->m_uCachedMapping;
    if (offCache < pUnwindCtx->m_cbCachedMapping)
        return dbgfR3UnwindCtxDoOneFrameCached(pUnwindCtx, offCache, pAddrFrame);

    /*
     * Try locate the module.
     */
    RTDBGMOD        hDbgMod = NIL_RTDBGMOD;
    RTUINTPTR       uBase   = 0;
    RTDBGSEGIDX     idxSeg  = NIL_RTDBGSEGIDX;
    int rc = RTDbgAsModuleByAddr(pUnwindCtx->m_hAs, pUnwindCtx->m_uPc, &hDbgMod, &uBase, &idxSeg);
    if (RT_SUCCESS(rc))
    {
        /* We cache the module regardless of unwind info. */
        dbgfR3UnwindCtxFlushCache(pUnwindCtx);
        pUnwindCtx->m_hCached         = hDbgMod;
        pUnwindCtx->m_uCachedMapping  = uBase;
        pUnwindCtx->m_cbCachedMapping = RTDbgModSegmentSize(hDbgMod, idxSeg);

        /* Play simple for now. */
        if (   idxSeg == NIL_RTDBGSEGIDX
            && RTDbgModImageGetFormat(hDbgMod) == RTLDRFMT_32BIT_HACK /// @todo RTLDRFMT_PE (disabled code)
            && RTDbgModImageGetArch(hDbgMod)   == RTLDRARCH_AMD64)
        {
            /*
             * Try query the unwind data.
             */
            size_t cbNeeded = 0;
            rc = RTDbgModImageQueryProp(hDbgMod, RTLDRPROP_UNWIND_TABLE, NULL, 0, &cbNeeded);
            if (   rc == VERR_BUFFER_OVERFLOW
                && cbNeeded >= sizeof(*pUnwindCtx->m_paFunctions)
                && cbNeeded < _64M)
            {
                void *pvBuf = RTMemAllocZ(cbNeeded + 32);
                if (pvBuf)
                {
                    rc = RTDbgModImageQueryProp(hDbgMod, RTLDRPROP_UNWIND_TABLE, pUnwindCtx->m_pbCachedInfo, cbNeeded, &cbNeeded);
                    if (RT_SUCCESS(rc))
                    {
                        pUnwindCtx->m_pbCachedInfo = (uint8_t *)pvBuf;
                        pUnwindCtx->m_cbCachedInfo = cbNeeded;
                        pUnwindCtx->m_paFunctions  = (PCIMAGE_RUNTIME_FUNCTION_ENTRY)pvBuf;
                        pUnwindCtx->m_cFunctions   = cbNeeded / sizeof(*pUnwindCtx->m_paFunctions);

                        return dbgfR3UnwindCtxDoOneFrameCached(pUnwindCtx, pUnwindCtx->m_uPc - pUnwindCtx->m_uCachedMapping,
                                                               pAddrFrame);
                    }
                    RTMemFree(pvBuf);
                }
            }
        }
    }
    return false;
}


/**
 * Read stack memory, will init entire buffer.
 */
DECLINLINE(int) dbgfR3StackRead(PUVM pUVM, VMCPUID idCpu, void *pvBuf, PCDBGFADDRESS pSrcAddr, size_t cb, size_t *pcbRead)
{
    int rc = DBGFR3MemRead(pUVM, idCpu, pSrcAddr, pvBuf, cb);
    if (RT_FAILURE(rc))
    {
        /* fallback: byte by byte and zero the ones we fail to read. */
        size_t cbRead;
        for (cbRead = 0; cbRead < cb; cbRead++)
        {
            DBGFADDRESS Addr = *pSrcAddr;
            rc = DBGFR3MemRead(pUVM, idCpu, DBGFR3AddrAdd(&Addr, cbRead), (uint8_t *)pvBuf + cbRead, 1);
            if (RT_FAILURE(rc))
                break;
        }
        if (cbRead)
            rc = VINF_SUCCESS;
        memset((char *)pvBuf + cbRead, 0, cb - cbRead);
        *pcbRead = cbRead;
    }
    else
        *pcbRead = cb;
    return rc;
}


/**
 * Internal worker routine.
 *
 * On x86 the typical stack frame layout is like this:
 *     ..  ..
 *     16  parameter 2
 *     12  parameter 1
 *      8  parameter 0
 *      4  return address
 *      0  old ebp; current ebp points here
 *
 * @todo Add AMD64 support (needs teaming up with the module management for
 *       unwind tables).
 */
DECL_NO_INLINE(static, int) dbgfR3StackWalk(PUVM pUVM, VMCPUID idCpu, RTDBGAS hAs, PDBGFSTACKFRAME pFrame)
{
    /*
     * Stop if we got a read error in the previous run.
     */
    if (pFrame->fFlags & DBGFSTACKFRAME_FLAGS_LAST)
        return VERR_NO_MORE_FILES;

    /*
     * Read the raw frame data.
     * We double cbRetAddr in case we find we have a far return.
     */
    const DBGFADDRESS   AddrOldPC = pFrame->AddrPC;
    unsigned            cbRetAddr = DBGFReturnTypeSize(pFrame->enmReturnType);
    unsigned            cbStackItem;
    switch (AddrOldPC.fFlags & DBGFADDRESS_FLAGS_TYPE_MASK)
    {
        case DBGFADDRESS_FLAGS_FAR16: cbStackItem = 2; break;
        case DBGFADDRESS_FLAGS_FAR32: cbStackItem = 4; break;
        case DBGFADDRESS_FLAGS_FAR64: cbStackItem = 8; break;
        case DBGFADDRESS_FLAGS_RING0: cbStackItem = sizeof(RTHCUINTPTR); break;
        default:
            switch (pFrame->enmReturnType)
            {
                case DBGFRETURNTYPE_FAR16:
                case DBGFRETURNTYPE_IRET16:
                case DBGFRETURNTYPE_IRET32_V86:
                case DBGFRETURNTYPE_NEAR16: cbStackItem = 2; break;

                case DBGFRETURNTYPE_FAR32:
                case DBGFRETURNTYPE_IRET32:
                case DBGFRETURNTYPE_IRET32_PRIV:
                case DBGFRETURNTYPE_NEAR32: cbStackItem = 4; break;

                case DBGFRETURNTYPE_FAR64:
                case DBGFRETURNTYPE_IRET64:
                case DBGFRETURNTYPE_NEAR64: cbStackItem = 8; break;

                default:
                    AssertMsgFailed(("%d\n", pFrame->enmReturnType));
                    cbStackItem = 4;
                    break;
            }
    }

    union
    {
        uint64_t *pu64;
        uint32_t *pu32;
        uint16_t *pu16;
        uint8_t  *pb;
        void     *pv;
    } u, uRet, uArgs, uBp;
    size_t cbRead = cbRetAddr*2 + cbStackItem + sizeof(pFrame->Args);
    u.pv = alloca(cbRead);
    uBp = u;
    uRet.pb = u.pb + cbStackItem;
    uArgs.pb = u.pb + cbStackItem + cbRetAddr;

    Assert(DBGFADDRESS_IS_VALID(&pFrame->AddrFrame));
    int rc = dbgfR3StackRead(pUVM, idCpu, u.pv,
                             pFrame->fFlags & DBGFSTACKFRAME_FLAGS_ALL_VALID
                             ? &pFrame->AddrReturnFrame
                             : &pFrame->AddrFrame,
                             cbRead, &cbRead);
    if (    RT_FAILURE(rc)
        ||  cbRead < cbRetAddr + cbStackItem)
        pFrame->fFlags |= DBGFSTACKFRAME_FLAGS_LAST;

    /*
     * The first step is taken in a different way than the others.
     */
    if (!(pFrame->fFlags & DBGFSTACKFRAME_FLAGS_ALL_VALID))
    {
        pFrame->fFlags |= DBGFSTACKFRAME_FLAGS_ALL_VALID;
        pFrame->iFrame = 0;

        /* Current PC - set by caller, just find symbol & line. */
        if (DBGFADDRESS_IS_VALID(&pFrame->AddrPC))
        {
            pFrame->pSymPC  = DBGFR3AsSymbolByAddrA(pUVM, hAs, &pFrame->AddrPC,
                                                    RTDBGSYMADDR_FLAGS_LESS_OR_EQUAL | RTDBGSYMADDR_FLAGS_SKIP_ABS_IN_DEFERRED,
                                                    NULL /*poffDisp*/, NULL /*phMod*/);
            pFrame->pLinePC = DBGFR3AsLineByAddrA(pUVM, hAs, &pFrame->AddrPC, NULL /*poffDisp*/, NULL /*phMod*/);
        }
    }
    else /* 2nd and subsequent steps */
    {
        /* frame, pc and stack is taken from the existing frames return members. */
        pFrame->AddrFrame = pFrame->AddrReturnFrame;
        pFrame->AddrPC    = pFrame->AddrReturnPC;
        pFrame->pSymPC    = pFrame->pSymReturnPC;
        pFrame->pLinePC   = pFrame->pLineReturnPC;

        /* increment the frame number. */
        pFrame->iFrame++;
    }

    /*
     * Return Frame address.
     */
    pFrame->AddrReturnFrame = pFrame->AddrFrame;
    switch (cbStackItem)
    {
        case 2:    pFrame->AddrReturnFrame.off = *uBp.pu16; break;
        case 4:    pFrame->AddrReturnFrame.off = *uBp.pu32; break;
        case 8:    pFrame->AddrReturnFrame.off = *uBp.pu64; break;
        default:    AssertMsgFailedReturn(("cbStackItem=%d\n", cbStackItem), VERR_DBGF_STACK_IPE_1);
    }

    /* Watcom tries to keep the frame pointer odd for far returns. */
    if (cbStackItem <= 4)
    {
        if (pFrame->AddrReturnFrame.off & 1)
        {
            pFrame->AddrReturnFrame.off &= ~(RTGCUINTPTR)1;
            if (pFrame->enmReturnType == DBGFRETURNTYPE_NEAR16)
            {
                pFrame->fFlags       |= DBGFSTACKFRAME_FLAGS_USED_ODD_EVEN;
                pFrame->enmReturnType = DBGFRETURNTYPE_FAR16;
                cbRetAddr = 4;
            }
            else if (pFrame->enmReturnType == DBGFRETURNTYPE_NEAR32)
            {
                pFrame->fFlags       |= DBGFSTACKFRAME_FLAGS_USED_ODD_EVEN;
                pFrame->enmReturnType = DBGFRETURNTYPE_FAR32;
                cbRetAddr = 8;
            }
        }
        else if (pFrame->fFlags & DBGFSTACKFRAME_FLAGS_USED_ODD_EVEN)
        {
            if (pFrame->enmReturnType == DBGFRETURNTYPE_FAR16)
            {
                pFrame->enmReturnType = DBGFRETURNTYPE_NEAR16;
                cbRetAddr = 2;
            }
            else if (pFrame->enmReturnType == DBGFRETURNTYPE_NEAR32)
            {
                pFrame->enmReturnType = DBGFRETURNTYPE_FAR32;
                cbRetAddr = 4;
            }
            pFrame->fFlags &= ~DBGFSTACKFRAME_FLAGS_USED_ODD_EVEN;
        }
        uArgs.pb = u.pb + cbStackItem + cbRetAddr;
    }

    pFrame->AddrReturnFrame.FlatPtr += pFrame->AddrReturnFrame.off - pFrame->AddrFrame.off;

    /*
     * Return PC and Stack Addresses.
     */
    /** @todo AddrReturnStack is not correct for stdcall and pascal. (requires scope info) */
    pFrame->AddrReturnStack          = pFrame->AddrFrame;
    pFrame->AddrReturnStack.off     += cbStackItem + cbRetAddr;
    pFrame->AddrReturnStack.FlatPtr += cbStackItem + cbRetAddr;

    pFrame->AddrReturnPC             = pFrame->AddrPC;
    switch (pFrame->enmReturnType)
    {
        case DBGFRETURNTYPE_NEAR16:
            if (DBGFADDRESS_IS_VALID(&pFrame->AddrReturnPC))
            {
                pFrame->AddrReturnPC.FlatPtr += *uRet.pu16 - pFrame->AddrReturnPC.off;
                pFrame->AddrReturnPC.off      = *uRet.pu16;
            }
            else
                DBGFR3AddrFromFlat(pUVM, &pFrame->AddrReturnPC, *uRet.pu16);
            break;
        case DBGFRETURNTYPE_NEAR32:
            if (DBGFADDRESS_IS_VALID(&pFrame->AddrReturnPC))
            {
                pFrame->AddrReturnPC.FlatPtr += *uRet.pu32 - pFrame->AddrReturnPC.off;
                pFrame->AddrReturnPC.off      = *uRet.pu32;
            }
            else
                DBGFR3AddrFromFlat(pUVM, &pFrame->AddrReturnPC, *uRet.pu32);
            break;
        case DBGFRETURNTYPE_NEAR64:
            if (DBGFADDRESS_IS_VALID(&pFrame->AddrReturnPC))
            {
                pFrame->AddrReturnPC.FlatPtr += *uRet.pu64 - pFrame->AddrReturnPC.off;
                pFrame->AddrReturnPC.off      = *uRet.pu64;
            }
            else
                DBGFR3AddrFromFlat(pUVM, &pFrame->AddrReturnPC, *uRet.pu64);
            break;
        case DBGFRETURNTYPE_FAR16:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[1], uRet.pu16[0]);
            break;
        case DBGFRETURNTYPE_FAR32:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[2], uRet.pu32[0]);
            break;
        case DBGFRETURNTYPE_FAR64:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[4], uRet.pu64[0]);
            break;
        case DBGFRETURNTYPE_IRET16:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[1], uRet.pu16[0]);
            break;
        case DBGFRETURNTYPE_IRET32:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[2], uRet.pu32[0]);
            break;
        case DBGFRETURNTYPE_IRET32_PRIV:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[2], uRet.pu32[0]);
            break;
        case DBGFRETURNTYPE_IRET32_V86:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[2], uRet.pu32[0]);
            break;
        case DBGFRETURNTYPE_IRET64:
            DBGFR3AddrFromSelOff(pUVM, idCpu, &pFrame->AddrReturnPC, uRet.pu16[4], uRet.pu64[0]);
            break;
        default:
            AssertMsgFailed(("enmReturnType=%d\n", pFrame->enmReturnType));
            return VERR_INVALID_PARAMETER;
    }

    pFrame->pSymReturnPC  = DBGFR3AsSymbolByAddrA(pUVM, hAs, &pFrame->AddrReturnPC,
                                                  RTDBGSYMADDR_FLAGS_LESS_OR_EQUAL | RTDBGSYMADDR_FLAGS_SKIP_ABS_IN_DEFERRED,
                                                  NULL /*poffDisp*/, NULL /*phMod*/);
    pFrame->pLineReturnPC = DBGFR3AsLineByAddrA(pUVM, hAs, &pFrame->AddrReturnPC, NULL /*poffDisp*/, NULL /*phMod*/);

    /*
     * Frame bitness flag.
     */
    switch (cbStackItem)
    {
        case 2: pFrame->fFlags |= DBGFSTACKFRAME_FLAGS_16BIT; break;
        case 4: pFrame->fFlags |= DBGFSTACKFRAME_FLAGS_32BIT; break;
        case 8: pFrame->fFlags |= DBGFSTACKFRAME_FLAGS_64BIT; break;
        default:    AssertMsgFailedReturn(("cbStackItem=%d\n", cbStackItem), VERR_DBGF_STACK_IPE_2);
    }

    /*
     * The arguments.
     */
    memcpy(&pFrame->Args, uArgs.pv, sizeof(pFrame->Args));

    return VINF_SUCCESS;
}


/**
 * Walks the entire stack allocating memory as we walk.
 */
static DECLCALLBACK(int) dbgfR3StackWalkCtxFull(PUVM pUVM, VMCPUID idCpu, PCCPUMCTX pCtx, RTDBGAS hAs,
                                                DBGFCODETYPE enmCodeType,
                                                PCDBGFADDRESS pAddrFrame,
                                                PCDBGFADDRESS pAddrStack,
                                                PCDBGFADDRESS pAddrPC,
                                                DBGFRETURNTYPE enmReturnType,
                                                PCDBGFSTACKFRAME *ppFirstFrame)
{
    DBGFUNWINDCTX UnwindCtx(pUVM, idCpu, pCtx, hAs);

    /* alloc first frame. */
    PDBGFSTACKFRAME pCur = (PDBGFSTACKFRAME)MMR3HeapAllocZU(pUVM, MM_TAG_DBGF_STACK, sizeof(*pCur));
    if (!pCur)
        return VERR_NO_MEMORY;

    /*
     * Initialize the frame.
     */
    pCur->pNextInternal = NULL;
    pCur->pFirstInternal = pCur;

    int rc = VINF_SUCCESS;
    if (pAddrPC)
        pCur->AddrPC = *pAddrPC;
    else if (enmCodeType != DBGFCODETYPE_GUEST)
        DBGFR3AddrFromFlat(pUVM, &pCur->AddrPC, pCtx->rip);
    else
        rc = DBGFR3AddrFromSelOff(pUVM, idCpu, &pCur->AddrPC, pCtx->cs.Sel, pCtx->rip);
    if (RT_SUCCESS(rc))
    {
        uint64_t fAddrMask;
        if (enmCodeType == DBGFCODETYPE_RING0)
            fAddrMask = HC_ARCH_BITS == 64 ? UINT64_MAX : UINT32_MAX;
        else if (enmCodeType == DBGFCODETYPE_HYPER)
            fAddrMask = UINT32_MAX;
        else if (DBGFADDRESS_IS_FAR16(&pCur->AddrPC))
            fAddrMask = UINT16_MAX;
        else if (DBGFADDRESS_IS_FAR32(&pCur->AddrPC))
            fAddrMask = UINT32_MAX;
        else if (DBGFADDRESS_IS_FAR64(&pCur->AddrPC))
            fAddrMask = UINT64_MAX;
        else
        {
            PVMCPU pVCpu = VMMGetCpuById(pUVM->pVM, idCpu);
            CPUMMODE enmCpuMode = CPUMGetGuestMode(pVCpu);
            if (enmCpuMode == CPUMMODE_REAL)
            {
                fAddrMask = UINT16_MAX;
                if (enmReturnType == DBGFRETURNTYPE_INVALID)
                    pCur->enmReturnType = DBGFRETURNTYPE_NEAR16;
            }
            else if (   enmCpuMode == CPUMMODE_PROTECTED
                     || !CPUMIsGuestIn64BitCode(pVCpu))
            {
                fAddrMask = UINT32_MAX;
                if (enmReturnType == DBGFRETURNTYPE_INVALID)
                    pCur->enmReturnType = DBGFRETURNTYPE_NEAR32;
            }
            else
            {
                fAddrMask = UINT64_MAX;
                if (enmReturnType == DBGFRETURNTYPE_INVALID)
                    pCur->enmReturnType = DBGFRETURNTYPE_NEAR64;
            }
        }

        if (enmReturnType == DBGFRETURNTYPE_INVALID)
            switch (pCur->AddrPC.fFlags & DBGFADDRESS_FLAGS_TYPE_MASK)
            {
                case DBGFADDRESS_FLAGS_FAR16: pCur->enmReturnType = DBGFRETURNTYPE_NEAR16; break;
                case DBGFADDRESS_FLAGS_FAR32: pCur->enmReturnType = DBGFRETURNTYPE_NEAR32; break;
                case DBGFADDRESS_FLAGS_FAR64: pCur->enmReturnType = DBGFRETURNTYPE_NEAR64; break;
                case DBGFADDRESS_FLAGS_RING0:
                    pCur->enmReturnType = HC_ARCH_BITS == 64 ? DBGFRETURNTYPE_NEAR64 : DBGFRETURNTYPE_NEAR32;
                    break;
                default:
                    pCur->enmReturnType = DBGFRETURNTYPE_NEAR32;
                    break;
            }


        if (pAddrStack)
            pCur->AddrStack = *pAddrStack;
        else if (enmCodeType != DBGFCODETYPE_GUEST)
            DBGFR3AddrFromFlat(pUVM, &pCur->AddrStack, pCtx->rsp & fAddrMask);
        else
            rc = DBGFR3AddrFromSelOff(pUVM, idCpu, &pCur->AddrStack, pCtx->ss.Sel, pCtx->rsp & fAddrMask);

        if (pAddrFrame)
            pCur->AddrFrame = *pAddrFrame;
        else
        {
            if (   RT_SUCCESS(rc)
                && dbgfR3UnwindCtxSetPcAndSp(&UnwindCtx, &pCur->AddrPC, &pCur->AddrStack)
                && dbgfR3UnwindCtxDoOneFrame(&UnwindCtx, &pCur->AddrFrame))
            { }
            else if (enmCodeType != DBGFCODETYPE_GUEST)
                DBGFR3AddrFromFlat(pUVM, &pCur->AddrFrame, pCtx->rbp & fAddrMask);
            else if (RT_SUCCESS(rc))
                rc = DBGFR3AddrFromSelOff(pUVM, idCpu, &pCur->AddrFrame, pCtx->ss.Sel, pCtx->rbp & fAddrMask);
        }
    }
    else
        pCur->enmReturnType = enmReturnType;

    /*
     * The first frame.
     */
    if (RT_SUCCESS(rc))
        rc = dbgfR3StackWalk(pUVM, idCpu, hAs, pCur);
    if (RT_FAILURE(rc))
    {
        DBGFR3StackWalkEnd(pCur);
        return rc;
    }

    /*
     * The other frames.
     */
    DBGFSTACKFRAME Next = *pCur;
    while (!(pCur->fFlags & (DBGFSTACKFRAME_FLAGS_LAST | DBGFSTACKFRAME_FLAGS_MAX_DEPTH | DBGFSTACKFRAME_FLAGS_LOOP)))
    {
        /* try walk. */
        rc = dbgfR3StackWalk(pUVM, idCpu, hAs, &Next);
        if (RT_FAILURE(rc))
            break;

        /* add the next frame to the chain. */
        PDBGFSTACKFRAME pNext = (PDBGFSTACKFRAME)MMR3HeapAllocU(pUVM, MM_TAG_DBGF_STACK, sizeof(*pNext));
        if (!pNext)
        {
            DBGFR3StackWalkEnd(pCur);
            return VERR_NO_MEMORY;
        }
        *pNext = Next;
        pCur->pNextInternal = pNext;
        pCur = pNext;
        Assert(pCur->pNextInternal == NULL);

        /* check for loop */
        for (PCDBGFSTACKFRAME pLoop = pCur->pFirstInternal;
             pLoop && pLoop != pCur;
             pLoop = pLoop->pNextInternal)
            if (pLoop->AddrFrame.FlatPtr == pCur->AddrFrame.FlatPtr)
            {
                pCur->fFlags |= DBGFSTACKFRAME_FLAGS_LOOP;
                break;
            }

        /* check for insane recursion */
        if (pCur->iFrame >= 2048)
            pCur->fFlags |= DBGFSTACKFRAME_FLAGS_MAX_DEPTH;
    }

    *ppFirstFrame = pCur->pFirstInternal;
    return rc;
}


/**
 * Common worker for DBGFR3StackWalkBeginGuestEx, DBGFR3StackWalkBeginHyperEx,
 * DBGFR3StackWalkBeginGuest and DBGFR3StackWalkBeginHyper.
 */
static int dbgfR3StackWalkBeginCommon(PUVM pUVM,
                                      VMCPUID idCpu,
                                      DBGFCODETYPE enmCodeType,
                                      PCDBGFADDRESS pAddrFrame,
                                      PCDBGFADDRESS pAddrStack,
                                      PCDBGFADDRESS pAddrPC,
                                      DBGFRETURNTYPE enmReturnType,
                                      PCDBGFSTACKFRAME *ppFirstFrame)
{
    /*
     * Validate parameters.
     */
    *ppFirstFrame = NULL;
    UVM_ASSERT_VALID_EXT_RETURN(pUVM, VERR_INVALID_VM_HANDLE);
    PVM pVM = pUVM->pVM;
    VM_ASSERT_VALID_EXT_RETURN(pVM, VERR_INVALID_VM_HANDLE);
    AssertReturn(idCpu < pVM->cCpus, VERR_INVALID_CPU_ID);
    if (pAddrFrame)
        AssertReturn(DBGFR3AddrIsValid(pUVM, pAddrFrame), VERR_INVALID_PARAMETER);
    if (pAddrStack)
        AssertReturn(DBGFR3AddrIsValid(pUVM, pAddrStack), VERR_INVALID_PARAMETER);
    if (pAddrPC)
        AssertReturn(DBGFR3AddrIsValid(pUVM, pAddrPC), VERR_INVALID_PARAMETER);
    AssertReturn(enmReturnType >= DBGFRETURNTYPE_INVALID && enmReturnType < DBGFRETURNTYPE_END, VERR_INVALID_PARAMETER);

    /*
     * Get the CPUM context pointer and pass it on the specified EMT.
     */
    RTDBGAS     hAs;
    PCCPUMCTX   pCtx;
    switch (enmCodeType)
    {
        case DBGFCODETYPE_GUEST:
            pCtx = CPUMQueryGuestCtxPtr(VMMGetCpuById(pVM, idCpu));
            hAs  = DBGF_AS_GLOBAL;
            break;
        case DBGFCODETYPE_HYPER:
            pCtx = CPUMQueryGuestCtxPtr(VMMGetCpuById(pVM, idCpu));
            hAs  = DBGF_AS_RC_AND_GC_GLOBAL;
            break;
        case DBGFCODETYPE_RING0:
            pCtx = NULL;    /* No valid context present. */
            hAs  = DBGF_AS_R0;
            break;
        default:
            AssertFailedReturn(VERR_INVALID_PARAMETER);
    }
    return VMR3ReqPriorityCallWaitU(pUVM, idCpu, (PFNRT)dbgfR3StackWalkCtxFull, 10,
                                    pUVM, idCpu, pCtx, hAs, enmCodeType,
                                    pAddrFrame, pAddrStack, pAddrPC, enmReturnType, ppFirstFrame);
}


/**
 * Begins a guest stack walk, extended version.
 *
 * This will walk the current stack, constructing a list of info frames which is
 * returned to the caller. The caller uses DBGFR3StackWalkNext to traverse the
 * list and DBGFR3StackWalkEnd to release it.
 *
 * @returns VINF_SUCCESS on success.
 * @returns VERR_NO_MEMORY if we're out of memory.
 *
 * @param   pUVM            The user mode VM handle.
 * @param   idCpu           The ID of the virtual CPU which stack we want to walk.
 * @param   enmCodeType     Code type
 * @param   pAddrFrame      Frame address to start at. (Optional)
 * @param   pAddrStack      Stack address to start at. (Optional)
 * @param   pAddrPC         Program counter to start at. (Optional)
 * @param   enmReturnType   The return address type. (Optional)
 * @param   ppFirstFrame    Where to return the pointer to the first info frame.
 */
VMMR3DECL(int) DBGFR3StackWalkBeginEx(PUVM pUVM,
                                      VMCPUID idCpu,
                                      DBGFCODETYPE enmCodeType,
                                      PCDBGFADDRESS pAddrFrame,
                                      PCDBGFADDRESS pAddrStack,
                                      PCDBGFADDRESS pAddrPC,
                                      DBGFRETURNTYPE enmReturnType,
                                      PCDBGFSTACKFRAME *ppFirstFrame)
{
    return dbgfR3StackWalkBeginCommon(pUVM, idCpu, enmCodeType, pAddrFrame, pAddrStack, pAddrPC, enmReturnType, ppFirstFrame);
}


/**
 * Begins a guest stack walk.
 *
 * This will walk the current stack, constructing a list of info frames which is
 * returned to the caller. The caller uses DBGFR3StackWalkNext to traverse the
 * list and DBGFR3StackWalkEnd to release it.
 *
 * @returns VINF_SUCCESS on success.
 * @returns VERR_NO_MEMORY if we're out of memory.
 *
 * @param   pUVM            The user mode VM handle.
 * @param   idCpu           The ID of the virtual CPU which stack we want to walk.
 * @param   enmCodeType     Code type
 * @param   ppFirstFrame    Where to return the pointer to the first info frame.
 */
VMMR3DECL(int) DBGFR3StackWalkBegin(PUVM pUVM, VMCPUID idCpu, DBGFCODETYPE enmCodeType, PCDBGFSTACKFRAME *ppFirstFrame)
{
    return dbgfR3StackWalkBeginCommon(pUVM, idCpu, enmCodeType, NULL, NULL, NULL, DBGFRETURNTYPE_INVALID, ppFirstFrame);
}

/**
 * Gets the next stack frame.
 *
 * @returns Pointer to the info for the next stack frame.
 *          NULL if no more frames.
 *
 * @param   pCurrent    Pointer to the current stack frame.
 *
 */
VMMR3DECL(PCDBGFSTACKFRAME) DBGFR3StackWalkNext(PCDBGFSTACKFRAME pCurrent)
{
    return pCurrent
         ? pCurrent->pNextInternal
         : NULL;
}


/**
 * Ends a stack walk process.
 *
 * This *must* be called after a successful first call to any of the stack
 * walker functions. If not called we will leak memory or other resources.
 *
 * @param   pFirstFrame     The frame returned by one of the begin functions.
 */
VMMR3DECL(void) DBGFR3StackWalkEnd(PCDBGFSTACKFRAME pFirstFrame)
{
    if (    !pFirstFrame
        ||  !pFirstFrame->pFirstInternal)
        return;

    PDBGFSTACKFRAME pFrame = (PDBGFSTACKFRAME)pFirstFrame->pFirstInternal;
    while (pFrame)
    {
        PDBGFSTACKFRAME pCur = pFrame;
        pFrame = (PDBGFSTACKFRAME)pCur->pNextInternal;
        if (pFrame)
        {
            if (pCur->pSymReturnPC == pFrame->pSymPC)
                pFrame->pSymPC = NULL;
            if (pCur->pSymReturnPC == pFrame->pSymReturnPC)
                pFrame->pSymReturnPC = NULL;

            if (pCur->pSymPC == pFrame->pSymPC)
                pFrame->pSymPC = NULL;
            if (pCur->pSymPC == pFrame->pSymReturnPC)
                pFrame->pSymReturnPC = NULL;

            if (pCur->pLineReturnPC == pFrame->pLinePC)
                pFrame->pLinePC = NULL;
            if (pCur->pLineReturnPC == pFrame->pLineReturnPC)
                pFrame->pLineReturnPC = NULL;

            if (pCur->pLinePC == pFrame->pLinePC)
                pFrame->pLinePC = NULL;
            if (pCur->pLinePC == pFrame->pLineReturnPC)
                pFrame->pLineReturnPC = NULL;
        }

        RTDbgSymbolFree(pCur->pSymPC);
        RTDbgSymbolFree(pCur->pSymReturnPC);
        RTDbgLineFree(pCur->pLinePC);
        RTDbgLineFree(pCur->pLineReturnPC);

        pCur->pNextInternal = NULL;
        pCur->pFirstInternal = NULL;
        pCur->fFlags = 0;
        MMR3HeapFree(pCur);
    }
}


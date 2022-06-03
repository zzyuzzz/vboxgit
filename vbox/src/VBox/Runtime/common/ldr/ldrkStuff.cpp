/* $Id: ldrkStuff.cpp 38531 2011-08-25 14:30:23Z vboxsync $ */
/** @file
 * IPRT - Binary Image Loader, kLdr Interface.
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP RTLOGGROUP_LDR
#include <iprt/ldr.h>
#include "internal/iprt.h"

#include <iprt/file.h>
#include <iprt/alloc.h>
#include <iprt/alloca.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/path.h>
#include <iprt/log.h>
#include <iprt/param.h>
#include <iprt/err.h>
#include "internal/ldr.h"
#define KLDR_ALREADY_INCLUDE_STD_TYPES
#define KLDR_NO_KLDR_H_INCLUDES
#include <k/kLdr.h>
#include <k/kRdrAll.h>
#include <k/kErr.h>
#include <k/kErrors.h>
#include <k/kMagics.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * kLdr file provider.
 */
typedef struct RTKLDRRDR
{
    /** The core. */
    KRDR            Core;
    /** The IPRT bit reader. */
    PRTLDRREADER    pReader;
} RTKLDRRDR, *PRTKLDRRDR;

/**
 * IPRT module.
 */
typedef struct RTLDRMODKLDR
{
    /** The Core module structure. */
    RTLDRMODINTERNAL    Core;
    /** The kLdr module. */
    PKLDRMOD            pMod;
} RTLDRMODKLDR, *PRTLDRMODKLDR;


/**
 * Arguments for a RTLDRMODKLDR callback wrapper.
 */
typedef struct RTLDRMODKLDRARGS
{
    union
    {
        PFNRT            pfn;
        PFNRTLDRENUMDBG  pfnEnumDbgInfo;
        PFNRTLDRENUMSYMS pfnEnumSyms;
        PFNRTLDRIMPORT   pfnGetImport;
    }                   u;
    void               *pvUser;
    const void         *pvBits;
    PRTLDRMODKLDR       pMod;
    int                 rc;
} RTLDRMODKLDRARGS, *PRTLDRMODKLDRARGS;


/**
 * Converts a kLdr error code to an IPRT one.
 */
static int rtkldrConvertError(int krc)
{
    if (!krc)
        return VINF_SUCCESS;
    switch (krc)
    {
        case KERR_INVALID_PARAMETER:                        return VERR_INVALID_PARAMETER;
        case KERR_INVALID_HANDLE:                           return VERR_INVALID_HANDLE;
        case KERR_NO_MEMORY:                                return VERR_NO_MEMORY;


        case KLDR_ERR_UNKNOWN_FORMAT:
        case KLDR_ERR_MZ_NOT_SUPPORTED:                     return VERR_MZ_EXE_NOT_SUPPORTED;
        case KLDR_ERR_NE_NOT_SUPPORTED:                     return VERR_NE_EXE_NOT_SUPPORTED;
        case KLDR_ERR_LX_NOT_SUPPORTED:                     return VERR_LX_EXE_NOT_SUPPORTED;
        case KLDR_ERR_LE_NOT_SUPPORTED:                     return VERR_LE_EXE_NOT_SUPPORTED;
        case KLDR_ERR_PE_NOT_SUPPORTED:                     return VERR_PE_EXE_NOT_SUPPORTED;
        case KLDR_ERR_ELF_NOT_SUPPORTED:                    return VERR_ELF_EXE_NOT_SUPPORTED;
        case KLDR_ERR_MACHO_NOT_SUPPORTED:                  return VERR_INVALID_EXE_SIGNATURE;
        case KLDR_ERR_AOUT_NOT_SUPPORTED:                   return VERR_AOUT_EXE_NOT_SUPPORTED;

        case KLDR_ERR_MODULE_NOT_FOUND:                     return VERR_MODULE_NOT_FOUND;
        case KLDR_ERR_PREREQUISITE_MODULE_NOT_FOUND:        return VERR_MODULE_NOT_FOUND;
        case KLDR_ERR_MAIN_STACK_ALLOC_FAILED:              return VERR_NO_MEMORY;
        case KERR_BUFFER_OVERFLOW:                          return VERR_BUFFER_OVERFLOW;
        case KLDR_ERR_SYMBOL_NOT_FOUND:                     return VERR_SYMBOL_NOT_FOUND;
        case KLDR_ERR_FORWARDER_SYMBOL:                     return VERR_BAD_EXE_FORMAT;
        case KLDR_ERR_BAD_FIXUP:                            AssertMsgFailedReturn(("KLDR_ERR_BAD_FIXUP\n"), VERR_BAD_EXE_FORMAT);
        case KLDR_ERR_IMPORT_ORDINAL_OUT_OF_BOUNDS:         return VERR_BAD_EXE_FORMAT;
        case KLDR_ERR_NO_DEBUG_INFO:                        return VERR_FILE_NOT_FOUND;
        case KLDR_ERR_ALREADY_MAPPED:                       return VERR_WRONG_ORDER;
        case KLDR_ERR_NOT_MAPPED:                           return VERR_WRONG_ORDER;
        case KLDR_ERR_ADDRESS_OVERFLOW:                     return VERR_NUMBER_TOO_BIG;
        case KLDR_ERR_TODO:                                 return VERR_NOT_IMPLEMENTED;

        case KLDR_ERR_NOT_LOADED_DYNAMICALLY:
        case KCPU_ERR_ARCH_CPU_NOT_COMPATIBLE:
        case KLDR_ERR_TOO_LONG_FORWARDER_CHAIN:
        case KLDR_ERR_MODULE_TERMINATING:
        case KLDR_ERR_PREREQUISITE_MODULE_TERMINATING:
        case KLDR_ERR_MODULE_INIT_FAILED:
        case KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED:
        case KLDR_ERR_MODULE_INIT_FAILED_ALREADY:
        case KLDR_ERR_PREREQUISITE_MODULE_INIT_FAILED_ALREADY:
        case KLDR_ERR_PREREQUISITE_RECURSED_TOO_DEEPLY:
        case KLDR_ERR_THREAD_ATTACH_FAILED:
        case KRDR_ERR_TOO_MANY_MAPPINGS:
        case KLDR_ERR_NOT_DLL:
        case KLDR_ERR_NOT_EXE:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_GENERAL_FAILURE);


        case KLDR_ERR_PE_UNSUPPORTED_MACHINE:
        case KLDR_ERR_PE_BAD_FILE_HEADER:
        case KLDR_ERR_PE_BAD_OPTIONAL_HEADER:
        case KLDR_ERR_PE_BAD_SECTION_HEADER:
        case KLDR_ERR_PE_BAD_FORWARDER:
        case KLDR_ERR_PE_FORWARDER_IMPORT_NOT_FOUND:
        case KLDR_ERR_PE_BAD_FIXUP:
        case KLDR_ERR_PE_BAD_IMPORT:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_GENERAL_FAILURE);

        case KLDR_ERR_LX_BAD_HEADER:
        case KLDR_ERR_LX_BAD_LOADER_SECTION:
        case KLDR_ERR_LX_BAD_FIXUP_SECTION:
        case KLDR_ERR_LX_BAD_OBJECT_TABLE:
        case KLDR_ERR_LX_BAD_PAGE_MAP:
        case KLDR_ERR_LX_BAD_ITERDATA:
        case KLDR_ERR_LX_BAD_ITERDATA2:
        case KLDR_ERR_LX_BAD_BUNDLE:
        case KLDR_ERR_LX_NO_SONAME:
        case KLDR_ERR_LX_BAD_SONAME:
        case KLDR_ERR_LX_BAD_FORWARDER:
        case KLDR_ERR_LX_NRICHAIN_NOT_SUPPORTED:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_GENERAL_FAILURE);

        case KLDR_ERR_MACHO_OTHER_ENDIAN_NOT_SUPPORTED:
        case KLDR_ERR_MACHO_BAD_HEADER:
        case KLDR_ERR_MACHO_UNSUPPORTED_FILE_TYPE:
        case KLDR_ERR_MACHO_UNSUPPORTED_MACHINE:
        case KLDR_ERR_MACHO_BAD_LOAD_COMMAND:
        case KLDR_ERR_MACHO_UNKNOWN_LOAD_COMMAND:
        case KLDR_ERR_MACHO_UNSUPPORTED_LOAD_COMMAND:
        case KLDR_ERR_MACHO_BAD_SECTION:
        case KLDR_ERR_MACHO_UNSUPPORTED_SECTION:
#ifdef KLDR_ERR_MACHO_UNSUPPORTED_INIT_SECTION
        case KLDR_ERR_MACHO_UNSUPPORTED_INIT_SECTION:
        case KLDR_ERR_MACHO_UNSUPPORTED_TERM_SECTION:
#endif
        case KLDR_ERR_MACHO_UNKNOWN_SECTION:
        case KLDR_ERR_MACHO_BAD_SECTION_ORDER:
        case KLDR_ERR_MACHO_BIT_MIX:
        case KLDR_ERR_MACHO_BAD_OBJECT_FILE:
        case KLDR_ERR_MACHO_BAD_SYMBOL:
        case KLDR_ERR_MACHO_UNSUPPORTED_FIXUP_TYPE:
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_GENERAL_FAILURE);

        default:
            if (RT_FAILURE(krc))
                return krc;
            AssertMsgFailedReturn(("krc=%d (%#x): %s\n", krc, krc, kErrName(krc)), VERR_NO_TRANSLATION);
    }
}


/**
 * Converts a IPRT error code to an kLdr one.
 */
static int rtkldrConvertErrorFromIPRT(int rc)
{
    if (RT_SUCCESS(rc))
        return 0;
    switch (rc)
    {
        case VERR_NO_MEMORY:            return KERR_NO_MEMORY;
        case VERR_INVALID_PARAMETER:    return KERR_INVALID_PARAMETER;
        case VERR_INVALID_HANDLE:       return KERR_INVALID_HANDLE;
        case VERR_BUFFER_OVERFLOW:      return KERR_BUFFER_OVERFLOW;
        default:
            return rc;
    }
}






/** @copydoc KLDRRDROPS::pfnCreate
 * @remark This is a dummy which isn't used. */
static int      rtkldrRdr_Create(  PPKRDR ppRdr, const char *pszFilename)
{
    AssertReleaseFailed();
    return -1;
}


/** @copydoc KLDRRDROPS::pfnDestroy */
static int      rtkldrRdr_Destroy( PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnDestroy(pReader);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @copydoc KLDRRDROPS::pfnRead */
static int      rtkldrRdr_Read(    PKRDR pRdr, void *pvBuf, KSIZE cb, KFOFF off)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnRead(pReader, pvBuf, cb, off);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @copydoc KLDRRDROPS::pfnAllMap */
static int      rtkldrRdr_AllMap(  PKRDR pRdr, const void **ppvBits)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnMap(pReader, ppvBits);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @copydoc KLDRRDROPS::pfnAllUnmap */
static int      rtkldrRdr_AllUnmap(PKRDR pRdr, const void *pvBits)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    int rc = pReader->pfnUnmap(pReader, pvBits);
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @copydoc KLDRRDROPS::pfnSize */
static KFOFF rtkldrRdr_Size(    PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    return (KFOFF)pReader->pfnSize(pReader);
}


/** @copydoc KLDRRDROPS::pfnTell */
static KFOFF rtkldrRdr_Tell(    PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    return (KFOFF)pReader->pfnTell(pReader);
}


/** @copydoc KLDRRDROPS::pfnName */
static const char * rtkldrRdr_Name(PKRDR pRdr)
{
    PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    return pReader->pfnLogName(pReader);
}


/** @copydoc KLDRRDROPS::pfnNativeFH */
static KIPTR rtkldrRdr_NativeFH(PKRDR pRdr)
{
    AssertFailed();
    return -1;
}


/** @copydoc KLDRRDROPS::pfnPageSize */
static KSIZE rtkldrRdr_PageSize(PKRDR pRdr)
{
    return PAGE_SIZE;
}


/** @copydoc KLDRRDROPS::pfnMap */
static int      rtkldrRdr_Map(     PKRDR pRdr, void **ppvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fFixed)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    AssertFailed();
    return -1;
}


/** @copydoc KLDRRDROPS::pfnRefresh */
static int      rtkldrRdr_Refresh( PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    AssertFailed();
    return -1;
}


/** @copydoc KLDRRDROPS::pfnProtect */
static int      rtkldrRdr_Protect( PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments, KBOOL fUnprotectOrProtect)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    AssertFailed();
    return -1;
}


/** @copydoc KLDRRDROPS::pfnUnmap */
static int      rtkldrRdr_Unmap(   PKRDR pRdr, void *pvBase, KU32 cSegments, PCKLDRSEG paSegments)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
    AssertFailed();
    return -1;
}

/** @copydoc KLDRRDROPS::pfnDone */
static void     rtkldrRdr_Done(    PKRDR pRdr)
{
    //PRTLDRREADER pReader = ((PRTKLDRRDR)pRdr)->pReader;
}


/**
 * The file reader operations.
 * We provide our own based on IPRT instead of using the kLdr ones.
 */
extern "C" const KRDROPS g_kLdrRdrFileOps;
extern "C" const KRDROPS g_kLdrRdrFileOps =
{
    /* .pszName = */        "IPRT",
    /* .pNext = */          NULL,
    /* .pfnCreate = */      rtkldrRdr_Create,
    /* .pfnDestroy = */     rtkldrRdr_Destroy,
    /* .pfnRead = */        rtkldrRdr_Read,
    /* .pfnAllMap = */      rtkldrRdr_AllMap,
    /* .pfnAllUnmap = */    rtkldrRdr_AllUnmap,
    /* .pfnSize = */        rtkldrRdr_Size,
    /* .pfnTell = */        rtkldrRdr_Tell,
    /* .pfnName = */        rtkldrRdr_Name,
    /* .pfnNativeFH = */    rtkldrRdr_NativeFH,
    /* .pfnPageSize = */    rtkldrRdr_PageSize,
    /* .pfnMap = */         rtkldrRdr_Map,
    /* .pfnRefresh = */     rtkldrRdr_Refresh,
    /* .pfnProtect = */     rtkldrRdr_Protect,
    /* .pfnUnmap =  */      rtkldrRdr_Unmap,
    /* .pfnDone = */        rtkldrRdr_Done,
    /* .u32Dummy = */       42
};





/** @copydoc RTLDROPS::pfnClose */
static DECLCALLBACK(int) rtkldr_Close(PRTLDRMODINTERNAL pMod)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    int rc = kLdrModClose(pModkLdr);
    return rtkldrConvertError(rc);
}


/** @copydoc RTLDROPS::pfnDone */
static DECLCALLBACK(int) rtkldr_Done(PRTLDRMODINTERNAL pMod)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    int rc = kLdrModMostlyDone(pModkLdr);
    return rtkldrConvertError(rc);
}


/** @copydoc FNKLDRMODENUMSYMS */
static int rtkldrEnumSymbolsWrapper(PKLDRMOD pMod, uint32_t iSymbol,
                                    const char *pchSymbol, KSIZE cchSymbol, const char *pszVersion,
                                    KLDRADDR uValue, uint32_t fKind, void *pvUser)
{
    PRTLDRMODKLDRARGS pArgs = (PRTLDRMODKLDRARGS)pvUser;

    /* If not zero terminated we'll have to use a temporary buffer. */
    const char *pszSymbol = pchSymbol;
    if (pchSymbol && pchSymbol[cchSymbol])
    {
        char *psz = (char *)alloca(cchSymbol) + 1;
        memcpy(psz, pchSymbol, cchSymbol);
        psz[cchSymbol] = '\0';
        pszSymbol = psz;
    }

#if defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    /* skip the underscore prefix. */
    if (*pszSymbol == '_')
        pszSymbol++;
#endif

    int rc = pArgs->u.pfnEnumSyms(&pArgs->pMod->Core, pszSymbol, iSymbol, uValue, pArgs->pvUser);
    if (RT_FAILURE(rc))
        return rc; /* don't bother converting. */
    return 0;
}


/** @copydoc RTLDROPS::pfnEnumSymbols */
static DECLCALLBACK(int) rtkldr_EnumSymbols(PRTLDRMODINTERNAL pMod, unsigned fFlags, const void *pvBits, RTUINTPTR BaseAddress,
                                            PFNRTLDRENUMSYMS pfnCallback, void *pvUser)
{
    PKLDRMOD            pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser         = pvUser;
    Args.u.pfnEnumSyms  = pfnCallback;
    Args.pMod           = (PRTLDRMODKLDR)pMod;
    Args.pvBits         = pvBits;
    Args.rc             = VINF_SUCCESS;
    int rc = kLdrModEnumSymbols(pModkLdr, pvBits, BaseAddress,
                                fFlags & RTLDR_ENUM_SYMBOL_FLAGS_ALL ? KLDRMOD_ENUM_SYMS_FLAGS_ALL : 0,
                                rtkldrEnumSymbolsWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    else
        rc = rtkldrConvertError(rc);
    return rc;
}


/** @copydoc RTLDROPS::pfnGetImageSize */
static DECLCALLBACK(size_t) rtkldr_GetImageSize(PRTLDRMODINTERNAL pMod)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    return kLdrModSize(pModkLdr);
}


/** @copydoc FNKLDRMODGETIMPORT */
static int rtkldrGetImportWrapper(PKLDRMOD pMod, uint32_t iImport, uint32_t iSymbol, const char *pchSymbol, KSIZE cchSymbol,
                                  const char *pszVersion, PKLDRADDR puValue, uint32_t *pfKind, void *pvUser)
{
    PRTLDRMODKLDRARGS pArgs = (PRTLDRMODKLDRARGS)pvUser;

    /* If not zero terminated we'll have to use a temporary buffer. */
    const char *pszSymbol = pchSymbol;
    if (pchSymbol && pchSymbol[cchSymbol])
    {
        char *psz = (char *)alloca(cchSymbol) + 1;
        memcpy(psz, pchSymbol, cchSymbol);
        psz[cchSymbol] = '\0';
        pszSymbol = psz;
    }

#if defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    /* skip the underscore prefix. */
    if (*pszSymbol == '_')
        pszSymbol++;
#endif

    /* get the import module name - TODO: cache this */
    const char *pszModule = NULL;
    if (iImport != NIL_KLDRMOD_IMPORT)
    {
        char *pszBuf = (char *)alloca(64);
        int rc = kLdrModGetImport(pMod, pArgs->pvBits, iImport, pszBuf, 64);
        if (rc)
            return rc;
        pszModule = pszBuf;
    }

    /* do the query */
    RTUINTPTR Value;
    int rc = pArgs->u.pfnGetImport(&pArgs->pMod->Core, pszModule, pszSymbol, pszSymbol ? ~0 : iSymbol, &Value, pArgs->pvUser);
    if (RT_SUCCESS(rc))
    {
        *puValue = Value;
        return 0;
    }
    return rtkldrConvertErrorFromIPRT(rc);
}


/** @copydoc RTLDROPS::pfnGetBits */
static DECLCALLBACK(int) rtkldr_GetBits(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR BaseAddress,
                                        PFNRTLDRIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMOD            pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser         = pvUser;
    Args.u.pfnGetImport = pfnGetImport;
    Args.pMod           = (PRTLDRMODKLDR)pMod;
    Args.pvBits         = pvBits;
    Args.rc             = VINF_SUCCESS;
    int rc = kLdrModGetBits(pModkLdr, pvBits, BaseAddress, rtkldrGetImportWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    else
        rc = rtkldrConvertError(rc);
    return rc;
}


/** @copydoc RTLDROPS::pfnRelocate */
static DECLCALLBACK(int) rtkldr_Relocate(PRTLDRMODINTERNAL pMod, void *pvBits, RTUINTPTR NewBaseAddress,
                                         RTUINTPTR OldBaseAddress, PFNRTLDRIMPORT pfnGetImport, void *pvUser)
{
    PKLDRMOD            pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser         = pvUser;
    Args.u.pfnGetImport = pfnGetImport;
    Args.pMod           = (PRTLDRMODKLDR)pMod;
    Args.pvBits         = pvBits;
    Args.rc             = VINF_SUCCESS;
    int rc = kLdrModRelocateBits(pModkLdr, pvBits, NewBaseAddress, OldBaseAddress, rtkldrGetImportWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    else
        rc = rtkldrConvertError(rc);
    return rc;
}


/** @copydoc RTLDROPS::pfnGetSymbolEx */
static DECLCALLBACK(int) rtkldr_GetSymbolEx(PRTLDRMODINTERNAL pMod, const void *pvBits, RTUINTPTR BaseAddress,
                                            const char *pszSymbol, RTUINTPTR *pValue)
{
    PKLDRMOD pModkLdr = ((PRTLDRMODKLDR)pMod)->pMod;
    KLDRADDR uValue;

#if defined(RT_OS_OS2) || defined(RT_OS_DARWIN)
    /*
     * Add underscore prefix.
     */
    if (pszSymbol)
    {
        size_t cch = strlen(pszSymbol);
        char *psz = (char *)alloca(cch + 2);
        memcpy(psz + 1, pszSymbol, cch + 1);
        *psz = '_';
        pszSymbol = psz;
    }
#endif

    int rc = kLdrModQuerySymbol(pModkLdr, pvBits, BaseAddress,
                                NIL_KLDRMOD_SYM_ORDINAL, pszSymbol, strlen(pszSymbol), NULL,
                                NULL, NULL, &uValue, NULL);
    if (!rc)
    {
        *pValue = uValue;
        return VINF_SUCCESS;
    }
    return rtkldrConvertError(rc);
}


/** @copydoc FNKLDRENUMDBG */
static int rtkldrEnumDbgInfoWrapper(PKLDRMOD pMod, KU32 iDbgInfo, KLDRDBGINFOTYPE enmType, KI16 iMajorVer, KI16 iMinorVer,
                                    const char *pszPartNm, KLDRFOFF offFile, KLDRADDR LinkAddress, KLDRSIZE cb,
                                    const char *pszExtFile, void *pvUser)
{
    PRTLDRMODKLDRARGS pArgs = (PRTLDRMODKLDRARGS)pvUser;

    RTLDRDBGINFOTYPE enmMyType;
    switch (enmType)
    {
        case KLDRDBGINFOTYPE_UNKNOWN:   enmMyType = RTLDRDBGINFOTYPE_UNKNOWN;  break;
        case KLDRDBGINFOTYPE_STABS:     enmMyType = RTLDRDBGINFOTYPE_STABS;    break;
        case KLDRDBGINFOTYPE_DWARF:     enmMyType = RTLDRDBGINFOTYPE_DWARF;    break;
        case KLDRDBGINFOTYPE_CODEVIEW:  enmMyType = RTLDRDBGINFOTYPE_CODEVIEW; break;
        case KLDRDBGINFOTYPE_WATCOM:    enmMyType = RTLDRDBGINFOTYPE_WATCOM;   break;
        case KLDRDBGINFOTYPE_HLL:       enmMyType = RTLDRDBGINFOTYPE_HLL;      break;
        default:
            AssertFailed();
            enmMyType = RTLDRDBGINFOTYPE_UNKNOWN;
            break;
    }

    int rc = pArgs->u.pfnEnumDbgInfo(&pArgs->pMod->Core, iDbgInfo, enmMyType, iMajorVer, iMinorVer, pszPartNm,
                                     offFile, LinkAddress, cb, pszExtFile, pArgs->pvUser);
    if (RT_FAILURE(rc))
        return rc; /* don't bother converting. */
    return 0;
}


/** @copydoc RTLDROPS::pfnEnumDbgInfo */
static DECLCALLBACK(int) rtkldr_EnumDbgInfo(PRTLDRMODINTERNAL pMod, const void *pvBits,
                                            PFNRTLDRENUMDBG pfnCallback, void *pvUser)
{
    PRTLDRMODKLDR       pThis = (PRTLDRMODKLDR)pMod;
    RTLDRMODKLDRARGS    Args;
    Args.pvUser             = pvUser;
    Args.u.pfnEnumDbgInfo   = pfnCallback;
    Args.pvBits             = pvBits;
    Args.pMod               = pThis;
    Args.rc                 = VINF_SUCCESS;
    int rc = kLdrModEnumDbgInfo(pThis->pMod, pvBits, rtkldrEnumDbgInfoWrapper, &Args);
    if (Args.rc != VINF_SUCCESS)
        rc = Args.rc;
    return rc;
}


/**
 * Operations for a kLdr module.
 */
static const RTLDROPS g_rtkldrOps =
{
    "kLdr",
    rtkldr_Close,
    NULL,
    rtkldr_Done,
    rtkldr_EnumSymbols,
    /* ext */
    rtkldr_GetImageSize,
    rtkldr_GetBits,
    rtkldr_Relocate,
    rtkldr_GetSymbolEx,
    rtkldr_EnumDbgInfo,
    42
};


/**
 * Open a image using kLdr.
 *
 * @returns iprt status code.
 * @param   pReader     The loader reader instance which will provide the raw image bits.
 * @param   fFlags      Reserved, MBZ.
 * @param   enmArch     CPU architecture specifier for the image to be loaded.
 * @param   phLdrMod    Where to store the handle.
 */
int rtldrkLdrOpen(PRTLDRREADER pReader, uint32_t fFlags, RTLDRARCH enmArch, PRTLDRMOD phLdrMod)
{
    /* Convert enmArch to k-speak. */
    KCPUARCH enmCpuArch;
    switch (enmArch)
    {
        case RTLDRARCH_WHATEVER:
            enmCpuArch = KCPUARCH_UNKNOWN;
            break;
        case RTLDRARCH_X86_32:
            enmCpuArch = KCPUARCH_X86_32;
            break;
        case RTLDRARCH_AMD64:
            enmCpuArch = KCPUARCH_AMD64;
            break;
        default:
            return VERR_INVALID_PARAMETER;
    }

    /* Create a rtkldrRdr_ instance. */
    PRTKLDRRDR pRdr = (PRTKLDRRDR)RTMemAllocZ(sizeof(*pRdr));
    if (!pRdr)
        return VERR_NO_MEMORY;
    pRdr->Core.u32Magic = KRDR_MAGIC;
    pRdr->Core.pOps = &g_kLdrRdrFileOps;
    pRdr->pReader = pReader;

    /* Try open it. */
    PKLDRMOD pMod;
    int krc = kLdrModOpenFromRdr(&pRdr->Core, fFlags, enmCpuArch, &pMod);
    if (!krc)
    {
        /* Create a module wrapper for it. */
        PRTLDRMODKLDR pNewMod = (PRTLDRMODKLDR)RTMemAllocZ(sizeof(*pNewMod));
        if (pNewMod)
        {
            pNewMod->Core.u32Magic = RTLDRMOD_MAGIC;
            pNewMod->Core.eState = LDR_STATE_OPENED;
            pNewMod->Core.pOps = &g_rtkldrOps;
            pNewMod->pMod = pMod;
            *phLdrMod = &pNewMod->Core;

#ifdef LOG_ENABLED
            Log(("rtldrkLdrOpen: '%s' (%s) %u segments\n",
                 pMod->pszName, pMod->pszFilename, pMod->cSegments));
            for (unsigned iSeg = 0; iSeg < pMod->cSegments; iSeg++)
            {
                Log(("Segment #%-2u: RVA=%08llx cb=%08llx '%.*s'\n", iSeg,
                     pMod->aSegments[iSeg].RVA,
                     pMod->aSegments[iSeg].cb,
                     pMod->aSegments[iSeg].cchName,
                     pMod->aSegments[iSeg].pchName));
            }
#endif
            return VINF_SUCCESS;
        }
        kLdrModClose(pMod);
        krc = KERR_NO_MEMORY;
    }
    return rtkldrConvertError(krc);
}


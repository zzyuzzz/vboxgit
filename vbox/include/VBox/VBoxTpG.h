/* $Id: VBoxTpG.h 40986 2012-04-19 10:32:03Z vboxsync $ */
/** @file
 * VBox Tracepoint Generator Structures.
 */

/*
 * Copyright (C) 2012 Oracle Corporation
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


#ifndef ___VBox_VTG_h___
#define ___VBox_VTG_h___

#include <iprt/types.h>
#include <iprt/assert.h>

RT_C_DECLS_BEGIN


/**
 * Probe location.
 */
typedef struct VTGPROBELOC
{
    uint32_t    uLine    : 31;
    uint32_t    fEnabled : 1;
    uint32_t    idProbe;
    const char *pszFunction;
    uint8_t    *pbProbe;
#if ARCH_BITS == 64
    uintptr_t   uAlignment;
#endif
} VTGPROBELOC;
AssertCompileSizeAlignment(VTGPROBELOC, 16);
/** Pointer to a probe location. */
typedef VTGPROBELOC *PVTGPROBELOC;

/** @def VTG_OBJ_SECT
 * The name of the section containing the other probe data provided by the
 * assembly / object generated by VBoxTpG. */
/** @def VTG_LOC_SECT
 * The name of the section containing the VTGPROBELOC structures.  This is
 * filled by the probe macros, @see VTG_DECL_VTGPROBELOC. */
/** @def VTG_DECL_VTGPROBELOC
 * Declares a static variable, @a a_VarName, of type VTGPROBELOC in the section
 * indicated by VTG_LOC_SECT.  */
#if defined(RT_OS_WINDOWS)
# define VTG_OBJ_SECT       "VTGObj"
# define VTG_LOC_SECT       "VTGPrLc.Data"
# ifdef _MSC_VER
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    __declspec(allocate(VTG_LOC_SECT)) static VTGPROBELOC a_VarName
# elif defined(__GNUC__)
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC __attribute__((section(VTG_LOC_SECT))) a_VarName
# else
#  error "Unsupported Windows compiler!"
# endif

#elif defined(RT_OS_DARWIN)
# define VTG_OBJ_SECT       "__VTGObj"
# define VTG_LOC_SECT       "__VTGPrLc"
# define VTG_LOC_SEG        "__VTG"
# ifdef __GNUC__
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC __attribute__((section(VTG_LOC_SEG "," VTG_LOC_SECT ",regular")/*, aligned(16)*/)) a_VarName
# else
#  error "Unsupported Darwin compiler!"
# endif

#elif defined(RT_OS_OS2)
# error "OS/2 is not supported"

#else /* Assume the rest uses ELF. */
# define VTG_OBJ_SECT       ".VTGObj"
# define VTG_LOC_SECT       ".VTGPrLc"
# ifdef __GNUC__
#  define VTG_DECL_VTGPROBELOC(a_VarName) \
    static VTGPROBELOC __attribute__((section(VTG_LOC_SECT))) a_VarName
# else
#  error "Unsupported compiler!"
# endif
#endif

/** VTG string table offset. */
typedef uint32_t VTGSTROFF;


/** @name VTG type flags
 * @{ */
/** Masking out the fixed size if given. */
#define VTG_TYPE_SIZE_MASK      UINT32_C(0x000000ff)
/** Indicates that VTG_TYPE_SIZE_MASK can be applied, UNSIGNED or SIGNED is
 * usually set as well, so may PHYS. */
#define VTG_TYPE_FIXED_SIZED    RT_BIT_32(8)
/** It's a pointer type, the size is given by the context the probe fired in. */
#define VTG_TYPE_POINTER        RT_BIT_32(9)
/** A context specfic pointer or address, consult VTG_TYPE_CTX_XXX. */
#define VTG_TYPE_CTX_POINTER    RT_BIT_32(10)
/** The type has the same size as the host architecture. */
#define VTG_TYPE_HC_ARCH_SIZED  RT_BIT_32(11)
/** The type applies to ring-3 context. */
#define VTG_TYPE_CTX_R3         RT_BIT_32(24)
/** The type applies to ring-0 context. */
#define VTG_TYPE_CTX_R0         RT_BIT_32(25)
/** The type applies to raw-mode context. */
#define VTG_TYPE_CTX_RC         RT_BIT_32(26)
/** The type applies to guest context. */
#define VTG_TYPE_CTX_GST        RT_BIT_32(27)
/** The type context mask. */
#define VTG_TYPE_CTX_MASK       UINT32_C(0x0f000000)
/** The type is a physical address. */
#define VTG_TYPE_PHYS           RT_BIT_32(29)
/** The type is unsigned. */
#define VTG_TYPE_UNSIGNED       RT_BIT_32(30)
/** The type is signed. */
#define VTG_TYPE_SIGNED         RT_BIT_32(31)
/** Mask of valid bits (for simple validation). */
#define VTG_TYPE_VALID_MASK     UINT32_C(0xef000fff)
/** @} */

/**
 * Checks if the VTG type flags indicates a large fixed size argument.
 */
#define VTG_TYPE_IS_LARGE(a_fType) \
    ( ((a_fType) & VTG_TYPE_SIZE_MASK) > 4 && ((a_fType) & VTG_TYPE_FIXED_SIZED) )


/**
 * VTG argument descriptor.
 */
typedef struct VTGDESCARG
{
    VTGSTROFF       offType;
    uint32_t        fType;
} VTGDESCARG;
/** Pointer to an argument descriptor. */
typedef VTGDESCARG         *PVTGDESCARG;


/**
 * VTG argument list descriptor.
 */
typedef struct VTGDESCARGLIST
{
    uint8_t         cArgs;
    uint8_t         fHaveLargeArgs;
    uint8_t         abReserved[2];
    VTGDESCARG      aArgs[1];
} VTGDESCARGLIST;
/** Pointer to a VTG argument list descriptor. */
typedef VTGDESCARGLIST     *PVTGDESCARGLIST;


/**
 * VTG probe descriptor.
 */
typedef struct VTGDESCPROBE
{
    VTGSTROFF       offName;
    uint32_t        offArgList;
    uint16_t        idxEnabled;
    uint16_t        idxProvider;
    /** The distance from this structure to the VTG object header. */
    int32_t         offObjHdr;
    uint32_t        u32User;
    uint32_t        u32User2;
} VTGDESCPROBE;
AssertCompileSize(VTGDESCPROBE, 24);
/** Pointer to a VTG probe descriptor. */
typedef VTGDESCPROBE       *PVTGDESCPROBE;


/**
 * Code/data stability.
 */
typedef enum kVTGStability
{
    kVTGStability_Invalid = 0,
    kVTGStability_Internal,
    kVTGStability_Private,
    kVTGStability_Obsolete,
    kVTGStability_External,
    kVTGStability_Unstable,
    kVTGStability_Evolving,
    kVTGStability_Stable,
    kVTGStability_Standard,
    kVTGStability_End
} kVTGStability;

/**
 * Data dependency.
 */
typedef enum kVTGClass
{
    kVTGClass_Invalid = 0,
    kVTGClass_Unknown,
    kVTGClass_Cpu,
    kVTGClass_Platform,
    kVTGClass_Group,
    kVTGClass_Isa,
    kVTGClass_Common,
    kVTGClass_End
} kVTGClass;


/**
 * VTG attributes.
 */
typedef struct VTGDESCATTR
{
    uint8_t         u8Code;
    uint8_t         u8Data;
    uint8_t         u8DataDep;
} VTGDESCATTR;
AssertCompileSize(VTGDESCATTR, 3);
/** Pointer to a const VTG attribute. */
typedef VTGDESCATTR const *PCVTGDESCATTR;


/**
 * VTG provider descriptor.
 */
typedef struct VTGDESCPROVIDER
{
    VTGSTROFF       offName;
    uint16_t        iFirstProbe;
    uint16_t        cProbes;
    VTGDESCATTR     AttrSelf;
    VTGDESCATTR     AttrModules;
    VTGDESCATTR     AttrFunctions;
    VTGDESCATTR     AttrNames;
    VTGDESCATTR     AttrArguments;
    uint8_t         bReserved;
} VTGDESCPROVIDER;
/** Pointer to a VTG provider descriptor. */
typedef VTGDESCPROVIDER    *PVTGDESCPROVIDER;


/**
 * VTG data object header.
 */
typedef struct VTGOBJHDR
{
    char                szMagic[24];
    uint32_t            cBits;
    uint32_t            u32Reserved0;
    PVTGDESCPROVIDER    paProviders;
    uintptr_t           cbProviders;
    PVTGDESCPROBE       paProbes;
    uintptr_t           cbProbes;
    bool               *pafProbeEnabled;
    uintptr_t           cbProbeEnabled;
    char               *pachStrTab;
    uintptr_t           cbStrTab;
    PVTGDESCARGLIST     paArgLists;
    uintptr_t           cbArgLists;
    PVTGPROBELOC        paProbLocs;
    PVTGPROBELOC        paProbLocsEnd;
    uintptr_t           auReserved1[4];
} VTGOBJHDR;
/** Pointer to a VTG data object header. */
typedef VTGOBJHDR          *PVTGOBJHDR;

/** The current VTGOBJHDR::szMagic value. */
#define VTGOBJHDR_MAGIC     "VTG Object Header v1.4\0"

/** The name of the VTG data object header symbol in the object file. */
extern VTGOBJHDR            g_VTGObjHeader;

RT_C_DECLS_END

#endif


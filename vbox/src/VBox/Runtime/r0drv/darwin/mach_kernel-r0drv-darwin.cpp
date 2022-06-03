/* $Id: mach_kernel-r0drv-darwin.cpp 37577 2011-06-21 13:01:00Z vboxsync $ */
/** @file
 * IPRT - mach_kernel symbol resolving hack, R0 Driver, Darwin.
 */

/*
 * Copyright (C) 2011 Oracle Corporation
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


#define RTMEM_WRAP_TO_EF_APIS
/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifdef IN_RING0
# include "the-darwin-kernel.h"
# include <sys/kauth.h>
RT_C_DECLS_BEGIN /* Buggy 10.4 headers, fixed in 10.5. */
# include <sys/kpi_mbuf.h>
# include <net/kpi_interfacefilter.h>
# include <sys/kpi_socket.h>
# include <sys/kpi_socketfilter.h>
RT_C_DECLS_END
# include <sys/buf.h>
# include <sys/vm.h>
# include <sys/vnode_if.h>
/*# include <sys/sysctl.h>*/
# include <sys/systm.h>
# include <vfs/vfs_support.h>
/*# include <miscfs/specfs/specdev.h>*/
#endif

#include "internal/iprt.h"
#include <iprt/darwin/machkernel.h>

#ifdef IN_RING0  /* till RTFILE is changed in types.h */
# include <iprt/types.h>
typedef struct RTFILENEWINT *RTFILENEW;
typedef RTFILENEW *PRTFILENEW;
# undef NIL_RTFILE
# define RTFILE         RTFILENEW
# define PRTFILE        PRTFILENEW
# define NIL_RTFILE     ((RTFILENEW)-1)
#endif

#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/file.h>
#include <iprt/log.h>
#include <iprt/mem.h>
#include <iprt/string.h>
#include "internal/ldrMach-O.h"

/** @def MY_CPU_TYPE
 * The CPU type targeted by the compiler. */
/** @def MY_CPU_TYPE
 * The "ALL" CPU subtype targeted by the compiler. */
/** @def MY_MACHO_HEADER
 * The Mach-O header targeted by the compiler.  */
/** @def MY_MACHO_MAGIC
 * The Mach-O header magic we're targeting.  */
/** @def MY_SEGMENT_COMMAND
 * The segment command targeted by the compiler.  */
/** @def MY_SECTION
 * The section struture targeted by the compiler.  */
/** @def MY_NLIST
 * The symbol table entry targeted by the compiler.  */
#ifdef RT_ARCH_X86
# define MY_CPU_TYPE            CPU_TYPE_I386
# define MY_CPU_SUBTYPE_ALL     CPU_SUBTYPE_I386_ALL
# define MY_MACHO_HEADER        mach_header_32_t
# define MY_MACHO_MAGIC         IMAGE_MACHO32_SIGNATURE
# define MY_SEGMENT_COMMAND     segment_command_32_t
# define MY_SECTION             section_32_t
# define MY_NLIST               macho_nlist_32_t

#elif defined(RT_ARCH_AMD64)
# define MY_CPU_TYPE            CPU_TYPE_X86_64
# define MY_CPU_SUBTYPE_ALL     CPU_SUBTYPE_X86_64_ALL
# define MY_MACHO_HEADER        mach_header_64_t
# define MY_MACHO_MAGIC         IMAGE_MACHO64_SIGNATURE
# define MY_SEGMENT_COMMAND     segment_command_64_t
# define MY_SECTION             section_64_t
# define MY_NLIST               macho_nlist_64_t

#else
# error "Port me!"
#endif

/** @name Return macros for make it simpler to track down too paranoid code.
 * @{
 */
#ifdef DEBUG
# define RETURN_VERR_BAD_EXE_FORMAT \
    do { Assert(!g_fBreakpointOnError);         return VERR_BAD_EXE_FORMAT; } while (0)
# define RETURN_VERR_LDR_UNEXPECTED \
    do { Assert(!g_fBreakpointOnError);         return VERR_LDR_UNEXPECTED; } while (0)
# define RETURN_VERR_LDR_ARCH_MISMATCH \
    do { Assert(!g_fBreakpointOnError);         return VERR_LDR_ARCH_MISMATCH; } while (0)
#else
# define RETURN_VERR_BAD_EXE_FORMAT     do {    return VERR_BAD_EXE_FORMAT; } while (0)
# define RETURN_VERR_LDR_UNEXPECTED     do {    return VERR_LDR_UNEXPECTED; } while (0)
# define RETURN_VERR_LDR_ARCH_MISMATCH  do {    return VERR_LDR_ARCH_MISMATCH; } while (0)
#endif
/** @} */

#define VERR_LDR_UNEXPECTED     (-641)


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Our internal representation of the mach_kernel after loading it's symbols
 * and successfully resolving their addresses.
 */
typedef struct RTR0MACHKERNELINT
{
    /** @name Result.
     * @{ */
    /** Pointer to the string table. */
    char               *pachStrTab;
    /** The size of the string table. */
    uint32_t            cbStrTab;
    /** The file offset of the string table. */
    uint32_t            offStrTab;
    /** Pointer to the symbol table. */
    MY_NLIST           *paSyms;
    /** The size of the symbol table. */
    uint32_t            cSyms;
    /** The file offset of the symbol table. */
    uint32_t            offSyms;
    /** @} */

    /** @name Used during loading.
     * @{ */
    /** The file handle.  */
    RTFILE              hFile;
    /** The architecture image offset (fat_arch_t::offset). */
    uint64_t            offArch;
    /** The architecture image size (fat_arch_t::size). */
    uint32_t            cbArch;
    /** The number of load commands (mach_header_XX_t::ncmds). */
    uint32_t            cLoadCmds;
    /** The size of the load commands. */
    uint32_t            cbLoadCmds;
    /** The load commands. */
    load_command_t     *pLoadCmds;
    /** Section pointer table (points into the load commands). */
    MY_SECTION const   *apSections[MACHO_MAX_SECT];
    /** The number of sections. */
    uint32_t            cSections;
    /** @} */

    /** Buffer space. */
    char                abBuf[_4K];
} RTR0MACHKERNELINT;


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
#ifdef DEBUG
static bool g_fBreakpointOnError = false;
#endif


#ifdef IN_RING0

/** Default file permissions for newly created files. */
#if defined(S_IRUSR) && defined(S_IWUSR)
# define RT_FILE_PERMISSION  (S_IRUSR | S_IWUSR)
#else
# define RT_FILE_PERMISSION  (00600)
#endif

/**
 * Darwin kernel file handle data.
 */
typedef struct RTFILENEWINT
{
    /** Magic value (RTFILE_MAGIC). */
    uint32_t        u32Magic;
    /** The open mode flags passed to the kernel API. */
    int             fOpenMode;
    /** The open flags passed to RTFileOpen. */
    uint64_t        fOpen;
    /** The VFS context in which the file was opened. */
    vfs_context_t   hVfsCtx;
    /** The vnode returned by vnode_open. */
    vnode_t         hVnode;
} RTFILENEWINT;
/** Magic number for RTFILENEWINT::u32Magic (To Be Determined). */
#define RTFILE_MAGIC                    UINT32_C(0x01020304)


RTDECL(int) RTFileOpen(PRTFILE phFile, const char *pszFilename, uint32_t fOpen)
{
    RTFILENEWINT *pThis = (RTFILENEWINT *)RTMemAllocZ(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;

    errno_t rc;
    pThis->u32Magic = RTFILE_MAGIC;
    pThis->fOpen    = fOpen;
    pThis->hVfsCtx  = vfs_context_current();
    if (pThis->hVfsCtx != NULL)
    {
        int             fCMode    = (fOpen & RTFILE_O_CREATE_MODE_MASK)
                                  ? (fOpen & RTFILE_O_CREATE_MODE_MASK) >> RTFILE_O_CREATE_MODE_SHIFT
                                  : RT_FILE_PERMISSION;
        int             fVnFlags  = 0; /* VNODE_LOOKUP_XXX */
        int             fOpenMode = 0;
        if (fOpen & RTFILE_O_NON_BLOCK)
            fOpenMode |= O_NONBLOCK;
        if (fOpen & RTFILE_O_WRITE_THROUGH)
            fOpenMode |= O_SYNC;

        /* create/truncate file */
        switch (fOpen & RTFILE_O_ACTION_MASK)
        {
            case RTFILE_O_OPEN:             break;
            case RTFILE_O_OPEN_CREATE:      fOpenMode |= O_CREAT; break;
            case RTFILE_O_CREATE:           fOpenMode |= O_CREAT | O_EXCL; break;
            case RTFILE_O_CREATE_REPLACE:   fOpenMode |= O_CREAT | O_TRUNC; break; /** @todo replacing needs fixing, this is *not* a 1:1 mapping! */
        }
        if (fOpen & RTFILE_O_TRUNCATE)
            fOpenMode |= O_TRUNC;

        switch (fOpen & RTFILE_O_ACCESS_MASK)
        {
            case RTFILE_O_READ:
                fOpenMode |= FREAD;
                break;
            case RTFILE_O_WRITE:
                fOpenMode |= fOpen & RTFILE_O_APPEND ? O_APPEND | FWRITE : FWRITE;
                break;
            case RTFILE_O_READWRITE:
                fOpenMode |= fOpen & RTFILE_O_APPEND ? O_APPEND | FWRITE | FREAD : FWRITE | FREAD;
                break;
            default:
                AssertMsgFailed(("RTFileOpen received an invalid RW value, fOpen=%#x\n", fOpen));
                return VERR_INVALID_PARAMETER;
        }

        pThis->fOpenMode = fOpenMode;
        rc = vnode_open(pszFilename, fOpenMode, fCMode, fVnFlags, &pThis->hVnode, pThis->hVfsCtx);
        if (rc == 0)
        {
            *phFile = pThis;
            return VINF_SUCCESS;
        }

        rc = RTErrConvertFromErrno(rc);
    }
    else
        rc = VERR_INTERNAL_ERROR_5;
    RTMemFree(pThis);

    return rc;
}


RTDECL(int) RTFileClose(RTFILE hFile)
{
    if (hFile == NIL_RTFILE)
        return VINF_SUCCESS;

    RTFILENEWINT *pThis = hFile;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTFILE_MAGIC, VERR_INVALID_HANDLE);
    pThis->u32Magic = ~RTFILE_MAGIC;

    errno_t rc = vnode_close(pThis->hVnode, pThis->fOpenMode & (FREAD | FWRITE), pThis->hVfsCtx);

    RTMemFree(pThis);
    return RTErrConvertFromErrno(rc);
}


RTDECL(int) RTFileReadAt(RTFILE hFile, RTFOFF off, void *pvBuf, size_t cbToRead, size_t *pcbRead)
{
    RTFILENEWINT *pThis = hFile;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->u32Magic == RTFILE_MAGIC, VERR_INVALID_HANDLE);

    off_t offNative = (off_t)off;
    AssertReturn((RTFOFF)offNative == off, VERR_OUT_OF_RANGE);

#if 0 /* Added in 10.6, grr. */
    errno_t rc;
    if (!pcbRead)
        rc = vn_rdwr(UIO_READ, pThis->hVnode, (char *)pvBuf, cbToRead, offNative, UIO_SYSSPACE, 0 /*ioflg*/,
                     vfs_context_ucred(pThis->hVfsCtx), NULL, vfs_context_proc(pThis->hVfsCtx));
    else
    {
        int cbLeft = 0;
        rc = vn_rdwr(UIO_READ, pThis->hVnode, (char *)pvBuf, cbToRead, offNative, UIO_SYSSPACE, 0 /*ioflg*/,
                     vfs_context_ucred(pThis->hVfsCtx), &cbLeft, vfs_context_proc(pThis->hVfsCtx));
        *pcbRead = cbToRead - cbLeft;
    }
    return !rc ? VINF_SUCCESS : RTErrConvertFromErrno(rc);

#else
    uio_t hUio = uio_create(1, offNative, UIO_SYSSPACE, UIO_READ);
    if (!hUio)
        return VERR_NO_MEMORY;
    errno_t rc;
    if (uio_addiov(hUio, (user_addr_t)(uintptr_t)pvBuf, cbToRead) == 0)
    {
        rc = VNOP_READ(pThis->hVnode, hUio, 0 /*ioflg*/, pThis->hVfsCtx);
        if (pcbRead)
            *pcbRead = cbToRead - uio_resid(hUio);
        else if (!rc && uio_resid(hUio))
            rc = VERR_FILE_IO_ERROR;
    }
    else
        rc = VERR_INTERNAL_ERROR_3;
    uio_free(hUio);
    return rc;

#endif
}

#endif


/**
 * Close and free up resources we no longer needs.
 *
 * @param   pThis               The internal scratch data.
 */
static void rtR0MachKernelLoadDone(RTR0MACHKERNELINT *pThis)
{
    RTFileClose(pThis->hFile);
    pThis->hFile = NIL_RTFILE;

    RTMemFree(pThis->pLoadCmds);
    pThis->pLoadCmds = NULL;
    memset((void *)&pThis->apSections[0], 0, sizeof(pThis->apSections[0]) * MACHO_MAX_SECT);
}


/**
 * Looks up a kernel symbol.
 *
 *
 * @returns The symbol address on success, 0 on failure.
 * @param   pThis               The internal scratch data.
 * @param   pszSymbol           The symbol to resolve.  Automatically prefixed
 *                              with an underscore.
 */
static uintptr_t rtR0MachKernelLookup(RTR0MACHKERNELINT *pThis, const char *pszSymbol)
{
    uint32_t const  cSyms = pThis->cSyms;
    MY_NLIST const *pSym = pThis->paSyms;

#if 1
    /* linear search. */
    for (uint32_t iSym = 0; iSym < cSyms; iSym++, pSym++)
    {
        if (pSym->n_type & MACHO_N_STAB)
            continue;

        const char *pszTabName= &pThis->pachStrTab[(uint32_t)pSym->n_un.n_strx];
        if (   *pszTabName == '_'
            && strcmp(pszTabName + 1, pszSymbol) == 0)
            return pSym->n_value;
    }
#else
    /** @todo binary search. */

#endif
    return 0;
}


/* Rainy day: Find the right headers for these symbols ... if there are any. */
extern "C" void ev_try_lock(void);
extern "C" void OSMalloc(void);
extern "C" void OSlibkernInit(void);
extern "C" int  osrelease;
extern "C" int  ostype;
extern "C" void kdp_set_interface(void);


/**
 * Check the symbol table against symbols we known symbols.
 *
 * This is done to detect whether the on disk image and the in
 * memory images matches.  Mismatches could stem from user
 * replacing the default kernel image on disk.
 *
 * @returns IPRT status code.
 * @param   pThis               The internal scratch data.
 */
static int rtR0MachKernelCheckStandardSymbols(RTR0MACHKERNELINT *pThis)
{
    static struct
    {
        const char *pszName;
        uintptr_t   uAddr;
    } const s_aStandardCandles[] =
    {
#ifdef IN_RING0
# define KNOWN_ENTRY(a_Sym)  { #a_Sym, (uintptr_t)&a_Sym }
#else
# define KNOWN_ENTRY(a_Sym)  { #a_Sym, 0 }
#endif
        /* IOKit: */
        KNOWN_ENTRY(IOMalloc),
        KNOWN_ENTRY(IOFree),
        KNOWN_ENTRY(IOSleep),
        KNOWN_ENTRY(IORWLockAlloc),
        KNOWN_ENTRY(IORecursiveLockLock),
        KNOWN_ENTRY(IOSimpleLockAlloc),
        KNOWN_ENTRY(PE_cpu_halt),
        KNOWN_ENTRY(gIOKitDebug),
        KNOWN_ENTRY(gIOServicePlane),
        KNOWN_ENTRY(ev_try_lock),

        /* Libkern: */
        KNOWN_ENTRY(OSAddAtomic),
        KNOWN_ENTRY(OSBitAndAtomic),
        KNOWN_ENTRY(OSBitOrAtomic),
        KNOWN_ENTRY(OSBitXorAtomic),
        KNOWN_ENTRY(OSCompareAndSwap),
        KNOWN_ENTRY(OSMalloc),
        KNOWN_ENTRY(OSlibkernInit),
        KNOWN_ENTRY(bcmp),
        KNOWN_ENTRY(copyout),
        KNOWN_ENTRY(copyin),
        KNOWN_ENTRY(kprintf),
        KNOWN_ENTRY(printf),
        KNOWN_ENTRY(lck_grp_alloc_init),
        KNOWN_ENTRY(lck_mtx_alloc_init),
        KNOWN_ENTRY(lck_rw_alloc_init),
        KNOWN_ENTRY(lck_spin_alloc_init),
        KNOWN_ENTRY(osrelease),
        KNOWN_ENTRY(ostype),
        KNOWN_ENTRY(panic),
        KNOWN_ENTRY(strprefix),
        KNOWN_ENTRY(sysctlbyname),
        KNOWN_ENTRY(vsscanf),
        KNOWN_ENTRY(page_mask),

        /* Mach: */
        KNOWN_ENTRY(absolutetime_to_nanoseconds),
        KNOWN_ENTRY(assert_wait),
        KNOWN_ENTRY(clock_delay_until),
        KNOWN_ENTRY(clock_get_uptime),
        KNOWN_ENTRY(current_task),
        KNOWN_ENTRY(current_thread),
        KNOWN_ENTRY(kernel_task),
        KNOWN_ENTRY(lck_mtx_sleep),
        KNOWN_ENTRY(lck_rw_sleep),
        KNOWN_ENTRY(lck_spin_sleep),
        KNOWN_ENTRY(mach_absolute_time),
        KNOWN_ENTRY(semaphore_create),
        KNOWN_ENTRY(task_reference),
        KNOWN_ENTRY(thread_block),
        KNOWN_ENTRY(thread_reference),
        KNOWN_ENTRY(thread_terminate),
        KNOWN_ENTRY(thread_wakeup_prim),

        /* BSDKernel: */
        KNOWN_ENTRY(buf_size),
        KNOWN_ENTRY(copystr),
        KNOWN_ENTRY(current_proc),
        KNOWN_ENTRY(ifnet_hdrlen),
        KNOWN_ENTRY(ifnet_set_promiscuous),
        KNOWN_ENTRY(kauth_getuid),
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1050
        KNOWN_ENTRY(kauth_cred_unref),
#else
        KNOWN_ENTRY(kauth_cred_rele),
#endif
        KNOWN_ENTRY(mbuf_data),
        KNOWN_ENTRY(msleep),
        KNOWN_ENTRY(nanotime),
        KNOWN_ENTRY(nop_close),
        KNOWN_ENTRY(proc_pid),
        KNOWN_ENTRY(sock_accept),
        KNOWN_ENTRY(sockopt_name),
        //KNOWN_ENTRY(spec_write),
        KNOWN_ENTRY(suword),
        //KNOWN_ENTRY(sysctl_int),
        KNOWN_ENTRY(uio_rw),
        KNOWN_ENTRY(vfs_flags),
        KNOWN_ENTRY(vfs_name),
        KNOWN_ENTRY(vfs_statfs),
        KNOWN_ENTRY(vn_rdwr),
        KNOWN_ENTRY(vnode_get),
        KNOWN_ENTRY(vnode_open),
        KNOWN_ENTRY(vnode_ref),
        KNOWN_ENTRY(vnode_rele),
        KNOWN_ENTRY(vnop_close_desc),
        KNOWN_ENTRY(wakeup),
        KNOWN_ENTRY(wakeup_one),

        /* Unsupported: */
        KNOWN_ENTRY(kdp_set_interface),
        KNOWN_ENTRY(pmap_find_phys),
        KNOWN_ENTRY(vm_map),
        KNOWN_ENTRY(vm_protect),
        KNOWN_ENTRY(vm_region),
        KNOWN_ENTRY(vm_map_wire),
        KNOWN_ENTRY(PE_kputc),
    };

    for (unsigned i = 0; i < RT_ELEMENTS(s_aStandardCandles); i++)
    {
        uintptr_t uAddr = rtR0MachKernelLookup(pThis, s_aStandardCandles[i].pszName);
#ifdef IN_RING0
        if (uAddr != s_aStandardCandles[i].uAddr)
#else
        if (uAddr == 0)
#endif
        {
            AssertLogRelMsgFailed(("%s (%p != %p)\n", s_aStandardCandles[i].pszName, uAddr, s_aStandardCandles[i].uAddr));
            return VERR_INTERNAL_ERROR_2;
        }
    }
    return VINF_SUCCESS;
}


/**
 * Loads and validates the symbol and string tables.
 *
 * @returns IPRT status code.
 * @param   pThis               The internal scratch data.
 */
static int rtR0MachKernelLoadSymTab(RTR0MACHKERNELINT *pThis)
{
    /*
     * Load the tables.
     */
    pThis->paSyms = (MY_NLIST *)RTMemAllocZ(pThis->cSyms * sizeof(MY_NLIST));
    if (!pThis->paSyms)
        return VERR_NO_MEMORY;

    int rc = RTFileReadAt(pThis->hFile, pThis->offArch + pThis->offSyms,
                          pThis->paSyms, pThis->cSyms * sizeof(MY_NLIST), NULL);
    if (RT_FAILURE(rc))
        return rc;

    pThis->pachStrTab = (char *)RTMemAllocZ(pThis->cbStrTab + 1);
    if (!pThis->pachStrTab)
        return VERR_NO_MEMORY;

    rc = RTFileReadAt(pThis->hFile, pThis->offArch + pThis->offStrTab,
                      pThis->pachStrTab, pThis->cbStrTab, NULL);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * The first string table symbol must be a zero length name.
     */
    if (pThis->pachStrTab[0] != '\0')
        RETURN_VERR_BAD_EXE_FORMAT;

    /*
     * Validate the symbol table.
     */
    const char     *pszPrev = "";
    uint32_t const  cSyms   = pThis->cSyms;
    MY_NLIST const  *pSym   = pThis->paSyms;
    for (uint32_t iSym = 0; iSym < cSyms; iSym++, pSym++)
    {
        if ((uint32_t)pSym->n_un.n_strx >= pThis->cbStrTab)
            RETURN_VERR_BAD_EXE_FORMAT;
        const char *pszSym = &pThis->pachStrTab[(uint32_t)pSym->n_un.n_strx];
#ifdef IN_RING3
        RTAssertMsg2("%05i: %02x:%08x %02x %04x %s\n", iSym, pSym->n_sect, pSym->n_value, pSym->n_type, pSym->n_desc, pszSym);
#endif

        if (strcmp(pszSym, pszPrev) < 0)
            RETURN_VERR_BAD_EXE_FORMAT; /* not sorted */

        if (!(pSym->n_type & MACHO_N_STAB))
        {
            switch (pSym->n_type & MACHO_N_TYPE)
            {
                case MACHO_N_SECT:
                    if (pSym->n_sect == MACHO_NO_SECT)
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (pSym->n_sect > pThis->cSections)
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (pSym->n_desc & ~(REFERENCED_DYNAMICALLY))
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (pSym->n_value < pThis->apSections[pSym->n_sect - 1]->addr)
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (   pSym->n_value - pThis->apSections[pSym->n_sect - 1]->addr
                        > pThis->apSections[pSym->n_sect - 1]->size)
                        RETURN_VERR_BAD_EXE_FORMAT;
                    break;

                case MACHO_N_ABS:
#if 0 /* Spec say MACHO_NO_SECT, __mh_execute_header has 1 with 10.7/amd64 */
                    if (pSym->n_sect != MACHO_NO_SECT)
#else
                    if (pSym->n_sect > pThis->cSections)
#endif
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (pSym->n_desc & ~(REFERENCED_DYNAMICALLY))
                        RETURN_VERR_BAD_EXE_FORMAT;
                    break;

                case MACHO_N_UNDF:
                    /* No undefined or common symbols in the kernel. */
                    RETURN_VERR_BAD_EXE_FORMAT;

                case MACHO_N_INDR:
                    /* No indirect symbols in the kernel. */
                    RETURN_VERR_BAD_EXE_FORMAT;

                case MACHO_N_PBUD:
                    /* No prebound symbols in the kernel. */
                    RETURN_VERR_BAD_EXE_FORMAT;

                default:
                    RETURN_VERR_BAD_EXE_FORMAT;
            }
        }
        /* else: Ignore debug symbols. */
    }

    return VINF_SUCCESS;
}


/**
 * Loads the load commands and validates them.
 *
 * @returns IPRT status code.
 * @param   pThis               The internal scratch data.
 */
static int rtR0MachKernelLoadCommands(RTR0MACHKERNELINT *pThis)
{
    pThis->offStrTab = 0;
    pThis->cbStrTab  = 0;
    pThis->offSyms   = 0;
    pThis->cSyms     = 0;
    pThis->cSections = 0;

    pThis->pLoadCmds = (load_command_t *)RTMemAlloc(pThis->cbLoadCmds);
    if (!pThis->pLoadCmds)
        return VERR_NO_MEMORY;

    int rc = RTFileReadAt(pThis->hFile, pThis->offArch + sizeof(MY_MACHO_HEADER),
                          pThis->pLoadCmds, pThis->cbLoadCmds, NULL);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Validate the relevant commands, picking up sections and the symbol
     * table location.
     */
    load_command_t const   *pCmd = pThis->pLoadCmds;
    for (uint32_t iCmd = 0; ; iCmd++)
    {
        /* cmd index & offset. */
        uintptr_t offCmd = (uintptr_t)pCmd - (uintptr_t)pThis->pLoadCmds;
        if (offCmd == pThis->cbLoadCmds && iCmd == pThis->cLoadCmds)
            break;
        if (offCmd + sizeof(*pCmd) > pThis->cbLoadCmds)
            RETURN_VERR_BAD_EXE_FORMAT;
        if (iCmd >= pThis->cLoadCmds)
            RETURN_VERR_BAD_EXE_FORMAT;

        /* cmdsize */
        if (pCmd->cmdsize < sizeof(*pCmd))
            RETURN_VERR_BAD_EXE_FORMAT;
        if (pCmd->cmdsize > pThis->cbLoadCmds)
            RETURN_VERR_BAD_EXE_FORMAT;
        if (RT_ALIGN_32(pCmd->cmdsize, 4) != pCmd->cmdsize)
            RETURN_VERR_BAD_EXE_FORMAT;

        /* cmd */
        switch (pCmd->cmd & ~LC_REQ_DYLD)
        {
            /* Validate and store the symbol table details. */
            case LC_SYMTAB:
            {
                struct symtab_command const *pSymTab = (struct symtab_command const *)pCmd;
                if (pSymTab->cmdsize != sizeof(*pSymTab))
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (pSymTab->nsyms > _1M)
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (pSymTab->strsize > _2M)
                    RETURN_VERR_BAD_EXE_FORMAT;

                pThis->offStrTab = pSymTab->stroff;
                pThis->cbStrTab  = pSymTab->strsize;
                pThis->offSyms   = pSymTab->symoff;
                pThis->cSyms     = pSymTab->nsyms;
                break;
            }

            /* Validate the segment. */
#if ARCH_BITS == 32
            case LC_SEGMENT_32:
#elif ARCH_BITS == 64
            case LC_SEGMENT_64:
#else
# error ARCH_BITS
#endif
            {
                MY_SEGMENT_COMMAND const *pSeg = (MY_SEGMENT_COMMAND const *)pCmd;
                if (pSeg->cmdsize < sizeof(*pSeg))
                    RETURN_VERR_BAD_EXE_FORMAT;

                if (pSeg->segname[0] == '\0')
                    RETURN_VERR_BAD_EXE_FORMAT;

                if (pSeg->nsects > MACHO_MAX_SECT)
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (pSeg->nsects * sizeof(MY_SECTION) + sizeof(*pSeg) != pSeg->cmdsize)
                    RETURN_VERR_BAD_EXE_FORMAT;

                if (pSeg->flags & ~(SG_HIGHVM | SG_FVMLIB | SG_NORELOC | SG_PROTECTED_VERSION_1))
                    RETURN_VERR_BAD_EXE_FORMAT;

                if (pSeg->vmaddr != 0)
                {
                    if (pSeg->vmaddr + RT_ALIGN_Z(pSeg->vmsize, RT_BIT_32(12)) < pSeg->vmaddr)
                        RETURN_VERR_BAD_EXE_FORMAT;
                }
                else if (pSeg->vmsize)
                    RETURN_VERR_BAD_EXE_FORMAT;

                if (pSeg->maxprot & ~VM_PROT_ALL)
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (pSeg->initprot & ~VM_PROT_ALL)
                    RETURN_VERR_BAD_EXE_FORMAT;

                /* Validate the sections. */
                uint32_t            uAlignment = 0;
                uintptr_t           uAddr      = pSeg->vmaddr;
                MY_SECTION const   *paSects    = (MY_SECTION const *)(pSeg + 1);
                for (uint32_t i = 0; i < pSeg->nsects; i++)
                {
                    if (paSects[i].sectname[0] == '\0')
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (memcmp(paSects[i].segname, pSeg->segname, sizeof(pSeg->segname)))
                        RETURN_VERR_BAD_EXE_FORMAT;

                    switch (paSects[i].flags & SECTION_TYPE)
                    {
                        case S_REGULAR:
                        case S_CSTRING_LITERALS:
                        case S_NON_LAZY_SYMBOL_POINTERS:
                        case S_MOD_INIT_FUNC_POINTERS:
                        case S_MOD_TERM_FUNC_POINTERS:
                        case S_COALESCED:
                            if (  pSeg->filesize != 0
                                ? paSects[i].offset - pSeg->fileoff >= pSeg->filesize
                                : paSects[i].offset - pSeg->fileoff != pSeg->filesize)
                                RETURN_VERR_BAD_EXE_FORMAT;
                            if (   paSects[i].addr != 0
                                && paSects[i].offset - pSeg->fileoff != paSects[i].addr - pSeg->vmaddr)
                                RETURN_VERR_BAD_EXE_FORMAT;
                            break;

                        case S_ZEROFILL:
                            if (paSects[i].offset != 0)
                                RETURN_VERR_BAD_EXE_FORMAT;
                            break;

                        /* not observed */
                        case S_SYMBOL_STUBS:
                        case S_INTERPOSING:
                        case S_4BYTE_LITERALS:
                        case S_8BYTE_LITERALS:
                        case S_16BYTE_LITERALS:
                        case S_DTRACE_DOF:
                        case S_LAZY_SYMBOL_POINTERS:
                        case S_LAZY_DYLIB_SYMBOL_POINTERS:
                            RETURN_VERR_LDR_UNEXPECTED;
                        case S_GB_ZEROFILL:
                            RETURN_VERR_LDR_UNEXPECTED;
                        default:
                            RETURN_VERR_BAD_EXE_FORMAT;
                    }

                    if (paSects[i].align > 12)
                        RETURN_VERR_BAD_EXE_FORMAT;
                    if (paSects[i].align > uAlignment)
                        uAlignment = paSects[i].align;

                    /* Add to the section table. */
                    if (pThis->cSections == MACHO_MAX_SECT)
                        RETURN_VERR_BAD_EXE_FORMAT;
                    pThis->apSections[pThis->cSections++] = &paSects[i];
                }

                if (RT_ALIGN_Z(pSeg->vmaddr, RT_BIT_32(uAlignment)) != pSeg->vmaddr)
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (   pSeg->filesize > RT_ALIGN_Z(pSeg->vmsize, RT_BIT_32(uAlignment))
                    && pSeg->vmsize != 0)
                    RETURN_VERR_BAD_EXE_FORMAT;
                break;
            }

            case LC_UUID:
                if (pCmd->cmdsize != sizeof(uuid_command))
                    RETURN_VERR_BAD_EXE_FORMAT;
                break;

            case LC_DYSYMTAB:
            case LC_UNIXTHREAD:
                break;

            /* not observed */
            case LC_SYMSEG:
#if ARCH_BITS == 32
            case LC_SEGMENT_64:
#elif ARCH_BITS == 64
            case LC_SEGMENT_32:
#endif
            case LC_ROUTINES_64:
            case LC_ROUTINES:
            case LC_THREAD:
            case LC_LOADFVMLIB:
            case LC_IDFVMLIB:
            case LC_IDENT:
            case LC_FVMFILE:
            case LC_PREPAGE:
            case LC_TWOLEVEL_HINTS:
            case LC_PREBIND_CKSUM:
                RETURN_VERR_LDR_UNEXPECTED;

            /* dylib */
            case LC_LOAD_DYLIB:
            case LC_ID_DYLIB:
            case LC_LOAD_DYLINKER:
            case LC_ID_DYLINKER:
            case LC_PREBOUND_DYLIB:
            case LC_LOAD_WEAK_DYLIB & ~LC_REQ_DYLD:
            case LC_SUB_FRAMEWORK:
            case LC_SUB_UMBRELLA:
            case LC_SUB_CLIENT:
            case LC_SUB_LIBRARY:
                RETURN_VERR_LDR_UNEXPECTED;

            default:
                RETURN_VERR_BAD_EXE_FORMAT;
        }

        /* next */
        pCmd = (load_command_t *)((uintptr_t)pCmd + pCmd->cmdsize);
    }

    return VINF_SUCCESS;
}


/**
 * Loads the FAT and MACHO headers, noting down the relevant info.
 *
 * @returns IPRT status code.
 * @param   pThis               The internal scratch data.
 */
static int rtR0MachKernelLoadFileHeaders(RTR0MACHKERNELINT *pThis)
{
    uint32_t i;

    pThis->offArch = 0;
    pThis->cbArch  = 0;

    /*
     * Read the first bit of the file, parse the FAT if found there.
     */
    int rc = RTFileReadAt(pThis->hFile, 0, pThis->abBuf, sizeof(fat_header_t) + sizeof(fat_arch_t) * 16, NULL);
    if (RT_FAILURE(rc))
        return rc;

    fat_header_t   *pFat        = (fat_header *)pThis->abBuf;
    fat_arch_t     *paFatArches = (fat_arch_t *)(pFat + 1);

    /* Correct FAT endian first. */
    if (pFat->magic == IMAGE_FAT_SIGNATURE_OE)
    {
        pFat->magic     = RT_BSWAP_U32(pFat->magic);
        pFat->nfat_arch = RT_BSWAP_U32(pFat->nfat_arch);
        i = RT_MIN(pFat->nfat_arch, 16);
        while (i-- > 0)
        {
            paFatArches[i].cputype    = RT_BSWAP_U32(paFatArches[i].cputype);
            paFatArches[i].cpusubtype = RT_BSWAP_U32(paFatArches[i].cpusubtype);
            paFatArches[i].offset     = RT_BSWAP_U32(paFatArches[i].offset);
            paFatArches[i].size       = RT_BSWAP_U32(paFatArches[i].size);
            paFatArches[i].align      = RT_BSWAP_U32(paFatArches[i].align);
        }
    }

    /* Lookup our architecture in the FAT. */
    if (pFat->magic == IMAGE_FAT_SIGNATURE)
    {
        if (pFat->nfat_arch > 16)
            RETURN_VERR_BAD_EXE_FORMAT;

        for (i = 0; i < pFat->nfat_arch; i++)
        {
            if (   paFatArches[i].cputype == MY_CPU_TYPE
                && paFatArches[i].cpusubtype == MY_CPU_SUBTYPE_ALL)
            {
                pThis->offArch = paFatArches[i].offset;
                pThis->cbArch  = paFatArches[i].size;
                if (!pThis->cbArch)
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (pThis->offArch < sizeof(fat_header_t) + sizeof(fat_arch_t) * pFat->nfat_arch)
                    RETURN_VERR_BAD_EXE_FORMAT;
                if (pThis->offArch + pThis->cbArch <= pThis->offArch)
                    RETURN_VERR_LDR_ARCH_MISMATCH;
                break;
            }
        }
        if (i >= pFat->nfat_arch)
            RETURN_VERR_LDR_ARCH_MISMATCH;
    }

    /*
     * Read the Mach-O header and validate it.
     */
    rc = RTFileReadAt(pThis->hFile, pThis->offArch, pThis->abBuf, sizeof(MY_MACHO_HEADER), NULL);
    if (RT_FAILURE(rc))
        return rc;
    MY_MACHO_HEADER const *pHdr = (MY_MACHO_HEADER const *)pThis->abBuf;
    if (pHdr->magic != MY_MACHO_MAGIC)
    {
        if (   pHdr->magic == IMAGE_MACHO32_SIGNATURE
            || pHdr->magic == IMAGE_MACHO32_SIGNATURE_OE
            || pHdr->magic == IMAGE_MACHO64_SIGNATURE
            || pHdr->magic == IMAGE_MACHO64_SIGNATURE_OE)
            RETURN_VERR_LDR_ARCH_MISMATCH;
        RETURN_VERR_BAD_EXE_FORMAT;
    }

    if (pHdr->cputype    != MY_CPU_TYPE)
        RETURN_VERR_LDR_ARCH_MISMATCH;
    if (pHdr->cpusubtype != MY_CPU_SUBTYPE_ALL)
        RETURN_VERR_LDR_ARCH_MISMATCH;
    if (pHdr->filetype   != MH_EXECUTE)
        RETURN_VERR_LDR_UNEXPECTED;
    if (pHdr->ncmds      < 4)
        RETURN_VERR_LDR_UNEXPECTED;
    if (pHdr->ncmds      > 256)
        RETURN_VERR_LDR_UNEXPECTED;
    if (pHdr->sizeofcmds <= pHdr->ncmds * sizeof(load_command_t))
        RETURN_VERR_LDR_UNEXPECTED;
    if (pHdr->sizeofcmds >= _1M)
        RETURN_VERR_LDR_UNEXPECTED;
    if (pHdr->flags & ~MH_VALID_FLAGS)
        RETURN_VERR_LDR_UNEXPECTED;

    pThis->cLoadCmds  = pHdr->ncmds;
    pThis->cbLoadCmds = pHdr->sizeofcmds;
    return VINF_SUCCESS;
}


RTDECL(int) RTR0MachKernelOpen(const char *pszMachKernel, PRTR0MACHKERNEL phKernel)
{
    *phKernel = NIL_RTR0MACHKERNEL;

    RTR0MACHKERNELINT *pThis = (RTR0MACHKERNELINT *)RTMemAllocZ(sizeof(*pThis));
    if (!pThis)
        return VERR_NO_MEMORY;
    pThis->hFile = NIL_RTFILE;

    int rc = RTFileOpen(&pThis->hFile, pszMachKernel, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
    if (RT_SUCCESS(rc))
        rc = rtR0MachKernelLoadFileHeaders(pThis);
    if (RT_SUCCESS(rc))
        rc = rtR0MachKernelLoadCommands(pThis);
    if (RT_SUCCESS(rc))
        rc = rtR0MachKernelLoadSymTab(pThis);
    if (RT_SUCCESS(rc))
        rc = rtR0MachKernelCheckStandardSymbols(pThis);

    rtR0MachKernelLoadDone(pThis);
    if (RT_SUCCESS(rc))
        *phKernel = pThis;
    else
        RTR0MachKernelClose(pThis);
    return rc;
}


RTR0DECL(int) RTR0MachKernelGetSymbol(RTR0MACHKERNEL hKernel, const char *pszSymbol, void **ppvValue)
{
    RTR0MACHKERNELINT *pThis = hKernel;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);

    uintptr_t uValue = rtR0MachKernelLookup(pThis, pszSymbol);
    if (ppvValue)
        *ppvValue = (void *)uValue;
    if (!uValue)
        return VERR_SYMBOL_NOT_FOUND;
    return VINF_SUCCESS;
}


RTR0DECL(int) RTR0MachKernelClose(RTR0MACHKERNEL hKernel)
{
    if (hKernel == NIL_RTR0MACHKERNEL)
        return VINF_SUCCESS;

    RTR0MACHKERNELINT *pThis = hKernel;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);

    RTMemFree(pThis->pachStrTab);
    pThis->pachStrTab = NULL;

    RTMemFree(pThis->paSyms);
    pThis->paSyms = NULL;

    RTMemFree(pThis);
    return VINF_SUCCESS;
}


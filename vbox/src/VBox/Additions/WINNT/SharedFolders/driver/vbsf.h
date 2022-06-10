/* $Id: vbsf.h 78328 2019-04-26 15:34:02Z vboxsync $ */
/** @file
 * VirtualBox Windows Guest Shared Folders - File System Driver header file
 */

/*
 * Copyright (C) 2012-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef GA_INCLUDED_SRC_WINNT_SharedFolders_driver_vbsf_h
#define GA_INCLUDED_SRC_WINNT_SharedFolders_driver_vbsf_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif


/*
 * This must be defined before including RX headers.
 */
#define MINIRDR__NAME               VBoxMRx
#define ___MINIRDR_IMPORTS_NAME     (VBoxMRxDeviceObject->RdbssExports)

/*
 * System and RX headers.
 */
#include <iprt/nt/nt.h> /* includes ntifs.h + wdm.h */
#include <iprt/win/windef.h>
#ifndef INVALID_HANDLE_VALUE
# define INVALID_HANDLE_VALUE RTNT_INVALID_HANDLE_VALUE /* (The rx.h definition causes warnings for amd64)  */
#endif
#include <iprt/nt/rx.h>

/*
 * VBox shared folders.
 */
#include "vbsfshared.h"
#include <VBox/log.h>
#include <VBox/VBoxGuestLibSharedFolders.h>
#ifndef __cplusplus /* not for Win2kWorkarounds.c */
# include <VBox/VBoxGuestLibSharedFoldersInline.h>
#endif


RT_C_DECLS_BEGIN

/*
 * Global data.
 */
extern PRDBSS_DEVICE_OBJECT VBoxMRxDeviceObject;

/**
 * Maximum drive letters (A - Z).
 */
#define _MRX_MAX_DRIVE_LETTERS 26

/**
 * The shared folders device extension.
 */
typedef struct _MRX_VBOX_DEVICE_EXTENSION
{
    /** The shared folders device object pointer. */
    PRDBSS_DEVICE_OBJECT pDeviceObject;

    /**
     * Keep a list of local connections used.
     * The size (_MRX_MAX_DRIVE_LETTERS = 26) of the array presents the available drive letters C: - Z: of Windows.
     */
    CHAR cLocalConnections[_MRX_MAX_DRIVE_LETTERS];
    PUNICODE_STRING wszLocalConnectionName[_MRX_MAX_DRIVE_LETTERS];
    FAST_MUTEX mtxLocalCon;

    /** Saved pointer to the original IRP_MJ_DEVICE_CONTROL handler. */
    NTSTATUS (* pfnRDBSSDeviceControl) (PDEVICE_OBJECT pDevObj, PIRP pIrp);
    /** Saved pointer to the original IRP_MJ_CREATE handler. */
    NTSTATUS (NTAPI * pfnRDBSSCreate)(PDEVICE_OBJECT pDevObj, PIRP pIrp);

} MRX_VBOX_DEVICE_EXTENSION, *PMRX_VBOX_DEVICE_EXTENSION;

/**
 * The shared folders NET_ROOT extension.
 */
typedef struct _MRX_VBOX_NETROOT_EXTENSION
{
    /** The shared folder map handle of this netroot. */
    VBGLSFMAP map;
    /** Simple initialized (mapped folder) indicator that works better with the
     *  zero filled defaults than SHFL_ROOT_NIL.  */
    bool        fInitialized;
} MRX_VBOX_NETROOT_EXTENSION, *PMRX_VBOX_NETROOT_EXTENSION;

#define VBOX_FOBX_F_INFO_CREATION_TIME   0x01
#define VBOX_FOBX_F_INFO_LASTACCESS_TIME 0x02
#define VBOX_FOBX_F_INFO_LASTWRITE_TIME  0x04
#define VBOX_FOBX_F_INFO_CHANGE_TIME     0x08
#define VBOX_FOBX_F_INFO_ATTRIBUTES      0x10

/**
 * The shared folders file extension.
 */
typedef struct MRX_VBOX_FOBX
{
    /** The host file handle. */
    SHFLHANDLE                  hFile;
    PMRX_SRV_CALL               pSrvCall;
    /** The RTTimeSystemNanoTS value when Info was retrieved, 0 to force update. */
    uint64_t                    nsUpToDate;
    /** Cached object info. */
    SHFLFSOBJINFO               Info;
    /** NT version of Info.
     * @todo try eliminate  */
    FILE_BASIC_INFORMATION      FileBasicInfo;

    BOOLEAN fKeepCreationTime;
    BOOLEAN fKeepLastAccessTime;
    BOOLEAN fKeepLastWriteTime;
    BOOLEAN fKeepChangeTime;
    BYTE SetFileInfoOnCloseFlags;
} MRX_VBOX_FOBX, *PMRX_VBOX_FOBX;

#define VBoxMRxGetDeviceExtension(RxContext) \
        (PMRX_VBOX_DEVICE_EXTENSION)((PBYTE)(RxContext->RxDeviceObject) + sizeof(RDBSS_DEVICE_OBJECT))

#define VBoxMRxGetNetRootExtension(pNetRoot) \
        (((pNetRoot) == NULL) ? NULL : (PMRX_VBOX_NETROOT_EXTENSION)((pNetRoot)->Context))

#define VBoxMRxGetSrvOpenExtension(pSrvOpen)  \
        (((pSrvOpen) == NULL) ? NULL : (PMRX_VBOX_SRV_OPEN)((pSrvOpen)->Context))

#define VBoxMRxGetFileObjectExtension(pFobx)  \
        (((pFobx) == NULL) ? NULL : (PMRX_VBOX_FOBX)((pFobx)->Context))

/** HACK ALERT: Special Create.ShareAccess indicating trailing slash for
 * non-directory IRP_MJ_CREATE request.
 * Set by VBoxHookMjCreate, used by VBoxMRxCreate. */
#define VBOX_MJ_CREATE_SLASH_HACK   UINT16_C(0x0400)

/** @name Prototypes for the dispatch table routines.
 * @{
 */
NTSTATUS VBoxMRxStart(IN OUT struct _RX_CONTEXT * RxContext,
                      IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject);
NTSTATUS VBoxMRxStop(IN OUT struct _RX_CONTEXT * RxContext,
                     IN OUT PRDBSS_DEVICE_OBJECT RxDeviceObject);

NTSTATUS VBoxMRxCreate(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxCollapseOpen(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxShouldTryToCollapseThisOpen(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxFlush(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxTruncate(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxCleanupFobx(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxCloseSrvOpen(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxDeallocateForFcb(IN OUT PMRX_FCB pFcb);
NTSTATUS VBoxMRxDeallocateForFobx(IN OUT PMRX_FOBX pFobx);
NTSTATUS VBoxMRxForceClosed(IN OUT PMRX_SRV_OPEN SrvOpen);

NTSTATUS VBoxMRxQueryDirectory(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxQueryFileInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetFileInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetFileInfoAtCleanup(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxQueryEaInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetEaInfo(IN OUT struct _RX_CONTEXT * RxContext);
NTSTATUS VBoxMRxQuerySdInfo(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxSetSdInfo(IN OUT struct _RX_CONTEXT * RxContext);
NTSTATUS VBoxMRxQueryVolumeInfo(IN OUT PRX_CONTEXT RxContext);

NTSTATUS VBoxMRxComputeNewBufferingState(IN OUT PMRX_SRV_OPEN pSrvOpen,
                                         IN PVOID pMRxContext,
                                         OUT ULONG *pNewBufferingState);

NTSTATUS VBoxMRxRead(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxWrite(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxLocks(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxFsCtl(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxIoCtl(IN OUT PRX_CONTEXT RxContext);
NTSTATUS VBoxMRxNotifyChangeDirectory(IN OUT PRX_CONTEXT RxContext);

ULONG NTAPI VBoxMRxExtendStub(IN OUT struct _RX_CONTEXT * RxContext,
                              IN OUT PLARGE_INTEGER pNewFileSize,
                              OUT PLARGE_INTEGER pNewAllocationSize);
NTSTATUS VBoxMRxCompleteBufferingStateChangeRequest(IN OUT PRX_CONTEXT RxContext,
                                                    IN OUT PMRX_SRV_OPEN SrvOpen,
                                                    IN PVOID pContext);

NTSTATUS VBoxMRxCreateVNetRoot(IN OUT PMRX_CREATENETROOT_CONTEXT pContext);
NTSTATUS VBoxMRxFinalizeVNetRoot(IN OUT PMRX_V_NET_ROOT pVirtualNetRoot,
                                 IN PBOOLEAN ForceDisconnect);
NTSTATUS VBoxMRxFinalizeNetRoot(IN OUT PMRX_NET_ROOT pNetRoot,
                                IN PBOOLEAN ForceDisconnect);
NTSTATUS VBoxMRxUpdateNetRootState(IN PMRX_NET_ROOT pNetRoot);
VOID     VBoxMRxExtractNetRootName(IN PUNICODE_STRING FilePathName,
                                   IN PMRX_SRV_CALL SrvCall,
                                   OUT PUNICODE_STRING NetRootName,
                                   OUT PUNICODE_STRING RestOfName OPTIONAL);

NTSTATUS VBoxMRxCreateSrvCall(PMRX_SRV_CALL pSrvCall,
                              PMRX_SRVCALL_CALLBACK_CONTEXT pCallbackContext);
NTSTATUS VBoxMRxSrvCallWinnerNotify(IN OUT PMRX_SRV_CALL pSrvCall,
                                    IN BOOLEAN ThisMinirdrIsTheWinner,
                                    IN OUT PVOID pSrvCallContext);
NTSTATUS VBoxMRxFinalizeSrvCall(PMRX_SRV_CALL pSrvCall,
                                BOOLEAN Force);

NTSTATUS VBoxMRxDevFcbXXXControlFile(IN OUT PRX_CONTEXT RxContext);
/** @} */

/** @name Support functions and helpers
 * @{
 */
NTSTATUS vbsfNtDeleteConnection(IN PRX_CONTEXT RxContext,
                                OUT PBOOLEAN PostToFsp);
NTSTATUS vbsfNtCreateConnection(IN PRX_CONTEXT RxContext,
                                OUT PBOOLEAN PostToFsp);

NTSTATUS vbsfNtSetEndOfFile(IN OUT struct _RX_CONTEXT * RxContext,
                            IN OUT PLARGE_INTEGER pNewFileSize,
                            OUT PLARGE_INTEGER pNewAllocationSize);
NTSTATUS vbsfNtRename(IN PRX_CONTEXT RxContext,
                      IN FILE_INFORMATION_CLASS FileInformationClass,
                      IN PVOID pBuffer,
                      IN ULONG BufferLength);
NTSTATUS vbsfNtRemove(IN PRX_CONTEXT RxContext);

void     vbsfNtCopyInfoToLegacy(PMRX_VBOX_FOBX pVBoxFobx, PCSHFLFSOBJINFO pInfo);

NTSTATUS vbsfNtVBoxStatusToNt(int vrc);
PVOID    vbsfNtAllocNonPagedMem(ULONG ulSize);
void     vbsfNtFreeNonPagedMem(PVOID lpMem);
NTSTATUS vbsfNtShflStringFromUnicodeAlloc(PSHFLSTRING *ppShflString, const WCHAR *pwc, uint16_t cb);
#if defined(DEBUG) || defined(LOG_ENABLED)
const char *vbsfNtMajorFunctionName(UCHAR MajorFunction, LONG MinorFunction);
#endif

/**
 * Converts VBox (IPRT) file mode to NT file attributes.
 *
 * @returns NT file attributes
 * @param   fIprtMode   IPRT file mode.
 *
 */
DECLINLINE(uint32_t) VBoxToNTFileAttributes(uint32_t fIprtMode)
{
    AssertCompile((RTFS_DOS_READONLY               >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_READONLY);
    AssertCompile((RTFS_DOS_HIDDEN                 >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_HIDDEN);
    AssertCompile((RTFS_DOS_SYSTEM                 >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_SYSTEM);
    AssertCompile((RTFS_DOS_DIRECTORY              >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_DIRECTORY);
    AssertCompile((RTFS_DOS_ARCHIVED               >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_ARCHIVE);
    AssertCompile((RTFS_DOS_NT_DEVICE              >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_DEVICE);
    AssertCompile((RTFS_DOS_NT_NORMAL              >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_NORMAL);
    AssertCompile((RTFS_DOS_NT_TEMPORARY           >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_TEMPORARY);
    AssertCompile((RTFS_DOS_NT_SPARSE_FILE         >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_SPARSE_FILE);
    AssertCompile((RTFS_DOS_NT_REPARSE_POINT       >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_REPARSE_POINT);
    AssertCompile((RTFS_DOS_NT_COMPRESSED          >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_COMPRESSED);
    AssertCompile((RTFS_DOS_NT_OFFLINE             >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_OFFLINE);
    AssertCompile((RTFS_DOS_NT_NOT_CONTENT_INDEXED >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    AssertCompile((RTFS_DOS_NT_ENCRYPTED           >> RTFS_DOS_SHIFT) == FILE_ATTRIBUTE_ENCRYPTED);

    uint32_t fNtAttribs = (fIprtMode & (RTFS_DOS_MASK_NT & ~(RTFS_DOS_NT_OFFLINE | RTFS_DOS_NT_DEVICE | RTFS_DOS_NT_REPARSE_POINT)))
                       >> RTFS_DOS_SHIFT;
    return fNtAttribs ? fNtAttribs : FILE_ATTRIBUTE_NORMAL;
}

/**
 * Converts NT file attributes to VBox (IPRT) ones.
 *
 * @returns IPRT file mode
 * @param   fNtAttribs      NT file attributes
 */
DECLINLINE(uint32_t) NTToVBoxFileAttributes(uint32_t fNtAttribs)
{
    uint32_t fIprtMode = (fNtAttribs << RTFS_DOS_SHIFT) & RTFS_DOS_MASK_NT;
    fIprtMode &= ~(RTFS_DOS_NT_OFFLINE | RTFS_DOS_NT_DEVICE | RTFS_DOS_NT_REPARSE_POINT);
    return fIprtMode ? fIprtMode : RTFS_DOS_NT_NORMAL;
}

/** @} */

RT_C_DECLS_END

#endif /* !GA_INCLUDED_SRC_WINNT_SharedFolders_driver_vbsf_h */

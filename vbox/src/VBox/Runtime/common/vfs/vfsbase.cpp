/* $Id: vfsbase.cpp 33975 2010-11-11 11:14:54Z vboxsync $ */
/** @file
 * IPRT - Virtual File System, Base.
 */

/*
 * Copyright (C) 2010 Oracle Corporation
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
#include <iprt/vfs.h>
#include <iprt/vfslowlevel.h>

#include <iprt/asm.h>
#include <iprt/err.h>
#include <iprt/file.h>
#include <iprt/mem.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/semaphore.h>

#include "internal/file.h"
#include "internal/magics.h"
//#include "internal/vfs.h"


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define RTVFSOBJ_MAGIC              UINT32_C(0x20109901)
#define RTVFSOBJ_MAGIC_DEAD         (~RTVFSOBJ_MAGIC)
#define RTVFS_MAGIC                 UINT32_C(0x20109902)
#define RTVFS_MAGIC_DEAD            (~RTVFS_MAGIC)
#define RTVFSFSSTREAM_MAGIC         UINT32_C(0x20109903)
#define RTVFSFSSTREAM_MAGIC_DEAD    (~RTVFSFSSTREAM_MAGIC)
#define RTVFSDIR_MAGIC              UINT32_C(0x20109904)
#define RTVFSDIR_MAGIC_DEAD         (~RTVFSDIR_MAGIC)
#define RTVFSFILE_MAGIC             UINT32_C(0x20109905)
#define RTVFSFILE_MAGIC_DEAD        (~RTVFSFILE_MAGIC)
#define RTVFSIOSTREAM_MAGIC         UINT32_C(0x20109906)
#define RTVFSIOSTREAM_MAGIC_DEAD    (~RTVFSIOSTREAM_MAGIC)
#define RTVFSSYMLINK_MAGIC          UINT32_C(0x20109907)
#define RTVFSSYMLINK_MAGIC_DEAD     (~RTVFSSYMLINK_MAGIC)

/** The instance data alignment. */
#define RTVFS_INST_ALIGNMENT        16U

/** The max number of symbolic links to resolve in a path. */
#define RTVFS_MAX_LINKS             20U



/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/** @todo Move all this stuff to internal/vfs.h */

/**
 * The VFS base object handle data.
 *
 * All other VFS handles are derived from this one.  The final handle type is
 * indicated by RTVFSOBJOPS::enmType via the RTVFSOBJINTERNAL::pOps member.
 */
typedef struct RTVFSOBJINTERNAL
{
    /** The VFS magic (RTVFSOBJ_MAGIC). */
    uint32_t                uMagic;
    /** The number of references to this VFS object. */
    uint32_t volatile       cRefs;
    /** Pointer to the instance data. */
    void                   *pvThis;
    /** The vtable. */
    PCRTVFSOBJOPS           pOps;
    /** Read-write semaphore protecting all access to the VFS
     * Only valid RTVFS_C_THREAD_SAFE is set, otherwise it is NIL_RTSEMRW. */
    RTSEMRW                 hSemRW;
    /** Reference back to the VFS containing this object. */
    RTVFS                   hVfs;
} RTVFSOBJINTERNAL;


/**
 * The VFS filesystem stream handle data.
 *
 * @extends RTVFSOBJINTERNAL
 */
typedef struct RTVFSFSSTREAMINTERNAL
{
    /** The VFS magic (RTVFSFSTREAM_MAGIC). */
    uint32_t                uMagic;
    /** File open flags, at a minimum the access mask. */
    uint32_t                fFlags;
    /** The vtable. */
    PCRTVFSFSSTREAMOPS      pOps;
    /** The base object handle data. */
    RTVFSOBJINTERNAL        Base;
} RTVFSFSSTREAMINTERNAL;


/**
 * The VFS handle data.
 *
 * @extends RTVFSOBJINTERNAL
 */
typedef struct RTVFSINTERNAL
{
    /** The VFS magic (RTVFS_MAGIC). */
    uint32_t                uMagic;
    /** Creation flags (RTVFS_C_XXX). */
    uint32_t                fFlags;
    /** The vtable. */
    PCRTVFSOPS              pOps;
    /** The base object handle data. */
    RTVFSOBJINTERNAL        Base;
} RTVFSINTERNAL;


/**
 * The VFS directory handle data.
 *
 * @extends RTVFSOBJINTERNAL
 */
typedef struct RTVFSDIRINTERNAL
{
    /** The VFS magic (RTVFSDIR_MAGIC). */
    uint32_t                uMagic;
    /** Reserved for flags or something. */
    uint32_t                fReserved;
    /** The vtable. */
    PCRTVFSDIROPS           pOps;
    /** The base object handle data. */
    RTVFSOBJINTERNAL        Base;
} RTVFSDIRINTERNAL;


/**
 * The VFS symbolic link handle data.
 *
 * @extends RTVFSOBJINTERNAL
 */
typedef struct RTVFSSYMLINKINTERNAL
{
    /** The VFS magic (RTVFSSYMLINK_MAGIC). */
    uint32_t                uMagic;
    /** Reserved for flags or something. */
    uint32_t                fReserved;
    /** The vtable. */
    PCRTVFSSYMLINKOPS       pOps;
    /** The base object handle data. */
    RTVFSOBJINTERNAL        Base;
} RTVFSSYMLINKINTERNAL;


/**
 * The VFS I/O stream handle data.
 *
 * This is often part of a type specific handle, like a file or pipe.
 *
 * @extends RTVFSOBJINTERNAL
 */
typedef struct RTVFSIOSTREAMINTERNAL
{
    /** The VFS magic (RTVFSIOSTREAM_MAGIC). */
    uint32_t                uMagic;
    /** File open flags, at a minimum the access mask. */
    uint32_t                fFlags;
    /** The vtable. */
    PCRTVFSIOSTREAMOPS      pOps;
    /** The base object handle data. */
    RTVFSOBJINTERNAL        Base;
} RTVFSIOSTREAMINTERNAL;


/**
 * The VFS file handle data.
 *
 * @extends RTVFSIOSTREAMINTERNAL
 */
typedef struct RTVFSFILEINTERNAL
{
    /** The VFS magic (RTVFSFILE_MAGIC). */
    uint32_t                uMagic;
    /** Reserved for flags or something. */
    uint32_t                fReserved;
    /** The vtable. */
    PCRTVFSFILEOPS          pOps;
    /** The stream handle data. */
    RTVFSIOSTREAMINTERNAL   Stream;
} RTVFSFILEINTERNAL;

#if 0 /* later */

/**
 * The VFS pipe handle data.
 *
 * @extends RTVFSIOSTREAMINTERNAL
 */
typedef struct RTVFSPIPEINTERNAL
{
    /** The VFS magic (RTVFSPIPE_MAGIC). */
    uint32_t                uMagic;
    /** Reserved for flags or something. */
    uint32_t                fReserved;
    /** The vtable. */
    PCRTVFSPIPEOPS          pOps;
    /** The stream handle data. */
    RTVFSIOSTREAMINTERNAL   Stream;
} RTVFSPIPEINTERNAL;


/**
 * The VFS socket handle data.
 *
 * @extends RTVFSIOSTREAMINTERNAL
 */
typedef struct RTVFSSOCKETINTERNAL
{
    /** The VFS magic (RTVFSSOCKET_MAGIC). */
    uint32_t                uMagic;
    /** Reserved for flags or something. */
    uint32_t                fReserved;
    /** The vtable. */
    PCRTVFSSOCKETOPS        pOps;
    /** The stream handle data. */
    RTVFSIOSTREAMINTERNAL   Stream;
} RTVFSSOCKETINTERNAL;

#endif /* later */


/*
 *
 *  B A S E   O B J E C T
 *  B A S E   O B J E C T
 *  B A S E   O B J E C T
 *
 */

/**
 * Write locks the object.
 *
 * @param   pThis               The object to lock.
 */
DECLINLINE(void) rtVfsObjWriteLock(RTVFSOBJINTERNAL *pThis)
{
    if (pThis->hSemRW != NIL_RTSEMRW)
    {
        int rc = RTSemRWRequestWrite(pThis->hSemRW, RT_INDEFINITE_WAIT);
        AssertRC(rc);
    }
}


/**
 * Undoing the effects of rtVfsObjWriteLock.
 *
 * @param   pThis               The object to lock.
 */
DECLINLINE(void) rtVfsObjWriteUnlock(RTVFSOBJINTERNAL *pThis)
{
    if (pThis->hSemRW != NIL_RTSEMRW)
    {
        int rc = RTSemRWReleaseWrite(pThis->hSemRW);
        AssertRC(rc);
    }
}

/**
 * Read locks the object.
 *
 * @param   pThis               The object to lock.
 */
DECLINLINE(void) rtVfsObjReadLock(RTVFSOBJINTERNAL *pThis)
{
    if (pThis->hSemRW != NIL_RTSEMRW)
    {
        int rc = RTSemRWRequestRead(pThis->hSemRW, RT_INDEFINITE_WAIT);
        AssertRC(rc);
    }
}


/**
 * Undoing the effects of rtVfsObjReadLock.
 *
 * @param   pThis               The object to lock.
 */
DECLINLINE(void) rtVfsObjReadUnlock(RTVFSOBJINTERNAL *pThis)
{
    if (pThis->hSemRW != NIL_RTSEMRW)
    {
        int rc = RTSemRWReleaseRead(pThis->hSemRW);
        AssertRC(rc);
    }
}



/**
 * Internal object retainer that asserts sanity in strict builds.
 *
 * @returns The new reference count.
 * @param   pThis               The base object handle data.
 */
DECLINLINE(uint32_t) rtVfsObjRetain(RTVFSOBJINTERNAL *pThis)
{
    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1M,
              ("%#x %p ops=%p %s (%d)\n", cRefs, pThis, pThis->pOps, pThis->pOps->pszName, pThis->pOps->enmType));
    return cRefs;
}


/**
 * Internal object retainer that asserts sanity in strict builds.
 *
 * @param   pThis               The base object handle data.
 */
DECLINLINE(void) rtVfsObjRetainVoid(RTVFSOBJINTERNAL *pThis)
{
    uint32_t cRefs = ASMAtomicIncU32(&pThis->cRefs);
    AssertMsg(cRefs > 1 && cRefs < _1M,
              ("%#x %p ops=%p %s (%d)\n", cRefs, pThis, pThis->pOps, pThis->pOps->pszName, pThis->pOps->enmType));
    NOREF(cRefs);
}


RTDECL(uint32_t) RTVfsObjRetain(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, UINT32_MAX);

    return rtVfsObjRetain(pThis);
}


/**
 * Does the actual object destruction for rtVfsObjRelease().
 *
 * @param   pThis               The object to destroy.
 */
static void rtVfsObjDestroy(RTVFSOBJINTERNAL *pThis)
{
    RTVFSOBJTYPE const enmType = pThis->pOps->enmType;

    /*
     * Invalidate the object.
     */
    rtVfsObjWriteLock(pThis);           /* paranoia */
    void *pvToFree;
    switch (enmType)
    {
        case RTVFSOBJTYPE_BASE:
            pvToFree = pThis;
            break;

        case RTVFSOBJTYPE_VFS:
            pvToFree         = RT_FROM_MEMBER(pThis, RTVFSINTERNAL, Base);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSINTERNAL, Base)->uMagic, RTVFS_MAGIC_DEAD);
            break;

        case RTVFSOBJTYPE_FS_STREAM:
            pvToFree         = RT_FROM_MEMBER(pThis, RTVFSFSSTREAMINTERNAL, Base);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSFSSTREAMINTERNAL, Base)->uMagic, RTVFSFSSTREAM_MAGIC_DEAD);
            break;

        case RTVFSOBJTYPE_IO_STREAM:
            pvToFree         = RT_FROM_MEMBER(pThis, RTVFSIOSTREAMINTERNAL, Base);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSIOSTREAMINTERNAL, Base)->uMagic, RTVFSIOSTREAM_MAGIC_DEAD);
            break;

        case RTVFSOBJTYPE_DIR:
            pvToFree         = RT_FROM_MEMBER(pThis, RTVFSDIRINTERNAL, Base);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSDIRINTERNAL, Base)->uMagic, RTVFSDIR_MAGIC_DEAD);
            break;

        case RTVFSOBJTYPE_FILE:
            pvToFree         = RT_FROM_MEMBER(pThis, RTVFSFILEINTERNAL, Stream.Base);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSFILEINTERNAL, Stream.Base)->uMagic, RTVFSFILE_MAGIC_DEAD);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSIOSTREAMINTERNAL, Base)->uMagic, RTVFSIOSTREAM_MAGIC_DEAD);
            break;

        case RTVFSOBJTYPE_SYMLINK:
            pvToFree         = RT_FROM_MEMBER(pThis, RTVFSSYMLINKINTERNAL, Base);
            ASMAtomicWriteU32(&RT_FROM_MEMBER(pThis, RTVFSSYMLINKINTERNAL, Base)->uMagic, RTVFSSYMLINK_MAGIC_DEAD);
            break;

        case RTVFSOBJTYPE_INVALID:
        case RTVFSOBJTYPE_END:
        case RTVFSOBJTYPE_32BIT_HACK:
            AssertMsgFailed(("enmType=%d ops=%p %s\n", enmType, pThis->pOps, pThis->pOps->pszName));
            break;
        /* no default as we want gcc warnings. */
    }
    ASMAtomicWriteU32(&pThis->uMagic, RTVFSOBJ_MAGIC_DEAD);
    rtVfsObjWriteUnlock(pThis);

    /*
     * Close the object and free the handle.
     */
    int rc = pThis->pOps->pfnClose(pThis->pvThis);
    AssertRC(rc);
    RTMemFree(pvToFree);
}


/**
 * Internal object releaser that asserts sanity in strict builds.
 *
 * @returns The new reference count.
 * @param   pcRefs              The reference counter.
 */
DECLINLINE(uint32_t) rtVfsObjRelease(RTVFSOBJINTERNAL *pThis)
{
    uint32_t cRefs = ASMAtomicDecU32(&pThis->cRefs);
    AssertMsg(cRefs < _1M, ("%#x %p ops=%p %s (%d)\n", cRefs, pThis, pThis->pOps, pThis->pOps->pszName, pThis->pOps->enmType));
    if (cRefs == 0)
        rtVfsObjDestroy(pThis);
    return cRefs;
}


RTDECL(uint32_t) RTVfsObjRelease(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, UINT32_MAX);
    return rtVfsObjRelease(pThis);
}


RTDECL(RTVFS)           RTVfsObjToVfs(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    if (pThis != NIL_RTVFSOBJ)
    {
        AssertPtrReturn(pThis, NIL_RTVFS);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFS);

        if (pThis->pOps->enmType == RTVFSOBJTYPE_VFS)
        {
            rtVfsObjRetainVoid(pThis);
            return RT_FROM_MEMBER(pThis, RTVFSINTERNAL, Base);
        }
    }
    return NIL_RTVFS;
}


RTDECL(RTVFSFSSTREAM)   RTVfsObjToFsStream(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    if (pThis != NIL_RTVFSOBJ)
    {
        AssertPtrReturn(pThis, NIL_RTVFSFSSTREAM);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSFSSTREAM);

        if (pThis->pOps->enmType == RTVFSOBJTYPE_FS_STREAM)
        {
            rtVfsObjRetainVoid(pThis);
            return RT_FROM_MEMBER(pThis, RTVFSFSSTREAMINTERNAL, Base);
        }
    }
    return NIL_RTVFSFSSTREAM;
}


RTDECL(RTVFSDIR)        RTVfsObjToDir(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    if (pThis != NIL_RTVFSOBJ)
    {
        AssertPtrReturn(pThis, NIL_RTVFSDIR);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSDIR);

        if (pThis->pOps->enmType == RTVFSOBJTYPE_VFS)
        {
            rtVfsObjRetainVoid(pThis);
            return RT_FROM_MEMBER(pThis, RTVFSDIRINTERNAL, Base);
        }
    }
    return NIL_RTVFSDIR;
}


RTDECL(RTVFSIOSTREAM)   RTVfsObjToIoStream(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    if (pThis != NIL_RTVFSOBJ)
    {
        AssertPtrReturn(pThis, NIL_RTVFSIOSTREAM);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSIOSTREAM);

        if (pThis->pOps->enmType == RTVFSOBJTYPE_VFS)
        {
            rtVfsObjRetainVoid(pThis);
            return RT_FROM_MEMBER(pThis, RTVFSIOSTREAMINTERNAL, Base);
        }
    }
    return NIL_RTVFSIOSTREAM;
}


RTDECL(RTVFSFILE)       RTVfsObjToFile(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    if (pThis != NIL_RTVFSOBJ)
    {
        AssertPtrReturn(pThis, NIL_RTVFSFILE);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSFILE);

        if (pThis->pOps->enmType == RTVFSOBJTYPE_VFS)
        {
            rtVfsObjRetainVoid(pThis);
            return RT_FROM_MEMBER(pThis, RTVFSFILEINTERNAL, Stream.Base);
        }
    }
    return NIL_RTVFSFILE;
}


RTDECL(RTVFSSYMLINK)    RTVfsObjToSymlink(RTVFSOBJ hVfsObj)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    if (pThis != NIL_RTVFSOBJ)
    {
        AssertPtrReturn(pThis, NIL_RTVFSSYMLINK);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSSYMLINK);

        if (pThis->pOps->enmType == RTVFSOBJTYPE_VFS)
        {
            rtVfsObjRetainVoid(pThis);
            return RT_FROM_MEMBER(pThis, RTVFSSYMLINKINTERNAL, Base);
        }
    }
    return NIL_RTVFSSYMLINK;
}


RTDECL(RTVFSOBJ)        RTVfsObjFromVfs(RTVFS hVfs)
{
    if (hVfs != NIL_RTVFS)
    {
        RTVFSOBJINTERNAL *pThis = &hVfs->Base;
        AssertPtrReturn(pThis, NIL_RTVFSOBJ);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSOBJ);

        rtVfsObjRetainVoid(pThis);
        return pThis;
    }
    return NIL_RTVFSOBJ;
}


RTDECL(RTVFSOBJ)        RTVfsObjFromFsStream(RTVFSFSSTREAM hVfsFss)
{
    if (hVfsFss != NIL_RTVFSFSSTREAM)
    {
        RTVFSOBJINTERNAL *pThis = &hVfsFss->Base;
        AssertPtrReturn(pThis, NIL_RTVFSOBJ);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSOBJ);

        rtVfsObjRetainVoid(pThis);
        return pThis;
    }
    return NIL_RTVFSOBJ;
}


RTDECL(RTVFSOBJ)        RTVfsObjFromDir(RTVFSDIR hVfsDir)
{
    if (hVfsDir != NIL_RTVFSDIR)
    {
        RTVFSOBJINTERNAL *pThis = &hVfsDir->Base;
        AssertPtrReturn(pThis, NIL_RTVFSOBJ);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSOBJ);

        rtVfsObjRetainVoid(pThis);
        return pThis;
    }
    return NIL_RTVFSOBJ;
}


RTDECL(RTVFSOBJ)        RTVfsObjFromIoStream(RTVFSIOSTREAM hVfsIos)
{
    if (hVfsIos != NIL_RTVFSIOSTREAM)
    {
        RTVFSOBJINTERNAL *pThis = &hVfsIos->Base;
        AssertPtrReturn(pThis, NIL_RTVFSOBJ);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSOBJ);

        rtVfsObjRetainVoid(pThis);
        return pThis;
    }
    return NIL_RTVFSOBJ;
}


RTDECL(RTVFSOBJ)        RTVfsObjFromFile(RTVFSFILE hVfsFile)
{
    if (hVfsFile != NIL_RTVFSFILE)
    {
        RTVFSOBJINTERNAL *pThis = &hVfsFile->Stream.Base;
        AssertPtrReturn(pThis, NIL_RTVFSOBJ);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSOBJ);

        rtVfsObjRetainVoid(pThis);
        return pThis;
    }
    return NIL_RTVFSOBJ;
}


RTDECL(RTVFSOBJ)        RTVfsObjFromSymlink(RTVFSSYMLINK hVfsSym)
{
    if (hVfsSym != NIL_RTVFSSYMLINK)
    {
        RTVFSOBJINTERNAL *pThis = &hVfsSym->Base;
        AssertPtrReturn(pThis, NIL_RTVFSOBJ);
        AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, NIL_RTVFSOBJ);

        rtVfsObjRetainVoid(pThis);
        return pThis;
    }
    return NIL_RTVFSOBJ;
}



RTDECL(int)         RTVfsObjQueryInfo(RTVFSOBJ hVfsObj, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    RTVFSOBJINTERNAL *pThis = hVfsObj;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSOBJ_MAGIC, VERR_INVALID_HANDLE);

    rtVfsObjReadLock(pThis);
    int rc = pThis->pOps->pfnQueryInfo(pThis->pvThis, pObjInfo, enmAddAttr);
    rtVfsObjReadUnlock(pThis);
    return rc;
}



/*
 *
 *  U T I L   U T I L   U T I L
 *  U T I L   U T I L   U T I L
 *  U T I L   U T I L   U T I L
 *
 */



/**
 * Removes dots from the path.
 *
 * @returns The new @a pszDst value.
 * @param   pPath               The path parsing buffer.
 * @param   pszDst              The current szPath position.  This will be
 *                              updated and returned.
 * @param   fTheEnd             Indicates whether we're at the end of the path
 *                              or not.
 * @param   piRestartComp       The component to restart parsing at.
 */
static char *rtVfsParsePathHandleDots(PRTVFSPARSEDPATH pPath, char *pszDst, bool fTheEnd, uint16_t *piRestartComp)
{
    if (pszDst[-1] != '.')
        return pszDst;

    if (pszDst[-2] == '/')
    {
        pPath->cComponents--;
        pszDst = &pPath->szPath[pPath->aoffComponents[pPath->cComponents]];
    }
    else if (pszDst[-2] == '.' && pszDst[-3] == '/')
    {
        pPath->cComponents -= pPath->cComponents > 1 ? 2 : 1;
        pszDst = &pPath->szPath[pPath->aoffComponents[pPath->cComponents]];
        if (piRestartComp && *piRestartComp + 1 >= pPath->cComponents)
            *piRestartComp = pPath->cComponents > 0 ? pPath->cComponents - 1 : 0;
    }
    else
        return pszDst;

    /*
     * Drop the trailing slash if we're at the end of the source path.
     */
    if (fTheEnd && pPath->cComponents == 0)
        pszDst--;
    return pszDst;
}


RTDECL(int) RTVfsParsePathAppend(PRTVFSPARSEDPATH pPath, const char *pszPath, uint16_t *piRestartComp)
{
    AssertReturn(*pszPath != '/', VERR_INTERNAL_ERROR_4);

    /* In case *piRestartComp was set higher than the number of components
       before making the call to this function. */
    if (piRestartComp && *piRestartComp + 1 >= pPath->cComponents)
        *piRestartComp = pPath->cComponents > 0 ? pPath->cComponents - 1 : 0;

    /*
     * Append a slash to the destination path if necessary.
     */
    char *pszDst = &pPath->szPath[pPath->cch];
    if (pPath->cComponents > 0)
    {
        *pszDst++ = '/';
        if (pszDst - &pPath->szPath[0] >= RTVFSPARSEDPATH_MAX)
            return VERR_FILENAME_TOO_LONG;
    }
    Assert(pszDst[-1] == '/');

    /*
     * Parse and append the relative path.
     */
    const char *pszSrc = pszPath;
    pPath->fDirSlash   = false;
    while (pszSrc[0])
    {
        /* Skip unncessary slashes. */
        while (pszSrc[0] == '/')
            pszSrc++;

        /* Copy until we encounter the next slash. */
        pPath->aoffComponents[pPath->cComponents++] = pszDst - &pPath->szPath[0];
        while (pszSrc[0])
        {
            if (pszSrc[0] == '/')
            {
                pszSrc++;
                if (pszSrc[0])
                    *pszDst++ = '/';
                else
                    pPath->fDirSlash = true;
                pszDst = rtVfsParsePathHandleDots(pPath, pszDst, pszSrc[0] == '\0', piRestartComp);
                break;
            }

            *pszDst++ = *pszSrc++;
            if (pszDst - &pPath->szPath[0] >= RTVFSPARSEDPATH_MAX)
                return VERR_FILENAME_TOO_LONG;
        }
    }
    pszDst = rtVfsParsePathHandleDots(pPath, pszDst, true /*fTheEnd*/, piRestartComp);

    /* Terminate the string and enter its length. */
    pszDst[0] = '\0';
    pszDst[1] = '\0';                   /* for aoffComponents */
    pPath->cch = (uint16_t)(pszDst - &pPath->szPath[0]);
    pPath->aoffComponents[pPath->cComponents] = pPath->cch + 1;

    return VINF_SUCCESS;
}


RTDECL(int) RTVfsParsePath(PRTVFSPARSEDPATH pPath, const char *pszPath, const char *pszCwd)
{
    if (*pszPath != '/')
    {
        /*
         * Relative, recurse and parse pszCwd first.
         */
        int rc = RTVfsParsePath(pPath, pszCwd, NULL /*crash if pszCwd is not absolute*/);
        if (RT_FAILURE(rc))
            return rc;
    }
    else
    {
        /*
         * Make pszPath relative, i.e. set up pPath for the root and skip
         * leading slashes in pszPath before appending it.
         */
        pPath->cch               = 1;
        pPath->cComponents       = 0;
        pPath->fDirSlash         = false;
        pPath->aoffComponents[0] = 1;
        pPath->aoffComponents[1] = 2;
        pPath->szPath[0]         = '/';
        pPath->szPath[1]         = '\0';
        pPath->szPath[2]         = '\0';
        while (pszPath[0] == '/')
            pszPath++;
        if (!pszPath[0])
            return VINF_SUCCESS;
    }
    return RTVfsParsePathAppend(pPath, pszPath, NULL);
}



RTDECL(int) RTVfsParsePathA(const char *pszPath, const char *pszCwd, PRTVFSPARSEDPATH *ppPath)
{
    /*
     * Allocate the output buffer and hand the problem to rtVfsParsePath.
     */
    int rc;
    PRTVFSPARSEDPATH pPath = (PRTVFSPARSEDPATH)RTMemTmpAlloc(sizeof(RTVFSPARSEDPATH));
    if (pPath)
    {
        rc = RTVfsParsePath(pPath, pszPath, pszCwd);
        if (RT_FAILURE(rc))
        {
            RTMemTmpFree(pPath);
            pPath = NULL;
        }
    }
    else
        rc = VERR_NO_TMP_MEMORY;
    *ppPath = pPath;                    /* always set it */
    return rc;
}


RTDECL(void) RTVfsParsePathFree(PRTVFSPARSEDPATH pPath)
{
    if (pPath)
    {
        pPath->cch               = UINT16_MAX;
        pPath->cComponents       = UINT16_MAX;
        pPath->aoffComponents[0] = UINT16_MAX;
        pPath->aoffComponents[1] = UINT16_MAX;
        RTMemTmpFree(pPath);
    }
}


/**
 * Handles a symbolic link, adding it to
 *
 * @returns IPRT status code.
 * @param   pPath               The parsed path to update.
 * @param   piComponent         The component iterator to update.
 * @param   hSymlink            The symbolic link to process.
 */
static int rtVfsTraverseHandleSymlink(PRTVFSPARSEDPATH pPath, uint16_t *piComponent, RTVFSSYMLINK hSymlink)
{
    /*
     * Read the link.
     */
    char szPath[RTPATH_MAX];
    int rc = RTVfsSymlinkRead(hSymlink, szPath, sizeof(szPath) - 1);
    if (RT_SUCCESS(rc))
    {
        szPath[sizeof(szPath) - 1] = '\0';
        if (szPath[0] == '/')
        {
            /*
             * Absolute symlink.
             */
            rc = RTVfsParsePath(pPath, szPath, NULL);
            if (RT_SUCCESS(rc))
            {
                *piComponent = 0;
                return VINF_SUCCESS;
            }
        }
        else
        {
            /*
             * Relative symlink, must replace the current component with the
             * link value.  We do that by using the remainder of the symlink
             * buffer as temporary storage.
             */
            uint16_t iComponent = *piComponent;
            if (iComponent + 1 < pPath->cComponents)
                rc = RTPathAppend(szPath, sizeof(szPath), &pPath->szPath[pPath->aoffComponents[iComponent + 1]]);
            if (RT_SUCCESS(rc))
            {
                pPath->cch = pPath->aoffComponents[iComponent] - (iComponent > 0);
                pPath->aoffComponents[iComponent + 1] = pPath->cch + 1;
                pPath->szPath[pPath->cch]     = '\0';
                pPath->szPath[pPath->cch + 1] = '\0';

                rc = RTVfsParsePathAppend(pPath, szPath, &iComponent);
                if (RT_SUCCESS(rc))
                {
                    *piComponent = iComponent;
                    return VINF_SUCCESS;
                }
            }
        }
    }
    return rc == VERR_BUFFER_OVERFLOW ? VERR_FILENAME_TOO_LONG : rc;
}


/**
 * Internal worker for various open functions as well as RTVfsTraverseToParent.
 *
 * @returns IPRT status code.
 * @param   pThis           The VFS.
 * @param   pPath           The parsed path.  This may be changed as symbolic
 *                          links are processed during the path traversal.
 * @param   fFollowSymlink  Whether to follow the final component if it is a
 *                          symbolic link.
 * @param   ppVfsParentDir  Where to return the parent directory handle
 *                          (referenced).
 */
static int rtVfsTraverseToParent(RTVFSINTERNAL *pThis, PRTVFSPARSEDPATH pPath, bool fFollowSymlink,
                                 RTVFSDIRINTERNAL **ppVfsParentDir)
{
    /*
     * Assert sanity.
     */
    AssertPtr(pThis);
    Assert(pThis->uMagic == RTVFS_MAGIC);
    Assert(pThis->Base.cRefs > 0);
    AssertPtr(pPath);
    AssertPtr(ppVfsParentDir);
    *ppVfsParentDir = NULL;
    AssertReturn(pPath->cComponents > 0, VERR_INTERNAL_ERROR_3);

    /*
     * Open the root directory.
     */
    /** @todo Union mounts, traversal optimization methods, races, ++ */
    RTVFSDIRINTERNAL *pCurDir;
    rtVfsObjReadLock(&pThis->Base);
    int rc = pThis->pOps->pfnOpenRoot(pThis->Base.pvThis, &pCurDir);
    rtVfsObjReadUnlock(&pThis->Base);
    if (RT_FAILURE(rc))
        return rc;
    Assert(pCurDir->uMagic == RTVFSDIR_MAGIC);

    /*
     * The traversal loop.
     */
    unsigned cLinks     = 0;
    uint16_t iComponent = 0;
    for (;;)
    {
        /*
         * Are we done yet?
         */
        bool fFinal = iComponent + 1 >= pPath->cComponents;
        if (fFinal && !fFollowSymlink)
        {
            *ppVfsParentDir = pCurDir;
            return VINF_SUCCESS;
        }

        /*
         * Try open the next entry.
         */
        const char     *pszEntry    = &pPath->szPath[pPath->aoffComponents[iComponent]];
        char           *pszEntryEnd = &pPath->szPath[pPath->aoffComponents[iComponent + 1] - 1];
        *pszEntryEnd = '\0';
        RTVFSDIR        hDir     = NIL_RTVFSDIR;
        RTVFSSYMLINK    hSymlink = NIL_RTVFSSYMLINK;
        RTVFS           hVfsMnt  = NIL_RTVFS;
        if (fFinal)
        {
            rtVfsObjReadLock(&pCurDir->Base);
            rc = pCurDir->pOps->pfnTraversalOpen(pCurDir->Base.pvThis, pszEntry, NULL, &hSymlink, NULL);
            rtVfsObjReadUnlock(&pCurDir->Base);
            *pszEntryEnd = '\0';
            if (rc == VERR_PATH_NOT_FOUND)
                rc = VINF_SUCCESS;
            if (RT_FAILURE(rc))
                break;

            if (hSymlink == NIL_RTVFSSYMLINK)
            {
                *ppVfsParentDir = pCurDir;
                return VINF_SUCCESS;
            }
        }
        else
        {
            rtVfsObjReadLock(&pCurDir->Base);
            rc = pCurDir->pOps->pfnTraversalOpen(pCurDir->Base.pvThis, pszEntry, &hDir, &hSymlink, &hVfsMnt);
            rtVfsObjReadUnlock(&pCurDir->Base);
            *pszEntryEnd = '/';
            if (RT_FAILURE(rc))
                break;

            if (   hDir     == NIL_RTVFSDIR
                && hSymlink == NIL_RTVFSSYMLINK
                && hVfsMnt  == NIL_RTVFS)
            {
                rc = VERR_NOT_A_DIRECTORY;
                break;
            }
        }
        Assert(   (hDir != NIL_RTVFSDIR && hSymlink == NIL_RTVFSSYMLINK && hVfsMnt == NIL_RTVFS)
               || (hDir == NIL_RTVFSDIR && hSymlink != NIL_RTVFSSYMLINK && hVfsMnt == NIL_RTVFS)
               || (hDir == NIL_RTVFSDIR && hSymlink == NIL_RTVFSSYMLINK && hVfsMnt != NIL_RTVFS));

        if (hDir != NIL_RTVFSDIR)
        {
            /*
             * Directory - advance down the path.
             */
            AssertPtr(hDir);
            Assert(hDir->uMagic == RTVFSDIR_MAGIC);
            RTVfsDirRelease(pCurDir);
            pCurDir = hDir;
            iComponent++;
        }
        else if (hSymlink != NIL_RTVFSSYMLINK)
        {
            /*
             * Symbolic link - deal with it and retry the current component.
             */
            AssertPtr(hSymlink);
            Assert(hSymlink->uMagic == RTVFSSYMLINK_MAGIC);
            cLinks++;
            if (cLinks >= RTVFS_MAX_LINKS)
            {
                rc = VERR_TOO_MANY_SYMLINKS;
                break;
            }
            uint16_t iRestartComp = iComponent;
            rc = rtVfsTraverseHandleSymlink(pPath, &iRestartComp, hSymlink);
            if (RT_FAILURE(rc))
                break;
            if (iRestartComp != iComponent)
            {
                /* Must restart from the root (optimize this). */
                RTVfsDirRelease(pCurDir);
                rtVfsObjReadLock(&pThis->Base);
                rc = pThis->pOps->pfnOpenRoot(pThis->Base.pvThis, &pCurDir);
                rtVfsObjReadUnlock(&pThis->Base);
                if (RT_FAILURE(rc))
                {
                    pCurDir = NULL;
                    break;
                }
                iComponent = 0;
            }
        }
        else
        {
            /*
             * Mount point - deal with it and retry the current component.
             */
            RTVfsDirRelease(pCurDir);
            rtVfsObjReadLock(&hVfsMnt->Base);
            rc = pThis->pOps->pfnOpenRoot(hVfsMnt->Base.pvThis, &pCurDir);
            rtVfsObjReadUnlock(&hVfsMnt->Base);
            if (RT_FAILURE(rc))
            {
                pCurDir = NULL;
                break;
            }
            iComponent = 0;
            /** @todo union mounts. */
        }
    }

    if (pCurDir)
        RTVfsDirRelease(pCurDir);

    return rc;
}





/*
 *
 *  F I L E S Y S T E M   S T R E A M
 *  F I L E S Y S T E M   S T R E A M
 *  F I L E S Y S T E M   S T R E A M
 *
 */


RTDECL(int) RTVfsNewFsStream(PCRTVFSFSSTREAMOPS pFsStreamOps, size_t cbInstance, RTVFS hVfs, RTSEMRW hSemRW,
                             PRTVFSFSSTREAM phVfsFss, void **ppvInstance)
{
    return VERR_NOT_IMPLEMENTED;
}


RTDECL(uint32_t)    RTVfsFsStrmRetain(RTVFSFSSTREAM hVfsFss)
{
    RTVFSFSSTREAMINTERNAL *pThis = hVfsFss;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSFSSTREAM_MAGIC, UINT32_MAX);
    return rtVfsObjRetain(&pThis->Base);
}


RTDECL(uint32_t)    RTVfsFsStrmRelease(RTVFSFSSTREAM hVfsFss)
{
    RTVFSFSSTREAMINTERNAL *pThis = hVfsFss;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSFSSTREAM_MAGIC, UINT32_MAX);
    return rtVfsObjRelease(&pThis->Base);
}


RTDECL(int)         RTVfsFsStrmQueryInfo(RTVFSFSSTREAM hVfsFss, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    RTVFSFSSTREAMINTERNAL *pThis = hVfsFss;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSFSSTREAM_MAGIC, VERR_INVALID_HANDLE);
    return RTVfsObjQueryInfo(&pThis->Base, pObjInfo, enmAddAttr);
}


RTDECL(int)         RTVfsFsStrmNext(RTVFSFSSTREAM hVfsFss, char **ppszName, RTVFSOBJTYPE *penmType, PRTVFSOBJ phVfsObj)
{
    RTVFSFSSTREAMINTERNAL *pThis = hVfsFss;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSDIR_MAGIC, VERR_INVALID_HANDLE);
    AssertPtrNullReturn(ppszName, VERR_INVALID_POINTER);
    if (ppszName)
        *ppszName = NULL;
    AssertPtrNullReturn(penmType, VERR_INVALID_POINTER);
    if (penmType)
        *penmType = RTVFSOBJTYPE_INVALID;
    AssertPtrNullReturn(penmType, VERR_INVALID_POINTER);
    if (phVfsObj)
        *phVfsObj = NIL_RTVFSOBJ;

    return pThis->pOps->pfnNext(pThis->Base.pvThis, ppszName, penmType, phVfsObj);
}




/*
 *
 *  D I R   D I R   D I R
 *  D I R   D I R   D I R
 *  D I R   D I R   D I R
 *
 */

RTDECL(uint32_t)    RTVfsDirRetain(RTVFSDIR hVfsDir)
{
    RTVFSDIRINTERNAL *pThis = hVfsDir;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSDIR_MAGIC, UINT32_MAX);
    return rtVfsObjRetain(&pThis->Base);
}


RTDECL(uint32_t)    RTVfsDirRelease(RTVFSDIR hVfsDir)
{
    RTVFSDIRINTERNAL *pThis = hVfsDir;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSDIR_MAGIC, UINT32_MAX);
    return rtVfsObjRelease(&pThis->Base);
}



/*
 *
 *  S Y M B O L I C   L I N K
 *  S Y M B O L I C   L I N K
 *  S Y M B O L I C   L I N K
 *
 */

RTDECL(uint32_t)    RTVfsSymlinkRetain(RTVFSSYMLINK hVfsSym)
{
    RTVFSSYMLINKINTERNAL *pThis = hVfsSym;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSSYMLINK_MAGIC, UINT32_MAX);
    return rtVfsObjRetain(&pThis->Base);
}


RTDECL(uint32_t)    RTVfsSymlinkRelease(RTVFSSYMLINK hVfsSym)
{
    RTVFSSYMLINKINTERNAL *pThis = hVfsSym;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSSYMLINK_MAGIC, UINT32_MAX);
    return rtVfsObjRelease(&pThis->Base);
}


RTDECL(int)         RTVfsSymlinkRead(RTVFSSYMLINK hVfsSym, char *pszTarget, size_t cbTarget)
{
    RTVFSSYMLINKINTERNAL *pThis = hVfsSym;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSSYMLINK_MAGIC, VERR_INVALID_HANDLE);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnRead(pThis->Base.pvThis, pszTarget, cbTarget);
    rtVfsObjWriteUnlock(&pThis->Base);

    return rc;
}



/*
 *
 *  I / O   S T R E A M     I / O   S T R E A M     I / O   S T R E A M
 *  I / O   S T R E A M     I / O   S T R E A M     I / O   S T R E A M
 *  I / O   S T R E A M     I / O   S T R E A M     I / O   S T R E A M
 *
 */

RTDECL(int) RTVfsNewIoStream(PCRTVFSIOSTREAMOPS pIoStreamOps, size_t cbInstance, uint32_t fOpen, RTVFS hVfs, RTSEMRW hSemRW,
                             PRTVFSIOSTREAM phVfsIos, void **ppvInstance)
{
    /*
     * Validate the input, be extra strict in strict builds.
     */
    AssertPtr(pIoStreamOps);
    AssertReturn(pIoStreamOps->uVersion   == RTVFSIOSTREAMOPS_VERSION, VERR_VERSION_MISMATCH);
    AssertReturn(pIoStreamOps->uEndMarker == RTVFSIOSTREAMOPS_VERSION, VERR_VERSION_MISMATCH);
    Assert(!pIoStreamOps->fReserved);
    Assert(cbInstance > 0);
    Assert(fOpen & RTFILE_O_ACCESS_MASK);
    AssertPtr(ppvInstance);
    AssertPtr(phVfsIos);

    RTVFSINTERNAL *pVfs = NULL;
    if (hVfs != NIL_RTVFS)
    {
        pVfs = hVfs;
        AssertPtrReturn(pVfs, VERR_INVALID_HANDLE);
        AssertReturn(pVfs->uMagic == RTVFS_MAGIC, VERR_INVALID_HANDLE);
    }

    /*
     * Allocate the handle + instance data.
     */
    size_t const cbThis = RT_ALIGN_Z(sizeof(RTVFSIOSTREAMINTERNAL), RTVFS_INST_ALIGNMENT)
                        + RT_ALIGN_Z(cbInstance, RTVFS_INST_ALIGNMENT);
    RTVFSIOSTREAMINTERNAL *pThis = (RTVFSIOSTREAMINTERNAL *)RTMemAllocZ(cbThis);
    if (!pThis)
        return VERR_NO_MEMORY;

    pThis->uMagic       = RTVFSIOSTREAM_MAGIC;
    pThis->fFlags       = fOpen;
    pThis->pOps         = pIoStreamOps;
    pThis->Base.uMagic  = RTVFSOBJ_MAGIC;
    pThis->Base.pvThis  = (char *)pThis + RT_ALIGN_Z(sizeof(*pThis), RTVFS_INST_ALIGNMENT);
    pThis->Base.pOps    = &pIoStreamOps->Obj;
    pThis->Base.hSemRW  = hSemRW != NIL_RTSEMRW ? hSemRW : pVfs ? pVfs->Base.hSemRW : NIL_RTSEMRW;
    pThis->Base.hVfs    = hVfs;
    pThis->Base.cRefs   = 1;
    if (hVfs != NIL_RTVFS)
        rtVfsObjRetainVoid(&pVfs->Base);

    *phVfsIos    = pThis;
    *ppvInstance = pThis->Base.pvThis;
    return VINF_SUCCESS;
}


RTDECL(uint32_t)    RTVfsIoStrmRetain(RTVFSIOSTREAM hVfsIos)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, UINT32_MAX);
    return rtVfsObjRetain(&pThis->Base);
}


RTDECL(uint32_t)    RTVfsIoStrmRelease(RTVFSIOSTREAM hVfsIos)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, UINT32_MAX);
    return rtVfsObjRelease(&pThis->Base);
}


RTDECL(RTVFSFILE)   RTVfsIoStrmToFile(RTVFSIOSTREAM hVfsIos)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, NIL_RTVFSFILE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, NIL_RTVFSFILE);

    if (pThis->pOps->Obj.enmType == RTVFSOBJTYPE_FILE)
    {
        rtVfsObjRetainVoid(&pThis->Base);
        return RT_FROM_MEMBER(pThis, RTVFSFILEINTERNAL, Stream);
    }

    /* this is no crime, so don't assert. */
    return NIL_RTVFSFILE;
}


RTDECL(int) RTVfsIoStrmQueryInfo(RTVFSIOSTREAM hVfsIos, PRTFSOBJINFO pObjInfo, RTFSOBJATTRADD enmAddAttr)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);
    return RTVfsObjQueryInfo(&pThis->Base, pObjInfo, enmAddAttr);
}


RTDECL(int) RTVfsIoStrmRead(RTVFSIOSTREAM hVfsIos, void *pvBuf, size_t cbToRead, bool fBlocking, size_t *pcbRead)
{
    AssertPtrNullReturn(pcbRead, VERR_INVALID_POINTER);
    if (pcbRead)
        *pcbRead = 0;
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);
    AssertReturn(fBlocking || pcbRead, VERR_INVALID_PARAMETER);

    RTSGSEG Seg = { pvBuf, cbToRead };
    RTSGBUF SgBuf;
    RTSgBufInit(&SgBuf, &Seg, 1);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnRead(pThis->Base.pvThis, -1 /*off*/, &SgBuf, fBlocking, pcbRead);
    rtVfsObjWriteUnlock(&pThis->Base);
    return rc;
}


RTDECL(int) RTVfsIoStrmWrite(RTVFSIOSTREAM hVfsIos, const void *pvBuf, size_t cbToWrite, bool fBlocking, size_t *pcbWritten)
{
    AssertPtrNullReturn(pcbWritten, VERR_INVALID_POINTER);
    if (pcbWritten)
        *pcbWritten = 0;
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);
    AssertReturn(fBlocking || pcbWritten, VERR_INVALID_PARAMETER);

    RTSGSEG Seg = { (void *)pvBuf, cbToWrite };
    RTSGBUF SgBuf;
    RTSgBufInit(&SgBuf, &Seg, 1);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnWrite(pThis->Base.pvThis, -1 /*off*/, &SgBuf, fBlocking, pcbWritten);
    rtVfsObjWriteUnlock(&pThis->Base);
    return rc;
}


RTDECL(int) RTVfsIoStrmSgRead(RTVFSIOSTREAM hVfsIos, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbRead)
{
    AssertPtrNullReturn(pcbRead, VERR_INVALID_POINTER);
    if (pcbRead)
        *pcbRead = 0;
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);
    AssertPtr(pSgBuf);
    AssertReturn(fBlocking || pcbRead, VERR_INVALID_PARAMETER);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnRead(pThis->Base.pvThis, -1 /*off*/, pSgBuf, fBlocking, pcbRead);
    rtVfsObjWriteUnlock(&pThis->Base);
    return rc;
}


RTDECL(int) RTVfsIoStrmSgWrite(RTVFSIOSTREAM hVfsIos, PCRTSGBUF pSgBuf, bool fBlocking, size_t *pcbWritten)
{
    AssertPtrNullReturn(pcbWritten, VERR_INVALID_POINTER);
    if (pcbWritten)
        *pcbWritten = 0;
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);
    AssertPtr(pSgBuf);
    AssertReturn(fBlocking || pcbWritten, VERR_INVALID_PARAMETER);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnWrite(pThis->Base.pvThis, -1 /*off*/, pSgBuf, fBlocking, pcbWritten);
    rtVfsObjWriteUnlock(&pThis->Base);
    return rc;
}


RTDECL(int) RTVfsIoStrmFlush(RTVFSIOSTREAM hVfsIos)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnFlush(pThis->Base.pvThis);
    rtVfsObjWriteUnlock(&pThis->Base);
    return rc;
}


RTDECL(RTFOFF) RTVfsIoStrmPoll(RTVFSIOSTREAM hVfsIos, uint32_t fEvents, RTMSINTERVAL cMillies, bool fIntr,
                               uint32_t *pfRetEvents)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, VERR_INVALID_HANDLE);

    rtVfsObjWriteLock(&pThis->Base);
    int rc = pThis->pOps->pfnPollOne(pThis->Base.pvThis, fEvents, cMillies, fIntr, pfRetEvents);
    rtVfsObjWriteUnlock(&pThis->Base);
    return rc;
}


RTDECL(RTFOFF) RTVfsIoStrmTell(RTVFSIOSTREAM hVfsIos)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, -1);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, -1);

    RTFOFF off;
    rtVfsObjReadLock(&pThis->Base);
    int rc = pThis->pOps->pfnTell(pThis->Base.pvThis, &off);
    rtVfsObjReadUnlock(&pThis->Base);
    if (RT_FAILURE(rc))
        off = rc;
    return off;
}


RTDECL(int) RTVfsIoStrmSkip(RTVFSIOSTREAM hVfsIos, RTFOFF cb)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, -1);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, -1);
    AssertReturn(cb >= 0, VERR_INVALID_PARAMETER);

    int rc;
    if (pThis->pOps->pfnSkip)
    {
        rtVfsObjWriteLock(&pThis->Base);
        rc = pThis->pOps->pfnSkip(pThis->Base.pvThis, cb);
        rtVfsObjWriteUnlock(&pThis->Base);
    }
    else
    {
        void *pvBuf = RTMemTmpAlloc(_64K);
        if (pvBuf)
        {
            rc = VINF_SUCCESS;
            while (cb > 0)
            {
                size_t cbToRead = RT_MIN(cb, _64K);
                rtVfsObjWriteLock(&pThis->Base);
                rc = RTVfsIoStrmRead(hVfsIos, pvBuf, cbToRead, true /*fBlocking*/, NULL);
                rtVfsObjWriteUnlock(&pThis->Base);
                if (RT_FAILURE(rc))
                    break;
                cb -= cbToRead;
            }

            RTMemTmpFree(pvBuf);
        }
        else
            rc = VERR_NO_TMP_MEMORY;
    }
    return rc;
}


RTDECL(int) RTVfsIoStrmZeroFill(RTVFSIOSTREAM hVfsIos, RTFOFF cb)
{
    RTVFSIOSTREAMINTERNAL *pThis = hVfsIos;
    AssertPtrReturn(pThis, -1);
    AssertReturn(pThis->uMagic == RTVFSIOSTREAM_MAGIC, -1);

    int rc;
    if (pThis->pOps->pfnSkip)
    {
        rtVfsObjWriteLock(&pThis->Base);
        rc = pThis->pOps->pfnZeroFill(pThis->Base.pvThis, cb);
        rtVfsObjWriteUnlock(&pThis->Base);
    }
    else
    {
        void *pvBuf = RTMemTmpAllocZ(_64K);
        if (pvBuf)
        {
            rc = VINF_SUCCESS;
            while (cb > 0)
            {
                size_t cbToWrite = RT_MIN(cb, _64K);
                rtVfsObjWriteLock(&pThis->Base);
                rc = RTVfsIoStrmWrite(hVfsIos, pvBuf, cbToWrite, true /*fBlocking*/, NULL);
                rtVfsObjWriteUnlock(&pThis->Base);
                if (RT_FAILURE(rc))
                    break;
                cb -= cbToWrite;
            }

            RTMemTmpFree(pvBuf);
        }
        else
            rc = VERR_NO_TMP_MEMORY;
    }
    return rc;
}






/*
 *
 *  F I L E   F I L E   F I L E
 *  F I L E   F I L E   F I L E
 *  F I L E   F I L E   F I L E
 *
 */

RTDECL(int) RTVfsNewFile(PCRTVFSFILEOPS pFileOps, size_t cbInstance, uint32_t fOpen, RTVFS hVfs,
                         PRTVFSFILE phVfsFile, void **ppvInstance)
{
    /*
     * Validate the input, be extra strict in strict builds.
     */
    AssertPtr(pFileOps);
    AssertReturn(pFileOps->uVersion   == RTVFSFILEOPS_VERSION, VERR_VERSION_MISMATCH);
    AssertReturn(pFileOps->uEndMarker == RTVFSFILEOPS_VERSION, VERR_VERSION_MISMATCH);
    Assert(!pFileOps->fReserved);
    Assert(cbInstance > 0);
    Assert(fOpen & RTFILE_O_ACCESS_MASK);
    AssertPtr(ppvInstance);
    AssertPtr(phVfsFile);

    RTVFSINTERNAL *pVfs = NULL;
    if (hVfs != NIL_RTVFS)
    {
        pVfs = hVfs;
        AssertPtrReturn(pVfs, VERR_INVALID_HANDLE);
        AssertReturn(pVfs->uMagic == RTVFS_MAGIC, VERR_INVALID_HANDLE);
    }

    /*
     * Allocate the handle + instance data.
     */
    size_t const cbThis = RT_ALIGN_Z(sizeof(RTVFSFILEINTERNAL), RTVFS_INST_ALIGNMENT)
                        + RT_ALIGN_Z(cbInstance, RTVFS_INST_ALIGNMENT);
    RTVFSFILEINTERNAL *pThis = (RTVFSFILEINTERNAL *)RTMemAllocZ(cbThis);
    if (!pThis)
        return VERR_NO_MEMORY;

    pThis->uMagic               = RTVFSFILE_MAGIC;
    pThis->fReserved            = 0;
    pThis->pOps                 = pFileOps;
    pThis->Stream.uMagic        = RTVFSIOSTREAM_MAGIC;
    pThis->Stream.fFlags        = fOpen;
    pThis->Stream.pOps          = &pFileOps->Stream;
    pThis->Stream.Base.pvThis   = (char *)pThis + RT_ALIGN_Z(sizeof(*pThis), RTVFS_INST_ALIGNMENT);
    pThis->Stream.Base.pOps     = &pFileOps->Stream.Obj;
    pThis->Stream.Base.hSemRW   = pVfs ? pVfs->Base.hSemRW : NIL_RTSEMRW;
    pThis->Stream.Base.hVfs     = hVfs;
    pThis->Stream.Base.cRefs    = 1;
    if (hVfs != NIL_RTVFS)
        rtVfsObjRetainVoid(&pVfs->Base);

    *phVfsFile   = pThis;
    *ppvInstance = pThis->Stream.Base.pvThis;
    return VINF_SUCCESS;
}


RTDECL(int)         RTVfsFileOpen(RTVFS hVfs, const char *pszFilename, uint32_t fOpen, PRTVFSFILE phVfsFile)
{
    /*
     * Validate input.
     */
    RTVFSINTERNAL *pThis = hVfs;
    AssertPtrReturn(pThis, VERR_INVALID_HANDLE);
    AssertReturn(pThis->uMagic == RTVFS_MAGIC, VERR_INVALID_HANDLE);
    AssertPtrReturn(pszFilename, VERR_INVALID_POINTER);
    AssertPtrReturn(phVfsFile, VERR_INVALID_POINTER);

    int rc = rtFileRecalcAndValidateFlags(&fOpen);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Parse the path, assume current directory is root since we've got no
     * caller context here.
     */
    PRTVFSPARSEDPATH pPath;
    rc = RTVfsParsePathA(pszFilename, "/", &pPath);
    if (RT_SUCCESS(rc))
    {
        if (!pPath->fDirSlash)
        {
            /*
             * Tranverse the path, resolving the parent node and any symlinks
             * in the final element, and ask the directory to open the file.
             */
            RTVFSDIRINTERNAL *pVfsParentDir;
            rc = rtVfsTraverseToParent(pThis, pPath, true /*fFollowSymlink*/, &pVfsParentDir);
            if (RT_SUCCESS(rc))
            {
                const char *pszEntryName = &pPath->szPath[pPath->aoffComponents[pPath->cComponents - 1]];

                /** @todo there is a symlink creation race here. */
                rtVfsObjWriteLock(&pVfsParentDir->Base);
                rc = pVfsParentDir->pOps->pfnOpenFile(pVfsParentDir->Base.pvThis, pszEntryName, fOpen, phVfsFile);
                rtVfsObjWriteUnlock(&pVfsParentDir->Base);

                RTVfsDirRelease(pVfsParentDir);

                if (RT_SUCCESS(rc))
                {
                    AssertPtr(*phVfsFile);
                    Assert((*phVfsFile)->uMagic == RTVFSFILE_MAGIC);
                }
            }
        }
        else
            rc = VERR_INVALID_PARAMETER;
        RTVfsParsePathFree(pPath);
    }
    return rc;
}


RTDECL(uint32_t)    RTVfsFileRetain(RTVFSFILE hVfsFile)
{
    RTVFSFILEINTERNAL *pThis = hVfsFile;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSFILE_MAGIC, UINT32_MAX);
    return rtVfsObjRetain(&pThis->Stream.Base);
}


RTDECL(uint32_t)    RTVfsFileRelease(RTVFSFILE hVfsFile)
{
    RTVFSFILEINTERNAL *pThis = hVfsFile;
    AssertPtrReturn(pThis, UINT32_MAX);
    AssertReturn(pThis->uMagic == RTVFSFILE_MAGIC, UINT32_MAX);
    return rtVfsObjRelease(&pThis->Stream.Base);
}


RTDECL(RTVFSIOSTREAM) RTVfsFileToIoStream(RTVFSFILE hVfsFile)
{
    RTVFSFILEINTERNAL *pThis = hVfsFile;
    AssertPtrReturn(pThis, NIL_RTVFSIOSTREAM);
    AssertReturn(pThis->uMagic == RTVFSFILE_MAGIC, NIL_RTVFSIOSTREAM);

    rtVfsObjRetainVoid(&pThis->Stream.Base);
    return &pThis->Stream;
}



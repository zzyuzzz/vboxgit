/* $Revision: 21227 $ */
/** @file
 * VBoxGuestLibR0 - Internal header.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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

#ifndef ___VBoxGuestLib_VBGLInternal_h
#define ___VBoxGuestLib_VBGLInternal_h

#include <VBox/VMMDev.h>
#include <VBox/VBoxGuest.h>
#include <VBox/VBoxGuestLib.h>

#include <VBox/log.h>

/** @todo dprintf() -> Log() */
#if (defined(DEBUG) && !defined(NO_LOGGING)) || defined(LOG_ENABLED)
# define dprintf(a) RTLogBackdoorPrintf a
#else
# define dprintf(a) do {} while (0)
#endif

#include "SysHlp.h"

#pragma pack(4)

struct _VBGLPHYSHEAPBLOCK;
typedef struct _VBGLPHYSHEAPBLOCK VBGLPHYSHEAPBLOCK;
struct _VBGLPHYSHEAPCHUNK;
typedef struct _VBGLPHYSHEAPCHUNK VBGLPHYSHEAPCHUNK;

#ifndef VBGL_VBOXGUEST
struct VBGLHGCMHANDLEDATA
{
    uint32_t fAllocated;
    VBGLDRIVER driver;
};
#endif

enum VbglLibStatus
{
    VbglStatusNotInitialized = 0,
    VbglStatusInitializing,
    VbglStatusReady
};

typedef struct _VBGLDATA
{
    enum VbglLibStatus status;

    VBGLIOPORT portVMMDev;

    VMMDevMemory *pVMMDevMemory;

    /**
     * Physical memory heap data.
     * @{
     */

    VBGLPHYSHEAPBLOCK *pFreeBlocksHead;
    VBGLPHYSHEAPBLOCK *pAllocBlocksHead;
    VBGLPHYSHEAPCHUNK *pChunkHead;

    RTSEMFASTMUTEX mutexHeap;
    /** @} */

#ifndef VBGL_VBOXGUEST
    /**
     * Fast heap for HGCM handles data.
     * @{
     */

    RTSEMFASTMUTEX mutexHGCMHandle;

    struct VBGLHGCMHANDLEDATA aHGCMHandleData[64];

    /** @} */
#endif
} VBGLDATA;

#pragma pack()

#ifndef VBGL_DECL_DATA
extern VBGLDATA g_vbgldata;
#endif

/* Check if library has been initialized before entering
 * a public library function.
 */
int VbglEnter (void);

#ifdef VBOX_WITH_HGCM
# ifndef VBGL_VBOXGUEST
/* Initialize HGCM subsystem. */
int vbglHGCMInit (void);
/* Terminate HGCM subsystem. */
int vbglHGCMTerminate (void);
# endif
#endif /* VBOX_WITH_HGCM */

#endif /* !___VBoxGuestLib_VBGLInternal_h */


/* $Id: libXfixes.c 25988 2010-01-24 20:51:35Z vboxsync $ */

/** @file
 * X.Org libXfixes.so linker stub
 */

/*
 * Copyright (C) 2010 Sun Microsystems, Inc.
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

#include <iprt/cdefs.h>
#include <iprt/types.h>

#define DECLXSTUB(func) \
    DECLEXPORT(void) func(void); \
    void func(void) {}

DECLXSTUB(XFixesTranslateRegion)
DECLXSTUB(XFixesRegionExtents)
DECLXSTUB(XFixesFetchRegion)
DECLXSTUB(XFixesExpandRegion)
DECLXSTUB(XFixesSelectSelectionInput)
DECLXSTUB(XFixesChangeCursorByName)
DECLXSTUB(XFixesInvertRegion)
DECLXSTUB(XFixesQueryVersion)
DECLXSTUB(XFixesCreateRegionFromBitmap)
DECLXSTUB(XFixesSetRegion)
DECLXSTUB(XFixesCreateRegion)
DECLXSTUB(XFixesGetCursorImage)
DECLXSTUB(XFixesDestroyRegion)
DECLXSTUB(XFixesSetCursorName)
DECLXSTUB(XFixesIntersectRegion)
DECLXSTUB(XFixesCopyRegion)
DECLXSTUB(XFixesSelectCursorInput)
DECLXSTUB(XFixesShowCursor)
DECLXSTUB(XFixesHideCursor)
DECLXSTUB(XFixesGetCursorName)
DECLXSTUB(XFixesExtensionName)
DECLXSTUB(XFixesVersion)
DECLXSTUB(XFixesSetGCClipRegion)
DECLXSTUB(XFixesQueryExtension)
DECLXSTUB(XFixesFindDisplay)
DECLXSTUB(XFixesChangeCursor)
DECLXSTUB(XFixesSetWindowShapeRegion)
DECLXSTUB(XFixesCreateRegionFromPicture)
DECLXSTUB(XFixesSetPictureClipRegion)
DECLXSTUB(XFixesFetchRegionAndBounds)
DECLXSTUB(XFixesExtensionInfo)
DECLXSTUB(XFixesSubtractRegion)
DECLXSTUB(XFixesUnionRegion)
DECLXSTUB(XFixesCreateRegionFromGC)
DECLXSTUB(XFixesChangeSaveSet)
DECLXSTUB(XFixesCreateRegionFromWindow)

/* $Id: darwin-pasteboard.h 21293 2009-07-07 08:01:25Z vboxsync $ */
/** @file
 * Shared Clipboard: Mac OS X host implementation.
 */

/*
 * Copyright (C) 2008 Sun Microsystems, Inc.
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

#ifndef ___DARWIN_PASTEBOARD_H
#define ___DARWIN_PASTEBOARD_H

typedef struct OpaquePasteboardRef;
typedef struct OpaquePasteboardRef *PasteboardRef;

int initPasteboard (PasteboardRef *pPasteboardRef);
void destroyPasteboard (PasteboardRef *pPasteboardRef);

int queryNewPasteboardFormats (PasteboardRef pPasteboard, uint32_t *pfFormats, bool *pfChanged);
int readFromPasteboard (PasteboardRef pPasteboard, uint32_t fFormat, void *pv, uint32_t cb, uint32_t *pcbActual);
int writeToPasteboard (PasteboardRef pPasteboard, void *pv, uint32_t cb, uint32_t fFormat);

#endif


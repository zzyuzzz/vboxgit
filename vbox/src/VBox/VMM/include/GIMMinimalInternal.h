/* $Id: GIMMinimalInternal.h 52675 2014-09-10 13:24:03Z vboxsync $ */
/** @file
 * GIM - Minimal, Internal header file.
 */

/*
 * Copyright (C) 2014 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___GIMMinimalInternal_h
#define ___GIMMinimalInternal_h

#include <iprt/cdefs.h>
#include <VBox/types.h>

RT_C_DECLS_BEGIN

#ifdef IN_RING3
VMMR3_INT_DECL(int)         GIMR3MinimalInit(PVM pVM);
VMMR3_INT_DECL(int)         GIMR3MinimalInitFinalize(PVM pVM);
VMMR3_INT_DECL(void)        GIMR3MinimalRelocate(PVM pVM, RTGCINTPTR offDelta);
#endif /* IN_RING3 */

RT_C_DECLS_END

#endif /* ___GIMMinimalInternal_h */


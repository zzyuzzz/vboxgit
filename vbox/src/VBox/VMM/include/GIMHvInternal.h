/* $Id: GIMHvInternal.h 50953 2014-04-02 14:47:00Z vboxsync $ */
/** @file
 * GIM - Hyper-V, Internal header file.
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

#ifndef ___GIMHvInternal_h
#define ___GIMHvInternal_h

#include <iprt/cdefs.h>
#include <VBox/types.h>

RT_C_DECLS_BEGIN

#ifdef IN_RING3
VMMR3_INT_DECL(int)         GIMR3HvInit(PVM pVM);
VMMR3_INT_DECL(void)        GIMR3HvRelocate(PVM pVM, RTGCINTPTR offDelta);
#endif /* IN_RING3 */

DECLEXPORT(int)             GIMHvHypercall(PVMCPU pVCpu, PCPUMCTX pCtx);

RT_C_DECLS_END

#endif /* ___GIMHvInternal_h */


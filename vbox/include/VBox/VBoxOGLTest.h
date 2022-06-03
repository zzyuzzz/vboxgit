/* $Id: VBoxOGLTest.h 40858 2012-04-10 19:52:43Z vboxsync $ */
/** @file
 * VBox 3D Support test API
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
#ifndef ___VBoxOGLTest_h__
#define ___VBoxOGLTest_h__

#include <iprt/cdefs.h>

RT_C_DECLS_BEGIN

bool RTCALL VBoxOglIs3DAccelerationSupported();

RT_C_DECLS_END

#endif /*#ifndef ___VBoxOGLTest_h__*/

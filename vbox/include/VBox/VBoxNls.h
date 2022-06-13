/** @file
 * VBox NLS.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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

#ifndef VBOX_INCLUDED_VBoxNls_h
#define VBOX_INCLUDED_VBoxNls_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef VBOX_WITH_MAIN_NLS

#include <VBox/com/defs.h>
#include <VBox/com/ptr.h>
#include <VBox/com/string.h>
#include "VirtualBoxTranslator.h"

#define DECLARE_TRANSLATION_CONTEXT(ctx) \
struct ctx \
{\
   static const char *tr(const char *pszSource, const char *pszComment = NULL, const int iNum = -1) \
   { \
       return VirtualBoxTranslator::translate(NULL, #ctx, pszSource, pszComment, iNum); \
   } \
};
#else
#define DECLARE_TRANSLATION_CONTEXT(ctx) \
struct ctx \
{\
   static const char *tr(const char *pszSource, const char *pszComment = NULL, const int iNum = -1) \
   { \
       NOREF(pszComment); \
       NOREF(iNum);       \
       return pszSource;  \
   } \
};
#endif

/** @} */

#endif /* !VBOX_INCLUDED_VBoxNls_h */


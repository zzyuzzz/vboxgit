/* $Id: bs3kit-template-footer.h 58628 2015-11-10 01:25:13Z vboxsync $ */
/** @file
 * BS3Kit footer for multi-mode code templates.
 */

/*
 * Copyright (C) 2007-2015 Oracle Corporation
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


/*
 * Undefine macros defined by the header.
 * This is a subset of what bs3kit-template-footer.mac does.
 */
#undef TMPL_RM
#undef TMPL_PE16
#undef TMPL_PE32
#undef TMPL_PEV86
#undef TMPL_PP16
#undef TMPL_PP32
#undef TMPL_PPV86
#undef TMPL_PAE16
#undef TMPL_PAE32
#undef TMPL_PAEV86
#undef TMPL_LM16
#undef TMPL_LM32
#undef TMPL_LM64

#undef TMPL_CMN_PE
#undef TMPL_CMN_PP
#undef TMPL_CMN_PAE
#undef TMPL_CMN_LM
#undef TMPL_CMN_V86

#undef TMPL_CMN_P16
#undef TMPL_CMN_P32
#undef TMPL_CMN_P64
#undef TMPL_CMN_R16
#undef TMPL_CMN_R86

#undef TMPL_NM
#undef TMPL_NM_CMN
#undef TMPL_MODE
#undef TMPL_MODE_STR
#undef TMPL_16BIT
#undef TMPL_32BIT
#undef TMPL_64BIT
#undef TMPL_BITS


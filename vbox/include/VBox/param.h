/** @file
 * VirtualBox Parameter Definitions.
 */

/*
 * Copyright (C) 2006-2007 innotek GmbH
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

#ifndef ___VBox_param_h
#define ___VBox_param_h

#include <iprt/param.h>


/** @defgroup   grp_vbox_param  VBox Parameter Definition
 * @{
 */


/** @defgroup   grp_vbox_param_mm  Memory Monitor Parameters
 * @ingroup grp_vbox_param
 * @{
 */

/** Initial address of Hypervisor Memory Area.
 * MUST BE PAGE TABLE ALIGNED! */
#define MM_HYPER_AREA_ADDRESS   0xa0000000

/** The max size of the hypervisor memory area. */
#define MM_HYPER_AREA_MAX_SIZE (16*1024*1024)

/** Maximum number of bytes we can dynamically map into the hypervisor region.
 * This must be a power of 2 number of pages!
 */
#define MM_HYPER_DYNAMIC_SIZE   (8 * PAGE_SIZE)

/** @} */


/** @defgroup   grp_vbox_param_vmm  VMM Parameters
 * @ingroup grp_vbox_param
 * @{
 */

/** VMM stack size. */
#define VMM_STACK_SIZE    8192

/** @} */


/** @} */

#endif


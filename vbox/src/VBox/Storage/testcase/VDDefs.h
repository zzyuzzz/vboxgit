/** $Id: VDDefs.h 76530 2018-12-30 06:00:13Z vboxsync $ */
/** @file
 *
 * VBox HDD container test utility, common definitions.
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __VDDefs_h__
#define __VDDefs_h__
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/sg.h>

/**
 * I/O transfer direction.
 */
typedef enum VDIOTXDIR
{
    /** Read. */
    VDIOTXDIR_READ = 0,
    /** Write. */
    VDIOTXDIR_WRITE,
    /** Flush. */
    VDIOTXDIR_FLUSH,
    /** Invalid. */
    VDIOTXDIR_INVALID
} VDIOTXDIR;

#endif /* __VDDefs_h__ */

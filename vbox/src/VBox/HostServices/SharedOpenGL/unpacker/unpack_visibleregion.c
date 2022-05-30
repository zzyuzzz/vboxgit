/* $Id: unpack_visibleregion.c 15532 2008-12-15 18:53:11Z vboxsync $ */

/** @file
 * VBox Packing VisibleRegion information
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

#include "unpacker.h"
#include "cr_error.h"
#include "cr_protocol.h"
#include "cr_mem.h"
#include "cr_version.h"

void crUnpackExtendWindowVisibleRegion( void )
{
    GLint window = READ_DATA( 8, GLint );
    GLint cRects = READ_DATA( 12, GLint );
    GLvoid *pRects = DATA_POINTER( 16, GLvoid );;
    cr_unpackDispatch.WindowVisibleRegion( window, cRects, pRects );
}

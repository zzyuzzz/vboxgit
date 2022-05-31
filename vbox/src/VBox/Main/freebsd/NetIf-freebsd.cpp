/* $Id: NetIf-freebsd.cpp 18971 2009-04-16 23:41:48Z vboxsync $ */
/** @file
 * Main - NetIfList, FreeBSD implementation.
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



/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_MAIN

#include "HostNetworkInterfaceImpl.h"
#include "netif.h"
#include "Logging.h"

int NetIfList(std::list <ComObjPtr <HostNetworkInterface> > &list)
{
    /** @todo implement */
    return VERR_NOT_IMPLEMENTED;
}

/** @file
 * IPRT - TCP/IP.
 */

/*
 * Copyright (C) 2008-2022 Oracle Corporation
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

#ifndef IPRT_INCLUDED_cidr_h
#define IPRT_INCLUDED_cidr_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/net.h>

/** @defgroup grp_rt_cidr   RTCidr - Classless Inter-Domain Routing notation
 * @ingroup grp_rt
 * @{
 */
RT_C_DECLS_BEGIN

/**
 * Parse a string which contains an IP address in CIDR (Classless
 * Inter-Domain Routing) notation.
 *
 * @warning The network address and the network mask are returned in
 *      @b host(!) byte order.  This is different from all the other
 *      RTNetStrTo* functions.
 *
 * @deprecated This function is superseded by RTNetStrToIPv4Cidr()
 *      that provides a better API consistent with other functions
 *      from that family.  It returns the prefix length, if you need a
 *      netmask, you can obtain it with RTNetPrefixToMaskIPv4().
 *
 * @return iprt status code.
 *
 * @param   pszAddress  The IP address in CIDR specificaion.
 * @param   pNetwork    The determined IP address / network in host byte order.
 * @param   pNetmask    The determined netmask in host byte order.
 */
RTDECL(int) RTCidrStrToIPv4(const char *pszAddress, PRTNETADDRIPV4 pNetwork, PRTNETADDRIPV4 pNetmask);

RT_C_DECLS_END
/** @} */

#endif /* !IPRT_INCLUDED_cidr_h */

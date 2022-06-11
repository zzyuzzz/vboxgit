/* $Id: rsa-internal.h 82968 2020-02-04 10:35:17Z vboxsync $ */
/** @file
 * IPRT - Crypto - RSA, Internal Header.
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

#ifndef IPRT_INCLUDED_SRC_common_crypto_rsa_internal_h
#define IPRT_INCLUDED_SRC_common_crypto_rsa_internal_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/** The max number of bits we support in the modulus. */
#define RTCRRSA_MAX_MODULUS_BITS        16384

#define RTASN1TMPL_TEMPLATE_FILE "../common/crypto/rsa-template.h"
#include <iprt/asn1-generator-internal-header.h>

#endif /* !IPRT_INCLUDED_SRC_common_crypto_rsa_internal_h */


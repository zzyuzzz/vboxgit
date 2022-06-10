/* $Id: pkix-signature-builtin.h 76585 2019-01-01 06:31:29Z vboxsync $ */
/** @file
 * IPRT - Crypto - Public Key Signature Schemas, Built-in providers.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
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

#ifndef IPRT_INCLUDED_SRC_common_crypto_pkix_signature_builtin_h
#define IPRT_INCLUDED_SRC_common_crypto_pkix_signature_builtin_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <iprt/crypto/pkix.h>

extern DECLHIDDEN(RTCRPKIXSIGNATUREDESC const) g_rtCrPkixSigningHashWithRsaDesc;

#endif /* !IPRT_INCLUDED_SRC_common_crypto_pkix_signature_builtin_h */


/* WARNING: do not edit! */
/* Generated by Makefile from crypto/include/internal/bn_conf.h.in */
/*
 * Copyright 2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef HEADER_BN_CONF_H
# define HEADER_BN_CONF_H

/*
 * The contents of this file are not used in the UEFI build, as
 * both 32-bit and 64-bit builds are supported from a single run
 * of the Configure script.
 */

/* Should we define BN_DIV2W here? */

/* Only one for the following should be defined */
#include <iprt/cdefs.h>
#if defined(RT_ARCH_AMD64) || defined(RT_ARCH_SPARC64) || defined(RT_ARCH_ARM64)
# ifdef _MSC_VER
#  undef SIXTY_FOUR_BIT_LONG
#  define SIXTY_FOUR_BIT
#  undef THIRTY_TWO_BIT
# else
#  define SIXTY_FOUR_BIT_LONG
#  undef SIXTY_FOUR_BIT
#  undef THIRTY_TWO_BIT
# endif
#elif defined(RT_ARCH_X86) || defined(RT_ARCH_SPARC) || defined(RT_ARCH_ARM32)
# undef SIXTY_FOUR_BIT_LONG
# undef SIXTY_FOUR_BIT
# define THIRTY_TWO_BIT
#else
# error "Unknown/missing RT_ARCH_*." * /* vbox: 64-bit (cannot safely use ARCH_BITS without including iprt/cdefs.h) */
#endif

#endif

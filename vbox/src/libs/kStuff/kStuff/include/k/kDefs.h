/* $Id: kDefs.h 3598 2007-10-04 18:46:12Z bird $ */
/** @file
 *
 * kTypes - Defines and Macros.
 *
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 *
 * This file is part of k*.
 *
 * k* is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * k* is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with k*; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef ___k_kDefs_h___
#define ___k_kDefs_h___

/** @defgroup grp_kDefs  kDefs - Defines and Macros
 * @{ */

/** @name Operative System Identifiers.
 * These are the value that the K_OS \#define can take.
 * @{
 */
/** Unknown OS. */
#define K_OS_UNKNOWN    0
/** Darwin - aka Mac OS X. */
#define K_OS_DARWIN     1
/** FreeBSD. */
#define K_OS_FREEBSD    2
/** Linux. */
#define K_OS_LINUX      3
/** NetBSD. */
#define K_OS_NETBSD     4
/** NT (native). */
#define K_OS_NT         5
/** OpenBSD*/
#define K_OS_OPENBSD    6
/** OS/2 */
#define K_OS_OS2        7
/** Solaris */
#define K_OS_SOLARIS    8
/** Windows. */
#define K_OS_WINDOWS    9
/** The max K_OS_* value (exclusive). */
#define K_OS_MAX        10
/** @} */

/** @def K_OS
 * Indicates which OS we're targetting. It's a \#define with is
 * assigned one of the K_OS_* defines above.
 *
 * So to test if we're on FreeBSD do the following:
 * @code
 *  #if K_OS == K_OS_FREEBSD
 *  some_funky_freebsd_specific_stuff();
 *  #endif
 * @endcode
 */
#ifndef K_OS
# if defined(__APPLE__)
#  define K_OS      K_OS_DARWIN
# elif defined(__FreeBSD__) /*??*/
#  define K_OS      K_OS_FREEBSD
# elif defined(__gnu_linux__)
#  define K_OS      K_OS_LINUX
# elif defined(__NetBSD__) /*??*/
#  define K_OS      K_OS_NETBSD
# elif defined(__OpenBSD__) /*??*/
#  define K_OS      K_OS_OPENBSD
# elif defined(__OS2__)
#  define K_OS      K_OS_OS2
# elif defined(__SunOrSomething__)
#  define K_OS      K_OS_SOLARIS
# elif defined(_WIN32) || defined(_WIN64)
#  define K_OS      K_OS_WINDOWS
# else
#  error "Port Me"
# endif
#endif
#if K_OS < K_OS_UNKNOWN || K_OS >= K_OS_MAX
# error "Invalid K_OS value."
#endif



/** @name   Architecture bit width.
 * @{ */
#define K_ARCH_BIT_8            0x0100  /**< 8-bit */
#define K_ARCH_BIT_16           0x0200  /**< 16-bit */
#define K_ARCH_BIT_32           0x0400  /**< 32-bit */
#define K_ARCH_BIT_64           0x0800  /**< 64-bit */
#define K_ARCH_BIT_128          0x1000  /**< 128-bit */
#define K_ARCH_BIT_MASK         0x1f00  /**< The bit mask. */
#define K_ARCH_BIT_SHIFT        5       /**< Shift count for producing the width in bits. */
#define K_ARCH_BYTE_SHIFT       8       /**< Shift count for producing the width in bytes. */
/** @} */

/** @name   Architecture Endianness.
 * @{ */
#define K_ARCH_END_LITTLE       0x2000  /**< Little-endian. */
#define K_ARCH_END_BIG          0x4000  /**< Big-endian. */
#define K_ARCH_END_BI           0x6000  /**< Bi-endian, can be switched. */
#define K_ARCH_END_MASK         0x6000  /**< The endian mask. */
#define K_ARCH_END_SHIFT        13      /**< Shift count for converting between this K_ENDIAN_*. */
/** @} */

/** @name Architecture Identifiers.
 * These are the value that the K_ARCH \#define can take.
 *@{ */
/** Unknown CPU architecture. */
#define K_ARCH_UNKNOWN          ( 0 )
/** Clone or Intel 16-bit x86. */
#define K_ARCH_X86_16           ( 1 | K_ARCH_BIT_16 | K_ARCH_END_LITTLE)
/** Clone or Intel 32-bit x86. */
#define K_ARCH_X86_32           ( 2 | K_ARCH_BIT_32 | K_ARCH_END_LITTLE)
/** AMD64 (including clones). */
#define K_ARCH_AMD64            ( 3 | K_ARCH_BIT_64 | K_ARCH_END_LITTLE)
/** Itanic (64-bit). */
#define K_ARCH_IA64             ( 4 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** ALPHA (64-bit). */
#define K_ARCH_ALPHA            ( 5 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** ALPHA limited to 32-bit. */
#define K_ARCH_ALPHA_32         ( 6 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 32-bit ARM. */
#define K_ARCH_ARM_32           ( 7 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit ARM. */
#define K_ARCH_ARM_64           ( 8 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32-bit MIPS. */
#define K_ARCH_MIPS_32          ( 9 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit MIPS. */
#define K_ARCH_MIPS_64          (10 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32-bit PowerPC. */
#define K_ARCH_POWERPC_32       (11 | K_ARCH_BIT_32 | K_ARCH_END_BI)
/** 64-bit PowerPC. */
#define K_ARCH_POWERPC_64       (12 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** 32-bit SPARC. */
#define K_ARCH_SPARC_32         (13 | K_ARCH_BIT_32 | K_ARCH_END_BIG)
/** 64-bit SPARC. */
#define K_ARCH_SPARC_64         (14 | K_ARCH_BIT_64 | K_ARCH_END_BI)
/** The end of the valid architecture values (exclusive). */
#define K_ARCH_MAX              (15)
/** @} */


/** @def K_ARCH
 * The value of this \#define indicates which architecture we're targetting.
 */
#ifndef K_ARCH
  /* detection based on compiler defines. */
# if defined(__amd64__) || defined(__x86_64__) || defined(__AMD64__) || defined(_M_X64)
#  define K_ARCH    K_ARCH_AMD64
# elif defined(__i386__) || defined(__x86__) || defined(__X86__) || defined(_M_IX86)
#  define K_ARCH    K_ARCH_X86_32
# elif defined(__ia64__) || defined(__IA64__) || defined(_M_IA64)
#  define K_ARCH    K_ARCH_IA64
# else
#  error "Port Me"
# endif
#else
  /* validate the user specified value. */
# if (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_8 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_16 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_32 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_64 \
  && (K_ARCH & K_ARCH_BIT_MASK) != K_ARCH_BIT_128
#  error "Invalid K_ARCH value (bit)"
# endif
# if (K_ARCH & K_ARCH_END_MASK) != K_ARCH_END_LITTLE \
  && (K_ARCH & K_ARCH_END_MASK) != K_ARCH_END_BIG \
  && (K_ARCH & K_ARCH_END_MASK) != K_ARCH_END_BI
#  error "Invalid K_ARCH value (endian)"
# endif
# if (K_ARCH & ~(K_ARCH_BIT_MASK | K_ARCH_BIT_END_MASK)) < K_ARCH_UNKNOWN \
  || (K_ARCH & ~(K_ARCH_BIT_MASK | K_ARCH_BIT_END_MASK)) >= K_ARCH_MAX
#  error "Invalid K_ARCH value"
# endif
#endif

/** @def K_ARCH_IS_VALID
 * Check if the architecture identifier is valid.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_IS_VALID(arch)   (   (   ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_8 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_16 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_32 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_64 \
                                     || ((arch) & K_ARCH_BIT_MASK) == K_ARCH_BIT_128) \
                                 && \
                                    (   ((arch) & K_ARCH_END_MASK) == K_ARCH_END_LITTLE \
                                     || ((arch) & K_ARCH_END_MASK) == K_ARCH_END_BIG \
                                     || ((arch) & K_ARCH_END_MASK) == K_ARCH_END_BI) \
                                 && \
                                    (   ((arch) & ~(K_ARCH_BIT_MASK | K_ARCH_END_MASK)) >= K_ARCH_UNKNOWN \
                                     && ((arch) & ~(K_ARCH_BIT_MASK | K_ARCH_END_MASK)) < K_ARCH_MAX) \
                                )

/** @def K_ARCH_BITS_EX
 * Determin the architure byte width of the specified architecture.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_BITS_EX(arch)    ( ((arch) & K_ARCH_BIT_MASK) >> K_ARCH_BIT_SHIFT )

/** @def K_ARCH_BYTES_EX
 * Determin the architure byte width of the specified architecture.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_BYTES_EX(arch)   ( ((arch) & K_ARCH_BIT_MASK) >> K_ARCH_BYTE_SHIFT )

/** @def K_ARCH_ENDIAN_EX
 * Determin the K_ENDIAN value for the specified architecture.
 * @param   arch            The K_ARCH_* define to examin.
 */
#define K_ARCH_ENDIAN_EX(arch)  ( ((arch) & K_ARCH_END_MASK) >> K_ARCH_END_SHIFT )

/** @def K_ARCH_BITS
 * Determin the target architure bit width.
 */
#define K_ARCH_BITS             K_ARCH_BITS_EX(K_ARCH)

/** @def K_ARCH_BYTES
 * Determin the target architure byte width.
 */
#define K_ARCH_BYTES            K_ARCH_BYTES_EX(K_ARCH)

/** @def K_ARCH_ENDIAN
 * Determin the target K_ENDIAN value.
 */
#define K_ARCH_ENDIAN           K_ARCH_ENDIAN_EX(K_ARCH)



/** @name Endianness Identifiers.
 * These are the value that the K_ENDIAN \#define can take.
 * @{ */
#define K_ENDIAN_LITTLE         1       /**< Little-endian. */
#define K_ENDIAN_BIG            2       /**< Big-endian. */
#define K_ENDIAN_BI             3       /**< Bi-endian, can be switched. Only used with K_ARCH. */
/** @} */

/** @def K_ENDIAN
 * The value of this \#define indicates the target endianness.
 *
 * @remark  It's necessary to define this (or add the necessary dection here)
 *          on bi-endian architectures.
 */
#ifndef K_ENDIAN
  /* use K_ARCH if possible. */
# if K_ARCH_END != K_ENDIAN_BI
#  define K_ENDIAN K_ARCH_ENDIAN
# else
#  error "Port Me or define K_ENDIAN."
# endif
#else
  /* validate the user defined value. */
# if K_ENDIAN != K_ENDIAN_LITTLE
  && K_ENDIAN != K_ENDIAN_BIG
#  error "K_ENDIAN must either be defined as K_ENDIAN_LITTLE or as K_ENDIAN_BIG."
# endif
#endif

/** @name Endian Conversion
 * @{ */

/** @def K_E2E_U16
 * Convert the endian of an unsigned 16-bit value. */
# define K_E2E_U16(u16)         ( (KU16) (((u16) >> 8) | ((u16) << 8)) )
/** @def K_E2E_U32
 * Convert the endian of an unsigned 32-bit value. */
# define K_E2E_U32(u32)         (   ( ((u32) & KU32_C(0xff000000)) >> 24 ) \
                                  | ( ((u32) & KU32_C(0x00ff0000)) >>  8 ) \
                                  | ( ((u32) & KU32_C(0x0000ff00)) <<  8 ) \
                                  | ( ((u32) & KU32_C(0x000000ff)) << 24 ) \
                                )
/** @def K_E2E_U64
 * Convert the endian of an unsigned 64-bit value. */
# define K_E2E_U64(u64)         (   ( ((u64) & KU64_C(0xff00000000000000)) >> 56 ) \
                                  | ( ((u64) & KU64_C(0x00ff000000000000)) >> 40 ) \
                                  | ( ((u64) & KU64_C(0x0000ff0000000000)) >> 24 ) \
                                  | ( ((u64) & KU64_C(0x000000ff00000000)) >>  8 ) \
                                  | ( ((u64) & KU64_C(0x00000000ff000000)) <<  8 ) \
                                  | ( ((u64) & KU64_C(0x0000000000ff0000)) << 24 ) \
                                  | ( ((u64) & KU64_C(0x000000000000ff00)) << 40 ) \
                                  | ( ((u64) & KU64_C(0x00000000000000ff)) << 56 ) \
                                )

/** @def K_LE2H_U16
 * Unsigned 16-bit little-endian to host endian. */
/** @def K_LE2H_U32
 * Unsigned 32-bit little-endian to host endian. */
/** @def K_LE2H_U64
 * Unsigned 64-bit little-endian to host endian. */
/** @def K_BE2H_U16
 * Unsigned 16-bit big-endian to host endian. */
/** @def K_BE2H_U32
 * Unsigned 32-bit big-endian to host endian. */
/** @def K_BE2H_U64
 * Unsigned 64-bit big-endian to host endian. */
#if K_ENDIAN == K_ENDIAN_LITTLE
# define K_LE2H_U16(u16)        ((KU16)(u16))
# define K_LE2H_U32(u32)        ((KU32)(u32))
# define K_LE2H_U64(u64)        ((KU64)(u32))
# define K_BE2H_U16(u16)        K_E2E_U16(u16)
# define K_BE2H_U32(u32)        K_E2E_U32(u32)
# define K_BE2H_U64(u64)        K_E2E_U64(u64)
#else
# define K_LE2H_U16(u16)        K_E2E_U16(u16)
# define K_LE2H_U32(u32)        K_E2E_U32(u32)
# define K_LE2H_U32(u64)        K_E2E_U64(u64)
# define K_BE2H_U16(u16)        ((KU16)(u16))
# define K_BE2H_U32(u32)        ((KU32)(u32))
# define K_BE2H_U64(u64)        ((KU64)(u32))
#endif



/** @def K_INLINE
 * How to say 'inline' in both C and C++ dialects.
 * @param   type        The return type.
 */
#ifdef __cplusplus
# if defined(__GNUC__)
#  define K_INLINE              static inline
# else
#  define K_INLINE              inline
# endif
#else
# if defined(__GNUC__)
#  define K_INLINE              static __inline__
# elif defined(_MSC_VER)
#  define K_INLINE              static _Inline
# else
#  error "Port Me"
# endif
#endif

/** @def K_EXPORT
 * What to put in front of an exported function.
 */
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
# define K_EXPORT               __declspec(dllexport)
#else
# define K_EXPORT
#endif

/** @def K_IMPORT
 * What to put in front of an imported function.
 */
#if K_OS == K_OS_OS2 || K_OS == K_OS_WINDOWS
# define K_IMPORT               __declspec(dllimport)
#else
# define K_IMPORT               extern
#endif

/** @def K_DECL_EXPORT
 * Declare an exported function.
 * @param type      The return type.
 */
#define K_DECL_EXPORT(type)     K_EXPORT type

/** @def K_DECL_IMPORT
 * Declare an import function.
 * @param type      The return type.
 */
#define K_DECL_IMPORT(type)     K_IMPORT type

/** @def K_DECL_INLINE
 * Declare an inline function.
 * @param type      The return type.
 * @remark  Don't use on (class) methods.
 */
#define K_DECL_INLINE(type)     K_INLINE type


/** Get the minimum of two values. */
#define K_MIN(a, b)             ( (a) <= (b) ? (a) : (b) )
/** Get the maximum of two values. */
#define K_MAX(a, b)             ( (a) >= (b) ? (a) : (b) )
/** Calculate the offset of a structure member. */
#define K_OFFSETOF(strct, memb) ( (KSIZE)( &((strct *)0)->memb ) )
/** Align a size_t value. */
#define K_ALIGN_Z(val, align)   ( ((val) + ((align) - 1)) & ~(KSIZE)((align) - 1) )
/** Align a void * value. */
#define K_ALIGN_P(pv, align)    ( (void *)( ((KUPTR)(pv) + ((align) - 1)) & ~(KUPTR)((align) - 1) ) )
/** Number of elements in an array. */
#define K_ELEMENTS(a)           ( sizeof(a) / sizeof((a)[0]) )
/** Checks if the specified pointer is a valid address or not. */
#define K_VALID_PTR(ptr)        ( (KUPTR)(ptr) + 0x1000U >= 0x2000U )
/** Makes a 32-bit bit mask. */
#define K_BIT32(bit)            ( KU32_C(1) << (bit))
/** Makes a 64-bit bit mask. */
#define K_BIT64(bit)            ( KU64_C(1) << (bit))
/** Shuts up unused parameter and unused variable warnings. */
#define K_NOREF(var)            ( (void)(var) )


/** @name Parameter validation macros
 * @{ */

/** Return/Crash validation of a string argument. */
#define K_VALIDATE_STRING(str) \
    do { \
        if (!K_VALID_PTR(str)) \
            return KERR_INVALID_POINTER; \
        kHlpStrLen(str); \
    } while (0)

/** Return/Crash validation of an optional string argument. */
#define K_VALIDATE_OPTIONAL_STRING(str) \
    do { \
        if (str) \
            K_VALIDATE_STRING(str); \
    } while (0)

/** Return/Crash validation of an output buffer. */
#define K_VALIDATE_BUFFER(buf, cb) \
    do { \
        if (!K_VALID_PTR(buf)) \
            return KERR_INVALID_POINTER; \
        if ((cb) != 0) \
        { \
            KU8             __b; \
            KU8 volatile   *__pb = (KU8 volatile *)(buf); \
            KSIZE           __cbPage1 = 0x1000 - ((KUPTR)(__pb) & 0xfff); /* ASSUMES page size! */ \
            __b = *__pb; *__pb = 0xff; *__pb = __b; \
            if ((cb) > __cbPage1) \
            { \
                KSIZE __cb = (cb) - __cbPage1; \
                __pb -= __cbPage1; \
                for (;;) \
                { \
                    __b = *__pb; *__pb = 0xff; *__pb = __b; \
                    if (__cb < 0x1000) \
                        break; \
                    __pb += 0x1000; \
                    __cb -= 0x1000; \
                } \
            } \
        } \
        else \
            return KERR_INVALID_PARAMETER; \
    } while (0)

/** Return/Crash validation of an optional output buffer. */
#define K_VALIDATE_OPTIONAL_BUFFER(buf, cb) \
    do { \
        if ((buf) && (cb) != 0) \
            K_VALIDATE_BUFFER(buf, cb); \
    } while (0)

/** Return validation of an enum argument. */
#define K_VALIDATE_ENUM(arg, enumname) \
    do { \
        if ((arg) <= enumname##_INVALID || (arg) >= enumname##_END) \
            return KERR_INVALID_PARAMETER; \
    } while (0)

/** Return validation of a flags argument. */
#define K_VALIDATE_FLAGS(arg, AllowedMask) \
    do { \
        if ((arg) & ~(AllowedMask)) \
            return KERR_INVALID_PARAMETER; \
    } while (0)

/** @} */

/** @def NULL
 * The nil pointer value. */
#ifndef NULL
# ifdef __cplusplus
#  define NULL          0
# else
#  define NULL          ((void *)0)
# endif
#endif

/** @} */

#endif


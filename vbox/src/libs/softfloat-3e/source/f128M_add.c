
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3e, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014 The Regents of the University of California.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice,
    this list of conditions, and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions, and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 3. Neither the name of the University nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS "AS IS", AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ARE
DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#include <stdbool.h>
#include <stdint.h>
#include "platform.h"
#include "internals.h"
#include "softfloat.h"

#ifdef SOFTFLOAT_FAST_INT64

void
 f128M_add( const float128_t *aPtr, const float128_t *bPtr, float128_t *zPtr )
{
    const uint64_t *aWPtr, *bWPtr;
    uint_fast64_t uiA64, uiA0;
    bool signA;
    uint_fast64_t uiB64, uiB0;
    bool signB;
#if ! defined INLINE_LEVEL || (INLINE_LEVEL < 2)
    float128_t
        (*magsFuncPtr)(
            uint_fast64_t, uint_fast64_t, uint_fast64_t, uint_fast64_t, bool );
#endif

    aWPtr = (const uint64_t *) aPtr;
    bWPtr = (const uint64_t *) bPtr;
    uiA64 = aWPtr[indexWord( 2, 1 )];
    uiA0  = aWPtr[indexWord( 2, 0 )];
    signA = signF128UI64( uiA64 );
    uiB64 = bWPtr[indexWord( 2, 1 )];
    uiB0  = bWPtr[indexWord( 2, 0 )];
    signB = signF128UI64( uiB64 );
#if defined INLINE_LEVEL && (2 <= INLINE_LEVEL)
    if ( signA == signB ) {
        *zPtr = softfloat_addMagsF128( uiA64, uiA0, uiB64, uiB0, signA );
    } else {
        *zPtr = softfloat_subMagsF128( uiA64, uiA0, uiB64, uiB0, signA );
    }
#else
    magsFuncPtr =
        (signA == signB) ? softfloat_addMagsF128 : softfloat_subMagsF128;
    *zPtr = (*magsFuncPtr)( uiA64, uiA0, uiB64, uiB0, signA );
#endif

}

#else

void
 f128M_add( const float128_t *aPtr, const float128_t *bPtr, float128_t *zPtr )
{

    softfloat_addF128M(
        (const uint32_t *) aPtr,
        (const uint32_t *) bPtr,
        (uint32_t *) zPtr,
        false
    );

}

#endif


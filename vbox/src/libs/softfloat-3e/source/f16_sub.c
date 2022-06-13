
/*============================================================================

This C source file is part of the SoftFloat IEEE Floating-Point Arithmetic
Package, Release 3e, by John R. Hauser.

Copyright 2011, 2012, 2013, 2014, 2015, 2016 The Regents of the University of
California.  All rights reserved.

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

float16_t f16_sub( float16_t a, float16_t b )
{
    union ui16_f16 uA;
    uint_fast16_t uiA;
    union ui16_f16 uB;
    uint_fast16_t uiB;
#if ! defined INLINE_LEVEL || (INLINE_LEVEL < 1)
    float16_t (*magsFuncPtr)( uint_fast16_t, uint_fast16_t );
#endif

    uA.f = a;
    uiA = uA.ui;
    uB.f = b;
    uiB = uB.ui;
#if defined INLINE_LEVEL && (1 <= INLINE_LEVEL)
    if ( signF16UI( uiA ^ uiB ) ) {
        return softfloat_addMagsF16( uiA, uiB );
    } else {
        return softfloat_subMagsF16( uiA, uiB );
    }
#else
    magsFuncPtr =
        signF16UI( uiA ^ uiB ) ? softfloat_addMagsF16 : softfloat_subMagsF16;
    return (*magsFuncPtr)( uiA, uiB );
#endif

}


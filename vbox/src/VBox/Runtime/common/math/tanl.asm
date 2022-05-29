; $Id: tanl.asm 5999 2007-12-07 15:05:06Z vboxsync $
;; @file
; innotek Portable Runtime - No-CRT tanl - AMD64 & X86.
;

;
; Copyright (C) 2006-2007 innotek GmbH
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;
; The contents of this file may alternatively be used under the terms
; of the Common Development and Distribution License Version 1.0
; (CDDL) only, as it comes in the "COPYING.CDDL" file of the
; VirtualBox OSE distribution, in which case the provisions of the
; CDDL are applicable instead of those of the GPL.
;
; You may elect to license modified versions of this file under the
; terms and conditions of either the GPL or the CDDL or both.
;

%include "iprt/asmdefs.mac"

BEGINCODE

%ifdef RT_ARCH_AMD64
 %define _SP rsp
 %define _BP rbp
 %define _S  8
%else
 %define _SP esp
 %define _BP ebp
 %define _S  4
%endif

;;
; Compute the sine of lrd
; @returns st(0)
; @param    lrd     [_SP + _S*2]
BEGINPROC RT_NOCRT(tanl)
    push    _BP
    mov     _BP, _SP
    sub     _SP, 10h

    fld     tword [_BP + _S*2]
    fptan
    fnstsw  ax
    test    ah, 04h                     ; check for C2
    jz      .done

    fldpi
    fadd    st0
    fxch    st1
.again:
    fprem1
    fnstsw  ax
    test    ah, 04h
    jnz     .again
    fstp    st1
    fptan

.done:
    fstp    st0
    leave
    ret
ENDPROC   RT_NOCRT(tanl)


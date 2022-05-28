; $Id: sinl.asm 1  vboxsync $
;; @file
; InnoTek Portable Runtime - No-CRT sinl - AMD64 & X86.
;

;
; Copyright (C) 2006 InnoTek Systemberatung GmbH
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License as published by the Free Software Foundation,
; in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
; distribution. VirtualBox OSE is distributed in the hope that it will
; be useful, but WITHOUT ANY WARRANTY of any kind.
;
; If you received this file as part of a commercial VirtualBox
; distribution, then only the terms of your commercial VirtualBox
; license agreement apply instead of the previous paragraph.
;


%include "iprt/asmdefs.mac"

BEGINCODE

%ifdef __AMD64__
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
BEGINPROC RT_NOCRT(sinl)
    push    _BP
    mov     _BP, _SP
    sub     _SP, 10h

    fld     tword [_BP + _S*2]
    fsin
    fnstsw  ax
    test    ah, 04h
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
    fsin

.done:
    leave
    ret
ENDPROC   RT_NOCRT(sinl)


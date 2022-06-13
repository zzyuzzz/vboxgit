; $Id: ASMAtomicUoDecU32.asm 92799 2021-12-08 00:47:27Z vboxsync $
;; @file
; IPRT - ASMAtomicUoDecU32().
;

;
; Copyright (C) 2014-2020 Oracle Corporation
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

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"

BEGINCODE

;;
; Atomically decrement an unsigned 32-bit value, unordered.
;
; @param    pu32     x86:esp+4   gcc:rdi  msc:rcx

; @returns  the new decremented value.
;
RT_BEGINPROC ASMAtomicUoDecU32
        mov     eax, -1
%ifdef RT_ARCH_AMD64
 %ifdef ASM_CALL64_MSC
        xadd    [rcx], eax
 %else
        xadd    [rdi], eax
 %endif
%elifdef RT_ARCH_X86
        mov     ecx, [esp + 04h]
        xadd    [ecx], eax
%endif
        dec     eax
        ret
ENDPROC ASMAtomicUoDecU32


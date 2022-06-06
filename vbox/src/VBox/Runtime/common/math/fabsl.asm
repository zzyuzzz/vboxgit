; $Id: fabsl.asm 56290 2015-06-09 14:01:31Z vboxsync $
;; @file
; IPRT - No-CRT fabsl - AMD64 & X86.
;

;
; Copyright (C) 2006-2015 Oracle Corporation
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

;;
; Compute the absolute value of lrd (|lrd|).
; @returns st(0)
; @param    lrd     [xSP + xCB*2]
BEGINPROC RT_NOCRT(fabsl)
    push    xBP
    mov     xBP, xSP

    fld     tword [xBP + xCB*2]
    fabs

.done:
    leave
    ret
ENDPROC   RT_NOCRT(fabsl)


; $Id: bs3-cmn-SwitchToRing1.asm 60000 2016-03-11 19:12:05Z vboxsync $
;; @file
; BS3Kit - Bs3SwitchToRing1
;

;
; Copyright (C) 2007-2016 Oracle Corporation
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

;*********************************************************************************************************************************
;*  Header Files                                                                                                                 *
;*********************************************************************************************************************************
%include "bs3kit-template-header.mac"


;*********************************************************************************************************************************
;*  External Symbols                                                                                                             *
;*********************************************************************************************************************************
BS3_EXTERN_CMN Bs3SwitchToRingX
TMPL_BEGIN_TEXT


;;
; @cproto   BS3_DECL(void) Bs3SwitchToRing1(void);
;
; @remarks  Does not require 20h of parameter scratch space in 64-bit mode.
;
BS3_PROC_BEGIN_CMN Bs3SwitchToRing1
        BS3_ONLY_64BIT_STMT sub rsp, 18h
        push    1
        BS3_CALL Bs3SwitchToRingX, 1
        add     xSP, xCB BS3_ONLY_64BIT(+ 18h)
        ret
BS3_PROC_END_CMN   Bs3SwitchToRing1


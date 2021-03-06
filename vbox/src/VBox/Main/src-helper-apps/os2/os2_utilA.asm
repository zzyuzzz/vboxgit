; $Id: os2_utilA.asm 93116 2022-01-02 04:58:13Z vboxsync $
;; @file
; Os2UtilA - Watcom assembly file that defines the stack.
;

;
; Copyright (C) 2022 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;

DGROUP  group _NULL,CONST,CONST2,STRINGS,_DATA,_BSS,STACK

_NULL   segment para public 'BEGDATA'
        dw      10 dup(0)
_NULL   ends

CONST   segment word public 'DATA'
CONST   ends

CONST2  segment word public 'DATA'
CONST2  ends

STRINGS segment word public 'DATA'
STRINGS ends

_DATA   segment word public 'DATA'
_DATA   ends

_BSS    segment word public 'BSS'
_BSS    ends

STACK   segment para stack 'STACK'
        db      1000h dup(?)
STACK   ends

        end


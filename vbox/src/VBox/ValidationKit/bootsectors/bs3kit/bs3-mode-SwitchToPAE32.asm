; $Id: bs3-mode-SwitchToPAE32.asm 59244 2016-01-03 21:24:20Z vboxsync $
;; @file
; BS3Kit - Bs3SwitchToPAE32
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

%include "bs3kit-template-header.mac"


;;
; Switch to PAE paged protected mode from any other mode.
;
; @cproto   BS3_DECL(void) Bs3SwitchToPAE32(void);
;
; @uses     Nothing (except high 32-bit register parts), upper part of ESP is
;           cleared if caller is in 16-bit mode.
;
; @remarks  Obviously returns to 32-bit mode, even if the caller was
;           in 16-bit or 64-bit mode.  It doesn't not preserve the callers
;           ring, but instead changes to ring-0.
;
BS3_PROC_BEGIN_MODE Bs3SwitchToPAE32
%ifdef TMPL_PAE32
        ret

%else
        ;
        ; Switch to real mode.
        ;
 %if TMPL_BITS != 32
  %if TMPL_BITS > 32
        shl     xPRE [xSP + xCB], 32    ; Adjust the return address from 64-bit to 32-bit.
        add     rsp, xCB - 4
  %else
        push    word 0                  ; Reserve space to expand the return address.
  %endif
        ; Must be in 16-bit segment when calling Bs3SwitchTo16Bit.
        jmp     .sixteen_bit_segment
BS3_BEGIN_TEXT16
        BS3_SET_BITS TMPL_BITS
.sixteen_bit_segment:
 %endif

        ;
        ; Switch to real mode.
        ;
        extern  TMPL_NM(Bs3SwitchToRM)
        call    TMPL_NM(Bs3SwitchToRM)
        BS3_SET_BITS 16

        push    eax
        push    ecx
        pushfd

        ;
        ; Make sure both PAE and PSE are enabled (requires pentium pro).
        ;
        mov     eax, cr4
        mov     ecx, eax
        or      eax, X86_CR4_PAE | X86_CR4_PSE
        cmp     eax, ecx
        je      .cr4_is_fine
        mov     cr4, eax
.cr4_is_fine:

        ;
        ; Get the page directory (returned in eax).
        ; Will lazy init page tables (in 16-bit prot mode).
        ;
        extern NAME(Bs3PagingGetRootForPAE32_rm)
        call   NAME(Bs3PagingGetRootForPAE32_rm)

        cli
        mov     cr3, eax

        ;
        ; Load the GDT and enable PE32.
        ;
BS3_EXTERN_SYSTEM16 Bs3Lgdt_Gdt
BS3_BEGIN_TEXT16
        mov     ax, BS3SYSTEM16
        mov     ds, ax
        lgdt    [Bs3Lgdt_Gdt]

        mov     eax, cr0
        or      eax, X86_CR0_PE | X86_CR0_PG
        mov     cr0, eax
        jmp     BS3_SEL_R0_CS32:dword .thirty_two_bit wrt FLAT
BS3_BEGIN_TEXT32
.thirty_two_bit:

        ;
        ; Convert the (now) real mode stack pointer to 32-bit flat.
        ;
        xor     eax, eax
        mov     ax, ss
        shl     eax, 4
        and     esp, 0ffffh
        add     esp, eax

        mov     ax, BS3_SEL_R0_SS32
        mov     ss, ax

        ;
        ; Call rountine for doing mode specific setups.
        ;
        extern  NAME(Bs3EnteredMode_pae32)
        call    NAME(Bs3EnteredMode_pae32)

        ;
        ; Restore ecx, eax and flags (IF).
        ;
 %if TMPL_BITS < 32
        movzx   eax, word [esp + 12 + 2] ; Load return address.
        add     eax, BS3_ADDR_BS3TEXT16  ; Convert it to a flat address.
        mov     [esp + 12], eax          ; Store it in the place right for 32-bit returns.
 %endif
        popfd
        pop     ecx
        pop     eax
        ret

 %if TMPL_BITS != 32
TMPL_BEGIN_TEXT
 %endif
%endif
BS3_PROC_END_MODE   Bs3SwitchToPAE32


; $Id: CPUMRCPatchHlp.asm 54763 2015-03-15 03:15:58Z vboxsync $
;; @file
; CPUM - Patch Helpers.
;

; Copyright (C) 2015 Oracle Corporation
;
; This file is part of VirtualBox Open Source Edition (OSE), as
; available from http://www.virtualbox.org. This file is free software;
; you can redistribute it and/or modify it under the terms of the GNU
; General Public License (GPL) as published by the Free Software
; Foundation, in version 2 as it comes in the "COPYING" file of the
; VirtualBox OSE distribution. VirtualBox OSE is distributed in the
; hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
;


;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VBox/vmm/cpum.mac"
%include "CPUMInternal.mac"
%include "VBox/vmm/vm.mac"
%include "VMMRC.mac"


;*******************************************************************************
;*  External Symbols                                                           *
;*******************************************************************************
extern IMPNAME(g_VM)


BEGIN_PATCH_HLP_SEG

;;
; Helper for PATMCpuidReplacement. 
; 
; We have at most 32 bytes of stack to play with, .
;
; @input    eax, ecx(, edx, ebx)
; @output   eax, ebx, ecx, ebx
;
; @uses     eflags (caller saves them)
;
BEGINPROC_EXPORTED CPUMPatchHlpCpuId
    ; Save the registers we use for passthru and sub-leaf matching (eax is not used).
    push    edx
    push    ecx
    push    ebx

    ; Use edi as VM pointer.
    push    edi
    mov     edi, IMP_SEG(ss, g_VM)

%define CPUMCPUIDLEAF_SIZE_LOG2 5       ; ASSUMES CPUMCPUIDLEAF_size == 32

    ;
    ; Perform a binary search looking for leaf with the EAX value.
    ;             
    mov     edx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.cCpuIdLeaves]   
    mov     ecx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.paCpuIdLeavesRC]
    test    edx, edx
    jz      cpuid_unknown
    shl     edx, CPUMCPUIDLEAF_SIZE_LOG2
    add     edx, ecx                    ; edx = end (exclusive); ecx = start.

cpuid_lookup_leaf:
    ; Find the middle element
    mov     ebx, edx
cpuid_lookup_leaf_ebx_loeaded:
    sub     ebx, ecx
    shr     ebx, 1 + CPUMCPUIDLEAF_SIZE_LOG2
    shl     ebx, CPUMCPUIDLEAF_SIZE_LOG2
    add     ebx, ecx                    ; ebx = middle element.

    ; Compare.
    cmp     eax, [ss:ebx + CPUMCPUIDLEAF.uLeaf]
    jae     cpuid_lookup_split_up

    ; The leaf is before ebx.
cpuid_lookup_split_down:
    cmp     ecx, ebx                    ; start == middle? if so, we failed.
    mov     edx, ebx                    ; end = middle;
    jne     cpuid_lookup_leaf_ebx_loeaded
    jmp     cpuid_unknown

    ; The leaf is at or after ebx.
cpuid_lookup_split_up:
    je      cpuid_match_eax
    lea     ecx, [ebx + CPUMCPUIDLEAF_size] ; start = middle + 1
    cmp     ecx, edx                    ; middle + 1 == start? if so, we failed.
    jne     cpuid_lookup_leaf
    jmp     cpuid_unknown

    ;
    ; We've to a matching leaf, does the sub-leaf match too?
    ;
cpuid_match_eax:    
    mov     ecx, [esp + 4]
    and     ecx, [ss:ebx + CPUMCPUIDLEAF.fSubLeafMask]
    cmp     ecx, [ss:ebx + CPUMCPUIDLEAF.uSubLeaf]
    je      cpuid_fetch
    ja      cpuid_lookup_subleaf_forwards

    ;
    ; Search backwards.
    ;
cpuid_lookup_subleaf_backwards:
    mov     edx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.paCpuIdLeavesRC] ; edx = first leaf

cpuid_lookup_subleaf_backwards_loop:
    cmp     ebx, edx                    ; Is there a leaf before the current? 
    jbe     cpuid_subleaf_not_found     ; If not we're out of luck.
    cmp     eax, [ss:ebx - CPUMCPUIDLEAF_size + CPUMCPUIDLEAF.uLeaf]
    jne     cpuid_subleaf_not_found     ; If the leaf before us does not have the same leaf number, we failed.
    sub     ebx, CPUMCPUIDLEAF_size
    cmp     ecx, [ss:ebx + CPUMCPUIDLEAF.uSubLeaf]
    je      cpuid_fetch                 ; If the subleaf matches, we're good!.
    jb      cpuid_lookup_subleaf_backwards_loop ; Still hope if the subleaf we're seeking is smaller.
    jmp     cpuid_subleaf_not_found     ; Too bad.

    ;
    ; Search forward until we've got a matching sub-leaf (or not).  
    ;
cpuid_lookup_subleaf_forwards:
    ; Calculate the last leaf address.
    mov     edx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.cCpuIdLeaves]   
    dec     edx
    shl     edx, CPUMCPUIDLEAF_SIZE_LOG2
    add     edx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.paCpuIdLeavesRC] ; edx = last leaf (inclusive)

cpuid_subleaf_lookup:
    cmp     ebx, edx
    jae     cpuid_subleaf_not_found
    cmp     eax, [ss:ebx + CPUMCPUIDLEAF_size + CPUMCPUIDLEAF.uLeaf]
    jne     cpuid_subleaf_not_found
    add     ebx, CPUMCPUIDLEAF_size
    cmp     ecx, [ss:ebx + CPUMCPUIDLEAF.uSubLeaf]    
    ja      cpuid_subleaf_lookup
    je      cpuid_fetch
    
    ;
    ; Out of range sub-leaves aren't quite as easy and pretty as we emulate them
    ; here, but we do an adequate job.
    ;    
cpuid_subleaf_not_found:
    xor     ecx, ecx
    test    dword [ss:ebx + CPUMCPUIDLEAF.fFlags], CPUMCPUIDLEAF_F_INTEL_TOPOLOGY_SUBLEAVES
    jz      cpuid_load_zeros_except_ecx
    mov     ecx, [esp + 4]
    and     ecx, 0ffh
cpuid_load_zeros_except_ecx:
    xor     edx, edx
    xor     eax, eax
    xor     ebx, ebx
    jmp     cpuid_done

    ;
    ; Different CPUs have different ways of dealing with unknown CPUID leaves.
    ;
cpuid_unknown:
    mov     ebx, IMP_SEG(ss, g_VM)
    mov     dword [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.enmUnknownCpuIdMethod], CPUMUNKNOWNCPUID_PASSTHRU
    je      cpuid_unknown_passthru
    ; Load the default cpuid leaf.
cpuid_unknown_def_leaf:
    mov     edx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.DefCpuId + CPUMCPUID.uEdx]
    mov     ecx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.DefCpuId + CPUMCPUID.uEcx]
    mov     eax, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.DefCpuId + CPUMCPUID.uEax]
    mov     ebx, [ss:edi + VM.cpum + CPUM.GuestInfo + CPUMINFO.DefCpuId + CPUMCPUID.uEbx]
    jmp     cpuid_done
    ; Pass thru the input values unmodified (eax is still virgin).
cpuid_unknown_passthru:
    mov     edx, [esp + 8]
    mov     ecx, [esp + 4]
    mov     ebx, [esp]
    jmp     cpuid_done

    ;
    ; Normal return.
    ;
cpuid_fetch:
    mov     edx, [ss:ebx + CPUMCPUIDLEAF.uEdx]
    mov     ecx, [ss:ebx + CPUMCPUIDLEAF.uEcx]
    mov     eax, [ss:ebx + CPUMCPUIDLEAF.uEax]
    mov     ebx, [ss:ebx + CPUMCPUIDLEAF.uEbx]
             
cpuid_done:
    pop     edi
    add     esp, 12
    ret
ENDPROC CPUMPatchHlpCpuId


; $Id: 32BitToPAE.asm 2988 2007-06-01 17:36:09Z vboxsync $
;; @file
; VMM - World Switchers, 32-Bit to 32-Bit.
;

;
; Copyright (C) 2006-2007 innotek GmbH
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

;*******************************************************************************
;*   Defined Constants And Macros                                              *
;*******************************************************************************
%define SWITCHER_TYPE               VMMSWITCHER_32_TO_PAE
%define SWITCHER_DESCRIPTION        "32-bit to/from PAE"
%define NAME_OVERLOAD(name)         vmmR3Switcher32BitToPAE_ %+ name
%define SWITCHER_FIX_INTER_CR3_HC   FIX_INTER_32BIT_CR3
%define SWITCHER_FIX_INTER_CR3_GC   FIX_INTER_PAE_CR3
%define SWITCHER_FIX_HYPER_CR3      FIX_HYPER_PAE_CR3
%define NEED_PAE_ON_32BIT_HOST  1

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "VBox/asmdefs.mac"
%include "VMMSwitcher/PAEand32Bit.mac"


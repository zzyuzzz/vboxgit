; $Id: assertA-r0drv-nt.asm 11899 2008-09-01 09:03:50Z vboxsync $
;; @file
;  IPRT - Assertion Workers, Ring-0 Drivers, NT.
;

;
; Copyright (c) 1999-2007 knut st. osmundsen <bird-src-spam@anduin.net>
;
; Permission is hereby granted, free of charge, to any person
; obtaining a copy of this software and associated documentation
; files (the "Software"), to deal in the Software without
; restriction, including without limitation the rights to use,
; copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the
; Software is furnished to do so, subject to the following
; conditions:
;
; The above copyright notice and this permission notice shall be
; included in all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
; EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
; OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
; NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
; HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
; WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
; FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
; OTHER DEALINGS IN THE SOFTWARE.
;

;*******************************************************************************
;* Header Files                                                                *
;*******************************************************************************
%include "iprt/asmdefs.mac"

BEGINCODE
%ifdef RT_ARCH_AMD64
%define _DbgPrint DbgPrint
%endif
extern _DbgPrint

;;
; Kind of alias for DbgPrint
BEGINPROC AssertMsg2
        jmp     _DbgPrint
ENDPROC AssertMsg2


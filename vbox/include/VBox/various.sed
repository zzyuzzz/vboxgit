# $Id: various.sed 69181 2017-10-23 18:13:56Z vboxsync $
## @file
# Converts some C header elements into nasm/yasm syntax.
#
# This is used by 'incs' in /Maintenance.kmk (/Makefile.kmk).
#

#
# Copyright (C) 2006-2017 Oracle Corporation
#
# This file is part of VirtualBox Open Source Edition (OSE), as
# available from http://www.virtualbox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualBox OSE distribution. VirtualBox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#
# The contents of this file may alternatively be used under the terms
# of the Common Development and Distribution License Version 1.0
# (CDDL) only, as it comes in the "COPYING.CDDL" file of the
# VirtualBox OSE distribution, in which case the provisions of the
# CDDL are applicable instead of those of the GPL.
#
# You may elect to license modified versions of this file under the
# terms and conditions of either the GPL or the CDDL or both.
#


# Check for markers (typically in comments).
/ASM-INC/basm-inc
/ASM-NOINC/basm-noinc

# Newline escapes.
:check-newline-escape
/\\$/!bno-more-newline-escapes
N
b check-newline-escape
:no-more-newline-escapes

# Strip comments and trailing space.
s/[[:space:]][[:space:]]*\/\*.*$//g
s/[[:space:]][[:space:]]*\/\/.*$//g
s/[[:space:]][[:space:]]*$//g

# Try identify the statement.
/#[[:space:]]*define[[:space:]]/bdefine
/#[[:space:]]*ifdef[[:space:]]/bifdef
/#[[:space:]]*ifndef[[:space:]]/bifndef
/#[[:space:]]*if[[:space:]]/bif
/#[[:space:]]*elif[[:space:]]/belif
/#[[:space:]]*else$/belse
/#[[:space:]]*endif$/bendif

# Not recognized, drop it.
:asm-noinc
d
b end

#
# Defines needs some extra massaging to work in yasm.
# Things like trailing type indicators ('U', 'ULL' ++) does not go down well.
#
:define
/\$/d
s/#\([[:space:]]*\)define/\1%define/

s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)U$/\1/
s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)U\([[:space:]]*\))$/\1\2)/
s/\([[:space:]][0-9][0-9]*\)U[[:space:]]*$/\1/
s/\([[:space:]][0-9][0-9]*\)U\([[:space:]]*\))$/\1\2)/

s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)UL$/\1/
s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)UL\([[:space:]]*\))$/\1\2)/
s/\([[:space:]][0-9][0-9]*\)UL[[:space:]]*$/\1/
s/\([[:space:]][0-9][0-9]*\)UL\([[:space:]]*\))$/\1\2)/

s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)ULL$/\1/
s/\([[:space:]]0[xX][0-9a-fA-F][0-9a-fA-F]*\)ULL\([[:space:]]*\))$/\1\2)/
s/\([[:space:]][0-9][0-9]*\)ULL[[:space:]]*$/\1/
s/\([[:space:]][0-9][0-9]*\)ULL\([[:space:]]*\))$/\1\2)/

s/UINT64_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT64_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/
s/UINT32_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT32_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/
s/UINT16_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT16_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/
s/UINT8_C([[:space:]]*\(0[xX][0-9a-fA-F][0-9a-fA-F]*\)[[:space:]]*)/\1/
s/UINT8_C([[:space:]]*\([0-9][0-9]*\)[[:space:]]*)/\1/

b end

#
# Conditional statements, 1:1.
#
:ifdef
s/#\([[:space:]]*\)ifdef/\1%ifdef/
b end

:ifndef
s/#\([[:space:]]*\)ifndef/\1%ifndef/
b end

:if
s/#\([[:space:]]*\)if/\1%if/
b end

:elif
s/#\([[:space:]]*\)elif/\1%elif/
b end

:else
s/#\([[:space:]]*\)else.*$/\1%else/
b end

:endif
s/#\([[:space:]]*\)endif.*$/\1%endif/
b end

#
# Assembly statement... may need adjusting when used.
#
:asm-inc
b end

:end


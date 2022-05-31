/*
 * Unicode definitions
 *
 * Copyright 2000 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Sun LGPL Disclaimer: For the avoidance of doubt, except that if any license choice
 * other than GPL or LGPL is available it will apply instead, Sun elects to use only
 * the Lesser General Public License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the language indicating
 * that LGPLv2 or any later version may be used, or where a choice of which version
 * of the LGPL is applied is otherwise unspecified.
 */

#ifndef __WINE_WCTYPE_H
#define __WINE_WCTYPE_H

#include <crtdefs.h>

#include <pshpack8.h>

/* ASCII char classification table - binary compatible */
#define _UPPER        0x0001  /* C1_UPPER */
#define _LOWER        0x0002  /* C1_LOWER */
#define _DIGIT        0x0004  /* C1_DIGIT */
#define _SPACE        0x0008  /* C1_SPACE */
#define _PUNCT        0x0010  /* C1_PUNCT */
#define _CONTROL      0x0020  /* C1_CNTRL */
#define _BLANK        0x0040  /* C1_BLANK */
#define _HEX          0x0080  /* C1_XDIGIT */
#define _LEADBYTE     0x8000
#define _ALPHA       (0x0100|_UPPER|_LOWER)  /* (C1_ALPHA|_UPPER|_LOWER) */

#ifndef WEOF
#define WEOF        (wint_t)(0xFFFF)
#endif

/* FIXME: there's something to do with __p__pctype and __p__pwctype */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED
int __cdecl is_wctype(wint_t,wctype_t);
int __cdecl isleadbyte(int);
int __cdecl iswalnum(wint_t);
int __cdecl iswalpha(wint_t);
int __cdecl iswascii(wint_t);
int __cdecl iswcntrl(wint_t);
int __cdecl iswctype(wint_t,wctype_t);
int __cdecl iswdigit(wint_t);
int __cdecl iswgraph(wint_t);
int __cdecl iswlower(wint_t);
int __cdecl iswprint(wint_t);
int __cdecl iswpunct(wint_t);
int __cdecl iswspace(wint_t);
int __cdecl iswupper(wint_t);
int __cdecl iswxdigit(wint_t);
wchar_t __cdecl towlower(wchar_t);
wchar_t __cdecl towupper(wchar_t);
#endif /* _WCTYPE_DEFINED */

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_WCTYPE_H */

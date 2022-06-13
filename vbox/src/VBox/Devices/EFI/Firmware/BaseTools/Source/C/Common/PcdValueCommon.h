/** @file
Header file for PcdValue structure definition.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PCD_VALUE_COMMON_H
#define _PCD_VALUE_COMMON_H

#include <Common/UefiBaseTypes.h>
#include <Common/UefiInternalFormRepresentation.h>

#define __FIELD_SIZE(TYPE, Field) (sizeof((TYPE *)0)->Field)
#define __ARRAY_ELEMENT_SIZE(TYPE, Field) (sizeof((TYPE *)0)->Field[0])
#define __OFFSET_OF(TYPE, Field) ((UINT32)(size_t) &(((TYPE *)0)->Field))
#define __FLEXIBLE_SIZE(Size, TYPE, Field, MaxIndex)   if (__FIELD_SIZE(TYPE, Field) == 0) Size = MAX((__OFFSET_OF(TYPE, Field) + __ARRAY_ELEMENT_SIZE(TYPE, Field) * (MaxIndex)), Size)
#define __ARRAY_SIZE(Array) (sizeof(Array)/sizeof(Array[0]))

#if defined(_MSC_EXTENSIONS)
#define __STATIC_ASSERT static_assert
#else
#define __STATIC_ASSERT _Static_assert
#endif

VOID
PcdEntryPoint (
  VOID
  )
/*++

Routine Description:

  Main function updates PCD values. It is auto generated by Build

Arguments:

  None

Returns:
  None
--*/
;

int
PcdValueMain (
  int   argc,
  char  *argv[]
  )
/*++

Routine Description:

  Main function updates PCD values.

Arguments:

  argc            Number of command line parameters.
  argv            Array of pointers to parameter strings.

Returns:
  EXIT_SUCCESS
--*/
;

VOID
__PcdSet (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT64  Value
  )
/*++

Routine Description:

  Get PCD value

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String

Returns:

  PCD value
--*/
;

VOID
__PcdSet (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT64  Value
  )
/*++

Routine Description:

  Set PCD value

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String
  Value                 PCD value to be set

Returns:

  None
--*/
;

VOID *
__PcdGetPtr (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT32  *Size
  )
/*++

Routine Description:

  Get PCD value buffer

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String
  Size                  Size of PCD value buffer

Returns:

  PCD value buffer
--*/
;

VOID
__PcdSetPtr (
  CHAR8   *SkuName             OPTIONAL,
  CHAR8   *DefaultValueName    OPTIONAL,
  CHAR8   *TokenSpaceGuidName,
  CHAR8   *TokenName,
  UINT32  Size,
  UINT8   *Value
  )
/*++

Routine Description:

  Set PCD value buffer

Arguments:

  SkuName               SkuName String
  DefaultValueName      DefaultValueName String
  TokenSpaceGuidName    TokenSpaceGuidName String
  TokenName             TokenName String
  Size                  Size of PCD value
  Value                 Pointer to the updated PCD value buffer

Returns:

  None
--*/
;

#define PcdGet(A, B, C, D)  __PcdGet(#A, #B, #C, #D)
#define PcdSet(A, B, C, D, Value)  __PcdSet(#A, #B, #C, #D, Value)
#define PcdGetPtr(A, B, C, D, Size)  __PcdGetPtr(#A, #B, #C, #D, Size)
#define PcdSetPtr(A, B, C, D, Size, Value)  __PcdSetPtr(#A, #B, #C, #D, Size, Value)

#endif

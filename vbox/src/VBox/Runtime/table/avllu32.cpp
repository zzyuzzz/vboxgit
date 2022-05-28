/* $Id: avllu32.cpp 4687 2007-09-11 09:13:32Z vboxsync $ */
/** @file
 * innotek Portable Runtime - AVL tree, uint32_t, unique keys.
 */

/*
 * Copyright (C) 2001-2006 knut st. osmundsen (bird-src-spam@anduin.net)
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef NOFILEID
static const char szFileId[] = "Id: kAVLULInt.c,v 1.4 2003/02/13 02:02:38 bird Exp $";
#endif

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/*
 * AVL configuration.
 */
#define KAVL_FN(a)                  RTAvllU32##a
#define KAVL_MAX_STACK              27  /* Up to 2^24 nodes. */
#define KAVL_EQUAL_ALLOWED          1   /* List duplicate keys! */
#define KAVLNODECORE                AVLLU32NODECORE
#define PKAVLNODECORE               PAVLLU32NODECORE
#define PPKAVLNODECORE              PPAVLLU32NODECORE
#define KAVLKEY                     AVLLU32KEY
#define PKAVLKEY                    PAVLLU32KEY
#define KAVLENUMDATA                AVLLU32ENUMDATA
#define PKAVLENUMDATA               PAVLLU32ENUMDATA
#define PKAVLCALLBACK               PAVLLU32CALLBACK


/*
 * AVL Compare macros
 */
#define KAVL_G(key1, key2)          ( (key1) >  (key2) )
#define KAVL_E(key1, key2)          ( (key1) == (key2) )
#define KAVL_NE(key1, key2)         ( (key1) != (key2) )


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/avl.h>
#include <iprt/assert.h>

/*
 * Include the code.
 */
#define SSToDS(ptr) ptr
#define KMAX RT_MAX
#define kASSERT Assert
#include "avl_Base.cpp.h"
#include "avl_Get.cpp.h"
#include "avl_GetBestFit.cpp.h"
#include "avl_RemoveBestFit.cpp.h"
#include "avl_DoWithAll.cpp.h"
#include "avl_Destroy.cpp.h"



/* $Id: VBoxPrintChar.c 28800 2010-04-27 08:22:32Z vboxsync $ */
/** @file
 * VBoxPrintChar.c - Implementation of the VBoxPrintChar() debug logging routine.
 */

/*
 * Copyright (C) 2009-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "VBoxDebugLib.h"
#include "DevEFI.h"


/**
 * Prints a char.
 * @returns 1
 * @param   ch              The char to print.
 */
size_t VBoxPrintChar(int ch)
{
    ASMOutU8(EFI_DEBUG_PORT, (uint8_t)ch);
    return 1;
}


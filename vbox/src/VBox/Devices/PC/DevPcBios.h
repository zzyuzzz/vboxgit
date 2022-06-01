/* $Id: DevPcBios.h 26728 2010-02-24 10:22:51Z vboxsync $ */
/** @file
 * DevPcBios - PC BIOS Device, header shared with the BIOS code.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef DEV_PCBIOS_H
#define DEV_PCBIOS_H

/** @def VBOX_DMI_TABLE_BASE */
#define VBOX_DMI_TABLE_BASE          0xe1000
#define VBOX_DMI_TABLE_VER           0x25
#define VBOX_DMI_TABLE_ENTR          5

/** def VBOX_DMI_TABLE_SIZE
 *
 * Must not be bigger than the minimal size of the DMI tables + 255 because
 * the length field of the the DMI end-of-table marker is 8 bits only. And
 * the size should be at least 16-byte aligned for a proper alignment of
 * the MPS table.
 */
#define VBOX_DMI_TABLE_SIZE          352

/** @def VBOX_VMI_BIOS_BASE
 *
 * Must be located between 0xC0000 and 0xDEFFF, otherwise it will not be
 * recognized as regular BIOS.
 */
#define VBOX_VMI_BIOS_BASE           0xdf000


/** @def VBOX_LANBOOT_SEG
 *
 * Should usually start right after the DMI BIOS page
 */
#define VBOX_LANBOOT_SEG             0xe200

#define VBOX_SMBIOS_MAJOR_VER        2
#define VBOX_SMBIOS_MINOR_VER        5
#define VBOX_SMBIOS_MAXSS            0xff   /* Not very accurate */

#endif

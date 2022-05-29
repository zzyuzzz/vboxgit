/* $Id: VBoxDDUDeps.cpp 8770 2008-05-12 02:04:54Z vboxsync $ */
/** @file
 * VBoxDDU - For dragging in library objects.
 */

/*
 * Copyright (C) 2007 Sun Microsystems, Inc.
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/types.h>
#ifdef VBOX_WITH_USB
# include <VBox/usblib.h>
# include <VBox/usbfilter.h>
# ifdef RT_OS_OS2
#  include <os2.h>
#  include <usbcalls.h>
# endif
#endif

/** Just a dummy global structure containing a bunch of
 * function pointers to code which is wanted in the link.
 */
PFNRT g_apfnVBoxDDUDeps[] =
{
#ifdef VBOX_WITH_USB
    (PFNRT)USBFilterInit,
    (PFNRT)USBLibHashSerial,
# ifdef RT_OS_OS2
    (PFNRT)UsbOpen,
# endif
# if (defined(RT_OS_DARWIN) && defined(VBOX_WITH_NEW_USB_CODE_ON_DARWIN)) \
  || defined(RT_OS_SOLARIS) || defined(RT_OS_WINDOWS) /* PORTME */
    (PFNRT)USBLibInit,
# endif
#endif /* VBOX_WITH_USB */
    NULL
};

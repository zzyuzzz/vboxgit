/* $Id: VBoxManageUtils.h 88079 2021-03-10 20:48:09Z vboxsync $ */
/** @file
 * VBoxManageUtils.h - Declarations for VBoxManage utility functions.
 */

/*
 * Copyright (C) 2006-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef VBOX_INCLUDED_SRC_VBoxManage_VBoxManageUtils_h
#define VBOX_INCLUDED_SRC_VBoxManage_VBoxManageUtils_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef VBOX_ONLY_DOCS
#include <VBox/com/com.h>
#include <VBox/com/ptr.h>
#include <VBox/com/VirtualBox.h>
#endif

void verifyHostNetworkInterfaceName(const ComPtr<IVirtualBox> &pVirtualBox,
                                    const char *pszTargetName,
                                    HostNetworkInterfaceType_T enmTargetType);

#endif /* !VBOX_INCLUDED_SRC_VBoxManage_VBoxManageUtils_h */

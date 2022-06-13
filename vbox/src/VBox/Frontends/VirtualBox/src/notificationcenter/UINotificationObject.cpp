/* $Id: UINotificationObject.cpp 90288 2021-07-22 14:16:13Z vboxsync $ */
/** @file
 * VBox Qt GUI - UINotificationObject class implementation.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* GUI includes: */
#include "UINotificationObject.h"


UINotificationObject::UINotificationObject()
{
}

void UINotificationObject::close()
{
    emit sigAboutToClose();
}

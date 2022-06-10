/* $Id: UITextTable.cpp 77432 2019-02-22 14:52:39Z vboxsync $ */
/** @file
 * VBox Qt GUI - UITextTable class implementation.
 */

/*
 * Copyright (C) 2012-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QAccessibleObject>
#include <QPainter>
#include <QTextLayout>
#include <QApplication>
#include <QFontMetrics>
#include <QGraphicsSceneHoverEvent>

/* GUI includes: */
#include "UITextTable.h"
#include "UIRichTextString.h"
#include "VBoxGlobal.h"

/* Other VBox includes: */
#include <iprt/assert.h>

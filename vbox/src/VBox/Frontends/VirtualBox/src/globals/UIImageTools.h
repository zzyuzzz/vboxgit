/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * Declarations of utility classes and functions for image manipulation
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __UIImageTools_h__
#define __UIImageTools_h__

/* Global includes */
#include <QImage>

QImage toGray(const QImage& image);
void dimImage(QImage& image);
void blurImage(const QImage &source, QImage &dest, int r);
void blurImageHorizontal(const QImage &source, QImage &dest, int r);
void blurImageVertical(const QImage &source, QImage &dest, int r);

#endif /* !__UIImageTools_h__ */


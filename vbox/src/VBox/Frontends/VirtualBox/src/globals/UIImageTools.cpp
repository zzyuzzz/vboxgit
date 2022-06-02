/* $Id: UIImageTools.cpp 30692 2010-07-07 09:17:35Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * Implementation of utility classes and functions for image manipulation
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

/* Local include */
#include "UIImageTools.h"

/* Todo: Think about the naming convention and if the images should be
 * processed in place or return changed copies. Make it more uniform. Add
 * asserts if the bit depth of the given image could not processed. */

QImage toGray(const QImage& image)
{
    QImage result = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < result.height(); ++y)
    {
        QRgb *pScanLine = (QRgb*)result.scanLine(y);
        for (int x = 0; x < result.width(); ++x)
        {
            const int g = qGray(pScanLine[x]);
            pScanLine[x] = qRgba(g, g, g, qAlpha(pScanLine[x]));
        }
    }
    return result;
}

void dimImage(QImage& image)
{
    /* Todo: factor out the < 32bit case, cause this can be done a lot faster
     * by just processing every second line. */
    for (int y = 0; y < image.height(); ++y)
    {
        QRgb *sl = (QRgb*)image.scanLine(y);
        if (y % 2)
        {
            if (image.depth() == 32)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    const int gray = qGray(sl[x]) / 2;
                    sl[x] = qRgba(gray, gray, gray, qAlpha(sl[x]));
                }
            }
            else
                ::memset(sl, 0, image.bytesPerLine());
        }
        else
        {
            if (image.depth() == 32)
            {
                for (int x = 0; x < image.width(); ++x)
                {
                    const int gray = (2 * qGray(sl[x])) / 3;
                    sl[x] = qRgba(gray, gray, gray, qAlpha(sl[x]));
                }
            }
        }
    }
}

void blurImage(const QImage &source, QImage &dest, int r)
{
    /* Blur in two steps. First horizontal and then vertical. Todo: search
     * for more optimization. */
    QImage tmpImage(source.size(), QImage::Format_ARGB32);
    blurImageHorizontal(source, tmpImage, r);
    blurImageVertical(tmpImage, dest, r);
}

void blurImageHorizontal(const QImage &source, QImage &dest, int r)
{
    QSize s = source.size();
    for (int y = 0; y < s.height(); ++y)
    {
        int rt = 0;
        int gt = 0;
        int bt = 0;
        int at = 0;

        /* In the horizontal case we can just use the scanline, which is
         * much faster than accessing every pixel with the QImage::pixel
         * method. Unfortunately this doesn't work in the vertical case. */
        QRgb* ssl = (QRgb*)source.scanLine(y);
        QRgb* dsl = (QRgb*)dest.scanLine(y);
        /* First process the horizontal zero line at once */
        int b = r + 1;
        for (int x1 = 0; x1 <= r; ++x1)
        {
            QRgb rgba = ssl[x1];
            rt += qRed(rgba);
            gt += qGreen(rgba);
            bt += qBlue(rgba);
            at += qAlpha(rgba);
        }
        /* Set the new weighted pixel */
        dsl[0] = qRgba(rt / b, gt / b, bt / b, at / b);

        /* Now process the rest */
        for (int x = 1; x < s.width(); ++x)
        {
            /* Subtract the pixel which fall out of our blur matrix */
            int x1 = x - r - 1;
            if (x1 >= 0)
            {
                --b; /* Adjust the weight (necessary for the border case) */
                QRgb rgba = ssl[x1];
                rt -= qRed(rgba);
                gt -= qGreen(rgba);
                bt -= qBlue(rgba);
                at -= qAlpha(rgba);
            }

            /* Add the pixel which get into our blur matrix */
            int x2 = x + r;
            if (x2 < s.width())
            {
                ++b; /* Adjust the weight (necessary for the border case) */
                QRgb rgba = ssl[x2];
                rt += qRed(rgba);
                gt += qGreen(rgba);
                bt += qBlue(rgba);
                at += qAlpha(rgba);
            }
            /* Set the new weighted pixel */
            dsl[x] = qRgba(rt / b, gt / b, bt / b, at / b);
        }
    }
}
void blurImageVertical(const QImage &source, QImage &dest, int r)
{
    QSize s = source.size();
    for (int x = 0; x < s.width(); ++x)
    {
        int rt = 0;
        int gt = 0;
        int bt = 0;
        int at = 0;

        /* First process the vertical zero line at once */
        int b = r + 1;
        for (int y1 = 0; y1 <= r; ++y1)
        {
            QRgb rgba = source.pixel(x, y1);
            rt += qRed(rgba);
            gt += qGreen(rgba);
            bt += qBlue(rgba);
            at += qAlpha(rgba);
        }
        /* Set the new weighted pixel */
        dest.setPixel(x, 0, qRgba(rt / b, gt / b, bt / b, at / b));

        /* Now process the rest */
        for (int y = 1; y < s.height(); ++y)
        {
            /* Subtract the pixel which fall out of our blur matrix */
            int y1 = y - r - 1;
            if (y1 >= 0)
            {
                --b; /* Adjust the weight (necessary for the border case) */
                QRgb rgba = source.pixel(x, y1);
                rt -= qRed(rgba);
                gt -= qGreen(rgba);
                bt -= qBlue(rgba);
                at -= qAlpha(rgba);
            }

            /* Add the pixel which get into our blur matrix */
            int y2 = y + r;
            if (y2 < s.height())
            {
                ++b; /* Adjust the weight (necessary for the border case) */
                QRgb rgba = source.pixel(x, y2);
                rt += qRed(rgba);
                gt += qGreen(rgba);
                bt += qBlue(rgba);
                at += qAlpha(rgba);
            }
            /* Set the new weighted pixel */
            dest.setPixel(x, y, qRgba(rt / b, gt / b, bt / b, at / b));
        }
    }
}


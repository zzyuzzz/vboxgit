/*
 * Duck Truemotion v1 Decoding Tables
 *
 * Data in this file was originally part of VpVision from On2 which is
 * distributed under the GNU GPL. It is redistributed with ffmpeg under the
 * GNU LGPL using the common understanding that data tables necessary for
 * decoding algorithms are not necessarily licensable.
 */
#ifndef TRUEMOTION1DATA_H
#define TRUEMOTION1DATA_H

/* Y delta tables, skinny and fat */
static const int16_t ydt1[8] = { 0, -2, 2, -6, 6, -12, 12, -12 };
static const int16_t ydt2[8] = { 0, -2, 4, -6, 8, -12, 12, -12 };
static const int16_t ydt3[8] = { 4, -6, 20, -20, 46, -46, 94, -94 };
static const int16_t fat_ydt3[8] = { 0, -15, 50, -50, 115, -115, 235, -235 };
static const int16_t ydt4[8] = { 0, -4, 4, -16, 16, -36, 36, -80 };
static const int16_t fat_ydt4[8] = { 0, 40, 80, -76, 160, -154, 236, -236 };

/* C delta tables, skinny and fat */
static const int16_t cdt1[8] = { 0, -1, 1, -2, 3, -4, 5, -4 };
static const int16_t cdt2[8] = { 0, -4, 3, -16, 20, -32, 36, -32 };
static const int16_t fat_cdt2[8] = { 0, -20, 15, -80, 100, -160, 180, -160 };
static const int16_t cdt3[8] = { 0, -2, 2, -8, 8, -18, 18, -40 };
/* NOTE: This table breaks the [+,-] pattern that the rest of the
 * tables maintain. Is this intentional? */
static const int16_t fat_cdt3[8] = { 0, 40, 80, -76, 160, -154, 236, -236 };

/* all the delta tables to choose from, at all 4 delta levels */
static const int16_t *ydts[] = { ydt1, ydt2, ydt3, ydt4, NULL };
static const int16_t *fat_ydts[] = { fat_ydt3, fat_ydt3, fat_ydt3, fat_ydt4, NULL };
static const int16_t *cdts[] = { cdt1, cdt1, cdt2, cdt3, NULL };
static const int16_t *fat_cdts[] = { fat_cdt2, fat_cdt2, fat_cdt2, fat_cdt3, NULL };

static const uint8_t pc_tbl2[] = {
0x8,0x00,0x00,0x00,0x00,
0x8,0x00,0x00,0x00,0x00,
0x8,0x10,0x00,0x00,0x00,
0x8,0x01,0x00,0x00,0x00,
0x8,0x00,0x10,0x00,0x00,
0x8,0x00,0x01,0x00,0x00,
0x8,0x00,0x00,0x10,0x00,
0x8,0x00,0x00,0x01,0x00,
0x8,0x00,0x00,0x00,0x10,
0x8,0x00,0x00,0x00,0x01,
0x6,0x00,0x00,0x00,
0x6,0x10,0x00,0x00,
0x6,0x01,0x00,0x00,
0x6,0x00,0x10,0x00,
0x6,0x00,0x01,0x00,
0x6,0x00,0x00,0x01,
0x6,0x00,0x00,0x10,
0x6,0x00,0x00,0x02,
0x6,0x00,0x00,0x20,
0x6,0x20,0x10,0x00,
0x6,0x00,0x02,0x01,
0x6,0x00,0x20,0x10,
0x6,0x02,0x01,0x00,
0x6,0x11,0x00,0x00,
0x6,0x00,0x20,0x00,
0x6,0x00,0x02,0x00,
0x6,0x20,0x00,0x00,
0x6,0x01,0x10,0x00,
0x6,0x02,0x00,0x00,
0x6,0x01,0x00,0x02,
0x6,0x10,0x00,0x20,
0x6,0x00,0x01,0x02,
0x6,0x10,0x01,0x00,
0x6,0x00,0x10,0x20,
0x6,0x10,0x10,0x00,
0x6,0x10,0x00,0x01,
0x6,0x20,0x00,0x10,
0x6,0x02,0x00,0x01,
0x6,0x01,0x01,0x00,
0x6,0x01,0x00,0x10,
0x6,0x00,0x11,0x00,
0x6,0x10,0x00,0x02,
0x6,0x00,0x01,0x10,
0x6,0x00,0x00,0x11,
0x6,0x10,0x00,0x10,
0x6,0x01,0x00,0x01,
0x6,0x00,0x00,0x22,
0x6,0x02,0x01,0x01,
0x6,0x10,0x20,0x10,
0x6,0x01,0x02,0x01,
0x6,0x20,0x10,0x10,
0x6,0x01,0x00,0x20,
0x6,0x00,0x10,0x01,
0x6,0x21,0x10,0x00,
0x6,0x10,0x02,0x01,
0x6,0x12,0x01,0x00,
0x6,0x01,0x20,0x10,
0x6,0x01,0x02,0x00,
0x6,0x10,0x20,0x00,
0x6,0x00,0x10,0x02,
0x6,0x00,0x01,0x20,
0x6,0x00,0x02,0x21,
0x6,0x00,0x02,0x20,
0x6,0x00,0x00,0x12,
0x6,0x00,0x00,0x21,
0x6,0x20,0x11,0x00,
0x6,0x00,0x01,0x01,
0x6,0x11,0x10,0x00,
0x6,0x00,0x20,0x12,
0x6,0x00,0x20,0x11,
0x6,0x20,0x10,0x02,
0x6,0x02,0x01,0x20,
0x6,0x00,0x22,0x11,
0x6,0x00,0x10,0x10,
0x6,0x02,0x11,0x00,
0x6,0x00,0x21,0x10,
0x6,0x00,0x02,0x03,
0x6,0x20,0x10,0x01,
0x6,0x00,0x12,0x01,
0x4,0x11,0x00,
0x4,0x00,0x22,
0x4,0x20,0x00,
0x4,0x01,0x10,
0x4,0x02,0x20,
0x4,0x00,0x20,
0x4,0x02,0x00,
0x4,0x10,0x01,
0x4,0x00,0x11,
0x4,0x02,0x01,
0x4,0x02,0x21,
0x4,0x00,0x02,
0x4,0x20,0x02,
0x4,0x01,0x01,
0x4,0x10,0x10,
0x4,0x10,0x02,
0x4,0x22,0x00,
0x4,0x10,0x00,
0x4,0x01,0x00,
0x4,0x21,0x00,
0x4,0x12,0x00,
0x4,0x00,0x10,
0x4,0x20,0x12,
0x4,0x01,0x11,
0x4,0x00,0x01,
0x4,0x01,0x02,
0x4,0x11,0x02,
0x4,0x11,0x01,
0x4,0x10,0x20,
0x4,0x20,0x01,
0x4,0x22,0x11,
0x4,0x00,0x12,
0x4,0x20,0x10,
0x4,0x22,0x01,
0x4,0x01,0x20,
0x4,0x00,0x21,
0x4,0x10,0x11,
0x4,0x21,0x10,
0x4,0x10,0x22,
0x4,0x02,0x03,
0x4,0x12,0x01,
0x4,0x20,0x11,
0x4,0x11,0x10,
0x4,0x20,0x30,
0x4,0x11,0x20,
0x4,0x02,0x10,
0x4,0x22,0x10,
0x4,0x11,0x11,
0x4,0x30,0x20,
0x4,0x30,0x00,
0x4,0x01,0x22,
0x4,0x01,0x12,
0x4,0x02,0x11,
0x4,0x03,0x02,
0x4,0x03,0x00,
0x4,0x10,0x21,
0x4,0x12,0x20,
0x4,0x00,0x00,
0x4,0x12,0x21,
0x4,0x21,0x11,
0x4,0x02,0x22,
0x4,0x10,0x12,
0x4,0x31,0x00,
0x4,0x20,0x20,
0x4,0x00,0x03,
0x4,0x02,0x02,
0x4,0x22,0x20,
0x4,0x01,0x21,
0x4,0x21,0x02,
0x4,0x21,0x12,
0x4,0x11,0x22,
0x4,0x00,0x30,
0x4,0x12,0x11,
0x4,0x20,0x22,
0x4,0x31,0x20,
0x4,0x21,0x30,
0x4,0x22,0x02,
0x4,0x22,0x22,
0x4,0x20,0x31,
0x4,0x13,0x02,
0x4,0x03,0x10,
0x4,0x11,0x12,
0x4,0x00,0x13,
0x4,0x21,0x01,
0x4,0x12,0x03,
0x4,0x13,0x00,
0x4,0x13,0x10,
0x4,0x02,0x13,
0x4,0x30,0x01,
0x4,0x12,0x10,
0x4,0x22,0x13,
0x4,0x03,0x12,
0x4,0x31,0x01,
0x4,0x30,0x22,
0x4,0x00,0x31,
0x4,0x01,0x31,
0x4,0x02,0x23,
0x4,0x01,0x30,
0x4,0x11,0x21,
0x4,0x22,0x21,
0x4,0x01,0x13,
0x4,0x10,0x03,
0x4,0x22,0x03,
0x4,0x30,0x21,
0x4,0x21,0x31,
0x4,0x33,0x00,
0x4,0x13,0x12,
0x4,0x11,0x31,
0x4,0x30,0x02,
0x4,0x12,0x02,
0x4,0x11,0x13,
0x4,0x12,0x22,
0x4,0x20,0x32,
0x4,0x10,0x13,
0x4,0x22,0x31,
0x4,0x21,0x20,
0x4,0x01,0x33,
0x4,0x33,0x10,
0x4,0x20,0x13,
0x4,0x31,0x22,
0x4,0x13,0x30,
0x4,0x01,0x03,
0x4,0x11,0x33,
0x4,0x20,0x21,
0x4,0x13,0x31,
0x4,0x03,0x22,
0x4,0x31,0x02,
0x4,0x00,0x24,
0x2,0x00,
0x2,0x10,
0x2,0x20,
0x2,0x30,
0x2,0x40,
0x2,0x50,
0x2,0x60,
0x2,0x01,
0x2,0x11,
0x2,0x21,
0x2,0x31,
0x2,0x41,
0x2,0x51,
0x2,0x61,
0x2,0x02,
0x2,0x12,
0x2,0x22,
0x2,0x32,
0x2,0x42,
0x2,0x52,
0x2,0x62,
0x2,0x03,
0x2,0x13,
0x2,0x23,
0x2,0x33,
0x2,0x43,
0x2,0x53,
0x2,0x63,
0x2,0x04,
0x2,0x14,
0x2,0x24,
0x2,0x34,
0x2,0x44,
0x2,0x54,
0x2,0x64,
0x2,0x05,
0x2,0x15,
0x2,0x25,
0x2,0x35,
0x2,0x45,
0x2,0x55,
0x2,0x65,
0x2,0x06,
0x2,0x16,
0x2,0x26,
0x2,0x36,
0x2,0x46,
0x2,0x56,
0x2,0x66
};

static const uint8_t pc_tbl3[] = {
0x6,0x00,0x00,0x00,
0x6,0x00,0x00,0x00,
0x6,0x00,0x00,0x01,
0x6,0x00,0x00,0x10,
0x6,0x00,0x00,0x11,
0x6,0x00,0x01,0x00,
0x6,0x00,0x01,0x01,
0x6,0x00,0x01,0x10,
0x6,0x00,0x01,0x11,
0x6,0x00,0x10,0x00,
0x6,0x00,0x10,0x01,
0x6,0x00,0x10,0x10,
0x6,0x00,0x10,0x11,
0x6,0x00,0x11,0x00,
0x6,0x00,0x11,0x01,
0x6,0x00,0x11,0x10,
0x6,0x00,0x11,0x11,
0x6,0x01,0x00,0x00,
0x6,0x01,0x00,0x01,
0x6,0x01,0x00,0x10,
0x6,0x01,0x00,0x11,
0x6,0x01,0x01,0x00,
0x6,0x01,0x01,0x01,
0x6,0x01,0x01,0x10,
0x6,0x01,0x01,0x11,
0x6,0x01,0x10,0x00,
0x6,0x01,0x10,0x01,
0x6,0x01,0x10,0x10,
0x6,0x01,0x10,0x11,
0x6,0x01,0x11,0x00,
0x6,0x01,0x11,0x01,
0x6,0x01,0x11,0x10,
0x6,0x01,0x11,0x11,
0x6,0x10,0x00,0x00,
0x6,0x10,0x00,0x01,
0x6,0x10,0x00,0x10,
0x6,0x10,0x00,0x11,
0x6,0x10,0x01,0x00,
0x6,0x10,0x01,0x01,
0x6,0x10,0x01,0x10,
0x6,0x10,0x01,0x11,
0x6,0x10,0x10,0x00,
0x6,0x10,0x10,0x01,
0x6,0x10,0x10,0x10,
0x6,0x10,0x10,0x11,
0x6,0x10,0x11,0x00,
0x6,0x10,0x11,0x01,
0x6,0x10,0x11,0x10,
0x6,0x10,0x11,0x11,
0x6,0x11,0x00,0x00,
0x6,0x11,0x00,0x01,
0x6,0x11,0x00,0x10,
0x6,0x11,0x00,0x11,
0x6,0x11,0x01,0x00,
0x6,0x11,0x01,0x01,
0x6,0x11,0x01,0x10,
0x6,0x11,0x01,0x11,
0x6,0x11,0x10,0x00,
0x6,0x11,0x10,0x01,
0x6,0x11,0x10,0x10,
0x6,0x11,0x10,0x11,
0x6,0x11,0x11,0x00,
0x6,0x11,0x11,0x01,
0x6,0x11,0x11,0x10,
0x4,0x00,0x00,
0x4,0x00,0x01,
0x4,0x00,0x02,
0x4,0x00,0x03,
0x4,0x00,0x10,
0x4,0x00,0x11,
0x4,0x00,0x12,
0x4,0x00,0x13,
0x4,0x00,0x20,
0x4,0x00,0x21,
0x4,0x00,0x22,
0x4,0x00,0x23,
0x4,0x00,0x30,
0x4,0x00,0x31,
0x4,0x00,0x32,
0x4,0x00,0x33,
0x4,0x01,0x00,
0x4,0x01,0x01,
0x4,0x01,0x02,
0x4,0x01,0x03,
0x4,0x01,0x10,
0x4,0x01,0x11,
0x4,0x01,0x12,
0x4,0x01,0x13,
0x4,0x01,0x20,
0x4,0x01,0x21,
0x4,0x01,0x22,
0x4,0x01,0x23,
0x4,0x01,0x30,
0x4,0x01,0x31,
0x4,0x01,0x32,
0x4,0x01,0x33,
0x4,0x02,0x00,
0x4,0x02,0x01,
0x4,0x02,0x02,
0x4,0x02,0x03,
0x4,0x02,0x10,
0x4,0x02,0x11,
0x4,0x02,0x12,
0x4,0x02,0x13,
0x4,0x02,0x20,
0x4,0x02,0x21,
0x4,0x02,0x22,
0x4,0x02,0x23,
0x4,0x02,0x30,
0x4,0x02,0x31,
0x4,0x02,0x32,
0x4,0x02,0x33,
0x4,0x03,0x00,
0x4,0x03,0x01,
0x4,0x03,0x02,
0x4,0x03,0x03,
0x4,0x03,0x10,
0x4,0x03,0x11,
0x4,0x03,0x12,
0x4,0x03,0x13,
0x4,0x03,0x20,
0x4,0x03,0x21,
0x4,0x03,0x22,
0x4,0x03,0x23,
0x4,0x03,0x30,
0x4,0x03,0x31,
0x4,0x03,0x32,
0x4,0x03,0x33,
0x4,0x10,0x00,
0x4,0x10,0x01,
0x4,0x10,0x02,
0x4,0x10,0x03,
0x4,0x10,0x10,
0x4,0x10,0x11,
0x4,0x10,0x12,
0x4,0x10,0x13,
0x4,0x10,0x20,
0x4,0x10,0x21,
0x4,0x10,0x22,
0x4,0x10,0x23,
0x4,0x10,0x30,
0x4,0x10,0x31,
0x4,0x10,0x32,
0x4,0x10,0x33,
0x4,0x11,0x00,
0x4,0x11,0x01,
0x4,0x11,0x02,
0x4,0x11,0x03,
0x4,0x11,0x10,
0x4,0x11,0x11,
0x4,0x11,0x12,
0x4,0x11,0x13,
0x4,0x11,0x20,
0x4,0x11,0x21,
0x4,0x11,0x22,
0x4,0x11,0x23,
0x4,0x11,0x30,
0x4,0x11,0x31,
0x4,0x11,0x32,
0x4,0x11,0x33,
0x4,0x12,0x00,
0x4,0x12,0x01,
0x4,0x12,0x02,
0x4,0x12,0x03,
0x4,0x12,0x10,
0x4,0x12,0x11,
0x4,0x12,0x12,
0x4,0x12,0x13,
0x4,0x12,0x20,
0x4,0x12,0x21,
0x4,0x12,0x22,
0x4,0x12,0x23,
0x4,0x12,0x30,
0x4,0x12,0x31,
0x4,0x12,0x32,
0x4,0x12,0x33,
0x4,0x13,0x00,
0x4,0x13,0x01,
0x4,0x13,0x02,
0x4,0x13,0x03,
0x4,0x13,0x10,
0x4,0x13,0x11,
0x4,0x13,0x12,
0x4,0x13,0x13,
0x4,0x13,0x20,
0x4,0x13,0x21,
0x4,0x13,0x22,
0x4,0x13,0x23,
0x4,0x13,0x30,
0x4,0x13,0x31,
0x4,0x13,0x32,
0x4,0x13,0x33,
0x2,0x00,
0x2,0x10,
0x2,0x20,
0x2,0x30,
0x2,0x40,
0x2,0x50,
0x2,0x60,
0x2,0x70,
0x2,0x01,
0x2,0x11,
0x2,0x21,
0x2,0x31,
0x2,0x41,
0x2,0x51,
0x2,0x61,
0x2,0x71,
0x2,0x02,
0x2,0x12,
0x2,0x22,
0x2,0x32,
0x2,0x42,
0x2,0x52,
0x2,0x62,
0x2,0x72,
0x2,0x03,
0x2,0x13,
0x2,0x23,
0x2,0x33,
0x2,0x43,
0x2,0x53,
0x2,0x63,
0x2,0x73,
0x2,0x04,
0x2,0x14,
0x2,0x24,
0x2,0x34,
0x2,0x44,
0x2,0x54,
0x2,0x64,
0x2,0x74,
0x2,0x05,
0x2,0x15,
0x2,0x25,
0x2,0x35,
0x2,0x45,
0x2,0x55,
0x2,0x65,
0x2,0x75,
0x2,0x06,
0x2,0x16,
0x2,0x26,
0x2,0x36,
0x2,0x46,
0x2,0x56,
0x2,0x66,
0x2,0x76,
0x2,0x07,
0x2,0x17,
0x2,0x27,
0x2,0x37,
0x2,0x47,
0x2,0x57,
0x2,0x67,
0x2,0x77
};

static const uint8_t pc_tbl4[] = {
0x8,0x00,0x00,0x00,0x00,
0x8,0x00,0x00,0x00,0x00,
0x8,0x20,0x00,0x00,0x00,
0x8,0x00,0x00,0x00,0x01,
0x8,0x10,0x00,0x00,0x00,
0x8,0x00,0x00,0x00,0x02,
0x8,0x01,0x00,0x00,0x00,
0x8,0x00,0x00,0x00,0x10,
0x8,0x02,0x00,0x00,0x00,
0x6,0x00,0x00,0x00,
0x6,0x20,0x00,0x00,
0x6,0x00,0x00,0x01,
0x6,0x10,0x00,0x00,
0x6,0x00,0x00,0x02,
0x6,0x00,0x10,0x00,
0x6,0x00,0x20,0x00,
0x6,0x00,0x02,0x00,
0x6,0x00,0x01,0x00,
0x6,0x01,0x00,0x00,
0x6,0x00,0x00,0x20,
0x6,0x02,0x00,0x00,
0x6,0x00,0x00,0x10,
0x6,0x10,0x00,0x20,
0x6,0x01,0x00,0x02,
0x6,0x20,0x00,0x10,
0x6,0x02,0x00,0x01,
0x6,0x20,0x10,0x00,
0x6,0x00,0x12,0x00,
0x6,0x00,0x02,0x01,
0x6,0x02,0x01,0x00,
0x6,0x00,0x21,0x00,
0x6,0x00,0x01,0x02,
0x6,0x00,0x20,0x10,
0x6,0x00,0x00,0x21,
0x6,0x00,0x00,0x12,
0x6,0x00,0x01,0x20,
0x6,0x12,0x00,0x00,
0x6,0x00,0x10,0x20,
0x6,0x01,0x20,0x00,
0x6,0x02,0x10,0x00,
0x6,0x10,0x20,0x00,
0x6,0x01,0x02,0x00,
0x6,0x21,0x00,0x00,
0x6,0x00,0x02,0x10,
0x6,0x20,0x01,0x00,
0x6,0x00,0x22,0x00,
0x6,0x10,0x02,0x00,
0x6,0x00,0x10,0x02,
0x6,0x11,0x00,0x00,
0x6,0x00,0x11,0x00,
0x6,0x22,0x00,0x00,
0x6,0x20,0x00,0x02,
0x6,0x10,0x00,0x01,
0x6,0x00,0x20,0x01,
0x6,0x02,0x20,0x00,
0x6,0x01,0x10,0x00,
0x6,0x01,0x00,0x20,
0x6,0x00,0x20,0x02,
0x6,0x01,0x20,0x02,
0x6,0x10,0x01,0x00,
0x6,0x02,0x00,0x10,
0x6,0x00,0x10,0x01,
0x6,0x10,0x01,0x20,
0x6,0x20,0x02,0x10,
0x6,0x00,0x00,0x22,
0x6,0x10,0x00,0x02,
0x6,0x00,0x02,0x20,
0x6,0x20,0x02,0x00,
0x6,0x00,0x00,0x11,
0x6,0x02,0x10,0x01,
0x6,0x00,0x01,0x10,
0x6,0x00,0x02,0x11,
0x4,0x01,0x02,
0x4,0x02,0x01,
0x4,0x01,0x00,
0x4,0x10,0x20,
0x4,0x20,0x10,
0x4,0x20,0x00,
0x4,0x11,0x00,
0x4,0x02,0x00,
0x4,0x12,0x00,
0x4,0x00,0x21,
0x4,0x22,0x00,
0x4,0x00,0x12,
0x4,0x21,0x00,
0x4,0x02,0x11,
0x4,0x00,0x01,
0x4,0x10,0x02,
0x4,0x02,0x20,
0x4,0x20,0x11,
0x4,0x01,0x10,
0x4,0x21,0x10,
0x4,0x10,0x00,
0x4,0x10,0x22,
0x4,0x20,0x20,
0x4,0x00,0x22,
0x4,0x01,0x22,
0x4,0x20,0x01,
0x4,0x02,0x02,
0x4,0x00,0x20,
0x4,0x00,0x10,
0x4,0x00,0x11,
0x4,0x22,0x01,
0x4,0x11,0x20,
0x4,0x12,0x01,
0x4,0x12,0x20,
0x4,0x11,0x02,
0x4,0x10,0x10,
0x4,0x01,0x01,
0x4,0x02,0x21,
0x4,0x20,0x12,
0x4,0x01,0x12,
0x4,0x22,0x11,
0x4,0x21,0x12,
0x4,0x22,0x10,
0x4,0x21,0x02,
0x4,0x20,0x02,
0x4,0x10,0x01,
0x4,0x00,0x02,
0x4,0x10,0x21,
0x4,0x01,0x20,
0x4,0x11,0x22,
0x4,0x12,0x21,
0x4,0x22,0x20,
0x4,0x02,0x10,
0x4,0x02,0x22,
0x4,0x11,0x10,
0x4,0x22,0x02,
0x4,0x20,0x21,
0x4,0x01,0x11,
0x4,0x11,0x01,
0x4,0x10,0x12,
0x4,0x02,0x12,
0x4,0x20,0x22,
0x4,0x21,0x20,
0x4,0x01,0x21,
0x4,0x12,0x02,
0x4,0x21,0x11,
0x4,0x12,0x22,
0x4,0x12,0x10,
0x4,0x22,0x21,
0x4,0x10,0x11,
0x4,0x21,0x01,
0x4,0x11,0x12,
0x4,0x12,0x11,
0x4,0x66,0x66,
0x4,0x22,0x22,
0x4,0x11,0x21,
0x4,0x11,0x11,
0x4,0x21,0x22,
0x4,0x00,0x00,
0x4,0x22,0x12,
0x4,0x12,0x12,
0x4,0x21,0x21,
0x4,0x42,0x00,
0x4,0x00,0x04,
0x4,0x40,0x00,
0x4,0x30,0x00,
0x4,0x31,0x00,
0x4,0x00,0x03,
0x4,0x00,0x14,
0x4,0x00,0x13,
0x4,0x01,0x24,
0x4,0x20,0x13,
0x4,0x01,0x42,
0x4,0x14,0x20,
0x4,0x42,0x02,
0x4,0x13,0x00,
0x4,0x00,0x24,
0x4,0x31,0x20,
0x4,0x22,0x13,
0x4,0x11,0x24,
0x4,0x12,0x66,
0x4,0x30,0x01,
0x4,0x02,0x13,
0x4,0x12,0x42,
0x4,0x40,0x10,
0x4,0x40,0x02,
0x4,0x01,0x04,
0x4,0x24,0x00,
0x4,0x42,0x10,
0x4,0x21,0x13,
0x4,0x13,0x12,
0x4,0x31,0x21,
0x4,0x21,0x24,
0x4,0x00,0x40,
0x4,0x10,0x24,
0x4,0x10,0x42,
0x4,0x32,0x01,
0x4,0x11,0x42,
0x4,0x20,0x31,
0x4,0x12,0x40,
0x2,0x00,
0x2,0x10,
0x2,0x20,
0x2,0x30,
0x2,0x40,
0x2,0x50,
0x2,0x60,
0x2,0x70,
0x2,0x01,
0x2,0x11,
0x2,0x21,
0x2,0x31,
0x2,0x41,
0x2,0x51,
0x2,0x61,
0x2,0x71,
0x2,0x02,
0x2,0x12,
0x2,0x22,
0x2,0x32,
0x2,0x42,
0x2,0x52,
0x2,0x62,
0x2,0x72,
0x2,0x03,
0x2,0x13,
0x2,0x23,
0x2,0x33,
0x2,0x43,
0x2,0x53,
0x2,0x63,
0x2,0x73,
0x2,0x04,
0x2,0x14,
0x2,0x24,
0x2,0x34,
0x2,0x44,
0x2,0x54,
0x2,0x64,
0x2,0x74,
0x2,0x05,
0x2,0x15,
0x2,0x25,
0x2,0x35,
0x2,0x45,
0x2,0x55,
0x2,0x65,
0x2,0x75,
0x2,0x06,
0x2,0x16,
0x2,0x26,
0x2,0x36,
0x2,0x46,
0x2,0x56,
0x2,0x66,
0x2,0x76,
0x2,0x07,
0x2,0x17,
0x2,0x27,
0x2,0x37,
0x2,0x47,
0x2,0x57,
0x2,0x67,
0x2,0x77
};

static const uint8_t *tables[] = { pc_tbl2, pc_tbl3, pc_tbl4 };
#endif

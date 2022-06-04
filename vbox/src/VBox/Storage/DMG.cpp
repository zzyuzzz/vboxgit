/* $Id: DMG.cpp 44233 2013-01-04 20:39:56Z vboxsync $ */
/** @file
 * VBoxDMG - Interpreter for Apple Disk Images (DMG).
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

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_VD_DMG
#include <VBox/vd-plugin.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/mem.h>
#include <iprt/ctype.h>
#include <iprt/string.h>
#include <iprt/base64.h>
#include <iprt/zip.h>

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

/** Sector size, multiply with all sector counts to get number of bytes. */
#define DMG_SECTOR_SIZE 512

/** Convert block number/size to byte offset/size. */
#define DMG_BLOCK2BYTE(u)          ((uint64_t)(u) << 9)

/** Convert byte offset/size to block number/size. */
#define DMG_BYTE2BLOCK(u)          ((u) >> 9)

/**
 * UDIF checksum structure.
 */
typedef struct DMGUDIFCKSUM
{
    uint32_t        u32Kind;                    /**< The kind of checksum.  */
    uint32_t        cBits;                      /**< The size of the checksum. */
    union
    {
        uint8_t     au8[128];                   /**< 8-bit view. */
        uint32_t    au32[32];                   /**< 32-bit view. */
    }               uSum;                       /**< The checksum. */
} DMGUDIFCKSUM;
AssertCompileSize(DMGUDIFCKSUM, 8 + 128);
typedef DMGUDIFCKSUM *PDMGUDIFCKSUM;
typedef const DMGUDIFCKSUM *PCDMGUDIFCKSUM;

/** @name Checksum Kind (DMGUDIFCKSUM::u32Kind)
 * @{ */
/** No checksum. */
#define DMGUDIFCKSUM_NONE          UINT32_C(0)
/** CRC-32. */
#define DMGUDIFCKSUM_CRC32         UINT32_C(2)
/** @} */

/**
 * UDIF ID.
 * This is kind of like a UUID only it isn't, but we'll use the UUID
 * representation of it for simplicity.
 */
typedef RTUUID DMGUDIFID;
AssertCompileSize(DMGUDIFID, 16);
typedef DMGUDIFID *PDMGUDIFID;
typedef const DMGUDIFID *PCDMGUDIFID;

/**
 * UDIF footer used by Apple Disk Images (DMG).
 *
 * This is a footer placed 512 bytes from the end of the file. Typically a DMG
 * file starts with the data, which is followed by the block table and then ends
 * with this structure.
 *
 * All fields are stored in big endian format.
 */
#pragma pack(1)
typedef struct DMGUDIF
{
    uint32_t            u32Magic;               /**< 0x000 - Magic, 'koly' (DMGUDIF_MAGIC).                       (fUDIFSignature) */
    uint32_t            u32Version;             /**< 0x004 - The UDIF version (DMGUDIF_VER_CURRENT).              (fUDIFVersion) */
    uint32_t            cbFooter;               /**< 0x008 - The size of the this structure (512).                 (fUDIFHeaderSize) */
    uint32_t            fFlags;                 /**< 0x00c - Flags.                                                (fUDIFFlags) */
    uint64_t            offRunData;             /**< 0x010 - Where the running data fork starts (usually 0).       (fUDIFRunningDataForkOffset) */
    uint64_t            offData;                /**< 0x018 - Where the data fork starts (usually 0).               (fUDIFDataForkOffset) */
    uint64_t            cbData;                 /**< 0x020 - Size of the data fork (in bytes).                     (fUDIFDataForkLength) */
    uint64_t            offRsrc;                /**< 0x028 - Where the resource fork starts (usually cbData or 0). (fUDIFRsrcForkOffset) */
    uint64_t            cbRsrc;                 /**< 0x030 - The size of the resource fork.                        (fUDIFRsrcForkLength)*/
    uint32_t            iSegment;               /**< 0x038 - The segment number of this file.                      (fUDIFSegmentNumber) */
    uint32_t            cSegments;              /**< 0x03c - The number of segments.                               (fUDIFSegmentCount) */
    DMGUDIFID           SegmentId;              /**< 0x040 - The segment ID.                                       (fUDIFSegmentID) */
    DMGUDIFCKSUM        DataCkSum;              /**< 0x050 - The data checksum.                                    (fUDIFDataForkChecksum) */
    uint64_t            offXml;                 /**< 0x0d8 - The XML offset (.plist kind of data).                 (fUDIFXMLOffset) */
    uint64_t            cbXml;                  /**< 0x0e0 - The size of the XML.                                  (fUDIFXMLSize) */
    uint8_t             abUnknown[120];         /**< 0x0e8 - Unknown stuff, hdiutil doesn't dump it... */
    DMGUDIFCKSUM        MasterCkSum;            /**< 0x160 - The master checksum.                                  (fUDIFMasterChecksum) */
    uint32_t            u32Type;                /**< 0x1e8 - The image type.                                       (fUDIFImageVariant) */
    uint64_t            cSectors;               /**< 0x1ec - The sector count. Warning! Unaligned!                 (fUDISectorCount) */
    uint32_t            au32Unknown[3];         /**< 0x1f4 - Unknown stuff, hdiutil doesn't dump it... */
} DMGUDIF;
#pragma pack()
AssertCompileSize(DMGUDIF, 512);
AssertCompileMemberOffset(DMGUDIF, cbRsrc,   0x030);
AssertCompileMemberOffset(DMGUDIF, cbXml,    0x0e0);
AssertCompileMemberOffset(DMGUDIF, cSectors, 0x1ec);

typedef DMGUDIF *PDMGUDIF;
typedef const DMGUDIF *PCDMGUDIF;

/** The UDIF magic 'koly' (DMGUDIF::u32Magic). */
#define DMGUDIF_MAGIC              UINT32_C(0x6b6f6c79)

/** The current UDIF version (DMGUDIF::u32Version).
 * This is currently the only we recognizes and will create. */
#define DMGUDIF_VER_CURRENT        4

/** @name UDIF flags (DMGUDIF::fFlags).
 * @{ */
/** Flatten image whatever that means.
 * (hdiutil -debug calls it kUDIFFlagsFlattened.) */
#define DMGUDIF_FLAGS_FLATTENED    RT_BIT_32(0)
/** Internet enabled image.
 * (hdiutil -debug calls it kUDIFFlagsInternetEnabled) */
#define DMGUDIF_FLAGS_INET_ENABLED RT_BIT_32(2)
/** Mask of known bits. */
#define DMGUDIF_FLAGS_KNOWN_MASK   (RT_BIT_32(0) | RT_BIT_32(2))
/** @} */

/** @name UDIF Image Types (DMGUDIF::u32Type).
 * @{ */
/** Device image type. (kUDIFDeviceImageType) */
#define DMGUDIF_TYPE_DEVICE        1
/** Device image type. (kUDIFPartitionImageType) */
#define DMGUDIF_TYPE_PARTITION     2
/** @}  */

/**
 * BLKX data.
 *
 * This contains the start offset and size of raw data stored in the image.
 *
 * All fields are stored in big endian format.
 */
#pragma pack(1)
typedef struct DMGBLKX
{
    uint32_t            u32Magic;               /**< 0x000 - Magic, 'mish' (DMGBLKX_MAGIC). */
    uint32_t            u32Version;             /**< 0x004 - The BLKX version (DMGBLKX_VER_CURRENT). */
    uint64_t            cSectornumberFirst;     /**< 0x008 - The first sector number the block represents in the virtual device. */
    uint64_t            cSectors;               /**< 0x010 - Number of sectors this block represents. */
    uint64_t            offDataStart;           /**< 0x018 - Start offset for raw data. */
    uint32_t            cSectorsDecompress;     /**< 0x020 - Size of the buffer in sectors needed to decompress. */
    uint32_t            u32BlocksDescriptor;    /**< 0x024 - Blocks descriptor. */
    uint8_t             abReserved[24];
    DMGUDIFCKSUM        BlkxCkSum;              /**< 0x03c - Checksum for the BLKX table. */
    uint32_t            cBlocksRunCount;        /**< 0x    - Number of entries in the blkx run table afterwards. */
} DMGBLKX;
#pragma pack()
AssertCompileSize(DMGBLKX, 204);

typedef DMGBLKX *PDMGBLKX;
typedef const DMGBLKX *PCDMGBLKX;

/** The BLKX magic 'mish' (DMGBLKX::u32Magic). */
#define DMGBLKX_MAGIC              UINT32_C(0x6d697368)
/** BLKX version (DMGBLKX::u32Version). */
#define DMGBLKX_VERSION            UINT32_C(0x00000001)

/** Blocks descriptor type: entire device. */
#define DMGBLKX_DESC_ENTIRE_DEVICE UINT32_C(0xfffffffe)

/**
 * BLKX table descriptor.
 *
 * All fields are stored in big endian format.
 */
#pragma pack(1)
typedef struct DMGBLKXDESC
{
    uint32_t            u32Type;                /**< 0x000 - Type of the descriptor. */
    uint32_t            u32Reserved;            /**< 0x004 - Reserved, but contains +beg or +end in case thisi is a comment descriptor. */
    uint64_t            u64SectorStart;         /**< 0x008 - First sector number in the block this entry describes. */
    uint64_t            u64SectorCount;         /**< 0x010 - Number of sectors this entry describes. */
    uint64_t            offData;                /**< 0x018 - Offset in the image where the data starts. */
    uint64_t            cbData;                 /**< 0x020 - Number of bytes in the image. */
} DMGBLKXDESC;
#pragma pack()
AssertCompileSize(DMGBLKXDESC, 40);

typedef DMGBLKXDESC *PDMGBLKXDESC;
typedef const DMGBLKXDESC *PCDMGBLKXDESC;

/** Raw image data type. */
#define DMGBLKXDESC_TYPE_RAW        1
/** Ignore type. */
#define DMGBLKXDESC_TYPE_IGNORE     2
/** Compressed with zlib type. */
#define DMGBLKXDESC_TYPE_ZLIB       UINT32_C(0x80000005)
/** Comment type. */
#define DMGBLKXDESC_TYPE_COMMENT    UINT32_C(0x7ffffffe)
/** Terminator type. */
#define DMGBLKXDESC_TYPE_TERMINATOR UINT32_C(0xffffffff)

/**
 * UDIF Resource Entry.
 */
typedef struct DMGUDIFRSRCENTRY
{
    /** The ID. */
    int32_t             iId;
    /** Attributes. */
    uint32_t            fAttributes;
    /** The name. */
    char               *pszName;
    /** The CoreFoundation name. Can be NULL. */
    char               *pszCFName;
    /** The size of the data. */
    size_t              cbData;
    /** The raw data. */
    uint8_t            *pbData;
} DMGUDIFRSRCENTRY;
/** Pointer to an UDIF resource entry. */
typedef DMGUDIFRSRCENTRY *PDMGUDIFRSRCENTRY;
/** Pointer to a const UDIF resource entry. */
typedef DMGUDIFRSRCENTRY const *PCDMGUDIFRSRCENTRY;

/**
 * UDIF Resource Array.
 */
typedef struct DMGUDIFRSRCARRAY
{
    /** The array name. */
    char                szName[12];
    /** The number of occupied entries. */
    uint32_t            cEntries;
    /** The array entries.
     * A lazy bird ASSUME there are no more than 4 entries in any DMG. Increase the
     * size if DMGs with more are found.
     * r=aeichner: Saw one with 6 here (image of a whole DVD) */
    DMGUDIFRSRCENTRY    aEntries[10];
} DMGUDIFRSRCARRAY;
/** Pointer to a UDIF resource array. */
typedef DMGUDIFRSRCARRAY *PDMGUDIFRSRCARRAY;
/** Pointer to a const UDIF resource array. */
typedef DMGUDIFRSRCARRAY const *PCDMGUDIFRSRCARRAY;

/**
 * DMG extent types.
 */
typedef enum DMGEXTENTTYPE
{
    /** Null, never used. */
    DMGEXTENTTYPE_NULL = 0,
    /** Raw image data. */
    DMGEXTENTTYPE_RAW,
    /** Zero extent, reads return 0 and writes have no effect. */
    DMGEXTENTTYPE_ZERO,
    /** Compressed extent - compression method ZLIB. */
    DMGEXTENTTYPE_COMP_ZLIB,
    /** 32bit hack. */
    DMGEXTENTTYPE_32BIT_HACK = 0x7fffffff
} DMGEXTENTTYPE, *PDMGEXTENTTYPE;

/**
 * DMG extent mapping a virtual image block to real file offsets.
 */
typedef struct DMGEXTENT
{
    /** Extent type. */
    DMGEXTENTTYPE        enmType;
    /** First sector this extent describes. */
    uint64_t             uSectorExtent;
    /** Number of sectors this extent describes. */
    uint64_t             cSectorsExtent;
    /** Start offset in the real file. */
    uint64_t             offFileStart;
    /** Number of bytes for the extent data in the file. */
    uint64_t             cbFile;
} DMGEXTENT;
/** Pointer to an DMG extent. */
typedef DMGEXTENT *PDMGEXTENT;

/**
 * VirtualBox Apple Disk Image (DMG) interpreter instance data.
 */
typedef struct DMGIMAGE
{
    /** Image name.
     * Kept around for logging and delete-on-close purposes. */
    const char         *pszFilename;
    /** Storage handle. */
    PVDIOSTORAGE        pStorage;

    /** Pointer to the per-disk VD interface list. */
    PVDINTERFACE        pVDIfsDisk;
    /** Pointer to the per-image VD interface list. */
    PVDINTERFACE        pVDIfsImage;
    /** Error interface. */
    PVDINTERFACEERROR   pIfError;
    /** I/O interface. */
    PVDINTERFACEIOINT   pIfIo;

    /** Flags the image was opened with. */
    uint32_t            uOpenFlags;
    /** Image flags. */
    unsigned            uImageFlags;
    /** Total size of the virtual image. */
    uint64_t            cbSize;
    /** Size of the image. */
    uint64_t            cbFile;
    /** Physical geometry of this image. */
    VDGEOMETRY          PCHSGeometry;
    /** Logical geometry of this image. */
    VDGEOMETRY          LCHSGeometry;

    /** The resources.
     * A lazy bird ASSUME there are only two arrays in the resource-fork section in
     * the XML, namely 'blkx' and 'plst'. These have been assigned fixed indexes. */
    DMGUDIFRSRCARRAY    aRsrcs[2];
    /** The UDIF footer. */
    DMGUDIF             Ftr;

    /** Number of valid extents in the array. */
    unsigned            cExtents;
    /** Number of entries the array can hold. */
    unsigned            cExtentsMax;
    /** Pointer to the extent array. */
    PDMGEXTENT          paExtents;
    /** Index of the last accessed extent. */
    unsigned            idxExtentLast;

    /** Extent which owns the data in the buffer. */
    PDMGEXTENT          pExtentDecomp;
    /** Buffer holding the decompressed data for a extent. */
    void               *pvDecompExtent;
    /** Size of the buffer. */
    size_t              cbDecompExtent;
} DMGIMAGE;
/** Pointer to an instance of the DMG Image Interpreter. */
typedef DMGIMAGE *PDMGIMAGE;

/** @name Resources indexes (into DMG::aRsrcs).
 * @{ */
#define DMG_RSRC_IDX_BLKX   0
#define DMG_RSRC_IDX_PLST   1
/** @} */

/** State for the input callout of the inflate reader. */
typedef struct DMGINFLATESTATE
{
    /* Image this operation relates to. */
    PDMGIMAGE pImage;
    /* Total size of the data to read. */
    size_t    cbSize;
    /* Offset in the file to read. */
    uint64_t  uFileOffset;
    /* Current read position. */
    ssize_t   iOffset;
} DMGINFLATESTATE;

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/** @def DMG_PRINTF
 * Wrapper for LogRel.
 */
#define DMG_PRINTF(a)  LogRel(a)

/** @def DMG_VALIDATE
 * For validating a struct thing and log/print what's wrong.
 */
# define DMG_VALIDATE(expr, logstuff) \
    do { \
        if (!(expr)) \
        { \
            LogRel(("DMG: validation failed: %s\nDMG: ", #expr)); \
            LogRel(logstuff); \
            fRc = false; \
        } \
    } while (0)

/** VBoxDMG: Unable to parse the XML. */
#define VERR_VD_DMG_XML_PARSE_ERROR         (-3280)


/*******************************************************************************
*   Static Variables                                                           *
*******************************************************************************/

/** NULL-terminated array of supported file extensions. */
static const VDFILEEXTENSION s_aDmgFileExtensions[] =
{
    {"dmg", VDTYPE_DVD},
    {NULL, VDTYPE_INVALID}
};

/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static void dmgUdifFtrHost2FileEndian(PDMGUDIF pUdif);
static void dmgUdifFtrFile2HostEndian(PDMGUDIF pUdif);

static void dmgUdifIdHost2FileEndian(PDMGUDIFID pId);
static void dmgUdifIdFile2HostEndian(PDMGUDIFID pId);

static void dmgUdifCkSumHost2FileEndian(PDMGUDIFCKSUM pCkSum);
static void dmgUdifCkSumFile2HostEndian(PDMGUDIFCKSUM pCkSum);
static bool dmgUdifCkSumIsValid(PCDMGUDIFCKSUM pCkSum, const char *pszPrefix);

static DECLCALLBACK(int) dmgFileInflateHelper(void *pvUser, void *pvBuf, size_t cbBuf, size_t *pcbBuf)
{
    DMGINFLATESTATE *pInflateState = (DMGINFLATESTATE *)pvUser;

    Assert(cbBuf);
    if (pInflateState->iOffset < 0)
    {
        *(uint8_t *)pvBuf = RTZIPTYPE_ZLIB;
        if (pcbBuf)
            *pcbBuf = 1;
        pInflateState->iOffset = 0;
        return VINF_SUCCESS;
    }
    cbBuf = RT_MIN(cbBuf, pInflateState->cbSize);
    int rc = vdIfIoIntFileReadSync(pInflateState->pImage->pIfIo,
                                   pInflateState->pImage->pStorage,
                                   pInflateState->uFileOffset,
                                   pvBuf, cbBuf);
    if (RT_FAILURE(rc))
        return rc;
    pInflateState->uFileOffset += cbBuf;
    pInflateState->iOffset += cbBuf;
    pInflateState->cbSize -= cbBuf;
    Assert(pcbBuf);
    *pcbBuf = cbBuf;
    return VINF_SUCCESS;
}

/**
 * Internal: read from a file and inflate the compressed data,
 * distinguishing between async and normal operation
 */
DECLINLINE(int) dmgFileInflateSync(PDMGIMAGE pImage, uint64_t uOffset, size_t cbToRead,
                                   void *pvBuf, size_t cbBuf)
{
    int rc;
    PRTZIPDECOMP pZip = NULL;
    DMGINFLATESTATE InflateState;
    size_t cbActuallyRead;

    InflateState.pImage      = pImage;
    InflateState.cbSize      = cbToRead;
    InflateState.uFileOffset = uOffset;
    InflateState.iOffset     = -1;

    rc = RTZipDecompCreate(&pZip, &InflateState, dmgFileInflateHelper);
    if (RT_FAILURE(rc))
        return rc;
    rc = RTZipDecompress(pZip, pvBuf, cbBuf, &cbActuallyRead);
    RTZipDecompDestroy(pZip);
    if (RT_FAILURE(rc))
        return rc;
    if (cbActuallyRead != cbBuf)
        rc = VERR_VD_VMDK_INVALID_FORMAT;
    return rc;
}

/**
 * Swaps endian.
 * @param   pUdif       The structure.
 */
static void dmgSwapEndianUdif(PDMGUDIF pUdif)
{
#ifndef RT_BIG_ENDIAN
    pUdif->u32Magic   = RT_BSWAP_U32(pUdif->u32Magic);
    pUdif->u32Version = RT_BSWAP_U32(pUdif->u32Version);
    pUdif->cbFooter   = RT_BSWAP_U32(pUdif->cbFooter);
    pUdif->fFlags     = RT_BSWAP_U32(pUdif->fFlags);
    pUdif->offRunData = RT_BSWAP_U64(pUdif->offRunData);
    pUdif->offData    = RT_BSWAP_U64(pUdif->offData);
    pUdif->cbData     = RT_BSWAP_U64(pUdif->cbData);
    pUdif->offRsrc    = RT_BSWAP_U64(pUdif->offRsrc);
    pUdif->cbRsrc     = RT_BSWAP_U64(pUdif->cbRsrc);
    pUdif->iSegment   = RT_BSWAP_U32(pUdif->iSegment);
    pUdif->cSegments  = RT_BSWAP_U32(pUdif->cSegments);
    pUdif->offXml     = RT_BSWAP_U64(pUdif->offXml);
    pUdif->cbXml      = RT_BSWAP_U64(pUdif->cbXml);
    pUdif->u32Type    = RT_BSWAP_U32(pUdif->u32Type);
    pUdif->cSectors   = RT_BSWAP_U64(pUdif->cSectors);
#endif
}


/**
 * Swaps endian from host cpu to file.
 * @param   pUdif       The structure.
 */
static void dmgUdifFtrHost2FileEndian(PDMGUDIF pUdif)
{
    dmgSwapEndianUdif(pUdif);
    dmgUdifIdHost2FileEndian(&pUdif->SegmentId);
    dmgUdifCkSumHost2FileEndian(&pUdif->DataCkSum);
    dmgUdifCkSumHost2FileEndian(&pUdif->MasterCkSum);
}


/**
 * Swaps endian from file to host cpu.
 * @param   pUdif       The structure.
 */
static void dmgUdifFtrFile2HostEndian(PDMGUDIF pUdif)
{
    dmgSwapEndianUdif(pUdif);
    dmgUdifIdFile2HostEndian(&pUdif->SegmentId);
    dmgUdifCkSumFile2HostEndian(&pUdif->DataCkSum);
    dmgUdifCkSumFile2HostEndian(&pUdif->MasterCkSum);
}

/**
 * Swaps endian from file to host cpu.
 * @param   pBlkx       The blkx structure.
 */
static void dmgBlkxFile2HostEndian(PDMGBLKX pBlkx)
{
    pBlkx->u32Magic            = RT_BE2H_U32(pBlkx->u32Magic);
    pBlkx->u32Version          = RT_BE2H_U32(pBlkx->u32Version);
    pBlkx->cSectornumberFirst  = RT_BE2H_U64(pBlkx->cSectornumberFirst);
    pBlkx->cSectors            = RT_BE2H_U64(pBlkx->cSectors);
    pBlkx->offDataStart        = RT_BE2H_U64(pBlkx->offDataStart);
    pBlkx->cSectorsDecompress  = RT_BE2H_U32(pBlkx->cSectorsDecompress);
    pBlkx->u32BlocksDescriptor = RT_BE2H_U32(pBlkx->u32BlocksDescriptor);
    pBlkx->cBlocksRunCount     = RT_BE2H_U32(pBlkx->cBlocksRunCount);
    dmgUdifCkSumFile2HostEndian(&pBlkx->BlkxCkSum);
}

/**
 * Swaps endian from file to host cpu.
 * @param   pBlkxDesc   The blkx descriptor structure.
 */
static void dmgBlkxDescFile2HostEndian(PDMGBLKXDESC pBlkxDesc)
{
    pBlkxDesc->u32Type        = RT_BE2H_U32(pBlkxDesc->u32Type);
    pBlkxDesc->u32Reserved    = RT_BE2H_U32(pBlkxDesc->u32Reserved);
    pBlkxDesc->u64SectorStart = RT_BE2H_U64(pBlkxDesc->u64SectorStart);
    pBlkxDesc->u64SectorCount = RT_BE2H_U64(pBlkxDesc->u64SectorCount);
    pBlkxDesc->offData        = RT_BE2H_U64(pBlkxDesc->offData);
    pBlkxDesc->cbData         = RT_BE2H_U64(pBlkxDesc->cbData);
}

/**
 * Validates an UDIF footer structure.
 *
 * @returns true if valid, false and LogRel()s on failure.
 * @param   pFtr        The UDIF footer to validate.
 * @param   offFtr      The offset of the structure.
 */
static bool dmgUdifFtrIsValid(PCDMGUDIF pFtr, uint64_t offFtr)
{
    bool fRc = true;

    DMG_VALIDATE(!(pFtr->fFlags & ~DMGUDIF_FLAGS_KNOWN_MASK), ("fFlags=%#RX32 fKnown=%RX32\n", pFtr->fFlags, DMGUDIF_FLAGS_KNOWN_MASK));
    DMG_VALIDATE(pFtr->offRunData < offFtr, ("offRunData=%#RX64\n", pFtr->offRunData));
    DMG_VALIDATE(pFtr->cbData    <= offFtr && pFtr->offData + pFtr->cbData <= offFtr, ("cbData=%#RX64 offData=%#RX64 offFtr=%#RX64\n", pFtr->cbData, pFtr->offData, offFtr));
    DMG_VALIDATE(pFtr->offData    < offFtr, ("offData=%#RX64\n", pFtr->offData));
    DMG_VALIDATE(pFtr->cbRsrc    <= offFtr && pFtr->offRsrc + pFtr->cbRsrc <= offFtr, ("cbRsrc=%#RX64 offRsrc=%#RX64 offFtr=%#RX64\n", pFtr->cbRsrc, pFtr->offRsrc, offFtr));
    DMG_VALIDATE(pFtr->offRsrc    < offFtr, ("offRsrc=%#RX64\n", pFtr->offRsrc));
    DMG_VALIDATE(pFtr->cSegments <= 1,      ("cSegments=%RU32\n", pFtr->cSegments));
    DMG_VALIDATE(pFtr->iSegment  == 0 || pFtr->iSegment == 1, ("iSegment=%RU32 cSegments=%RU32\n", pFtr->iSegment, pFtr->cSegments));
    DMG_VALIDATE(pFtr->cbXml    <= offFtr && pFtr->offXml + pFtr->cbXml <= offFtr, ("cbXml=%#RX64 offXml=%#RX64 offFtr=%#RX64\n", pFtr->cbXml, pFtr->offXml, offFtr));
    DMG_VALIDATE(pFtr->offXml    < offFtr,  ("offXml=%#RX64\n", pFtr->offXml));
    DMG_VALIDATE(pFtr->cbXml     > 128,     ("cbXml=%#RX64\n", pFtr->cbXml));
    DMG_VALIDATE(pFtr->cbXml     < 10 * _1M,     ("cbXml=%#RX64\n", pFtr->cbXml));
    DMG_VALIDATE(pFtr->u32Type == DMGUDIF_TYPE_DEVICE || pFtr->u32Type == DMGUDIF_TYPE_PARTITION,  ("u32Type=%RU32\n", pFtr->u32Type));
    DMG_VALIDATE(pFtr->cSectors != 0,       ("cSectors=%#RX64\n", pFtr->cSectors));
    fRc &= dmgUdifCkSumIsValid(&pFtr->DataCkSum, "DataCkSum");
    fRc &= dmgUdifCkSumIsValid(&pFtr->MasterCkSum, "MasterCkSum");

    return fRc;
}


static bool dmgBlkxIsValid(PCDMGBLKX pBlkx)
{
    bool fRc = true;

    fRc &= dmgUdifCkSumIsValid(&pBlkx->BlkxCkSum, "BlkxCkSum");
    DMG_VALIDATE(pBlkx->u32Magic == DMGBLKX_MAGIC, ("u32Magic=%#RX32 u32MagicExpected=%#RX32\n", pBlkx->u32Magic, DMGBLKX_MAGIC));
    DMG_VALIDATE(pBlkx->u32Version == DMGBLKX_VERSION, ("u32Version=%#RX32 u32VersionExpected=%#RX32\n", pBlkx->u32Magic, DMGBLKX_VERSION));

    return fRc;
}

/**
 * Swaps endian from host cpu to file.
 * @param   pId         The structure.
 */
static void dmgUdifIdHost2FileEndian(PDMGUDIFID pId)
{
    NOREF(pId);
}


/**
 * Swaps endian from file to host cpu.
 * @param   pId         The structure.
 */
static void dmgUdifIdFile2HostEndian(PDMGUDIFID pId)
{
    dmgUdifIdHost2FileEndian(pId);
}


/**
 * Swaps endian.
 * @param   pCkSum      The structure.
 */
static void dmgSwapEndianUdifCkSum(PDMGUDIFCKSUM pCkSum, uint32_t u32Kind, uint32_t cBits)
{
#ifdef RT_BIG_ENDIAN
    NOREF(pCkSum);
    NOREF(u32Kind);
    NOREF(cBits);
#else
    switch (u32Kind)
    {
        case DMGUDIFCKSUM_NONE:
            /* nothing to do here */
            break;

        case DMGUDIFCKSUM_CRC32:
            Assert(cBits == 32);
            pCkSum->u32Kind      = RT_BSWAP_U32(pCkSum->u32Kind);
            pCkSum->cBits        = RT_BSWAP_U32(pCkSum->cBits);
            pCkSum->uSum.au32[0] = RT_BSWAP_U32(pCkSum->uSum.au32[0]);
            break;

        default:
            AssertMsgFailed(("%x\n", u32Kind));
            break;
    }
    NOREF(cBits);
#endif
}


/**
 * Swaps endian from host cpu to file.
 * @param   pCkSum      The structure.
 */
static void dmgUdifCkSumHost2FileEndian(PDMGUDIFCKSUM pCkSum)
{
    dmgSwapEndianUdifCkSum(pCkSum, pCkSum->u32Kind, pCkSum->cBits);
}


/**
 * Swaps endian from file to host cpu.
 * @param   pCkSum      The structure.
 */
static void dmgUdifCkSumFile2HostEndian(PDMGUDIFCKSUM pCkSum)
{
    dmgSwapEndianUdifCkSum(pCkSum, RT_BE2H_U32(pCkSum->u32Kind), RT_BE2H_U32(pCkSum->cBits));
}


/**
 * Validates an UDIF checksum structure.
 *
 * @returns true if valid, false and LogRel()s on failure.
 * @param   pCkSum      The checksum structure.
 * @param   pszPrefix   The message prefix.
 * @remarks This does not check the checksummed data.
 */
static bool dmgUdifCkSumIsValid(PCDMGUDIFCKSUM pCkSum, const char *pszPrefix)
{
    bool fRc = true;

    switch (pCkSum->u32Kind)
    {
        case DMGUDIFCKSUM_NONE:
            DMG_VALIDATE(pCkSum->cBits == 0, ("%s/NONE: cBits=%d\n", pszPrefix, pCkSum->cBits));
            break;

        case DMGUDIFCKSUM_CRC32:
            DMG_VALIDATE(pCkSum->cBits == 32, ("%s/NONE: cBits=%d\n", pszPrefix, pCkSum->cBits));
            break;

        default:
            DMG_VALIDATE(0, ("%s: u32Kind=%#RX32\n", pszPrefix, pCkSum->u32Kind));
            break;
    }
    return fRc;
}


/**
 * Internal. Flush image data to disk.
 */
static int dmgFlushImage(PDMGIMAGE pThis)
{
    int rc = VINF_SUCCESS;

    if (   pThis->pStorage
        && !(pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY))
    {
        /* @todo handle writable files, update checksums etc. */
    }

    return rc;
}


/**
 * Internal. Free all allocated space for representing an image except pThis,
 * and optionally delete the image from disk.
 */
static int dmgFreeImage(PDMGIMAGE pThis, bool fDelete)
{
    int rc = VINF_SUCCESS;

    /* Freeing a never allocated image (e.g. because the open failed) is
     * not signalled as an error. After all nothing bad happens. */
    if (pThis)
    {
        if (pThis->pStorage)
        {
            /* No point updating the file that is deleted anyway. */
            if (!fDelete)
                dmgFlushImage(pThis);

            vdIfIoIntFileClose(pThis->pIfIo, pThis->pStorage);
            pThis->pStorage = NULL;
        }

        for (unsigned iRsrc = 0; iRsrc < RT_ELEMENTS(pThis->aRsrcs); iRsrc++)
            for (unsigned i = 0; i < pThis->aRsrcs[iRsrc].cEntries; i++)
            {
                if (pThis->aRsrcs[iRsrc].aEntries[i].pbData)
                {
                    RTMemFree(pThis->aRsrcs[iRsrc].aEntries[i].pbData);
                    pThis->aRsrcs[iRsrc].aEntries[i].pbData = NULL;
                }
                if (pThis->aRsrcs[iRsrc].aEntries[i].pszName)
                {
                    RTMemFree(pThis->aRsrcs[iRsrc].aEntries[i].pszName);
                    pThis->aRsrcs[iRsrc].aEntries[i].pszName = NULL;
                }
                if (pThis->aRsrcs[iRsrc].aEntries[i].pszCFName)
                {
                    RTMemFree(pThis->aRsrcs[iRsrc].aEntries[i].pszCFName);
                    pThis->aRsrcs[iRsrc].aEntries[i].pszCFName = NULL;
                }
            }

        if (fDelete && pThis->pszFilename)
            vdIfIoIntFileDelete(pThis->pIfIo, pThis->pszFilename);

        if (pThis->pvDecompExtent)
        {
            RTMemFree(pThis->pvDecompExtent);
            pThis->pvDecompExtent = NULL;
            pThis->cbDecompExtent = 0;
        }

    }

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}


#define STARTS_WITH(pszString, szStart) \
        ( strncmp(pszString, szStart, sizeof(szStart) - 1) == 0 )

#define STARTS_WITH_WORD(pszString, szWord) \
        (   STARTS_WITH(pszString, szWord) \
         && !RT_C_IS_ALNUM((pszString)[sizeof(szWord) - 1]) )

#define SKIP_AHEAD(psz, szWord) \
        do { \
            (psz) = RTStrStripL((psz) + sizeof(szWord) - 1); \
        } while (0)

#define REQUIRE_WORD(psz, szWord) \
        do { \
            if (!STARTS_WITH_WORD(psz, szWord)) \
                return psz; \
            (psz) = RTStrStripL((psz) + sizeof(szWord) - 1); \
        } while (0)

#define REQUIRE_TAG(psz, szTag) \
        do { \
            if (!STARTS_WITH(psz, "<" szTag ">")) \
                return psz; \
            (psz) = RTStrStripL((psz) + sizeof("<" szTag ">") - 1); \
        } while (0)

#define REQUIRE_TAG_NO_STRIP(psz, szTag) \
        do { \
            if (!STARTS_WITH(psz, "<" szTag ">")) \
                return psz; \
            (psz) += sizeof("<" szTag ">") - 1; \
        } while (0)

#define REQUIRE_END_TAG(psz, szTag) \
        do { \
            if (!STARTS_WITH(psz, "</" szTag ">")) \
                return psz; \
            (psz) = RTStrStripL((psz) + sizeof("</" szTag ">") - 1); \
        } while (0)


/**
 * Finds the next tag end.
 *
 * @returns Pointer to a '>' or '\0'.
 * @param   pszCur      The current position.
 */
static const char *dmgXmlFindTagEnd(const char *pszCur)
{
    /* Might want to take quoted '>' into account? */
    char ch;
    while ((ch = *pszCur) != '\0' && ch != '>')
        pszCur++;
    return pszCur;
}


/**
 * Finds the end tag.
 *
 * Does not deal with '<tag attr="1"/>' style tags.
 *
 * @returns Pointer to the first char in the end tag. NULL if another tag
 *          was encountered first or if we hit the end of the file.
 * @param   ppszCur     The current position (IN/OUT).
 * @param   pszTag      The tag name.
 */
static const char *dmgXmlFindEndTag(const char **ppszCur, const char *pszTag)
{
    const char         *psz = *ppszCur;
    char                ch;
    while ((ch = *psz))
    {
        if (ch == '<')
        {
            size_t const cchTag = strlen(pszTag);
            if (    psz[1] == '/'
                &&  !memcmp(&psz[2], pszTag, cchTag)
                &&  psz[2 + cchTag] == '>')
            {
                *ppszCur = psz + 2 + cchTag + 1;
                return psz;
            }
            break;
        }
        psz++;
    }
    return NULL;
}


/**
 * Reads a signed 32-bit value.
 *
 * @returns NULL on success, pointer to the offending text on failure.
 * @param   ppszCur     The text position (IN/OUT).
 * @param   pi32        Where to store the value.
 */
static const char *dmgXmlParseS32(const char **ppszCur, int32_t *pi32)
{
    const char *psz = *ppszCur;

    /*
     * <string>-1</string>
     */
    REQUIRE_TAG_NO_STRIP(psz, "string");

    char *pszNext;
    int rc = RTStrToInt32Ex(psz, &pszNext, 0, pi32);
    if (rc != VWRN_TRAILING_CHARS)
        return *ppszCur;
    psz = pszNext;

    REQUIRE_END_TAG(psz, "string");
    *ppszCur = psz;
    return NULL;
}


/**
 * Reads an unsigned 32-bit value.
 *
 * @returns NULL on success, pointer to the offending text on failure.
 * @param   ppszCur     The text position (IN/OUT).
 * @param   pu32        Where to store the value.
 */
static const char *dmgXmlParseU32(const char **ppszCur, uint32_t *pu32)
{
    const char *psz = *ppszCur;

    /*
     * <string>0x00ff</string>
     */
    REQUIRE_TAG_NO_STRIP(psz, "string");

    char *pszNext;
    int rc = RTStrToUInt32Ex(psz, &pszNext, 0, pu32);
    if (rc != VWRN_TRAILING_CHARS)
        return *ppszCur;
    psz = pszNext;

    REQUIRE_END_TAG(psz, "string");
    *ppszCur = psz;
    return NULL;
}


/**
 * Reads a string value.
 *
 * @returns NULL on success, pointer to the offending text on failure.
 * @param   ppszCur     The text position (IN/OUT).
 * @param   ppszString  Where to store the pointer to the string. The caller
 *                      must free this using RTMemFree.
 */
static const char *dmgXmlParseString(const char **ppszCur, char **ppszString)
{
    const char *psz = *ppszCur;

    /*
     * <string>Driver Descriptor Map (DDM : 0)</string>
     */
    REQUIRE_TAG_NO_STRIP(psz, "string");

    const char *pszStart = psz;
    const char *pszEnd = dmgXmlFindEndTag(&psz, "string");
    if (!pszEnd)
        return *ppszCur;
    psz = RTStrStripL(psz);

    *ppszString = (char *)RTMemDupEx(pszStart, pszEnd - pszStart, 1);
    if (!*ppszString)
        return *ppszCur;

    *ppszCur = psz;
    return NULL;
}


/**
 * Parses the BASE-64 coded data tags.
 *
 * @returns NULL on success, pointer to the offending text on failure.
 * @param   ppszCur     The text position (IN/OUT).
 * @param   ppbData     Where to store the pointer to the data we've read. The
 *                      caller must free this using RTMemFree.
 * @param   pcbData     The number of bytes we're returning.
 */
static const char *dmgXmlParseData(const char **ppszCur, uint8_t **ppbData, size_t *pcbData)
{
    const char *psz = *ppszCur;

    /*
     * <data>   AAAAA...    </data>
     */
    REQUIRE_TAG(psz, "data");

    const char *pszStart = psz;
    ssize_t cbData = RTBase64DecodedSize(pszStart, (char **)&psz);
    if (cbData == -1)
        return *ppszCur;
    const char *pszEnd = psz;

    REQUIRE_END_TAG(psz, "data");

    *ppbData = (uint8_t *)RTMemAlloc(cbData);
    if (!*ppbData)
        return *ppszCur;
    char *pszIgnored;
    int rc = RTBase64Decode(pszStart, *ppbData, cbData, pcbData, &pszIgnored);
    if (RT_FAILURE(rc))
    {
        RTMemFree(*ppbData);
        *ppbData = NULL;
        return *ppszCur;
    }

    *ppszCur = psz;
    return NULL;
}


/**
 * Parses the XML resource-fork in a rather presumptive manner.
 *
 * This function is supposed to construct the DMG::aRsrcs instance data
 * parts.
 *
 * @returns NULL on success, pointer to the problematic text on failure.
 * @param   pThis       The DMG instance data.
 * @param   pszXml      The XML text to parse, UTF-8.
 * @param   cch         The size of the XML text.
 */
static const char *dmgOpenXmlToRsrc(PDMGIMAGE pThis, char const *pszXml)
{
    const char *psz = pszXml;

    /*
     * Verify the ?xml, !DOCTYPE and plist tags.
     */
    SKIP_AHEAD(psz, "");

    /* <?xml version="1.0" encoding="UTF-8"?> */
    REQUIRE_WORD(psz, "<?xml");
    while (*psz != '?')
    {
        if (!*psz)
            return psz;
        if (STARTS_WITH_WORD(psz, "version="))
        {
            SKIP_AHEAD(psz, "version=");
            REQUIRE_WORD(psz, "\"1.0\"");
        }
        else if (STARTS_WITH_WORD(psz, "encoding="))
        {
            SKIP_AHEAD(psz, "encoding=");
            REQUIRE_WORD(psz, "\"UTF-8\"");
        }
        else
            return psz;
    }
    SKIP_AHEAD(psz, "?>");

    /* <!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd"> */
    REQUIRE_WORD(psz, "<!DOCTYPE");
    REQUIRE_WORD(psz, "plist");
    REQUIRE_WORD(psz, "PUBLIC");
    psz = dmgXmlFindTagEnd(psz);
    REQUIRE_WORD(psz, ">");

    /* <plist version="1.0"> */
    REQUIRE_WORD(psz, "<plist");
    REQUIRE_WORD(psz, "version=");
    REQUIRE_WORD(psz, "\"1.0\"");
    REQUIRE_WORD(psz, ">");

    /*
     * Descend down to the 'resource-fork' dictionary.
     * ASSUME it's the only top level dictionary.
     */
    /* <dict> <key>resource-fork</key> */
    REQUIRE_TAG(psz, "dict");
    REQUIRE_WORD(psz, "<key>resource-fork</key>");

    /*
     * Parse the keys in the resource-fork dictionary.
     * ASSUME that there are just two, 'blkx' and 'plst'.
     */
    REQUIRE_TAG(psz, "dict");
    while (!STARTS_WITH_WORD(psz, "</dict>"))
    {
        /*
         * Parse the key and Create the resource-fork entry.
         */
        unsigned iRsrc;
        if (STARTS_WITH_WORD(psz, "<key>blkx</key>"))
        {
            REQUIRE_WORD(psz, "<key>blkx</key>");
            iRsrc = DMG_RSRC_IDX_BLKX;
            strcpy(&pThis->aRsrcs[iRsrc].szName[0], "blkx");
        }
        else if (STARTS_WITH_WORD(psz, "<key>plst</key>"))
        {
            REQUIRE_WORD(psz, "<key>plst</key>");
            iRsrc = DMG_RSRC_IDX_PLST;
            strcpy(&pThis->aRsrcs[iRsrc].szName[0], "plst");
        }
        else
        {
            SKIP_AHEAD(psz, "</array>");
            continue;
        }


        /*
         * Descend into the array and add the elements to the resource entry.
         */
        /* <array> */
        REQUIRE_TAG(psz, "array");
        while (!STARTS_WITH_WORD(psz, "</array>"))
        {
            REQUIRE_TAG(psz, "dict");
            uint32_t i = pThis->aRsrcs[iRsrc].cEntries;
            if (i == RT_ELEMENTS(pThis->aRsrcs[iRsrc].aEntries))
                return psz;

            while (!STARTS_WITH_WORD(psz, "</dict>"))
            {

                /* switch on the key. */
                const char *pszErr;
                if (STARTS_WITH_WORD(psz, "<key>Attributes</key>"))
                {
                    REQUIRE_WORD(psz, "<key>Attributes</key>");
                    pszErr = dmgXmlParseU32(&psz, &pThis->aRsrcs[iRsrc].aEntries[i].fAttributes);
                }
                else if (STARTS_WITH_WORD(psz, "<key>ID</key>"))
                {
                    REQUIRE_WORD(psz, "<key>ID</key>");
                    pszErr = dmgXmlParseS32(&psz, &pThis->aRsrcs[iRsrc].aEntries[i].iId);
                }
                else if (STARTS_WITH_WORD(psz, "<key>Name</key>"))
                {
                    REQUIRE_WORD(psz, "<key>Name</key>");
                    pszErr = dmgXmlParseString(&psz, &pThis->aRsrcs[iRsrc].aEntries[i].pszName);
                }
                else if (STARTS_WITH_WORD(psz, "<key>CFName</key>"))
                {
                    REQUIRE_WORD(psz, "<key>CFName</key>");
                    pszErr = dmgXmlParseString(&psz, &pThis->aRsrcs[iRsrc].aEntries[i].pszCFName);
                }
                else if (STARTS_WITH_WORD(psz, "<key>Data</key>"))
                {
                    REQUIRE_WORD(psz, "<key>Data</key>");
                    pszErr = dmgXmlParseData(&psz, &pThis->aRsrcs[iRsrc].aEntries[i].pbData, &pThis->aRsrcs[iRsrc].aEntries[i].cbData);
                }
                else
                    pszErr = psz;
                if (pszErr)
                    return pszErr;
            } /* while not </dict> */
            REQUIRE_END_TAG(psz, "dict");

            pThis->aRsrcs[iRsrc].cEntries++;
        } /* while not </array> */
        REQUIRE_END_TAG(psz, "array");

    } /* while not </dict> */
    REQUIRE_END_TAG(psz, "dict");

    /*
     * ASSUMING there is only the 'resource-fork', we'll now see the end of
     * the outer dict, plist and text.
     */
    /* </dict> </plist> */
    REQUIRE_END_TAG(psz, "dict");
    REQUIRE_END_TAG(psz, "plist");

    /* the end */
    if (*psz)
        return psz;

    return NULL;
}

#undef REQUIRE_END_TAG
#undef REQUIRE_TAG_NO_STRIP
#undef REQUIRE_TAG
#undef REQUIRE_WORD
#undef SKIP_AHEAD
#undef STARTS_WITH_WORD
#undef STARTS_WITH

/**
 * Returns the data attached to a resource.
 *
 * @returns VBox status code.
 * @param   pThis        The DMG instance data.
 * @param   pcszRsrcName Name of the resource to get.
 */
static int dmgGetRsrcData(PDMGIMAGE pThis, const char *pcszRsrcName,
                          PCDMGUDIFRSRCARRAY *ppcRsrc)
{
    int rc = VERR_NOT_FOUND;

    for (unsigned i = 0; i < RT_ELEMENTS(pThis->aRsrcs); i++)
    {
        if (!strcmp(pThis->aRsrcs[i].szName, pcszRsrcName))
        {
            *ppcRsrc = &pThis->aRsrcs[i];
            rc = VINF_SUCCESS;
            break;
        }
    }

    return rc;
}

/**
 * Creates a new extent from the given blkx descriptor.
 *
 * @returns VBox status code.
 * @param   pThis          DMG instance data.
 * @param   uSectorPart    First sector the partition owning the blkx descriptor has.
 * @param   pBlkxDesc      The blkx descriptor.
 */
static int dmgExtentCreateFromBlkxDesc(PDMGIMAGE pThis, uint64_t uSectorPart, PDMGBLKXDESC pBlkxDesc)
{
    int rc = VINF_SUCCESS;
    DMGEXTENTTYPE enmExtentTypeNew;
    PDMGEXTENT pExtentNew = NULL;

    if (pBlkxDesc->u32Type == DMGBLKXDESC_TYPE_RAW)
        enmExtentTypeNew = DMGEXTENTTYPE_RAW;
    else if (pBlkxDesc->u32Type == DMGBLKXDESC_TYPE_IGNORE)
        enmExtentTypeNew = DMGEXTENTTYPE_ZERO;
    else if (pBlkxDesc->u32Type == DMGBLKXDESC_TYPE_ZLIB)
        enmExtentTypeNew = DMGEXTENTTYPE_COMP_ZLIB;
    else
    {
        AssertMsgFailed(("This method supports only raw or zero extents!\n"));
        return VERR_NOT_SUPPORTED;
    }

    /** @todo: Merge raw extents if possible to save memory. */
#if 0
    pExtentNew = pThis->pExtentLast;
    if (   pExtentNew
        && pExtentNew->enmType == enmExtentTypeNew
        && enmExtentTypeNew == DMGEXTENTTYPE_RAW
        && pExtentNew->uSectorExtent + pExtentNew->cSectorsExtent == offDevice + pBlkxDesc->u64SectorStart * DMG_SECTOR_SIZE;
        && pExtentNew->offFileStart + pExtentNew->cbExtent == pBlkxDesc->offData)
    {
        /* Increase the last extent. */
        pExtentNew->cbExtent += pBlkxDesc->cbData;
    }
    else
#endif
    {
        if (pThis->cExtentsMax == pThis->cExtents)
        {
            pThis->cExtentsMax += 64;

            /* Increase the array. */
            PDMGEXTENT paExtentsNew = (PDMGEXTENT)RTMemRealloc(pThis->paExtents, sizeof(DMGEXTENT) * pThis->cExtentsMax);
            if (!paExtentsNew)
            {
                rc = VERR_NO_MEMORY;
                pThis->cExtentsMax -= 64;
            }
            else
                pThis->paExtents = paExtentsNew;
        }

        if (RT_SUCCESS(rc))
        {
            pExtentNew = &pThis->paExtents[pThis->cExtents++];

            pExtentNew->enmType        = enmExtentTypeNew;
            pExtentNew->uSectorExtent  = uSectorPart + pBlkxDesc->u64SectorStart;
            pExtentNew->cSectorsExtent = pBlkxDesc->u64SectorCount;
            pExtentNew->offFileStart   = pBlkxDesc->offData;
            pExtentNew->cbFile         = pBlkxDesc->cbData;
        }
    }

    return rc;
}

/**
 * Find the extent for the given sector number.
 */
static PDMGEXTENT dmgExtentGetFromOffset(PDMGIMAGE pThis, uint64_t uSector)
{
    /*
     * We assume that the array is ordered from lower to higher sector
     * numbers.
     * This makes it possible to bisect the array to find the extent
     * faster than using a linked list.
     */
    PDMGEXTENT pExtent = NULL;
    unsigned idxCur = pThis->idxExtentLast;
    unsigned idxMax = pThis->cExtents;
    unsigned idxMin = 0;

    while (idxMin < idxMax)
    {
        PDMGEXTENT pExtentCur = &pThis->paExtents[idxCur];

        /* Determine the search direction. */
        if (uSector < pExtentCur->uSectorExtent)
        {
            /* Search left from the current extent. */
            idxMax = idxCur;
        }
        else if (uSector >= pExtentCur->uSectorExtent + pExtentCur->cSectorsExtent)
        {
            /* Search right from the current extent. */
            idxMin = idxCur;
        }
        else
        {
            /* The sector lies in the extent, stop searching. */
            pExtent = pExtentCur;
            break;
        }

        idxCur = idxMin + (idxMax - idxMin) / 2;
    }

    if (pExtent)
        pThis->idxExtentLast = idxCur;

    return pExtent;
}

/**
 * Goes through the BLKX structure and creates the necessary extents.
 */
static int dmgBlkxParse(PDMGIMAGE pThis, PDMGBLKX pBlkx)
{
    int rc = VINF_SUCCESS;
    PDMGBLKXDESC pBlkxDesc = (PDMGBLKXDESC)(pBlkx + 1);

    for (unsigned i = 0; i < pBlkx->cBlocksRunCount; i++)
    {
        dmgBlkxDescFile2HostEndian(pBlkxDesc);

        switch (pBlkxDesc->u32Type)
        {
            case DMGBLKXDESC_TYPE_RAW:
            case DMGBLKXDESC_TYPE_IGNORE:
            case DMGBLKXDESC_TYPE_ZLIB:
            {
                rc = dmgExtentCreateFromBlkxDesc(pThis, pBlkx->cSectornumberFirst, pBlkxDesc);
                break;
            }
            case DMGBLKXDESC_TYPE_COMMENT:
            case DMGBLKXDESC_TYPE_TERMINATOR:
                break;
            default:
                rc = VERR_VD_DMG_INVALID_HEADER;
                break;
        }

        if (   pBlkxDesc->u32Type == DMGBLKXDESC_TYPE_TERMINATOR
            || RT_FAILURE(rc))
                break;

        pBlkxDesc++;
    }

    return rc;
}

/**
 * Worker for dmgOpen that reads in and validates all the necessary
 * structures from the image.
 *
 * @returns VBox status code.
 * @param   pThis       The DMG instance data.
 * @param   uOpenFlags  Flags for defining the open type.
 */
static int dmgOpenImage(PDMGIMAGE pThis, unsigned uOpenFlags)
{
    pThis->uOpenFlags  = uOpenFlags;

    pThis->pIfError = VDIfErrorGet(pThis->pVDIfsDisk);
    pThis->pIfIo = VDIfIoIntGet(pThis->pVDIfsImage);
    AssertPtrReturn(pThis->pIfIo, VERR_INVALID_PARAMETER);

    int rc = vdIfIoIntFileOpen(pThis->pIfIo, pThis->pszFilename,
                               VDOpenFlagsToFileOpenFlags(uOpenFlags,
                                                          false /* fCreate */),
                               &pThis->pStorage);
    if (RT_FAILURE(rc))
    {
        /* Do NOT signal an appropriate error here, as the VD layer has the
         * choice of retrying the open if it failed. */
        return rc;
    }

    /*
     * Read the footer.
     */
    rc = vdIfIoIntFileGetSize(pThis->pIfIo, pThis->pStorage, &pThis->cbFile);
    if (RT_FAILURE(rc))
        return rc;
    if (pThis->cbFile < 1024)
        return VERR_VD_DMG_INVALID_HEADER;
    rc = vdIfIoIntFileReadSync(pThis->pIfIo, pThis->pStorage,
                               pThis->cbFile - sizeof(pThis->Ftr),
                               &pThis->Ftr, sizeof(pThis->Ftr));
    if (RT_FAILURE(rc))
        return rc;
    dmgUdifFtrFile2HostEndian(&pThis->Ftr);

    /*
     * Do we recognize the footer structure? If so, is it valid?
     */
    if (pThis->Ftr.u32Magic != DMGUDIF_MAGIC)
        return VERR_VD_DMG_INVALID_HEADER;
    if (pThis->Ftr.u32Version != DMGUDIF_VER_CURRENT)
        return VERR_VD_DMG_INVALID_HEADER;
    if (pThis->Ftr.cbFooter != sizeof(pThis->Ftr))
        return VERR_VD_DMG_INVALID_HEADER;

    if (!dmgUdifFtrIsValid(&pThis->Ftr, pThis->cbFile - sizeof(pThis->Ftr)))
    {
        DMG_PRINTF(("Bad DMG: '%s' cbFile=%RTfoff\n", pThis->pszFilename, pThis->cbFile));
        return VERR_VD_DMG_INVALID_HEADER;
    }

    pThis->cbSize = pThis->Ftr.cSectors * DMG_SECTOR_SIZE;

    /*
     * Read and parse the XML portion.
     */
    size_t cchXml = (size_t)pThis->Ftr.cbXml;
    char *pszXml = (char *)RTMemAlloc(cchXml + 1);
    if (!pszXml)
        return VERR_NO_MEMORY;
    rc = vdIfIoIntFileReadSync(pThis->pIfIo, pThis->pStorage, pThis->Ftr.offXml,
                               pszXml, cchXml);
    if (RT_SUCCESS(rc))
    {
        pszXml[cchXml] = '\0';
        const char *pszError = dmgOpenXmlToRsrc(pThis, pszXml);
        if (!pszError)
        {
            PCDMGUDIFRSRCARRAY pRsrcBlkx = NULL;

            rc = dmgGetRsrcData(pThis, "blkx", &pRsrcBlkx);
            if (RT_SUCCESS(rc))
            {
                for (unsigned idxBlkx = 0; idxBlkx < pRsrcBlkx->cEntries; idxBlkx++)
                {
                    PDMGBLKX pBlkx = NULL;

                    if (pRsrcBlkx->aEntries[idxBlkx].cbData < sizeof(DMGBLKX))
                    {
                        rc = VERR_VD_DMG_INVALID_HEADER;
                        break;
                    }

                    pBlkx = (PDMGBLKX)RTMemAllocZ(pRsrcBlkx->aEntries[idxBlkx].cbData);
                    if (!pBlkx)
                    {
                        rc = VERR_NO_MEMORY;
                        break;
                    }

                    memcpy(pBlkx, pRsrcBlkx->aEntries[idxBlkx].pbData, pRsrcBlkx->aEntries[idxBlkx].cbData);

                    dmgBlkxFile2HostEndian(pBlkx);

                    if (   dmgBlkxIsValid(pBlkx)
                        && pRsrcBlkx->aEntries[idxBlkx].cbData == pBlkx->cBlocksRunCount * sizeof(DMGBLKXDESC) + sizeof(DMGBLKX))
                        rc = dmgBlkxParse(pThis, pBlkx);
                    else
                        rc = VERR_VD_DMG_INVALID_HEADER;

                    RTMemFree(pBlkx);

                    if (RT_FAILURE(rc))
                        break;
                }
            }
            else
                rc = VERR_VD_DMG_INVALID_HEADER;
        }
        else
        {
            DMG_PRINTF(("**** XML DUMP BEGIN ***\n%s\n**** XML DUMP END ****\n", pszXml));
            DMG_PRINTF(("**** Bad XML at %#lx (%lu) ***\n%.256s\n**** Bad XML END ****\n",
                            (unsigned long)(pszError - pszXml), (unsigned long)(pszError - pszXml), pszError));
            rc = VERR_VD_DMG_XML_PARSE_ERROR;
        }
    }
    RTMemFree(pszXml);

    if (RT_FAILURE(rc))
        dmgFreeImage(pThis, false);
    return rc;
}


/** @copydoc VBOXHDDBACKEND::pfnCheckIfValid */
static int dmgCheckIfValid(const char *pszFilename, PVDINTERFACE pVDIfsDisk,
                           PVDINTERFACE pVDIfsImage, VDTYPE *penmType)
{
    LogFlowFunc(("pszFilename=\"%s\" pVDIfsDisk=%#p pVDIfsImage=%#p penmType=%#p\n",
                 pszFilename, pVDIfsDisk, pVDIfsImage, penmType));
    int rc;
    PVDIOSTORAGE pStorage;
    uint64_t cbFile, offFtr = 0;
    DMGUDIF Ftr;

    PVDINTERFACEIOINT pIfIo = VDIfIoIntGet(pVDIfsImage);
    AssertPtrReturn(pIfIo, VERR_INVALID_PARAMETER);

    /*
     * Open the file and read the footer.
     */
    rc = vdIfIoIntFileOpen(pIfIo, pszFilename,
                           VDOpenFlagsToFileOpenFlags(VD_OPEN_FLAGS_READONLY,
                                                      false /* fCreate */),
                           &pStorage);
    if (RT_SUCCESS(rc))
        rc = vdIfIoIntFileGetSize(pIfIo, pStorage, &cbFile);
    if (RT_SUCCESS(rc))
    {
        offFtr = cbFile - sizeof(Ftr);
        rc = vdIfIoIntFileReadSync(pIfIo, pStorage, offFtr, &Ftr, sizeof(Ftr));
    }
    else
    {
        vdIfIoIntFileClose(pIfIo, pStorage);
        rc = VERR_VD_DMG_INVALID_HEADER;
    }

    if (RT_SUCCESS(rc))
    {
        /*
         * Do we recognize this stuff? Does it look valid?
         */
        if (    Ftr.u32Magic    == RT_H2BE_U32(DMGUDIF_MAGIC)
            &&  Ftr.u32Version  == RT_H2BE_U32(DMGUDIF_VER_CURRENT)
            &&  Ftr.cbFooter    == RT_H2BE_U32(sizeof(Ftr)))
        {
            dmgUdifFtrFile2HostEndian(&Ftr);
            if (dmgUdifFtrIsValid(&Ftr, offFtr))
            {
                rc = VINF_SUCCESS;
                *penmType = VDTYPE_DVD;
            }
            else
            {
                DMG_PRINTF(("Bad DMG: '%s' offFtr=%RTfoff\n", pszFilename, offFtr));
                rc = VERR_VD_DMG_INVALID_HEADER;
            }
        }
        else
            rc = VERR_VD_DMG_INVALID_HEADER;
    }

    vdIfIoIntFileClose(pIfIo, pStorage);

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnOpen */
static int dmgOpen(const char *pszFilename, unsigned uOpenFlags,
                   PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                   VDTYPE enmType, void **ppBackendData)
{
    LogFlowFunc(("pszFilename=\"%s\" uOpenFlags=%#x pVDIfsDisk=%#p pVDIfsImage=%#p ppBackendData=%#p\n", pszFilename, uOpenFlags, pVDIfsDisk, pVDIfsImage, ppBackendData));
    int rc = VINF_SUCCESS;
    PDMGIMAGE pThis;

    /* Check open flags. All valid flags are (in principle) supported. */
    if (uOpenFlags & ~VD_OPEN_FLAGS_MASK)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Check remaining arguments. */
    if (   !VALID_PTR(pszFilename)
        || !*pszFilename)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /*
     * Reject combinations we don't currently support.
     *
     * There is no point in being paranoid about the input here as we're just a
     * simple backend and can expect the caller to be the only user and already
     * have validate what it passes thru to us.
     */
    if (   !(uOpenFlags & VD_OPEN_FLAGS_READONLY)
        || (uOpenFlags & VD_OPEN_FLAGS_ASYNC_IO))
    {
        rc = VERR_NOT_SUPPORTED;
        goto out;
    }

    /*
     * Create the basic instance data structure and open the file,
     * then hand it over to a worker function that does all the rest.
     */
    pThis = (PDMGIMAGE)RTMemAllocZ(sizeof(*pThis));
    if (!pThis)
    {
        rc = VERR_NO_MEMORY;
        goto out;
    }

    pThis->pszFilename = pszFilename;
    pThis->pStorage    = NULL;
    pThis->pVDIfsDisk  = pVDIfsDisk;
    pThis->pVDIfsImage = pVDIfsImage;

    rc = dmgOpenImage(pThis, uOpenFlags);
  if (RT_SUCCESS(rc))
        *ppBackendData = pThis;
    else
        RTMemFree(pThis);

out:
    LogFlowFunc(("returns %Rrc (pBackendData=%#p)\n", rc, *ppBackendData));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnCreate */
static int dmgCreate(const char *pszFilename, uint64_t cbSize,
                     unsigned uImageFlags, const char *pszComment,
                     PCVDGEOMETRY pPCHSGeometry, PCVDGEOMETRY pLCHSGeometry,
                     PCRTUUID pUuid, unsigned uOpenFlags,
                     unsigned uPercentStart, unsigned uPercentSpan,
                     PVDINTERFACE pVDIfsDisk, PVDINTERFACE pVDIfsImage,
                     PVDINTERFACE pVDIfsOperation, void **ppBackendData)
{
    LogFlowFunc(("pszFilename=\"%s\" cbSize=%llu uImageFlags=%#x pszComment=\"%s\" pPCHSGeometry=%#p pLCHSGeometry=%#p Uuid=%RTuuid uOpenFlags=%#x uPercentStart=%u uPercentSpan=%u pVDIfsDisk=%#p pVDIfsImage=%#p pVDIfsOperation=%#p ppBackendData=%#p", pszFilename, cbSize, uImageFlags, pszComment, pPCHSGeometry, pLCHSGeometry, pUuid, uOpenFlags, uPercentStart, uPercentSpan, pVDIfsDisk, pVDIfsImage, pVDIfsOperation, ppBackendData));
    int rc = VERR_NOT_SUPPORTED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnRename */
static int dmgRename(void *pBackendData, const char *pszFilename)
{
    LogFlowFunc(("pBackendData=%#p pszFilename=%#p\n", pBackendData, pszFilename));
    int rc = VERR_NOT_SUPPORTED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnClose */
static int dmgClose(void *pBackendData, bool fDelete)
{
    LogFlowFunc(("pBackendData=%#p fDelete=%d\n", pBackendData, fDelete));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    rc = dmgFreeImage(pThis, fDelete);
    RTMemFree(pThis);

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnRead */
static int dmgRead(void *pBackendData, uint64_t uOffset, void *pvBuf,
                   size_t cbToRead, size_t *pcbActuallyRead)
{
    LogFlowFunc(("pBackendData=%#p uOffset=%llu pvBuf=%#p cbToRead=%zu pcbActuallyRead=%#p\n", pBackendData, uOffset, pvBuf, cbToRead, pcbActuallyRead));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    PDMGEXTENT pExtent = NULL;
    int rc = VINF_SUCCESS;

    AssertPtr(pThis);
    Assert(uOffset % DMG_SECTOR_SIZE == 0);
    Assert(cbToRead % DMG_SECTOR_SIZE == 0);

    if (   uOffset + cbToRead > pThis->cbSize
        || cbToRead == 0)
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    pExtent = dmgExtentGetFromOffset(pThis, DMG_BYTE2BLOCK(uOffset));

    if (pExtent)
    {
        uint64_t uExtentRel = DMG_BYTE2BLOCK(uOffset) - pExtent->uSectorExtent;

        /* Remain in this extent. */
        cbToRead = RT_MIN(cbToRead, DMG_BLOCK2BYTE(pExtent->cSectorsExtent - uExtentRel));

        switch (pExtent->enmType)
        {
            case DMGEXTENTTYPE_RAW:
            {
                rc = vdIfIoIntFileReadSync(pThis->pIfIo, pThis->pStorage,
                                           pExtent->offFileStart + DMG_BLOCK2BYTE(uExtentRel),
                                           pvBuf, cbToRead);
                break;
            }
            case DMGEXTENTTYPE_ZERO:
            {
                memset(pvBuf, 0, cbToRead);
                break;
            }
            case DMGEXTENTTYPE_COMP_ZLIB:
            {
                if (pThis->pExtentDecomp != pExtent)
                {
                    if (DMG_BLOCK2BYTE(pExtent->cSectorsExtent) > pThis->cbDecompExtent)
                    {
                        if (RT_LIKELY(pThis->pvDecompExtent))
                            RTMemFree(pThis->pvDecompExtent);

                        pThis->pvDecompExtent = RTMemAllocZ(DMG_BLOCK2BYTE(pExtent->cSectorsExtent));
                        if (!pThis->pvDecompExtent)
                            rc = VERR_NO_MEMORY;
                        else
                            pThis->cbDecompExtent = DMG_BLOCK2BYTE(pExtent->cSectorsExtent);
                    }

                    if (RT_SUCCESS(rc))
                    {
                        rc = dmgFileInflateSync(pThis, pExtent->offFileStart, pExtent->cbFile,
                                                pThis->pvDecompExtent,
                                                RT_MIN(pThis->cbDecompExtent, DMG_BLOCK2BYTE(pExtent->cSectorsExtent)));
                        if (RT_SUCCESS(rc))
                            pThis->pExtentDecomp = pExtent;
                    }
                }

                if (RT_SUCCESS(rc))
                    memcpy(pvBuf, (uint8_t *)pThis->pvDecompExtent + DMG_BLOCK2BYTE(uExtentRel), cbToRead);
                break;
            }
            default:
                AssertMsgFailed(("Invalid extent type\n"));
        }

        if (RT_SUCCESS(rc))
            *pcbActuallyRead = cbToRead;
    }
    else
        rc = VERR_INVALID_PARAMETER;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnWrite */
static int dmgWrite(void *pBackendData, uint64_t uOffset, const void *pvBuf,
                    size_t cbToWrite, size_t *pcbWriteProcess,
                    size_t *pcbPreRead, size_t *pcbPostRead, unsigned fWrite)
{
    LogFlowFunc(("pBackendData=%#p uOffset=%llu pvBuf=%#p cbToWrite=%zu pcbWriteProcess=%#p pcbPreRead=%#p pcbPostRead=%#p\n",
                 pBackendData, uOffset, pvBuf, cbToWrite, pcbWriteProcess, pcbPreRead, pcbPostRead));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc = VERR_NOT_IMPLEMENTED;

    AssertPtr(pThis);
    Assert(uOffset % 512 == 0);
    Assert(cbToWrite % 512 == 0);

    if (pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY)
    {
        rc = VERR_VD_IMAGE_READ_ONLY;
        goto out;
    }

    AssertMsgFailed(("Not implemented\n"));

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnFlush */
static int dmgFlush(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    rc = dmgFlushImage(pThis);

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetVersion */
static unsigned dmgGetVersion(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;

    AssertPtr(pThis);

    if (pThis)
        return 1;
    else
        return 0;
}

/** @copydoc VBOXHDDBACKEND::pfnGetSize */
static uint64_t dmgGetSize(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    uint64_t cb = 0;

    AssertPtr(pThis);

    if (pThis && pThis->pStorage)
        cb = pThis->cbSize;

    LogFlowFunc(("returns %llu\n", cb));
    return cb;
}

/** @copydoc VBOXHDDBACKEND::pfnGetFileSize */
static uint64_t dmgGetFileSize(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    uint64_t cb = 0;

    AssertPtr(pThis);

    if (pThis)
    {
        uint64_t cbFile;
        if (pThis->pStorage)
        {
            int rc = vdIfIoIntFileGetSize(pThis->pIfIo, pThis->pStorage, &cbFile);
            if (RT_SUCCESS(rc))
                cb = cbFile;
        }
    }

    LogFlowFunc(("returns %lld\n", cb));
    return cb;
}

/** @copydoc VBOXHDDBACKEND::pfnGetPCHSGeometry */
static int dmgGetPCHSGeometry(void *pBackendData, PVDGEOMETRY pPCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pPCHSGeometry=%#p\n", pBackendData, pPCHSGeometry));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (pThis->PCHSGeometry.cCylinders)
        {
            *pPCHSGeometry = pThis->PCHSGeometry;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_VD_GEOMETRY_NOT_SET;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (PCHS=%u/%u/%u)\n", rc, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetPCHSGeometry */
static int dmgSetPCHSGeometry(void *pBackendData, PCVDGEOMETRY pPCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pPCHSGeometry=%#p PCHS=%u/%u/%u\n",
                 pBackendData, pPCHSGeometry, pPCHSGeometry->cCylinders, pPCHSGeometry->cHeads, pPCHSGeometry->cSectors));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY)
        {
            rc = VERR_VD_IMAGE_READ_ONLY;
            goto out;
        }

        pThis->PCHSGeometry = *pPCHSGeometry;
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_VD_NOT_OPENED;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetLCHSGeometry */
static int dmgGetLCHSGeometry(void *pBackendData, PVDGEOMETRY pLCHSGeometry)
{
     LogFlowFunc(("pBackendData=%#p pLCHSGeometry=%#p\n", pBackendData, pLCHSGeometry));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (pThis->LCHSGeometry.cCylinders)
        {
            *pLCHSGeometry = pThis->LCHSGeometry;
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_VD_GEOMETRY_NOT_SET;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (LCHS=%u/%u/%u)\n", rc, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetLCHSGeometry */
static int dmgSetLCHSGeometry(void *pBackendData, PCVDGEOMETRY pLCHSGeometry)
{
    LogFlowFunc(("pBackendData=%#p pLCHSGeometry=%#p LCHS=%u/%u/%u\n",
                 pBackendData, pLCHSGeometry, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY)
        {
            rc = VERR_VD_IMAGE_READ_ONLY;
            goto out;
        }

        pThis->LCHSGeometry = *pLCHSGeometry;
        rc = VINF_SUCCESS;
    }
    else
        rc = VERR_VD_NOT_OPENED;

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetImageFlags */
static unsigned dmgGetImageFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    unsigned uImageFlags;

    AssertPtr(pThis);

    if (pThis)
        uImageFlags = pThis->uImageFlags;
    else
        uImageFlags = 0;

    LogFlowFunc(("returns %#x\n", uImageFlags));
    return uImageFlags;
}

/** @copydoc VBOXHDDBACKEND::pfnGetOpenFlags */
static unsigned dmgGetOpenFlags(void *pBackendData)
{
    LogFlowFunc(("pBackendData=%#p\n", pBackendData));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    unsigned uOpenFlags;

    AssertPtr(pThis);

    if (pThis)
        uOpenFlags = pThis->uOpenFlags;
    else
        uOpenFlags = 0;

    LogFlowFunc(("returns %#x\n", uOpenFlags));
    return uOpenFlags;
}

/** @copydoc VBOXHDDBACKEND::pfnSetOpenFlags */
static int dmgSetOpenFlags(void *pBackendData, unsigned uOpenFlags)
{
    LogFlowFunc(("pBackendData=%#p\n uOpenFlags=%#x", pBackendData, uOpenFlags));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    /* Image must be opened and the new flags must be valid. */
    if (!pThis || (uOpenFlags & ~(  VD_OPEN_FLAGS_READONLY | VD_OPEN_FLAGS_INFO
                                  | VD_OPEN_FLAGS_SHAREABLE | VD_OPEN_FLAGS_SEQUENTIAL
                                  | VD_OPEN_FLAGS_SKIP_CONSISTENCY_CHECKS)))
    {
        rc = VERR_INVALID_PARAMETER;
        goto out;
    }

    /* Implement this operation via reopening the image. */
    rc = dmgFreeImage(pThis, false);
    if (RT_FAILURE(rc))
        goto out;
    rc = dmgOpenImage(pThis, uOpenFlags);

out:
    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetComment */
static int dmgGetComment(void *pBackendData, char *pszComment,
                         size_t cbComment)
{
    LogFlowFunc(("pBackendData=%#p pszComment=%#p cbComment=%zu\n", pBackendData, pszComment, cbComment));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc comment='%s'\n", rc, pszComment));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetComment */
static int dmgSetComment(void *pBackendData, const char *pszComment)
{
    LogFlowFunc(("pBackendData=%#p pszComment=\"%s\"\n", pBackendData, pszComment));
    PDMGIMAGE pImage = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pImage);

    if (pImage)
    {
        if (pImage->uOpenFlags & VD_OPEN_FLAGS_READONLY)
            rc = VERR_VD_IMAGE_READ_ONLY;
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetUuid */
static int dmgGetUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetUuid */
static int dmgSetUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    LogFlowFunc(("%RTuuid\n", pUuid));
    AssertPtr(pThis);

    if (pThis)
    {
        if (!(pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetModificationUuid */
static int dmgGetModificationUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetModificationUuid */
static int dmgSetModificationUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (!(pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetParentUuid */
static int dmgGetParentUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetParentUuid */
static int dmgSetParentUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (!(pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnGetParentModificationUuid */
static int dmgGetParentModificationUuid(void *pBackendData, PRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p pUuid=%#p\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
        rc = VERR_NOT_SUPPORTED;
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc (%RTuuid)\n", rc, pUuid));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnSetParentModificationUuid */
static int dmgSetParentModificationUuid(void *pBackendData, PCRTUUID pUuid)
{
    LogFlowFunc(("pBackendData=%#p Uuid=%RTuuid\n", pBackendData, pUuid));
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;
    int rc;

    AssertPtr(pThis);

    if (pThis)
    {
        if (!(pThis->uOpenFlags & VD_OPEN_FLAGS_READONLY))
            rc = VERR_NOT_SUPPORTED;
        else
            rc = VERR_VD_IMAGE_READ_ONLY;
    }
    else
        rc = VERR_VD_NOT_OPENED;

    LogFlowFunc(("returns %Rrc\n", rc));
    return rc;
}

/** @copydoc VBOXHDDBACKEND::pfnDump */
static void dmgDump(void *pBackendData)
{
    PDMGIMAGE pThis = (PDMGIMAGE)pBackendData;

    AssertPtr(pThis);
    if (pThis)
    {
        vdIfErrorMessage(pThis->pIfError, "Header: Geometry PCHS=%u/%u/%u LCHS=%u/%u/%u cbSector=%llu\n",
                         pThis->PCHSGeometry.cCylinders, pThis->PCHSGeometry.cHeads, pThis->PCHSGeometry.cSectors,
                         pThis->LCHSGeometry.cCylinders, pThis->LCHSGeometry.cHeads, pThis->LCHSGeometry.cSectors,
                         pThis->cbSize / 512);
    }
}


VBOXHDDBACKEND g_DmgBackend =
{
    /* pszBackendName */
    "DMG",
    /* cbSize */
    sizeof(VBOXHDDBACKEND),
    /* uBackendCaps */
    VD_CAP_FILE | VD_CAP_VFS,
    /* paFileExtensions */
    s_aDmgFileExtensions,
    /* paConfigInfo */
    NULL,
    /* hPlugin */
    NIL_RTLDRMOD,
    /* pfnCheckIfValid */
    dmgCheckIfValid,
    /* pfnOpen */
    dmgOpen,
    /* pfnCreate */
    dmgCreate,
    /* pfnRename */
    dmgRename,
    /* pfnClose */
    dmgClose,
    /* pfnRead */
    dmgRead,
    /* pfnWrite */
    dmgWrite,
    /* pfnFlush */
    dmgFlush,
    /* pfnGetVersion */
    dmgGetVersion,
    /* pfnGetSize */
    dmgGetSize,
    /* pfnGetFileSize */
    dmgGetFileSize,
    /* pfnGetPCHSGeometry */
    dmgGetPCHSGeometry,
    /* pfnSetPCHSGeometry */
    dmgSetPCHSGeometry,
    /* pfnGetLCHSGeometry */
    dmgGetLCHSGeometry,
    /* pfnSetLCHSGeometry */
    dmgSetLCHSGeometry,
    /* pfnGetImageFlags */
    dmgGetImageFlags,
    /* pfnGetOpenFlags */
    dmgGetOpenFlags,
    /* pfnSetOpenFlags */
    dmgSetOpenFlags,
    /* pfnGetComment */
    dmgGetComment,
    /* pfnSetComment */
    dmgSetComment,
    /* pfnGetUuid */
    dmgGetUuid,
    /* pfnSetUuid */
    dmgSetUuid,
    /* pfnGetModificationUuid */
    dmgGetModificationUuid,
    /* pfnSetModificationUuid */
    dmgSetModificationUuid,
    /* pfnGetParentUuid */
    dmgGetParentUuid,
    /* pfnSetParentUuid */
    dmgSetParentUuid,
    /* pfnGetParentModificationUuid */
    dmgGetParentModificationUuid,
    /* pfnSetParentModificationUuid */
    dmgSetParentModificationUuid,
    /* pfnDump */
    dmgDump,
    /* pfnGetTimeStamp */
    NULL,
    /* pfnGetParentTimeStamp */
    NULL,
    /* pfnSetParentTimeStamp */
    NULL,
    /* pfnGetParentFilename */
    NULL,
    /* pfnSetParentFilename */
    NULL,
    /* pfnAsyncRead */
    NULL,
    /* pfnAsyncWrite */
    NULL,
    /* pfnAsyncFlush */
    NULL,
    /* pfnComposeLocation */
    genericFileComposeLocation,
    /* pfnComposeName */
    genericFileComposeName,
    /* pfnCompact */
    NULL,
    /* pfnResize */
    NULL,
    /* pfnDiscard */
    NULL,
    /* pfnAsyncDiscard */
    NULL,
    /* pfnRepair */
    NULL
};

/** @file
 *
 * VBox storage devices:
 * VBox HDD container implementation
 */

/*
 * Copyright (C) 2006 InnoTek Systemberatung GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_DRV_VBOXHDD
#include <VBox/VBoxHDD.h>
#include <VBox/pdm.h>
#include <VBox/mm.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/alloc.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/file.h>
#include <iprt/string.h>

#include "Builtins.h"

/*******************************************************************************
*   Constants And Macros, Structures and Typedefs                              *
*******************************************************************************/
/** The Sector size.
 * Currently we support only 512 bytes sectors.
 */
#define VDI_GEOMETRY_SECTOR_SIZE    (512)
/**  512 = 2^^9 */
#define VDI_GEOMETRY_SECTOR_SHIFT   (9)

/**
 * Harddisk geometry.
 */
#pragma pack(1)
typedef struct VDIDISKGEOMETRY
{
    /** Cylinders. */
    uint32_t    cCylinders;
    /** Heads. */
    uint32_t    cHeads;
    /** Sectors per track. */
    uint32_t    cSectors;
    /** Sector size. (bytes per sector) */
    uint32_t    cbSector;
} VDIDISKGEOMETRY, *PVDIDISKGEOMETRY;
#pragma pack()

/** Image signature. */
#define VDI_IMAGE_SIGNATURE   (0xbeda107f)

/**
 * Pre-Header to be stored in image file - used for version control.
 */
#pragma pack(1)
typedef struct VDIPREHEADER
{
    /** Just text info about image type, for eyes only. */
    char            szFileInfo[64];
    /** The image signature (VDI_IMAGE_SIGNATURE). */
    uint32_t        u32Signature;
    /** The image version (VDI_IMAGE_VERSION). */
    uint32_t        u32Version;
} VDIPREHEADER, *PVDIPREHEADER;
#pragma pack()

/**
 * Size of szComment field of HDD image header.
 */
#define VDI_IMAGE_COMMENT_SIZE    256

/**
 * Header to be stored in image file, VDI_IMAGE_VERSION_MAJOR = 0.
 * Prepended by VDIPREHEADER.
 */
#pragma pack(1)
typedef struct VDIHEADER0
{
    /** The image type (VDI_IMAGE_TYPE_*). */
    uint32_t        u32Type;
    /** Image flags (VDI_IMAGE_FLAGS_*). */
    uint32_t        fFlags;
    /** Image comment. (UTF-8) */
    char            szComment[VDI_IMAGE_COMMENT_SIZE];
    /** Image geometry. */
    VDIDISKGEOMETRY Geometry;
    /** Size of disk (in bytes). */
    uint64_t        cbDisk;
    /** Block size. (For instance VDI_IMAGE_BLOCK_SIZE.) */
    uint32_t        cbBlock;
    /** Number of blocks. */
    uint32_t        cBlocks;
    /** Number of allocated blocks. */
    uint32_t        cBlocksAllocated;
    /** UUID of image. */
    RTUUID          uuidCreate;
    /** UUID of image's last modification. */
    RTUUID          uuidModify;
    /** Only for secondary images - UUID of primary image. */
    RTUUID          uuidLinkage;
} VDIHEADER0, *PVDIHEADER0;
#pragma pack()

/**
 * Header to be stored in image file, VDI_IMAGE_VERSION_MAJOR = 1.
 * Prepended by VDIPREHEADER.
 */
#pragma pack(1)
typedef struct VDIHEADER1
{
    /** Size of this structure in bytes. */
    uint32_t        cbHeader;
    /** The image type (VDI_IMAGE_TYPE_*). */
    uint32_t        u32Type;
    /** Image flags (VDI_IMAGE_FLAGS_*). */
    uint32_t        fFlags;
    /** Image comment. (UTF-8) */
    char            szComment[VDI_IMAGE_COMMENT_SIZE];
    /** Offset of Blocks array from the begining of image file.
     * Should be sector-aligned for HDD access optimization. */
    uint32_t        offBlocks;
    /** Offset of image data from the begining of image file.
     * Should be sector-aligned for HDD access optimization. */
    uint32_t        offData;
    /** Image geometry. */
    VDIDISKGEOMETRY Geometry;
    /** BIOS HDD translation mode, see PDMBIOSTRANSLATION. */
    uint32_t        u32Translation;
    /** Size of disk (in bytes). */
    uint64_t        cbDisk;
    /** Block size. (For instance VDI_IMAGE_BLOCK_SIZE.) Should be a power of 2! */
    uint32_t        cbBlock;
    /** Size of additional service information of every data block.
     * Prepended before block data. May be 0.
     * Should be a power of 2 and sector-aligned for optimization reasons. */
    uint32_t        cbBlockExtra;
    /** Number of blocks. */
    uint32_t        cBlocks;
    /** Number of allocated blocks. */
    uint32_t        cBlocksAllocated;
    /** UUID of image. */
    RTUUID          uuidCreate;
    /** UUID of image's last modification. */
    RTUUID          uuidModify;
    /** Only for secondary images - UUID of previous image. */
    RTUUID          uuidLinkage;
    /** Only for secondary images - UUID of previous image's last modification. */
    RTUUID          uuidParentModify;
} VDIHEADER1, *PVDIHEADER1;
#pragma pack()

/**
 * Header structure for all versions.
 */
typedef struct VDIHEADER
{
    unsigned        uVersion;
    union
    {
        VDIHEADER0    v0;
        VDIHEADER1    v1;
    } u;
} VDIHEADER, *PVDIHEADER;

/** Block 'pointer'. */
typedef uint32_t    VDIIMAGEBLOCKPOINTER;
/** Pointer to a block 'pointer'. */
typedef VDIIMAGEBLOCKPOINTER *PVDIIMAGEBLOCKPOINTER;

/**
 * Block marked as free is not allocated in image file, read from this
 * block may returns any random data.
 */
#define VDI_IMAGE_BLOCK_FREE   ((VDIIMAGEBLOCKPOINTER)~0)

/**
 * Block marked as zero is not allocated in image file, read from this
 * block returns zeroes.
 */
#define VDI_IMAGE_BLOCK_ZERO   ((VDIIMAGEBLOCKPOINTER)~1)

/**
 * Block 'pointer' >= VDI_IMAGE_BLOCK_UNALLOCATED indicates block is not
 * allocated in image file.
 */
#define VDI_IMAGE_BLOCK_UNALLOCATED   (VDI_IMAGE_BLOCK_ZERO)
#define IS_VDI_IMAGE_BLOCK_ALLOCATED(bp)   (bp < VDI_IMAGE_BLOCK_UNALLOCATED)

#define GET_MAJOR_HEADER_VERSION(ph) (VDI_GET_VERSION_MAJOR((ph)->uVersion))
#define GET_MINOR_HEADER_VERSION(ph) (VDI_GET_VERSION_MINOR((ph)->uVersion))

/*******************************************************************************
*   Internal Functions for header access                                       *
*******************************************************************************/
static inline VDIIMAGETYPE getImageType(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return (VDIIMAGETYPE)ph->u.v0.u32Type;
        case 1: return (VDIIMAGETYPE)ph->u.v1.u32Type;
    }
    AssertFailed();
    return (VDIIMAGETYPE)0;
}

static inline unsigned getImageFlags(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return ph->u.v0.fFlags;
        case 1: return ph->u.v1.fFlags;
    }
    AssertFailed();
    return 0;
}

static inline char *getImageComment(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return &ph->u.v0.szComment[0];
        case 1: return &ph->u.v1.szComment[0];
    }
    AssertFailed();
    return NULL;
}

static inline unsigned getImageBlocksOffset(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return (sizeof(VDIPREHEADER) + sizeof(VDIHEADER0));
        case 1: return ph->u.v1.offBlocks;
    }
    AssertFailed();
    return 0;
}

static inline unsigned getImageDataOffset(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return sizeof(VDIPREHEADER) + sizeof(VDIHEADER0) + \
                       (ph->u.v0.cBlocks * sizeof(VDIIMAGEBLOCKPOINTER));
        case 1: return ph->u.v1.offData;
    }
    AssertFailed();
    return 0;
}

static inline PVDIDISKGEOMETRY getImageGeometry(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return &ph->u.v0.Geometry;
        case 1: return &ph->u.v1.Geometry;
    }
    AssertFailed();
    return NULL;
}

static inline PDMBIOSTRANSLATION getImageTranslation(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return PDMBIOSTRANSLATION_AUTO;
        case 1: return (PDMBIOSTRANSLATION)ph->u.v1.u32Translation;
    }
    AssertFailed();
    return PDMBIOSTRANSLATION_NONE;
}

static inline void setImageTranslation(PVDIHEADER ph, PDMBIOSTRANSLATION enmTranslation)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0:                                                     return;
        case 1: ph->u.v1.u32Translation = (uint32_t)enmTranslation; return;
    }
    AssertFailed();
}

static inline uint64_t getImageDiskSize(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return ph->u.v0.cbDisk;
        case 1: return ph->u.v1.cbDisk;
    }
    AssertFailed();
    return 0;
}

static inline unsigned getImageBlockSize(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return ph->u.v0.cbBlock;
        case 1: return ph->u.v1.cbBlock;
    }
    AssertFailed();
    return 0;
}

static inline unsigned getImageExtraBlockSize(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return 0;
        case 1: return ph->u.v1.cbBlockExtra;
    }
    AssertFailed();
    return 0;
}

static inline unsigned getImageBlocks(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return ph->u.v0.cBlocks;
        case 1: return ph->u.v1.cBlocks;
    }
    AssertFailed();
    return 0;
}

static inline unsigned getImageBlocksAllocated(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return ph->u.v0.cBlocksAllocated;
        case 1: return ph->u.v1.cBlocksAllocated;
    }
    AssertFailed();
    return 0;
}

static inline void setImageBlocksAllocated(PVDIHEADER ph, unsigned cBlocks)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: ph->u.v0.cBlocksAllocated = cBlocks; return;
        case 1: ph->u.v1.cBlocksAllocated = cBlocks; return;
    }
    AssertFailed();
}

static inline PRTUUID getImageCreationUUID(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return &ph->u.v0.uuidCreate;
        case 1: return &ph->u.v1.uuidCreate;
    }
    AssertFailed();
    return NULL;
}

static inline PRTUUID getImageModificationUUID(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return &ph->u.v0.uuidModify;
        case 1: return &ph->u.v1.uuidModify;
    }
    AssertFailed();
    return NULL;
}

static inline PRTUUID getImageParentUUID(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 0: return &ph->u.v0.uuidLinkage;
        case 1: return &ph->u.v1.uuidLinkage;
    }
    AssertFailed();
    return NULL;
}

static inline PRTUUID getImageParentModificationUUID(PVDIHEADER ph)
{
    switch (GET_MAJOR_HEADER_VERSION(ph))
    {
        case 1: return &ph->u.v1.uuidParentModify;
    }
    AssertFailed();
    return NULL;
}

/**
 * Default image block size, may be changed by setBlockSize/getBlockSize.
 *
 * Note: for speed reasons block size should be a power of 2 !
 */
#define VDI_IMAGE_DEFAULT_BLOCK_SIZE            _1M

/**
 * fModified bit flags.
 */
#define VDI_IMAGE_MODIFIED_FLAG                 BIT(0)
#define VDI_IMAGE_MODIFIED_FIRST                BIT(1)
#define VDI_IMAGE_MODIFIED_DISABLE_UUID_UPDATE  BIT(2)

/**
 * Image structure
 */
typedef struct VDIIMAGEDESC
{
    /** Link to parent image descriptor, if any. */
    struct VDIIMAGEDESC    *pPrev;
    /** Link to child image descriptor, if any. */
    struct VDIIMAGEDESC    *pNext;
    /** File handle. */
    RTFILE                  File;
    /** True if the image is operating in readonly mode. */
    bool                    fReadOnly;
    /** Image open flags, VDI_OPEN_FLAGS_*. */
    unsigned                fOpen;
    /** Image pre-header. */
    VDIPREHEADER            PreHeader;
    /** Image header. */
    VDIHEADER               Header;
    /** Pointer to a block array. */
    PVDIIMAGEBLOCKPOINTER   paBlocks;
    /** fFlags copy from image header, for speed optimization. */
    unsigned                fFlags;
    /** Start offset of block array in image file, here for speed optimization. */
    unsigned                offStartBlocks;
    /** Start offset of data in image file, here for speed optimization. */
    unsigned                offStartData;
    /** Block mask for getting the offset into a block from a byte hdd offset. */
    unsigned                uBlockMask;
    /** Block shift value for converting byte hdd offset into paBlock index. */
    unsigned                uShiftOffset2Index;
    /** Block shift value for converting block index into offset in image. */
    unsigned                uShiftIndex2Offset;
    /** Offset of data from the beginning of block. */
    unsigned                offStartBlockData;
    /** Image is modified flags (VDI_IMAGE_MODIFIED*). */
    unsigned                fModified;
    /** Container filename. (UTF-8)
     * @todo Make this variable length to save a bunch of bytes. (low prio) */
    char                    szFilename[RTPATH_MAX];
} VDIIMAGEDESC, *PVDIIMAGEDESC;

/**
 * Default work buffer size, may be changed by setBufferSize() method.
 *
 * For best speed performance it must be equal to image block size.
 */
#define VDIDISK_DEFAULT_BUFFER_SIZE   (VDI_IMAGE_DEFAULT_BLOCK_SIZE)

/** VDIDISK Signature. */
#define VDIDISK_SIGNATURE (0xbedafeda)

/**
 * VBox HDD Container main structure, private part.
 */
struct VDIDISK
{
    /** Structure signature (VDIDISK_SIGNATURE). */
    uint32_t        u32Signature;

    /** Number of opened images. */
    unsigned        cImages;

    /** Base image. */
    PVDIIMAGEDESC   pBase;

    /** Last opened image in the chain.
     * The same as pBase if only one image is used or the last opened diff image. */
    PVDIIMAGEDESC   pLast;

    /** Default block size for newly created images. */
    unsigned        cbBlock;

    /** Working buffer size, allocated only while committing data,
     * copying block from primary image to secondary and saving previously
     * zero block. Buffer deallocated after operation complete.
     * @remark  For best performance buffer size must be equal to image's
     *          block size, however it may be decreased for memory saving.
     */
    unsigned        cbBuf;

    /** The media interface. */
    PDMIMEDIA       IMedia;
    /** Pointer to the driver instance. */
    PPDMDRVINS      pDrvIns;
};


/** Converts a pointer to VDIDISK::IMedia to a PVDIDISK. */
#define PDMIMEDIA_2_VDIDISK(pInterface) ( (PVDIDISK)((uintptr_t)pInterface - RT_OFFSETOF(VDIDISK, IMedia)) )

/** Converts a pointer to PDMDRVINS::IBase to a PPDMDRVINS. */
#define PDMIBASE_2_DRVINS(pInterface)   ( (PPDMDRVINS)((uintptr_t)pInterface - RT_OFFSETOF(PDMDRVINS, IBase)) )

/** Converts a pointer to PDMDRVINS::IBase to a PVDIDISK. */
#define PDMIBASE_2_VDIDISK(pInterface)  ( PDMINS2DATA(PDMIBASE_2_DRVINS(pInterface), PVDIDISK) )


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static unsigned getPowerOfTwo(unsigned uNumber);
static void vdiInitPreHeader(PVDIPREHEADER pPreHdr);
static int  vdiValidatePreHeader(PVDIPREHEADER pPreHdr);
static void vdiInitHeader(PVDIHEADER pHeader, VDIIMAGETYPE enmType, uint32_t fFlags,
                          const char *pszComment, uint64_t cbDisk, uint32_t cbBlock,
                          uint32_t cbBlockExtra);
static int  vdiValidateHeader(PVDIHEADER pHeader);
static int  vdiCreateImage(const char *pszFilename, VDIIMAGETYPE enmType, unsigned fFlags,
                          uint64_t cbSize, const char *pszComment, PVDIIMAGEDESC pParent,
                          PFNVMPROGRESS pfnProgress, void *pvUser);
static void vdiInitImageDesc(PVDIIMAGEDESC pImage);
static void vdiSetupImageDesc(PVDIIMAGEDESC pImage);
static int  vdiOpenImage(PVDIIMAGEDESC *ppImage, const char *pszFilename, unsigned fOpen,
                        PVDIIMAGEDESC pParent);
static int  vdiUpdateHeader(PVDIIMAGEDESC pImage);
static int  vdiUpdateBlockInfo(PVDIIMAGEDESC pImage, unsigned uBlock);
static int  vdiUpdateBlocks(PVDIIMAGEDESC pImage);
static void vdiSetModifiedFlag(PVDIIMAGEDESC pImage);
static void vdiResetModifiedFlag(PVDIIMAGEDESC pImage);
static void vdiDisableLastModifiedUpdate(PVDIIMAGEDESC pImage);
#if 0 /* unused */
static void vdiEnableLastModifiedUpdate(PVDIIMAGEDESC pImage);
#endif
static void vdiFlushImage(PVDIIMAGEDESC pImage);
static void vdiCloseImage(PVDIIMAGEDESC pImage);
static int  vdiReadInBlock(PVDIIMAGEDESC pImage, unsigned uBlock, unsigned offRead,
                           unsigned cbToRead, void *pvBuf);
static int  vdiFillBlockByZeroes(PVDIDISK pDisk, PVDIIMAGEDESC pImage, unsigned uBlock);
static int  vdiWriteInBlock(PVDIDISK pDisk, PVDIIMAGEDESC pImage, unsigned uBlock,
                            unsigned offWrite, unsigned cbToWrite, const void *pvBuf);
static int  vdiCopyBlock(PVDIDISK pDisk, PVDIIMAGEDESC pImage, unsigned uBlock);
static int  vdiMergeImages(PVDIIMAGEDESC pImageFrom, PVDIIMAGEDESC pImageTo, bool fParentToChild,
                          PFNVMPROGRESS pfnProgress, void *pvUser);
static void vdiInitVDIDisk(PVDIDISK pDisk);
static void vdiAddImageToList(PVDIDISK pDisk, PVDIIMAGEDESC pImage);
static void vdiRemoveImageFromList(PVDIDISK pDisk, PVDIIMAGEDESC pImage);
static PVDIIMAGEDESC vdiGetImageByNumber(PVDIDISK pDisk, int nImage);
static int  vdiChangeImageMode(PVDIIMAGEDESC pImage, bool fReadOnly);
static int  vdiUpdateReadOnlyHeader(PVDIIMAGEDESC pImage);

static int  vdiCommitToImage(PVDIDISK pDisk, PVDIIMAGEDESC pDstImage,
                            PFNVMPROGRESS pfnProgress, void *pvUser);
static void vdiDumpImage(PVDIIMAGEDESC pImage);

static DECLCALLBACK(int)  vdiConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfgHandle);
static DECLCALLBACK(void) vdiDestruct(PPDMDRVINS pDrvIns);
static DECLCALLBACK(int)  vdiRead(PPDMIMEDIA pInterface,
                                  uint64_t off, void *pvBuf, size_t cbRead);
static DECLCALLBACK(int)  vdiWrite(PPDMIMEDIA pInterface,
                                   uint64_t off, const void *pvBuf, size_t cbWrite);
static DECLCALLBACK(int)  vdiFlush(PPDMIMEDIA pInterface);
static DECLCALLBACK(uint64_t) vdiGetSize(PPDMIMEDIA pInterface);
static DECLCALLBACK(int)  vdiBiosGetGeometry(PPDMIMEDIA pInterface, uint32_t *pcCylinders,
                                             uint32_t *pcHeads, uint32_t *pcSectors);
static DECLCALLBACK(int)  vdiBiosSetGeometry(PPDMIMEDIA pInterface, uint32_t cCylinders,
                                             uint32_t cHeads, uint32_t cSectors);
static DECLCALLBACK(int)  vdiGetUuid(PPDMIMEDIA pInterface, PRTUUID pUuid);
static DECLCALLBACK(bool) vdiIsReadOnly(PPDMIMEDIA pInterface);
static DECLCALLBACK(int)  vdiBiosGetTranslation(PPDMIMEDIA pInterface,
                                                PPDMBIOSTRANSLATION penmTranslation);
static DECLCALLBACK(int)  vdiBiosSetTranslation(PPDMIMEDIA pInterface,
                                                PDMBIOSTRANSLATION enmTranslation);
static DECLCALLBACK(void *) vdiQueryInterface(PPDMIBASE pInterface, PDMINTERFACE enmInterface);


/**
 * internal: return power of 2 or 0 if num error.
 */
static unsigned getPowerOfTwo(unsigned uNumber)
{
    if (uNumber == 0)
        return 0;
    unsigned uPower2 = 0;
    while ((uNumber & 1) == 0)
    {
        uNumber >>= 1;
        uPower2++;
    }
    return uNumber == 1 ? uPower2 : 0;
}

/**
 * internal: init HDD preheader.
 */
static void vdiInitPreHeader(PVDIPREHEADER pPreHdr)
{
    pPreHdr->u32Signature = VDI_IMAGE_SIGNATURE;
    pPreHdr->u32Version = VDI_IMAGE_VERSION;
    memset(pPreHdr->szFileInfo, 0, sizeof(pPreHdr->szFileInfo));
    strncat(pPreHdr->szFileInfo, VDI_IMAGE_FILE_INFO, sizeof(pPreHdr->szFileInfo));
}

/**
 * internal: check HDD preheader.
 */
static int vdiValidatePreHeader(PVDIPREHEADER pPreHdr)
{
    if (pPreHdr->u32Signature != VDI_IMAGE_SIGNATURE)
        return VERR_VDI_INVALID_SIGNATURE;

    if (    pPreHdr->u32Version != VDI_IMAGE_VERSION
        &&  pPreHdr->u32Version != 0x00000002)    /* old version. */
        return VERR_VDI_UNSUPPORTED_VERSION;

    return VINF_SUCCESS;
}

/**
 * internal: init HDD header. Always use latest header version.
 * @param   pHeader     Assumes it was initially initialized to all zeros.
 */
static void vdiInitHeader(PVDIHEADER pHeader, VDIIMAGETYPE enmType, uint32_t fFlags,
                          const char *pszComment, uint64_t cbDisk, uint32_t cbBlock,
                          uint32_t cbBlockExtra)
{
    pHeader->uVersion = VDI_IMAGE_VERSION;
    pHeader->u.v1.cbHeader = sizeof(VDIHEADER1);
    pHeader->u.v1.u32Type = (uint32_t)enmType;
    pHeader->u.v1.fFlags = fFlags;
#ifdef VBOX_STRICT
    char achZero[VDI_IMAGE_COMMENT_SIZE] = {0};
    Assert(!memcmp(pHeader->u.v1.szComment, achZero, VDI_IMAGE_COMMENT_SIZE));
#endif
    pHeader->u.v1.szComment[0] = '\0';
    if (pszComment)
    {
        AssertMsg(strlen(pszComment) < sizeof(pHeader->u.v1.szComment),
                  ("HDD Comment is too long, cb=%d\n", strlen(pszComment)));
        strncat(pHeader->u.v1.szComment, pszComment, sizeof(pHeader->u.v1.szComment));
    }

    /* Mark the geometry not-calculated. */
    pHeader->u.v1.Geometry.cCylinders = 0;
    pHeader->u.v1.Geometry.cHeads = 0;
    pHeader->u.v1.Geometry.cSectors = 0;
    pHeader->u.v1.Geometry.cbSector = VDI_GEOMETRY_SECTOR_SIZE;
    pHeader->u.v1.u32Translation = PDMBIOSTRANSLATION_AUTO;

    pHeader->u.v1.cbDisk = cbDisk;
    pHeader->u.v1.cbBlock = cbBlock;
    pHeader->u.v1.cBlocks = (uint32_t)(cbDisk / cbBlock);
    if (cbDisk % cbBlock)
        pHeader->u.v1.cBlocks++;
    pHeader->u.v1.cbBlockExtra = cbBlockExtra;
    pHeader->u.v1.cBlocksAllocated = 0;

    /* Init offsets. */
    pHeader->u.v1.offBlocks = RT_ALIGN_32(sizeof(VDIPREHEADER) + sizeof(VDIHEADER1), VDI_GEOMETRY_SECTOR_SIZE);
    pHeader->u.v1.offData = RT_ALIGN_32(pHeader->u.v1.offBlocks + (pHeader->u.v1.cBlocks * sizeof(VDIIMAGEBLOCKPOINTER)), VDI_GEOMETRY_SECTOR_SIZE);

    /* Init uuids. */
    RTUuidCreate(&pHeader->u.v1.uuidCreate);
    RTUuidClear(&pHeader->u.v1.uuidModify);
    RTUuidClear(&pHeader->u.v1.uuidLinkage);
    RTUuidClear(&pHeader->u.v1.uuidParentModify);
}

/**
 * internal: check HDD header.
 */
static int vdiValidateHeader(PVDIHEADER pHeader)
{
    /* Check verion-dependend header parameters. */
    switch (GET_MAJOR_HEADER_VERSION(pHeader))
    {
        case 0:
        {
            /* Old header version. */
            break;
        }
        case 1:
        {
            /* Current header version. */

            if (pHeader->u.v1.cbHeader < sizeof(VDIHEADER1))
            {
                LogRel(("VDI: v1 header size wrong (%d < %d)\n",
                       pHeader->u.v1.cbHeader, sizeof(VDIHEADER1)));
                return VERR_VDI_INVALID_HEADER;
            }

            if (getImageBlocksOffset(pHeader) < (sizeof(VDIPREHEADER) + sizeof(VDIHEADER1)))
            {
                LogRel(("VDI: v1 blocks offset wrong (%d < %d)\n",
                       getImageBlocksOffset(pHeader), sizeof(VDIPREHEADER) + sizeof(VDIHEADER1)));
                return VERR_VDI_INVALID_HEADER;
            }

            if (getImageDataOffset(pHeader) < (getImageBlocksOffset(pHeader) + getImageBlocks(pHeader) * sizeof(VDIIMAGEBLOCKPOINTER)))
            {
                LogRel(("VDI: v1 image data offset wrong (%d < %d)\n",
                       getImageDataOffset(pHeader), getImageBlocksOffset(pHeader) + getImageBlocks(pHeader) * sizeof(VDIIMAGEBLOCKPOINTER)));
                return VERR_VDI_INVALID_HEADER;
            }

            if (    getImageType(pHeader) == VDI_IMAGE_TYPE_UNDO
                ||  getImageType(pHeader) == VDI_IMAGE_TYPE_DIFF)
            {
                if (RTUuidIsNull(getImageParentUUID(pHeader)))
                {
                    LogRel(("VDI: v1 uuid of parent is 0)\n"));
                    return VERR_VDI_INVALID_HEADER;
                }
                if (RTUuidIsNull(getImageParentModificationUUID(pHeader)))
                {
                    LogRel(("VDI: v1 uuid of parent modification is 0\n"));
                    return VERR_VDI_INVALID_HEADER;
                }
            }

            break;
        }
        default:
            /* Unsupported. */
            return VERR_VDI_UNSUPPORTED_VERSION;
    }

    /* Check common header parameters. */

    bool fFailed = false;

    if (    getImageType(pHeader) < VDI_IMAGE_TYPE_FIRST
        ||  getImageType(pHeader) > VDI_IMAGE_TYPE_LAST)
    {
        LogRel(("VDI: bad image type %d\n", getImageType(pHeader)));
        fFailed = true;
    }

    if (getImageFlags(pHeader) & ~VDI_IMAGE_FLAGS_MASK)
    {
        LogRel(("VDI: bad image flags %08x\n", getImageFlags(pHeader)));
        fFailed = true;
    }

    if ((getImageGeometry(pHeader))->cbSector != VDI_GEOMETRY_SECTOR_SIZE)
    {
        LogRel(("VDI: wrong sector size (%d != %d)\n",
               (getImageGeometry(pHeader))->cbSector, VDI_GEOMETRY_SECTOR_SIZE));
        fFailed = true;
    }

    if (    getImageDiskSize(pHeader) == 0
        ||  getImageBlockSize(pHeader) == 0
        ||  getImageBlocks(pHeader) == 0
        ||  getPowerOfTwo(getImageBlockSize(pHeader)) == 0)
    {
        LogRel(("VDI: wrong size (%lld, %d, %d, %d)\n",
              getImageDiskSize(pHeader), getImageBlockSize(pHeader),
              getImageBlocks(pHeader), getPowerOfTwo(getImageBlockSize(pHeader))));
        fFailed = true;
    }

    if (getImageBlocksAllocated(pHeader) > getImageBlocks(pHeader))
    {
        LogRel(("VDI: too many blocks allocated (%d > %d)\n"
                "     blocksize=%d disksize=%lld\n",
              getImageBlocksAllocated(pHeader), getImageBlocks(pHeader),
              getImageBlockSize(pHeader), getImageDiskSize(pHeader)));
        fFailed = true;
    }

    if (    getImageExtraBlockSize(pHeader) != 0
        &&  getPowerOfTwo(getImageExtraBlockSize(pHeader)) == 0)
    {
        LogRel(("VDI: wrong extra size (%d, %d)\n",
               getImageExtraBlockSize(pHeader), getPowerOfTwo(getImageExtraBlockSize(pHeader))));
        fFailed = true;
    }

    if ((uint64_t)getImageBlockSize(pHeader) * getImageBlocks(pHeader) < getImageDiskSize(pHeader))
    {
        LogRel(("VDI: wrong disk size (%d, %d, %lld)\n",
               getImageBlockSize(pHeader), getImageBlocks(pHeader), getImageDiskSize(pHeader)));
        fFailed = true;
    }

    if (RTUuidIsNull(getImageCreationUUID(pHeader)))
    {
        LogRel(("VDI: uuid of creator is 0\n"));
        fFailed = true;
    }

    if (RTUuidIsNull(getImageModificationUUID(pHeader)))
    {
        LogRel(("VDI: uuid of modificator is 0\n"));
        fFailed = true;
    }

    return fFailed ? VERR_VDI_INVALID_HEADER : VINF_SUCCESS;
}

/**
 * internal: init VDIIMAGEDESC structure.
 */
static void vdiInitImageDesc(PVDIIMAGEDESC pImage)
{
    pImage->pPrev = NULL;
    pImage->pNext = NULL;
    pImage->File = NIL_RTFILE;
    pImage->paBlocks = NULL;
}

/**
 * internal: setup VDIIMAGEDESC structure by image header.
 */
static void vdiSetupImageDesc(PVDIIMAGEDESC pImage)
{
    pImage->fFlags             = getImageFlags(&pImage->Header);
    pImage->offStartBlocks     = getImageBlocksOffset(&pImage->Header);
    pImage->offStartData       = getImageDataOffset(&pImage->Header);
    pImage->uBlockMask         = getImageBlockSize(&pImage->Header) - 1;
    pImage->uShiftIndex2Offset =
    pImage->uShiftOffset2Index = getPowerOfTwo(getImageBlockSize(&pImage->Header));
    pImage->offStartBlockData  = getImageExtraBlockSize(&pImage->Header);
    if (pImage->offStartBlockData != 0)
        pImage->uShiftIndex2Offset += getPowerOfTwo(pImage->offStartBlockData);
}

/**
 * internal: create image.
 */
static int vdiCreateImage(const char *pszFilename, VDIIMAGETYPE enmType, unsigned fFlags,
                          uint64_t cbSize, const char *pszComment, PVDIIMAGEDESC pParent,
                          PFNVMPROGRESS pfnProgress, void *pvUser)
{
    /* Check args. */
    Assert(pszFilename);
    Assert(enmType >= VDI_IMAGE_TYPE_FIRST && enmType <= VDI_IMAGE_TYPE_LAST);
    Assert(!(fFlags & ~VDI_IMAGE_FLAGS_MASK));
    Assert(cbSize);

    /* Special check for comment length. */
    if (    pszComment
        &&  strlen(pszComment) >= VDI_IMAGE_COMMENT_SIZE)
    {
        Log(("vdiCreateImage: pszComment is too long, cb=%d\n", strlen(pszComment)));
        return VERR_VDI_COMMENT_TOO_LONG;
    }

    if (    enmType == VDI_IMAGE_TYPE_UNDO
        ||  enmType == VDI_IMAGE_TYPE_DIFF)
    {
        Assert(pParent);
        if ((pParent->PreHeader.u32Version >> 16) != VDI_IMAGE_VERSION_MAJOR)
        {
            /* Invalid parent image version. */
            Log(("vdiCreateImage: unsupported parent version=%08X\n", pParent->PreHeader.u32Version));
            return VERR_VDI_UNSUPPORTED_VERSION;
        }

        /* get image params from the parent image. */
        fFlags = getImageFlags(&pParent->Header);
        cbSize = getImageDiskSize(&pParent->Header);
    }

    PVDIIMAGEDESC pImage = (PVDIIMAGEDESC)RTMemAllocZ(sizeof(VDIIMAGEDESC));
    if (!pImage)
        return VERR_NO_MEMORY;
    vdiInitImageDesc(pImage);

    vdiInitPreHeader(&pImage->PreHeader);
    vdiInitHeader(&pImage->Header, enmType, fFlags, pszComment, cbSize, VDI_IMAGE_DEFAULT_BLOCK_SIZE, 0);

    if (    enmType == VDI_IMAGE_TYPE_UNDO
        ||  enmType == VDI_IMAGE_TYPE_DIFF)
    {
        /* Set up linkage information. */
        pImage->Header.u.v1.uuidLinkage = *getImageCreationUUID(&pParent->Header);
        pImage->Header.u.v1.uuidParentModify = *getImageModificationUUID(&pParent->Header);
    }

    pImage->paBlocks = (PVDIIMAGEBLOCKPOINTER)RTMemAlloc(sizeof(VDIIMAGEBLOCKPOINTER) * getImageBlocks(&pImage->Header));
    if (!pImage->paBlocks)
    {
        RTMemFree(pImage);
        return VERR_NO_MEMORY;
    }

    if (enmType != VDI_IMAGE_TYPE_FIXED)
    {
        /* for growing images mark all blocks in paBlocks as free. */
        for (unsigned i = 0; i < pImage->Header.u.v1.cBlocks; i++)
            pImage->paBlocks[i] = VDI_IMAGE_BLOCK_FREE;
    }
    else
    {
        /* for fixed images mark all blocks in paBlocks as allocated */
        for (unsigned i = 0; i < pImage->Header.u.v1.cBlocks; i++)
            pImage->paBlocks[i] = i;
        pImage->Header.u.v1.cBlocksAllocated = pImage->Header.u.v1.cBlocks;
    }

    /* Setup image parameters. */
    vdiSetupImageDesc(pImage);

    /* create file */
    int rc = RTFileOpen(&pImage->File,
                        pszFilename,
                        RTFILE_O_READWRITE | RTFILE_O_CREATE | RTFILE_O_DENY_ALL);
    if (VBOX_SUCCESS(rc))
    {
        /* Lock image exclusively to close any wrong access by VDI API calls. */
        uint64_t cbLock = pImage->offStartData
                        + ((uint64_t)getImageBlocks(&pImage->Header) << pImage->uShiftIndex2Offset);
        rc = RTFileLock(pImage->File,
                        RTFILE_LOCK_WRITE | RTFILE_LOCK_IMMEDIATELY, 0, cbLock);
        if (VBOX_FAILURE(rc))
        {
            cbLock = 0;    /* Not locked. */
            goto l_create_failed;
        }

        if (enmType == VDI_IMAGE_TYPE_FIXED)
        {
            /*
             * Allocate & commit whole file if fixed image, it must be more
             * effective than expanding file by write operations.
             */
            rc = RTFileSetSize(pImage->File,
                               pImage->offStartData
                             + ((uint64_t)getImageBlocks(&pImage->Header) << pImage->uShiftIndex2Offset));
        }
        else
        {
            /* Set file size to hold header and blocks array. */
            rc = RTFileSetSize(pImage->File, pImage->offStartData);
        }
        if (VBOX_FAILURE(rc))
            goto l_create_failed;

        /* Generate image last-modify uuid */
        RTUuidCreate(getImageModificationUUID(&pImage->Header));

        /* Write pre-header. */
        rc = RTFileWrite(pImage->File, &pImage->PreHeader, sizeof(pImage->PreHeader), NULL);
        if (VBOX_FAILURE(rc))
            goto l_create_failed;

        /* Write header. */
        rc = RTFileWrite(pImage->File, &pImage->Header.u.v1, sizeof(pImage->Header.u.v1), NULL);
        if (VBOX_FAILURE(rc))
            goto l_create_failed;

        /* Write blocks array. */
        rc = RTFileSeek(pImage->File, pImage->offStartBlocks, RTFILE_SEEK_BEGIN, NULL);
        if (VBOX_FAILURE(rc))
            goto l_create_failed;
        rc = RTFileWrite(pImage->File,
                         pImage->paBlocks,
                         getImageBlocks(&pImage->Header) * sizeof(VDIIMAGEBLOCKPOINTER),
                         NULL);
        if (VBOX_FAILURE(rc))
            goto l_create_failed;

        if (    (enmType == VDI_IMAGE_TYPE_FIXED)
            &&  (fFlags & VDI_IMAGE_FLAGS_ZERO_EXPAND))
        {
            /* Fill image with zeroes. */

            rc = RTFileSeek(pImage->File, pImage->offStartData, RTFILE_SEEK_BEGIN, NULL);
            if (VBOX_FAILURE(rc))
                goto l_create_failed;

            /* alloc tmp zero-filled buffer */
            void *pvBuf = RTMemTmpAllocZ(VDIDISK_DEFAULT_BUFFER_SIZE);
            if (pvBuf)
            {
                uint64_t cbFill = (uint64_t)getImageBlocks(&pImage->Header) << pImage->uShiftIndex2Offset;
                uint64_t cbDisk = cbFill;

                /* do loop to fill all image. */
                while (cbFill > 0)
                {
                    unsigned to_fill = (unsigned)RT_MIN(cbFill, VDIDISK_DEFAULT_BUFFER_SIZE);

                    rc = RTFileWrite(pImage->File, pvBuf, to_fill, NULL);
                    if (VBOX_FAILURE(rc))
                        break;

                    cbFill -= to_fill;

                    if (pfnProgress)
                    {
                        rc = pfnProgress(NULL /* WARNING! pVM=NULL  */,
                                         (unsigned)(((cbDisk - cbFill) * 100) / cbDisk),
                                         pvUser);
                        if (VBOX_FAILURE(rc))
                            break;
                    }
                }
                RTMemTmpFree(pvBuf);
            }
            else
            {
                /* alloc error */
                rc = VERR_NO_MEMORY;
            }
        }

    l_create_failed:

        if (cbLock)
            RTFileUnlock(pImage->File, 0, cbLock);

        RTFileClose(pImage->File);

        /* Delete image file if error occured while creating */
        if (VBOX_FAILURE(rc))
            RTFileDelete(pszFilename);
    }

    RTMemFree(pImage->paBlocks);
    RTMemFree(pImage);

    if (    VBOX_SUCCESS(rc)
        &&  pfnProgress)
        pfnProgress(NULL /* WARNING! pVM=NULL  */, 100, pvUser);

    Log(("vdiCreateImage: done, filename=\"%s\", rc=%Vrc\n", pszFilename, rc));

    return rc;
}

/**
 * Open an image.
 * @internal
 */
static int vdiOpenImage(PVDIIMAGEDESC *ppImage, const char *pszFilename,
                        unsigned fOpen, PVDIIMAGEDESC pParent)
{
    /*
     * Validate input.
     */
    Assert(ppImage);
    Assert(pszFilename);
    Assert(!(fOpen & ~VDI_OPEN_FLAGS_MASK));

    PVDIIMAGEDESC   pImage;
    size_t          cchFilename = strlen(pszFilename);
    if (cchFilename >= sizeof(pImage->szFilename))
    {
        AssertMsgFailed(("filename=\"%s\" is too long (%d bytes)!\n", pszFilename, cchFilename));
        return VERR_FILENAME_TOO_LONG;
    }

    pImage = (PVDIIMAGEDESC)RTMemAllocZ(sizeof(VDIIMAGEDESC));
    if (!pImage)
        return VERR_NO_MEMORY;
    vdiInitImageDesc(pImage);

    memcpy(pImage->szFilename, pszFilename, cchFilename);
    pImage->fOpen = fOpen;

    /*
     * Open the image.
     */
    int rc = RTFileOpen(&pImage->File,
                        pImage->szFilename,
                        fOpen & VDI_OPEN_FLAGS_READONLY
                        ? RTFILE_O_READ      | RTFILE_O_OPEN | RTFILE_O_DENY_NONE
                        : RTFILE_O_READWRITE | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
    if (VBOX_FAILURE(rc))
    {
        if (!(fOpen & VDI_OPEN_FLAGS_READONLY))
        {
            /* Try to open image for reading only. */
            rc = RTFileOpen(&pImage->File,
                            pImage->szFilename,
                            RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_NONE);
            if (VBOX_SUCCESS(rc))
                pImage->fOpen |= VDI_OPEN_FLAGS_READONLY;
        }
        if (VBOX_FAILURE(rc))
        {
            RTMemFree(pImage);
            return rc;
        }
    }
    /* Set up current image r/w state. */
    pImage->fReadOnly = !!(pImage->fOpen & VDI_OPEN_FLAGS_READONLY);

    /*
     * Set initial file lock for reading header only.
     * Length of lock doesn't matter, it just must include image header.
     */
    uint64_t cbLock = _1M;
    rc = RTFileLock(pImage->File, RTFILE_LOCK_READ | RTFILE_LOCK_IMMEDIATELY, 0, cbLock);
    if (VBOX_FAILURE(rc))
    {
        cbLock = 0;
        goto l_open_failed;
    }

    /* Read pre-header. */
    rc = RTFileRead(pImage->File, &pImage->PreHeader, sizeof(pImage->PreHeader), NULL);
    if (VBOX_FAILURE(rc))
        goto l_open_failed;
    rc = vdiValidatePreHeader(&pImage->PreHeader);
    if (VBOX_FAILURE(rc))
        goto l_open_failed;

    /* Read header. */
    pImage->Header.uVersion = pImage->PreHeader.u32Version;
    switch (GET_MAJOR_HEADER_VERSION(&pImage->Header))
    {
        case 0:
            rc = RTFileRead(pImage->File, &pImage->Header.u.v0, sizeof(pImage->Header.u.v0), NULL);
            break;
        case 1:
            rc = RTFileRead(pImage->File, &pImage->Header.u.v1, sizeof(pImage->Header.u.v1), NULL);
            break;
        default:
            rc = VERR_VDI_UNSUPPORTED_VERSION;
            break;
    }
    if (VBOX_FAILURE(rc))
        goto l_open_failed;

    rc = vdiValidateHeader(&pImage->Header);
    if (VBOX_FAILURE(rc))
        goto l_open_failed;

    /* Check diff image correctness. */
    if (pParent)
    {
        if (pImage->PreHeader.u32Version != pParent->PreHeader.u32Version)
        {
            rc = VERR_VDI_IMAGES_VERSION_MISMATCH;
            goto l_open_failed;
        }

        if (    getImageType(&pImage->Header) != VDI_IMAGE_TYPE_UNDO
            &&  getImageType(&pImage->Header) != VDI_IMAGE_TYPE_DIFF)
        {
            rc = VERR_VDI_WRONG_DIFF_IMAGE;
            goto l_open_failed;
        }

        if (    getImageDiskSize(&pImage->Header) != getImageDiskSize(&pParent->Header)
            ||  getImageBlockSize(&pImage->Header) != getImageBlockSize(&pParent->Header)
            ||  getImageBlocks(&pImage->Header) != getImageBlocks(&pParent->Header)
            ||  getImageExtraBlockSize(&pImage->Header) != getImageExtraBlockSize(&pParent->Header))
        {
            rc = VERR_VDI_WRONG_DIFF_IMAGE;
            goto l_open_failed;
        }

        /* Check linkage data. */
        if (    RTUuidCompare(getImageParentUUID(&pImage->Header),
                              getImageCreationUUID(&pParent->Header))
            ||  RTUuidCompare(getImageParentModificationUUID(&pImage->Header),
                              getImageModificationUUID(&pParent->Header)))
        {
            rc = VERR_VDI_IMAGES_UUID_MISMATCH;
            goto l_open_failed;
        }
    }

    /* Setup image parameters by header. */
    vdiSetupImageDesc(pImage);

    /* reset modified flag into first-modified state. */
    pImage->fModified = VDI_IMAGE_MODIFIED_FIRST;

    /* Image is validated, set working file lock on it. */
    rc = RTFileUnlock(pImage->File, 0, cbLock);
    AssertRC(rc);
    cbLock = pImage->offStartData
           + ((uint64_t)getImageBlocks(&pImage->Header) << pImage->uShiftIndex2Offset);
    rc = RTFileLock(pImage->File,
                    (pImage->fReadOnly) ?
                        RTFILE_LOCK_READ | RTFILE_LOCK_IMMEDIATELY :
                        RTFILE_LOCK_WRITE | RTFILE_LOCK_IMMEDIATELY,
                    0,
                    cbLock);
    if (    VBOX_FAILURE(rc)
        &&  !pImage->fReadOnly)
    {
        /* Failed to lock image for writing, try read-only lock. */
        rc = RTFileLock(pImage->File,
                        RTFILE_LOCK_READ | RTFILE_LOCK_IMMEDIATELY, 0, cbLock);
        if (VBOX_SUCCESS(rc))
            pImage->fReadOnly = true;
    }
    if (VBOX_FAILURE(rc))
    {
        cbLock = 0;    /* Not locked. */
        goto l_open_failed;
    }

    /* Allocate memory for blocks array. */
    pImage->paBlocks = (PVDIIMAGEBLOCKPOINTER)RTMemAlloc(sizeof(VDIIMAGEBLOCKPOINTER) * getImageBlocks(&pImage->Header));
    if (!pImage->paBlocks)
    {
        rc = VERR_NO_MEMORY;
        goto l_open_failed;
    }

    /* Read blocks array. */
    rc = RTFileSeek(pImage->File, pImage->offStartBlocks, RTFILE_SEEK_BEGIN, NULL);
    if (VBOX_FAILURE(rc))
        goto l_open_failed;
    rc = RTFileRead(pImage->File, pImage->paBlocks,
                    getImageBlocks(&pImage->Header) * sizeof(VDIIMAGEBLOCKPOINTER), NULL);
    if (VBOX_FAILURE(rc))
        goto l_open_failed;

    /* all done. */
    *ppImage = pImage;
    return VINF_SUCCESS;

l_open_failed:
    /* Clean up. */
    if (pImage->paBlocks)
        RTMemFree(pImage->paBlocks);
    if (cbLock)
        RTFileUnlock(pImage->File, 0, cbLock);
    RTFileClose(pImage->File);
    RTMemFree(pImage);
    Log(("vdiOpenImage: failed, filename=\"%s\", rc=%Vrc\n", pszFilename, rc));
    return rc;
}

/**
 * internal: save header to file.
 */
static int vdiUpdateHeader(PVDIIMAGEDESC pImage)
{
    /* Seek to header start. */
    int rc = RTFileSeek(pImage->File, sizeof(VDIPREHEADER), RTFILE_SEEK_BEGIN, NULL);
    if (VBOX_SUCCESS(rc))
    {
        switch (GET_MAJOR_HEADER_VERSION(&pImage->Header))
        {
            case 0:
                rc = RTFileWrite(pImage->File, &pImage->Header.u.v0, sizeof(pImage->Header.u.v0), NULL);
                break;
            case 1:
                rc = RTFileWrite(pImage->File, &pImage->Header.u.v1, sizeof(pImage->Header.u.v1), NULL);
                break;
            default:
                rc = VERR_VDI_UNSUPPORTED_VERSION;
                break;
        }
    }
    AssertMsgRC(rc, ("vdiUpdateHeader failed, filename=\"%s\" rc=%Vrc\n", pImage->szFilename, rc));
    return rc;
}

/**
 * internal: save block pointer to file, save header to file.
 */
static int vdiUpdateBlockInfo(PVDIIMAGEDESC pImage, unsigned uBlock)
{
    /* Update image header. */
    int rc = vdiUpdateHeader(pImage);
    if (VBOX_SUCCESS(rc))
    {
        /* write only one block pointer. */
        rc = RTFileSeek(pImage->File,
                        pImage->offStartBlocks + uBlock * sizeof(VDIIMAGEBLOCKPOINTER),
                        RTFILE_SEEK_BEGIN,
                        NULL);
        if (VBOX_SUCCESS(rc))
            rc = RTFileWrite(pImage->File,
                             &pImage->paBlocks[uBlock],
                             sizeof(VDIIMAGEBLOCKPOINTER),
                             NULL);
        AssertMsgRC(rc, ("vdiUpdateBlockInfo failed to update block=%u, filename=\"%s\", rc=%Vrc\n",
                         uBlock, pImage->szFilename, rc));
    }
    return rc;
}

/**
 * internal: save blocks array to file, save header to file.
 */
static int vdiUpdateBlocks(PVDIIMAGEDESC pImage)
{
    /* Update image header. */
    int rc = vdiUpdateHeader(pImage);
    if (VBOX_SUCCESS(rc))
    {
        /* write the block pointers array. */
        rc = RTFileSeek(pImage->File, pImage->offStartBlocks, RTFILE_SEEK_BEGIN, NULL);
        if (VBOX_SUCCESS(rc))
            rc = RTFileWrite(pImage->File,
                             pImage->paBlocks,
                             sizeof(VDIIMAGEBLOCKPOINTER) * getImageBlocks(&pImage->Header),
                             NULL);
        AssertMsgRC(rc, ("vdiUpdateBlocks failed, filename=\"%s\", rc=%Vrc\n",
                         pImage->szFilename, rc));
    }
    return rc;
}

/**
 * internal: mark image as modified, if this is the first change - update image header
 * on disk with a new uuidModify value.
 */
static void vdiSetModifiedFlag(PVDIIMAGEDESC pImage)
{
    pImage->fModified |= VDI_IMAGE_MODIFIED_FLAG;
    if (pImage->fModified & VDI_IMAGE_MODIFIED_FIRST)
    {
        pImage->fModified &= ~VDI_IMAGE_MODIFIED_FIRST;

        /* first modify - generate uuidModify and save to file. */
        vdiResetModifiedFlag(pImage);

        if (!(pImage->fModified | VDI_IMAGE_MODIFIED_DISABLE_UUID_UPDATE))
        {
            /* save header to file,
             * note: no rc checking.
             */
            vdiUpdateHeader(pImage);
        }
    }
}

/**
 * internal: generate new uuidModify if the image was changed.
 */
static void vdiResetModifiedFlag(PVDIIMAGEDESC pImage)
{
    if (pImage->fModified & VDI_IMAGE_MODIFIED_FLAG)
    {
        /* generate new last-modified uuid */
        if (!(pImage->fModified | VDI_IMAGE_MODIFIED_DISABLE_UUID_UPDATE))
            RTUuidCreate(getImageModificationUUID(&pImage->Header));

        pImage->fModified &= ~VDI_IMAGE_MODIFIED_FLAG;
    }
}

/**
 * internal: disables updates of the last-modified UUID
 * when performing image writes.
 */
static void vdiDisableLastModifiedUpdate(PVDIIMAGEDESC pImage)
{
    pImage->fModified |= VDI_IMAGE_MODIFIED_DISABLE_UUID_UPDATE;
}

#if 0 /* unused */
/**
 * internal: enables updates of the last-modified UUID
 * when performing image writes.
 */
static void vdiEnableLastModifiedUpdate(PVDIIMAGEDESC pImage)
{
    pImage->fModified &= ~VDI_IMAGE_MODIFIED_DISABLE_UUID_UPDATE;
}
#endif

/**
 * internal: flush image file to disk.
 */
static void vdiFlushImage(PVDIIMAGEDESC pImage)
{
    if (!pImage->fReadOnly)
    {
        /* Update last-modified uuid if need. */
        vdiResetModifiedFlag(pImage);

        /* Save header. */
        int rc = vdiUpdateHeader(pImage);
        AssertMsgRC(rc, ("vdiUpdateHeader() failed, filename=\"%s\", rc=%Vrc\n",
                         pImage->szFilename, rc));
        RTFileFlush(pImage->File);
    }
}

/**
 * internal: close image file.
 */
static void vdiCloseImage(PVDIIMAGEDESC pImage)
{
    /* Params checking. */
    Assert(pImage);
    Assert(pImage->File != NIL_RTFILE);

    vdiFlushImage(pImage);
    RTFileUnlock(pImage->File,
                 0,
                 pImage->offStartData
               + ((uint64_t)getImageBlocks(&pImage->Header) << pImage->uShiftIndex2Offset));
    RTFileClose(pImage->File);

    /* free image resources */
    RTMemFree(pImage->paBlocks);
    RTMemFree(pImage);
}

/**
 * internal: read data inside image block.
 *
 * note: uBlock must be valid, readed data must not overlap block bounds.
 */
static int vdiReadInBlock(PVDIIMAGEDESC pImage, unsigned uBlock, unsigned offRead,
                          unsigned cbToRead, void *pvBuf)
{
    if (IS_VDI_IMAGE_BLOCK_ALLOCATED(pImage->paBlocks[uBlock]))
    {
        /* block present in image file */
        uint64_t u64Offset = ((uint64_t)pImage->paBlocks[uBlock] << pImage->uShiftIndex2Offset)
                           + (pImage->offStartData + pImage->offStartBlockData + offRead);
        int rc = RTFileSeek(pImage->File, u64Offset, RTFILE_SEEK_BEGIN, NULL);
        if (VBOX_SUCCESS(rc))
            rc = RTFileRead(pImage->File, pvBuf, cbToRead, NULL);
        if (VBOX_FAILURE(rc))
            Log(("vdiReadInBlock: rc=%Vrc filename=\"%s\" uBlock=%u offRead=%u cbToRead=%u u64Offset=%llu\n",
                 rc, pImage->szFilename, uBlock, offRead, cbToRead, u64Offset));
        return rc;
    }

    /* Returns zeroes for both free and zero block types. */
    memset(pvBuf, 0, cbToRead);
    return VINF_SUCCESS;
}

/**
 * Read data from virtual HDD.
 *
 * @returns VBox status code.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   offStart        Offset of first reading byte from start of disk.
 * @param   pvBuf           Pointer to buffer for reading data.
 * @param   cbToRead        Number of bytes to read.
 */
IDER3DECL(int) VDIDiskRead(PVDIDISK pDisk, uint64_t offStart, void *pvBuf, unsigned cbToRead)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    PVDIIMAGEDESC pImage = pDisk->pLast;
    Assert(pImage);

    /* Check params. */
    if (    offStart + cbToRead > getImageDiskSize(&pImage->Header)
        ||  cbToRead == 0)
    {
        AssertMsgFailed(("offStart=%llu cbToRead=%u\n", offStart, cbToRead));
        return VERR_INVALID_PARAMETER;
    }

    /* Calculate starting block number and offset inside it. */
    unsigned uBlock = (unsigned)(offStart >> pImage->uShiftOffset2Index);
    unsigned offRead = (unsigned)offStart & pImage->uBlockMask;

    /* Save block size here for speed optimization. */
    unsigned cbBlock = getImageBlockSize(&pImage->Header);

    /* loop through blocks */
    int rc;
    for (;;)
    {
        unsigned to_read;
        if ((offRead + cbToRead) <= cbBlock)
            to_read = cbToRead;
        else
            to_read = cbBlock - offRead;

        if (pDisk->cImages > 1)
        {
            /* Differencing images are used, handle them. */
            pImage = pDisk->pLast;

            /* Search for image with allocated block. */
            while (pImage->paBlocks[uBlock] == VDI_IMAGE_BLOCK_FREE)
            {
                pImage = pImage->pPrev;
                if (!pImage)
                {
                    /* Block is not allocated in all images of chain. */
                    pImage = pDisk->pLast;
                    break;
                }
            }
        }

        rc = vdiReadInBlock(pImage, uBlock, offRead, to_read, pvBuf);

        cbToRead -= to_read;
        if (    cbToRead == 0
            ||  VBOX_FAILURE(rc))
            break;

        /* goto next block */
        uBlock++;
        offRead = 0;
        pvBuf = (char *)pvBuf + to_read;
    }

    return rc;
}

/**
 * internal: fill the whole block with zeroes.
 *
 * note: block id must be valid, block must be already allocated in file.
 * note: if pDisk is NULL, the default buffer size is used
 */
static int vdiFillBlockByZeroes(PVDIDISK pDisk, PVDIIMAGEDESC pImage, unsigned uBlock)
{
    int rc;

    /* seek to start of block in file. */
    uint64_t u64Offset = ((uint64_t)pImage->paBlocks[uBlock] << pImage->uShiftIndex2Offset)
                       + (pImage->offStartData + pImage->offStartBlockData);
    rc = RTFileSeek(pImage->File, u64Offset, RTFILE_SEEK_BEGIN, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("vdiFillBlockByZeroes: seek rc=%Vrc filename=\"%s\" uBlock=%u u64Offset=%llu\n",
             rc, pImage->szFilename, uBlock, u64Offset));
        return rc;
    }

    /* alloc tmp zero-filled buffer */
    void *pvBuf = RTMemTmpAllocZ(pDisk ? pDisk->cbBuf : VDIDISK_DEFAULT_BUFFER_SIZE);
    if (!pvBuf)
        return VERR_NO_MEMORY;

    unsigned cbFill = getImageBlockSize(&pImage->Header);

    /* do loop, because buffer size may be less then block size */
    while (cbFill > 0)
    {
        unsigned to_fill = RT_MIN(cbFill, pDisk ? pDisk->cbBuf : VDIDISK_DEFAULT_BUFFER_SIZE);
        rc = RTFileWrite(pImage->File, pvBuf, to_fill, NULL);
        if (VBOX_FAILURE(rc))
        {
            Log(("vdiFillBlockByZeroes: write rc=%Vrc filename=\"%s\" uBlock=%u u64Offset=%llu cbFill=%u to_fill=%u\n",
                 rc, pImage->szFilename, uBlock, u64Offset, cbFill, to_fill));
            break;
        }

        cbFill -= to_fill;
    }

    RTMemTmpFree(pvBuf);
    return rc;
}

/**
 * internal: write data inside image block.
 *
 * note: uBlock must be valid, written data must not overlap block bounds.
 */
static int vdiWriteInBlock(PVDIDISK pDisk, PVDIIMAGEDESC pImage, unsigned uBlock, unsigned offWrite, unsigned cbToWrite, const void *pvBuf)
{
    int rc;

    /* Check if we can write into file. */
    if (pImage->fReadOnly)
    {
        Log(("vdiWriteInBlock: failed, image \"%s\" is read-only!\n", pImage->szFilename));
        return VERR_WRITE_PROTECT;
    }

    vdiSetModifiedFlag(pImage);

    if (!IS_VDI_IMAGE_BLOCK_ALLOCATED(pImage->paBlocks[uBlock]))
    {
        /* need to allocate a new block in image file */

        /* expand file by one block */
        uint64_t u64Size = (((uint64_t)(getImageBlocksAllocated(&pImage->Header) + 1)) << pImage->uShiftIndex2Offset)
                         + pImage->offStartData;
        rc = RTFileSetSize(pImage->File, u64Size);
        if (VBOX_FAILURE(rc))
        {
            Log(("vdiWriteInBlock: set size rc=%Vrc filename=\"%s\" uBlock=%u u64Size=%llu\n",
                 rc, pImage->szFilename, uBlock, u64Size));
            return rc;
        }

        unsigned cBlocksAllocated = getImageBlocksAllocated(&pImage->Header);
        pImage->paBlocks[uBlock] = cBlocksAllocated;
        setImageBlocksAllocated(&pImage->Header, cBlocksAllocated + 1);

        if (    pImage->fFlags & VDI_IMAGE_FLAGS_ZERO_EXPAND
            ||  pImage->paBlocks[uBlock] == VDI_IMAGE_BLOCK_ZERO)
        {
            /* Fill newly allocated block by zeroes. */

            if (offWrite || cbToWrite != getImageBlockSize(&pImage->Header))
            {
                rc = vdiFillBlockByZeroes(pDisk, pImage, uBlock);
                if (VBOX_FAILURE(rc))
                    return rc;
            }
        }

        rc = vdiUpdateBlockInfo(pImage, uBlock);
        if (VBOX_FAILURE(rc))
            return rc;
    }

    /* Now block present in image file, write data inside it. */
    uint64_t u64Offset = ((uint64_t)pImage->paBlocks[uBlock] << pImage->uShiftIndex2Offset)
                       + (pImage->offStartData + pImage->offStartBlockData + offWrite);
    rc = RTFileSeek(pImage->File, u64Offset, RTFILE_SEEK_BEGIN, NULL);
    if (VBOX_SUCCESS(rc))
    {
        rc = RTFileWrite(pImage->File, pvBuf, cbToWrite, NULL);
        if (VBOX_FAILURE(rc))
            Log(("vdiWriteInBlock: write rc=%Vrc filename=\"%s\" uBlock=%u offWrite=%u u64Offset=%llu cbToWrite=%u\n",
                 rc, pImage->szFilename, uBlock, offWrite, u64Offset, cbToWrite));
    }
    else
        Log(("vdiWriteInBlock: seek rc=%Vrc filename=\"%s\" uBlock=%u offWrite=%u u64Offset=%llu\n",
             rc, pImage->szFilename, uBlock, offWrite, u64Offset));

    return rc;
}

/**
 * internal: copy data block from one (parent) image to last image.
 */
static int vdiCopyBlock(PVDIDISK pDisk, PVDIIMAGEDESC pImage, unsigned uBlock)
{
    Assert(pImage != pDisk->pLast);

    if (pImage->paBlocks[uBlock] == VDI_IMAGE_BLOCK_ZERO)
    {
        /*
         * if src block is zero, set dst block to zero too.
         */
        pDisk->pLast->paBlocks[uBlock] = VDI_IMAGE_BLOCK_ZERO;
        return VINF_SUCCESS;
    }

    /* alloc tmp buffer */
    void *pvBuf = RTMemTmpAlloc(pDisk->cbBuf);
    if (!pvBuf)
        return VERR_NO_MEMORY;

    int rc = VINF_SUCCESS;

    unsigned cbCopy = getImageBlockSize(&pImage->Header);
    unsigned offCopy = 0;

    /* do loop, because buffer size may be less then block size */
    while (cbCopy > 0)
    {
        unsigned to_copy = RT_MIN(cbCopy, pDisk->cbBuf);
        rc = vdiReadInBlock(pImage, uBlock, offCopy, to_copy, pvBuf);
        if (VBOX_FAILURE(rc))
            break;

        rc = vdiWriteInBlock(pDisk, pDisk->pLast, uBlock, offCopy, to_copy, pvBuf);
        if (VBOX_FAILURE(rc))
            break;

        cbCopy -= to_copy;
        offCopy += to_copy;
    }

    RTMemTmpFree(pvBuf);
    return rc;
}

/**
 * Write data to virtual HDD.
 *
 * @returns VBox status code.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   offStart        Offset of first writing byte from start of HDD.
 * @param   pvBuf           Pointer to buffer of writing data.
 * @param   cbToWrite       Number of bytes to write.
 */
IDER3DECL(int) VDIDiskWrite(PVDIDISK pDisk, uint64_t offStart, const void *pvBuf, unsigned cbToWrite)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    PVDIIMAGEDESC pImage = pDisk->pLast;
    Assert(pImage);

    /* Check params. */
    if (    offStart + cbToWrite > getImageDiskSize(&pImage->Header)
        ||  cbToWrite == 0)
    {
        AssertMsgFailed(("offStart=%llu cbToWrite=%u\n", offStart, cbToWrite));
        return VERR_INVALID_PARAMETER;
    }

    /* Calculate starting block number and offset inside it. */
    unsigned uBlock   = (unsigned)(offStart >> pImage->uShiftOffset2Index);
    unsigned offWrite = (unsigned)offStart   & pImage->uBlockMask;
    unsigned cbBlock  = getImageBlockSize(&pImage->Header);

    /* loop through blocks */
    int rc = VINF_SUCCESS;
    for (;;)
    {
        unsigned to_write;
        if (offWrite + cbToWrite <= cbBlock)
            to_write = cbToWrite;
        else
            to_write = cbBlock - offWrite;

        /* All callers write less than a VDI block right now (assuming
         * default VDI block size). So not worth optimizing for the case
         * where a full block is overwritten (no copying required).
         * Checking whether a block is all zeroes after the write is too
         * expensive (would require reading the rest of the block). */

        if (pDisk->cImages > 1)
        {
            /* Differencing images are used, handle them. */

            /* Search for image with allocated block. */
            while (pImage->paBlocks[uBlock] == VDI_IMAGE_BLOCK_FREE)
            {
                pImage = pImage->pPrev;
                if (!pImage)
                {
                    /* Block is not allocated in all images of chain. */
                    pImage = pDisk->pLast;
                    break;
                }
            }

            if (pImage != pDisk->pLast)
            {
                /* One of parent image has a block data, copy it into last image. */
                rc = vdiCopyBlock(pDisk, pImage, uBlock);
                if (VBOX_FAILURE(rc))
                    break;
                pImage = pDisk->pLast;
            }
        }

        /* If the destination block is unallocated at this point, it's either
         * a zero block or a block which hasn't been used so far (which also
         * means that it's a zero block. Don't need to write anything to this
         * block if the data consists of just zeroes. */
        bool fBlockZeroed = false; /* assume data, for blocks already with data */
        if (!IS_VDI_IMAGE_BLOCK_ALLOCATED(pImage->paBlocks[uBlock]))
        {
            /* Check block for data. */
            fBlockZeroed = true;    /* Block is zeroed flag. */
            for (unsigned i = 0; i < (to_write >> 2); i++)
                if (((uint32_t *)pvBuf)[i] != 0)
                {
                    /* Block is not zeroed! */
                    fBlockZeroed = false;
                    break;
                }
        }

        /* Actually write the data into block. */
        if (!fBlockZeroed)
            rc = vdiWriteInBlock(pDisk, pImage, uBlock, offWrite, to_write, pvBuf);

        cbToWrite -= to_write;
        if (    cbToWrite == 0
            || VBOX_FAILURE(rc))
            break;

        /* goto next block */
        uBlock++;
        offWrite = 0;
        pvBuf = (char *)pvBuf + to_write;
    }

    return rc;
}

/**
 * internal: commit one image to another, no changes to header, just
 * plain copy operation. Blocks that are not allocated in the source
 * image (i.e. inherited by its parent(s)) are not merged.
 *
 * @param pImageFrom        source image
 * @param pImageTo          target image (will receive all the modifications)
 * @param fParentToChild    true if the source image is parent of the target one,
 *                          false of the target image is the parent of the source.
 * @param pfnProgress       progress callback (NULL if not to be used)
 * @param pvUser            user argument for the progress callback
 *
 * @note the target image has to be opened read/write
 * @note this method does not check whether merging is possible!
 */
static int vdiMergeImages(PVDIIMAGEDESC pImageFrom, PVDIIMAGEDESC pImageTo, bool fParentToChild,
                          PFNVMPROGRESS pfnProgress, void *pvUser)
{
    Assert(pImageFrom);
    Assert(pImageTo);

    Log(("vdiMergeImages: merging from image \"%s\" to image \"%s\" (fParentToChild=%d)\n",
         pImageFrom->szFilename, pImageTo->szFilename, fParentToChild));

    /* alloc tmp buffer */
    void *pvBuf = RTMemTmpAlloc(VDIDISK_DEFAULT_BUFFER_SIZE);
    if (!pvBuf)
        return VERR_NO_MEMORY;

    int rc = VINF_SUCCESS;

    if (!fParentToChild)
    {
        /*
         *  Commit the child image to the parent image.
         *  Child is the source (from), parent is the target (to).
         */

        unsigned cBlocks = getImageBlocks(&pImageFrom->Header);

        for (unsigned uBlock = 0; uBlock < cBlocks; uBlock++)
        {
            /* only process blocks that are allocated in the source image */
            if (pImageFrom->paBlocks[uBlock] != VDI_IMAGE_BLOCK_FREE)
            {
                /* Found used block in source image, commit it. */
                if (    pImageFrom->paBlocks[uBlock] == VDI_IMAGE_BLOCK_ZERO
                    &&  !IS_VDI_IMAGE_BLOCK_ALLOCATED(pImageTo->paBlocks[uBlock]))
                {
                    /* Block is zero in the source image and not allocated in the target image. */
                    pImageTo->paBlocks[uBlock] = VDI_IMAGE_BLOCK_ZERO;
                    vdiSetModifiedFlag(pImageTo);
                }
                else
                {
                    /* Block is not zero / allocated in source image. */
                    unsigned cbCommit = getImageBlockSize(&pImageFrom->Header);
                    unsigned offCommit = 0;

                    /* do loop, because buffer size may be less then block size */
                    while (cbCommit > 0)
                    {
                        unsigned cbToCopy = RT_MIN(cbCommit, VDIDISK_DEFAULT_BUFFER_SIZE);

                        rc = vdiReadInBlock(pImageFrom, uBlock, offCommit, cbToCopy, pvBuf);
                        if (VBOX_FAILURE(rc))
                            break;

                        rc = vdiWriteInBlock(NULL, pImageTo, uBlock, offCommit, cbToCopy, pvBuf);
                        if (VBOX_FAILURE(rc))
                            break;

                        cbCommit -= cbToCopy;
                        offCommit += cbToCopy;
                    }
                    if (VBOX_FAILURE(rc))
                        break;
                }
            }

            if (pfnProgress)
            {
                pfnProgress(NULL /* WARNING! pVM=NULL  */,
                            (uBlock * 100) / cBlocks,
                            pvUser);
                /* Note: commiting is non breakable operation, skipping rc here. */
            }
        }
    }
    else
    {
        /*
         *  Commit the parent image to the child image.
         *  Parent is the source (from), child is the target (to).
         */

        unsigned cBlocks = getImageBlocks(&pImageFrom->Header);

        for (unsigned uBlock = 0; uBlock < cBlocks; uBlock++)
        {
            /*
             *  only process blocks that are allocated or zero in the source image
             *  and NEITHER allocated NOR zero in the target image
             */
            if (pImageFrom->paBlocks[uBlock] != VDI_IMAGE_BLOCK_FREE &&
                pImageTo->paBlocks[uBlock] == VDI_IMAGE_BLOCK_FREE)
            {
                /* Found used block in source image (but unused in target), commit it. */
                if (    pImageFrom->paBlocks[uBlock] == VDI_IMAGE_BLOCK_ZERO)
                {
                    /* Block is zero in the source image and not allocated in the target image. */
                    pImageTo->paBlocks[uBlock] = VDI_IMAGE_BLOCK_ZERO;
                    vdiSetModifiedFlag(pImageTo);
                }
                else
                {
                    /* Block is not zero / allocated in source image. */
                    unsigned cbCommit = getImageBlockSize(&pImageFrom->Header);
                    unsigned offCommit = 0;

                    /* do loop, because buffer size may be less then block size */
                    while (cbCommit > 0)
                    {
                        unsigned cbToCopy = RT_MIN(cbCommit, VDIDISK_DEFAULT_BUFFER_SIZE);

                        rc = vdiReadInBlock(pImageFrom, uBlock, offCommit, cbToCopy, pvBuf);
                        if (VBOX_FAILURE(rc))
                            break;

                        rc = vdiWriteInBlock(NULL, pImageTo, uBlock, offCommit, cbToCopy, pvBuf);
                        if (VBOX_FAILURE(rc))
                            break;

                        cbCommit -= cbToCopy;
                        offCommit += cbToCopy;
                    }
                    if (VBOX_FAILURE(rc))
                        break;
                }
            }

            if (pfnProgress)
            {
                pfnProgress(NULL /* WARNING! pVM=NULL  */,
                            (uBlock * 100) / cBlocks,
                            pvUser);
                /* Note: commiting is non breakable operation, skipping rc here. */
            }
        }
    }

    RTMemTmpFree(pvBuf);
    return rc;
}

/**
 * internal: commit last image(s) to selected previous image.
 * note: all images accessed across this call must be opened in R/W mode.
 * @remark    Only used by tstVDI.
 */
static int vdiCommitToImage(PVDIDISK pDisk, PVDIIMAGEDESC pDstImage,
                            PFNVMPROGRESS pfnProgress, void *pvUser)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pDstImage);

    PVDIIMAGEDESC pImage = pDisk->pLast;
    Assert(pImage);
    Log(("vdiCommitToImage: commiting from image \"%s\" to image \"%s\"\n",
         pImage->szFilename, pDstImage->szFilename));
    if (pDstImage == pImage)
    {
        Log(("vdiCommitToImage: attempt to commit to the same image!\n"));
        return VERR_VDI_NO_DIFF_IMAGES;
    }

    /* Scan images for pDstImage. */
    while (pImage && pImage != pDstImage)
        pImage = pImage->pPrev;
    if (!pImage)
    {
        AssertMsgFailed(("Invalid arguments: pDstImage is not in images chain\n"));
        return VERR_INVALID_PARAMETER;
    }
    pImage = pDisk->pLast;

    /* alloc tmp buffer */
    void *pvBuf = RTMemTmpAlloc(pDisk->cbBuf);
    if (!pvBuf)
        return VERR_NO_MEMORY;

    int rc = VINF_SUCCESS;
    unsigned cBlocks = getImageBlocks(&pImage->Header);

    for (unsigned uBlock = 0; uBlock < cBlocks; uBlock++)
    {
        pImage = pDisk->pLast;

        /* Find allocated block to commit. */
        while (    pImage->paBlocks[uBlock] == VDI_IMAGE_BLOCK_FREE
               &&  pImage != pDstImage)
            pImage = pImage->pPrev;

        if (pImage != pDstImage)
        {
            /* Found used block in diff image (pImage), commit it. */
            if (    pImage->paBlocks[uBlock] == VDI_IMAGE_BLOCK_ZERO
                &&  !IS_VDI_IMAGE_BLOCK_ALLOCATED(pDstImage->paBlocks[uBlock]))
            {
                /* Block is zero in difference image and not allocated in primary image. */
                pDstImage->paBlocks[uBlock] = VDI_IMAGE_BLOCK_ZERO;
                vdiSetModifiedFlag(pDstImage);
            }
            else
            {
                /* Block is not zero / allocated in primary image. */
                unsigned cbCommit = getImageBlockSize(&pImage->Header);
                unsigned offCommit = 0;

                /* do loop, because buffer size may be less then block size */
                while (cbCommit > 0)
                {
                    unsigned cbToCopy = RT_MIN(cbCommit, pDisk->cbBuf);

                    rc = vdiReadInBlock(pImage, uBlock, offCommit, cbToCopy, pvBuf);
                    if (VBOX_FAILURE(rc))
                        break;

                    rc = vdiWriteInBlock(pDisk, pDstImage, uBlock, offCommit, cbToCopy, pvBuf);
                    if (VBOX_FAILURE(rc))
                        break;

                    cbCommit -= cbToCopy;
                    offCommit += cbToCopy;
                }
                if (VBOX_FAILURE(rc))
                    break;
            }
            pImage->paBlocks[uBlock] = VDI_IMAGE_BLOCK_FREE;
        }

        if (pfnProgress)
        {
            pfnProgress(NULL /* WARNING! pVM=NULL  */,
                        (uBlock * 100) / cBlocks,
                        pvUser);
            /* Note: commiting is non breakable operation, skipping rc here. */
        }
    }

    RTMemTmpFree(pvBuf);

    /* Go forward and update linkage information. */
    for (pImage = pDstImage; pImage; pImage = pImage->pNext)
    {
        /* generate new last-modified uuid. */
        RTUuidCreate(getImageModificationUUID(&pImage->Header));

        /* fix up linkage. */
        if (pImage != pDstImage)
            *getImageParentModificationUUID(&pImage->Header) = *getImageModificationUUID(&pImage->pPrev->Header);

        /* reset modified flag. */
        pImage->fModified = 0;
    }

    /* Process committed images - truncate them. */
    for (pImage = pDisk->pLast; pImage != pDstImage; pImage = pImage->pPrev)
    {
        /* note: can't understand how to do error works here? */

        setImageBlocksAllocated(&pImage->Header, 0);

        /* Truncate file. */
        int rc2 = RTFileSetSize(pImage->File, pImage->offStartData);
        if (VBOX_FAILURE(rc2))
        {
            rc = rc2;
            Log(("vdiCommitToImage: set size (truncate) rc=%Vrc filename=\"%s\"\n",
                 rc, pImage->szFilename));
        }

        /* Save header and blocks array. */
        rc2 = vdiUpdateBlocks(pImage);
        if (VBOX_FAILURE(rc2))
        {
            rc = rc2;
            Log(("vdiCommitToImage: update blocks and header rc=%Vrc filename=\"%s\"\n",
                 rc, pImage->szFilename));
        }
    }

    if (pfnProgress)
    {
        pfnProgress(NULL /* WARNING! pVM=NULL */, 100, pvUser);
       /* Note: commiting is non breakable operation, skipping rc here. */
    }

    Log(("vdiCommitToImage: done, rc=%Vrc\n", rc));

    return rc;
}

/**
 * Checks if image is available and not broken, returns some useful image parameters if requested.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the image file to check.
 * @param   puVersion       Where to store the version of image. NULL is ok.
 * @param   penmType        Where to store the type of image. NULL is ok.
 * @param   pcbSize         Where to store the size of image in bytes. NULL is ok.
 * @param   pUuid           Where to store the uuid of image creation. NULL is ok.
 * @param   pParentUuid     Where to store the UUID of the parent image. NULL is ok.
 * @param   pszComment      Where to store the comment string of image. NULL is ok.
 * @param   cbComment       The size of pszComment buffer. 0 is ok.
 */
IDER3DECL(int) VDICheckImage(const char *pszFilename, unsigned *puVersion, PVDIIMAGETYPE penmType,
                             uint64_t *pcbSize, PRTUUID pUuid, PRTUUID pParentUuid,
                             char *pszComment, unsigned cbComment)
{
    LogFlow(("VDICheckImage:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, VDI_OPEN_FLAGS_READONLY, NULL);
    if (VBOX_SUCCESS(rc))
    {
        Log(("VDICheckImage: filename=\"%s\" version=%08X type=%X cbDisk=%llu uuid={%Vuuid}\n",
             pszFilename,
             pImage->PreHeader.u32Version,
             getImageType(&pImage->Header),
             getImageDiskSize(&pImage->Header),
             getImageCreationUUID(&pImage->Header)));

        if (    pszComment
            &&  cbComment > 0)
        {
            char *pszTmp = getImageComment(&pImage->Header);
            unsigned cb = strlen(pszTmp);
            if (cbComment > cb)
                memcpy(pszComment, pszTmp, cb + 1);
            else
                rc = VERR_BUFFER_OVERFLOW;
        }
        if (VBOX_SUCCESS(rc))
        {
            if (puVersion)
                *puVersion = pImage->PreHeader.u32Version;
            if (penmType)
                *penmType = getImageType(&pImage->Header);
            if (pcbSize)
                *pcbSize = getImageDiskSize(&pImage->Header);
            if (pUuid)
                *pUuid = *getImageCreationUUID(&pImage->Header);
            if (pParentUuid)
                *pParentUuid = *getImageParentUUID(&pImage->Header);
        }
        vdiCloseImage(pImage);
    }

    LogFlow(("VDICheckImage: returns %Vrc\n", rc));
    return rc;
}

/**
 * Changes an image's comment string.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the image file to operate on.
 * @param   pszComment      New comment string (UTF-8). NULL is allowed to reset the comment.
 */
IDER3DECL(int) VDISetImageComment(const char *pszFilename, const char *pszComment)
{
    LogFlow(("VDISetImageComment:\n"));

    /*
     * Validate arguments.
     */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    const size_t cchComment = pszComment ? strlen(pszComment) : 0;
    if (cchComment >= VDI_IMAGE_COMMENT_SIZE)
    {
        Log(("VDISetImageComment: pszComment is too long, %d bytes!\n", cchComment));
        return VERR_VDI_COMMENT_TOO_LONG;
    }

    /*
     * Open the image for updating.
     */
    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, VDI_OPEN_FLAGS_NORMAL, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDISetImageComment: vdiOpenImage rc=%Vrc filename=\"%s\"!\n", rc, pszFilename));
        return rc;
    }
    if (!pImage->fReadOnly)
    {
        /* we don't support old style images */
        if (GET_MAJOR_HEADER_VERSION(&pImage->Header) == 1)
        {
            /*
             * Update the comment field, making sure to zero out all of the previous comment.
             */
            memset(pImage->Header.u.v1.szComment, '\0', VDI_IMAGE_COMMENT_SIZE);
            memcpy(pImage->Header.u.v1.szComment, pszComment, cchComment);

            /* write out new the header */
            rc = vdiUpdateHeader(pImage);
            AssertMsgRC(rc, ("vdiUpdateHeader() failed, filename=\"%s\", rc=%Vrc\n",
                             pImage->szFilename, rc));
        }
        else
        {
            Log(("VDISetImageComment: Unsupported version!\n"));
            rc = VERR_VDI_UNSUPPORTED_VERSION;
        }
    }
    else
    {
        Log(("VDISetImageComment: image \"%s\" is opened as read-only!\n", pszFilename));
        rc = VERR_VDI_IMAGE_READ_ONLY;
    }

    vdiCloseImage(pImage);
    return rc;
}

/**
 * Creates a new base image file.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the creating image file.
 * @param   enmType         Image type, only base image types are acceptable.
 * @param   cbSize          Image size in bytes.
 * @param   pszComment      Pointer to image comment. NULL is ok.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 */
IDER3DECL(int) VDICreateBaseImage(const char *pszFilename, VDIIMAGETYPE enmType, uint64_t cbSize,
                                  const char *pszComment, PFNVMPROGRESS pfnProgress, void *pvUser)
{
    LogFlow(("VDICreateBaseImage:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0'
        ||  (enmType != VDI_IMAGE_TYPE_NORMAL && enmType != VDI_IMAGE_TYPE_FIXED)
        ||  cbSize < VDI_IMAGE_DEFAULT_BLOCK_SIZE)
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p enmType=%x cbSize=%llu\n",
                         pszFilename, enmType, cbSize));
        return VERR_INVALID_PARAMETER;
    }

    int rc = vdiCreateImage(pszFilename, enmType, VDI_IMAGE_FLAGS_DEFAULT, cbSize, pszComment, NULL,
                            pfnProgress, pvUser);
    LogFlow(("VDICreateBaseImage: returns %Vrc for filename=\"%s\"\n", rc, pszFilename));
    return rc;
}

/**
 * Creates a differencing dynamically growing image file for specified parent image.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the creating differencing image file.
 * @param   pszParent       Name of the parent image file. May be base or diff image type.
 * @param   pszComment      Pointer to image comment. NULL is ok.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 */
IDER3DECL(int) VDICreateDifferenceImage(const char *pszFilename, const char *pszParent,
                                        const char *pszComment, PFNVMPROGRESS pfnProgress,
                                        void *pvUser)
{
    LogFlow(("VDICreateDifferenceImage:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0'
        ||  !pszParent
        ||  *pszParent == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p pszParent=%p\n",
                         pszFilename, pszParent));
        return VERR_INVALID_PARAMETER;
    }

    PVDIIMAGEDESC pParent;
    int rc = vdiOpenImage(&pParent, pszParent, VDI_OPEN_FLAGS_READONLY, NULL);
    if (VBOX_SUCCESS(rc))
    {
        rc = vdiCreateImage(pszFilename, VDI_IMAGE_TYPE_DIFF, VDI_IMAGE_FLAGS_DEFAULT,
                            getImageDiskSize(&pParent->Header), pszComment, pParent,
                            pfnProgress, pvUser);
        vdiCloseImage(pParent);
    }
    LogFlow(("VDICreateDifferenceImage: returns %Vrc for filename=\"%s\"\n", rc, pszFilename));
    return rc;
}

/**
 * Deletes an image. Only valid image files can be deleted by this call.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the image file to check.
 */
IDER3DECL(int) VDIDeleteImage(const char *pszFilename)
{
    LogFlow(("VDIDeleteImage:\n"));
    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    int rc = VDICheckImage(pszFilename, NULL, NULL, NULL, NULL, NULL, NULL, 0);
    if (VBOX_SUCCESS(rc))
        rc = RTFileDelete(pszFilename);

    LogFlow(("VDIDeleteImage: returns %Vrc for filename=\"%s\"\n", rc, pszFilename));
    return rc;
}

/**
 * Makes a copy of image file with a new (other) creation uuid.
 *
 * @returns VBox status code.
 * @param   pszDstFilename  Name of the image file to create.
 * @param   pszSrcFilename  Name of the image file to copy from.
 * @param   pszComment      Pointer to image comment. If NULL specified comment
 *                          will be copied from source image.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 */
IDER3DECL(int) VDICopyImage(const char *pszDstFilename, const char *pszSrcFilename,
                            const char *pszComment, PFNVMPROGRESS pfnProgress, void *pvUser)
{
    LogFlow(("VDICopyImage:\n"));

    /* Check arguments. */
    if (    !pszDstFilename
        ||  *pszDstFilename == '\0'
        ||  !pszSrcFilename
        ||  *pszSrcFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszDstFilename=%p pszSrcFilename=%p\n",
                         pszDstFilename, pszSrcFilename));
        return VERR_INVALID_PARAMETER;
    }

    /* Special check for comment length. */
    if (    pszComment
        &&  strlen(pszComment) >= VDI_IMAGE_COMMENT_SIZE)
    {
        Log(("VDICopyImage: pszComment is too long, cb=%d\n", strlen(pszComment)));
        return VERR_VDI_COMMENT_TOO_LONG;
    }

    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszSrcFilename, VDI_OPEN_FLAGS_READONLY, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDICopyImage: src image \"%s\" open failed rc=%Vrc\n", pszSrcFilename, rc));
        return rc;
    }

    uint64_t cbFile = pImage->offStartData
                    + ((uint64_t)getImageBlocksAllocated(&pImage->Header) << pImage->uShiftIndex2Offset);

    /* create file */
    RTFILE File;
    rc = RTFileOpen(&File,
                    pszDstFilename,
                    RTFILE_O_READWRITE | RTFILE_O_CREATE | RTFILE_O_DENY_ALL);
    if (VBOX_SUCCESS(rc))
    {
        /* lock new image exclusively to close any wrong access by VDI API calls. */
        rc = RTFileLock(File, RTFILE_LOCK_WRITE | RTFILE_LOCK_IMMEDIATELY, 0, cbFile);
        if (VBOX_SUCCESS(rc))
        {
            /* Set the size of a new file. */
            rc = RTFileSetSize(File, cbFile);
            if (VBOX_SUCCESS(rc))
            {
                /* A dirty trick - use original image data to fill the new image. */
                RTFILE oldFileHandle = pImage->File;
                pImage->File = File;
                pImage->fReadOnly = false;

                /* generate a new image creation uuid. */
                RTUuidCreate(getImageCreationUUID(&pImage->Header));
                /* generate a new image last-modified uuid. */
                RTUuidCreate(getImageModificationUUID(&pImage->Header));
                /* set image comment, if present. */
                if (pszComment)
                    strncpy(getImageComment(&pImage->Header), pszComment, VDI_IMAGE_COMMENT_SIZE);

                /* Write the pre-header to new image. */
                rc = RTFileSeek(pImage->File, 0, RTFILE_SEEK_BEGIN, NULL);
                if (VBOX_SUCCESS(rc))
                    rc = RTFileWrite(pImage->File,
                                     &pImage->PreHeader,
                                     sizeof(pImage->PreHeader),
                                     NULL);

                /* Write the header and the blocks array to new image. */
                if (VBOX_SUCCESS(rc))
                    rc = vdiUpdateBlocks(pImage);

                pImage->File = oldFileHandle;
                pImage->fReadOnly = true;

                /* Seek to the data start in both images. */
                if (VBOX_SUCCESS(rc))
                    rc = RTFileSeek(pImage->File,
                                    pImage->offStartData,
                                    RTFILE_SEEK_BEGIN,
                                    NULL);
                if (VBOX_SUCCESS(rc))
                    rc = RTFileSeek(File,
                                    pImage->offStartData,
                                    RTFILE_SEEK_BEGIN,
                                    NULL);

                if (VBOX_SUCCESS(rc))
                {
                    /* alloc tmp buffer */
                    void *pvBuf = RTMemTmpAlloc(VDIDISK_DEFAULT_BUFFER_SIZE);
                    if (pvBuf)
                    {
                        /* Main copy loop. */
                        uint64_t cbData = cbFile - pImage->offStartData;
                        unsigned cBlocks = (unsigned)(cbData / VDIDISK_DEFAULT_BUFFER_SIZE);
                        unsigned c = 0;

                        while (cbData)
                        {
                            unsigned cbToCopy = (unsigned)RT_MIN(cbData, VDIDISK_DEFAULT_BUFFER_SIZE);

                            /* Read. */
                            rc = RTFileRead(pImage->File, pvBuf, cbToCopy, NULL);
                            if (VBOX_FAILURE(rc))
                                break;

                            /* Write. */
                            rc = RTFileWrite(File, pvBuf, cbToCopy, NULL);
                            if (VBOX_FAILURE(rc))
                                break;

                            if (pfnProgress)
                            {
                                c++;
                                rc = pfnProgress(NULL /* WARNING! pVM=NULL  */,
                                                 (c * 100) / cBlocks,
                                                 pvUser);
                                if (VBOX_FAILURE(rc))
                                    break;
                            }
                            cbData -= cbToCopy;
                        }

                        RTMemTmpFree(pvBuf);
                    }
                    else
                        rc = VERR_NO_MEMORY;
                }
            }

            RTFileUnlock(File, 0, cbFile);
        }

        RTFileClose(File);

        if (VBOX_FAILURE(rc))
            RTFileDelete(pszDstFilename);

        if (pfnProgress)
            pfnProgress(NULL /* WARNING! pVM=NULL  */, 100, pvUser);
    }

    vdiCloseImage(pImage);

    LogFlow(("VDICopyImage: returns %Vrc for pszSrcFilename=\"%s\" pszDstFilename=\"%s\"\n",
             rc, pszSrcFilename, pszDstFilename));
    return rc;
}

/**
 * Shrinks growing image file by removing zeroed data blocks.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the image file to shrink.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 */
IDER3DECL(int) VDIShrinkImage(const char *pszFilename, PFNVMPROGRESS pfnProgress, void *pvUser)
{
    LogFlow(("VDIShrinkImage:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, VDI_OPEN_FLAGS_NORMAL, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDIShrinkImage: vdiOpenImage rc=%Vrc filename=\"%s\"\n", rc, pszFilename));
        return rc;
    }
    if (pImage->fReadOnly)
    {
        Log(("VDIShrinkImage: image \"%s\" is opened as read-only!\n", pszFilename));
        vdiCloseImage(pImage);
        return VERR_VDI_IMAGE_READ_ONLY;
    }

    /* Do debug dump. */
    vdiDumpImage(pImage);

    /* Working data. */
    unsigned cbBlock          = getImageBlockSize(&pImage->Header);
    unsigned cBlocks          = getImageBlocks(&pImage->Header);
    unsigned cBlocksAllocated = getImageBlocksAllocated(&pImage->Header);

    uint64_t cbFile;
    rc = RTFileGetSize(pImage->File, &cbFile);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDIShrinkImage: RTFileGetSize rc=%Vrc for file=\"%s\"\n", rc, pszFilename));
        vdiCloseImage(pImage);
        return rc;
    }

    uint64_t cbData = cbFile - pImage->offStartData;
    unsigned cBlocksAllocated2 = (unsigned)(cbData >> pImage->uShiftIndex2Offset);
    if (cbData != (uint64_t)cBlocksAllocated << pImage->uShiftIndex2Offset)
        Log(("VDIShrinkImage: invalid image file length, cbBlock=%u cBlocks=%u cBlocksAllocated=%u cBlocksAllocated2=%u cbData=%llu\n",
             cbBlock, cBlocks, cBlocksAllocated, cBlocksAllocated2, cbData));

    /* Allocate second blocks array for back resolving. */
    PVDIIMAGEBLOCKPOINTER paBlocks2 =
                (PVDIIMAGEBLOCKPOINTER)RTMemTmpAlloc(sizeof(VDIIMAGEBLOCKPOINTER) * cBlocks);
    if (!paBlocks2)
    {
        Log(("VDIShrinkImage: failed to allocate paBlocks2 buffer (%u bytes)\n", sizeof(VDIIMAGEBLOCKPOINTER) * cBlocks));
        vdiCloseImage(pImage);
        return VERR_NO_MEMORY;
    }

    /* Init second blocks array. */
    for (unsigned n = 0; n < cBlocks; n++)
        paBlocks2[n] = VDI_IMAGE_BLOCK_FREE;

    /* Fill second blocks array, check for allocational errors. */
    for (unsigned n = 0; n < cBlocks; n++)
    {
        if (IS_VDI_IMAGE_BLOCK_ALLOCATED(pImage->paBlocks[n]))
        {
            unsigned uBlock = pImage->paBlocks[n];
            if (uBlock < cBlocksAllocated2)
            {
                if (paBlocks2[uBlock] == VDI_IMAGE_BLOCK_FREE)
                    paBlocks2[uBlock] = n;
                else
                {
                    Log(("VDIShrinkImage: block n=%u -> uBlock=%u is already in use!\n", n, uBlock));
                    /* free second link to block. */
                    pImage->paBlocks[n] = VDI_IMAGE_BLOCK_FREE;
                }
            }
            else
            {
                Log(("VDIShrinkImage: block n=%u -> uBlock=%u is out of blocks range! (cbBlock=%u cBlocks=%u cBlocksAllocated=%u cBlocksAllocated2=%u cbData=%llu)\n",
                     n, uBlock, cbBlock, cBlocks, cBlocksAllocated, cBlocksAllocated2, cbData));
                /* free link to invalid block. */
                pImage->paBlocks[n] = VDI_IMAGE_BLOCK_FREE;
            }
        }
    }

    /* Allocate a working buffer for one block. */
    void *pvBuf = RTMemTmpAlloc(cbBlock);
    if (pvBuf)
    {
        /* Main voodoo loop, search holes and fill it. */
        unsigned uBlockWrite = 0;
        for (unsigned uBlock = 0; uBlock < cBlocksAllocated2; uBlock++)
        {
            if (paBlocks2[uBlock] != VDI_IMAGE_BLOCK_FREE)
            {
                /* Read the block from file and check for zeroes. */
                uint64_t u64Offset = ((uint64_t)uBlock << pImage->uShiftIndex2Offset)
                                   + (pImage->offStartData + pImage->offStartBlockData);
                rc = RTFileSeek(pImage->File, u64Offset, RTFILE_SEEK_BEGIN, NULL);
                if (VBOX_FAILURE(rc))
                {
                    Log(("VDIShrinkImage: seek rc=%Vrc filename=\"%s\" uBlock=%u cBlocks=%u cBlocksAllocated=%u cBlocksAllocated2=%u cbData=%llu\n",
                         rc, pImage->szFilename, uBlock, cBlocks, cBlocksAllocated, cBlocksAllocated2, cbData));
                    break;
                }
                rc = RTFileRead(pImage->File, pvBuf, cbBlock, NULL);
                if (VBOX_FAILURE(rc))
                {
                    Log(("VDIShrinkImage: read rc=%Vrc filename=\"%s\" cbBlock=%u uBlock=%u cBlocks=%u cBlocksAllocated=%u cBlocksAllocated2=%u cbData=%llu\n",
                         rc, pImage->szFilename, cbBlock, uBlock, cBlocks, cBlocksAllocated, cBlocksAllocated2, cbData));
                    break;
                }

                /* Check block for data. */
                bool fBlockZeroed = true;    /* Block is zeroed flag. */
                for (unsigned i = 0; i < (cbBlock >> 2); i++)
                    if (((uint32_t *)pvBuf)[i] != 0)
                    {
                        /* Block is not zeroed! */
                        fBlockZeroed = false;
                        break;
                    }

                if (!fBlockZeroed)
                {
                    /* Block has a data, may be it must be moved. */
                    if (uBlockWrite < uBlock)
                    {
                        /* Move the block. */
                        u64Offset = ((uint64_t)uBlockWrite << pImage->uShiftIndex2Offset)
                                  + (pImage->offStartData + pImage->offStartBlockData);
                        rc = RTFileSeek(pImage->File, u64Offset, RTFILE_SEEK_BEGIN, NULL);
                        if (VBOX_FAILURE(rc))
                        {
                            Log(("VDIShrinkImage: seek(2) rc=%Vrc filename=\"%s\" uBlockWrite=%u cBlocks=%u cBlocksAllocated=%u cBlocksAllocated2=%u cbData=%llu\n",
                                 rc, pImage->szFilename, uBlockWrite, cBlocks, cBlocksAllocated, cBlocksAllocated2, cbData));
                            break;
                        }
                        rc = RTFileWrite(pImage->File, pvBuf, cbBlock, NULL);
                        if (VBOX_FAILURE(rc))
                        {
                            Log(("VDIShrinkImage: write rc=%Vrc filename=\"%s\" cbBlock=%u uBlockWrite=%u cBlocks=%u cBlocksAllocated=%u cBlocksAllocated2=%u cbData=%llu\n",
                                 rc, pImage->szFilename, cbBlock, uBlockWrite, cBlocks, cBlocksAllocated, cBlocksAllocated2, cbData));
                            break;
                        }
                    }
                    /* Fix the block pointer. */
                    pImage->paBlocks[paBlocks2[uBlock]] = uBlockWrite;
                    uBlockWrite++;
                }
                else
                {
                    Log(("VDIShrinkImage: found a zeroed block, uBlock=%u\n", uBlock));

                    /* Fix the block pointer. */
                    pImage->paBlocks[paBlocks2[uBlock]] = VDI_IMAGE_BLOCK_ZERO;
                }
            }
            else
                Log(("VDIShrinkImage: found an unused block, uBlock=%u\n", uBlock));

            if (pfnProgress)
            {
                pfnProgress(NULL /* WARNING! pVM=NULL  */,
                            (uBlock * 100) / cBlocksAllocated2,
                            pvUser);
                /* Shrink is unbreakable operation! */
            }
        }

        RTMemTmpFree(pvBuf);

        if (    VBOX_SUCCESS(rc)
            &&  uBlockWrite < cBlocksAllocated2)
        {
            /* File size must be shrinked. */
            Log(("VDIShrinkImage: shrinking file size from %llu to %llu bytes\n",
                 cbFile,
                 pImage->offStartData + ((uint64_t)uBlockWrite << pImage->uShiftIndex2Offset)));
            rc = RTFileSetSize(pImage->File,
                               pImage->offStartData + ((uint64_t)uBlockWrite << pImage->uShiftIndex2Offset));
            if (VBOX_FAILURE(rc))
                Log(("VDIShrinkImage: RTFileSetSize rc=%Vrc\n", rc));
        }
        cBlocksAllocated2 = uBlockWrite;
    }
    else
    {
        Log(("VDIShrinkImage: failed to allocate working buffer (%u bytes)\n", cbBlock));
        rc = VERR_NO_MEMORY;
    }

    /* Save header and blocks array. */
    if (VBOX_SUCCESS(rc))
    {
        setImageBlocksAllocated(&pImage->Header, cBlocksAllocated2);
        rc = vdiUpdateBlocks(pImage);
        if (pfnProgress)
            pfnProgress(NULL /* WARNING! pVM=NULL */, 100, pvUser);
    }

    /* Do debug dump. */
    vdiDumpImage(pImage);

    /* Clean up. */
    RTMemTmpFree(paBlocks2);
    vdiCloseImage(pImage);

    LogFlow(("VDIShrinkImage: returns %Vrc for filename=\"%s\"\n", rc, pszFilename));
    return rc;
}

/**
 * Converts image file from older VDI formats to current one.
 *
 * @returns VBox status code.
 * @param   pszFilename     Name of the image file to convert.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 * @remark  Only used by vditool
 */
IDER3DECL(int) VDIConvertImage(const char *pszFilename, PFNVMPROGRESS pfnProgress, void *pvUser)
{
    LogFlow(("VDIConvertImage:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, VDI_OPEN_FLAGS_NORMAL, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDIConvertImage: vdiOpenImage rc=%Vrc filename=\"%s\"\n", rc, pszFilename));
        return rc;
    }

    VDIHEADER Header = {0};
    int off;
    uint64_t cbFile;
    uint64_t cbData;

    if (pImage->fReadOnly)
    {
        Log(("VDIConvertImage: image \"%s\" is opened as read-only!\n", pszFilename));
        rc = VERR_VDI_IMAGE_READ_ONLY;
        goto l_conversion_failed;
    }

    if (pImage->PreHeader.u32Version != 0x00000002)
    {
        Log(("VDIConvertImage: unsupported version=%08X filename=\"%s\"\n",
             pImage->PreHeader.u32Version, pszFilename));
        rc = VERR_VDI_UNSUPPORTED_VERSION;
        goto l_conversion_failed;
    }

    /* Build new version header from old one. */
    vdiInitHeader(&Header,
                  getImageType(&pImage->Header),
                  VDI_IMAGE_FLAGS_DEFAULT,    /* Safety issue: Always use default flags. */
                  getImageComment(&pImage->Header),
                  getImageDiskSize(&pImage->Header),
                  getImageBlockSize(&pImage->Header),
                  0);
    setImageBlocksAllocated(&Header, getImageBlocksAllocated(&pImage->Header));
    *getImageGeometry(&Header) = *getImageGeometry(&pImage->Header);
    setImageTranslation(&Header, getImageTranslation(&pImage->Header));
    *getImageCreationUUID(&Header) = *getImageCreationUUID(&pImage->Header);
    *getImageModificationUUID(&Header) = *getImageModificationUUID(&pImage->Header);

    /* Calc data offset. */
    off = getImageDataOffset(&Header) - getImageDataOffset(&pImage->Header);
    if (off <= 0)
    {
        rc = VERR_VDI_INVALID_HEADER;
        goto l_conversion_failed;
    }

    rc = RTFileGetSize(pImage->File, &cbFile);
    if (VBOX_FAILURE(rc))
        goto l_conversion_failed;

    /* Check file size. */
    cbData = cbFile - getImageDataOffset(&pImage->Header);
    if (cbData != (uint64_t)getImageBlocksAllocated(&pImage->Header) << pImage->uShiftIndex2Offset)
    {
        AssertMsgFailed(("Invalid file size, broken image?\n"));
        rc = VERR_VDI_INVALID_HEADER;
        goto l_conversion_failed;
    }

    /* Expand file. */
    rc = RTFileSetSize(pImage->File, cbFile + off);
    if (VBOX_FAILURE(rc))
        goto l_conversion_failed;

    if (cbData > 0)
    {
        /* Calc current file position to move data from. */
        uint64_t offFile;
        if (cbData > VDIDISK_DEFAULT_BUFFER_SIZE)
            offFile = cbFile - VDIDISK_DEFAULT_BUFFER_SIZE;
        else
            offFile = getImageDataOffset(&pImage->Header);

        unsigned cMoves = (unsigned)(cbData / VDIDISK_DEFAULT_BUFFER_SIZE);
        unsigned c = 0;

        /* alloc tmp buffer */
        void *pvBuf = RTMemTmpAlloc(VDIDISK_DEFAULT_BUFFER_SIZE);
        if (pvBuf)
        {
            /* Move data. */
            for (;;)
            {
                unsigned cbToMove = (unsigned)RT_MIN(cbData, VDIDISK_DEFAULT_BUFFER_SIZE);

                /* Read. */
                rc = RTFileSeek(pImage->File, offFile, RTFILE_SEEK_BEGIN, NULL);
                if (VBOX_FAILURE(rc))
                    break;
                rc = RTFileRead(pImage->File, pvBuf, cbToMove, NULL);
                if (VBOX_FAILURE(rc))
                    break;

                /* Write. */
                rc = RTFileSeek(pImage->File, offFile + off, RTFILE_SEEK_BEGIN, NULL);
                if (VBOX_FAILURE(rc))
                    break;
                rc = RTFileWrite(pImage->File, pvBuf, cbToMove, NULL);
                if (VBOX_FAILURE(rc))
                    break;

                if (pfnProgress)
                {
                    c++;
                    pfnProgress(NULL /* WARNING! pVM=NULL  */,
                                (c * 100) / cMoves,
                                pvUser);
                    /* Note: conversion is non breakable operation, skipping rc here. */
                }

                cbData -= cbToMove;
                if (cbData == 0)
                    break;

                if (cbData > VDIDISK_DEFAULT_BUFFER_SIZE)
                    offFile -= VDIDISK_DEFAULT_BUFFER_SIZE;
                else
                    offFile = getImageDataOffset(&pImage->Header);
            }

            /* Fill the beginning of file with zeroes to wipe out old headers etc. */
            if (VBOX_SUCCESS(rc))
            {
                Assert(offFile + off <= VDIDISK_DEFAULT_BUFFER_SIZE);
                rc = RTFileSeek(pImage->File, 0, RTFILE_SEEK_BEGIN, NULL);
                if (VBOX_SUCCESS(rc))
                {
                    memset(pvBuf, 0, (unsigned)offFile + off);
                    rc = RTFileWrite(pImage->File, pvBuf, (unsigned)offFile + off, NULL);
                }
            }

            RTMemTmpFree(pvBuf);
        }
        else
            rc = VERR_NO_MEMORY;

        if (VBOX_FAILURE(rc))
            goto l_conversion_failed;
    }

    if (pfnProgress)
    {
        pfnProgress(NULL /* WARNING! pVM=NULL  */, 100, pvUser);
       /* Note: conversion is non breakable operation, skipping rc here. */
    }

    /* Data moved, now we need to save new pre header, header and blocks array. */

    vdiInitPreHeader(&pImage->PreHeader);
    pImage->Header = Header;

    /* Setup image parameters by header. */
    vdiSetupImageDesc(pImage);

    /* Write pre-header. */
    rc = RTFileSeek(pImage->File, 0, RTFILE_SEEK_BEGIN, NULL);
    if (VBOX_FAILURE(rc))
        goto l_conversion_failed;
    rc = RTFileWrite(pImage->File, &pImage->PreHeader, sizeof(pImage->PreHeader), NULL);
    if (VBOX_FAILURE(rc))
        goto l_conversion_failed;

    /* Write header and blocks array. */
    rc = vdiUpdateBlocks(pImage);

l_conversion_failed:
    vdiCloseImage(pImage);

    LogFlow(("VDIConvertImage: returns %Vrc for filename=\"%s\"\n", rc, pszFilename));
    return rc;
}

/**
 * Queries the image's UUID and parent UUIDs.
 *
 * @returns VBox status code.
 * @param   pszFilename             Name of the image file to operate on.
 * @param   pUuid                   Where to store image UUID (can be NULL).
 * @param   pModificationUuid       Where to store modification UUID (can be NULL).
 * @param   pParentUuuid            Where to store parent UUID (can be NULL).
 * @param   pParentModificationUuid Where to store parent modification UUID (can be NULL).
 */
IDER3DECL(int) VDIGetImageUUIDs(const char *pszFilename,
                                PRTUUID pUuid, PRTUUID pModificationUuid,
                                PRTUUID pParentUuid, PRTUUID pParentModificationUuid)
{
    LogFlow(("VDIGetImageUUIDs:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Try open the specified image.
     */
    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, VDI_OPEN_FLAGS_NORMAL, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDIGetImageUUIDs: vdiOpenImage rc=%Vrc filename=\"%s\"\n", rc, pszFilename));
        return rc;
    }

    /*
     * Query data.
     */
    if (pUuid)
    {
        PCRTUUID pTmpUuid = getImageCreationUUID(&pImage->Header);
        if (pTmpUuid)
            *pUuid = *pTmpUuid;
        else
            RTUuidClear(pUuid);
    }
    if (pModificationUuid)
    {
        PCRTUUID pTmpUuid = getImageModificationUUID(&pImage->Header);
        if (pTmpUuid)
            *pModificationUuid = *pTmpUuid;
        else
            RTUuidClear(pModificationUuid);
    }
    if (pParentUuid)
    {
        PCRTUUID pTmpUuid = getImageParentUUID(&pImage->Header);
        if (pTmpUuid)
            *pParentUuid = *pTmpUuid;
        else
            RTUuidClear(pParentUuid);
    }
    if (pParentModificationUuid)
    {
        PCRTUUID pTmpUuid = getImageParentModificationUUID(&pImage->Header);
        if (pTmpUuid)
            *pParentModificationUuid = *pTmpUuid;
        else
            RTUuidClear(pParentModificationUuid);
    }

    /*
     * Close the image.
     */
    vdiCloseImage(pImage);

    return VINF_SUCCESS;
}

/**
 * Changes the image's UUID and parent UUIDs.
 *
 * @returns VBox status code.
 * @param   pszFilename             Name of the image file to operate on.
 * @param   pUuid                   Optional parameter, new UUID of the image.
 * @param   pModificationUuid       Optional parameter, new modification UUID of the image.
 * @param   pParentUuuid            Optional parameter, new parent UUID of the image.
 * @param   pParentModificationUuid Optional parameter, new parent modification UUID of the image.
 */
IDER3DECL(int) VDISetImageUUIDs(const char *pszFilename,
                                PCRTUUID pUuid, PCRTUUID pModificationUuid,
                                PCRTUUID pParentUuid, PCRTUUID pParentModificationUuid)
{
    LogFlow(("VDISetImageUUIDs:\n"));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p\n", pszFilename));
        return VERR_INVALID_PARAMETER;
    }

    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, VDI_OPEN_FLAGS_NORMAL, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDISetImageUUIDs: vdiOpenImage rc=%Vrc filename=\"%s\"\n", rc, pszFilename));
        return rc;
    }
    if (!pImage->fReadOnly)
    {
        /* we only support new images */
        if (GET_MAJOR_HEADER_VERSION(&pImage->Header) == 1)
        {
            if (pUuid)
                pImage->Header.u.v1.uuidCreate = *pUuid;

            if (pModificationUuid)
                pImage->Header.u.v1.uuidModify = *pModificationUuid;

            if (pParentUuid)
                pImage->Header.u.v1.uuidLinkage = *pParentUuid;

            if (pParentModificationUuid)
                pImage->Header.u.v1.uuidParentModify = *pParentModificationUuid;

            /* write out new header */
            rc = vdiUpdateHeader(pImage);
            AssertMsgRC(rc, ("vdiUpdateHeader() failed, filename=\"%s\", rc=%Vrc\n",
                             pImage->szFilename, rc));
        }
        else
        {
            Log(("VDISetImageUUIDs: Version is not supported!\n"));
            rc = VERR_VDI_UNSUPPORTED_VERSION;
        }
    }
    else
    {
        Log(("VDISetImageUUIDs: image \"%s\" is opened as read-only!\n", pszFilename));
        rc = VERR_VDI_IMAGE_READ_ONLY;
    }

    vdiCloseImage(pImage);
    return rc;
}

/**
 * Merges two images having a parent/child relationship (both directions).
 *
 * @returns VBox status code.
 * @param   pszFilenameFrom         Name of the image file to merge from.
 * @param   pszFilenameTo           Name of the image file to merge into.
 * @param   pfnProgress     Progress callback. Optional. NULL if not to be used.
 * @param   pvUser          User argument for the progress callback.
 */
IDER3DECL(int) VDIMergeImage(const char *pszFilenameFrom, const char *pszFilenameTo,
                             PFNVMPROGRESS pfnProgress, void *pvUser)
{
    LogFlow(("VDIMergeImage:\n"));

    /* Check arguments. */
    if (    !pszFilenameFrom
        ||  *pszFilenameFrom == '\0'
        ||  !pszFilenameTo
        ||  *pszFilenameTo   == '\0')
    {
        AssertMsgFailed(("Invalid arguments: pszFilenameFrom=%p, pszFilenameTo=%p\n", pszFilenameFrom, pszFilenameTo));
        return VERR_INVALID_PARAMETER;
    }

    PVDIIMAGEDESC pImageFrom;
    int rc = vdiOpenImage(&pImageFrom, pszFilenameFrom, VDI_OPEN_FLAGS_READONLY, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDIMergeImage: vdiOpenImage rc=%Vrc pstFilenameFrom=\"%s\"\n", rc, pszFilenameFrom));
        return rc;
    }

    PVDIIMAGEDESC pImageTo;
    rc = vdiOpenImage(&pImageTo, pszFilenameTo, VDI_OPEN_FLAGS_NORMAL, NULL);
    if (VBOX_FAILURE(rc))
    {
        Log(("VDIMergeImage: vdiOpenImage rc=%Vrc pszFilenameTo=\"%s\"\n", rc, pszFilenameTo));
        vdiCloseImage(pImageFrom);
        return rc;
    }
    if (pImageTo->fReadOnly)
    {
        Log(("VDIMergeImage: image \"%s\" is opened as read-only!\n", pszFilenameTo));
        vdiCloseImage(pImageFrom);
        vdiCloseImage(pImageTo);
        return VERR_VDI_IMAGE_READ_ONLY;
    }

    /*
     *  when merging, we should not update the modification uuid of the target
     *  image, because from the point of view of its children, it hasn't been
     *  logically changed after the successful merge.
     */
    vdiDisableLastModifiedUpdate(pImageTo);

    /*
     * Check in which direction we merge
     */

    bool bParentToChild = false;
    if (   getImageParentUUID(&pImageFrom->Header)
        && !RTUuidCompare(getImageParentUUID(&pImageFrom->Header),
                          getImageCreationUUID(&pImageTo->Header))
        && !RTUuidCompare(getImageParentModificationUUID(&pImageFrom->Header),
                          getImageModificationUUID(&pImageTo->Header)))
    {
        /* we merge from a child to its parent */
    }
    else
    if (   getImageParentUUID(&pImageTo->Header)
        && !RTUuidCompare(getImageParentUUID(&pImageTo->Header),
                          getImageCreationUUID(&pImageFrom->Header))
        && !RTUuidCompare(getImageParentModificationUUID(&pImageTo->Header),
                          getImageModificationUUID(&pImageFrom->Header)))
    {
        /* we merge from a parent to its child */
        bParentToChild = true;
    }
    else
    {
        /* the images are not related, we can't merge! */
        Log(("VDIMergeImages: images do not have a parent/child or child/parent relationship!\n"));
        rc = VERR_VDI_IMAGES_UUID_MISMATCH;
    }

    rc = vdiMergeImages(pImageFrom, pImageTo, bParentToChild, pfnProgress, pvUser);

    if (pfnProgress)
    {
        pfnProgress(NULL /* WARNING! pVM=NULL */, 100, pvUser);
       /* Note: commiting is non breakable operation, skipping rc here. */
    }

    /* cleanup */
    vdiCloseImage(pImageFrom);
    vdiCloseImage(pImageTo);

    Log(("VDIMergeImage: done, returning with rc = %Vrc\n", rc));
    return rc;
}


/**
 * internal: initialize VDIDISK structure.
 */
static void vdiInitVDIDisk(PVDIDISK pDisk)
{
    Assert(pDisk);
    pDisk->u32Signature = VDIDISK_SIGNATURE;
    pDisk->cImages = 0;
    pDisk->pBase   = NULL;
    pDisk->pLast   = NULL;
    pDisk->cbBlock = VDI_IMAGE_DEFAULT_BLOCK_SIZE;
    pDisk->cbBuf   = VDIDISK_DEFAULT_BUFFER_SIZE;
}

/**
 * internal: add image structure to the end of images list.
 */
static void vdiAddImageToList(PVDIDISK pDisk, PVDIIMAGEDESC pImage)
{
    pImage->pPrev = NULL;
    pImage->pNext = NULL;

    if (pDisk->pBase)
    {
        Assert(pDisk->cImages > 0);
        pImage->pPrev = pDisk->pLast;
        pDisk->pLast->pNext = pImage;
        pDisk->pLast = pImage;
    }
    else
    {
        Assert(pDisk->cImages == 0);
        pDisk->pBase = pImage;
        pDisk->pLast = pImage;
    }

    pDisk->cImages++;
}

/**
 * internal: remove image structure from the images list.
 */
static void vdiRemoveImageFromList(PVDIDISK pDisk, PVDIIMAGEDESC pImage)
{
    Assert(pDisk->cImages > 0);

    if (pImage->pPrev)
        pImage->pPrev->pNext = pImage->pNext;
    else
        pDisk->pBase = pImage->pNext;

    if (pImage->pNext)
        pImage->pNext->pPrev = pImage->pPrev;
    else
        pDisk->pLast = pImage->pPrev;

    pImage->pPrev = NULL;
    pImage->pNext = NULL;

    pDisk->cImages--;
}

/**
 * Allocates and initializes VDI HDD container.
 *
 * @returns Pointer to newly created HDD container with no one opened image file.
 * @returns NULL on failure, typically out of memory.
 */
IDER3DECL(PVDIDISK) VDIDiskCreate(void)
{
    PVDIDISK pDisk = (PVDIDISK)RTMemAllocZ(sizeof(VDIDISK));
    if (pDisk)
        vdiInitVDIDisk(pDisk);
    LogFlow(("VDIDiskCreate: returns pDisk=%X\n", pDisk));
    return pDisk;
}

/**
 * Destroys VDI HDD container. If container has opened image files they will be closed.
 *
 * @param   pDisk           Pointer to VDI HDD container.
 */
IDER3DECL(void) VDIDiskDestroy(PVDIDISK pDisk)
{
    LogFlow(("VDIDiskDestroy: pDisk=%X\n", pDisk));
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk)
    {
        VDIDiskCloseAllImages(pDisk);
        RTMemFree(pDisk);
    }
}

/**
 * Get working buffer size of VDI HDD container.
 *
 * @returns Working buffer size in bytes.
 */
IDER3DECL(unsigned) VDIDiskGetBufferSize(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    LogFlow(("VDIDiskGetBufferSize: returns %u\n", pDisk->cbBuf));
    return pDisk->cbBuf;
}

/**
 * Get read/write mode of VDI HDD.
 *
 * @returns Disk ReadOnly status.
 * @returns true if no one VDI image is opened in HDD container.
 */
IDER3DECL(bool) VDIDiskIsReadOnly(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk->pLast)
    {
        LogFlow(("VDIDiskIsReadOnly: returns %u\n", pDisk->pLast->fReadOnly));
        return pDisk->pLast->fReadOnly;
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return true;
}

/**
 * Get disk size of VDI HDD container.
 *
 * @returns Virtual disk size in bytes.
 * @returns 0 if no one VDI image is opened in HDD container.
 */
IDER3DECL(uint64_t) VDIDiskGetSize(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk->pBase)
    {
        LogFlow(("VDIDiskGetSize: returns %llu\n", getImageDiskSize(&pDisk->pBase->Header)));
        return getImageDiskSize(&pDisk->pBase->Header);
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return 0;
}

/**
 * Get block size of VDI HDD container.
 *
 * @returns VDI image block size in bytes.
 * @returns 0 if no one VDI image is opened in HDD container.
 */
IDER3DECL(unsigned) VDIDiskGetBlockSize(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk->pBase)
    {
        LogFlow(("VDIDiskGetBlockSize: returns %u\n", getImageBlockSize(&pDisk->pBase->Header)));
        return getImageBlockSize(&pDisk->pBase->Header);
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return 0;
}

/**
 * Get virtual disk geometry stored in image file.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_NOT_OPENED if no one VDI image is opened in HDD container.
 * @returns VERR_VDI_GEOMETRY_NOT_SET if no geometry has been setted.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   pcCylinders     Where to store the number of cylinders. NULL is ok.
 * @param   pcHeads         Where to store the number of heads. NULL is ok.
 * @param   pcSectors       Where to store the number of sectors. NULL is ok.
 */
IDER3DECL(int) VDIDiskGetGeometry(PVDIDISK pDisk, unsigned *pcCylinders, unsigned *pcHeads, unsigned *pcSectors)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk->pBase)
    {
        int rc = VINF_SUCCESS;
        PVDIDISKGEOMETRY pGeometry = getImageGeometry(&pDisk->pBase->Header);
        LogFlow(("VDIDiskGetGeometry: C/H/S = %u/%u/%u\n",
                 pGeometry->cCylinders, pGeometry->cHeads, pGeometry->cSectors));
        if (    pGeometry->cCylinders > 0
            &&  pGeometry->cHeads > 0
            &&  pGeometry->cSectors > 0)
        {
            if (pcCylinders)
                *pcCylinders = pGeometry->cCylinders;
            if (pcHeads)
                *pcHeads = pGeometry->cHeads;
            if (pcSectors)
                *pcSectors = pGeometry->cSectors;
        }
        else
            rc = VERR_VDI_GEOMETRY_NOT_SET;

        LogFlow(("VDIDiskGetGeometry: returns %Vrc\n", rc));
        return rc;
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return VERR_VDI_NOT_OPENED;
}

/**
 * Store virtual disk geometry into base image file of HDD container.
 *
 * Note that in case of unrecoverable error all images of HDD container will be closed.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_NOT_OPENED if no one VDI image is opened in HDD container.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   cCylinders      Number of cylinders.
 * @param   cHeads          Number of heads.
 * @param   cSectors        Number of sectors.
 */
IDER3DECL(int) VDIDiskSetGeometry(PVDIDISK pDisk, unsigned cCylinders, unsigned cHeads, unsigned cSectors)
{
    LogFlow(("VDIDiskSetGeometry: C/H/S = %u/%u/%u\n", cCylinders, cHeads, cSectors));
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk->pBase)
    {
        PVDIDISKGEOMETRY pGeometry = getImageGeometry(&pDisk->pBase->Header);
        pGeometry->cCylinders = cCylinders;
        pGeometry->cHeads = cHeads;
        pGeometry->cSectors = cSectors;
        pGeometry->cbSector = VDI_GEOMETRY_SECTOR_SIZE;

        /* Update header information in base image file. */
        int rc = vdiUpdateReadOnlyHeader(pDisk->pBase);
        LogFlow(("VDIDiskSetGeometry: returns %Vrc\n", rc));
        return rc;
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return VERR_VDI_NOT_OPENED;
}

/**
 * Get virtual disk translation mode stored in image file.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_NOT_OPENED if no one VDI image is opened in HDD container.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   penmTranslation Where to store the translation mode (see pdm.h).
 */
IDER3DECL(int) VDIDiskGetTranslation(PVDIDISK pDisk, PPDMBIOSTRANSLATION penmTranslation)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(penmTranslation);

    if (pDisk->pBase)
    {
        *penmTranslation = getImageTranslation(&pDisk->pBase->Header);
        LogFlow(("VDIDiskGetTranslation: translation=%d\n", *penmTranslation));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return VERR_VDI_NOT_OPENED;
}

/**
 * Store virtual disk translation mode into base image file of HDD container.
 *
 * Note that in case of unrecoverable error all images of HDD container will be closed.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_NOT_OPENED if no one VDI image is opened in HDD container.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   enmTranslation  Translation mode (see pdm.h).
 */
IDER3DECL(int) VDIDiskSetTranslation(PVDIDISK pDisk, PDMBIOSTRANSLATION enmTranslation)
{
    LogFlow(("VDIDiskSetTranslation: enmTranslation=%d\n", enmTranslation));
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    if (pDisk->pBase)
    {
        setImageTranslation(&pDisk->pBase->Header, enmTranslation);

        /* Update header information in base image file. */
        int rc = vdiUpdateReadOnlyHeader(pDisk->pBase);
        LogFlow(("VDIDiskSetTranslation: returns %Vrc\n", rc));
        return rc;
    }

    AssertMsgFailed(("No one disk image is opened!\n"));
    return VERR_VDI_NOT_OPENED;
}

/**
 * Get number of opened images in HDD container.
 *
 * @returns Number of opened images for HDD container. 0 if no images is opened.
 * @param   pDisk           Pointer to VDI HDD container.
 */
IDER3DECL(int) VDIDiskGetImagesCount(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    LogFlow(("VDIDiskGetImagesCount: returns %d\n", pDisk->cImages));
    return pDisk->cImages;
}

static PVDIIMAGEDESC vdiGetImageByNumber(PVDIDISK pDisk, int nImage)
{
    PVDIIMAGEDESC pImage = pDisk->pBase;
    while (pImage && nImage)
    {
        pImage = pImage->pNext;
        nImage--;
    }
    return pImage;
}

/**
 * Get version of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   puVersion       Where to store the image version.
 */
IDER3DECL(int) VDIDiskGetImageVersion(PVDIDISK pDisk, int nImage, unsigned *puVersion)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(puVersion);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        *puVersion = pImage->PreHeader.u32Version;
        LogFlow(("VDIDiskGetImageVersion: returns %08X\n", pImage->PreHeader.u32Version));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get filename of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @returns VERR_BUFFER_OVERFLOW if pszFilename buffer too small to hold filename.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pszFilename     Where to store the image file name.
 * @param   cbFilename      Size of buffer pszFilename points to.
 */
IDER3DECL(int) VDIDiskGetImageFilename(PVDIDISK pDisk, int nImage, char *pszFilename, unsigned cbFilename)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pszFilename);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        unsigned cb = strlen(pImage->szFilename);
        if (cb < cbFilename)
        {
            /* memcpy is much better than strncpy. */
            memcpy(pszFilename, pImage->szFilename, cb + 1);
            LogFlow(("VDIDiskGetImageFilename: returns VINF_SUCCESS, filename=\"%s\" nImage=%d\n",
                     pszFilename, nImage));
            return VINF_SUCCESS;
        }
        else
        {
            AssertMsgFailed(("Out of buffer space, cbFilename=%d needed=%d\n", cbFilename, cb + 1));
            return VERR_BUFFER_OVERFLOW;
        }
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get the comment line of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @returns VERR_BUFFER_OVERFLOW if pszComment buffer too small to hold comment text.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pszComment      Where to store the comment string of image. NULL is ok.
 * @param   cbComment       The size of pszComment buffer. 0 is ok.
 */
IDER3DECL(int) VDIDiskGetImageComment(PVDIDISK pDisk, int nImage, char *pszComment, unsigned cbComment)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pszComment);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        char *pszTmp = getImageComment(&pImage->Header);
        unsigned cb = strlen(pszTmp);
        if (cb < cbComment)
        {
            /* memcpy is much better than strncpy. */
            memcpy(pszComment, pszTmp, cb + 1);
            LogFlow(("VDIDiskGetImageComment: returns VINF_SUCCESS, comment=\"%s\" nImage=%d\n",
                     pszTmp, nImage));
            return VINF_SUCCESS;
        }
        else
        {
            AssertMsgFailed(("Out of buffer space, cbComment=%d needed=%d\n", cbComment, cb + 1));
            return VERR_BUFFER_OVERFLOW;
        }
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get type of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   penmType        Where to store the image type.
 */
IDER3DECL(int) VDIDiskGetImageType(PVDIDISK pDisk, int nImage, PVDIIMAGETYPE penmType)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(penmType);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        *penmType = getImageType(&pImage->Header);
        LogFlow(("VDIDiskGetImageType: returns VINF_SUCCESS, type=%X nImage=%d\n",
                 *penmType, nImage));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get flags of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pfFlags         Where to store the image flags.
 */
IDER3DECL(int) VDIDiskGetImageFlags(PVDIDISK pDisk, int nImage, unsigned *pfFlags)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pfFlags);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        *pfFlags = getImageFlags(&pImage->Header);
        LogFlow(("VDIDiskGetImageFlags: returns VINF_SUCCESS, flags=%08X nImage=%d\n",
                 *pfFlags, nImage));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get Uuid of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           Where to store the image creation uuid.
 */
IDER3DECL(int) VDIDiskGetImageUuid(PVDIDISK pDisk, int nImage, PRTUUID pUuid)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pUuid);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        *pUuid = *getImageCreationUUID(&pImage->Header);
        LogFlow(("VDIDiskGetImageUuid: returns VINF_SUCCESS, uuid={%Vuuid} nImage=%d\n",
                 pUuid, nImage));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get last modification Uuid of opened image of HDD container.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of container.
 * @param   pUuid           Where to store the image modification uuid.
 */
IDER3DECL(int) VDIDiskGetImageModificationUuid(PVDIDISK pDisk, int nImage, PRTUUID pUuid)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pUuid);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    Assert(pImage);

    if (pImage)
    {
        *pUuid = *getImageModificationUUID(&pImage->Header);
        LogFlow(("VDIDiskGetImageModificationUuid: returns VINF_SUCCESS, uuid={%Vuuid} nImage=%d\n",
                 pUuid, nImage));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * Get Uuid of opened image's parent image.
 *
 * @returns VBox status code.
 * @returns VERR_VDI_IMAGE_NOT_FOUND if image with specified number was not opened.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   nImage          Image number, counts from 0. 0 is always base image of the container.
 * @param   pUuid           Where to store the image creation uuid.
 */
IDER3DECL(int) VDIDiskGetParentImageUuid(PVDIDISK pDisk, int nImage, PRTUUID pUuid)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pUuid);

    PVDIIMAGEDESC pImage = vdiGetImageByNumber(pDisk, nImage);
    if (pImage)
    {
        *pUuid = *getImageParentUUID(&pImage->Header);
        LogFlow(("VDIDiskGetParentImageUuid: returns VINF_SUCCESS, *pUuid={%Vuuid} nImage=%d\n",
                 pUuid, nImage));
        return VINF_SUCCESS;
    }

    AssertMsgFailed(("Image %d was not found (cImages=%d)\n", nImage, pDisk->cImages));
    return VERR_VDI_IMAGE_NOT_FOUND;
}

/**
 * internal: Relock image as read/write or read-only.
 */
static int vdiChangeImageMode(PVDIIMAGEDESC pImage, bool fReadOnly)
{
    Assert(pImage);

    if (    !fReadOnly
        &&  pImage->fOpen & VDI_OPEN_FLAGS_READONLY)
    {
        /* Can't switch read-only opened image to read-write mode. */
        Log(("vdiChangeImageMode: can't switch r/o image to r/w mode, filename=\"%s\" fOpen=%X\n",
             pImage->szFilename, pImage->fOpen));
        return VERR_VDI_IMAGE_READ_ONLY;
    }

    /* Flush last image changes if was r/w mode. */
    vdiFlushImage(pImage);

    /* Change image locking. */
    uint64_t cbLock = pImage->offStartData
                    + ((uint64_t)getImageBlocks(&pImage->Header) << pImage->uShiftIndex2Offset);
    int rc = RTFileChangeLock(pImage->File,
                              (fReadOnly) ?
                                  RTFILE_LOCK_READ | RTFILE_LOCK_IMMEDIATELY :
                                  RTFILE_LOCK_WRITE | RTFILE_LOCK_IMMEDIATELY,
                              0,
                              cbLock);
    if (VBOX_SUCCESS(rc))
    {
        pImage->fReadOnly = fReadOnly;
        Log(("vdiChangeImageMode: Image \"%s\" mode changed to %s\n",
             pImage->szFilename, (pImage->fReadOnly) ? "read-only" : "read/write"));
        return VINF_SUCCESS;
    }

    /* Check for the most bad error in the world. Damn! It must never happens in real life! */
    if (rc == VERR_FILE_LOCK_LOST)
    {
        /* And what we can do now?! */
        AssertMsgFailed(("Image lock has been lost for file=\"%s\"", pImage->szFilename));
        Log(("vdiChangeImageMode: image lock has been lost for file=\"%s\", blocking on r/o lock wait",
             pImage->szFilename));

        /* Try to obtain read lock in blocking mode. Maybe it's a very bad method. */
        rc = RTFileLock(pImage->File, RTFILE_LOCK_READ | RTFILE_LOCK_WAIT, 0, cbLock);
        AssertReleaseRC(rc);

        pImage->fReadOnly = false;
        if (pImage->fReadOnly != fReadOnly)
            rc = VERR_FILE_LOCK_VIOLATION;
    }

    Log(("vdiChangeImageMode: Image \"%s\" mode change failed with rc=%Vrc, mode is %s\n",
         pImage->szFilename, rc, (pImage->fReadOnly) ? "read-only" : "read/write"));

    return rc;
}

/**
 * internal: try to save header in image file even if image is in read-only mode.
 */
static int vdiUpdateReadOnlyHeader(PVDIIMAGEDESC pImage)
{
    int rc = VINF_SUCCESS;

    if (pImage->fReadOnly)
    {
        rc = vdiChangeImageMode(pImage, false);
        if (VBOX_SUCCESS(rc))
        {
            vdiFlushImage(pImage);
            rc = vdiChangeImageMode(pImage, true);
            AssertReleaseRC(rc);
        }
    }
    else
        vdiFlushImage(pImage);

    return rc;
}

/**
 * Opens an image file.
 *
 * The first opened image file in a HDD container must have a base image type,
 * others (next opened images) must be a differencing or undo images.
 * Linkage is checked for differencing image to be in consistence with the previously opened image.
 * When a next differencing image is opened and the last image was opened in read/write access
 * mode, then the last image is reopened in read-only with deny write sharing mode. This allows
 * other processes to use images in read-only mode too.
 *
 * Note that the image can be opened in read-only mode if a read/write open is not possible.
 * Use VDIDiskIsReadOnly to check open mode.
 *
 * @returns VBox status code.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   pszFilename     Name of the image file to open.
 * @param   fOpen           Image file open mode, see VDI_OPEN_FLAGS_* constants.
 */
IDER3DECL(int) VDIDiskOpenImage(PVDIDISK pDisk, const char *pszFilename, unsigned fOpen)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    /* Check arguments. */
    if (    !pszFilename
        ||  *pszFilename == '\0'
        ||  (fOpen & ~VDI_OPEN_FLAGS_MASK))
    {
        AssertMsgFailed(("Invalid arguments: pszFilename=%p fOpen=%x\n", pszFilename, fOpen));
        return VERR_INVALID_PARAMETER;
    }
    LogFlow(("VDIDiskOpenImage: pszFilename=\"%s\" fOpen=%X\n", pszFilename, fOpen));

    PVDIIMAGEDESC pImage;
    int rc = vdiOpenImage(&pImage, pszFilename, fOpen, pDisk->pLast);
    if (VBOX_SUCCESS(rc))
    {
        if (pDisk->pLast)
        {
            /* Opening differencing image. */
            if (!pDisk->pLast->fReadOnly)
            {
                /*
                 * Previous image is opened in read/write mode -> switch it into read-only.
                 */
                rc = vdiChangeImageMode(pDisk->pLast, true);
            }
        }
        else
        {
            /* Opening base image, check its type. */
            if (    getImageType(&pImage->Header) != VDI_IMAGE_TYPE_NORMAL
                &&  getImageType(&pImage->Header) != VDI_IMAGE_TYPE_FIXED)
            {
                rc = VERR_VDI_INVALID_TYPE;
            }
        }

        if (VBOX_SUCCESS(rc))
            vdiAddImageToList(pDisk, pImage);
        else
            vdiCloseImage(pImage);
    }

    LogFlow(("VDIDiskOpenImage: returns %Vrc\n", rc));
    return rc;
}

/**
 * Closes the last opened image file in the HDD container. Leaves all changes inside it.
 * If previous image file was opened in read-only mode (that is normal) and closing image
 * was opened in read-write mode (the whole disk was in read-write mode) - the previous image
 * will be reopened in read/write mode.
 *
 * @param   pDisk           Pointer to VDI HDD container.
 */
IDER3DECL(void) VDIDiskCloseImage(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    PVDIIMAGEDESC pImage = pDisk->pLast;
    if (pImage)
    {
        LogFlow(("VDIDiskCloseImage: closing image \"%s\"\n", pImage->szFilename));

        bool fWasReadOnly = pImage->fReadOnly;
        vdiRemoveImageFromList(pDisk, pImage);
        vdiCloseImage(pImage);

        if (    !fWasReadOnly
            &&  pDisk->pLast
            &&  pDisk->pLast->fReadOnly
            &&  !(pDisk->pLast->fOpen & VDI_OPEN_FLAGS_READONLY))
        {
            /*
             * Closed image was opened in read/write mode, previous image was opened
             * in read-only mode, try to switch it into read/write.
             */
            int rc = vdiChangeImageMode(pDisk->pLast, false);
            NOREF(rc); /* gcc still hates unused variables... */
        }

        return;
    }
    AssertMsgFailed(("No images to close\n"));
}

/**
 * Closes all opened image files in HDD container.
 *
 * @param   pDisk           Pointer to VDI HDD container.
 */
IDER3DECL(void) VDIDiskCloseAllImages(PVDIDISK pDisk)
{
    LogFlow(("VDIDiskCloseAllImages:\n"));
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    PVDIIMAGEDESC pImage = pDisk->pLast;
    while (pImage)
    {
        PVDIIMAGEDESC pPrev = pImage->pPrev;
        vdiRemoveImageFromList(pDisk, pImage);
        vdiCloseImage(pImage);
        pImage = pPrev;
    }
    Assert(pDisk->pLast == NULL);
}

/**
 * Commits last opened differencing/undo image file of HDD container to previous one.
 * If previous image file was opened in read-only mode (that must be always so) it is reopened
 * as read/write to do commit operation.
 * After successfull commit the previous image file again reopened in read-only mode, last opened
 * image file is cleared of data and remains open and active in HDD container.
 * If you want to delete image after commit you must do it manually by VDIDiskCloseImage and
 * VDIDeleteImage calls.
 *
 * Note that in case of unrecoverable error all images of HDD container will be closed.
 *
 * @returns VBox status code.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 * @remark  Only used by tstVDI.
 */
IDER3DECL(int) VDIDiskCommitLastDiff(PVDIDISK pDisk, PFNVMPROGRESS pfnProgress, void *pvUser)
{
    LogFlow(("VDIDiskCommitLastDiff:\n"));
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    int rc = VINF_SUCCESS;
    PVDIIMAGEDESC pImage = pDisk->pLast;
    if (!pImage)
    {
        AssertMsgFailed(("No one disk image is opened!\n"));
        return VERR_VDI_NOT_OPENED;
    }

    if (pImage->fReadOnly)
    {
        AssertMsgFailed(("Image \"%s\" is read-only!\n", pImage->szFilename));
        return VERR_VDI_IMAGE_READ_ONLY;
    }

    if (!pImage->pPrev)
    {
        AssertMsgFailed(("No images to commit to!\n"));
        return VERR_VDI_NO_DIFF_IMAGES;
    }

    bool fWasReadOnly = pImage->pPrev->fReadOnly;
    if (fWasReadOnly)
    {
        /* Change previous image mode to r/w. */
        rc = vdiChangeImageMode(pImage->pPrev, false);
        if (VBOX_FAILURE(rc))
        {
            Log(("VDIDiskCommitLastDiff: can't switch previous image into r/w mode, rc=%Vrc\n", rc));
            return rc;
        }
    }

    rc = vdiCommitToImage(pDisk, pImage->pPrev, pfnProgress, pvUser);
    if (VBOX_SUCCESS(rc) && fWasReadOnly)
    {
        /* Change previous image mode back to r/o. */
        rc = vdiChangeImageMode(pImage->pPrev, true);
    }

    if (VBOX_FAILURE(rc))
    {
        /* Failed! Close all images, can't work with VHDD at all. */
        VDIDiskCloseAllImages(pDisk);
        AssertMsgFailed(("Fatal: commit failed, rc=%Vrc\n", rc));
    }

    return rc;
}

/**
 * Creates and opens a new differencing image file in HDD container.
 * See comments for VDIDiskOpenImage function about differencing images.
 *
 * @returns VBox status code.
 * @param   pDisk           Pointer to VDI HDD container.
 * @param   pszFilename     Name of the image file to create and open.
 * @param   pszComment      Pointer to image comment. NULL is ok.
 * @param   pfnProgress     Progress callback. Optional.
 * @param   pvUser          User argument for the progress callback.
 * @remark  Only used by tstVDI.
 */
IDER3DECL(int) VDIDiskCreateOpenDifferenceImage(PVDIDISK pDisk, const char *pszFilename,
                                                const char *pszComment, PFNVMPROGRESS pfnProgress,
                                                void *pvUser)
{
    LogFlow(("VDIDiskCreateOpenDifferenceImage:\n"));
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));
    Assert(pszFilename);

    if (!pDisk->pLast)
    {
        AssertMsgFailed(("No one disk image is opened!\n"));
        return VERR_VDI_NOT_OPENED;
    }

    /* Flush last parent image changes if possible. */
    vdiFlushImage(pDisk->pLast);

    int rc = vdiCreateImage(pszFilename,
                            VDI_IMAGE_TYPE_DIFF,
                            VDI_IMAGE_FLAGS_DEFAULT,
                            getImageDiskSize(&pDisk->pLast->Header),
                            pszComment,
                            pDisk->pLast,
                            pfnProgress, pvUser);
    if (VBOX_SUCCESS(rc))
    {
        rc = VDIDiskOpenImage(pDisk, pszFilename, VDI_OPEN_FLAGS_NORMAL);
        if (VBOX_FAILURE(rc))
            VDIDeleteImage(pszFilename);
    }
    LogFlow(("VDIDiskCreateOpenDifferenceImage: returns %Vrc, filename=\"%s\"\n", rc, pszFilename));
    return rc;
}

/**
 * internal: debug image dump.
 *
 * @remark  Only used by tstVDI.
 */
static void vdiDumpImage(PVDIIMAGEDESC pImage)
{
    RTLogPrintf("Dumping VDI image \"%s\" mode=%s fOpen=%X File=%08X\n",
                pImage->szFilename,
                (pImage->fReadOnly) ? "r/o" : "r/w",
                pImage->fOpen,
                pImage->File);
    RTLogPrintf("Header: Version=%08X Type=%X Flags=%X Size=%llu\n",
                pImage->PreHeader.u32Version,
                getImageType(&pImage->Header),
                getImageFlags(&pImage->Header),
                getImageDiskSize(&pImage->Header));
    RTLogPrintf("Header: cbBlock=%u cbBlockExtra=%u cBlocks=%u cBlocksAllocated=%u\n",
                getImageBlockSize(&pImage->Header),
                getImageExtraBlockSize(&pImage->Header),
                getImageBlocks(&pImage->Header),
                getImageBlocksAllocated(&pImage->Header));
    RTLogPrintf("Header: offBlocks=%u offData=%u\n",
                getImageBlocksOffset(&pImage->Header),
                getImageDataOffset(&pImage->Header));
    PVDIDISKGEOMETRY pg = getImageGeometry(&pImage->Header);
    RTLogPrintf("Header: Geometry: C/H/S=%u/%u/%u cbSector=%u Mode=%u\n",
                pg->cCylinders, pg->cHeads, pg->cSectors, pg->cbSector,
                getImageTranslation(&pImage->Header));
    RTLogPrintf("Header: uuidCreation={%Vuuid}\n", getImageCreationUUID(&pImage->Header));
    RTLogPrintf("Header: uuidModification={%Vuuid}\n", getImageModificationUUID(&pImage->Header));
    RTLogPrintf("Header: uuidParent={%Vuuid}\n", getImageParentUUID(&pImage->Header));
    if (GET_MAJOR_HEADER_VERSION(&pImage->Header) >= 1)
        RTLogPrintf("Header: uuidParentModification={%Vuuid}\n", getImageParentModificationUUID(&pImage->Header));
    RTLogPrintf("Image:  fFlags=%08X offStartBlocks=%u offStartData=%u\n",
                pImage->fFlags, pImage->offStartBlocks, pImage->offStartData);
    RTLogPrintf("Image:  uBlockMask=%08X uShiftIndex2Offset=%u uShiftOffset2Index=%u offStartBlockData=%u\n",
                pImage->uBlockMask,
                pImage->uShiftIndex2Offset,
                pImage->uShiftOffset2Index,
                pImage->offStartBlockData);

    unsigned uBlock, cBlocksNotFree, cBadBlocks, cBlocks = getImageBlocks(&pImage->Header);
    for (uBlock=0, cBlocksNotFree=0, cBadBlocks=0; uBlock<cBlocks; uBlock++)
    {
        if (IS_VDI_IMAGE_BLOCK_ALLOCATED(pImage->paBlocks[uBlock]))
        {
            cBlocksNotFree++;
            if (pImage->paBlocks[uBlock] >= cBlocks)
                cBadBlocks++;
        }
    }
    if (cBlocksNotFree != getImageBlocksAllocated(&pImage->Header))
    {
        RTLogPrintf("!! WARNING: %u blocks actually allocated (cBlocksAllocated=%u) !!\n",
                cBlocksNotFree, getImageBlocksAllocated(&pImage->Header));
    }
    if (cBadBlocks)
    {
        RTLogPrintf("!! WARNING: %u bad blocks found !!\n",
                cBadBlocks);
    }
}

/**
 * Debug helper - dumps all opened images of HDD container into the log file.
 *
 * @param   pDisk           Pointer to VDI HDD container.
 * @remark  Only used by tstVDI and vditool
 */
IDER3DECL(void) VDIDiskDumpImages(PVDIDISK pDisk)
{
    /* sanity check */
    Assert(pDisk);
    AssertMsg(pDisk->u32Signature == VDIDISK_SIGNATURE, ("u32Signature=%08x\n", pDisk->u32Signature));

    RTLogPrintf("--- Dumping VDI Disk, Images=%u\n", pDisk->cImages);
    for (PVDIIMAGEDESC pImage = pDisk->pBase; pImage; pImage = pImage->pNext)
        vdiDumpImage(pImage);
}


/*******************************************************************************
*   PDM interface                                                              *
*******************************************************************************/

/**
 * Construct a VBox HDD media driver instance.
 *
 * @returns VBox status.
 * @param   pDrvIns     The driver instance data.
 *                      If the registration structure is needed, pDrvIns->pDrvReg points to it.
 * @param   pCfgHandle  Configuration node handle for the driver. Use this to obtain the configuration
 *                      of the driver instance. It's also found in pDrvIns->pCfgHandle, but like
 *                      iInstance it's expected to be used a bit in this function.
 */
static DECLCALLBACK(int) vdiConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfgHandle)
{
    LogFlow(("vdiConstruct:\n"));
    PVDIDISK pData = PDMINS2DATA(pDrvIns, PVDIDISK);
    char *pszName;      /**< The path of the disk image file. */
    bool fReadOnly;     /**< True if the media is readonly. */

    /*
     * Init the static parts.
     */
    pDrvIns->IBase.pfnQueryInterface    = vdiQueryInterface;
    pData->pDrvIns = pDrvIns;

    vdiInitVDIDisk(pData);

    /* IMedia */
    pData->IMedia.pfnRead               = vdiRead;
    pData->IMedia.pfnWrite              = vdiWrite;
    pData->IMedia.pfnFlush              = vdiFlush;
    pData->IMedia.pfnGetSize            = vdiGetSize;
    pData->IMedia.pfnGetUuid            = vdiGetUuid;
    pData->IMedia.pfnIsReadOnly         = vdiIsReadOnly;
    pData->IMedia.pfnBiosGetGeometry    = vdiBiosGetGeometry;
    pData->IMedia.pfnBiosSetGeometry    = vdiBiosSetGeometry;
    pData->IMedia.pfnBiosGetTranslation = vdiBiosGetTranslation;
    pData->IMedia.pfnBiosSetTranslation = vdiBiosSetTranslation;

#if 1 /** @todo someone review this! it's a shot in the dark from my side. */
    /*
     * Validate configuration and find the great to the level of umpteen grandparent.
     * The parents are found in the 'Parent' subtree, so it's sorta up side down
     * from the image dependency tree.
     */
    unsigned    iLevel = 0;
    PCFGMNODE   pCurNode = pCfgHandle;
    for (;;)
    {
        if (!CFGMR3AreValuesValid(pCfgHandle, "Path\0ReadOnly\0"))
            return VERR_PDM_DRVINS_UNKNOWN_CFG_VALUES;

        PCFGMNODE pParent = CFGMR3GetChild(pCurNode, "Parent");
        if (!pParent)
            break;
        pCurNode = pParent;
        iLevel++;
    }

    /*
     * Open the images.
     */
    int rc = VINF_SUCCESS;
    while (pCurNode && VBOX_SUCCESS(rc))
    {
        /*
         * Read the image configuration.
         */
        int rc = CFGMR3QueryStringAlloc(pCurNode, "Path", &pszName);
        if (VBOX_FAILURE(rc))
            return PDMDRV_SET_ERROR(pDrvIns, rc,
                                    N_("VHDD: Configuration error: Querying \"Path\" as string failed"));

        rc = CFGMR3QueryBool(pCfgHandle, "ReadOnly", &fReadOnly);
        if (rc == VERR_CFGM_VALUE_NOT_FOUND)
            fReadOnly = false;
        else if (VBOX_FAILURE(rc))
        {
            MMR3HeapFree(pszName);
            return PDMDRV_SET_ERROR(pDrvIns, rc,
                                    N_("VHDD: Configuration error: Querying \"ReadOnly\" as boolean failed"));
        }

        /*
         * Open the image.
         */
        rc = VDIDiskOpenImage(pData, pszName, fReadOnly ? VDI_OPEN_FLAGS_READONLY
                                                        : VDI_OPEN_FLAGS_NORMAL);
        if (VBOX_SUCCESS(rc))
            Log(("vdiConstruct: %d - Opened '%s' in %s mode\n",
                 iLevel, pszName, VDIDiskIsReadOnly(pData) ? "read-only" : "read-write"));
        else
            AssertMsgFailed(("Failed to open image '%s' rc=%Vrc\n", pszName, rc));
        MMR3HeapFree(pszName);

        /* next */
        iLevel--;
        pCurNode = CFGMR3GetParent(pCurNode);
    }

    /* On failure, vdiDestruct will be called, so no need to clean up here. */

#else /* old */
    /*
     * Validate and read top level configuration.
     */
    int rc = CFGMR3QueryStringAlloc(pCfgHandle, "Path", &pszName);
    if (VBOX_FAILURE(rc))
        return PDMDRV_SET_ERROR(pDrvIns, rc,
                                N_("VHDD: Configuration error: Querying \"Path\" as string failed"));

    rc = CFGMR3QueryBool(pCfgHandle, "ReadOnly", &fReadOnly);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        fReadOnly = false;
    else if (VBOX_FAILURE(rc))
    {
        MMR3HeapFree(pszName);
        return PDMDRV_SET_ERROR(pDrvIns, rc,
                                N_("VHDD: Configuration error: Querying \"ReadOnly\" as boolean failed"));
    }

    /*
     * Open the image.
     */
    rc = VDIDiskOpenImage(pData, pszName, fReadOnly ? VDI_OPEN_FLAGS_READONLY
                                                    : VDI_OPEN_FLAGS_NORMAL);
    if (VBOX_SUCCESS(rc))
        Log(("vdiConstruct: Opened '%s' in %s mode\n", pszName, VDIDiskIsReadOnly(pData) ? "read-only" : "read-write"));
    else
        AssertMsgFailed(("Failed to open image '%s' rc=%Vrc\n", pszName, rc));

    MMR3HeapFree(pszName);
    pszName = NULL;
#endif

    if (rc == VERR_ACCESS_DENIED)
        /* This should never happen here since this case is covered by Console::PowerUp */
        return PDMDrvHlpVMSetError(pDrvIns, rc, RT_SRC_POS,
                                   N_("Cannot open virtual disk image '%s' for %s access"),
                                   pszName, fReadOnly ? "readonly" : "read/write");

    return rc;
}

/**
 * Destruct a driver instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) vdiDestruct(PPDMDRVINS pDrvIns)
{
    LogFlow(("vdiDestruct:\n"));
    PVDIDISK pData = PDMINS2DATA(pDrvIns, PVDIDISK);
    VDIDiskCloseAllImages(pData);
}

/**
 * When the VM has been suspended we'll change the image mode to read-only
 * so that main and others can read the VDIs. This is important when
 * saving state and so forth.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) vdiSuspend(PPDMDRVINS pDrvIns)
{
    LogFlow(("vdiSuspend:\n"));
#if 1 // #ifdef DEBUG_dmik
    PVDIDISK pData = PDMINS2DATA(pDrvIns, PVDIDISK);
    if (!(pData->pLast->fOpen & VDI_OPEN_FLAGS_READONLY))
    {
        int rc = vdiChangeImageMode(pData->pLast, true);
        AssertRC(rc);
    }
#endif
}

/**
 * Before the VM resumes we'll have to undo the read-only mode change
 * done in vdiSuspend.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) vdiResume(PPDMDRVINS pDrvIns)
{
    LogFlow(("vdiSuspend:\n"));
#if 1 //#ifdef DEBUG_dmik
    PVDIDISK pData = PDMINS2DATA(pDrvIns, PVDIDISK);
    if (!(pData->pLast->fOpen & VDI_OPEN_FLAGS_READONLY))
    {
        int rc = vdiChangeImageMode(pData->pLast, false);
        AssertRC(rc);
    }
#endif
}


/** @copydoc PDMIMEDIA::pfnGetSize */
static DECLCALLBACK(uint64_t) vdiGetSize(PPDMIMEDIA pInterface)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    uint64_t cb = VDIDiskGetSize(pData);
    LogFlow(("vdiGetSize: returns %#llx (%llu)\n", cb, cb));
    return cb;
}

/**
 * Get stored media geometry - BIOS property.
 *
 * @see PDMIMEDIA::pfnBiosGetGeometry for details.
 */
static DECLCALLBACK(int) vdiBiosGetGeometry(PPDMIMEDIA pInterface, uint32_t *pcCylinders, uint32_t *pcHeads, uint32_t *pcSectors)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    int rc = VDIDiskGetGeometry(pData, pcCylinders, pcHeads, pcSectors);
    if (VBOX_SUCCESS(rc))
    {
        LogFlow(("vdiBiosGetGeometry: returns VINF_SUCCESS\n"));
        return VINF_SUCCESS;
    }
    Log(("vdiBiosGetGeometry: The Bios geometry data was not available.\n"));
    return VERR_PDM_GEOMETRY_NOT_SET;
}

/**
 * Set stored media geometry - BIOS property.
 *
 * @see PDMIMEDIA::pfnBiosSetGeometry for details.
 */
static DECLCALLBACK(int) vdiBiosSetGeometry(PPDMIMEDIA pInterface, uint32_t cCylinders, uint32_t cHeads, uint32_t cSectors)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    int rc = VDIDiskSetGeometry(pData, cCylinders, cHeads, cSectors);
    LogFlow(("vdiBiosSetGeometry: returns %Vrc (%d,%d,%d)\n", rc, cCylinders, cHeads, cSectors));
    return rc;
}

/**
 * Read bits.
 *
 * @see PDMIMEDIA::pfnRead for details.
 */
static DECLCALLBACK(int) vdiRead(PPDMIMEDIA pInterface, uint64_t off, void *pvBuf, size_t cbRead)
{
    LogFlow(("vdiRead: off=%#llx pvBuf=%p cbRead=%d\n", off, pvBuf, cbRead));
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    int rc = VDIDiskRead(pData, off, pvBuf, cbRead);
    if (VBOX_SUCCESS(rc))
        Log2(("vdiRead: off=%#llx pvBuf=%p cbRead=%d\n"
              "%.*Vhxd\n",
              off, pvBuf, cbRead, cbRead, pvBuf));
    LogFlow(("vdiRead: returns %Vrc\n", rc));
    return rc;
}

/**
 * Write bits.
 *
 * @see PDMIMEDIA::pfnWrite for details.
 */
static DECLCALLBACK(int) vdiWrite(PPDMIMEDIA pInterface, uint64_t off, const void *pvBuf, size_t cbWrite)
{
    LogFlow(("vdiWrite: off=%#llx pvBuf=%p cbWrite=%d\n", off, pvBuf, cbWrite));
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    Log2(("vdiWrite: off=%#llx pvBuf=%p cbWrite=%d\n"
          "%.*Vhxd\n",
          off, pvBuf, cbWrite, cbWrite, pvBuf));
    int rc = VDIDiskWrite(pData, off, pvBuf, cbWrite);
    LogFlow(("vdiWrite: returns %Vrc\n", rc));
    return rc;
}

/**
 * Flush bits to media.
 *
 * @see PDMIMEDIA::pfnFlush for details.
 */
static DECLCALLBACK(int) vdiFlush(PPDMIMEDIA pInterface)
{
    LogFlow(("vdiFlush:\n"));
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    vdiFlushImage(pData->pLast);
    int rc = VINF_SUCCESS;
    LogFlow(("vdiFlush: returns %Vrc\n", rc));
    return rc;
}

/** @copydoc PDMIMEDIA::pfnGetUuid */
static DECLCALLBACK(int) vdiGetUuid(PPDMIMEDIA pInterface, PRTUUID pUuid)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    int rc = VDIDiskGetImageUuid(pData, 0, pUuid);
    LogFlow(("vdiGetUuid: returns %Vrc ({%Vuuid})\n", rc, pUuid));
    return rc;
}

/** @copydoc PDMIMEDIA::pfnIsReadOnly */
static DECLCALLBACK(bool) vdiIsReadOnly(PPDMIMEDIA pInterface)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    LogFlow(("vdiIsReadOnly: returns %d\n", VDIDiskIsReadOnly(pData)));
    return VDIDiskIsReadOnly(pData);
}

/** @copydoc PDMIMEDIA::pfnBiosGetTranslation */
static DECLCALLBACK(int) vdiBiosGetTranslation(PPDMIMEDIA pInterface,
                                               PPDMBIOSTRANSLATION penmTranslation)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    int rc = VDIDiskGetTranslation(pData, penmTranslation);
    LogFlow(("vdiBiosGetTranslation: returns %Vrc (%d)\n", rc, *penmTranslation));
    return rc;
}

/** @copydoc PDMIMEDIA::pfnBiosSetTranslation */
static DECLCALLBACK(int) vdiBiosSetTranslation(PPDMIMEDIA pInterface,
                                               PDMBIOSTRANSLATION enmTranslation)
{
    PVDIDISK pData = PDMIMEDIA_2_VDIDISK(pInterface);
    int rc = VDIDiskSetTranslation(pData, enmTranslation);
    LogFlow(("vdiBiosSetTranslation: returns %Vrc (%d)\n", rc, enmTranslation));
    return rc;
}


/**
 * Queries an interface to the driver.
 *
 * @returns Pointer to interface.
 * @returns NULL if the interface was not supported by the driver.
 * @param   pInterface          Pointer to this interface structure.
 * @param   enmInterface        The requested interface identification.
 * @thread  Any thread.
 */
static DECLCALLBACK(void *) vdiQueryInterface(PPDMIBASE pInterface, PDMINTERFACE enmInterface)
{
    PPDMDRVINS pDrvIns = PDMIBASE_2_DRVINS(pInterface);
    PVDIDISK pData = PDMINS2DATA(pDrvIns, PVDIDISK);
    switch (enmInterface)
    {
        case PDMINTERFACE_BASE:
            return &pDrvIns->IBase;
        case PDMINTERFACE_MEDIA:
            return &pData->IMedia;
        default:
            return NULL;
    }
}


/**
 * VBox HDD driver registration record.
 */
const PDMDRVREG g_DrvVBoxHDD =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szDriverName */
    "VBoxHDD",
    /* pszDescription */
    "VBoxHDD media driver.",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass. */
    PDM_DRVREG_CLASS_MEDIA,
    /* cMaxInstances */
    ~0,
    /* cbInstance */
    sizeof(VDIDISK),
    /* pfnConstruct */
    vdiConstruct,
    /* pfnDestruct */
    vdiDestruct,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    vdiSuspend,
    /* pfnResume */
    vdiResume,
    /* pfnDetach */
    NULL
};

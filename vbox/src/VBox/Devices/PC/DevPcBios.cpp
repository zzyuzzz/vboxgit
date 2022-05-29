/* $Id: DevPcBios.cpp 8155 2008-04-18 15:16:47Z vboxsync $ */
/** @file
 * PC BIOS Device.
 */

/*
 * Copyright (C) 2006-2008 Sun Microsystems, Inc.
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
#define LOG_GROUP LOG_GROUP_DEV_PC_BIOS
#include <VBox/pdmdev.h>
#include <VBox/mm.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/alloc.h>
#include <iprt/file.h>
#include <iprt/string.h>
#include <iprt/uuid.h>
#include <VBox/err.h>
#include <VBox/param.h>

#include "Builtins.h"
#include "Builtins2.h"
#include "DevPcBios.h"


/*
 * The BIOS uses a CMOS to store configuration data.
 * It is currently used as followed:
 *
 *     Base memory:
 *          0x15
 *          0x16
 *     Extended memory:
 *          0x17
 *          0x18
 *          0x30
 *          0x31
 *     Amount of memory above 16M:
 *          0x34
 *          0x35
 *     Boot device (BOCHS bios specific):
 *          0x3d
 *          0x38
 *          0x3c
 *     PXE debug:
 *          0x3f
 *     Floppy drive type:
 *          0x10
 *     Equipment byte:
 *          0x14
 *     First HDD:
 *          0x19
 *          0x1e - 0x25
 *     Second HDD:
 *          0x1a
 *          0x26 - 0x2d
 *     Third HDD:
 *          0x67 - 0x6e
 *     Fourth HDD:
 *          0x70 - 0x77
 *     Extended:
 *          0x12
 *     First Sata HDD:
 *          0x40 - 0x47
 *     Second Sata HDD:
 *          0x48 - 0x4f
 *     Third Sata HDD:
 *          0x50 - 0x57
 *     Fourth Sata HDD:
 *          0x58 - 0x5f
 */

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/

/**
 * The boot device.
 */
typedef enum DEVPCBIOSBOOT
{
    DEVPCBIOSBOOT_NONE,
    DEVPCBIOSBOOT_FLOPPY,
    DEVPCBIOSBOOT_HD,
    DEVPCBIOSBOOT_DVD,
    DEVPCBIOSBOOT_LAN
} DEVPCBIOSBOOT;

/**
 * PC Bios instance data structure.
 */
typedef struct DEVPCBIOS
{
    /** Pointer back to the device instance. */
    PPDMDEVINS      pDevIns;

    /** Boot devices (ordered). */
    DEVPCBIOSBOOT   aenmBootDevice[4];
    /** Ram Size (in bytes). */
    uint64_t        cbRam;
    /** Bochs shutdown index. */
    uint32_t        iShutdown;
    /** Floppy device. */
    char           *pszFDDevice;
    /** Harddisk device. */
    char           *pszHDDevice;
    /** Sata harddisk device. */
    char           *pszSataDevice;
    /** LUN of the four harddisks which are emulated as IDE. */
    uint32_t        iSataHDLUN[4];
    /** Bios message buffer. */
    char            szMsg[256];
    /** Bios message buffer index. */
    uint32_t        iMsg;
    /** The system BIOS ROM data. */
    uint8_t        *pu8PcBios;
    /** The size of the system BIOS ROM. */
    uint64_t        cbPcBios;
    /** The name of the BIOS ROM file. */
    char           *pszPcBiosFile;
    /** The LAN boot ROM data. */
    uint8_t        *pu8LanBoot;
    /** The name of the LAN boot ROM file. */
    char           *pszLanBootFile;
    /** The DMI tables. */
    uint8_t        au8DMIPage[0x1000];
    /** The boot countdown (in seconds). */
    uint8_t        uBootDelay;
    /** I/O-APIC enabled? */
    uint8_t        u8IOAPIC;
    /** PXE debug logging enabled? */
    uint8_t        u8PXEDebug;
} DEVPCBIOS, *PDEVPCBIOS;

#pragma pack(1)

/** DMI header */
typedef struct DMIHDR
{
    uint8_t         u8Type;
    uint8_t         u8Length;
    uint16_t        u16Handle;
} *PDMIHDR;
AssertCompileSize(DMIHDR, 4);

/** DMI BIOS information */
typedef struct DMIBIOSINF
{
    DMIHDR          header;
    uint8_t         u8Vendor;
    uint8_t         u8Version;
    uint16_t        u16Start;
    uint8_t         u8Release;
    uint8_t         u8ROMSize;
    uint64_t        u64Characteristics;
    uint8_t         u8CharacteristicsByte1;
    uint8_t         u8CharacteristicsByte2;
} *PDMIBIOSINF;
AssertCompileSize(DMIBIOSINF, 0x14);

/** DMI system information */
typedef struct DMISYSTEMINF
{
    DMIHDR          header;
    uint8_t         u8Manufacturer;
    uint8_t         u8ProductName;
    uint8_t         u8Version;
    uint8_t         u8SerialNumber;
    uint8_t         au8Uuid[16];
    uint8_t         u8WakeupType;
    uint8_t         u8SKUNumber;
    uint8_t         u8Family;
} *PDMISYSTEMINF;
AssertCompileSize(DMISYSTEMINF, 0x1b);

/** MPS floating pointer structure */
typedef struct MPSFLOATPTR
{
    uint8_t         au8Signature[4];
    uint32_t        u32MPSAddr;
    uint8_t         u8Length;
    uint8_t         u8SpecRev;
    uint8_t         u8Checksum;
    uint8_t         au8Feature[5];
} *PMPSFLOATPTR;
AssertCompileSize(MPSFLOATPTR, 16);

/** MPS config table header */
typedef struct MPSCFGTBLHEADER
{
    uint8_t         au8Signature[4];
    uint16_t        u16Length;
    uint8_t         u8SpecRev;
    uint8_t         u8Checksum;
    uint8_t         au8OemId[8];
    uint8_t         au8ProductId[12];
    uint32_t        u32OemTablePtr;
    uint16_t        u16OemTableSize;
    uint16_t        u16EntryCount;
    uint32_t        u32AddrLocalApic;
    uint16_t        u16ExtTableLength;
    uint8_t         u8ExtTableChecksxum;
    uint8_t         u8Reserved;
} *PMPSCFGTBLHEADER;
AssertCompileSize(MPSCFGTBLHEADER, 0x2c);

/** MPS processor entry */
typedef struct MPSPROCENTRY
{
    uint8_t         u8EntryType;
    uint8_t         u8LocalApicId;
    uint8_t         u8LocalApicVersion;
    uint8_t         u8CPUFlags;
    uint32_t        u32CPUSignature;
    uint32_t        u32CPUFeatureFlags;
    uint32_t        u32Reserved[2];
} *PMPSPROCENTRY;
AssertCompileSize(MPSPROCENTRY, 20);

/** MPS bus entry */
typedef struct MPSBUSENTRY
{
    uint8_t         u8EntryType;
    uint8_t         u8BusId;
    uint8_t         au8BusTypeStr[6];
} *PMPSBUSENTRY;
AssertCompileSize(MPSBUSENTRY, 8);

/** MPS I/O-APIC entry */
typedef struct MPSIOAPICENTRY
{
    uint8_t         u8EntryType;
    uint8_t         u8Id;
    uint8_t         u8Version;
    uint8_t         u8Flags;
    uint32_t        u32Addr;
} *PMPSIOAPICENTRY;
AssertCompileSize(MPSIOAPICENTRY, 8);

/** MPS I/O-Interrupt entry */
typedef struct MPSIOINTERRUPTENTRY
{
    uint8_t         u8EntryType;
    uint8_t         u8Type;
    uint16_t        u16Flags;
    uint8_t         u8SrcBusId;
    uint8_t         u8SrcBusIrq;
    uint8_t         u8DstIOAPICId;
    uint8_t         u8DstIOAPICInt;
} *PMPSIOIRQENTRY;
AssertCompileSize(MPSIOINTERRUPTENTRY, 8);

#pragma pack()

/* Attempt to guess the LCHS disk geometry from the MS-DOS master boot
 * record (partition table). */
static int biosGuessDiskLCHS(PPDMIBLOCK pBlock, PPDMMEDIAGEOMETRY pLCHSGeometry)
{
    uint8_t aMBR[512], *p;
    int rc;
    uint32_t iEndHead, iEndSector, cLCHSCylinders, cLCHSHeads, cLCHSSectors;

    if (!pBlock)
        return VERR_INVALID_PARAMETER;
    rc = pBlock->pfnRead(pBlock, 0, aMBR, sizeof(aMBR));
    if (VBOX_FAILURE(rc))
        return rc;
    /* Test MBR magic number. */
    if (aMBR[510] != 0x55 || aMBR[511] != 0xaa)
        return VERR_INVALID_PARAMETER;
    for (uint32_t i = 0; i < 4; i++)
    {
        /* Figure out the start of a partition table entry. */
        p = &aMBR[0x1be + i * 16];
        iEndHead = p[5];
        iEndSector = p[6] & 63;
        if ((p[12] | p[13] | p[14] | p[15]) && iEndSector & iEndHead)
        {
            /* Assumption: partition terminates on a cylinder boundary. */
            cLCHSHeads = iEndHead + 1;
            cLCHSSectors = iEndSector;
            cLCHSCylinders = RT_MIN(1024, pBlock->pfnGetSize(pBlock) / (512 * cLCHSHeads * cLCHSSectors));
            if (cLCHSCylinders >= 1)
            {
                pLCHSGeometry->cCylinders = cLCHSCylinders;
                pLCHSGeometry->cHeads = cLCHSHeads;
                pLCHSGeometry->cSectors = cLCHSSectors;
                Log(("%s: LCHS=%d %d %d\n", __FUNCTION__, cLCHSCylinders, cLCHSHeads, cLCHSSectors));
                return VINF_SUCCESS;
            }
        }
    }
    return VERR_INVALID_PARAMETER;
}


/**
 * Write to CMOS memory.
 * This is used by the init complete code.
 */
static void pcbiosCmosWrite(PPDMDEVINS pDevIns, int off, uint32_t u32Val)
{
    Assert(off < 128);
    Assert(u32Val < 256);

#if 1
    int rc = PDMDevHlpCMOSWrite(pDevIns, off, u32Val);
    AssertRC(rc);
#else
    PVM pVM = PDMDevHlpGetVM(pDevIns);
    IOMIOPortWrite(pVM, 0x70, off, 1);
    IOMIOPortWrite(pVM, 0x71, u32Val, 1);
    IOMIOPortWrite(pVM, 0x70, 0, 1);
#endif
}

/* -=-=-=-=-=-=- based on code from pc.c -=-=-=-=-=-=- */

/**
 * Initializes the CMOS data for one harddisk.
 */
static void pcbiosCmosInitHardDisk(PPDMDEVINS pDevIns, int offType, int offInfo, PCPDMMEDIAGEOMETRY pLCHSGeometry)
{
    Log2(("%s: offInfo=%#x: LCHS=%d/%d/%d\n", __FUNCTION__, offInfo, pLCHSGeometry->cCylinders, pLCHSGeometry->cHeads, pLCHSGeometry->cSectors));
    if (offType)
        pcbiosCmosWrite(pDevIns, offType, 48);
    /* Cylinders low */
    pcbiosCmosWrite(pDevIns, offInfo + 0, RT_MIN(pLCHSGeometry->cCylinders, 1024) & 0xff);
    /* Cylinders high */
    pcbiosCmosWrite(pDevIns, offInfo + 1, RT_MIN(pLCHSGeometry->cCylinders, 1024) >> 8);
    /* Heads */
    pcbiosCmosWrite(pDevIns, offInfo + 2, pLCHSGeometry->cHeads);
    /* Landing zone low */
    pcbiosCmosWrite(pDevIns, offInfo + 3, 0xff);
    /* Landing zone high */
    pcbiosCmosWrite(pDevIns, offInfo + 4, 0xff);
    /* Write precomp low */
    pcbiosCmosWrite(pDevIns, offInfo + 5, 0xff);
    /* Write precomp high */
    pcbiosCmosWrite(pDevIns, offInfo + 6, 0xff);
    /* Sectors */
    pcbiosCmosWrite(pDevIns, offInfo + 7, pLCHSGeometry->cSectors);
}

/**
 * Set logical CHS geometry for a hard disk
 *
 * @returns VBox status code.
 * @param   pBase         Base interface for the device.
 * @param   pHardDisk     The hard disk.
 * @param   pLCHSGeometry Where to store the geometry settings.
 */
static int setLogicalDiskGeometry(PPDMIBASE pBase, PPDMIBLOCKBIOS pHardDisk, PPDMMEDIAGEOMETRY pLCHSGeometry)
{
    PDMMEDIAGEOMETRY LCHSGeometry;
    int rc = VINF_SUCCESS;

    rc = pHardDisk->pfnGetLCHSGeometry(pHardDisk, &LCHSGeometry);
    if (   rc == VERR_PDM_GEOMETRY_NOT_SET
        || LCHSGeometry.cCylinders == 0
        || LCHSGeometry.cCylinders > 1024
        || LCHSGeometry.cHeads == 0
        || LCHSGeometry.cHeads > 255
        || LCHSGeometry.cSectors == 0
        || LCHSGeometry.cSectors > 63)
    {
        PPDMIBLOCK pBlock;
        pBlock = (PPDMIBLOCK)pBase->pfnQueryInterface(pBase, PDMINTERFACE_BLOCK);
        /* No LCHS geometry, autodetect and set. */
        rc = biosGuessDiskLCHS(pBlock, &LCHSGeometry);
        if (VBOX_FAILURE(rc))
        {
            /* Try if PCHS geometry works, otherwise fall back. */
            rc = pHardDisk->pfnGetPCHSGeometry(pHardDisk, &LCHSGeometry);
        }
        if (   VBOX_FAILURE(rc)
            || LCHSGeometry.cCylinders == 0
            || LCHSGeometry.cCylinders > 1024
            || LCHSGeometry.cHeads == 0
            || LCHSGeometry.cHeads > 16
            || LCHSGeometry.cSectors == 0
            || LCHSGeometry.cSectors > 63)
        {
            uint64_t cSectors = pBlock->pfnGetSize(pBlock) / 512;
            if (cSectors / 16 / 63 <= 1024)
            {
                LCHSGeometry.cCylinders = RT_MAX(cSectors / 16 / 63, 1);
                LCHSGeometry.cHeads = 16;
            }
            else if (cSectors / 32 / 63 <= 1024)
            {
                LCHSGeometry.cCylinders = RT_MAX(cSectors / 32 / 63, 1);
                LCHSGeometry.cHeads = 32;
            }
            else if (cSectors / 64 / 63 <= 1024)
            {
                LCHSGeometry.cCylinders = cSectors / 64 / 63;
                LCHSGeometry.cHeads = 64;
            }
            else if (cSectors / 128 / 63 <= 1024)
            {
                LCHSGeometry.cCylinders = cSectors / 128 / 63;
                LCHSGeometry.cHeads = 128;
            }
            else
            {
                LCHSGeometry.cCylinders = RT_MIN(cSectors / 255 / 63, 1024);
                LCHSGeometry.cHeads = 255;
            }
            LCHSGeometry.cSectors = 63;

        }
        rc = pHardDisk->pfnSetLCHSGeometry(pHardDisk, &LCHSGeometry);
        if (rc == VERR_VDI_IMAGE_READ_ONLY)
        {
            LogRel(("DevPcBios: ATA failed to update LCHS geometry\n"));
            rc = VINF_SUCCESS;
        }
    }

    *pLCHSGeometry = LCHSGeometry;

    return rc;
}

/**
 * Get BIOS boot code from enmBootDevice in order
 *
 * @todo r=bird: This is a rather silly function since the conversion is 1:1.
 */
static uint8_t getBiosBootCode(PDEVPCBIOS pData, unsigned iOrder)
{
    switch (pData->aenmBootDevice[iOrder])
    {
        case DEVPCBIOSBOOT_NONE:
            return 0;
        case DEVPCBIOSBOOT_FLOPPY:
            return 1;
        case DEVPCBIOSBOOT_HD:
            return 2;
        case DEVPCBIOSBOOT_DVD:
            return 3;
        case DEVPCBIOSBOOT_LAN:
            return 4;
        default:
            AssertMsgFailed(("aenmBootDevice[%d]=%d\n", iOrder, pData->aenmBootDevice[iOrder]));
            return 0;
    }
}


/**
 * Init complete notification.
 * This routine will write information needed by the bios to the CMOS.
 *
 * @returns VBOX status code.
 * @param   pDevIns     The device instance.
 * @see     http://www.brl.ntt.co.jp/people/takehiko/interrupt/CMOS.LST.txt for
 *          a description of standard and non-standard CMOS registers.
 */
static DECLCALLBACK(int) pcbiosInitComplete(PPDMDEVINS pDevIns)
{
    PDEVPCBIOS      pData = PDMINS2DATA(pDevIns, PDEVPCBIOS);
    uint32_t        u32;
    unsigned        i;
    PVM             pVM = PDMDevHlpGetVM(pDevIns);
    PPDMIBLOCKBIOS  apHDs[4] = {0};
    PPDMIBLOCKBIOS  apFDs[2] = {0};
    AssertRelease(pVM);
    LogFlow(("pcbiosInitComplete:\n"));

    /*
     * Memory sizes.
     */
    /* base memory. */
    u32 = pData->cbRam > 640 ? 640 : (uint32_t)pData->cbRam / _1K;
    pcbiosCmosWrite(pDevIns, 0x15, u32 & 0xff);                                 /* 15h - Base Memory in K, Low Byte */
    pcbiosCmosWrite(pDevIns, 0x16, u32 >> 8);                                   /* 16h - Base Memory in K, High Byte */

    /* Extended memory, up to 65MB */
    u32 = pData->cbRam >= 65 * _1M ? 0xffff : ((uint32_t)pData->cbRam - _1M) / _1K;
    pcbiosCmosWrite(pDevIns, 0x17, u32 & 0xff);                                 /* 17h - Extended Memory in K, Low Byte */
    pcbiosCmosWrite(pDevIns, 0x18, u32 >> 8);                                   /* 18h - Extended Memory in K, High Byte */
    pcbiosCmosWrite(pDevIns, 0x30, u32 & 0xff);                                 /* 30h - Extended Memory in K, Low Byte */
    pcbiosCmosWrite(pDevIns, 0x31, u32 >> 8);                                   /* 31h - Extended Memory in K, High Byte */

    /* Bochs BIOS specific? Anyway, it's the amount of memory above 16MB */
    if (pData->cbRam > 16 * _1M)
    {
        u32 = (uint32_t)( (pData->cbRam - 16 * _1M) / _64K );
        u32 = RT_MIN(u32, 0xffff);
    }
    else
        u32 = 0;
    pcbiosCmosWrite(pDevIns, 0x34, u32 & 0xff);
    pcbiosCmosWrite(pDevIns, 0x35, u32 >> 8);

    /*
     * Bochs BIOS specifics - boot device.
     * We do both new and old (ami-style) settings.
     * See rombios.c line ~7215 (int19_function).
     */

    uint8_t reg3d = getBiosBootCode(pData, 0) | (getBiosBootCode(pData, 1) << 4);
    uint8_t reg38 = /* pcbiosCmosRead(pDevIns, 0x38) | */ getBiosBootCode(pData, 2) << 4;
    /* This is an extension. Bochs BIOS normally supports only 3 boot devices. */
    uint8_t reg3c = getBiosBootCode(pData, 3) | (pData->uBootDelay << 4);
    pcbiosCmosWrite(pDevIns, 0x3d, reg3d);
    pcbiosCmosWrite(pDevIns, 0x38, reg38);
    pcbiosCmosWrite(pDevIns, 0x3c, reg3c);

    /*
     * PXE debug option.
     */
    pcbiosCmosWrite(pDevIns, 0x3f, pData->u8PXEDebug);

    /*
     * Floppy drive type.
     */
    for (i = 0; i < ELEMENTS(apFDs); i++)
    {
        PPDMIBASE pBase;
        int rc = PDMR3QueryLun(pVM, pData->pszFDDevice, 0, i, &pBase);
        if (VBOX_SUCCESS(rc))
            apFDs[i] = (PPDMIBLOCKBIOS)pBase->pfnQueryInterface(pBase, PDMINTERFACE_BLOCK_BIOS);
    }
    u32 = 0;
    if (apFDs[0])
        switch (apFDs[0]->pfnGetType(apFDs[0]))
        {
            case PDMBLOCKTYPE_FLOPPY_360:   u32 |= 1 << 4; break;
            case PDMBLOCKTYPE_FLOPPY_1_20:  u32 |= 2 << 4; break;
            case PDMBLOCKTYPE_FLOPPY_720:   u32 |= 3 << 4; break;
            case PDMBLOCKTYPE_FLOPPY_1_44:  u32 |= 4 << 4; break;
            case PDMBLOCKTYPE_FLOPPY_2_88:  u32 |= 5 << 4; break;
            default:                        AssertFailed(); break;
        }
    if (apFDs[1])
        switch (apFDs[1]->pfnGetType(apFDs[1]))
        {
            case PDMBLOCKTYPE_FLOPPY_360:   u32 |= 1; break;
            case PDMBLOCKTYPE_FLOPPY_1_20:  u32 |= 2; break;
            case PDMBLOCKTYPE_FLOPPY_720:   u32 |= 3; break;
            case PDMBLOCKTYPE_FLOPPY_1_44:  u32 |= 4; break;
            case PDMBLOCKTYPE_FLOPPY_2_88:  u32 |= 5; break;
            default:                        AssertFailed(); break;
        }
    pcbiosCmosWrite(pDevIns, 0x10, u32);                                        /* 10h - Floppy Drive Type */

    /*
     * Equipment byte.
     */
    u32 = !!apFDs[0] + !!apFDs[1];
    switch (u32)
    {
        case 1: u32 = 0x01; break;      /* floppy installed, 2 drives. */
        default:u32 = 0;    break;      /* floppy not installed. */
    }
    u32 |= RT_BIT(1);                      /* math coprocessor installed  */
    u32 |= RT_BIT(2);                      /* keyboard enabled (or mouse?) */
    u32 |= RT_BIT(3);                      /* display enabled (monitory type is 0, i.e. vga) */
    pcbiosCmosWrite(pDevIns, 0x14, u32);                                        /* 14h - Equipment Byte */

    /*
     * Harddisks.
     */
    for (i = 0; i < ELEMENTS(apHDs); i++)
    {
        PPDMIBASE pBase;
        int rc = PDMR3QueryLun(pVM, pData->pszHDDevice, 0, i, &pBase);
        if (VBOX_SUCCESS(rc))
            apHDs[i] = (PPDMIBLOCKBIOS)pBase->pfnQueryInterface(pBase, PDMINTERFACE_BLOCK_BIOS);
        if (   apHDs[i]
            && (   apHDs[i]->pfnGetType(apHDs[i]) != PDMBLOCKTYPE_HARD_DISK
                || !apHDs[i]->pfnIsVisible(apHDs[i])))
            apHDs[i] = NULL;
        if (apHDs[i])
        {
            PDMMEDIAGEOMETRY LCHSGeometry;
            int rc = setLogicalDiskGeometry(pBase, apHDs[i], &LCHSGeometry);
            AssertRC(rc);

            if (i < 4)
            {
                /* Award BIOS extended drive types for first to fourth disk.
                 * Used by the BIOS for setting the logical geometry. */
                int offType, offInfo;
                switch (i)
                {
                    case 0:
                        offType = 0x19;
                        offInfo = 0x1e;
                        break;
                    case 1:
                        offType = 0x1a;
                        offInfo = 0x26;
                        break;
                    case 2:
                        offType = 0x00;
                        offInfo = 0x67;
                        break;
                    case 3:
                    default:
                        offType = 0x00;
                        offInfo = 0x70;
                        break;
                }
                pcbiosCmosInitHardDisk(pDevIns, offType, offInfo,
                                       &LCHSGeometry);
            }
            LogRel(("DevPcBios: ATA LUN#%d LCHS=%u/%u/%u\n", i, LCHSGeometry.cCylinders, LCHSGeometry.cHeads, LCHSGeometry.cSectors));
        }
    }

    /* 0Fh means extended and points to 19h, 1Ah */
    u32 = (apHDs[0] ? 0xf0 : 0) | (apHDs[1] ? 0x0f : 0);
    pcbiosCmosWrite(pDevIns, 0x12, u32);

    /*
     * Sata Harddisks.
     */
    if (pData->pszSataDevice)
    {
        for (i = 0; i < ELEMENTS(apHDs); i++)
        {
            PPDMIBASE pBase;
            int rc = PDMR3QueryLun(pVM, pData->pszSataDevice, 0, pData->iSataHDLUN[i], &pBase);
            if (VBOX_SUCCESS(rc))
                apHDs[i] = (PPDMIBLOCKBIOS)pBase->pfnQueryInterface(pBase, PDMINTERFACE_BLOCK_BIOS);
            if (   apHDs[i]
                && (   apHDs[i]->pfnGetType(apHDs[i]) != PDMBLOCKTYPE_HARD_DISK
                    || !apHDs[i]->pfnIsVisible(apHDs[i])))
                apHDs[i] = NULL;
            if (apHDs[i])
            {
                PDMMEDIAGEOMETRY LCHSGeometry;
                int rc = setLogicalDiskGeometry(pBase, apHDs[i], &LCHSGeometry);
                AssertRC(rc);

                if (i < 4)
                {
                    /* Award BIOS extended drive types for first to fourth disk.
                     * Used by the BIOS for setting the logical geometry. */
                    int offInfo;
                    switch (i)
                    {
                        case 0:
                            offInfo = 0x40;
                            break;
                        case 1:
                            offInfo = 0x48;
                            break;
                        case 2:
                            offInfo = 0x50;
                            break;
                        case 3:
                        default:
                            offInfo = 0x58;
                            break;
                    }
                    pcbiosCmosInitHardDisk(pDevIns, 0x00, offInfo,
                                           &LCHSGeometry);
                }
                LogRel(("DevPcBios: SATA LUN#%d LCHS=%u/%u/%u\n", i, LCHSGeometry.cCylinders, LCHSGeometry.cHeads, LCHSGeometry.cSectors));
            }
        }
    }

    LogFlow(("%s: returns VINF_SUCCESS\n", __FUNCTION__));
    return VINF_SUCCESS;
}


/**
 * Port I/O Handler for IN operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument - ignored.
 * @param   Port        Port number used for the IN operation.
 * @param   pu32        Where to store the result.
 * @param   cb          Number of bytes read.
 */
static DECLCALLBACK(int) pcbiosIOPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t *pu32, unsigned cb)
{
    NOREF(pDevIns);
    NOREF(pvUser);
    NOREF(Port);
    NOREF(pu32);
    NOREF(cb);
    return VERR_IOM_IOPORT_UNUSED;
}


/**
 * Port I/O Handler for OUT operations.
 *
 * @returns VBox status code.
 *
 * @param   pDevIns     The device instance.
 * @param   pvUser      User argument - ignored.
 * @param   Port        Port number used for the IN operation.
 * @param   u32         The value to output.
 * @param   cb          The value size in bytes.
 */
static DECLCALLBACK(int) pcbiosIOPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT Port, uint32_t u32, unsigned cb)
{
    /*
     * Bochs BIOS Panic
     */
    if (    cb == 2
        &&  (   Port == 0x400
             || Port == 0x401))
    {
        Log(("pcbios: PC BIOS panic at rombios.c(%d)\n", u32));
        AssertReleaseMsgFailed(("PC BIOS panic at rombios.c(%d)\n", u32));
        return VERR_INTERNAL_ERROR;
    }

    /*
     * Bochs BIOS char printing.
     */
    if (    cb == 1
        &&  (   Port == 0x402
             || Port == 0x403))
    {
        PDEVPCBIOS pData = PDMINS2DATA(pDevIns, PDEVPCBIOS);
        /* The raw version. */
        switch (u32)
        {
            case '\r': Log2(("pcbios: <return>\n")); break;
            case '\n': Log2(("pcbios: <newline>\n")); break;
            case '\t': Log2(("pcbios: <tab>\n")); break;
            default:   Log2(("pcbios: %c (%02x)\n", u32, u32)); break;
        }

        /* The readable, buffered version. */
        if (u32 == '\n' || u32 == '\r')
        {
            pData->szMsg[pData->iMsg] = '\0';
            if (pData->iMsg)
                Log(("pcbios: %s\n", pData->szMsg));
            pData->iMsg = 0;
        }
        else
        {
            if (pData->iMsg >= sizeof(pData->szMsg)-1)
            {
                pData->szMsg[pData->iMsg] = '\0';
                Log(("pcbios: %s\n", pData->szMsg));
                pData->iMsg = 0;
            }
            pData->szMsg[pData->iMsg] = (char )u32;
            pData->szMsg[++pData->iMsg] = '\0';
        }
        return VINF_SUCCESS;
    }

    /*
     * Bochs BIOS shutdown request.
     */
    if (cb == 1 && Port == 0x8900)
    {
        static const unsigned char szShutdown[] = "Shutdown";
        PDEVPCBIOS pData = PDMINS2DATA(pDevIns, PDEVPCBIOS);
        if (u32 == szShutdown[pData->iShutdown])
        {
            pData->iShutdown++;
            if (pData->iShutdown == 8)
            {
                pData->iShutdown = 0;
                LogRel(("8900h shutdown request.\n"));
                return PDMDevHlpVMPowerOff(pDevIns);
            }
        }
        else
            pData->iShutdown = 0;
        return VINF_SUCCESS;
    }

    /* not in use. */
    return VINF_SUCCESS;
}


/**
 * Construct the DMI table.
 *
 * @returns VBox status code.
 * @param   pDevIns     The device instance.
 * @param   pTable      Where to create the DMI table.
 * @param   cbMax       The max size of the DMI table.
 * @param   pUuid       Pointer to the UUID to use if the DmiUuid
 *                      configuration string isn't present.
 * @param   pCfgHandle  The handle to our config node.
 */
static int pcbiosPlantDMITable(PPDMDEVINS pDevIns, uint8_t *pTable, unsigned cbMax, PRTUUID pUuid, PCFGMNODE pCfgHandle)
{
    char *pszStr = (char *)pTable;
    int iStrNr;
    int rc;
    char *pszDmiVendor, *pszDmiProduct, *pszDmiVersion, *pszDmiRelease, *pszDmiSerial, *pszDmiUuid, *pszDmiFamily;

#define STRCPY(p, s) \
    do { \
        size_t _len = strlen(s) + 1; \
        size_t _max = (size_t)(pszStr + _len - (char *)pTable) + 1; /* +1 for strtab terminator */ \
        if (_max > cbMax) \
            return PDMDevHlpVMSetError(pDevIns, VERR_TOO_MUCH_DATA, RT_SRC_POS, \
                    N_("One of the DMI strings is too long. Check all bios/Dmi* configuration entries. At least %zu bytes are needed but there is no space for more than %d bytes"), _max, cbMax); \
        memcpy(p, s, _len); \
        p += _len; \
    } while (0)
#define READCFG(name, variable, default_value) \
    do { \
        rc = CFGMR3QueryStringAlloc(pCfgHandle, name, & variable); \
        if (rc == VERR_CFGM_VALUE_NOT_FOUND) \
            variable = MMR3HeapStrDup(PDMDevHlpGetVM(pDevIns), MM_TAG_CFGM, default_value); \
        else if (VBOX_FAILURE(rc)) \
            return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS, \
                    N_("Configuration error: Querying \"" name "\" as a string failed")); \
    } while (0)


    READCFG("DmiVendor",  pszDmiVendor,  "innotek GmbH");
    READCFG("DmiProduct", pszDmiProduct, "VirtualBox");
    READCFG("DmiVersion", pszDmiVersion, "1.2");
    READCFG("DmiRelease", pszDmiRelease, "12/01/2006");
    READCFG("DmiSerial",  pszDmiSerial,  "0");
    rc = CFGMR3QueryStringAlloc(pCfgHandle, "DmiUuid", &pszDmiUuid);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        pszDmiUuid = NULL;
    else if (VBOX_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Querying \"DmiUuid\" as a string failed"));
    READCFG("DmiFamily",  pszDmiFamily,   "Virtual Machine");

    /* DMI BIOS information */
    PDMIBIOSINF pBIOSInf         = (PDMIBIOSINF)pszStr;
    pszStr                       = (char *)(pBIOSInf + 1);
    iStrNr                       = 1;
    pBIOSInf->header.u8Type      = 0; /* BIOS Information */
    pBIOSInf->header.u8Length    = sizeof(*pBIOSInf);
    pBIOSInf->header.u16Handle   = 0x0000;
    pBIOSInf->u8Vendor           = iStrNr++;
    STRCPY(pszStr, pszDmiVendor);
    pBIOSInf->u8Version          = iStrNr++;
    STRCPY(pszStr, pszDmiProduct);
    pBIOSInf->u16Start           = 0xE000;
    pBIOSInf->u8Release          = iStrNr++;
    STRCPY(pszStr, pszDmiRelease);
    pBIOSInf->u8ROMSize          = 1; /* 128K */
    pBIOSInf->u64Characteristics = RT_BIT(4)   /* ISA is supported */
                                 | RT_BIT(7)   /* PCI is supported */
                                 | RT_BIT(15)  /* Boot from CD is supported */
                                 | RT_BIT(16)  /* Selectable Boot is supported */
                                 | RT_BIT(27)  /* Int 9h, 8042 Keyboard services supported */
                                 | RT_BIT(30)  /* Int 10h, CGA/Mono Video Services supported */
                                 /* any more?? */
                                 ;
    pBIOSInf->u8CharacteristicsByte1 = RT_BIT(0)   /* ACPI is supported */
                                     /* any more?? */
                                     ;
    pBIOSInf->u8CharacteristicsByte2 = 0
                                     /* any more?? */
                                     ;
    *pszStr++                    = '\0';

    /* DMI system information */
    PDMISYSTEMINF pSystemInf     = (PDMISYSTEMINF)pszStr;
    pszStr                       = (char *)(pSystemInf + 1);
    iStrNr                       = 1;
    pSystemInf->header.u8Type    = 1; /* System Information */
    pSystemInf->header.u8Length  = sizeof(*pSystemInf);
    pSystemInf->header.u16Handle = 0x0001;
    pSystemInf->u8Manufacturer   = iStrNr++;
    STRCPY(pszStr, pszDmiVendor);
    pSystemInf->u8ProductName    = iStrNr++;
    STRCPY(pszStr, pszDmiProduct);
    pSystemInf->u8Version        = iStrNr++;
    STRCPY(pszStr, pszDmiVersion);
    pSystemInf->u8SerialNumber   = iStrNr++;
    STRCPY(pszStr, pszDmiSerial);

    RTUUID uuid;
    if (pszDmiUuid)
    {
        int rc = RTUuidFromStr(&uuid, pszDmiUuid);
        if (VBOX_FAILURE(rc))
            return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                       N_("Invalid UUID for DMI tables specified"));
        uuid.Gen.u32TimeLow = RT_H2BE_U32(uuid.Gen.u32TimeLow);
        uuid.Gen.u16TimeMid = RT_H2BE_U16(uuid.Gen.u16TimeMid);
        uuid.Gen.u16TimeHiAndVersion = RT_H2BE_U16(uuid.Gen.u16TimeHiAndVersion);
        pUuid = &uuid;
    }
    memcpy(pSystemInf->au8Uuid, pUuid, sizeof(RTUUID));

    pSystemInf->u8WakeupType     = 6; /* Power Switch */
    pSystemInf->u8SKUNumber      = 0;
    pSystemInf->u8Family         = iStrNr++;
    STRCPY(pszStr, pszDmiFamily);
    *pszStr++                    = '\0';

    /* If more fields are added here, fix the size check in STRCPY */

#undef STRCPY
#undef READCFG

    MMR3HeapFree(pszDmiVendor);
    MMR3HeapFree(pszDmiProduct);
    MMR3HeapFree(pszDmiVersion);
    MMR3HeapFree(pszDmiRelease);
    MMR3HeapFree(pszDmiSerial);
    MMR3HeapFree(pszDmiUuid);
    MMR3HeapFree(pszDmiFamily);

    return VINF_SUCCESS;
}
AssertCompile(VBOX_DMI_TABLE_ENTR == 2);


/**
 * Calculate a simple checksum for the MPS table.
 *
 * @param   data            data
 * @param   len             size of data
 */
static uint8_t pcbiosChecksum(const uint8_t * const au8Data, uint32_t u32Length)
{
    uint8_t u8Sum = 0;
    for (size_t i = 0; i < u32Length; ++i)
        u8Sum += au8Data[i];
    return -u8Sum;
}


/**
 * Construct the MPS table. Only applicable if IOAPIC is active.
 *
 * @param   pDevIns    The device instance data.
 * @param   addr       physical address in guest memory.
 */
static void pcbiosPlantMPStable(PPDMDEVINS pDevIns, uint8_t *pTable)
{
    /* configuration table */
    PMPSCFGTBLHEADER pCfgTab      = (MPSCFGTBLHEADER*)pTable;
    memcpy(pCfgTab->au8Signature, "PCMP", 4);
    pCfgTab->u8SpecRev             =  4;    /* 1.4 */
    memcpy(pCfgTab->au8OemId, "VBOXCPU ", 8);
    memcpy(pCfgTab->au8ProductId, "VirtualBox  ", 12);
    pCfgTab->u32OemTablePtr        =  0;
    pCfgTab->u16OemTableSize       =  0;
    pCfgTab->u16EntryCount         =  1 /* Processor */
                                   +  1 /* ISA Bus */
                                   +  1 /* I/O-APIC */
                                   + 16 /* Interrupts */;
    pCfgTab->u32AddrLocalApic      = 0xfee00000;
    pCfgTab->u16ExtTableLength     =  0;
    pCfgTab->u8ExtTableChecksxum   =  0;
    pCfgTab->u8Reserved            =  0;

    uint32_t u32Eax, u32Ebx, u32Ecx, u32Edx;
    uint32_t u32CPUSignature = 0x0520; /* default: Pentium 100 */
    uint32_t u32FeatureFlags = 0x0001; /* default: FPU */
    PDMDevHlpGetCpuId(pDevIns, 0, &u32Eax, &u32Ebx, &u32Ecx, &u32Edx);
    if (u32Eax >= 1)
    {
        PDMDevHlpGetCpuId(pDevIns, 1, &u32Eax, &u32Ebx, &u32Ecx, &u32Edx);
        u32CPUSignature = u32Eax & 0xfff;
        /* Local APIC will be enabled later so override it here. Since we provide
         * an MP table we have an IOAPIC and therefore a Local APIC. */
        u32FeatureFlags = u32Edx | X86_CPUID_FEATURE_EDX_APIC;
    }

    /* one processor so far */
    PMPSPROCENTRY pProcEntry       = (PMPSPROCENTRY)(pCfgTab+1);
    pProcEntry->u8EntryType        = 0; /* processor entry */
    pProcEntry->u8LocalApicId      = 0;
    pProcEntry->u8LocalApicVersion = 0x11;
    pProcEntry->u8CPUFlags         = 2 /* bootstrap processor */ | 1 /* enabled */;
    pProcEntry->u32CPUSignature    = u32CPUSignature;
    pProcEntry->u32CPUFeatureFlags = u32FeatureFlags;
    pProcEntry->u32Reserved[0]     =
    pProcEntry->u32Reserved[1]     = 0;

    /* ISA bus */
    PMPSBUSENTRY pBusEntry         = (PMPSBUSENTRY)(pProcEntry+1);
    pBusEntry->u8EntryType         = 1; /* bus entry */
    pBusEntry->u8BusId             = 0; /* this ID is referenced by the interrupt entries */
    memcpy(pBusEntry->au8BusTypeStr, "ISA   ", 6);

    /* PCI bus? */

    /* I/O-APIC.
     * MP spec: "The configuration table contains one or more entries for I/O APICs.
     *           ... At least one I/O APIC must be enabled." */
    PMPSIOAPICENTRY pIOAPICEntry   = (PMPSIOAPICENTRY)(pBusEntry+1);
    pIOAPICEntry->u8EntryType      = 2; /* I/O-APIC entry */
    pIOAPICEntry->u8Id             = 1; /* this ID is referenced by the interrupt entries */
    pIOAPICEntry->u8Version        = 0x11;
    pIOAPICEntry->u8Flags          = 1 /* enable */;
    pIOAPICEntry->u32Addr          = 0xfec00000;

    PMPSIOIRQENTRY pIrqEntry       = (PMPSIOIRQENTRY)(pIOAPICEntry+1);
    for (int i = 0; i < 16; i++, pIrqEntry++)
    {
        pIrqEntry->u8EntryType     = 3; /* I/O interrupt entry */
        pIrqEntry->u8Type          = 0; /* INT, vectored interrupt */
        pIrqEntry->u16Flags        = 0; /* polarity of APIC I/O input signal = conforms to bus,
                                           trigger mode = conforms to bus */
        pIrqEntry->u8SrcBusId      = 0; /* ISA bus */
        pIrqEntry->u8SrcBusIrq     = i;
        pIrqEntry->u8DstIOAPICId   = 1;
        pIrqEntry->u8DstIOAPICInt  = i;
    }

    pCfgTab->u16Length             = (uint8_t*)pIrqEntry - pTable;
    pCfgTab->u8Checksum            = pcbiosChecksum(pTable, pCfgTab->u16Length);

    AssertMsg(pCfgTab->u16Length < 0x1000 - 0x100,
              ("VBOX_MPS_TABLE_SIZE=%d, maximum allowed size is %d",
              pCfgTab->u16Length, 0x1000-0x100));

    MPSFLOATPTR floatPtr;
    floatPtr.au8Signature[0]       = '_';
    floatPtr.au8Signature[1]       = 'M';
    floatPtr.au8Signature[2]       = 'P';
    floatPtr.au8Signature[3]       = '_';
    floatPtr.u32MPSAddr            = VBOX_MPS_TABLE_BASE;
    floatPtr.u8Length              = 1; /* structure size in paragraphs */
    floatPtr.u8SpecRev             = 4; /* MPS revision 1.4 */
    floatPtr.u8Checksum            = 0;
    floatPtr.au8Feature[0]         = 0;
    floatPtr.au8Feature[1]         = 0;
    floatPtr.au8Feature[2]         = 0;
    floatPtr.au8Feature[3]         = 0;
    floatPtr.au8Feature[4]         = 0;
    floatPtr.u8Checksum            = pcbiosChecksum((uint8_t*)&floatPtr, 16);
    PDMDevHlpPhysWrite (pDevIns, 0x9fff0, &floatPtr, 16);
}


/**
 * Reset notification.
 *
 * @returns VBox status.
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(void) pcbiosReset(PPDMDEVINS pDevIns)
{
    PDEVPCBIOS  pData = PDMINS2DATA(pDevIns, PDEVPCBIOS);
    LogFlow(("pcbiosReset:\n"));

    if (pData->u8IOAPIC)
        pcbiosPlantMPStable(pDevIns, pData->au8DMIPage + 0x100);
}


/**
 * Destruct a device instance.
 *
 * Most VM resources are freed by the VM. This callback is provided so that any non-VM
 * resources can be freed correctly.
 *
 * @param   pDevIns     The device instance data.
 */
static DECLCALLBACK(int) pcbiosDestruct(PPDMDEVINS pDevIns)
{
    PDEVPCBIOS  pData = PDMINS2DATA(pDevIns, PDEVPCBIOS);
    LogFlow(("pcbiosDestruct:\n"));

    /*
     * Free MM heap pointers.
     */
    if (pData->pu8PcBios)
    {
        MMR3HeapFree(pData->pu8PcBios);
        pData->pu8PcBios = NULL;
    }

    if (pData->pszPcBiosFile)
    {
        MMR3HeapFree(pData->pszPcBiosFile);
        pData->pszPcBiosFile = NULL;
    }

    if (pData->pu8LanBoot)
    {
        MMR3HeapFree(pData->pu8LanBoot);
        pData->pu8LanBoot = NULL;
    }

    if (pData->pszLanBootFile)
    {
        MMR3HeapFree(pData->pszLanBootFile);
        pData->pszLanBootFile = NULL;
    }

    return VINF_SUCCESS;
}


/**
 * Convert config value to DEVPCBIOSBOOT.
 *
 * @returns VBox status code.
 * @param   pCfgHandle      Configuration handle.
 * @param   pszParam        The name of the value to read.
 * @param   penmBoot        Where to store the boot method.
 */
static int pcbiosBootFromCfg(PPDMDEVINS pDevIns, PCFGMNODE pCfgHandle, const char *pszParam, DEVPCBIOSBOOT *penmBoot)
{
    char *psz;
    int rc = CFGMR3QueryStringAlloc(pCfgHandle, pszParam, &psz);
    if (VBOX_FAILURE(rc))
        return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                   N_("Configuration error: Querying \"%s\" as a string failed"),
                                   pszParam);
    if (!strcmp(psz, "DVD") || !strcmp(psz, "CDROM"))
        *penmBoot = DEVPCBIOSBOOT_DVD;
    else if (!strcmp(psz, "IDE"))
        *penmBoot = DEVPCBIOSBOOT_HD;
    else if (!strcmp(psz, "FLOPPY"))
        *penmBoot = DEVPCBIOSBOOT_FLOPPY;
    else if (!strcmp(psz, "LAN"))
        *penmBoot = DEVPCBIOSBOOT_LAN;
    else if (!strcmp(psz, "NONE"))
        *penmBoot = DEVPCBIOSBOOT_NONE;
    else
    {
        PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                            N_("Configuration error: The \"%s\" value \"%s\" is unknown"),
                            pszParam, psz);
        rc = VERR_INTERNAL_ERROR;
    }
    MMR3HeapFree(psz);
    return rc;
}

/**
 * Construct a device instance for a VM.
 *
 * @returns VBox status.
 * @param   pDevIns     The device instance data.
 *                      If the registration structure is needed, pDevIns->pDevReg points to it.
 * @param   iInstance   Instance number. Use this to figure out which registers and such to use.
 *                      The device number is also found in pDevIns->iInstance, but since it's
 *                      likely to be freqently used PDM passes it as parameter.
 * @param   pCfgHandle  Configuration node handle for the device. Use this to obtain the configuration
 *                      of the device instance. It's also found in pDevIns->pCfgHandle, but like
 *                      iInstance it's expected to be used a bit in this function.
 */
static DECLCALLBACK(int)  pcbiosConstruct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfgHandle)
{
    unsigned    i;
    PDEVPCBIOS  pData = PDMINS2DATA(pDevIns, PDEVPCBIOS);
    int         rc;
    int         cb;

    Assert(iInstance == 0);

    /*
     * Validate configuration.
     */
    if (!CFGMR3AreValuesValid(pCfgHandle,
                              "BootDevice0\0"
                              "BootDevice1\0"
                              "BootDevice2\0"
                              "BootDevice3\0"
                              "RamSize\0"
                              "HardDiskDevice\0"
                              "SataHardDiskDevice\0"
                              "SataPrimaryMasterLUN\0"
                              "SataPrimarySlaveLUN\0"
                              "SataSecondaryMasterLUN\0"
                              "SataSecondarySlaveLUN\0"
                              "FloppyDevice\0"
                              "DelayBoot\0"
                              "BiosRom\0"
                              "LanBootRom\0"
                              "PXEDebug\0"
                              "UUID\0"
                              "IOAPIC\0"
                              "DmiVendor\0"
                              "DmiProduct\0"
                              "DmiVersion\0"
                              "DmiSerial\0"
                              "DmiUuid\0"
                              "DmiFamily\0"))
        return PDMDEV_SET_ERROR(pDevIns, VERR_PDM_DEVINS_UNKNOWN_CFG_VALUES,
                                N_("Invalid configuraton for  device pcbios device"));

    /*
     * Init the data.
     */
    rc = CFGMR3QueryU64(pCfgHandle, "RamSize", &pData->cbRam);
    if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"RamSize\" as integer failed"));

    rc = CFGMR3QueryU8 (pCfgHandle, "IOAPIC", &pData->u8IOAPIC);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        pData->u8IOAPIC = 1;
    else if (VBOX_FAILURE (rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to read \"IOAPIC\""));

    static const char * const s_apszBootDevices[] = { "BootDevice0", "BootDevice1", "BootDevice2", "BootDevice3" };
    Assert(ELEMENTS(s_apszBootDevices) == ELEMENTS(pData->aenmBootDevice));
    for (i = 0; i < ELEMENTS(pData->aenmBootDevice); i++)
    {
        rc = pcbiosBootFromCfg(pDevIns, pCfgHandle, s_apszBootDevices[i], &pData->aenmBootDevice[i]);
        if (VBOX_FAILURE(rc))
            return rc;
    }

    rc = CFGMR3QueryStringAlloc(pCfgHandle, "HardDiskDevice", &pData->pszHDDevice);
    if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"HardDiskDevice\" as a string failed"));

    rc = CFGMR3QueryStringAlloc(pCfgHandle, "FloppyDevice", &pData->pszFDDevice);
    if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"FloppyDevice\" as a string failed"));

    rc = CFGMR3QueryStringAlloc(pCfgHandle, "SataHardDiskDevice", &pData->pszSataDevice);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        pData->pszSataDevice = NULL;
    else if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"SataHardDiskDevice\" as a string failed"));

    if (pData->pszSataDevice)
    {
        static const char * const s_apszSataDisks[] =
            { "SataPrimaryMasterLUN", "SataPrimarySlaveLUN", "SataSecondaryMasterLUN", "SataSecondarySlaveLUN" };
        Assert(ELEMENTS(s_apszSataDisks) == ELEMENTS(pData->iSataHDLUN));
        for (i = 0; i < ELEMENTS(pData->iSataHDLUN); i++)
        {
            rc = CFGMR3QueryU32(pCfgHandle, s_apszSataDisks[i], &pData->iSataHDLUN[i]);
            if (rc == VERR_CFGM_VALUE_NOT_FOUND)
                pData->iSataHDLUN[i] = i;
            else if (VBOX_FAILURE(rc))
                return PDMDevHlpVMSetError(pDevIns, rc, RT_SRC_POS,
                                           N_("Configuration error: Querying \"%s\" as a string failed"), s_apszSataDisks);
        }
    }
    /*
     * Register I/O Ports and PC BIOS.
     */
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x400, 4, NULL, pcbiosIOPortWrite, pcbiosIOPortRead,
                                 NULL, NULL, "Bochs PC BIOS - Panic & Debug");
    if (VBOX_FAILURE(rc))
        return rc;
    rc = PDMDevHlpIOPortRegister(pDevIns, 0x8900, 1, NULL, pcbiosIOPortWrite, pcbiosIOPortRead,
                                 NULL, NULL, "Bochs PC BIOS - Shutdown");
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Query the machine's UUID for SMBIOS/DMI use.
     */
    RTUUID  uuid;
    rc = CFGMR3QueryBytes(pCfgHandle, "UUID", &uuid, sizeof(uuid));
    if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"UUID\" failed"));


    /* Convert the UUID to network byte order. Not entirely straightforward as parts are MSB already... */
    uuid.Gen.u32TimeLow = RT_H2BE_U32(uuid.Gen.u32TimeLow);
    uuid.Gen.u16TimeMid = RT_H2BE_U16(uuid.Gen.u16TimeMid);
    uuid.Gen.u16TimeHiAndVersion = RT_H2BE_U16(uuid.Gen.u16TimeHiAndVersion);
    rc = pcbiosPlantDMITable(pDevIns, pData->au8DMIPage, VBOX_DMI_TABLE_SIZE, &uuid, pCfgHandle);
    if (VBOX_FAILURE(rc))
        return rc;
    if (pData->u8IOAPIC)
        pcbiosPlantMPStable(pDevIns, pData->au8DMIPage + VBOX_DMI_TABLE_SIZE);

    rc = PDMDevHlpROMRegister(pDevIns, VBOX_DMI_TABLE_BASE, 0x1000, pData->au8DMIPage, false /* fShadow */, "DMI tables");
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Read the PXE debug logging option.
     */
    rc = CFGMR3QueryU8(pCfgHandle, "PXEDebug", &pData->u8PXEDebug);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
        pData->u8PXEDebug = 0;
    else if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"PXEDebug\" as integer failed"));

    /*
     * Get the system BIOS ROM file name.
     */
    rc = CFGMR3QueryStringAlloc(pCfgHandle, "BiosRom", &pData->pszPcBiosFile);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        pData->pszPcBiosFile = NULL;
        rc = VINF_SUCCESS;
    }
    else if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"BiosRom\" as a string failed"));
    else if (!*pData->pszPcBiosFile)
    {
        MMR3HeapFree(pData->pszPcBiosFile);
        pData->pszPcBiosFile = NULL;
    }

    const uint8_t *pu8PcBiosBinary = NULL;
    uint64_t cbPcBiosBinary;
    /*
     * Determine the system BIOS ROM size, open specified ROM file in the process.
     */
    RTFILE FilePcBios = NIL_RTFILE;
    if (pData->pszPcBiosFile)
    {
        rc = RTFileOpen(&FilePcBios, pData->pszPcBiosFile,
                        RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
        if (VBOX_SUCCESS(rc))
        {
            rc = RTFileGetSize(FilePcBios, &pData->cbPcBios);
            if (VBOX_SUCCESS(rc))
            {
                /* The following checks should be in sync the AssertReleaseMsg's below. */
                if (    RT_ALIGN(pData->cbPcBios, _64K) != pData->cbPcBios
                    ||  pData->cbPcBios > 32 * _64K
                    ||  pData->cbPcBios < _64K)
                    rc = VERR_TOO_MUCH_DATA;
            }
        }
        if (VBOX_FAILURE(rc))
        {
            /*
             * In case of failure simply fall back to the built-in BIOS ROM.
             */
            Log(("pcbiosConstruct: Failed to open system BIOS ROM file '%s', rc=%Vrc!\n", pData->pszPcBiosFile, rc));
            RTFileClose(FilePcBios);
            FilePcBios = NIL_RTFILE;
            MMR3HeapFree(pData->pszPcBiosFile);
            pData->pszPcBiosFile = NULL;
        }
    }

    /*
     * Attempt to get the system BIOS ROM data from file.
     */
    if (pData->pszPcBiosFile)
    {
        /*
         * Allocate buffer for the system BIOS ROM data.
         */
        pData->pu8PcBios = (uint8_t *)PDMDevHlpMMHeapAlloc(pDevIns, pData->cbPcBios);
        if (pData->pu8PcBios)
        {
            rc = RTFileRead(FilePcBios, pData->pu8PcBios, pData->cbPcBios, NULL);
            if (VBOX_FAILURE(rc))
            {
                AssertMsgFailed(("RTFileRead(,,%d,NULL) -> %Vrc\n", pData->cbPcBios, rc));
                MMR3HeapFree(pData->pu8PcBios);
                pData->pu8PcBios = NULL;
            }
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        pData->pu8PcBios = NULL;

    /* cleanup */
    if (FilePcBios != NIL_RTFILE)
        RTFileClose(FilePcBios);

    /* If we were unable to get the data from file for whatever reason, fall
     * back to the built-in ROM image.
     */
    if (pData->pu8PcBios == NULL)
    {
        pu8PcBiosBinary = g_abPcBiosBinary;
        cbPcBiosBinary  = g_cbPcBiosBinary;
    }
    else
    {
        pu8PcBiosBinary = pData->pu8PcBios;
        cbPcBiosBinary  = pData->cbPcBios;
    }

    /*
     * Map the BIOS into memory.
     * There are two mappings:
     *      1. 0x000e0000 to 0x000fffff contains the last 128 kb of the bios.
     *         The bios code might be 64 kb in size, and will then start at 0xf0000.
     *      2. 0xfffxxxxx to 0xffffffff contains the entire bios.
     */
    AssertReleaseMsg(cbPcBiosBinary >= _64K, ("cbPcBiosBinary=%#x\n", cbPcBiosBinary));
    AssertReleaseMsg(RT_ALIGN_Z(cbPcBiosBinary, _64K) == cbPcBiosBinary,
                     ("cbPcBiosBinary=%#x\n", cbPcBiosBinary));
    cb = RT_MIN(cbPcBiosBinary, 128 * _1K); /* Effectively either 64 or 128K. */
    rc = PDMDevHlpROMRegister(pDevIns, 0x00100000 - cb, cb, &pu8PcBiosBinary[cbPcBiosBinary - cb],
                              false /* fShadow */, "PC BIOS - 0xfffff");
    if (VBOX_FAILURE(rc))
        return rc;
    rc = PDMDevHlpROMRegister(pDevIns, (uint32_t)-(int32_t)cbPcBiosBinary, cbPcBiosBinary, pu8PcBiosBinary,
                              false /* fShadow */, "PC BIOS - 0xffffffff");
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Call reset to set values and stuff.
     */
    pcbiosReset(pDevIns);

    /*
     * Get the LAN boot ROM file name.
     */
    rc = CFGMR3QueryStringAlloc(pCfgHandle, "LanBootRom", &pData->pszLanBootFile);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        pData->pszLanBootFile = NULL;
        rc = VINF_SUCCESS;
    }
    else if (VBOX_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Querying \"LanBootRom\" as a string failed"));
    else if (!*pData->pszLanBootFile)
    {
        MMR3HeapFree(pData->pszLanBootFile);
        pData->pszLanBootFile = NULL;
    }

    uint64_t cbFileLanBoot;
    const uint8_t *pu8LanBootBinary = NULL;
    uint64_t cbLanBootBinary;

    /*
     * Determine the LAN boot ROM size, open specified ROM file in the process.
     */
    RTFILE FileLanBoot = NIL_RTFILE;
    if (pData->pszLanBootFile)
    {
        rc = RTFileOpen(&FileLanBoot, pData->pszLanBootFile,
                        RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
        if (VBOX_SUCCESS(rc))
        {
            rc = RTFileGetSize(FileLanBoot, &cbFileLanBoot);
            if (VBOX_SUCCESS(rc))
            {
                if (    RT_ALIGN(cbFileLanBoot, _4K) != cbFileLanBoot
                    ||  cbFileLanBoot > _64K)
                    rc = VERR_TOO_MUCH_DATA;
            }
        }
        if (VBOX_FAILURE(rc))
        {
            /*
             * Ignore failure and fall back to the built-in LAN boot ROM.
             */
            Log(("pcbiosConstruct: Failed to open LAN boot ROM file '%s', rc=%Vrc!\n", pData->pszLanBootFile, rc));
            RTFileClose(FileLanBoot);
            FileLanBoot = NIL_RTFILE;
            MMR3HeapFree(pData->pszLanBootFile);
            pData->pszLanBootFile = NULL;
        }
    }

    /*
     * Get the LAN boot ROM data.
     */
    if (pData->pszLanBootFile)
    {
        /*
         * Allocate buffer for the LAN boot ROM data.
         */
        pData->pu8LanBoot = (uint8_t *)PDMDevHlpMMHeapAlloc(pDevIns, cbFileLanBoot);
        if (pData->pu8LanBoot)
        {
            rc = RTFileRead(FileLanBoot, pData->pu8LanBoot, cbFileLanBoot, NULL);
            if (VBOX_FAILURE(rc))
            {
                AssertMsgFailed(("RTFileRead(,,%d,NULL) -> %Vrc\n", cbFileLanBoot, rc));
                MMR3HeapFree(pData->pu8LanBoot);
                pData->pu8LanBoot = NULL;
            }
            rc = VINF_SUCCESS;
        }
        else
            rc = VERR_NO_MEMORY;
    }
    else
        pData->pu8LanBoot = NULL;

    /* cleanup */
    if (FileLanBoot != NIL_RTFILE)
        RTFileClose(FileLanBoot);

    /* If we were unable to get the data from file for whatever reason, fall
     * back to the built-in LAN boot ROM image.
     */
    if (pData->pu8LanBoot == NULL)
    {
        pu8LanBootBinary = g_abNetBiosBinary;
        cbLanBootBinary  = g_cbNetBiosBinary;
    }
    else
    {
        pu8LanBootBinary = pData->pu8LanBoot;
        cbLanBootBinary  = cbFileLanBoot;
    }

    /*
     * Map the Network Boot ROM into memory.
     * Currently there is a fixed mapping: 0x000c8000 to 0x000cffff contains
     * the (up to) 32 kb ROM image.
     */
    if (pu8LanBootBinary)
        rc = PDMDevHlpROMRegister(pDevIns, VBOX_LANBOOT_SEG << 4, cbLanBootBinary, pu8LanBootBinary,
                                  true /* fShadow */, "Net Boot ROM");

    rc = CFGMR3QueryU8(pCfgHandle, "DelayBoot", &pData->uBootDelay);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        pData->uBootDelay = 0;
        rc = VINF_SUCCESS;
    }
    else
    {
        if (VBOX_FAILURE(rc))
            return PDMDEV_SET_ERROR(pDevIns, rc,
                                    N_("Configuration error: Querying \"DelayBoot\" as integer failed"));
        if (pData->uBootDelay > 15)
            pData->uBootDelay = 15;
    }

    return rc;
}


/**
 * The device registration structure.
 */
const PDMDEVREG g_DevicePcBios =
{
    /* u32Version */
    PDM_DEVREG_VERSION,
    /* szDeviceName */
    "pcbios",
    /* szGCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "PC BIOS Device",
    /* fFlags */
    PDM_DEVREG_FLAGS_HOST_BITS_DEFAULT | PDM_DEVREG_FLAGS_GUEST_BITS_32_64,
    /* fClass */
    PDM_DEVREG_CLASS_ARCH_BIOS,
    /* cMaxInstances */
    1,
    /* cbInstance */
    sizeof(DEVPCBIOS),
    /* pfnConstruct */
    pcbiosConstruct,
    /* pfnDestruct */
    pcbiosDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    pcbiosReset,
    /* pfnSuspend */
    NULL,
    /* pfnResume */
    NULL,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnQueryInterface. */
    NULL,
    /* pfnInitComplete. */
    pcbiosInitComplete
};


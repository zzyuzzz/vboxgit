/* $Id: pdmpcidevint.h 81031 2019-09-26 19:26:33Z vboxsync $ */
/** @file
 * DevPCI - PDM PCI Internal header - Only for hiding bits of PDMPCIDEV.
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef VBOX_INCLUDED_vmm_pdmpcidevint_h
#define VBOX_INCLUDED_vmm_pdmpcidevint_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <VBox/vmm/pdmdev.h>

/** @defgroup grp_pdm_pcidev_int    The PDM PCI Device Internals
 * @ingroup grp_pdm_pcidev
 *
 * @remarks The PDM PCI device internals are visible to both PDM and the PCI Bus
 *          implementation, thus it lives among the the public headers despite
 *          being rather private and internal.
 *
 * @{
 */


/**
 * PCI I/O region.
 */
typedef struct PCIIOREGION
{
    /** Current PCI mapping address, 0xffffffff means not mapped. */
    uint64_t                        addr;
    uint64_t                        size;
    uint8_t                         type; /* PCIADDRESSSPACE */
    uint8_t                         padding[HC_ARCH_BITS == 32 ? 3 : 7];
    /** Callback called when the region is mapped. */
    R3PTRTYPE(PFNPCIIOREGIONMAP)    map_func;
} PCIIOREGION, PCIIORegion;
/** Pointer to PCI I/O region. */
typedef PCIIOREGION *PPCIIOREGION;

/**
 * Callback function for reading from the PCI configuration space.
 *
 * @returns Strict VBox status code.
 * @param   pDevIns         Pointer to the device instance of the PCI bus.
 * @param   iBus            The bus number this device is on.
 * @param   iDevice         The number of the device on the bus.
 * @param   u32Address      The configuration space register address. [0..255]
 * @param   cb              The register size. [1,2,4]
 * @param   pu32Value       Where to return the register value.
 */
typedef DECLCALLBACK(VBOXSTRICTRC) FNPCIBRIDGECONFIGREAD(PPDMDEVINSR3 pDevIns, uint8_t iBus, uint8_t iDevice,
                                                         uint32_t u32Address, unsigned cb, uint32_t *pu32Value);
/** Pointer to a FNPCICONFIGREAD() function. */
typedef FNPCIBRIDGECONFIGREAD *PFNPCIBRIDGECONFIGREAD;
/** Pointer to a PFNPCICONFIGREAD. */
typedef PFNPCIBRIDGECONFIGREAD *PPFNPCIBRIDGECONFIGREAD;

/**
 * Callback function for writing to the PCI configuration space.
 *
 * @returns Strict VBox status code.
 * @param   pDevIns         Pointer to the device instance of the PCI bus.
 * @param   iBus            The bus number this device is on.
 * @param   iDevice         The number of the device on the bus.
 * @param   u32Address      The configuration space register address. [0..255]
 * @param   cb              The register size. [1,2,4]
 * @param   u32Value        The value that's being written. The number of bits actually used from
 *                          this value is determined by the cb parameter.
 */
typedef DECLCALLBACK(VBOXSTRICTRC) FNPCIBRIDGECONFIGWRITE(PPDMDEVINSR3 pDevIns, uint8_t iBus, uint8_t iDevice,
                                                          uint32_t u32Address, unsigned cb, uint32_t u32Value);
/** Pointer to a FNPCICONFIGWRITE() function. */
typedef FNPCIBRIDGECONFIGWRITE *PFNPCIBRIDGECONFIGWRITE;
/** Pointer to a PFNPCICONFIGWRITE. */
typedef PFNPCIBRIDGECONFIGWRITE *PPFNPCIBRIDGECONFIGWRITE;

/* Forward declaration */
struct DEVPCIBUS;

enum {
    /** Flag whether the device is a pci-to-pci bridge.
     * This is set prior to device registration.  */
    PCIDEV_FLAG_PCI_TO_PCI_BRIDGE  = RT_BIT_32(1),
    /** Flag whether the device is a PCI Express device.
     * This is set prior to device registration.  */
    PCIDEV_FLAG_PCI_EXPRESS_DEVICE = RT_BIT_32(2),
    /** Flag whether the device is capable of MSI.
     * This one is set by MsiInit().  */
    PCIDEV_FLAG_MSI_CAPABLE        = RT_BIT_32(3),
    /** Flag whether the device is capable of MSI-X.
     * This one is set by MsixInit().  */
    PCIDEV_FLAG_MSIX_CAPABLE       = RT_BIT_32(4),
    /** Flag if device represents real physical device in passthrough mode. */
    PCIDEV_FLAG_PASSTHROUGH        = RT_BIT_32(5),
    /** Flag whether the device is capable of MSI using 64-bit address.  */
    PCIDEV_FLAG_MSI64_CAPABLE      = RT_BIT_32(6)

};


/**
 * PDM PCI Device - Internal data.
 *
 * @sa PDMPCIDEV
 */
typedef struct PDMPCIDEVINT
{
    /** @name Owned by PDM.
     * @remarks The bus may use the device instance pointers.
     * @{
     */
    /** Pointer to the PDM device the PCI device belongs to. (R3 ptr)  */
    PPDMDEVINSR3                    pDevInsR3;
    /** The CFGM device configuration index (default, PciDev1..255).
     * This also works as the internal sub-device ordinal with MMIOEx.
     * @note Same value as idxSubDev, can therefore be removed later. */
    uint8_t                         idxDevCfg;
    /** Set if the it can be reassigned to a different PCI device number. */
    bool                            fReassignableDevNo;
    /** Set if the it can be reassigned to a different PCI function number. */
    bool                            fReassignableFunNo;
    /** Alignment padding - used by ICH9 for region swapping (DevVGA hack).   */
    uint8_t                         bPadding0;
    /** Index into the PDM internal bus array (PDM::aPciBuses). */
    uint8_t                         idxPdmBus;
    /** Set if this device has been registered. */
    bool                            fRegistered;
    /** Index into PDMDEVINSR3::apPciDevs (same as PDMPCIDEV::idxSubDev). */
    uint16_t                        idxSubDev;
    /** @} */

    /** @name Owned by the PCI Bus
     * @remarks PDM will not touch anything here (includes not relocating anything).
     * @{
     */
    /** Pointer to the PCI bus of the device. (R3 ptr) */
    R3PTRTYPE(struct DEVPCIBUS *)   pBusR3;
    /** Read config callback. */
    R3PTRTYPE(PFNPCICONFIGREAD)     pfnConfigRead;
    /** Write config callback. */
    R3PTRTYPE(PFNPCICONFIGWRITE)    pfnConfigWrite;
    /** Read config callback for PCI bridges to pass requests
     * to devices on another bus. */
    R3PTRTYPE(PFNPCIBRIDGECONFIGREAD) pfnBridgeConfigRead;
    /** Write config callback for PCI bridges to pass requests
     * to devices on another bus. */
    R3PTRTYPE(PFNPCIBRIDGECONFIGWRITE) pfnBridgeConfigWrite;

    /** Flags of this PCI device, see PCIDEV_FLAG_XXX constants. */
    uint32_t                        fFlags;
    /** Current state of the IRQ pin of the device. */
    int32_t                         uIrqPinState;

    /** Offset of MSI PCI capability in config space, or 0.
     * @todo fix non-standard naming.  */
    uint8_t                         u8MsiCapOffset;
    /** Size of MSI PCI capability in config space, or 0.
     * @todo fix non-standard naming.  */
    uint8_t                         u8MsiCapSize;
    /** Offset of MSI-X PCI capability in config space, or 0.
     * @todo fix non-standard naming.  */
    uint8_t                         u8MsixCapOffset;
    /** Size of MSI-X PCI capability in config space, or 0.
     * @todo fix non-standard naming.  */
    uint8_t                         u8MsixCapSize;
    /** Size of the MSI-X region. */
    uint16_t                        cbMsixRegion;
    /** Offset to the PBA for MSI-X.   */
    uint16_t                        offMsixPba;
    /** Add padding to align aIORegions to an 16 byte boundary. */
    uint8_t                         abPadding2[HC_ARCH_BITS == 32 ? 0x14 : 0x10];

    /** Pointer to bus specific data. (R3 ptr) */
    R3PTRTYPE(const void *)         pPciBusPtrR3;
    /** I/O regions. */
    PCIIOREGION                     aIORegions[VBOX_PCI_NUM_REGIONS];
    /** @}  */
} PDMPCIDEVINT;
AssertCompileMemberAlignment(PDMPCIDEVINT, aIORegions, 8);
AssertCompileSize(PDMPCIDEVINT, HC_ARCH_BITS == 32 ? 0x60 : 0x140);

/** Indicate that PDMPCIDEV::Int.s can be declared. */
#define PDMPCIDEVINT_DECLARED

/** @} */

#endif /* !VBOX_INCLUDED_vmm_pdmpcidevint_h */


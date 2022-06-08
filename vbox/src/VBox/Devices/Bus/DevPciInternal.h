/* $Id: DevPciInternal.h 64419 2016-10-25 15:35:56Z vboxsync $ */
/** @file
 * DevPCI - Common Internal Header.
 */

/*
 * Copyright (C) 2010-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___Bus_DevPciInternal_h___
#define ___Bus_DevPciInternal_h___

#ifndef PDMPCIDEV_INCLUDE_PRIVATE
# define PDMPCIDEV_INCLUDE_PRIVATE /* Hack to get pdmpcidevint.h included at the right point. */
#endif
#include <VBox/vmm/pdmdev.h>


/**
 * PCI bus instance (common to both).
 */
typedef struct DEVPCIBUS
{
    /** Bus number. */
    int32_t                 iBus;
    /** Number of bridges attached to the bus. */
    uint32_t                cBridges;
    /** Start device number - always zero (only for DevPCI source compat). */
    uint32_t                iDevSearch;
    /** Set if PIIX3 type. */
    uint32_t                fTypePiix3 : 1;
    /** Set if ICH9 type. */
    uint32_t                fTypeIch9: 1;
    /** Set if this is a pure bridge, i.e. not part of DEVPCIGLOBALS struct. */
    uint32_t                fPureBridge : 1;
    /** Reserved for future config flags. */
    uint32_t                uReservedConfigFlags : 29;

    /** R3 pointer to the device instance. */
    PPDMDEVINSR3            pDevInsR3;
    /** Pointer to the PCI R3  helpers. */
    PCPDMPCIHLPR3           pPciHlpR3;

    /** R0 pointer to the device instance. */
    PPDMDEVINSR0            pDevInsR0;
    /** Pointer to the PCI R0 helpers. */
    PCPDMPCIHLPR0           pPciHlpR0;

    /** RC pointer to the device instance. */
    PPDMDEVINSRC            pDevInsRC;
    /** Pointer to the PCI RC helpers. */
    PCPDMPCIHLPRC           pPciHlpRC;

    /** Array of bridges attached to the bus. */
    R3PTRTYPE(PPDMPCIDEV *) papBridgesR3;
#if HC_ARCH_BITS == 32
    uint32_t                au32Alignment1[5]; /**< Cache line align apDevices. */
#endif
    /** Array of PCI devices. We assume 32 slots, each with 8 functions. */
    R3PTRTYPE(PPDMPCIDEV)   apDevices[256];

    /** The PCI device for the PCI bridge. */
    PDMPCIDEV               PciDev;
} DEVPCIBUS;
/** Pointer to a PCI bus instance.   */
typedef DEVPCIBUS *PDEVPCIBUS;


/** @def DEVPCI_APIC_IRQ_PINS
 * Number of pins for interrupts if the APIC is used.
 */
#define DEVPCI_APIC_IRQ_PINS    8
/** @def DEVPCI_LEGACY_IRQ_PINS
 * Number of pins for interrupts (PIRQ#0...PIRQ#3).
 * @remarks Labling this "legacy" might be a bit off...
 */
#define DEVPCI_LEGACY_IRQ_PINS  4

/**
 * PIIX3 ISA bridge state.
 */
typedef struct PIIX3ISABRIDGE
{
    /** The PCI device of the bridge. */
    PDMPCIDEV dev;
} PIIX3ISABRIDGE;


/**
 * PCI Globals - This is the host-to-pci bridge and the root bus.
 *
 * @note Only used by the root bus, not the bridges.
 */
typedef struct DEVPCIROOT
{
    /** PCI bus which is attached to the host-to-PCI bridge.
     * @note This must come first so we can share more code with the bridges!  */
    DEVPCIBUS           PciBus;

    /** R3 pointer to the device instance. */
    PPDMDEVINSR3        pDevInsR3;
    /** R0 pointer to the device instance. */
    PPDMDEVINSR0        pDevInsR0;
    /** RC pointer to the device instance. */
    PPDMDEVINSRC        pDevInsRC;

    /** I/O APIC usage flag (always true of ICH9, see constructor). */
    bool                fUseIoApic;
    /** Reserved for future config flags. */
    bool                afFutureFlags[3];
    /** Physical address of PCI config space MMIO region. */
    uint64_t            u64PciConfigMMioAddress;
    /** Length of PCI config space MMIO region. */
    uint64_t            u64PciConfigMMioLength;

    /** I/O APIC irq levels */
    volatile uint32_t   auPciApicIrqLevels[DEVPCI_APIC_IRQ_PINS];
    /** Value latched in Configuration Address Port (0CF8h) */
    uint32_t            uConfigReg;
    /** Alignment padding.   */
    uint32_t            u32Alignment1;
    /** Members only used by the PIIX3 code variant. */
    struct
    {
        /** ACPI IRQ level */
        uint32_t            iAcpiIrqLevel;
        /** ACPI PIC IRQ */
        int32_t             iAcpiIrq;
        /** Irq levels for the four PCI Irqs.
         * These count how many devices asserted the IRQ line.  If greater 0 an IRQ
         * is sent to the guest.  If it drops to 0 the IRQ is deasserted.
         * @remarks Labling this "legacy" might be a bit off...
         */
        volatile uint32_t   auPciLegacyIrqLevels[DEVPCI_LEGACY_IRQ_PINS];
        /** ISA bridge state. */
        PIIX3ISABRIDGE      PIIX3State;
    } Piix3;

#if 1 /* Will be moved into the BIOS "soon". */
    /** Current bus number (?). */
    uint8_t             uPciBiosBus;
    uint8_t             abAlignment2[7];
    /** The next I/O port address which the PCI BIOS will use. */
    uint32_t            uPciBiosIo;
    /** The next MMIO address which the PCI BIOS will use. */
    uint32_t            uPciBiosMmio;
    /** The next 64-bit MMIO address which the PCI BIOS will use. */
    uint64_t            uPciBiosMmio64;
#endif

} DEVPCIROOT;
/** Pointer to PCI device globals. */
typedef DEVPCIROOT *PDEVPCIROOT;


#endif


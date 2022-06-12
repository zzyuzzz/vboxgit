/* $Id: buslogic.c 89272 2021-05-25 12:38:19Z vboxsync $ */
/** @file
 * BusLogic SCSI host adapter driver to boot from disks.
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

#include <stdint.h>
#include <string.h>
#include "biosint.h"
#include "ebda.h"
#include "inlines.h"
#include "pciutil.h"
#include "vds.h"
#include "scsi.h"

//#define DEBUG_BUSLOGIC 1
#if DEBUG_BUSLOGIC
# define DBG_BUSLOGIC(...)        BX_INFO(__VA_ARGS__)
#else
# define DBG_BUSLOGIC(...)
#endif

#define BUSLOGICCOMMAND_DISABLE_HOST_ADAPTER_INTERRUPT 0x25
#define BUSLOGICCOMMAND_EXECUTE_SCSI_COMMAND           0x83


#define RT_BIT(bit) (1 << (bit))

/** Register offsets in the I/O port space. */
#define BUSLOGIC_REGISTER_CONTROL   0 /**< Writeonly */
/** Fields for the control register. */
# define BL_CTRL_RSBUS  RT_BIT(4)   /* Reset SCSI Bus. */
# define BL_CTRL_RINT   RT_BIT(5)   /* Reset Interrupt. */
# define BL_CTRL_RSOFT  RT_BIT(6)   /* Soft Reset. */
# define BL_CTRL_RHARD  RT_BIT(7)   /* Hard Reset. */

#define BUSLOGIC_REGISTER_STATUS    0 /**< Readonly */
/** Fields for the status register. */
# define BL_STAT_CMDINV RT_BIT(0)   /* Command Invalid. */
# define BL_STAT_DIRRDY RT_BIT(2)   /* Data In Register Ready. */
# define BL_STAT_CPRBSY RT_BIT(3)   /* Command/Parameter Out Register Busy. */
# define BL_STAT_HARDY  RT_BIT(4)   /* Host Adapter Ready. */
# define BL_STAT_INREQ  RT_BIT(5)   /* Initialization Required. */
# define BL_STAT_DFAIL  RT_BIT(6)   /* Diagnostic Failure. */
# define BL_STAT_DACT   RT_BIT(7)   /* Diagnistic Active. */

#define BUSLOGIC_REGISTER_COMMAND   1 /**< Writeonly */
#define BUSLOGIC_REGISTER_DATAIN    1 /**< Readonly */
#define BUSLOGIC_REGISTER_INTERRUPT 2 /**< Readonly */
/** Fields for the interrupt register. */
# define BL_INTR_IMBL   RT_BIT(0)   /* Incoming Mailbox Loaded. */
# define BL_INTR_OMBR   RT_BIT(1)   /* Outgoing Mailbox Available. */
# define BL_INTR_CMDC   RT_BIT(2)   /* Command Complete. */
# define BL_INTR_RSTS   RT_BIT(3)   /* SCSI Bus Reset State. */
# define BL_INTR_INTV   RT_BIT(7)   /* Interrupt Valid. */

/**
 * The structure for the "Execute SCSI Command" command.
 */
typedef struct ESCMD
{
    /** Data length. */
    uint32_t        cbData;
    /** Data pointer. */
    uint32_t        u32PhysAddrData;
    /** The device the request is sent to. */
    uint8_t         uTargetId;
    /** The LUN in the device. */
    uint8_t         uLogicalUnit;
    /** Reserved */
    unsigned char   uReserved1 : 3;
    /** Data direction for the request. */
    unsigned char   uDataDirection : 2;
    /** Reserved */
    unsigned char   uReserved2 : 3;
    /** Length of the SCSI CDB. */
    uint8_t         cbCDB;
    /** The SCSI CDB.  (A CDB can be 12 bytes long.)   */
    uint8_t         abCDB[16];
} ESCMD, *PESCMD;

/**
 * BusLogic-SCSI controller data.
 */
typedef struct
{
    /** The execute SCSI command. */
    ESCMD            EsCmd;
    /** I/O base of device. */
    uint16_t         u16IoBase;
    /** The sink buf. */
    void __far       *pvSinkBuf;
    /** Size of the sink buffer in bytes. */
    uint16_t         cbSinkBuf;
} buslogic_t;

/* The BusLogic specific data must fit into 1KB (statically allocated). */
ct_assert(sizeof(buslogic_t) <= 1024);

/**
 * Converts a segment:offset pair into a 32bit physical address.
 */
static uint32_t buslogic_addr_to_phys(void __far *ptr)
{
    return ((uint32_t)FP_SEG(ptr) << 4) + FP_OFF(ptr);
}

static int buslogic_cmd(buslogic_t __far *buslogic, uint8_t uCmd, uint8_t __far *pbReq, uint16_t cbReq,
                        uint8_t __far *pbReply, uint16_t cbReply)
{
    uint16_t i;

    outb(buslogic->u16IoBase + BUSLOGIC_REGISTER_COMMAND, uCmd);
    for (i = 0; i < cbReq; i++)
        outb(buslogic->u16IoBase + BUSLOGIC_REGISTER_COMMAND, *pbReq++);

    /* Wait for the HBA to finish processing the command. */
    if (cbReply)
    {
        while (!(inb(buslogic->u16IoBase + BUSLOGIC_REGISTER_STATUS) & BL_STAT_DIRRDY));
        for (i = 0; i < cbReply; i++)
            *pbReply++ = inb(buslogic->u16IoBase + BUSLOGIC_REGISTER_DATAIN);
    }

    while (!(inb(buslogic->u16IoBase + BUSLOGIC_REGISTER_STATUS) & BL_STAT_HARDY));

    /* Clear interrupt status. */
    outb(buslogic->u16IoBase + BUSLOGIC_REGISTER_CONTROL, BL_CTRL_RINT);

    return 0;
}

int buslogic_scsi_cmd_data_out(void __far *pvHba, uint8_t idTgt, uint8_t __far *aCDB,
                               uint8_t cbCDB, uint8_t __far *buffer, uint32_t length)
{
    buslogic_t __far *buslogic = (buslogic_t __far *)pvHba;
    uint8_t abReply[4];
    int i;
    int rc;

    _fmemset(&buslogic->EsCmd, 0, sizeof(buslogic->EsCmd));
    _fmemset(abReply, 0, sizeof(abReply));

    buslogic->EsCmd.cbData = length;
    buslogic->EsCmd.u32PhysAddrData = buslogic_addr_to_phys(buffer);
    buslogic->EsCmd.uTargetId       = idTgt;
    buslogic->EsCmd.uLogicalUnit    = 0;
    buslogic->EsCmd.uDataDirection  = 0;
    buslogic->EsCmd.cbCDB           = cbCDB;

    for (i = 0; i < cbCDB; i++)
        buslogic->EsCmd.abCDB[i] = aCDB[i];

    rc = buslogic_cmd(buslogic, BUSLOGICCOMMAND_EXECUTE_SCSI_COMMAND, (uint8_t __far *)&buslogic->EsCmd,
                      sizeof(buslogic->EsCmd) - sizeof(buslogic->EsCmd.abCDB) + cbCDB, &abReply[0], sizeof(abReply));
    if (!rc)
        rc = abReply[2];

    return rc;
}

int buslogic_scsi_cmd_data_in(void __far *pvHba, uint8_t idTgt, uint8_t __far *aCDB,
                            uint8_t cbCDB, uint8_t __far *buffer, uint32_t length, uint16_t skip_b,
                            uint16_t skip_a)
{
    buslogic_t __far *buslogic = (buslogic_t __far *)pvHba;
    uint8_t abReply[4];
    int i;
    int rc;

    DBG_BUSLOGIC("buslogic_scsi_cmd_data_in:\n");

    if (   (   skip_b
            || skip_a)
        && skip_b + length + skip_a > buslogic->cbSinkBuf) /* Sink buffer is only 16KB at the moment. */
        return 1;

    _fmemset(&buslogic->EsCmd, 0, sizeof(buslogic->EsCmd));
    _fmemset(abReply, 0, sizeof(abReply));

    if (!skip_b && !skip_a)
    {
        buslogic->EsCmd.cbData = length;
        buslogic->EsCmd.u32PhysAddrData = buslogic_addr_to_phys(buffer);
    }
    else
    {
        buslogic->EsCmd.cbData = length + skip_b + skip_a;
        buslogic->EsCmd.u32PhysAddrData = buslogic_addr_to_phys(buslogic->pvSinkBuf); /* Requires the sink buffer because there is no S/G variant. */
    }
    buslogic->EsCmd.uTargetId       = idTgt;
    buslogic->EsCmd.uLogicalUnit    = 0;
    buslogic->EsCmd.uDataDirection  = 0;
    buslogic->EsCmd.cbCDB           = cbCDB;

    for (i = 0; i < cbCDB; i++)
        buslogic->EsCmd.abCDB[i] = aCDB[i];

    rc = buslogic_cmd(buslogic, BUSLOGICCOMMAND_EXECUTE_SCSI_COMMAND, (uint8_t __far *)&buslogic->EsCmd,
                      sizeof(buslogic->EsCmd) - sizeof(buslogic->EsCmd.abCDB) + cbCDB, &abReply[0], sizeof(abReply));
    if (!rc)
    {
        /* Copy the data over from the sink buffer. */
        if (abReply[2] == 0)
        {
            if (skip_b || skip_a)
                _fmemcpy(buffer, (const uint8_t __far *)buslogic->pvSinkBuf + skip_b, length);
        }
        else
            rc = abReply[2];
    }

    return rc;
}

/**
 * Initializes the BusLogic SCSI HBA and detects attached devices.
 */
static int buslogic_scsi_hba_init(buslogic_t __far *buslogic)
{
    /* Hard reset. */
    outb(buslogic->u16IoBase + BUSLOGIC_REGISTER_CONTROL, BL_CTRL_RHARD);
    while (!(inb(buslogic->u16IoBase + BUSLOGIC_REGISTER_STATUS) & BL_STAT_HARDY));

    return 0;
}

/**
 * Init the BusLogic SCSI driver and detect attached disks.
 */
int buslogic_scsi_init(void __far *pvHba, void __far *pvSinkBuf, uint16_t cbSinkBuf, uint8_t u8Bus, uint8_t u8DevFn)
{
    buslogic_t __far *buslogic = (buslogic_t __far *)pvHba;
    uint32_t u32Bar;

    DBG_BUSLOGIC("BusLogic SCSI HBA at Bus %u DevFn 0x%x (raw 0x%x)\n", u8Bus, u8DevFn);

    u32Bar = pci_read_config_dword(u8Bus, u8DevFn, 0x10);

    DBG_BUSLOGIC("BAR at 0x10 : 0x%x\n", u32Bar);

    if ((u32Bar & 0x01) != 0)
    {
        uint16_t u16IoBase = (u32Bar & 0xfff0);

        /* Enable PCI memory, I/O, bus mastering access in command register. */
        pci_write_config_word(u8Bus, u8DevFn, 4, 0x7);

        DBG_BUSLOGIC("I/O base: 0x%x\n", u16IoBase);
        buslogic->u16IoBase = u16IoBase;
        buslogic->pvSinkBuf = pvSinkBuf;
        buslogic->cbSinkBuf = cbSinkBuf;
        return buslogic_scsi_hba_init(buslogic);
    }
    else
        DBG_BUSLOGIC("BAR is MMIO\n");

    return 1;
}

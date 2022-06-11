/* $Id: DevSerial.cpp 81889 2019-11-15 22:44:32Z vboxsync $ */
/** @file
 * DevSerial - 16550A UART emulation.
 *
 * The documentation for this device was taken from the PC16550D spec from TI.
 */

/*
 * Copyright (C) 2018-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_SERIAL
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmserialifs.h>
#include <iprt/assert.h>
#include <iprt/uuid.h>
#include <iprt/string.h>
#include <iprt/semaphore.h>
#include <iprt/critsect.h>

#include "VBoxDD.h"
#include "UartCore.h"


/*********************************************************************************************************************************
*   Structures and Typedefs                                                                                                      *
*********************************************************************************************************************************/

/**
 * Serial device.
 */
typedef struct DEVSERIAL
{
    /** Pointer to the device instance - R3 Ptr. */
    PPDMDEVINSR3                    pDevInsR3;
    /** Pointer to the device instance - R0 Ptr. */
    PPDMDEVINSR0                    pDevInsR0;
    /** Pointer to the device instance - RC Ptr. */
    PPDMDEVINSRC                    pDevInsRC;
    /** The IRQ value. */
    uint8_t                         uIrq;
    uint8_t                         bAlignment;
    /** The base I/O port the device is registered at. */
    RTIOPORT                        PortBase;
    /** The I/O ports registration. */
    IOMIOPORTHANDLE                 hIoPorts;

    /** The UART core. */
    UARTCORE                        UartCore;
} DEVSERIAL;
/** Pointer to the serial device state. */
typedef DEVSERIAL *PDEVSERIAL;

#ifndef VBOX_DEVICE_STRUCT_TESTCASE



PDMBOTHCBDECL(void) serialIrqReq(PPDMDEVINS pDevIns, PUARTCORE pUart, unsigned iLUN, int iLvl)
{
    RT_NOREF(pUart, iLUN);
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    PDMDevHlpISASetIrqNoWait(pDevIns, pThis->uIrq, iLvl);
}


/* -=-=-=-=-=-=-=-=- I/O Port Access Handlers -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNIOMIOPORTNEWOUT}
 */
static DECLCALLBACK(VBOXSTRICTRC)
serialIoPortWrite(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t u32, unsigned cb)
{
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    RT_NOREF_PV(pvUser);

    return uartRegWrite(&pThis->UartCore, offPort, u32, cb);
}


/**
 * @callback_method_impl{FNIOMIOPORTNEWIN}
 */
static DECLCALLBACK(VBOXSTRICTRC)
serialIoPortRead(PPDMDEVINS pDevIns, void *pvUser, RTIOPORT offPort, uint32_t *pu32, unsigned cb)
{
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    RT_NOREF_PV(pvUser);

    return uartRegRead(&pThis->UartCore, offPort, pu32, cb);
}


#ifdef IN_RING3


/**
 * Returns the matching UART type from the given string.
 *
 * @returns UART type based on the given string or UARTTYPE_INVALID if an invalid type was passed.
 * @param   pszUartType         The UART type.
 */
static UARTTYPE serialR3GetUartTypeFromString(const char *pszUartType)
{
    if (!RTStrCmp(pszUartType, "16450"))
        return UARTTYPE_16450;
    else if (!RTStrCmp(pszUartType, "16550A"))
        return UARTTYPE_16550A;
    else if (!RTStrCmp(pszUartType, "16750"))
        return UARTTYPE_16750;

    AssertLogRelMsgFailedReturn(("Unknown UART type \"%s\" specified", pszUartType),
                                UARTTYPE_INVALID);
}


/* -=-=-=-=-=-=-=-=- Saved State -=-=-=-=-=-=-=-=- */

/**
 * @callback_method_impl{FNSSMDEVLIVEEXEC}
 */
static DECLCALLBACK(int) serialR3LiveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uPass)
{
    PDEVSERIAL      pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    PCPDMDEVHLPR3   pHlp  = pDevIns->pHlpR3;
    RT_NOREF(uPass);

    pHlp->pfnSSMPutU8(pSSM, pThis->uIrq);
    pHlp->pfnSSMPutIOPort(pSSM, pThis->PortBase);
    pHlp->pfnSSMPutU32(pSSM, pThis->UartCore.enmType);

    return VINF_SSM_DONT_CALL_AGAIN;
}


/**
 * @callback_method_impl{FNSSMDEVSAVEEXEC}
 */
static DECLCALLBACK(int) serialR3SaveExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVSERIAL      pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    PCPDMDEVHLPR3   pHlp  = pDevIns->pHlpR3;

    pHlp->pfnSSMPutU8(    pSSM, pThis->uIrq);
    pHlp->pfnSSMPutIOPort(pSSM, pThis->PortBase);
    pHlp->pfnSSMPutU32(   pSSM, pThis->UartCore.enmType);

    uartR3SaveExec(&pThis->UartCore, pSSM);
    return pHlp->pfnSSMPutU32(pSSM, UINT32_MAX); /* sanity/terminator */
}


/**
 * @callback_method_impl{FNSSMDEVLOADEXEC}
 */
static DECLCALLBACK(int) serialR3LoadExec(PPDMDEVINS pDevIns, PSSMHANDLE pSSM, uint32_t uVersion, uint32_t uPass)
{
    PDEVSERIAL      pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    PCPDMDEVHLPR3   pHlp  = pDevIns->pHlpR3;
    uint8_t         bIrq;
    RTIOPORT        PortBase;
    UARTTYPE        enmType;
    int rc;

    AssertMsgReturn(uVersion >= UART_SAVED_STATE_VERSION_16450, ("%d\n", uVersion), VERR_SSM_UNSUPPORTED_DATA_UNIT_VERSION);
    if (uVersion > UART_SAVED_STATE_VERSION_LEGACY_CODE)
    {
        pHlp->pfnSSMGetU8(    pSSM, &bIrq);
        pHlp->pfnSSMGetIOPort(pSSM, &PortBase);
        PDMDEVHLP_SSM_GET_ENUM32_RET(pHlp, pSSM, enmType, UARTTYPE);
        if (uPass == SSM_PASS_FINAL)
        {
            rc = uartR3LoadExec(&pThis->UartCore, pSSM, uVersion, uPass, NULL, NULL);
            AssertRCReturn(rc, rc);
        }
    }
    else
    {
        enmType = uVersion > UART_SAVED_STATE_VERSION_16450 ? UARTTYPE_16550A : UARTTYPE_16450;
        if (uPass != SSM_PASS_FINAL)
        {
            int32_t iIrqTmp;
            pHlp->pfnSSMGetS32(pSSM, &iIrqTmp);
            uint32_t uPortBaseTmp;
            rc = pHlp->pfnSSMGetU32(pSSM, &uPortBaseTmp);
            AssertRCReturn(rc, rc);

            bIrq     = (uint8_t)iIrqTmp;
            PortBase = (uint32_t)uPortBaseTmp;
        }
        else
        {
            rc = uartR3LoadExec(&pThis->UartCore, pSSM, uVersion, uPass, &bIrq, &PortBase);
            AssertRCReturn(rc, rc);
        }
    }

    if (uPass == SSM_PASS_FINAL)
    {
        /* The marker. */
        uint32_t u32;
        rc = pHlp->pfnSSMGetU32(pSSM, &u32);
        AssertRCReturn(rc, rc);
        AssertMsgReturn(u32 == UINT32_MAX, ("%#x\n", u32), VERR_SSM_DATA_UNIT_FORMAT_CHANGED);
    }

    /*
     * Check the config.
     */
    if (    pThis->uIrq     != bIrq
        ||  pThis->PortBase != PortBase
        ||  pThis->UartCore.enmType != enmType)
        return pHlp->pfnSSMSetCfgError(pSSM, RT_SRC_POS,
                                       N_("Config mismatch - saved IRQ=%#x PortBase=%#x Type=%d; configured IRQ=%#x PortBase=%#x Type=%d"),
                                       bIrq, PortBase, enmType, pThis->uIrq, pThis->PortBase, pThis->UartCore.enmType);

    return VINF_SUCCESS;
}


/**
 * @callback_method_impl{FNSSMDEVLOADDONE}
 */
static DECLCALLBACK(int) serialR3LoadDone(PPDMDEVINS pDevIns, PSSMHANDLE pSSM)
{
    PDEVSERIAL  pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    return uartR3LoadDone(&pThis->UartCore, pSSM);
}


/* -=-=-=-=-=-=-=-=- PDMDEVREG -=-=-=-=-=-=-=-=- */

/**
 * @interface_method_impl{PDMDEVREG,pfnRelocate}
 */
static DECLCALLBACK(void) serialR3Relocate(PPDMDEVINS pDevIns, RTGCINTPTR offDelta)
{
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    uartR3Relocate(&pThis->UartCore, offDelta);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnReset}
 */
static DECLCALLBACK(void) serialR3Reset(PPDMDEVINS pDevIns)
{
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    uartR3Reset(&pThis->UartCore);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnAttach}
 */
static DECLCALLBACK(int) serialR3Attach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    RT_NOREF(fFlags);
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    return uartR3Attach(&pThis->UartCore, iLUN);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDetach}
 */
static DECLCALLBACK(void) serialR3Detach(PPDMDEVINS pDevIns, unsigned iLUN, uint32_t fFlags)
{
    RT_NOREF(iLUN, fFlags);
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    uartR3Detach(&pThis->UartCore);
}


/**
 * @interface_method_impl{PDMDEVREG,pfnDestruct}
 */
static DECLCALLBACK(int) serialR3Destruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN_QUIET(pDevIns);
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);

    uartR3Destruct(&pThis->UartCore);
    return VINF_SUCCESS;
}


/**
 * @interface_method_impl{PDMDEVREG,pfnConstruct}
 */
static DECLCALLBACK(int) serialR3Construct(PPDMDEVINS pDevIns, int iInstance, PCFGMNODE pCfg)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    PDEVSERIAL      pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);
    PCPDMDEVHLPR3   pHlp  = pDevIns->pHlpR3;
    int             rc;

    Assert(iInstance < 4);

    /*
     * Initialize the instance data.
     * (Do this early or the destructor might choke on something!)
     */
    pThis->pDevInsR3 = pDevIns;
    pThis->pDevInsR0 = PDMDEVINS_2_R0PTR(pDevIns);
    pThis->pDevInsRC = PDMDEVINS_2_RCPTR(pDevIns);

    /*
     * Validate and read the configuration.
     */
    PDMDEV_VALIDATE_CONFIG_RETURN(pDevIns, "IRQ|IOBase|YieldOnLSRRead|UartType", "");

    bool fYieldOnLSRRead = false;
    rc = pHlp->pfnCFGMQueryBoolDef(pCfg, "YieldOnLSRRead", &fYieldOnLSRRead, false);
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Failed to get the \"YieldOnLSRRead\" value"));

    uint8_t uIrq = 0;
    rc = pHlp->pfnCFGMQueryU8(pCfg, "IRQ", &uIrq);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        /* Provide sensible defaults. */
        if (iInstance == 0)
            uIrq = 4;
        else if (iInstance == 1)
            uIrq = 3;
        else
            AssertReleaseFailed(); /* irq_lvl is undefined. */
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: Failed to get the \"IRQ\" value"));

    uint16_t uIoBase = 0;
    rc = pHlp->pfnCFGMQueryU16(pCfg, "IOBase", &uIoBase);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND)
    {
        if (iInstance == 0)
            uIoBase = 0x3f8;
        else if (iInstance == 1)
            uIoBase = 0x2f8;
        else
            AssertReleaseFailed(); /* uIoBase is undefined */
    }
    else if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc,
                                N_("Configuration error: Failed to get the \"IOBase\" value"));

    char szUartType[32];
    rc = pHlp->pfnCFGMQueryStringDef(pCfg, "UartType", szUartType, sizeof(szUartType), "16550A");
    if (RT_FAILURE(rc))
        return PDMDEV_SET_ERROR(pDevIns, rc, N_("Configuration error: failed to read \"UartType\" as string"));

    UARTTYPE enmUartType = serialR3GetUartTypeFromString(szUartType);
    if (enmUartType != UARTTYPE_INVALID)
        LogRel(("Serial#%d: emulating %s (IOBase: %04x IRQ: %u)\n", pDevIns->iInstance, szUartType, uIoBase, uIrq));
    else
        return PDMDevHlpVMSetError(pDevIns, VERR_INVALID_PARAMETER, RT_SRC_POS,
                                   N_("Configuration error: Invalid \"UartType\" type value: %s"), szUartType);

    pThis->uIrq     = uIrq;
    pThis->PortBase = uIoBase;

    /*
     * Init locks, using explicit locking where necessary.
     */
    rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    /*
     * Register the I/O ports.
     */
    rc = PDMDevHlpIoPortCreateAndMap(pDevIns, uIoBase, 8 /*cPorts*/, serialIoPortWrite, serialIoPortRead,
                                     "SERIAL", NULL /*paExtDescs*/, &pThis->hIoPorts);
    AssertRCReturn(rc, rc);

    PVM pVM = PDMDevHlpGetVM(pDevIns);
    RTR0PTR pfnSerialIrqReqR0 = NIL_RTR0PTR;
    RTRCPTR pfnSerialIrqReqRC = NIL_RTRCPTR;

#ifdef VBOX_WITH_RAW_MODE_KEEP
    if (pDevIns->fRCEnabled)
    {
        if (VM_IS_RAW_MODE_ENABLED(pVM)) /** @todo this dynamic symbol resolving will be reworked later! */
            rc = PDMR3LdrGetSymbolRC(pVM, pDevIns->pReg->pszRCMod, "serialIrqReq", &pfnSerialIrqReqRC);
        if (RT_FAILURE(rc))
            return rc;
    }
#endif

    if (pDevIns->fR0Enabled)
    {
        /** @todo this dynamic symbol resolving will be reworked later! */
        rc = PDMR3LdrGetSymbolR0(pVM, pDevIns->pReg->pszR0Mod, "serialIrqReq", &pfnSerialIrqReqR0);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Saved state.
     */
    rc = PDMDevHlpSSMRegisterEx(pDevIns, UART_SAVED_STATE_VERSION, sizeof(*pThis), NULL,
                                NULL, serialR3LiveExec, NULL,
                                NULL, serialR3SaveExec, NULL,
                                NULL, serialR3LoadExec, serialR3LoadDone);
    AssertRCReturn(rc, rc);

    /* Init the UART core structure. */
    rc = uartR3Init(&pThis->UartCore, pDevIns, enmUartType, 0,
                    fYieldOnLSRRead ? UART_CORE_YIELD_ON_LSR_READ : 0, serialIrqReq, pfnSerialIrqReqR0, pfnSerialIrqReqRC);
    AssertRCReturn(rc, rc);

    serialR3Reset(pDevIns);
    return VINF_SUCCESS;
}

#else  /* !IN_RING3 */

/**
 * @callback_method_impl{PDMDEVREGR0,pfnConstruct}
 */
static DECLCALLBACK(int) serialRZConstruct(PPDMDEVINS pDevIns)
{
    PDMDEV_CHECK_VERSIONS_RETURN(pDevIns);
    PDEVSERIAL pThis = PDMDEVINS_2_DATA(pDevIns, PDEVSERIAL);

    int rc = PDMDevHlpSetDeviceCritSect(pDevIns, PDMDevHlpCritSectGetNop(pDevIns));
    AssertRCReturn(rc, rc);

    rc = PDMDevHlpIoPortSetUpContext(pDevIns, pThis->hIoPorts, serialIoPortWrite, serialIoPortRead, NULL /*pvUser*/);
    AssertRCReturn(rc, rc);

    return VINF_SUCCESS;
}

#endif /* !IN_RING3 */

/**
 * The device registration structure.
 */
const PDMDEVREG g_DeviceSerialPort =
{
    /* .u32Version = */             PDM_DEVREG_VERSION,
    /* .uReserved0 = */             0,
    /* .szName = */                 "serial",
    /* .fFlags = */                 PDM_DEVREG_FLAGS_DEFAULT_BITS | PDM_DEVREG_FLAGS_RZ | PDM_DEVREG_FLAGS_NEW_STYLE,
    /* .fClass = */                 PDM_DEVREG_CLASS_SERIAL,
    /* .cMaxInstances = */          UINT32_MAX,
    /* .uSharedVersion = */         42,
    /* .cbInstanceShared = */       sizeof(DEVSERIAL),
    /* .cbInstanceCC = */           0,
    /* .cbInstanceRC = */           0,
    /* .cMaxPciDevices = */         0,
    /* .cMaxMsixVectors = */        0,
    /* .pszDescription = */         "Serial Communication Port",
#if defined(IN_RING3)
    /* .pszRCMod = */               "VBoxDDRC.rc",
    /* .pszR0Mod = */               "VBoxDDR0.r0",
    /* .pfnConstruct = */           serialR3Construct,
    /* .pfnDestruct = */            serialR3Destruct,
    /* .pfnRelocate = */            serialR3Relocate,
    /* .pfnMemSetup = */            NULL,
    /* .pfnPowerOn = */             NULL,
    /* .pfnReset = */               serialR3Reset,
    /* .pfnSuspend = */             NULL,
    /* .pfnResume = */              NULL,
    /* .pfnAttach = */              serialR3Attach,
    /* .pfnDetach = */              serialR3Detach,
    /* .pfnQueryInterface = */      NULL,
    /* .pfnInitComplete = */        NULL,
    /* .pfnPowerOff = */            NULL,
    /* .pfnSoftReset = */           NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RING0)
    /* .pfnEarlyConstruct = */      NULL,
    /* .pfnConstruct = */           serialRZConstruct,
    /* .pfnDestruct = */            NULL,
    /* .pfnFinalDestruct = */       NULL,
    /* .pfnRequest = */             NULL,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#elif defined(IN_RC)
    /* .pfnConstruct = */           serialRZConstruct,
    /* .pfnReserved0 = */           NULL,
    /* .pfnReserved1 = */           NULL,
    /* .pfnReserved2 = */           NULL,
    /* .pfnReserved3 = */           NULL,
    /* .pfnReserved4 = */           NULL,
    /* .pfnReserved5 = */           NULL,
    /* .pfnReserved6 = */           NULL,
    /* .pfnReserved7 = */           NULL,
#else
# error "Not in IN_RING3, IN_RING0 or IN_RC!"
#endif
    /* .u32VersionEnd = */          PDM_DEVREG_VERSION
};

#endif /* !VBOX_DEVICE_STRUCT_TESTCASE */


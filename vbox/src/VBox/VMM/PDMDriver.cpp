/* $Id: PDMDriver.cpp 34207 2010-11-19 15:31:50Z vboxsync $ */
/** @file
 * PDM - Pluggable Device and Driver Manager, Driver parts.
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
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
#define LOG_GROUP LOG_GROUP_PDM_DRIVER
#include "PDMInternal.h"
#include <VBox/pdm.h>
#include <VBox/mm.h>
#include <VBox/cfgm.h>
#include <VBox/vmm.h>
#include <VBox/sup.h>
#include <VBox/vm.h>
#include <VBox/version.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/asm.h>
#include <iprt/ctype.h>
#include <iprt/mem.h>
#include <iprt/thread.h>
#include <iprt/path.h>
#include <iprt/string.h>


/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Internal callback structure pointer.
 *
 * The main purpose is to define the extra data we associate
 * with PDMDRVREGCB so we can find the VM instance and so on.
 */
typedef struct PDMDRVREGCBINT
{
    /** The callback structure. */
    PDMDRVREGCB     Core;
    /** A bit of padding. */
    uint32_t        u32[4];
    /** VM Handle. */
    PVM             pVM;
} PDMDRVREGCBINT, *PPDMDRVREGCBINT;
typedef const PDMDRVREGCBINT *PCPDMDRVREGCBINT;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static DECLCALLBACK(int) pdmR3DrvRegister(PCPDMDRVREGCB pCallbacks, PCPDMDRVREG pReg);
static int pdmR3DrvLoad(PVM pVM, PPDMDRVREGCBINT pRegCB, const char *pszFilename, const char *pszName);



/**
 * Register external drivers
 *
 * @returns VBox status code.
 * @param   pVM         The VM to operate on.
 * @param   pfnCallback Driver registration callback
 */
VMMR3DECL(int) PDMR3RegisterDrivers(PVM pVM, FNPDMVBOXDRIVERSREGISTER pfnCallback)
{
    /*
     * The registration callbacks.
     */
    PDMDRVREGCBINT  RegCB;
    RegCB.Core.u32Version   = PDM_DRVREG_CB_VERSION;
    RegCB.Core.pfnRegister  = pdmR3DrvRegister;
    RegCB.pVM               = pVM;

    int rc = pfnCallback(&RegCB.Core, VBOX_VERSION);
    if (RT_FAILURE(rc))
        AssertMsgFailed(("VBoxDriversRegister failed with rc=%Rrc\n"));

    return rc;
}

/**
 * This function will initialize the drivers for this VM instance.
 *
 * First of all this mean loading the builtin drivers and letting them
 * register themselves. Beyond that any additional driver modules are
 * loaded and called for registration.
 *
 * @returns VBox status code.
 * @param   pVM     VM Handle.
 */
int pdmR3DrvInit(PVM pVM)
{
    LogFlow(("pdmR3DrvInit:\n"));

    AssertRelease(!(RT_OFFSETOF(PDMDRVINS, achInstanceData) & 15));
    PPDMDRVINS pDrvInsAssert; NOREF(pDrvInsAssert);
    AssertCompile(sizeof(pDrvInsAssert->Internal.s) <= sizeof(pDrvInsAssert->Internal.padding));
    AssertRelease(sizeof(pDrvInsAssert->Internal.s) <= sizeof(pDrvInsAssert->Internal.padding));

    /*
     * The registration callbacks.
     */
    PDMDRVREGCBINT  RegCB;
    RegCB.Core.u32Version   = PDM_DRVREG_CB_VERSION;
    RegCB.Core.pfnRegister  = pdmR3DrvRegister;
    RegCB.pVM               = pVM;

    /*
     * Load the builtin module
     */
    PCFGMNODE pDriversNode = CFGMR3GetChild(CFGMR3GetRoot(pVM), "PDM/Drivers");
    bool fLoadBuiltin;
    int rc = CFGMR3QueryBool(pDriversNode, "LoadBuiltin", &fLoadBuiltin);
    if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
        fLoadBuiltin = true;
    else if (RT_FAILURE(rc))
    {
        AssertMsgFailed(("Configuration error: Querying boolean \"LoadBuiltin\" failed with %Rrc\n", rc));
        return rc;
    }
    if (fLoadBuiltin)
    {
        /* make filename */
        char *pszFilename = pdmR3FileR3("VBoxDD", /*fShared=*/true);
        if (!pszFilename)
            return VERR_NO_TMP_MEMORY;
        rc = pdmR3DrvLoad(pVM, &RegCB, pszFilename, "VBoxDD");
        RTMemTmpFree(pszFilename);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Load additional driver modules.
     */
    for (PCFGMNODE pCur = CFGMR3GetFirstChild(pDriversNode); pCur; pCur = CFGMR3GetNextChild(pCur))
    {
        /*
         * Get the name and path.
         */
        char szName[PDMMOD_NAME_LEN];
        rc = CFGMR3GetName(pCur, &szName[0], sizeof(szName));
        if (rc == VERR_CFGM_NOT_ENOUGH_SPACE)
        {
            AssertMsgFailed(("configuration error: The module name is too long, cchName=%zu.\n", CFGMR3GetNameLen(pCur)));
            return VERR_PDM_MODULE_NAME_TOO_LONG;
        }
        else if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("CFGMR3GetName -> %Rrc.\n", rc));
            return rc;
        }

        /* the path is optional, if no path the module name + path is used. */
        char szFilename[RTPATH_MAX];
        rc = CFGMR3QueryString(pCur, "Path", &szFilename[0], sizeof(szFilename));
        if (rc == VERR_CFGM_VALUE_NOT_FOUND || rc == VERR_CFGM_NO_PARENT)
            strcpy(szFilename, szName);
        else if (RT_FAILURE(rc))
        {
            AssertMsgFailed(("configuration error: Failure to query the module path, rc=%Rrc.\n", rc));
            return rc;
        }

        /* prepend path? */
        if (!RTPathHavePath(szFilename))
        {
            char *psz = pdmR3FileR3(szFilename);
            if (!psz)
                return VERR_NO_TMP_MEMORY;
            size_t cch = strlen(psz) + 1;
            if (cch > sizeof(szFilename))
            {
                RTMemTmpFree(psz);
                AssertMsgFailed(("Filename too long! cch=%d '%s'\n", cch, psz));
                return VERR_FILENAME_TOO_LONG;
            }
            memcpy(szFilename, psz, cch);
            RTMemTmpFree(psz);
        }

        /*
         * Load the module and register it's drivers.
         */
        rc = pdmR3DrvLoad(pVM, &RegCB, szFilename, szName);
        if (RT_FAILURE(rc))
            return rc;
    }

    LogFlow(("pdmR3DrvInit: returns VINF_SUCCESS\n"));
    return VINF_SUCCESS;
}


/**
 * Loads one driver module and call the registration entry point.
 *
 * @returns VBox status code.
 * @param   pVM             VM handle.
 * @param   pRegCB          The registration callback stuff.
 * @param   pszFilename     Module filename.
 * @param   pszName         Module name.
 */
static int pdmR3DrvLoad(PVM pVM, PPDMDRVREGCBINT pRegCB, const char *pszFilename, const char *pszName)
{
    /*
     * Load it.
     */
    int rc = pdmR3LoadR3U(pVM->pUVM, pszFilename, pszName);
    if (RT_SUCCESS(rc))
    {
        /*
         * Get the registration export and call it.
         */
        FNPDMVBOXDRIVERSREGISTER *pfnVBoxDriversRegister;
        rc = PDMR3LdrGetSymbolR3(pVM, pszName, "VBoxDriversRegister", (void **)&pfnVBoxDriversRegister);
        if (RT_SUCCESS(rc))
        {
            Log(("PDM: Calling VBoxDriversRegister (%p) of %s (%s)\n", pfnVBoxDriversRegister, pszName, pszFilename));
            rc = pfnVBoxDriversRegister(&pRegCB->Core, VBOX_VERSION);
            if (RT_SUCCESS(rc))
                Log(("PDM: Successfully loaded driver module %s (%s).\n", pszName, pszFilename));
            else
                AssertMsgFailed(("VBoxDriversRegister failed with rc=%Rrc\n"));
        }
        else
        {
            AssertMsgFailed(("Failed to locate 'VBoxDriversRegister' in %s (%s) rc=%Rrc\n", pszName, pszFilename, rc));
            if (rc == VERR_SYMBOL_NOT_FOUND)
                rc = VERR_PDM_NO_REGISTRATION_EXPORT;
        }
    }
    else
        AssertMsgFailed(("Failed to load %s (%s) rc=%Rrc!\n", pszName, pszFilename, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVREGCB,pfnRegister} */
static DECLCALLBACK(int) pdmR3DrvRegister(PCPDMDRVREGCB pCallbacks, PCPDMDRVREG pReg)
{
    /*
     * Validate the registration structure.
     */
    AssertPtrReturn(pReg, VERR_INVALID_POINTER);
    AssertMsgReturn(pReg->u32Version == PDM_DRVREG_VERSION,
                    ("%#x\n", pReg->u32Version),
                    VERR_PDM_UNKNOWN_DRVREG_VERSION);
    AssertReturn(pReg->szName[0], VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(RTStrEnd(pReg->szName, sizeof(pReg->szName)),
                    (".*s\n", sizeof(pReg->szName), pReg->szName),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(    !(pReg->fFlags & PDM_DRVREG_FLAGS_R0)
                    ||  (   pReg->szR0Mod[0]
                         && RTStrEnd(pReg->szR0Mod, sizeof(pReg->szR0Mod))),
                    ("%s: %.*s\n", pReg->szName, sizeof(pReg->szR0Mod), pReg->szR0Mod),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(    !(pReg->fFlags & PDM_DRVREG_FLAGS_RC)
                    ||  (   pReg->szRCMod[0]
                         && RTStrEnd(pReg->szRCMod, sizeof(pReg->szRCMod))),
                    ("%s: %.*s\n", pReg->szName, sizeof(pReg->szRCMod), pReg->szRCMod),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(VALID_PTR(pReg->pszDescription),
                    ("%s: %p\n", pReg->szName, pReg->pszDescription),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(!(pReg->fFlags & ~(PDM_DRVREG_FLAGS_HOST_BITS_MASK | PDM_DRVREG_FLAGS_R0 | PDM_DRVREG_FLAGS_RC)),
                    ("%s: %#x\n", pReg->szName, pReg->fFlags),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn((pReg->fFlags & PDM_DRVREG_FLAGS_HOST_BITS_MASK) == PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
                    ("%s: %#x\n", pReg->szName, pReg->fFlags),
                    VERR_PDM_INVALID_DRIVER_HOST_BITS);
    AssertMsgReturn(pReg->cMaxInstances > 0,
                    ("%s: %#x\n", pReg->szName, pReg->cMaxInstances),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(pReg->cbInstance <= _1M,
                    ("%s: %#x\n", pReg->szName, pReg->cbInstance),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(VALID_PTR(pReg->pfnConstruct),
                    ("%s: %p\n", pReg->szName, pReg->pfnConstruct),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(VALID_PTR(pReg->pfnRelocate) || !(pReg->fFlags & PDM_DRVREG_FLAGS_RC),
                    ("%s: %#x\n", pReg->szName, pReg->cbInstance),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(pReg->pfnSoftReset == NULL,
                    ("%s: %p\n", pReg->szName, pReg->pfnSoftReset),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);
    AssertMsgReturn(pReg->u32VersionEnd == PDM_DRVREG_VERSION,
                    ("%s: #x\n", pReg->szName, pReg->u32VersionEnd),
                    VERR_PDM_INVALID_DRIVER_REGISTRATION);

    /*
     * Check for duplicate and find FIFO entry at the same time.
     */
    PCPDMDRVREGCBINT pRegCB = (PCPDMDRVREGCBINT)pCallbacks;
    PPDMDRV pDrvPrev = NULL;
    PPDMDRV pDrv = pRegCB->pVM->pdm.s.pDrvs;
    for (; pDrv; pDrvPrev = pDrv, pDrv = pDrv->pNext)
    {
        if (!strcmp(pDrv->pReg->szName, pReg->szName))
        {
            AssertMsgFailed(("Driver '%s' already exists\n", pReg->szName));
            return VERR_PDM_DRIVER_NAME_CLASH;
        }
    }

    /*
     * Allocate new driver structure and insert it into the list.
     */
    pDrv = (PPDMDRV)MMR3HeapAlloc(pRegCB->pVM, MM_TAG_PDM_DRIVER, sizeof(*pDrv));
    if (pDrv)
    {
        pDrv->pNext = NULL;
        pDrv->cInstances = 0;
        pDrv->iNextInstance = 0;
        pDrv->pReg = pReg;

        if (pDrvPrev)
            pDrvPrev->pNext = pDrv;
        else
            pRegCB->pVM->pdm.s.pDrvs = pDrv;
        Log(("PDM: Registered driver '%s'\n", pReg->szName));
        return VINF_SUCCESS;
    }
    return VERR_NO_MEMORY;
}


/**
 * Lookups a driver structure by name.
 * @internal
 */
PPDMDRV pdmR3DrvLookup(PVM pVM, const char *pszName)
{
    for (PPDMDRV pDrv = pVM->pdm.s.pDrvs; pDrv; pDrv = pDrv->pNext)
        if (!strcmp(pDrv->pReg->szName, pszName))
            return pDrv;
    return NULL;
}


/**
 * Instantiate a driver.
 *
 * @returns VBox status code, including informational statuses.
 *
 * @param   pVM                 The VM handle.
 * @param   pNode               The CFGM node for the driver.
 * @param   pBaseInterface      The base interface.
 * @param   pDrvAbove           The driver above it.  NULL if it's the top-most
 *                              driver.
 * @param   pLun                The LUN the driver is being attached to.  NULL
 *                              if we're instantiating a driver chain before
 *                              attaching it - untested.
 * @param   ppBaseInterface     Where to return the pointer to the base
 *                              interface of the newly created driver.
 *
 * @remarks Recursive calls to this function is normal as the drivers will
 *          attach to anything below them during the pfnContruct call.
 */
int pdmR3DrvInstantiate(PVM pVM, PCFGMNODE pNode, PPDMIBASE pBaseInterface, PPDMDRVINS pDrvAbove,
                        PPDMLUN pLun, PPDMIBASE *ppBaseInterface)
{
    Assert(!pDrvAbove || !pDrvAbove->Internal.s.pDown);
    Assert(!pDrvAbove || !pDrvAbove->pDownBase);

    Assert(pBaseInterface->pfnQueryInterface(pBaseInterface, PDMIBASE_IID) == pBaseInterface);

    /*
     * Find the driver.
     */
    char *pszName;
    int rc = CFGMR3QueryStringAlloc(pNode, "Driver", &pszName);
    if (RT_SUCCESS(rc))
    {
        PPDMDRV pDrv = pdmR3DrvLookup(pVM, pszName);
        MMR3HeapFree(pszName);
        if (    pDrv
            &&  pDrv->cInstances < pDrv->pReg->cMaxInstances)
        {
            /* config node */
            PCFGMNODE pConfigNode = CFGMR3GetChild(pNode, "Config");
            if (!pConfigNode)
                rc = CFGMR3InsertNode(pNode, "Config", &pConfigNode);
            if (RT_SUCCESS(rc))
            {
                CFGMR3SetRestrictedRoot(pConfigNode);

                /*
                 * Allocate the driver instance.
                 */
                size_t cb = RT_OFFSETOF(PDMDRVINS, achInstanceData[pDrv->pReg->cbInstance]);
                cb = RT_ALIGN_Z(cb, 16);
                bool const fHyperHeap = !!(pDrv->pReg->fFlags & (PDM_DRVREG_FLAGS_R0 | PDM_DRVREG_FLAGS_RC));
                PPDMDRVINS pNew;
                if (fHyperHeap)
                    rc = MMHyperAlloc(pVM, cb, 64, MM_TAG_PDM_DRIVER, (void **)&pNew);
                else
                    rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_DRIVER, cb, (void **)&pNew);
                if (pNew)
                {
                    /*
                     * Initialize the instance structure (declaration order).
                     */
                    pNew->u32Version                = PDM_DRVINS_VERSION;
                    pNew->Internal.s.pUp            = pDrvAbove ? pDrvAbove : NULL;
                    //pNew->Internal.s.pDown          = NULL;
                    pNew->Internal.s.pLun           = pLun;
                    pNew->Internal.s.pDrv           = pDrv;
                    pNew->Internal.s.pVMR3          = pVM;
                    pNew->Internal.s.pVMR0          = pDrv->pReg->fFlags & PDM_DRVREG_FLAGS_R0 ? pVM->pVMR0 : NIL_RTR0PTR;
                    pNew->Internal.s.pVMRC          = pDrv->pReg->fFlags & PDM_DRVREG_FLAGS_RC ? pVM->pVMRC : NIL_RTRCPTR;
                    //pNew->Internal.s.fDetaching     = false;
                    pNew->Internal.s.fVMSuspended   = true;
                    //pNew->Internal.s.fVMReset       = false;
                    pNew->Internal.s.fHyperHeap     = fHyperHeap;
                    //pNew->Internal.s.pfnAsyncNotify = NULL;
                    pNew->Internal.s.pCfgHandle     = pNode;
                    pNew->pReg                      = pDrv->pReg;
                    pNew->pCfg                      = pConfigNode;
                    pNew->iInstance                 = pDrv->iNextInstance;
                    pNew->pUpBase                   = pBaseInterface;
                    Assert(!pDrvAbove || pBaseInterface == &pDrvAbove->IBase);
                    //pNew->pDownBase                 = NULL;
                    //pNew->IBase.pfnQueryInterface   = NULL;
                    pNew->pHlpR3                    = &g_pdmR3DrvHlp;
                    pNew->pvInstanceDataR3          = &pNew->achInstanceData[0];
                    if (pDrv->pReg->fFlags & PDM_DRVREG_FLAGS_R0)
                    {
                        pNew->pvInstanceDataR0      = MMHyperR3ToR0(pVM, &pNew->achInstanceData[0]);
                        rc = PDMR3LdrGetSymbolR0(pVM, NULL, "g_pdmR0DrvHlp", &pNew->pHlpR0);
                        AssertReleaseRCReturn(rc, rc);

                    }
                    if (pDrv->pReg->fFlags & PDM_DRVREG_FLAGS_RC)
                    {
                        pNew->pvInstanceDataR0      = MMHyperR3ToRC(pVM, &pNew->achInstanceData[0]);
                        rc = PDMR3LdrGetSymbolRC(pVM, NULL, "g_pdmRCDrvHlp", &pNew->pHlpRC);
                        AssertReleaseRCReturn(rc, rc);
                    }

                    pDrv->iNextInstance++;
                    pDrv->cInstances++;

                    /*
                     * Link with it with the driver above / LUN.
                     */
                    if (pDrvAbove)
                    {
                        pDrvAbove->pDownBase        = &pNew->IBase;
                        pDrvAbove->Internal.s.pDown = pNew;
                    }
                    else if (pLun)
                        pLun->pTop                  = pNew;
                    if (pLun)
                        pLun->pBottom               = pNew;

                    /*
                     * Invoke the constructor.
                     */
                    rc = pDrv->pReg->pfnConstruct(pNew, pNew->pCfg, 0 /*fFlags*/);
                    if (RT_SUCCESS(rc))
                    {
                        AssertPtr(pNew->IBase.pfnQueryInterface);
                        Assert(pNew->IBase.pfnQueryInterface(&pNew->IBase, PDMIBASE_IID) == &pNew->IBase);

                        /* Success! */
                        *ppBaseInterface = &pNew->IBase;
                        if (pLun)
                            Log(("PDM: Attached driver %p:'%s'/%d to LUN#%d on device '%s'/%d, pDrvAbove=%p:'%s'/%d\n",
                                 pNew, pDrv->pReg->szName, pNew->iInstance,
                                 pLun->iLun,
                                 pLun->pDevIns ? pLun->pDevIns->pReg->szName : pLun->pUsbIns->pReg->szName,
                                 pLun->pDevIns ? pLun->pDevIns->iInstance    : pLun->pUsbIns->iInstance,
                                 pDrvAbove, pDrvAbove ? pDrvAbove->pReg->szName : "", pDrvAbove ? pDrvAbove->iInstance : UINT32_MAX));
                        else
                            Log(("PDM: Attached driver %p:'%s'/%d, pDrvAbove=%p:'%s'/%d\n",
                                 pNew, pDrv->pReg->szName, pNew->iInstance,
                                 pDrvAbove, pDrvAbove ? pDrvAbove->pReg->szName : "", pDrvAbove ? pDrvAbove->iInstance : UINT32_MAX));
                    }
                    else
                        pdmR3DrvDestroyChain(pNew, PDM_TACH_FLAGS_NO_CALLBACKS);
                }
                else
                {
                    AssertMsgFailed(("Failed to allocate %d bytes for instantiating driver '%s'\n", cb, pszName));
                    rc = VERR_NO_MEMORY;
                }
            }
            else
                AssertMsgFailed(("Failed to create Config node! rc=%Rrc\n", rc));
        }
        else if (pDrv)
        {
            AssertMsgFailed(("Too many instances of driver '%s', max is %u\n", pszName, pDrv->pReg->cMaxInstances));
            rc = VERR_PDM_TOO_MANY_DRIVER_INSTANCES;
        }
        else
        {
            AssertMsgFailed(("Driver '%s' wasn't found!\n", pszName));
            rc = VERR_PDM_DRIVER_NOT_FOUND;
        }
    }
    else
    {
        AssertMsgFailed(("Query for string value of \"Driver\" -> %Rrc\n", rc));
        if (rc == VERR_CFGM_VALUE_NOT_FOUND)
            rc = VERR_PDM_CFG_MISSING_DRIVER_NAME;
    }
    return rc;
}


/**
 * Detaches a driver from whatever it's attached to.
 * This will of course lead to the destruction of the driver and all drivers below it in the chain.
 *
 * @returns VINF_SUCCESS
 * @param   pDrvIns     The driver instance to detach.
 * @param   fFlags      Flags, combination of the PDMDEVATT_FLAGS_* \#defines.
 */
int pdmR3DrvDetach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvDetach: pDrvIns=%p '%s'/%d\n", pDrvIns, pDrvIns->pReg->szName, pDrvIns->iInstance));
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    /*
     * Check that we're not doing this recursively, that could have unwanted sideeffects!
     */
    if (pDrvIns->Internal.s.fDetaching)
    {
        AssertMsgFailed(("Recursive detach! '%s'/%d\n", pDrvIns->pReg->szName, pDrvIns->iInstance));
        return VINF_SUCCESS;           }

    /*
     * Check that we actually can detach this instance.
     * The requirement is that the driver/device above has a detach method.
     */
    if (pDrvIns->Internal.s.pUp
        ? !pDrvIns->Internal.s.pUp->pReg->pfnDetach
        : !pDrvIns->Internal.s.pLun->pDevIns->pReg->pfnDetach)
    {
        AssertMsgFailed(("Cannot detach driver instance because the driver/device above doesn't support it!\n"));
        return VERR_PDM_DRIVER_DETACH_NOT_POSSIBLE;
    }

    /*
     * Join paths with pdmR3DrvDestroyChain.
     */
    pdmR3DrvDestroyChain(pDrvIns, fFlags);
    return VINF_SUCCESS;
}


/**
 * Destroys a driver chain starting with the specified driver.
 *
 * This is used when unplugging a device at run time.
 *
 * @param   pDrvIns     Pointer to the driver instance to start with.
 * @param   fFlags      PDM_TACH_FLAGS_NOT_HOT_PLUG, PDM_TACH_FLAGS_NO_CALLBACKS
 *                      or 0.
 */
void pdmR3DrvDestroyChain(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PVM pVM = pDrvIns->Internal.s.pVMR3;
    VM_ASSERT_EMT(pVM);

    /*
     * Detach the bottommost driver until we've detached pDrvIns.
     */
    pDrvIns->Internal.s.fDetaching = true;
    PPDMDRVINS pCur;
    do
    {
        /* find the driver to detach. */
        pCur = pDrvIns;
        while (pCur->Internal.s.pDown)
            pCur = pCur->Internal.s.pDown;
        LogFlow(("pdmR3DrvDestroyChain: pCur=%p '%s'/%d\n", pCur, pCur->pReg->szName, pCur->iInstance));

        /*
         * Unlink it and notify parent.
         */
        pCur->Internal.s.fDetaching = true;

        PPDMLUN pLun = pCur->Internal.s.pLun;
        Assert(pLun->pBottom == pCur);
        pLun->pBottom = pCur->Internal.s.pUp;

        if (pCur->Internal.s.pUp)
        {
            /* driver parent */
            PPDMDRVINS pParent = pCur->Internal.s.pUp;
            pCur->Internal.s.pUp = NULL;
            pParent->Internal.s.pDown = NULL;

            if (!(fFlags & PDM_TACH_FLAGS_NO_CALLBACKS) && pParent->pReg->pfnDetach)
                pParent->pReg->pfnDetach(pParent, fFlags);

            pParent->pDownBase = NULL;
        }
        else
        {
            /* device parent */
            Assert(pLun->pTop == pCur);
            pLun->pTop = NULL;
            if (!(fFlags & PDM_TACH_FLAGS_NO_CALLBACKS) && pLun->pDevIns->pReg->pfnDetach)
                pLun->pDevIns->pReg->pfnDetach(pLun->pDevIns, pLun->iLun, fFlags);
        }

        /*
         * Call destructor.
         */
        pCur->pUpBase = NULL;
        if (pCur->pReg->pfnDestruct)
            pCur->pReg->pfnDestruct(pCur);
        pCur->Internal.s.pDrv->cInstances--;

        /*
         * Free all resources allocated by the driver.
         */
        /* Queues. */
        int rc = PDMR3QueueDestroyDriver(pVM, pCur);
        AssertRC(rc);

        /* Timers. */
        rc = TMR3TimerDestroyDriver(pVM, pCur);
        AssertRC(rc);

        /* SSM data units. */
        rc = SSMR3DeregisterDriver(pVM, pCur, NULL, 0);
        AssertRC(rc);

        /* PDM threads. */
        rc = pdmR3ThreadDestroyDriver(pVM, pCur);
        AssertRC(rc);

        /* Info handlers. */
        rc = DBGFR3InfoDeregisterDriver(pVM, pCur, NULL);
        AssertRC(rc);

        /* PDM critsects. */
        rc = pdmR3CritSectDeleteDriver(pVM, pCur);
        AssertRC(rc);

        /* Finally, the driver it self. */
        bool fHyperHeap = pCur->Internal.s.fHyperHeap;
        ASMMemFill32(pCur, RT_OFFSETOF(PDMDRVINS, achInstanceData[pCur->pReg->cbInstance]), 0xdeadd0d0);
        if (fHyperHeap)
            MMHyperFree(pVM, pCur);
        else
            MMR3HeapFree(pCur);

    } while (pCur != pDrvIns);
}




/** @name Driver Helpers
 * @{
 */

/** @interface_method_impl{PDMDRVHLP,pfnAttach} */
static DECLCALLBACK(int) pdmR3DrvHlp_Attach(PPDMDRVINS pDrvIns, uint32_t fFlags, PPDMIBASE *ppBaseInterface)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    PVM pVM = pDrvIns->Internal.s.pVMR3;
    VM_ASSERT_EMT(pVM);
    LogFlow(("pdmR3DrvHlp_Attach: caller='%s'/%d: fFlags=%#x\n", pDrvIns->pReg->szName, pDrvIns->iInstance, fFlags));
    Assert(!(fFlags & ~(PDM_TACH_FLAGS_NOT_HOT_PLUG)));

    /*
     * Check that there isn't anything attached already.
     */
    int rc;
    if (!pDrvIns->Internal.s.pDown)
    {
        Assert(pDrvIns->Internal.s.pLun->pBottom == pDrvIns);

        /*
         * Get the attached driver configuration.
         */
        PCFGMNODE pNode = CFGMR3GetChild(pDrvIns->Internal.s.pCfgHandle, "AttachedDriver");
        if (pNode)
            rc = pdmR3DrvInstantiate(pVM, pNode, &pDrvIns->IBase, pDrvIns, pDrvIns->Internal.s.pLun, ppBaseInterface);
        else
            rc = VERR_PDM_NO_ATTACHED_DRIVER;
    }
    else
    {
        AssertMsgFailed(("Already got a driver attached. The driver should keep track of such things!\n"));
        rc = VERR_PDM_DRIVER_ALREADY_ATTACHED;
    }

    LogFlow(("pdmR3DrvHlp_Attach: caller='%s'/%d: return %Rrc\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnDetach} */
static DECLCALLBACK(int) pdmR3DrvHlp_Detach(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_Detach: caller='%s'/%d: fFlags=%#x\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, fFlags));
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    /*
     * Anything attached?
     */
    int rc;
    if (pDrvIns->Internal.s.pDown)
        rc = pdmR3DrvDetach(pDrvIns->Internal.s.pDown, fFlags);
    else
    {
        AssertMsgFailed(("Nothing attached!\n"));
        rc = VERR_PDM_NO_DRIVER_ATTACHED;
    }

    LogFlow(("pdmR3DrvHlp_Detach: caller='%s'/%d: returns %Rrc\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnDetachSelf} */
static DECLCALLBACK(int) pdmR3DrvHlp_DetachSelf(PPDMDRVINS pDrvIns, uint32_t fFlags)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_DetachSelf: caller='%s'/%d: fFlags=%#x\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, fFlags));
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    int rc = pdmR3DrvDetach(pDrvIns, fFlags);

    LogFlow(("pdmR3DrvHlp_Detach: returns %Rrc\n", rc)); /* pDrvIns is freed by now. */
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnMountPrepare} */
static DECLCALLBACK(int) pdmR3DrvHlp_MountPrepare(PPDMDRVINS pDrvIns, const char *pszFilename, const char *pszCoreDriver)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_MountPrepare: caller='%s'/%d: pszFilename=%p:{%s} pszCoreDriver=%p:{%s}\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pszFilename, pszFilename, pszCoreDriver, pszCoreDriver));
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    /*
     * Do the caller have anything attached below itself?
     */
    if (pDrvIns->Internal.s.pDown)
    {
        AssertMsgFailed(("Cannot prepare a mount when something's attached to you!\n"));
        return VERR_PDM_DRIVER_ALREADY_ATTACHED;
    }

    /*
     * We're asked to prepare, so we'll start off by nuking the
     * attached configuration tree.
     */
    PCFGMNODE   pNode = CFGMR3GetChild(pDrvIns->Internal.s.pCfgHandle, "AttachedDriver");
    if (pNode)
        CFGMR3RemoveNode(pNode);

    /*
     * If there is no core driver, we'll have to probe for it.
     */
    if (!pszCoreDriver)
    {
        /** @todo implement image probing. */
        AssertReleaseMsgFailed(("Not implemented!\n"));
        return VERR_NOT_IMPLEMENTED;
    }

    /*
     * Construct the basic attached driver configuration.
     */
    int rc = CFGMR3InsertNode(pDrvIns->Internal.s.pCfgHandle, "AttachedDriver", &pNode);
    if (RT_SUCCESS(rc))
    {
        rc = CFGMR3InsertString(pNode, "Driver", pszCoreDriver);
        if (RT_SUCCESS(rc))
        {
            PCFGMNODE pCfg;
            rc = CFGMR3InsertNode(pNode, "Config", &pCfg);
            if (RT_SUCCESS(rc))
            {
                rc = CFGMR3InsertString(pCfg, "Path", pszFilename);
                if (RT_SUCCESS(rc))
                {
                    LogFlow(("pdmR3DrvHlp_MountPrepare: caller='%s'/%d: returns %Rrc (Driver=%s)\n",
                             pDrvIns->pReg->szName, pDrvIns->iInstance, rc, pszCoreDriver));
                    return rc;
                }
                else
                    AssertMsgFailed(("Path string insert failed, rc=%Rrc\n", rc));
            }
            else
                AssertMsgFailed(("Config node failed, rc=%Rrc\n", rc));
        }
        else
            AssertMsgFailed(("Driver string insert failed, rc=%Rrc\n", rc));
        CFGMR3RemoveNode(pNode);
    }
    else
        AssertMsgFailed(("AttachedDriver node insert failed, rc=%Rrc\n", rc));

    LogFlow(("pdmR3DrvHlp_MountPrepare: caller='%s'/%d: returns %Rrc\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnAssertEMT} */
static DECLCALLBACK(bool) pdmR3DrvHlp_AssertEMT(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    if (VM_IS_EMT(pDrvIns->Internal.s.pVMR3))
        return true;

    char szMsg[100];
    RTStrPrintf(szMsg, sizeof(szMsg), "AssertEMT '%s'/%d\n", pDrvIns->pReg->szName, pDrvIns->iInstance);
    RTAssertMsg1Weak(szMsg, iLine, pszFile, pszFunction);
    AssertBreakpoint();
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    return false;
}


/** @interface_method_impl{PDMDRVHLP,pfnAssertOther} */
static DECLCALLBACK(bool) pdmR3DrvHlp_AssertOther(PPDMDRVINS pDrvIns, const char *pszFile, unsigned iLine, const char *pszFunction)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    if (!VM_IS_EMT(pDrvIns->Internal.s.pVMR3))
        return true;

    char szMsg[100];
    RTStrPrintf(szMsg, sizeof(szMsg), "AssertOther '%s'/%d\n", pDrvIns->pReg->szName, pDrvIns->iInstance);
    RTAssertMsg1Weak(szMsg, iLine, pszFile, pszFunction);
    AssertBreakpoint();
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    return false;
}


/** @interface_method_impl{PDMDRVHLP,pfnVMSetError} */
static DECLCALLBACK(int) pdmR3DrvHlp_VMSetError(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, ...)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    va_list args;
    va_start(args, pszFormat);
    int rc2 = VMSetErrorV(pDrvIns->Internal.s.pVMR3, rc, RT_SRC_POS_ARGS, pszFormat, args); Assert(rc2 == rc); NOREF(rc2);
    va_end(args);
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnVMSetErrorV} */
static DECLCALLBACK(int) pdmR3DrvHlp_VMSetErrorV(PPDMDRVINS pDrvIns, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    int rc2 = VMSetErrorV(pDrvIns->Internal.s.pVMR3, rc, RT_SRC_POS_ARGS, pszFormat, va); Assert(rc2 == rc); NOREF(rc2);
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnVMSetRuntimeError} */
static DECLCALLBACK(int) pdmR3DrvHlp_VMSetRuntimeError(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, ...)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    va_list args;
    va_start(args, pszFormat);
    int rc = VMSetRuntimeErrorV(pDrvIns->Internal.s.pVMR3, fFlags, pszErrorId, pszFormat, args);
    va_end(args);
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnVMSetRuntimeErrorV} */
static DECLCALLBACK(int) pdmR3DrvHlp_VMSetRuntimeErrorV(PPDMDRVINS pDrvIns, uint32_t fFlags, const char *pszErrorId, const char *pszFormat, va_list va)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    int rc = VMSetRuntimeErrorV(pDrvIns->Internal.s.pVMR3, fFlags, pszErrorId, pszFormat, va);
    return rc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMState} */
static DECLCALLBACK(VMSTATE) pdmR3DrvHlp_VMState(PPDMDRVINS pDrvIns)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);

    VMSTATE enmVMState = VMR3GetState(pDrvIns->Internal.s.pVMR3);

    LogFlow(("pdmR3DrvHlp_VMState: caller='%s'/%d: returns %d (%s)\n", pDrvIns->pReg->szName, pDrvIns->iInstance,
             enmVMState, VMR3GetStateName(enmVMState)));
    return enmVMState;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnVMTeleportedAndNotFullyResumedYet} */
static DECLCALLBACK(bool) pdmR3DrvHlp_VMTeleportedAndNotFullyResumedYet(PPDMDRVINS pDrvIns)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);

    bool fRc = VMR3TeleportedAndNotFullyResumedYet(pDrvIns->Internal.s.pVMR3);

    LogFlow(("pdmR3DrvHlp_VMState: caller='%s'/%d: returns %RTbool)\n", pDrvIns->pReg->szName, pDrvIns->iInstance,
             fRc));
    return fRc;
}


/** @interface_method_impl{PDMDEVHLPR3,pfnGetSupDrvSession} */
static DECLCALLBACK(PSUPDRVSESSION) pdmR3DrvHlp_GetSupDrvSession(PPDMDRVINS pDrvIns)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);

    PSUPDRVSESSION pSession = pDrvIns->Internal.s.pVMR3->pSession;
    LogFlow(("pdmR3DrvHlp_GetSupDrvSession: caller='%s'/%d: returns %p)\n", pDrvIns->pReg->szName, pDrvIns->iInstance,
             pSession));
    return pSession;
}


/** @interface_method_impl{PDMDRVHLP,pfnQueueCreate} */
static DECLCALLBACK(int) pdmR3DrvHlp_QueueCreate(PPDMDRVINS pDrvIns, uint32_t cbItem, uint32_t cItems, uint32_t cMilliesInterval,
                                                 PFNPDMQUEUEDRV pfnCallback, const char *pszName, PPDMQUEUE *ppQueue)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_PDMQueueCreate: caller='%s'/%d: cbItem=%d cItems=%d cMilliesInterval=%d pfnCallback=%p pszName=%p:{%s} ppQueue=%p\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, cbItem, cItems, cMilliesInterval, pfnCallback, pszName, pszName, ppQueue, ppQueue));
    PVM pVM = pDrvIns->Internal.s.pVMR3;
    VM_ASSERT_EMT(pVM);

    if (pDrvIns->iInstance > 0)
    {
        pszName = MMR3HeapAPrintf(pVM, MM_TAG_PDM_DRIVER_DESC, "%s_%u", pszName, pDrvIns->iInstance);
        AssertLogRelReturn(pszName, VERR_NO_MEMORY);
    }

    int rc = PDMR3QueueCreateDriver(pVM, pDrvIns, cbItem, cItems, cMilliesInterval, pfnCallback, pszName, ppQueue);

    LogFlow(("pdmR3DrvHlp_PDMQueueCreate: caller='%s'/%d: returns %Rrc *ppQueue=%p\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc, *ppQueue));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnTMGetVirtualFreq} */
static DECLCALLBACK(uint64_t) pdmR3DrvHlp_TMGetVirtualFreq(PPDMDRVINS pDrvIns)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);

    return TMVirtualGetFreq(pDrvIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMDRVHLP,pfnTMGetVirtualTime} */
static DECLCALLBACK(uint64_t) pdmR3DrvHlp_TMGetVirtualTime(PPDMDRVINS pDrvIns)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);

    return TMVirtualGet(pDrvIns->Internal.s.pVMR3);
}


/** @interface_method_impl{PDMDRVHLP,pfnTMTimerCreate} */
static DECLCALLBACK(int) pdmR3DrvHlp_TMTimerCreate(PPDMDRVINS pDrvIns, TMCLOCK enmClock, PFNTMTIMERDRV pfnCallback, void *pvUser, uint32_t fFlags, const char *pszDesc, PPTMTIMERR3 ppTimer)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_TMTimerCreate: caller='%s'/%d: enmClock=%d pfnCallback=%p pvUser=%p fFlags=%#x pszDesc=%p:{%s} ppTimer=%p\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, enmClock, pfnCallback, pvUser, fFlags, pszDesc, pszDesc, ppTimer));

    int rc = TMR3TimerCreateDriver(pDrvIns->Internal.s.pVMR3, pDrvIns, enmClock, pfnCallback, pvUser, fFlags, pszDesc, ppTimer);

    LogFlow(("pdmR3DrvHlp_TMTimerCreate: caller='%s'/%d: returns %Rrc *ppTimer=%p\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc, *ppTimer));
    return rc;
}



/** @interface_method_impl{PDMDRVHLP,pfnSSMRegister} */
static DECLCALLBACK(int) pdmR3DrvHlp_SSMRegister(PPDMDRVINS pDrvIns, uint32_t uVersion, size_t cbGuess,
                                                 PFNSSMDRVLIVEPREP pfnLivePrep, PFNSSMDRVLIVEEXEC pfnLiveExec, PFNSSMDRVLIVEVOTE pfnLiveVote,
                                                 PFNSSMDRVSAVEPREP pfnSavePrep, PFNSSMDRVSAVEEXEC pfnSaveExec, PFNSSMDRVSAVEDONE pfnSaveDone,
                                                 PFNSSMDRVLOADPREP pfnLoadPrep, PFNSSMDRVLOADEXEC pfnLoadExec, PFNSSMDRVLOADDONE pfnLoadDone)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_SSMRegister: caller='%s'/%d: uVersion=#x cbGuess=%#x \n"
             "    pfnLivePrep=%p pfnLiveExec=%p pfnLiveVote=%p  pfnSavePrep=%p pfnSaveExec=%p pfnSaveDone=%p pszLoadPrep=%p pfnLoadExec=%p pfnLoaddone=%p\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, uVersion, cbGuess,
             pfnLivePrep, pfnLiveExec, pfnLiveVote,
             pfnSavePrep, pfnSaveExec, pfnSaveDone, pfnLoadPrep, pfnLoadExec, pfnLoadDone));

    int rc = SSMR3RegisterDriver(pDrvIns->Internal.s.pVMR3, pDrvIns, pDrvIns->pReg->szName, pDrvIns->iInstance,
                                 uVersion, cbGuess,
                                 pfnLivePrep, pfnLiveExec, pfnLiveVote,
                                 pfnSavePrep, pfnSaveExec, pfnSaveDone,
                                 pfnLoadPrep, pfnLoadExec, pfnLoadDone);

    LogFlow(("pdmR3DrvHlp_SSMRegister: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnSSMDeregister} */
static DECLCALLBACK(int) pdmR3DrvHlp_SSMDeregister(PPDMDRVINS pDrvIns, const char *pszName, uint32_t u32Instance)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_SSMDeregister: caller='%s'/%d: pszName=%p:{%s} u32Instance=%#x\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pszName, pszName, u32Instance));

    int rc = SSMR3DeregisterDriver(pDrvIns->Internal.s.pVMR3, pDrvIns, pszName, u32Instance);

    LogFlow(("pdmR3DrvHlp_SSMDeregister: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLP,pfnDBGFInfoRegister} */
static DECLCALLBACK(int) pdmR3DrvHlp_DBGFInfoRegister(PPDMDRVINS pDrvIns, const char *pszName, const char *pszDesc, PFNDBGFHANDLERDRV pfnHandler)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_DBGFInfoRegister: caller='%s'/%d: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pszName, pszName, pszDesc, pszDesc, pfnHandler));

    int rc = DBGFR3InfoRegisterDriver(pDrvIns->Internal.s.pVMR3, pszName, pszDesc, pfnHandler, pDrvIns);

    LogFlow(("pdmR3DrvHlp_DBGFInfoRegister: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDEVHLP,pfnDBGFInfoDeregister} */
static DECLCALLBACK(int) pdmR3DrvHlp_DBGFInfoDeregister(PPDMDRVINS pDrvIns, const char *pszName)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_DBGFInfoDeregister: caller='%s'/%d: pszName=%p:{%s} pszDesc=%p:{%s} pfnHandler=%p\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pszName));

    int rc = DBGFR3InfoDeregisterDriver(pDrvIns->Internal.s.pVMR3, pDrvIns, pszName);

    LogFlow(("pdmR3DrvHlp_DBGFInfoDeregister: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));

    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnSTAMRegister} */
static DECLCALLBACK(void) pdmR3DrvHlp_STAMRegister(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, const char *pszName, STAMUNIT enmUnit, const char *pszDesc)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    STAM_REG(pDrvIns->Internal.s.pVMR3, pvSample, enmType, pszName, enmUnit, pszDesc);
    /** @todo track the samples so they can be dumped & deregistered when the driver instance is destroyed.
     * For now we just have to be careful not to use this call for drivers which can be unloaded. */
}


/** @interface_method_impl{PDMDRVHLP,pfnSTAMRegisterF} */
static DECLCALLBACK(void) pdmR3DrvHlp_STAMRegisterF(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                    STAMUNIT enmUnit, const char *pszDesc, const char *pszName, ...)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    va_list args;
    va_start(args, pszName);
    int rc = STAMR3RegisterV(pDrvIns->Internal.s.pVMR3, pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, args);
    va_end(args);
    AssertRC(rc);
}


/** @interface_method_impl{PDMDRVHLP,pfnSTAMRegisterV} */
static DECLCALLBACK(void) pdmR3DrvHlp_STAMRegisterV(PPDMDRVINS pDrvIns, void *pvSample, STAMTYPE enmType, STAMVISIBILITY enmVisibility,
                                                    STAMUNIT enmUnit, const char *pszDesc, const char *pszName, va_list args)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    int rc = STAMR3RegisterV(pDrvIns->Internal.s.pVMR3, pvSample, enmType, enmVisibility, enmUnit, pszDesc, pszName, args);
    AssertRC(rc);
}


/** @interface_method_impl{PDMDRVHLP,pfnSTAMDeregister} */
static DECLCALLBACK(int) pdmR3DrvHlp_STAMDeregister(PPDMDRVINS pDrvIns, void *pvSample)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);

    int rc = STAMR3DeregisterU(pDrvIns->Internal.s.pVMR3->pUVM, pvSample);
    AssertRC(rc);
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnSUPCallVMMR0Ex} */
static DECLCALLBACK(int) pdmR3DrvHlp_SUPCallVMMR0Ex(PPDMDRVINS pDrvIns, unsigned uOperation, void *pvArg, unsigned cbArg)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_SSMCallVMMR0Ex: caller='%s'/%d: uOperation=%u pvArg=%p cbArg=%d\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, uOperation, pvArg, cbArg));
    int rc;
    if (    uOperation >= VMMR0_DO_SRV_START
        &&  uOperation <  VMMR0_DO_SRV_END)
        rc = SUPR3CallVMMR0Ex(pDrvIns->Internal.s.pVMR3->pVMR0, NIL_VMCPUID, uOperation, 0, (PSUPVMMR0REQHDR)pvArg);
    else
    {
        AssertMsgFailed(("Invalid uOperation=%u\n", uOperation));
        rc = VERR_INVALID_PARAMETER;
    }

    LogFlow(("pdmR3DrvHlp_SUPCallVMMR0Ex: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnUSBRegisterHub} */
static DECLCALLBACK(int) pdmR3DrvHlp_USBRegisterHub(PPDMDRVINS pDrvIns, uint32_t fVersions, uint32_t cPorts, PCPDMUSBHUBREG pUsbHubReg, PPCPDMUSBHUBHLP ppUsbHubHlp)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_USBRegisterHub: caller='%s'/%d: fVersions=%#x cPorts=%#x pUsbHubReg=%p ppUsbHubHlp=%p\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, fVersions, cPorts, pUsbHubReg, ppUsbHubHlp));

#ifdef VBOX_WITH_USB
    int rc = pdmR3UsbRegisterHub(pDrvIns->Internal.s.pVMR3, pDrvIns, fVersions, cPorts, pUsbHubReg, ppUsbHubHlp);
#else
    int rc = VERR_NOT_SUPPORTED;
#endif

    LogFlow(("pdmR3DrvHlp_USBRegisterHub: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnSetAsyncNotification} */
static DECLCALLBACK(int) pdmR3DrvHlp_SetAsyncNotification(PPDMDRVINS pDrvIns, PFNPDMDRVASYNCNOTIFY pfnAsyncNotify)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT0(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_SetAsyncNotification: caller='%s'/%d: pfnAsyncNotify=%p\n", pDrvIns->pReg->szName, pDrvIns->iInstance, pfnAsyncNotify));

    int rc = VINF_SUCCESS;
    AssertStmt(pfnAsyncNotify, rc = VERR_INVALID_PARAMETER);
    AssertStmt(!pDrvIns->Internal.s.pfnAsyncNotify, rc = VERR_WRONG_ORDER);
    AssertStmt(pDrvIns->Internal.s.fVMSuspended || pDrvIns->Internal.s.fVMReset, rc = VERR_WRONG_ORDER);
    VMSTATE enmVMState = VMR3GetState(pDrvIns->Internal.s.pVMR3);
    AssertStmt(   enmVMState == VMSTATE_SUSPENDING
               || enmVMState == VMSTATE_SUSPENDING_EXT_LS
               || enmVMState == VMSTATE_SUSPENDING_LS
               || enmVMState == VMSTATE_RESETTING
               || enmVMState == VMSTATE_RESETTING_LS
               || enmVMState == VMSTATE_POWERING_OFF
               || enmVMState == VMSTATE_POWERING_OFF_LS,
               rc = VERR_INVALID_STATE);

    if (RT_SUCCESS(rc))
        pDrvIns->Internal.s.pfnAsyncNotify = pfnAsyncNotify;

    LogFlow(("pdmR3DrvHlp_SetAsyncNotification: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName, pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnAsyncNotificationCompleted} */
static DECLCALLBACK(void) pdmR3DrvHlp_AsyncNotificationCompleted(PPDMDRVINS pDrvIns)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    PVM pVM = pDrvIns->Internal.s.pVMR3;

    VMSTATE enmVMState = VMR3GetState(pVM);
    if (   enmVMState == VMSTATE_SUSPENDING
        || enmVMState == VMSTATE_SUSPENDING_EXT_LS
        || enmVMState == VMSTATE_SUSPENDING_LS
        || enmVMState == VMSTATE_RESETTING
        || enmVMState == VMSTATE_RESETTING_LS
        || enmVMState == VMSTATE_POWERING_OFF
        || enmVMState == VMSTATE_POWERING_OFF_LS)
    {
        LogFlow(("pdmR3DrvHlp_AsyncNotificationCompleted: caller='%s'/%d:\n", pDrvIns->pReg->szName, pDrvIns->iInstance));
        VMR3AsyncPdmNotificationWakeupU(pVM->pUVM);
    }
    else
        LogFlow(("pdmR3DrvHlp_AsyncNotificationCompleted: caller='%s'/%d: enmVMState=%d\n", pDrvIns->pReg->szName, pDrvIns->iInstance, enmVMState));
}


/** @interface_method_impl{PDMDRVHLP,pfnThreadCreate} */
static DECLCALLBACK(int) pdmR3DrvHlp_ThreadCreate(PPDMDRVINS pDrvIns, PPPDMTHREAD ppThread, void *pvUser, PFNPDMTHREADDRV pfnThread,
                                                  PFNPDMTHREADWAKEUPDRV pfnWakeup, size_t cbStack, RTTHREADTYPE enmType, const char *pszName)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_ThreadCreate: caller='%s'/%d: ppThread=%p pvUser=%p pfnThread=%p pfnWakeup=%p cbStack=%#zx enmType=%d pszName=%p:{%s}\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, ppThread, pvUser, pfnThread, pfnWakeup, cbStack, enmType, pszName, pszName));

    int rc = pdmR3ThreadCreateDriver(pDrvIns->Internal.s.pVMR3, pDrvIns, ppThread, pvUser, pfnThread, pfnWakeup, cbStack, enmType, pszName);

    LogFlow(("pdmR3DrvHlp_ThreadCreate: caller='%s'/%d: returns %Rrc *ppThread=%RTthrd\n", pDrvIns->pReg->szName, pDrvIns->iInstance,
            rc, *ppThread));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnAsyncCompletionTemplateCreate} */
static DECLCALLBACK(int) pdmR3DrvHlp_AsyncCompletionTemplateCreate(PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate,
                                                                   PFNPDMASYNCCOMPLETEDRV pfnCompleted, void *pvTemplateUser,
                                                                   const char *pszDesc)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    LogFlow(("pdmR3DrvHlp_AsyncCompletionTemplateCreate: caller='%s'/%d: ppTemplate=%p pfnCompleted=%p pszDesc=%p:{%s}\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, ppTemplate, pfnCompleted, pszDesc, pszDesc));

    int rc = PDMR3AsyncCompletionTemplateCreateDriver(pDrvIns->Internal.s.pVMR3, pDrvIns, ppTemplate, pfnCompleted, pvTemplateUser, pszDesc);

    LogFlow(("pdmR3DrvHlp_AsyncCompletionTemplateCreate: caller='%s'/%d: returns %Rrc *ppThread=%p\n", pDrvIns->pReg->szName,
             pDrvIns->iInstance, rc, *ppTemplate));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnLdrGetRCInterfaceSymbols} */
static DECLCALLBACK(int) pdmR3DrvHlp_LdrGetRCInterfaceSymbols(PPDMDRVINS pDrvIns, void *pvInterface, size_t cbInterface,
                                                              const char *pszSymPrefix, const char *pszSymList)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_LdrGetRCInterfaceSymbols: caller='%s'/%d: pvInterface=%p cbInterface=%zu pszSymPrefix=%p:{%s} pszSymList=%p:{%s}\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pvInterface, cbInterface, pszSymPrefix, pszSymPrefix, pszSymList, pszSymList));

    int rc;
    if (   strncmp(pszSymPrefix, "drv", 3) == 0
        && RTStrIStr(pszSymPrefix + 3, pDrvIns->pReg->szName) != NULL)
    {
        if (pDrvIns->pReg->fFlags & PDM_DRVREG_FLAGS_RC)
            rc = PDMR3LdrGetInterfaceSymbols(pDrvIns->Internal.s.pVMR3, pvInterface, cbInterface,
                                             pDrvIns->pReg->szRCMod, pszSymPrefix, pszSymList,
                                             false /*fRing0OrRC*/);
        else
        {
            AssertMsgFailed(("Not a raw-mode enabled driver\n"));
            rc = VERR_PERMISSION_DENIED;
        }
    }
    else
    {
        AssertMsgFailed(("Invalid prefix '%s' for '%s'; must start with 'drv' and contain the driver name!\n",
                         pszSymPrefix, pDrvIns->pReg->szName));
        rc = VERR_INVALID_NAME;
    }

    LogFlow(("pdmR3DrvHlp_LdrGetRCInterfaceSymbols: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName,
             pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnLdrGetR0InterfaceSymbols} */
static DECLCALLBACK(int) pdmR3DrvHlp_LdrGetR0InterfaceSymbols(PPDMDRVINS pDrvIns, void *pvInterface, size_t cbInterface,
                                                              const char *pszSymPrefix, const char *pszSymList)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    VM_ASSERT_EMT(pDrvIns->Internal.s.pVMR3);
    LogFlow(("pdmR3DrvHlp_LdrGetR0InterfaceSymbols: caller='%s'/%d: pvInterface=%p cbInterface=%zu pszSymPrefix=%p:{%s} pszSymList=%p:{%s}\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pvInterface, cbInterface, pszSymPrefix, pszSymPrefix, pszSymList, pszSymList));

    int rc;
    if (   strncmp(pszSymPrefix, "drv", 3) == 0
        && RTStrIStr(pszSymPrefix + 3, pDrvIns->pReg->szName) != NULL)
    {
        if (pDrvIns->pReg->fFlags & PDM_DRVREG_FLAGS_R0)
            rc = PDMR3LdrGetInterfaceSymbols(pDrvIns->Internal.s.pVMR3, pvInterface, cbInterface,
                                             pDrvIns->pReg->szR0Mod, pszSymPrefix, pszSymList,
                                             true /*fRing0OrRC*/);
        else
        {
            AssertMsgFailed(("Not a ring-0 enabled driver\n"));
            rc = VERR_PERMISSION_DENIED;
        }
    }
    else
    {
        AssertMsgFailed(("Invalid prefix '%s' for '%s'; must start with 'drv' and contain the driver name!\n",
                         pszSymPrefix, pDrvIns->pReg->szName));
        rc = VERR_INVALID_NAME;
    }

    LogFlow(("pdmR3DrvHlp_LdrGetR0InterfaceSymbols: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName,
             pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnCritSectInit} */
static DECLCALLBACK(int) pdmR3DrvHlp_CritSectInit(PPDMDRVINS pDrvIns, PPDMCRITSECT pCritSect,
                                                  RT_SRC_POS_DECL, const char *pszName)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    PVM pVM = pDrvIns->Internal.s.pVMR3;
    VM_ASSERT_EMT(pVM);
    LogFlow(("pdmR3DrvHlp_CritSectInit: caller='%s'/%d: pCritSect=%p pszName=%s\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, pCritSect, pszName));

    int rc = pdmR3CritSectInitDriver(pVM, pDrvIns, pCritSect, RT_SRC_POS_ARGS, "%s_%u", pszName, pDrvIns->iInstance);

    LogFlow(("pdmR3DrvHlp_CritSectInit: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName,
             pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnCallR0} */
static DECLCALLBACK(int) pdmR3DrvHlp_CallR0(PPDMDRVINS pDrvIns, uint32_t uOperation, uint64_t u64Arg)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    PVM pVM = pDrvIns->Internal.s.pVMR3;
    LogFlow(("pdmR3DrvHlp_CallR0: caller='%s'/%d: uOperation=%#x u64Arg=%#RX64\n",
             pDrvIns->pReg->szName, pDrvIns->iInstance, uOperation, u64Arg));

    /*
     * Lazy resolve the ring-0 entry point.
     */
    int rc = VINF_SUCCESS;
    PFNPDMDRVREQHANDLERR0 pfnReqHandlerR0 = pDrvIns->Internal.s.pfnReqHandlerR0;
    if (RT_UNLIKELY(pfnReqHandlerR0 == NIL_RTR0PTR))
    {
        if (pDrvIns->pReg->fFlags & PDM_DRVREG_FLAGS_R0)
        {
            char szSymbol[          sizeof("drvR0") + sizeof(pDrvIns->pReg->szName) + sizeof("ReqHandler")];
            strcat(strcat(strcpy(szSymbol, "drvR0"),         pDrvIns->pReg->szName),         "ReqHandler");
            szSymbol[sizeof("drvR0") - 1] = RT_C_TO_UPPER(szSymbol[sizeof("drvR0") - 1]);

            rc = PDMR3LdrGetSymbolR0Lazy(pVM, pDrvIns->pReg->szR0Mod, szSymbol, &pfnReqHandlerR0);
            if (RT_SUCCESS(rc))
                pDrvIns->Internal.s.pfnReqHandlerR0 = pfnReqHandlerR0;
            else
                pfnReqHandlerR0 = NIL_RTR0PTR;
        }
        else
            rc = VERR_ACCESS_DENIED;
    }
    if (RT_LIKELY(pfnReqHandlerR0 != NIL_RTR0PTR))
    {
        /*
         * Make the ring-0 call.
         */
        PDMDRIVERCALLREQHANDLERREQ Req;
        Req.Hdr.u32Magic    = SUPVMMR0REQHDR_MAGIC;
        Req.Hdr.cbReq       = sizeof(Req);
        Req.pDrvInsR0       = PDMDRVINS_2_R0PTR(pDrvIns);
        Req.uOperation      = uOperation;
        Req.u32Alignment    = 0;
        Req.u64Arg          = u64Arg;
        rc = SUPR3CallVMMR0Ex(pVM->pVMR0, NIL_VMCPUID, VMMR0_DO_PDM_DRIVER_CALL_REQ_HANDLER, 0, &Req.Hdr);
    }

    LogFlow(("pdmR3DrvHlp_CallR0: caller='%s'/%d: returns %Rrc\n", pDrvIns->pReg->szName,
             pDrvIns->iInstance, rc));
    return rc;
}


/** @interface_method_impl{PDMDRVHLP,pfnFTSetCheckpoint} */
static DECLCALLBACK(int) pdmR3DrvHlp_FTSetCheckpoint(PPDMDRVINS pDrvIns, FTMCHECKPOINTTYPE enmType)
{
    PDMDRV_ASSERT_DRVINS(pDrvIns);
    return FTMSetCheckpoint(pDrvIns->Internal.s.pVMR3, enmType);
}


/**
 * The driver helper structure.
 */
const PDMDRVHLPR3 g_pdmR3DrvHlp =
{
    PDM_DRVHLPR3_VERSION,
    pdmR3DrvHlp_Attach,
    pdmR3DrvHlp_Detach,
    pdmR3DrvHlp_DetachSelf,
    pdmR3DrvHlp_MountPrepare,
    pdmR3DrvHlp_AssertEMT,
    pdmR3DrvHlp_AssertOther,
    pdmR3DrvHlp_VMSetError,
    pdmR3DrvHlp_VMSetErrorV,
    pdmR3DrvHlp_VMSetRuntimeError,
    pdmR3DrvHlp_VMSetRuntimeErrorV,
    pdmR3DrvHlp_VMState,
    pdmR3DrvHlp_VMTeleportedAndNotFullyResumedYet,
    pdmR3DrvHlp_GetSupDrvSession,
    pdmR3DrvHlp_QueueCreate,
    pdmR3DrvHlp_TMGetVirtualFreq,
    pdmR3DrvHlp_TMGetVirtualTime,
    pdmR3DrvHlp_TMTimerCreate,
    pdmR3DrvHlp_SSMRegister,
    pdmR3DrvHlp_SSMDeregister,
    pdmR3DrvHlp_DBGFInfoRegister,
    pdmR3DrvHlp_DBGFInfoDeregister,
    pdmR3DrvHlp_STAMRegister,
    pdmR3DrvHlp_STAMRegisterF,
    pdmR3DrvHlp_STAMRegisterV,
    pdmR3DrvHlp_STAMDeregister,
    pdmR3DrvHlp_SUPCallVMMR0Ex,
    pdmR3DrvHlp_USBRegisterHub,
    pdmR3DrvHlp_SetAsyncNotification,
    pdmR3DrvHlp_AsyncNotificationCompleted,
    pdmR3DrvHlp_ThreadCreate,
    pdmR3DrvHlp_AsyncCompletionTemplateCreate,
    pdmR3DrvHlp_LdrGetRCInterfaceSymbols,
    pdmR3DrvHlp_LdrGetR0InterfaceSymbols,
    pdmR3DrvHlp_CritSectInit,
    pdmR3DrvHlp_CallR0,
    pdmR3DrvHlp_FTSetCheckpoint,
    PDM_DRVHLPR3_VERSION /* u32TheEnd */
};

/** @} */

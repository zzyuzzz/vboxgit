/* $Id: PDMAsyncCompletion.cpp 20185 2009-06-02 12:04:10Z vboxsync $ */
/** @file
 * PDM Async I/O - Transport data asynchronous in R3 using EMT.
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
#define LOG_GROUP LOG_GROUP_PDM_ASYNC_COMPLETION
#include "PDMInternal.h"
#include <VBox/pdm.h>
#include <VBox/mm.h>
#include <VBox/rem.h>
#include <VBox/vm.h>
#include <VBox/err.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/thread.h>
#include <iprt/mem.h>
#include <iprt/critsect.h>
#include <iprt/tcp.h>

#include <VBox/pdmasynccompletion.h>
#include "PDMAsyncCompletionInternal.h"

/**
 * Async I/O type.
 */
typedef enum PDMASYNCCOMPLETIONTEMPLATETYPE
{
    /** Device . */
    PDMASYNCCOMPLETIONTEMPLATETYPE_DEV = 1,
    /** Driver consumer. */
    PDMASYNCCOMPLETIONTEMPLATETYPE_DRV,
    /** Internal consumer. */
    PDMASYNCCOMPLETIONTEMPLATETYPE_INTERNAL,
    /** Usb consumer. */
    PDMASYNCCOMPLETIONTEMPLATETYPE_USB
} PDMASYNCTEMPLATETYPE;

/**
 * PDM Async I/O template.
 */
typedef struct PDMASYNCCOMPLETIONTEMPLATE
{
    /** Pointer to the next template in the list. */
    R3PTRTYPE(PPDMASYNCCOMPLETIONTEMPLATE)    pNext;
    /** Pointer to the previous template in the list. */
    R3PTRTYPE(PPDMASYNCCOMPLETIONTEMPLATE)    pPrev;
    /** Type specific data. */
    union
    {
        /** PDMASYNCCOMPLETIONTEMPLATETYPE_DEV */
        struct
        {
            /** Pointer to consumer function. */
            R3PTRTYPE(PFNPDMASYNCCOMPLETEDEV)   pfnCompleted;
            /** Pointer to the device instance owning the template. */
            R3PTRTYPE(PPDMDEVINS)               pDevIns;
        } Dev;
        /** PDMASYNCCOMPLETIONTEMPLATETYPE_DRV */
        struct
        {
            /** Pointer to consumer function. */
            R3PTRTYPE(PFNPDMASYNCCOMPLETEDRV)   pfnCompleted;
            /** Pointer to the driver instance owning the template. */
            R3PTRTYPE(PPDMDRVINS)               pDrvIns;
            /** User agument given during template creation.
             *  This is only here to make things much easier
             *  for DrVVD. */
            void                               *pvTemplateUser;
        } Drv;
        /** PDMASYNCCOMPLETIONTEMPLATETYPE_INTERNAL */
        struct
        {
            /** Pointer to consumer function. */
            R3PTRTYPE(PFNPDMASYNCCOMPLETEINT)   pfnCompleted;
            /** Pointer to user data. */
            R3PTRTYPE(void *)                   pvUser;
        } Int;
        /** PDMASYNCCOMPLETIONTEMPLATETYPE_USB */
        struct
        {
            /** Pointer to consumer function. */
            R3PTRTYPE(PFNPDMASYNCCOMPLETEUSB)   pfnCompleted;
            /** Pointer to the usb instance owning the template. */
            R3PTRTYPE(PPDMUSBINS)               pUsbIns;
        } Usb;
    } u;
    /** Template type. */
    PDMASYNCCOMPLETIONTEMPLATETYPE          enmType;
    /** Pointer to the VM. */
    R3PTRTYPE(PVM)                          pVM;
    /** Use count of the template. */
    volatile uint32_t                       cUsed;
} PDMASYNCCOMPLETIONTEMPLATE;

static void pdmR3AsyncCompletionPutTask(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, PPDMASYNCCOMPLETIONTASK pTask, bool fLocal);

/**
 * Internal worker for the creation apis
 *
 * @returns VBox status.
 * @param   pVM           VM handle.
 * @param   ppTemplate    Where to store the template handle.
 */
static int pdmR3AsyncCompletionTemplateCreate(PVM pVM, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate, PDMASYNCCOMPLETIONTEMPLATETYPE enmType)
{
    int rc = VINF_SUCCESS;

    if (ppTemplate == NULL)
    {
        AssertMsgFailed(("ppTemplate is NULL\n"));
        return VERR_INVALID_PARAMETER;
    }

    PPDMASYNCCOMPLETIONTEMPLATE pTemplate;
    rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_ASYNC_COMPLETION, sizeof(PDMASYNCCOMPLETIONTEMPLATE), (void **)&pTemplate);
    if (RT_FAILURE(rc))
        return rc;

    /*
     * Initialize fields.
     */
    pTemplate->pVM = pVM;
    pTemplate->cUsed = 0;
    pTemplate->enmType = enmType;

    /*
     * Add template to the global VM template list.
     */
    pTemplate->pNext = pVM->pdm.s.pAsyncCompletionTemplates;
    if (pVM->pdm.s.pAsyncCompletionTemplates)
        pVM->pdm.s.pAsyncCompletionTemplates->pPrev = pTemplate;
    pVM->pdm.s.pAsyncCompletionTemplates = pTemplate;

    *ppTemplate = pTemplate;
    return VINF_SUCCESS;
}

/**
 * Creates a async completion template for a device instance.
 *
 * The template is used when creating new completion tasks.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pDevIns         The device instance.
 * @param   ppTemplate      Where to store the template pointer on success.
 * @param   pfnCompleted    The completion callback routine.
 * @param   pszDesc         Description.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateCreateDevice(PVM pVM, PPDMDEVINS pDevIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate, PFNPDMASYNCCOMPLETEDEV pfnCompleted, const char *pszDesc)
{
    LogFlow(("%s: pDevIns=%p ppTemplate=%p pfnCompleted=%p pszDesc=%s\n",
              __FUNCTION__, pDevIns, ppTemplate, pfnCompleted, pszDesc));

    /*
     * Validate input.
     */
    VM_ASSERT_EMT(pVM);
    if (!pfnCompleted)
    {
        AssertMsgFailed(("No completion callback!\n"));
        return VERR_INVALID_PARAMETER;
    }

    if (!ppTemplate)
    {
        AssertMsgFailed(("Template pointer is NULL!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Create the template.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate;
    int rc = pdmR3AsyncCompletionTemplateCreate(pVM, &pTemplate, PDMASYNCCOMPLETIONTEMPLATETYPE_DEV);
    if (RT_SUCCESS(rc))
    {
        pTemplate->u.Dev.pDevIns = pDevIns;
        pTemplate->u.Dev.pfnCompleted = pfnCompleted;

        *ppTemplate = pTemplate;
        Log(("PDM: Created device template %p: pfnCompleted=%p pDevIns=%p\n",
             pTemplate, pfnCompleted, pDevIns));
    }

    return rc;
}

/**
 * Creates a async completion template for a driver instance.
 *
 * The template is used when creating new completion tasks.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pDrvIns         The driver instance.
 * @param   ppTemplate      Where to store the template pointer on success.
 * @param   pfnCompleted    The completion callback routine.
 * @param   pvTemplateUser  Template user argument
 * @param   pszDesc         Description.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateCreateDriver(PVM pVM, PPDMDRVINS pDrvIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate, PFNPDMASYNCCOMPLETEDRV pfnCompleted, void *pvTemplateUser, const char *pszDesc)
{
    LogFlow(("%s: pDrvIns=%p ppTemplate=%p pfnCompleted=%p pszDesc=%s\n",
              __FUNCTION__, pDrvIns, ppTemplate, pfnCompleted, pszDesc));

    /*
     * Validate input.
     */
    VM_ASSERT_EMT(pVM);
    if (!pfnCompleted)
    {
        AssertMsgFailed(("No completion callback!\n"));
        return VERR_INVALID_PARAMETER;
    }

    if (!ppTemplate)
    {
        AssertMsgFailed(("Template pointer is NULL!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Create the template.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate;
    int rc = pdmR3AsyncCompletionTemplateCreate(pVM, &pTemplate, PDMASYNCCOMPLETIONTEMPLATETYPE_DRV);
    if (RT_SUCCESS(rc))
    {
        pTemplate->u.Drv.pDrvIns        = pDrvIns;
        pTemplate->u.Drv.pfnCompleted   = pfnCompleted;
        pTemplate->u.Drv.pvTemplateUser = pvTemplateUser;

        *ppTemplate = pTemplate;
        Log(("PDM: Created driver template %p: pfnCompleted=%p pDrvIns=%p\n",
             pTemplate, pfnCompleted, pDrvIns));
    }

    return rc;
}

/**
 * Creates a async completion template for a USB device instance.
 *
 * The template is used when creating new completion tasks.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pUsbIns         The USB device instance.
 * @param   ppTemplate      Where to store the template pointer on success.
 * @param   pfnCompleted    The completion callback routine.
 * @param   pszDesc         Description.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateCreateUsb(PVM pVM, PPDMUSBINS pUsbIns, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate, PFNPDMASYNCCOMPLETEUSB pfnCompleted, const char *pszDesc)
{
    LogFlow(("%s: pUsbIns=%p ppTemplate=%p pfnCompleted=%p pszDesc=%s\n",
              __FUNCTION__, pUsbIns, ppTemplate, pfnCompleted, pszDesc));

    /*
     * Validate input.
     */
    VM_ASSERT_EMT(pVM);
    if (!pfnCompleted)
    {
        AssertMsgFailed(("No completion callback!\n"));
        return VERR_INVALID_PARAMETER;
    }

    if (!ppTemplate)
    {
        AssertMsgFailed(("Template pointer is NULL!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Create the template.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate;
    int rc = pdmR3AsyncCompletionTemplateCreate(pVM, &pTemplate, PDMASYNCCOMPLETIONTEMPLATETYPE_USB);
    if (RT_SUCCESS(rc))
    {
        pTemplate->u.Usb.pUsbIns = pUsbIns;
        pTemplate->u.Usb.pfnCompleted = pfnCompleted;

        *ppTemplate = pTemplate;
        Log(("PDM: Created usb template %p: pfnCompleted=%p pDevIns=%p\n",
             pTemplate, pfnCompleted, pUsbIns));
    }

    return rc;
}

/**
 * Creates a async completion template for internally by the VMM.
 *
 * The template is used when creating new completion tasks.
 *
 * @returns VBox status code.
 * @param   pVM             Pointer to the shared VM structure.
 * @param   ppTemplate      Where to store the template pointer on success.
 * @param   pfnCompleted    The completion callback routine.
 * @param   pvUser2         The 2nd user argument for the callback.
 * @param   pszDesc         Description.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateCreateInternal(PVM pVM, PPPDMASYNCCOMPLETIONTEMPLATE ppTemplate, PFNPDMASYNCCOMPLETEINT pfnCompleted, void *pvUser2, const char *pszDesc)
{
    LogFlow(("%s: ppTemplate=%p pfnCompleted=%p pvUser2=%p pszDesc=%s\n",
              __FUNCTION__, ppTemplate, pfnCompleted, pvUser2, pszDesc));

    /*
     * Validate input.
     */
    VM_ASSERT_EMT(pVM);
    if (!pfnCompleted)
    {
        AssertMsgFailed(("No completion callback!\n"));
        return VERR_INVALID_PARAMETER;
    }

    if (!ppTemplate)
    {
        AssertMsgFailed(("Template pointer is NULL!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Create the template.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate;
    int rc = pdmR3AsyncCompletionTemplateCreate(pVM, &pTemplate, PDMASYNCCOMPLETIONTEMPLATETYPE_INTERNAL);
    if (RT_SUCCESS(rc))
    {
        pTemplate->u.Int.pvUser = pvUser2;
        pTemplate->u.Int.pfnCompleted = pfnCompleted;

        *ppTemplate = pTemplate;
        Log(("PDM: Created internal template %p: pfnCompleted=%p pvUser2=%p\n",
             pTemplate, pfnCompleted, pvUser2));
    }

    return rc;
}

/**
 * Destroys the specified async completion template.
 *
 * @returns VBox status codes:
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PDM_ASYNC_TEMPLATE_BUSY if the template is still in use.
 *
 * @param   pTemplate       The template in question.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateDestroy(PPDMASYNCCOMPLETIONTEMPLATE pTemplate)
{
    LogFlow(("%s: pTemplate=%p\n", __FUNCTION__, pTemplate));

    if (!pTemplate)
    {
        AssertMsgFailed(("pTemplate is NULL!\n"));
        return VERR_INVALID_PARAMETER;
    }

    /*
     * Check if the template is still used.
     */
    if (pTemplate->cUsed > 0)
    {
        AssertMsgFailed(("Template is still in use\n"));
        return VERR_PDM_ASYNC_TEMPLATE_BUSY;
    }

    /*
     * Unlink the template from the list.
     */
    PVM pVM = pTemplate->pVM;
    PPDMASYNCCOMPLETIONTEMPLATE pPrev = pTemplate->pPrev;
    PPDMASYNCCOMPLETIONTEMPLATE pNext = pTemplate->pNext;

    if (pPrev)
        pPrev->pNext = pNext;
    else
        pVM->pdm.s.pAsyncCompletionTemplates = pNext;

    if (pNext)
        pNext->pPrev = pPrev;

    /*
     * Free the template.
     */
    MMR3HeapFree(pTemplate);

    return VINF_SUCCESS;
}

/**
 * Destroys all the specified async completion templates for the given device instance.
 *
 * @returns VBox status codes:
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PDM_ASYNC_TEMPLATE_BUSY if one or more of the templates are still in use.
 *
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pDevIns         The device instance.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateDestroyDevice(PVM pVM, PPDMDEVINS pDevIns)
{
    LogFlow(("%s: pDevIns=%p\n", __FUNCTION__, pDevIns));

    /*
     * Validate input.
     */
    if (!pDevIns)
        return VERR_INVALID_PARAMETER;
    VM_ASSERT_EMT(pVM);

    /*
     * Unlink it.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate = pVM->pdm.s.pAsyncCompletionTemplates;
    while (pTemplate)
    {
        if (    pTemplate->enmType == PDMASYNCCOMPLETIONTEMPLATETYPE_DEV
            &&  pTemplate->u.Dev.pDevIns == pDevIns)
        {
            PPDMASYNCCOMPLETIONTEMPLATE pTemplateDestroy = pTemplate;
            pTemplate = pTemplate->pNext;
            int rc = PDMR3AsyncCompletionTemplateDestroy(pTemplateDestroy);
            if (RT_FAILURE(rc))
                return rc;
        }
        else
            pTemplate = pTemplate->pNext;
    }

    return VINF_SUCCESS;
}

/**
 * Destroys all the specified async completion templates for the given driver instance.
 *
 * @returns VBox status codes:
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PDM_ASYNC_TEMPLATE_BUSY if one or more of the templates are still in use.
 *
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pDrvIns         The driver instance.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateDestroyDriver(PVM pVM, PPDMDRVINS pDrvIns)
{
    LogFlow(("%s: pDevIns=%p\n", __FUNCTION__, pDrvIns));

    /*
     * Validate input.
     */
    if (!pDrvIns)
        return VERR_INVALID_PARAMETER;
    VM_ASSERT_EMT(pVM);

    /*
     * Unlink it.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate = pVM->pdm.s.pAsyncCompletionTemplates;
    while (pTemplate)
    {
        if (    pTemplate->enmType == PDMASYNCCOMPLETIONTEMPLATETYPE_DRV
            &&  pTemplate->u.Drv.pDrvIns == pDrvIns)
        {
            PPDMASYNCCOMPLETIONTEMPLATE pTemplateDestroy = pTemplate;
            pTemplate = pTemplate->pNext;
            int rc = PDMR3AsyncCompletionTemplateDestroy(pTemplateDestroy);
            if (RT_FAILURE(rc))
                return rc;
        }
        else
            pTemplate = pTemplate->pNext;
    }

    return VINF_SUCCESS;
}

/**
 * Destroys all the specified async completion templates for the given USB device instance.
 *
 * @returns VBox status codes:
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_PDM_ASYNC_TEMPLATE_BUSY if one or more of the templates are still in use.
 *
 * @param   pVM             Pointer to the shared VM structure.
 * @param   pUsbIns         The USB device instance.
 */
VMMR3DECL(int) PDMR3AsyncCompletionTemplateDestroyUsb(PVM pVM, PPDMUSBINS pUsbIns)
{
    LogFlow(("%s: pUsbIns=%p\n", __FUNCTION__, pUsbIns));

    /*
     * Validate input.
     */
    if (!pUsbIns)
        return VERR_INVALID_PARAMETER;
    VM_ASSERT_EMT(pVM);

    /*
     * Unlink it.
     */
    PPDMASYNCCOMPLETIONTEMPLATE pTemplate = pVM->pdm.s.pAsyncCompletionTemplates;
    while (pTemplate)
    {
        if (    pTemplate->enmType == PDMASYNCCOMPLETIONTEMPLATETYPE_USB
            &&  pTemplate->u.Usb.pUsbIns == pUsbIns)
        {
            PPDMASYNCCOMPLETIONTEMPLATE pTemplateDestroy = pTemplate;
            pTemplate = pTemplate->pNext;
            int rc = PDMR3AsyncCompletionTemplateDestroy(pTemplateDestroy);
            if (RT_FAILURE(rc))
                return rc;
        }
        else
            pTemplate = pTemplate->pNext;
    }

    return VINF_SUCCESS;
}

void pdmR3AsyncCompletionCompleteTask(PPDMASYNCCOMPLETIONTASK pTask)
{
    LogFlow(("%s: pTask=%p\n", __FUNCTION__, pTask));

    PPDMASYNCCOMPLETIONTEMPLATE pTemplate = pTask->pEndpoint->pTemplate;

    switch (pTemplate->enmType)
    {
        case PDMASYNCCOMPLETIONTEMPLATETYPE_DEV:
        {
            pTemplate->u.Dev.pfnCompleted(pTemplate->u.Dev.pDevIns, pTask->pvUser);
            break;
        }
        case PDMASYNCCOMPLETIONTEMPLATETYPE_DRV:
        {
            pTemplate->u.Drv.pfnCompleted(pTemplate->u.Drv.pDrvIns, pTemplate->u.Drv.pvTemplateUser, pTask->pvUser);
            break;
        }
        case PDMASYNCCOMPLETIONTEMPLATETYPE_USB:
        {
            pTemplate->u.Usb.pfnCompleted(pTemplate->u.Usb.pUsbIns, pTask->pvUser);
            break;
        }
        case PDMASYNCCOMPLETIONTEMPLATETYPE_INTERNAL:
        {
            pTemplate->u.Int.pfnCompleted(pTemplate->pVM, pTask->pvUser, pTemplate->u.Int.pvUser);
            break;
        }
        default:
            AssertMsgFailed(("Unknown template type!\n"));
    }

    pdmR3AsyncCompletionPutTask(pTask->pEndpoint, pTask, true);
}

/**
 * Worker initializing a endpoint class.
 *
 * @returns VBox statis code.
 * @param   pVM        Pointer to the shared VM instance data.
 * @param   pEpClass   Pointer to the endpoint class structure.
 * @param   pCfgHandle Pointer to the the CFGM tree.
 */
int pdmR3AsyncCompletionEpClassInit(PVM pVM, PCPDMASYNCCOMPLETIONEPCLASSOPS pEpClassOps, PCFGMNODE pCfgHandle)
{
    int rc = VINF_SUCCESS;

    /* Validate input. */
    if (   !pEpClassOps
        || (pEpClassOps->u32Version != PDMAC_EPCLASS_OPS_VERSION)
        || (pEpClassOps->u32VersionEnd != PDMAC_EPCLASS_OPS_VERSION))
        AssertMsgFailedReturn(("Invalid endpoint class data\n"), VERR_VERSION_MISMATCH);

    LogFlowFunc((": pVM=%p pEpClassOps=%p{%s}\n", pVM, pEpClassOps, pEpClassOps->pcszName));

    /* Allocate global class data. */
    PPDMASYNCCOMPLETIONEPCLASS pEndpointClass = NULL;

    rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                          pEpClassOps->cbEndpointClassGlobal,
                          (void **)&pEndpointClass);
    if (RT_SUCCESS(rc))
    {
        /* Initialize common data. */
        pEndpointClass->pVM          = pVM;
        pEndpointClass->pEndpointOps = pEpClassOps;

        rc = RTCritSectInit(&pEndpointClass->CritSect);
        if (RT_SUCCESS(rc))
        {
            PCFGMNODE pCfgNodeClass = CFGMR3GetChild(pCfgHandle, pEpClassOps->pcszName);

            /* Query the common CFGM options */
            rc = CFGMR3QueryU32Def(pCfgNodeClass, "TaskCachePerEndpoint", &pEndpointClass->cEndpointCacheSize, 5);
            AssertRCReturn(rc, rc);

            rc = CFGMR3QueryU32Def(pCfgNodeClass, "TaskCachePerClass", &pEndpointClass->cEpClassCacheSize, 50);
            AssertRCReturn(rc, rc);

            /* Call the specific endpoint class initializer. */
            rc = pEpClassOps->pfnInitialize(pEndpointClass, pCfgNodeClass);
            if (RT_SUCCESS(rc))
            {
                AssertMsg(!pVM->pdm.s.papAsyncCompletionEndpointClass[pEpClassOps->enmClassType],
                              ("Endpoint class was already initialized\n"));

                pVM->pdm.s.papAsyncCompletionEndpointClass[pEpClassOps->enmClassType] = pEndpointClass;
                LogFlowFunc((": Initialized endpoint class \"%s\" rc=%Rrc\n", pEpClassOps->pcszName, rc));
                return VINF_SUCCESS;
            }
            RTCritSectDelete(&pEndpointClass->CritSect);
        }
        MMR3HeapFree(pEndpointClass);
    }

    LogFlowFunc((": Failed to initialize endpoint class rc=%Rrc\n", rc));

    return rc;
}

/**
 * Worker terminating all endpoint classes.
 *
 * @returns nothing
 * @param   pEndpointClass    Pointer to the endpoint class to terminate.
 *
 * @remarks This method ensures that any still open endpoint is closed.
 */
static void pdmR3AsyncCompletionEpClassTerminate(PPDMASYNCCOMPLETIONEPCLASS pEndpointClass)
{
    int rc = VINF_SUCCESS;
    PVM pVM = pEndpointClass->pVM;

    /* Close all still open endpoints. */
    while (pEndpointClass->pEndpointsHead)
        PDMR3AsyncCompletionEpClose(pEndpointClass->pEndpointsHead);

    /* Destroy all cached tasks. */
    for (unsigned i = 0; i < RT_ELEMENTS(pEndpointClass->apTaskCache); i++)
    {
        PPDMASYNCCOMPLETIONTASK pTask = pEndpointClass->apTaskCache[i];

        while (pTask)
        {
            PPDMASYNCCOMPLETIONTASK pTaskFree = pTask;
            pTask = pTask->pNext;
            MMR3HeapFree(pTaskFree);
        }
    }

    /* Call the termination callback of the class. */
    pEndpointClass->pEndpointOps->pfnTerminate(pEndpointClass);

    RTCritSectDelete(&pEndpointClass->CritSect);

    /* Free the memory of the class finally and clear the entry in the class array. */
    pVM->pdm.s.papAsyncCompletionEndpointClass[pEndpointClass->pEndpointOps->enmClassType] = NULL;
    MMR3HeapFree(pEndpointClass);
}

/**
 * Initialize the async completion manager.
 *
 * @returns VBox status code
 * @param   pVM Pointer to the shared VM structure.
 */
int pdmR3AsyncCompletionInit(PVM pVM)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc((": pVM=%p\n", pVM));

    VM_ASSERT_EMT(pVM);

    do
    {
        /* Allocate array for global class data. */
        rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                              sizeof(PPDMASYNCCOMPLETIONEPCLASS) * PDMASYNCCOMPLETIONEPCLASSTYPE_MAX,
                              (void **)&pVM->pdm.s.papAsyncCompletionEndpointClass);
        if (RT_FAILURE(rc))
            break;

        PCFGMNODE pCfgRoot = CFGMR3GetRoot(pVM);
        PCFGMNODE pCfgAsyncCompletion = CFGMR3GetChild(CFGMR3GetChild(pCfgRoot, "PDM"), "AsyncCompletion");

        rc = pdmR3AsyncCompletionEpClassInit(pVM, &g_PDMAsyncCompletionEndpointClassFile, pCfgAsyncCompletion);
        if (RT_FAILURE(rc))
            break;

        /* Put other classes here. */
    } while (0);

    LogFlowFunc((": pVM=%p rc=%Rrc\n", pVM, rc));

    return rc;
}

/**
 * Terminates the async completion manager.
 *
 * @returns VBox status code
 * @param   pVM Pointer to the shared VM structure.
 */
int pdmR3AsyncCompletionTerm(PVM pVM)
{
    LogFlowFunc((": pVM=%p\n", pVM));

    pdmR3AsyncCompletionEpClassTerminate(pVM->pdm.s.papAsyncCompletionEndpointClass[PDMASYNCCOMPLETIONEPCLASSTYPE_FILE]);

    MMR3HeapFree(pVM->pdm.s.papAsyncCompletionEndpointClass);
    return VINF_SUCCESS;
}

/**
 * Tries to get a free task from the endpoint or class cache
 * allocating the task if it fails.
 *
 * @returns Pointer to a new and initialized task or NULL
 * @param   pEndpoint    The endpoint the task is for.
 * @param   pvUser       Opaque user data for the task.
 */
static PPDMASYNCCOMPLETIONTASK pdmR3AsyncCompletionGetTask(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, void *pvUser)
{
    PPDMASYNCCOMPLETIONTASK pTask = NULL;

    /* Try the small per endpoint cache first. */
    if (pEndpoint->pTasksFreeHead == pEndpoint->pTasksFreeTail)
    {
        /* Try the bigger per endpoint class cache. */
        PPDMASYNCCOMPLETIONEPCLASS pEndpointClass = pEndpoint->pEpClass;

        /* We start with the assigned slot id to distribute the load when allocating new tasks. */
        unsigned iSlot = pEndpoint->iSlotStart;
        do
        {
            pTask = (PPDMASYNCCOMPLETIONTASK)ASMAtomicXchgPtr((void * volatile *)&pEndpointClass->apTaskCache[iSlot], NULL);
            if (pTask)
                break;

            iSlot = (iSlot + 1) % RT_ELEMENTS(pEndpointClass->apTaskCache);
        } while (iSlot != pEndpoint->iSlotStart);

        if (!pTask)
        {
            /*
             * Allocate completely new.
             * If this fails we return NULL.
             */
            int rc = MMR3HeapAllocZEx(pEndpointClass->pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                                      pEndpointClass->pEndpointOps->cbTask,
                                      (void **)&pTask);
            if (RT_FAILURE(rc))
                pTask = NULL;
        }
        else
        {
            /* Remove the first element and put the rest into the slot again. */
            PPDMASYNCCOMPLETIONTASK pTaskHeadNew = pTask->pNext;

            /* Put back into the list adding any new tasks. */
            while (true)
            {
                bool fChanged = ASMAtomicCmpXchgPtr((void * volatile *)&pEndpointClass->apTaskCache[iSlot], pTaskHeadNew, NULL);

                if (fChanged)
                    break;

                PPDMASYNCCOMPLETIONTASK pTaskHead = (PPDMASYNCCOMPLETIONTASK)ASMAtomicXchgPtr((void * volatile *)&pEndpointClass->apTaskCache[iSlot], NULL);

                /* The new task could be taken inbetween */
                if (pTaskHead)
                {
                    /* Go to the end of the probably much shorter new list. */
                    PPDMASYNCCOMPLETIONTASK pTaskTail = pTaskHead;
                    while (pTaskTail->pNext)
                        pTaskTail = pTaskTail->pNext;

                    /* Concatenate */
                    pTaskTail->pNext = pTaskHeadNew;

                    pTaskHeadNew = pTaskHead;
                }
                /* Another round trying to change the list. */
            }
            /* We got a task from the global cache so decrement the counter */
            ASMAtomicDecU32(&pEndpointClass->cTasksCached);
        }
    }
    else
    {
        /* Grab a free task from the head. */
        AssertMsg(pEndpoint->cTasksCached > 0, ("No tasks cached but list contain more than one element\n"));

        pTask = pEndpoint->pTasksFreeHead;
        pEndpoint->pTasksFreeHead = pTask->pNext;
        ASMAtomicDecU32(&pEndpoint->cTasksCached);
    }

    if (RT_LIKELY(pTask))
    {
        /* Get ID of the task. */
        pTask->uTaskId   = ASMAtomicIncU32(&pEndpoint->uTaskIdNext);

        /* Initialize common parts. */
        pTask->pvUser    = pvUser;
        pTask->pEndpoint = pEndpoint;
        /* Clear list pointers for safety. */
        pTask->pPrev     = NULL;
        pTask->pNext     = NULL;
    }

    return pTask;
}

/**
 * Puts a task in one of the caches.
 *
 * @returns nothing.
 * @param   pEndpoint    The endpoint the task belongs to.
 * @param   pTask        The task to cache.
 * @param   fLocal       Whether the per endpoint cache should be tried first.
 */
static void pdmR3AsyncCompletionPutTask(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, PPDMASYNCCOMPLETIONTASK pTask, bool fLocal)
{
    PPDMASYNCCOMPLETIONEPCLASS pEndpointClass = pEndpoint->pEpClass;

    /* Check whether we can use the per endpoint cache */
    if (   fLocal
        && (pEndpoint->cTasksCached < pEndpointClass->cEndpointCacheSize))
    {
        /* Add it to the list. */
        pTask->pPrev = NULL;
        pEndpoint->pTasksFreeTail->pNext = pTask;
        pEndpoint->pTasksFreeTail        = pTask;
        ASMAtomicIncU32(&pEndpoint->cTasksCached);
    }
    else if (ASMAtomicReadU32(&pEndpoint->cTasksCached) < pEndpointClass->cEpClassCacheSize)
    {
        /* Use the global cache. */
        ASMAtomicIncU32(&pEndpointClass->cTasksCached);

        PPDMASYNCCOMPLETIONTASK pNext;
        do
        {
            pNext = pEndpointClass->apTaskCache[pEndpoint->iSlotStart];
            pTask->pNext = pNext;
        } while (!ASMAtomicCmpXchgPtr((void * volatile *)&pEndpointClass->apTaskCache[pEndpoint->iSlotStart], (void *)pTask, (void *)pNext));
    }
    else
    {
        /* Free it */
        MMR3HeapFree(pTask);
    }
}

VMMR3DECL(int) PDMR3AsyncCompletionEpCreateForFile(PPPDMASYNCCOMPLETIONENDPOINT ppEndpoint,
                                                   const char *pszFilename, uint32_t fFlags,
                                                   PPDMASYNCCOMPLETIONTEMPLATE pTemplate)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc((": ppEndpoint=%p pszFilename=%p{%s} fFlags=%u pTemplate=%p\n",
                 ppEndpoint, pszFilename, pszFilename, fFlags, pTemplate));

    /* Sanity checks. */
    AssertReturn(VALID_PTR(ppEndpoint),  VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(pszFilename), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(pTemplate),   VERR_INVALID_POINTER);

    /* Check that the flags are valid. */
    AssertReturn(((~(PDMACEP_FILE_FLAGS_READ_ONLY | PDMACEP_FILE_FLAGS_CACHING) & fFlags) == 0),
                 VERR_INVALID_PARAMETER);

    PVM pVM = pTemplate->pVM;
    PPDMASYNCCOMPLETIONEPCLASS  pEndpointClass = pVM->pdm.s.papAsyncCompletionEndpointClass[PDMASYNCCOMPLETIONEPCLASSTYPE_FILE];
    PPDMASYNCCOMPLETIONENDPOINT pEndpoint = NULL;

    AssertMsg(pEndpointClass, ("File endpoint class was not initialized\n"));

    rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                          pEndpointClass->pEndpointOps->cbEndpoint,
                          (void **)&pEndpoint);
    if (RT_SUCCESS(rc))
    {

        /* Initialize common parts. */
        pEndpoint->pNext             = NULL;
        pEndpoint->pPrev             = NULL;
        pEndpoint->pEpClass          = pEndpointClass;
        pEndpoint->pTasksFreeHead    = NULL;
        pEndpoint->pTasksFreeTail    = NULL;
        pEndpoint->cTasksCached      = 0;
        pEndpoint->uTaskIdNext       = 0;
        pEndpoint->fTaskIdWraparound = false;
        pEndpoint->pTemplate         = pTemplate;
        pEndpoint->iSlotStart        = pEndpointClass->cEndpoints % RT_ELEMENTS(pEndpointClass->apTaskCache);

        /* Init the cache. */
        rc = MMR3HeapAllocZEx(pVM, MM_TAG_PDM_ASYNC_COMPLETION,
                              pEndpointClass->pEndpointOps->cbTask,
                              (void **)&pEndpoint->pTasksFreeHead);
        if (RT_SUCCESS(rc))
        {
            pEndpoint->pTasksFreeTail = pEndpoint->pTasksFreeHead;

            /* Call the initializer for the endpoint. */
            rc = pEndpointClass->pEndpointOps->pfnEpInitialize(pEndpoint, pszFilename, fFlags);
            if (RT_SUCCESS(rc))
            {
                /* Link it into the list of endpoints. */
                rc = RTCritSectEnter(&pEndpointClass->CritSect);
                AssertMsg(RT_SUCCESS(rc), ("Failed to enter critical section rc=%Rrc\n", rc));

                pEndpoint->pNext = pEndpointClass->pEndpointsHead;
                if (pEndpointClass->pEndpointsHead)
                    pEndpointClass->pEndpointsHead->pPrev = pEndpoint;

                pEndpointClass->pEndpointsHead = pEndpoint;
                pEndpointClass->cEndpoints++;

                rc = RTCritSectLeave(&pEndpointClass->CritSect);
                AssertMsg(RT_SUCCESS(rc), ("Failed to enter critical section rc=%Rrc\n", rc));

                /* Reference the template. */
                ASMAtomicIncU32(&pTemplate->cUsed);

                *ppEndpoint = pEndpoint;

                LogFlowFunc((": Created endpoint for %s: rc=%Rrc\n", pszFilename, rc));
                return VINF_SUCCESS;
            }
            MMR3HeapFree(pEndpoint->pTasksFreeHead);
        }
        MMR3HeapFree(pEndpoint);
    }

    LogFlowFunc((": Creation of endpoint for %s failed: rc=%Rrc\n", pszFilename, rc));

    return rc;
}

VMMR3DECL(void) PDMR3AsyncCompletionEpClose(PPDMASYNCCOMPLETIONENDPOINT pEndpoint)
{
    LogFlowFunc((": pEndpoint=%p\n", pEndpoint));

    /* Sanity checks. */
    AssertReturnVoid(VALID_PTR(pEndpoint));

    PPDMASYNCCOMPLETIONEPCLASS pEndpointClass = pEndpoint->pEpClass;
    pEndpointClass->pEndpointOps->pfnEpClose(pEndpoint);

    /* Free cached tasks. */
    PPDMASYNCCOMPLETIONTASK pTask = pEndpoint->pTasksFreeHead;

    while (pTask)
    {
        PPDMASYNCCOMPLETIONTASK pTaskFree = pTask;
        pTask = pTask->pNext;
        MMR3HeapFree(pTaskFree);
    }

    /* Drop reference from the template. */
    ASMAtomicDecU32(&pEndpoint->pTemplate->cUsed);

    /* Unlink the endpoint from the list. */
    int rc = RTCritSectEnter(&pEndpointClass->CritSect);
    AssertMsg(RT_SUCCESS(rc), ("Failed to enter critical section rc=%Rrc\n", rc));

    PPDMASYNCCOMPLETIONENDPOINT pEndpointNext = pEndpoint->pNext;
    PPDMASYNCCOMPLETIONENDPOINT pEndpointPrev = pEndpoint->pPrev;

    if (pEndpointPrev)
        pEndpointPrev->pNext = pEndpointNext;
    else
        pEndpointClass->pEndpointsHead = pEndpointNext;
    if (pEndpointNext)
        pEndpointNext->pPrev = pEndpointPrev;

    pEndpointClass->cEndpoints--;

    rc = RTCritSectLeave(&pEndpointClass->CritSect);
    AssertMsg(RT_SUCCESS(rc), ("Failed to enter critical section rc=%Rrc\n", rc));

    MMR3HeapFree(pEndpoint);
}

VMMR3DECL(int) PDMR3AsyncCompletionEpRead(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                          PCPDMDATASEG paSegments, size_t cSegments,
                                          size_t cbRead, void *pvUser,
                                          PPPDMASYNCCOMPLETIONTASK ppTask)
{
    int rc = VINF_SUCCESS;

    AssertReturn(VALID_PTR(pEndpoint), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(paSegments), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(ppTask), VERR_INVALID_POINTER);
    AssertReturn(cSegments > 0, VERR_INVALID_PARAMETER);
    AssertReturn(cbRead > 0, VERR_INVALID_PARAMETER);
    AssertReturn(off >= 0, VERR_INVALID_PARAMETER);

    PPDMASYNCCOMPLETIONTASK pTask;

    pTask = pdmR3AsyncCompletionGetTask(pEndpoint, pvUser);
    if (!pTask)
        return VERR_NO_MEMORY;

    rc = pEndpoint->pEpClass->pEndpointOps->pfnEpRead(pTask, pEndpoint, off,
                                                      paSegments, cSegments, cbRead);
    if (RT_SUCCESS(rc))
    {
        *ppTask = pTask;
    }
    else
        pdmR3AsyncCompletionPutTask(pEndpoint, pTask, false);

    return rc;
}

VMMR3DECL(int) PDMR3AsyncCompletionEpWrite(PPDMASYNCCOMPLETIONENDPOINT pEndpoint, RTFOFF off,
                                           PCPDMDATASEG paSegments, size_t cSegments,
                                           size_t cbWrite, void *pvUser,
                                           PPPDMASYNCCOMPLETIONTASK ppTask)
{
    int rc = VINF_SUCCESS;

    AssertReturn(VALID_PTR(pEndpoint), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(paSegments), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(ppTask), VERR_INVALID_POINTER);
    AssertReturn(cSegments > 0, VERR_INVALID_PARAMETER);
    AssertReturn(cbWrite > 0, VERR_INVALID_PARAMETER);
    AssertReturn(off >= 0, VERR_INVALID_PARAMETER);

    PPDMASYNCCOMPLETIONTASK pTask;

    pTask = pdmR3AsyncCompletionGetTask(pEndpoint, pvUser);
    if (!pTask)
        return VERR_NO_MEMORY;

    rc = pEndpoint->pEpClass->pEndpointOps->pfnEpWrite(pTask, pEndpoint, off,
                                                       paSegments, cSegments, cbWrite);
    if (RT_SUCCESS(rc))
    {
        *ppTask = pTask;
    }
    else
        pdmR3AsyncCompletionPutTask(pEndpoint, pTask, false);

    return rc;
}

VMMR3DECL(int) PDMR3AsyncCompletionEpFlush(PPDMASYNCCOMPLETIONENDPOINT pEndpoint,
                                           void *pvUser,
                                           PPPDMASYNCCOMPLETIONTASK ppTask)
{
    int rc = VINF_SUCCESS;

    AssertReturn(VALID_PTR(pEndpoint), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(ppTask), VERR_INVALID_POINTER);

    PPDMASYNCCOMPLETIONTASK pTask;

    pTask = pdmR3AsyncCompletionGetTask(pEndpoint, pvUser);
    if (!pTask)
        return VERR_NO_MEMORY;

    rc = pEndpoint->pEpClass->pEndpointOps->pfnEpFlush(pTask, pEndpoint);
    if (RT_SUCCESS(rc))
    {
        *ppTask = pTask;
    }
    else
        pdmR3AsyncCompletionPutTask(pEndpoint, pTask, false);

    return rc;
}

VMMR3DECL(int) PDMR3AsyncCompletionEpGetSize(PPDMASYNCCOMPLETIONENDPOINT pEndpoint,
                                             uint64_t *pcbSize)
{
    AssertReturn(VALID_PTR(pEndpoint), VERR_INVALID_POINTER);
    AssertReturn(VALID_PTR(pcbSize), VERR_INVALID_POINTER);

    return pEndpoint->pEpClass->pEndpointOps->pfnEpGetSize(pEndpoint, pcbSize);
}

VMMR3DECL(int) PDMR3AsyncCompletionTaskCancel(PPDMASYNCCOMPLETIONTASK pTask)
{
    return VERR_NOT_IMPLEMENTED;
}


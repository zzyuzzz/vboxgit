/** @file
 *
 * Shared Information Services:
 * Host service entry points.
 */

/*
 * Copyright (C) 2008 Sun Microsystems, Inc.
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

/**
 * An HGCM service for passing requests which do not need any persistant state
 * to handle.  We currently only support three types of request - set guest
 * property (SET_CONFIG_KEY and SET_CONFIG_KEY_HOST), get guest property
 * (GET_CONFIG_KEY and GET_CONFIG_KEY_HOST) and remove guest property
 * (DEL_CONFIG_KEY and DEL_CONFIG_KEY_HOST).  These may be used to read, to
 * write and to remove configuration information which is available to
 * both guest and host.  This configuration information is stored in a CFGM
 * node using the CFGM APIs.  It is the responsibility of whoever creates the
 * service to create this node and to tell the service about it using the
 * SET_CFGM_NODE host call.  It is also the responsibility of the service
 * creator to save this information to disk and to retrieve it when needed.
 * Since the CFGM APIs are single threaded, the creator must also ensure that
 * no-one else accesses the configuration node while the service is running.
 *
 * If this service is extended to deal with new requests it would probably be a
 * good idea to split it up into several files.
 */

#define LOG_GROUP LOG_GROUP_HGCM

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/HostServices/VBoxInfoSvc.h>

#include <memory>  /* for auto_ptr */

#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <VBox/log.h>

#include <VBox/cfgm.h>

#include "noncopyable.h"

/*******************************************************************************
*   Internal functions                                                         *
*******************************************************************************/
/** Extract a pointer value from an HGCM parameter structure */
static int VBoxHGCMParmPtrGet (VBOXHGCMSVCPARM *pParm, void **ppv, uint32_t *pcb)
{
    if (pParm->type == VBOX_HGCM_SVC_PARM_PTR)
    {
        *ppv = pParm->u.pointer.addr;
        *pcb = pParm->u.pointer.size;
        return VINF_SUCCESS;
    }

    return VERR_INVALID_PARAMETER;
}

/** Set a uint32_t value to an HGCM parameter structure */
static void VBoxHGCMParmUInt32Set (VBOXHGCMSVCPARM *pParm, uint32_t u32)
{
    pParm->type = VBOX_HGCM_SVC_PARM_32BIT;
    pParm->u.uint32 = u32;
}


namespace svcInfo {

/**
 * Class containing the shared information service functionality.
 */
class Service : public noncopyable
{
private:
    /** Type definition for use in callback functions */
    typedef Service SELF;
    /** HGCM helper functions. */
    PVBOXHGCMSVCHELPERS mpHelpers;
    /** Pointer to our configuration node. */
    PCFGMNODE mpNode;

public:
    explicit Service(PVBOXHGCMSVCHELPERS pHelpers)
                  : mpHelpers(pHelpers), mpNode(NULL) {}

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnUnload
     * Simply deletes the service object
     */
    static DECLCALLBACK(int) svcUnload (void *pvService)
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        delete pSelf;
        return VINF_SUCCESS;
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnConnect
     * Stub implementation of pfnConnect and pfnDisconnect.
     */
    static DECLCALLBACK(int) svcConnectDisconnect (void * /* pvService */,
                                                   uint32_t /* u32ClientID */,
                                                   void * /* pvClient */)
    {
        return VINF_SUCCESS;
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnCall
     * Wraps to the call member function
     */
    static DECLCALLBACK(void) svcCall (void * pvService,
                                       VBOXHGCMCALLHANDLE callHandle,
                                       uint32_t u32ClientID,
                                       void *pvClient,
                                       uint32_t u32Function,
                                       uint32_t cParms,
                                       VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturnVoid(VALID_PTR(pvService));
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        pSelf->call(callHandle, u32ClientID, pvClient, u32Function, cParms, paParms);
    }

    /**
     * @copydoc VBOXHGCMSVCHELPERS::pfnHostCall
     * Wraps to the hostCall member function
     */
    static DECLCALLBACK(int) svcHostCall (void *pvService,
                                          uint32_t u32Function,
                                          uint32_t cParms,
                                          VBOXHGCMSVCPARM paParms[])
    {
        AssertLogRelReturn(VALID_PTR(pvService), VERR_INVALID_PARAMETER);
        SELF *pSelf = reinterpret_cast<SELF *>(pvService);
        return pSelf->hostCall(u32Function, cParms, paParms);
    }
private:
    int validateKey(const char *pszKey, uint32_t cbKey);
    int validateValue(char *pszValue, uint32_t cbValue);
    int getKey(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int setKey(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    int delKey(uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
    void call (VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID,
               void *pvClient, uint32_t eFunction, uint32_t cParms,
               VBOXHGCMSVCPARM paParms[]);
    int hostCall (uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[]);
};


/**
 * Checking that the key passed by the guest fits our criteria for a
 * configuration key
 *
 * @returns IPRT status code
 * @param   pszKey    the key passed by the guest
 * @param   cbKey     the number of bytes pszKey points to, including the
 *                    terminating '\0'
 * @thread  HGCM
 */
int Service::validateKey(const char *pszKey, uint32_t cbKey)
{
    LogFlowFunc(("cbKey=%d\n", cbKey));

    size_t count;
    int rc = VINF_SUCCESS;

    /* Validate the format of the key. */
    /* Only accept names in valid Utf8. */
    rc = RTStrValidateEncodingEx(pszKey, cbKey, 0);
    if (RT_SUCCESS(rc))
        /* We want the byte length, not the Utf8 length */
        count = strlen(pszKey);
    if (RT_SUCCESS(rc) && (count > KEY_MAX_LEN))
        rc = VERR_INVALID_PARAMETER;

    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}


/**
 * Check that the data passed by the guest fits our criteria for the value of
 * a configuration key
 *
 * @returns IPRT status code
 * @param   pszValue  the value to store in the key
 * @param   cbValue   the number of bytes in the buffer pszValue points to
 * @thread  HGCM
 */
int Service::validateValue(char *pszValue, uint32_t cbValue)
{
    LogFlowFunc(("cbValue=%d\n", cbValue));

    size_t count;
    int rc = VINF_SUCCESS;

    /* Validate the format of the value. */
    /* Only accept values in valid Utf8 */
    rc = RTStrValidateEncodingEx(pszValue, cbValue, 0);
    if (RT_SUCCESS(rc))
        /* We want the byte length, not the Utf8 length */
        count = strlen(pszValue);
    if (RT_SUCCESS(rc) && (count > KEY_MAX_VALUE_LEN))
        rc = VERR_INVALID_PARAMETER;

    if (RT_SUCCESS(rc))
        LogFlow(("    pszValue=%s\n", cbValue > 0 ? pszValue : NULL));
    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}


/**
 * Retrieve a value from the guest registry by key, checking the validity
 * of the arguments passed.  If the guest has not allocated enough buffer
 * space for the value then we return VERR_OVERFLOW and set the size of the
 * buffer needed in the "size" HGCM parameter.  If the key was not found at
 * all, we return VERR_NOT_FOUND.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 */
int Service::getKey(uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    char *pszKey, *pszValue;
    uint32_t cbKey, cbValue;
    size_t cbValueActual;

    LogFlowThisFunc(("\n"));
    if (   (cParms != 3)  /* Hardcoded value as the next lines depend on it. */
        || (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)   /* key */
        || (paParms[1].type != VBOX_HGCM_SVC_PARM_PTR)   /* value */
       )
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        rc = VBoxHGCMParmPtrGet(&paParms[0], (void **) &pszKey, &cbKey);
    if (RT_SUCCESS(rc))
        rc = VBoxHGCMParmPtrGet(&paParms[1], (void **) &pszValue, &cbValue);
    if (RT_SUCCESS(rc))
        rc = validateKey(pszKey, cbKey);
    if (RT_SUCCESS(rc))
        rc = CFGMR3QuerySize(mpNode, pszKey, &cbValueActual);
    if (RT_SUCCESS(rc))
        VBoxHGCMParmUInt32Set(&paParms[2], cbValueActual);
    if (RT_SUCCESS(rc) && (cbValueActual > cbValue))
        rc = VERR_BUFFER_OVERFLOW;
    if (RT_SUCCESS(rc) && (rc != VINF_BUFFER_OVERFLOW))
        rc = CFGMR3QueryString(mpNode, pszKey, pszValue, cbValue);
    if (RT_SUCCESS(rc) && (rc != VINF_BUFFER_OVERFLOW))
        Log2(("Queried string %s, rc=%Rrc, value=%.*s\n", pszKey, rc, cbValue, pszValue));
    else if (VERR_CFGM_VALUE_NOT_FOUND == rc)
        rc = VERR_NOT_FOUND;
    LogFlowThisFunc(("rc = %Rrc\n", rc));
    return rc;
}


/**
 * Set a value in the guest registry by key, checking the validity
 * of the arguments passed.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 */
int Service::setKey(uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    char *pszKey, *pszValue;
    uint32_t cbKey, cbValue;

    LogFlowThisFunc(("\n"));
    if (   (cParms != 2)  /* Hardcoded value as the next lines depend on it. */
        || (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)   /* key */
        || (paParms[1].type != VBOX_HGCM_SVC_PARM_PTR)   /* value */
       )
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        rc = VBoxHGCMParmPtrGet(&paParms[0], (void **) &pszKey, &cbKey);
    if (RT_SUCCESS(rc))
        rc = VBoxHGCMParmPtrGet(&paParms[1], (void **) &pszValue, &cbValue);
    if (RT_SUCCESS(rc))
        rc = validateKey(pszKey, cbKey);
    if (RT_SUCCESS(rc))
        rc = validateValue(pszValue, cbValue);
    if (RT_SUCCESS(rc))
    {
        /* Limit the number of keys that we can set. */
        unsigned cChildren = 0;
        for (PCFGMNODE pChild = CFGMR3GetFirstChild(mpNode); pChild != 0; pChild = CFGMR3GetNextChild(pChild))
            ++cChildren;
        if (cChildren >= KEY_MAX_KEYS)
            rc = VERR_TOO_MUCH_DATA;
    }
    if (RT_SUCCESS(rc))
    {
        CFGMR3RemoveValue(mpNode, pszKey);
        if (pszValue > 0)
            rc = CFGMR3InsertString(mpNode, pszKey, pszValue);
    }
    if (RT_SUCCESS(rc))
        Log2(("Set string %s, rc=%Rrc, value=%s\n", pszKey, rc, pszValue));
    LogFlowThisFunc(("rc = %Rrc\n", rc));
    return rc;
}


/**
 * Remove a value in the guest registry by key, checking the validity
 * of the arguments passed.
 *
 * @returns iprt status value
 * @param   cParms  the number of HGCM parameters supplied
 * @param   paParms the array of HGCM parameters
 * @thread  HGCM
 */
int Service::delKey(uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    char *pszKey;
    uint32_t cbKey;

    LogFlowThisFunc(("\n"));
    if (   (cParms != 1)  /* Hardcoded value as the next lines depend on it. */
        || (paParms[0].type != VBOX_HGCM_SVC_PARM_PTR)   /* key */
       )
        rc = VERR_INVALID_PARAMETER;
    if (RT_SUCCESS(rc))
        rc = VBoxHGCMParmPtrGet(&paParms[0], (void **) &pszKey, &cbKey);
    if (RT_SUCCESS(rc))
        rc = validateKey(pszKey, cbKey);
    if (RT_SUCCESS(rc))
        CFGMR3RemoveValue(mpNode, pszKey);
    LogFlowThisFunc(("rc = %Rrc\n", rc));
    return rc;
}


/**
 * Handle an HGCM service call.
 * @copydoc VBOXHGCMSVCFNTABLE::pfnCall
 * @note    All functions which do not involve an unreasonable delay will be
 *          handled synchronously.  If needed, we will add a request handler
 *          thread in future for those which do.
 *
 * @thread  HGCM
 */
void Service::call (VBOXHGCMCALLHANDLE callHandle, uint32_t u32ClientID,
                    void * /* pvClient */, uint32_t eFunction, uint32_t cParms,
                    VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;
    bool fCallSync = true;

    LogFlowFunc(("u32ClientID = %d, fn = %d, cParms = %d, pparms = %d\n",
                 u32ClientID, eFunction, cParms, paParms));

    switch (eFunction)
    {
        /* The guest wishes to read a configuration value */
        case GET_CONFIG_KEY:
            LogFlowFunc(("GET_CONFIG_KEY\n"));
            rc = getKey(cParms, paParms);
            break;

        /* The guest wishes to set a configuration value */
        case SET_CONFIG_KEY:
            LogFlowFunc(("SET_CONFIG_KEY\n"));
            rc = setKey(cParms, paParms);
            break;

        /* The guest wishes to remove a configuration value */
        case DEL_CONFIG_KEY:
            LogFlowFunc(("DEL_CONFIG_KEY\n"));
            rc = delKey(cParms, paParms);
            break;

        default:
            rc = VERR_NOT_IMPLEMENTED;
    }
    if (fCallSync)
    {
        LogFlowFunc(("rc = %Rrc\n", rc));
        mpHelpers->pfnCallComplete (callHandle, rc);
    }
}


/**
 * Service call handler for the host.
 * @copydoc VBOXHGCMSVCFNTABLE::pfnHostCall
 * @thread  hgcm
 */
int Service::hostCall (uint32_t eFunction, uint32_t cParms, VBOXHGCMSVCPARM paParms[])
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("fn = %d, cParms = %d, pparms = %d\n",
                 eFunction, cParms, paParms));

    switch (eFunction)
    {
        /* Set the root CFGM node used.  This should be called when instantiating
         * the service. */
        case SET_CFGM_NODE:
        {
            LogFlowFunc(("SET_CFGM_NODE\n"));

            if (cParms != 1)
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else if (   paParms[0].type != VBOX_HGCM_SVC_PARM_PTR   /* pNode */
                    )
            {
                rc = VERR_INVALID_PARAMETER;
            }
            else
            {
                PCFGMNODE pNode = NULL;
                uint32_t cbDummy;

                rc = VBoxHGCMParmPtrGet (&paParms[0], (void **) &pNode, &cbDummy);
                mpNode = pNode;
            }
        } break;

        /* The host wishes to read a configuration value */
        case GET_CONFIG_KEY_HOST:
            LogFlowFunc(("GET_CONFIG_KEY_HOST\n"));
            rc = getKey(cParms, paParms);
            break;

        /* The host wishes to set a configuration value */
        case SET_CONFIG_KEY_HOST:
            LogFlowFunc(("SET_CONFIG_KEY_HOST\n"));
            rc = setKey(cParms, paParms);
            break;

        /* The host wishes to remove a configuration value */
        case DEL_CONFIG_KEY_HOST:
            LogFlowFunc(("DEL_CONFIG_KEY_HOST\n"));
            rc = delKey(cParms, paParms);
            break;

        default:
            rc = VERR_NOT_SUPPORTED;
            break;
    }

    LogFlowFunc(("rc = %Vrc\n", rc));
    return rc;
}

} /* namespace svcInfo */

using svcInfo::Service;

/**
 * @copydoc VBOXHGCMSVCLOAD
 */
extern "C" DECLCALLBACK(DECLEXPORT(int)) VBoxHGCMSvcLoad (VBOXHGCMSVCFNTABLE *ptable)
{
    int rc = VINF_SUCCESS;

    LogFlowFunc(("ptable = %p\n", ptable));

    if (!VALID_PTR(ptable))
    {
        rc = VERR_INVALID_PARAMETER;
    }
    else
    {
        LogFlowFunc(("ptable->cbSize = %d, ptable->u32Version = 0x%08X\n", ptable->cbSize, ptable->u32Version));

        if (   ptable->cbSize != sizeof (VBOXHGCMSVCFNTABLE)
            || ptable->u32Version != VBOX_HGCM_SVC_VERSION)
        {
            rc = VERR_VERSION_MISMATCH;
        }
        else
        {
            std::auto_ptr<Service> apService;
            try {
                apService = std::auto_ptr<Service>(new Service(ptable->pHelpers));
            } catch (...) {
                /* No exceptions may propogate outside. */
                rc = VERR_UNRESOLVED_ERROR;
            }

            if (RT_SUCCESS(rc))
            {
                /* We do not maintain connections, so no client data is needed. */
                ptable->cbClient = 0;

                ptable->pfnUnload     = Service::svcUnload;
                ptable->pfnConnect    = Service::svcConnectDisconnect;
                ptable->pfnDisconnect = Service::svcConnectDisconnect;
                ptable->pfnCall       = Service::svcCall;
                ptable->pfnHostCall   = Service::svcHostCall;
                ptable->pfnSaveState  = NULL;  /* The service is stateless by definition, so the */
                ptable->pfnLoadState  = NULL;  /* normal construction done before restoring suffices */
                ptable->pfnRegisterExtension  = NULL;

                /* Service specific initialization. */
                ptable->pvService = apService.release();
            }
        }
    }

    LogFlowFunc(("returning %Rrc\n", rc));
    return rc;
}

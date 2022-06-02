/** @file
 *
 * VirtualBox Remote USB backend
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

#define LOG_GROUP LOG_GROUP_DEV_USB
#include "ConsoleImpl.h"
#include "ConsoleVRDPServer.h"
#include "RemoteUSBBackend.h"
#include "RemoteUSBDeviceImpl.h"

#include <VBox/vrdpapi.h>
#include <VBox/vrdpusb.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <VBox/vusb.h>

#include <iprt/time.h>

/** @page pg_vrdb_usb           Async Remote USB
 *
 *
 * USB backend functions are called in EMT so care must be taken to prevent
 * delays in the functions execution.
 *
 * Among 11 backend functions 10 just return a success indicator.
 *
 * Such a function usually will check pending error code and if everything is ok,
 * submit asynchronous RDP request and return success immediately.
 *
 * On actual completion of each request, the status will be saved as
 * pending, so in case of an error all further functions will fail with
 * device disconnected condition.
 * @todo May be a device disconnect notification for console is required?
 *
 * The only remaining function that needs special processing is
 * the reap_urb. It has a timeout parameter.
 * Normally, the timeout is 0, as result of polling from VUSB frame timer.
 * It is ok for async processing, the backend will periodically reap urbs from client.
 * And already reaped URBs from client will be returned for the call.
 * Exceptions:
 * 1) during device initialization, when obtaining device descriptions
 * the timeout is -1, and the request is expected to be processed synchronously.
 * It looks like only 3 URBs with some information are retrieved that way.
 * Probably, one can return this information in DEVICE_LIST together with the
 * device description and when such request are submitted, just return
 * the prefetched data.
 * 2) during suspend timeout is non zero (10 or less milliseconds),
 * and URB's are reaped for about 1 second. But here network delays
 * will not affect the timeout, so it is ok.
 *
 *
 * @subsection sub_vrdb_usb_dad  Device attaching/detaching
 *
 * Devices are attached when client is connected or when a new device is connected to client.
 * Devices are detached when client is disconnected (all devices) or a device is disconnected
 * the client side.
 *
 * The backend polls the client for list of attached USB devices from RemoteUSBThread.
 *
 */

/* Queued URB submitted to VRDP client. */
typedef struct _REMOTEUSBQURB
{
    struct _REMOTEUSBQURB *next;
    struct _REMOTEUSBQURB *prev;

    PREMOTEUSBDEVICE pDevice;          /* Device, the URB is queued for. */

    uint32_t  u32Handle;               /* The handle of the URB. Generated by the Remote USB backend. */

    void     *pvData;                  /* Pointer to URB data allocated by VUSB. */
    void     *pvURB;                   /* Pointer to URB known to VUSB. */

    uint32_t  u32Len;                  /* Data length returned by the VRDP client. */
    uint32_t  u32Err;                  /* URB error code returned by the VRDP client. */

    bool      fCompleted;              /* The URB has been returned back by VRDP client. */
    bool      fInput;                  /* This URB receives data from the client. */

    uint32_t  u32TransferredLen;       /* For VRDP_USB_DIRECTION_OUT URBs = bytes written.
                                        * For VRDP_USB_DIRECTION_IN URBs = bytes received.
                                        */
} REMOTEUSBQURB;

/* Remote USB device instance data. */
typedef struct _REMOTEUSBDEVICE
{
    struct _REMOTEUSBDEVICE *prev;
    struct _REMOTEUSBDEVICE *next;

    RemoteUSBBackend *pOwner;

    VRDPUSBDEVID      id;              /* The remote identifier, assigned by client. */

    uint32_t          u32ClientId;     /* The identifier of the remote client. */

    REMOTEUSBQURB    *pHeadQURBs;      /* List of URBs queued for the device. */
    REMOTEUSBQURB    *pTailQURBs;

    volatile uint32_t hURB;            /* Source for URB's handles. */
    bool              fFailed;         /* True if an operation has failed for the device. */
    RTCRITSECT        critsect;        /* Protects the queued urb list. */
} REMOTEUSBDEVICE;



static void requestDevice(REMOTEUSBDEVICE *pDevice)
{
    int rc = RTCritSectEnter(&pDevice->critsect);
    AssertRC(rc);
}

static void releaseDevice(REMOTEUSBDEVICE *pDevice)
{
    RTCritSectLeave(&pDevice->critsect);
}

static REMOTEUSBQURB *qurbAlloc(PREMOTEUSBDEVICE pDevice)
{
    /* @todo reuse URBs. */
    REMOTEUSBQURB *pQURB = (REMOTEUSBQURB *)RTMemAllocZ (sizeof (REMOTEUSBQURB));

    if (pQURB)
    {
        pQURB->pDevice = pDevice;
    }

    return pQURB;
}

static void qurbFree (REMOTEUSBQURB *pQURB)
{
     RTMemFree (pQURB);
     return;
}


/* Called by VRDP server when the client responds to a request on USB channel. */
DECLCALLBACK(int) USBClientResponseCallback (void *pv, uint32_t u32ClientId, uint8_t code, const void *pvRet, uint32_t cbRet)
{
    int rc = VINF_SUCCESS;

    LogFlow(("USBClientResponseCallback: id = %d, pv = %p, code = %d, pvRet = %p, cbRet = %d\n",
              u32ClientId, pv, code, pvRet, cbRet));

    RemoteUSBBackend *pThis = (RemoteUSBBackend *)pv;

    switch (code)
    {
        case VRDP_USB_REQ_DEVICE_LIST:
        {
            rc = pThis->saveDeviceList (pvRet, cbRet);
        } break;

        case VRDP_USB_REQ_NEGOTIATE:
        {
            if (pvRet && cbRet >= sizeof (VRDPUSBREQNEGOTIATERET))
            {
                VRDPUSBREQNEGOTIATERET *pret = (VRDPUSBREQNEGOTIATERET *)pvRet;

                rc = pThis->negotiateResponse (pret, cbRet);
            }
            else
            {
                Log(("USBClientResponseCallback: WARNING: not enough data in response: pv = %p, cb = %d, expected %d.\n",
                     pvRet, cbRet, sizeof (VRDPUSBREQNEGOTIATERET)));

                rc = VERR_INVALID_PARAMETER;
            }
        } break;

        case VRDP_USB_REQ_REAP_URB:
        {
            rc = pThis->reapURB (pvRet, cbRet);

            LogFlow(("USBClientResponseCallback: reap URB, rc = %Rrc.\n", rc));
        } break;

        case VRDP_USB_REQ_QUEUE_URB:
        case VRDP_USB_REQ_CLOSE:
        case VRDP_USB_REQ_CANCEL_URB:
        {
            /* Do nothing, actually this should not happen. */
            Log(("USBClientResponseCallback: WARNING: response to a request %d is not expected!!!\n", code));
        } break;

        case VRDP_USB_REQ_OPEN:
        case VRDP_USB_REQ_RESET:
        case VRDP_USB_REQ_SET_CONFIG:
        case VRDP_USB_REQ_CLAIM_INTERFACE:
        case VRDP_USB_REQ_RELEASE_INTERFACE:
        case VRDP_USB_REQ_INTERFACE_SETTING:
        case VRDP_USB_REQ_CLEAR_HALTED_EP:
        {
            /*
             * Device specific responses with status codes.
             */
            if (pvRet && cbRet >= sizeof (VRDPUSBREQRETHDR))
            {
                VRDPUSBREQRETHDR *pret = (VRDPUSBREQRETHDR *)pvRet;

                if (pret->status != VRDP_USB_STATUS_SUCCESS)
                {
                    REMOTEUSBDEVICE *pDevice = pThis->deviceFromId (pret->id);

                    if (!pDevice)
                    {
                        Log(("USBClientResponseCallback: WARNING: invalid device id %08X.\n", pret->id));
                        rc = VERR_INVALID_PARAMETER;
                    }
                    else
                    {
                        Log(("USBClientResponseCallback: WARNING: the operation failed, status %d\n", pret->status));
                        pDevice->fFailed = true;
                    }
                }
            }
            else
            {
                Log(("USBClientResponseCallback: WARNING: not enough data in response: pv = %p, cb = %d, expected %d.\n",
                     pvRet, cbRet, sizeof (VRDPUSBREQRETHDR)));
            }
        } break;

        default:
        {
            Log(("USBClientResponseCallback: WARNING: invalid code %d\n", code));
        } break;
    }

    return rc;
}

/*
 * Backend entry points.
 */
static DECLCALLBACK(int) iface_Open (PREMOTEUSBBACKEND pInstance, const char *pszAddress, size_t cbAddress, PREMOTEUSBDEVICE *ppDevice)
{
    int rc = VINF_SUCCESS;

    RemoteUSBBackend *pThis = (RemoteUSBBackend *)pInstance;

    REMOTEUSBDEVICE *pDevice = (REMOTEUSBDEVICE *)RTMemAllocZ (sizeof (REMOTEUSBDEVICE));

    if (!pDevice)
    {
        rc = VERR_NO_MEMORY;
    }
    else
    {
        /* Parse given address string to find the device identifier.
         * The format is "REMOTEUSB0xAAAABBBB&0xCCCCDDDD", where AAAABBBB is hex device identifier
         * and CCCCDDDD is hex client id.
         */
        if (strncmp (pszAddress, REMOTE_USB_BACKEND_PREFIX_S, REMOTE_USB_BACKEND_PREFIX_LEN) != 0)
        {
            AssertFailed();
            rc = VERR_INVALID_PARAMETER;
        }
        else
        {
            /* Initialize the device structure. */
            pDevice->pOwner = pThis;

            rc = RTCritSectInit(&pDevice->critsect);
            AssertRC(rc);

            if (RT_SUCCESS(rc))
            {
                pDevice->id = RTStrToUInt32 (&pszAddress[REMOTE_USB_BACKEND_PREFIX_LEN]);

                size_t l = strlen (pszAddress);

                if (l >= REMOTE_USB_BACKEND_PREFIX_LEN + strlen ("0x12345678&0x87654321"))
                {
                    const char *p = &pszAddress[REMOTE_USB_BACKEND_PREFIX_LEN + strlen ("0x12345678")];
                    if (*p == '&')
                    {
                        pDevice->u32ClientId = RTStrToUInt32 (p + 1);
                    }
                    else
                    {
                        AssertFailed ();
                        rc = VERR_INVALID_PARAMETER;
                    }
                }
                else
                {
                    AssertFailed ();
                    rc = VERR_INVALID_PARAMETER;
                }

                if (RT_SUCCESS(rc))
                {
                    VRDP_USB_REQ_OPEN_PARM parm;

                    parm.code = VRDP_USB_REQ_OPEN;
                    parm.id = pDevice->id;

                    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));
                }
            }
        }
    }

    if (RT_SUCCESS(rc))
    {
        *ppDevice = pDevice;

        pThis->addDevice (pDevice);
    }
    else
    {
        RTMemFree (pDevice);
    }

    return rc;
}

static DECLCALLBACK(void) iface_Close (PREMOTEUSBDEVICE pDevice)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    VRDP_USB_REQ_CLOSE_PARM parm;

    parm.code = VRDP_USB_REQ_CLOSE;
    parm.id = pDevice->id;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    pThis->removeDevice (pDevice);

    if (RTCritSectIsInitialized (&pDevice->critsect))
    {
        RTCritSectDelete (&pDevice->critsect);
    }

    RTMemFree (pDevice);

    return;
}

static DECLCALLBACK(int) iface_Reset (PREMOTEUSBDEVICE pDevice)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    VRDP_USB_REQ_RESET_PARM parm;

    parm.code = VRDP_USB_REQ_RESET;
    parm.id = pDevice->id;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) iface_SetConfig (PREMOTEUSBDEVICE pDevice, uint8_t u8Cfg)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    VRDP_USB_REQ_SET_CONFIG_PARM parm;

    parm.code = VRDP_USB_REQ_SET_CONFIG;
    parm.id = pDevice->id;
    parm.configuration = u8Cfg;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) iface_ClaimInterface (PREMOTEUSBDEVICE pDevice, uint8_t u8Ifnum)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    VRDP_USB_REQ_CLAIM_INTERFACE_PARM parm;

    parm.code = VRDP_USB_REQ_CLAIM_INTERFACE;
    parm.id = pDevice->id;
    parm.iface = u8Ifnum;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) iface_ReleaseInterface (PREMOTEUSBDEVICE pDevice, uint8_t u8Ifnum)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    VRDP_USB_REQ_RELEASE_INTERFACE_PARM parm;

    parm.code = VRDP_USB_REQ_RELEASE_INTERFACE;
    parm.id = pDevice->id;
    parm.iface = u8Ifnum;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) iface_InterfaceSetting (PREMOTEUSBDEVICE pDevice, uint8_t u8Ifnum, uint8_t u8Setting)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    VRDP_USB_REQ_INTERFACE_SETTING_PARM parm;

    parm.code = VRDP_USB_REQ_INTERFACE_SETTING;
    parm.id = pDevice->id;
    parm.iface = u8Ifnum;
    parm.setting = u8Setting;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    return VINF_SUCCESS;
}

static DECLCALLBACK(int) iface_ClearHaltedEP (PREMOTEUSBDEVICE pDevice, uint8_t u8Ep)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    VRDP_USB_REQ_CLEAR_HALTED_EP_PARM parm;

    parm.code = VRDP_USB_REQ_CLEAR_HALTED_EP;
    parm.id = pDevice->id;
    parm.ep = u8Ep;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    return VINF_SUCCESS;
}

static DECLCALLBACK(void) iface_CancelURB (PREMOTEUSBDEVICE pDevice, PREMOTEUSBQURB pRemoteURB)
{
    RemoteUSBBackend *pThis = pDevice->pOwner;

    VRDP_USB_REQ_CANCEL_URB_PARM parm;

    parm.code = VRDP_USB_REQ_CANCEL_URB;
    parm.id = pDevice->id;
    parm.handle = pRemoteURB->u32Handle;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

    requestDevice (pDevice);

    /* Remove this urb from the queue. It is safe because if
     * client will return the URB, it will be just ignored
     * in reapURB.
     */
    if (pRemoteURB->prev)
    {
        pRemoteURB->prev->next = pRemoteURB->next;
    }
    else
    {
        pDevice->pHeadQURBs = pRemoteURB->next;
    }

    if (pRemoteURB->next)
    {
        pRemoteURB->next->prev = pRemoteURB->prev;
    }
    else
    {
        pDevice->pTailQURBs = pRemoteURB->prev;
    }

    qurbFree (pRemoteURB);

    releaseDevice (pDevice);

    return;
}

static DECLCALLBACK(int) iface_QueueURB (PREMOTEUSBDEVICE pDevice, uint8_t u8Type, uint8_t u8Ep, uint8_t u8Direction, uint32_t u32Len, void *pvData, void *pvURB, PREMOTEUSBQURB *ppRemoteURB)
{
    int rc = VINF_SUCCESS;

#ifdef DEBUG_sunlover
    LogFlow(("RemoteUSBBackend::iface_QueueURB: u8Type = %d, u8Ep = %d, u8Direction = %d, data\n%.*Rhxd\n", u8Type, u8Ep, u8Direction, u32Len, pvData));
#endif /* DEBUG_sunlover */

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    RemoteUSBBackend *pThis = pDevice->pOwner;

    VRDP_USB_REQ_QUEUE_URB_PARM parm;
    uint32_t u32Handle = 0;
    uint32_t u32DataLen = 0;

    REMOTEUSBQURB *qurb = qurbAlloc (pDevice);

    if (qurb == NULL)
    {
        rc = VERR_NO_MEMORY;
        goto l_leave;
    }

    /*
     * Compute length of data which need to be transferred to the client.
     */
    switch(u8Direction)
    {
        case VUSB_DIRECTION_IN:
        {
            if (u8Type == VUSBXFERTYPE_MSG)
            {
                u32DataLen = 8; /* 8 byte header. */
                // u32DataLen = u32Len; // @todo do messages need all information?
            }
        } break;

        case VUSB_DIRECTION_OUT:
        {
            u32DataLen = u32Len;
        } break;

        default:
        {
            AssertFailed();
            rc = VERR_INVALID_PARAMETER;
            goto l_leave;
        }
    }

    parm.code = VRDP_USB_REQ_QUEUE_URB;
    parm.id = pDevice->id;

    u32Handle = pDevice->hURB++;
    if (u32Handle == 0)
    {
        u32Handle = pDevice->hURB++;
    }

    LogFlow(("RemoteUSBBackend::iface_QueueURB: handle = %d\n", u32Handle));

    parm.handle = u32Handle;

    switch(u8Type)
    {
        case VUSBXFERTYPE_CTRL: parm.type = VRDP_USB_TRANSFER_TYPE_CTRL; break;
        case VUSBXFERTYPE_ISOC: parm.type = VRDP_USB_TRANSFER_TYPE_ISOC; break;
        case VUSBXFERTYPE_BULK: parm.type = VRDP_USB_TRANSFER_TYPE_BULK; break;
        case VUSBXFERTYPE_INTR: parm.type = VRDP_USB_TRANSFER_TYPE_INTR; break;
        case VUSBXFERTYPE_MSG:  parm.type = VRDP_USB_TRANSFER_TYPE_MSG;  break;
        default: AssertFailed(); rc = VERR_INVALID_PARAMETER; goto l_leave;
    }

    parm.ep = u8Ep;

    switch(u8Direction)
    {
        case VUSB_DIRECTION_SETUP: AssertFailed(); parm.direction = VRDP_USB_DIRECTION_SETUP; break;
        case VUSB_DIRECTION_IN:    parm.direction = VRDP_USB_DIRECTION_IN;    break;
        case VUSB_DIRECTION_OUT:   parm.direction = VRDP_USB_DIRECTION_OUT;   break;
        default: AssertFailed(); rc = VERR_INVALID_PARAMETER; goto l_leave;
    }

    parm.urblen = u32Len;
    parm.datalen = u32DataLen;

    if (u32DataLen)
    {
        parm.data = pvData;
    }

    requestDevice (pDevice);

    /* Add at tail of queued urb list. */
    qurb->next       = NULL;
    qurb->prev       = pDevice->pTailQURBs;
    qurb->u32Err     = VRDP_USB_XFER_OK;
    qurb->u32Len     = u32Len;
    qurb->pvData     = pvData;
    qurb->pvURB      = pvURB;
    qurb->u32Handle  = u32Handle;
    qurb->fCompleted = false;
    qurb->fInput     = (u8Direction == VUSB_DIRECTION_IN);
    qurb->u32TransferredLen = 0;

    if (pDevice->pTailQURBs)
    {
        Assert(pDevice->pTailQURBs->next == NULL);
        pDevice->pTailQURBs->next = qurb;
    }
    else
    {
        /* This is the first URB to be added. */
        Assert(pDevice->pHeadQURBs == NULL);
        pDevice->pHeadQURBs = qurb;
    }

    pDevice->pTailQURBs = qurb;

    releaseDevice (pDevice);

    *ppRemoteURB = qurb;

    pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));

l_leave:
    if (RT_FAILURE(rc))
    {
        qurbFree (qurb);
    }

    return rc;
}

/* The function checks the URB queue for completed URBs. Also if the client
 * has requested URB polling, the function will send URB poll requests.
 */
static DECLCALLBACK(int) iface_ReapURB (PREMOTEUSBDEVICE pDevice, uint32_t u32Millies, void **ppvURB, uint32_t *pu32Len, uint32_t *pu32Err)
{
    int rc = VINF_SUCCESS;

    LogFlow(("RemoteUSBBackend::iface_ReapURB %d ms\n", u32Millies));

    if (pDevice->fFailed)
    {
        return VERR_VUSB_DEVICE_NOT_ATTACHED;
    }

    RemoteUSBBackend *pThis = pDevice->pOwner;

    /* Wait for transaction completion. */
    uint64_t u64StartTime = RTTimeMilliTS ();

    if (pThis->pollingEnabledURB ())
    {
        VRDP_USB_REQ_REAP_URB_PARM parm;

        parm.code = VRDP_USB_REQ_REAP_URB;

        pThis->VRDPServer()->SendUSBRequest (pDevice->u32ClientId, &parm, sizeof (parm));
    }

    REMOTEUSBQURB *qurb = NULL;

    for (;;)
    {
        uint32_t u32ClientId;

        /* Scan queued URBs, look for completed. */
        requestDevice (pDevice);

        u32ClientId = pDevice->u32ClientId;

        qurb = pDevice->pHeadQURBs;

        while (qurb)
        {
            if (qurb->fCompleted)
            {
                /* Remove this completed urb from the queue. */
                if (qurb->prev)
                {
                    qurb->prev->next = qurb->next;
                }
                else
                {
                    pDevice->pHeadQURBs = qurb->next;
                }

                if (qurb->next)
                {
                    qurb->next->prev = qurb->prev;
                }
                else
                {
                    pDevice->pTailQURBs = qurb->prev;
                }

                qurb->next = NULL;
                qurb->prev = NULL;

                break;
            }

            qurb = qurb->next;
        }

        releaseDevice (pDevice);

        if (   qurb
            || !pDevice->pHeadQURBs
            || u32Millies == 0
            || pDevice->fFailed
            || (RTTimeMilliTS () - u64StartTime >= (uint64_t)u32Millies))
        {
            /* Got an URB or do not have to wait for an URB. */
            break;
        }

        LogFlow(("RemoteUSBBackend::iface_ReapURB iteration.\n"));

        RTThreadSleep (10);

        if (pThis->pollingEnabledURB ())
        {
            VRDP_USB_REQ_REAP_URB_PARM parm;

            parm.code = VRDP_USB_REQ_REAP_URB;

            pThis->VRDPServer()->SendUSBRequest (u32ClientId, &parm, sizeof (parm));
        }
    }

    LogFlow(("RemoteUSBBackend::iface_ReapURB completed in %lld ms, qurb = %p\n", RTTimeMilliTS () - u64StartTime, qurb));

    if (!qurb)
    {
        *ppvURB = NULL;
        *pu32Len = 0;
        *pu32Err = VUSBSTATUS_OK;
    }
    else
    {
        *ppvURB = qurb->pvURB;
        *pu32Len = qurb->u32Len;
        *pu32Err = qurb->u32Err;

#ifdef LOG_ENABLED
        Log(("URB len = %d, data = %p\n", qurb->u32Len, qurb->pvURB));
        if (qurb->u32Len)
        {
            Log(("Received URB content:\n%.*Rhxd\n", qurb->u32Len, qurb->pvData));
        }
#endif

        qurbFree (qurb);
    }

    return rc;
}

void RemoteUSBBackend::AddRef (void)
{
    cRefs++;
}

void RemoteUSBBackend::Release (void)
{
    cRefs--;

    if (cRefs <= 0)
    {
        delete this;
    }
}

void RemoteUSBBackend::PollRemoteDevices (void)
{
    if (   mfWillBeDeleted
        && menmPollRemoteDevicesStatus != PollRemoteDevicesStatus_Dereferenced)
    {
        /* Unmount all remote USB devices. */
        mConsole->processRemoteUSBDevices (mu32ClientId, NULL, 0);

        menmPollRemoteDevicesStatus = PollRemoteDevicesStatus_Dereferenced;

        Release ();

        return;
    }

    switch (menmPollRemoteDevicesStatus)
    {
        case PollRemoteDevicesStatus_Negotiate:
        {
            VRDPUSBREQNEGOTIATEPARM parm;

            parm.code = VRDP_USB_REQ_NEGOTIATE;
            parm.version = VRDP_USB_VERSION;
            parm.flags = 0;

            mServer->SendUSBRequest (mu32ClientId, &parm, sizeof (parm));

            /* Reference the object. When the client disconnects and
             * the backend is about to be deleted, the method must be called
             * to disconnect the USB devices (as stated above).
             */
            AddRef ();

            /* Goto the disabled state. When a responce will be received
             * the state will be changed to the SendRequest.
             */
            menmPollRemoteDevicesStatus = PollRemoteDevicesStatus_WaitNegotiateResponse;
        } break;

        case PollRemoteDevicesStatus_WaitNegotiateResponse:
        {
            LogFlow(("USB::PollRemoteDevices: WaitNegotiateResponse\n"));
            /* Do nothing. */
        } break;

        case PollRemoteDevicesStatus_SendRequest:
        {
            LogFlow(("USB::PollRemoteDevices: SendRequest\n"));

            /* Send a request for device list. */
            VRDP_USB_REQ_DEVICE_LIST_PARM parm;

            parm.code = VRDP_USB_REQ_DEVICE_LIST;

            mServer->SendUSBRequest (mu32ClientId, &parm, sizeof (parm));

            menmPollRemoteDevicesStatus = PollRemoteDevicesStatus_WaitResponse;
        } break;

        case PollRemoteDevicesStatus_WaitResponse:
        {
            LogFlow(("USB::PollRemoteDevices: WaitResponse\n"));

            if (mfHasDeviceList)
            {
                mConsole->processRemoteUSBDevices (mu32ClientId, (VRDPUSBDEVICEDESC *)mpvDeviceList, mcbDeviceList);
                LogFlow(("USB::PollRemoteDevices: WaitResponse after process\n"));

                menmPollRemoteDevicesStatus = PollRemoteDevicesStatus_SendRequest;

                mfHasDeviceList = false;
            }
        } break;

        case PollRemoteDevicesStatus_Dereferenced:
        {
            LogFlow(("USB::PollRemoteDevices: Dereferenced\n"));
            /* Do nothing. */
        } break;

        default:
        {
           AssertFailed ();
        } break;
    }
}

void RemoteUSBBackend::NotifyDelete (void)
{
    mfWillBeDeleted = true;
}

/*
 * The backend maintains a list of UUIDs of devices
 * which are managed by the backend.
 */
bool RemoteUSBBackend::addUUID (const Guid *pUuid)
{
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(aGuids); i++)
    {
        if (aGuids[i].isEmpty ())
        {
            aGuids[i] = *pUuid;
            return true;
        }
    }

    return false;
}

bool RemoteUSBBackend::findUUID (const Guid *pUuid)
{
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(aGuids); i++)
    {
        if (aGuids[i] == *pUuid)
        {
            return true;
        }
    }

    return false;
}

void RemoteUSBBackend::removeUUID (const Guid *pUuid)
{
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(aGuids); i++)
    {
        if (aGuids[i] == *pUuid)
        {
            aGuids[i].clear ();
            break;
        }
    }
}

RemoteUSBBackend::RemoteUSBBackend(Console *console, ConsoleVRDPServer *server, uint32_t u32ClientId)
    :
    mConsole (console),
    mServer (server),
    cRefs (0),
    mu32ClientId (u32ClientId),
    mfHasDeviceList (false),
    mpvDeviceList (NULL),
    mcbDeviceList (0),
    menmPollRemoteDevicesStatus (PollRemoteDevicesStatus_Negotiate),
    mfPollURB (true),
    mpDevices (NULL),
    mfWillBeDeleted (false),
    mClientVersion (0)                   /* VRDP_USB_VERSION_2: the client version. */
{
    Assert(console);
    Assert(server);

    int rc = RTCritSectInit (&mCritsect);

    if (RT_FAILURE(rc))
    {
        AssertFailed ();
        memset (&mCritsect, 0, sizeof (mCritsect));
    }

    mCallback.pInstance           = (PREMOTEUSBBACKEND)this;
    mCallback.pfnOpen             = iface_Open;
    mCallback.pfnClose            = iface_Close;
    mCallback.pfnReset            = iface_Reset;
    mCallback.pfnSetConfig        = iface_SetConfig;
    mCallback.pfnClaimInterface   = iface_ClaimInterface;
    mCallback.pfnReleaseInterface = iface_ReleaseInterface;
    mCallback.pfnInterfaceSetting = iface_InterfaceSetting;
    mCallback.pfnQueueURB         = iface_QueueURB;
    mCallback.pfnReapURB          = iface_ReapURB;
    mCallback.pfnClearHaltedEP    = iface_ClearHaltedEP;
    mCallback.pfnCancelURB        = iface_CancelURB;
}

RemoteUSBBackend::~RemoteUSBBackend()
{
    Assert(cRefs == 0);

    if (RTCritSectIsInitialized (&mCritsect))
    {
        RTCritSectDelete (&mCritsect);
    }

    RTMemFree (mpvDeviceList);

    mServer->usbBackendRemoveFromList (this);
}

int RemoteUSBBackend::negotiateResponse (const VRDPUSBREQNEGOTIATERET *pret, uint32_t cbRet)
{
    int rc = VINF_SUCCESS;

    Log(("RemoteUSBBackend::negotiateResponse: flags = %02X.\n", pret->flags));

    LogRel(("Remote USB: Received negotiate response. Flags 0x%02X.\n",
             pret->flags));

    if (pret->flags & VRDP_USB_CAPS_FLAG_POLL)
    {
        Log(("RemoteUSBBackend::negotiateResponse: client requested URB polling.\n"));
        mfPollURB = true;
    }
    else
    {
        mfPollURB = false;
    }

    /* VRDP_USB_VERSION_2: check the client version. */
    if (pret->flags & VRDP_USB_CAPS2_FLAG_VERSION)
    {
        /* This could be a client version > 1. */
        if (cbRet >= sizeof (VRDPUSBREQNEGOTIATERET_2))
        {
            VRDPUSBREQNEGOTIATERET_2 *pret2 = (VRDPUSBREQNEGOTIATERET_2 *)pret;

            if (pret2->u32Version <= VRDP_USB_VERSION)
            {
                /* This is OK. The client wants a version supported by the server. */
                mClientVersion = pret2->u32Version;
            }
            else
            {
                LogRel(("VRDP: ERROR: unsupported remote USB protocol client version %d.\n", pret2->u32Version));
                rc = VERR_NOT_SUPPORTED;
            }
        }
        else
        {
            LogRel(("VRDP: ERROR: invalid remote USB negotiate request packet size %d.\n", cbRet));
            rc = VERR_NOT_SUPPORTED;
        }
    }
    else
    {
        /* This is a client version 1. */
        mClientVersion = VRDP_USB_VERSION_1;
    }

    if (RT_SUCCESS(rc))
    {
        LogRel(("VRDP: remote USB protocol version %d.\n", mClientVersion));

        menmPollRemoteDevicesStatus = PollRemoteDevicesStatus_SendRequest;
    }

    return rc;
}

int RemoteUSBBackend::saveDeviceList (const void *pvList, uint32_t cbList)
{
    Log(("RemoteUSBBackend::saveDeviceList: pvList = %p, cbList = %d\n", pvList, cbList));

    if (!mfHasDeviceList)
    {
        RTMemFree (mpvDeviceList);
        mpvDeviceList = NULL;

        mcbDeviceList = cbList;

        if (cbList > 0)
        {
            mpvDeviceList = RTMemAlloc (cbList);
            memcpy (mpvDeviceList, pvList, cbList);
        }

        mfHasDeviceList = true;
    }

    return VINF_SUCCESS;
}

void RemoteUSBBackend::request (void)
{
    int rc = RTCritSectEnter(&mCritsect);
    AssertRC(rc);
}

void RemoteUSBBackend::release (void)
{
    RTCritSectLeave(&mCritsect);
}

PREMOTEUSBDEVICE RemoteUSBBackend::deviceFromId (VRDPUSBDEVID id)
{
    request ();

    REMOTEUSBDEVICE *pDevice = mpDevices;

    while (pDevice && pDevice->id != id)
    {
        pDevice = pDevice->next;
    }

    release ();

    return pDevice;
}

void RemoteUSBBackend::addDevice (PREMOTEUSBDEVICE pDevice)
{
    request ();

    pDevice->next = mpDevices;

    if (mpDevices)
    {
        mpDevices->prev = pDevice;
    }

    mpDevices = pDevice;

    release ();
}

void RemoteUSBBackend::removeDevice (PREMOTEUSBDEVICE pDevice)
{
    request ();

    if (pDevice->prev)
    {
        pDevice->prev->next = pDevice->next;
    }
    else
    {
        mpDevices = pDevice->next;
    }

    if (pDevice->next)
    {
        pDevice->next->prev = pDevice->prev;
    }

    release ();
}

int RemoteUSBBackend::reapURB (const void *pvBody, uint32_t cbBody)
{
    int rc = VINF_SUCCESS;

    LogFlow(("RemoteUSBBackend::reapURB: pvBody = %p, cbBody = %d\n", pvBody, cbBody));

    VRDPUSBREQREAPURBBODY *pBody = (VRDPUSBREQREAPURBBODY *)pvBody;

    while (cbBody >= sizeof (VRDPUSBREQREAPURBBODY))
    {
        Log(("RemoteUSBBackend::reapURB: id = %d,  flags = %02X, error = %d, handle %d, len = %d.\n",
             pBody->id, pBody->flags, pBody->error, pBody->handle, pBody->len));

        uint8_t fu8ReapValidFlags;

        if (mClientVersion == VRDP_USB_VERSION_1 || mClientVersion == VRDP_USB_VERSION_2)
        {
            fu8ReapValidFlags = VRDP_USB_REAP_VALID_FLAGS;
        }
        else
        {
            fu8ReapValidFlags = VRDP_USB_REAP_VALID_FLAGS_3;
        }

        /* Verify client's data. */
        if (   (pBody->flags & ~fu8ReapValidFlags) != 0
            || sizeof (VRDPUSBREQREAPURBBODY) > cbBody
            || pBody->handle == 0)
        {
            LogFlow(("RemoteUSBBackend::reapURB: WARNING: invalid reply data. Skipping the reply.\n"));
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        PREMOTEUSBDEVICE pDevice = deviceFromId (pBody->id);

        if (!pDevice)
        {
            LogFlow(("RemoteUSBBackend::reapURB: WARNING: invalid device id. Skipping the reply.\n"));
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        uint32_t cbBodyData = 0; /* Data contained in the URB body structure for input URBs. */

        requestDevice (pDevice);

        /* Search the queued URB for given handle. */
        REMOTEUSBQURB *qurb = pDevice->pHeadQURBs;

        while (qurb && qurb->u32Handle != pBody->handle)
        {
            LogFlow(("RemoteUSBBackend::reapURB: searching: %p handle = %d.\n", qurb, qurb->u32Handle));
            qurb = qurb->next;
        }

        if (!qurb)
        {
            LogFlow(("RemoteUSBBackend::reapURB: Queued URB not found, probably already canceled. Skipping the URB.\n"));
        }
        else
        {
            LogFlow(("RemoteUSBBackend::reapURB: qurb = %p\n", qurb));

            /* Update the URB error field. */
            if (mClientVersion == VRDP_USB_VERSION_1)
            {
                switch(pBody->error)
                {
                    case VRDP_USB_XFER_OK:    qurb->u32Err = VUSBSTATUS_OK;    break;
                    case VRDP_USB_XFER_STALL: qurb->u32Err = VUSBSTATUS_STALL; break;
                    case VRDP_USB_XFER_DNR:   qurb->u32Err = VUSBSTATUS_DNR;   break;
                    case VRDP_USB_XFER_CRC:   qurb->u32Err = VUSBSTATUS_CRC;   break;
                    default: Log(("RemoteUSBBackend::reapURB: Invalid error %d\n", pBody->error));
                                              qurb->u32Err = VUSBSTATUS_DNR;   break;
                }
            }
            else if (   mClientVersion == VRDP_USB_VERSION_2
                     || mClientVersion == VRDP_USB_VERSION_3)
            {
                switch(pBody->error)
                {
                    case VRDP_USB_XFER_OK:    qurb->u32Err = VUSBSTATUS_OK;            break;
                    case VRDP_USB_XFER_STALL: qurb->u32Err = VUSBSTATUS_STALL;         break;
                    case VRDP_USB_XFER_DNR:   qurb->u32Err = VUSBSTATUS_DNR;           break;
                    case VRDP_USB_XFER_CRC:   qurb->u32Err = VUSBSTATUS_CRC;           break;
                    case VRDP_USB_XFER_DO:    qurb->u32Err = VUSBSTATUS_DATA_OVERRUN;  break;
                    case VRDP_USB_XFER_DU:    qurb->u32Err = VUSBSTATUS_DATA_UNDERRUN; break;

                    /* Unmapped errors. */
                    case VRDP_USB_XFER_BS:
                    case VRDP_USB_XFER_DTM:
                    case VRDP_USB_XFER_PCF:
                    case VRDP_USB_XFER_UPID:
                    case VRDP_USB_XFER_BO:
                    case VRDP_USB_XFER_BU:
                    case VRDP_USB_XFER_ERR:
                    default: Log(("RemoteUSBBackend::reapURB: Invalid error %d\n", pBody->error));
                                              qurb->u32Err = VUSBSTATUS_DNR;   break;
                }
            }
            else
            {
                qurb->u32Err = VUSBSTATUS_DNR;
            }

            /* Get the URB data. */
            bool fURBCompleted = true;

            if (qurb->fInput)
            {
                cbBodyData = pBody->len; /* VRDP_USB_DIRECTION_IN URBs include some data. */
            }

            if (   qurb->u32Err == VUSBSTATUS_OK
                && qurb->fInput)
            {
                LogFlow(("RemoteUSBBackend::reapURB: copying data %d bytes\n", pBody->len));

                uint32_t u32DataLen = qurb->u32TransferredLen + pBody->len;

                if (u32DataLen > qurb->u32Len)
                {
                    /* Received more data than expected for this URB. If there more fragments follow,
                     * they will be discarded because the URB handle will not be valid anymore.
                     */
                    qurb->u32Err = VUSBSTATUS_DNR;
                }
                else
                {
                    memcpy ((uint8_t *)qurb->pvData + qurb->u32TransferredLen, &pBody[1], pBody->len);
                }

                if (   qurb->u32Err == VUSBSTATUS_OK
                    && (pBody->flags & VRDP_USB_REAP_FLAG_FRAGMENT) != 0)
                {
                    /* If the client sends fragmented packets, accumulate the URB data. */
                    fURBCompleted = false;
                }
            }

            qurb->u32TransferredLen += pBody->len; /* Update the value for all URBs. */

            if (fURBCompleted)
            {
                /* Move the URB to head of URB list, so the iface_ReapURB could find it faster. */
                if (qurb->prev)
                {
                    /* The URB is not in the head. */
                    qurb->prev->next = qurb->next;

                    if (qurb->next)
                    {
                        qurb->next->prev = qurb->prev;
                    }
                    else
                    {
                        pDevice->pTailQURBs = qurb->prev;
                    }

                    qurb->next = pDevice->pHeadQURBs;
                    qurb->prev = NULL;

                    pDevice->pHeadQURBs->prev = qurb;
                    pDevice->pHeadQURBs = qurb;
                }

                qurb->u32Len = qurb->u32TransferredLen; /* Update the final length. */
                qurb->fCompleted = true;
            }
        }

        releaseDevice (pDevice);

        if (pBody->flags & VRDP_USB_REAP_FLAG_LAST)
        {
            break;
        }

        /* There is probably a further URB body. */
        uint32_t cbBodySize = sizeof (VRDPUSBREQREAPURBBODY) + cbBodyData;

        if (cbBodySize > cbBody)
        {
            rc = VERR_INVALID_PARAMETER;
            break;
        }

        pBody = (VRDPUSBREQREAPURBBODY *)((uint8_t *)pBody + cbBodySize);
        cbBody -= cbBodySize;
    }

    LogFlow(("RemoteUSBBackend::reapURB: returns %Rrc\n", rc));

    return rc;
}
/* vi: set tabstop=4 shiftwidth=4 expandtab: */

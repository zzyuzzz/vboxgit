/* $Id: VBoxNetAdp.c 16960 2009-02-19 22:13:33Z vboxsync $ */
/** @file
 * VBoxNetAdp - Virtual Network Adapter Driver (Host), Common Code.
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

#define LOG_GROUP LOG_GROUP_NET_TAP_DRV
#include "VBoxNetAdpInternal.h"

#include <VBox/sup.h>
#include <VBox/log.h>
#include <VBox/err.h>
#include <VBox/version.h>
#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/spinlock.h>
#include <iprt/uuid.h>

#include <net/ethernet.h>
#include <net/if_ether.h>
#include <net/if_types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/errno.h>
#include <sys/param.h>

/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
#define IFPORT_2_VBOXNETADP(pIfPort) \
    ( (PVBOXNETADP)((uint8_t *)pIfPort - RT_OFFSETOF(VBOXNETADP, MyPort)) )


DECLHIDDEN(void) vboxNetAdpComposeMACAddress(PVBOXNETADP pThis, PRTMAC pMac)
{
    /* Note that terminating 0 is included. */
    memcpy(pMac->au8, "\0vbox", sizeof(pMac->au8));
    pMac->au8[sizeof(pMac->au8) - 1] += pThis->uUnit;
}


/**
 * Sets the enmState member atomically.
 *
 * Used for all updates.
 *
 * @param   pThis           The instance.
 * @param   enmNewState     The new value.
 */
DECLINLINE(void) vboxNetAdpSetState(PVBOXNETADP pThis, VBOXNETADPSTATE enmNewState)
{
    ASMAtomicWriteU32((uint32_t volatile *)&pThis->enmState, enmNewState);
}


/**
 * Gets the enmState member atomically.
 *
 * Used for all reads.
 *
 * @returns The enmState value.
 * @param   pThis           The instance.
 */
DECLINLINE(VBOXNETADPSTATE) vboxNetAdpGetState(PVBOXNETADP pThis)
{
    return (VBOXNETADPSTATE)ASMAtomicUoReadU32((uint32_t volatile *)&pThis->enmState);
}


DECLINLINE(bool) vboxNetAdpIsAvailable(PVBOXNETADP pAdp)
{
    return pAdp->enmState == kVBoxNetAdpState_Available;
}

DECLINLINE(bool) vboxNetAdpIsConnected(PVBOXNETADP pAdp)
{
    return pAdp->enmState >= kVBoxNetAdpState_Connected;
}

DECLINLINE(bool) vboxNetAdpIsVoid(PVBOXNETADP pAdp)
{
    return pAdp->enmState == kVBoxNetAdpState_Invalid;
}

DECLINLINE(bool) vboxNetAdpIsValid(PVBOXNETADP pAdp)
{
    return pAdp->enmState != kVBoxNetAdpState_Invalid
        && pAdp->enmState != kVBoxNetAdpState_Transitional;
}

DECLINLINE(bool) vboxNetAdpIsBusy(PVBOXNETADP pAdp)
{
    return pAdp->enmState == kVBoxNetAdpState_Connected;
}


static PVBOXNETADP vboxNetAdpFind(PVBOXNETADPGLOBALS pGlobals, const char *pszName)
{
    unsigned i;

    for (i = 0; i < RT_ELEMENTS(pGlobals->aAdapters); i++)
    {
        RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
        PVBOXNETADP pThis = &pGlobals->aAdapters[i];
        RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
        if (vboxNetAdpIsValid(pThis)
            && !strcmp(pThis->szName, pszName))
        {
            RTSpinlockRelease(pThis->hSpinlock, &Tmp);
            return pThis;
        }
        RTSpinlockRelease(pThis->hSpinlock, &Tmp);
    }
    return NULL;
}

/**
 * Releases a reference to the specified instance.
 *
 * @param   pThis           The instance.
 * @param   fBusy           Whether the busy counter should be decremented too.
 */
DECLHIDDEN(void) vboxNetAdpRelease(PVBOXNETADP pThis)
{
    uint32_t cRefs;

    /*
     * Paranoid Android.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->MyPort.u32VersionEnd == INTNETTRUNKIFPORT_VERSION);
    Assert(vboxNetAdpGetState(pThis) > kVBoxNetAdpState_Invalid);
    AssertPtr(pThis->pGlobals);
    Assert(pThis->hEventIdle != NIL_RTSEMEVENT);
    Assert(pThis->hSpinlock != NIL_RTSPINLOCK);
    Assert(pThis->szName[0]);

    /*
     * The object reference counting.
     */
    cRefs = ASMAtomicDecU32(&pThis->cRefs);
/*     if (!cRefs) */
/*         vboxNetAdpCheckDestroyInstance(pThis); */
/*     else */
        Assert(cRefs < UINT32_MAX / 2);
}

/**
 * Decrements the busy counter and does idle wakeup.
 *
 * @param   pThis           The instance.
 */
DECLHIDDEN(void) vboxNetAdpIdle(PVBOXNETADP pThis)
{
    uint32_t cBusy;

    /*
     * Paranoid Android.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->MyPort.u32VersionEnd == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->enmState == kVBoxNetAdpState_Connected);
    AssertPtr(pThis->pGlobals);
    Assert(pThis->hEventIdle != NIL_RTSEMEVENT);

    cBusy = ASMAtomicDecU32(&pThis->cBusy);
    if (!cBusy)
    {
        int rc = RTSemEventSignal(pThis->hEventIdle);
        AssertRC(rc);
    }
    else
        Assert(cBusy < UINT32_MAX / 2);
}

/**
 * Retains a reference to the specified instance.
 *
 * @param   pThis           The instance.
 */
DECLHIDDEN(void) vboxNetAdpRetain(PVBOXNETADP pThis)
{
    uint32_t cRefs;

    /*
     * Paranoid Android.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->MyPort.u32VersionEnd == INTNETTRUNKIFPORT_VERSION);
    Assert(vboxNetAdpGetState(pThis) > kVBoxNetAdpState_Invalid);
    AssertPtr(pThis->pGlobals);
    Assert(pThis->hEventIdle != NIL_RTSEMEVENT);
    Assert(pThis->hSpinlock != NIL_RTSPINLOCK);
    Assert(pThis->szName[0]);

    /*
     * Retain the object.
     */
    cRefs = ASMAtomicIncU32(&pThis->cRefs);
    Assert(cRefs > 1 && cRefs < UINT32_MAX / 2);

    NOREF(cRefs);
}

/**
 * Increments busy counter.
 *
 * @param   pThis           The instance.
 */
DECLHIDDEN(void) vboxNetAdpBusy(PVBOXNETADP pThis)
{
    uint32_t cBusy;

    /*
     * Are we vigilant enough?
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->MyPort.u32VersionEnd == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->enmState == kVBoxNetAdpState_Connected);
    AssertPtr(pThis->pGlobals);
    Assert(pThis->hEventIdle != NIL_RTSEMEVENT);
    cBusy = ASMAtomicIncU32(&pThis->cBusy);
    Assert(cBusy > 0 && cBusy < UINT32_MAX / 2);

    NOREF(cBusy);
}

/**
 * Checks if receive is possible and increases busy and ref counters if so.
 *
 * @param   pThis           The instance.
 */
DECLHIDDEN(bool) vboxNetAdpPrepareToReceive(PVBOXNETADP pThis)
{
    RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
    bool fCanReceive  = false;
    /*
     * Input validation.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    if (pThis->enmState == kVBoxNetAdpState_Active)
    {
        fCanReceive = true;
        vboxNetAdpRetain(pThis);
        vboxNetAdpBusy(pThis);
    }
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);

    return fCanReceive;
}

/**
 * Forwards scatter/gather list to internal network and decreases busy and ref counters.
 *
 * @param   pThis           The instance.
 */
DECLHIDDEN(void) vboxNetAdpReceive(PVBOXNETADP pThis, PINTNETSG pSG)
{
    RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
    /*
     * Input validation.
     */
    AssertPtr(pThis);
    AssertPtr(pSG);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    pThis->pSwitchPort->pfnRecv(pThis->pSwitchPort, pSG, INTNETTRUNKDIR_HOST);
    vboxNetAdpIdle(pThis);
    vboxNetAdpRelease(pThis);
}

/**
 * Decreases busy and ref counters.
 *
 * @param   pThis           The instance.
 */
DECLHIDDEN(void) vboxNetAdpCancelReceive(PVBOXNETADP pThis)
{
    vboxNetAdpIdle(pThis);
    vboxNetAdpRelease(pThis);
}

#ifdef RT_WITH_W64_UNWIND_HACK
# if defined(RT_OS_WINDOWS) && defined(RT_ARCH_AMD64)
#  define NETADP_DECL_CALLBACK(type) DECLASM(DECLHIDDEN(type))
#  define NETADP_CALLBACK(_n) netfltNtWrap##_n

NETADP_DECL_CALLBACK(int)  NETADP_CALLBACK(vboxNetAdpPortXmit)(PINTNETTRUNKIFPORT pIfPort, PINTNETSG pSG, uint32_t fDst);
NETADP_DECL_CALLBACK(bool) NETADP_CALLBACK(vboxNetAdpPortIsPromiscuous)(PINTNETTRUNKIFPORT pIfPort);
NETADP_DECL_CALLBACK(void) NETADP_CALLBACK(vboxNetAdpPortGetMacAddress)(PINTNETTRUNKIFPORT pIfPort, PRTMAC pMac);
NETADP_DECL_CALLBACK(bool) NETADP_CALLBACK(vboxNetAdpPortIsHostMac)(PINTNETTRUNKIFPORT pIfPort, PCRTMAC pMac);
NETADP_DECL_CALLBACK(int)  NETADP_CALLBACK(vboxNetAdpPortWaitForIdle)(PINTNETTRUNKIFPORT pIfPort, uint32_t cMillies);
NETADP_DECL_CALLBACK(bool) NETADP_CALLBACK(vboxNetAdpPortSetActive)(PINTNETTRUNKIFPORT pIfPort, bool fActive);
NETADP_DECL_CALLBACK(void) NETADP_CALLBACK(vboxNetAdpPortDisconnectAndRelease)(PINTNETTRUNKIFPORT pIfPort);
NETADP_DECL_CALLBACK(void) NETADP_CALLBACK(vboxNetAdpPortRetain)(PINTNETTRUNKIFPORT pIfPort);
NETADP_DECL_CALLBACK(void) NETADP_CALLBACK(vboxNetAdpPortRelease)(PINTNETTRUNKIFPORT pIfPort);

# else
#  error "UNSUPPORTED (RT_WITH_W64_UNWIND_HACK)"
# endif
#else
# define NETADP_DECL_CALLBACK(type) static DECLCALLBACK(type)
# define NETADP_CALLBACK(_n) _n
#endif

/**
 * @copydoc INTNETTRUNKIFPORT::pfnXmit
 */
NETADP_DECL_CALLBACK(int) vboxNetAdpPortXmit(PINTNETTRUNKIFPORT pIfPort, PINTNETSG pSG, uint32_t fDst)
{
    RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);
    int rc = VINF_SUCCESS;

    /*
     * Input validation.
     */
    AssertPtr(pThis);
    AssertPtr(pSG);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);

    /*
     * Do a retain/busy, invoke the OS specific code.
     */
    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    if (pThis->enmState != kVBoxNetAdpState_Active)
    {
        RTSpinlockRelease(pThis->hSpinlock, &Tmp);
        Log(("vboxNetAdpReceive: Dropping incoming packet for inactive interface %s.\n",
             pThis->szName));
        return VERR_INVALID_STATE;
    }
    vboxNetAdpRetain(pThis);
    vboxNetAdpBusy(pThis);
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);
    rc = vboxNetAdpPortOsXmit(pThis, pSG, fDst);
    vboxNetAdpIdle(pThis);
    vboxNetAdpRelease(pThis);

    return rc;
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnIsPromiscuous
 */
NETADP_DECL_CALLBACK(bool) vboxNetAdpPortIsPromiscuous(PINTNETTRUNKIFPORT pIfPort)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);

    /*
     * Input validation.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(vboxNetAdpGetState(pThis) == kVBoxNetAdpState_Connected);

    /*
     * Ask the OS specific code.
     */
    return vboxNetAdpPortOsIsPromiscuous(pThis);
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnGetMacAddress
 */
NETADP_DECL_CALLBACK(void) vboxNetAdpPortGetMacAddress(PINTNETTRUNKIFPORT pIfPort, PRTMAC pMac)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);

    /*
     * Input validation.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(vboxNetAdpGetState(pThis) == kVBoxNetAdpState_Connected);

    /*
     * Forward the question to the OS specific code.
     */
    vboxNetAdpPortOsGetMacAddress(pThis, pMac);
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnIsHostMac
 */
NETADP_DECL_CALLBACK(bool) vboxNetAdpPortIsHostMac(PINTNETTRUNKIFPORT pIfPort, PCRTMAC pMac)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);

    /*
     * Input validation.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(vboxNetAdpGetState(pThis) == kVBoxNetAdpState_Connected);

    /*
     * Ask the OS specific code.
     */
    return vboxNetAdpPortOsIsHostMac(pThis, pMac);
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnWaitForIdle
 */
NETADP_DECL_CALLBACK(int) vboxNetAdpPortWaitForIdle(PINTNETTRUNKIFPORT pIfPort, uint32_t cMillies)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);
    int rc;

    /*
     * Input validation.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    AssertReturn(vboxNetAdpGetState(pThis) == kVBoxNetAdpState_Connected, VERR_INVALID_STATE);

    /*
     * Go to sleep on the semaphore after checking the busy count.
     */
    vboxNetAdpRetain(pThis);

    rc = VINF_SUCCESS;
    while (pThis->cBusy && RT_SUCCESS(rc))
        rc = RTSemEventWait(pThis->hEventIdle, cMillies); /** @todo make interruptible? */

    vboxNetAdpRelease(pThis);

    return rc;
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnSetActive
 */
NETADP_DECL_CALLBACK(bool) vboxNetAdpPortSetActive(PINTNETTRUNKIFPORT pIfPort, bool fActive)
{
    bool fPreviouslyActive;
    RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);

    /*
     * Input validation.
     */
    AssertPtr(pThis);
    AssertPtr(pThis->pGlobals);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);

    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    switch (vboxNetAdpGetState(pThis))
    {
        case kVBoxNetAdpState_Connected:
            fPreviouslyActive = false;
            pThis->enmState = kVBoxNetAdpState_Active;
            break;
        case kVBoxNetAdpState_Active:
            fPreviouslyActive = true;
            pThis->enmState = kVBoxNetAdpState_Connected;
            break;
        default:
            fPreviouslyActive = false;
            break;
    }
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);
    return fPreviouslyActive;
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnDisconnectAndRelease
 */
NETADP_DECL_CALLBACK(void) vboxNetAdpPortDisconnectAndRelease(PINTNETTRUNKIFPORT pIfPort)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);
    RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;

    /*
     * Serious paranoia.
     */
    AssertPtr(pThis);
    Assert(pThis->MyPort.u32Version == INTNETTRUNKIFPORT_VERSION);
    Assert(pThis->MyPort.u32VersionEnd == INTNETTRUNKIFPORT_VERSION);
    AssertPtr(pThis->pGlobals);
    Assert(pThis->hEventIdle != NIL_RTSEMEVENT);
    Assert(pThis->hSpinlock != NIL_RTSPINLOCK);

    Assert(pThis->enmState == kVBoxNetAdpState_Connected);
    Assert(!pThis->cBusy);

    /*
     * Disconnect and release it.
     */
    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    vboxNetAdpSetState(pThis, kVBoxNetAdpState_Transitional);
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);

    vboxNetAdpOsDisconnectIt(pThis);
    pThis->pSwitchPort = NULL;

    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    vboxNetAdpSetState(pThis, kVBoxNetAdpState_Available);
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);

    vboxNetAdpRelease(pThis);
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnRelease
 */
NETADP_DECL_CALLBACK(void) vboxNetAdpPortRelease(PINTNETTRUNKIFPORT pIfPort)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);
    vboxNetAdpRelease(pThis);
}


/**
 * @copydoc INTNETTRUNKIFPORT::pfnRetain
 */
NETADP_DECL_CALLBACK(void) vboxNetAdpPortRetain(PINTNETTRUNKIFPORT pIfPort)
{
    PVBOXNETADP pThis = IFPORT_2_VBOXNETADP(pIfPort);
    vboxNetAdpRetain(pThis);
}


/**
 * Connects the instance to the specified switch port.
 *
 * Called while owning the lock. We're ASSUMING that the internal
 * networking code is already owning an recursive mutex, so, there
 * will be no deadlocks when vboxNetAdpOsConnectIt calls back into
 * it for setting preferences.
 *
 * @returns VBox status code.
 * @param   pThis               The instance.
 * @param   pSwitchPort         The port on the internal network 'switch'.
 * @param   ppIfPort            Where to return our port interface.
 */
static int vboxNetAdpConnectIt(PVBOXNETADP pThis, PINTNETTRUNKSWPORT pSwitchPort, PINTNETTRUNKIFPORT *ppIfPort)
{
    int rc;

    /*
     * Validate state.
     */
    //Assert(!pThis->fActive);
    Assert(!pThis->cBusy);
    Assert(vboxNetAdpGetState(pThis) == kVBoxNetAdpState_Transitional);

    /*
     * Do the job.
     * Note that we're calling the os stuff while owning the semaphore here.
     */
    pThis->pSwitchPort = pSwitchPort;
    rc = vboxNetAdpOsConnectIt(pThis);
    if (RT_SUCCESS(rc))
    {
        *ppIfPort = &pThis->MyPort;
    }
    else
        pThis->pSwitchPort = NULL;

    //Assert(!pThis->fActive);
    return rc;
}

/**
 * @copydoc INTNETTRUNKFACTORY::pfnCreateAndConnect
 */
static DECLCALLBACK(int) vboxNetAdpFactoryCreateAndConnect(PINTNETTRUNKFACTORY pIfFactory, const char *pszName,
                                                           PINTNETTRUNKSWPORT pSwitchPort, PINTNETTRUNKIFPORT *ppIfPort, bool fNoPromisc)
{
    PVBOXNETADP pThis;
    int rc;
    PVBOXNETADPGLOBALS pGlobals = (PVBOXNETADPGLOBALS)((uint8_t *)pIfFactory - RT_OFFSETOF(VBOXNETADPGLOBALS, TrunkFactory));

    LogFlow(("vboxNetAdpFactoryCreateAndConnect: pszName=%p:{%s}\n", pszName, pszName));
    Assert(pGlobals->cFactoryRefs > 0);

    /*
     * Find instance, check if busy, connect if not.
     */
    pThis = vboxNetAdpFind(pGlobals, pszName);
    if (pThis)
    {
        RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
        RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
        if (pThis->enmState == kVBoxNetAdpState_Available)
        {
            pThis->enmState = kVBoxNetAdpState_Transitional;
            RTSpinlockRelease(pThis->hSpinlock, &Tmp);
            rc = vboxNetAdpConnectIt(pThis, pSwitchPort, ppIfPort);
            RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
            if (RT_SUCCESS(rc))
                pThis->enmState = kVBoxNetAdpState_Connected;
            else
                pThis->enmState = kVBoxNetAdpState_Available;
        }
        else
            rc = VERR_INTNET_FLT_IF_BUSY;
        RTSpinlockRelease(pThis->hSpinlock, &Tmp);
    }
    else
        rc = VERR_INTNET_FLT_IF_NOT_FOUND;

    return rc;
}

/**
 * @copydoc INTNETTRUNKFACTORY::pfnRelease
 */
static DECLCALLBACK(void) vboxNetAdpFactoryRelease(PINTNETTRUNKFACTORY pIfFactory)
{
    PVBOXNETADPGLOBALS pGlobals = (PVBOXNETADPGLOBALS)((uint8_t *)pIfFactory - RT_OFFSETOF(VBOXNETADPGLOBALS, TrunkFactory));

    int32_t cRefs = ASMAtomicDecS32(&pGlobals->cFactoryRefs);
    Assert(cRefs >= 0); NOREF(cRefs);
    LogFlow(("vboxNetAdpFactoryRelease: cRefs=%d (new)\n", cRefs));
}


/**
 * Implements the SUPDRV component factor interface query method.
 *
 * @returns Pointer to an interface. NULL if not supported.
 *
 * @param   pSupDrvFactory      Pointer to the component factory registration structure.
 * @param   pSession            The session - unused.
 * @param   pszInterfaceUuid    The factory interface id.
 */
static DECLCALLBACK(void *) vboxNetAdpQueryFactoryInterface(PCSUPDRVFACTORY pSupDrvFactory, PSUPDRVSESSION pSession, const char *pszInterfaceUuid)
{
    PVBOXNETADPGLOBALS pGlobals = (PVBOXNETADPGLOBALS)((uint8_t *)pSupDrvFactory - RT_OFFSETOF(VBOXNETADPGLOBALS, SupDrvFactory));

    /*
     * Convert the UUID strings and compare them.
     */
    RTUUID UuidReq;
    int rc = RTUuidFromStr(&UuidReq, pszInterfaceUuid);
    if (RT_SUCCESS(rc))
    {
        if (!RTUuidCompareStr(&UuidReq, INTNETTRUNKFACTORY_UUID_STR))
        {
            ASMAtomicIncS32(&pGlobals->cFactoryRefs);
            return &pGlobals->TrunkFactory;
        }
#ifdef LOG_ENABLED
        else
            Log(("VBoxNetAdp: unknown factory interface query (%s)\n", pszInterfaceUuid));
#endif
    }
    else
        Log(("VBoxNetAdp: rc=%Rrc, uuid=%s\n", rc, pszInterfaceUuid));

    return NULL;
}


/**
 * Checks whether the VBoxNetAdp wossname can be unloaded.
 *
 * This will return false if someone is currently using the module.
 *
 * @returns true if it's relatively safe to unload it, otherwise false.
 * @param   pGlobals        Pointer to the globals.
 */
DECLHIDDEN(bool) vboxNetAdpCanUnload(PVBOXNETADPGLOBALS pGlobals)
{
    int rc;
    unsigned i;
    bool fRc = true; /* Assume it can be unloaded. */

    for (i = 0; i < RT_ELEMENTS(pGlobals->aAdapters); i++)
    {
        RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
        PVBOXNETADP pThis = &pGlobals->aAdapters[i];
        RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
        if (vboxNetAdpIsConnected(&pGlobals->aAdapters[i]))
        {
            RTSpinlockRelease(pThis->hSpinlock, &Tmp);
            fRc = false;
            break; /* We already know the answer. */
        }
        RTSpinlockRelease(pThis->hSpinlock, &Tmp);
    }
    return fRc && ASMAtomicUoReadS32((int32_t volatile *)&pGlobals->cFactoryRefs) <= 0;
}

/**
 * tries to deinitialize Idc
 * we separate the globals settings "base" which is actually
 * "general" globals settings except for Idc, and idc.
 * This is needed for windows filter driver, which gets loaded prior to VBoxDrv,
 * thus it's not possible to make idc initialization from the driver startup routine for it,
 * though the "base is still needed for the driver to functions".
 * @param pGlobals
 * @return VINF_SUCCESS on succes, VERR_WRONG_ORDER if we're busy.
 */
DECLHIDDEN(int) vboxNetAdpTryDeleteIdc(PVBOXNETADPGLOBALS pGlobals)
{
    int rc;

    Assert(pGlobals->hFastMtx != NIL_RTSEMFASTMUTEX);

    /*
     * Check before trying to deregister the factory.
     */
    if (!vboxNetAdpCanUnload(pGlobals))
        return VERR_WRONG_ORDER;

    /*
     * Disconnect from SUPDRV and check that nobody raced us,
     * reconnect if that should happen.
     */
    rc = SUPR0IdcComponentDeregisterFactory(&pGlobals->SupDrvIDC, &pGlobals->SupDrvFactory);
    AssertRC(rc);
    if (!vboxNetAdpCanUnload(pGlobals))
    {
        rc = SUPR0IdcComponentRegisterFactory(&pGlobals->SupDrvIDC, &pGlobals->SupDrvFactory);
        AssertRC(rc);
        return VERR_WRONG_ORDER;
    }

    SUPR0IdcClose(&pGlobals->SupDrvIDC);

    return rc;
}

/**
 * performs "base" globals deinitialization
 * we separate the globals settings "base" which is actually
 * "general" globals settings except for Idc, and idc.
 * This is needed for windows filter driver, which gets loaded prior to VBoxDrv,
 * thus it's not possible to make idc initialization from the driver startup routine for it,
 * though the "base is still needed for the driver to functions".
 * @param pGlobals
 * @return none
 */
DECLHIDDEN(void) vboxNetAdpDeleteGlobalsBase(PVBOXNETADPGLOBALS pGlobals)
{
    /*
     * Release resources.
     */
    RTSemFastMutexDestroy(pGlobals->hFastMtx);
    pGlobals->hFastMtx = NIL_RTSEMFASTMUTEX;

#ifdef VBOXNETADP_STATIC_CONFIG
    RTSemEventDestroy(pGlobals->hTimerEvent);
    pGlobals->hTimerEvent = NIL_RTSEMEVENT;
#endif

}

/**
 * Called by the native part when the OS wants the driver to unload.
 *
 * @returns VINF_SUCCESS on succes, VERR_WRONG_ORDER if we're busy.
 *
 * @param   pGlobals        Pointer to the globals.
 */
DECLHIDDEN(int) vboxNetAdpTryDeleteGlobals(PVBOXNETADPGLOBALS pGlobals)
{
    int rc = vboxNetAdpTryDeleteIdc(pGlobals);
    if(RT_SUCCESS(rc))
    {
        vboxNetAdpDeleteGlobalsBase(pGlobals);
    }
    return rc;
}

/**
 * performs the "base" globals initialization
 * we separate the globals initialization to globals "base" initialization which is actually
 * "general" globals initialization except for Idc not being initialized, and idc initialization.
 * This is needed for windows filter driver, which gets loaded prior to VBoxDrv,
 * thus it's not possible to make idc initialization from the driver startup routine for it.
 *
 * @returns VBox status code.
 * @param   pGlobals    Pointer to the globals. */
DECLHIDDEN(int) vboxNetAdpInitGlobalsBase(PVBOXNETADPGLOBALS pGlobals)
{
    /*
     * Initialize the common portions of the structure.
     */
    int i;
    int rc = RTSemFastMutexCreate(&pGlobals->hFastMtx);
    if (RT_SUCCESS(rc))
    {
        memset(pGlobals->aAdapters, 0, sizeof(pGlobals->aAdapters));
        for (i = 0; i < (int)RT_ELEMENTS(pGlobals->aAdapters); i++)
        {
            rc = RTSpinlockCreate(&pGlobals->aAdapters[i].hSpinlock);
            if (RT_FAILURE(rc))
            {
                /* Clean up. */
                while (--i >= 0)
                    RTSpinlockDestroy(pGlobals->aAdapters[i].hSpinlock);
                Log(("vboxNetAdpInitGlobalsBase: Failed to create fast mutex (rc=%Rrc).\n", rc));
                RTSemFastMutexDestroy(pGlobals->hFastMtx);
                return rc;
            }
        }
        pGlobals->TrunkFactory.pfnRelease = vboxNetAdpFactoryRelease;
        pGlobals->TrunkFactory.pfnCreateAndConnect = vboxNetAdpFactoryCreateAndConnect;

        strcpy(pGlobals->SupDrvFactory.szName, "VBoxNetTap");
        pGlobals->SupDrvFactory.pfnQueryFactoryInterface = vboxNetAdpQueryFactoryInterface;
    }

    return rc;
}

/**
 * performs the Idc initialization
 * we separate the globals initialization to globals "base" initialization which is actually
 * "general" globals initialization except for Idc not being initialized, and idc initialization.
 * This is needed for windows filter driver, which gets loaded prior to VBoxDrv,
 * thus it's not possible to make idc initialization from the driver startup routine for it.
 *
 * @returns VBox status code.
 * @param   pGlobals    Pointer to the globals. */
DECLHIDDEN(int) vboxNetAdpInitIdc(PVBOXNETADPGLOBALS pGlobals)
{
    int rc;
    /*
     * Establish a connection to SUPDRV and register our component factory.
     */
    rc = SUPR0IdcOpen(&pGlobals->SupDrvIDC, 0 /* iReqVersion = default */, 0 /* iMinVersion = default */, NULL, NULL, NULL);
    if (RT_SUCCESS(rc))
    {
        rc = SUPR0IdcComponentRegisterFactory(&pGlobals->SupDrvIDC, &pGlobals->SupDrvFactory);
        if (RT_SUCCESS(rc))
        {
            Log(("VBoxNetAdp: pSession=%p\n", SUPR0IdcGetSession(&pGlobals->SupDrvIDC)));
            return rc;
        }

        /* bail out. */
        LogRel(("VBoxNetAdp: Failed to register component factory, rc=%Rrc\n", rc));
        SUPR0IdcClose(&pGlobals->SupDrvIDC);
    }

    return rc;
}

/**
 * Called by the native driver/kext module initialization routine.
 *
 * It will initialize the common parts of the globals, assuming the caller
 * has already taken care of the OS specific bits.
 *
 * @returns VBox status code.
 * @param   pGlobals    Pointer to the globals.
 */
DECLHIDDEN(int) vboxNetAdpInitGlobals(PVBOXNETADPGLOBALS pGlobals)
{
    /*
     * Initialize the common portions of the structure.
     */
    int rc = vboxNetAdpInitGlobalsBase(pGlobals);
    if (RT_SUCCESS(rc))
    {
        rc = vboxNetAdpInitIdc(pGlobals);
        if (RT_SUCCESS(rc))
        {
            return rc;
        }

        /* bail out. */
        vboxNetAdpDeleteGlobalsBase(pGlobals);
    }

    return rc;
}

//////

int vboxNetAdpCreate (PINTNETTRUNKFACTORY pIfFactory, PVBOXNETADP *ppNew)
{
    int rc;
    unsigned i;
    PVBOXNETADPGLOBALS pGlobals = (PVBOXNETADPGLOBALS)((uint8_t *)pIfFactory - RT_OFFSETOF(VBOXNETADPGLOBALS, TrunkFactory));

    for (i = 0; i < RT_ELEMENTS(pGlobals->aAdapters); i++)
    {
        RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
        PVBOXNETADP pThis = &pGlobals->aAdapters[i];
    
        RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
        if (vboxNetAdpIsVoid(pThis))
        {
            RTMAC Mac;

            pThis->enmState = kVBoxNetAdpState_Transitional;
            RTSpinlockRelease(pThis->hSpinlock, &Tmp);
            /* Found an empty slot -- use it. */
            pThis->uUnit = i;
            vboxNetAdpComposeMACAddress(pThis, &Mac);
            rc = vboxNetAdpOsCreate(pThis, &Mac);
            *ppNew = pThis;
            RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
            pThis->enmState = kVBoxNetAdpState_Available;
            RTSpinlockRelease(pThis->hSpinlock, &Tmp);
            return rc;
        }
        RTSpinlockRelease(pThis->hSpinlock, &Tmp);
    }

    /* All slots in adapter array are busy. */
    return VERR_OUT_OF_RESOURCES;
}

int vboxNetAdpDestroy (PVBOXNETADP pThis)
{
    int rc;
    RTSPINLOCKTMP Tmp = RTSPINLOCKTMP_INITIALIZER;
    
    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    if (pThis->enmState != kVBoxNetAdpState_Available || pThis->cBusy)
    {
        RTSpinlockRelease(pThis->hSpinlock, &Tmp);
        return VERR_INTNET_FLT_IF_BUSY;
    }
    pThis->enmState = kVBoxNetAdpState_Transitional;
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);

    vboxNetAdpOsDestroy(pThis);

    RTSpinlockAcquire(pThis->hSpinlock, &Tmp);
    pThis->enmState = kVBoxNetAdpState_Invalid;
    RTSpinlockRelease(pThis->hSpinlock, &Tmp);

    return rc;
}

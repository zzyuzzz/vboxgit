/** @file
 * VUSB - VirtualBox USB.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
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
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___VBox_vusb_h
#define ___VBox_vusb_h

#include <VBox/cdefs.h>
#include <VBox/types.h>

__BEGIN_DECLS

/** @defgroup grp_vusb  VBox USB API
 * @{
 */

/** @defgroup grp_vusb_std  Standard Stuff
 * @{ */

/** Frequency of USB bus (from spec). */
#define VUSB_BUS_HZ                 12000000


/** @name USB Descriptor types (from spec)
 * @{ */
#define VUSB_DT_DEVICE                  0x01
#define VUSB_DT_CONFIG                  0x02
#define VUSB_DT_STRING                  0x03
#define VUSB_DT_INTERFACE               0x04
#define VUSB_DT_ENDPOINT                0x05
#define VUSB_DT_DEVICE_QUALIFIER        0x06
#define VUSB_DT_OTHER_SPEED_CFG         0x07
#define VUSB_DT_INTERFACE_POWER         0x08
/** @} */

/** @name USB Descriptor minimum sizes (from spec)
 * @{ */
#define VUSB_DT_DEVICE_MIN_LEN          18
#define VUSB_DT_CONFIG_MIN_LEN          9
#define VUSB_DT_CONFIG_STRING_MIN_LEN   2
#define VUSB_DT_INTERFACE_MIN_LEN       9
#define VUSB_DT_ENDPOINT_MIN_LEN        7
/** @} */


#pragma pack(1) /* ensure byte packing of the descriptors. */

/**
 * USB language id descriptor (from specs).
 */
typedef struct VUSBDESCLANGID
{
    uint8_t bLength;
    uint8_t bDescriptorType;
} VUSBDESCLANGID;
/** Pointer to a USB language id descriptor. */
typedef VUSBDESCLANGID *PVUSBDESCLANGID;
/** Pointer to a const USB language id descriptor. */
typedef const VUSBDESCLANGID *PCVUSBDESCLANGID;


/**
 * USB string descriptor (from specs).
 */
typedef struct VUSBDESCSTRING
{
    uint8_t bLength;
    uint8_t bDescriptorType;
} VUSBDESCSTRING;
/** Pointer to a USB string descriptor. */
typedef VUSBDESCSTRING *PVUSBDESCSTRING;
/** Pointer to a const USB string descriptor. */
typedef const VUSBDESCSTRING *PCVUSBDESCSTRING;


/**
 * USB device descriptor (from spec)
 */
typedef struct VUSBDESCDEVICE
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} VUSBDESCDEVICE;
/** Pointer to a USB device descriptor. */
typedef VUSBDESCDEVICE *PVUSBDESCDEVICE;
/** Pointer to a const USB device descriptor. */
typedef const VUSBDESCDEVICE *PCVUSBDESCDEVICE;


/**
 * USB configuration descriptor (from spec).
 */
typedef struct VUSBDESCCONFIG
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength; /**< recalculated by VUSB when involved in URB. */
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  MaxPower;
} VUSBDESCCONFIG;
/** Pointer to a USB configuration descriptor. */
typedef VUSBDESCCONFIG *PVUSBDESCCONFIG;
/** Pointer to a readonly USB configuration descriptor. */
typedef const VUSBDESCCONFIG *PCVUSBDESCCONFIG;


/**
 * USB interface descriptor (from spec)
 */
typedef struct VUSBDESCINTERFACE
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
} VUSBDESCINTERFACE;
/** Pointer to an USB interface descriptor. */
typedef VUSBDESCINTERFACE *PVUSBDESCINTERFACE;
/** Pointer to a const USB interface descriptor. */
typedef const VUSBDESCINTERFACE *PCVUSBDESCINTERFACE;


/**
 * USB endpoint descriptor (from spec)
 */
typedef struct VUSBDESCENDPOINT
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} VUSBDESCENDPOINT;
/** Pointer to an USB endpoint descriptor. */
typedef VUSBDESCENDPOINT *PVUSBDESCENDPOINT;
/** Pointer to a const USB endpoint descriptor. */
typedef const VUSBDESCENDPOINT *PCVUSBDESCENDPOINT;

#pragma pack() /* end of the byte packing. */


/**
 * USB configuration descriptor, the parsed variant used by VUSB.
 */
typedef struct VUSBDESCCONFIGEX
{
    /** The USB descriptor data.
     * @remark  The wTotalLength member is recalculated before the data is passed to the guest. */
    VUSBDESCCONFIG Core;
    /** Pointer to additional descriptor bytes following what's covered by VUSBDESCCONFIG. */
    void *pvMore;
    /** Pointer to an array of the interfaces referenced in the configuration.
     * Core.bNumInterfaces in size. */
    const struct VUSBINTERFACE *iface;
} VUSBDESCCONFIGEX;
/** Pointer to a parsed USB configuration descriptor. */
typedef VUSBDESCCONFIGEX *PVUSBDESCCONFIGEX;
/** Pointer to a const parsed USB configuration descriptor. */
typedef const VUSBDESCCONFIGEX *PCVUSBDESCCONFIGEX;


/**
 * For tracking the alternate interface settings of a configuration.
 */
typedef struct VUSBINTERFACE
{
    /** Pointer to an array of interfaces. */
    const struct VUSBDESCINTERFACEEX *setting;
    /** The number of entries in the array. */
    unsigned int num_settings;
} VUSBINTERFACE;
/** Pointer to a VUSBINTERFACE. */
typedef VUSBINTERFACE *PVUSBINTERFACE;
/** Pointer to a const VUSBINTERFACE. */
typedef const VUSBINTERFACE *PCVUSBINTERFACE;


/**
 * USB interface descriptor, the parsed variant used by VUSB.
 */
typedef struct VUSBDESCINTERFACEEX
{
    /** The USB descriptor data. */
    VUSBDESCINTERFACE Core;
    /** Pointer to additional descriptor bytes following what's covered by VUSBDESCINTERFACE. */
    void *pvMore;
    /** Pointer to an array of the endpoints referenced by the interface.
     * Core.bNumEndpoints in size. */
    const struct VUSBDESCENDPOINTEX *endpoint;
} VUSBDESCINTERFACEEX;
/** Pointer to an prased USB interface descriptor. */
typedef VUSBDESCINTERFACEEX *PVUSBDESCINTERFACEEX;
/** Pointer to a const parsed USB interface descriptor. */
typedef const VUSBDESCINTERFACEEX *PCVUSBDESCINTERFACEEX;


/**
 * USB endpoint descriptor, the parsed variant used by VUSB.
 */
typedef struct VUSBDESCENDPOINTEX
{
    /** The USB descriptor data.
     * @remark The wMaxPacketSize member is converted to native endian. */
    VUSBDESCENDPOINT Core;
    /** Pointer to additional descriptor bytes following what's covered by VUSBDESCENDPOINT. */
    void *pvMore;
} VUSBDESCENDPOINTEX;
/** Pointer to a parsed USB endpoint descriptor. */
typedef VUSBDESCENDPOINTEX *PVUSBDESCENDPOINTEX;
/** Pointer to a const parsed USB endpoint descriptor. */
typedef const VUSBDESCENDPOINTEX *PCVUSBDESCENDPOINTEX;


/** @name USB Control message recipient codes (from spec)
 * @{ */
#define VUSB_TO_DEVICE          0x0
#define VUSB_TO_INTERFACE       0x1
#define VUSB_TO_ENDPOINT        0x2
#define VUSB_TO_OTHER           0x3
#define VUSB_RECIP_MASK         0x1f
/** @} */

/** @name USB control pipe setup packet structure (from spec)
 * @{ */
#define VUSB_REQ_SHIFT          (5)
#define VUSB_REQ_STANDARD       (0x0 << VUSB_REQ_SHIFT)
#define VUSB_REQ_CLASS          (0x1 << VUSB_REQ_SHIFT)
#define VUSB_REQ_VENDOR         (0x2 << VUSB_REQ_SHIFT)
#define VUSB_REQ_RESERVED       (0x3 << VUSB_REQ_SHIFT)
#define VUSB_REQ_MASK           (0x3 << VUSB_REQ_SHIFT)
/** @} */

#define VUSB_DIR_TO_DEVICE      0x00
#define VUSB_DIR_TO_HOST        0x80
#define	VUSB_DIR_MASK           0x80

/**
 * USB Setup request (from spec)
 */
typedef struct vusb_setup
{
    uint8_t  bmRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} VUSBSETUP;
/** Pointer to a setup request. */
typedef VUSBSETUP *PVUSBSETUP;
/** Pointer to a const setup request. */
typedef const VUSBSETUP *PCVUSBSETUP;

/** @name USB Standard device requests (from spec)
 * @{ */
#define VUSB_REQ_GET_STATUS         0x00
#define VUSB_REQ_CLEAR_FEATURE      0x01
#define VUSB_REQ_SET_FEATURE        0x03
#define VUSB_REQ_SET_ADDRESS        0x05
#define VUSB_REQ_GET_DESCRIPTOR     0x06
#define VUSB_REQ_SET_DESCRIPTOR     0x07
#define VUSB_REQ_GET_CONFIGURATION  0x08
#define VUSB_REQ_SET_CONFIGURATION  0x09
#define VUSB_REQ_GET_INTERFACE      0x0a
#define VUSB_REQ_SET_INTERFACE      0x0b
#define VUSB_REQ_SYNCH_FRAME        0x0c
#define VUSB_REQ_MAX                0x0d
/** @} */

/** @} */ /* end of grp_vusb_std */



/** @name USB Standard version flags.
 * @{ */
/** Indicates USB 1.1 support. */
#define VUSB_STDVER_11              RT_BIT(1)
/** Indicates USB 2.0 support. */
#define VUSB_STDVER_20              RT_BIT(2)
/** @} */


/**
 * USB frame timer callback function.
 *
 * @param   pDevIns         Device instance of the device which registered the timer.
 * @param   pTimer          The timer handle.
 */
typedef DECLCALLBACK(void) FNUSBTIMERDEV(PPDMDEVINS pDevIns, PTMTIMER pTimer);
/** Pointer to a device timer callback function. */
typedef FNUSBTIMERDEV *PFNUSBTIMERDEV;

/** Pointer to a VBox USB device interface. */
typedef struct VUSBIDEVICE      *PVUSBIDEVICE;

/** Pointer to a VUSB RootHub port interface. */
typedef struct VUSBIROOTHUBPORT *PVUSBIROOTHUBPORT;

/** Pointer to a VBox USB timer interface. */
typedef struct VUSBITIMER       *PVUSBITIMER;

/** Pointer to an USB request descriptor. */
typedef struct VUSBURB          *PVUSBURB;



/**
 * VBox USB port bitmap.
 *
 * Bit 0 == Port 0, ... , Bit 127 == Port 127.
 */
typedef struct VUSBPORTBITMAP
{
    /** 128 bits */
    char ach[16];
} VUSBPORTBITMAP;
/** Pointer to a VBox USB port bitmap. */
typedef VUSBPORTBITMAP *PVUSBPORTBITMAP;


/**
 * The VUSB RootHub port interface provided by the HCI.
 */
typedef struct VUSBIROOTHUBPORT
{
    /**
     * Get the number of avilable ports in the hub.
     *
     * @returns The number of ports available.
     * @param   pInterface      Pointer to this structure.
     * @param   pAvailable      Bitmap indicating the available ports. Set bit == available port.
     */
    DECLR3CALLBACKMEMBER(unsigned, pfnGetAvailablePorts,(PVUSBIROOTHUBPORT pInterface, PVUSBPORTBITMAP pAvailable));

    /**
     * Gets the supported USB versions.
     *
     * @returns The mask of supported USB versions.
     * @param   pInterface      Pointer to this structure.
     */
    DECLR3CALLBACKMEMBER(uint32_t, pfnGetUSBVersions,(PVUSBIROOTHUBPORT pInterface));

    /**
     * A device is being attached to a port in the roothub.
     *
     * @param   pInterface      Pointer to this structure.
     * @param   pDev            Pointer to the device being attached.
     * @param   uPort           The port number assigned to the device.
     */
    DECLR3CALLBACKMEMBER(int, pfnAttach,(PVUSBIROOTHUBPORT pInterface, PVUSBIDEVICE pDev, unsigned uPort));

    /**
     * A device is being detached from a port in the roothub.
     *
     * @param   pInterface      Pointer to this structure.
     * @param   pDev            Pointer to the device being detached.
     * @param   uPort           The port number assigned to the device.
     */
    DECLR3CALLBACKMEMBER(void, pfnDetach,(PVUSBIROOTHUBPORT pInterface, PVUSBIDEVICE pDev, unsigned uPort));

    /**
     * Reset the root hub.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to this structure.
     * @param   pResetOnLinux   Whether or not to do real reset on linux.
     */
    DECLR3CALLBACKMEMBER(int, pfnReset,(PVUSBIROOTHUBPORT pInterface, bool fResetOnLinux));

    /**
     * Transfer completion callback routine.
     *
     * VUSB will call this when a transfer have been completed
     * in a one or another way.
     *
     * @param   pInterface      Pointer to this structure.
     * @param   pUrb            Pointer to the URB in question.
     */
    DECLR3CALLBACKMEMBER(void, pfnXferCompletion,(PVUSBIROOTHUBPORT pInterface, PVUSBURB urb));

    /**
     * Handle transfer errors.
     *
     * VUSB calls this when a transfer attempt failed. This function will respond
     * indicating wheter to retry or complete the URB with failure.
     *
     * @returns Retry indicator.
     * @param   pInterface      Pointer to this structure.
     * @param   pUrb            Pointer to the URB in question.
     */
    DECLR3CALLBACKMEMBER(bool, pfnXferError,(PVUSBIROOTHUBPORT pInterface, PVUSBURB pUrb));

    /** Alignment dummy. */
    RTR3PTR Alignment;

} VUSBIROOTHUBPORT;


/** Pointer to a VUSB RootHub connector interface. */
typedef struct VUSBIROOTHUBCONNECTOR *PVUSBIROOTHUBCONNECTOR;

/**
 * The VUSB RootHub connector interface provided by the VBox USB RootHub driver.
 */
typedef struct VUSBIROOTHUBCONNECTOR
{
    /**
     * Allocates a new URB for a transfer.
     *
     * Either submit using pfnSubmitUrb or free using VUSBUrbFree().
     *
     * @returns Pointer to a new URB.
     * @returns NULL on failure - try again later.
     *          This will not fail if the device wasn't found. We'll fail it
     *          at submit time, since that makes the usage of this api simpler.
     * @param   pInterface  Pointer to this struct.
     * @param   DstAddress  The destination address of the URB.
     * @param   cbData      The amount of data space required.
     * @param   cTds        The amount of TD space.
     */
    DECLR3CALLBACKMEMBER(PVUSBURB, pfnNewUrb,(PVUSBIROOTHUBCONNECTOR pInterface, uint8_t DstAddress, uint32_t cbData, uint32_t cTds));

    /**
     * Submits a URB for transfer.
     * The transfer will do asynchronously if possible.
     *
     * @returns VBox status code.
     * @param   pInterface  Pointer to this struct.
     * @param   pUrb        Pointer to the URB returned by pfnNewUrb.
     *                      The URB will be freed in case of failure.
     * @param   pLed        Pointer to USB Status LED
     */
    DECLR3CALLBACKMEMBER(int, pfnSubmitUrb,(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBURB pUrb, struct PDMLED *pLed));

    /**
     * Call to service asynchronous URB completions in a polling fashion.
     *
     * Reaped URBs will be finished by calling the completion callback,
     * thus there is no return code or input or anything from this function
     * except for potential state changes elsewhere.
     *
     * @returns VINF_SUCCESS if no URBs are pending upon return.
     * @returns VERR_TIMEOUT if one or more URBs are still in flight upon returning.
     * @returns Other VBox status code.
     *
     * @param   pInterface  Pointer to this struct.
     * @param   cMillies    Number of milliseconds to poll for completion.
     */
    DECLR3CALLBACKMEMBER(void, pfnReapAsyncUrbs,(PVUSBIROOTHUBCONNECTOR pInterface, unsigned cMillies));

    /**
     * Cancels and completes - with CRC failure - all in-flight async URBs.
     * This is typically done before saving a state.
     *
     * @param   pInterface  Pointer to this struct.
     */
    DECLR3CALLBACKMEMBER(void, pfnCancelAllUrbs,(PVUSBIROOTHUBCONNECTOR pInterface));

    /**
     * Attach the device to the root hub.
     * The device must not be attached to any hub for this call to succeed.
     *
     * @returns VBox status code.
     * @param   pInterface  Pointer to this struct.
     * @param   pDevice     Pointer to the device (interface) attach.
     */
    DECLR3CALLBACKMEMBER(int, pfnAttachDevice,(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice));

    /**
     * Detach the device from the root hub.
     * The device must already be attached for this call to succeed.
     *
     * @returns VBox status code.
     * @param   pInterface  Pointer to this struct.
     * @param   pDevice     Pointer to the device (interface) to detach.
     */
    DECLR3CALLBACKMEMBER(int, pfnDetachDevice,(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice));

} VUSBIROOTHUBCONNECTOR;


#ifdef IN_RING3
/** @copydoc VUSBIROOTHUBCONNECTOR::pfnNewUrb */
DECLINLINE(PVUSBURB) VUSBIRhNewUrb(PVUSBIROOTHUBCONNECTOR pInterface, uint32_t DstAddress, uint32_t cbData, uint32_t cTds)
{
    return pInterface->pfnNewUrb(pInterface, DstAddress, cbData, cTds);
}

/** @copydoc VUSBIROOTHUBCONNECTOR::pfnSubmitUrb */
DECLINLINE(int) VUSBIRhSubmitUrb(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBURB pUrb, struct PDMLED *pLed)
{
    return pInterface->pfnSubmitUrb(pInterface, pUrb, pLed);
}

/** @copydoc VUSBIROOTHUBCONNECTOR::pfnReapAsyncUrbs */
DECLINLINE(void) VUSBIRhReapAsyncUrbs(PVUSBIROOTHUBCONNECTOR pInterface, unsigned cMillies)
{
    pInterface->pfnReapAsyncUrbs(pInterface, cMillies);
}

/** @copydoc VUSBIROOTHUBCONNECTOR::pfnCancelAllUrbs */
DECLINLINE(void) VUSBIRhCancelAllUrbs(PVUSBIROOTHUBCONNECTOR pInterface)
{
    pInterface->pfnCancelAllUrbs(pInterface);
}

/** @copydoc VUSBIROOTHUBCONNECTOR::pfnAttachDevice */
DECLINLINE(int) VUSBIRhAttachDevice(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice)
{
    return pInterface->pfnAttachDevice(pInterface, pDevice);
}

/** @copydoc VUSBIROOTHUBCONNECTOR::pfnDetachDevice */
DECLINLINE(int) VUSBIRhDetachDevice(PVUSBIROOTHUBCONNECTOR pInterface, PVUSBIDEVICE pDevice)
{
    return pInterface->pfnDetachDevice(pInterface, pDevice);
}
#endif /* IN_RING3 */



/** Pointer to a Root Hub Configuration Interface. */
typedef struct VUSBIRHCONFIG *PVUSBIRHCONFIG;

/**
 * Root Hub Configuration Interface (intended for MAIN).
 */
typedef struct VUSBIRHCONFIG
{
    /**
     * Creates a USB proxy device and attaches it to the root hub.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the root hub configuration interface structure.
     * @param   pUuid           Pointer to the UUID for the new device.
     * @param   fRemote         Whether the device must use the VRDP backend.
     * @param   pszAddress      OS specific device address.
     * @param   pvBackend       An opaque pointer for the backend. Only used by
     *                          the VRDP backend so far.
     */
    DECLR3CALLBACKMEMBER(int, pfnCreateProxyDevice,(PVUSBIRHCONFIG pInterface, PCRTUUID pUuid, bool fRemote, const char *pszAddress, void *pvBackend));

    /**
     * Removes a USB proxy device from the root hub and destroys it.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the root hub configuration interface structure.
     * @param   pUuid           Pointer to the UUID for the device.
     */
    DECLR3CALLBACKMEMBER(int, pfnDestroyProxyDevice,(PVUSBIRHCONFIG pInterface, PCRTUUID pUuid));

} VUSBIRHCONFIG;

#ifdef IN_RING3
/** @copydoc  VUSBIRHCONFIG::pfnCreateProxyDevice */
DECLINLINE(int) VUSBIRhCreateProxyDevice(PVUSBIRHCONFIG pInterface, PCRTUUID pUuid, bool fRemote, const char *pszAddress, void *pvBackend)
{
    return pInterface->pfnCreateProxyDevice(pInterface, pUuid, fRemote, pszAddress, pvBackend);
}

/** @copydoc VUSBIRHCONFIG::pfnDestroyProxyDevice */
DECLINLINE(int) VUSBIRhDestroyProxyDevice(PVUSBIRHCONFIG pInterface, PCRTUUID pUuid)
{
    return pInterface->pfnDestroyProxyDevice(pInterface, pUuid);
}
#endif /* IN_RING3 */



/**
 * VUSB device reset completion callback function.
 * This is called by the reset thread when the reset has been completed.
 *
 * @param   pDev        Pointer to the virtual USB device core.
 * @param   rc      The VBox status code of the reset operation.
 * @param   pvUser      User specific argument.
 *
 * @thread  The reset thread or EMT.
 */
typedef DECLCALLBACK(void) FNVUSBRESETDONE(PVUSBIDEVICE pDevice, int rc, void *pvUser);
/** Pointer to a device reset completion callback function (FNUSBRESETDONE). */
typedef FNVUSBRESETDONE *PFNVUSBRESETDONE;

/**
 * The state of a VUSB Device.
 *
 * @remark  The order of these states is vital.
 */
typedef enum VUSBDEVICESTATE
{
    VUSB_DEVICE_STATE_INVALID = 0,
    VUSB_DEVICE_STATE_DETACHED,
    VUSB_DEVICE_STATE_ATTACHED,
    VUSB_DEVICE_STATE_POWERED,
    VUSB_DEVICE_STATE_DEFAULT,
    VUSB_DEVICE_STATE_ADDRESS,
    VUSB_DEVICE_STATE_CONFIGURED,
    VUSB_DEVICE_STATE_SUSPENDED,
    /** The device is being reset. Don't mess with it.
     * Next states: VUSB_DEVICE_STATE_DEFAULT, VUSB_DEVICE_STATE_DESTROYED
     */
    VUSB_DEVICE_STATE_RESET,
    /** The device has been destroy. */
    VUSB_DEVICE_STATE_DESTROYED,
    /** The usual 32-bit hack. */
    VUSB_DEVICE_STATE_32BIT_HACK = 0x7fffffff
} VUSBDEVICESTATE;


/**
 * USB Device Interface.
 */
typedef struct VUSBIDEVICE
{
    /**
     * Resets the device.
     *
     * Since a device reset shall take at least 10ms from the guest point of view,
     * it must be performed asynchronously. We create a thread which performs this
     * operation and ensures it will take at least 10ms.
     *
     * At times - like init - a synchronous reset is required, this can be done
     * by passing NULL for pfnDone.
     *
     * -- internal stuff, move it --
     * While the device is being reset it is in the VUSB_DEVICE_STATE_RESET state.
     * On completion it will be in the VUSB_DEVICE_STATE_DEFAULT state if successful,
     * or in the VUSB_DEVICE_STATE_DETACHED state if the rest failed.
     * -- internal stuff, move it --
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to this structure.
     * @param   fResetOnLinux   Set if we can permit a real reset and a potential logical
     *                          device reconnect on linux hosts.
     * @param   pfnDone         Pointer to the completion routine. If NULL a synchronous
     *                          reset  is preformed not respecting the 10ms.
     * @param   pvUser          User argument to the completion routine.
     * @param   pVM             Pointer to the VM handle if callback in EMT is required. (optional)
     */
    DECLR3CALLBACKMEMBER(int, pfnReset,(PVUSBIDEVICE pInterface, bool fResetOnLinux,
                                        PFNVUSBRESETDONE pfnDone, void *pvUser, PVM pVM));

    /**
     * Powers on the device.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the device interface structure.
     */
    DECLR3CALLBACKMEMBER(int, pfnPowerOn,(PVUSBIDEVICE pInterface));

    /**
     * Powers off the device.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the device interface structure.
     */
    DECLR3CALLBACKMEMBER(int, pfnPowerOff,(PVUSBIDEVICE pInterface));

    /**
     * Get the state of the device.
     *
     * @returns Device state.
     * @param   pInterface      Pointer to the device interface structure.
     */
    DECLR3CALLBACKMEMBER(VUSBDEVICESTATE, pfnGetState,(PVUSBIDEVICE pInterface));

} VUSBIDEVICE;


#ifdef IN_RING3
/**
 * Resets the device.
 *
 * Since a device reset shall take at least 10ms from the guest point of view,
 * it must be performed asynchronously. We create a thread which performs this
 * operation and ensures it will take at least 10ms.
 *
 * At times - like init - a synchronous reset is required, this can be done
 * by passing NULL for pfnDone.
 *
 * -- internal stuff, move it --
 * While the device is being reset it is in the VUSB_DEVICE_STATE_RESET state.
 * On completion it will be in the VUSB_DEVICE_STATE_DEFAULT state if successful,
 * or in the VUSB_DEVICE_STATE_DETACHED state if the rest failed.
 * -- internal stuff, move it --
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the device interface structure.
 * @param   fResetOnLinux   Set if we can permit a real reset and a potential logical
 *                          device reconnect on linux hosts.
 * @param   pfnDone         Pointer to the completion routine. If NULL a synchronous
 *                          reset  is preformed not respecting the 10ms.
 * @param   pvUser          User argument to the completion routine.
 * @param   pVM             Pointer to the VM handle if callback in EMT is required. (optional)
 */
DECLINLINE(int) VUSBIDevReset(PVUSBIDEVICE pInterface, bool fResetOnLinux, PFNVUSBRESETDONE pfnDone, void *pvUser, PVM pVM)
{
    return pInterface->pfnReset(pInterface, fResetOnLinux, pfnDone, pvUser, pVM);
}

/**
 * Powers on the device.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the device interface structure.
 */
DECLINLINE(int) VUSBIDevPowerOn(PVUSBIDEVICE pInterface)
{
    return pInterface->pfnPowerOn(pInterface);
}

/**
 * Powers off the device.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the device interface structure.
 */
DECLINLINE(int) VUSBIDevPowerOff(PVUSBIDEVICE pInterface)
{
    return pInterface->pfnPowerOff(pInterface);
}

/**
 * Get the state of the device.
 *
 * @returns Device state.
 * @param   pInterface      Pointer to the device interface structure.
 */
DECLINLINE(VUSBDEVICESTATE) VUSBIDevGetState(PVUSBIDEVICE pInterface)
{
    return pInterface->pfnGetState(pInterface);
}
#endif /* IN_RING3 */


/**
 * USB Timer Interface.
 * @todo r=bird: why is this code still here?
 */
typedef struct VUSBITIMER
{
    /**
     * Sets up initial frame timer parameters.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the timer interface structure.
     * @param   pfnCallback     Pointer to the timer callback function.
     * @param   rate            Requested frame rate (normally 1,000).
     */
    DECLR3CALLBACKMEMBER(int, pfnTimerSetup,(PVUSBITIMER pInterface, PFNUSBTIMERDEV pfnCallback, uint32_t rate));

    /**
     * Requests another tick of the frame timer.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the timer interface structure.
     */
    DECLR3CALLBACKMEMBER(int, pfnTimerSetNext,(PVUSBITIMER pInterface));

    /**
     * Stops the frame timer for the caller.
     *
     * @returns VBox status code.
     * @param   pInterface      Pointer to the timer interface structure.
     */
    DECLR3CALLBACKMEMBER(int, pfnTimerStop,(PVUSBITIMER pInterface));

} VUSBITIMER;


#ifdef IN_RING3
/**
 * Sets up initial frame timer parameters.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the timer interface structure.
 * @param   pfnCallback     Pointer to the timer callback function.
 * @param   rate            Requested frame rate (normally 1,000).
 */
DECLINLINE(int) VUSBITimerSetup(PVUSBITIMER pInterface, PFNUSBTIMERDEV pfnCallback, uint32_t rate)
{
    return pInterface->pfnTimerSetup(pInterface, pfnCallback, rate);
}

/**
 * Requests another tick of the USB frame timer.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the timer interface structure.
 */
DECLINLINE(int) VUSBITimerSetNext(PVUSBITIMER pInterface)
{
    return pInterface->pfnTimerSetNext(pInterface);
}

/**
 * Stops the USB frame timer for the caller.
 *
 * @returns VBox status code.
 * @param   pInterface      Pointer to the timer interface structure.
 */
DECLINLINE(int) VUSBITimerStop(PVUSBITIMER pInterface)
{
    return pInterface->pfnTimerStop(pInterface);
}
#endif /* IN_RING3 */


/** @name URB
 * @{ */

/**
 * VUSB Transfer status codes.
 */
typedef enum VUSBSTATUS
{
    /** Transer was ok. */
    VUSBSTATUS_OK = 0,
    /** Transfer stalled, endpoint halted. */
    VUSBSTATUS_STALL,
    /** Device not responding. */
    VUSBSTATUS_DNR,
    /** CRC error. */
    VUSBSTATUS_CRC,
    /** Data overrun error. */
    VUSBSTATUS_DATA_UNDERRUN,
    /** Data overrun error. */
    VUSBSTATUS_DATA_OVERRUN,
    /** The isochronous buffer hasn't been touched. */
    VUSBSTATUS_NOT_ACCESSED,
    /** Invalid status. */
    VUSBSTATUS_INVALID = 0x7f
} VUSBSTATUS;


/**
 * VUSB Transfer types.
 */
typedef enum VUSBXFERTYPE
{
    /** Control message. Used to represent a single control transfer. */
    VUSBXFERTYPE_CTRL = 0,
    /* Isochronous transfer. */
    VUSBXFERTYPE_ISOC,
    /** Bulk transfer. */
    VUSBXFERTYPE_BULK,
    /** Interrupt transfer. */
    VUSBXFERTYPE_INTR,
    /** Complete control message. Used to represent an entire control message. */
    VUSBXFERTYPE_MSG,
    /** Invalid transfer type. */
    VUSBXFERTYPE_INVALID = 0x7f
} VUSBXFERTYPE;


/**
 * VUSB transfer direction.
 */
typedef enum VUSBDIRECTION
{
    /** Setup */
    VUSBDIRECTION_SETUP = 0,
#define VUSB_DIRECTION_SETUP    VUSBDIRECTION_SETUP
    /** In - Device to host. */
    VUSBDIRECTION_IN = 1,
#define VUSB_DIRECTION_IN       VUSBDIRECTION_IN
    /** Out - Host to device. */
    VUSBDIRECTION_OUT = 2,
#define VUSB_DIRECTION_OUT  VUSBDIRECTION_OUT
    /** Invalid direction */
    VUSBDIRECTION_INVALID = 0x7f
} VUSBDIRECTION;

/**
 * The URB states
 */
typedef enum VUSBURBSTATE
{
    /** The usual invalid state. */
    VUSBURBSTATE_INVALID = 0,
    /** The URB is free, i.e. not in use.
     * Next state: ALLOCATED */
    VUSBURBSTATE_FREE,
    /** The URB is allocated, i.e. being prepared for submission.
     * Next state: FREE, IN_FLIGHT */
    VUSBURBSTATE_ALLOCATED,
    /** The URB is in flight.
     * Next state: REAPED, CANCELLED */
    VUSBURBSTATE_IN_FLIGHT,
    /** The URB has been reaped and is being completed.
     * Next state: FREE */
    VUSBURBSTATE_REAPED,
    /** The URB has been cancelled and is awaiting reaping and immediate freeing.
     * Next state: FREE */
    VUSBURBSTATE_CANCELLED,
    /** The end of the valid states (exclusive). */
    VUSBURBSTATE_END,
    /** The usual 32-bit blow up. */
    VUSBURBSTATE_32BIT_HACK = 0x7fffffff
} VUSBURBSTATE;


/**
 * Information about a isochronous packet.
 */
typedef struct VUSBURBISOCPKT
{
    /** The size of the packet.
     * IN: The packet size. I.e. the number of bytes to the next packet or end of buffer.
     * OUT: The actual size transfered. */
    uint16_t        cb;
    /** The offset of the packet. (Relative to VUSBURB::abData[0].)
     * OUT: This can be changed by the USB device if it does some kind of buffer squeezing. */
    uint16_t        off;
    /** The status of the transfer.
     * IN: VUSBSTATUS_INVALID
     * OUT: VUSBSTATUS_INVALID if nothing was done, otherwise the correct status. */
    VUSBSTATUS      enmStatus;
} VUSBURBISOCPKT;
/** Pointer to a isochronous packet. */
typedef VUSBURBISOCPKT *PVUSBURBISOCPTK;
/** Pointer to a const isochronous packet. */
typedef const VUSBURBISOCPKT *PCVUSBURBISOCPKT;

/**
 * Asynchronous USB request descriptor
 */
typedef struct VUSBURB
{
    /** URB magic value. */
    uint32_t        u32Magic;
    /** The USR state. */
    VUSBURBSTATE    enmState;
    /** URB description, can be null. intended for logging. */
    char           *pszDesc;

    /** The VUSB data. */
    struct VUSBURBVUSB
    {
        /** URB chain pointer. */
        PVUSBURB        pNext;
        /** URB chain pointer. */
        PVUSBURB       *ppPrev;
        /** Pointer to the original for control messages. */
        PVUSBURB        pCtrlUrb;
        /** Pointer to the VUSB device.
         * This may be NULL if the destination address is invalid. */
        struct VUSBDEV *pDev;
        /** Sepcific to the pfnFree function. */
        void           *pvFreeCtx;
        /**
         * Callback which will free the URB once it's reaped and completed.
         * @param   pUrb    The URB.
         */
        DECLCALLBACKMEMBER(void, pfnFree)(PVUSBURB pUrb);
        /** Submit timestamp. (logging only) */
        uint64_t        u64SubmitTS;
        /** The allocated data length. */
        uint32_t        cbDataAllocated;
        /** The allocated TD length. */
        uint32_t        cTdsAllocated;
    } VUsb;

    /** The host controller data. */
    struct VUSBURBHCI
    {
        /** The endpoint descriptor address. */
        RTGCPHYS32      EdAddr;
        /** Number of Tds in the array. */
        uint32_t        cTds;
        /** Pointer to an array of TD info items.*/
        struct VUSBURBHCITD
        {
            /** Type of TD (private) */
            uint32_t        TdType;
            /** The address of the */
            RTGCPHYS32      TdAddr;
            /** A copy of the TD. */
            uint32_t        TdCopy[16];
        }              *paTds;
        /** URB chain pointer. */
        PVUSBURB        pNext;
        /** When this URB was created.
         * (Used for isochronous frames and for logging.) */
        uint32_t        u32FrameNo;
        /** Flag indicating that the TDs have been unlinked. */
        bool            fUnlinked;
    } Hci;

    /** The device data. */
    struct VUSBURBDEV
    {
        /** Pointer to the proxy URB.  */
        void           *pvProxyUrb;
    } Dev;

    /** The USB device instance this belongs to.
     * This is NULL if the device address is invalid, in which case this belongs to the hub. */
    PPDMUSBINS      pUsbIns;
    /** The device address.
     * This is set at allocation time. */
    uint8_t         DstAddress;

    /** The endpoint.
     * IN: Must be set before submitting the URB.
     * @remark This does not have the high bit (direction) set! */
    uint8_t         EndPt;
    /** The transfer type.
     * IN: Must be set before submitting the URB. */
    VUSBXFERTYPE    enmType;
    /** The transfer direction.
     * IN: Must be set before submitting the URB. */
    VUSBDIRECTION   enmDir;
    /** Indicates whether it is OK to receive/send less data than requested.
     * IN: Must be initialized before submitting the URB. */
    bool            fShortNotOk;
    /** The transfer status.
     * OUT: This is set when reaping the URB. */
    VUSBSTATUS      enmStatus;

    /** The number of isochronous packets describe in aIsocPkts.
     * This is ignored when enmType isn't VUSBXFERTYPE_ISOC. */
    uint32_t        cIsocPkts;
    /** The iso packets within abData.
     * This is ignored when enmType isn't VUSBXFERTYPE_ISOC. */
    VUSBURBISOCPKT  aIsocPkts[8];

    /** The message length.
     * IN: The amount of data to send / receive - set at allocation time.
     * OUT: The amount of data sent / received. */
    uint32_t        cbData;
    /** The message data.
     * IN: On host to device transfers, the data to send.
     * OUT: On device to host transfers, the data to received. */
    uint8_t         abData[8*_1K];
} VUSBURB;

/** The magic value of a valid VUSBURB. (Murakami Haruki) */
#define VUSBURB_MAGIC   0x19490112

/** @} */


/** @} */

__END_DECLS

#endif

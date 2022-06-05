#ifndef _IPXE_DEVICE_H
#define _IPXE_DEVICE_H

/**
 * @file
 *
 * Device model
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <ipxe/list.h>
#include <ipxe/tables.h>
#include <ipxe/io.h>

#define NET_BOOT_NICS 4
struct interface;

/** A hardware device description */
struct device_description {
	/** Bus type
	 *
	 * This must be a BUS_TYPE_XXX constant.
	 */
	unsigned int bus_type;
	/** Location
	 *
	 * The interpretation of this field is bus-type-specific.
	 */
	unsigned int location;
	/** Vendor ID */
	unsigned int vendor;
	/** Device ID */
	unsigned int device;
	/** Device class */
	unsigned long class;
	/** I/O address */
	unsigned long ioaddr;
	/** IRQ */
	unsigned int irq;
};

/** PCI bus type */
#define BUS_TYPE_PCI 1

/** ISAPnP bus type */
#define BUS_TYPE_ISAPNP 2

/** EISA bus type */
#define BUS_TYPE_EISA 3

/** MCA bus type */
#define BUS_TYPE_MCA 4

/** ISA bus type */
#define BUS_TYPE_ISA 5

/** A hardware device */
struct device {
	/** Name */
	char name[16];
	/** Driver name */
	const char *driver_name;
	/** Device description */
	struct device_description desc;
	/** Devices on the same bus */
	struct list_head siblings;
	/** Devices attached to this device */
	struct list_head children;
	/** Bus device */
	struct device *parent;
};

/**
 * A root device
 *
 * Root devices are system buses such as PCI, EISA, etc.
 *
 */
struct root_device {
	/** Device chain
	 *
	 * A root device has a NULL parent field.
	 */
	struct device dev;
	/** Root device driver */
	struct root_driver *driver;
};

/** A root device driver */
struct root_driver {
	/**
	 * Add root device
	 *
	 * @v rootdev	Root device
	 * @ret rc	Return status code
	 *
	 * Called from probe_devices() for all root devices in the build.
	 */
	int ( * probe ) ( struct root_device *rootdev );
	/**
	 * Remove root device
	 *
	 * @v rootdev	Root device
	 *
	 * Called from remove_device() for all successfully-probed
	 * root devices.
	 */
	void ( * remove ) ( struct root_device *rootdev );
};

/** Root device table */
#define ROOT_DEVICES __table ( struct root_device, "root_devices" )

/** Declare a root device */
#define __root_device __table_entry ( ROOT_DEVICES, 01 )

extern int device_keep_count;
#ifdef VBOX
static inline uint8_t cmos2_read(uint8_t offset)
{
	outb(offset, 0x72);
	return inb(0x73);		
}
#endif

/**
 * Prevent devices from being removed on shutdown
 *
 */
static inline void devices_get ( void ) {
	device_keep_count++;
}

/**
 * Allow devices to be removed on shutdown
 *
 */
static inline void devices_put ( void ) {
	device_keep_count--;
}

extern struct device * identify_device ( struct interface *intf );
#define identify_device_TYPE( object_type ) \
	typeof ( struct device * ( object_type ) )

#endif /* _IPXE_DEVICE_H */

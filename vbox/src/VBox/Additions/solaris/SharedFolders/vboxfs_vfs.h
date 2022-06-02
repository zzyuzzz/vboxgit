/* $Id: vboxfs_vfs.h 30519 2010-06-30 08:04:26Z vboxsync $ */
/** @file
 * VirtualBox File System for Solaris Guests, VFS header.
 */

/*
 * Copyright (C) 2009-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef	___VBoxFS_vfs_Solaris_h
#define	___VBoxFS_vfs_Solaris_h

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Shared Folders filesystem per-mount data structure.
 */
typedef struct sffs_data {
	vfs_t		*sf_vfsp;	/* filesystem's vfs struct */
	vnode_t		*sf_rootnode;	/* of vnode of the root directory */
	uid_t		sf_uid;		/* owner of all shared folders */
	gid_t		sf_gid;		/* group of all shared folders */
	int  		sf_stat_ttl;	/* ttl for stat caches (in ms) */
	char		*sf_share_name;
	char 		*sf_mntpath;	/* name of mount point */
	sfp_mount_t	*sf_handle;
	uint64_t	sf_ino;		/* per FS ino generator */
} sffs_data_t;


#ifdef	__cplusplus
}
#endif

#endif	/* !___VBoxFS_vfs_Solaris_h */


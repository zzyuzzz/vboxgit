/* $Id: boot_service_table.h 56292 2015-06-09 14:20:46Z vboxsync $ */
/** @file
 * boot_service_table.h - boot service table declaration.
 */

/*
 * Copyright (C) 2009-2015 Oracle Corporation
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
 */

/* NVOID - Non VOID, RVOID - Return VOID */
/*
 * PRINTS FLAGS
 * SCL -type
 * PTR, PTR2, PTR3  - type *(**)(***)
 */
#if 0
/* Too much traffic */
TBL_ENTRY(EFI_RAISE_TPL, RaiseTPL, NVOID, EFI_TPL, 1, (SCL(EFI_TPL)))
TBL_ENTRY(EFI_RESTORE_TPL, RestoreTPL, RVOID, VOID, 1, (SCL(EFI_TPL)))
#endif
TBL_ENTRY(EFI_ALLOCATE_PAGES, AllocatePages, NVOID, EFI_STATUS, 4, (SCL(EFI_ALLOCATE_TYPE ), SCL(EFI_MEMORY_TYPE), SCL(UINTN), PTR(EFI_PHYSICAL_ADDRESS)))
TBL_ENTRY(EFI_FREE_PAGES,  FreePages, NVOID, EFI_STATUS, 2, (SCL(EFI_PHYSICAL_ADDRESS), SCL(UINTN)))
TBL_ENTRY(EFI_GET_MEMORY_MAP,  GetMemoryMap, NVOID, EFI_STATUS, 5, (PTR(UINTN), PTR(EFI_MEMORY_DESCRIPTOR), PTR(UINTN), PTR(UINTN), PTR(UINT32)))
TBL_ENTRY(EFI_ALLOCATE_POOL,  AllocatePool, NVOID, EFI_STATUS, 3, (SCL(EFI_MEMORY_TYPE), SCL(UINTN), PTR2(VOID)))
TBL_ENTRY(EFI_FREE_POOL, FreePool, NVOID, EFI_STATUS,  1, (PTR(VOID)))
TBL_ENTRY(EFI_CREATE_EVENT, CreateEvent, NVOID, EFI_STATUS, 5, (SCL(UINT32), SCL(EFI_TPL), SCL(EFI_EVENT_NOTIFY), PTR(VOID), PTR(EFI_EVENT)))
TBL_ENTRY(EFI_SET_TIMER, SetTimer, NVOID, EFI_STATUS, 3, (SCL(EFI_EVENT), SCL(EFI_TIMER_DELAY), SCL(UINT64)))
TBL_ENTRY(EFI_WAIT_FOR_EVENT, WaitForEvent, NVOID, EFI_STATUS, 3, (SCL(UINTN), PTR(EFI_EVENT), PTR(UINTN)))
TBL_ENTRY(EFI_SIGNAL_EVENT, SignalEvent, NVOID, EFI_STATUS, 1, (SCL(EFI_EVENT)))
TBL_ENTRY(EFI_CLOSE_EVENT, CloseEvent, NVOID, EFI_STATUS, 1, (SCL(EFI_EVENT)))
#if 0
/* Too much traffic */
TBL_ENTRY(EFI_CHECK_EVENT, CheckEvent, NVOID, EFI_STATUS, 1, (SCL(EFI_EVENT)))
#endif

TBL_ENTRY(EFI_INSTALL_PROTOCOL_INTERFACE, InstallProtocolInterface, NVOID, EFI_STATUS, 4, (PTR(EFI_HANDLE), PTR(EFI_GUID), SCL(EFI_INTERFACE_TYPE), PTR(VOID)))
TBL_ENTRY(EFI_REINSTALL_PROTOCOL_INTERFACE, ReinstallProtocolInterface, NVOID, EFI_STATUS, 4, (SCL(EFI_HANDLE), PTR(EFI_GUID), PTR(VOID), PTR(VOID)))
TBL_ENTRY(EFI_UNINSTALL_PROTOCOL_INTERFACE, UninstallProtocolInterface, NVOID, EFI_STATUS, 3, (SCL(EFI_HANDLE), PTR(EFI_GUID), PTR(VOID)))
TBL_ENTRY(EFI_HANDLE_PROTOCOL, HandleProtocol, NVOID, EFI_STATUS, 3, (SCL(EFI_HANDLE), PTR(EFI_GUID), PTR2(VOID)))
TBL_ENTRY(EFI_REGISTER_PROTOCOL_NOTIFY, RegisterProtocolNotify, NVOID, EFI_STATUS, 3, (PTR(EFI_GUID), SCL(EFI_EVENT), PTR2(VOID)))

TBL_ENTRY(EFI_LOCATE_HANDLE, LocateHandle, NVOID, EFI_STATUS, 5, (SCL(EFI_LOCATE_SEARCH_TYPE), PTR(EFI_GUID), PTR(VOID), PTR(UINTN), PTR(EFI_HANDLE)))

TBL_ENTRY(EFI_LOCATE_DEVICE_PATH, LocateDevicePath, NVOID, EFI_STATUS, 3, (PTR(EFI_GUID), PTR2(EFI_DEVICE_PATH_PROTOCOL), PTR(EFI_HANDLE)))
TBL_ENTRY(EFI_INSTALL_CONFIGURATION_TABLE, InstallConfigurationTable, NVOID, EFI_STATUS, 2, (PTR(EFI_GUID), PTR(VOID)))

TBL_ENTRY(EFI_IMAGE_LOAD, LoadImage, NVOID, EFI_STATUS, 6, (SCL(BOOLEAN), SCL(EFI_HANDLE), PTR(EFI_DEVICE_PATH_PROTOCOL), PTR(VOID), SCL(UINTN), PTR(EFI_HANDLE)))
TBL_ENTRY(EFI_IMAGE_START, StartImage, NVOID, EFI_STATUS, 3, (SCL(EFI_HANDLE), PTR(UINTN), PTR2(CHAR16)))

TBL_ENTRY(EFI_EXIT, Exit, NVOID, EFI_STATUS, 4, (SCL(EFI_HANDLE), SCL(EFI_STATUS), SCL(UINTN), PTR(CHAR16)))
TBL_ENTRY(EFI_IMAGE_UNLOAD, UnloadImage, NVOID, EFI_STATUS, 1, (SCL(EFI_HANDLE)))
TBL_ENTRY(EFI_EXIT_BOOT_SERVICES, ExitBootServices, NVOID, EFI_STATUS, 2, (SCL(EFI_HANDLE), SCL(UINTN)))

TBL_ENTRY(EFI_GET_NEXT_MONOTONIC_COUNT, GetNextMonotonicCount, NVOID, EFI_STATUS, 1, (PTR(UINT64)))
TBL_ENTRY(EFI_STALL, Stall, NVOID, EFI_STATUS, 1, (SCL(UINTN)))
TBL_ENTRY(EFI_SET_WATCHDOG_TIMER, SetWatchdogTimer, NVOID, EFI_STATUS, 4, (SCL(UINTN), SCL(UINT64), SCL(UINTN), PTR(CHAR16)))

TBL_ENTRY(EFI_CONNECT_CONTROLLER, ConnectController, NVOID, EFI_STATUS, 4, (SCL(EFI_HANDLE), PTR(EFI_HANDLE), PTR(EFI_DEVICE_PATH_PROTOCOL), SCL(BOOLEAN)))
TBL_ENTRY(EFI_DISCONNECT_CONTROLLER, DisconnectController, NVOID, EFI_STATUS, 3, (SCL(EFI_HANDLE), SCL(EFI_HANDLE), SCL(EFI_HANDLE)))

TBL_ENTRY(EFI_OPEN_PROTOCOL, OpenProtocol, NVOID, EFI_STATUS, 6, (SCL(EFI_HANDLE), PTR(EFI_GUID), PTR2(VOID), SCL(EFI_HANDLE), SCL(EFI_HANDLE), SCL(UINT32)))
TBL_ENTRY(EFI_CLOSE_PROTOCOL, CloseProtocol, NVOID, EFI_STATUS, 4, (SCL(EFI_HANDLE), PTR(EFI_GUID), SCL(EFI_HANDLE), SCL(EFI_HANDLE)))
TBL_ENTRY(EFI_OPEN_PROTOCOL_INFORMATION, OpenProtocolInformation, NVOID, EFI_STATUS, 4, (SCL(EFI_HANDLE), PTR(EFI_GUID), PTR2(EFI_OPEN_PROTOCOL_INFORMATION_ENTRY), PTR(UINTN)))

TBL_ENTRY(EFI_PROTOCOLS_PER_HANDLE, ProtocolsPerHandle, NVOID, EFI_STATUS, 3, (SCL(EFI_HANDLE), PTR3(EFI_GUID), PTR(UINTN)))
TBL_ENTRY(EFI_LOCATE_HANDLE_BUFFER, LocateHandleBuffer, NVOID, EFI_STATUS, 5, (SCL(EFI_LOCATE_SEARCH_TYPE), PTR(EFI_GUID), PTR(VOID), PTR(UINTN), PTR2(EFI_HANDLE)))
TBL_ENTRY(EFI_LOCATE_PROTOCOL, LocateProtocol, NVOID, EFI_STATUS, 3, (PTR(EFI_GUID), PTR(VOID), PTR2(VOID)))
#if 0
/* No var args */
TBL_ENTRY(EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES, InstallMultipleProtocolInterfaces)
TBL_ENTRY(EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES, UninstallMultipleProtocolInterfaces)
#endif
TBL_ENTRY(EFI_CALCULATE_CRC32, CalculateCrc32, NVOID, EFI_STATUS, 3, (PTR(VOID), SCL(UINTN), PTR(UINT32)))
TBL_ENTRY(EFI_COPY_MEM, CopyMem, RVOID, VOID, 3, (PTR(VOID), PTR(VOID), SCL(UINTN)))
TBL_ENTRY(EFI_SET_MEM, SetMem, RVOID, VOID, 3, (PTR(VOID), SCL(UINTN), SCL(UINT8)))
TBL_ENTRY(EFI_CREATE_EVENT_EX, CreateEventEx, NVOID, EFI_STATUS, 6, (SCL(UINT32), SCL(EFI_TPL), SCL(EFI_EVENT_NOTIFY), PTRC(VOID), PTRC(EFI_GUID), PTR(EFI_EVENT)))

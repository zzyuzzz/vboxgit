/* $Id: VBoxVMInfoNet.cpp 11982 2008-09-02 13:09:44Z vboxsync $ */
/** @file
 * VBoxVMInfoNet - Network information for the host.
 */

/*
 * Copyright (C) 2006-2007 Sun Microsystems, Inc.
 *
 * Sun Microsystems, Inc. confidential
 * All rights reserved
 */

#include "VBoxService.h"
#include "VBoxVMInfo.h"
#include "VBoxVMInfoNet.h"

int vboxVMInfoNet(VBOXINFORMATIONCONTEXT* a_pCtx)
{
    DWORD dwCurIface = 0;

    SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
    if (sd == SOCKET_ERROR)
    {
        Log(("vboxVMInfoThread: Failed to get a socket: Error %d\n", WSAGetLastError()));
        return -1;
    }

    INTERFACE_INFO InterfaceList[20];
    unsigned long nBytesReturned;
    if (    WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
                     sizeof(InterfaceList), &nBytesReturned, 0, 0)
        ==  SOCKET_ERROR)
    {
        Log(("vboxVMInfoThread: Failed calling WSAIoctl: Error: %d\n", WSAGetLastError()));
        return -1;
    }

    int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);
    Log(("vboxVMInfoThread: There are %d interfaces:\n", nNumInterfaces-1));

    dwCurIface = 0;

    for (int i = 0; i < nNumInterfaces; ++i)
    {
        if (InterfaceList[i].iiFlags & IFF_LOOPBACK)    /* Skip loopback device. */
            continue;

        sockaddr_in *pAddress;
        pAddress = (sockaddr_in *) & (InterfaceList[i].iiAddress);
        Log((" %s", inet_ntoa(pAddress->sin_addr)));

        pAddress = (sockaddr_in *) & (InterfaceList[i].iiBroadcastAddress);
        Log((" has bcast %s", inet_ntoa(pAddress->sin_addr)));

        pAddress = (sockaddr_in *) & (InterfaceList[i].iiNetmask);
        Log((" and netmask %s", inet_ntoa(pAddress->sin_addr)));

        Log((" Iface is "));
        u_long nFlags = InterfaceList[i].iiFlags;
        if (nFlags & IFF_UP) Log(("up"));
        else                 Log(("down"));
        if (nFlags & IFF_POINTTOPOINT) Log((", is point-to-point"));
        Log((", and can do: "));
        if (nFlags & IFF_BROADCAST) Log(("bcast " ));
        if (nFlags & IFF_MULTICAST) Log(("multicast "));
        Log(("\n"));

        /** @todo Add more information & storage here! */
    }

    closesocket(sd);

    return 0;
}


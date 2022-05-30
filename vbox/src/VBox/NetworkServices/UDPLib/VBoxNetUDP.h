/* $Id: VBoxNetUDP.h 17780 2009-03-12 22:23:53Z vboxsync $ */
/** @file
 * VBoxNetUDP - UDP Library for IntNet.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
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

#ifndef ___VBoxNetUDP_h___
#define ___VBoxNetUDP_h___

#include <iprt/net.h>
#include <VBox/intnet.h>

__BEGIN_DECLS


/**
 * Header pointers optionally returned by VBoxNetUDPMatch.
 */
typedef struct VBOXNETUDPHDRS
{
    PCRTNETETHERHDR     pEth;           /**< Pointer to the ethernet header. */
    PCRTNETIPV4         pIpv4;          /**< Pointer to the IPV4 header if IPV4 packet. */
    PCRTNETUDP          pUdp;           /**< Pointer to the UDP header. */
} VBOXNETUDPHDRS;
/** Pointer to a VBOXNETUDPHDRS structure. */
typedef VBOXNETUDPHDRS *PVBOXNETUDPHDRS;


/** @name VBoxNetUDPMatch flags.
 * @{ */
#define VBOXNETUDP_MATCH_UNICAST            RT_BIT_32(0)
#define VBOXNETUDP_MATCH_BROADCAST          RT_BIT_32(1)
#define VBOXNETUDP_MATCH_CHECKSUM           RT_BIT_32(2)
#define VBOXNETUDP_MATCH_REQUIRE_CHECKSUM   RT_BIT_32(3)
#define VBOXNETUDP_MATCH_PRINT_STDERR       RT_BIT_32(31)
/** @}  */

void *  VBoxNetUDPMatch(PCINTNETBUF pBuf, unsigned uDstPort, PCRTMAC pDstMac, uint32_t fFlags, PVBOXNETUDPHDRS pHdrs, size_t *pcb);
int     VBoxNetUDPUnicast(PSUPDRVSESSION pSession, INTNETIFHANDLE hIf, PINTNETBUF pBuf,
                          RTNETADDRIPV4 SrcIPv4Addr, PCRTMAC SrcMacAddr, unsigned uSrcPort,
                          RTNETADDRIPV4 DstIPv4Addr, PCRTMAC DstMacAddr, unsigned uDstPort,
                          void const *pvData, size_t cbData);
int     VBoxNetUDPBroadcast(PSUPDRVSESSION pSession, INTNETIFHANDLE hIf, PINTNETBUF pBuf,
                            RTNETADDRIPV4 SrcIPv4Addr, PCRTMAC SrcMacAddr, unsigned uSrcPort,
                            unsigned uDstPort,
                            void const *pvData, size_t cbData);

__END_DECLS

#endif


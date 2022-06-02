/** @file
 * INTNET - Internal Networking. (DEV,++)
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

#ifndef ___VBox_intnet_h
#define ___VBox_intnet_h

#include <VBox/types.h>
#include <VBox/vmm/stam.h>
#include <VBox/sup.h>
#include <iprt/assert.h>
#include <iprt/asm.h>

RT_C_DECLS_BEGIN


/**
 * Generic two-sided ring buffer.
 *
 * The deal is that there is exactly one writer and one reader.
 * When offRead equals offWrite the buffer is empty. In the other
 * extreme the writer will not use the last free byte in the buffer.
 */
typedef struct INTNETRINGBUF
{
    /** The offset from this structure to the start of the buffer. */
    uint32_t            offStart;
    /** The offset from this structure to the end of the buffer. (exclusive). */
    uint32_t            offEnd;
    /** The current read offset. */
    uint32_t volatile   offReadX;
    /** Alignment. */
    uint32_t            u32Align0;

    /** The committed write offset. */
    uint32_t volatile   offWriteCom;
    /** Writer internal current write offset.
     * This is ahead of offWriteCom when buffer space is handed to a third party for
     * data gathering.  offWriteCom will be assigned this value by the writer then
     * the frame is ready. */
    uint32_t volatile   offWriteInt;
    /** The number of bytes written (not counting overflows). */
    STAMCOUNTER         cbStatWritten;
    /** The number of frames written (not counting overflows). */
    STAMCOUNTER         cStatFrames;
    /** The number of overflows. */
    STAMCOUNTER         cOverflows;
} INTNETRINGBUF;
AssertCompileSize(INTNETRINGBUF, 48);
/** Pointer to a ring buffer. */
typedef INTNETRINGBUF *PINTNETRINGBUF;

/** The alignment of a ring buffer. */
#define INTNETRINGBUF_ALIGNMENT     sizeof(INTNETHDR)

/**
 * Asserts the sanity of the specified INTNETRINGBUF structure.
 */
#define INTNETRINGBUF_ASSERT_SANITY(pRingBuf) \
    do \
    { \
        AssertPtr(pRingBuf); \
        { \
            uint32_t const offWriteCom = (pRingBuf)->offWriteCom; \
            uint32_t const offRead     = (pRingBuf)->offReadX; \
            uint32_t const offWriteInt = (pRingBuf)->offWriteInt; \
            \
            AssertMsg(offWriteCom == RT_ALIGN_32(offWriteCom, INTNETHDR_ALIGNMENT), ("%#x\n", offWriteCom)); \
            AssertMsg(offWriteCom >= (pRingBuf)->offStart, ("%#x %#x\n", offWriteCom, (pRingBuf)->offStart)); \
            AssertMsg(offWriteCom <  (pRingBuf)->offEnd,   ("%#x %#x\n", offWriteCom, (pRingBuf)->offEnd)); \
            \
            AssertMsg(offRead == RT_ALIGN_32(offRead, INTNETHDR_ALIGNMENT), ("%#x\n", offRead)); \
            AssertMsg(offRead >= (pRingBuf)->offStart, ("%#x %#x\n", offRead, (pRingBuf)->offStart)); \
            AssertMsg(offRead <  (pRingBuf)->offEnd,   ("%#x %#x\n", offRead, (pRingBuf)->offEnd)); \
            \
            AssertMsg(offWriteInt == RT_ALIGN_32(offWriteInt, INTNETHDR_ALIGNMENT), ("%#x\n", offWriteInt)); \
            AssertMsg(offWriteInt >= (pRingBuf)->offStart, ("%#x %#x\n", offWriteInt, (pRingBuf)->offStart)); \
            AssertMsg(offWriteInt <  (pRingBuf)->offEnd,   ("%#x %#x\n", offWriteInt, (pRingBuf)->offEnd)); \
            AssertMsg(  offRead <= offWriteCom \
                      ? offWriteCom <= offWriteInt || offWriteInt < offRead \
                      : offWriteCom <= offWriteInt, \
                      ("W=%#x W'=%#x R=%#x\n", offWriteCom, offWriteInt, offRead)); \
        } \
    } while (0)



/**
 * A interface buffer.
 */
typedef struct INTNETBUF
{
    /** Magic number (INTNETBUF_MAGIC). */
    uint32_t        u32Magic;
    /** The size of the entire buffer. */
    uint32_t        cbBuf;
    /** The size of the send area. */
    uint32_t        cbSend;
    /** The size of the receive area. */
    uint32_t        cbRecv;
    /** The receive buffer. */
    INTNETRINGBUF   Recv;
    /** The send buffer. */
    INTNETRINGBUF   Send;
    /** Number of times yields help solve an overflow. */
    STAMCOUNTER     cStatYieldsOk;
    /** Number of times yields didn't help solve an overflow. */
    STAMCOUNTER     cStatYieldsNok;
    /** Number of lost packets due to overflows. */
    STAMCOUNTER     cStatLost;
    /** Number of bad frames (both rings). */
    STAMCOUNTER     cStatBadFrames;
    /** Reserved for future use. */
    STAMCOUNTER     aStatReserved[2];
    /** Reserved for future send profiling. */
    STAMPROFILE     StatSend1;
    /** Reserved for future send profiling. */
    STAMPROFILE     StatSend2;
    /** Reserved for future receive profiling. */
    STAMPROFILE     StatRecv1;
    /** Reserved for future receive profiling. */
    STAMPROFILE     StatRecv2;
    /** Reserved for future profiling. */
    STAMPROFILE     StatReserved;
} INTNETBUF;
AssertCompileSize(INTNETBUF, 320);
AssertCompileMemberOffset(INTNETBUF, Recv, 16);
AssertCompileMemberOffset(INTNETBUF, Send, 64);

/** Pointer to an interface buffer. */
typedef INTNETBUF *PINTNETBUF;
/** Pointer to a const interface buffer. */
typedef INTNETBUF const *PCINTNETBUF;

/** Magic number for INTNETBUF::u32Magic (Sir William Gerald Golding). */
#define INTNETBUF_MAGIC             UINT32_C(0x19110919)

/**
 * Asserts the sanity of the specified INTNETBUF structure.
 */
#define INTNETBUF_ASSERT_SANITY(pBuf) \
    do \
    { \
        AssertPtr(pBuf); \
        Assert((pBuf)->u32Magic == INTNETBUF_MAGIC); \
        { \
            uint32_t const offRecvStart = (pBuf)->Recv.offStart + RT_OFFSETOF(INTNETBUF, Recv); \
            uint32_t const offRecvEnd   = (pBuf)->Recv.offStart + RT_OFFSETOF(INTNETBUF, Recv); \
            uint32_t const offSendStart = (pBuf)->Send.offStart + RT_OFFSETOF(INTNETBUF, Send); \
            uint32_t const offSendEnd   = (pBuf)->Send.offStart + RT_OFFSETOF(INTNETBUF, Send); \
            \
            Assert(offRecvEnd > offRecvStart); \
            Assert(offRecvEnd - offRecvStart == (pBuf)->cbRecv); \
            Assert(offRecvStart == sizeof(INTNETBUF)); \
            \
            Assert(offSendEnd > offSendStart); \
            Assert(offSendEnd - offSendStart == (pBuf)->cbSend); \
            Assert(pffSendEnd <= (pBuf)->cbBuf); \
            \
            Assert(offSendStart == offRecvEnd); \
        } \
    } while (0)


/** Internal networking interface handle. */
typedef uint32_t    INTNETIFHANDLE;
/** Pointer to an internal networking interface handle. */
typedef INTNETIFHANDLE *PINTNETIFHANDLE;

/** Or mask to obscure the handle index. */
#define INTNET_HANDLE_MAGIC         0x88880000
/** Mask to extract the handle index. */
#define INTNET_HANDLE_INDEX_MASK    0xffff
/** The maximum number of handles (exclusive) */
#define INTNET_HANDLE_MAX           0xffff
/** Invalid handle. */
#define INTNET_HANDLE_INVALID       (0)


/**
 * The frame header.
 *
 * The header is intentionally 8 bytes long. It will always
 * start at an 8 byte aligned address. Assuming that the buffer
 * size is a multiple of 8 bytes, that means that we can guarantee
 * that the entire header is contiguous in both virtual and physical
 * memory.
 */
typedef struct INTNETHDR
{
    /** Header type. This is currently serving as a magic, it
     * can be extended later to encode special command frames and stuff. */
    uint16_t        u16Type;
    /** The size of the frame. */
    uint16_t        cbFrame;
    /** The offset from the start of this header to where the actual frame starts.
     * This is used to keep the frame it self contiguous in virtual memory and
     * thereby both simplify access as well as the descriptor. */
    int32_t         offFrame;
} INTNETHDR;
AssertCompileSize(INTNETHDR, 8);
AssertCompileSizeAlignment(INTNETBUF, sizeof(INTNETHDR));
/** Pointer to a frame header.*/
typedef INTNETHDR *PINTNETHDR;
/** Pointer to a const frame header.*/
typedef INTNETHDR const *PCINTNETHDR;

/** The alignment of a frame header. */
#define INTNETHDR_ALIGNMENT         sizeof(INTNETHDR)
AssertCompile(sizeof(INTNETHDR) == INTNETHDR_ALIGNMENT);
AssertCompile(INTNETHDR_ALIGNMENT <= INTNETRINGBUF_ALIGNMENT);

/** @name Frame types (INTNETHDR::u16Type).
 * @{ */
/** Normal frames. */
#define INTNETHDR_TYPE_FRAME        0x2442
/** Padding frames. */
#define INTNETHDR_TYPE_PADDING      0x3553
/** Generic segment offload frames.
 * The frame starts with a PDMNETWORKGSO structure which is followed by the
 * header template and data. */
#define INTNETHDR_TYPE_GSO          0x4664
AssertCompileSize(PDMNETWORKGSO, 8);
/** @}  */

/**
 * Asserts the sanity of the specified INTNETHDR.
 */
#define INTNETHDR_ASSERT_SANITY(pHdr, pRingBuf) \
    do \
    { \
        AssertPtr(pHdr); \
        Assert(RT_ALIGN_PT(pHdr, INTNETHDR_ALIGNMENT, INTNETHDR *) == pHdr); \
        Assert(   (pHdr)->u16Type == INTNETHDR_TYPE_FRAME \
               || (pHdr)->u16Type == INTNETHDR_TYPE_GSO \
               || (pHdr)->u16Type == INTNETHDR_TYPE_PADDING); \
        { \
            uintptr_t const offHdr   = (uintptr_t)pHdr - (uintptr_t)pRingBuf; \
            uintptr_t const offFrame = offHdr + (pHdr)->offFrame; \
            \
            Assert(offHdr >= (pRingBuf)->offStart); \
            Assert(offHdr <  (pRingBuf)->offEnd); \
            \
            /* could do more thorough work here... later, perhaps. */ \
            Assert(offFrame >= (pRingBuf)->offStart); \
            Assert(offFrame <  (pRingBuf)->offEnd); \
        } \
    } while (0)


/**
 * Scatter / Gather segment (internal networking).
 */
typedef struct INTNETSEG
{
    /** The physical address. NIL_RTHCPHYS is not set. */
    RTHCPHYS        Phys;
    /** Pointer to the segment data. */
    void           *pv;
    /** The segment size. */
    uint32_t        cb;
} INTNETSEG;
/** Pointer to a internal networking frame segment. */
typedef INTNETSEG *PINTNETSEG;
/** Pointer to a internal networking frame segment. */
typedef INTNETSEG const *PCINTNETSEG;


/**
 * Scatter / Gather list (internal networking).
 *
 * This is used when communicating with the trunk port.
 */
typedef struct INTNETSG
{
    /** Owner data, don't touch! */
    void               *pvOwnerData;
    /** User data. */
    void               *pvUserData;
    /** User data 2 in case anyone needs it. */
    void               *pvUserData2;
    /** GSO context information, set the type to invalid if not relevant. */
    PDMNETWORKGSO       GsoCtx;
    /** The total length of the scatter gather list. */
    uint32_t            cbTotal;
    /** The number of users (references).
     * This is used by the SGRelease code to decide when it can be freed. */
    uint16_t volatile   cUsers;
    /** Flags, see INTNETSG_FLAGS_* */
    uint16_t volatile   fFlags;
#if ARCH_BITS == 64
    /** Alignment padding. */
    uint16_t            uPadding;
#endif
    /** The number of segments allocated. */
    uint16_t            cSegsAlloc;
    /** The number of segments actually used. */
    uint16_t            cSegsUsed;
    /** Variable sized list of segments. */
    INTNETSEG           aSegs[1];
} INTNETSG;
AssertCompileSizeAlignment(INTNETSG, 8);
/** Pointer to a scatter / gather list. */
typedef INTNETSG *PINTNETSG;
/** Pointer to a const scatter / gather list. */
typedef INTNETSG const *PCINTNETSG;

/** @name INTNETSG::fFlags definitions.
 * @{ */
/** Set if the SG is free. */
#define INTNETSG_FLAGS_FREE             RT_BIT_32(1)
/** Set if the SG is a temporary one that will become invalid upon return.
 * Try to finish using it before returning, and if that's not possible copy
 * to other buffers.
 * When not set, the callee should always free the SG.
 * Attempts to free it made by the callee will be quietly ignored. */
#define INTNETSG_FLAGS_TEMP             RT_BIT_32(2)
/** ARP packet, IPv4 + MAC.
 * @internal */
#define INTNETSG_FLAGS_ARP_IPV4         RT_BIT_32(3)
/** Copied to the temporary buffer.
 * @internal */
#define INTNETSG_FLAGS_PKT_CP_IN_TMP    RT_BIT_32(4)
/** @} */


/** @name Direction (frame source or destination)
 * @{ */
/** To/From the wire. */
#define INTNETTRUNKDIR_WIRE             RT_BIT_32(0)
/** To/From the host. */
#define INTNETTRUNKDIR_HOST             RT_BIT_32(1)
/** Mask of valid bits. */
#define INTNETTRUNKDIR_VALID_MASK       UINT32_C(3)
/** @} */

/**
 * Switch decisions returned by INTNETTRUNKSWPORT::pfnPreRecv.
 */
typedef enum INTNETSWDECISION
{
    /** The usual invalid zero value. */
    INTNETSWDECISION_INVALID = 0,
    /** Everywhere. */
    INTNETSWDECISION_BROADCAST,
    /** Only to the internal network. */
    INTNETSWDECISION_INTNET,
    /** Only for the trunk (host/wire). */
    INTNETSWDECISION_TRUNK,
    /** Used internally to indicate that the packet cannot be handled in the
     * current context. */
    INTNETSWDECISION_BAD_CONTEXT,
    /** Used internally to indicate that the packet should be dropped. */
    INTNETSWDECISION_DROP,
    /** The usual 32-bit type expansion. */
    INTNETSWDECISION_32BIT_HACK = 0x7fffffff
} INTNETSWDECISION;


/** Pointer to the switch side of a trunk port. */
typedef struct INTNETTRUNKSWPORT *PINTNETTRUNKSWPORT;
/**
 * This is the port on the internal network 'switch', i.e.
 * what the driver is connected to.
 *
 * This is only used for the in-kernel trunk connections.
 */
typedef struct INTNETTRUNKSWPORT
{
    /** Structure version number. (INTNETTRUNKSWPORT_VERSION) */
    uint32_t u32Version;

    /**
     * Examine the packet and figure out where it is going.
     *
     * This method is for making packet switching decisions in contexts where
     * pfnRecv cannot be called or is no longer applicable.  This method can be
     * called from any context.
     *
     * @returns INTNETSWDECISION_BROADCAST, INTNETSWDECISION_INTNET or
     *          INTNETSWDECISION_TRUNK.  The source is excluded from broadcast &
     *          trunk, of course.
     *
     * @param   pSwitchPort Pointer to this structure.
     * @param   pvHdrs      Pointer to the packet headers.
     * @param   cbHdrs      Size of the packet headers.  This must be at least 6
     *                      bytes (the destination MAC address), but should if
     *                      possible also include any VLAN tag and network
     *                      layer header (wireless mac address sharing).
     * @param   fSrc        Where this frame comes from.  Only one bit should be
     *                      set!
     *
     * @remarks Will only grab the switch table spinlock (interrupt safe).  May
     *          signal an event semaphore iff we're racing network cleanup.  The
     *          caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(INTNETSWDECISION, pfnPreRecv,(PINTNETTRUNKSWPORT pSwitchPort,
                                                       void const *pvHdrs, size_t cbHdrs, uint32_t fSrc));

    /**
     * Incoming frame.
     *
     * The frame may be modified when the trunk port on the switch is set to share
     * the mac address of the host when hitting the wire.  Currently frames
     * containing ARP packets are subject to this, later other protocols like
     * NDP/ICMPv6 may need editing as well when operating in this mode.  The edited
     * packet should be forwarded to the host/wire when @c false is returned.
     *
     * @returns true if we've handled it and it should be dropped.
     *          false if it should hit the wire/host.
     *
     * @param   pSwitchPort Pointer to this structure.
     * @param   pvIf        Pointer to the interface which received this frame
     *                      if available.  Can be NULL.
     * @param   pSG         The (scatter /) gather structure for the frame.  This
     *                      will only be use during the call, so a temporary one can
     *                      be used.  The Phys member will not be used.
     * @param   fSrc        Where this frame comes from.  Exactly one bit shall be
     *                      set!
     *
     * @remarks Will only grab the switch table spinlock (interrupt safe).  Will
     *          signal event semaphores.  The caller must be busy when calling.
     *
     * @remarks NAT and TAP will use this interface.
     *
     * @todo    Do any of the host require notification before frame modifications?
     *          If so, we'll add a callback to INTNETTRUNKIFPORT for this
     *          (pfnSGModifying) and a SG flag.
     */
    DECLR0CALLBACKMEMBER(bool, pfnRecv,(PINTNETTRUNKSWPORT pSwitchPort, void *pvIf, PINTNETSG pSG, uint32_t fSrc));

    /**
     * Retain a SG.
     *
     * @param   pSwitchPort Pointer to this structure.
     * @param   pSG         Pointer to the (scatter /) gather structure.
     *
     * @remarks Will not grab any locks.  The caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(void, pfnSGRetain,(PINTNETTRUNKSWPORT pSwitchPort, PINTNETSG pSG));

    /**
     * Release a SG.
     *
     * This is called by the pfnXmit code when done with a SG.  This may safe
     * be done in an asynchronous manner.
     *
     * @param   pSwitchPort Pointer to this structure.
     * @param   pSG         Pointer to the (scatter /) gather structure.
     *
     * @remarks May signal an event semaphore later on, currently code won't though.
     *          The caller is busy when making this call.
     */
    DECLR0CALLBACKMEMBER(void, pfnSGRelease,(PINTNETTRUNKSWPORT pSwitchPort, PINTNETSG pSG));

    /**
     * Selects whether outgoing SGs should have their physical address set.
     *
     * By enabling physical addresses in the scatter / gather segments it should
     * be possible to save some unnecessary address translation and memory locking
     * in the network stack.  (Internal networking knows the physical address for
     * all the INTNETBUF data and that it's locked memory.)  There is a negative
     * side effects though, frames that crosses page boundaries will require
     * multiple scather / gather segments.
     *
     * @returns The old setting.
     *
     * @param   pSwitchPort Pointer to this structure.
     * @param   fEnable     Whether to enable or disable it.
     *
     * @remarks Will not grab any locks.  The caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(bool, pfnSetSGPhys,(PINTNETTRUNKSWPORT pSwitchPort, bool fEnable));

    /**
     * Reports the MAC address of the trunk.
     *
     * This is supposed to be called when creating, connection or reconnecting the
     * trunk and when the MAC address is changed by the system admin.
     *
     * @param   pSwitchPort         Pointer to this structure.
     * @param   pMacAddr            The MAC address.
     *
     * @remarks May take a spinlock or two.  The caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(void, pfnReportMacAddress,(PINTNETTRUNKSWPORT pSwitchPort, PCRTMAC pMacAddr));

    /**
     * Reports the promicuousness of the interface.
     *
     * This is supposed to be called when creating, connection or reconnecting the
     * trunk and when the mode is changed by the system admin.
     *
     * @param   pSwitchPort         Pointer to this structure.
     * @param   fPromiscuous        True if the host operates the interface in
     *                              promiscuous mode, false if not.
     *
     * @remarks May take a spinlock or two.  The caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(void, pfnReportPromiscuousMode,(PINTNETTRUNKSWPORT pSwitchPort, bool fPromiscuous));

    /**
     * Reports the GSO capabilities of the host, wire or both.
     *
     * This is supposed to be used only when creating, connecting or reconnecting
     * the trunk.  It is assumed that the GSO capabilities are kind of static the
     * rest of the time.
     *
     * @param   pSwitchPort         Pointer to this structure.
     * @param   fGsoCapabilities    The GSO capability bit mask.  The bits
     *                              corresponds to the GSO type with the same value.
     * @param   fDst                The destination mask (INTNETTRUNKDIR_XXX).
     *
     * @remarks Does not take any locks.  The caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(void, pfnReportGsoCapabilities,(PINTNETTRUNKSWPORT pSwitchPort, uint32_t fGsoCapabilities, uint32_t fDst));

    /**
     * Reports the no-preemption-xmit capabilities of the host and wire.
     *
     * This is supposed to be used only when creating, connecting or reconnecting
     * the trunk.  It is assumed that the GSO capabilities are kind of static the
     * rest of the time.
     *
     * @param   pSwitchPort         Pointer to this structure.
     * @param   fNoPreemptDsts      The destinations  (INTNETTRUNKDIR_XXX) which it
     *                              is safe to transmit to with preemption disabled.
     * @param   fDst                The destination mask (INTNETTRUNKDIR_XXX).
     *
     * @remarks Does not take any locks.  The caller must be busy when calling.
     */
    DECLR0CALLBACKMEMBER(void, pfnReportNoPreemptDsts,(PINTNETTRUNKSWPORT pSwitchPort, uint32_t fNoPreemptDsts));

    /** Structure version number. (INTNETTRUNKSWPORT_VERSION) */
    uint32_t u32VersionEnd;
} INTNETTRUNKSWPORT;

/** Version number for the INTNETTRUNKIFPORT::u32Version and INTNETTRUNKIFPORT::u32VersionEnd fields. */
#define INTNETTRUNKSWPORT_VERSION   UINT32_C(0xA2CDf001)


/**
 * The trunk interface state used set by INTNETTRUNKIFPORT::pfnSetState.
 */
typedef enum INTNETTRUNKIFSTATE
{
    /** The invalid zero entry. */
    INTNETTRUNKIFSTATE_INVALID = 0,
    /** The trunk is inactive.  No calls to INTNETTRUNKSWPORT::pfnRecv or
     *  INTNETTRUNKSWPORT::pfnPreRecv.  Calling other methods is OK. */
    INTNETTRUNKIFSTATE_INACTIVE,
    /** The trunk is active, no restrictions on methods or anything. */
    INTNETTRUNKIFSTATE_ACTIVE,
    /** The trunk is about to be disconnected from the internal network.  No
     *  calls to any INTNETRUNKSWPORT methods. */
    INTNETTRUNKIFSTATE_DISCONNECTING,
    /** The end of the valid states. */
    INTNETTRUNKIFSTATE_END,
    /** The usual 32-bit type blow up hack. */
    INTNETTRUNKIFSTATE_32BIT_HACK = 0x7fffffff
} INTNETTRUNKIFSTATE;

/** Pointer to the interface side of a trunk port. */
typedef struct INTNETTRUNKIFPORT *PINTNETTRUNKIFPORT;
/**
 * This is the port on the trunk interface, i.e. the driver side which the
 * internal network is connected to.
 *
 * This is only used for the in-kernel trunk connections.
 */
typedef struct INTNETTRUNKIFPORT
{
    /** Structure version number. (INTNETTRUNKIFPORT_VERSION) */
    uint32_t u32Version;

    /**
     * Retain the object.
     *
     * It will normally be called while owning the internal network semaphore.
     *
     * @param   pIfPort     Pointer to this structure.
     *
     * @remarks May own the big mutex, no spinlocks.
     */
    DECLR0CALLBACKMEMBER(void, pfnRetain,(PINTNETTRUNKIFPORT pIfPort));

    /**
     * Releases the object.
     *
     * This must be called for every pfnRetain call.
     *
     *
     * @param   pIfPort     Pointer to this structure.
     *
     * @remarks May own the big mutex, no spinlocks.
     */
    DECLR0CALLBACKMEMBER(void, pfnRelease,(PINTNETTRUNKIFPORT pIfPort));

    /**
     * Disconnect from the switch and release the object.
     *
     * The is the counter action of the
     * INTNETTRUNKNETFLTFACTORY::pfnCreateAndConnect method.
     *
     * @param   pIfPort     Pointer to this structure.
     *
     * @remarks Owns the big mutex.
     */
    DECLR0CALLBACKMEMBER(void, pfnDisconnectAndRelease,(PINTNETTRUNKIFPORT pIfPort));

    /**
     * Changes the state of the trunk interface.
     *
     * The interface is created in the inactive state (INTNETTRUNKIFSTATE_INACTIVE).
     * When the first connect VM or service is activated, the internal network
     * activates the trunk (INTNETTRUNKIFSTATE_ACTIVE).  The state may then be set
     * back and forth between INACTIVE and ACTIVE as VMs are paused, added and
     * removed.
     *
     * Eventually though, the network is destroyed as a result of there being no
     * more VMs left in it and the state is changed to disconnecting
     * (INTNETTRUNKIFSTATE_DISCONNECTING) and pfnWaitForIdle is called to make sure
     * there are no active calls in either direction when pfnDisconnectAndRelease is
     * called.
     *
     * A typical operation to performed by this method is to enable/disable promiscuous
     * mode on the host network interface when entering/leaving the active state.
     *
     * @returns The previous state.
     *
     * @param   pIfPort     Pointer to this structure.
     * @param   enmState    The new state.
     *
     * @remarks Owns the big mutex.  No racing pfnSetState, pfnWaitForIdle,
     *          pfnDisconnectAndRelease or INTNETTRUNKFACTORY::pfnCreateAndConnect
     *          calls.
     */
    DECLR0CALLBACKMEMBER(INTNETTRUNKIFSTATE, pfnSetState,(PINTNETTRUNKIFPORT pIfPort, INTNETTRUNKIFSTATE enmState));

    /**
     * Notifies when the MAC address of an interface is set or changes.
     *
     * @param   pIfPort     Pointer to this structure.
     * @param   pvIfData    Pointer to the trunk's interface data (see
     *                      pfnConnectInterface).
     * @param   pMac        Pointer to the MAC address of the connecting VM NIC.
     *
     * @remarks Only busy references to the trunk and the interface.
     */
    DECLR0CALLBACKMEMBER(void, pfnNotifyMacAddress,(PINTNETTRUNKIFPORT pIfPort, void *pvIfData, PCRTMAC pMac));

    /**
     * Called when an interface is connected to the network.
     *
     * @returns IPRT status code.
     * @param   pIfPort     Pointer to this structure.
     * @param   pvIf        Opaque pointer to the interface being connected.
     *                      For use INTNETTRUNKSWPORT::pfnRecv.
     * @param   ppvIfData   Pointer  to a pointer variable that the trunk
     *                      implementation can use to associate data with the
     *                      interface.  This pointer will be passed to the
     *                      pfnXmit, pfnNotifyMacAddress and
     *                      pfnDisconnectInterface methods.
     *
     * @remarks Owns the big mutex.  No racing pfnDisconnectAndRelease.
     */
    DECLR0CALLBACKMEMBER(int, pfnConnectInterface,(PINTNETTRUNKIFPORT pIfPort, void *pvIf, void **ppvIfData));

    /**
     * Called when an interface is disconnected from the network.
     *
     * @param   pIfPort     Pointer to this structure.
     * @param   pvIfData    Pointer to the trunk's interface data (see
     *                      pfnConnectInterface).
     *
     * @remarks Owns the big mutex.  No racing pfnDisconnectAndRelease.
     */
    DECLR0CALLBACKMEMBER(void, pfnDisconnectInterface,(PINTNETTRUNKIFPORT pIfPort, void *pvIfData));

    /**
     * Waits for the interface to become idle.
     *
     * This method must be called before disconnecting and releasing the object in
     * order to prevent racing incoming/outgoing frames and device
     * enabling/disabling.
     *
     * @returns IPRT status code (see RTSemEventWait).
     * @param   pIfPort     Pointer to this structure.
     * @param   cMillies    The number of milliseconds to wait. 0 means
     *                      no waiting at all. Use RT_INDEFINITE_WAIT for
     *                      an indefinite wait.
     *
     * @remarks Owns the big mutex.  No racing pfnDisconnectAndRelease.
     */
    DECLR0CALLBACKMEMBER(int, pfnWaitForIdle,(PINTNETTRUNKIFPORT pIfPort, uint32_t cMillies));

    /**
     * Transmit a frame.
     *
     * @return  VBox status code. Error generally means we'll drop the frame.
     * @param   pIfPort     Pointer to this structure.
     * @param   pvIfData    Pointer to the trunk's interface data (see
     *                      pfnConnectInterface).
     * @param   pSG         Pointer to the (scatter /) gather structure for the frame.
     *                      This may or may not be a temporary buffer. If it's temporary
     *                      the transmit operation(s) then it's required to make a copy
     *                      of the frame unless it can be transmitted synchronously.
     * @param   fDst        The destination mask. At least one bit will be set.
     *
     * @remarks No locks.  May be called concurrently on several threads.
     */
    DECLR0CALLBACKMEMBER(int, pfnXmit,(PINTNETTRUNKIFPORT pIfPort, void *pvIfData, PINTNETSG pSG, uint32_t fDst));

    /** Structure version number. (INTNETTRUNKIFPORT_VERSION) */
    uint32_t u32VersionEnd;
} INTNETTRUNKIFPORT;

/** Version number for the INTNETTRUNKIFPORT::u32Version and INTNETTRUNKIFPORT::u32VersionEnd fields. */
#define INTNETTRUNKIFPORT_VERSION   UINT32_C(0xA2CDe001)


/**
 * The component factory interface for create a network
 * interface filter (like VBoxNetFlt).
 */
typedef struct INTNETTRUNKFACTORY
{
    /**
     * Release this factory.
     *
     * SUPR0ComponentQueryFactory (SUPDRVFACTORY::pfnQueryFactoryInterface to be precise)
     * will retain a reference to the factory and the caller has to call this method to
     * release it once the pfnCreateAndConnect call(s) has been done.
     *
     * @param   pIfFactory          Pointer to this structure.
     */
    DECLR0CALLBACKMEMBER(void, pfnRelease,(struct INTNETTRUNKFACTORY *pIfFactory));

    /**
     * Create an instance for the specfied host interface and connects it
     * to the internal network trunk port.
     *
     * The initial interface active state is false (suspended).
     *
     *
     * @returns VBox status code.
     * @retval  VINF_SUCCESS and *ppIfPort set on success.
     * @retval  VERR_INTNET_FLT_IF_NOT_FOUND if the interface was not found.
     * @retval  VERR_INTNET_FLT_IF_BUSY if the interface is already connected.
     * @retval  VERR_INTNET_FLT_IF_FAILED if it failed for some other reason.
     *
     * @param   pIfFactory          Pointer to this structure.
     * @param   pszName             The interface name (OS specific).
     * @param   pSwitchPort         Pointer to the port interface on the switch that
     *                              this interface is being connected to.
     * @param   fFlags              Creation flags, see below.
     * @param   ppIfPort            Where to store the pointer to the interface port
     *                              on success.
     *
     * @remarks Called while owning the network and the out-bound trunk semaphores.
     */
    DECLR0CALLBACKMEMBER(int, pfnCreateAndConnect,(struct INTNETTRUNKFACTORY *pIfFactory, const char *pszName,
                                                   PINTNETTRUNKSWPORT pSwitchPort, uint32_t fFlags,
                                                   PINTNETTRUNKIFPORT *ppIfPort));
} INTNETTRUNKFACTORY;
/** Pointer to the trunk factory. */
typedef INTNETTRUNKFACTORY *PINTNETTRUNKFACTORY;

/** The UUID for the (current) trunk factory. (case sensitive) */
#define INTNETTRUNKFACTORY_UUID_STR     "de504d93-1d1e-4781-8b73-6ea39a0e36a2"

/** @name INTNETTRUNKFACTORY::pfnCreateAndConnect flags.
 * @{ */
/** Don't put the filtered interface in promiscuous mode.
 * This is used for wireless interface since these can misbehave if
 * we try to put them in promiscuous mode. (Wireless interfaces are
 * normally bridged on level 3 instead of level 2.) */
#define INTNETTRUNKFACTORY_FLAG_NO_PROMISC      RT_BIT_32(0)
/** @} */


/**
 * The trunk connection type.
 *
 * Used by IntNetR0Open and associated interfaces.
 */
typedef enum INTNETTRUNKTYPE
{
    /** Invalid trunk type. */
    kIntNetTrunkType_Invalid = 0,
    /** No trunk connection. */
    kIntNetTrunkType_None,
    /** We don't care which kind of trunk connection if the network exists,
     * if it doesn't exist create it without a connection. */
    kIntNetTrunkType_WhateverNone,
    /** VirtualBox host network interface filter driver.
     * The trunk name is the name of the host network interface. */
    kIntNetTrunkType_NetFlt,
    /** VirtualBox adapter host driver. */
    kIntNetTrunkType_NetAdp,
    /** Nat service (ring-0). */
    kIntNetTrunkType_SrvNat,
    /** The end of valid types. */
    kIntNetTrunkType_End,
    /** The usual 32-bit hack. */
    kIntNetTrunkType_32bitHack = 0x7fffffff
} INTNETTRUNKTYPE;

/** @name IntNetR0Open flags.
 * @{ */
/** Share the MAC address with the host when sending something to the wire via the trunk.
 * This is typically used when the trunk is a NetFlt for a wireless interface. */
#define INTNET_OPEN_FLAGS_SHARED_MAC_ON_WIRE                    RT_BIT_32(0)
/** Whether new participants should be subjected to access check or not. */
#define INTNET_OPEN_FLAGS_PUBLIC                                RT_BIT_32(1)
/** Ignore any requests for promiscuous mode. */
#define INTNET_OPEN_FLAGS_IGNORE_PROMISC                        RT_BIT_32(2)
/** Ignore any requests for promiscuous mode, quietly applied/ignored on open. */
#define INTNET_OPEN_FLAGS_QUIETLY_IGNORE_PROMISC                RT_BIT_32(3)
/** Ignore any requests for promiscuous mode on the trunk wire connection. */
#define INTNET_OPEN_FLAGS_IGNORE_PROMISC_TRUNK_WIRE             RT_BIT_32(4)
/** Ignore any requests for promiscuous mode on the trunk wire connection, quietly applied/ignored on open. */
#define INTNET_OPEN_FLAGS_QUIETLY_IGNORE_PROMISC_TRUNK_WIRE     RT_BIT_32(5)
/** Ignore any requests for promiscuous mode on the trunk host connection. */
#define INTNET_OPEN_FLAGS_IGNORE_PROMISC_TRUNK_HOST             RT_BIT_32(6)
/** Ignore any requests for promiscuous mode on the trunk host connection, quietly applied/ignored on open. */
#define INTNET_OPEN_FLAGS_QUIETLY_IGNORE_PROMISC_TRUNK_HOST     RT_BIT_32(7)
/** The mask of flags which causes flag incompatibilities. */
#define INTNET_OPEN_FLAGS_COMPATIBILITY_XOR_MASK                (RT_BIT_32(0) | RT_BIT_32(1) | RT_BIT_32(2) | RT_BIT_32(4) | RT_BIT_32(6))
/** The mask of flags is always ORed in, even on open. (the quiet stuff) */
#define INTNET_OPEN_FLAGS_SECURITY_OR_MASK                      (RT_BIT_32(3) | RT_BIT_32(5) | RT_BIT_32(7))
/** The mask of valid flags. */
#define INTNET_OPEN_FLAGS_MASK                                  UINT32_C(0x000000ff)
/** @} */

/** The maximum length of a network name. */
#define INTNET_MAX_NETWORK_NAME     128

/** The maximum length of a trunk name. */
#define INTNET_MAX_TRUNK_NAME       64


/**
 * Request buffer for IntNetR0OpenReq / VMMR0_DO_INTNET_OPEN.
 * @see IntNetR0Open.
 */
typedef struct INTNETOPENREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** The network name. (input) */
    char            szNetwork[INTNET_MAX_NETWORK_NAME];
    /** What to connect to the trunk port. (input)
     * This is specific to the trunk type below. */
    char            szTrunk[INTNET_MAX_TRUNK_NAME];
    /** The type of trunk link (NAT, Filter, TAP, etc). (input) */
    INTNETTRUNKTYPE enmTrunkType;
    /** Flags, see INTNET_OPEN_FLAGS_*. (input) */
    uint32_t        fFlags;
    /** The size of the send buffer. (input) */
    uint32_t        cbSend;
    /** The size of the receive buffer. (input) */
    uint32_t        cbRecv;
    /** The handle to the network interface. (output) */
    INTNETIFHANDLE  hIf;
} INTNETOPENREQ;
/** Pointer to an IntNetR0OpenReq / VMMR0_DO_INTNET_OPEN request buffer. */
typedef INTNETOPENREQ *PINTNETOPENREQ;

INTNETR0DECL(int) IntNetR0OpenReq(PSUPDRVSESSION pSession, PINTNETOPENREQ pReq);


/**
 * Request buffer for IntNetR0IfCloseReq / VMMR0_DO_INTNET_IF_CLOSE.
 * @see IntNetR0IfClose.
 */
typedef struct INTNETIFCLOSEREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** The handle to the network interface. */
    INTNETIFHANDLE  hIf;
} INTNETIFCLOSEREQ;
/** Pointer to an IntNetR0IfCloseReq / VMMR0_DO_INTNET_IF_CLOSE request
 *  buffer. */
typedef INTNETIFCLOSEREQ *PINTNETIFCLOSEREQ;

INTNETR0DECL(int) IntNetR0IfCloseReq(PSUPDRVSESSION pSession, PINTNETIFCLOSEREQ pReq);


/**
 * Request buffer for IntNetR0IfGetRing3BufferReq /
 * VMMR0_DO_INTNET_IF_GET_BUFFER_PTRS.
 * @see IntNetR0IfGetRing3Buffer.
 */
typedef struct INTNETIFGETBUFFERPTRSREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
    /** The pointer to the ring-3 buffer. (output) */
    R3PTRTYPE(PINTNETBUF)   pRing3Buf;
    /** The pointer to the ring-0 buffer. (output) */
    R0PTRTYPE(PINTNETBUF)   pRing0Buf;
} INTNETIFGETBUFFERPTRSREQ;
/** Pointer to an IntNetR0IfGetRing3BufferReq /
 *  VMMR0_DO_INTNET_IF_GET_BUFFER_PTRS request buffer. */
typedef INTNETIFGETBUFFERPTRSREQ *PINTNETIFGETBUFFERPTRSREQ;

INTNETR0DECL(int) IntNetR0IfGetBufferPtrsReq(PSUPDRVSESSION pSession, PINTNETIFGETBUFFERPTRSREQ pReq);


/**
 * Request buffer for IntNetR0IfSetPromiscuousModeReq /
 * VMMR0_DO_INTNET_IF_SET_PROMISCUOUS_MODE.
 * @see IntNetR0IfSetPromiscuousMode.
 */
typedef struct INTNETIFSETPROMISCUOUSMODEREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
    /** The new promiscuous mode. */
    bool            fPromiscuous;
} INTNETIFSETPROMISCUOUSMODEREQ;
/** Pointer to an IntNetR0IfSetPromiscuousModeReq /
 *  VMMR0_DO_INTNET_IF_SET_PROMISCUOUS_MODE request buffer. */
typedef INTNETIFSETPROMISCUOUSMODEREQ *PINTNETIFSETPROMISCUOUSMODEREQ;

INTNETR0DECL(int) IntNetR0IfSetPromiscuousModeReq(PSUPDRVSESSION pSession, PINTNETIFSETPROMISCUOUSMODEREQ pReq);


/**
 * Request buffer for IntNetR0IfSetMacAddressReq /
 * VMMR0_DO_INTNET_IF_SET_MAC_ADDRESS.
 * @see IntNetR0IfSetMacAddress.
 */
typedef struct INTNETIFSETMACADDRESSREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
    /** The new MAC address. */
    RTMAC           Mac;
} INTNETIFSETMACADDRESSREQ;
/** Pointer to an IntNetR0IfSetMacAddressReq /
 *  VMMR0_DO_INTNET_IF_SET_MAC_ADDRESS request buffer. */
typedef INTNETIFSETMACADDRESSREQ *PINTNETIFSETMACADDRESSREQ;

INTNETR0DECL(int) IntNetR0IfSetMacAddressReq(PSUPDRVSESSION pSession, PINTNETIFSETMACADDRESSREQ pReq);


/**
 * Request buffer for IntNetR0IfSetActiveReq / VMMR0_DO_INTNET_IF_SET_ACTIVE.
 * @see IntNetR0IfSetActive.
 */
typedef struct INTNETIFSETACTIVEREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
    /** The new state. */
    bool            fActive;
} INTNETIFSETACTIVEREQ;
/** Pointer to an IntNetR0IfSetActiveReq / VMMR0_DO_INTNET_IF_SET_ACTIVE
 *  request buffer. */
typedef INTNETIFSETACTIVEREQ *PINTNETIFSETACTIVEREQ;

INTNETR0DECL(int) IntNetR0IfSetActiveReq(PSUPDRVSESSION pSession, PINTNETIFSETACTIVEREQ pReq);


/**
 * Request buffer for IntNetR0IfSendReq / VMMR0_DO_INTNET_IF_SEND.
 * @see IntNetR0IfSend.
 */
typedef struct INTNETIFSENDREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
} INTNETIFSENDREQ;
/** Pointer to an IntNetR0IfSend() argument package. */
typedef INTNETIFSENDREQ *PINTNETIFSENDREQ;

INTNETR0DECL(int) IntNetR0IfSendReq(PSUPDRVSESSION pSession, PINTNETIFSENDREQ pReq);


/**
 * Request buffer for IntNetR0IfWaitReq / VMMR0_DO_INTNET_IF_WAIT.
 * @see IntNetR0IfWait.
 */
typedef struct INTNETIFWAITREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
    /** The number of milliseconds to wait. */
    uint32_t        cMillies;
} INTNETIFWAITREQ;
/** Pointer to an IntNetR0IfWaitReq / VMMR0_DO_INTNET_IF_WAIT request buffer. */
typedef INTNETIFWAITREQ *PINTNETIFWAITREQ;

INTNETR0DECL(int) IntNetR0IfWaitReq(PSUPDRVSESSION pSession, PINTNETIFWAITREQ pReq);


/**
 * Request buffer for IntNetR0IfAbortWaitReq / VMMR0_DO_INTNET_IF_ABORT_WAIT.
 * @see IntNetR0IfAbortWait.
 */
typedef struct INTNETIFABORTWAITREQ
{
    /** The request header. */
    SUPVMMR0REQHDR  Hdr;
    /** Alternative to passing the taking the session from the VM handle.
     * Either use this member or use the VM handle, don't do both. */
    PSUPDRVSESSION  pSession;
    /** Handle to the interface. */
    INTNETIFHANDLE  hIf;
    /** Set this to fend off all future IntNetR0Wait calls.  */
    bool            fNoMoreWaits;
} INTNETIFABORTWAITREQ;
/** Pointer to an IntNetR0IfAbortWaitReq / VMMR0_DO_INTNET_IF_ABORT_WAIT
 *  request buffer. */
typedef INTNETIFABORTWAITREQ *PINTNETIFABORTWAITREQ;

INTNETR0DECL(int) IntNetR0IfAbortWaitReq(PSUPDRVSESSION pSession, PINTNETIFABORTWAITREQ pReq);


#if defined(IN_RING0) || defined(IN_INTNET_TESTCASE)
/** @name
 * @{
 */

INTNETR0DECL(int)      IntNetR0Init(void);
INTNETR0DECL(void)     IntNetR0Term(void);
INTNETR0DECL(int)      IntNetR0Open(PSUPDRVSESSION pSession, const char *pszNetwork,
                                    INTNETTRUNKTYPE enmTrunkType, const char *pszTrunk, uint32_t fFlags,
                                    uint32_t cbSend, uint32_t cbRecv, PINTNETIFHANDLE phIf);
INTNETR0DECL(uint32_t) IntNetR0GetNetworkCount(void);

INTNETR0DECL(int)       IntNetR0IfClose(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession);
INTNETR0DECL(int)       IntNetR0IfGetBufferPtrs(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession,
                                                R3PTRTYPE(PINTNETBUF) *ppRing3Buf, R0PTRTYPE(PINTNETBUF) *ppRing0Buf);
INTNETR0DECL(int)       IntNetR0IfSetPromiscuousMode(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession, bool fPromiscuous);
INTNETR0DECL(int)       IntNetR0IfSetMacAddress(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession, PCRTMAC pMac);
INTNETR0DECL(int)       IntNetR0IfSetActive(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession, bool fActive);
INTNETR0DECL(int)       IntNetR0IfSend(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession);
INTNETR0DECL(int)       IntNetR0IfWait(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession, uint32_t cMillies);
INTNETR0DECL(int)       IntNetR0IfAbortWait(INTNETIFHANDLE hIf, PSUPDRVSESSION pSession);

/** @} */
#endif /* IN_RING0 */

RT_C_DECLS_END

#endif

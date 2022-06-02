/* $Id: FTMInternal.h 32286 2010-09-07 12:37:46Z vboxsync $ */
/** @file
 * FTM - Internal header file.
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___FTMInternal_h
#define ___FTMInternal_h

#include <VBox/cdefs.h>
#include <VBox/types.h>
#include <VBox/ftm.h>
#include <VBox/stam.h>
#include <VBox/pdmcritsect.h>
#include <iprt/avl.h>

/** @defgroup grp_ftm_int Internals.
 * @ingroup grp_ftm
 * @{
 */

/** Physical page tree node. */
typedef struct FTMPHYSPAGETREENODE
{
    AVLGCPHYSNODECORE   Core;
    void               *pPage;
} FTMPHYSPAGETREENODE;
/** Pointer to FTMPHYSPAGETREENODE */
typedef FTMPHYSPAGETREENODE *PFTMPHYSPAGETREENODE;

/**
 * FTM VM Instance data.
 * Changes to this must checked against the padding of the ftm union in VM!
 */
typedef struct FTM
{
    /** Address of the standby VM. */
    R3PTRTYPE(char *)   pszAddress;
    /** Password to access the syncing server of the standby VM. */
    R3PTRTYPE(char *)   pszPassword;
    /** Port of the standby VM. */
    unsigned            uPort;
    /** Syncing interval in ms. */
    unsigned            uInterval;

    /** Set when this VM is the standby FT node. */
    bool                fIsStandbyNode;
    /** Set when this master VM is busy with checkpointing. */
    bool                fCheckpointingActive;
    /** Set when VM save/restore should only include changed pages. */
    bool                fDeltaLoadSaveActive;
    bool                fAlignment[5];

    /** Current active socket. */
    RTSOCKET            hSocket;
#if HC_ARCH_BITS == 32
    RTSOCKET            hSocketAlignment;
#endif

    /** State sync. */
    struct
    {
        unsigned            uOffStream;
        unsigned            cbReadBlock;
        bool                fStopReading;
        bool                fIOError;
        bool                fEndOfStream;
        bool                fAlignment[5];
    } syncstate;

    struct
    {
        R3PTRTYPE(PRTTCPSERVER)    hServer;
        R3PTRTYPE(AVLGCPHYSTREE)   pPhysPageTree;
    } standby;

    struct
    {
        RTSEMEVENT      hShutdownEvent;
    } master;

    /** FTM critical section.
     * This makes sure only the checkpoint or sync is active
     */
    PDMCRITSECT         CritSect;

    STAMCOUNTER         StatReceivedMem;
    STAMCOUNTER         StatReceivedState;
    STAMCOUNTER         StatSentMem;
    STAMCOUNTER         StatSentState;
    STAMCOUNTER         StatDeltaMem;
    STAMCOUNTER         StatDeltaVM;
    STAMCOUNTER         StatFullSync;
    STAMCOUNTER         StatCheckpointNetwork;
    STAMCOUNTER         StatCheckpointStorage;
    STAMPROFILE         StatCheckpoint;
} FTM;
AssertCompileMemberAlignment(FTM, CritSect, 8);

/** @} */

#endif

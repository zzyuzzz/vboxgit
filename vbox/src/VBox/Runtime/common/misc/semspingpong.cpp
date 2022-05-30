/* $Id: semspingpong.cpp 12874 2008-10-01 20:09:09Z vboxsync $ */
/** @file
 * IPRT - Thread Ping-Pong Construct.
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


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/semaphore.h>
#include <iprt/thread.h>
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <iprt/err.h>


/*******************************************************************************
*   Defined Constants And Macros                                               *
*******************************************************************************/
/**
 * Validation macro returns if invalid parameter.
 *
 * Expects a enmSpeaker variable to be handy and will set it to the current
 * enmSpeaker value.
 */
#define RTSEMPP_VALIDATE_RETURN(pPP) \
    do { \
        AssertPtrReturn(pPP, VERR_INVALID_PARAMETER); \
        ASMAtomicUoReadSize(&pPP->enmSpeaker, &enmSpeaker); \
        AssertMsgReturn(    enmSpeaker == RTPINGPONGSPEAKER_PING \
                        ||  enmSpeaker == RTPINGPONGSPEAKER_PONG \
                        ||  enmSpeaker == RTPINGPONGSPEAKER_PONG_SIGNALED \
                        ||  enmSpeaker == RTPINGPONGSPEAKER_PING_SIGNALED, \
                        ("enmSpeaker=%d\n", enmSpeaker), \
                        VERR_INVALID_PARAMETER); \
    } while (0)


/**
 * Init a Ping-Pong construct.
 *
 * @returns iprt status code.
 * @param   pPP         Pointer to the ping-pong structure which needs initialization.
 */
RTR3DECL(int) RTSemPingPongInit(PRTPINGPONG pPP)
{
    /*
     * Init the structure.
     */
    pPP->enmSpeaker = RTPINGPONGSPEAKER_PING;

    int rc = RTSemEventCreate(&pPP->Ping);
    if (RT_SUCCESS(rc))
    {
        rc = RTSemEventCreate(&pPP->Pong);
        if (RT_SUCCESS(rc))
            return VINF_SUCCESS;
        RTSemEventDestroy(pPP->Ping);
    }

    return rc;
}


/**
 * Destroys a Ping-Pong construct.
 *
 * @returns iprt status code.
 * @param   pPP         Pointer to the ping-pong structure which is to be destroyed.
 *                      (I.e. put into uninitialized state.)
 */
RTR3DECL(int) RTSemPingPongDelete(PRTPINGPONG pPP)
{
    /*
     * Validate input
     */
    if (!pPP)
        return VINF_SUCCESS;
    RTPINGPONGSPEAKER enmSpeaker;
    RTSEMPP_VALIDATE_RETURN(pPP);

    /*
     * Invalidate the ping pong handle and destroy the event semaphores.
     */
    ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_UNINITIALIZE);
    int rc = RTSemEventDestroy(pPP->Ping);
    int rc2 = RTSemEventDestroy(pPP->Pong);
    AssertRC(rc);
    AssertRC(rc2);

    return VINF_SUCCESS;
}


/**
 * Signals the pong thread in a ping-pong construct. (I.e. sends ping.)
 * This is called by the ping thread.
 *
 * @returns iprt status code.
 * @param   pPP         Pointer to the ping-pong structure to ping.
 */
RTR3DECL(int) RTSemPing(PRTPINGPONG pPP)
{
    /*
     * Validate input
     */
    RTPINGPONGSPEAKER enmSpeaker;
    RTSEMPP_VALIDATE_RETURN(pPP);
    AssertMsgReturn(enmSpeaker == RTPINGPONGSPEAKER_PING,("Speaking out of turn! enmSpeaker=%d\n", enmSpeaker),
                    VERR_SEM_OUT_OF_TURN);

    /*
     * Signal the other thread.
     */
    ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_PONG_SIGNALED);
    int rc = RTSemEventSignal(pPP->Pong);
    if (RT_SUCCESS(rc))
        return rc;

    /* restore the state. */
    AssertMsgFailed(("Failed to signal pong sem %x. rc=%Rrc\n",  pPP->Pong,  rc));
    ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_PING);
    return rc;
}


/**
 * Signals the ping thread in a ping-pong construct. (I.e. sends pong.)
 * This is called by the pong thread.
 *
 * @returns iprt status code.
 * @param   pPP         Pointer to the ping-pong structure to pong.
 */
RTR3DECL(int) RTSemPong(PRTPINGPONG pPP)
{
    /*
     * Validate input
     */
    RTPINGPONGSPEAKER enmSpeaker;
    RTSEMPP_VALIDATE_RETURN(pPP);
    AssertMsgReturn(enmSpeaker == RTPINGPONGSPEAKER_PONG,("Speaking out of turn! enmSpeaker=%d\n", enmSpeaker),
                    VERR_SEM_OUT_OF_TURN);

    /*
     * Signal the other thread.
     */
    ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_PING_SIGNALED);
    int rc = RTSemEventSignal(pPP->Ping);
    if (RT_SUCCESS(rc))
        return rc;

    /* restore the state. */
    AssertMsgFailed(("Failed to signal ping sem %x. rc=%Rrc\n",  pPP->Ping,  rc));
    ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_PONG);
    return rc;
}


/**
 * Wait function for the ping thread.
 *
 * @returns iprt status code.
 *          Will not return VERR_INTERRUPTED.
 * @param   pPP         Pointer to the ping-pong structure to wait on.
 * @param   cMillies    Number of milliseconds to wait.
 */
RTR3DECL(int) RTSemPingWait(PRTPINGPONG pPP, unsigned cMillies)
{
    /*
     * Validate input
     */
    RTPINGPONGSPEAKER enmSpeaker;
    RTSEMPP_VALIDATE_RETURN(pPP);
    AssertMsgReturn(    enmSpeaker == RTPINGPONGSPEAKER_PONG
                    ||  enmSpeaker == RTPINGPONGSPEAKER_PONG_SIGNALED
                    ||  enmSpeaker == RTPINGPONGSPEAKER_PING_SIGNALED,
                    ("Speaking out of turn! enmSpeaker=%d\n", enmSpeaker),
                    VERR_SEM_OUT_OF_TURN);

    /*
     * Wait.
     */
    int rc = RTSemEventWait(pPP->Ping, cMillies);
    if (RT_SUCCESS(rc))
        ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_PING);
    Assert(rc != VERR_INTERRUPTED);
    return rc;
}


/**
 * Wait function for the pong thread.
 *
 * @returns iprt status code.
 *          Will not return VERR_INTERRUPTED.
 * @param   pPP         Pointer to the ping-pong structure to wait on.
 * @param   cMillies    Number of milliseconds to wait.
 */
RTR3DECL(int) RTSemPongWait(PRTPINGPONG pPP, unsigned cMillies)
{
    /*
     * Validate input
     */
    RTPINGPONGSPEAKER enmSpeaker;
    RTSEMPP_VALIDATE_RETURN(pPP);
    AssertMsgReturn(    enmSpeaker == RTPINGPONGSPEAKER_PING
                    ||  enmSpeaker == RTPINGPONGSPEAKER_PING_SIGNALED
                    ||  enmSpeaker == RTPINGPONGSPEAKER_PONG_SIGNALED,
                    ("Speaking out of turn! enmSpeaker=%d\n", enmSpeaker),
                    VERR_SEM_OUT_OF_TURN);

    /*
     * Wait.
     */
    int rc = RTSemEventWait(pPP->Pong, cMillies);
    if (RT_SUCCESS(rc))
        ASMAtomicWriteSize(&pPP->enmSpeaker, RTPINGPONGSPEAKER_PONG);
    Assert(rc != VERR_INTERRUPTED);
    return rc;
}


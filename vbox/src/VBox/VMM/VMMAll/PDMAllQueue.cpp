/* $Id: PDMAllQueue.cpp 21363 2009-07-07 17:10:52Z vboxsync $ */
/** @file
 * PDM Queue - Transport data and tasks to EMT and R3.
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
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_PDM_QUEUE
#include "PDMInternal.h"
#include <VBox/pdm.h>
#ifndef IN_RC
# include <VBox/rem.h>
# include <VBox/mm.h>
#endif
#include <VBox/vm.h>
#include <VBox/err.h>
#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/assert.h>


/**
 * Allocate an item from a queue.
 * The allocated item must be handed on to PDMR3QueueInsert() after the
 * data have been filled in.
 *
 * @returns Pointer to allocated queue item.
 * @returns NULL on failure. The queue is exhausted.
 * @param   pQueue      The queue handle.
 * @thread  Any thread.
 */
VMMDECL(PPDMQUEUEITEMCORE) PDMQueueAlloc(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue) && pQueue->CTX_SUFF(pVM));
    PPDMQUEUEITEMCORE pNew;
    uint32_t iNext;
    uint32_t i;
    do
    {
        i = pQueue->iFreeTail;
        if (i == pQueue->iFreeHead)
        {
            STAM_REL_COUNTER_INC(&pQueue->StatAllocFailures);
            return NULL;
        }
        pNew = pQueue->aFreeItems[i].CTX_SUFF(pItem);
        iNext = (i + 1) % (pQueue->cItems + PDMQUEUE_FREE_SLACK);
    } while (!ASMAtomicCmpXchgU32(&pQueue->iFreeTail, iNext, i));
    return pNew;
}


/**
 * Queue an item.
 * The item must have been obtained using PDMQueueAlloc(). Once the item
 * have been passed to this function it must not be touched!
 *
 * @param   pQueue      The queue handle.
 * @param   pItem       The item to insert.
 * @thread  Any thread.
 */
VMMDECL(void) PDMQueueInsert(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem)
{
    Assert(VALID_PTR(pQueue) && pQueue->CTX_SUFF(pVM));
    Assert(VALID_PTR(pItem));

    PPDMQUEUEITEMCORE pNext;
    do
    {
        pNext = pQueue->CTX_SUFF(pPending);
        pItem->CTX_SUFF(pNext) = pNext;
    } while (!ASMAtomicCmpXchgPtr((void * volatile *)&pQueue->CTX_SUFF(pPending), pItem, pNext));

    if (!pQueue->pTimer)
    {
        PVM pVM = pQueue->CTX_SUFF(pVM);
        Log2(("PDMQueueInsert: VM_FF_PDM_QUEUES %d -> 1\n", VM_FF_ISSET(pVM, VM_FF_PDM_QUEUES)));
        VM_FF_SET(pVM, VM_FF_PDM_QUEUES);
        ASMAtomicBitSet(&pVM->pdm.s.fQueueFlushing, PDM_QUEUE_FLUSH_FLAG_PENDING_BIT);
#ifdef IN_RING3
        REMR3NotifyQueuePending(pVM); /** @todo r=bird: we can remove REMR3NotifyQueuePending and let VMR3NotifyFF do the work. */
        VMR3NotifyGlobalFFU(pVM->pUVM, VMNOTIFYFF_FLAGS_DONE_REM);
#endif
    }
    STAM_REL_COUNTER_INC(&pQueue->StatInsert);
}


/**
 * Queue an item.
 * The item must have been obtained using PDMQueueAlloc(). Once the item
 * have been passed to this function it must not be touched!
 *
 * @param   pQueue          The queue handle.
 * @param   pItem           The item to insert.
 * @param   NanoMaxDelay    The maximum delay before processing the queue, in nanoseconds.
 *                          This applies only to GC.
 * @thread  Any thread.
 */
VMMDECL(void) PDMQueueInsertEx(PPDMQUEUE pQueue, PPDMQUEUEITEMCORE pItem, uint64_t NanoMaxDelay)
{
    PDMQueueInsert(pQueue, pItem);
#ifdef IN_RC
    PVM pVM = pQueue->CTX_SUFF(pVM);
    /** @todo figure out where to put this, the next bit should go there too.
    if (NanoMaxDelay)
    {

    }
    else */
    {
        VMCPU_FF_SET(VMMGetCpu0(pVM), VMCPU_FF_TO_R3);
        Log2(("PDMQueueInsertEx: Setting VMCPU_FF_TO_R3\n"));
    }
#endif
}



/**
 * Gets the RC pointer for the specified queue.
 *
 * @returns The RC address of the queue.
 * @returns NULL if pQueue is invalid.
 * @param   pQueue          The queue handle.
 */
VMMDECL(RCPTRTYPE(PPDMQUEUE)) PDMQueueRCPtr(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue));
    Assert(pQueue->pVMR3 && pQueue->pVMRC);
#ifdef IN_RC
    return pQueue;
#else
    return MMHyperCCToRC(pQueue->CTX_SUFF(pVM), pQueue);
#endif
}


/**
 * Gets the ring-0 pointer for the specified queue.
 *
 * @returns The ring-0 address of the queue.
 * @returns NULL if pQueue is invalid.
 * @param   pQueue          The queue handle.
 */
VMMDECL(R0PTRTYPE(PPDMQUEUE)) PDMQueueR0Ptr(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue));
    Assert(pQueue->pVMR3 && pQueue->pVMR0);
#ifdef IN_RING0
    return pQueue;
#else
    return MMHyperCCToR0(pQueue->CTX_SUFF(pVM), pQueue);
#endif
}


/**
 * Flushes a PDM queue.
 *
 * @param   pQueue          The queue handle.
 */
VMMDECL(void) PDMQueueFlush(PPDMQUEUE pQueue)
{
    Assert(VALID_PTR(pQueue));
    Assert(pQueue->pVMR3);
    PVM pVM = pQueue->CTX_SUFF(pVM);

#if defined(IN_RC) || defined(IN_RING0)
    Assert(pQueue->CTX_SUFF(pVM));
    pVM->pdm.s.CTX_SUFF(pQueueFlush) = pQueue;
    VMMRZCallRing3NoCpu(pVM, VMMCALLRING3_PDM_QUEUE_FLUSH, (uintptr_t)pQueue);

#else /* IN_RING3: */
    PVMREQ pReq;
    VMR3ReqCall(pVM, VMCPUID_ANY, &pReq, RT_INDEFINITE_WAIT, (PFNRT)PDMR3QueueFlushWorker, 2, pVM, pQueue);
    VMR3ReqFree(pReq);
#endif
}

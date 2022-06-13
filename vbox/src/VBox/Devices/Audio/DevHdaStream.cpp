/* $Id: DevHdaStream.cpp 89861 2021-06-23 14:23:42Z vboxsync $ */
/** @file
 * Intel HD Audio Controller Emulation - Streams.
 */

/*
 * Copyright (C) 2017-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEV_HDA
#include <VBox/log.h>

#include <iprt/mem.h>
#include <iprt/semaphore.h>

#include <VBox/AssertGuest.h>
#include <VBox/vmm/pdmdev.h>
#include <VBox/vmm/pdmaudioifs.h>
#include <VBox/vmm/pdmaudioinline.h>

#include "AudioHlp.h"

#include "DevHda.h"
#include "DevHdaStream.h"

#ifdef VBOX_WITH_DTRACE
# include "dtrace/VBoxDD.h"
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static void     hdaStreamSetPositionAbs(PHDASTREAM pStreamShared, PPDMDEVINS pDevIns, PHDASTATE pThis, uint32_t uLPIB);
#ifdef IN_RING3
# ifdef VBOX_HDA_WITH_ON_REG_ACCESS_DMA
static void     hdaR3StreamFlushDmaBounceBufferOutput(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3);
# endif
static uint32_t hdaR3StreamHandleDmaBufferOverrun(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, PAUDMIXSINK pSink,
                                                  uint32_t cbNeeded, uint64_t nsNow,
                                                  const char *pszCaller, uint32_t const cbStreamFree);
static void     hdaR3StreamUpdateDma(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTATER3 pThisCC,
                                     PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3);
#endif


#ifdef IN_RING3

/**
 * Creates an HDA stream.
 *
 * @returns VBox status code.
 * @param   pStreamShared       The HDA stream to construct - shared bits.
 * @param   pStreamR3           The HDA stream to construct - ring-3 bits.
 * @param   pThis               The shared HDA device instance.
 * @param   pThisCC             The ring-3 HDA device instance.
 * @param   uSD                 Stream descriptor number to assign.
 */
int hdaR3StreamConstruct(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, PHDASTATE pThis, PHDASTATER3 pThisCC, uint8_t uSD)
{
    pStreamR3->u8SD             = uSD;
    pStreamShared->u8SD         = uSD;
    pStreamR3->pMixSink         = NULL;
    pStreamR3->pHDAStateShared  = pThis;
    pStreamR3->pHDAStateR3      = pThisCC;
    Assert(pStreamShared->hTimer != NIL_TMTIMERHANDLE); /* hdaR3Construct initalized this one already. */

    pStreamShared->State.fInReset = false;
    pStreamShared->State.fRunning = false;
# ifdef HDA_USE_DMA_ACCESS_HANDLER
    RTListInit(&pStreamR3->State.lstDMAHandlers);
# endif

    AssertPtr(pStreamR3->pHDAStateR3);
    AssertPtr(pStreamR3->pHDAStateR3->pDevIns);

# ifdef DEBUG
    int rc = RTCritSectInit(&pStreamR3->Dbg.CritSect);
    AssertRCReturn(rc, rc);
# endif

    const bool fIsInput = hdaGetDirFromSD(uSD) == PDMAUDIODIR_IN;

    if (fIsInput)
    {
        pStreamShared->State.Cfg.enmPath = PDMAUDIOPATH_UNKNOWN;
        pStreamShared->State.Cfg.enmDir  = PDMAUDIODIR_IN;
    }
    else
    {
        pStreamShared->State.Cfg.enmPath = PDMAUDIOPATH_UNKNOWN;
        pStreamShared->State.Cfg.enmDir  = PDMAUDIODIR_OUT;
    }

    pStreamR3->Dbg.Runtime.fEnabled = pThisCC->Dbg.fEnabled;

    if (pStreamR3->Dbg.Runtime.fEnabled)
    {
        char szFile[64];
        char szPath[RTPATH_MAX];

        /* pFileStream */
        if (fIsInput)
            RTStrPrintf(szFile, sizeof(szFile), "hdaStreamWriteSD%RU8", uSD);
        else
            RTStrPrintf(szFile, sizeof(szFile), "hdaStreamReadSD%RU8", uSD);

        int rc2 = AudioHlpFileNameGet(szPath, sizeof(szPath), pThisCC->Dbg.pszOutPath, szFile,
                                      0 /* uInst */, AUDIOHLPFILETYPE_WAV, AUDIOHLPFILENAME_FLAGS_NONE);
        AssertRC(rc2);

        rc2 = AudioHlpFileCreate(AUDIOHLPFILETYPE_WAV, szPath, AUDIOHLPFILE_FLAGS_NONE, &pStreamR3->Dbg.Runtime.pFileStream);
        AssertRC(rc2);

        /* pFileDMARaw */
        if (fIsInput)
            RTStrPrintf(szFile, sizeof(szFile), "hdaDMARawWriteSD%RU8", uSD);
        else
            RTStrPrintf(szFile, sizeof(szFile), "hdaDMARawReadSD%RU8", uSD);

        rc2 = AudioHlpFileNameGet(szPath, sizeof(szPath), pThisCC->Dbg.pszOutPath, szFile,
                                  0 /* uInst */, AUDIOHLPFILETYPE_WAV, AUDIOHLPFILENAME_FLAGS_NONE);
        AssertRC(rc2);

        rc2 = AudioHlpFileCreate(AUDIOHLPFILETYPE_WAV, szPath, AUDIOHLPFILE_FLAGS_NONE, &pStreamR3->Dbg.Runtime.pFileDMARaw);
        AssertRC(rc2);

        /* pFileDMAMapped */
        if (fIsInput)
            RTStrPrintf(szFile, sizeof(szFile), "hdaDMAWriteMappedSD%RU8", uSD);
        else
            RTStrPrintf(szFile, sizeof(szFile), "hdaDMAReadMappedSD%RU8", uSD);

        rc2 = AudioHlpFileNameGet(szPath, sizeof(szPath), pThisCC->Dbg.pszOutPath, szFile,
                                  0 /* uInst */, AUDIOHLPFILETYPE_WAV, AUDIOHLPFILENAME_FLAGS_NONE);
        AssertRC(rc2);

        rc2 = AudioHlpFileCreate(AUDIOHLPFILETYPE_WAV, szPath, AUDIOHLPFILE_FLAGS_NONE, &pStreamR3->Dbg.Runtime.pFileDMAMapped);
        AssertRC(rc2);

        /* Delete stale debugging files from a former run. */
        AudioHlpFileDelete(pStreamR3->Dbg.Runtime.pFileStream);
        AudioHlpFileDelete(pStreamR3->Dbg.Runtime.pFileDMARaw);
        AudioHlpFileDelete(pStreamR3->Dbg.Runtime.pFileDMAMapped);
    }

    return VINF_SUCCESS;
}

/**
 * Destroys an HDA stream.
 *
 * @param   pStreamR3           The HDA stream to destroy - ring-3 bits.
 */
void hdaR3StreamDestroy(PHDASTREAMR3 pStreamR3)
{
    LogFlowFunc(("[SD%RU8] Destroying ...\n", pStreamR3->u8SD));
    int rc2;

    if (pStreamR3->State.pAioRegSink)
    {
        rc2 = AudioMixerSinkRemoveUpdateJob(pStreamR3->State.pAioRegSink, hdaR3StreamUpdateAsyncIoJob, pStreamR3);
        AssertRC(rc2);
        pStreamR3->State.pAioRegSink = NULL;
    }

    if (pStreamR3->State.pCircBuf)
    {
        RTCircBufDestroy(pStreamR3->State.pCircBuf);
        pStreamR3->State.pCircBuf = NULL;
        pStreamR3->State.StatDmaBufSize = 0;
        pStreamR3->State.StatDmaBufUsed = 0;
    }

# ifdef DEBUG
    if (RTCritSectIsInitialized(&pStreamR3->Dbg.CritSect))
    {
        rc2 = RTCritSectDelete(&pStreamR3->Dbg.CritSect);
        AssertRC(rc2);
    }
# endif

    if (pStreamR3->Dbg.Runtime.fEnabled)
    {
        AudioHlpFileDestroy(pStreamR3->Dbg.Runtime.pFileStream);
        pStreamR3->Dbg.Runtime.pFileStream = NULL;

        AudioHlpFileDestroy(pStreamR3->Dbg.Runtime.pFileDMARaw);
        pStreamR3->Dbg.Runtime.pFileDMARaw = NULL;

        AudioHlpFileDestroy(pStreamR3->Dbg.Runtime.pFileDMAMapped);
        pStreamR3->Dbg.Runtime.pFileDMAMapped = NULL;
    }

    LogFlowFuncLeave();
}


/**
 * Appends a item to the scheduler.
 *
 * @returns VBox status code.
 * @param   pStreamShared   The stream which scheduler should be modified.
 * @param   cbCur           The period length in guest bytes.
 * @param   cbMaxPeriod     The max period in guest bytes.
 * @param   idxLastBdle     The last BDLE in the period.
 * @param   pHostProps      The host PCM properties.
 * @param   pGuestProps     The guest PCM properties.
 * @param   pcbBorrow       Where to account for bytes borrowed across buffers
 *                          to align scheduling items on frame boundraries.
 */
static int hdaR3StreamAddScheduleItem(PHDASTREAM pStreamShared, uint32_t cbCur, uint32_t cbMaxPeriod, uint32_t idxLastBdle,
                                      PCPDMAUDIOPCMPROPS pHostProps, PCPDMAUDIOPCMPROPS pGuestProps, uint32_t *pcbBorrow)
{
    /* Check that we've got room (shouldn't ever be a problem). */
    size_t idx = pStreamShared->State.cSchedule;
    AssertLogRelReturn(idx + 1 < RT_ELEMENTS(pStreamShared->State.aSchedule), VERR_INTERNAL_ERROR_5);

    /* Figure out the BDLE range for this period. */
    uint32_t const idxFirstBdle = idx == 0 ? 0
                                : RT_MIN((uint32_t)(  pStreamShared->State.aSchedule[idx - 1].idxFirst
                                                    + pStreamShared->State.aSchedule[idx - 1].cEntries),
                                         idxLastBdle);

    pStreamShared->State.aSchedule[idx].idxFirst = (uint8_t)idxFirstBdle;
    pStreamShared->State.aSchedule[idx].cEntries = idxLastBdle >= idxFirstBdle
                                                 ? idxLastBdle - idxFirstBdle + 1
                                                 : pStreamShared->State.cBdles - idxFirstBdle + idxLastBdle + 1;

    /* Deal with borrowing due to unaligned IOC buffers. */
    uint32_t const cbBorrowed = *pcbBorrow;
    if (cbBorrowed < cbCur)
        cbCur -= cbBorrowed;
    else
    {
        /* Note. We can probably gloss over this, but it's not a situation a sane guest would put us, so don't bother for now. */
        ASSERT_GUEST_MSG_FAILED(("#%u: cbBorrow=%#x cbCur=%#x BDLE[%u..%u]\n",
                                 pStreamShared->u8SD, cbBorrowed, cbCur, idxFirstBdle, idxLastBdle));
        LogRelMax(32, ("HDA: Stream #%u has a scheduling error: cbBorrow=%#x cbCur=%#x BDLE[%u..%u]\n",
                       pStreamShared->u8SD, cbBorrowed, cbCur, idxFirstBdle, idxLastBdle));
        return VERR_OUT_OF_RANGE;
    }

    uint32_t cbCurAligned = PDMAudioPropsRoundUpBytesToFrame(pGuestProps, cbCur);
    *pcbBorrow = cbCurAligned - cbCur;

    /* Do we need to split up the period? */
    if (cbCurAligned <= cbMaxPeriod)
    {
        uint32_t cbHost = PDMAudioPropsFramesToBytes(pHostProps, PDMAudioPropsBytesToFrames(pGuestProps, cbCurAligned));
        pStreamShared->State.aSchedule[idx].cbPeriod = cbHost;
        pStreamShared->State.aSchedule[idx].cLoops   = 1;
    }
    else
    {
        /* Reduce till we've below the threshold. */
        uint32_t cbLoop = cbCurAligned;
        do
            cbLoop = cbCurAligned / 2;
        while (cbLoop > cbMaxPeriod);
        cbLoop = PDMAudioPropsRoundUpBytesToFrame(pGuestProps, cbLoop);

        /* Complete the scheduling item. */
        uint32_t cbHost = PDMAudioPropsFramesToBytes(pHostProps, PDMAudioPropsBytesToFrames(pGuestProps, cbLoop));
        pStreamShared->State.aSchedule[idx].cbPeriod = cbHost;
        pStreamShared->State.aSchedule[idx].cLoops   = cbCurAligned / cbLoop;

        /* If there is a remainder, add it as a separate entry (this is
           why the schedule must be more than twice the size of the BDL).*/
        cbCurAligned %= cbLoop;
        if (cbCurAligned)
        {
            pStreamShared->State.aSchedule[idx + 1] = pStreamShared->State.aSchedule[idx];
            idx++;
            cbHost = PDMAudioPropsFramesToBytes(pHostProps, PDMAudioPropsBytesToFrames(pGuestProps, cbCurAligned));
            pStreamShared->State.aSchedule[idx].cbPeriod = cbHost;
            pStreamShared->State.aSchedule[idx].cLoops   = 1;
        }
    }

    /* Done. */
    pStreamShared->State.cSchedule = (uint16_t)(idx + 1);

    return VINF_SUCCESS;
}

/**
 * Creates the DMA timer schedule for the stream
 *
 * This is called from the stream setup code.
 *
 * @returns VBox status code.
 * @param   pStreamShared       The stream to create a schedule for.  The BDL
 *                              must be loaded.
 * @param   cSegments           Number of BDL segments.
 * @param   cBufferIrqs         Number of the BDLEs with IOC=1.
 * @param   cbTotal             The total BDL length in guest bytes.
 * @param   cbMaxPeriod         Max period in guest bytes.   This is in case the
 *                              guest want to play the whole "Der Ring des
 *                              Nibelungen" cycle in one go.
 * @param   cTimerTicksPerSec   The DMA timer frequency.
 * @param   pHostProps          The host PCM properties.
 * @param   pGuestProps         The guest PCM properties.
 */
static int hdaR3StreamCreateSchedule(PHDASTREAM pStreamShared, uint32_t cSegments, uint32_t cBufferIrqs, uint32_t cbTotal,
                                     uint32_t cbMaxPeriod, uint64_t cTimerTicksPerSec,
                                     PCPDMAUDIOPCMPROPS pHostProps, PCPDMAUDIOPCMPROPS pGuestProps)
{
    int rc;

    /*
     * Reset scheduling state.
     */
    RT_ZERO(pStreamShared->State.aSchedule);
    pStreamShared->State.cSchedule         = 0;
    pStreamShared->State.cSchedulePrologue = 0;
    pStreamShared->State.idxSchedule       = 0;
    pStreamShared->State.idxScheduleLoop   = 0;

    /*
     * Do the basic schedule compilation.
     */
    uint32_t cPotentialPrologue = 0;
    uint32_t cbBorrow           = 0;
    uint32_t cbCur              = 0;
    uint32_t cbMin              = UINT32_MAX;
    pStreamShared->State.aSchedule[0].idxFirst = 0;
    for (uint32_t i = 0; i < cSegments; i++)
    {
        cbCur += pStreamShared->State.aBdl[i].cb;
        if (pStreamShared->State.aBdl[i].cb < cbMin)
            cbMin = pStreamShared->State.aBdl[i].cb;
        if (pStreamShared->State.aBdl[i].fFlags & HDA_BDLE_F_IOC)
        {
            rc = hdaR3StreamAddScheduleItem(pStreamShared, cbCur, cbMaxPeriod, i, pHostProps, pGuestProps, &cbBorrow);
            ASSERT_GUEST_RC_RETURN(rc, rc);

            if (cPotentialPrologue == 0)
                cPotentialPrologue = pStreamShared->State.cSchedule;
            cbCur = 0;
        }
    }
    AssertLogRelMsgReturn(cbBorrow == 0, ("HDA: Internal scheduling error on stream #%u: cbBorrow=%#x cbTotal=%#x cbCur=%#x\n",
                                          pStreamShared->u8SD, cbBorrow, cbTotal, cbCur),
                          VERR_INTERNAL_ERROR_3);

    /*
     * Deal with any loose ends.
     */
    if (cbCur && cBufferIrqs == 0)
    {
        /*
         * No IOC. Vista ends up here, typically with three buffers configured.
         *
         * The perferred option here is to aim at processing one average BDLE with
         * each DMA timer period, since that best matches how we update LPIB at
         * present.
         *
         * The second alternative is to divide the whole span up into 3-4 periods
         * to try increase our chances of keeping ahead of the guest.  We may need
         * to pick this if there are too few buffer descriptor or they are too small.
         *
         * However, what we probably should be doing is to do real DMA work whenever
         * the guest reads a DMA related register (like LPIB) and just do 3-4 DMA
         * timer periods, however we'll be postponing the DMA timer every time we
         * return to ring-3 and signal the AIO, so in the end we'd probably not use
         * the timer callback at all.  (This is assuming a small shared per-stream
         * buffer for keeping the DMA data in and that it's size will force a return
         * to ring-3 often enough to keep the AIO thread going at a reasonable rate.)
         */
        Assert(cbCur == cbTotal);

        /* Match the BDLEs 1:1 if there are 3 or more and that the smallest one
           is at least 5ms big. */
        if (cSegments >= 3 && PDMAudioPropsBytesToMilli(pGuestProps, cbMin) >= 5 /*ms*/)
        {
            for (uint32_t i = 0; i < cSegments; i++)
            {
                rc = hdaR3StreamAddScheduleItem(pStreamShared, pStreamShared->State.aBdl[i].cb, cbMaxPeriod,
                                                i, pHostProps, pGuestProps, &cbBorrow);
                ASSERT_GUEST_RC_RETURN(rc, rc);
            }
        }
        /* Otherwise, just divide the work into 3 or 4 portions and hope for the best.
           It seems, though, that this only really work for windows vista if we avoid
           working accross buffer lines.  */
        /** @todo This can be simplified/relaxed/uncluttered if we do DMA work when LPIB
         * is read, assuming ofc that LPIB is read before each buffer update. */
        else
        {
            uint32_t const cPeriods = cSegments != 3 && PDMAudioPropsBytesToMilli(pGuestProps, cbCur) >= 4 * 5 /*ms*/
                                    ? 4 : cSegments != 2 ? 3 : 2;
            uint32_t const cbPeriod = PDMAudioPropsFloorBytesToFrame(pGuestProps, cbCur / cPeriods);
            uint32_t       iBdle    = 0;
            uint32_t       offBdle  = 0;
            for (uint32_t iPeriod = 0; iPeriod < cPeriods; iPeriod++)
            {
                if (iPeriod + 1 < cPeriods)
                {
                    offBdle += cbPeriod;
                    while (iBdle < cSegments && offBdle >= pStreamShared->State.aBdl[iBdle].cb)
                        offBdle -= pStreamShared->State.aBdl[iBdle++].cb;
                    rc = hdaR3StreamAddScheduleItem(pStreamShared, cbPeriod, cbMaxPeriod, offBdle != 0 ? iBdle : iBdle - 1,
                                                    pHostProps, pGuestProps, &cbBorrow);
                }
                else
                    rc = hdaR3StreamAddScheduleItem(pStreamShared, cbCur - iPeriod * cbPeriod, cbMaxPeriod, cSegments - 1,
                                                    pHostProps, pGuestProps, &cbBorrow);
                ASSERT_GUEST_RC_RETURN(rc, rc);
            }

        }
        Assert(cbBorrow == 0);
    }
    else if (cbCur)
    {
        /* The last BDLE didn't have IOC set, so we must continue processing
           from the start till we hit one that has.  */
        uint32_t i;
        for (i = 0; i < cSegments; i++)
        {
            cbCur += pStreamShared->State.aBdl[i].cb;
            if (pStreamShared->State.aBdl[i].fFlags & HDA_BDLE_F_IOC)
                break;
        }
        rc = hdaR3StreamAddScheduleItem(pStreamShared, cbCur, cbMaxPeriod, i, pHostProps, pGuestProps, &cbBorrow);
        ASSERT_GUEST_RC_RETURN(rc, rc);

        /* The initial scheduling items covering the wrap around area are
           considered a prologue and must not repeated later. */
        Assert(cPotentialPrologue);
        pStreamShared->State.cSchedulePrologue = (uint8_t)cPotentialPrologue;
    }

    /*
     * If there is just one BDLE with IOC set, we have to make sure
     * we've got at least two periods scheduled, otherwise there is
     * a very good chance the guest will overwrite the start of the
     * buffer before we ever get around to reading it.
     */
    if (cBufferIrqs == 1)
    {
        uint32_t i = pStreamShared->State.cSchedulePrologue;
        Assert(i < pStreamShared->State.cSchedule);
        if (   i + 1 == pStreamShared->State.cSchedule
            && pStreamShared->State.aSchedule[i].cLoops == 1)
        {
            uint32_t const cbFirstHalf = PDMAudioPropsFloorBytesToFrame(pHostProps, pStreamShared->State.aSchedule[i].cbPeriod / 2);
            uint32_t const cbOtherHalf = pStreamShared->State.aSchedule[i].cbPeriod - cbFirstHalf;
            pStreamShared->State.aSchedule[i].cbPeriod = cbFirstHalf;
            if (cbFirstHalf == cbOtherHalf)
                pStreamShared->State.aSchedule[i].cLoops = 2;
            else
            {
                pStreamShared->State.aSchedule[i + 1] = pStreamShared->State.aSchedule[i];
                pStreamShared->State.aSchedule[i].cbPeriod = cbOtherHalf;
                pStreamShared->State.cSchedule++;
            }
        }
    }

    /*
     * Go over the schduling entries and calculate the timer ticks for each period.
     */
    LogRel2(("HDA: Stream #%u schedule: %u items, %u prologue\n",
             pStreamShared->u8SD, pStreamShared->State.cSchedule, pStreamShared->State.cSchedulePrologue));
    uint64_t const cbHostPerSec = PDMAudioPropsFramesToBytes(pHostProps, pHostProps->uHz);
    for (uint32_t i = 0; i < pStreamShared->State.cSchedule; i++)
    {
        uint64_t const cTicks = ASMMultU64ByU32DivByU32(cTimerTicksPerSec, pStreamShared->State.aSchedule[i].cbPeriod,
                                                        cbHostPerSec);
        AssertLogRelMsgReturn((uint32_t)cTicks == cTicks, ("cTicks=%RU64 (%#RX64)\n", cTicks, cTicks), VERR_INTERNAL_ERROR_4);
        pStreamShared->State.aSchedule[i].cPeriodTicks = RT_MAX((uint32_t)cTicks, 16);
        LogRel2(("HDA:  #%u: %u ticks / %u bytes, %u loops, BDLE%u L %u\n", i, pStreamShared->State.aSchedule[i].cPeriodTicks,
                 pStreamShared->State.aSchedule[i].cbPeriod, pStreamShared->State.aSchedule[i].cLoops,
                 pStreamShared->State.aSchedule[i].idxFirst, pStreamShared->State.aSchedule[i].cEntries));
    }

    return VINF_SUCCESS;
}


/**
 * Sets up ((re-)iniitalizes) an HDA stream.
 *
 * @returns VBox status code. VINF_NO_CHANGE if the stream does not need
 *          be set-up again because the stream's (hardware) parameters did
 *          not change.
 * @param   pDevIns         The device instance.
 * @param   pThis           The shared HDA device state (for HW register
 *                          parameters).
 * @param   pStreamShared   HDA stream to set up, shared portion.
 * @param   pStreamR3       HDA stream to set up, ring-3 portion.
 * @param   uSD             Stream descriptor number to assign it.
 */
int hdaR3StreamSetUp(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, uint8_t uSD)
{
    /* This must be valid all times. */
    AssertReturn(uSD < HDA_MAX_STREAMS, VERR_INVALID_PARAMETER);

    /* These member can only change on data corruption, despite what the code does further down (bird).  */
    AssertReturn(pStreamShared->u8SD == uSD, VERR_WRONG_ORDER);
    AssertReturn(pStreamR3->u8SD     == uSD, VERR_WRONG_ORDER);

    const uint64_t u64BDLBase = RT_MAKE_U64(HDA_STREAM_REG(pThis, BDPL, uSD),
                                            HDA_STREAM_REG(pThis, BDPU, uSD));
    const uint16_t u16LVI     = HDA_STREAM_REG(pThis, LVI, uSD);
    const uint32_t u32CBL     = HDA_STREAM_REG(pThis, CBL, uSD);
    const uint8_t  u8FIFOS    = HDA_STREAM_REG(pThis, FIFOS, uSD) + 1;
    uint8_t        u8FIFOW    = hdaSDFIFOWToBytes(HDA_STREAM_REG(pThis, FIFOW, uSD));
    const uint16_t u16FMT     = HDA_STREAM_REG(pThis, FMT, uSD);

    /* Is the bare minimum set of registers configured for the stream?
     * If not, bail out early, as there's nothing to do here for us (yet). */
    if (   !u64BDLBase
        || !u16LVI
        || !u32CBL
        || !u8FIFOS
        || !u8FIFOW
        || !u16FMT)
    {
        LogFunc(("[SD%RU8] Registers not set up yet, skipping (re-)initialization\n", uSD));
        return VINF_SUCCESS;
    }

    /*
     * Convert the config to PDM PCM properties and configure the stream.
     */
    PPDMAUDIOSTREAMCFG pCfg = &pStreamShared->State.Cfg;
    int rc = hdaR3SDFMTToPCMProps(u16FMT, &pCfg->Props);
    if (RT_SUCCESS(rc))
        pCfg->enmDir = hdaGetDirFromSD(uSD);
    else
    {
        LogRelMax(32, ("HDA: Warning: Format 0x%x for stream #%RU8 not supported\n", HDA_STREAM_REG(pThis, FMT, uSD), uSD));
        return rc;
    }

    ASSERT_GUEST_LOGREL_MSG_RETURN(   PDMAudioPropsFrameSize(&pCfg->Props) > 0
                                   && u32CBL % PDMAudioPropsFrameSize(&pCfg->Props) == 0,
                                   ("CBL for stream #%RU8 does not align to frame size (u32CBL=%u cbFrameSize=%u)\n",
                                    uSD, u32CBL, PDMAudioPropsFrameSize(&pCfg->Props)),
                                   VERR_INVALID_PARAMETER);

    /* Make sure the guest behaves regarding the stream's FIFO. */
    ASSERT_GUEST_LOGREL_MSG_STMT(u8FIFOW <= u8FIFOS,
                                 ("Guest tried setting a bigger FIFOW (%RU8) than FIFOS (%RU8), limiting\n", u8FIFOW, u8FIFOS),
                                 u8FIFOW = u8FIFOS /* ASSUMES that u8FIFOS has been validated. */);

    pStreamShared->u8SD       = uSD;

    /* Update all register copies so that we later know that something has changed. */
    pStreamShared->u64BDLBase = u64BDLBase;
    pStreamShared->u16LVI     = u16LVI;
    pStreamShared->u32CBL     = u32CBL;
    pStreamShared->u8FIFOS    = u8FIFOS;
    pStreamShared->u8FIFOW    = u8FIFOW;
    pStreamShared->u16FMT     = u16FMT;

    /* The the stream's name, based on the direction. */
    switch (pCfg->enmDir)
    {
        case PDMAUDIODIR_IN:
# ifdef VBOX_WITH_AUDIO_HDA_MIC_IN
#  error "Implement me!"
# else
            pCfg->enmPath = PDMAUDIOPATH_IN_LINE;
            RTStrCopy(pCfg->szName, sizeof(pCfg->szName), "Line In");
# endif
            break;

        case PDMAUDIODIR_OUT:
            /* Destination(s) will be set in hdaR3AddStreamOut(),
             * based on the channels / stream layout. */
            break;

        default:
            AssertFailedReturn(VERR_NOT_SUPPORTED);
            break;
    }

    LogRel2(("HDA: Stream #%RU8 DMA @ 0x%x (%RU32 bytes = %RU64ms total)\n", uSD, pStreamShared->u64BDLBase,
             pStreamShared->u32CBL, PDMAudioPropsBytesToMilli(&pCfg->Props, pStreamShared->u32CBL)));

    /*
     * Load the buffer descriptor list.
     *
     * Section 3.6.2 states that "the BDL should not be modified unless the RUN
     * bit is 0", so it should be within the specs to read it once here and not
     * re-read any BDLEs later.
     */
    /* Reset BDL state. */
    RT_ZERO(pStreamShared->State.aBdl);
    pStreamShared->State.offCurBdle = 0;
    pStreamShared->State.idxCurBdle = 0;

    uint32_t /*const*/ cTransferFragments = (pStreamShared->u16LVI & 0xff) + 1;
    if (cTransferFragments <= 1)
        LogRel(("HDA: Warning: Stream #%RU8 transfer buffer count invalid: (%RU16)! Buggy guest audio driver!\n", uSD, pStreamShared->u16LVI));
    AssertLogRelReturn(cTransferFragments <= RT_ELEMENTS(pStreamShared->State.aBdl), VERR_INTERNAL_ERROR_5);
    pStreamShared->State.cBdles = cTransferFragments;

    /* Load them. */
    rc = PDMDevHlpPCIPhysRead(pDevIns, u64BDLBase, pStreamShared->State.aBdl,
                              sizeof(pStreamShared->State.aBdl[0]) * cTransferFragments);
    AssertRC(rc);

    /* Check what we just loaded.  Refuse overly large buffer lists. */
    uint64_t cbTotal     = 0;
    uint32_t cBufferIrqs = 0;
    for (uint32_t i = 0; i < cTransferFragments; i++)
    {
        if (pStreamShared->State.aBdl[i].fFlags & HDA_BDLE_F_IOC)
            cBufferIrqs++;
        cbTotal += pStreamShared->State.aBdl[i].cb;
    }
    ASSERT_GUEST_STMT_RETURN(cbTotal < _2G,
                             LogRelMax(32, ("HDA: Error: Stream #%u is configured with an insane amount of buffer space - refusing do work with it: %RU64 (%#RX64) bytes.\n",
                                            uSD, cbTotal, cbTotal)),
                             VERR_NOT_SUPPORTED);
    ASSERT_GUEST_STMT_RETURN(cbTotal == u32CBL,
                             LogRelMax(32, ("HDA: Warning: Stream #%u has a mismatch between CBL and configured buffer space: %RU32 (%#RX32) vs %RU64 (%#RX64)\n",
                                            uSD, u32CBL, u32CBL, cbTotal, cbTotal)),
                             VERR_NOT_SUPPORTED);

    /*
     * Create a DMA timer schedule.
     */
    /** @todo clean up this, pGuestProps and pHostProps are the same now. */
    rc = hdaR3StreamCreateSchedule(pStreamShared, cTransferFragments, cBufferIrqs, (uint32_t)cbTotal,
                                   PDMAudioPropsMilliToBytes(&pCfg->Props, 100 /** @todo make configurable */),
                                   PDMDevHlpTimerGetFreq(pDevIns, pStreamShared->hTimer),
                                   &pCfg->Props, &pCfg->Props);
    if (RT_FAILURE(rc))
        return rc;

    pStreamShared->State.cbTransferSize = pStreamShared->State.aSchedule[0].cbPeriod;

    /*
     * Calculate the transfer Hz for use in the circular buffer calculation.
     */
    uint32_t cbMaxPeriod = 0;
    uint32_t cbMinPeriod = UINT32_MAX;
    uint32_t cPeriods    = 0;
    for (uint32_t i = 0; i < pStreamShared->State.cSchedule; i++)
    {
        uint32_t cbPeriod = pStreamShared->State.aSchedule[i].cbPeriod;
        cbMaxPeriod = RT_MAX(cbMaxPeriod, cbPeriod);
        cbMinPeriod = RT_MIN(cbMinPeriod, cbPeriod);
        cPeriods   += pStreamShared->State.aSchedule[i].cLoops;
    }
    uint64_t const cbTransferPerSec  = RT_MAX(PDMAudioPropsFramesToBytes(&pCfg->Props, pCfg->Props.uHz),
                                              4096 /* zero div prevention: min is 6kHz, picked 4k in case I'm mistaken */);
    unsigned uTransferHz = cbTransferPerSec * 1000 / cbMaxPeriod;
    LogRel2(("HDA: Stream #%RU8 needs a %u.%03u Hz timer rate (period: %u..%u host bytes)\n",
             uSD, uTransferHz / 1000, uTransferHz % 1000, cbMinPeriod, cbMaxPeriod));
    uTransferHz /= 1000;

    if (uTransferHz > 400) /* Anything above 400 Hz looks fishy -- tell the user. */
        LogRelMax(32, ("HDA: Warning: Calculated transfer Hz rate for stream #%RU8 looks incorrect (%u), please re-run with audio debug mode and report a bug\n",
                       uSD, uTransferHz));

    pStreamShared->State.cbAvgTransfer = (uint32_t)(cbTotal + cPeriods - 1) / cPeriods;

    /* For input streams we must determin a pre-buffering requirement.
       We use the initial delay as a basis here, though we must have at
       least two max periods worth of data queued up due to the way we
       work the AIO thread. */
    pStreamShared->State.fInputPreBuffered = false;
    pStreamShared->State.cbInputPreBuffer  = PDMAudioPropsMilliToBytes(&pCfg->Props, pThis->msInitialDelay);
    pStreamShared->State.cbInputPreBuffer  = RT_MIN(cbMaxPeriod * 2, pStreamShared->State.cbInputPreBuffer);

    /*
     * Set up data transfer stuff.
     */

    /* Assign the global device rate to the stream I/O timer as default. */
    pStreamShared->State.uTimerIoHz = pThis->uTimerHz;
    ASSERT_GUEST_LOGREL_MSG_STMT(pStreamShared->State.uTimerIoHz,
                                 ("I/O timer Hz rate for stream #%RU8 is invalid\n", uSD),
                                 pStreamShared->State.uTimerIoHz = HDA_TIMER_HZ_DEFAULT);

    /* Set I/O scheduling hint for the backends. */
    /** @todo r=bird: derive this from the schedule instead of using the
     *        uTimerIoHz, as that's almost pure non-sense now. */
    pCfg->Device.cMsSchedulingHint = RT_MS_1SEC / pStreamShared->State.uTimerIoHz;
    LogRel2(("HDA: Stream #%RU8 set scheduling hint for the backends to %RU32ms\n", uSD, pCfg->Device.cMsSchedulingHint));


    /* Make sure to also update the stream's DMA counter (based on its current LPIB value). */
    /** @todo r=bird: We use LPIB as-is here, so if it's not zero we have to
     *        locate the right place in the schedule and whatnot... */
    if (HDA_STREAM_REG(pThis, LPIB, uSD) != 0)
        LogRel2(("HDA: Warning! Stream #%RU8 is set up with LPIB=%#RX32 instead of zero!\n", uSD, HDA_STREAM_REG(pThis, LPIB, uSD)));
    hdaStreamSetPositionAbs(pStreamShared, pDevIns, pThis, HDA_STREAM_REG(pThis, LPIB, uSD));

# ifdef LOG_ENABLED
    hdaR3BDLEDumpAll(pDevIns, pThis, pStreamShared->u64BDLBase, pStreamShared->u16LVI + 1);
# endif

    /*
     * Set up internal ring buffer.
     */

    /* (Re-)Allocate the stream's internal DMA buffer,
     * based on the timing *and* PCM properties we just got above. */
    if (pStreamR3->State.pCircBuf)
    {
        RTCircBufDestroy(pStreamR3->State.pCircBuf);
        pStreamR3->State.pCircBuf = NULL;
        pStreamR3->State.StatDmaBufSize = 0;
        pStreamR3->State.StatDmaBufUsed = 0;
    }
    pStreamShared->State.offWrite = 0;
    pStreamShared->State.offRead  = 0;

    /*
     * The default internal ring buffer size must be:
     *
     *      - Large enough for at least three periodic DMA transfers.
     *
     *        It is critically important that we don't experience underruns
     *        in the DMA OUT code, because it will cause the buffer processing
     *        to get skewed and possibly overlap with what the guest is updating.
     *        At the time of writing (2021-03-05) there is no code for getting
     *        back into sync there.
     *
     *      - Large enough for at least three I/O scheduling hints.
     *
     *        We want to lag behind a DMA period or two, but there must be
     *        sufficent space for the AIO thread to get schedule and shuffle
     *        data thru the mixer and onto the host audio hardware.
     *
     *      - Both above with plenty to spare.
     *
     * So, just take the longest of the two periods and multipling it by 6.
     * We aren't not talking about very large base buffers heres, so size isn't
     * an issue.
     *
     * Note: Use pCfg->Props as PCM properties here, as we only want to store the
     *       samples we actually need, in other words, skipping the interleaved
     *       channels we don't support / need to save space.
     */
    uint32_t msCircBuf = RT_MS_1SEC * 6 / RT_MIN(uTransferHz, pStreamShared->State.uTimerIoHz);
    msCircBuf = RT_MAX(msCircBuf, pThis->msInitialDelay + RT_MS_1SEC * 6 / uTransferHz);

    uint32_t cbCircBuf = PDMAudioPropsMilliToBytes(&pCfg->Props, msCircBuf);
    LogRel2(("HDA: Stream #%RU8 default ring buffer size is %RU32 bytes / %RU64 ms\n",
             uSD, cbCircBuf, PDMAudioPropsBytesToMilli(&pCfg->Props, cbCircBuf)));

    uint32_t msCircBufCfg = hdaGetDirFromSD(uSD) == PDMAUDIODIR_IN ? pThis->cMsCircBufIn : pThis->cMsCircBufOut;
    if (msCircBufCfg) /* Anything set via CFGM? */
    {
        cbCircBuf = PDMAudioPropsMilliToBytes(&pCfg->Props, msCircBufCfg);
        LogRel2(("HDA: Stream #%RU8 is using a custom ring buffer size of %RU32 bytes / %RU64 ms\n",
                 uSD, cbCircBuf, PDMAudioPropsBytesToMilli(&pCfg->Props, cbCircBuf)));
    }

    /* Serious paranoia: */
    ASSERT_GUEST_LOGREL_MSG_STMT(cbCircBuf % PDMAudioPropsFrameSize(&pCfg->Props) == 0,
                                 ("Ring buffer size (%RU32) for stream #%RU8 not aligned to the (host) frame size (%RU8)\n",
                                  cbCircBuf, uSD, PDMAudioPropsFrameSize(&pCfg->Props)),
                                 rc = VERR_INVALID_PARAMETER);
    ASSERT_GUEST_LOGREL_MSG_STMT(cbCircBuf, ("Ring buffer size for stream #%RU8 is invalid\n", uSD),
                                 rc = VERR_INVALID_PARAMETER);
    if (RT_SUCCESS(rc))
    {
        rc = RTCircBufCreate(&pStreamR3->State.pCircBuf, cbCircBuf);
        if (RT_SUCCESS(rc))
        {
            pStreamR3->State.StatDmaBufSize = cbCircBuf;

            /*
             * Forward the timer frequency hint to TM as well for better accuracy on
             * systems w/o preemption timers (also good for 'info timers').
             */
            PDMDevHlpTimerSetFrequencyHint(pDevIns, pStreamShared->hTimer, uTransferHz);
        }
    }

    if (RT_FAILURE(rc))
        LogRelMax(32, ("HDA: Initializing stream #%RU8 failed with %Rrc\n", uSD, rc));

# ifdef VBOX_WITH_DTRACE
    VBOXDD_HDA_STREAM_SETUP((uint32_t)uSD, rc, pStreamShared->State.Cfg.Props.uHz,
                            pStreamShared->State.aSchedule[pStreamShared->State.cSchedule - 1].cPeriodTicks,
                            pStreamShared->State.aSchedule[pStreamShared->State.cSchedule - 1].cbPeriod);
# endif
    return rc;
}

/**
 * Resets an HDA stream.
 *
 * @param   pThis               The shared HDA device state.
 * @param   pThisCC             The ring-3 HDA device state.
 * @param   pStreamShared       HDA stream to reset (shared).
 * @param   pStreamR3           HDA stream to reset (ring-3).
 * @param   uSD                 Stream descriptor (SD) number to use for this stream.
 */
void hdaR3StreamReset(PHDASTATE pThis, PHDASTATER3 pThisCC, PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, uint8_t uSD)
{
    LogFunc(("[SD%RU8] Reset\n", uSD));

    /*
     * Assert some sanity.
     */
    AssertPtr(pThis);
    AssertPtr(pStreamShared);
    AssertPtr(pStreamR3);
    Assert(uSD < HDA_MAX_STREAMS);
    Assert(pStreamShared->u8SD == uSD);
    Assert(pStreamR3->u8SD == uSD);
    AssertMsg(!pStreamShared->State.fRunning, ("[SD%RU8] Cannot reset stream while in running state\n", uSD));

    /*
     * Set reset state.
     */
    Assert(ASMAtomicReadBool(&pStreamShared->State.fInReset) == false); /* No nested calls. */
    ASMAtomicXchgBool(&pStreamShared->State.fInReset, true);

    /*
     * Second, initialize the registers.
     */
    /* See 6.2.33: Clear on reset. */
    HDA_STREAM_REG(pThis, STS,   uSD) = 0;
    /* According to the ICH6 datasheet, 0x40000 is the default value for stream descriptor register 23:20
     * bits are reserved for stream number 18.2.33, resets SDnCTL except SRST bit. */
    HDA_STREAM_REG(pThis, CTL,   uSD) = HDA_SDCTL_TP | (HDA_STREAM_REG(pThis, CTL, uSD) & HDA_SDCTL_SRST);
    /* ICH6 defines default values (120 bytes for input and 192 bytes for output descriptors) of FIFO size. 18.2.39. */
    HDA_STREAM_REG(pThis, FIFOS, uSD) = hdaGetDirFromSD(uSD) == PDMAUDIODIR_IN ? HDA_SDIFIFO_120B : HDA_SDOFIFO_192B;
    /* See 18.2.38: Always defaults to 0x4 (32 bytes). */
    HDA_STREAM_REG(pThis, FIFOW, uSD) = HDA_SDFIFOW_32B;
    HDA_STREAM_REG(pThis, LPIB,  uSD) = 0;
    HDA_STREAM_REG(pThis, CBL,   uSD) = 0;
    HDA_STREAM_REG(pThis, LVI,   uSD) = 0;
    HDA_STREAM_REG(pThis, FMT,   uSD) = 0;
    HDA_STREAM_REG(pThis, BDPU,  uSD) = 0;
    HDA_STREAM_REG(pThis, BDPL,  uSD) = 0;

# ifdef HDA_USE_DMA_ACCESS_HANDLER
    hdaR3StreamUnregisterDMAHandlers(pThis, pStream);
# endif

    /* Assign the default mixer sink to the stream. */
    pStreamR3->pMixSink = hdaR3GetDefaultSink(pThisCC, uSD);
    if (pStreamR3->State.pAioRegSink)
    {
        int rc2 = AudioMixerSinkRemoveUpdateJob(pStreamR3->State.pAioRegSink, hdaR3StreamUpdateAsyncIoJob, pStreamR3);
        AssertRC(rc2);
        pStreamR3->State.pAioRegSink = NULL;
    }

    /* Reset transfer stuff. */
    pStreamShared->State.cTransferPendingInterrupts = 0;
    pStreamShared->State.tsTransferLast = 0;
    pStreamShared->State.tsTransferNext = 0;

    /* Initialize timestamps. */
    pStreamShared->State.tsLastTransferNs = 0;
    pStreamShared->State.tsLastReadNs     = 0;
    pStreamShared->State.tsAioDelayEnd    = UINT64_MAX;
    pStreamShared->State.tsStart          = 0;

    RT_ZERO(pStreamShared->State.aBdl);
    RT_ZERO(pStreamShared->State.aSchedule);
    pStreamShared->State.offCurBdle        = 0;
    pStreamShared->State.cBdles            = 0;
    pStreamShared->State.idxCurBdle        = 0;
    pStreamShared->State.cSchedulePrologue = 0;
    pStreamShared->State.cSchedule         = 0;
    pStreamShared->State.idxSchedule       = 0;
    pStreamShared->State.idxScheduleLoop   = 0;
    pStreamShared->State.fInputPreBuffered = false;

    if (pStreamR3->State.pCircBuf)
        RTCircBufReset(pStreamR3->State.pCircBuf);
    pStreamShared->State.offWrite = 0;
    pStreamShared->State.offRead  = 0;

# ifdef DEBUG
    pStreamR3->Dbg.cReadsTotal      = 0;
    pStreamR3->Dbg.cbReadTotal      = 0;
    pStreamR3->Dbg.tsLastReadNs     = 0;
    pStreamR3->Dbg.cWritesTotal     = 0;
    pStreamR3->Dbg.cbWrittenTotal   = 0;
    pStreamR3->Dbg.cWritesHz        = 0;
    pStreamR3->Dbg.cbWrittenHz      = 0;
    pStreamR3->Dbg.tsWriteSlotBegin = 0;
# endif

    /* Report that we're done resetting this stream. */
    HDA_STREAM_REG(pThis, CTL,   uSD) = 0;

# ifdef VBOX_WITH_DTRACE
    VBOXDD_HDA_STREAM_RESET((uint32_t)uSD);
# endif
    LogFunc(("[SD%RU8] Reset\n", uSD));

    /* Exit reset mode. */
    ASMAtomicXchgBool(&pStreamShared->State.fInReset, false);
}

/**
 * Enables or disables an HDA audio stream.
 *
 * @returns VBox status code.
 * @param   pThis               The shared HDA device state.
 * @param   pStreamShared       HDA stream to enable or disable - shared bits.
 * @param   pStreamR3           HDA stream to enable or disable - ring-3 bits.
 * @param   fEnable             Whether to enable or disble the stream.
 */
int hdaR3StreamEnable(PHDASTATE pThis, PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, bool fEnable)
{
    AssertPtr(pStreamR3);
    AssertPtr(pStreamShared);

    LogFunc(("[SD%RU8] fEnable=%RTbool, pMixSink=%p\n", pStreamShared->u8SD, fEnable, pStreamR3->pMixSink));

    /* First, enable or disable the stream and the stream's sink, if any. */
    int               rc    = VINF_SUCCESS;
    PAUDMIXSINK const pSink = pStreamR3->pMixSink ? pStreamR3->pMixSink->pMixSink : NULL;
    if (pSink)
    {
        if (fEnable)
        {
            if (pStreamR3->State.pAioRegSink != pSink)
            {
                if (pStreamR3->State.pAioRegSink)
                {
                    rc = AudioMixerSinkRemoveUpdateJob(pStreamR3->State.pAioRegSink, hdaR3StreamUpdateAsyncIoJob, pStreamR3);
                    AssertRC(rc);
                }
                rc = AudioMixerSinkAddUpdateJob(pSink, hdaR3StreamUpdateAsyncIoJob, pStreamR3,
                                                pStreamShared->State.Cfg.Device.cMsSchedulingHint);
                AssertLogRelRC(rc);
                pStreamR3->State.pAioRegSink = RT_SUCCESS(rc) ? pSink : NULL;
            }
            rc = AudioMixerSinkStart(pSink);
        }
        else
            rc = AudioMixerSinkDrainAndStop(pSink,
                                            pStreamR3->State.pCircBuf ? (uint32_t)RTCircBufUsed(pStreamR3->State.pCircBuf) : 0);
    }
    if (   RT_SUCCESS(rc)
        && fEnable
        && pStreamR3->Dbg.Runtime.fEnabled)
    {
        Assert(AudioHlpPcmPropsAreValid(&pStreamShared->State.Cfg.Props));

        if (fEnable)
        {
            if (!AudioHlpFileIsOpen(pStreamR3->Dbg.Runtime.pFileStream))
            {
                int rc2 = AudioHlpFileOpen(pStreamR3->Dbg.Runtime.pFileStream, AUDIOHLPFILE_DEFAULT_OPEN_FLAGS,
                                           &pStreamShared->State.Cfg.Props);
                AssertRC(rc2);
            }

            if (!AudioHlpFileIsOpen(pStreamR3->Dbg.Runtime.pFileDMARaw))
            {
                int rc2 = AudioHlpFileOpen(pStreamR3->Dbg.Runtime.pFileDMARaw, AUDIOHLPFILE_DEFAULT_OPEN_FLAGS,
                                           &pStreamShared->State.Cfg.Props);
                AssertRC(rc2);
            }

            if (!AudioHlpFileIsOpen(pStreamR3->Dbg.Runtime.pFileDMAMapped))
            {
                int rc2 = AudioHlpFileOpen(pStreamR3->Dbg.Runtime.pFileDMAMapped, AUDIOHLPFILE_DEFAULT_OPEN_FLAGS,
                                           &pStreamShared->State.Cfg.Props);
                AssertRC(rc2);
            }
        }
    }

    if (RT_SUCCESS(rc))
    {
        if (fEnable)
            pStreamShared->State.tsTransferLast = 0; /* Make sure it's not stale and messes up WALCLK calculations. */
        pStreamShared->State.fRunning = fEnable;

        /*
         * Set the FIFORDY bit when we start running and clear it when stopping.
         *
         * This prevents Linux from timing out in snd_hdac_stream_sync when starting
         * a stream.  Technically, Linux also uses the SSYNC feature there, but we
         * can get away with just setting the FIFORDY bit for now.
         */
        if (fEnable)
            HDA_STREAM_REG(pThis, STS, pStreamShared->u8SD) |= HDA_SDSTS_FIFORDY;
        else
            HDA_STREAM_REG(pThis, STS, pStreamShared->u8SD) &= ~HDA_SDSTS_FIFORDY;
    }

    LogFunc(("[SD%RU8] rc=%Rrc\n", pStreamShared->u8SD, rc));
    return rc;
}

/**
 * Marks the stream as started.
 *
 * Used after the stream has been enabled and the DMA timer has been armed.
 */
void hdaR3StreamMarkStarted(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTREAM pStreamShared, uint64_t tsNow)
{
    pStreamShared->State.tsLastReadNs  = RTTimeNanoTS();
    pStreamShared->State.tsStart       = tsNow;
    pStreamShared->State.tsAioDelayEnd = tsNow + PDMDevHlpTimerFromMilli(pDevIns, pStreamShared->hTimer, pThis->msInitialDelay);
    Log3Func(("#%u: tsStart=%RU64 tsAioDelayEnd=%RU64 tsLastReadNs=%RU64\n", pStreamShared->u8SD,
              pStreamShared->State.tsStart, pStreamShared->State.tsAioDelayEnd, pStreamShared->State.tsLastReadNs));

}

/**
 * Marks the stream as stopped.
 */
void hdaR3StreamMarkStopped(PHDASTREAM pStreamShared)
{
    Log3Func(("#%u\n", pStreamShared->u8SD));
    pStreamShared->State.tsAioDelayEnd = UINT64_MAX;
}

#endif /* IN_RING3 */

/**
 * Updates an HDA stream's current read or write buffer position (depending on the stream type) by
 * setting its associated LPIB register and DMA position buffer (if enabled) to an absolute value.
 *
 * @param   pStreamShared       HDA stream to update read / write position for (shared).
 * @param   pDevIns             The device instance.
 * @param   pThis               The shared HDA device state.
 * @param   uLPIB               Absolute position (in bytes) to set current read / write position to.
 */
static void hdaStreamSetPositionAbs(PHDASTREAM pStreamShared, PPDMDEVINS pDevIns, PHDASTATE pThis, uint32_t uLPIB)
{
    AssertPtrReturnVoid(pStreamShared);
    AssertMsgStmt(uLPIB <= pStreamShared->u32CBL, ("%#x\n", uLPIB), uLPIB = pStreamShared->u32CBL);

    Log3Func(("[SD%RU8] LPIB=%RU32 (DMA Position Buffer Enabled: %RTbool)\n",  pStreamShared->u8SD, uLPIB, pThis->fDMAPosition));

    /* Update LPIB in any case. */
    HDA_STREAM_REG(pThis, LPIB, pStreamShared->u8SD) = uLPIB;

    /* Do we need to tell the current DMA position? */
    if (pThis->fDMAPosition)
    {
        /*
         * Linux switched to using the position buffers some time during 2.6.x.
         * 2.6.12 used LPIB, 2.6.17 defaulted to DMA position buffers, between
         * the two version things were being changing quite a bit.
         *
         * Since 2.6.17, they will treat a zero DMA position value during the first
         * period/IRQ as reason to fall back to LPIB mode (see azx_position_ok in
         * 2.6.27+, and azx_pcm_pointer before that).  They later also added
         * UINT32_MAX to the values causing same.
         *
         * Since 2.6.35 azx_position_ok will read the wall clock register before
         * determining the position.
         */
        int rc2 = PDMDevHlpPCIPhysWrite(pDevIns,
                                        pThis->u64DPBase + (pStreamShared->u8SD * 2 * sizeof(uint32_t)),
                                        (void *)&uLPIB, sizeof(uint32_t));
        AssertRC(rc2);
    }
}

/**
 * Updates an HDA stream's current read or write buffer position (depending on the stream type) by
 * adding a value to its associated LPIB register and DMA position buffer (if enabled).
 *
 * @note    Handles automatic CBL wrap-around.
 *
 * @param   pStreamShared       HDA stream to update read / write position for (shared).
 * @param   pDevIns             The device instance.
 * @param   pThis               The shared HDA device state.
 * @param   cbToAdd             Position (in bytes) to add to the current read / write position.
 */
static void hdaStreamSetPositionAdd(PHDASTREAM pStreamShared, PPDMDEVINS pDevIns, PHDASTATE pThis, uint32_t cbToAdd)
{
    if (cbToAdd) /* No need to update anything if 0. */
    {
        uint32_t const uCBL = pStreamShared->u32CBL;
        if (uCBL) /* paranoia */
        {
            uint32_t uNewLpid = HDA_STREAM_REG(pThis, LPIB, pStreamShared->u8SD) + cbToAdd;
#if 1 /** @todo r=bird: this is wrong according to the spec */
            uNewLpid %= uCBL;
#else
            /* The spec says it goes to CBL then wraps arpimd to 1, not back to zero. See 3.3.37.  */
            if (uNewLpid > uCBL)
                uNewLpid %= uCBL;
#endif
            hdaStreamSetPositionAbs(pStreamShared, pDevIns, pThis, uNewLpid);
        }
    }
}

#ifdef IN_RING3

/**
 * Retrieves the available size of (buffered) audio data (in bytes) of a given HDA stream.
 *
 * @returns Available data (in bytes).
 * @param   pStreamR3           HDA stream to retrieve size for (ring-3).
 */
static uint32_t hdaR3StreamGetUsed(PHDASTREAMR3 pStreamR3)
{
    AssertPtrReturn(pStreamR3, 0);

    if (pStreamR3->State.pCircBuf)
        return (uint32_t)RTCircBufUsed(pStreamR3->State.pCircBuf);
    return 0;
}

/**
 * Retrieves the free size of audio data (in bytes) of a given HDA stream.
 *
 * @returns Free data (in bytes).
 * @param   pStreamR3           HDA stream to retrieve size for (ring-3).
 */
static uint32_t hdaR3StreamGetFree(PHDASTREAMR3 pStreamR3)
{
    AssertPtrReturn(pStreamR3, 0);

    if (pStreamR3->State.pCircBuf)
        return (uint32_t)RTCircBufFree(pStreamR3->State.pCircBuf);
    return 0;
}

#endif /* IN_RING3 */

/**
 * Get the current address and number of bytes left in the current BDLE.
 *
 * @returns The current physical address.
 * @param   pStreamShared   The stream to check.
 * @param   pcbLeft         The number of bytes left at the returned address.
 */
DECLINLINE(RTGCPHYS) hdaStreamDmaBufGet(PHDASTREAM pStreamShared, uint32_t *pcbLeft)
{
    uint8_t        idxBdle    = pStreamShared->State.idxCurBdle;
    AssertStmt(idxBdle < pStreamShared->State.cBdles, idxBdle = 0);

    uint32_t const cbCurBdl   = pStreamShared->State.aBdl[idxBdle].cb;
    uint32_t       offCurBdle = pStreamShared->State.offCurBdle;
    AssertStmt(pStreamShared->State.offCurBdle <= cbCurBdl, offCurBdle = cbCurBdl);

    *pcbLeft = cbCurBdl - offCurBdle;
    return pStreamShared->State.aBdl[idxBdle].GCPhys + offCurBdle;
}

/**
 * Get the size of the current BDLE.
 *
 * @returns The size (in bytes).
 * @param   pStreamShared   The stream to check.
 */
DECLINLINE(RTGCPHYS) hdaStreamDmaBufGetSize(PHDASTREAM pStreamShared)
{
    uint8_t idxBdle = pStreamShared->State.idxCurBdle;
    AssertStmt(idxBdle < pStreamShared->State.cBdles, idxBdle = 0);
    return pStreamShared->State.aBdl[idxBdle].cb;
}

/**
 * Checks if the current BDLE is completed.
 *
 * @retval  true if complete
 * @retval  false if not.
 * @param   pStreamShared   The stream to check.
 */
DECLINLINE(bool) hdaStreamDmaBufIsComplete(PHDASTREAM pStreamShared)
{
    uint8_t const  idxBdle    = pStreamShared->State.idxCurBdle;
    AssertReturn(idxBdle < pStreamShared->State.cBdles, true);

    uint32_t const cbCurBdl   = pStreamShared->State.aBdl[idxBdle].cb;
    uint32_t const offCurBdle = pStreamShared->State.offCurBdle;
    Assert(offCurBdle <= cbCurBdl);
    return offCurBdle >= cbCurBdl;
}

/**
 * Checks if the current BDLE needs a completion IRQ.
 *
 * @retval  true if IRQ is needed.
 * @retval  false if not.
 * @param   pStreamShared   The stream to check.
 */
DECLINLINE(bool) hdaStreamDmaBufNeedsIrq(PHDASTREAM pStreamShared)
{
    uint8_t const idxBdle = pStreamShared->State.idxCurBdle;
    AssertReturn(idxBdle < pStreamShared->State.cBdles, false);
    return (pStreamShared->State.aBdl[idxBdle].fFlags & HDA_BDLE_F_IOC) != 0;
}

/**
 * Advances the DMA engine to the next BDLE.
 *
 * @param   pStreamShared   The stream which DMA engine is to be updated.
 */
DECLINLINE(void) hdaStreamDmaBufAdvanceToNext(PHDASTREAM pStreamShared)
{
    uint8_t idxBdle = pStreamShared->State.idxCurBdle;
    Assert(pStreamShared->State.offCurBdle == pStreamShared->State.aBdl[idxBdle].cb);

    if (idxBdle < pStreamShared->State.cBdles - 1)
        idxBdle++;
    else
        idxBdle = 0;
    pStreamShared->State.idxCurBdle = idxBdle;
    pStreamShared->State.offCurBdle = 0;
}

#ifdef IN_RING3

/**
 * Common do-DMA prologue code.
 *
 * @retval  true if DMA processing can take place
 * @retval  false if caller should return immediately.
 * @param   pThis           The shared HDA device state.
 * @param   pStreamShared   HDA stream to update (shared).
 * @param   pStreamR3       HDA stream to update (ring-3).
 * @param   uSD             The stream ID (for asserting).
 * @param   tsNowNs         The current RTTimeNano() value.
 * @param   pszFunction     The function name (for logging).
 */
DECLINLINE(bool) hdaR3StreamDoDmaPrologue(PHDASTATE pThis, PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, uint8_t uSD,
                                          uint64_t tsNowNs, const char *pszFunction)
{
    RT_NOREF(uSD, pszFunction);

    /*
     * Check if we should skip town...
     */
    /* Stream not running (anymore)? */
    if (pStreamShared->State.fRunning)
    { /* likely */ }
    else
    {
        Log3(("%s: [SD%RU8] Not running, skipping transfer\n", pszFunction, uSD));
        return false;
    }

    if (!(HDA_STREAM_REG(pThis, STS, uSD) & HDA_SDSTS_BCIS))
    { /* likely */ }
    else
    {
        /** @todo r=bird: This is a bit fishy.  We should make effort the reschedule
         *        the transfer immediately after the guest clears the interrupt.
         *        The same fishy code is present in AC'97 with just a little
         *        explanation as here, see @bugref{9890#c95}.
         *
         *        The reasoning is probably that the developer noticed some windows
         *        versions don't like having their BCIS interrupts bundled.  There were
         *        comments to that effect elsewhere, probably as a result of a fixed
         *        uTimerHz approach to DMA scheduling.  However, pausing DMA for a
         *        period isn't going to help us with the host backends, as they don't
         *        pause and will want samples ASAP.  So, we should at least unpause
         *        DMA as quickly as we possible when BCIS is cleared.  We might even
         *        not skip it iff the DMA work here doesn't involve raising any IOC,
         *        which is possible although unlikely. */
        Log3(("%s: [SD%RU8] BCIS bit set, skipping transfer\n", pszFunction, uSD));
        STAM_REL_COUNTER_INC(&pStreamR3->State.StatDmaSkippedPendingBcis);
        Log(("%s: [SD%RU8] BCIS bit set, skipping transfer\n", pszFunction, uSD));
# ifdef HDA_STRICT
        /* Timing emulation bug or guest is misbehaving -- let me know. */
        AssertMsgFailed(("%s: BCIS bit for stream #%RU8 still set when it shouldn't\n", pszFunction, uSD));
# endif
        return false;
    }

    /*
     * Stream sanity checks.
     */
    /* Register sanity checks. */
    Assert(uSD < HDA_MAX_STREAMS);
    Assert(pStreamShared->u64BDLBase);
    Assert(pStreamShared->u32CBL);
    Assert(pStreamShared->u8FIFOS);

    /* State sanity checks. */
    Assert(ASMAtomicReadBool(&pStreamShared->State.fInReset) == false);
    Assert(ASMAtomicReadBool(&pStreamShared->State.fRunning));

    /*
     * Some timestamp stuff for logging/debugging.
     */
    /*const uint64_t tsNowNs = RTTimeNanoTS();*/
    Log3(("%s: [SD%RU8] tsDeltaNs=%'RU64 ns\n", pszFunction, uSD, tsNowNs - pStreamShared->State.tsLastTransferNs));
    pStreamShared->State.tsLastTransferNs = tsNowNs;

    return true;
}

/**
 * Common do-DMA epilogue.
 *
 * @param   pDevIns         The device instance.
 * @param   pStreamShared   The HDA stream (shared).
 * @param   pStreamR3       The HDA stream (ring-3).
 */
DECLINLINE(void) hdaR3StreamDoDmaEpilogue(PPDMDEVINS pDevIns, PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3)
{
    /*
     * We must update this in the epilogue rather than in the prologue
     * as it is used for WALCLK calculation and we must make sure the
     * guest doesn't think we've processed the current period till we
     * actually have.
     */
    pStreamShared->State.tsTransferLast = PDMDevHlpTimerGet(pDevIns, pStreamShared->hTimer);

    /*
     * Update the buffer statistics.
     */
    pStreamR3->State.StatDmaBufUsed = (uint32_t)RTCircBufUsed(pStreamR3->State.pCircBuf);
}

#endif /* IN_RING3 */

/**
 * Completes a BDLE at the end of a DMA loop iteration, if possible.
 *
 * @retval  true if buffer completed and new loaded.
 * @retval  false if buffer not completed.
 * @param   pDevIns         The device instance.
 * @param   pThis           The shared HDA device state.
 * @param   pStreamShared   HDA stream to update (shared).
 * @param   pszFunction     The function name (for logging).
 */
DECLINLINE(bool) hdaStreamDoDmaMaybeCompleteBuffer(PPDMDEVINS pDevIns, PHDASTATE pThis,
                                                   PHDASTREAM pStreamShared, const char *pszFunction)
{
    RT_NOREF(pszFunction);

    /*
     * Is the buffer descriptor complete.
     */
    if (hdaStreamDmaBufIsComplete(pStreamShared))
    {
        Log3(("%s: [SD%RU8] Completed BDLE%u %#RX64 LB %#RX32 fFlags=%#x\n", pszFunction, pStreamShared->u8SD,
              pStreamShared->State.idxCurBdle, pStreamShared->State.aBdl[pStreamShared->State.idxCurBdle].GCPhys,
              pStreamShared->State.aBdl[pStreamShared->State.idxCurBdle].cb,
              pStreamShared->State.aBdl[pStreamShared->State.idxCurBdle].fFlags));

#if 0 /* Moved to the transfer loops */
        /*
         * Update the stream's current position.
         *
         * Do this as accurate and close to the actual data transfer as possible.
         * All guests rely on this, depending on the mechanism they use (LPIB register or DMA counters).
         *
         * Note for Windows 10: The OS' driver is *very* picky about *when* the (DMA) positions get updated!
         *                      Not doing this at the right time will result in ugly sound crackles!
         */
        hdaStreamSetPositionAdd(pStreamShared, pDevIns, pThis, hdaStreamDmaBufGetSize(pStreamShared));
#endif

        /* Does the current BDLE require an interrupt to be sent? */
        if (hdaStreamDmaBufNeedsIrq(pStreamShared))
        {
            /* If the IOCE ("Interrupt On Completion Enable") bit of the SDCTL
               register is set we need to generate an interrupt. */
            if (HDA_STREAM_REG(pThis, CTL, pStreamShared->u8SD) & HDA_SDCTL_IOCE)
            {
                /* Assert the interrupt before actually fetching the next BDLE below. */
                pStreamShared->State.cTransferPendingInterrupts = 1;
                Log3(("%s: [SD%RU8] Scheduling interrupt\n", pszFunction, pStreamShared->u8SD));

                /* Trigger an interrupt first and let hdaRegWriteSDSTS() deal with
                 * ending / beginning of a period. */
                /** @todo r=bird: What does the above comment mean? */
                HDA_STREAM_REG(pThis, STS, pStreamShared->u8SD) |= HDA_SDSTS_BCIS;
                HDA_PROCESS_INTERRUPT(pDevIns, pThis);
            }
        }

        /*
         * Advance to the next BDLE.
         */
        hdaStreamDmaBufAdvanceToNext(pStreamShared);
        return true;
    }

    Log3(("%s: [SD%RU8] Incomplete BDLE%u %#RX64 LB %#RX32 fFlags=%#x: off=%#RX32\n", pszFunction, pStreamShared->u8SD,
          pStreamShared->State.idxCurBdle, pStreamShared->State.aBdl[pStreamShared->State.idxCurBdle].GCPhys,
          pStreamShared->State.aBdl[pStreamShared->State.idxCurBdle].cb,
          pStreamShared->State.aBdl[pStreamShared->State.idxCurBdle].fFlags, pStreamShared->State.offCurBdle));
    return false;
}

#ifdef IN_RING3

/**
 * Does DMA transfer for an HDA input stream.
 *
 * Reads audio data from the HDA stream's internal DMA buffer and writing to
 * guest memory.
 *
 * @param   pDevIns             The device instance.
 * @param   pThis               The shared HDA device state.
 * @param   pStreamShared       HDA stream to update (shared).
 * @param   pStreamR3           HDA stream to update (ring-3).
 * @param   cbToConsume         The max amount of data to consume from the
 *                              internal DMA buffer.  The caller will make sure
 *                              this is always the transfer size fo the current
 *                              period (unless something is seriously wrong).
 * @param   fWriteSilence       Whether to feed the guest silence rather than
 *                              fetching bytes from the internal DMA buffer.
 *                              This is set initially while we pre-buffer a
 *                              little bit of input, so we can better handle
 *                              time catch-ups and other schduling fun.
 * @param   tsNowNs             The current RTTimeNano() value.
 *
 * @remarks Caller owns the stream lock.
 */
static void hdaR3StreamDoDmaInput(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTREAM pStreamShared,
                                  PHDASTREAMR3 pStreamR3, uint32_t const cbToConsume, bool fWriteSilence, uint64_t tsNowNs)
{
    uint8_t const uSD = pStreamShared->u8SD;
    LogFlowFunc(("ENTER - #%u cbToConsume=%#x%s\n", uSD, cbToConsume, fWriteSilence ? " silence" : ""));

    /*
     * Common prologue.
     */
    if (hdaR3StreamDoDmaPrologue(pThis, pStreamShared, pStreamR3, uSD, tsNowNs, "hdaR3StreamDoDmaInput"))
    { /* likely */ }
    else
        return;

    /*
     *
     * The DMA copy loop.
     *
     */
    uint8_t    abBounce[4096 + 128];    /* Most guest does at most 4KB BDLE. So, 4KB + space for a partial frame to reduce loops. */
    uint32_t   cbBounce = 0;            /* in case of incomplete frames between buffer segments */
    PRTCIRCBUF pCircBuf = pStreamR3->State.pCircBuf;
    uint32_t   cbLeft   = cbToConsume;
    Assert(cbLeft == pStreamShared->State.cbTransferSize);
    Assert(PDMAudioPropsIsSizeAligned(&pStreamShared->State.Cfg.Props, cbLeft));

    while (cbLeft > 0)
    {
        STAM_PROFILE_START(&pThis->StatIn, a);

        /*
         * Figure out how much we can read & write in this iteration.
         */
        uint32_t cbChunk = 0;
        RTGCPHYS GCPhys  = hdaStreamDmaBufGet(pStreamShared, &cbChunk);

        /* If we're writing silence.  */
        uint32_t cbWritten = 0;
        if (!fWriteSilence)
        {
            if (cbChunk <= cbLeft)
            { /* very likely */ }
            else
                cbChunk = cbLeft;

            /*
             * Write the host data directly into the guest buffers.
             */
            while (cbChunk > 0)
            {
                /* Grab internal DMA buffer space and read into it. */
                void /*const*/ *pvBufSrc;
                size_t          cbBufSrc;
                RTCircBufAcquireReadBlock(pCircBuf, cbChunk, &pvBufSrc, &cbBufSrc);
                AssertBreakStmt(cbBufSrc, RTCircBufReleaseReadBlock(pCircBuf, 0));

                int rc2 = PDMDevHlpPCIPhysWrite(pDevIns, GCPhys, pvBufSrc, cbBufSrc);
                AssertRC(rc2);

# ifdef HDA_DEBUG_SILENCE
                fix me if relevant;
# endif
                if (RT_LIKELY(!pStreamR3->Dbg.Runtime.fEnabled))
                { /* likely */ }
                else
                    AudioHlpFileWrite(pStreamR3->Dbg.Runtime.pFileDMARaw, pvBufSrc, cbBufSrc, 0 /* fFlags */);

# ifdef VBOX_WITH_DTRACE
                VBOXDD_HDA_STREAM_DMA_IN((uint32_t)uSD, (uint32_t)cbBufSrc, pStreamShared->State.offRead);
# endif
                pStreamShared->State.offRead += cbBufSrc;
                RTCircBufReleaseReadBlock(pCircBuf, cbBufSrc);
                STAM_COUNTER_ADD(&pThis->StatBytesWritten, cbBufSrc);

                /* advance */
                cbChunk                         -= (uint32_t)cbBufSrc;
                cbWritten                       += (uint32_t)cbBufSrc;
                GCPhys                          +=           cbBufSrc;
                pStreamShared->State.offCurBdle += (uint32_t)cbBufSrc;
            }
        }
        /*
         * We've got some initial silence to write, or we need to do
         * channel mapping.  We produce guest output into the bounce buffer,
         * which is then copied into guest memory.  The bounce buffer may keep
         * partial frames there for the next BDLE, if an BDLE isn't frame aligned.
         *
         * Note! cbLeft is relative to the input (host) frame size.
         *       cbChunk OTOH is relative to output (guest) size.
         */
        else
        {
/** @todo clean up host/guest props distinction, they're the same now w/o the
 *        mapping done by the mixer rather than us.  */
            PCPDMAUDIOPCMPROPS pGuestProps = &pStreamShared->State.Cfg.Props;
            Assert(PDMAudioPropsIsSizeAligned(&pStreamShared->State.Cfg.Props, cbLeft));
            uint32_t const cbLeftGuest = PDMAudioPropsFramesToBytes(pGuestProps,
                                                                    PDMAudioPropsBytesToFrames(&pStreamShared->State.Cfg.Props,
                                                                                               cbLeft));
            if (cbChunk <= cbLeftGuest)
            { /* very likely */ }
            else
                cbChunk = cbLeftGuest;

            /*
             * Work till we've covered the chunk.
             */
            Log5Func(("loop0:  GCPhys=%RGp cbChunk=%#x + cbBounce=%#x\n", GCPhys, cbChunk, cbBounce));
            while (cbChunk > 0)
            {
                /* Figure out how much we need to convert into the bounce buffer: */
                uint32_t cbGuest = PDMAudioPropsRoundUpBytesToFrame(pGuestProps, cbChunk - cbBounce);
                uint32_t cFrames = PDMAudioPropsBytesToFrames(pGuestProps, RT_MIN(cbGuest, sizeof(abBounce) - cbBounce));

                cbGuest  = PDMAudioPropsFramesToBytes(pGuestProps, cFrames);
                PDMAudioPropsClearBuffer(pGuestProps, &abBounce[cbBounce], cbGuest, cFrames);
                cbGuest += cbBounce;

                /* Write it to the guest buffer. */
                uint32_t cbGuestActual = RT_MIN(cbGuest, cbChunk);
                int rc2 = PDMDevHlpPCIPhysWrite(pDevIns, GCPhys, abBounce, cbGuestActual);
                AssertRC(rc2);
                STAM_COUNTER_ADD(&pThis->StatBytesWritten, cbGuestActual);

                /* advance */
                cbWritten                       += cbGuestActual;
                cbChunk                         -= cbGuestActual;
                GCPhys                          += cbGuestActual;
                pStreamShared->State.offCurBdle += cbGuestActual;

                cbBounce = cbGuest - cbGuestActual;
                if (cbBounce)
                    memmove(abBounce, &abBounce[cbGuestActual], cbBounce);

                Log5Func((" loop1: GCPhys=%RGp cbGuestActual=%#x cbBounce=%#x cFrames=%#x\n", GCPhys, cbGuestActual, cbBounce, cFrames));
            }
            Log5Func(("loop0: GCPhys=%RGp cbBounce=%#x cbLeft=%#x\n", GCPhys, cbBounce, cbLeft - cbWritten));
        }

        cbLeft -= cbWritten;
        STAM_PROFILE_STOP(&pThis->StatIn, a);

        /*
         * Complete the buffer if necessary (common with the output DMA code).
         *
         * Must update the DMA position before we do this as the buffer IRQ may
         * fire on another vCPU and run in parallel to us, although it is very
         * unlikely it can make much progress as long as we're sitting on the
         * lock, it could still read the DMA position (Linux won't, as it reads
         * WALCLK and possibly SDnSTS before the DMA position).
         */
        hdaStreamSetPositionAdd(pStreamShared, pDevIns, pThis, cbWritten);
        hdaStreamDoDmaMaybeCompleteBuffer(pDevIns, pThis, pStreamShared, "hdaR3StreamDoDmaInput");
    }

    Assert(cbLeft == 0); /* There shall be no break statements in the above loop, so cbLeft is always zero here! */
    AssertMsg(cbBounce == 0, ("%#x\n", cbBounce));

    /*
     * Common epilogue.
     */
    hdaR3StreamDoDmaEpilogue(pDevIns, pStreamShared, pStreamR3);

    /*
     * Log and leave.
     */
    Log3Func(("LEAVE - [SD%RU8] %#RX32/%#RX32 @ %#RX64 - cTransferPendingInterrupts=%RU8\n",
              uSD, cbToConsume, pStreamShared->State.cbTransferSize, pStreamShared->State.offRead - cbToConsume,
              pStreamShared->State.cTransferPendingInterrupts));
}


/**
 * Input streams: Pulls data from the mixer, putting it in the internal DMA
 * buffer.
 *
 * @param   pStreamShared   HDA stream to update (shared).
 * @param   pStreamR3       HDA stream to update (ring-3 bits).
 * @param   pSink           The mixer sink to pull from.
 */
static void hdaR3StreamPullFromMixer(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, PAUDMIXSINK pSink)
{
# ifdef LOG_ENABLED
    uint64_t const offWriteOld = pStreamShared->State.offWrite;
# endif
    pStreamShared->State.offWrite = AudioMixerSinkTransferToCircBuf(pSink,
                                                                    pStreamR3->State.pCircBuf,
                                                                    pStreamShared->State.offWrite,
                                                                    pStreamR3->u8SD,
                                                                    pStreamR3->Dbg.Runtime.fEnabled
                                                                    ? pStreamR3->Dbg.Runtime.pFileStream : NULL);

    Log3Func(("[SD%RU8] transferred=%#RX64 bytes -> @%#RX64\n", pStreamR3->u8SD,
              pStreamShared->State.offWrite - offWriteOld, pStreamShared->State.offWrite));

    /* Update buffer stats. */
    pStreamR3->State.StatDmaBufUsed = (uint32_t)RTCircBufUsed(pStreamR3->State.pCircBuf);
}


/**
 * Does DMA transfer for an HDA output stream.
 *
 * This transfers one DMA timer period worth of data from the guest and into the
 * internal DMA buffer.
 *
 * @param   pDevIns             The device instance.
 * @param   pThis               The shared HDA device state.
 * @param   pStreamShared       HDA stream to update (shared).
 * @param   pStreamR3           HDA stream to update (ring-3).
 * @param   cbToProduce         The max amount of data to produce (i.e. put into
 *                              the circular buffer).  Unless something is going
 *                              seriously wrong, this will always be transfer
 *                              size for the current period.
 * @param   tsNowNs             The current RTTimeNano() value.
 *
 * @remarks Caller owns the stream lock.
 */
static void hdaR3StreamDoDmaOutput(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTREAM pStreamShared,
                                   PHDASTREAMR3 pStreamR3, uint32_t const cbToProduce, uint64_t tsNowNs)
{
    uint8_t const uSD = pStreamShared->u8SD;
    LogFlowFunc(("ENTER - #%u cbToProduce=%#x\n", uSD, cbToProduce));

    /*
     * Common prologue.
     */
    if (hdaR3StreamDoDmaPrologue(pThis, pStreamShared, pStreamR3, uSD, tsNowNs, "hdaR3StreamDoDmaOutput"))
    { /* likely */ }
    else
        return;

    /*
     *
     * The DMA copy loop.
     *
     */
# if 0
    uint8_t    abBounce[4096 + 128];    /* Most guest does at most 4KB BDLE. So, 4KB + space for a partial frame to reduce loops. */
    uint32_t   cbBounce = 0;            /* in case of incomplete frames between buffer segments */
# endif
    PRTCIRCBUF pCircBuf = pStreamR3->State.pCircBuf;
    uint32_t   cbLeft   = cbToProduce;
# ifdef VBOX_HDA_WITH_ON_REG_ACCESS_DMA
    Assert(cbLeft <= pStreamShared->State.cbTransferSize); /* a little pointless with the DMA'ing on LPIB read. */
# else
    Assert(cbLeft == pStreamShared->State.cbTransferSize);
# endif
    Assert(PDMAudioPropsIsSizeAligned(&pStreamShared->State.Cfg.Props, cbLeft));

    while (cbLeft > 0)
    {
        STAM_PROFILE_START(&pThis->StatOut, a);

        /*
         * Figure out how much we can read & write in this iteration.
         */
        uint32_t cbChunk = 0;
        RTGCPHYS GCPhys  = hdaStreamDmaBufGet(pStreamShared, &cbChunk);

        /* Need to diverge if the BDLEs contain misaligned entries.  */
        uint32_t cbRead  = 0;
# if 0
        if (/** @todo pStreamShared->State.fFrameAlignedBuffers */)
# endif
        {
            if (cbChunk <= cbLeft)
            { /* very likely */ }
            else
                cbChunk = cbLeft;

            /*
             * Read the guest data directly into the internal DMA buffer.
             */
            while (cbChunk > 0)
            {
                /* Grab internal DMA buffer space and read into it. */
                void  *pvBufDst;
                size_t cbBufDst;
                RTCircBufAcquireWriteBlock(pCircBuf, cbChunk, &pvBufDst, &cbBufDst);
                AssertBreakStmt(cbBufDst, RTCircBufReleaseWriteBlock(pCircBuf, 0));

                int rc2 = PDMDevHlpPCIPhysRead(pDevIns, GCPhys, pvBufDst, cbBufDst);
                AssertRC(rc2);

# ifdef HDA_DEBUG_SILENCE
                fix me if relevant;
# endif
                if (RT_LIKELY(!pStreamR3->Dbg.Runtime.fEnabled))
                { /* likely */ }
                else
                    AudioHlpFileWrite(pStreamR3->Dbg.Runtime.pFileDMARaw, pvBufDst, cbBufDst, 0 /* fFlags */);

# ifdef VBOX_WITH_DTRACE
                VBOXDD_HDA_STREAM_DMA_OUT((uint32_t)uSD, (uint32_t)cbBufDst, pStreamShared->State.offWrite);
# endif
                pStreamShared->State.offWrite   += cbBufDst;
                RTCircBufReleaseWriteBlock(pCircBuf, cbBufDst);
                STAM_COUNTER_ADD(&pThis->StatBytesRead, cbBufDst);

                /* advance */
                cbChunk                         -= (uint32_t)cbBufDst;
                cbRead                          += (uint32_t)cbBufDst;
                GCPhys                          +=           cbBufDst;
                pStreamShared->State.offCurBdle += (uint32_t)cbBufDst;
            }
        }
# if 0
        /*
         * Need to map the frame content, so we need to read the guest data
         * into a temporary buffer, though the output can be directly written
         * into the internal buffer as it is assumed to be frame aligned.
         *
         * Note! cbLeft is relative to the output frame size.
         *       cbChunk OTOH is relative to input size.
         */
        else
        {
/** @todo clean up host/guest props distinction, they're the same now w/o the
 *        mapping done by the mixer rather than us.  */
            PCPDMAUDIOPCMPROPS pGuestProps = &pStreamShared->State.Cfg.Props;
            Assert(PDMAudioPropsIsSizeAligned(&pStreamShared->State.Cfg.Props, cbLeft));
            uint32_t const cbLeftGuest = PDMAudioPropsFramesToBytes(pGuestProps,
                                                                    PDMAudioPropsBytesToFrames(&pStreamShared->State.Cfg.Props,
                                                                                               cbLeft));
            if (cbChunk <= cbLeftGuest)
            { /* very likely */ }
            else
                cbChunk = cbLeftGuest;

            /*
             * Loop till we've covered the chunk.
             */
            Log5Func(("loop0:  GCPhys=%RGp cbChunk=%#x + cbBounce=%#x\n", GCPhys, cbChunk, cbBounce));
            while (cbChunk > 0)
            {
                /* Read into the bounce buffer. */
                uint32_t const cbToRead = RT_MIN(cbChunk, sizeof(abBounce) - cbBounce);
                int rc2 = PDMDevHlpPCIPhysRead(pDevIns, GCPhys, &abBounce[cbBounce], cbToRead);
                AssertRC(rc2);
                cbBounce += cbToRead;

                /* Convert the size to whole frames and a remainder. */
                uint32_t cFrames = PDMAudioPropsBytesToFrames(pGuestProps, cbBounce);
                uint32_t const cbRemainder = cbBounce - PDMAudioPropsFramesToBytes(pGuestProps, cFrames);
                Log5Func((" loop1: GCPhys=%RGp cbToRead=%#x cbBounce=%#x cFrames=%#x\n", GCPhys, cbToRead, cbBounce, cFrames));

                /*
                 * Convert from the bounce buffer and into the internal DMA buffer.
                 */
                uint32_t offBounce = 0;
                while (cFrames > 0)
                {
                    void  *pvBufDst;
                    size_t cbBufDst;
                    RTCircBufAcquireWriteBlock(pCircBuf, PDMAudioPropsFramesToBytes(&pStreamShared->State.Cfg.Props, cFrames),
                                               &pvBufDst, &cbBufDst);

                    uint32_t const cFramesToConvert = PDMAudioPropsBytesToFrames(&pStreamShared->State.Cfg.Props, (uint32_t)cbBufDst);
                    Assert(PDMAudioPropsFramesToBytes(&pStreamShared->State.Cfg.Props, cFramesToConvert) == cbBufDst);
                    Assert(cFramesToConvert > 0);
                    Assert(cFramesToConvert <= cFrames);

                    pStreamR3->State.Mapping.pfnGuestToHost(pvBufDst, &abBounce[offBounce], cFramesToConvert,
                                                            &pStreamR3->State.Mapping);
                    Log5Func(("  loop2: offBounce=%#05x cFramesToConvert=%#05x cbBufDst=%#x%s\n",
                              offBounce, cFramesToConvert, cbBufDst, ASMMemIsZero(pvBufDst, cbBufDst) ? " all zero" : ""));

#  ifdef HDA_DEBUG_SILENCE
                    fix me if relevant;
#  endif
                    if (RT_LIKELY(!pStreamR3->Dbg.Runtime.fEnabled))
                    { /* likely */ }
                    else
                        AudioHlpFileWrite(pStreamR3->Dbg.Runtime.pFileDMARaw, pvBufDst, cbBufDst, 0 /* fFlags */);

                    pStreamR3->State.offWrite += cbBufDst;
                    RTCircBufReleaseWriteBlock(pCircBuf, cbBufDst);
                    STAM_COUNTER_ADD(&pThis->StatBytesRead, cbBufDst);

                    /* advance */
                    cbLeft    -= (uint32_t)cbBufDst;
                    cFrames   -= cFramesToConvert;
                    offBounce += PDMAudioPropsFramesToBytes(pGuestProps, cFramesToConvert);
                }

                /* advance */
                cbChunk                         -= cbToRead;
                GCPhys                          += cbToRead;
                pStreamShared->State.offCurBdle += cbToRead;
                if (cbRemainder)
                    memmove(&abBounce[0], &abBounce[cbBounce - cbRemainder], cbRemainder);
                cbBounce = cbRemainder;
            }
            Log5Func(("loop0: GCPhys=%RGp cbBounce=%#x cbLeft=%#x\n", GCPhys, cbBounce, cbLeft));
        }
# endif

        cbLeft -= cbRead;
        STAM_PROFILE_STOP(&pThis->StatOut, a);

        /*
         * Complete the buffer if necessary (common with the input DMA code).
         *
         * Must update the DMA position before we do this as the buffer IRQ may
         * fire on another vCPU and run in parallel to us, although it is very
         * unlikely it can make much progress as long as we're sitting on the
         * lock, it could still read the DMA position (Linux won't, as it reads
         * WALCLK and possibly SDnSTS before the DMA position).
         */
        hdaStreamSetPositionAdd(pStreamShared, pDevIns, pThis, cbRead);
        hdaStreamDoDmaMaybeCompleteBuffer(pDevIns, pThis, pStreamShared, "hdaR3StreamDoDmaOutput");
    }

    Assert(cbLeft == 0); /* There shall be no break statements in the above loop, so cbLeft is always zero here! */
# if 0
    AssertMsg(cbBounce == 0, ("%#x\n", cbBounce));
# endif

    /*
     * Common epilogue.
     */
    hdaR3StreamDoDmaEpilogue(pDevIns, pStreamShared, pStreamR3);

    /*
     * Log and leave.
     */
    Log3Func(("LEAVE - [SD%RU8] %#RX32/%#RX32 @ %#RX64 - cTransferPendingInterrupts=%RU8\n",
              uSD, cbToProduce, pStreamShared->State.cbTransferSize, pStreamShared->State.offWrite - cbToProduce,
              pStreamShared->State.cTransferPendingInterrupts));
}

#endif /* IN_RING3 */

#ifdef VBOX_HDA_WITH_ON_REG_ACCESS_DMA
/**
 * Do DMA output transfer on LPIB register access.
 *
 * @returns VINF_SUCCESS or VINF_IOM_R3_MMIO_READ.
 * @param   pDevIns         The device instance.
 * @param   pThis           The shared instance data.
 * @param   pStreamShared   The shared stream data.
 * @param   cbToTransfer    How much to transfer.
 */
VBOXSTRICTRC hdaStreamDoOnAccessDmaOutput(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTREAM pStreamShared, uint32_t cbToTransfer)
{
    AssertReturn(cbToTransfer > 0, VINF_SUCCESS);
    int rc = VINF_SUCCESS;

    /*
     * Check if we're exceeding the available buffer, go to ring-3 to
     * handle that (we would perhaps always take this path when in ring-3).
     */
    uint32_t cbDma = pStreamShared->State.cbDma;
    ASMCompilerBarrier();
    if (   cbDma                >= sizeof(pStreamShared->State.abDma) /* paranoia */
        || cbToTransfer         >= sizeof(pStreamShared->State.abDma) /* paranoia */
        || cbDma + cbToTransfer >  sizeof(pStreamShared->State.abDma))
    {
# ifndef IN_RING3
        STAM_REL_COUNTER_INC(&pThis->StatAccessDmaOutputToR3);
        LogFlowFunc(("[SD%RU8] out of DMA buffer space (%#x, need %#x) -> VINF_IOM_R3_MMIO_READ\n",
                     pStreamShared->u8SD, sizeof(pStreamShared->State.abDma) - pStreamShared->State.cbDma, cbToTransfer));
        return VINF_IOM_R3_MMIO_READ;
# else  /* IN_RING3 */
        /*
         * Flush the bounce buffer, then do direct transfers to the
         * internal DMA buffer (updates LPIB).
         */
        PHDASTATER3 const       pThisCC       = PDMDEVINS_2_DATA_CC(pDevIns, PHDASTATER3);
        uintptr_t const         idxStream     = pStreamShared->u8SD;
        AssertReturn(idxStream < RT_ELEMENTS(pThisCC->aStreams), VERR_INTERNAL_ERROR_4);
        PHDASTREAMR3 const      pStreamR3     = &pThisCC->aStreams[idxStream];

        hdaR3StreamFlushDmaBounceBufferOutput(pStreamShared, pStreamR3);

        uint32_t cbStreamFree = hdaR3StreamGetFree(pStreamR3);
        if (cbStreamFree >= cbToTransfer)
        { /* likely */ }
        else
        {
            PAUDMIXSINK pSink = pStreamR3->pMixSink ? pStreamR3->pMixSink->pMixSink : NULL;
            if (pSink)
                cbStreamFree = hdaR3StreamHandleDmaBufferOverrun(pStreamShared, pStreamR3, pSink, cbToTransfer, RTTimeNanoTS(),
                                                                 "hdaStreamDoOnAccessDmaOutput", cbStreamFree);
            else
            {
                LogFunc(("[SD%RU8] No sink and insufficient internal DMA buffer space (%#x) - won't do anything\n",
                         pStreamShared->u8SD, cbStreamFree));
                return VINF_SUCCESS;
            }
            cbToTransfer = RT_MIN(cbToTransfer, cbStreamFree);
            if (cbToTransfer < PDMAudioPropsFrameSize(&pStreamShared->State.Cfg.Props))
            {
                LogFunc(("[SD%RU8] No internal DMA buffer space (%#x) - won't do anything\n", pStreamShared->u8SD, cbStreamFree));
                return VINF_SUCCESS;
            }
        }
        hdaR3StreamDoDmaOutput(pDevIns, pThis, pStreamShared, pStreamR3, cbToTransfer, RTTimeNanoTS());
        pStreamShared->State.cbDmaTotal += cbToTransfer;
# endif /* IN_RING3 */
    }
    else
    {
        /*
         * Transfer into the DMA bounce buffer.
         */
        LogFlowFunc(("[SD%RU8] Transfering %#x bytes to DMA bounce buffer (cbDma=%#x cbDmaTotal=%#x) (%p/%u)\n",
                     pStreamShared->u8SD, cbToTransfer, cbDma, pStreamShared->State.cbDmaTotal, pStreamShared, pStreamShared->u8SD));
        uint32_t cbLeft = cbToTransfer;
        do
        {
            uint32_t cbChunk = 0;
            RTGCPHYS GCPhys  = hdaStreamDmaBufGet(pStreamShared, &cbChunk);

            bool fMustAdvanceBuffer;
            if (cbLeft < cbChunk)
            {
                fMustAdvanceBuffer = false;
                cbChunk            = cbLeft;
            }
            else
                fMustAdvanceBuffer = true;

            /* Read the guest data directly into the DMA bounce buffer. */
            int rc2 = PDMDevHlpPCIPhysRead(pDevIns, GCPhys, &pStreamShared->State.abDma[cbDma], cbChunk);
            AssertRC(rc2);

            /* We update offWrite and StatBytesRead here even if we haven't moved the data
               to the internal DMA buffer yet, because we want the dtrace even to fire here. */
# ifdef VBOX_WITH_DTRACE
            VBOXDD_HDA_STREAM_DMA_OUT((uint32_t)pStreamShared->u8SD, cbChunk, pStreamShared->State.offWrite);
# endif
            pStreamShared->State.offWrite   += cbChunk;
            STAM_COUNTER_ADD(&pThis->StatBytesRead, cbChunk);

            /* advance */
            pStreamShared->State.offCurBdle += cbChunk;
            pStreamShared->State.cbDmaTotal += cbChunk;
            cbDma                           += cbChunk;
            pStreamShared->State.cbDma       = cbDma;
            cbLeft                          -= cbChunk;
            Log6Func(("cbLeft=%#x cbDma=%#x cbDmaTotal=%#x offCurBdle=%#x idxCurBdle=%#x (%p/%u)\n",
                      cbLeft, cbDma, pStreamShared->State.cbDmaTotal, pStreamShared->State.offCurBdle,
                      pStreamShared->State.idxCurBdle, pStreamShared, pStreamShared->u8SD));

            /* Next buffer. */
            bool fAdvanced = hdaStreamDoDmaMaybeCompleteBuffer(pDevIns, pThis, pStreamShared, "hdaStreamDoOnAccessDmaOutput");
            AssertMsgStmt(fMustAdvanceBuffer == fAdvanced, ("%d %d\n", fMustAdvanceBuffer, fAdvanced), rc = VERR_INTERNAL_ERROR_3);
        } while (cbLeft > 0);

        /*
         * Advance LPIB.
         */
        hdaStreamSetPositionAdd(pStreamShared, pDevIns, pThis, cbToTransfer - cbLeft);
    }

# ifdef VBOX_STRICT
    uint32_t       idxSched = pStreamShared->State.idxSchedule;
    AssertStmt(idxSched < RT_MIN(RT_ELEMENTS(pStreamShared->State.aSchedule), pStreamShared->State.cSchedule), idxSched = 0);
    uint32_t const cbPeriod = pStreamShared->State.aSchedule[idxSched].cbPeriod;
    AssertMsg(pStreamShared->State.cbDmaTotal < cbPeriod, ("%#x vs %#x\n", pStreamShared->State.cbDmaTotal, cbPeriod));
# endif

    STAM_REL_COUNTER_INC(&pThis->StatAccessDmaOutput);
    return rc;
}
#endif /* VBOX_HDA_WITH_ON_REG_ACCESS_DMA */


#ifdef IN_RING3

/**
 * Output streams: Pushes data to the mixer.
 *
 * @param   pStreamShared   HDA stream to update (shared bits).
 * @param   pStreamR3       HDA stream to update (ring-3 bits).
 * @param   pSink           The mixer sink to push to.
 * @param   nsNow           The current RTTimeNanoTS() value.
 */
static void hdaR3StreamPushToMixer(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, PAUDMIXSINK pSink, uint64_t nsNow)
{
# ifdef LOG_ENABLED
    uint64_t const offReadOld = pStreamShared->State.offRead;
# endif
    pStreamShared->State.offRead = AudioMixerSinkTransferFromCircBuf(pSink,
                                                                     pStreamR3->State.pCircBuf,
                                                                     pStreamShared->State.offRead,
                                                                     pStreamR3->u8SD,
                                                                     pStreamR3->Dbg.Runtime.fEnabled
                                                                     ? pStreamR3->Dbg.Runtime.pFileStream : NULL);

    Assert(nsNow >= pStreamShared->State.tsLastReadNs);
    Log3Func(("[SD%RU8] nsDeltaLastRead=%RI64 transferred=%#RX64 bytes -> @%#RX64\n", pStreamR3->u8SD,
              nsNow - pStreamShared->State.tsLastReadNs, pStreamShared->State.offRead - offReadOld, pStreamShared->State.offRead));
    RT_NOREF(pStreamShared, nsNow);

    /* Update buffer stats. */
    pStreamR3->State.StatDmaBufUsed = (uint32_t)RTCircBufUsed(pStreamR3->State.pCircBuf);
}


/**
 * Deals with a DMA buffer overrun.
 *
 * Makes sure we return with @a cbNeeded bytes of free space in pCircBuf.
 *
 * @returns Number of bytes free in the internal DMA buffer.
 * @param   pStreamShared   The shared data for the HDA stream.
 * @param   pStreamR3       The ring-3 data for the HDA stream.
 * @param   pSink           The mixer sink (valid).
 * @param   cbNeeded        How much space we need (in bytes).
 * @param   nsNow           Current RTNanoTimeTS() timestamp.
 * @param   cbStreamFree    The current amount of free buffer space.
 * @param   pszCaller       The caller (for logging).
 */
static uint32_t hdaR3StreamHandleDmaBufferOverrun(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3, PAUDMIXSINK pSink,
                                                  uint32_t cbNeeded, uint64_t nsNow,
                                                  const char *pszCaller, uint32_t const cbStreamFree)
{
    STAM_REL_COUNTER_INC(&pStreamR3->State.StatDmaFlowProblems);
    Log(("%s: Warning! Stream #%u has insufficient space free: %#x bytes, need %#x.  Will try move data out of the buffer...\n",
         pszCaller, pStreamShared->u8SD, cbStreamFree, cbNeeded));
    RT_NOREF(pszCaller, cbStreamFree);

    int rc = AudioMixerSinkTryLock(pSink);
    if (RT_SUCCESS(rc))
    {
        hdaR3StreamPushToMixer(pStreamShared, pStreamR3, pSink, nsNow);
        AudioMixerSinkUpdate(pSink, 0, 0);
        AudioMixerSinkUnlock(pSink);
    }
    else
        RTThreadYield();

    uint32_t const cbRet = hdaR3StreamGetFree(pStreamR3);
    Log(("%s: Gained %u bytes.\n", pszCaller, cbRet - cbStreamFree));
    if (cbRet >= cbNeeded)
        return cbRet;

    /*
     * Unable to make sufficient space.  Drop the whole buffer content.
     *
     * This is needed in order to keep the device emulation running at a
     * constant rate, at the cost of losing valid (but too much) data.
     */
    STAM_REL_COUNTER_INC(&pStreamR3->State.StatDmaFlowErrors);
    LogRel2(("HDA: Warning: Hit stream #%RU8 overflow, dropping %u bytes of audio data (%s)\n",
             pStreamShared->u8SD, hdaR3StreamGetUsed(pStreamR3), pszCaller));
# ifdef HDA_STRICT
    AssertMsgFailed(("Hit stream #%RU8 overflow -- timing bug?\n", pStreamShared->u8SD));
# endif
/**
 *
 * @todo r=bird: I don't think RTCircBufReset is entirely safe w/o
 * owning the AIO lock.  See the note in the documentation about it not being
 * multi-threading aware (safe).   Wish I'd verified this code much earlier.
 * Sigh^3!
 *
 */
    RTCircBufReset(pStreamR3->State.pCircBuf);
    pStreamShared->State.offWrite = 0;
    pStreamShared->State.offRead  = 0;
    return hdaR3StreamGetFree(pStreamR3);
}


# ifdef VBOX_HDA_WITH_ON_REG_ACCESS_DMA
/**
 * Flushes the DMA bounce buffer content to the internal DMA buffer.
 *
 * @param   pStreamShared   The shared data of the stream to have its DMA bounce
 *                          buffer flushed.
 * @param   pStreamR3       The ring-3 stream data for same.
 */
static void hdaR3StreamFlushDmaBounceBufferOutput(PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3)
{
    uint32_t cbDma = pStreamShared->State.cbDma;
    LogFlowFunc(("cbDma=%#x\n", cbDma));
    if (cbDma)
    {
        AssertReturnVoid(cbDma <= sizeof(pStreamShared->State.abDma));
        PRTCIRCBUF const pCircBuf = pStreamR3->State.pCircBuf;
        if (pCircBuf)
        {
            uint32_t offDma = 0;
            while (offDma < cbDma)
            {
                uint32_t const cbSrcLeft = cbDma - offDma;

                /*
                 * Grab a chunk of the internal DMA buffer.
                 */
                void  *pvBufDst = NULL;
                size_t cbBufDst = 0;
                RTCircBufAcquireWriteBlock(pCircBuf, cbSrcLeft, &pvBufDst, &cbBufDst);
                if (cbBufDst > 0)
                { /* likely */ }
                else
                {
                    /* We've got buffering trouble. */
                    RTCircBufReleaseWriteBlock(pCircBuf, 0);

                    PAUDMIXSINK pSink = pStreamR3->pMixSink ? pStreamR3->pMixSink->pMixSink : NULL;
                    if (pSink)
                        hdaR3StreamHandleDmaBufferOverrun(pStreamShared, pStreamR3, pSink, cbSrcLeft, RTTimeNanoTS(),
                                                          "hdaR3StreamFlushDmaBounceBufferOutput", 0 /*cbStreamFree*/);
                    else
                    {
                        LogFunc(("Stream #%u has no sink. Dropping the rest of the data\n", pStreamR3->u8SD));
                        break;
                    }

                    RTCircBufAcquireWriteBlock(pCircBuf, cbSrcLeft, &pvBufDst, &cbBufDst);
                    AssertBreakStmt(cbBufDst, RTCircBufReleaseWriteBlock(pCircBuf, 0));
                }

                /*
                 * Copy the samples into it and write it to the debug file if open.
                 *
                 * We do not fire the dtrace probe here nor update offRead as that was
                 * done already (not sure that was a good idea?).
                 */
                memcpy(pvBufDst, &pStreamShared->State.abDma[offDma], cbBufDst);

                if (RT_LIKELY(!pStreamR3->Dbg.Runtime.fEnabled))
                { /* likely */ }
                else
                    AudioHlpFileWrite(pStreamR3->Dbg.Runtime.pFileDMARaw, pvBufDst, cbBufDst, 0 /* fFlags */);

                RTCircBufReleaseWriteBlock(pCircBuf, cbBufDst);

                offDma += (uint32_t)cbBufDst;
            }
        }

        /*
         * Mark the buffer empty.
         */
        pStreamShared->State.cbDma = 0;
    }
}
# endif /* VBOX_HDA_WITH_ON_REG_ACCESS_DMA */


/**
 * The stream's main function when called by the timer.
 *
 * @note This function also will be called without timer invocation when
 *       starting (enabling) the stream to minimize startup latency.
 *
 * @returns Current timer time if the timer is enabled, otherwise zero.
 * @param   pDevIns         The device instance.
 * @param   pThis           The shared HDA device state.
 * @param   pThisCC         The ring-3 HDA device state.
 * @param   pStreamShared   HDA stream to update (shared bits).
 * @param   pStreamR3       HDA stream to update (ring-3 bits).
 */
uint64_t hdaR3StreamTimerMain(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTATER3 pThisCC,
                              PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3)
{
    Assert(PDMDevHlpCritSectIsOwner(pDevIns, &pThis->CritSect));
    Assert(PDMDevHlpTimerIsLockOwner(pDevIns, pStreamShared->hTimer));

    /* Do the work: */
    hdaR3StreamUpdateDma(pDevIns, pThis, pThisCC, pStreamShared, pStreamR3);

    /* Re-arm the timer if the sink is still active: */
    if (   pStreamShared->State.fRunning
        && pStreamR3->pMixSink
        && AudioMixerSinkIsActive(pStreamR3->pMixSink->pMixSink))
    {
        /* Advance the schduling: */
        uint32_t idxSched = pStreamShared->State.idxSchedule;
        AssertStmt(idxSched < RT_ELEMENTS(pStreamShared->State.aSchedule), idxSched = 0);
        uint32_t idxLoop  = pStreamShared->State.idxScheduleLoop + 1;
        if (idxLoop >= pStreamShared->State.aSchedule[idxSched].cLoops)
        {
            idxSched += 1;
            if (   idxSched >= pStreamShared->State.cSchedule
                || idxSched >= RT_ELEMENTS(pStreamShared->State.aSchedule) /*paranoia^2*/)
            {
                idxSched = pStreamShared->State.cSchedulePrologue;
                AssertStmt(idxSched < RT_ELEMENTS(pStreamShared->State.aSchedule), idxSched = 0);
            }
            pStreamShared->State.idxSchedule = idxSched;
            idxLoop = 0;
        }
        pStreamShared->State.idxScheduleLoop = (uint16_t)idxLoop;

        /* Do the actual timer re-arming. */
        uint64_t const tsNow = PDMDevHlpTimerGet(pDevIns, pStreamShared->hTimer); /* (For virtual sync this remains the same for the whole callout IIRC) */
        uint64_t const tsTransferNext = tsNow + pStreamShared->State.aSchedule[idxSched].cPeriodTicks;
        Log3Func(("[SD%RU8] fSinkActive=true, tsTransferNext=%RU64 (in %RU64)\n",
                  pStreamShared->u8SD, tsTransferNext, tsTransferNext - tsNow));
        int rc = PDMDevHlpTimerSet(pDevIns, pStreamShared->hTimer, tsTransferNext);
        AssertRC(rc);

        /* Some legacy stuff: */
        pStreamShared->State.tsTransferNext = tsTransferNext;
        pStreamShared->State.cbTransferSize = pStreamShared->State.aSchedule[idxSched].cbPeriod;

        return tsNow;
    }

    Log3Func(("[SD%RU8] fSinkActive=false\n", pStreamShared->u8SD));
    return 0;
}


/**
 * Updates a HDA stream by doing DMA transfers.
 *
 * Will do mixer transfers too to try fix an overrun/underrun situation.
 *
 * The host sink(s) set the overall pace (bird: no it doesn't, the DMA timer
 * does - we just hope like heck it matches the speed at which the *backend*
 * host audio driver processes samples).
 *
 * @param   pDevIns         The device instance.
 * @param   pThis           The shared HDA device state.
 * @param   pThisCC         The ring-3 HDA device state.
 * @param   pStreamShared   HDA stream to update (shared bits).
 * @param   pStreamR3       HDA stream to update (ring-3 bits).
 */
static void hdaR3StreamUpdateDma(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTATER3 pThisCC,
                                 PHDASTREAM pStreamShared, PHDASTREAMR3 pStreamR3)
{
    RT_NOREF(pThisCC);
    int rc2;

    /*
     * Make sure we're running and got an active mixer sink.
     */
    if (RT_LIKELY(pStreamShared->State.fRunning))
    { /* likely */ }
    else
        return;

    PAUDMIXSINK pSink = NULL;
    if (pStreamR3->pMixSink)
        pSink = pStreamR3->pMixSink->pMixSink;
    if (RT_LIKELY(AudioMixerSinkIsActive(pSink)))
    { /* likely */ }
    else
        return;

    /*
     * Get scheduling info common to both input and output streams.
     */
    const uint64_t tsNowNs  = RTTimeNanoTS();
    uint32_t       idxSched = pStreamShared->State.idxSchedule;
    AssertStmt(idxSched < RT_MIN(RT_ELEMENTS(pStreamShared->State.aSchedule), pStreamShared->State.cSchedule), idxSched = 0);
    uint32_t       cbPeriod = pStreamShared->State.aSchedule[idxSched].cbPeriod;

    /*
     * Output streams (SDO).
     */
    if (hdaGetDirFromSD(pStreamShared->u8SD) == PDMAUDIODIR_OUT)
    {
# ifdef VBOX_HDA_WITH_ON_REG_ACCESS_DMA
        /* Subtract already transferred bytes and flush the DMA bounce buffer. */
        uint32_t cbDmaTotal = pStreamShared->State.cbDmaTotal;
        if (cbDmaTotal > 0)
        {
            AssertStmt(cbDmaTotal < cbPeriod, cbDmaTotal = cbPeriod);
            cbPeriod -= cbDmaTotal;
            pStreamShared->State.cbDmaTotal = 0;
            hdaR3StreamFlushDmaBounceBufferOutput(pStreamShared, pStreamR3);
        }
        else
            Assert(pStreamShared->State.cbDma == 0);
# endif

        /*
         * Check how much room we have in our DMA buffer.  There should be at
         * least one period worth of space there or we're in an overflow situation.
         */
        uint32_t cbStreamFree = hdaR3StreamGetFree(pStreamR3);
        if (cbStreamFree >= cbPeriod)
        { /* likely */ }
        else
            cbStreamFree = hdaR3StreamHandleDmaBufferOverrun(pStreamShared, pStreamR3, pSink, cbPeriod, tsNowNs,
                                                             "hdaR3StreamUpdateDma", cbStreamFree);

        /*
         * Do the DMA transfer.
         */
        uint64_t const offWriteBefore = pStreamShared->State.offWrite;
        hdaR3StreamDoDmaOutput(pDevIns, pThis, pStreamShared, pStreamR3, RT_MIN(cbStreamFree, cbPeriod), tsNowNs);

        /*
         * Should we push data to down thru the mixer to and to the host drivers?
         *
         * We initially delay this by pThis->msInitialDelay, but after than we'll
         * kick the AIO thread every time we've put more data in the buffer (which is
         * every time) as the host audio device needs to get data in a timely manner.
         *
         * (We used to try only wake up the AIO thread according to pThis->uIoTimer
         * and host wall clock, but that meant we would miss a wakup after the DMA
         * timer was called a little late or if TM entered into catch-up mode.)
         */
        bool fKickAioThread;
        if (!pStreamShared->State.tsAioDelayEnd)
            fKickAioThread = pStreamShared->State.offWrite > offWriteBefore
                          || hdaR3StreamGetFree(pStreamR3) < pStreamShared->State.cbAvgTransfer * 2;
        else if (PDMDevHlpTimerGet(pDevIns, pStreamShared->hTimer) >= pStreamShared->State.tsAioDelayEnd)
        {
            Log3Func(("Initial delay done: Passed tsAioDelayEnd.\n"));
            pStreamShared->State.tsAioDelayEnd = 0;
            fKickAioThread = true;
        }
        else if (hdaR3StreamGetFree(pStreamR3) < pStreamShared->State.cbAvgTransfer * 2)
        {
            Log3Func(("Initial delay done: Passed running short on buffer.\n"));
            pStreamShared->State.tsAioDelayEnd = 0;
            fKickAioThread = true;
        }
        else
        {
            Log3Func(("Initial delay pending...\n"));
            fKickAioThread = false;
        }

        Log3Func(("msDelta=%RU64 (vs %u) cbStreamFree=%#x (vs %#x) => fKickAioThread=%RTbool\n",
                  (tsNowNs - pStreamShared->State.tsLastReadNs) / RT_NS_1MS,
                  pStreamShared->State.Cfg.Device.cMsSchedulingHint, cbStreamFree,
                  pStreamShared->State.cbAvgTransfer * 2, fKickAioThread));

        if (fKickAioThread)
        {
            /* Notify the async I/O worker thread that there's work to do. */
            Log5Func(("Notifying AIO thread\n"));
            rc2 = AudioMixerSinkSignalUpdateJob(pSink);
            AssertRC(rc2);
            /* Update last read timestamp for logging/debugging. */
            pStreamShared->State.tsLastReadNs = tsNowNs;
        }
    }
    /*
     * Input stream (SDI).
     */
    else
    {
        Assert(hdaGetDirFromSD(pStreamShared->u8SD) == PDMAUDIODIR_IN);

        /*
         * See how much data we've got buffered...
         */
        bool     fWriteSilence = false;
        uint32_t cbStreamUsed  = hdaR3StreamGetUsed(pStreamR3);
        if (pStreamShared->State.fInputPreBuffered && cbStreamUsed >= cbPeriod)
        { /*likely*/ }
        /*
         * Because it may take a while for the input stream to get going (at
         * least with pulseaudio), we feed the guest silence till we've
         * pre-buffer a reasonable amount of audio.
         */
        else if (!pStreamShared->State.fInputPreBuffered)
        {
            if (cbStreamUsed < pStreamShared->State.cbInputPreBuffer)
            {
                Log3(("hdaR3StreamUpdateDma: Pre-buffering (got %#x out of %#x bytes)...\n",
                      cbStreamUsed, pStreamShared->State.cbInputPreBuffer));
                fWriteSilence = true;
            }
            else
            {
                Log3(("hdaR3StreamUpdateDma: Completed pre-buffering (got %#x, needed %#x bytes).\n",
                      cbStreamUsed, pStreamShared->State.cbInputPreBuffer));
                pStreamShared->State.fInputPreBuffered = true;
                fWriteSilence = true; /* For now, just do the most conservative thing. */
            }
            cbStreamUsed = cbPeriod;
        }
        /*
         * When we're low on data, we must really try fetch some ourselves
         * as buffer underruns must not happen.
         */
        else
        {
            /** @todo We're ending up here to frequently with pulse audio at least (just
             *        watch the stream stats in the statistcs viewer, and way to often we
             *        have to inject silence bytes.  I suspect part of the problem is
             *        that the HDA device require a much better latency than what the
             *        pulse audio is configured for by default (10 ms vs 150ms). */
            STAM_REL_COUNTER_INC(&pStreamR3->State.StatDmaFlowProblems);
            Log(("hdaR3StreamUpdateDma: Warning! Stream #%u has insufficient data available: %u bytes, need %u.  Will try move pull more data into the buffer...\n",
                 pStreamShared->u8SD, cbStreamUsed, cbPeriod));
            int rc = AudioMixerSinkTryLock(pSink);
            if (RT_SUCCESS(rc))
            {
                AudioMixerSinkUpdate(pSink, cbStreamUsed, cbPeriod);
                hdaR3StreamPullFromMixer(pStreamShared, pStreamR3, pSink);
                AudioMixerSinkUnlock(pSink);
            }
            else
                RTThreadYield();
            Log(("hdaR3StreamUpdateDma: Gained %u bytes.\n", hdaR3StreamGetUsed(pStreamR3) - cbStreamUsed));
            cbStreamUsed = hdaR3StreamGetUsed(pStreamR3);
            if (cbStreamUsed < cbPeriod)
            {
                /* Unable to find sufficient input data by simple prodding.
                   In order to keep a constant byte stream following thru the DMA
                   engine into the guest, we will try again and then fall back on
                   filling the gap with silence. */
                uint32_t cbSilence = 0;
                do
                {
                    AudioMixerSinkLock(pSink);

                    cbStreamUsed = hdaR3StreamGetUsed(pStreamR3);
                    if (cbStreamUsed < cbPeriod)
                    {
                        hdaR3StreamPullFromMixer(pStreamShared, pStreamR3, pSink);
                        cbStreamUsed = hdaR3StreamGetUsed(pStreamR3);
                        while (cbStreamUsed < cbPeriod)
                        {
                            void  *pvDstBuf;
                            size_t cbDstBuf;
                            RTCircBufAcquireWriteBlock(pStreamR3->State.pCircBuf, cbPeriod - cbStreamUsed,
                                                       &pvDstBuf, &cbDstBuf);
                            RT_BZERO(pvDstBuf, cbDstBuf);
                            RTCircBufReleaseWriteBlock(pStreamR3->State.pCircBuf, cbDstBuf);
                            cbSilence    += (uint32_t)cbDstBuf;
                            cbStreamUsed += (uint32_t)cbDstBuf;
                        }
                    }

                    AudioMixerSinkUnlock(pSink);
                } while (cbStreamUsed < cbPeriod);
                if (cbSilence > 0)
                {
                    STAM_REL_COUNTER_INC(&pStreamR3->State.StatDmaFlowErrors);
                    STAM_REL_COUNTER_ADD(&pStreamR3->State.StatDmaFlowErrorBytes, cbSilence);
                    LogRel2(("HDA: Warning: Stream #%RU8 underrun, added %u bytes of silence (%u us)\n", pStreamShared->u8SD,
                             cbSilence, PDMAudioPropsBytesToMicro(&pStreamShared->State.Cfg.Props, cbSilence)));
                }
            }
        }

        /*
         * Do the DMA'ing.
         */
        if (cbStreamUsed)
            hdaR3StreamDoDmaInput(pDevIns, pThis, pStreamShared, pStreamR3,
                                  RT_MIN(cbStreamUsed, cbPeriod), fWriteSilence, tsNowNs);

        /*
         * We should always kick the AIO thread.
         */
        /** @todo This isn't entirely ideal.  If we get into an underrun situation,
         *        we ideally want the AIO thread to run right before the DMA timer
         *        rather than right after it ran. */
        Log5Func(("Notifying AIO thread\n"));
        rc2 = AudioMixerSinkSignalUpdateJob(pSink);
        AssertRC(rc2);
        pStreamShared->State.tsLastReadNs = tsNowNs;
    }
}


/**
 * @callback_method_impl{FNAUDMIXSINKUPDATE}
 *
 * For output streams this moves data from the internal DMA buffer (in which
 * hdaR3StreamUpdateDma put it), thru the mixer and to the various backend audio
 * devices.
 *
 * For input streams this pulls data from the backend audio device(s), thru the
 * mixer and puts it in the internal DMA buffer ready for hdaR3StreamUpdateDma
 * to pump into guest memory.
 */
DECLCALLBACK(void) hdaR3StreamUpdateAsyncIoJob(PPDMDEVINS pDevIns, PAUDMIXSINK pSink, void *pvUser)
{
    PHDASTATE const         pThis         = PDMDEVINS_2_DATA(pDevIns, PHDASTATE);
    PHDASTATER3 const       pThisCC       = PDMDEVINS_2_DATA_CC(pDevIns, PHDASTATER3);
    PHDASTREAMR3 const      pStreamR3     = (PHDASTREAMR3)pvUser;
    PHDASTREAM const        pStreamShared = &pThis->aStreams[pStreamR3 - &pThisCC->aStreams[0]];
    Assert(pStreamR3 - &pThisCC->aStreams[0] == pStreamR3->u8SD);
    Assert(pStreamShared->u8SD == pStreamR3->u8SD);
    RT_NOREF(pSink);

    /*
     * Make sure we haven't change sink and that it's still active (it
     * should be or we wouldn't have been called).
     */
    AssertReturnVoid(pStreamR3->pMixSink && pSink == pStreamR3->pMixSink->pMixSink);
    AssertReturnVoid(AudioMixerSinkIsActive(pSink));

    /*
     * Output streams (SDO).
     */
    if (hdaGetDirFromSD(pStreamShared->u8SD) == PDMAUDIODIR_OUT)
        hdaR3StreamPushToMixer(pStreamShared, pStreamR3, pSink, RTTimeNanoTS());
    /*
     * Input stream (SDI).
     */
    else
    {
        Assert(hdaGetDirFromSD(pStreamShared->u8SD) == PDMAUDIODIR_IN);
        hdaR3StreamPullFromMixer(pStreamShared, pStreamR3, pSink);
    }
}


# if 0 /* unused - no prototype even */
/**
 * Updates an HDA stream's current read or write buffer position (depending on the stream type) by
 * updating its associated LPIB register and DMA position buffer (if enabled).
 *
 * @returns Set LPIB value.
 * @param   pDevIns             The device instance.
 * @param   pStream             HDA stream to update read / write position for.
 * @param   u32LPIB             New LPIB (position) value to set.
 */
uint32_t hdaR3StreamUpdateLPIB(PPDMDEVINS pDevIns, PHDASTATE pThis, PHDASTREAM pStreamShared, uint32_t u32LPIB)
{
    AssertMsg(u32LPIB <= pStreamShared->u32CBL,
              ("[SD%RU8] New LPIB (%RU32) exceeds CBL (%RU32)\n", pStreamShared->u8SD, u32LPIB, pStreamShared->u32CBL));

    u32LPIB = RT_MIN(u32LPIB, pStreamShared->u32CBL);

    LogFlowFunc(("[SD%RU8] LPIB=%RU32 (DMA Position Buffer Enabled: %RTbool)\n",
                 pStreamShared->u8SD, u32LPIB, pThis->fDMAPosition));

    /* Update LPIB in any case. */
    HDA_STREAM_REG(pThis, LPIB, pStreamShared->u8SD) = u32LPIB;

    /* Do we need to tell the current DMA position? */
    if (pThis->fDMAPosition)
    {
        int rc2 = PDMDevHlpPCIPhysWrite(pDevIns,
                                        pThis->u64DPBase + (pStreamShared->u8SD * 2 * sizeof(uint32_t)),
                                        (void *)&u32LPIB, sizeof(uint32_t));
        AssertRC(rc2);
    }

    return u32LPIB;
}
# endif

# ifdef HDA_USE_DMA_ACCESS_HANDLER
/**
 * Registers access handlers for a stream's BDLE DMA accesses.
 *
 * @returns true if registration was successful, false if not.
 * @param   pStream             HDA stream to register BDLE access handlers for.
 */
bool hdaR3StreamRegisterDMAHandlers(PHDASTREAM pStream)
{
    /* At least LVI and the BDL base must be set. */
    if (   !pStreamShared->u16LVI
        || !pStreamShared->u64BDLBase)
    {
        return false;
    }

    hdaR3StreamUnregisterDMAHandlers(pStream);

    LogFunc(("Registering ...\n"));

    int rc = VINF_SUCCESS;

    /*
     * Create BDLE ranges.
     */

    struct BDLERANGE
    {
        RTGCPHYS uAddr;
        uint32_t uSize;
    } arrRanges[16]; /** @todo Use a define. */

    size_t cRanges = 0;

    for (uint16_t i = 0; i < pStreamShared->u16LVI + 1; i++)
    {
        HDABDLE BDLE;
        rc = hdaR3BDLEFetch(pDevIns, &BDLE, pStreamShared->u64BDLBase, i /* Index */);
        if (RT_FAILURE(rc))
            break;

        bool fAddRange = true;
        BDLERANGE *pRange;

        if (cRanges)
        {
            pRange = &arrRanges[cRanges - 1];

            /* Is the current range a direct neighbor of the current BLDE? */
            if ((pRange->uAddr + pRange->uSize) == BDLE.Desc.u64BufAddr)
            {
                /* Expand the current range by the current BDLE's size. */
                pRange->uSize += BDLE.Desc.u32BufSize;

                /* Adding a new range in this case is not needed anymore. */
                fAddRange = false;

                LogFunc(("Expanding range %zu by %RU32 (%RU32 total now)\n", cRanges - 1, BDLE.Desc.u32BufSize, pRange->uSize));
            }
        }

        /* Do we need to add a new range? */
        if (   fAddRange
            && cRanges < RT_ELEMENTS(arrRanges))
        {
            pRange = &arrRanges[cRanges];

            pRange->uAddr = BDLE.Desc.u64BufAddr;
            pRange->uSize = BDLE.Desc.u32BufSize;

            LogFunc(("Adding range %zu - 0x%x (%RU32)\n", cRanges, pRange->uAddr, pRange->uSize));

            cRanges++;
        }
    }

    LogFunc(("%zu ranges total\n", cRanges));

    /*
     * Register all ranges as DMA access handlers.
     */

    for (size_t i = 0; i < cRanges; i++)
    {
        BDLERANGE *pRange = &arrRanges[i];

        PHDADMAACCESSHANDLER pHandler = (PHDADMAACCESSHANDLER)RTMemAllocZ(sizeof(HDADMAACCESSHANDLER));
        if (!pHandler)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        RTListAppend(&pStream->State.lstDMAHandlers, &pHandler->Node);

        pHandler->pStream = pStream; /* Save a back reference to the owner. */

        char szDesc[32];
        RTStrPrintf(szDesc, sizeof(szDesc), "HDA[SD%RU8 - RANGE%02zu]", pStream->u8SD, i);

        int rc2 = PGMR3HandlerPhysicalTypeRegister(PDMDevHlpGetVM(pStream->pHDAState->pDevInsR3), PGMPHYSHANDLERKIND_WRITE,
                                                   hdaDMAAccessHandler,
                                                   NULL, NULL, NULL,
                                                   NULL, NULL, NULL,
                                                   szDesc, &pHandler->hAccessHandlerType);
        AssertRCBreak(rc2);

        pHandler->BDLEAddr  = pRange->uAddr;
        pHandler->BDLESize  = pRange->uSize;

        /* Get first and last pages of the BDLE range. */
        RTGCPHYS pgFirst = pRange->uAddr & ~PAGE_OFFSET_MASK;
        RTGCPHYS pgLast  = RT_ALIGN(pgFirst + pRange->uSize, PAGE_SIZE);

        /* Calculate the region size (in pages). */
        RTGCPHYS regionSize = RT_ALIGN(pgLast - pgFirst, PAGE_SIZE);

        pHandler->GCPhysFirst = pgFirst;
        pHandler->GCPhysLast  = pHandler->GCPhysFirst + (regionSize - 1);

        LogFunc(("  Registering region '%s': %#RGp - %#RGp (region size: %#zx)\n",
                 szDesc, pHandler->GCPhysFirst, pHandler->GCPhysLast, regionSize));
        LogFunc(("  BDLE @ %#RGp - %#RGp (%#RX32)\n",
                 pHandler->BDLEAddr, pHandler->BDLEAddr + pHandler->BDLESize, pHandler->BDLESize));

        rc2 = PGMHandlerPhysicalRegister(PDMDevHlpGetVM(pStream->pHDAState->pDevInsR3),
                                         pHandler->GCPhysFirst, pHandler->GCPhysLast,
                                         pHandler->hAccessHandlerType, pHandler, NIL_RTR0PTR, NIL_RTRCPTR,
                                         szDesc);
        AssertRCBreak(rc2);

        pHandler->fRegistered = true;
    }

    LogFunc(("Registration ended with rc=%Rrc\n", rc));

    return RT_SUCCESS(rc);
}

/**
 * Unregisters access handlers of a stream's BDLEs.
 *
 * @param   pStream             HDA stream to unregister BDLE access handlers for.
 */
void hdaR3StreamUnregisterDMAHandlers(PHDASTREAM pStream)
{
    LogFunc(("\n"));

    PHDADMAACCESSHANDLER pHandler, pHandlerNext;
    RTListForEachSafe(&pStream->State.lstDMAHandlers, pHandler, pHandlerNext, HDADMAACCESSHANDLER, Node)
    {
        if (!pHandler->fRegistered) /* Handler not registered? Skip. */
            continue;

        LogFunc(("Unregistering 0x%x - 0x%x (%zu)\n",
                 pHandler->GCPhysFirst, pHandler->GCPhysLast, pHandler->GCPhysLast - pHandler->GCPhysFirst));

        int rc2 = PGMHandlerPhysicalDeregister(PDMDevHlpGetVM(pStream->pHDAState->pDevInsR3),
                                               pHandler->GCPhysFirst);
        AssertRC(rc2);

        RTListNodeRemove(&pHandler->Node);

        RTMemFree(pHandler);
        pHandler = NULL;
    }

    Assert(RTListIsEmpty(&pStream->State.lstDMAHandlers));
}

# endif /* HDA_USE_DMA_ACCESS_HANDLER */

#endif /* IN_RING3 */

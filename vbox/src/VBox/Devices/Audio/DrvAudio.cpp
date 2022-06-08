/* $Id: DrvAudio.cpp 63743 2016-09-07 09:26:22Z vboxsync $ */
/** @file
 * Intermediate audio driver header.
 *
 * @remarks Intermediate audio driver for connecting the audio device emulation
 *          with the host backend.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 * --------------------------------------------------------------------
 */
#define LOG_GROUP LOG_GROUP_DRV_AUDIO
#include <VBox/log.h>
#include <VBox/vmm/pdm.h>
#include <VBox/err.h>
#include <VBox/vmm/mm.h>
#include <VBox/vmm/pdmaudioifs.h>

#include <iprt/alloc.h>
#include <iprt/asm-math.h>
#include <iprt/assert.h>
#include <iprt/circbuf.h>
#include <iprt/string.h>
#include <iprt/uuid.h>

#include "VBoxDD.h"

#include <ctype.h>
#include <stdlib.h>

#include "DrvAudio.h"
#include "AudioMixBuffer.h"

static int drvAudioDevicesEnumerateInternal(PDRVAUDIO pThis, bool fLog, PPDMAUDIODEVICEENUM pDevEnum);

static DECLCALLBACK(int) drvAudioStreamDestroy(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamControlInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd);
static int drvAudioStreamControlInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd);
static int drvAudioStreamCreateInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq);
static int drvAudioStreamDestroyInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream);
static int drvAudioStreamUninitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest);
static int drvAudioStreamIterateInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamReInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream);
static int drvAudioStreamLinkToInternal(PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAM pPair);

#ifndef VBOX_AUDIO_TESTCASE

# if 0 /* unused */

static PDMAUDIOFMT drvAudioGetConfFormat(PCFGMNODE pCfgHandle, const char *pszKey,
                                         PDMAUDIOFMT enmDefault, bool *pfDefault)
{
    if (   pCfgHandle == NULL
        || pszKey == NULL)
    {
        *pfDefault = true;
        return enmDefault;
    }

    char *pszValue = NULL;
    int rc = CFGMR3QueryStringAlloc(pCfgHandle, pszKey, &pszValue);
    if (RT_FAILURE(rc))
    {
        *pfDefault = true;
        return enmDefault;
    }

    PDMAUDIOFMT fmt = DrvAudioHlpStrToAudFmt(pszValue);
    if (fmt == PDMAUDIOFMT_INVALID)
    {
         *pfDefault = true;
        return enmDefault;
    }

    *pfDefault = false;
    return fmt;
}

static int drvAudioGetConfInt(PCFGMNODE pCfgHandle, const char *pszKey,
                              int iDefault, bool *pfDefault)
{

    if (   pCfgHandle == NULL
        || pszKey == NULL)
    {
        *pfDefault = true;
        return iDefault;
    }

    uint64_t u64Data = 0;
    int rc = CFGMR3QueryInteger(pCfgHandle, pszKey, &u64Data);
    if (RT_FAILURE(rc))
    {
        *pfDefault = true;
        return iDefault;

    }

    *pfDefault = false;
    return u64Data;
}

static const char *drvAudioGetConfStr(PCFGMNODE pCfgHandle, const char *pszKey,
                                      const char *pszDefault, bool *pfDefault)
{
    if (   pCfgHandle == NULL
        || pszKey == NULL)
    {
        *pfDefault = true;
        return pszDefault;
    }

    char *pszValue = NULL;
    int rc = CFGMR3QueryStringAlloc(pCfgHandle, pszKey, &pszValue);
    if (RT_FAILURE(rc))
    {
        *pfDefault = true;
        return pszDefault;
    }

    *pfDefault = false;
    return pszValue;
}

# endif /* unused */

/**
 * Returns the host stream part of an audio stream pair, or NULL
 * if no host stream has been assigned / is not available.
 *
 * @returns IPRT status code.
 * @param   pStream             Audio stream to retrieve host stream part for.
 */
DECLINLINE(PPDMAUDIOSTREAM) drvAudioGetHostStream(PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pStream, NULL);

    if (!pStream)
        return NULL;

    PPDMAUDIOSTREAM pHstStream = pStream->enmCtx == PDMAUDIOSTREAMCTX_HOST
                               ? pStream
                               : pStream->pPair;
    if (pHstStream)
    {
        AssertReleaseMsg(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST,
                         ("Stream '%s' resolved as host part is not marked as such (enmCtx=%RU32)\n",
                          pHstStream->szName, pHstStream->enmCtx));

        AssertReleaseMsg(pHstStream->pPair != NULL,
                         ("Stream '%s' resolved as host part has no guest part (anymore)\n", pHstStream->szName));
    }
    else
        AssertReleaseMsgFailed(("Stream '%s' does not have a host pair (anymore)\n",
                                pStream->szName));

    return pHstStream;
}

# if 0 /* unused */
static int drvAudioProcessOptions(PCFGMNODE pCfgHandle, const char *pszPrefix, audio_option *paOpts, size_t cOpts)
{
    AssertPtrReturn(pCfgHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pszPrefix,  VERR_INVALID_POINTER);
    /* oaOpts and cOpts are optional. */

    PCFGMNODE pCfgChildHandle = NULL;
    PCFGMNODE pCfgChildChildHandle = NULL;

   /* If pCfgHandle is NULL, let NULL be passed to get int and get string functions..
    * The getter function will return default values.
    */
    if (pCfgHandle != NULL)
    {
       /* If its audio general setting, need to traverse to one child node.
        * /Devices/ichac97/0/LUN#0/Config/Audio
        */
       if(!strncmp(pszPrefix, "AUDIO", 5)) /** @todo Use a \#define */
       {
            pCfgChildHandle = CFGMR3GetFirstChild(pCfgHandle);
            if(pCfgChildHandle)
                pCfgHandle = pCfgChildHandle;
        }
        else
        {
            /* If its driver specific configuration , then need to traverse two level deep child
             * child nodes. for eg. in case of DirectSoundConfiguration item
             * /Devices/ichac97/0/LUN#0/Config/Audio/DirectSoundConfig
             */
            pCfgChildHandle = CFGMR3GetFirstChild(pCfgHandle);
            if (pCfgChildHandle)
            {
                pCfgChildChildHandle = CFGMR3GetFirstChild(pCfgChildHandle);
                if (pCfgChildChildHandle)
                    pCfgHandle = pCfgChildChildHandle;
            }
        }
    }

    for (size_t i = 0; i < cOpts; i++)
    {
        audio_option *pOpt = &paOpts[i];
        if (!pOpt->valp)
        {
            LogFlowFunc(("Option value pointer for `%s' is not set\n", pOpt->name));
            continue;
        }

        bool fUseDefault;

        switch (pOpt->tag)
        {
            case AUD_OPT_BOOL:
            case AUD_OPT_INT:
            {
                int *intp = (int *)pOpt->valp;
                *intp = drvAudioGetConfInt(pCfgHandle, pOpt->name, *intp, &fUseDefault);

                break;
            }

            case AUD_OPT_FMT:
            {
                PDMAUDIOFMT *fmtp = (PDMAUDIOFMT *)pOpt->valp;
                *fmtp = drvAudioGetConfFormat(pCfgHandle, pOpt->name, *fmtp, &fUseDefault);

                break;
            }

            case AUD_OPT_STR:
            {
                const char **strp = (const char **)pOpt->valp;
                *strp = drvAudioGetConfStr(pCfgHandle, pOpt->name, *strp, &fUseDefault);

                break;
            }

            default:
                LogFlowFunc(("Bad value tag for option `%s' - %d\n", pOpt->name, pOpt->tag));
                fUseDefault = false;
                break;
        }

        if (!pOpt->overridenp)
            pOpt->overridenp = &pOpt->overriden;

        *pOpt->overridenp = !fUseDefault;
    }

    return VINF_SUCCESS;
}
# endif /* unused */
#endif /* !VBOX_AUDIO_TESTCASE */

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamControl}
 */
static DECLCALLBACK(int) drvAudioStreamControl(PPDMIAUDIOCONNECTOR pInterface,
                                               PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);

    if (!pStream)
        return VINF_SUCCESS;

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    LogFlowFunc(("[%s] enmStreamCmd=%RU32\n", pStream->szName, enmStreamCmd));

    rc = drvAudioStreamControlInternal(pThis, pStream, enmStreamCmd);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}

/**
 * Controls an audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to control.
 * @param   enmStreamCmd        Control command.
 */
static int drvAudioStreamControlInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFunc(("[%s] enmStreamCmd=%RU32\n", pStream->szName, enmStreamCmd));

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;
    AssertPtr(pGstStream);

    LogFlowFunc(("Status host=0x%x, guest=0x%x\n", pHstStream->fStatus, pGstStream->fStatus));

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            if (!(pGstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED))
            {
                if (pHstStream)
                {
                    /* Is a pending disable outstanding? Then disable first. */
                    if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE)
                        rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);

                    if (RT_SUCCESS(rc))
                        rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_ENABLE);
                }

                pGstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_ENABLED;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            if (pGstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED)
            {
                /* Is the guest side stream still active?
                 * Mark the host stream as pending disable and bail out. */
                if (pHstStream)
                {
                    LogFunc(("[%s] Pending disable/pause\n", pHstStream->szName));
                    pHstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE;
                }

                if (enmStreamCmd == PDMAUDIOSTREAMCMD_DISABLE)
                {
                    pGstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_ENABLED;
                }
                else if (enmStreamCmd == PDMAUDIOSTREAMCMD_PAUSE)
                    pGstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_PAUSED;
                else
                    AssertFailedBreakStmt(rc = VERR_NOT_IMPLEMENTED);
            }

            if (   pHstStream
                && !(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE))
            {
                rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, enmStreamCmd);
                if (RT_SUCCESS(rc))
                    pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE;
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            if (pGstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PAUSED)
            {
                if (pHstStream)
                    rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_RESUME);

                pGstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PAUSED;
            }
            break;
        }

        default:
            AssertMsgFailed(("Command %RU32 not implemented\n", enmStreamCmd));
            rc = VERR_NOT_IMPLEMENTED;
            break;
    }

    if (RT_FAILURE(rc))
        LogFunc(("[%s] Failed with %Rrc\n", pStream->szName, rc));

    return rc;
}

/**
 * Controls a stream's backend.
 * If the stream has no backend available, VERR_NOT_FOUND is returned.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to control.
 * @param   enmStreamCmd        Control command.
 */
static int drvAudioStreamControlInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream, PDMAUDIOSTREAMCMD enmStreamCmd)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (!pHstStream) /* Stream does not have a host backend? Bail out. */
        return VERR_NOT_FOUND;

    LogFlowFunc(("[%s] enmStreamCmd=%RU32, fStatus=0x%x\n", pHstStream->szName, enmStreamCmd, pHstStream->fStatus));

    AssertPtr(pThis->pHostDrvAudio);

    int rc = VINF_SUCCESS;

    switch (enmStreamCmd)
    {
        case PDMAUDIOSTREAMCMD_ENABLE:
        {
            if (!(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED))
            {
                LogRel2(("Audio: Enabling stream '%s'\n", pHstStream->szName));
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream, PDMAUDIOSTREAMCMD_ENABLE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_ENABLED;
                }
                else
                    LogRel2(("Audio: Disabling stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_DISABLE:
        {
            if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED)
            {
                LogRel2(("Audio: Disabling stream '%s'\n", pHstStream->szName));
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_ENABLED;
                    pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE;
                    AudioMixBufReset(&pHstStream->MixBuf);
                }
                else
                    LogRel2(("Audio: Disabling stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_PAUSE:
        {
            /* Only pause if the stream is enabled. */
            if (!(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED))
                break;

            if (!(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PAUSED))
            {
                LogRel2(("Audio: Pausing stream '%s'\n", pHstStream->szName));
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream, PDMAUDIOSTREAMCMD_PAUSE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_PAUSED;
                }
                else
                    LogRel2(("Audio: Pausing stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
            }
            break;
        }

        case PDMAUDIOSTREAMCMD_RESUME:
        {
            /* Only need to resume if the stream is enabled. */
            if (!(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED))
                break;

            if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PAUSED)
            {
                LogRel2(("Audio: Resuming stream '%s'\n", pHstStream->szName));
                rc = pThis->pHostDrvAudio->pfnStreamControl(pThis->pHostDrvAudio, pHstStream, PDMAUDIOSTREAMCMD_RESUME);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PAUSED;
                }
                else
                    LogRel2(("Audio: Resuming stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
            }
            break;
        }

        default:
        {
            AssertMsgFailed(("Command %RU32 not implemented\n", enmStreamCmd));
            rc = VERR_NOT_IMPLEMENTED;
            break;
        }
    }

    if (RT_FAILURE(rc))
        LogFunc(("[%s] Failed with %Rrc\n", pStream->szName, rc));

    return rc;
}

/**
 * Initializes an audio stream with a given host and guest stream configuration.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to initialize.
 * @param   pCfgHost            Stream configuration to use for the host side (backend).
 * @param   pCfgGuest           Stream configuration to use for the guest side.
 */
static int drvAudioStreamInitInternal(PDRVAUDIO pThis,
                                      PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;
    AssertPtr(pGstStream);

    /*
     * Init host stream.
     */

#ifdef DEBUG
    LogFunc(("[%s] Requested host format:\n", pStream->szName));
    DrvAudioHlpStreamCfgPrint(pCfgHost);
#else
    LogRel2(("Audio: Creating stream '%s'\n", pStream->szName));
    LogRel2(("Audio: Requested %s host format for '%s': %RU32Hz, %s, %RU8 %s\n",
             pCfgGuest->enmDir == PDMAUDIODIR_IN ? "recording" : "playback", pStream->szName,
             pCfgHost->uHz, DrvAudioHlpAudFmtToStr(pCfgHost->enmFormat),
             pCfgHost->cChannels, pCfgHost->cChannels == 0 ? "Channel" : "Channels"));
#endif

    PDMAUDIOSTREAMCFG CfgHostAcq;
    int rc = drvAudioStreamCreateInternalBackend(pThis,pHstStream, pCfgHost, &CfgHostAcq);
    if (RT_FAILURE(rc))
        return rc;

#ifdef DEBUG
    LogFunc(("[%s] Acquired host format:\n",  pStream->szName));
    DrvAudioHlpStreamCfgPrint(&CfgHostAcq);
#else
    LogRel2(("Audio: Acquired %s host format for '%s': %RU32Hz, %s, %RU8 %s\n",
             pCfgGuest->enmDir == PDMAUDIODIR_IN ? "recording" : "playback",  pStream->szName,
             CfgHostAcq.uHz, DrvAudioHlpAudFmtToStr(CfgHostAcq.enmFormat),
             CfgHostAcq.cChannels, CfgHostAcq.cChannels == 0 ? "Channel" : "Channels"));
#endif

    /* No sample buffer size hint given by the backend? Default to some sane value. */
    if (!CfgHostAcq.cSampleBufferSize)
        CfgHostAcq.cSampleBufferSize = _1K; /** @todo Make this configurable? */

    PDMAUDIOPCMPROPS PCMProps;
    int rc2 = DrvAudioHlpStreamCfgToProps(&CfgHostAcq, &PCMProps);
    AssertRC(rc2);

    /* Destroy any former mixing buffer. */
    AudioMixBufDestroy(&pHstStream->MixBuf);

    LogFlowFunc(("[%s] cSamples=%RU32\n", pHstStream->szName, CfgHostAcq.cSampleBufferSize * 4));

    rc2 = AudioMixBufInit(&pHstStream->MixBuf, pHstStream->szName, &PCMProps, CfgHostAcq.cSampleBufferSize * 4);
    AssertRC(rc2);

    /* Make a copy of the host stream configuration. */
    memcpy(&pHstStream->Cfg, pCfgHost, sizeof(PDMAUDIOSTREAMCFG));

    /*
     * Init guest stream.
     */

    RT_ZERO(PCMProps);
    rc2 = DrvAudioHlpStreamCfgToProps(pCfgGuest, &PCMProps);
    AssertRC(rc2);

    /* Destroy any former mixing buffer. */
    AudioMixBufDestroy(&pGstStream->MixBuf);

    LogFlowFunc(("[%s] cSamples=%RU32\n", pGstStream->szName, CfgHostAcq.cSampleBufferSize * 2));

    rc2 = AudioMixBufInit(&pGstStream->MixBuf, pGstStream->szName, &PCMProps, CfgHostAcq.cSampleBufferSize * 2);
    AssertRC(rc2);

    if (pCfgGuest->enmDir == PDMAUDIODIR_IN)
    {
        /* Host (Parent) -> Guest (Child). */
        rc2 = AudioMixBufLinkTo(&pHstStream->MixBuf, &pGstStream->MixBuf);
        AssertRC(rc2);
    }
    else
    {
        /* Guest (Parent) -> Host (Child). */
        rc2 = AudioMixBufLinkTo(&pGstStream->MixBuf, &pHstStream->MixBuf);
        AssertRC(rc2);
    }

    /* Make a copy of the host stream configuration. */
    memcpy(&pGstStream->Cfg, pCfgGuest, sizeof(PDMAUDIOSTREAMCFG));

    if (RT_FAILURE(rc))
        LogRel2(("Audio: Creating stream '%s' failed with %Rrc\n", pStream->szName, rc));

    LogFlowFunc(("[%s] Returning %Rrc\n", pStream->szName, rc));
    return rc;
}

/**
 * Schedules a re-initialization of all current audio streams.
 * The actual re-initialization will happen at some later point in time.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 */
static int drvAudioScheduleReInitInternal(PDRVAUDIO pThis)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFunc(("\n"));

    /* Mark all host streams to re-initialize. */
    PPDMAUDIOSTREAM pHstStream;
    RTListForEach(&pThis->lstHstStreams, pHstStream, PDMAUDIOSTREAM, Node)
        pHstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_PENDING_REINIT;

    /* Re-enumerate all host devices as soon as possible. */
    pThis->fEnumerateDevices = true;

    return VINF_SUCCESS;
}

/**
 * Re-initializes an audio stream with its existing host and guest stream configuration.
 * This might be the case if the backend told us we need to re-initialize because something
 * on the host side has changed.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to re-initialize.
 */
static int drvAudioStreamReInitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFlowFunc(("[%s]\n", pStream->szName));

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    AssertPtr(pHstStream);

    /*
     * Gather current stream status.
     */
    bool fIsEnabled = RT_BOOL(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED); /* Stream is enabled? */

    /*
     * Destroy and re-create stream on backend side.
     */
    int rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
    if (RT_SUCCESS(rc))
    {
        rc = drvAudioStreamDestroyInternalBackend(pThis, pHstStream);
        if (RT_SUCCESS(rc))
        {
            rc = drvAudioStreamCreateInternalBackend(pThis, pHstStream, &pHstStream->Cfg, NULL /* pCfgAcq */);
            /** @todo Validate (re-)acquired configuration with pHstStream->Cfg? */
        }
    }

    /*
     * Restore previous stream state.
     */
    if (RT_SUCCESS(rc))
    {
        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;

        if (fIsEnabled)
        {
            rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_ENABLE);
            if (RT_SUCCESS(rc))
            {
                if (pGstStream)
                {
                    /* Also reset the guest stream mixing buffer. */
                    AudioMixBufReset(&pGstStream->MixBuf);
                }
            }
        }

#ifdef VBOX_WITH_STATISTICS
        /*
         * Reset statistics.
         */
        if (RT_SUCCESS(rc))
        {
            if (pHstStream->enmDir == PDMAUDIODIR_IN)
            {
                STAM_COUNTER_RESET(&pHstStream->In.StatBytesElapsed);
                STAM_COUNTER_RESET(&pHstStream->In.StatBytesTotalRead);
                STAM_COUNTER_RESET(&pHstStream->In.StatSamplesCaptured);

                if (pGstStream)
                {
                    Assert(pGstStream->enmDir == pHstStream->enmDir);

                    STAM_COUNTER_RESET(&pGstStream->In.StatBytesElapsed);
                    STAM_COUNTER_RESET(&pGstStream->In.StatBytesTotalRead);
                    STAM_COUNTER_RESET(&pGstStream->In.StatSamplesCaptured);
                }
            }
            else if (pHstStream->enmDir == PDMAUDIODIR_OUT)
            {
                STAM_COUNTER_RESET(&pHstStream->Out.StatBytesElapsed);
                STAM_COUNTER_RESET(&pHstStream->Out.StatBytesTotalWritten);
                STAM_COUNTER_RESET(&pHstStream->Out.StatSamplesPlayed);

                if (pGstStream)
                {
                    Assert(pGstStream->enmDir == pHstStream->enmDir);

                    STAM_COUNTER_RESET(&pGstStream->Out.StatBytesElapsed);
                    STAM_COUNTER_RESET(&pGstStream->Out.StatBytesTotalWritten);
                    STAM_COUNTER_RESET(&pGstStream->Out.StatSamplesPlayed);
                }
            }
            else
                AssertFailed();
        }
#endif
    }

    if (RT_FAILURE(rc))
        LogRel2(("Audio: Re-initializing stream '%s' failed with %Rrc\n", pStream->szName, rc));

    LogFunc(("[%s] Returning %Rrc\n", pStream->szName, rc));
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamWrite}
 */
static DECLCALLBACK(int) drvAudioStreamWrite(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream,
                                             const void *pvBuf, uint32_t cbBuf, uint32_t *pcbWritten)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pvBuf,      VERR_INVALID_POINTER);
    /* pcbWritten is optional. */

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    if (   !pStream
        || !cbBuf)
    {
        if (pcbWritten)
            *pcbWritten = 0;
        return VINF_SUCCESS;
    }

    AssertMsg(pStream->enmDir == PDMAUDIODIR_OUT,
              ("Stream '%s' is not an output stream and therefore cannot be written to (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cbWritten = 0;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    do
    {
        if (   pThis->pHostDrvAudio
            && pThis->pHostDrvAudio->pfnGetStatus
            && pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, PDMAUDIODIR_OUT) != PDMAUDIOBACKENDSTS_RUNNING)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        if (!pHstStream)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;

        AssertMsg(pGstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED,
                  ("Writing to disabled guest output stream \"%s\" not possible\n", pGstStream->szName));

        pGstStream->Out.tsLastWriteMS = RTTimeMilliTS();

        if (!AudioMixBufFreeBytes(&pGstStream->MixBuf))
        {
            LogRel2(("Audio: Guest stream '%s' full, expect stuttering audio output\n", pGstStream->szName));
            break;
        }

        uint32_t cWritten = 0;
        rc = AudioMixBufWriteCirc(&pGstStream->MixBuf, pvBuf, cbBuf, &cWritten);
        if (rc == VINF_BUFFER_OVERFLOW)
        {
            LogRel2(("Audio: Lost audio samples from guest stream '%s', expect stuttering audio output\n", pGstStream->szName));
            rc = VINF_SUCCESS;
            break;
        }

#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_ADD(&pThis->Stats.TotalBytesWritten, AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cWritten));
        STAM_COUNTER_ADD(&pGstStream->Out.StatBytesTotalWritten, AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cWritten));
#endif
        cbWritten = AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cWritten);

        Log3Func(("[%s] cUsed=%RU32, cLive=%RU32\n",
                  pGstStream->szName, AudioMixBufUsed(&pGstStream->MixBuf), AudioMixBufLive(&pGstStream->MixBuf)));

    } while (0);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
        if (pcbWritten)
            *pcbWritten = cbWritten;
    }

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamRetain}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamRetain(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
   AssertPtrReturn(pInterface, UINT32_MAX);
   AssertPtrReturn(pStream,    UINT32_MAX);

   NOREF(pInterface);

   return ++pStream->cRefs;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamRelease}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamRelease(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
   AssertPtrReturn(pInterface, UINT32_MAX);
   AssertPtrReturn(pStream,    UINT32_MAX);

   NOREF(pInterface);

   if (pStream->cRefs > 1) /* 1 reference always is kept by this audio driver. */
       pStream->cRefs--;

   return pStream->cRefs;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamIterate}
 */
static DECLCALLBACK(int) drvAudioStreamIterate(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    /* pcData is optional. */

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    rc = drvAudioStreamIterateInternal(pThis, pStream);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_FAILURE(rc))
        LogFlowFuncLeaveRC(rc);

    return rc;
}

/**
 * Does one iteration of an audio stream.
 * This function gives the backend the chance of iterating / altering data and
 * does the actual mixing between the guest <-> host mixing buffers.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Stream to iterate.
 *
 * @remark
 */
static int drvAudioStreamIterateInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    if (!pStream)
        return VINF_SUCCESS;

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    AssertPtr(pHstStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
    AssertPtr(pGstStream);

    int rc;

    /* Is the stream scheduled for re-initialization? Do so now. */
    if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PENDING_REINIT)
    {
        if (pThis->fEnumerateDevices)
        {
            /* Re-enumerate all host devices. */
            drvAudioDevicesEnumerateInternal(pThis, true /* fLog */, NULL /* pDevEnum */);

            pThis->fEnumerateDevices = false;
        }

        /* Remove the pending re-init flag in any case, regardless whether the actual re-initialization succeeded
         * or not. If it failed, the backend needs to notify us again to try again at some later point in time. */
        pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PENDING_REINIT;

        rc = drvAudioStreamReInitInternal(pThis, pStream);
        if (RT_FAILURE(rc))
            return rc;
    }

    /* Whether to try closing a pending to close stream. */
    bool fTryClosePending = false;

    do
    {
        uint32_t cSamplesMixed = 0;

        rc = pThis->pHostDrvAudio->pfnStreamIterate(pThis->pHostDrvAudio, pHstStream);
        if (RT_FAILURE(rc))
            break;

        if (pHstStream->enmDir == PDMAUDIODIR_IN)
        {
            /* Has the host captured any samples which were not mixed to the guest side yet? */
            uint32_t cSamplesCaptured = AudioMixBufUsed(&pHstStream->MixBuf);

            Log3Func(("[%s] %RU32 samples captured\n", pHstStream->szName, cSamplesCaptured));

            if (cSamplesCaptured)
            {
                /* When capturing samples, the guest is the parent while the host is the child.
                 * So try mixing not yet mixed host-side samples to the guest-side buffer. */
                rc = AudioMixBufMixToParent(&pHstStream->MixBuf, cSamplesCaptured, &cSamplesMixed);
                if (   RT_SUCCESS(rc)
                    && cSamplesMixed)
                {
                    Log3Func(("[%s] %RU32 captured samples mixed\n", pHstStream->szName, cSamplesMixed));
                }
            }
            else
            {
                fTryClosePending = true;
            }
        }
        else if (pHstStream->enmDir == PDMAUDIODIR_OUT)
        {
            /* When playing samples, the host is the parent while the guest is the child.
             * So try mixing not yet mixed guest-side samples to the host-side buffer. */
            rc = AudioMixBufMixToParent(&pGstStream->MixBuf, AudioMixBufUsed(&pGstStream->MixBuf), &cSamplesMixed);
            if (   RT_SUCCESS(rc)
                && cSamplesMixed)
            {
                Log3Func(("[%s] %RU32 samples mixed, guest has %RU32 samples left (%RU32 live)\n",
                          pHstStream->szName, cSamplesMixed,
                          AudioMixBufUsed(&pGstStream->MixBuf), AudioMixBufLive(&pGstStream->MixBuf)));
            }

            uint32_t cSamplesLeft = AudioMixBufUsed(&pGstStream->MixBuf);
            if (!cSamplesLeft) /* No samples (anymore)? */
            {
                fTryClosePending = true;
            }
        }
        else
            AssertFailedStmt(rc = VERR_NOT_IMPLEMENTED);

        if (fTryClosePending)
        {
            /* Has the host stream marked as disabled but there still were guest streams relying
             * on it? Check if the stream now can be closed and do so, if possible. */
            if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE)
            {
                LogFunc(("[%s] Closing pending stream\n", pHstStream->szName));
                rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE;
                }
                else
                    LogFunc(("%s: Backend vetoed against closing output stream, rc=%Rrc\n", pHstStream->szName, rc));
            }
        }

    } while (0);

    /* Update timestamps. */
    pHstStream->tsLastIterateMS = RTTimeMilliTS();
    pGstStream->tsLastIterateMS = RTTimeMilliTS();

    if (RT_FAILURE(rc))
        LogFunc(("Failed with %Rrc\n", rc));

    return rc;
}

/**
 * Links an audio stream to another audio stream (pair).
 *
 * @returns IPRT status code.
 * @param   pStream             Stream to handle linking for.
 * @param   pPair               Stream to link pStream to. Specify NULL to unlink pStream from a former linked stream.
 */
static int drvAudioStreamLinkToInternal(PPDMAUDIOSTREAM pStream, PPDMAUDIOSTREAM pPair)
{
    if (pPair) /* Link. */
    {
        pStream->pPair = pPair;
        pPair->pPair   = pStream;

        LogRel2(("Linked audio stream '%s' to '%s'\n", pStream->szName, pPair->szName));
    }
    else /* Unlink. */
    {
        if (pStream->pPair)
        {
            LogRel2(("Unlinked pair '%s' from stream '%s'\n", pStream->pPair->szName, pStream->szName));

            AssertMsg(pStream->pPair->pPair == pStream,
                      ("Pair '%s' is not linked to '%s' (linked to '%s')\n",
                       pStream->pPair->szName, pStream->szName, pStream->pPair->pPair ? pStream->pPair->pPair->szName : "<NONE>"));

            pStream->pPair->pPair = NULL; /* Also make sure to unlink the pair from pStream */
            pStream->pPair = NULL;
        }
    }

    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamPlay}
 */
static DECLCALLBACK(int) drvAudioStreamPlay(PPDMIAUDIOCONNECTOR pInterface,
                                            PPDMAUDIOSTREAM pStream, uint32_t *pcSamplesPlayed)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    /* pcSamplesPlayed is optional. */

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    AssertMsg(pStream->enmDir == PDMAUDIODIR_OUT,
              ("Stream '%s' is not an output stream and therefore cannot be played back (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cSamplesPlayed = 0;

    do
    {
        if (!pThis->pHostDrvAudio)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        AssertPtr(pHstStream);
        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
        AssertPtr(pGstStream);

        AssertReleaseMsgBreakStmt(pHstStream != NULL,
                                  ("%s: Host stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);
        AssertReleaseMsgBreakStmt(pGstStream != NULL,
                                  ("%s: Guest stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);

        AssertPtr(pThis->pHostDrvAudio->pfnStreamGetStatus);
        PDMAUDIOSTRMSTS strmSts = pThis->pHostDrvAudio->pfnStreamGetStatus(pThis->pHostDrvAudio, pHstStream);

        uint32_t cSamplesLive = AudioMixBufLive(&pGstStream->MixBuf);
        if (cSamplesLive)
        {
            if (   (strmSts & PDMAUDIOSTRMSTS_FLAG_INITIALIZED)
                && (strmSts & PDMAUDIOSTRMSTS_FLAG_DATA_WRITABLE))
            {
                AssertPtr(pThis->pHostDrvAudio->pfnStreamPlay);
                rc = pThis->pHostDrvAudio->pfnStreamPlay(pThis->pHostDrvAudio, pHstStream, NULL /* pvBuf */, 0 /* cbBuf */,
                                                         &cSamplesPlayed);
                if (RT_FAILURE(rc))
                {
                    int rc2 = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                    AssertRC(rc2);
                }
                else
                {
#ifdef VBOX_WITH_STATISTICS
                    STAM_COUNTER_ADD(&pThis->Stats.TotalSamplesPlayed, cSamplesPlayed);
                    STAM_COUNTER_ADD(&pHstStream->Out.StatSamplesPlayed, cSamplesPlayed);
#endif
                    cSamplesLive = AudioMixBufLive(&pGstStream->MixBuf);
                }
            }
        }

        if (!cSamplesLive)
        {
            /* Has the host stream marked as disabled but there still were guest streams relying
             * on it? Check if the stream now can be closed and do so, if possible. */
            if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE)
            {
                LogFunc(("[%s] Closing pending stream\n", pHstStream->szName));
                rc = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                if (RT_SUCCESS(rc))
                {
                    pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_PENDING_DISABLE;
                }
                else
                    LogFunc(("[%s] Backend vetoed against closing output stream, rc=%Rrc\n", pHstStream->szName, rc));
            }
        }

    } while (0);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
        if (pcSamplesPlayed)
            *pcSamplesPlayed = cSamplesPlayed;
    }

    if (RT_FAILURE(rc))
        LogFlowFunc(("[%s] Failed with %Rrc\n", pStream->szName, rc));

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamCapture}
 */
static DECLCALLBACK(int) drvAudioStreamCapture(PPDMIAUDIOCONNECTOR pInterface,
                                               PPDMAUDIOSTREAM pStream, uint32_t *pcSamplesCaptured)
{
    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    AssertMsg(pStream->enmDir == PDMAUDIODIR_IN,
              ("Stream '%s' is not an input stream and therefore cannot be captured (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    Log3Func(("[%s]\n", pStream->szName));

    uint32_t cSamplesCaptured = 0;

    do
    {
        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        AssertPtr(pHstStream);
        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
        AssertPtr(pGstStream);

        AssertReleaseMsgBreakStmt(pHstStream != NULL,
                                  ("%s: Host stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);
        AssertReleaseMsgBreakStmt(pGstStream != NULL,
                                  ("%s: Guest stream is NULL (cRefs=%RU32, fStatus=%x, enmCtx=%ld)\n",
                                   pStream->szName, pStream->cRefs, pStream->fStatus, pStream->enmCtx),
                                  rc = VERR_NOT_AVAILABLE);

        AssertPtr(pThis->pHostDrvAudio->pfnStreamGetStatus);
        PDMAUDIOSTRMSTS strmSts = pThis->pHostDrvAudio->pfnStreamGetStatus(pThis->pHostDrvAudio, pHstStream);

        uint32_t cSamplesLive = AudioMixBufLive(&pGstStream->MixBuf);
        if (!cSamplesLive)
        {
            if (   (strmSts & PDMAUDIOSTRMSTS_FLAG_INITIALIZED)
                && (strmSts & PDMAUDIOSTRMSTS_FLAG_DATA_READABLE))
            {
                rc = pThis->pHostDrvAudio->pfnStreamCapture(pThis->pHostDrvAudio, pHstStream, NULL /* pvBuf */, 0 /* cbBuf */,
                                                            &cSamplesCaptured);
                if (RT_FAILURE(rc))
                {
                    int rc2 = drvAudioStreamControlInternalBackend(pThis, pHstStream, PDMAUDIOSTREAMCMD_DISABLE);
                    AssertRC(rc2);
                }
                else
                {
#ifdef VBOX_WITH_STATISTICS
                    STAM_COUNTER_ADD(&pHstStream->In.StatSamplesCaptured, cSamplesCaptured);
#endif
                }
            }

            Log3Func(("[%s] strmSts=0x%x, cSamplesCaptured=%RU32, rc=%Rrc\n", pHstStream->szName, strmSts, cSamplesCaptured, rc));
        }

    } while (0);

    if (RT_SUCCESS(rc))
    {
        if (pcSamplesCaptured)
            *pcSamplesCaptured = cSamplesCaptured;
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_FAILURE(rc))
        LogFlowFuncLeaveRC(rc);

    return rc;
}

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
/**
 * Duplicates an audio callback.
 *
 * @returns Pointer to duplicated callback, or NULL on failure.
 * @param   pCB                 Callback to duplicate.
 */
static PPDMAUDIOCALLBACK drvAudioCallbackDuplicate(PPDMAUDIOCALLBACK pCB)
{
    AssertPtrReturn(pCB, NULL);

    PPDMAUDIOCALLBACK pCBCopy = (PPDMAUDIOCALLBACK)RTMemDup((void *)pCB, sizeof(PDMAUDIOCALLBACK));
    if (!pCBCopy)
        return NULL;

    if (pCB->pvCtx)
    {
        pCBCopy->pvCtx = RTMemDup(pCB->pvCtx, pCB->cbCtx);
        if (!pCBCopy->pvCtx)
        {
            RTMemFree(pCBCopy);
            return NULL;
        }

        pCBCopy->cbCtx = pCB->cbCtx;
    }

    return pCBCopy;
}

/**
 * Destroys a given callback.
 *
 * @param   pCB                 Callback to destroy.
 */
static void drvAudioCallbackDestroy(PPDMAUDIOCALLBACK pCB)
{
    if (!pCB)
        return;

    RTListNodeRemove(&pCB->Node);
    if (pCB->pvCtx)
    {
        Assert(pCB->cbCtx);
        RTMemFree(pCB->pvCtx);
    }
    RTMemFree(pCB);
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnRegisterCallbacks}
 */
static DECLCALLBACK(int) drvAudioRegisterCallbacks(PPDMIAUDIOCONNECTOR pInterface,
                                                   PPDMAUDIOCALLBACK paCallbacks, size_t cCallbacks)
{
    AssertPtrReturn(pInterface,  VERR_INVALID_POINTER);
    AssertPtrReturn(paCallbacks, VERR_INVALID_POINTER);
    AssertReturn(cCallbacks,     VERR_INVALID_PARAMETER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    for (size_t i = 0; i < cCallbacks; i++)
    {
        PPDMAUDIOCALLBACK pCB = drvAudioCallbackDuplicate(&paCallbacks[i]);
        if (!pCB)
        {
            rc = VERR_NO_MEMORY;
            break;
        }

        switch (pCB->enmType)
        {
            case PDMAUDIOCBTYPE_DATA_INPUT:
                RTListAppend(&pThis->lstCBIn, &pCB->Node);
                break;

            case PDMAUDIOCBTYPE_DATA_OUTPUT:
                RTListAppend(&pThis->lstCBOut, &pCB->Node);
                break;

            default:
                AssertMsgFailed(("Not supported\n"));
                break;
        }
    }

    /** @todo Undo allocations on error. */

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnCallback}
 */
static DECLCALLBACK(int) drvAudioCallback(PPDMIAUDIOCONNECTOR pInterface, PDMAUDIOCBTYPE enmType,
                                          void *pvUser, size_t cbUser)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pvUser,     VERR_INVALID_POINTER);
    AssertReturn(cbUser,        VERR_INVALID_PARAMETER);

    PDRVAUDIO     pThis       = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);
    PRTLISTANCHOR pListAnchor = NULL;

    switch (enmType)
    {
        case PDMAUDIOCBTYPE_DATA_INPUT:
            pListAnchor = &pThis->lstCBIn;
            break;

        case PDMAUDIOCBTYPE_DATA_OUTPUT:
            pListAnchor = &pThis->lstCBOut;
            break;

        default:
            AssertMsgFailed(("Not supported\n"));
            break;
    }

    if (pListAnchor)
    {
        PPDMAUDIOCALLBACK pCB;
        RTListForEach(pListAnchor, pCB, PDMAUDIOCALLBACK, Node)
        {
            Assert(pCB->enmType == enmType);
            int rc2 = pCB->pfnCallback(enmType, pCB->pvCtx, pCB->cbCtx, pvUser, cbUser);
            if (RT_FAILURE(rc2))
                LogFunc(("Failed with %Rrc\n", rc2));
        }

        return VINF_SUCCESS;
    }

    return VERR_NOT_SUPPORTED;
}
#endif /* VBOX_WITH_AUDIO_DEVICE_CALLBACKS */

#ifdef VBOX_WITH_AUDIO_CALLBACKS
/**
 * Backend callback implementation.
 *
 * Important: No calls back to the backend within this function, as the backend
 *            might hold any locks / critical sections while executing this callback.
 *            Will result in some ugly deadlocks (or at least locking order violations) then.
 *
 * @copydoc FNPDMHOSTAUDIOCALLBACK
 */
static DECLCALLBACK(int) drvAudioBackendCallback(PPDMDRVINS pDrvIns,
                                                 PDMAUDIOCBTYPE enmType, void *pvUser, size_t cbUser)
{
    AssertPtrReturn(pDrvIns, VERR_INVALID_POINTER);
    RT_NOREF(pvUser, cbUser);
    /* pvUser and cbUser are optional. */

    /* Get the upper driver (PDMIAUDIOCONNECTOR). */
    AssertPtr(pDrvIns->pUpBase);
    PPDMIAUDIOCONNECTOR pInterface = PDMIBASE_QUERY_INTERFACE(pDrvIns->pUpBase, PDMIAUDIOCONNECTOR);
    AssertPtr(pInterface);
    PDRVAUDIO           pThis      = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    AssertRCReturn(rc, rc);

    LogFunc(("pThis=%p, enmType=%RU32, pvUser=%p, cbUser=%zu\n", pThis, enmType, pvUser, cbUser));

    switch (enmType)
    {
        case PDMAUDIOCBTYPE_DEVICES_CHANGED:
            LogRel(("Audio: Host audio device configuration has changed\n"));
            rc = drvAudioScheduleReInitInternal(pThis);
            break;

        default:
            AssertMsgFailed(("Not supported\n"));
            break;
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}
#endif /* VBOX_WITH_AUDIO_CALLBACKS */

/**
 * Enumerates all host audio devices.
 * This functionality might not be implemented by all backends and will return VERR_NOT_SUPPORTED
 * if not being supported.
 *
 * @returns IPRT status code.
 * @param   pThis               Driver instance to be called.
 * @param   fLog                Whether to print the enumerated device to the release log or not.
 * @param   pDevEnum            Where to store the device enumeration.
 */
static int drvAudioDevicesEnumerateInternal(PDRVAUDIO pThis, bool fLog, PPDMAUDIODEVICEENUM pDevEnum)
{
    int rc;

    /*
     * If the backend supports it, do a device enumeration.
     */
    if (pThis->pHostDrvAudio->pfnGetDevices)
    {
        PDMAUDIODEVICEENUM DevEnum;
        rc = pThis->pHostDrvAudio->pfnGetDevices(pThis->pHostDrvAudio, &DevEnum);
        if (RT_SUCCESS(rc))
        {
            if (fLog)
                LogRel(("Audio: Found %RU16 devices\n", DevEnum.cDevices));

            PPDMAUDIODEVICE pDev;
            RTListForEach(&DevEnum.lstDevices, pDev, PDMAUDIODEVICE, Node)
            {
                if (fLog)
                {
                    char *pszFlags = DrvAudioHlpAudDevFlagsToStrA(pDev->fFlags);

                    LogRel(("Audio: Device '%s':\n", pDev->szName));
                    LogRel(("Audio: \tUsage           = %s\n",   DrvAudioHlpAudDirToStr(pDev->enmUsage)));
                    LogRel(("Audio: \tFlags           = %s\n",   pszFlags ? pszFlags : "<NONE>"));
                    LogRel(("Audio: \tInput channels  = %RU8\n", pDev->cMaxInputChannels));
                    LogRel(("Audio: \tOutput channels = %RU8\n", pDev->cMaxOutputChannels));

                    if (pszFlags)
                        RTStrFree(pszFlags);
                }
            }

            if (pDevEnum)
                rc = DrvAudioHlpDeviceEnumCopy(pDevEnum, &DevEnum);

            DrvAudioHlpDeviceEnumFree(&DevEnum);
        }
        else
        {
            if (fLog)
                LogRel(("Audio: Device enumeration failed with %Rrc\n", rc));
            /* Not fatal. */
        }
    }
    else
    {
        rc = VERR_NOT_SUPPORTED;

        if (fLog)
            LogRel3(("Audio: Host audio backend does not support audio device enumeration, skipping\n"));
    }

    LogFunc(("Returning %Rrc\n", rc));
    return rc;
}

/**
 * Initializes the host backend and queries its initial configuration.
 * If the host backend fails, VERR_AUDIO_BACKEND_INIT_FAILED will be returned.
 *
 * Note: As this routine is called when attaching to the device LUN in the
 *       device emulation, we either check for success or VERR_AUDIO_BACKEND_INIT_FAILED.
 *       Everything else is considered as fatal and must be handled separately in
 *       the device emulation!
 *
 * @return  IPRT status code.
 * @param   pThis               Driver instance to be called.
 * @param   pCfgHandle          CFGM configuration handle to use for this driver.
 */
static int drvAudioHostInit(PDRVAUDIO pThis, PCFGMNODE pCfgHandle)
{
    /* pCfgHandle is optional. */
    NOREF(pCfgHandle);
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    LogFlowFuncEnter();

    AssertPtr(pThis->pHostDrvAudio);
    int rc = pThis->pHostDrvAudio->pfnInit(pThis->pHostDrvAudio);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Initialization of host backend failed with %Rrc\n", rc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }

    /*
     * Get the backend configuration.
     */
    rc = pThis->pHostDrvAudio->pfnGetConfig(pThis->pHostDrvAudio, &pThis->BackendCfg);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Getting host backend configuration failed with %Rrc\n", rc));
        return VERR_AUDIO_BACKEND_INIT_FAILED;
    }

    pThis->cStreamsFreeIn  = pThis->BackendCfg.cMaxStreamsIn;
    pThis->cStreamsFreeOut = pThis->BackendCfg.cMaxStreamsOut;

    LogFlowFunc(("cStreamsFreeIn=%RU8, cStreamsFreeOut=%RU8\n", pThis->cStreamsFreeIn, pThis->cStreamsFreeOut));

    LogRel2(("Audio: Host audio backend supports %RU32 input streams and %RU32 output streams at once\n",
             /* Clamp for logging. Unlimited streams are defined by UINT32_MAX. */
             RT_MIN(64, pThis->cStreamsFreeIn), RT_MIN(64, pThis->cStreamsFreeOut)));

    int rc2 = drvAudioDevicesEnumerateInternal(pThis, true /* fLog */, NULL /* pDevEnum */);
    /* Ignore rc. */

#ifdef VBOX_WITH_AUDIO_CALLBACKS
    /*
     * If the backend supports it, offer a callback to this connector.
     */
    if (pThis->pHostDrvAudio->pfnSetCallback)
    {
        rc2 = pThis->pHostDrvAudio->pfnSetCallback(pThis->pHostDrvAudio, drvAudioBackendCallback);
        if (RT_FAILURE(rc2))
             LogRel(("Audio: Error registering backend callback, rc=%Rrc\n", rc2));
        /* Not fatal. */
    }
#endif

    LogFlowFuncLeave();
    return VINF_SUCCESS;
}

/**
 * Handles state changes for all audio streams.
 *
 * @param   pDrvIns             Pointer to driver instance.
 * @param   enmCmd              Stream command to set for all streams.
 */
static void drvAudioStateHandler(PPDMDRVINS pDrvIns, PDMAUDIOSTREAMCMD enmCmd)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    LogFlowFunc(("enmCmd=%RU32\n", enmCmd));

    if (!pThis->pHostDrvAudio)
        return;

    PPDMAUDIOSTREAM pHstStream;
    RTListForEach(&pThis->lstHstStreams, pHstStream, PDMAUDIOSTREAM, Node)
        drvAudioStreamControlInternalBackend(pThis, pHstStream, enmCmd);
}

/**
 * Intializes an audio driver instance.
 *
 * @returns IPRT status code.
 * @param   pDrvIns             Pointer to driver instance.
 * @param   pCfgHandle          CFGM handle to use for configuration.
 */
static int drvAudioInit(PPDMDRVINS pDrvIns, PCFGMNODE pCfgHandle)
{
    AssertPtrReturn(pCfgHandle, VERR_INVALID_POINTER);
    AssertPtrReturn(pDrvIns,    VERR_INVALID_POINTER);

    LogRel2(("Audio: Verbose logging enabled\n"));

    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);
    LogFlowFunc(("pThis=%p, pDrvIns=%p\n", pThis, pDrvIns));

    int rc = RTCritSectInit(&pThis->CritSect);
    AssertRCReturn(rc, rc);

    /** @todo Add audio driver options. */

    /*
     * If everything went well, initialize the lower driver.
     */
    rc = drvAudioHostInit(pThis, pCfgHandle);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamRead}
 */
static DECLCALLBACK(int) drvAudioStreamRead(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream,
                                            void *pvBuf, uint32_t cbBuf, uint32_t *pcbRead)
{
    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);
    AssertPtrReturn(pThis, VERR_INVALID_POINTER);

    if (!pStream)
    {
        if (pcbRead)
            *pcbRead = 0;
        return VINF_SUCCESS;
    }

    AssertPtrReturn(pvBuf, VERR_INVALID_POINTER);
    AssertReturn(cbBuf,    VERR_INVALID_PARAMETER);
    /* pcbWritten is optional. */

    AssertMsg(pStream->enmDir == PDMAUDIODIR_IN,
              ("Stream '%s' is not an input stream and therefore cannot be read from (direction is 0x%x)\n",
               pStream->szName, pStream->enmDir));

    uint32_t cbRead = 0;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    do
    {
        if (   pThis->pHostDrvAudio
            && pThis->pHostDrvAudio->pfnGetStatus
            && pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, PDMAUDIODIR_IN) != PDMAUDIOBACKENDSTS_RUNNING)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        if (!pHstStream)
        {
            rc = VERR_NOT_AVAILABLE;
            break;
        }

        PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;

        AssertMsg(pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_ENABLED,
                  ("Reading from disabled host input stream '%s' not possible\n", pHstStream->szName));

        pGstStream->In.tsLastReadMS = RTTimeMilliTS();

        Log3Func(("%s\n", pStream->szName));

        /*
         * Read from the parent buffer (that is, the guest buffer) which
         * should have the audio data in the format the guest needs.
         */
        uint32_t cRead;
        rc = AudioMixBufReadCirc(&pGstStream->MixBuf, pvBuf, cbBuf, &cRead);
        if (RT_SUCCESS(rc))
        {
            if (cRead)
            {
#ifdef VBOX_WITH_STATISTICS
                STAM_COUNTER_ADD(&pThis->Stats.TotalBytesRead, AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cRead));
                STAM_COUNTER_ADD(&pGstStream->In.StatBytesTotalRead, AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cRead));
#endif
                AudioMixBufFinish(&pGstStream->MixBuf, cRead);

                cbRead = AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cRead);
            }
        }

        Log3Func(("cRead=%RU32 (%RU32 bytes), rc=%Rrc\n", cRead, AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cRead), rc));

    } while (0);

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    if (RT_SUCCESS(rc))
    {
        if (pcbRead)
            *pcbRead = cbRead;
    }

    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamCreate}
 */
static DECLCALLBACK(int) drvAudioStreamCreate(PPDMIAUDIOCONNECTOR pInterface,
                                              PPDMAUDIOSTREAMCFG pCfgHost, PPDMAUDIOSTREAMCFG pCfgGuest,
                                              PPDMAUDIOSTREAM *ppStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgHost,   VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgGuest,  VERR_INVALID_POINTER);
    AssertPtrReturn(ppStream,   VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    LogFlowFunc(("Host=%s, Guest=%s\n", pCfgHost->szName, pCfgGuest->szName));
#ifdef DEBUG
    DrvAudioHlpStreamCfgPrint(pCfgHost);
    DrvAudioHlpStreamCfgPrint(pCfgGuest);
#endif

    /*
     * The guest stream always will get the audio stream configuration told
     * by the device emulation (which in turn was/could be set by the guest OS).
     */
    PPDMAUDIOSTREAM pGstStrm = NULL;

    /** @todo Docs! */
    PPDMAUDIOSTREAM pHstStrm = NULL;

#define RC_BREAK(x) { rc = x; break; }

    do
    {
        if (   !DrvAudioHlpStreamCfgIsValid(pCfgHost)
            || !DrvAudioHlpStreamCfgIsValid(pCfgGuest))
        {
            RC_BREAK(VERR_INVALID_PARAMETER);
        }

        /* Make sure that both configurations actually intend the same thing. */
        if (pCfgHost->enmDir != pCfgGuest->enmDir)
        {
            AssertMsgFailed(("Stream configuration directions do not match\n"));
            RC_BREAK(VERR_INVALID_PARAMETER);
        }

        /* Note: cbHstStrm will contain sizeof(PDMAUDIOSTREAM) + additional data
         *       which the host backend will need. */
        size_t cbHstStrm;
        if (pCfgHost->enmDir == PDMAUDIODIR_IN)
        {
            if (!pThis->cStreamsFreeIn)
                LogFunc(("Warning: No more input streams free to use\n"));

            /* Validate backend configuration. */
            if (!pThis->BackendCfg.cbStreamIn)
            {
                LogFunc(("Backend input configuration not valid, bailing out\n"));
                RC_BREAK(VERR_INVALID_PARAMETER);
            }

            cbHstStrm = pThis->BackendCfg.cbStreamIn;
        }
        else /* Out */
        {
            if (!pThis->cStreamsFreeOut)
            {
                LogFlowFunc(("Maximum number of host output streams reached\n"));
                RC_BREAK(VERR_AUDIO_NO_FREE_OUTPUT_STREAMS);
            }

            /* Validate backend configuration. */
            if (!pThis->BackendCfg.cbStreamOut)
            {
                LogFlowFunc(("Backend output configuration invalid, bailing out\n"));
                RC_BREAK(VERR_INVALID_PARAMETER);
            }

            cbHstStrm = pThis->BackendCfg.cbStreamOut;
        }

        pHstStrm = (PPDMAUDIOSTREAM)RTMemAllocZ(cbHstStrm);
        AssertPtrBreakStmt(pHstStrm, rc = VERR_NO_MEMORY);

        pHstStrm->enmCtx = PDMAUDIOSTREAMCTX_HOST;
        pHstStrm->enmDir = pCfgHost->enmDir;

        pGstStrm = (PPDMAUDIOSTREAM)RTMemAllocZ(sizeof(PDMAUDIOSTREAM));
        AssertPtrBreakStmt(pGstStrm, rc = VERR_NO_MEMORY);

        pGstStrm->enmCtx = PDMAUDIOSTREAMCTX_GUEST;
        pGstStrm->enmDir = pCfgGuest->enmDir;

        /*
         * Init host stream.
         */
        RTStrPrintf(pHstStrm->szName, RT_ELEMENTS(pHstStrm->szName), "%s (Host)",
                    strlen(pCfgHost->szName) ? pCfgHost->szName : "<Untitled>");

        rc = drvAudioStreamLinkToInternal(pHstStrm, pGstStrm);
        AssertRCBreak(rc);

        /*
         * Init guest stream.
         */
        RTStrPrintf(pGstStrm->szName, RT_ELEMENTS(pGstStrm->szName), "%s (Guest)",
                    strlen(pCfgGuest->szName) ? pCfgGuest->szName : "<Untitled>");

        pGstStrm->fStatus = pHstStrm->fStatus; /* Reflect the host stream's status. */

        rc = drvAudioStreamLinkToInternal(pGstStrm, pHstStrm);
        AssertRCBreak(rc);

        /*
         * Try to init the rest.
         */
        rc = drvAudioStreamInitInternal(pThis, pHstStrm, pCfgHost, pCfgGuest);
        if (RT_FAILURE(rc))
        {
            LogFlowFunc(("Stream not available (yet)\n"));
            rc = VINF_SUCCESS;
        }

#ifdef VBOX_WITH_STATISTICS
        char szStatName[255];

        if (pCfgGuest->enmDir == PDMAUDIODIR_IN)
        {
            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesElapsed", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->In.StatBytesElapsed,
                                      szStatName, STAMUNIT_BYTES, "Elapsed bytes read.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesRead", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->In.StatBytesTotalRead,
                                      szStatName, STAMUNIT_BYTES, "Total bytes read.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Host/%s/SamplesCaptured", pHstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pHstStrm->In.StatSamplesCaptured,
                                      szStatName, STAMUNIT_COUNT, "Total samples captured.");
        }
        else if (pCfgGuest->enmDir == PDMAUDIODIR_OUT)
        {
            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesElapsed", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->Out.StatBytesElapsed,
                                      szStatName, STAMUNIT_BYTES, "Elapsed bytes written.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Guest/%s/BytesRead", pGstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pGstStrm->Out.StatBytesTotalWritten,
                                      szStatName, STAMUNIT_BYTES, "Total bytes written.");

            RTStrPrintf(szStatName, sizeof(szStatName), "Host/%s/SamplesPlayed", pHstStrm->szName);
            PDMDrvHlpSTAMRegCounterEx(pThis->pDrvIns, &pHstStrm->Out.StatSamplesPlayed,
                                      szStatName, STAMUNIT_COUNT, "Total samples played.");
        }
#endif

    } while (0);

#undef RC_BREAK

    if (RT_FAILURE(rc))
    {
        if (pGstStrm)
        {
            int rc2 = drvAudioStreamUninitInternal(pThis, pGstStrm);
            if (RT_SUCCESS(rc2))
            {
                RTMemFree(pGstStrm);
                pGstStrm = NULL;
            }
        }

        if (pHstStrm)
        {
            int rc2 = drvAudioStreamUninitInternal(pThis, pHstStrm);
            if (RT_SUCCESS(rc2))
            {
                RTMemFree(pHstStrm);
                pHstStrm = NULL;
            }
        }
    }
    else
    {
        /* Set initial reference counts. */
        RTListAppend(&pThis->lstGstStreams, &pGstStrm->Node);
        pGstStrm->cRefs = 1;

        RTListAppend(&pThis->lstHstStreams, &pHstStrm->Node);
        pHstStrm->cRefs = 1;

        if (pCfgHost->enmDir == PDMAUDIODIR_IN)
        {
            if (pThis->cStreamsFreeIn)
                pThis->cStreamsFreeIn--;
        }
        else /* Out */
        {
            if (pThis->cStreamsFreeOut)
                pThis->cStreamsFreeOut--;
        }

#ifdef VBOX_WITH_STATISTICS
        STAM_COUNTER_ADD(&pThis->Stats.TotalStreamsCreated, 1);
#endif
        /* Always return the guest-side part to the device emulation. */
        *ppStream = pGstStrm;
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnGetConfig}
 */
static DECLCALLBACK(int) drvAudioGetConfig(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOBACKENDCFG pCfg)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfg,       VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_FAILURE(rc))
        return rc;

    if (pThis->pHostDrvAudio)
    {
        if (pThis->pHostDrvAudio->pfnGetConfig)
            rc = pThis->pHostDrvAudio->pfnGetConfig(pThis->pHostDrvAudio, pCfg);
        else
            rc = VERR_NOT_SUPPORTED;
    }
    else
        AssertFailed();

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnGetStatus}
 */
static DECLCALLBACK(PDMAUDIOBACKENDSTS) drvAudioGetStatus(PPDMIAUDIOCONNECTOR pInterface, PDMAUDIODIR enmDir)
{
    AssertPtrReturn(pInterface, PDMAUDIOBACKENDSTS_UNKNOWN);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    PDMAUDIOBACKENDSTS backendSts = PDMAUDIOBACKENDSTS_UNKNOWN;

    int rc = RTCritSectEnter(&pThis->CritSect);
    if (RT_SUCCESS(rc))
    {
        if (   pThis->pHostDrvAudio
            && pThis->pHostDrvAudio->pfnGetStatus)
        {
             backendSts = pThis->pHostDrvAudio->pfnGetStatus(pThis->pHostDrvAudio, enmDir);
        }

        int rc2 = RTCritSectLeave(&pThis->CritSect);
        if (RT_SUCCESS(rc))
            rc = rc2;
    }

    LogFlowFuncLeaveRC(rc);
    return backendSts;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamGetReadable}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamGetReadable(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, 0);
    AssertPtrReturn(pStream,    0);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    AssertMsg(pStream->enmDir == PDMAUDIODIR_IN, ("Can't read from a non-input stream\n"));

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (!pHstStream) /* No host stream available? Bail out early. */
    {
        rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);

        return 0;
    }

    uint32_t cReadable = 0;

    PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
    if (pGstStream)
        cReadable = AudioMixBufLive(&pGstStream->MixBuf);

    Log3Func(("[%s] cbReadable=%RU32 (%zu bytes)\n", pHstStream->szName, cReadable,
              AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cReadable)));

    uint32_t cbReadable = AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cReadable);

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    /* Return bytes instead of audio samples. */
    return cbReadable;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamGetWritable}
 */
static DECLCALLBACK(uint32_t) drvAudioStreamGetWritable(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, 0);
    AssertPtrReturn(pStream,    0);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    AssertMsg(pStream->enmDir == PDMAUDIODIR_OUT, ("Can't write to a non-output stream\n"));

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (!pHstStream) /* No host stream available? Bail out early. */
    {
        rc2 = RTCritSectLeave(&pThis->CritSect);
        AssertRC(rc2);

        AssertMsgFailed(("Guest stream '%s' does not have a host stream attached\n", pStream->szName));
        return 0;
    }

    PPDMAUDIOSTREAM pGstStream = pHstStream->pPair;
    AssertPtr(pGstStream);

    uint32_t cWritable = AudioMixBufFree(&pGstStream->MixBuf);

    Log3Func(("[%s] cWritable=%RU32 (%zu bytes)\n", pHstStream->szName, cWritable,
              AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cWritable)));

    uint32_t cbWritable = AUDIOMIXBUF_S2B(&pGstStream->MixBuf, cWritable);

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    /* Return bytes instead of audio samples. */
    return cbWritable;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamGetStatus}
 */
static DECLCALLBACK(PDMAUDIOSTRMSTS) drvAudioStreamGetStatus(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, false);

    if (!pStream)
        return PDMAUDIOSTRMSTS_FLAG_NONE;

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    PDMAUDIOSTRMSTS strmSts = PDMAUDIOSTRMSTS_FLAG_NONE;

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    if (pHstStream)
    {
        strmSts = pHstStream->fStatus;
        Log3Func(("%s: strmSts=0x%x\n", pHstStream->szName, strmSts));
    }

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    return strmSts;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamSetVolume}
 */
static DECLCALLBACK(int) drvAudioStreamSetVolume(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream, PPDMAUDIOVOLUME pVol)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);
    AssertPtrReturn(pVol,       VERR_INVALID_POINTER);

    LogFlowFunc(("%s: volL=%RU32, volR=%RU32, fMute=%RTbool\n", pStream->szName, pVol->uLeft, pVol->uRight, pVol->fMuted));

    PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
    PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;

    AudioMixBufSetVolume(&pHstStream->MixBuf, pVol);
    AudioMixBufSetVolume(&pGstStream->MixBuf, pVol);
    return VINF_SUCCESS;
}

/**
 * @interface_method_impl{PDMIAUDIOCONNECTOR,pfnStreamDestroy}
 */
static DECLCALLBACK(int) drvAudioStreamDestroy(PPDMIAUDIOCONNECTOR pInterface, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pInterface, VERR_INVALID_POINTER);
    AssertPtrReturn(pStream,    VERR_INVALID_POINTER);

    PDRVAUDIO pThis = PDMIAUDIOCONNECTOR_2_DRVAUDIO(pInterface);

    int rc = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc);

    PDMAUDIODIR enmDir = pStream->enmDir;

    LogFlowFunc(("%s: cRefs=%RU32\n", pStream->szName, pStream->cRefs));
    if (pStream->cRefs > 1)
        rc = VERR_WRONG_ORDER;

    if (RT_SUCCESS(rc))
    {
        PPDMAUDIOSTREAM pHstStream = drvAudioGetHostStream(pStream);
        PPDMAUDIOSTREAM pGstStream = pHstStream ? pHstStream->pPair : pStream;

        LogRel2(("Audio: Destroying host stream '%s' (guest stream '%s')\n",
                 pHstStream ? pHstStream->szName : "<None>",
                 pGstStream ? pGstStream->szName : "<None>"));

        /* Should prevent double frees. */
        Assert(pHstStream != pGstStream);

        if (pHstStream)
        {
            rc = drvAudioStreamUninitInternal(pThis, pHstStream);
            if (RT_SUCCESS(rc))
            {
#ifdef VBOX_WITH_STATISTICS
                if (pHstStream->enmDir == PDMAUDIODIR_IN)
                {
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pHstStream->In.StatSamplesCaptured);
                }
                else if (pHstStream->enmDir == PDMAUDIODIR_OUT)
                {
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pHstStream->Out.StatSamplesPlayed);
                }
#endif
                RTListNodeRemove(&pHstStream->Node);

                RTMemFree(pHstStream);
                pHstStream = NULL;
            }
            else
                LogRel2(("Audio: Uninitializing host stream '%s' failed with %Rrc\n", pHstStream->szName, rc));
        }

        if (   RT_SUCCESS(rc)
            && pGstStream)
        {
            rc = drvAudioStreamUninitInternal(pThis, pGstStream);
            if (RT_SUCCESS(rc))
            {
#ifdef VBOX_WITH_STATISTICS
                if (pGstStream->enmDir == PDMAUDIODIR_IN)
                {
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->In.StatBytesElapsed);
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->In.StatBytesTotalRead);
                }
                else if (pGstStream->enmDir == PDMAUDIODIR_OUT)
                {
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->Out.StatBytesElapsed);
                    PDMDrvHlpSTAMDeregister(pThis->pDrvIns, &pGstStream->Out.StatBytesTotalWritten);
                }
#endif
                RTListNodeRemove(&pGstStream->Node);

                RTMemFree(pGstStream);
                pGstStream = NULL;
            }
            else
                LogRel2(("Audio: Uninitializing guest stream '%s' failed with %Rrc\n", pGstStream->szName, rc));
        }
    }

    if (RT_SUCCESS(rc))
    {
        if (enmDir == PDMAUDIODIR_IN)
        {
            pThis->cStreamsFreeIn++;
        }
        else /* Out */
        {
            pThis->cStreamsFreeOut++;
        }
    }

    int rc2 = RTCritSectLeave(&pThis->CritSect);
    if (RT_SUCCESS(rc))
        rc = rc2;

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Creates an audio stream on the backend side.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pHstStream          (Host) audio stream to use for creating the stream on the backend side.
 * @param   pCfgReq             Requested audio stream configuration to use for stream creation.
 * @param   pCfgAcq             Acquired audio stream configuration returned by the backend. Optional, can be NULL.
 */
static int drvAudioStreamCreateInternalBackend(PDRVAUDIO pThis,
                                               PPDMAUDIOSTREAM pHstStream, PPDMAUDIOSTREAMCFG pCfgReq, PPDMAUDIOSTREAMCFG pCfgAcq)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);
    AssertPtrReturn(pCfgReq,    VERR_INVALID_POINTER);
    /* pCfgAcq is optional. */

    AssertMsg(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST,
              ("Stream '%s' is not a host stream and therefore has no backend\n", pHstStream->szName));

    AssertMsg((pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_INITIALIZED) == 0,
              ("Stream '%s' already initialized in backend\n", pHstStream->szName));

    PDMAUDIOSTREAMCFG CfgAcq;

    /* Make the acquired host configuration the requested host configuration initially,
     * in case the backend does not report back an acquired configuration. */
    memcpy(&CfgAcq, pCfgReq, sizeof(PDMAUDIOSTREAMCFG));

    int rc = pThis->pHostDrvAudio->pfnStreamCreate(pThis->pHostDrvAudio, pHstStream, pCfgReq, &CfgAcq);
    if (RT_FAILURE(rc))
    {
        LogRel2(("Audio: Creating stream '%s' in backend failed with %Rrc\n", pHstStream->szName, rc));
        return rc;
    }

    /* Validate acquired configuration. */
    if (!DrvAudioHlpStreamCfgIsValid(&CfgAcq))
    {
        LogRel2(("Audio: Creating stream '%s' has an invalid configuration, skipping\n", pHstStream->szName));
        return VERR_INVALID_PARAMETER;
    }

    /* Only set the host's stream to initialized if we were able create the stream
     * in the host backend. This is necessary for trying to re-initialize the stream
     * at some later point in time. */
    pHstStream->fStatus |= PDMAUDIOSTRMSTS_FLAG_INITIALIZED;

    if (pCfgAcq)
        memcpy(pCfgAcq, &CfgAcq, sizeof(PDMAUDIOSTREAMCFG));

    return VINF_SUCCESS;
}

/**
 * Calls the backend to give it the chance to destroy its part of the audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pHstStream          Host audio stream to call the backend destruction for.
 */
static int drvAudioStreamDestroyInternalBackend(PDRVAUDIO pThis, PPDMAUDIOSTREAM pHstStream)
{
    AssertPtrReturn(pThis,      VERR_INVALID_POINTER);
    AssertPtrReturn(pHstStream, VERR_INVALID_POINTER);

    AssertMsg(pHstStream->enmCtx == PDMAUDIOSTREAMCTX_HOST,
              ("Stream '%s' is not a host stream and therefore has no backend\n", pHstStream->szName));

    int rc = VINF_SUCCESS;

    LogFlowFunc(("%s: fStatus=0x%x\n", pHstStream->szName, pHstStream->fStatus));

    if (pHstStream->fStatus & PDMAUDIOSTRMSTS_FLAG_INITIALIZED)
    {
        /* Check if the pointer to  the host audio driver is still valid.
         * It can be NULL if we were called in drvAudioDestruct, for example. */
        if (pThis->pHostDrvAudio)
            rc = pThis->pHostDrvAudio->pfnStreamDestroy(pThis->pHostDrvAudio, pHstStream);
        if (RT_SUCCESS(rc))
            pHstStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_INITIALIZED;
    }

    LogFlowFunc(("%s: Returning %Rrc\n", pHstStream->szName, rc));
    return rc;
}

/**
 * Uninitializes an audio stream.
 *
 * @returns IPRT status code.
 * @param   pThis               Pointer to driver instance.
 * @param   pStream             Pointer to audio stream to uninitialize.
 */
static int drvAudioStreamUninitInternal(PDRVAUDIO pThis, PPDMAUDIOSTREAM pStream)
{
    AssertPtrReturn(pThis,   VERR_INVALID_POINTER);
    AssertPtrReturn(pStream, VERR_INVALID_POINTER);

    LogFlowFunc(("%s: cRefs=%RU32\n", pStream->szName, pStream->cRefs));

    if (pStream->cRefs > 1)
        return VERR_WRONG_ORDER;

    int rc = VINF_SUCCESS;

    if (pStream->enmCtx == PDMAUDIOSTREAMCTX_GUEST)
    {
        if (pStream->fStatus & PDMAUDIOSTRMSTS_FLAG_INITIALIZED)
        {
            rc = drvAudioStreamControlInternal(pThis, pStream, PDMAUDIOSTREAMCMD_DISABLE);
            if (RT_SUCCESS(rc))
                pStream->fStatus &= ~PDMAUDIOSTRMSTS_FLAG_INITIALIZED;
        }
    }
    else if (pStream->enmCtx == PDMAUDIOSTREAMCTX_HOST)
    {
        rc = drvAudioStreamDestroyInternalBackend(pThis, pStream);
    }
    else
        AssertFailedReturn(VERR_NOT_IMPLEMENTED);

    if (RT_SUCCESS(rc))
    {
        /* Make sure that the pair (if any) knows that we're not valid anymore. */
        int rc2 = drvAudioStreamLinkToInternal(pStream, NULL);
        AssertRC(rc2);

        /* Reset status. */
        pStream->fStatus = PDMAUDIOSTRMSTS_FLAG_NONE;

        /* Destroy mixing buffer. */
        AudioMixBufDestroy(&pStream->MixBuf);
    }

    LogFlowFunc(("Returning %Rrc\n", rc));
    return rc;
}

/********************************************************************/

/**
 * @interface_method_impl{PDMIBASE,pfnQueryInterface}
 */
static DECLCALLBACK(void *) drvAudioQueryInterface(PPDMIBASE pInterface, const char *pszIID)
{
    LogFlowFunc(("pInterface=%p, pszIID=%s\n", pInterface, pszIID));

    PPDMDRVINS pDrvIns = PDMIBASE_2_PDMDRV(pInterface);
    PDRVAUDIO  pThis   = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIBASE, &pDrvIns->IBase);
    PDMIBASE_RETURN_INTERFACE(pszIID, PDMIAUDIOCONNECTOR, &pThis->IAudioConnector);

    return NULL;
}

/**
 * Power Off notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvAudioPowerOff(PPDMDRVINS pDrvIns)
{
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    LogFlowFuncEnter();

    /* Just destroy the host stream on the backend side.
     * The rest will either be destructed by the device emulation or
     * in drvAudioDestruct(). */
    PPDMAUDIOSTREAM pStream;
    RTListForEach(&pThis->lstHstStreams, pStream, PDMAUDIOSTREAM, Node)
        drvAudioStreamDestroyInternalBackend(pThis, pStream);

    /*
     * Last call for the driver below us.
     * Let it know that we reached end of life.
     */
    if (pThis->pHostDrvAudio->pfnShutdown)
        pThis->pHostDrvAudio->pfnShutdown(pThis->pHostDrvAudio);

    pThis->pHostDrvAudio = NULL;

    LogFlowFuncLeave();
}

/**
 * Constructs an audio driver instance.
 *
 * @copydoc FNPDMDRVCONSTRUCT
 */
static DECLCALLBACK(int) drvAudioConstruct(PPDMDRVINS pDrvIns, PCFGMNODE pCfgHandle, uint32_t fFlags)
{
    LogFlowFunc(("pDrvIns=%#p, pCfgHandle=%#p, fFlags=%x\n", pDrvIns, pCfgHandle, fFlags));

    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);
    PDMDRV_CHECK_VERSIONS_RETURN(pDrvIns);

    RTListInit(&pThis->lstHstStreams);
    RTListInit(&pThis->lstGstStreams);
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    RTListInit(&pThis->lstCBIn);
    RTListInit(&pThis->lstCBOut);
#endif

    /*
     * Init the static parts.
     */
    pThis->pDrvIns                              = pDrvIns;
    /* IBase. */
    pDrvIns->IBase.pfnQueryInterface            = drvAudioQueryInterface;
    /* IAudioConnector. */
    pThis->IAudioConnector.pfnGetConfig         = drvAudioGetConfig;
    pThis->IAudioConnector.pfnGetStatus         = drvAudioGetStatus;
    pThis->IAudioConnector.pfnStreamCreate      = drvAudioStreamCreate;
    pThis->IAudioConnector.pfnStreamDestroy     = drvAudioStreamDestroy;
    pThis->IAudioConnector.pfnStreamRetain      = drvAudioStreamRetain;
    pThis->IAudioConnector.pfnStreamRelease     = drvAudioStreamRelease;
    pThis->IAudioConnector.pfnStreamControl     = drvAudioStreamControl;
    pThis->IAudioConnector.pfnStreamRead        = drvAudioStreamRead;
    pThis->IAudioConnector.pfnStreamWrite       = drvAudioStreamWrite;
    pThis->IAudioConnector.pfnStreamIterate     = drvAudioStreamIterate;
    pThis->IAudioConnector.pfnStreamGetReadable = drvAudioStreamGetReadable;
    pThis->IAudioConnector.pfnStreamGetWritable = drvAudioStreamGetWritable;
    pThis->IAudioConnector.pfnStreamGetStatus   = drvAudioStreamGetStatus;
    pThis->IAudioConnector.pfnStreamSetVolume   = drvAudioStreamSetVolume;
    pThis->IAudioConnector.pfnStreamPlay        = drvAudioStreamPlay;
    pThis->IAudioConnector.pfnStreamCapture     = drvAudioStreamCapture;
#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    pThis->IAudioConnector.pfnRegisterCallbacks = drvAudioRegisterCallbacks;
    pThis->IAudioConnector.pfnCallback          = drvAudioCallback;
#endif

    /*
     * Attach driver below and query its connector interface.
     */
    PPDMIBASE pDownBase;
    int rc = PDMDrvHlpAttach(pDrvIns, fFlags, &pDownBase);
    if (RT_FAILURE(rc))
    {
        LogRel(("Audio: Failed to attach to driver %p below (flags=0x%x), rc=%Rrc\n",
                pDrvIns, fFlags, rc));
        return rc;
    }

    pThis->pHostDrvAudio = PDMIBASE_QUERY_INTERFACE(pDownBase, PDMIHOSTAUDIO);
    if (!pThis->pHostDrvAudio)
    {
        LogRel(("Audio: Failed to query interface for underlying host driver\n"));
        return PDMDRV_SET_ERROR(pDrvIns, VERR_PDM_MISSING_INTERFACE_BELOW,
                                N_("Host audio backend missing or invalid"));
    }

    rc = drvAudioInit(pDrvIns, pCfgHandle);
    if (RT_SUCCESS(rc))
    {
        pThis->fTerminate = false;
        pThis->pDrvIns    = pDrvIns;

#ifdef VBOX_WITH_STATISTICS
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalStreamsActive,   "TotalStreamsActive",
                                  STAMUNIT_COUNT, "Active input streams.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalStreamsCreated,  "TotalStreamsCreated",
                                  STAMUNIT_COUNT, "Total created input streams.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalSamplesPlayed,   "TotalSamplesPlayed",
                                  STAMUNIT_COUNT, "Total samples played.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalSamplesCaptured, "TotalSamplesCaptured",
                                  STAMUNIT_COUNT, "Total samples captured.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalBytesRead,       "TotalBytesRead",
                                  STAMUNIT_BYTES, "Total bytes read.");
        PDMDrvHlpSTAMRegCounterEx(pDrvIns, &pThis->Stats.TotalBytesWritten,    "TotalBytesWritten",
                                  STAMUNIT_BYTES, "Total bytes written.");
#endif
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

/**
 * Destructs an audio driver instance.
 *
 * @copydoc FNPDMDRVDESTRUCT
 */
static DECLCALLBACK(void) drvAudioDestruct(PPDMDRVINS pDrvIns)
{
    PDMDRV_CHECK_VERSIONS_RETURN_VOID(pDrvIns);
    PDRVAUDIO pThis = PDMINS_2_DATA(pDrvIns, PDRVAUDIO);

    LogFlowFuncEnter();

    int rc2 = RTCritSectEnter(&pThis->CritSect);
    AssertRC(rc2);

    /*
     * Note: No calls here to the driver below us anymore,
     *       as PDM already has destroyed it.
     *       If you need to call something from the host driver,
     *       do this in drvAudioPowerOff() instead.
     */

    /* Thus, NULL the pointer to the host audio driver first,
     * so that routines like drvAudioStreamDestroyInternal() don't call the driver(s) below us anymore. */
    pThis->pHostDrvAudio = NULL;

    PPDMAUDIOSTREAM pStream, pStreamNext;
    RTListForEachSafe(&pThis->lstHstStreams, pStream, pStreamNext, PDMAUDIOSTREAM, Node)
    {
        rc2 = drvAudioStreamUninitInternal(pThis, pStream);
        if (RT_SUCCESS(rc2))
        {
            RTListNodeRemove(&pStream->Node);

            RTMemFree(pStream);
            pStream = NULL;
        }
    }

    /* Sanity. */
    Assert(RTListIsEmpty(&pThis->lstHstStreams));

    RTListForEachSafe(&pThis->lstGstStreams, pStream, pStreamNext, PDMAUDIOSTREAM, Node)
    {
        rc2 = drvAudioStreamUninitInternal(pThis, pStream);
        if (RT_SUCCESS(rc2))
        {
            RTListNodeRemove(&pStream->Node);

            RTMemFree(pStream);
            pStream = NULL;
        }
    }

    /* Sanity. */
    Assert(RTListIsEmpty(&pThis->lstGstStreams));

#ifdef VBOX_WITH_AUDIO_DEVICE_CALLBACKS
    /*
     * Destroy device callbacks, if any.
     */
    PPDMAUDIOCALLBACK pCB, pCBNext;
    RTListForEachSafe(&pThis->lstCBIn, pCB, pCBNext, PDMAUDIOCALLBACK, Node)
        drvAudioCallbackDestroy(pCB);

    RTListForEachSafe(&pThis->lstCBOut, pCB, pCBNext, PDMAUDIOCALLBACK, Node)
        drvAudioCallbackDestroy(pCB);
#endif

    rc2 = RTCritSectLeave(&pThis->CritSect);
    AssertRC(rc2);

    rc2 = RTCritSectDelete(&pThis->CritSect);
    AssertRC(rc2);

    LogFlowFuncLeave();
}

/**
 * Suspend notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvAudioSuspend(PPDMDRVINS pDrvIns)
{
    drvAudioStateHandler(pDrvIns, PDMAUDIOSTREAMCMD_PAUSE);
}

/**
 * Resume notification.
 *
 * @param   pDrvIns     The driver instance data.
 */
static DECLCALLBACK(void) drvAudioResume(PPDMDRVINS pDrvIns)
{
    drvAudioStateHandler(pDrvIns, PDMAUDIOSTREAMCMD_RESUME);
}

/**
 * Audio driver registration record.
 */
const PDMDRVREG g_DrvAUDIO =
{
    /* u32Version */
    PDM_DRVREG_VERSION,
    /* szName */
    "AUDIO",
    /* szRCMod */
    "",
    /* szR0Mod */
    "",
    /* pszDescription */
    "Audio connector driver",
    /* fFlags */
    PDM_DRVREG_FLAGS_HOST_BITS_DEFAULT,
    /* fClass */
    PDM_DRVREG_CLASS_AUDIO,
    /* cMaxInstances */
    UINT32_MAX,
    /* cbInstance */
    sizeof(DRVAUDIO),
    /* pfnConstruct */
    drvAudioConstruct,
    /* pfnDestruct */
    drvAudioDestruct,
    /* pfnRelocate */
    NULL,
    /* pfnIOCtl */
    NULL,
    /* pfnPowerOn */
    NULL,
    /* pfnReset */
    NULL,
    /* pfnSuspend */
    drvAudioSuspend,
    /* pfnResume */
    drvAudioResume,
    /* pfnAttach */
    NULL,
    /* pfnDetach */
    NULL,
    /* pfnPowerOff */
    drvAudioPowerOff,
    /* pfnSoftReset */
    NULL,
    /* u32EndVersion */
    PDM_DRVREG_VERSION
};


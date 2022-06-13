/* $Id: vkat.cpp 91454 2021-09-29 10:35:39Z vboxsync $ */
/** @file
 * Validation Kit Audio Test (VKAT) utility for testing and validating the audio stack.
 */

/*
 * Copyright (C) 2021 Oracle Corporation
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_AUDIO_TEST

#include <iprt/buildconfig.h>
#include <iprt/ctype.h>
#include <iprt/dir.h>
#include <iprt/errcore.h>
#include <iprt/file.h>
#include <iprt/initterm.h>
#include <iprt/getopt.h>
#include <iprt/message.h>
#include <iprt/path.h>
#include <iprt/process.h>
#include <iprt/rand.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/test.h>

#include <package-generated.h>
#include "product-generated.h"

#include <VBox/version.h>
#include <VBox/log.h>

#ifdef RT_OS_WINDOWS
# include <iprt/win/windows.h> /* for CoInitializeEx */
#endif
#include <signal.h>

#include "vkatInternal.h"


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int audioVerifyOne(const char *pszPathSetA, const char *pszPathSetB, PAUDIOTESTVERIFYOPTS pOpts);


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Backends description table.
 *
 * @note The first backend in the array is the default one for the platform.
 */
AUDIOTESTBACKENDDESC const g_aBackends[] =
{
#ifdef VBOX_WITH_AUDIO_PULSE
    {   &g_DrvHostPulseAudio,         "pulseaudio" },
    {   &g_DrvHostPulseAudio,         "pulse" },
    {   &g_DrvHostPulseAudio,         "pa" },
#endif
/*
 * Note: ALSA has to come second so that PulseAudio above always is the default on Linux-y OSes
 *       -- most distros are using an ALSA plugin for PulseAudio nowadays.
 *       However, some of these configurations do not seem to work by default (can't create audio streams).
 *
 *       If PulseAudio is not available, the (optional) probing ("--probe") will choose the "pure" ALSA stack instead then.
 */
#if defined(VBOX_WITH_AUDIO_ALSA) && defined(RT_OS_LINUX)
    {   &g_DrvHostALSAAudio,          "alsa" },
#endif
#ifdef VBOX_WITH_AUDIO_OSS
    {   &g_DrvHostOSSAudio,           "oss" },
#endif
#if defined(RT_OS_DARWIN)
    {   &g_DrvHostCoreAudio,          "coreaudio" },
    {   &g_DrvHostCoreAudio,          "core" },
    {   &g_DrvHostCoreAudio,          "ca" },
#endif
#if defined(RT_OS_WINDOWS)
    {   &g_DrvHostAudioWas,           "wasapi" },
    {   &g_DrvHostAudioWas,           "was" },
    {   &g_DrvHostDSound,             "directsound" },
    {   &g_DrvHostDSound,             "dsound" },
    {   &g_DrvHostDSound,             "ds" },
#endif
#ifdef VBOX_WITH_AUDIO_DEBUG
    {   &g_DrvHostDebugAudio,         "debug" },
#endif
    {   &g_DrvHostValidationKitAudio, "valkit" }
};
AssertCompile(sizeof(g_aBackends) > 0 /* port me */);
/** Number of backends defined. */
unsigned g_cBackends = RT_ELEMENTS(g_aBackends);

/**
 * Long option values for the 'test' command.
 */
enum
{
    VKAT_TEST_OPT_COUNT = 900,
    VKAT_TEST_OPT_DEV,
    VKAT_TEST_OPT_TONE_DURATION_MS,
    VKAT_TEST_OPT_GUEST_ATS_ADDR,
    VKAT_TEST_OPT_GUEST_ATS_PORT,
    VKAT_TEST_OPT_HOST_ATS_ADDR,
    VKAT_TEST_OPT_HOST_ATS_PORT,
    VKAT_TEST_OPT_MODE,
    VKAT_TEST_OPT_NO_VERIFY,
    VKAT_TEST_OPT_OUTDIR,
    VKAT_TEST_OPT_PAUSE,
    VKAT_TEST_OPT_PCM_HZ,
    VKAT_TEST_OPT_PCM_BIT,
    VKAT_TEST_OPT_PCM_CHAN,
    VKAT_TEST_OPT_PCM_SIGNED,
    VKAT_TEST_OPT_PROBE_BACKENDS,
    VKAT_TEST_OPT_TAG,
    VKAT_TEST_OPT_TEMPDIR,
    VKAT_TEST_OPT_VOL,
    VKAT_TEST_OPT_TCP_BIND_ADDRESS,
    VKAT_TEST_OPT_TCP_BIND_PORT,
    VKAT_TEST_OPT_TCP_CONNECT_ADDRESS,
    VKAT_TEST_OPT_TCP_CONNECT_PORT
};

/**
 * Long option values for the 'verify' command.
 */
enum
{
    VKAT_VERIFY_OPT_MAX_DIFF_COUNT = 900,
    VKAT_VERIFY_OPT_MAX_DIFF_PERCENT,
    VKAT_VERIFY_OPT_MAX_SIZE_PERCENT
};

/**
 * Common command line parameters.
 */
static const RTGETOPTDEF g_aCmdCommonOptions[] =
{
    { "--quiet",            'q',                                        RTGETOPT_REQ_NOTHING },
    { "--verbose",          'v',                                        RTGETOPT_REQ_NOTHING },
    { "--daemonize",        AUDIO_TEST_OPT_CMN_DAEMONIZE,               RTGETOPT_REQ_NOTHING },
    { "--daemonized",       AUDIO_TEST_OPT_CMN_DAEMONIZED,              RTGETOPT_REQ_NOTHING },
    { "--debug-audio",      AUDIO_TEST_OPT_CMN_DEBUG_AUDIO_ENABLE,      RTGETOPT_REQ_NOTHING },
    { "--debug-audio-path", AUDIO_TEST_OPT_CMN_DEBUG_AUDIO_PATH,        RTGETOPT_REQ_STRING  },
};

/**
 * Command line parameters for test mode.
 */
static const RTGETOPTDEF g_aCmdTestOptions[] =
{
    { "--backend",           'b',                               RTGETOPT_REQ_STRING  },
    { "--drvaudio",          'd',                               RTGETOPT_REQ_NOTHING },
    { "--exclude",           'e',                               RTGETOPT_REQ_UINT32  },
    { "--exclude-all",       'a',                               RTGETOPT_REQ_NOTHING },
    { "--guest-ats-addr",    VKAT_TEST_OPT_GUEST_ATS_ADDR,      RTGETOPT_REQ_STRING  },
    { "--guest-ats-port",    VKAT_TEST_OPT_GUEST_ATS_PORT,      RTGETOPT_REQ_UINT32  },
    { "--host-ats-address",  VKAT_TEST_OPT_HOST_ATS_ADDR,       RTGETOPT_REQ_STRING  },
    { "--host-ats-port",     VKAT_TEST_OPT_HOST_ATS_PORT,       RTGETOPT_REQ_UINT32  },
    { "--include",           'i',                               RTGETOPT_REQ_UINT32  },
    { "--outdir",            VKAT_TEST_OPT_OUTDIR,              RTGETOPT_REQ_STRING  },
    { "--count",             VKAT_TEST_OPT_COUNT,               RTGETOPT_REQ_UINT32  },
    { "--device",            VKAT_TEST_OPT_DEV,                 RTGETOPT_REQ_STRING  },
    { "--pause",             VKAT_TEST_OPT_PAUSE,               RTGETOPT_REQ_UINT32  },
    { "--pcm-bit",           VKAT_TEST_OPT_PCM_BIT,             RTGETOPT_REQ_UINT8   },
    { "--pcm-chan",          VKAT_TEST_OPT_PCM_CHAN,            RTGETOPT_REQ_UINT8   },
    { "--pcm-hz",            VKAT_TEST_OPT_PCM_HZ,              RTGETOPT_REQ_UINT16  },
    { "--pcm-signed",        VKAT_TEST_OPT_PCM_SIGNED,          RTGETOPT_REQ_BOOL    },
    { "--probe-backends",    VKAT_TEST_OPT_PROBE_BACKENDS,      RTGETOPT_REQ_NOTHING },
    { "--mode",              VKAT_TEST_OPT_MODE,                RTGETOPT_REQ_STRING  },
    { "--no-verify",         VKAT_TEST_OPT_NO_VERIFY,           RTGETOPT_REQ_NOTHING },
    { "--tag",               VKAT_TEST_OPT_TAG,                 RTGETOPT_REQ_STRING  },
    { "--tempdir",           VKAT_TEST_OPT_TEMPDIR,             RTGETOPT_REQ_STRING  },
    { "--volume",            VKAT_TEST_OPT_VOL,                 RTGETOPT_REQ_UINT8   },
    { "--tcp-bind-addr",     VKAT_TEST_OPT_TCP_BIND_ADDRESS,    RTGETOPT_REQ_STRING  },
    { "--tcp-bind-port",     VKAT_TEST_OPT_TCP_BIND_PORT,       RTGETOPT_REQ_UINT16  },
    { "--tcp-connect-addr",  VKAT_TEST_OPT_TCP_CONNECT_ADDRESS, RTGETOPT_REQ_STRING  },
    { "--tcp-connect-port",  VKAT_TEST_OPT_TCP_CONNECT_PORT,    RTGETOPT_REQ_UINT16  },
    { "--tone-duration",     VKAT_TEST_OPT_TONE_DURATION_MS,    RTGETOPT_REQ_UINT32  }
};

/**
 * Command line parameters for verification mode.
 */
static const RTGETOPTDEF g_aCmdVerifyOptions[] =
{
    { "--max-diff-count",      VKAT_VERIFY_OPT_MAX_DIFF_COUNT,     RTGETOPT_REQ_UINT32 },
    { "--max-diff-percent",    VKAT_VERIFY_OPT_MAX_DIFF_PERCENT,   RTGETOPT_REQ_UINT8  },
    { "--max-size-percent",    VKAT_VERIFY_OPT_MAX_SIZE_PERCENT,   RTGETOPT_REQ_UINT8  }
};

/** Terminate ASAP if set.  Set on Ctrl-C. */
bool volatile    g_fTerminate = false;
/** The release logger. */
PRTLOGGER        g_pRelLogger = NULL;
/** The test handle. */
RTTEST           g_hTest;
/** The current verbosity level. */
unsigned         g_uVerbosity = 0;
/** DrvAudio: Enable debug (or not). */
bool             g_fDrvAudioDebug = false;
/** DrvAudio: The debug output path. */
const char      *g_pszDrvAudioDebug = NULL;


/**
 * Get default backend.
 */
PCPDMDRVREG AudioTestGetDefaultBackend(void)
{
    return g_aBackends[0].pDrvReg;
}


/**
 * Helper for handling --backend options.
 *
 * @returns Pointer to the specified backend, NULL if not found (error
 *          displayed).
 * @param   pszBackend      The backend option value.
 */
PCPDMDRVREG AudioTestFindBackendOpt(const char *pszBackend)
{
    for (uintptr_t i = 0; i < RT_ELEMENTS(g_aBackends); i++)
        if (   strcmp(pszBackend, g_aBackends[i].pszName) == 0
            || strcmp(pszBackend, g_aBackends[i].pDrvReg->szName) == 0)
            return g_aBackends[i].pDrvReg;
    RTMsgError("Unknown backend: '%s'", pszBackend);
    return NULL;
}


/*********************************************************************************************************************************
*   Test callbacks                                                                                                               *
*********************************************************************************************************************************/

/**
 * @copydoc FNAUDIOTESTSETUP
 */
static DECLCALLBACK(int) audioTestPlayToneSetup(PAUDIOTESTENV pTstEnv, PAUDIOTESTDESC pTstDesc, PAUDIOTESTPARMS pTstParmsAcq, void **ppvCtx)
{
    RT_NOREF(pTstDesc, ppvCtx);

    int rc = VINF_SUCCESS;

    if (strlen(pTstEnv->szDev))
    {
        rc = audioTestDriverStackSetDevice(pTstEnv->pDrvStack, PDMAUDIODIR_OUT, pTstEnv->szDev);
        if (RT_FAILURE(rc))
            return rc;
    }

    pTstParmsAcq->enmType     = AUDIOTESTTYPE_TESTTONE_PLAY;
    pTstParmsAcq->Props       = pTstEnv->Props;
    pTstParmsAcq->enmDir      = PDMAUDIODIR_OUT;
    pTstParmsAcq->cIterations = pTstEnv->cIterations == 0 ? RTRandU32Ex(1, 10) : pTstEnv->cIterations;
    pTstParmsAcq->idxCurrent  = 0;

    PAUDIOTESTTONEPARMS pToneParms = &pTstParmsAcq->TestTone;

    pToneParms->Props          = pTstParmsAcq->Props;
    pToneParms->dbFreqHz       = AudioTestToneGetRandomFreq();
    pToneParms->msPrequel      = 0; /** @todo Implement analyzing this first! */
    pToneParms->msDuration     = pTstEnv->cMsToneDuration == 0 ? RTRandU32Ex(200, RT_MS_30SEC) : pTstEnv->cMsToneDuration;
    pToneParms->msSequel       = 0;   /** @todo Implement analyzing this first! */
    pToneParms->uVolumePercent = 100; /** @todo Implement analyzing this first! */

    return rc;
}

/**
 * @copydoc FNAUDIOTESTEXEC
 */
static DECLCALLBACK(int) audioTestPlayToneExec(PAUDIOTESTENV pTstEnv, void *pvCtx, PAUDIOTESTPARMS pTstParms)
{
    RT_NOREF(pvCtx);

    int rc = VINF_SUCCESS;

    for (uint32_t i = 0; i < pTstParms->cIterations; i++)
    {
        PAUDIOTESTTONEPARMS const pToneParms = &pTstParms->TestTone;

        pToneParms->Hdr.idxSeq = i;
        RTTIMESPEC NowTimeSpec;
        RTTimeExplode(&pToneParms->Hdr.tsCreated, RTTimeNow(&NowTimeSpec));

        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Test #%RU32/%RU16: Playing test tone (%RU16Hz, %RU32ms)\n",
                     pTstParms->idxCurrent, i, (uint16_t)pToneParms->dbFreqHz, pToneParms->msDuration);

        /*
         * 1. Arm the (host) ValKit ATS with the recording parameters.
         */
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Telling ValKit audio driver on host to record new tone ...\n");
        rc = AudioTestSvcClientToneRecord(&pTstEnv->u.Host.AtsClValKit, pToneParms);
        if (RT_SUCCESS(rc))
        {
            /* Give the Validaiton Kit audio driver on the host a bit of time to register / arming the new test. */
            RTThreadSleep(5000); /* Fudge factor. */

            /*
             * 2. Tell VKAT on guest  to start playback.
             */
            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Telling VKAT on guest to play tone ...\n");
            rc = AudioTestSvcClientTonePlay(&pTstEnv->u.Host.AtsClGuest, pToneParms);
            if (RT_FAILURE(rc))
                RTTestFailed(g_hTest, "Test #%RU32/%RU16: AudioTestSvcClientTonePlay() failed with %Rrc\n",
                             pTstParms->idxCurrent, i, rc);
        }
        else
            RTTestFailed(g_hTest, "Test #%RU32/%RU16: AudioTestSvcClientToneRecord() failed with %Rrc\n",
                         pTstParms->idxCurrent, i, rc);

        if (RT_SUCCESS(rc))
        {
            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Playing tone done\n");

            /* Give the audio stack a random amount of time for draining data before the next iteration. */
            if (pTstParms->cIterations > 1)
                RTThreadSleep(RTRandU32Ex(2000, 5000)); /** @todo Implement some dedicated ATS command for this? */
        }

        if (RT_FAILURE(rc))
        {
            RTTestFailed(g_hTest, "Test #%RU32/%RU16: Playing test tone failed with %Rrc\n", pTstParms->idxCurrent, i, rc);
            break; /* Not worth retrying, bail out. */
        }
    }

    return rc;
}

/**
 * @copydoc FNAUDIOTESTDESTROY
 */
static DECLCALLBACK(int) audioTestPlayToneDestroy(PAUDIOTESTENV pTstEnv, void *pvCtx)
{
    RT_NOREF(pTstEnv, pvCtx);

    return VINF_SUCCESS;
}

/**
 * @copydoc FNAUDIOTESTSETUP
 */
static DECLCALLBACK(int) audioTestRecordToneSetup(PAUDIOTESTENV pTstEnv, PAUDIOTESTDESC pTstDesc, PAUDIOTESTPARMS pTstParmsAcq, void **ppvCtx)
{
    RT_NOREF(pTstDesc, ppvCtx);

    int rc = VINF_SUCCESS;

    if (strlen(pTstEnv->szDev))
    {
        rc = audioTestDriverStackSetDevice(pTstEnv->pDrvStack, PDMAUDIODIR_IN, pTstEnv->szDev);
        if (RT_FAILURE(rc))
            return rc;
    }

    pTstParmsAcq->enmType     = AUDIOTESTTYPE_TESTTONE_RECORD;
    pTstParmsAcq->Props       = pTstEnv->Props;
    pTstParmsAcq->enmDir      = PDMAUDIODIR_IN;
    pTstParmsAcq->cIterations = pTstEnv->cIterations == 0 ? RTRandU32Ex(1, 10) : pTstEnv->cIterations;
    pTstParmsAcq->idxCurrent  = 0;

    PAUDIOTESTTONEPARMS pToneParms = &pTstParmsAcq->TestTone;

    pToneParms->Props          = pTstParmsAcq->Props;
    pToneParms->dbFreqHz       = AudioTestToneGetRandomFreq();
    pToneParms->msPrequel      = 0; /** @todo Implement analyzing this first! */
    pToneParms->Props          = pTstParmsAcq->Props;
    pToneParms->msDuration     = pTstEnv->cMsToneDuration == 0 ? RTRandU32Ex(200, RT_MS_30SEC) : pTstEnv->cMsToneDuration;
    pToneParms->msSequel       = 0;   /** @todo Implement analyzing this first! */
    pToneParms->uVolumePercent = 100; /** @todo Implement analyzing this first! */

    return rc;
}

/**
 * @copydoc FNAUDIOTESTEXEC
 */
static DECLCALLBACK(int) audioTestRecordToneExec(PAUDIOTESTENV pTstEnv, void *pvCtx, PAUDIOTESTPARMS pTstParms)
{
    RT_NOREF(pvCtx);

    int rc = VINF_SUCCESS;

    for (uint32_t i = 0; i < pTstParms->cIterations; i++)
    {
        PAUDIOTESTTONEPARMS const pToneParms = &pTstParms->TestTone;

        pToneParms->Hdr.idxSeq = i;
        RTTIMESPEC NowTimeSpec;
        RTTimeExplode(&pToneParms->Hdr.tsCreated, RTTimeNow(&NowTimeSpec));

        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Test #%RU32/%RU16: Recording test tone (%RU16Hz, %RU32ms)\n",
                     pTstParms->idxCurrent, i, (uint16_t)pToneParms->dbFreqHz, pToneParms->msDuration);

        /*
         * 1. Arm the (host) ValKit ATS with the playback parameters.
         */
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Telling ValKit audio driver on host to inject recording data ...\n");
        rc = AudioTestSvcClientTonePlay(&pTstEnv->u.Host.AtsClValKit, &pTstParms->TestTone);
        if (RT_SUCCESS(rc))
        {
            /*
             * 2. Tell the guest ATS to start recording.
             */
            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Telling VKAT on guest to record audio ...\n");
            rc = AudioTestSvcClientToneRecord(&pTstEnv->u.Host.AtsClGuest, &pTstParms->TestTone);
            if (RT_FAILURE(rc))
                RTTestFailed(g_hTest, "Test #%RU32/%RU16: AudioTestSvcClientToneRecord() failed with %Rrc\n",
                             pTstParms->idxCurrent, i, rc);
        }
        else
            RTTestFailed(g_hTest, "Test #%RU32/%RU16: AudioTestSvcClientTonePlay() failed with %Rrc\n",
                         pTstParms->idxCurrent, i, rc);

        if (RT_SUCCESS(rc))
        {
            /* Wait a bit to let the left over audio bits being processed. */
            if (pTstParms->cIterations > 1)
                RTThreadSleep(RTRandU32Ex(2000, 5000)); /** @todo Implement some dedicated ATS command for this? */
        }

        if (RT_FAILURE(rc))
        {
            RTTestFailed(g_hTest, "Test #%RU32/%RU16: Recording test tone failed with %Rrc\n", pTstParms->idxCurrent, i, rc);
            break; /* Not worth retrying, bail out. */
        }
    }

    return rc;
}

/**
 * @copydoc FNAUDIOTESTDESTROY
 */
static DECLCALLBACK(int) audioTestRecordToneDestroy(PAUDIOTESTENV pTstEnv, void *pvCtx)
{
    RT_NOREF(pTstEnv, pvCtx);

    return VINF_SUCCESS;
}


/*********************************************************************************************************************************
*   Test execution                                                                                                               *
*********************************************************************************************************************************/

/** Test definition table. */
AUDIOTESTDESC g_aTests[] =
{
    /* pszTest      fExcluded      pfnSetup */
    { "PlayTone",   false,         audioTestPlayToneSetup,       audioTestPlayToneExec,      audioTestPlayToneDestroy },
    { "RecordTone", false,         audioTestRecordToneSetup,     audioTestRecordToneExec,    audioTestRecordToneDestroy }
};
/** Number of tests defined. */
unsigned g_cTests = RT_ELEMENTS(g_aTests);

/**
 * Runs one specific audio test.
 *
 * @returns VBox status code.
 * @param   pTstEnv             Test environment to use for running the test.
 * @param   pTstDesc            Test to run.
 * @param   uSeq                Test sequence # in case there are more tests.
 */
static int audioTestOne(PAUDIOTESTENV pTstEnv, PAUDIOTESTDESC pTstDesc, unsigned uSeq)
{
    RT_NOREF(uSeq);

    int rc;

    AUDIOTESTPARMS TstParms;
    audioTestParmsInit(&TstParms);

    RTTestSub(g_hTest, pTstDesc->pszName);

    if (pTstDesc->fExcluded)
    {
        RTTestSkipped(g_hTest, "Test #%u is excluded from list, skipping", uSeq);
        return VINF_SUCCESS;
    }

    void *pvCtx = NULL; /* Test-specific opaque context. Optional and can be NULL. */

    if (pTstDesc->pfnSetup)
    {
        rc = pTstDesc->pfnSetup(pTstEnv, pTstDesc, &TstParms, &pvCtx);
        if (RT_FAILURE(rc))
        {
            RTTestFailed(g_hTest, "Test #%u setup failed with %Rrc\n", uSeq, rc);
            return rc;
        }
    }

    RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Test #%u (%RU32 iterations total)\n", uSeq, TstParms.cIterations);

    AssertPtr(pTstDesc->pfnExec);
    rc = pTstDesc->pfnExec(pTstEnv, pvCtx, &TstParms);
    if (RT_FAILURE(rc))
        RTTestFailed(g_hTest, "Test #%u failed with %Rrc\n", uSeq, rc);

    RTTestSubDone(g_hTest);

    if (pTstDesc->pfnDestroy)
    {
        int rc2 = pTstDesc->pfnDestroy(pTstEnv, pvCtx);
        if (RT_SUCCESS(rc))
            rc = rc2;

        if (RT_FAILURE(rc2))
            RTTestFailed(g_hTest, "Test #%u destruction failed with %Rrc\n", uSeq, rc2);
    }

    audioTestParmsDestroy(&TstParms);

    return rc;
}

/**
 * Runs all specified tests in a row.
 *
 * @returns VBox status code.
 * @param   pTstEnv             Test environment to use for running all tests.
 */
int audioTestWorker(PAUDIOTESTENV pTstEnv)
{
    int rc = VINF_SUCCESS;

    if (pTstEnv->enmMode == AUDIOTESTMODE_GUEST)
    {
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Guest ATS running\n");

        while (!g_fTerminate)
            RTThreadSleep(100);

        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Shutting down guest ATS ...\n");

        int rc2 = AudioTestSvcStop(pTstEnv->pSrv);
        if (RT_SUCCESS(rc))
            rc = rc2;

        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Guest ATS shutdown complete\n");
    }
    else if (pTstEnv->enmMode == AUDIOTESTMODE_HOST)
    {
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Using tag '%s'\n", pTstEnv->szTag);

        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Telling ValKit audio driver on host to begin a new test set ...\n");
        rc = AudioTestSvcClientTestSetBegin(&pTstEnv->u.Host.AtsClValKit, pTstEnv->szTag);
        if (RT_SUCCESS(rc))
        {
            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Telling VKAT on guest to begin a new test set ...\n");
            rc = AudioTestSvcClientTestSetBegin(&pTstEnv->u.Host.AtsClGuest, pTstEnv->szTag);
            if (RT_FAILURE(rc))
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS,
                             "Beginning test set on guest failed with %Rrc\n", rc);
        }
        else
            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS,
                         "Beginning test set on host (Validation Kit audio driver) failed with %Rrc\n", rc);

        if (RT_SUCCESS(rc))
        {
            unsigned uSeq = 0;
            for (unsigned i = 0; i < RT_ELEMENTS(g_aTests); i++)
            {
                int rc2 = audioTestOne(pTstEnv, &g_aTests[i], uSeq);
                if (RT_SUCCESS(rc))
                    rc = rc2;

                if (!g_aTests[i].fExcluded)
                    uSeq++;

                if (g_fTerminate)
                    break;
            }

            if (RT_SUCCESS(rc))
            {
                /** @todo Fudge! */
                RTMSINTERVAL const msWait = RTRandU32Ex(RT_MS_5SEC, RT_MS_30SEC);
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS,
                             "Waiting %RU32ms to let guest and the audio stack process remaining data  ...\n", msWait);
                RTThreadSleep(msWait);
            }

            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Ending test set on guest ...\n");
            int rc2 = AudioTestSvcClientTestSetEnd(&pTstEnv->u.Host.AtsClGuest, pTstEnv->szTag);
            if (RT_FAILURE(rc2))
            {
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Ending test set on guest failed with %Rrc\n", rc2);
                if (RT_SUCCESS(rc))
                    rc = rc2;
            }

            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Ending test set on host (Validation Kit audio driver) ...\n");
            rc2 = AudioTestSvcClientTestSetEnd(&pTstEnv->u.Host.AtsClValKit, pTstEnv->szTag);
            if (RT_FAILURE(rc2))
            {
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS,
                             "Ending test set on host (Validation Kit audio driver) failed with %Rrc\n", rc2);
                if (RT_SUCCESS(rc))
                    rc = rc2;
            }

            if (   !g_fTerminate
                && RT_SUCCESS(rc))
            {
                /*
                 * Download guest + Validation Kit audio driver test sets to our output directory.
                 */
                char szFileName[RTPATH_MAX];
                if (RTStrPrintf2(szFileName, sizeof(szFileName), "%s-guest.tar.gz", pTstEnv->szTag))
                {
                    rc = RTPathJoin(pTstEnv->u.Host.szPathTestSetGuest, sizeof(pTstEnv->u.Host.szPathTestSetGuest),
                                    pTstEnv->szPathOut, szFileName);
                    if (RT_SUCCESS(rc))
                    {
                        if (RTStrPrintf2(szFileName, sizeof(szFileName), "%s-host.tar.gz", pTstEnv->szTag))
                        {
                            rc = RTPathJoin(pTstEnv->u.Host.szPathTestSetValKit, sizeof(pTstEnv->u.Host.szPathTestSetValKit),
                                            pTstEnv->szPathOut, szFileName);
                        }
                        else
                            rc = VERR_BUFFER_OVERFLOW;
                    }
                    else
                        rc = VERR_BUFFER_OVERFLOW;

                    if (RT_SUCCESS(rc))
                    {
                        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Downloading guest test set to '%s'\n",
                                     pTstEnv->u.Host.szPathTestSetGuest);
                        rc = AudioTestSvcClientTestSetDownload(&pTstEnv->u.Host.AtsClGuest,
                                                               pTstEnv->szTag, pTstEnv->u.Host.szPathTestSetGuest);
                    }

                    if (RT_SUCCESS(rc))
                    {
                        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Downloading host test set to '%s'\n",
                                     pTstEnv->u.Host.szPathTestSetValKit);
                        rc = AudioTestSvcClientTestSetDownload(&pTstEnv->u.Host.AtsClValKit,
                                                               pTstEnv->szTag, pTstEnv->u.Host.szPathTestSetValKit);
                    }
                }
                else
                    rc = VERR_BUFFER_OVERFLOW;

                if (   RT_SUCCESS(rc)
                    && !pTstEnv->fSkipVerify)
                {
                    rc = audioVerifyOne(pTstEnv->u.Host.szPathTestSetGuest, pTstEnv->u.Host.szPathTestSetValKit, NULL /* pOpts */);
                }
                else
                    RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Verification skipped\n");

                if (!pTstEnv->fSkipVerify)
                {
                    RTFileDelete(pTstEnv->u.Host.szPathTestSetGuest);
                    RTFileDelete(pTstEnv->u.Host.szPathTestSetValKit);
                }
                else
                    RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Leaving test set files behind\n");
            }
        }
    }
    else
        rc = VERR_NOT_IMPLEMENTED;

    /* Clean up. */
    RTDirRemove(pTstEnv->szPathTemp);
    RTDirRemove(pTstEnv->szPathOut);

    if (RT_FAILURE(rc))
        RTTestFailed(g_hTest, "Test worker failed with %Rrc", rc);

    return rc;
}

/** Option help for the 'test' command.   */
static DECLCALLBACK(const char *) audioTestCmdTestHelp(PCRTGETOPTDEF pOpt)
{
    switch (pOpt->iShort)
    {
        case 'a':                               return "Exclude all tests from the list (useful to enable single tests later with --include)";
        case 'b':                               return "The audio backend to use";
        case 'd':                               return "Go via DrvAudio instead of directly interfacing with the backend";
        case 'e':                               return "Exclude the given test id from the list";
        case 'i':                               return "Include the given test id in the list";
        case VKAT_TEST_OPT_COUNT:               return "Number of test iterations to perform for selected tests\n"
                                                       "    Default: random number";
        case VKAT_TEST_OPT_DEV:                 return "Name of the input/output device to use\n"
                                                       "    Default: default device";
        case VKAT_TEST_OPT_TONE_DURATION_MS:    return "Duration (in ms) of test tone to play / record for selected tests\n"
                                                       "    Default: random number";
        case VKAT_TEST_OPT_GUEST_ATS_ADDR:      return "Address of guest ATS to connect to\n"
                                                       "    Default: " ATS_TCP_DEF_CONNECT_GUEST_STR;
        case VKAT_TEST_OPT_GUEST_ATS_PORT:      return "Port of guest ATS to connect to (needs NAT port forwarding)\n"
                                                       "    Default: 6042"; /* ATS_TCP_DEF_CONNECT_PORT_GUEST */
        case VKAT_TEST_OPT_HOST_ATS_ADDR:       return "Address of host ATS to connect to\n"
                                                       "    Default: " ATS_TCP_DEF_CONNECT_HOST_ADDR_STR;
        case VKAT_TEST_OPT_HOST_ATS_PORT:       return "Port of host ATS to connect to\n"
                                                       "    Default: 6052"; /* ATS_TCP_DEF_BIND_PORT_VALKIT */
        case VKAT_TEST_OPT_MODE:                return "Specifies the test mode to use when running the tests";
        case VKAT_TEST_OPT_NO_VERIFY:           return "Skips the verification step";
        case VKAT_TEST_OPT_OUTDIR:              return "Specifies the output directory to use";
        case VKAT_TEST_OPT_PAUSE:               return "Not yet implemented";
        case VKAT_TEST_OPT_PCM_HZ:              return "Specifies the PCM Hetz (Hz) rate to use\n"
                                                       "    Default: 44100";
        case VKAT_TEST_OPT_PCM_BIT:             return "Specifies the PCM sample bits (i.e. 16) to use\n"
                                                       "    Default: 16";
        case VKAT_TEST_OPT_PCM_CHAN:            return "Specifies the number of PCM channels to use\n"
                                                       "    Default: 2";
        case VKAT_TEST_OPT_PCM_SIGNED:          return "Specifies whether to use signed (true) or unsigned (false) samples\n"
                                                       "    Default: true";
        case VKAT_TEST_OPT_PROBE_BACKENDS:      return "Specifies whether to probe all (available) backends until a working one is found\n"
                                                       "    Default: false";
        case VKAT_TEST_OPT_TAG:                 return "Specifies the test set tag to use";
        case VKAT_TEST_OPT_TEMPDIR:             return "Specifies the temporary directory to use";
        case VKAT_TEST_OPT_VOL:                 return "Specifies the audio volume (in percent, 0-100) to use";
        case VKAT_TEST_OPT_TCP_BIND_ADDRESS:    return "Specifies the TCP address listening to (server mode)";
        case VKAT_TEST_OPT_TCP_BIND_PORT:       return "Specifies the TCP port listening to (server mode)";
        case VKAT_TEST_OPT_TCP_CONNECT_ADDRESS: return "Specifies the TCP address to connect to (client mode)";
        case VKAT_TEST_OPT_TCP_CONNECT_PORT:    return "Specifies the TCP port to connect to (client mode)";
        default:
            break;
    }
    return NULL;
}

/**
 * Main (entry) function for the testing functionality of VKAT.
 *
 * @returns Program exit code.
 * @param   pGetState   RTGetOpt state.
 */
static DECLCALLBACK(RTEXITCODE) audioTestMain(PRTGETOPTSTATE pGetState)
{
    AUDIOTESTENV TstEnv;
    RT_ZERO(TstEnv);

    int         rc;

    PCPDMDRVREG pDrvReg       = AudioTestGetDefaultBackend();
    bool        fWithDrvAudio = false;
    uint8_t     cPcmSampleBit = 0;
    uint8_t     cPcmChannels  = 0;
    uint32_t    uPcmHz        = 0;
    bool        fPcmSigned    = true;
    bool        fProbeBackends = false;

    const char *pszGuestTcpAddr  = NULL;
    uint16_t    uGuestTcpPort    = ATS_TCP_DEF_BIND_PORT_GUEST;
    const char *pszValKitTcpAddr = NULL;
    uint16_t    uValKitTcpPort   = ATS_TCP_DEF_BIND_PORT_VALKIT;

    int           ch;
    RTGETOPTUNION ValueUnion;
    while ((ch = RTGetOpt(pGetState, &ValueUnion)))
    {
        switch (ch)
        {
            case 'a':
                for (unsigned i = 0; i < RT_ELEMENTS(g_aTests); i++)
                    g_aTests[i].fExcluded = true;
                break;

            case 'b':
                pDrvReg = AudioTestFindBackendOpt(ValueUnion.psz);
                if (pDrvReg == NULL)
                    return RTEXITCODE_SYNTAX;
                break;

            case 'd':
                fWithDrvAudio = true;
                break;

            case 'e':
                if (ValueUnion.u32 >= RT_ELEMENTS(g_aTests))
                    return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Invalid test number %u passed to --exclude", ValueUnion.u32);
                g_aTests[ValueUnion.u32].fExcluded = true;
                break;

            case VKAT_TEST_OPT_GUEST_ATS_ADDR:
                pszGuestTcpAddr = ValueUnion.psz;
                break;

            case VKAT_TEST_OPT_GUEST_ATS_PORT:
                uGuestTcpPort = ValueUnion.u32;
                break;

            case VKAT_TEST_OPT_HOST_ATS_ADDR:
                pszValKitTcpAddr = ValueUnion.psz;
                break;

            case VKAT_TEST_OPT_HOST_ATS_PORT:
                uValKitTcpPort = ValueUnion.u32;
                break;

            case VKAT_TEST_OPT_MODE:
                if (TstEnv.enmMode != AUDIOTESTMODE_UNKNOWN)
                    return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Test mode (guest / host) already specified");
                TstEnv.enmMode = RTStrICmp(ValueUnion.psz, "guest") == 0 ? AUDIOTESTMODE_GUEST : AUDIOTESTMODE_HOST;
                break;

            case VKAT_TEST_OPT_NO_VERIFY:
                TstEnv.fSkipVerify = true;
                break;

            case 'i':
                if (ValueUnion.u32 >= RT_ELEMENTS(g_aTests))
                    return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Invalid test number %u passed to --include", ValueUnion.u32);
                g_aTests[ValueUnion.u32].fExcluded = false;
                break;

            case VKAT_TEST_OPT_COUNT:
                TstEnv.cIterations = ValueUnion.u32;
                break;

            case VKAT_TEST_OPT_DEV:
                rc = RTStrCopy(TstEnv.szDev, sizeof(TstEnv.szDev), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExitFailure("Failed to copy out device: %Rrc", rc);
                break;

            case VKAT_TEST_OPT_TONE_DURATION_MS:
                TstEnv.cMsToneDuration = ValueUnion.u32;
                break;

            case VKAT_TEST_OPT_PAUSE:
                return RTMsgErrorExitFailure("Not yet implemented!");

            case VKAT_TEST_OPT_OUTDIR:
                rc = RTStrCopy(TstEnv.szPathOut, sizeof(TstEnv.szPathOut), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExitFailure("Failed to copy out directory: %Rrc", rc);
                break;

            case VKAT_TEST_OPT_PCM_BIT:
                cPcmSampleBit = ValueUnion.u8;
                break;

            case VKAT_TEST_OPT_PCM_CHAN:
                cPcmChannels = ValueUnion.u8;
                break;

            case VKAT_TEST_OPT_PCM_HZ:
                uPcmHz = ValueUnion.u32;
                break;

            case VKAT_TEST_OPT_PCM_SIGNED:
                fPcmSigned = ValueUnion.f;
                break;

            case VKAT_TEST_OPT_PROBE_BACKENDS:
                fProbeBackends = ValueUnion.f;
                break;

            case VKAT_TEST_OPT_TAG:
                rc = RTStrCopy(TstEnv.szTag, sizeof(TstEnv.szTag), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExit(RTEXITCODE_FAILURE, "Tag invalid, rc=%Rrc", rc);
                break;

            case VKAT_TEST_OPT_TEMPDIR:
                rc = RTStrCopy(TstEnv.szPathTemp, sizeof(TstEnv.szPathTemp), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExit(RTEXITCODE_FAILURE, "Temp dir invalid, rc=%Rrc", rc);
                break;

            case VKAT_TEST_OPT_VOL:
                TstEnv.uVolumePercent = ValueUnion.u8;
                break;

            case VKAT_TEST_OPT_TCP_BIND_ADDRESS:
                rc = RTStrCopy(TstEnv.TcpOpts.szBindAddr, sizeof(TstEnv.TcpOpts.szBindAddr), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExit(RTEXITCODE_FAILURE, "Bind address invalid, rc=%Rrc", rc);
                break;

            case VKAT_TEST_OPT_TCP_BIND_PORT:
                TstEnv.TcpOpts.uBindPort = ValueUnion.u16;
                break;

            case VKAT_TEST_OPT_TCP_CONNECT_ADDRESS:
                rc = RTStrCopy(TstEnv.TcpOpts.szConnectAddr, sizeof(TstEnv.TcpOpts.szConnectAddr), ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return RTMsgErrorExit(RTEXITCODE_FAILURE, "Connect address invalid, rc=%Rrc", rc);
                break;

            case VKAT_TEST_OPT_TCP_CONNECT_PORT:
                TstEnv.TcpOpts.uConnectPort = ValueUnion.u16;
                break;

            AUDIO_TEST_COMMON_OPTION_CASES(ValueUnion);

            default:
                return RTGetOptPrintError(ch, &ValueUnion);
        }
    }

    /*
     * Start testing.
     */
    RTTestBanner(g_hTest);

    /* Initialize the custom test parameters with sensible defaults if nothing else is given. */
    PDMAudioPropsInit(&TstEnv.Props,
                      cPcmSampleBit ? cPcmSampleBit / 8 : 2 /* 16-bit */, fPcmSigned, cPcmChannels ? cPcmChannels : 2,
                      uPcmHz ? uPcmHz : 44100);

    if (TstEnv.enmMode == AUDIOTESTMODE_UNKNOWN)
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "No test mode (--mode) specified!\n");

    /* Validate TCP options. */
    if (   TstEnv.TcpOpts.szBindAddr[0]
        && TstEnv.TcpOpts.szConnectAddr[0])
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Only one TCP connection mode (connect as client *or* bind as server) can be specified) at a time!\n");

    AUDIOTESTDRVSTACK DrvStack;
    if (fProbeBackends)
        rc = audioTestDriverStackProbe(&DrvStack, pDrvReg,
                                       true /* fEnabledIn */, true /* fEnabledOut */, fWithDrvAudio); /** @todo Make in/out configurable, too. */
    else
        rc = audioTestDriverStackInitEx(&DrvStack, pDrvReg,
                                        true /* fEnabledIn */, true /* fEnabledOut */, fWithDrvAudio); /** @todo Make in/out configurable, too. */
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Unable to init driver stack: %Rrc\n", rc);

    PPDMAUDIOHOSTDEV pDev;
    rc = audioTestDevicesEnumerateAndCheck(&DrvStack, TstEnv.szDev, &pDev);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_SYNTAX, "Enumerating device(s) failed: %Rrc\n", rc);

    /* For now all tests have the same test environment and driver stack. */
    rc = audioTestEnvInit(&TstEnv, &DrvStack);
    if (RT_SUCCESS(rc))
        rc = audioTestWorker(&TstEnv);

    audioTestEnvDestroy(&TstEnv);
    audioTestDriverStackDelete(&DrvStack);

    if (RT_FAILURE(rc)) /* Let us know that something went wrong in case we forgot to mention it. */
        RTTestFailed(g_hTest, "Testing failed with %Rrc\n", rc);

    /*
     * Print summary and exit.
     */
    return RTTestSummaryAndDestroy(g_hTest);
}


const VKATCMD g_CmdTest =
{
    "test",
    audioTestMain,
    "Runs audio tests and creates an audio test set.",
    g_aCmdTestOptions,
    RT_ELEMENTS(g_aCmdTestOptions),
    audioTestCmdTestHelp,
    true /* fNeedsTransport */
};


/*********************************************************************************************************************************
*   Command: verify                                                                                                              *
*********************************************************************************************************************************/

static int audioVerifyOpenTestSet(const char *pszPathSet, PAUDIOTESTSET pSet)
{
    int rc;

    char szPathExtracted[RTPATH_MAX];

    RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Opening test set '%s'\n", pszPathSet);

    const bool fPacked = AudioTestSetIsPacked(pszPathSet);

    if (fPacked)
    {
        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Test set is an archive and needs to be unpacked\n");

        if (!RTFileExists(pszPathSet))
        {
            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Test set '%s' does not exist\n", pszPathSet);
            rc = VERR_FILE_NOT_FOUND;
        }
        else
            rc = VINF_SUCCESS;

        if (RT_SUCCESS(rc))
        {
            char szPathTemp[RTPATH_MAX];
            rc = RTPathTemp(szPathTemp, sizeof(szPathTemp));
            if (RT_SUCCESS(rc))
            {
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Using temporary directory '%s'\n", szPathTemp);

                rc = RTPathJoin(szPathExtracted, sizeof(szPathExtracted), szPathTemp, "vkat-testset-XXXX");
                if (RT_SUCCESS(rc))
                {
                    rc = RTDirCreateTemp(szPathExtracted, 0755);
                    if (RT_SUCCESS(rc))
                    {
                        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Unpacking archive to '%s'\n", szPathExtracted);
                        rc = AudioTestSetUnpack(pszPathSet, szPathExtracted);
                        if (RT_SUCCESS(rc))
                            RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Archive successfully unpacked\n");
                    }
                }
            }
        }
    }
    else
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
        rc = AudioTestSetOpen(pSet, fPacked ? szPathExtracted : pszPathSet);

    if (RT_FAILURE(rc))
        RTTestFailed(g_hTest, "Unable to open / unpack test set archive: %Rrc", rc);

    return rc;
}

/**
 * Verifies one test set pair.
 *
 * @returns VBox status code.
 * @param   pszPathSetA         Absolute path to test set A.
 * @param   pszPathSetB         Absolute path to test set B.
 * @param   pOpts               Verification options to use. Optional.
 *                              When NULL, the (very strict) defaults will be used.
 */
static int audioVerifyOne(const char *pszPathSetA, const char *pszPathSetB, PAUDIOTESTVERIFYOPTS pOpts)
{
    RTTestSubF(g_hTest, "Verifying");
    RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Verifying test set '%s' with test set '%s'\n", pszPathSetA, pszPathSetB);

    AUDIOTESTSET SetA, SetB;
    int rc = audioVerifyOpenTestSet(pszPathSetA, &SetA);
    if (RT_SUCCESS(rc))
    {
        rc = audioVerifyOpenTestSet(pszPathSetB, &SetB);
        if (RT_SUCCESS(rc))
        {
            AUDIOTESTERRORDESC errDesc;
            if (pOpts)
                rc = AudioTestSetVerifyEx(&SetA, &SetB, pOpts, &errDesc);
            else
                rc = AudioTestSetVerify(&SetA, &SetB, &errDesc);
            if (RT_SUCCESS(rc))
            {
                uint32_t const cErr = AudioTestErrorDescCount(&errDesc);
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "%RU32 errors occurred while verifying\n", cErr);

                /** @todo Use some AudioTestErrorXXX API for enumeration here later. */
                PAUDIOTESTERRORENTRY pErrEntry;
                RTListForEach(&errDesc.List, pErrEntry, AUDIOTESTERRORENTRY, Node)
                {
                    if (RT_FAILURE(pErrEntry->rc))
                        RTTestFailed(g_hTest, "%s\n", pErrEntry->szDesc);
                    else
                        RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "%s\n", pErrEntry->szDesc);
                }

                if (cErr == 0)
                    RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS, "Verification successful\n");

                AudioTestErrorDescDestroy(&errDesc);
            }
            else
                RTTestFailed(g_hTest, "Verification failed with %Rrc", rc);

#ifdef DEBUG
            if (g_fDrvAudioDebug)
                RTTestPrintf(g_hTest, RTTESTLVL_ALWAYS,
                             "\n"
                             "Use the following command line to re-run verification in the debugger:\n"
                             "gdb --args ./VBoxAudioTest -vvvv --debug-audio verify \"%s\" \"%s\"\n",
                             SetA.szPathAbs, SetB.szPathAbs);
#endif
            if (!g_fDrvAudioDebug) /* Don't wipe stuff when debugging. Can be useful for introspecting data. */
                AudioTestSetWipe(&SetB);
            AudioTestSetClose(&SetB);
        }

        if (!g_fDrvAudioDebug) /* Ditto. */
            AudioTestSetWipe(&SetA);
        AudioTestSetClose(&SetA);
    }

    RTTestSubDone(g_hTest);

    return rc;
}

/**
 * Main (entry) function for the verification functionality of VKAT.
 *
 * @returns Program exit code.
 * @param   pGetState   RTGetOpt state.
 */
static DECLCALLBACK(RTEXITCODE) audioVerifyMain(PRTGETOPTSTATE pGetState)
{
    /*
     * Parse options and process arguments.
     */
    const char *apszSets[2] = { NULL, NULL };
    unsigned    iTestSet    = 0;

    AUDIOTESTVERIFYOPTS Opts;
    AudioTestSetVerifyOptsInit(&Opts);

    int           ch;
    RTGETOPTUNION ValueUnion;
    while ((ch = RTGetOpt(pGetState, &ValueUnion)))
    {
        switch (ch)
        {
            case VKAT_VERIFY_OPT_MAX_DIFF_COUNT:
                Opts.cMaxDiff = ValueUnion.u32;
                break;

            case VKAT_VERIFY_OPT_MAX_DIFF_PERCENT:
                Opts.uMaxDiffPercent = ValueUnion.u8;
                break;

            case VKAT_VERIFY_OPT_MAX_SIZE_PERCENT:
                Opts.uMaxSizePercent = ValueUnion.u8;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (iTestSet == 0)
                    RTTestBanner(g_hTest);
                if (iTestSet >= RT_ELEMENTS(apszSets))
                    return RTMsgErrorExitFailure("Only two test sets can be verified at one time");
                apszSets[iTestSet++] = ValueUnion.psz;
                break;

            AUDIO_TEST_COMMON_OPTION_CASES(ValueUnion);

            default:
                return RTGetOptPrintError(ch, &ValueUnion);
        }
    }

    if (!iTestSet)
        return RTMsgErrorExitFailure("At least one test set must be specified");

    int rc = VINF_SUCCESS;

    /*
     * If only test set A is given, default to the current directory
     * for test set B.
     */
    char szDirCur[RTPATH_MAX];
    if (iTestSet == 1)
    {
        rc = RTPathGetCurrent(szDirCur, sizeof(szDirCur));
        if (RT_SUCCESS(rc))
            apszSets[1] = szDirCur;
        else
            RTTestFailed(g_hTest, "Failed to retrieve current directory: %Rrc", rc);
    }

    if (RT_SUCCESS(rc))
        audioVerifyOne(apszSets[0], apszSets[1], &Opts);

    /*
     * Print summary and exit.
     */
    return RTTestSummaryAndDestroy(g_hTest);
}


const VKATCMD g_CmdVerify =
{
    "verify",
    audioVerifyMain,
    "Verifies a formerly created audio test set.",
    g_aCmdVerifyOptions,
    RT_ELEMENTS(g_aCmdVerifyOptions),
    NULL,
    false /* fNeedsTransport */
};


/*********************************************************************************************************************************
*   Main                                                                                                                         *
*********************************************************************************************************************************/

/**
 * Ctrl-C signal handler.
 *
 * This just sets g_fTerminate and hope it will be noticed soon.  It restores
 * the SIGINT action to default, so that a second Ctrl-C will have the normal
 * effect (just in case the code doesn't respond to g_fTerminate).
 */
static void audioTestSignalHandler(int iSig) RT_NOEXCEPT
{
    Assert(iSig == SIGINT); RT_NOREF(iSig);
    RTPrintf("Ctrl-C!\n");
    ASMAtomicWriteBool(&g_fTerminate, true);
    signal(SIGINT, SIG_DFL);
}

/**
 * Commands.
 */
const VKATCMD *g_apCommands[] =
{
    &g_CmdTest,
    &g_CmdVerify,
    &g_CmdEnum,
    &g_CmdPlay,
    &g_CmdRec,
    &g_CmdSelfTest
};

/**
 * Shows tool usage text.
 */
RTEXITCODE audioTestUsage(PRTSTREAM pStrm)
{
    RTStrmPrintf(pStrm, "usage: %s [global options] <command> [command-options]\n",
                 RTPathFilename(RTProcExecutablePath()));
    RTStrmPrintf(pStrm,
                 "\n"
                 "Global Options:\n"
                 "  --debug-audio\n"
                 "    Enables (DrvAudio) debugging.\n"
                 "  --debug-audio-path=<path>\n"
                 "    Tells DrvAudio where to put its debug output (wav-files).\n"
                 "  -q, --quiet\n"
                 "    Sets verbosity to zero.\n"
                 "  -v, --verbose\n"
                 "    Increase verbosity.\n"
                 "  -V, --version\n"
                 "    Displays version.\n"
                 "  -h, -?, --help\n"
                 "    Displays help.\n"
                 );

    for (uintptr_t iCmd = 0; iCmd < RT_ELEMENTS(g_apCommands); iCmd++)
    {
        PCVKATCMD const pCmd = g_apCommands[iCmd];
        RTStrmPrintf(pStrm,
                     "\n"
                     "Command '%s':\n"
                     "    %s\n"
                     "Options for '%s':\n",
                     pCmd->pszCommand, pCmd->pszDesc, pCmd->pszCommand);
        PCRTGETOPTDEF const paOptions = pCmd->paOptions;
        for (unsigned i = 0; i < pCmd->cOptions; i++)
        {
            if (RT_C_IS_PRINT(paOptions[i].iShort))
                RTStrmPrintf(pStrm, "  -%c, %s\n", paOptions[i].iShort, paOptions[i].pszLong);
            else
                RTStrmPrintf(pStrm, "  %s\n", paOptions[i].pszLong);

            const char *pszHelp = NULL;
            if (pCmd->pfnOptionHelp)
                pszHelp = pCmd->pfnOptionHelp(&paOptions[i]);
            if (pszHelp)
                RTStrmPrintf(pStrm, "    %s\n", pszHelp);
        }

        if (pCmd->fNeedsTransport)
        {
            for (uintptr_t iTx = 0; iTx < g_cTransports; iTx++)
                g_apTransports[iTx]->pfnUsage(pStrm);
        }
    }

    return RTEXITCODE_SUCCESS;
}

/**
 * Shows tool version.
 */
RTEXITCODE audioTestVersion(void)
{
    RTPrintf("%s\n", RTBldCfgRevisionStr());
    return RTEXITCODE_SUCCESS;
}

/**
 * Shows the logo.
 *
 * @param   pStream             Output stream to show logo on.
 */
void audioTestShowLogo(PRTSTREAM pStream)
{
    RTStrmPrintf(pStream, VBOX_PRODUCT " VKAT (Validation Kit Audio Test) Version " VBOX_VERSION_STRING " - r%s\n"
                 "(C) " VBOX_C_YEAR " " VBOX_VENDOR "\n"
                 "All rights reserved.\n\n", RTBldCfgRevisionStr());
}

int main(int argc, char **argv)
{
    /*
     * Init IPRT.
     */
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
    {
        RTPrintf("RTR3InitExe() failed with %Rrc\n", rc);
        return RTMsgInitFailure(rc);
    }

    /*
     * Handle special command line options which need parsing before
     * everything else.
     */
    bool fDaemonize  = false;
    bool fDaemonized = false;

    RTGETOPTSTATE GetState;
    rc = RTGetOptInit(&GetState, argc, argv, g_aCmdCommonOptions,
                      RT_ELEMENTS(g_aCmdCommonOptions), 1 /*idxFirst*/, 0 /*fFlags - must not sort! */);
    AssertRCReturn(rc, RTEXITCODE_INIT);

    int           ch;
    RTGETOPTUNION ValueUnion;
    while ((ch = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (ch)
        {
            case AUDIO_TEST_OPT_CMN_DAEMONIZE:
                fDaemonize = true;
                break;

            case AUDIO_TEST_OPT_CMN_DAEMONIZED:
                fDaemonized = true;
                break;

            case 'v':
                g_uVerbosity++;
                break;

            default:
                break;
        }
    }

    audioTestShowLogo(g_pStdOut);

    if (fDaemonize)
    {
        if (!fDaemonized)
        {
            rc = RTProcDaemonize(argv, "--daemonized");
            if (RT_FAILURE(rc))
                return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTProcDaemonize() failed with %Rrc\n", rc);

            RTMsgInfo("Starting in background (daemonizing) ...");
            return RTEXITCODE_SUCCESS;
        }
        /* else continue running in background. */
    }

    /*
     * Init test and globals.
     * Note: Needs to be done *after* daemonizing, otherwise the child will fail!
     */
    rc = RTTestCreate("AudioTest", &g_hTest);
    if (RT_FAILURE(rc))
        return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTTestCreate() failed with %Rrc\n", rc);

#ifdef RT_OS_WINDOWS
    HRESULT hrc = CoInitializeEx(NULL /*pReserved*/, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hrc))
        RTMsgWarning("CoInitializeEx failed: %#x", hrc);
#endif

    /*
     * Configure release logging to go to stderr.
     */
    RTUINT fFlags = RTLOGFLAGS_PREFIX_THREAD | RTLOGFLAGS_PREFIX_TIME_PROG;
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    fFlags |= RTLOGFLAGS_USECRLF;
#endif
    static const char * const g_apszLogGroups[] = VBOX_LOGGROUP_NAMES;
    rc = RTLogCreate(&g_pRelLogger, fFlags, "all.e.l", "VKAT_RELEASE_LOG",
                     RT_ELEMENTS(g_apszLogGroups), g_apszLogGroups, RTLOGDEST_STDOUT, NULL /*"vkat-release.log"*/);
    if (RT_SUCCESS(rc))
    {
        RTLogRelSetDefaultInstance(g_pRelLogger);
        if (g_uVerbosity)
        {
            RTMsgInfo("Setting verbosity logging to level %u\n", g_uVerbosity);
            switch (g_uVerbosity) /* Not very elegant, but has to do it for now. */
            {
                case 1:
                    rc = RTLogGroupSettings(g_pRelLogger,
                                            "drv_audio.e.l+audio_mixer.e.l+audio_test.e.l");
                    break;

                case 2:
                    rc = RTLogGroupSettings(g_pRelLogger,
                                            "drv_audio.e.l.l2+audio_mixer.e.l.l2+audio_test.e.l.l2");
                    break;

                case 3:
                    rc = RTLogGroupSettings(g_pRelLogger,
                                            "drv_audio.e.l.l2.l3+audio_mixer.e.l.l2.l3+audio_test.e.l.l2.l3");
                    break;

                case 4:
                    RT_FALL_THROUGH();
                default:
                    rc = RTLogGroupSettings(g_pRelLogger,
                                            "drv_audio.e.l.l2.l3.f+audio_mixer.e.l.l2.l3.f+audio_test.e.l.l2.l3.f");
                    break;
            }
            if (RT_FAILURE(rc))
                RTMsgError("Setting debug logging failed, rc=%Rrc\n", rc);
        }
    }
    else
        RTMsgWarning("Failed to create release logger: %Rrc", rc);

    /*
     * Install a Ctrl-C signal handler.
     */
#ifdef RT_OS_WINDOWS
    signal(SIGINT, audioTestSignalHandler);
#else
    struct sigaction sa;
    RT_ZERO(sa);
    sa.sa_handler = audioTestSignalHandler;
    sigaction(SIGINT, &sa, NULL);
#endif

    /*
     * Process common options.
     */
    RT_ZERO(GetState);
    rc = RTGetOptInit(&GetState, argc, argv, g_aCmdCommonOptions,
                      RT_ELEMENTS(g_aCmdCommonOptions), 1 /*idxFirst*/, 0 /*fFlags - must not sort! */);
    AssertRCReturn(rc, RTEXITCODE_INIT);

    while ((ch = RTGetOpt(&GetState, &ValueUnion)) != 0)
    {
        switch (ch)
        {
            AUDIO_TEST_COMMON_OPTION_CASES(ValueUnion);

            case VINF_GETOPT_NOT_OPTION:
            {
                for (uintptr_t iCmd = 0; iCmd < RT_ELEMENTS(g_apCommands); iCmd++)
                {
                    PCVKATCMD const pCmd = g_apCommands[iCmd];
                    if (strcmp(ValueUnion.psz, pCmd->pszCommand) == 0)
                    {
                        size_t cCombinedOptions  = pCmd->cOptions + RT_ELEMENTS(g_aCmdCommonOptions);
                        if (pCmd->fNeedsTransport)
                        {
                            for (uintptr_t iTx = 0; iTx < g_cTransports; iTx++)
                                cCombinedOptions += g_apTransports[iTx]->cOpts;
                        }
                        PRTGETOPTDEF paCombinedOptions = (PRTGETOPTDEF)RTMemAlloc(cCombinedOptions * sizeof(RTGETOPTDEF));
                        if (paCombinedOptions)
                        {
                            uint32_t idxOpts = 0;
                            memcpy(paCombinedOptions, g_aCmdCommonOptions, sizeof(g_aCmdCommonOptions));
                            idxOpts += RT_ELEMENTS(g_aCmdCommonOptions);
                            memcpy(&paCombinedOptions[idxOpts], pCmd->paOptions, pCmd->cOptions * sizeof(RTGETOPTDEF));
                            idxOpts += (uint32_t)pCmd->cOptions;
                            if (pCmd->fNeedsTransport)
                            {
                                for (uintptr_t iTx = 0; iTx < g_cTransports; iTx++)
                                {
                                    memcpy(&paCombinedOptions[idxOpts],
                                           g_apTransports[iTx]->paOpts, g_apTransports[iTx]->cOpts * sizeof(RTGETOPTDEF));
                                    idxOpts += (uint32_t)g_apTransports[iTx]->cOpts;
                                }
                            }

                            rc = RTGetOptInit(&GetState, argc, argv, paCombinedOptions, cCombinedOptions,
                                              GetState.iNext /*idxFirst*/, RTGETOPTINIT_FLAGS_OPTS_FIRST);
                            if (RT_SUCCESS(rc))
                            {
                                RTEXITCODE rcExit = pCmd->pfnHandler(&GetState);
                                RTMemFree(paCombinedOptions);
                                return rcExit;
                            }
                            return RTMsgErrorExitFailure("RTGetOptInit failed for '%s': %Rrc", ValueUnion.psz, rc);
                        }
                        return RTMsgErrorExitFailure("Out of memory!");
                    }
                }
                RTMsgError("Unknown command '%s'!\n", ValueUnion.psz);
                audioTestUsage(g_pStdErr);
                return RTEXITCODE_SYNTAX;
            }

            default:
                return RTGetOptPrintError(ch, &ValueUnion);
        }
    }

    RTMsgError("No command specified!\n");
    audioTestUsage(g_pStdErr);
    return RTEXITCODE_SYNTAX;
}

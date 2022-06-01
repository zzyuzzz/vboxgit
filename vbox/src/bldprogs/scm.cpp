/* $Id: scm.cpp 26347 2010-02-09 04:26:18Z vboxsync $ */
/** @file
 * IPRT Testcase / Tool - Source Code Massager.
 */

/*
 * Copyright (C) 2010 Sun Microsystems, Inc.
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
#include <iprt/assert.h>
#include <iprt/ctype.h>
#include <iprt/dir.h>
#include <iprt/file.h>
#include <iprt/err.h>
#include <iprt/getopt.h>
#include <iprt/initterm.h>
#include <iprt/mem.h>
#include <iprt/message.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
/** End of line marker type. */
typedef enum SCMEOL
{
    SCMEOL_NONE = 0,
    SCMEOL_LF = 1,
    SCMEOL_CRLF = 2
} SCMEOL;
/** Pointer to an end of line marker type. */
typedef SCMEOL *PSCMEOL;

/**
 * Line record.
 */
typedef struct SCMSTREAMLINE
{
    /** The offset of the line. */
    size_t          off;
    /** The line length, excluding the LF character.
     * @todo This could be derived from the offset of the next line if that wasn't
     *       so tedious. */
    size_t          cch;
    /** The end of line marker type. */
    SCMEOL          enmEol;
} SCMSTREAMLINE;
/** Pointer to a line record. */
typedef SCMSTREAMLINE *PSCMSTREAMLINE;

/**
 * Source code massager stream.
 */
typedef struct SCMSTREAM
{
    /** Pointer to the file memory. */
    char           *pch;
    /** The current stream position. */
    size_t          off;
    /** The current stream size. */
    size_t          cb;
    /** The size of the memory pb points to. */
    size_t          cbAllocated;

    /** Line records. */
    PSCMSTREAMLINE  paLines;
    /** The current line. */
    size_t          iLine;
    /** The current stream size given in lines.   */
    size_t          cLines;
    /** The sizeof the the memory backing paLines.   */
    size_t          cLinesAllocated;

    /** Set if write-only, clear if read-only. */
    bool            fWriteOrRead;
    /** Set if the memory pb points to is from RTFileReadAll. */
    bool            fFileMemory;
    /** Set if fully broken into lines. */
    bool            fFullyLineated;

    /** Stream status code (IPRT). */
    int             rc;
} SCMSTREAM;
/** Pointer to a SCM stream. */
typedef SCMSTREAM *PSCMSTREAM;
/** Pointer to a const SCM stream. */
typedef SCMSTREAM const *PCSCMSTREAM;

/**
 * A rewriter.
 *
 * This works like a stream editor, reading @a pIn, modifying it and writing it
 * to @a pOut.
 *
 * @returns true if any changes were made, false if not.
 * @param   pIn         The input stream.
 * @param   pOut        The output stream.
 */
typedef bool (*PFNSCMREWRITER)(PSCMSTREAM pIn, PSCMSTREAM pOut);


/**
 * Configuration entry.
 */
typedef struct SCMCFGENTRY
{
    /** Number of rewriters. */
    size_t          cRewriters;
    /** Pointer to an array of rewriters. */
    PFNSCMREWRITER const  *papfnRewriter;
    /** File pattern (simple).  */
    const char     *pszFilePattern;
} SCMCFGENTRY;
typedef SCMCFGENTRY *PSCMCFGENTRY;
typedef SCMCFGENTRY const *PCSCMCFGENTRY;


/**
 * Diff state.
 */
typedef struct SCMDIFFSTATE
{
    size_t          cDiffs;
    const char     *pszFilename;

    PSCMSTREAM      pLeft;
    PSCMSTREAM      pRight;

    /** Whether to ignore end of line markers when diffing. */
    bool            fIgnoreEol;
    /** Whether to ignore trailing whitespace. */
    bool            fIgnoreTrailingWhite;
    /** Whether to ignore leading whitespace. */
    bool            fIgnoreLeadingWhite;
    /** Whether to print special characters in human readable form or not. */
    bool            fSpecialChars;
    /** Where to push the diff. */
    PRTSTREAM       pDiff;
} SCMDIFFSTATE;
/** Pointer to a diff state. */
typedef SCMDIFFSTATE *PSCMDIFFSTATE;


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static bool rewrite_StripTrailingBlanks(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_ExpandTabs(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_ForceNativeEol(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_ForceLF(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_ForceCRLF(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_AdjustTrailingLines(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_Makefile_kup(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_Makefile_kmk(PSCMSTREAM pIn, PSCMSTREAM pOut);
static bool rewrite_C_and_CPP(PSCMSTREAM pIn, PSCMSTREAM pOut);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
static const char   g_szProgName[]          = "scm";
static const char  *g_pszChangedSuff        = "";
static bool         g_fDryRun               = true;
static bool         g_fStripTrailingBlanks  = true;
static bool         g_fStripTrailingLines   = true;
static bool         g_fForceFinalEol        = true;
static bool         g_fForceTrailingLine    = false;
static bool         g_fConvertTabs          = true;
static unsigned     g_cchTab                = 8;
static const char   g_szTabSpaces[16+1]     = "                ";
static bool         g_fConvertEol           = true;
static bool         g_fDiffSpecialChars     = true;
static bool         g_fDiffIgnoreEol        = false;
static bool         g_fDiffIgnoreLeadingWS  = false;
static bool         g_fDiffIgnoreTrailingWS = false;
static int          g_iVerbosity            = 2;//99; //0;
static const char  *g_pszFileFilter         = "*";
static const char  *g_pszFileFilterOut      =
    "*.exe|"
    "*.com|"
    "20*-*-*.log|"
    "*/src/VBox/Runtime/testcase/soundcard.h|"
    "*/src/VBox/Runtime/include/internal/ldrELF*.h|"
    "*/src/VBox/Runtime/common/math/x86/fenv-x86.c|"
    "*/src/VBox/Runtime/common/checksum/md5.cpp|"
    "/dummy/."
;
static const char  *g_pszDirFilter          = NULL;
static const char  *g_pszDirFilterOut       =
    // generic
    ".svn|"
    "CVS|"
    // root
    "out|"
    "tools|"
    "webtools|"
    "kBuild|"
    "debian|"
    "SlickEdit|"
    // src
    "*/src/libs/.|"
    "*/src/apps/.|"
    "*/src/VBox/Frontends/.|"
    "*/src/VBox/Additions/x11/x11include/.|"
    "*/src/VBox/Additions/WINNT/Graphics/Wine/.|"
    "*/src/VBox/Additions/common/crOpenGL/.|"
    "*/src/VBox/HostServices/SharedOpenGL/.|"
    "*/src/VBox/GuestHost/OpenGL/*/.|"
    "*/src/VBox/Devices/PC/Etherboot-src/*/.|"
    "*/src/VBox/Devices/Network/lwip/.|"
    "*/src/VBox/Devices/Storage/VBoxHDDFormats/StorageCraft/*/.|"
    "*/src/VBox/Runtime/r0drv/solaris/vbi/*/.|"
    "*/src/VBox/Runtime/common/math/gcc/.|"
    "/dummy"
;

static PFNSCMREWRITER const g_aRewritersFor_Makefile_kup[] =
{
    rewrite_Makefile_kup
};

static PFNSCMREWRITER const g_aRewritersFor_Makefile_kmk[] =
{
    rewrite_ForceNativeEol,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_Makefile_kmk
};

static PFNSCMREWRITER const g_aRewritersFor_C_and_CPP[] =
{
    rewrite_ForceNativeEol,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks,
    rewrite_AdjustTrailingLines,
    rewrite_C_and_CPP
};

static PFNSCMREWRITER const g_aRewritersFor_ShellScripts[] =
{
    rewrite_ForceLF,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks
};

static PFNSCMREWRITER const g_aRewritersFor_BatchFiles[] =
{
    rewrite_ForceCRLF,
    rewrite_ExpandTabs,
    rewrite_StripTrailingBlanks
};

static SCMCFGENTRY const g_aConfigs[] =
{
    { RT_ELEMENTS(g_aRewritersFor_Makefile_kup), &g_aRewritersFor_Makefile_kup[0], "Makefile.kup" },
    { RT_ELEMENTS(g_aRewritersFor_Makefile_kmk), &g_aRewritersFor_Makefile_kmk[0], "Makefile.kmk" },
    { RT_ELEMENTS(g_aRewritersFor_C_and_CPP),    &g_aRewritersFor_C_and_CPP[0],    "*.c|*.h|*.cpp|*.hpp|*.C|*.CPP|*.cxx|*.cc" },
    { RT_ELEMENTS(g_aRewritersFor_ShellScripts), &g_aRewritersFor_ShellScripts[0], "*.sh|configure" },
    { RT_ELEMENTS(g_aRewritersFor_BatchFiles),   &g_aRewritersFor_BatchFiles[0],   "*.bat|*.cmd|*.btm|*.vbs|*.ps1" },
};


/* -=-=-=-=-=- memory streams -=-=-=-=-=- */


/**
 * Initializes the stream structure.
 *
 * @param   pStream             The stream structure.
 * @param   fWriteOrRead        The value of the fWriteOrRead stream member.
 */
static void scmStreamInitInternal(PSCMSTREAM pStream, bool fWriteOrRead)
{
    pStream->pch                = NULL;
    pStream->off                = 0;
    pStream->cb                 = 0;
    pStream->cbAllocated        = 0;

    pStream->paLines            = NULL;
    pStream->iLine              = 0;
    pStream->cLines             = 0;
    pStream->cLinesAllocated    = 0;

    pStream->fWriteOrRead       = fWriteOrRead;
    pStream->fFileMemory        = false;
    pStream->fFullyLineated     = false;

    pStream->rc                 = VINF_SUCCESS;
}

/**
 * Initialize an input stream.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream to initialize.
 * @param   pszFilename         The file to take the stream content from.
 */
int ScmStreamInitForReading(PSCMSTREAM pStream, const char *pszFilename)
{
    scmStreamInitInternal(pStream, false /*fWriteOrRead*/);

    void *pvFile;
    size_t cbFile;
    int rc = pStream->rc = RTFileReadAll(pszFilename, &pvFile, &cbFile);
    if (RT_SUCCESS(rc))
    {
        pStream->pch            = (char *)pvFile;
        pStream->cb             = cbFile;
        pStream->cbAllocated    = cbFile;
        pStream->fFileMemory    = true;
    }
    return rc;
}

/**
 * Initialize an output stream.
 *
 * @returns IPRT status code
 * @param   pStream             The stream to initialize.
 * @param   pRelatedStream      Pointer to a related stream.  NULL is fine.
 */
int ScmStreamInitForWriting(PSCMSTREAM pStream, PCSCMSTREAM pRelatedStream)
{
    scmStreamInitInternal(pStream, true /*fWriteOrRead*/);

    /* allocate stuff */
    size_t cbEstimate = pRelatedStream
                      ? pRelatedStream->cb + pRelatedStream->cb / 10
                      : _64K;
    cbEstimate = RT_ALIGN(cbEstimate, _4K);
    pStream->pch = (char *)RTMemAlloc(cbEstimate);
    if (pStream->pch)
    {
        size_t cLinesEstimate = pRelatedStream && pRelatedStream->fFullyLineated
                              ? pRelatedStream->cLines + pRelatedStream->cLines / 10
                              : cbEstimate / 24;
        cLinesEstimate = RT_ALIGN(cLinesEstimate, 512);
        pStream->paLines = (PSCMSTREAMLINE)RTMemAlloc(cLinesEstimate * sizeof(SCMSTREAMLINE));
        if (pStream->paLines)
        {
            pStream->paLines[0].off     = 0;
            pStream->paLines[0].cch     = 0;
            pStream->paLines[0].enmEol  = SCMEOL_NONE;
            pStream->cbAllocated        = cbEstimate;
            pStream->cLinesAllocated    = cLinesEstimate;
            return VINF_SUCCESS;
        }

        RTMemFree(pStream->pch);
        pStream->pch = NULL;
    }
    return pStream->rc = VERR_NO_MEMORY;
}

/**
 * Frees the resources associated with the stream.
 *
 * Nothing is happens to whatever the stream was initialized from or dumped to.
 *
 * @param   pStream             The stream to delete.
 */
void ScmStreamDelete(PSCMSTREAM pStream)
{
    if (pStream->pch)
    {
        if (pStream->fFileMemory)
            RTFileReadAllFree(pStream->pch, pStream->cbAllocated);
        else
            RTMemFree(pStream->pch);
        pStream->pch = NULL;
    }
    pStream->cbAllocated = 0;

    if (pStream->paLines)
    {
        RTMemFree(pStream->paLines);
        pStream->paLines = NULL;
    }
    pStream->cLinesAllocated = 0;
}

/**
 * Get the stream status code.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.
 */
int ScmStreamGetStatus(PCSCMSTREAM pStream)
{
    return pStream->rc;
}

/**
 * Grows the buffer of a write stream.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.  Must be in write mode.
 * @param   cbAppending         The minimum number of bytes to grow the buffer
 *                              with.
 */
static int scmStreamGrowBuffer(PSCMSTREAM pStream, size_t cbAppending)
{
    size_t cbAllocated = pStream->cbAllocated;
    cbAllocated += RT_MAX(0x1000 + cbAppending, cbAllocated);
    cbAllocated = RT_ALIGN(cbAllocated, 0x1000);
    void *pvNew;
    if (!pStream->fFileMemory)
    {
        pvNew = RTMemRealloc(pStream->pch, cbAllocated);
        if (!pvNew)
            return pStream->rc = VERR_NO_MEMORY;
    }
    else
    {
        pvNew = RTMemDupEx(pStream->pch, pStream->off, cbAllocated - pStream->off);
        if (!pvNew)
            return pStream->rc = VERR_NO_MEMORY;
        RTFileReadAllFree(pStream->pch, pStream->cbAllocated);
        pStream->fFileMemory = false;
    }
    pStream->pch = (char *)pvNew;
    pStream->cbAllocated = cbAllocated;

    return VINF_SUCCESS;
}

/**
 * Grows the line array of a stream.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.
 * @param   iMinLine            Minimum line number.
 */
static int scmStreamGrowLines(PSCMSTREAM pStream, size_t iMinLine)
{
    size_t cLinesAllocated = pStream->cLinesAllocated;
    cLinesAllocated += RT_MAX(512 + iMinLine, cLinesAllocated);
    cLinesAllocated = RT_ALIGN(cLinesAllocated, 512);
    void *pvNew = RTMemRealloc(pStream->paLines, cLinesAllocated * sizeof(SCMSTREAMLINE));
    if (!pvNew)
        return pStream->rc = VERR_NO_MEMORY;

    pStream->paLines = (PSCMSTREAMLINE)pvNew;
    pStream->cLinesAllocated = cLinesAllocated;
    return VINF_SUCCESS;
}

/**
 * Rewinds the stream and sets the mode to read.
 *
 * @param   pStream             The stream.
 */
void ScmStreamRewindForReading(PSCMSTREAM pStream)
{
    pStream->off          = 0;
    pStream->iLine        = 0;
    pStream->fWriteOrRead = false;
    pStream->rc           = VINF_SUCCESS;
}

/**
 * Rewinds the stream and sets the mode to write.
 *
 * @param   pStream             The stream.
 */
void ScmStreamRewindForWriting(PSCMSTREAM pStream)
{
    pStream->off            = 0;
    pStream->iLine          = 0;
    pStream->cLines         = 0;
    pStream->fWriteOrRead   = true;
    pStream->fFullyLineated = true;
    pStream->rc             = VINF_SUCCESS;
}

/**
 * Checks if it's a text stream.
 *
 * Not 100% proof.
 *
 * @returns true if it probably is a text file, false if not.
 * @param   pStream             The stream. Write or read, doesn't matter.
 */
bool ScmStreamIsText(PSCMSTREAM pStream)
{
    if (memchr(pStream->pch, '\0', pStream->cb))
        return false;
    if (!pStream->cb)
        return false;
    return true;
}

/**
 * Performs an integrity check of the stream.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.
 */
int ScmStreamCheckItegrity(PSCMSTREAM pStream)
{
    /*
     * Perform sanity checks.
     */
    size_t const cbFile = pStream->cb;
    for (size_t iLine = 0; iLine < pStream->cLines; iLine++)
    {
        size_t offEol = pStream->paLines[iLine].off + pStream->paLines[iLine].cch;
        AssertReturn(offEol + pStream->paLines[iLine].enmEol <= cbFile, VERR_INTERNAL_ERROR_2);
        switch (pStream->paLines[iLine].enmEol)
        {
            case SCMEOL_LF:
                AssertReturn(pStream->pch[offEol] == '\n', VERR_INTERNAL_ERROR_3);
                break;
            case SCMEOL_CRLF:
                AssertReturn(pStream->pch[offEol] == '\r', VERR_INTERNAL_ERROR_3);
                AssertReturn(pStream->pch[offEol + 1] == '\n', VERR_INTERNAL_ERROR_3);
                break;
            case SCMEOL_NONE:
                AssertReturn(iLine + 1 >= pStream->cLines, VERR_INTERNAL_ERROR_4);
                break;
            default:
                AssertReturn(iLine + 1 >= pStream->cLines, VERR_INTERNAL_ERROR_5);
        }
    }
    return VINF_SUCCESS;
}

/**
 * Writes the stream to a file.
 *
 * @returns IPRT status code
 * @param   pStream             The stream.
 * @param   pszFilenameFmt      The filename format string.
 * @param   ...                 Format arguments.
 */
int ScmStreamWriteToFile(PSCMSTREAM pStream, const char *pszFilenameFmt, ...)
{
    int rc;

#ifdef RT_STRICT
    /*
     * Check that what we're going to write makes sense first.
     */
    rc = ScmStreamCheckItegrity(pStream);
    if (RT_FAILURE(rc))
        return rc;
#endif

    /*
     * Do the actual writing.
     */
    RTFILE hFile;
    va_list va;
    va_start(va, pszFilenameFmt);
    rc = RTFileOpenV(&hFile, RTFILE_O_WRITE | RTFILE_O_CREATE_REPLACE | RTFILE_O_DENY_WRITE, pszFilenameFmt, va);
    if (RT_SUCCESS(rc))
    {
        rc = RTFileWrite(hFile, pStream->pch, pStream->cb, NULL);
        RTFileClose(hFile);
    }
    return rc;
}

/**
 * Worker for ScmStreamGetLine that builds the line number index while parsing
 * the stream.
 *
 * @returns Same as SCMStreamGetLine.
 * @param   pStream             The stream.  Must be in read mode.
 * @param   pcchLine            Where to return the line length.
 * @param   penmEol             Where to return the kind of end of line marker.
 */
static const char *scmStreamGetLineInternal(PSCMSTREAM pStream, size_t *pcchLine, PSCMEOL penmEol)
{
    AssertReturn(!pStream->fWriteOrRead, NULL);
    if (RT_FAILURE(pStream->rc))
        return NULL;

    size_t off = pStream->off;
    size_t cb  = pStream->cb;
    if (RT_UNLIKELY(off >= cb))
    {
        pStream->fFullyLineated = true;
        return NULL;
    }

    size_t iLine = pStream->iLine;
    if (RT_UNLIKELY(iLine >= pStream->cLinesAllocated))
    {
        int rc = scmStreamGrowLines(pStream, iLine);
        if (RT_FAILURE(rc))
            return NULL;
    }
    pStream->paLines[iLine].off = off;

    cb -= off;
    const char *pchRet = &pStream->pch[off];
    const char *pch = (const char *)memchr(pchRet, '\n', cb);
    if (RT_LIKELY(pch))
    {
        cb = pch - pchRet;
        pStream->off = off + cb + 1;
        if (   cb < 1
            || pch[-1] != '\r')
            pStream->paLines[iLine].enmEol = *penmEol = SCMEOL_LF;
        else
        {
            pStream->paLines[iLine].enmEol = *penmEol = SCMEOL_CRLF;
            cb--;
        }
    }
    else
    {
        pStream->off = off + cb;
        pStream->paLines[iLine].enmEol = *penmEol = SCMEOL_NONE;
    }
    *pcchLine = cb;
    pStream->paLines[iLine].cch = cb;
    pStream->cLines = pStream->iLine = ++iLine;

    return pchRet;
}

/**
 * Internal worker that lineates a stream.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.  Caller must check that it is in
 *                              read mode.
 */
static int scmStreamLineate(PSCMSTREAM pStream)
{
    /* Save the stream position. */
    size_t const offSaved   = pStream->off;
    size_t const iLineSaved = pStream->iLine;

    /* Get each line. */
    size_t cchLine;
    SCMEOL enmEol;
    while (scmStreamGetLineInternal(pStream, &cchLine, &enmEol))
        /* nothing */;
    Assert(RT_FAILURE(pStream->rc) || pStream->fFullyLineated);

    /* Restore the position */
    pStream->off   = offSaved;
    pStream->iLine = iLineSaved;

    return pStream->rc;
}

/**
 * Get the current stream position as a line number.
 *
 * @returns The current line (0-based).
 * @param   pStream             The stream.
 */
size_t ScmStreamTellLine(PSCMSTREAM pStream)
{
    return pStream->iLine;
}

/**
 * Gets the number of lines in the stream.
 *
 * @returns The number of lines.
 * @param   pStream             The stream.
 */
size_t ScmStreamCountLines(PSCMSTREAM pStream)
{
    if (!pStream->fFullyLineated)
        scmStreamLineate(pStream);
    return pStream->cLines;
}

/**
 * Seeks to a given line in the tream.
 *
 * @returns IPRT status code.
 *
 * @param   pStream             The stream.  Must be in read mode.
 * @param   iLine               The line to seek to.  If this is beyond the end
 *                              of the stream, the position is set to the end.
 */
int ScmStreamSeekByLine(PSCMSTREAM pStream, size_t iLine)
{
    AssertReturn(!pStream->fWriteOrRead, VERR_ACCESS_DENIED);
    if (RT_FAILURE(pStream->rc))
        return pStream->rc;

    /* Must be fully lineated of course. */
    if (RT_UNLIKELY(!pStream->fFullyLineated))
    {
        int rc = scmStreamLineate(pStream);
        if (RT_FAILURE(rc))
            return rc;
    }

    /* Ok, do the job. */
    if (iLine < pStream->cLines)
    {
        pStream->off   = pStream->paLines[iLine].off;
        pStream->iLine = iLine;
    }
    else
    {
        pStream->off   = pStream->cb;
        pStream->iLine = pStream->cLines;
    }
    return VINF_SUCCESS;
}

/**
 * Get a numbered line from the stream (changes the position).
 *
 * A line is always delimited by a LF character or the end of the stream.  The
 * delimiter is not included in returned line length, but instead returned via
 * the @a penmEol indicator.
 *
 * @returns Pointer to the first character in the line, not NULL terminated.
 *          NULL if the end of the stream has been reached or some problem
 *          occured.
 *
 * @param   pStream             The stream.  Must be in read mode.
 * @param   iLine               The line to get (0-based).
 * @param   pcchLine            The length.
 * @param   penmEol             Where to return the end of line type indicator.
 */
static const char *ScmStreamGetLineByNo(PSCMSTREAM pStream, size_t iLine, size_t *pcchLine, PSCMEOL penmEol)
{
    AssertReturn(!pStream->fWriteOrRead, NULL);
    if (RT_FAILURE(pStream->rc))
        return NULL;

    /* Make sure it's fully lineated so we can use the index. */
    if (RT_UNLIKELY(!pStream->fFullyLineated))
    {
        int rc = scmStreamLineate(pStream);
        if (RT_FAILURE(rc))
            return NULL;
    }

    /* End of stream? */
    if (RT_UNLIKELY(iLine >= pStream->cLines))
    {
        pStream->off   = pStream->cb;
        pStream->iLine = pStream->cLines;
        return NULL;
    }

    /* Get the data. */
    const char *pchRet = &pStream->pch[pStream->paLines[iLine].off];
    *pcchLine          = pStream->paLines[iLine].cch;
    *penmEol           = pStream->paLines[iLine].enmEol;

    /* update the stream position. */
    pStream->off       = pStream->paLines[iLine].cch + pStream->paLines[iLine].enmEol;
    pStream->iLine     = iLine + 1;

    return pchRet;
}

/**
 * Get a line from the stream.
 *
 * A line is always delimited by a LF character or the end of the stream.  The
 * delimiter is not included in returned line length, but instead returned via
 * the @a penmEol indicator.
 *
 * @returns Pointer to the first character in the line, not NULL terminated.
 *          NULL if the end of the stream has been reached or some problem
 *          occured.
 *
 * @param   pStream             The stream.  Must be in read mode.
 * @param   pcchLine            The length.
 * @param   penmEol             Where to return the end of line type indicator.
 */
static const char *ScmStreamGetLine(PSCMSTREAM pStream, size_t *pcchLine, PSCMEOL penmEol)
{
    if (!pStream->fFullyLineated)
        return scmStreamGetLineInternal(pStream, pcchLine, penmEol);
    return ScmStreamGetLineByNo(pStream, pStream->iLine, pcchLine, penmEol);
}

/**
 * Checks if the given line is empty or full of white space.
 *
 * @returns true if white space only, false if not (or if non-existant).
 * @param   pStream             The stream.  Must be in read mode.
 * @param   iLine               The line in question.
 */
static bool ScmStreamIsWhiteLine(PSCMSTREAM pStream, size_t iLine)
{
    SCMEOL      enmEol;
    size_t      cchLine;
    const char *pchLine = ScmStreamGetLineByNo(pStream, iLine, &cchLine, &enmEol);
    if (!pchLine)
        return false;
    while (cchLine && RT_C_IS_SPACE(*pchLine))
        pchLine++, cchLine--;
    return cchLine == 0;
}

/**
 * Try figure out the end of line style of the give stream.
 *
 * @returns Most likely end of line style.
 * @param   pStream             The stream.
 */
SCMEOL ScmStreamGetEol(PSCMSTREAM pStream)
{
    SCMEOL enmEol;
    if (pStream->cLines > 0)
        enmEol = pStream->paLines[0].enmEol;
    else if (pStream->cb == 0)
        enmEol = SCMEOL_NONE;
    else
    {
        const char *pchLF = (const char *)memchr(pStream->pch, '\n', pStream->cb);
        if (pchLF && pchLF != pStream->pch && pchLF[-1] == '\r')
            enmEol = SCMEOL_CRLF;
        else
            enmEol = SCMEOL_LF;
    }

    if (enmEol == SCMEOL_NONE)
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
        enmEol = SCMEOL_CRLF;
#else
        enmEol = SCMEOL_LF;
#endif
    return enmEol;
}

/**
 * Get the end of line indicator type for a line.
 *
 * @returns The EOL indicator.  If the line isn't found, the default EOL
 *          indicator is return.
 * @param   pStream             The stream.
 * @param   iLine               The line (0-base).
 */
SCMEOL ScmStreamGetEolByLine(PSCMSTREAM pStream, size_t iLine)
{
    SCMEOL enmEol;
    if (iLine < pStream->cLines)
        enmEol = pStream->paLines[iLine].enmEol;
    else
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
        enmEol = SCMEOL_CRLF;
#else
        enmEol = SCMEOL_LF;
#endif
    return enmEol;
}

/**
 * Appends a line to the stream.
 *
 * @returns IPRT status code.
 * @param   pStream             The stream.  Must be in write mode.
 * @param   pchLine             Pointer to the line.
 * @param   cchLine             Line length.
 * @param   enmEol              Which end of line indicator to use.
 */
int ScmStreamPutLine(PSCMSTREAM pStream, const char *pchLine, size_t cchLine, SCMEOL enmEol)
{
    AssertReturn(pStream->fWriteOrRead, VERR_ACCESS_DENIED);
    if (RT_FAILURE(pStream->rc))
        return pStream->rc;

    /*
     * Make sure the previous line has a new-line indicator.
     */
    size_t off   = pStream->off;
    size_t iLine = pStream->iLine;
    if (RT_UNLIKELY(   iLine != 0
                    && pStream->paLines[iLine - 1].enmEol == SCMEOL_NONE))
    {
        AssertReturn(pStream->paLines[iLine].cch == 0, VERR_INTERNAL_ERROR_3);
        SCMEOL enmEol2 = enmEol != SCMEOL_NONE ? enmEol : ScmStreamGetEol(pStream);
        if (RT_UNLIKELY(off + cchLine + enmEol + enmEol2 > pStream->cbAllocated))
        {
            int rc = scmStreamGrowBuffer(pStream, cchLine + enmEol + enmEol2);
            if (RT_FAILURE(rc))
                return rc;
        }
        if (enmEol2 == SCMEOL_LF)
            pStream->pch[off++] = '\n';
        else
        {
            pStream->pch[off++] = '\r';
            pStream->pch[off++] = '\n';
        }
        pStream->paLines[iLine - 1].enmEol = enmEol2;
        pStream->paLines[iLine].off = off;
        pStream->off = off;
        pStream->cb  = off;
    }

    /*
     * Ensure we've got sufficient buffer space.
     */
    if (RT_UNLIKELY(off + cchLine + enmEol > pStream->cbAllocated))
    {
        int rc = scmStreamGrowBuffer(pStream, cchLine + enmEol);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Add a line record.
     */
    if (RT_UNLIKELY(iLine + 1 >= pStream->cLinesAllocated))
    {
        int rc = scmStreamGrowLines(pStream, iLine);
        if (RT_FAILURE(rc))
            return rc;
    }

    pStream->paLines[iLine].cch    = off - pStream->paLines[iLine].off + cchLine;
    pStream->paLines[iLine].enmEol = enmEol;

    iLine++;
    pStream->cLines = iLine;
    pStream->iLine  = iLine;

    /*
     * Copy the line
     */
    memcpy(&pStream->pch[off], pchLine, cchLine);
    off += cchLine;
    if (enmEol == SCMEOL_LF)
        pStream->pch[off++] = '\n';
    else if (enmEol == SCMEOL_CRLF)
    {
        pStream->pch[off++] = '\r';
        pStream->pch[off++] = '\n';
    }
    pStream->off = off;
    pStream->cb  = off;

    /*
     * Start a new line.
     */
    pStream->paLines[iLine].off    = off;
    pStream->paLines[iLine].cch    = 0;
    pStream->paLines[iLine].enmEol = SCMEOL_NONE;

    return VINF_SUCCESS;
}

/**
 * Writes to the stream.
 *
 * @returns IPRT status code
 * @param   pStream             The stream.  Must be in write mode.
 * @param   pchBuf              What to write.
 * @param   cchBuf              How much to write.
 */
int ScmStreamWrite(PSCMSTREAM pStream, const char *pchBuf, size_t cchBuf)
{
    AssertReturn(pStream->fWriteOrRead, VERR_ACCESS_DENIED);
    if (RT_FAILURE(pStream->rc))
        return pStream->rc;

    /*
     * Ensure we've got sufficient buffer space.
     */
    size_t off = pStream->off;
    if (RT_UNLIKELY(off + cchBuf > pStream->cbAllocated))
    {
        int rc = scmStreamGrowBuffer(pStream, cchBuf);
        if (RT_FAILURE(rc))
            return rc;
    }

    /*
     * Deal with the odd case where we've already pushed a line with SCMEOL_NONE.
     */
    size_t iLine = pStream->iLine;
    if (RT_UNLIKELY(   iLine > 0
                    && pStream->paLines[iLine - 1].enmEol == SCMEOL_NONE))
    {
        iLine--;
        pStream->cLines = iLine;
        pStream->iLine  = iLine;
    }

    /*
     * Deal with lines.
     */
    const char *pchLF = (const char *)memchr(pchBuf, '\n', cchBuf);
    if (!pchLF)
        pStream->paLines[iLine].cch += cchBuf;
    else
    {
        const char *pchLine = pchBuf;
        for (;;)
        {
            if (RT_UNLIKELY(iLine + 1 >= pStream->cLinesAllocated))
            {
                int rc = scmStreamGrowLines(pStream, iLine);
                if (RT_FAILURE(rc))
                {
                    iLine = pStream->iLine;
                    pStream->paLines[iLine].cch    = off - pStream->paLines[iLine].off;
                    pStream->paLines[iLine].enmEol = SCMEOL_NONE;
                    return rc;
                }
            }

            size_t cchLine = pchLF - pchLine;
            if (   cchLine
                ?  pchLF[-1] != '\r'
                :     !pStream->paLines[iLine].cch
                   || pStream->pch[pStream->paLines[iLine].off + pStream->paLines[iLine].cch - 1] != '\r')
                pStream->paLines[iLine].enmEol = SCMEOL_LF;
            else
            {
                pStream->paLines[iLine].enmEol = SCMEOL_CRLF;
                cchLine--;
            }
            pStream->paLines[iLine].cch += cchLine;

            iLine++;
            size_t offBuf = pchLF + 1 - pchBuf;
            pStream->paLines[iLine].off    = off + offBuf;
            pStream->paLines[iLine].cch    = 0;
            pStream->paLines[iLine].enmEol = SCMEOL_NONE;

            size_t cchLeft = cchBuf - offBuf;
            pchLF = (const char *)memchr(pchLF + 1, '\n', cchLeft);
            if (!pchLF)
            {
                pStream->paLines[iLine].cch = cchLeft;
                break;
            }
        }

        pStream->iLine  = iLine;
        pStream->cLines = iLine;
    }

    /*
     * Copy the data and update position and size.
     */
    memcpy(&pStream->pch[off], pchBuf, cchBuf);
    off += cchBuf;
    pStream->off = off;
    pStream->cb  = off;

    return VINF_SUCCESS;
}

/**
 * Write a character to the stream.
 *
 * @returns IPRT status code
 * @param   pStream             The stream.  Must be in write mode.
 * @param   pchBuf              What to write.
 * @param   cchBuf              How much to write.
 */
int ScmStreamPutCh(PSCMSTREAM pStream, char ch)
{
    AssertReturn(pStream->fWriteOrRead, VERR_ACCESS_DENIED);
    if (RT_FAILURE(pStream->rc))
        return pStream->rc;

    /*
     * Only deal with the simple cases here, use ScmStreamWrite for the
     * annyoing stuff.
     */
    size_t off = pStream->off;
    if (   ch == '\n'
        || RT_UNLIKELY(off + 1 > pStream->cbAllocated))
        return ScmStreamWrite(pStream, &ch, 1);

    /*
     * Just append it.
     */
    pStream->pch[off] = ch;
    pStream->off = off + 1;
    pStream->paLines[pStream->iLine].cch++;

    return VINF_SUCCESS;
}

/**
 * Copies @a cLines from the @a pSrc stream onto the @a pDst stream.
 *
 * The stream positions will be used and changed in both streams.
 *
 * @returns IPRT status code.
 * @param   pDst                The destionation stream.  Must be in write mode.
 * @param   cLines              The number of lines.  (0 is accepted.)
 * @param   pSrc                The source stream.  Must be in read mode.
 */
int ScmStreamCopyLines(PSCMSTREAM pDst, PSCMSTREAM pSrc, size_t cLines)
{
    AssertReturn(pDst->fWriteOrRead, VERR_ACCESS_DENIED);
    if (RT_FAILURE(pDst->rc))
        return pDst->rc;

    AssertReturn(!pSrc->fWriteOrRead, VERR_ACCESS_DENIED);
    if (RT_FAILURE(pSrc->rc))
        return pSrc->rc;

    while (cLines-- > 0)
    {
        SCMEOL      enmEol;
        size_t      cchLine;
        const char *pchLine = ScmStreamGetLine(pSrc, &cchLine, &enmEol);
        if (!pchLine)
            return pDst->rc = (RT_FAILURE(pSrc->rc) ? pSrc->rc : VERR_EOF);

        int rc = ScmStreamPutLine(pDst, pchLine, cchLine, enmEol);
        if (RT_FAILURE(rc))
            return rc;
    }

    return VINF_SUCCESS;
}

/* -=-=-=-=-=- diff -=-=-=-=-=- */


/**
 * Prints a range of lines with a prefix.
 *
 * @param   pState              The diff state.
 * @param   chPrefix            The prefix.
 * @param   pStream             The stream to get the lines from.
 * @param   iLine               The first line.
 * @param   cLines              The number of lines.
 */
static void scmDiffPrintLines(PSCMDIFFSTATE pState, char chPrefix, PSCMSTREAM pStream, size_t iLine, size_t cLines)
{
    while (cLines-- > 0)
    {
        SCMEOL      enmEol;
        size_t      cchLine;
        const char *pchLine = ScmStreamGetLineByNo(pStream, iLine, &cchLine, &enmEol);

        RTStrmPutCh(pState->pDiff, chPrefix);
        if (pchLine && cchLine)
        {
            if (!pState->fSpecialChars)
                RTStrmWrite(pState->pDiff, pchLine, cchLine);
            else
            {
                size_t      offVir   = 0;
                const char *pchStart = pchLine;
                const char *pchTab   = (const char *)memchr(pchLine, '\t', cchLine);
                while (pchTab)
                {
                    RTStrmWrite(pState->pDiff, pchStart, pchTab - pchStart);
                    offVir += pchTab - pchStart;

                    size_t cchTab = g_cchTab - offVir % g_cchTab;
                    switch (cchTab)
                    {
                        case 1: RTStrmPutStr(pState->pDiff, "."); break;
                        case 2: RTStrmPutStr(pState->pDiff, ".."); break;
                        case 3: RTStrmPutStr(pState->pDiff, "[T]"); break;
                        case 4: RTStrmPutStr(pState->pDiff, "[TA]"); break;
                        case 5: RTStrmPutStr(pState->pDiff, "[TAB]"); break;
                        default: RTStrmPrintf(pState->pDiff, "[TAB%.*s]", cchTab - 5, g_szTabSpaces); break;
                    }
                    offVir += cchTab;

                    /* next */
                    pchStart = pchTab + 1;
                    pchTab = (const char *)memchr(pchStart, '\t', cchLine - (pchStart - pchLine));
                }
                size_t cchLeft = cchLine - (pchStart - pchLine);
                if (cchLeft)
                    RTStrmWrite(pState->pDiff, pchStart, cchLeft);
            }
        }

        if (!pState->fSpecialChars)
            RTStrmPutCh(pState->pDiff, '\n');
        else if (enmEol == SCMEOL_LF)
            RTStrmPutStr(pState->pDiff, "[LF]\n");
        else if (enmEol == SCMEOL_CRLF)
            RTStrmPutStr(pState->pDiff, "[CRLF]\n");
        else
            RTStrmPutStr(pState->pDiff, "[NONE]\n");

        iLine++;
    }
}


/**
 * Reports a difference and propells the streams to the lines following the
 * resync.
 *
 *
 * @returns New pState->cDiff value (just to return something).
 * @param   pState              The diff state.  The cDiffs member will be
 *                              incremented.
 * @param   cMatches            The resync length.
 * @param   iLeft               Where the difference starts on the left side.
 * @param   cLeft               How long it is on this side.  ~(size_t)0 is used
 *                              to indicate that it goes all the way to the end.
 * @param   iRight              Where the difference starts on the right side.
 * @param   cRight              How long it is.
 */
static size_t scmDiffReport(PSCMDIFFSTATE pState, size_t cMatches,
                            size_t iLeft, size_t cLeft,
                            size_t iRight, size_t cRight)
{
    /*
     * Adjust the input.
     */
    if (cLeft == ~(size_t)0)
    {
        size_t c = ScmStreamCountLines(pState->pLeft);
        if (c >= iLeft)
            cLeft = c - iLeft;
        else
        {
            iLeft = c;
            cLeft = 0;
        }
    }

    if (cRight == ~(size_t)0)
    {
        size_t c = ScmStreamCountLines(pState->pRight);
        if (c >= iRight)
            cRight = c - iRight;
        else
        {
            iRight = c;
            cRight = 0;
        }
    }

    /*
     * Print header if it's the first difference
     */
    if (!pState->cDiffs)
        RTStrmPrintf(pState->pDiff, "diff %s %s\n", pState->pszFilename, pState->pszFilename);

    /*
     * Emit the change description.
     */
    char ch = cLeft == 0
            ? 'a'
            : cRight == 0
            ? 'd'
            : 'c';
    if (cLeft > 1 && cRight > 1)
        RTStrmPrintf(pState->pDiff, "%zu,%zu%c%zu,%zu\n", iLeft + 1, iLeft + cLeft, ch, iRight + 1, iRight + cRight);
    else if (cLeft > 1)
        RTStrmPrintf(pState->pDiff, "%zu,%zu%c%zu\n",     iLeft + 1, iLeft + cLeft, ch, iRight + 1);
    else if (cRight > 1)
        RTStrmPrintf(pState->pDiff, "%zu%c%zu,%zu\n",     iLeft + 1,                ch, iRight + 1, iRight + cRight);
    else
        RTStrmPrintf(pState->pDiff, "%zu%c%zu\n",         iLeft + 1,                ch, iRight + 1);

    /*
     * And the lines.
     */
    if (cLeft)
        scmDiffPrintLines(pState, '<', pState->pLeft, iLeft, cLeft);
    if (cLeft && cRight)
        RTStrmPrintf(pState->pDiff, "---\n");
    if (cRight)
        scmDiffPrintLines(pState, '>', pState->pRight, iRight, cRight);

    /*
     * Reposition the streams (safely ignores return value).
     */
    ScmStreamSeekByLine(pState->pLeft,  iLeft  + cLeft  + cMatches);
    ScmStreamSeekByLine(pState->pRight, iRight + cRight + cMatches);

    pState->cDiffs++;
    return pState->cDiffs;
}

/**
 * Helper for scmDiffCompare that takes care of trailing spaces and stuff
 * like that.
 */
static bool scmDiffCompareSlow(PSCMDIFFSTATE pState,
                               const char *pchLeft,  size_t cchLeft,  SCMEOL enmEolLeft,
                               const char *pchRight, size_t cchRight, SCMEOL enmEolRight)
{
    if (pState->fIgnoreTrailingWhite)
    {
        while (cchLeft > 0 && RT_C_IS_SPACE(pchLeft[cchLeft - 1]))
            cchLeft--;
        while (cchRight > 0 && RT_C_IS_SPACE(pchRight[cchRight - 1]))
            cchRight--;
    }

    if (pState->fIgnoreLeadingWhite)
    {
        while (cchLeft > 0 && RT_C_IS_SPACE(*pchLeft))
            pchLeft++, cchLeft--;
        while (cchRight > 0 && RT_C_IS_SPACE(*pchRight))
            pchRight++, cchRight--;
    }

    if (   cchLeft != cchRight
        || (enmEolLeft != enmEolRight && !pState->fIgnoreEol)
        || memcmp(pchLeft, pchRight, cchLeft))
        return false;
    return true;
}

/**
 * Compare two lines.
 *
 * @returns true if the are equal, false if not.
 */
DECLINLINE(bool) scmDiffCompare(PSCMDIFFSTATE pState,
                                const char *pchLeft,  size_t cchLeft,  SCMEOL enmEolLeft,
                                const char *pchRight, size_t cchRight, SCMEOL enmEolRight)
{
    if (   cchLeft != cchRight
        || (enmEolLeft != enmEolRight && !pState->fIgnoreEol)
        || memcmp(pchLeft, pchRight, cchLeft))
    {
        if (   pState->fIgnoreTrailingWhite
            || pState->fIgnoreTrailingWhite)
            return scmDiffCompareSlow(pState,
                                      pchLeft, cchLeft, enmEolLeft,
                                      pchRight, cchRight, enmEolRight);
        return false;
    }
    return true;
}

/**
 * Compares two sets of lines from the two files.
 *
 * @returns true if they matches, false if they don't.
 * @param   pState              The diff state.
 * @param   iLeft               Where to start in the left stream.
 * @param   iRight              Where to start in the right stream.
 * @param   cLines              How many lines to compare.
 */
static bool scmDiffCompareLines(PSCMDIFFSTATE pState, size_t iLeft, size_t iRight, size_t cLines)
{
    for (size_t iLine = 0; iLine < cLines; iLine++)
    {
        SCMEOL      enmEolLeft;
        size_t      cchLeft;
        const char *pchLeft  = ScmStreamGetLineByNo(pState->pLeft,  iLeft + iLine,  &cchLeft,  &enmEolLeft);

        SCMEOL      enmEolRight;
        size_t      cchRight;
        const char *pchRight = ScmStreamGetLineByNo(pState->pRight, iRight + iLine, &cchRight, &enmEolRight);

        if (!scmDiffCompare(pState, pchLeft, cchLeft, enmEolLeft, pchRight, cchRight, enmEolRight))
            return false;
    }
    return true;
}


/**
 * Resynchronize the two streams and reports the difference.
 *
 * Upon return, the streams will be positioned after the block of @a cMatches
 * lines where it resynchronized them.
 *
 * @returns pState->cDiffs (just so we can use it in a return statement).
 * @param   pState              The state.
 * @param   cMatches            The number of lines that needs to match for the
 *                              stream to be considered synchronized again.
 */
static size_t scmDiffSynchronize(PSCMDIFFSTATE pState, size_t cMatches)
{
    size_t const iStartLeft  = ScmStreamTellLine(pState->pLeft)  - 1;
    size_t const iStartRight = ScmStreamTellLine(pState->pRight) - 1;
    Assert(cMatches > 0);

    /*
     * Compare each new line from each of the streams will all the preceding
     * ones, including iStartLeft/Right.
     */
    for (size_t iRange = 1; ; iRange++)
    {
        /*
         * Get the next line in the left stream and compare it against all the
         * preceding lines on the right side.
         */
        SCMEOL      enmEol;
        size_t      cchLine;
        const char *pchLine = ScmStreamGetLineByNo(pState->pLeft, iStartLeft + iRange, &cchLine, &enmEol);
        if (!pchLine)
            return scmDiffReport(pState, 0, iStartLeft, ~(size_t)0, iStartRight, ~(size_t)0);

        for (size_t iRight = cMatches - 1; iRight < iRange; iRight++)
        {
            SCMEOL      enmEolRight;
            size_t      cchRight;
            const char *pchRight = ScmStreamGetLineByNo(pState->pRight, iStartRight + iRight,
                                                        &cchRight, &enmEolRight);
            if (   scmDiffCompare(pState, pchLine, cchLine, enmEol, pchRight, cchRight, enmEolRight)
                && scmDiffCompareLines(pState,
                                       iStartLeft  + iRange + 1 - cMatches,
                                       iStartRight + iRight + 1 - cMatches,
                                       cMatches - 1)
               )
                return scmDiffReport(pState, cMatches,
                                     iStartLeft,  iRange + 1 - cMatches,
                                     iStartRight, iRight + 1 - cMatches);
        }

        /*
         * Get the next line in the right stream and compare it against all the
         * lines on the right side.
         */
        pchLine = ScmStreamGetLineByNo(pState->pRight, iStartRight + iRange, &cchLine, &enmEol);
        if (!pchLine)
            return scmDiffReport(pState, 0, iStartLeft, ~(size_t)0, iStartRight, ~(size_t)0);

        for (size_t iLeft = cMatches - 1; iLeft <= iRange; iLeft++)
        {
            SCMEOL      enmEolLeft;
            size_t      cchLeft;
            const char *pchLeft = ScmStreamGetLineByNo(pState->pLeft, iStartLeft + iLeft,
                                                       &cchLeft, &enmEolLeft);
            if (    scmDiffCompare(pState, pchLeft, cchLeft, enmEolLeft, pchLine, cchLine, enmEol)
                && scmDiffCompareLines(pState,
                                       iStartLeft  + iLeft  + 1 - cMatches,
                                       iStartRight + iRange + 1 - cMatches,
                                       cMatches - 1)
               )
                return scmDiffReport(pState, cMatches,
                                     iStartLeft,  iLeft  + 1 - cMatches,
                                     iStartRight, iRange + 1 - cMatches);
        }
    }
}

/**
 * Creates a diff of the changes between the streams @a pLeft and @a pRight.
 *
 * This currently only implements the simplest diff format, so no contexts.
 *
 * Also, note that we won't detect differences in the final newline of the
 * streams.
 *
 * @returns The number of differences.
 * @param   pszFilename         The filename.
 * @param   pLeft               The left side stream.
 * @param   pRight              The right side stream.
 * @param   fIgnoreEol          Whether to ignore end of line markers.
 * @param   fIgnoreLeadingWhite Set if leading white space should be ignored.
 * @param   fIgnoreTrailingWhite  Set if trailing white space should be ignored.
 * @param   fSpecialChars       Whether to print special chars in a human
 *                              readable form or not.
 * @param   pDiff               Where to write the diff.
 */
size_t ScmDiffStreams(const char *pszFilename, PSCMSTREAM pLeft, PSCMSTREAM pRight, bool fIgnoreEol,
                      bool fIgnoreLeadingWhite, bool fIgnoreTrailingWhite, bool fSpecialChars, PRTSTREAM pDiff)
{
#ifdef RT_STRICT
    ScmStreamCheckItegrity(pLeft);
    ScmStreamCheckItegrity(pRight);
#endif

    /*
     * Set up the diff state.
     */
    SCMDIFFSTATE State;
    State.cDiffs                = 0;
    State.pszFilename           = pszFilename;
    State.pLeft                 = pLeft;
    State.pRight                = pRight;
    State.fIgnoreEol            = fIgnoreEol;
    State.fIgnoreLeadingWhite   = fIgnoreLeadingWhite;
    State.fIgnoreTrailingWhite  = fIgnoreTrailingWhite;
    State.fSpecialChars         = fSpecialChars;
    State.pDiff                 = pDiff;

    /*
     * Compare them line by line.
     */
    ScmStreamRewindForReading(pLeft);
    ScmStreamRewindForReading(pRight);
    const char *pchLeft;
    const char *pchRight;

    for (;;)
    {
        SCMEOL  enmEolLeft;
        size_t  cchLeft;
        pchLeft  = ScmStreamGetLine(pLeft,  &cchLeft,  &enmEolLeft);

        SCMEOL  enmEolRight;
        size_t  cchRight;
        pchRight = ScmStreamGetLine(pRight, &cchRight, &enmEolRight);
        if (!pchLeft || !pchRight)
            break;

        if (!scmDiffCompare(&State, pchLeft, cchLeft, enmEolLeft, pchRight, cchRight, enmEolRight))
            scmDiffSynchronize(&State, 3);
    }

    /*
     * Deal with any remaining differences.
     */
    if (pchLeft)
        scmDiffReport(&State, 0, ScmStreamTellLine(pLeft) - 1, ~(size_t)0, ScmStreamTellLine(pRight), 0);
    else if (pchRight)
        scmDiffReport(&State, 0, ScmStreamTellLine(pLeft), 0, ScmStreamTellLine(pRight) - 1, ~(size_t)0);

    /*
     * Report any errors.
     */
    if (RT_FAILURE(ScmStreamGetStatus(pLeft)))
        RTMsgError("Left diff stream error: %Rrc\n", ScmStreamGetStatus(pLeft));
    if (RT_FAILURE(ScmStreamGetStatus(pRight)))
        RTMsgError("Right diff stream error: %Rrc\n", ScmStreamGetStatus(pRight));

    return State.cDiffs;
}




/* -=-=-=-=-=- misc -=-=-=-=-=- */


/**
 * Prints a verbose message if the level is high enough.
 *
 * @param   iLevel              The required verbosity level.
 * @param   pszFormat           The message format string.
 * @param   ...                 Format arguments.
 */
static void ScmVerbose(int iLevel, const char *pszFormat, ...)
{
    if (iLevel <= g_iVerbosity)
    {
        RTPrintf("%s: info: ", g_szProgName);
        va_list va;
        va_start(va, pszFormat);
        RTPrintfV(pszFormat, va);
        va_end(va);
    }
}


/* -=-=-=-=-=- rewriters -=-=-=-=-=- */


/**
 * Strip trailing blanks (space & tab).
 *
 * @returns True if modified, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 */
static bool rewrite_StripTrailingBlanks(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    if (!g_fStripTrailingBlanks)
        return false;

    bool        fModified = false;
    SCMEOL      enmEol;
    size_t      cchLine;
    const char *pchLine;
    while ((pchLine = ScmStreamGetLine(pIn, &cchLine, &enmEol)) != NULL)
    {
        int rc;
        if (    cchLine == 0
            ||  !RT_C_IS_BLANK(pchLine[cchLine - 1]) )
            rc = ScmStreamPutLine(pOut, pchLine, cchLine, enmEol);
        else
        {
            cchLine--;
            while (cchLine > 0 && RT_C_IS_BLANK(pchLine[cchLine - 1]))
                cchLine--;
            rc = ScmStreamPutLine(pOut, pchLine, cchLine, enmEol);
            fModified = true;
        }
        if (RT_FAILURE(rc))
            return false;
    }
    if (fModified)
        ScmVerbose(2, " * Stripped trailing blanks\n");
    return fModified;
}

/**
 * Expand tabs.
 *
 * @returns True if modified, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 */
static bool rewrite_ExpandTabs(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    if (!g_fConvertTabs)
        return false;

    bool        fModified = false;
    SCMEOL      enmEol;
    size_t      cchLine;
    const char *pchLine;
    while ((pchLine = ScmStreamGetLine(pIn, &cchLine, &enmEol)) != NULL)
    {
        int rc;
        const char *pchTab = (const char *)memchr(pchLine, '\t', cchLine);
        if (!pchTab)
            rc = ScmStreamPutLine(pOut, pchLine, cchLine, enmEol);
        else
        {
            size_t      offTab   = 0;
            const char *pchChunk = pchLine;
            for (;;)
            {
                size_t  cchChunk = pchTab - pchChunk;
                offTab += cchChunk;
                ScmStreamWrite(pOut, pchChunk, cchChunk);

                size_t  cchToTab = g_cchTab - offTab % g_cchTab;
                ScmStreamWrite(pOut, g_szTabSpaces, cchToTab);
                offTab += cchToTab;

                pchChunk = pchTab + 1;
                size_t  cchLeft  = cchLine - (pchChunk - pchLine);
                pchTab = (const char *)memchr(pchChunk, '\t', cchLeft);
                if (!pchTab)
                {
                    rc = ScmStreamPutLine(pOut, pchChunk, cchLeft, enmEol);
                    break;
                }
            }

            fModified = true;
        }
        if (RT_FAILURE(rc))
            return false;
    }
    if (fModified)
        ScmVerbose(2, " * Expanded tabs\n");
    return fModified;
}

/**
 * Worker for rewrite_ForceNativeEol, rewrite_ForceLF and rewrite_ForceCRLF.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 * @param   enmDesiredEol       The desired end of line indicator type.
 */
static bool rewrite_ForceEol(PSCMSTREAM pIn, PSCMSTREAM pOut, SCMEOL enmDesiredEol)
{
    if (!g_fConvertEol)
        return false;

    bool        fModified = false;
    SCMEOL      enmEol;
    size_t      cchLine;
    const char *pchLine;
    while ((pchLine = ScmStreamGetLine(pIn, &cchLine, &enmEol)) != NULL)
    {
        if (   enmEol != enmDesiredEol
            && enmEol != SCMEOL_NONE)
        {
            fModified = true;
            enmEol = enmDesiredEol;
        }
        int rc = ScmStreamPutLine(pOut, pchLine, cchLine, enmEol);
        if (RT_FAILURE(rc))
            return false;
    }
    if (fModified)
        ScmVerbose(2, " * Converted EOL markers\n");
    return fModified;
}

/**
 * Force native end of line indicator.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 */
static bool rewrite_ForceNativeEol(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
#if defined(RT_OS_WINDOWS) || defined(RT_OS_OS2)
    return rewrite_ForceEol(pIn, pOut, SCMEOL_CRLF);
#else
    return rewrite_ForceEol(pIn, pOut, SCMEOL_LF);
#endif
}

/**
 * Force the stream to use LF as the end of line indicator.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 */
static bool rewrite_ForceLF(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    return rewrite_ForceEol(pIn, pOut, SCMEOL_LF);
}

/**
 * Force the stream to use CRLF as the end of line indicator.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 */
static bool rewrite_ForceCRLF(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    return rewrite_ForceEol(pIn, pOut, SCMEOL_CRLF);
}

/**
 * Strip trailing blank lines and/or make sure there is exactly one blank line
 * at the end of the file.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 *
 * @remarks ASSUMES trailing white space has been removed already.
 */
static bool rewrite_AdjustTrailingLines(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    if (!g_fStripTrailingLines && !g_fForceTrailingLine && !g_fForceFinalEol)
        return false;

    size_t const cLines = ScmStreamCountLines(pIn);

    /* Empty files remains empty. */
    if (cLines <= 1)
        return false;

    /* Figure out if we need to adjust the number of lines or not. */
    size_t cLinesNew = cLines;

    if (   g_fStripTrailingLines
        && ScmStreamIsWhiteLine(pIn, cLinesNew - 1))
    {
        while (   cLinesNew > 1
               && ScmStreamIsWhiteLine(pIn, cLinesNew - 2))
            cLinesNew--;
    }

    if (    g_fForceTrailingLine
        && !ScmStreamIsWhiteLine(pIn, cLinesNew - 1))
        cLinesNew++;

    bool fFixMissingEol = g_fForceFinalEol
                       && ScmStreamGetEolByLine(pIn, cLinesNew - 1) == SCMEOL_NONE;

    if (   !fFixMissingEol
        && cLines == cLinesNew)
        return false;

    /* Copy the number of lines we've arrived at. */
    ScmStreamRewindForReading(pIn);

    size_t cCopied = RT_MIN(cLinesNew, cLines);
    ScmStreamCopyLines(pOut, pIn, cCopied);

    if (cCopied != cLinesNew)
    {
        while (cCopied++ < cLinesNew)
            ScmStreamPutLine(pOut, "", 0, ScmStreamGetEol(pIn));
    }
    else if (fFixMissingEol)
    {
        if (ScmStreamGetEol(pIn) == SCMEOL_LF)
            ScmStreamWrite(pOut, "\n", 1);
        else
            ScmStreamWrite(pOut, "\r\n", 2);
    }

    ScmVerbose(2, " * Adjusted trailing blank lines\n");
    return true;
}

/**
 * Makefile.kup are empty files, enforce this.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 */
static bool rewrite_Makefile_kup(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    /* These files should be zero bytes. */
    if (pIn->cb == 0)
        return false;
    ScmVerbose(2, " * Truncated file to zero bytes\n");
    return true;
}

/**
 * Rewrite a kBuild makefile.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 *
 * @todo
 *
 * Ideas for Makefile.kmk and Config.kmk:
 *      - sort if1of/ifn1of sets.
 *      - line continuation slashes should only be preceeded by one space.
 */
static bool rewrite_Makefile_kmk(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    return false;
}

/**
 * Rewrite a C/C++ source or header file.
 *
 * @returns true if modifications were made, false if not.
 * @param   pIn                 The input stream.
 * @param   pOut                The output stream.
 *
 * @todo
 *
 * Ideas for C/C++:
 *      - space after if, while, for, switch
 *      - spaces in for (i=0;i<x;i++)
 *      - complex conditional, bird style.
 *      - remove unnecessary parentheses.
 *      - sort defined RT_OS_*||  and RT_ARCH
 *      - sizeof without parenthesis.
 *      - defined without parenthesis.
 *      - trailing spaces.
 *      - parameter indentation.
 *      - space after comma.
 *      - while (x--); -> multi line + comment.
 *      - else statement;
 *      - space between function and left parenthesis.
 *      - TODO, XXX, @todo cleanup.
 *      - Space before/after '*'.
 *      - ensure new line at end of file.
 *      - Indentation of precompiler statements (#ifdef, #defines).
 *      - space between functions.
 */
static bool rewrite_C_and_CPP(PSCMSTREAM pIn, PSCMSTREAM pOut)
{
    return false;
}

/* -=-=-=-=-=- file and directory processing -=-=-=-=-=- */

/**
 * Processes a file.
 *
 * @returns IPRT status code.
 * @param   pszFilename         The file name.
 * @param   pszBasename         The base name (pointer within @a pszFilename).
 * @param   cchBasename         The length of the base name.  (For passing to
 *                              RTStrSimplePatternMultiMatch.)
 */
static int scmProcessFile(const char *pszFilename, const char *pszBasename, size_t cchBasename)
{
    /*
     * Do the file level filtering.
     */
    if (   g_pszFileFilter
        && !RTStrSimplePatternMultiMatch(g_pszFileFilter, RTSTR_MAX, pszBasename, cchBasename, NULL))
    {
        ScmVerbose(4, "file filter mismatch: \"%s\"\n", pszFilename);
        return VINF_SUCCESS;
    }
    if (   g_pszFileFilterOut
        && (   RTStrSimplePatternMultiMatch(g_pszFileFilterOut, RTSTR_MAX, pszBasename, cchBasename, NULL)
            || RTStrSimplePatternMultiMatch(g_pszFileFilterOut, RTSTR_MAX, pszFilename, RTSTR_MAX, NULL)) )
    {
        ScmVerbose(4, "file filter out: \"%s\"\n", pszFilename);
        return VINF_SUCCESS;
    }

    /*
     * Try find a matching rewrite config for this filename.
     */
    PCSCMCFGENTRY pCfg = NULL;
    for (size_t iCfg = 0; iCfg < RT_ELEMENTS(g_aConfigs); iCfg++)
        if (RTStrSimplePatternMultiMatch(g_aConfigs[iCfg].pszFilePattern, RTSTR_MAX, pszBasename, cchBasename, NULL))
        {
            pCfg = &g_aConfigs[iCfg];
            break;
        }
    if (!pCfg)
    {
        ScmVerbose(3, "No rewriters configured for \"%s\"\n", pszFilename);
        return VINF_SUCCESS;
    }
    ScmVerbose(3, "\"%s\" matched \"%s\"\n", pszFilename, pCfg->pszFilePattern);

    /*
     * Create an input stream from the file and check that it's text.
     */
    SCMSTREAM Stream1;
    int rc = ScmStreamInitForReading(&Stream1, pszFilename);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Failed to read '%s': %Rrc\n", pszFilename, rc);
        return rc;
    }
    if (ScmStreamIsText(&Stream1))
    {
        ScmVerbose(1, "Processing '%s'...\n", pszFilename);

        /*
         * Create two more streams for output and push the text thru all the
         * rewriters, switching the two streams around when something is
         * actually rewritten.  Stream1 remains unchanged.
         */
        SCMSTREAM Stream2;
        rc = ScmStreamInitForWriting(&Stream2, &Stream1);
        if (RT_SUCCESS(rc))
        {
            SCMSTREAM Stream3;
            rc = ScmStreamInitForWriting(&Stream3, &Stream1);
            if (RT_SUCCESS(rc))
            {
                bool        fModified = false;
                PSCMSTREAM  pIn       = &Stream1;
                PSCMSTREAM  pOut      = &Stream2;
                for (size_t iRw = 0; iRw < pCfg->cRewriters; iRw++)
                {
                    bool fRc = pCfg->papfnRewriter[iRw](pIn, pOut);
                    if (fRc)
                    {
                        PSCMSTREAM pTmp = pOut;
                        pOut = pIn == &Stream1 ? &Stream3 : pIn;
                        pIn  = pTmp;
                        fModified = true;
                    }
                    ScmStreamRewindForReading(pIn);
                    ScmStreamRewindForWriting(pOut);
                }

                /*
                 * If rewritten, write it back to disk.
                 */
                if (fModified)
                {
                    if (!g_fDryRun)
                    {
                        ScmVerbose(1, "Writing modified file to \"%s%s\"\n", pszFilename, g_pszChangedSuff);
                        rc = ScmStreamWriteToFile(pIn, "%s%s", pszFilename, g_pszChangedSuff);
                        if (RT_FAILURE(rc))
                            RTMsgError("Error writing '%s%s': %Rrc\n", pszFilename, g_pszChangedSuff, rc);
                    }
                    else
                    {
                        ScmVerbose(1, "Would have modified file \"%s\"\n", pszFilename);
                        ScmDiffStreams(pszFilename, &Stream1, pIn, g_fDiffIgnoreEol, g_fDiffIgnoreLeadingWS,
                                       g_fDiffIgnoreTrailingWS, g_fDiffSpecialChars, g_pStdOut);
                    }
                }
                else
                    ScmVerbose(2, "Unchanged \"%s\"\n", pszFilename);

                ScmStreamDelete(&Stream3);
            }
            else
                RTMsgError("Failed to init stream for writing: %Rrc\n", rc);
            ScmStreamDelete(&Stream2);
        }
        else
            RTMsgError("Failed to init stream for writing: %Rrc\n", rc);
    }
    else
        ScmVerbose(3, "not text file: \"%s\"\n", pszFilename);
    ScmStreamDelete(&Stream1);

    return rc;
}

/**
 * Tries to correct RTDIRENTRY_UNKNOWN.
 *
 * @returns Corrected type.
 * @param   pszPath             The path to the object in question.
 */
static RTDIRENTRYTYPE scmFigureUnknownType(const char *pszPath)
{
    RTFSOBJINFO Info;
    int rc = RTPathQueryInfo(pszPath, &Info, RTFSOBJATTRADD_NOTHING);
    if (RT_FAILURE(rc))
        return RTDIRENTRYTYPE_UNKNOWN;
    if (RTFS_IS_DIRECTORY(Info.Attr.fMode))
        return RTDIRENTRYTYPE_DIRECTORY;
    if (RTFS_IS_FILE(Info.Attr.fMode))
        return RTDIRENTRYTYPE_FILE;
    return RTDIRENTRYTYPE_UNKNOWN;
}

/**
 * Recurse into a sub-directory and process all the files and directories.
 *
 * @returns IPRT status code.
 * @param   pszBuf              Path buffer containing the directory path on
 *                              entry.  This ends with a dot.  This is passed
 *                              along when recusing in order to save stack space
 *                              and avoid needless copying.
 * @param   cchDir              Length of our path in pszbuf.
 * @param   pEntry              Directory entry buffer.  This is also passed
 *                              along when recursing to save stack space.
 * @param   iRecursion          The recursion depth.  This is used to restrict
 *                              the recursions.
 */
static int scmProcessDirTreeRecursion(char *pszBuf, size_t cchDir, PRTDIRENTRY pEntry, unsigned iRecursion)
{
    Assert(cchDir > 1 && pszBuf[cchDir - 1] == '.');

    /*
     * Make sure we stop somewhere.
     */
    if (iRecursion > 128)
    {
        RTMsgError("recursion too deep: %d\n", iRecursion);
        return VINF_SUCCESS; /* ignore */
    }

    /*
     * Try open and read the directory.
     */
    PRTDIR pDir;
    int rc = RTDirOpenFiltered(&pDir, pszBuf, RTDIRFILTER_NONE);
    if (RT_FAILURE(rc))
    {
        RTMsgError("Failed to enumerate directory '%s': %Rrc", pszBuf, rc);
        return rc;
    }
    for (;;)
    {
        /* Read the next entry. */
        rc = RTDirRead(pDir, pEntry, NULL);
        if (RT_FAILURE(rc) && rc != VERR_NO_MORE_FILES)
            RTMsgError("RTDirRead -> %Rrc\n", rc);
        if (RT_FAILURE(rc))
            break;

        /* Skip '.' and '..'. */
        if (    pEntry->szName[0] == '.'
            &&  (   pEntry->cbName == 1
                 || (   pEntry->cbName == 2
                     && pEntry->szName[1] == '.')))
            continue;

        /* Enter it into the buffer so we've got a full name to work
           with when needed. */
        if (pEntry->cbName + cchDir >= RTPATH_MAX)
        {
            RTMsgError("Skipping too long entry: %s", pEntry->szName);
            continue;
        }
        memcpy(&pszBuf[cchDir - 1], pEntry->szName, pEntry->cbName + 1);

        /* Figure the type. */
        RTDIRENTRYTYPE enmType = pEntry->enmType;
        if (enmType == RTDIRENTRYTYPE_UNKNOWN)
            enmType = scmFigureUnknownType(pszBuf);

        /* Process the file or directory, skip the rest. */
        if (enmType == RTDIRENTRYTYPE_FILE)
            rc = scmProcessFile(pszBuf, pEntry->szName, pEntry->cbName);
        else if (enmType == RTDIRENTRYTYPE_DIRECTORY)
        {
            /* Append the dot for the benefit of the pattern matching. */
            if (pEntry->cbName + cchDir + 5 >= RTPATH_MAX)
            {
                RTMsgError("Skipping too deep dir entry: %s", pEntry->szName);
                continue;
            }
            memcpy(&pszBuf[cchDir - 1 + pEntry->cbName], "/.", sizeof("/."));
            size_t cchSubDir = cchDir - 1 + pEntry->cbName + sizeof("/.") - 1;

            if (    (   g_pszDirFilter == NULL
                     || RTStrSimplePatternMultiMatch(g_pszDirFilter, RTSTR_MAX,
                                                     pEntry->szName, pEntry->cbName, NULL))
                 && (   g_pszDirFilterOut == NULL
                     || (   !RTStrSimplePatternMultiMatch(g_pszDirFilterOut, RTSTR_MAX,
                                                          pEntry->szName, pEntry->cbName, NULL)
                         && !RTStrSimplePatternMultiMatch(g_pszDirFilterOut, RTSTR_MAX,
                                                          pszBuf, cchSubDir, NULL)
                        )
                    )
               )
            {
                rc = scmProcessDirTreeRecursion(pszBuf, cchSubDir, pEntry, iRecursion + 1);
            }
        }
        if (RT_FAILURE(rc))
            break;
    }
    RTDirClose(pDir);
    return RT_SUCCESS(rc) ? 0 : 1;

}

/**
 * Process a directory tree.
 *
 * @returns IPRT status code.
 * @param   pszDir              The directory to start with.  This is pointer to
 *                              a RTPATH_MAX sized buffer.
 */
static int scmProcessDirTree(char *pszDir)
{
    /*
     * Setup the recursion.
     */
    int rc = RTPathAppend(pszDir, RTPATH_MAX, ".");
    if (RT_SUCCESS(rc))
    {
        RTDIRENTRY Entry;
        rc = scmProcessDirTreeRecursion(pszDir, strlen(pszDir), &Entry, 0);
    }
    else
        RTMsgError("RTPathAppend: %Rrc\n", rc);
    return rc;
}


/**
 * Processes a file or directory specified as an command line argument.
 *
 * @returns IPRT status code
 * @param   pszSomething    What we found in the commad line arguments.
 */
static int scmProcessSomething(const char *pszSomething)
{
    char szBuf[RTPATH_MAX];
    int rc = RTPathAbs(pszSomething, szBuf, sizeof(szBuf));
    if (RT_SUCCESS(rc))
    {
        if (RTFileExists(szBuf))
        {
            const char *pszBasename = RTPathFilename(szBuf);
            if (pszBasename)
            {
                size_t cchBasename = strlen(pszBasename);
                rc = scmProcessFile(szBuf, pszBasename, cchBasename);
            }
            else
            {
                RTMsgError("RTPathFilename: NULL\n");
                rc = VERR_IS_A_DIRECTORY;
            }
        }
        else
            rc = scmProcessDirTree(szBuf);
    }
    else
        RTMsgError("RTPathAbs: %Rrc\n", rc);
    return rc;
}

int main(int argc, char **argv)
{
    int rc = RTR3Init();
    if (RT_FAILURE(rc))
        return 1;

    /*
     * Parse arguments and process input in order (because this is the only
     * thing that works at the moment).
     */
    enum SCMOPT
    {
        SCMOPT_DIFF_IGNORE_EOL = 10000,
        SCMOPT_DIFF_NO_IGNORE_EOL,
        SCMOPT_DIFF_IGNORE_SPACE,
        SCMOPT_DIFF_NO_IGNORE_SPACE,
        SCMOPT_DIFF_IGNORE_LEADING_SPACE,
        SCMOPT_DIFF_NO_IGNORE_LEADING_SPACE,
        SCMOPT_DIFF_IGNORE_TRAILING_SPACE,
        SCMOPT_DIFF_NO_IGNORE_TRAILING_SPACE,
        SCMOPT_DIFF_SPECIAL_CHARS,
        SCMOPT_DIFF_NO_SPECIAL_CHARS,
        SCMOPT_STRIP_TRAILING_LINES,
        SCMOPT_NO_STRIP_TRAILING_LINES,
        SCMOPT_FORCE_FINAL_EOL,
        SCMOPT_NO_FORCE_FINAL_EOL,
        SCMOPT_FORCE_TRAILING_LINE,
        SCMOPT_NO_FORCE_TRAILING_LINE,
        SCMOPT_LAST
    };
    static const RTGETOPTDEF s_aOpts[] =
    {
        { "--strip-trailing-blanks",            'b',                                    RTGETOPT_REQ_NOTHING },
        { "--no-strip-trailing-blanks",         'B',                                    RTGETOPT_REQ_NOTHING },
        { "--convert-tabs",                     'c',                                    RTGETOPT_REQ_NOTHING },
        { "--no-convert-tabs",                  'C',                                    RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-eol",                  SCMOPT_DIFF_IGNORE_EOL,                 RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-eol",               SCMOPT_DIFF_NO_IGNORE_EOL,              RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-space",                SCMOPT_DIFF_IGNORE_SPACE,               RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-space",             SCMOPT_DIFF_NO_IGNORE_SPACE,            RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-leading-space",        SCMOPT_DIFF_IGNORE_LEADING_SPACE,       RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-leading-space",     SCMOPT_DIFF_NO_IGNORE_LEADING_SPACE,    RTGETOPT_REQ_NOTHING },
        { "--diff-ignore-trailing-space",       SCMOPT_DIFF_IGNORE_TRAILING_SPACE,      RTGETOPT_REQ_NOTHING },
        { "--diff-no-ignore-trailing-space",    SCMOPT_DIFF_NO_IGNORE_TRAILING_SPACE,   RTGETOPT_REQ_NOTHING },
        { "--diff-special-chars",               SCMOPT_DIFF_SPECIAL_CHARS,              RTGETOPT_REQ_NOTHING },
        { "--diff-no-special-chars",            SCMOPT_DIFF_NO_SPECIAL_CHARS,           RTGETOPT_REQ_NOTHING },
        { "--dry-run",                          'd',                                    RTGETOPT_REQ_NOTHING },
        { "--real-run",                         'D',                                    RTGETOPT_REQ_NOTHING },
        { "--strip-trailing-lines",             SCMOPT_STRIP_TRAILING_LINES,            RTGETOPT_REQ_NOTHING },
        { "--strip-no-trailing-lines",          SCMOPT_NO_STRIP_TRAILING_LINES,         RTGETOPT_REQ_NOTHING },
        { "--force-final-eol",                  SCMOPT_FORCE_FINAL_EOL,                 RTGETOPT_REQ_NOTHING },
        { "--no-force-final-eol",               SCMOPT_NO_FORCE_FINAL_EOL,              RTGETOPT_REQ_NOTHING },
        { "--force-trailing-line",              SCMOPT_FORCE_TRAILING_LINE,             RTGETOPT_REQ_NOTHING },
        { "--no-force-trailing-line",           SCMOPT_NO_FORCE_TRAILING_LINE,          RTGETOPT_REQ_NOTHING },
        { "--convert-eol",                      'e',                                    RTGETOPT_REQ_NOTHING },
        { "--no-convert-eol",                   'E',                                    RTGETOPT_REQ_NOTHING },
        { "--file-filter",                      'f',                                    RTGETOPT_REQ_STRING  },
        { "--help",                             'h',                                    RTGETOPT_REQ_NOTHING },
        { "--quiet",                            'q',                                    RTGETOPT_REQ_NOTHING },
        { "--tab-size",                         't',                                    RTGETOPT_REQ_UINT8   },
        { "--verbose",                          'v',                                    RTGETOPT_REQ_NOTHING },
    };

    RTGETOPTUNION   ValueUnion;
    RTGETOPTSTATE   GetOptState;
    rc = RTGetOptInit(&GetOptState, argc, argv, &s_aOpts[0], RT_ELEMENTS(s_aOpts), 1, 0 /*fFlags*/);
    AssertReleaseRCReturn(rc, 1);
    size_t          cProcessed = 0;

    while ((rc = RTGetOpt(&GetOptState, &ValueUnion)) != 0)
    {
        switch (rc)
        {
            case 'b':
                g_fStripTrailingBlanks = true;
                break;

            case 'B':
                g_fStripTrailingBlanks = false;
                break;

            case 'c':
                g_fConvertTabs = true;
                break;

            case 'C':
                g_fConvertTabs = false;
                break;

            case 'd':
                g_fDryRun = true;
                break;

            case 'D':
                g_fDryRun = false;
                break;

            case 'e':
                g_fConvertEol = true;
                break;

            case 'E':
                g_fConvertEol = false;
                break;

            case 'f':
                g_pszFileFilter = ValueUnion.psz;
                break;

            case 'h':
                RTPrintf("Source Code Massager\n"
                         "\n"
                         "Usage: %s [options] <files & dirs>\n"
                         "\n"
                         "Options:\n", g_szProgName);
                for (size_t i = 0; i < RT_ELEMENTS(s_aOpts); i++)
                    if ((s_aOpts[i].fFlags & RTGETOPT_REQ_MASK) == RTGETOPT_REQ_NOTHING)
                        RTPrintf("  %s\n", s_aOpts[i].pszLong);
                    else if ((s_aOpts[i].fFlags & RTGETOPT_REQ_MASK) == RTGETOPT_REQ_STRING)
                        RTPrintf("  %s string\n", s_aOpts[i].pszLong);
                    else
                        RTPrintf("  %s value\n", s_aOpts[i].pszLong);
                return 1;

            case 'q':
                g_iVerbosity = 0;
                break;

            case 't':
                if (   ValueUnion.u8 < 1
                    || ValueUnion.u8 >= RT_ELEMENTS(g_szTabSpaces))
                {
                    RTMsgError("Invalid tab size: %u - must be in {1..%u}\n",
                               ValueUnion.u8, RT_ELEMENTS(g_szTabSpaces) - 1);
                    return 2;
                }
                g_cchTab = ValueUnion.u8;
                break;

            case 'v':
                g_iVerbosity++;
                break;

            case SCMOPT_DIFF_IGNORE_EOL:
                g_fDiffIgnoreEol = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_EOL:
                g_fDiffIgnoreEol = false;
                break;

            case SCMOPT_DIFF_IGNORE_SPACE:
                g_fDiffIgnoreTrailingWS = g_fDiffIgnoreLeadingWS = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_SPACE:
                g_fDiffIgnoreTrailingWS = g_fDiffIgnoreLeadingWS = false;
                break;

            case SCMOPT_DIFF_IGNORE_LEADING_SPACE:
                g_fDiffIgnoreLeadingWS = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_LEADING_SPACE:
                g_fDiffIgnoreLeadingWS = false;
                break;

            case SCMOPT_DIFF_IGNORE_TRAILING_SPACE:
                g_fDiffIgnoreTrailingWS = true;
                break;
            case SCMOPT_DIFF_NO_IGNORE_TRAILING_SPACE:
                g_fDiffIgnoreTrailingWS = false;
                break;

            case SCMOPT_DIFF_SPECIAL_CHARS:
                g_fDiffSpecialChars = true;
                break;
            case SCMOPT_DIFF_NO_SPECIAL_CHARS:
                g_fDiffSpecialChars = false;
                break;

            case SCMOPT_STRIP_TRAILING_LINES:
                g_fStripTrailingLines = true;
                break;
            case SCMOPT_NO_STRIP_TRAILING_LINES:
                g_fStripTrailingLines = false;
                break;

            case SCMOPT_FORCE_FINAL_EOL:
                g_fForceFinalEol = true;
                break;
            case SCMOPT_NO_FORCE_FINAL_EOL:
                g_fForceFinalEol = false;
                break;

            case SCMOPT_FORCE_TRAILING_LINE:
                g_fForceTrailingLine = true;
                break;
            case SCMOPT_NO_FORCE_TRAILING_LINE:
                g_fForceTrailingLine = false;
                break;

            case VINF_GETOPT_NOT_OPTION:
            {
                if (!g_fDryRun)
                {
                    if (!cProcessed)
                    {
                        RTPrintf("%s: Warning! This program will make changes to your source files and\n"
                                 "%s:          there is a slight risk that bugs or a full disk may cause\n"
                                 "%s:          LOSS OF DATA.   So, please make sure you have checked in\n"
                                 "%s:          all your changes already.  If you didn't, then don't blam\n"
                                 "%s:          anyone for not warning you!\n"
                                 "%s:\n"
                                 "%s:          Press any key to continue...\n",
                                 g_szProgName, g_szProgName, g_szProgName, g_szProgName, g_szProgName,
                                 g_szProgName, g_szProgName);
                        RTStrmGetCh(g_pStdIn);
                    }
                    cProcessed++;
                }
                rc = scmProcessSomething(ValueUnion.psz);
                if (RT_FAILURE(rc))
                    return rc;
                break;
            }

            default:
                return RTGetOptPrintError(rc, &ValueUnion);
        }
    }

    return 0;
}


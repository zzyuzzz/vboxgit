/* $Id: VBoxManageDisk.cpp 26603 2010-02-17 12:24:34Z vboxsync $ */
/** @file
 * VBoxManage - The disk delated commands.
 */

/*
 * Copyright (C) 2006-2009 Sun Microsystems, Inc.
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

#ifndef VBOX_ONLY_DOCS

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/com/com.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/VirtualBox.h>

#include <iprt/asm.h>
#include <iprt/file.h>
#include <iprt/path.h>
#include <iprt/param.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/ctype.h>
#include <iprt/getopt.h>
#include <VBox/log.h>
#include <VBox/VBoxHDD.h>

#include "VBoxManage.h"
using namespace com;


// funcs
///////////////////////////////////////////////////////////////////////////////


static DECLCALLBACK(void) handleVDError(void *pvUser, int rc, RT_SRC_POS_DECL, const char *pszFormat, va_list va)
{
    RTPrintf("ERROR: ");
    RTPrintfV(pszFormat, va);
    RTPrintf("\n");
    RTPrintf("Error code %Rrc at %s(%u) in function %s\n", rc, RT_SRC_POS_ARGS);
}


static int parseDiskVariant(const char *psz, MediumVariant_T *pDiskVariant)
{
    int rc = VINF_SUCCESS;
    unsigned DiskVariant = (unsigned)(*pDiskVariant);
    while (psz && *psz && RT_SUCCESS(rc))
    {
        size_t len;
        const char *pszComma = strchr(psz, ',');
        if (pszComma)
            len = pszComma - psz;
        else
            len = strlen(psz);
        if (len > 0)
        {
            // Parsing is intentionally inconsistent: "standard" resets the
            // variant, whereas the other flags are cumulative.
            if (!RTStrNICmp(psz, "standard", len))
                DiskVariant = MediumVariant_Standard;
            else if (   !RTStrNICmp(psz, "fixed", len)
                     || !RTStrNICmp(psz, "static", len))
                DiskVariant |= MediumVariant_Fixed;
            else if (!RTStrNICmp(psz, "Diff", len))
                DiskVariant |= MediumVariant_Diff;
            else if (!RTStrNICmp(psz, "split2g", len))
                DiskVariant |= MediumVariant_VmdkSplit2G;
            else if (   !RTStrNICmp(psz, "stream", len)
                     || !RTStrNICmp(psz, "streamoptimized", len))
                DiskVariant |= MediumVariant_VmdkStreamOptimized;
            else if (!RTStrNICmp(psz, "esx", len))
                DiskVariant |= MediumVariant_VmdkESX;
            else
                rc = VERR_PARSE_ERROR;
        }
        if (pszComma)
            psz += len + 1;
        else
            psz += len;
    }

    if (RT_SUCCESS(rc))
        *pDiskVariant = (MediumVariant_T)DiskVariant;
    return rc;
}

static int parseDiskType(const char *psz, MediumType_T *pDiskType)
{
    int rc = VINF_SUCCESS;
    MediumType_T DiskType = MediumType_Normal;
    if (!RTStrICmp(psz, "normal"))
        DiskType = MediumType_Normal;
    else if (!RTStrICmp(psz, "immutable"))
        DiskType = MediumType_Immutable;
    else if (!RTStrICmp(psz, "writethrough"))
        DiskType = MediumType_Writethrough;
    else
        rc = VERR_PARSE_ERROR;

    if (RT_SUCCESS(rc))
        *pDiskType = DiskType;
    return rc;
}

/** @todo move this into getopt, as getting bool values is generic */
static int parseBool(const char *psz, bool *pb)
{
    int rc = VINF_SUCCESS;
    if (    !RTStrICmp(psz, "on")
        ||  !RTStrICmp(psz, "yes")
        ||  !RTStrICmp(psz, "true")
        ||  !RTStrICmp(psz, "1")
        ||  !RTStrICmp(psz, "enable")
        ||  !RTStrICmp(psz, "enabled"))
    {
        *pb = true;
    }
    else if (   !RTStrICmp(psz, "off")
             || !RTStrICmp(psz, "no")
             || !RTStrICmp(psz, "false")
             || !RTStrICmp(psz, "0")
             || !RTStrICmp(psz, "disable")
             || !RTStrICmp(psz, "disabled"))
    {
        *pb = false;
    }
    else
        rc = VERR_PARSE_ERROR;

    return rc;
}

static const RTGETOPTDEF g_aCreateHardDiskOptions[] =
{
    { "--filename",     'f', RTGETOPT_REQ_STRING },
    { "-filename",      'f', RTGETOPT_REQ_STRING },     // deprecated
    { "--size",         's', RTGETOPT_REQ_UINT64 },
    { "-size",          's', RTGETOPT_REQ_UINT64 },     // deprecated
    { "--format",       'o', RTGETOPT_REQ_STRING },
    { "-format",        'o', RTGETOPT_REQ_STRING },     // deprecated
    { "--static",       'F', RTGETOPT_REQ_NOTHING },
    { "-static",        'F', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--variant",      'm', RTGETOPT_REQ_STRING },
    { "-variant",       'm', RTGETOPT_REQ_STRING },     // deprecated
    { "--type",         't', RTGETOPT_REQ_STRING },
    { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
    { "--comment",      'c', RTGETOPT_REQ_STRING },
    { "-comment",       'c', RTGETOPT_REQ_STRING },     // deprecated
    { "--remember",     'r', RTGETOPT_REQ_NOTHING },
    { "-remember",      'r', RTGETOPT_REQ_NOTHING },    // deprecated
    { "--register",     'r', RTGETOPT_REQ_NOTHING },    // deprecated (inofficial)
    { "-register",      'r', RTGETOPT_REQ_NOTHING },    // deprecated
};

int handleCreateHardDisk(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    Bstr filename;
    uint64_t sizeMB = 0;
    Bstr format = "VDI";
    MediumVariant_T DiskVariant = MediumVariant_Standard;
    Bstr comment;
    bool fRemember = false;
    MediumType_T DiskType = MediumType_Normal;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCreateHardDiskOptions, RT_ELEMENTS(g_aCreateHardDiskOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'f':   // --filename
                filename = ValueUnion.psz;
                break;

            case 's':   // --size
                sizeMB = ValueUnion.u64;
                break;

            case 'o':   // --format
                format = ValueUnion.psz;
                break;

            case 'F':   // --static ("fixed"/"flat")
            {
                unsigned uDiskVariant = (unsigned)DiskVariant;
                uDiskVariant |= MediumVariant_Fixed;
                DiskVariant = (MediumVariant_T)uDiskVariant;
                break;
            }

            case 'm':   // --variant
                vrc = parseDiskVariant(ValueUnion.psz, &DiskVariant);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid hard disk variant '%s'", ValueUnion.psz);
                break;

            case 'c':   // --comment
                comment = ValueUnion.psz;
                break;

            case 'r':   // --remember
                fRemember = true;
                break;

            case 't':   // --type
                vrc = parseDiskType(ValueUnion.psz, &DiskType);
                if (    RT_FAILURE(vrc)
                    ||  (DiskType != MediumType_Normal && DiskType != MediumType_Writethrough))
                    return errorArgument("Invalid hard disk type '%s'", ValueUnion.psz);
                break;

            case VINF_GETOPT_NOT_OPTION:
                return errorSyntax(USAGE_CREATEHD, "Invalid parameter '%s'", ValueUnion.psz);

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_CREATEHD, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_CREATEHD, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_CREATEHD, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_CREATEHD, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_CREATEHD, "error: %Rrs", c);
        }
    }

    /* check the outcome */
    if (   !filename
        || sizeMB == 0)
        return errorSyntax(USAGE_CREATEHD, "Parameters --filename and --size are required");

    /* check for filename extension */
    Utf8Str strName(filename);
    if (!RTPathHaveExt(strName.c_str()))
    {
        Utf8Str strFormat(format);
        if (strFormat.compare("vmdk", iprt::MiniString::CaseInsensitive) == 0)
            strName.append(".vmdk");
        else if (strFormat.compare("vhd", iprt::MiniString::CaseInsensitive) == 0)
            strName.append(".vhd");
        else
            strName.append(".vdi");
        filename = Bstr(strName);
    }

    ComPtr<IMedium> hardDisk;
    CHECK_ERROR(a->virtualBox, CreateHardDisk(format, filename, hardDisk.asOutParam()));
    if (SUCCEEDED(rc) && hardDisk)
    {
        /* we will close the hard disk after the storage has been successfully
         * created unless fRemember is set */
        bool doClose = false;

        if (!comment.isNull())
        {
            CHECK_ERROR(hardDisk,COMSETTER(Description)(comment));
        }
        ComPtr<IProgress> progress;
        CHECK_ERROR(hardDisk, CreateBaseStorage(sizeMB, DiskVariant, progress.asOutParam()));
        if (SUCCEEDED(rc) && progress)
        {
            rc = showProgress(progress);
            if (FAILED(rc))
            {
                com::ProgressErrorInfo info(progress);
                if (info.isBasicAvailable())
                    RTPrintf("Error: failed to create hard disk. Error message: %lS\n", info.getText().raw());
                else
                    RTPrintf("Error: failed to create hard disk. No error message available!\n");
            }
            else
            {
                doClose = !fRemember;

                Bstr uuid;
                CHECK_ERROR(hardDisk, COMGETTER(Id)(uuid.asOutParam()));

                if (DiskType == MediumType_Writethrough)
                {
                    CHECK_ERROR(hardDisk, COMSETTER(Type)(MediumType_Writethrough));
                }

                RTPrintf("Disk image created. UUID: %s\n", Utf8Str(uuid).raw());
            }
        }
        if (doClose)
        {
            CHECK_ERROR(hardDisk, Close());
        }
    }
    return SUCCEEDED(rc) ? 0 : 1;
}

#if 0 /* disabled until disk shrinking is implemented based on VBoxHDD */
static DECLCALLBACK(int) hardDiskProgressCallback(PVM pVM, unsigned uPercent, void *pvUser)
{
    unsigned *pPercent = (unsigned *)pvUser;

    if (*pPercent != uPercent)
    {
        *pPercent = uPercent;
        RTPrintf(".");
        if ((uPercent % 10) == 0 && uPercent)
            RTPrintf("%d%%", uPercent);
        RTStrmFlush(g_pStdOut);
    }

    return VINF_SUCCESS;
}
#endif

static const RTGETOPTDEF g_aModifyHardDiskOptions[] =
{
    { "--type",         't', RTGETOPT_REQ_STRING },
    { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
    { "settype",        't', RTGETOPT_REQ_STRING },     // deprecated
    { "--autoreset",    'z', RTGETOPT_REQ_STRING },
    { "-autoreset",     'z', RTGETOPT_REQ_STRING },     // deprecated
    { "autoreset",      'z', RTGETOPT_REQ_STRING },     // deprecated
    { "--compact",      'c', RTGETOPT_REQ_NOTHING },
    { "-compact",       'c', RTGETOPT_REQ_NOTHING },    // deprecated
    { "compact",        'c', RTGETOPT_REQ_NOTHING },    // deprecated
};

int handleModifyHardDisk(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    ComPtr<IMedium> hardDisk;
    MediumType_T DiskType;
    bool AutoReset = false;
    bool fModifyDiskType = false, fModifyAutoReset = false, fModifyCompact = false;;
    const char *FilenameOrUuid = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aModifyHardDiskOptions, RT_ELEMENTS(g_aModifyHardDiskOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 't':   // --type
                vrc = parseDiskType(ValueUnion.psz, &DiskType);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid hard disk type '%s'", ValueUnion.psz);
                fModifyDiskType = true;
                break;

            case 'z':   // --autoreset
                vrc = parseBool(ValueUnion.psz, &AutoReset);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid autoreset parameter '%s'", ValueUnion.psz);
                fModifyAutoReset = true;
                break;

            case 'c':   // --compact
                fModifyCompact = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!FilenameOrUuid)
                    FilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CREATEHD, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_MODIFYHD, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_MODIFYHD, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_MODIFYHD, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_MODIFYHD, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_MODIFYHD, "error: %Rrs", c);
        }
    }

    if (!FilenameOrUuid)
        return errorSyntax(USAGE_MODIFYHD, "Disk name or UUID required");

    if (!fModifyDiskType && !fModifyAutoReset && !fModifyCompact)
        return errorSyntax(USAGE_MODIFYHD, "No operation specified");

    /* first guess is that it's a UUID */
    Guid uuid(FilenameOrUuid);
    rc = a->virtualBox->GetHardDisk(uuid.toUtf16(), hardDisk.asOutParam());
    /* no? then it must be a filename */
    if (!hardDisk)
    {
        CHECK_ERROR(a->virtualBox, FindHardDisk(Bstr(FilenameOrUuid), hardDisk.asOutParam()));
        if (FAILED(rc))
            return 1;
    }

    if (fModifyDiskType)
    {
        /* hard disk must be registered */
        if (SUCCEEDED(rc) && hardDisk)
        {
            MediumType_T hddType;
            CHECK_ERROR(hardDisk, COMGETTER(Type)(&hddType));

            if (hddType != DiskType)
                CHECK_ERROR(hardDisk, COMSETTER(Type)(DiskType));
        }
        else
            return errorArgument("Hard disk image not registered");
    }

    if (fModifyAutoReset)
    {
        CHECK_ERROR(hardDisk, COMSETTER(AutoReset)(AutoReset));
    }

    if (fModifyCompact)
    {
        bool unknown = false;
        /* the hard disk image might not be registered */
        if (!hardDisk)
        {
            unknown = true;
            rc = a->virtualBox->OpenHardDisk(Bstr(FilenameOrUuid), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), hardDisk.asOutParam());
            if (rc == VBOX_E_FILE_ERROR)
            {
                char szFilenameAbs[RTPATH_MAX] = "";
                int irc = RTPathAbs(FilenameOrUuid, szFilenameAbs, sizeof(szFilenameAbs));
                if (RT_FAILURE(irc))
                {
                    RTPrintf("Cannot convert filename \"%s\" to absolute path\n", FilenameOrUuid);
                    return 1;
                }
                CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(szFilenameAbs), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), hardDisk.asOutParam()));
            }
            else if (FAILED(rc))
                CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(FilenameOrUuid), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), hardDisk.asOutParam()));
        }
        if (SUCCEEDED(rc) && hardDisk)
        {
            ComPtr<IProgress> progress;
            CHECK_ERROR(hardDisk, Compact(progress.asOutParam()));
            if (SUCCEEDED(rc))
                rc = showProgress(progress);
            if (FAILED(rc))
            {
                if (rc == E_NOTIMPL)
                {
                    RTPrintf("Error: Compact hard disk operation is not implemented!\n");
                    RTPrintf("The functionality will be restored later.\n");
                }
                else if (rc == VBOX_E_NOT_SUPPORTED)
                {
                    RTPrintf("Error: Compact hard disk operation for this format is not implemented yet!\n");
                }
                else
                    com::GluePrintRCMessage(rc);
            }
            if (unknown)
                hardDisk->Close();
        }
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

static const RTGETOPTDEF g_aCloneHardDiskOptions[] =
{
    { "--format",       'o', RTGETOPT_REQ_STRING },
    { "-format",        'o', RTGETOPT_REQ_STRING },
    { "--static",       'F', RTGETOPT_REQ_NOTHING },
    { "-static",        'F', RTGETOPT_REQ_NOTHING },
    { "--existing",     'E', RTGETOPT_REQ_NOTHING },
    { "--variant",      'm', RTGETOPT_REQ_STRING },
    { "-variant",       'm', RTGETOPT_REQ_STRING },
    { "--type",         't', RTGETOPT_REQ_STRING },
    { "-type",          't', RTGETOPT_REQ_STRING },
    { "--remember",     'r', RTGETOPT_REQ_NOTHING },
    { "-remember",      'r', RTGETOPT_REQ_NOTHING },
    { "--register",     'r', RTGETOPT_REQ_NOTHING },
    { "-register",      'r', RTGETOPT_REQ_NOTHING },
};

int handleCloneHardDisk(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    Bstr src, dst;
    Bstr format;
    MediumVariant_T DiskVariant = MediumVariant_Standard;
    bool fExisting = false;
    bool fRemember = false;
    bool fSetDiskType = false;
    MediumType_T DiskType = MediumType_Normal;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCloneHardDiskOptions, RT_ELEMENTS(g_aCloneHardDiskOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'o':   // --format
                format = ValueUnion.psz;
                break;

            case 'F':   // --static
            {
                unsigned uDiskVariant = (unsigned)DiskVariant;
                uDiskVariant |= MediumVariant_Fixed;
                DiskVariant = (MediumVariant_T)uDiskVariant;
                break;
            }

            case 'E':   // --existing
                fExisting = true;
                break;

            case 'm':   // --variant
                vrc = parseDiskVariant(ValueUnion.psz, &DiskVariant);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid hard disk variant '%s'", ValueUnion.psz);
                break;

            case 'r':   // --remember
                fRemember = true;
                break;

            case 't':   // --type
                vrc = parseDiskType(ValueUnion.psz, &DiskType);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid hard disk type '%s'", ValueUnion.psz);
                fSetDiskType = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (src.isEmpty())
                    src = ValueUnion.psz;
                else if (dst.isEmpty())
                    dst = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CLONEHD, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_GRAPH(c))
                        return errorSyntax(USAGE_CLONEHD, "unhandled option: -%c", c);
                    else
                        return errorSyntax(USAGE_CLONEHD, "unhandled option: %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_CLONEHD, "unknown option: %s", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_CLONEHD, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_CLONEHD, "error: %Rrs", c);
        }
    }

    if (src.isEmpty())
        return errorSyntax(USAGE_CLONEHD, "Mandatory UUID or input file parameter missing");
    if (dst.isEmpty())
        return errorSyntax(USAGE_CLONEHD, "Mandatory output file parameter missing");
    if (fExisting && (!format.isEmpty() || DiskVariant != MediumType_Normal))
        return errorSyntax(USAGE_CLONEHD, "Specified options which cannot be used with --existing");

    ComPtr<IMedium> srcDisk;
    ComPtr<IMedium> dstDisk;
    bool fSrcUnknown = false;
    bool fDstUnknown = false;

    /* first guess is that it's a UUID */
    rc = a->virtualBox->GetHardDisk(src, srcDisk.asOutParam());
    /* no? then it must be a filename */
    if (FAILED (rc))
        rc = a->virtualBox->FindHardDisk(src, srcDisk.asOutParam());
    /* no? well, then it's an unknown image */
    if (FAILED (rc))
    {
        rc = a->virtualBox->OpenHardDisk(src, AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), srcDisk.asOutParam());
        if (rc == VBOX_E_FILE_ERROR)
        {
            char szFilenameAbs[RTPATH_MAX] = "";
            int irc = RTPathAbs(Utf8Str(src).c_str(), szFilenameAbs, sizeof(szFilenameAbs));
            if (RT_FAILURE(irc))
            {
                RTPrintf("Cannot convert filename \"%s\" to absolute path\n", Utf8Str(src).raw());
                return 1;
            }
            CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(szFilenameAbs), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), srcDisk.asOutParam()));
        }
        else if (FAILED(rc))
            CHECK_ERROR(a->virtualBox, OpenHardDisk(src, AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), srcDisk.asOutParam()));
        if (SUCCEEDED (rc))
            fSrcUnknown = true;
    }

    do
    {
        if (!SUCCEEDED(rc))
            break;

        /* open/create destination hard disk */
        if (fExisting)
        {
            /* first guess is that it's a UUID */
            rc = a->virtualBox->GetHardDisk(dst, dstDisk.asOutParam());
            /* no? then it must be a filename */
            if (FAILED (rc))
                rc = a->virtualBox->FindHardDisk(dst, dstDisk.asOutParam());
            /* no? well, then it's an unknown image */
            if (FAILED (rc))
            {
                rc = a->virtualBox->OpenHardDisk(dst, AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), dstDisk.asOutParam());
                if (rc == VBOX_E_FILE_ERROR)
                {
                    char szFilenameAbs[RTPATH_MAX] = "";
                    int irc = RTPathAbs(Utf8Str(dst).c_str(), szFilenameAbs, sizeof(szFilenameAbs));
                    if (RT_FAILURE(irc))
                    {
                        RTPrintf("Cannot convert filename \"%s\" to absolute path\n", Utf8Str(dst).raw());
                        return 1;
                    }
                    CHECK_ERROR_BREAK(a->virtualBox, OpenHardDisk(Bstr(szFilenameAbs), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), dstDisk.asOutParam()));
                }
                else if (FAILED(rc))
                    CHECK_ERROR_BREAK(a->virtualBox, OpenHardDisk(dst, AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), dstDisk.asOutParam()));
                if (SUCCEEDED (rc))
                    fDstUnknown = true;
            }
            else
                fRemember = true;
            if (SUCCEEDED(rc))
            {
                /* Perform accessibility check now. */
                MediumState_T state;
                CHECK_ERROR_BREAK(dstDisk, RefreshState(&state));
            }
            CHECK_ERROR_BREAK(dstDisk, COMGETTER(Format) (format.asOutParam()));
        }
        else
        {
            /* use the format of the source hard disk if unspecified */
            if (format.isEmpty())
                CHECK_ERROR_BREAK(srcDisk, COMGETTER(Format) (format.asOutParam()));
            CHECK_ERROR_BREAK(a->virtualBox, CreateHardDisk(format, dst, dstDisk.asOutParam()));
        }

        ComPtr<IProgress> progress;
        CHECK_ERROR_BREAK(srcDisk, CloneTo(dstDisk, DiskVariant, NULL, progress.asOutParam()));

        rc = showProgress(progress);
        if (FAILED(rc))
        {
            com::ProgressErrorInfo info(progress);
            if (info.isBasicAvailable())
                RTPrintf("Error: failed to clone hard disk. Error message: %lS\n", info.getText().raw());
            else
                RTPrintf("Error: failed to clone hard disk. No error message available!\n");
            break;
        }

        Bstr uuid;
        CHECK_ERROR_BREAK(dstDisk, COMGETTER(Id)(uuid.asOutParam()));

        RTPrintf("Clone hard disk created in format '%ls'. UUID: %s\n",
                 format.raw(), Utf8Str(uuid).raw());
    }
    while (0);

    if (!fRemember && !dstDisk.isNull())
    {
        /* forget the created clone */
        dstDisk->Close();
    }
    else if (fSetDiskType)
    {
        CHECK_ERROR(dstDisk, COMSETTER(Type)(DiskType));
    }

    if (fSrcUnknown)
    {
        /* close the unknown hard disk to forget it again */
        srcDisk->Close();
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

static const RTGETOPTDEF g_aConvertFromRawHardDiskOptions[] =
{
    { "--format",       'o', RTGETOPT_REQ_STRING },
    { "-format",        'o', RTGETOPT_REQ_STRING },
    { "--static",       'F', RTGETOPT_REQ_NOTHING },
    { "-static",        'F', RTGETOPT_REQ_NOTHING },
    { "--variant",      'm', RTGETOPT_REQ_STRING },
    { "-variant",       'm', RTGETOPT_REQ_STRING },
};

int handleConvertFromRaw(int argc, char *argv[])
{
    int rc = VINF_SUCCESS;
    bool fReadFromStdIn = false;
    const char *format = "VDI";
    const char *srcfilename = NULL;
    const char *dstfilename = NULL;
    const char *filesize = NULL;
    unsigned uImageFlags = VD_IMAGE_FLAGS_NONE;
    void *pvBuf = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, argc, argv, g_aConvertFromRawHardDiskOptions, RT_ELEMENTS(g_aConvertFromRawHardDiskOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'o':   // --format
                format = ValueUnion.psz;
                break;

            case 'm':   // --variant
                MediumVariant_T DiskVariant;
                rc = parseDiskVariant(ValueUnion.psz, &DiskVariant);
                if (RT_FAILURE(rc))
                    return errorArgument("Invalid hard disk variant '%s'", ValueUnion.psz);
                /// @todo cleaner solution than assuming 1:1 mapping?
                uImageFlags = (unsigned)DiskVariant;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!srcfilename)
                {
                    srcfilename = ValueUnion.psz;
#if defined(RT_OS_LINUX) || defined(RT_OS_DARWIN) || defined(RT_OS_SOLARIS)
                    fReadFromStdIn = !strcmp(srcfilename, "stdin");
#endif
                }
                else if (!dstfilename)
                    dstfilename = ValueUnion.psz;
                else if (fReadFromStdIn && !filesize)
                    filesize = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CONVERTFROMRAW, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                return errorGetOpt(USAGE_CONVERTFROMRAW, c, &ValueUnion);
        }
    }

    if (!srcfilename || !dstfilename || (fReadFromStdIn && !filesize))
        return errorSyntax(USAGE_CONVERTFROMRAW, "Incorrect number of parameters");
    RTPrintf("Converting from raw image file=\"%s\" to file=\"%s\"...\n",
             srcfilename, dstfilename);

    PVBOXHDD pDisk = NULL;

    PVDINTERFACE     pVDIfs = NULL;
    VDINTERFACE      vdInterfaceError;
    VDINTERFACEERROR vdInterfaceErrorCallbacks;
    vdInterfaceErrorCallbacks.cbSize       = sizeof(VDINTERFACEERROR);
    vdInterfaceErrorCallbacks.enmInterface = VDINTERFACETYPE_ERROR;
    vdInterfaceErrorCallbacks.pfnError     = handleVDError;
    vdInterfaceErrorCallbacks.pfnMessage   = NULL;

    rc = VDInterfaceAdd(&vdInterfaceError, "VBoxManage_IError", VDINTERFACETYPE_ERROR,
                        &vdInterfaceErrorCallbacks, NULL, &pVDIfs);
    AssertRC(rc);

    /* open raw image file. */
    RTFILE File;
    if (fReadFromStdIn)
        File = 0;
    else
        rc = RTFileOpen(&File, srcfilename, RTFILE_O_READ | RTFILE_O_OPEN | RTFILE_O_DENY_WRITE);
    if (RT_FAILURE(rc))
    {
        RTPrintf("File=\"%s\" open error: %Rrf\n", srcfilename, rc);
        goto out;
    }

    uint64_t cbFile;
    /* get image size. */
    if (fReadFromStdIn)
        cbFile = RTStrToUInt64(filesize);
    else
        rc = RTFileGetSize(File, &cbFile);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Error getting image size for file \"%s\": %Rrc\n", srcfilename, rc);
        goto out;
    }

    RTPrintf("Creating %s image with size %RU64 bytes (%RU64MB)...\n", (uImageFlags & VD_IMAGE_FLAGS_FIXED) ? "fixed" : "dynamic", cbFile, (cbFile + _1M - 1) / _1M);
    char pszComment[256];
    RTStrPrintf(pszComment, sizeof(pszComment), "Converted image from %s", srcfilename);
    rc = VDCreate(pVDIfs, &pDisk);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Error while creating the virtual disk container: %Rrc\n", rc);
        goto out;
    }

    Assert(RT_MIN(cbFile / 512 / 16 / 63, 16383) -
           (unsigned int)RT_MIN(cbFile / 512 / 16 / 63, 16383) == 0);
    PDMMEDIAGEOMETRY PCHS, LCHS;
    PCHS.cCylinders = (unsigned int)RT_MIN(cbFile / 512 / 16 / 63, 16383);
    PCHS.cHeads = 16;
    PCHS.cSectors = 63;
    LCHS.cCylinders = 0;
    LCHS.cHeads = 0;
    LCHS.cSectors = 0;
    rc = VDCreateBase(pDisk, format, dstfilename, cbFile,
                      uImageFlags, pszComment, &PCHS, &LCHS, NULL,
                      VD_OPEN_FLAGS_NORMAL, NULL, NULL);
    if (RT_FAILURE(rc))
    {
        RTPrintf("Error while creating the disk image \"%s\": %Rrc\n", dstfilename, rc);
        goto out;
    }

    size_t cbBuffer;
    cbBuffer = _1M;
    pvBuf = RTMemAlloc(cbBuffer);
    if (!pvBuf)
    {
        rc = VERR_NO_MEMORY;
        RTPrintf("Not enough memory allocating buffers for image \"%s\": %Rrc\n", dstfilename, rc);
        goto out;
    }

    uint64_t offFile;
    offFile = 0;
    while (offFile < cbFile)
    {
        size_t cbRead;
        size_t cbToRead;
        cbRead = 0;
        cbToRead = cbFile - offFile >= (uint64_t)cbBuffer ?
                            cbBuffer : (size_t) (cbFile - offFile);
        rc = RTFileRead(File, pvBuf, cbToRead, &cbRead);
        if (RT_FAILURE(rc) || !cbRead)
            break;
        rc = VDWrite(pDisk, offFile, pvBuf, cbRead);
        if (RT_FAILURE(rc))
        {
            RTPrintf("Failed to write to disk image \"%s\": %Rrc\n", dstfilename, rc);
            goto out;
        }
        offFile += cbRead;
    }

out:
    if (pvBuf)
        RTMemFree(pvBuf);
    if (pDisk)
        VDClose(pDisk, RT_FAILURE(rc));
    if (File != NIL_RTFILE)
        RTFileClose(File);

    return RT_FAILURE(rc);
}

static const RTGETOPTDEF g_aAddiSCSIDiskOptions[] =
{
    { "--server",       's', RTGETOPT_REQ_STRING },
    { "-server",        's', RTGETOPT_REQ_STRING },     // deprecated
    { "--target",       'T', RTGETOPT_REQ_STRING },
    { "-target",        'T', RTGETOPT_REQ_STRING },     // deprecated
    { "--port",         'p', RTGETOPT_REQ_STRING },
    { "-port",          'p', RTGETOPT_REQ_STRING },     // deprecated
    { "--lun",          'l', RTGETOPT_REQ_STRING },
    { "-lun",           'l', RTGETOPT_REQ_STRING },     // deprecated
    { "--encodedlun",   'L', RTGETOPT_REQ_STRING },
    { "-encodedlun",    'L', RTGETOPT_REQ_STRING },     // deprecated
    { "--username",     'u', RTGETOPT_REQ_STRING },
    { "-username",      'u', RTGETOPT_REQ_STRING },     // deprecated
    { "--password",     'P', RTGETOPT_REQ_STRING },
    { "-password",      'P', RTGETOPT_REQ_STRING },     // deprecated
    { "--type",         't', RTGETOPT_REQ_STRING },
    { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
    { "--intnet",       'I', RTGETOPT_REQ_NOTHING },
    { "-intnet",        'I', RTGETOPT_REQ_NOTHING },    // deprecated
};

int handleAddiSCSIDisk(HandlerArg *a)
{
    HRESULT rc;
    int vrc;
    Bstr server;
    Bstr target;
    Bstr port;
    Bstr lun;
    Bstr username;
    Bstr password;
    Bstr comment;
    bool fIntNet = false;
    MediumType_T DiskType = MediumType_Normal;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aAddiSCSIDiskOptions, RT_ELEMENTS(g_aAddiSCSIDiskOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 's':   // --server
                server = ValueUnion.psz;
                break;

            case 'T':   // --target
                target = ValueUnion.psz;
                break;

            case 'p':   // --port
                port = ValueUnion.psz;
                break;

            case 'l':   // --lun
                lun = ValueUnion.psz;
                break;

            case 'L':   // --encodedlun
                lun = BstrFmt("enc%s", ValueUnion.psz);
                break;

            case 'u':   // --username
                username = ValueUnion.psz;
                break;

            case 'P':   // --password
                password = ValueUnion.psz;
                break;

            case 't':   // --type
                vrc = parseDiskType(ValueUnion.psz, &DiskType);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid hard disk type '%s'", ValueUnion.psz);
                break;

            case 'I':   // --intnet
                fIntNet = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                return errorSyntax(USAGE_ADDISCSIDISK, "Invalid parameter '%s'", ValueUnion.psz);

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_ADDISCSIDISK, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_ADDISCSIDISK, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_ADDISCSIDISK, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_ADDISCSIDISK, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_ADDISCSIDISK, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (!server || !target)
        return errorSyntax(USAGE_ADDISCSIDISK, "Parameters --server and --target are required");

    do
    {
        ComPtr<IMedium> hardDisk;
        /** @todo move the location stuff to Main, which can use pfnComposeName
         * from the disk backends to construct the location properly. Also do
         * not use slashes to separate the parts, as otherwise only the last
         * element comtaining information will be shown. */
        if (lun.isEmpty() || lun == "0" || lun == "enc0")
        {
            CHECK_ERROR_BREAK (a->virtualBox,
                CreateHardDisk(Bstr ("iSCSI"),
                               BstrFmt ("%ls|%ls", server.raw(), target.raw()),
                               hardDisk.asOutParam()));
        }
        else
        {
            CHECK_ERROR_BREAK (a->virtualBox,
                CreateHardDisk(Bstr ("iSCSI"),
                               BstrFmt ("%ls|%ls|%ls", server.raw(), target.raw(), lun.raw()),
                               hardDisk.asOutParam()));
        }
        if (FAILED(rc)) break;

        if (!port.isNull())
            server = BstrFmt ("%ls:%ls", server.raw(), port.raw());

        com::SafeArray <BSTR> names;
        com::SafeArray <BSTR> values;

        Bstr ("TargetAddress").detachTo (names.appendedRaw());
        server.detachTo (values.appendedRaw());
        Bstr ("TargetName").detachTo (names.appendedRaw());
        target.detachTo (values.appendedRaw());

        if (!lun.isNull())
        {
            Bstr ("LUN").detachTo (names.appendedRaw());
            lun.detachTo (values.appendedRaw());
        }
        if (!username.isNull())
        {
            Bstr ("InitiatorUsername").detachTo (names.appendedRaw());
            username.detachTo (values.appendedRaw());
        }
        if (!password.isNull())
        {
            Bstr ("InitiatorSecret").detachTo (names.appendedRaw());
            password.detachTo (values.appendedRaw());
        }

        /// @todo add --initiator option
        Bstr ("InitiatorName").detachTo (names.appendedRaw());
        Bstr ("iqn.2008-04.com.sun.virtualbox.initiator").detachTo (values.appendedRaw());

        /// @todo add --targetName and --targetPassword options

        if (fIntNet)
        {
            Bstr ("HostIPStack").detachTo (names.appendedRaw());
            Bstr ("0").detachTo (values.appendedRaw());
        }

        CHECK_ERROR_BREAK (hardDisk,
            SetProperties (ComSafeArrayAsInParam (names),
                           ComSafeArrayAsInParam (values)));

        if (DiskType != MediumType_Normal)
        {
            CHECK_ERROR(hardDisk, COMSETTER(Type)(DiskType));
        }

        Bstr guid;
        CHECK_ERROR(hardDisk, COMGETTER(Id)(guid.asOutParam()));
        RTPrintf("iSCSI disk created. UUID: %s\n", Utf8Str(guid).raw());
    }
    while (0);

    return SUCCEEDED(rc) ? 0 : 1;
}

static const RTGETOPTDEF g_aShowHardDiskInfoOptions[] =
{
    { "--dummy",    256, RTGETOPT_REQ_NOTHING },   // placeholder for C++
};

int handleShowHardDiskInfo(HandlerArg *a)
{
    HRESULT rc;
    const char *FilenameOrUuid = NULL;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aShowHardDiskInfoOptions, RT_ELEMENTS(g_aShowHardDiskInfoOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case VINF_GETOPT_NOT_OPTION:
                if (!FilenameOrUuid)
                    FilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_SHOWHDINFO, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_SHOWHDINFO, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_SHOWHDINFO, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_SHOWHDINFO, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_SHOWHDINFO, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_SHOWHDINFO, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (!FilenameOrUuid)
        return errorSyntax(USAGE_SHOWHDINFO, "Disk name or UUID required");

    ComPtr<IMedium> hardDisk;
    bool unknown = false;
    /* first guess is that it's a UUID */
    Bstr uuid(FilenameOrUuid);
    rc = a->virtualBox->GetHardDisk(uuid, hardDisk.asOutParam());
    /* no? then it must be a filename */
    if (FAILED (rc))
    {
        rc = a->virtualBox->FindHardDisk(Bstr(FilenameOrUuid), hardDisk.asOutParam());
        /* no? well, then it's an unkwnown image */
        if (FAILED (rc))
        {
            rc = a->virtualBox->OpenHardDisk(Bstr(FilenameOrUuid), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), hardDisk.asOutParam());
            if (rc == VBOX_E_FILE_ERROR)
            {
                char szFilenameAbs[RTPATH_MAX] = "";
                int vrc = RTPathAbs(FilenameOrUuid, szFilenameAbs, sizeof(szFilenameAbs));
                if (RT_FAILURE(vrc))
                {
                    RTPrintf("Cannot convert filename \"%s\" to absolute path\n", FilenameOrUuid);
                    return 1;
                }
                CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(szFilenameAbs), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), hardDisk.asOutParam()));
            }
            else if (FAILED(rc))
                CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(FilenameOrUuid), AccessMode_ReadWrite, false, Bstr(""), false, Bstr(""), hardDisk.asOutParam()));
            if (SUCCEEDED (rc))
            {
                unknown = true;
            }
        }
    }
    do
    {
        if (!SUCCEEDED(rc))
            break;

        hardDisk->COMGETTER(Id)(uuid.asOutParam());
        RTPrintf("UUID:                 %s\n", Utf8Str(uuid).raw());

        /* check for accessibility */
        /// @todo NEWMEDIA check accessibility of all parents
        /// @todo NEWMEDIA print the full state value
        MediumState_T state;
        CHECK_ERROR_BREAK (hardDisk, RefreshState(&state));
        RTPrintf("Accessible:           %s\n", state != MediumState_Inaccessible ? "yes" : "no");

        if (state == MediumState_Inaccessible)
        {
            Bstr err;
            CHECK_ERROR_BREAK (hardDisk, COMGETTER(LastAccessError)(err.asOutParam()));
            RTPrintf("Access Error:         %lS\n", err.raw());
        }

        Bstr description;
        hardDisk->COMGETTER(Description)(description.asOutParam());
        if (description)
        {
            RTPrintf("Description:          %lS\n", description.raw());
        }

        ULONG64 logicalSize;
        hardDisk->COMGETTER(LogicalSize)(&logicalSize);
        RTPrintf("Logical size:         %llu MBytes\n", logicalSize);
        ULONG64 actualSize;
        hardDisk->COMGETTER(Size)(&actualSize);
        RTPrintf("Current size on disk: %llu MBytes\n", actualSize >> 20);

        ComPtr <IMedium> parent;
        hardDisk->COMGETTER(Parent) (parent.asOutParam());

        MediumType_T type;
        hardDisk->COMGETTER(Type)(&type);
        const char *typeStr = "unknown";
        switch (type)
        {
            case MediumType_Normal:
                if (!parent.isNull())
                    typeStr = "normal (differencing)";
                else
                    typeStr = "normal (base)";
                break;
            case MediumType_Immutable:
                typeStr = "immutable";
                break;
            case MediumType_Writethrough:
                typeStr = "writethrough";
                break;
        }
        RTPrintf("Type:                 %s\n", typeStr);

        Bstr format;
        hardDisk->COMGETTER(Format)(format.asOutParam());
        RTPrintf("Storage format:       %lS\n", format.raw());

        if (!unknown)
        {
            com::SafeArray<BSTR> machineIds;
            hardDisk->COMGETTER(MachineIds)(ComSafeArrayAsOutParam(machineIds));
            for (size_t j = 0; j < machineIds.size(); ++ j)
            {
                ComPtr<IMachine> machine;
                CHECK_ERROR(a->virtualBox, GetMachine(machineIds[j], machine.asOutParam()));
                ASSERT(machine);
                Bstr name;
                machine->COMGETTER(Name)(name.asOutParam());
                machine->COMGETTER(Id)(uuid.asOutParam());
                RTPrintf("%s%lS (UUID: %lS)\n",
                         j == 0 ? "In use by VMs:        " : "                      ",
                         name.raw(), machineIds[j]);
            }
            /// @todo NEWMEDIA check usage in snapshots too
            /// @todo NEWMEDIA also list children
        }

        Bstr loc;
        hardDisk->COMGETTER(Location)(loc.asOutParam());
        RTPrintf("Location:             %lS\n", loc.raw());

        /* print out information specific for differencing hard disks */
        if (!parent.isNull())
        {
            BOOL autoReset = FALSE;
            hardDisk->COMGETTER(AutoReset)(&autoReset);
            RTPrintf("Auto-Reset:           %s\n", autoReset ? "on" : "off");
        }
    }
    while (0);

    if (unknown)
    {
        /* close the unknown hard disk to forget it again */
        hardDisk->Close();
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

static const RTGETOPTDEF g_aOpenMediumOptions[] =
{
    { "disk",           'd', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'f', RTGETOPT_REQ_NOTHING },
    { "--type",         't', RTGETOPT_REQ_STRING },
    { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
    { "--uuid",         'u', RTGETOPT_REQ_UUID },
    { "--parentuuid",   'p', RTGETOPT_REQ_UUID },
};

int handleOpenMedium(HandlerArg *a)
{
    HRESULT rc = S_OK;
    int vrc;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *Filename = NULL;
    MediumType_T DiskType = MediumType_Normal;
    bool fDiskType = false;
    bool fSetImageId = false;
    bool fSetParentId = false;
    Guid ImageId;
    ImageId.clear();
    Guid ParentId;
    ParentId.clear();

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aOpenMediumOptions, RT_ELEMENTS(g_aOpenMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_OPENMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_OPENMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'f':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_OPENMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case 't':   // --type
                vrc = parseDiskType(ValueUnion.psz, &DiskType);
                if (RT_FAILURE(vrc))
                    return errorArgument("Invalid hard disk type '%s'", ValueUnion.psz);
                fDiskType = true;
                break;

            case 'u':   // --uuid
                ImageId = ValueUnion.Uuid;
                fSetImageId = true;
                break;

            case 'p':   // --parentuuid
                ParentId = ValueUnion.Uuid;
                fSetParentId = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!Filename)
                    Filename = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_OPENMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_OPENMEDIUM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_OPENMEDIUM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_OPENMEDIUM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_OPENMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_OPENMEDIUM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (cmd == CMD_NONE)
        return errorSyntax(USAGE_OPENMEDIUM, "Command variant disk/dvd/floppy required");
    if (!Filename)
        return errorSyntax(USAGE_OPENMEDIUM, "Disk name required");

    /** @todo remove this hack!
     * First try opening the image as is (using the regular API semantics for
     * images with relative path or without path), and if that fails with a
     * file related error then try it again with what the client thinks the
     * relative path would mean. Requires doing the command twice in certain
     * cases. This is an ugly hack and needs to be removed whevever we have a
     * chance to clean up the API semantics. */
    if (cmd == CMD_DISK)
    {
        ComPtr<IMedium> hardDisk;
        Bstr ImageIdStr = BstrFmt("%RTuuid", &ImageId);
        Bstr ParentIdStr = BstrFmt("%RTuuid", &ParentId);
        rc = a->virtualBox->OpenHardDisk(Bstr(Filename), AccessMode_ReadWrite, fSetImageId, ImageIdStr, fSetParentId, ParentIdStr, hardDisk.asOutParam());
        if (rc == VBOX_E_FILE_ERROR)
        {
            char szFilenameAbs[RTPATH_MAX] = "";
            int irc = RTPathAbs(Filename, szFilenameAbs, sizeof(szFilenameAbs));
            if (RT_FAILURE(irc))
            {
                RTPrintf("Cannot convert filename \"%s\" to absolute path\n", Filename);
                return 1;
            }
            CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(szFilenameAbs), AccessMode_ReadWrite, fSetImageId, ImageIdStr, fSetParentId, ParentIdStr, hardDisk.asOutParam()));
        }
        else if (FAILED(rc))
            CHECK_ERROR(a->virtualBox, OpenHardDisk(Bstr(Filename), AccessMode_ReadWrite, fSetImageId, ImageIdStr, fSetParentId, ParentIdStr, hardDisk.asOutParam()));
        if (SUCCEEDED(rc) && hardDisk)
        {
            /* change the type if requested */
            if (DiskType != MediumType_Normal)
            {
                CHECK_ERROR(hardDisk, COMSETTER(Type)(DiskType));
            }
        }
    }
    else if (cmd == CMD_DVD)
    {
        if (fDiskType || fSetParentId)
            return errorSyntax(USAGE_OPENMEDIUM, "Invalid option for DVD images");
        Bstr ImageIdStr = BstrFmt("%RTuuid", &ImageId);
        ComPtr<IMedium> dvdImage;
        rc = a->virtualBox->OpenDVDImage(Bstr(Filename), ImageIdStr, dvdImage.asOutParam());
        if (rc == VBOX_E_FILE_ERROR)
        {
            char szFilenameAbs[RTPATH_MAX] = "";
            int irc = RTPathAbs(Filename, szFilenameAbs, sizeof(szFilenameAbs));
            if (RT_FAILURE(irc))
            {
                RTPrintf("Cannot convert filename \"%s\" to absolute path\n", Filename);
                return 1;
            }
            CHECK_ERROR(a->virtualBox, OpenDVDImage(Bstr(szFilenameAbs), ImageIdStr, dvdImage.asOutParam()));
        }
        else if (FAILED(rc))
            CHECK_ERROR(a->virtualBox, OpenDVDImage(Bstr(Filename), ImageIdStr, dvdImage.asOutParam()));
    }
    else if (cmd == CMD_FLOPPY)
    {
        if (fDiskType || fSetImageId || fSetParentId)
            return errorSyntax(USAGE_OPENMEDIUM, "Invalid option for floppy images");
        Bstr ImageIdStr = BstrFmt("%RTuuid", &ImageId);
        ComPtr<IMedium> floppyImage;
        rc = a->virtualBox->OpenFloppyImage(Bstr(Filename), ImageIdStr, floppyImage.asOutParam());
        if (rc == VBOX_E_FILE_ERROR)
        {
            char szFilenameAbs[RTPATH_MAX] = "";
            int irc = RTPathAbs(Filename, szFilenameAbs, sizeof(szFilenameAbs));
            if (RT_FAILURE(irc))
            {
                RTPrintf("Cannot convert filename \"%s\" to absolute path\n", Filename);
                return 1;
            }
            CHECK_ERROR(a->virtualBox, OpenFloppyImage(Bstr(szFilenameAbs), ImageIdStr, floppyImage.asOutParam()));
        }
        else if (FAILED(rc))
            CHECK_ERROR(a->virtualBox, OpenFloppyImage(Bstr(Filename), ImageIdStr, floppyImage.asOutParam()));
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

static const RTGETOPTDEF g_aCloseMediumOptions[] =
{
    { "disk",           'd', RTGETOPT_REQ_NOTHING },
    { "dvd",            'D', RTGETOPT_REQ_NOTHING },
    { "floppy",         'f', RTGETOPT_REQ_NOTHING },
    { "--delete",       'r', RTGETOPT_REQ_NOTHING },
};

int handleCloseMedium(HandlerArg *a)
{
    HRESULT rc = S_OK;
    enum {
        CMD_NONE,
        CMD_DISK,
        CMD_DVD,
        CMD_FLOPPY
    } cmd = CMD_NONE;
    const char *FilenameOrUuid = NULL;
    bool fDelete = false;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aCloseMediumOptions, RT_ELEMENTS(g_aCloseMediumOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // disk
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DISK;
                break;

            case 'D':   // DVD
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_DVD;
                break;

            case 'f':   // floppy
                if (cmd != CMD_NONE)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Only one command can be specified: '%s'", ValueUnion.psz);
                cmd = CMD_FLOPPY;
                break;

            case 'r':   // --delete
                fDelete = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!FilenameOrUuid)
                    FilenameOrUuid = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_CLOSEMEDIUM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_CLOSEMEDIUM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_CLOSEMEDIUM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_CLOSEMEDIUM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_CLOSEMEDIUM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (cmd == CMD_NONE)
        return errorSyntax(USAGE_CLOSEMEDIUM, "Command variant disk/dvd/floppy required");
    if (!FilenameOrUuid)
        return errorSyntax(USAGE_CLOSEMEDIUM, "Disk name or UUID required");

    ComPtr<IMedium> medium;

    /* first guess is that it's a UUID */
    Bstr uuid(FilenameOrUuid);

    if (cmd == CMD_DISK)
    {
        rc = a->virtualBox->GetHardDisk(uuid, medium.asOutParam());
        /* not a UUID or not registered? Then it must be a filename */
        if (!medium)
        {
            CHECK_ERROR(a->virtualBox, FindHardDisk(Bstr(FilenameOrUuid), medium.asOutParam()));
        }
    }
    else
    if (cmd == CMD_DVD)
    {
        rc = a->virtualBox->GetDVDImage(uuid, medium.asOutParam());
        /* not a UUID or not registered? Then it must be a filename */
        if (!medium)
        {
            CHECK_ERROR(a->virtualBox, FindDVDImage(Bstr(FilenameOrUuid), medium.asOutParam()));
        }
    }
    else
    if (cmd == CMD_FLOPPY)
    {
        rc = a->virtualBox->GetFloppyImage(uuid, medium.asOutParam());
        /* not a UUID or not registered? Then it must be a filename */
        if (!medium)
        {
            CHECK_ERROR(a->virtualBox, FindFloppyImage(Bstr(FilenameOrUuid), medium.asOutParam()));
        }
    }

    if (SUCCEEDED(rc) && medium)
    {
        if (fDelete)
        {
            ComPtr<IProgress> progress;
            CHECK_ERROR(medium, DeleteStorage(progress.asOutParam()));
            if (SUCCEEDED(rc))
            {
                rc = showProgress(progress);
                if (FAILED(rc))
                {
                    com::ProgressErrorInfo info(progress);
                    if (info.isBasicAvailable())
                        RTPrintf("Error: failed to delete medium. Error message: %lS\n", info.getText().raw());
                    else
                        RTPrintf("Error: failed to delete medium. No error message available!\n");
                }
            }
            else
                RTPrintf("Error: failed to delete medium. Error code %Rrc\n", rc);
        }
        CHECK_ERROR(medium, Close());
    }

    return SUCCEEDED(rc) ? 0 : 1;
}
#endif /* !VBOX_ONLY_DOCS */

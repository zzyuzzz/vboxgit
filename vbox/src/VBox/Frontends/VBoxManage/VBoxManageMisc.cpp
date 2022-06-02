/* $Id: VBoxManageMisc.cpp 34292 2010-11-23 16:06:39Z vboxsync $ */
/** @file
 * VBoxManage - VirtualBox's command-line interface.
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
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#ifndef VBOX_ONLY_DOCS
#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>
#include <VBox/com/EventQueue.h>

#include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <iprt/asm.h>
#include <iprt/buildconfig.h>
#include <iprt/cidr.h>
#include <iprt/ctype.h>
#include <iprt/dir.h>
#include <iprt/env.h>
#include <VBox/err.h>
#include <iprt/file.h>
#include <iprt/initterm.h>
#include <iprt/param.h>
#include <iprt/path.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/stdarg.h>
#include <iprt/thread.h>
#include <iprt/uuid.h>
#include <iprt/getopt.h>
#include <iprt/ctype.h>
#include <VBox/version.h>
#include <VBox/log.h>

#include "VBoxManage.h"

using namespace com;



int handleRegisterVM(HandlerArg *a)
{
    HRESULT rc;

    if (a->argc != 1)
        return errorSyntax(USAGE_REGISTERVM, "Incorrect number of parameters");

    ComPtr<IMachine> machine;
    /** @todo Ugly hack to get both the API interpretation of relative paths
     * and the client's interpretation of relative paths. Remove after the API
     * has been redesigned. */
    rc = a->virtualBox->OpenMachine(Bstr(a->argv[0]).raw(),
                                    machine.asOutParam());
    if (rc == VBOX_E_FILE_ERROR)
    {
        char szVMFileAbs[RTPATH_MAX] = "";
        int vrc = RTPathAbs(a->argv[0], szVMFileAbs, sizeof(szVMFileAbs));
        if (RT_FAILURE(vrc))
        {
            RTMsgError("Cannot convert filename \"%s\" to absolute path", a->argv[0]);
            return 1;
        }
        CHECK_ERROR(a->virtualBox, OpenMachine(Bstr(szVMFileAbs).raw(),
                                               machine.asOutParam()));
    }
    else if (FAILED(rc))
        CHECK_ERROR(a->virtualBox, OpenMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()));
    if (SUCCEEDED(rc))
    {
        ASSERT(machine);
        CHECK_ERROR(a->virtualBox, RegisterMachine(machine));
    }
    return SUCCEEDED(rc) ? 0 : 1;
}

static const RTGETOPTDEF g_aUnregisterVMOptions[] =
{
    { "--delete",       'd', RTGETOPT_REQ_NOTHING },
    { "-delete",        'd', RTGETOPT_REQ_NOTHING },    // deprecated
};

int handleUnregisterVM(HandlerArg *a)
{
    HRESULT rc;
    const char *VMName = NULL;
    bool fDelete = false;

    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, g_aUnregisterVMOptions, RT_ELEMENTS(g_aUnregisterVMOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 'd':   // --delete
                fDelete = true;
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!VMName)
                    VMName = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_UNREGISTERVM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_UNREGISTERVM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_UNREGISTERVM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_UNREGISTERVM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_UNREGISTERVM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_UNREGISTERVM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (!VMName)
        return errorSyntax(USAGE_UNREGISTERVM, "VM name required");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(VMName).raw(),
                                           machine.asOutParam()));
    if (machine)
    {
        SafeIfaceArray<IMedium> aMedia;
        CleanupMode_T cleanupMode = CleanupMode_DetachAllReturnNone;
        if (fDelete)
            cleanupMode = CleanupMode_DetachAllReturnHardDisksOnly;
        CHECK_ERROR(machine, Unregister(cleanupMode,
                                        ComSafeArrayAsOutParam(aMedia)));
        if (SUCCEEDED(rc))
        {
            if (fDelete)
            {
                ComPtr<IProgress> pProgress;
                CHECK_ERROR(machine, Delete(ComSafeArrayAsInParam(aMedia), pProgress.asOutParam()));
                CHECK_ERROR(pProgress, WaitForCompletion(-1));
            }
        }
    }
    return SUCCEEDED(rc) ? 0 : 1;
}

int handleCreateVM(HandlerArg *a)
{
    HRESULT rc;
    Bstr baseFolder;
    Bstr name;
    Bstr osTypeId;
    RTUUID id;
    bool fRegister = false;

    RTUuidClear(&id);
    for (int i = 0; i < a->argc; i++)
    {
        if (   !strcmp(a->argv[i], "--basefolder")
            || !strcmp(a->argv[i], "-basefolder"))
        {
            if (a->argc <= i + 1)
                return errorArgument("Missing argument to '%s'", a->argv[i]);
            i++;
            baseFolder = a->argv[i];
        }
        else if (   !strcmp(a->argv[i], "--name")
                 || !strcmp(a->argv[i], "-name"))
        {
            if (a->argc <= i + 1)
                return errorArgument("Missing argument to '%s'", a->argv[i]);
            i++;
            name = a->argv[i];
        }
        else if (   !strcmp(a->argv[i], "--ostype")
                 || !strcmp(a->argv[i], "-ostype"))
        {
            if (a->argc <= i + 1)
                return errorArgument("Missing argument to '%s'", a->argv[i]);
            i++;
            osTypeId = a->argv[i];
        }
        else if (   !strcmp(a->argv[i], "--uuid")
                 || !strcmp(a->argv[i], "-uuid"))
        {
            if (a->argc <= i + 1)
                return errorArgument("Missing argument to '%s'", a->argv[i]);
            i++;
            if (RT_FAILURE(RTUuidFromStr(&id, a->argv[i])))
                return errorArgument("Invalid UUID format %s\n", a->argv[i]);
        }
        else if (   !strcmp(a->argv[i], "--register")
                 || !strcmp(a->argv[i], "-register"))
        {
            fRegister = true;
        }
        else
            return errorSyntax(USAGE_CREATEVM, "Invalid parameter '%s'", Utf8Str(a->argv[i]).c_str());
    }

    /* check for required options */
    if (name.isEmpty())
        return errorSyntax(USAGE_CREATEVM, "Parameter --name is required");

    do
    {
        Bstr bstrSettingsFile;
        CHECK_ERROR_BREAK(a->virtualBox,
                          ComposeMachineFilename(name.raw(),
                                                 baseFolder.raw(),
                                                 bstrSettingsFile.asOutParam()));
        ComPtr<IMachine> machine;
        CHECK_ERROR_BREAK(a->virtualBox,
                          CreateMachine(bstrSettingsFile.raw(),
                                        name.raw(),
                                        osTypeId.raw(),
                                        Guid(id).toUtf16().raw(),
                                        FALSE /* forceOverwrite */,
                                        machine.asOutParam()));

        CHECK_ERROR_BREAK(machine, SaveSettings());
        if (fRegister)
        {
            CHECK_ERROR_BREAK(a->virtualBox, RegisterMachine(machine));
        }
        Bstr uuid;
        CHECK_ERROR_BREAK(machine, COMGETTER(Id)(uuid.asOutParam()));
        Bstr settingsFile;
        CHECK_ERROR_BREAK(machine, COMGETTER(SettingsFilePath)(settingsFile.asOutParam()));
        RTPrintf("Virtual machine '%ls' is created%s.\n"
                 "UUID: %s\n"
                 "Settings file: '%ls'\n",
                 name.raw(), fRegister ? " and registered" : "",
                 Utf8Str(uuid).c_str(), settingsFile.raw());
    }
    while (0);

    return SUCCEEDED(rc) ? 0 : 1;
}

int handleStartVM(HandlerArg *a)
{
    HRESULT rc;
    const char *VMName = NULL;
    Bstr sessionType = "gui";

    static const RTGETOPTDEF s_aStartVMOptions[] =
    {
        { "--type",         't', RTGETOPT_REQ_STRING },
        { "-type",          't', RTGETOPT_REQ_STRING },     // deprecated
    };
    int c;
    RTGETOPTUNION ValueUnion;
    RTGETOPTSTATE GetState;
    // start at 0 because main() has hacked both the argc and argv given to us
    RTGetOptInit(&GetState, a->argc, a->argv, s_aStartVMOptions, RT_ELEMENTS(s_aStartVMOptions),
                 0, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
    while ((c = RTGetOpt(&GetState, &ValueUnion)))
    {
        switch (c)
        {
            case 't':   // --type
                if (!RTStrICmp(ValueUnion.psz, "gui"))
                {
                    sessionType = "gui";
                }
#ifdef VBOX_WITH_VBOXSDL
                else if (!RTStrICmp(ValueUnion.psz, "sdl"))
                {
                    sessionType = "sdl";
                }
#endif
#ifdef VBOX_WITH_HEADLESS
                else if (!RTStrICmp(ValueUnion.psz, "capture"))
                {
                    sessionType = "capture";
                }
                else if (!RTStrICmp(ValueUnion.psz, "headless"))
                {
                    sessionType = "headless";
                }
#endif
                else
                    return errorArgument("Invalid session type '%s'", ValueUnion.psz);
                break;

            case VINF_GETOPT_NOT_OPTION:
                if (!VMName)
                    VMName = ValueUnion.psz;
                else
                    return errorSyntax(USAGE_STARTVM, "Invalid parameter '%s'", ValueUnion.psz);
                break;

            default:
                if (c > 0)
                {
                    if (RT_C_IS_PRINT(c))
                        return errorSyntax(USAGE_STARTVM, "Invalid option -%c", c);
                    else
                        return errorSyntax(USAGE_STARTVM, "Invalid option case %i", c);
                }
                else if (c == VERR_GETOPT_UNKNOWN_OPTION)
                    return errorSyntax(USAGE_STARTVM, "unknown option: %s\n", ValueUnion.psz);
                else if (ValueUnion.pDef)
                    return errorSyntax(USAGE_STARTVM, "%s: %Rrs", ValueUnion.pDef->pszLong, c);
                else
                    return errorSyntax(USAGE_STARTVM, "error: %Rrs", c);
        }
    }

    /* check for required options */
    if (!VMName)
        return errorSyntax(USAGE_STARTVM, "VM name required");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(VMName).raw(),
                                           machine.asOutParam()));
    if (machine)
    {
        Bstr env;
#if defined(RT_OS_LINUX) || defined(RT_OS_SOLARIS)
        /* make sure the VM process will start on the same display as VBoxManage */
        Utf8Str str;
        const char *pszDisplay = RTEnvGet("DISPLAY");
        if (pszDisplay)
            str = Utf8StrFmt("DISPLAY=%s\n", pszDisplay);
        const char *pszXAuth = RTEnvGet("XAUTHORITY");
        if (pszXAuth)
            str.append(Utf8StrFmt("XAUTHORITY=%s\n", pszXAuth));
        env = str;
#endif
        ComPtr<IProgress> progress;
        CHECK_ERROR_RET(machine, LaunchVMProcess(a->session, sessionType.raw(),
                                                 env.raw(), progress.asOutParam()), rc);
        RTPrintf("Waiting for the VM to power on...\n");
        CHECK_ERROR_RET(progress, WaitForCompletion(-1), 1);

        BOOL completed;
        CHECK_ERROR_RET(progress, COMGETTER(Completed)(&completed), rc);
        ASSERT(completed);

        LONG iRc;
        CHECK_ERROR_RET(progress, COMGETTER(ResultCode)(&iRc), rc);
        if (FAILED(iRc))
        {
            ComPtr<IVirtualBoxErrorInfo> errorInfo;
            CHECK_ERROR_RET(progress, COMGETTER(ErrorInfo)(errorInfo.asOutParam()), 1);
            ErrorInfo info(errorInfo, COM_IIDOF(IVirtualBoxErrorInfo));
            com::GluePrintErrorInfo(info);
        }
        else
        {
            RTPrintf("VM has been successfully started.\n");
        }
    }

    /* it's important to always close sessions */
    a->session->UnlockMachine();

    return SUCCEEDED(rc) ? 0 : 1;
}

int handleDiscardState(HandlerArg *a)
{
    HRESULT rc;

    if (a->argc != 1)
        return errorSyntax(USAGE_DISCARDSTATE, "Incorrect number of parameters");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                           machine.asOutParam()));
    if (machine)
    {
        do
        {
            /* we have to open a session for this task */
            CHECK_ERROR_BREAK(machine, LockMachine(a->session, LockType_Write));
            do
            {
                ComPtr<IConsole> console;
                CHECK_ERROR_BREAK(a->session, COMGETTER(Console)(console.asOutParam()));
                CHECK_ERROR_BREAK(console, DiscardSavedState(true /* fDeleteFile */));
            } while (0);
            CHECK_ERROR_BREAK(a->session, UnlockMachine());
        } while (0);
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

int handleAdoptState(HandlerArg *a)
{
    HRESULT rc;

    if (a->argc != 2)
        return errorSyntax(USAGE_ADOPTSTATE, "Incorrect number of parameters");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                           machine.asOutParam()));
    if (machine)
    {
        do
        {
            /* we have to open a session for this task */
            CHECK_ERROR_BREAK(machine, LockMachine(a->session, LockType_Write));
            do
            {
                ComPtr<IConsole> console;
                CHECK_ERROR_BREAK(a->session, COMGETTER(Console)(console.asOutParam()));
                CHECK_ERROR_BREAK(console, AdoptSavedState(Bstr(a->argv[1]).raw()));
            } while (0);
            CHECK_ERROR_BREAK(a->session, UnlockMachine());
        } while (0);
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

int handleGetExtraData(HandlerArg *a)
{
    HRESULT rc = S_OK;

    if (a->argc != 2)
        return errorSyntax(USAGE_GETEXTRADATA, "Incorrect number of parameters");

    /* global data? */
    if (!strcmp(a->argv[0], "global"))
    {
        /* enumeration? */
        if (!strcmp(a->argv[1], "enumerate"))
        {
            SafeArray<BSTR> aKeys;
            CHECK_ERROR(a->virtualBox, GetExtraDataKeys(ComSafeArrayAsOutParam(aKeys)));

            for (size_t i = 0;
                 i < aKeys.size();
                 ++i)
            {
                Bstr bstrKey(aKeys[i]);
                Bstr bstrValue;
                CHECK_ERROR(a->virtualBox, GetExtraData(bstrKey.raw(),
                                                        bstrValue.asOutParam()));

                RTPrintf("Key: %lS, Value: %lS\n", bstrKey.raw(), bstrValue.raw());
            }
        }
        else
        {
            Bstr value;
            CHECK_ERROR(a->virtualBox, GetExtraData(Bstr(a->argv[1]).raw(),
                                                    value.asOutParam()));
            if (!value.isEmpty())
                RTPrintf("Value: %lS\n", value.raw());
            else
                RTPrintf("No value set!\n");
        }
    }
    else
    {
        ComPtr<IMachine> machine;
        CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()));
        if (machine)
        {
            /* enumeration? */
            if (!strcmp(a->argv[1], "enumerate"))
            {
                SafeArray<BSTR> aKeys;
                CHECK_ERROR(machine, GetExtraDataKeys(ComSafeArrayAsOutParam(aKeys)));

                for (size_t i = 0;
                    i < aKeys.size();
                    ++i)
                {
                    Bstr bstrKey(aKeys[i]);
                    Bstr bstrValue;
                    CHECK_ERROR(machine, GetExtraData(bstrKey.raw(),
                                                      bstrValue.asOutParam()));

                    RTPrintf("Key: %lS, Value: %lS\n", bstrKey.raw(), bstrValue.raw());
                }
            }
            else
            {
                Bstr value;
                CHECK_ERROR(machine, GetExtraData(Bstr(a->argv[1]).raw(),
                                                  value.asOutParam()));
                if (!value.isEmpty())
                    RTPrintf("Value: %lS\n", value.raw());
                else
                    RTPrintf("No value set!\n");
            }
        }
    }
    return SUCCEEDED(rc) ? 0 : 1;
}

int handleSetExtraData(HandlerArg *a)
{
    HRESULT rc = S_OK;

    if (a->argc < 2)
        return errorSyntax(USAGE_SETEXTRADATA, "Not enough parameters");

    /* global data? */
    if (!strcmp(a->argv[0], "global"))
    {
        /** @todo passing NULL is deprecated */
        if (a->argc < 3)
            CHECK_ERROR(a->virtualBox, SetExtraData(Bstr(a->argv[1]).raw(),
                                                    NULL));
        else if (a->argc == 3)
            CHECK_ERROR(a->virtualBox, SetExtraData(Bstr(a->argv[1]).raw(),
                                                    Bstr(a->argv[2]).raw()));
        else
            return errorSyntax(USAGE_SETEXTRADATA, "Too many parameters");
    }
    else
    {
        ComPtr<IMachine> machine;
        CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                               machine.asOutParam()));
        if (machine)
        {
            /** @todo passing NULL is deprecated */
            if (a->argc < 3)
                CHECK_ERROR(machine, SetExtraData(Bstr(a->argv[1]).raw(),
                                                  NULL));
            else if (a->argc == 3)
                CHECK_ERROR(machine, SetExtraData(Bstr(a->argv[1]).raw(),
                                                  Bstr(a->argv[2]).raw()));
            else
                return errorSyntax(USAGE_SETEXTRADATA, "Too many parameters");
        }
    }
    return SUCCEEDED(rc) ? 0 : 1;
}

int handleSetProperty(HandlerArg *a)
{
    HRESULT rc;

    /* there must be two arguments: property name and value */
    if (a->argc != 2)
        return errorSyntax(USAGE_SETPROPERTY, "Incorrect number of parameters");

    ComPtr<ISystemProperties> systemProperties;
    a->virtualBox->COMGETTER(SystemProperties)(systemProperties.asOutParam());

    if (!strcmp(a->argv[0], "machinefolder"))
    {
        /* reset to default? */
        if (!strcmp(a->argv[1], "default"))
            CHECK_ERROR(systemProperties, COMSETTER(DefaultMachineFolder)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(DefaultMachineFolder)(Bstr(a->argv[1]).raw()));
    }
    else if (   !strcmp(a->argv[0], "vrdeauthlibrary")
             || !strcmp(a->argv[0], "vrdpauthlibrary"))
    {
        if (!strcmp(a->argv[0], "vrdpauthlibrary"))
            RTStrmPrintf(g_pStdErr, "Warning: 'vrdpauthlibrary' is deprecated. Use 'vrdeauthlibrary'.\n");

        /* reset to default? */
        if (!strcmp(a->argv[1], "default"))
            CHECK_ERROR(systemProperties, COMSETTER(VRDEAuthLibrary)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(VRDEAuthLibrary)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "websrvauthlibrary"))
    {
        /* reset to default? */
        if (!strcmp(a->argv[1], "default"))
            CHECK_ERROR(systemProperties, COMSETTER(WebServiceAuthLibrary)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(WebServiceAuthLibrary)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "vrdeextpack"))
    {
        /* disable? */
        if (!strcmp(a->argv[1], "null"))
            CHECK_ERROR(systemProperties, COMSETTER(DefaultVRDEExtPack)(NULL));
        else
            CHECK_ERROR(systemProperties, COMSETTER(DefaultVRDEExtPack)(Bstr(a->argv[1]).raw()));
    }
    else if (!strcmp(a->argv[0], "loghistorycount"))
    {
        uint32_t uVal;
        int vrc;
        vrc = RTStrToUInt32Ex(a->argv[1], NULL, 0, &uVal);
        if (vrc != VINF_SUCCESS)
            return errorArgument("Error parsing Log history count '%s'", a->argv[1]);
        CHECK_ERROR(systemProperties, COMSETTER(LogHistoryCount)(uVal));
    }
    else
        return errorSyntax(USAGE_SETPROPERTY, "Invalid parameter '%s'", a->argv[0]);

    return SUCCEEDED(rc) ? 0 : 1;
}

int handleSharedFolder(HandlerArg *a)
{
    HRESULT rc;

    /* we need at least a command and target */
    if (a->argc < 2)
        return errorSyntax(USAGE_SHAREDFOLDER, "Not enough parameters");

    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[1]).raw(),
                                           machine.asOutParam()));
    if (!machine)
        return 1;

    if (!strcmp(a->argv[0], "add"))
    {
        /* we need at least four more parameters */
        if (a->argc < 5)
            return errorSyntax(USAGE_SHAREDFOLDER_ADD, "Not enough parameters");

        char *name = NULL;
        char *hostpath = NULL;
        bool fTransient = false;
        bool fWritable = true;
        bool fAutoMount = false;

        for (int i = 2; i < a->argc; i++)
        {
            if (   !strcmp(a->argv[i], "--name")
                || !strcmp(a->argv[i], "-name"))
            {
                if (a->argc <= i + 1 || !*a->argv[i+1])
                    return errorArgument("Missing argument to '%s'", a->argv[i]);
                i++;
                name = a->argv[i];
            }
            else if (   !strcmp(a->argv[i], "--hostpath")
                     || !strcmp(a->argv[i], "-hostpath"))
            {
                if (a->argc <= i + 1 || !*a->argv[i+1])
                    return errorArgument("Missing argument to '%s'", a->argv[i]);
                i++;
                hostpath = a->argv[i];
            }
            else if (   !strcmp(a->argv[i], "--readonly")
                     || !strcmp(a->argv[i], "-readonly"))
            {
                fWritable = false;
            }
            else if (   !strcmp(a->argv[i], "--transient")
                     || !strcmp(a->argv[i], "-transient"))
            {
                fTransient = true;
            }
            else if (   !strcmp(a->argv[i], "--automount")
                     || !strcmp(a->argv[i], "-automount"))
            {
                fAutoMount = true;
            }
            else
                return errorSyntax(USAGE_SHAREDFOLDER_ADD, "Invalid parameter '%s'", Utf8Str(a->argv[i]).c_str());
        }

        if (NULL != strstr(name, " "))
            return errorSyntax(USAGE_SHAREDFOLDER_ADD, "No spaces allowed in parameter '-name'!");

        /* required arguments */
        if (!name || !hostpath)
        {
            return errorSyntax(USAGE_SHAREDFOLDER_ADD, "Parameters --name and --hostpath are required");
        }

        if (fTransient)
        {
            ComPtr <IConsole> console;

            /* open an existing session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), 1);
            /* get the session machine */
            CHECK_ERROR_RET(a->session, COMGETTER(Machine)(machine.asOutParam()), 1);
            /* get the session console */
            CHECK_ERROR_RET(a->session, COMGETTER(Console)(console.asOutParam()), 1);

            CHECK_ERROR(console, CreateSharedFolder(Bstr(name).raw(),
                                                    Bstr(hostpath).raw(),
                                                    fWritable, fAutoMount));
            if (console)
                a->session->UnlockMachine();
        }
        else
        {
            /* open a session for the VM */
            SessionType_T st;
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Write), 1);

            /* get the mutable session machine */
            a->session->COMGETTER(Machine)(machine.asOutParam());

            CHECK_ERROR(machine, CreateSharedFolder(Bstr(name).raw(),
                                                    Bstr(hostpath).raw(),
                                                    fWritable, fAutoMount));
            if (SUCCEEDED(rc))
                CHECK_ERROR(machine, SaveSettings());

            a->session->UnlockMachine();
        }
    }
    else if (!strcmp(a->argv[0], "remove"))
    {
        /* we need at least two more parameters */
        if (a->argc < 3)
            return errorSyntax(USAGE_SHAREDFOLDER_REMOVE, "Not enough parameters");

        char *name = NULL;
        bool fTransient = false;

        for (int i = 2; i < a->argc; i++)
        {
            if (   !strcmp(a->argv[i], "--name")
                || !strcmp(a->argv[i], "-name"))
            {
                if (a->argc <= i + 1 || !*a->argv[i+1])
                    return errorArgument("Missing argument to '%s'", a->argv[i]);
                i++;
                name = a->argv[i];
            }
            else if (   !strcmp(a->argv[i], "--transient")
                     || !strcmp(a->argv[i], "-transient"))
            {
                fTransient = true;
            }
            else
                return errorSyntax(USAGE_SHAREDFOLDER_REMOVE, "Invalid parameter '%s'", Utf8Str(a->argv[i]).c_str());
        }

        /* required arguments */
        if (!name)
            return errorSyntax(USAGE_SHAREDFOLDER_REMOVE, "Parameter --name is required");

        if (fTransient)
        {
            ComPtr <IConsole> console;

            /* open an existing session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Shared), 1);
            /* get the session machine */
            CHECK_ERROR_RET(a->session, COMGETTER(Machine)(machine.asOutParam()), 1);
            /* get the session console */
            CHECK_ERROR_RET(a->session, COMGETTER(Console)(console.asOutParam()), 1);

            CHECK_ERROR(console, RemoveSharedFolder(Bstr(name).raw()));

            if (console)
                a->session->UnlockMachine();
        }
        else
        {
            /* open a session for the VM */
            CHECK_ERROR_RET(machine, LockMachine(a->session, LockType_Write), 1);

            /* get the mutable session machine */
            a->session->COMGETTER(Machine)(machine.asOutParam());

            CHECK_ERROR(machine, RemoveSharedFolder(Bstr(name).raw()));

            /* commit and close the session */
            CHECK_ERROR(machine, SaveSettings());
            a->session->UnlockMachine();
        }
    }
    else
        return errorSyntax(USAGE_SETPROPERTY, "Invalid parameter '%s'", Utf8Str(a->argv[0]).c_str());

    return 0;
}

int handleVMStatistics(HandlerArg *a)
{
    HRESULT rc;

    /* at least one option: the UUID or name of the VM */
    if (a->argc < 1)
        return errorSyntax(USAGE_VM_STATISTICS, "Incorrect number of parameters");

    /* try to find the given machine */
    ComPtr<IMachine> machine;
    CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]).raw(),
                                           machine.asOutParam()));
    if (FAILED(rc))
        return 1;

    /* parse arguments. */
    bool fReset = false;
    bool fWithDescriptions = false;
    const char *pszPattern = NULL; /* all */
    for (int i = 1; i < a->argc; i++)
    {
        if (   !strcmp(a->argv[i], "--pattern")
            || !strcmp(a->argv[i], "-pattern"))
        {
            if (pszPattern)
                return errorSyntax(USAGE_VM_STATISTICS, "Multiple --patterns options is not permitted");
            if (i + 1 >= a->argc)
                return errorArgument("Missing argument to '%s'", a->argv[i]);
            pszPattern = a->argv[++i];
        }
        else if (   !strcmp(a->argv[i], "--descriptions")
                 || !strcmp(a->argv[i], "-descriptions"))
            fWithDescriptions = true;
        /* add: --file <filename> and --formatted */
        else if (   !strcmp(a->argv[i], "--reset")
                 || !strcmp(a->argv[i], "-reset"))
            fReset = true;
        else
            return errorSyntax(USAGE_VM_STATISTICS, "Unknown option '%s'", a->argv[i]);
    }
    if (fReset && fWithDescriptions)
        return errorSyntax(USAGE_VM_STATISTICS, "The --reset and --descriptions options does not mix");


    /* open an existing session for the VM. */
    CHECK_ERROR(machine, LockMachine(a->session, LockType_Shared));
    if (SUCCEEDED(rc))
    {
        /* get the session console. */
        ComPtr <IConsole> console;
        CHECK_ERROR(a->session, COMGETTER(Console)(console.asOutParam()));
        if (SUCCEEDED(rc))
        {
            /* get the machine debugger. */
            ComPtr <IMachineDebugger> debugger;
            CHECK_ERROR(console, COMGETTER(Debugger)(debugger.asOutParam()));
            if (SUCCEEDED(rc))
            {
                if (fReset)
                    CHECK_ERROR(debugger, ResetStats(Bstr(pszPattern).raw()));
                else
                {
                    Bstr stats;
                    CHECK_ERROR(debugger, GetStats(Bstr(pszPattern).raw(),
                                                   fWithDescriptions,
                                                   stats.asOutParam()));
                    if (SUCCEEDED(rc))
                    {
                        /* if (fFormatted)
                         { big mess }
                         else
                         */
                        RTPrintf("%ls\n", stats.raw());
                    }
                }
            }
            a->session->UnlockMachine();
        }
    }

    return SUCCEEDED(rc) ? 0 : 1;
}

int handleExtPack(HandlerArg *a)
{
    if (a->argc < 1)
        return errorSyntax(USAGE_EXTPACK, "Incorrect number of parameters");

    ComObjPtr<IExtPackManager> ptrExtPackMgr;
    CHECK_ERROR2_RET(a->virtualBox, COMGETTER(ExtensionPackManager)(ptrExtPackMgr.asOutParam()), RTEXITCODE_FAILURE);

    RTGETOPTSTATE   GetState;
    RTGETOPTUNION   ValueUnion;
    int             ch;
    HRESULT         hrc = S_OK;

    if (!strcmp(a->argv[0], "install"))
    {
        if (a->argc > 2)
            return errorSyntax(USAGE_EXTPACK, "Too many parameters given to \"extpack install\"");

        char szPath[RTPATH_MAX];
        int vrc = RTPathAbs(a->argv[1], szPath, sizeof(szPath));
        if (RT_FAILURE(vrc))
            return RTMsgErrorExit(RTEXITCODE_FAILURE, "RTPathAbs(%s,,) failed with rc=%Rrc", a->argv[1], vrc);

        Bstr bstrTarball(szPath);
        Bstr bstrName;
        CHECK_ERROR2_RET(ptrExtPackMgr, Install(bstrTarball.raw(), bstrName.asOutParam()), RTEXITCODE_FAILURE);
        RTPrintf("Successfully installed \"%lS\".\n", bstrName.raw());
    }
    else if (!strcmp(a->argv[0], "uninstall"))
    {
        const char *pszName = NULL;
        bool        fForced = false;

        static const RTGETOPTDEF s_aUninstallOptions[] =
        {
            { "--forced",  'f', RTGETOPT_REQ_NOTHING },
        };

        RTGetOptInit(&GetState, a->argc, a->argv, s_aUninstallOptions, RT_ELEMENTS(s_aUninstallOptions),
                     1, RTGETOPTINIT_FLAGS_NO_STD_OPTS);
        while ((ch = RTGetOpt(&GetState, &ValueUnion)))
        {
            switch (ch)
            {
                case 'f':
                    fForced = true;
                    break;

                case VINF_GETOPT_NOT_OPTION:
                    if (pszName)
                        return errorSyntax(USAGE_EXTPACK, "Too many extension pack names given to \"extpack uninstall\"");
                    pszName = ValueUnion.psz;
                    break;

                default:
                    return errorGetOpt(USAGE_EXTPACK, ch, &ValueUnion);
            }
        }
        if (!pszName)
            return errorSyntax(USAGE_EXTPACK, "Not extension pack name was given to \"extpack uninstall\"");

        Bstr bstrName(pszName);
        CHECK_ERROR2_RET(ptrExtPackMgr, Uninstall(bstrName.raw(), fForced), RTEXITCODE_FAILURE);
        RTPrintf("Successfully uninstalled \"%s\".\n", pszName);
    }
    else if (!strcmp(a->argv[0], "cleanup"))
    {
        if (a->argc > 1)
            return errorSyntax(USAGE_EXTPACK, "Too many parameters given to \"extpack cleanup\"");

        CHECK_ERROR2_RET(ptrExtPackMgr, Cleanup(), RTEXITCODE_FAILURE);
        RTPrintf("Successfully performed extension pack cleanup\n");
    }
    else
        return errorSyntax(USAGE_EXTPACK, "Unknown command \"%s\"", a->argv[0]);

    return RTEXITCODE_SUCCESS;
}


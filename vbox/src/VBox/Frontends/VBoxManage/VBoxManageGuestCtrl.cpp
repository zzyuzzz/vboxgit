/* $Id: VBoxManageGuestCtrl.cpp 27927 2010-04-01 09:19:12Z vboxsync $ */
/** @file
 * VBoxManage - The 'guestcontrol' command.
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
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include "VBoxManage.h"

#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <VBox/com/VirtualBox.h>
#include <VBox/com/EventQueue.h>

#include <VBox/log.h>
#include <iprt/asm.h>
#include <iprt/stream.h>
#include <iprt/string.h>
#include <iprt/time.h>
#include <iprt/thread.h>

#ifdef USE_XPCOM_QUEUE
# include <sys/select.h>
# include <errno.h>
#endif

#ifdef RT_OS_DARWIN
# include <CoreFoundation/CFRunLoop.h>
#endif

using namespace com;

/**
 * IVirtualBoxCallback implementation for handling the GuestControlCallback in
 * relation to the "guestcontrol * wait" command.
 */
/** @todo */

void usageGuestControl(void)
{
    RTPrintf("VBoxManage guestcontrol     execute <vmname>|<uuid>\n"
             "                            <path to program> [--args <arguments>] [--env NAME=VALUE]\n"
             "                            [--flags <flags>] [--user <name> [--password <password>]]\n"
             "                            [--timeout <msec>]\n"
             "\n");
}

static int handleExecProgram(HandlerArg *a)
{
    HRESULT rc = S_OK;

    /*
     * Check the syntax.  We can deduce the correct syntax from the number of
     * arguments.
     */
    /** @todo */
    if (a->argc < 2) /* At least the command we want to execute should be present. */
        return errorSyntax(USAGE_GUESTCONTROL, "Incorrect parameters");
    Utf8Str Utf8Cmd(a->argv[1]);
    uint32_t uFlags = 0;
    Utf8Str Utf8Args;
    com::SafeArray <BSTR> env;
    Utf8Str Utf8StdIn;
    Utf8Str Utf8StdOut;
    Utf8Str Utf8StdErr;
    Utf8Str Utf8UserName;
    Utf8Str Utf8Password;
    uint32_t uTimeoutMS = 0;

    /* lookup VM. */
    ComPtr<IMachine> machine;
    /* assume it's an UUID */
    rc = a->virtualBox->GetMachine(Bstr(a->argv[0]), machine.asOutParam());
    if (FAILED(rc) || !machine)
    {
        /* must be a name */
        CHECK_ERROR(a->virtualBox, FindMachine(Bstr(a->argv[0]), machine.asOutParam()));
    }
    if (machine)
    {
        do
        {
            Bstr uuid;
            machine->COMGETTER(Id)(uuid.asOutParam());
    
            /* open an existing session for VM - so the VM has to be running */
            CHECK_ERROR_BREAK(a->virtualBox, OpenExistingSession(a->session, uuid));
    
            /* get the mutable session machine */
            a->session->COMGETTER(Machine)(machine.asOutParam());
    
            /* get the associated console */
            ComPtr<IConsole> console;
            CHECK_ERROR_BREAK(a->session, COMGETTER(Console)(console.asOutParam()));
    
            ComPtr<IGuest> guest;
            CHECK_ERROR_BREAK(console, COMGETTER(Guest)(guest.asOutParam()));

            ULONG uPID = 0;
            CHECK_ERROR_BREAK(guest, ExecuteProgram(Bstr(Utf8Cmd), uFlags, 
                                                    Bstr(Utf8Args), ComSafeArrayAsInParam(env), 
                                                    Bstr(Utf8StdIn), Bstr(Utf8StdOut), Bstr(Utf8StdErr),
                                                    Bstr(Utf8UserName), Bstr(Utf8Password), uTimeoutMS,
                                                    &uPID));
            a->session->Close();
        } while (0);
    }
    return SUCCEEDED(rc) ? 0 : 1;
}

/**
 * Access the guest control store.
 *
 * @returns 0 on success, 1 on failure
 * @note see the command line API description for parameters
 */
int handleGuestControl(HandlerArg *a)
{
    HandlerArg arg = *a;
    arg.argc = a->argc - 1;
    arg.argv = a->argv + 1;

    if (a->argc == 0)
        return errorSyntax(USAGE_GUESTCONTROL, "Incorrect parameters");

    /* switch (cmd) */
    if (strcmp(a->argv[0], "exec") == 0)
        return handleExecProgram(&arg);

    /* default: */
    return errorSyntax(USAGE_GUESTCONTROL, "Incorrect parameters");
}

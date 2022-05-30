/** @file libXPCOMtoC.cpp
 *
 * Utility functions to use with the C binding for XPCOM.
 *
 * $Id: libXPCOMtoC.cpp 16483 2009-02-03 10:52:11Z vboxsync $
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
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

#include <iostream>
#include <iomanip>

#include <nsMemory.h>
#include <nsString.h>
#include <nsIServiceManager.h>
#include <nsEventQueueUtils.h>

#include <iprt/string.h>

#include "VirtualBox_XPCOM.h"
#include "cbinding.h"

using namespace std;

static ISession            *Session;
static IVirtualBox         *Ivirtualbox;
static nsIServiceManager   *serviceManager;
static nsIComponentManager *manager;

int VBoxUtf16ToUtf8(const PRUnichar *pszString, char **ppwszString)
{
    return RTUtf16ToUtf8(pszString, ppwszString);
}

int VBoxStrToUtf16(const char *pszString, PRUnichar **ppwszString)
{
    return RTStrToUtf16(pszString, ppwszString);
}

void VBoxUtf16Free(PRUnichar *pwszString)
{
    RTUtf16Free(pwszString);
}

void VBoxStrFree(char *pszString)
{
    RTStrFree(pszString);
}

const PRUnichar* VBoxConvertUTF8toPRUnichar(char *src)
{
    return NS_ConvertUTF8toUTF16(src).get();
}

const char* VBoxConvertPRUnichartoUTF8(PRUnichar *src)
{
    return NS_ConvertUTF16toUTF8(src).get();
}

const PRUnichar* VBoxConvertAsciitoPRUnichar(char *src)
{
    return NS_ConvertASCIItoUTF16(src).get();
}

const char* VBoxConvertPRUnichartoAscii(PRUnichar *src)
{
    return NS_LossyConvertUTF16toASCII(src).get();
}

void VBoxComUnallocStr(PRUnichar *str_dealloc)
{
    if (str_dealloc) {
        nsMemory::Free(str_dealloc);
    }
}

void VBoxComUnallocIID(nsIID *iid)
{
    if (iid) {
        nsMemory::Free(iid);
    }
}

void VBoxComInitialize(IVirtualBox **virtualBox, ISession **session)
{
    nsresult rc;

    *session    = NULL;
    *virtualBox = NULL;

    Session     = *session;
    Ivirtualbox = *virtualBox;

    // All numbers on stderr in hex prefixed with 0X.
    cerr.setf(ios_base::showbase | ios_base::uppercase);
    cerr.setf(ios_base::hex, ios_base::basefield);

    rc = NS_InitXPCOM2(&serviceManager, nsnull, nsnull);
    if (NS_FAILED(rc))
    {
        cerr << "XPCOM could not be initialized! rc=" << rc << endl;
        VBoxComUninitialize();
        return;
    }

    rc = NS_GetComponentManager (&manager);
    if (NS_FAILED(rc))
    {
        cerr << "could not get component manager! rc=" << rc << endl;
        VBoxComUninitialize();
        return;
    }

    rc = manager->CreateInstanceByContractID(NS_VIRTUALBOX_CONTRACTID,
                                             nsnull,
                                             NS_GET_IID(IVirtualBox),
                                             (void **)virtualBox);
    if (NS_FAILED(rc))
    {
        cerr << "could not instantiate VirtualBox object! rc=" << rc << endl;
        VBoxComUninitialize();
        return;
    }

    cout << "VirtualBox object created." << endl;

    rc = manager->CreateInstanceByContractID (NS_SESSION_CONTRACTID,
                                              nsnull,
                                              NS_GET_IID(ISession),
                                              (void **)session);
    if (NS_FAILED(rc))
    {
        cerr << "could not instantiate Session object! rc=" << rc << endl;
        VBoxComUninitialize();
        return;
    }

    cout << "ISession object created." << endl;
}

void VBoxComUninitialize(void)
{
    if (Session)
        NS_RELEASE(Session);        // decrement refcount
    if (Ivirtualbox)
        NS_RELEASE(Ivirtualbox);    // decrement refcount
    if (manager)
        NS_RELEASE(manager);        // decrement refcount
    if (serviceManager)
        NS_RELEASE(serviceManager); // decrement refcount
    NS_ShutdownXPCOM(nsnull);
    cout << "Done!" << endl;
}

/* vim: set ts=4 sw=4 et: */

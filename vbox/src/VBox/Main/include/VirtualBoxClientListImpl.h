/* $Id: VirtualBoxClientListImpl.h 76066 2018-12-07 22:17:39Z vboxsync $ */
/** @file
 * Main - VBoxSDS - VirtualBoxClientList.
 */

/*
 * Copyright (C) 2017-2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_VIRTUALBOXCLIENTLISTIMPL
#define ____H_VIRTUALBOXCLIENTLISTIMPL

#include <vector>
#include <set>
#include <memory>
#include <iprt/thread.h>
#include <iprt/semaphore.h>

#include "VirtualBoxBase.h"
#include "VirtualBoxClientListWrap.h"

typedef std::set<LONG> TClientSet;

class CClientListWatcher;

/**
 * The IVirtualBoxClientList implementation.
 *
 * This class provides COM interface to track and get list of
 * API client processes.
 */
class ATL_NO_VTABLE VirtualBoxClientList
    : public VirtualBoxClientListWrap
#ifdef RT_OS_WINDOWS
    , public ATL::CComCoClass<VirtualBoxClientList, &CLSID_VirtualBoxClientList>
#endif
{
private:
    /** Lock protecting m_ClientSet.*/
    RTCRITSECTRW    m_MapCritSect;
    TClientSet      m_ClientSet;

public:
    DECLARE_CLASSFACTORY_SINGLETON(VirtualBoxClientList) /**< r=bird: It is _NOT_ a singleton. */
    DECLARE_NOT_AGGREGATABLE(VirtualBoxClientList)
    VirtualBoxClientList() : m_pWatcher(NULL) {}
    virtual ~VirtualBoxClientList() {}

    HRESULT FinalConstruct();
    void    FinalRelease();

private:
    // VirtualBoxClientList methods
    HRESULT registerClient(LONG aPid);
    HRESULT getClients(std::vector<LONG> &aEnvironment);

    // Private members
    // polling of unexpectedly finished api client processes
    CClientListWatcher *m_pWatcher;
};



#endif // !____H_VIRTUALBOXCLIENTLISTIMPL


/* $Id: HostVideoCaptureDeviceImpl.h 48589 2013-09-20 13:36:09Z vboxsync $ */

/** @file
 *
 * A host video capture device description.
 */

/*
 * Copyright (C) 2013 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef HOSTVIDEOCAPTUREDEVICE_IMPL_H_
#define HOSTVIDEOCAPTUREDEVICE_IMPL_H_

#include "HostVideoCaptureDeviceWrap.h"

#include <list>

class HostVideoCaptureDevice;

typedef std::list<ComObjPtr<HostVideoCaptureDevice> > HostVideoCaptureDeviceList;

class ATL_NO_VTABLE HostVideoCaptureDevice :
    public HostVideoCaptureDeviceWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(HostVideoCaptureDevice)

    HRESULT FinalConstruct();
    void FinalRelease();

    /* Public initializer/uninitializer for internal purposes only. */
    HRESULT init(com::Utf8Str name, com::Utf8Str path, com::Utf8Str alias);
    void uninit();

    static HRESULT queryHostDevices(HostVideoCaptureDeviceList *pList);

private:

    // wrapped IHostVideoCaptureDevice properties
    virtual HRESULT getName(com::Utf8Str &aName) { aName = m.name; return S_OK; }
    virtual HRESULT getPath(com::Utf8Str &aPath) { aPath = m.path; return S_OK; }
    virtual HRESULT getAlias(com::Utf8Str &aAlias) { aAlias = m.alias; return S_OK; }

    /* Data. */
    struct Data
    {
        Data()
	{
	}

        com::Utf8Str name;
        com::Utf8Str path;
        com::Utf8Str alias;
    };

    Data m;
};

#endif // HOSTVIDEOCAPTUREDEVICE_IMPL_H_

/* vi: set tabstop=4 shiftwidth=4 expandtab: */

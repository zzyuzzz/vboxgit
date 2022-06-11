/* $Id: DHCPServerImpl.h 79644 2019-07-09 14:01:06Z vboxsync $ */
/** @file
 * VirtualBox COM class implementation
 */

/*
 * Copyright (C) 2006-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef MAIN_INCLUDED_DHCPServerImpl_h
#define MAIN_INCLUDED_DHCPServerImpl_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include "DHCPServerWrap.h"
#include <map>

namespace settings
{
    struct DHCPServer;
    struct DhcpOptValue;
    typedef std::map<DhcpOpt_T, DhcpOptValue> DhcpOptionMap;
}


/**
 *  for server configuration needs, it's perhaps better to use (VM,slot) pair
 *  (vm-name, slot) <----> (MAC)
 *
 *  but for client configuration, when server will have MACs at hand, it'd be
 *  easier to requiest options by MAC.
 *  (MAC) <----> (option-list)
 *
 *  Doubts: What should be done if MAC changed for (vm-name, slot), when syncing should?
 *  XML: serialization of dependecy (DHCP options) - (VM,slot) shouldn't be done via MAC in
 *  the middle.
 */
class ATL_NO_VTABLE DHCPServer
    : public DHCPServerWrap
{
public:

    DECLARE_EMPTY_CTOR_DTOR(DHCPServer)

    HRESULT FinalConstruct();
    void FinalRelease();

    HRESULT init(VirtualBox *aVirtualBox,
                 const com::Utf8Str &aName);
    HRESULT init(VirtualBox *aVirtualBox,
                 const settings::DHCPServer &data);
    void uninit();

    // Public internal methids.
    HRESULT i_saveSettings(settings::DHCPServer &data);
    settings::DhcpOptionMap &i_findOptMapByVmNameSlot(const com::Utf8Str &aVmName, LONG Slot);

private:
    HRESULT encodeOption(com::Utf8Str &aEncoded, uint32_t aOptCode, const settings::DhcpOptValue &aOptValue);
    int addOption(settings::DhcpOptionMap &aMap, DhcpOpt_T aOption, const com::Utf8Str &aValue);

    // wrapped IDHCPServer properties
    HRESULT getEventSource(ComPtr<IEventSource> &aEventSource);
    HRESULT getEnabled(BOOL *aEnabled);
    HRESULT setEnabled(BOOL aEnabled);
    HRESULT getIPAddress(com::Utf8Str &aIPAddress);
    HRESULT getNetworkMask(com::Utf8Str &aNetworkMask);
    HRESULT getNetworkName(com::Utf8Str &aName);
    HRESULT getLowerIP(com::Utf8Str &aIPAddress);
    HRESULT getUpperIP(com::Utf8Str &aIPAddress);
    HRESULT getGlobalOptions(std::vector<com::Utf8Str> &aGlobalOptions);
    HRESULT getVmConfigs(std::vector<com::Utf8Str> &aVmConfigs);
    HRESULT getMacOptions(const com::Utf8Str &aMAC, std::vector<com::Utf8Str> &aValues);
    HRESULT setConfiguration(const com::Utf8Str &aIPAddress,
                             const com::Utf8Str &aNetworkMask,
                             const com::Utf8Str &aFromIPAddress,
                             const com::Utf8Str &aToIPAddress);
    HRESULT getVmSlotOptions(const com::Utf8Str &aVmName,
                             LONG aSlot,
                             std::vector<com::Utf8Str> &aValues);

    // Wrapped IDHCPServer Methods
    HRESULT addGlobalOption(DhcpOpt_T aOption,
                            const com::Utf8Str &aValue);
    HRESULT removeGlobalOption(DhcpOpt_T aOption);
    HRESULT removeGlobalOptions();
    HRESULT addVmSlotOption(const com::Utf8Str &aVmName,
                            LONG aSlot,
                            DhcpOpt_T aOption,
                            const com::Utf8Str &aValue);
    HRESULT removeVmSlotOption(const com::Utf8Str &aVmName,
                               LONG aSlot,
                               DhcpOpt_T aOption);
    HRESULT removeVmSlotOptions(const com::Utf8Str &aVmName,
                                LONG aSlot);
    HRESULT start(const com::Utf8Str &aNetworkName,
                  const com::Utf8Str &aTrunkName,
                  const com::Utf8Str &aTrunkType);
    HRESULT stop();
    HRESULT restart();
    HRESULT findLeaseByMAC(const com::Utf8Str &aMac, LONG aType,
                           com::Utf8Str &aAddress, com::Utf8Str &aState, LONG64 *aIssued, LONG64 *aExpire) RT_OVERRIDE;

    /** @name Helpers
     * @{  */
    HRESULT i_calcLeasesFilename(const com::Utf8Str &aNetwork) RT_NOEXCEPT;
    /** @} */

    struct Data;
    Data *m;
    /** weak VirtualBox parent */
    VirtualBox *const mVirtualBox;
    const Utf8Str mName;
};

#endif /* !MAIN_INCLUDED_DHCPServerImpl_h */

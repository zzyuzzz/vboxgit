/* $Id: NATNetworkImpl.cpp 48107 2013-08-27 22:15:13Z vboxsync $ */
/** @file
 * INATNetwork implementation.
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

#include "NetworkServiceRunner.h"
#include "DHCPServerImpl.h"
#include "NATNetworkImpl.h"
#include "AutoCaller.h"
#include "Logging.h"

#include <iprt/asm.h>
#include <iprt/cpp/utils.h>
#include <iprt/cidr.h>
#include <iprt/net.h>
#include <VBox/com/array.h>
#include <VBox/com/ptr.h>
#include <VBox/settings.h>

#include "EventImpl.h"
#include "VBoxEvents.h"

#include "VirtualBoxImpl.h"
#include <algorithm>
#include <list>

#ifndef RT_OS_WINDOWS
# include <netinet/in.h>
#else
# define IN_LOOPBACKNET 127
#endif


// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

struct NATNetwork::Data
{
    Data()
      : fEnabled(FALSE)
      , fIPv6Enabled(FALSE)
      , fAdvertiseDefaultIPv6Route(FALSE)
      , fNeedDhcpServer(FALSE)
    {
        IPv4Gateway.setNull();
        IPv4NetworkCidr.setNull();
        IPv6Prefix.setNull();
        IPv4DhcpServer.setNull();
        IPv4NetworkMask.setNull();
        IPv4DhcpServerLowerIp.setNull();
        IPv4DhcpServerUpperIp.setNull();
    }
    virtual ~Data(){}
    const ComObjPtr<EventSource> pEventSource;
#ifdef VBOX_WITH_NAT_SERVICE
    NATNetworkServiceRunner NATRunner;
    ComObjPtr<IDHCPServer> dhcpServer;
#endif
    Bstr IPv4Gateway;
    Bstr IPv4NetworkCidr;
    Bstr IPv4NetworkMask;
    Bstr IPv4DhcpServer;
    Bstr IPv4DhcpServerLowerIp;
    Bstr IPv4DhcpServerUpperIp;
    BOOL fEnabled;
    BOOL fIPv6Enabled;
    Bstr IPv6Prefix;
    BOOL fAdvertiseDefaultIPv6Route;
    BOOL fNeedDhcpServer;
    NATRuleMap mapName2PortForwardRule4;
    NATRuleMap mapName2PortForwardRule6;
    settings::NATLoopbackOffsetList llNATLoopbackOffsetList;
    uint32_t u32LoopbackIp6;
    uint32_t offGateway;
    uint32_t offDhcp;
};

NATNetwork::NATNetwork()
    : mVirtualBox(NULL)
{
}

NATNetwork::~NATNetwork()
{
}

HRESULT NATNetwork::FinalConstruct()
{
    return BaseFinalConstruct();
}

void NATNetwork::FinalRelease()
{
    uninit ();

    BaseFinalRelease();
}

void NATNetwork::uninit()
{
    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
    delete m;
    m = NULL;
    unconst(mVirtualBox) = NULL;
}

HRESULT NATNetwork::init(VirtualBox *aVirtualBox, IN_BSTR aName)
{
    AssertReturn(aName != NULL, E_INVALIDARG);

    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    /* share VirtualBox weakly (parent remains NULL so far) */
    unconst(mVirtualBox) = aVirtualBox;
    unconst(mName) = aName;
    m = new Data();
    m->offGateway = 1;
    m->IPv4NetworkCidr = "10.0.2.0/24";
    m->IPv6Prefix = "fe80::/64";
    m->fEnabled = FALSE;

    settings::NATHostLoopbackOffset off;
    off.strLoopbackHostAddress = "127.0.0.1";
    off.u32Offset = (uint32_t)2;
    m->llNATLoopbackOffsetList.push_back(off);

    recalculateIpv4AddressAssignments();

    HRESULT hrc = unconst(m->pEventSource).createObject();
    if (FAILED(hrc)) throw hrc;

    hrc = m->pEventSource->init(static_cast<INATNetwork *>(this));
    if (FAILED(hrc)) throw hrc;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}


HRESULT NATNetwork::init(VirtualBox *aVirtualBox,
                         const settings::NATNetwork &data)
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    /* share VirtualBox weakly (parent remains NULL so far) */
    unconst(mVirtualBox) = aVirtualBox;

    unconst(mName) = data.strNetworkName;
    m = new Data();
    m->IPv4NetworkCidr = data.strNetwork;
    m->fEnabled = data.fEnabled;
    m->fAdvertiseDefaultIPv6Route = data.fAdvertiseDefaultIPv6Route;
    m->fNeedDhcpServer = data.fNeedDhcpServer;

    m->u32LoopbackIp6 = data.u32HostLoopback6Offset;

    m->llNATLoopbackOffsetList.clear();
    m->llNATLoopbackOffsetList.assign(data.llHostLoopbackOffsetList.begin(),
                               data.llHostLoopbackOffsetList.end());

    recalculateIpv4AddressAssignments();

    /* IPv4 port-forward rules */
    m->mapName2PortForwardRule4.clear();
    for (settings::NATRuleList::const_iterator it = data.llPortForwardRules4.begin();
        it != data.llPortForwardRules4.end(); ++it)
    {
        m->mapName2PortForwardRule4.insert(std::make_pair(it->strName.c_str(), *it));
    }

    /* IPv6 port-forward rules */
    m->mapName2PortForwardRule6.clear();
    for (settings::NATRuleList::const_iterator it = data.llPortForwardRules6.begin();
        it != data.llPortForwardRules6.end(); ++it)
    {
        m->mapName2PortForwardRule6.insert(std::make_pair(it->strName, *it));
    }

    HRESULT hrc = unconst(m->pEventSource).createObject();
    if (FAILED(hrc)) throw hrc;

    hrc = m->pEventSource->init(static_cast<INATNetwork *>(this));
    if (FAILED(hrc)) throw hrc;

    autoInitSpan.setSucceeded();

    return S_OK;
}

#ifdef NAT_XML_SERIALIZATION
HRESULT NATNetwork::saveSettings(settings::NATNetwork &data)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data.strNetworkName = mName;
    data.strNetwork = m->IPv4NetworkCidr;
    data.fEnabled = RT_BOOL(m->fEnabled);
    data.fAdvertiseDefaultIPv6Route = RT_BOOL(m->fAdvertiseDefaultIPv6Route);
    data.fNeedDhcpServer = RT_BOOL(m->fNeedDhcpServer);
    data.fIPv6 = RT_BOOL(m->fIPv6Enabled);
    data.strIPv6Prefix = m->IPv6Prefix;

    /* saving ipv4 port-forward Rules*/
    data.llPortForwardRules4.clear();
    for (NATRuleMap::iterator it = m->mapName2PortForwardRule4.begin();
         it != m->mapName2PortForwardRule4.end(); ++it)
        data.llPortForwardRules4.push_back(it->second);

    /* saving ipv6 port-forward Rules*/
    data.llPortForwardRules6.clear();
    for (NATRuleMap::iterator it = m->mapName2PortForwardRule6.begin();
         it != m->mapName2PortForwardRule6.end(); ++it)
        data.llPortForwardRules4.push_back(it->second);

    data.u32HostLoopback6Offset = m->u32LoopbackIp6;
    
    data.llHostLoopbackOffsetList.clear();
    data.llHostLoopbackOffsetList.assign(m->llNATLoopbackOffsetList.begin(),
                                         m->llNATLoopbackOffsetList.end());

    mVirtualBox->onNATNetworkSetting(mName.raw(),
                                     data.fEnabled ? TRUE : FALSE,
                                     m->IPv4NetworkCidr.raw(),
                                     m->IPv4Gateway.raw(),
                                     data.fAdvertiseDefaultIPv6Route ? TRUE : FALSE,
                                     data.fNeedDhcpServer ? TRUE : FALSE);

    return S_OK;
}
#endif /* NAT_XML_SERIALIZATION */

STDMETHODIMP NATNetwork::COMGETTER(EventSource)(IEventSource ** aEventSource)
{
    CheckComArgOutPointerValid(aEventSource);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    /* event source is const, no need to lock */
    m->pEventSource.queryInterfaceTo(aEventSource);

    return S_OK;
}

STDMETHODIMP NATNetwork::COMGETTER(NetworkName)(BSTR *aName)
{
    CheckComArgOutPointerValid(aName);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    mName.cloneTo(aName);

    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(NetworkName)(IN_BSTR aName)
{
    CheckComArgOutPointerValid(aName);

    HRESULT rc = S_OK;
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    unconst(mName) = aName;

    alock.release();

#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif
    return rc;
}


STDMETHODIMP NATNetwork::COMGETTER(Enabled)(BOOL *aEnabled)
{
    CheckComArgOutPointerValid(aEnabled);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    *aEnabled = m->fEnabled;
    recalculateIpv4AddressAssignments();

    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(Enabled)(BOOL aEnabled)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    HRESULT rc = S_OK;
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->fEnabled = aEnabled;

    // save the global settings; for that we should hold only the VirtualBox lock
    alock.release();
#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif
    return rc;
}

STDMETHODIMP NATNetwork::COMGETTER(Gateway)(BSTR *aIPv4Gateway)
{
    CheckComArgOutPointerValid(aIPv4Gateway);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    m->IPv4Gateway.cloneTo(aIPv4Gateway);

    return S_OK;
}

STDMETHODIMP NATNetwork::COMGETTER(Network)(BSTR *aIPv4NetworkCidr)
{
    CheckComArgOutPointerValid(aIPv4NetworkCidr);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    m->IPv4NetworkCidr.cloneTo(aIPv4NetworkCidr);
    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(Network)(IN_BSTR aIPv4NetworkCidr)
{
    CheckComArgOutPointerValid(aIPv4NetworkCidr);

    HRESULT rc = S_OK;
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    /* silently ignore network cidr update */
    if (m->mapName2PortForwardRule4.empty())
    {

        unconst(m->IPv4NetworkCidr) = Bstr(aIPv4NetworkCidr);
        recalculateIpv4AddressAssignments();
        alock.release();

#ifdef NAT_XML_SERIALIZATION
        AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
        rc = mVirtualBox->saveSettings();
#endif
    }
    return rc;
}

STDMETHODIMP NATNetwork::COMGETTER(IPv6Enabled)(BOOL *aAdvertiseDefaultIPv6Route)
{
    CheckComArgOutPointerValid(aAdvertiseDefaultIPv6Route);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    *aAdvertiseDefaultIPv6Route = m->fAdvertiseDefaultIPv6Route;

    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(IPv6Enabled)(BOOL aAdvertiseDefaultIPv6Route)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->fAdvertiseDefaultIPv6Route = aAdvertiseDefaultIPv6Route;

    // save the global settings; for that we should hold only the VirtualBox lock
    alock.release();

    HRESULT rc = S_OK;
#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif
    return rc;
}

STDMETHODIMP NATNetwork::COMGETTER(IPv6Prefix) (BSTR *aIPv6Prefix)
{
    CheckComArgOutPointerValid(aIPv6Prefix);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->IPv6Prefix.cloneTo(aIPv6Prefix);

    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(IPv6Prefix) (IN_BSTR aIPv6Prefix)
{
    CheckComArgOutPointerValid(aIPv6Prefix);

    HRESULT rc = S_OK;
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* silently ignore network cidr update */
    if (m->mapName2PortForwardRule6.empty())
    {

        unconst(m->IPv6Prefix) = Bstr(aIPv6Prefix);
        /* @todo: do we need recalculation ? */
        alock.release();

#ifdef NAT_XML_SERIALIZATION
        AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
        rc = mVirtualBox->saveSettings();
#endif
    }
    return rc;
}

STDMETHODIMP NATNetwork::COMGETTER(AdvertiseDefaultIPv6RouteEnabled)(BOOL *aAdvertiseDefaultIPv6Route)
{
    CheckComArgOutPointerValid(aAdvertiseDefaultIPv6Route);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    *aAdvertiseDefaultIPv6Route = m->fAdvertiseDefaultIPv6Route;

    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(AdvertiseDefaultIPv6RouteEnabled)(BOOL aAdvertiseDefaultIPv6Route)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->fAdvertiseDefaultIPv6Route = aAdvertiseDefaultIPv6Route;

    // save the global settings; for that we should hold only the VirtualBox lock
    alock.release();

    HRESULT rc = S_OK;
#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif
    return rc;
}

STDMETHODIMP NATNetwork::COMGETTER(NeedDhcpServer)(BOOL *aNeedDhcpServer)
{
    CheckComArgOutPointerValid(aNeedDhcpServer);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    *aNeedDhcpServer = m->fNeedDhcpServer;

    return S_OK;
}

STDMETHODIMP NATNetwork::COMSETTER(NeedDhcpServer)(BOOL aNeedDhcpServer)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->fNeedDhcpServer = aNeedDhcpServer;

    recalculateIpv4AddressAssignments();

    // save the global settings; for that we should hold only the VirtualBox lock
    alock.release();

    HRESULT rc = S_OK;
#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif
    return rc;
}


STDMETHODIMP NATNetwork::COMGETTER(LocalMappings)(ComSafeArrayOut(BSTR, aLocalMappings))
{
    CheckComArgOutSafeArrayPointerValid(aLocalMappings);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    com::SafeArray<BSTR> sf(m->llNATLoopbackOffsetList.size());

    size_t i = 0;
    settings::NATLoopbackOffsetList::const_iterator it;

    for (it = m->llNATLoopbackOffsetList.begin();
         it != m->llNATLoopbackOffsetList.end(); ++it, ++i)
      {
          BstrFmt bstr("%s;%d",
                       (*it).strLoopbackHostAddress.c_str(),
                       (*it).u32Offset);
        bstr.detachTo(&sf[i]);
    }
    sf.detachTo(ComSafeArrayOutArg(aLocalMappings));

    return S_OK;
}


STDMETHODIMP NATNetwork::AddLocalMapping(IN_BSTR aHostId, LONG aOffset)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    //AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    RTNETADDRIPV4 addr, net, mask;

    int rc = RTNetStrToIPv4Addr(Utf8Str(aHostId).c_str(), &addr);
    if (RT_FAILURE(rc)) 
        return E_INVALIDARG;
    
    /* check against 127/8 */
    if ((RT_N2H_U32(addr.u) >> IN_CLASSA_NSHIFT) != IN_LOOPBACKNET)
        return E_INVALIDARG;
    
    /* check against networkid vs network mask */
    rc = RTCidrStrToIPv4(Utf8Str(m->IPv4NetworkCidr).c_str(), &net, &mask);
    if (RT_FAILURE(rc)) 
        return E_INVALIDARG;

    if (((net.u + aOffset) & mask.u) != net.u)
        return E_INVALIDARG;

    settings::NATLoopbackOffsetList::iterator it;

    it = std::find(m->llNATLoopbackOffsetList.begin(),
                   m->llNATLoopbackOffsetList.end(), 
                   Utf8Str(aHostId).c_str());

    if (it != m->llNATLoopbackOffsetList.end())
    {
        if (aOffset == 0) /* erase */
            m->llNATLoopbackOffsetList.erase(it, it);
        else /* modify */
        {
            settings::NATLoopbackOffsetList::iterator it1;
            it1 = std::find(m->llNATLoopbackOffsetList.begin(),
                           m->llNATLoopbackOffsetList.end(), 
                           (uint32_t)aOffset);
            if (it1 != m->llNATLoopbackOffsetList.end())
                return E_INVALIDARG; /* this offset is already registered. */

            (*it).u32Offset = aOffset;
        }

        AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
        return mVirtualBox->saveSettings();
    }

    /* injection */
    it = std::find(m->llNATLoopbackOffsetList.begin(),
                   m->llNATLoopbackOffsetList.end(), 
                   (uint32_t)aOffset);

    if (it != m->llNATLoopbackOffsetList.end())
        return E_INVALIDARG; /* offset is already registered. */

    settings::NATHostLoopbackOffset off;
    off.strLoopbackHostAddress = aHostId;
    off.u32Offset = (uint32_t)aOffset;
    m->llNATLoopbackOffsetList.push_back(off);

    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    return mVirtualBox->saveSettings();
}


STDMETHODIMP NATNetwork::COMGETTER(LoopbackIp6)(LONG *aLoopbackIp6)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aLoopbackIp6 = m->u32LoopbackIp6;
    return S_OK;
}


STDMETHODIMP NATNetwork::COMSETTER(LoopbackIp6)(LONG aLoopbackIp6)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    m->u32LoopbackIp6 = aLoopbackIp6;
    
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    return mVirtualBox->saveSettings();
}

STDMETHODIMP NATNetwork::COMGETTER(PortForwardRules4)(ComSafeArrayOut(BSTR, aPortForwardRules4))
{
    CheckComArgOutSafeArrayPointerValid(aPortForwardRules4);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    GetPortForwardRulesFromMap(ComSafeArrayInArg(aPortForwardRules4),
                               m->mapName2PortForwardRule4);
    alock.release();
    return S_OK;
}

STDMETHODIMP NATNetwork::COMGETTER(PortForwardRules6)(ComSafeArrayOut(BSTR,
								      aPortForwardRules6))
{
    CheckComArgOutSafeArrayPointerValid(aPortForwardRules6);

    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    GetPortForwardRulesFromMap(ComSafeArrayInArg(aPortForwardRules6), m->mapName2PortForwardRule6);
    return S_OK;
}

STDMETHODIMP NATNetwork::AddPortForwardRule(BOOL aIsIpv6,
                                            IN_BSTR aPortForwardRuleName,
                                            NATProtocol_T aProto,
                                            IN_BSTR aHostIp,
                                            USHORT aHostPort,
                                            IN_BSTR aGuestIp,
                                            USHORT aGuestPort)
{
    int rc = S_OK;
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    Utf8Str name = aPortForwardRuleName;
    Utf8Str proto;
    settings::NATRule r;
    NATRuleMap& mapRules = aIsIpv6 ? m->mapName2PortForwardRule6 : m->mapName2PortForwardRule4;
        switch (aProto)
    {
        case NATProtocol_TCP:
            proto = "tcp";
            break;
        case NATProtocol_UDP:
            proto = "udp";
            break;
        default:
            return E_INVALIDARG;
    }
    if (name.isEmpty())
        name = Utf8StrFmt("%s_[%s]%%%d_[%s]%%%d", proto.c_str(),
                          Utf8Str(aHostIp).c_str(), aHostPort,
                          Utf8Str(aGuestIp).c_str(), aGuestPort);

    NATRuleMap::iterator it;

    for (it = mapRules.begin(); it != mapRules.end(); ++it)
    {
        r = it->second;
        if (it->first == name)
            return setError(E_INVALIDARG,
                            tr("A NAT rule of this name already exists"));
        if (   r.strHostIP == Utf8Str(aHostIp)
            && r.u16HostPort == aHostPort
            && r.proto == aProto)
            return setError(E_INVALIDARG,
                            tr("A NAT rule for this host port and this host IP already exists"));
    }

    r.strName = name.c_str();
    r.proto = aProto;
    r.strHostIP = aHostIp;
    r.u16HostPort = aHostPort;
    r.strGuestIP = aGuestIp;
    r.u16GuestPort = aGuestPort;
    mapRules.insert(std::make_pair(name, r));

    alock.release();
    mVirtualBox->onNATNetworkPortForward(mName.raw(), TRUE, aIsIpv6,
                                         aPortForwardRuleName, aProto,
                                         aHostIp, aHostPort,
                                         aGuestIp, aGuestPort);

    /* Notify listerners listening on this network only */
    fireNATNetworkPortForwardEvent(m->pEventSource, mName.raw(), TRUE,
                                   aIsIpv6, aPortForwardRuleName, aProto,
                                   aHostIp, aHostPort,
                                   aGuestIp, aGuestPort);

#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif
    return rc;
}

STDMETHODIMP NATNetwork::RemovePortForwardRule(BOOL aIsIpv6, IN_BSTR aPortForwardRuleName)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    NATRuleMap& mapRules = aIsIpv6 ? m->mapName2PortForwardRule6 : m->mapName2PortForwardRule4;
    NATRuleMap::iterator it = mapRules.find(aPortForwardRuleName);

    if (it == mapRules.end())
        return E_INVALIDARG;

    Utf8Str strHostIP = it->second.strHostIP;
    Utf8Str strGuestIP = it->second.strGuestIP;
    uint16_t u16HostPort = it->second.u16HostPort;
    uint16_t u16GuestPort = it->second.u16GuestPort;
    NATProtocol_T proto = it->second.proto;

    mapRules.erase(it);

    alock.release();

    mVirtualBox->onNATNetworkPortForward(mName.raw(), FALSE, aIsIpv6,
                                         aPortForwardRuleName, proto,
                                         Bstr(strHostIP).raw(), u16HostPort,
                                         Bstr(strGuestIP).raw(), u16GuestPort);

    /* Notify listerners listening on this network only */
    fireNATNetworkPortForwardEvent(m->pEventSource, mName.raw(), FALSE,
                                   aIsIpv6, aPortForwardRuleName, proto,
                                   Bstr(strHostIP).raw(), u16HostPort,
                                   Bstr(strGuestIP).raw(), u16GuestPort);
    HRESULT rc = S_OK;
#ifdef NAT_XML_SERIALIZATION
    AutoWriteLock vboxLock(mVirtualBox COMMA_LOCKVAL_SRC_POS);
    rc = mVirtualBox->saveSettings();
#endif

    return rc;
}


STDMETHODIMP NATNetwork::Start(IN_BSTR aTrunkType)
{
#ifdef VBOX_WITH_NAT_SERVICE
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    if (!m->fEnabled) return S_OK;

    m->NATRunner.setOption(NETCFG_NETNAME, mName, true);
    m->NATRunner.setOption(NETCFG_TRUNKTYPE, Utf8Str(aTrunkType), true);
    m->NATRunner.setOption(NETCFG_IPADDRESS, m->IPv4Gateway, true);
    m->NATRunner.setOption(NETCFG_NETMASK, m->IPv4NetworkMask, true);

    /* No portforwarding rules from command-line, all will be fetched via API */

    if (m->fNeedDhcpServer)
    {
        /*
         * Just to as idea... via API (on creation user pass the cidr of network and)
         * and we calculate it's addreses (mutable?).
         */

        /*
         * Configuration and running DHCP server:
         * 1. find server first createDHCPServer
         * 2. if return status is E_INVALARG => server already exists just find and start.
         * 3. if return status neither E_INVALRG nor S_OK => return E_FAIL
         * 4. if return status S_OK proceed to DHCP server configuration
         * 5. call setConfiguration() and pass all required parameters
         * 6. start dhcp server.
         */
        int rc = mVirtualBox->FindDHCPServerByNetworkName(mName.raw(),
                                                      m->dhcpServer.asOutParam());
        switch (rc)
        {
            case E_INVALIDARG:
                /* server haven't beeen found let create it then */
                rc = mVirtualBox->CreateDHCPServer(mName.raw(),
                                                   m->dhcpServer.asOutParam());
                if (FAILED(rc))
                  return E_FAIL;
                /* breakthrough */

            {
                LogFunc(("gateway: %s, dhcpserver:%s, dhcplowerip:%s, dhcpupperip:%s\n",
                         Utf8Str(m->IPv4Gateway.raw()).c_str(),
                         Utf8Str(m->IPv4DhcpServer.raw()).c_str(),
                         Utf8Str(m->IPv4DhcpServerLowerIp.raw()).c_str(),
                         Utf8Str(m->IPv4DhcpServerUpperIp.raw()).c_str()));

                m->dhcpServer->AddGlobalOption(DhcpOpt_Router, m->IPv4Gateway.raw());

                rc = m->dhcpServer->COMSETTER(Enabled)(true);

                BSTR dhcpip = NULL;
                BSTR netmask = NULL;
                BSTR lowerip = NULL;
                BSTR upperip = NULL;

                m->IPv4DhcpServer.cloneTo(&dhcpip);
                m->IPv4NetworkMask.cloneTo(&netmask);
                m->IPv4DhcpServerLowerIp.cloneTo(&lowerip);
                m->IPv4DhcpServerUpperIp.cloneTo(&upperip);
                rc = m->dhcpServer->SetConfiguration(dhcpip,
                                                     netmask,
                                                     lowerip,
                                                     upperip);
            }
            case S_OK:
                break;

            default:
                return E_FAIL;
        }

        rc = m->dhcpServer->Start(mName.raw(), Bstr("").raw(), aTrunkType);
        if (FAILED(rc))
        {
            m->dhcpServer.setNull();
            return E_FAIL;
        }
    }

    if (RT_SUCCESS(m->NATRunner.start()))
    {
        mVirtualBox->onNATNetworkStartStop(mName.raw(), TRUE);
        return S_OK;
    }
    /** @todo missing setError()! */
    return E_FAIL;
#else
    NOREF(aTrunkType);
    ReturnComNotImplemented();
#endif
}

STDMETHODIMP NATNetwork::Stop()
{
#ifdef VBOX_WITH_NAT_SERVICE
    if (!m->dhcpServer.isNull())
        m->dhcpServer->Stop();

    if (RT_SUCCESS(m->NATRunner.stop()))
    {
        mVirtualBox->onNATNetworkStartStop(mName.raw(), FALSE);
        return S_OK;
    }
    /** @todo missing setError()! */
    return E_FAIL;
#else
    ReturnComNotImplemented();
#endif
}

void NATNetwork::GetPortForwardRulesFromMap(ComSafeArrayOut(BSTR, aPortForwardRules), NATRuleMap& aRules)
{
    com::SafeArray<BSTR> sf(aRules.size());
    size_t i = 0;
    NATRuleMap::const_iterator it;
    for (it = aRules.begin();
         it != aRules.end(); ++it, ++i)
      {
        settings::NATRule r = it->second;
        BstrFmt bstr("%s:%s:[%s]:%d:[%s]:%d",
                     r.strName.c_str(),
                     (r.proto == NATProtocol_TCP? "tcp" : "udp"),
                     r.strHostIP.c_str(),
                     r.u16HostPort,
                     r.strGuestIP.c_str(),
                     r.u16GuestPort);
        bstr.detachTo(&sf[i]);
    }
    sf.detachTo(ComSafeArrayOutArg(aPortForwardRules));
}


int NATNetwork::findFirstAvailableOffset(uint32_t *poff)
{
    RTNETADDRIPV4 network, netmask;

    int rc = RTCidrStrToIPv4(Utf8Str(m->IPv4NetworkCidr.raw()).c_str(),
                             &network,
                             &netmask);
    AssertRCReturn(rc, rc);

    uint32_t off;
    settings::NATLoopbackOffsetList::iterator it;
    for (off = 1; off < (network.u & ~netmask.u); ++off)
    {

        if (off == m->offGateway)
            continue;

        if (off == m->offDhcp)
            continue;
        
        bool skip = false;
        for (it = m->llNATLoopbackOffsetList.begin();
             it != m->llNATLoopbackOffsetList.end();
             ++it)
        {
            if ((*it).u32Offset == off)
            {
                skip = true;
                break;
            }

        }
        
        if (!skip)
            break;
    }
    
    if (poff)
        *poff = off;
    
    return VINF_SUCCESS;
}

int NATNetwork::recalculateIpv4AddressAssignments()
{
    RTNETADDRIPV4 network, netmask;
    int rc = RTCidrStrToIPv4(Utf8Str(m->IPv4NetworkCidr.raw()).c_str(),
                             &network,
                             &netmask);
    AssertRCReturn(rc, rc);

    findFirstAvailableOffset(&m->offGateway);
    if (m->fNeedDhcpServer)
        findFirstAvailableOffset(&m->offDhcp);

    /* I don't remember the reason CIDR calculated on the host. */
    RTNETADDRIPV4 gateway = network;
    gateway.u += m->offGateway;
    gateway.u = RT_H2N_U32(gateway.u);
    char szTmpIp[16];
    RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", gateway);
    m->IPv4Gateway = szTmpIp;

    if (m->fNeedDhcpServer)
    {
        RTNETADDRIPV4 dhcpserver = network;
        dhcpserver.u += m->offDhcp;

        /* XXX: adding more services should change the math here */
        RTNETADDRIPV4 dhcplowerip;
        dhcplowerip.u = RT_H2N_U32(dhcpserver.u + 1);
        RTNETADDRIPV4 dhcpupperip;
        dhcpupperip.u = RT_H2N_U32((network.u | ~netmask.u) - 1);

        dhcpserver.u = RT_H2N_U32(dhcpserver.u);
        network.u = RT_H2N_U32(network.u);

        RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", dhcpserver);
        m->IPv4DhcpServer = szTmpIp;
        RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", dhcplowerip);
        m->IPv4DhcpServerLowerIp = szTmpIp;
        RTStrPrintf(szTmpIp, sizeof(szTmpIp), "%RTnaipv4", dhcpupperip);
        m->IPv4DhcpServerUpperIp = szTmpIp;

        LogFunc(("network:%RTnaipv4, dhcpserver:%RTnaipv4, dhcplowerip:%RTnaipv4, dhcpupperip:%RTnaipv4\n",
                 network, dhcpserver, dhcplowerip, dhcpupperip));
    }

    /* we need IPv4NetworkMask for NAT's gw service start */
    netmask.u = RT_H2N_U32(netmask.u);
    RTStrPrintf(szTmpIp, 16, "%RTnaipv4", netmask);
    m->IPv4NetworkMask = szTmpIp;

    LogFlowFunc(("getaway:%RTnaipv4, netmask:%RTnaipv4\n", gateway, netmask));
    return VINF_SUCCESS;
}


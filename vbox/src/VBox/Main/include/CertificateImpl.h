/* $Id: CertificateImpl.h 60334 2016-04-05 13:55:31Z vboxsync $ */
/** @file
 * VirtualBox COM ICertificate implementation.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_CERTIFICATEIMPL
#define ____H_CERTIFICATEIMPL

/* VBox includes */
#include <VBox/settings.h>
#include <iprt/crypto/x509.h>
#include "CertificateWrap.h"

#include <vector>

using namespace std;

class ATL_NO_VTABLE Certificate :
    public CertificateWrap
{

public:

    DECLARE_EMPTY_CTOR_DTOR(Certificate)

    HRESULT initCertificate(PCRTCRX509CERTIFICATE a_pCert, bool a_fTrusted);
    void uninit();

    HRESULT FinalConstruct();
    void FinalRelease();

private:
    // wrapped ICertificate properties
    HRESULT getVersionNumber(CertificateVersion_T *aVersionNumber);
    HRESULT getSerialNumber(com::Utf8Str &aSerialNumber);
    HRESULT getSignatureAlgorithmOID(com::Utf8Str &aSignatureAlgorithmOID);
    HRESULT getSignatureAlgorithmName(com::Utf8Str &aSignatureAlgorithmName);
    HRESULT getPublicKeyAlgorithmOID(com::Utf8Str &aPublicKeyAlgorithmOID);
    HRESULT getPublicKeyAlgorithm(com::Utf8Str &aPublicKeyAlgorithm);
    HRESULT getIssuerName(std::vector<com::Utf8Str> &aIssuerName);
    HRESULT getSubjectName(std::vector<com::Utf8Str> &aSubjectName);
    HRESULT getValidityPeriodNotBefore(com::Utf8Str &aValidityPeriodNotBefore);
    HRESULT getValidityPeriodNotAfter(com::Utf8Str &aValidityPeriodNotAfter);
    HRESULT getSubjectPublicKey(std::vector<BYTE> &aSubjectPublicKey);
    HRESULT getIssuerUniqueIdentifier(com::Utf8Str &aIssuerUniqueIdentifier);
    HRESULT getSubjectUniqueIdentifier(com::Utf8Str &aSubjectUniqueIdentifier);
    HRESULT getCertificateAuthority(BOOL *aCertificateAuthority);
    HRESULT getKeyUsage(ULONG *aKeyUsage);
    HRESULT getExtendedKeyUsage(std::vector<com::Utf8Str> &aExtendedKeyUsage);
    HRESULT getRawCertData(std::vector<BYTE> &aRawCertData);
    HRESULT getSelfSigned(BOOL *aSelfSigned);
    HRESULT getTrusted(BOOL *aTrusted);
    // wrapped ICertificate methods
    HRESULT queryInfo(LONG aWhat, com::Utf8Str &aResult);

    /** @name Methods extracting COM data from the certificate object
     * @{  */
    HRESULT i_getAlgorithmName(PCRTCRX509ALGORITHMIDENTIFIER a_pAlgId, com::Utf8Str &a_rReturn);
    HRESULT i_getX509Name(PCRTCRX509NAME a_pName, std::vector<com::Utf8Str> &a_rReturn);
    HRESULT i_getTime(PCRTASN1TIME a_pTime, com::Utf8Str &a_rReturn);
    HRESULT i_getUniqueIdentifier(PCRTCRX509UNIQUEIDENTIFIER a_pUniqueId, com::Utf8Str &a_rReturn);
    HRESULT i_getEncodedBytes(PRTASN1CORE a_pAsn1Obj, std::vector<BYTE> &a_rReturn);
    /** @} */

    //data
    struct Data;
    Data *mData;

};

#endif // !____H_CERTIFICATEIMPL


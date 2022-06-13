/** @file
  Application for RSA PSS Primitives Validation.

Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "TestBaseCryptLib.h"

//
// RSA PSS test vectors from NIST FIPS 186-3 RSA files
//

//
// Public Modulus of RSA Key
//
UINT8 RsaPssN[]={
    0xa4, 0x7d, 0x04, 0xe7, 0xca, 0xcd, 0xba, 0x4e, 0xa2, 0x6e, 0xca, 0x8a, 0x4c, 0x6e, 0x14, 0x56,
    0x3c, 0x2c, 0xe0, 0x3b, 0x62, 0x3b, 0x76, 0x8c, 0x0d, 0x49, 0x86, 0x8a, 0x57, 0x12, 0x13, 0x01,
    0xdb, 0xf7, 0x83, 0xd8, 0x2f, 0x4c, 0x05, 0x5e, 0x73, 0x96, 0x0e, 0x70, 0x55, 0x01, 0x87, 0xd0,
    0xaf, 0x62, 0xac, 0x34, 0x96, 0xf0, 0xa3, 0xd9, 0x10, 0x3c, 0x2e, 0xb7, 0x91, 0x9a, 0x72, 0x75,
    0x2f, 0xa7, 0xce, 0x8c, 0x68, 0x8d, 0x81, 0xe3, 0xae, 0xe9, 0x94, 0x68, 0x88, 0x7a, 0x15, 0x28,
    0x8a, 0xfb, 0xb7, 0xac, 0xb8, 0x45, 0xb7, 0xc5, 0x22, 0xb5, 0xc6, 0x4e, 0x67, 0x8f, 0xcd, 0x3d,
    0x22, 0xfe, 0xb8, 0x4b, 0x44, 0x27, 0x27, 0x00, 0xbe, 0x52, 0x7d, 0x2b, 0x20, 0x25, 0xa3, 0xf8,
    0x3c, 0x23, 0x83, 0xbf, 0x6a, 0x39, 0xcf, 0x5b, 0x4e, 0x48, 0xb3, 0xcf, 0x2f, 0x56, 0xee, 0xf0,
    0xdf, 0xff, 0x18, 0x55, 0x5e, 0x31, 0x03, 0x7b, 0x91, 0x52, 0x48, 0x69, 0x48, 0x76, 0xf3, 0x04,
    0x78, 0x14, 0x41, 0x51, 0x64, 0xf2, 0xc6, 0x60, 0x88, 0x1e, 0x69, 0x4b, 0x58, 0xc2, 0x80, 0x38,
    0xa0, 0x32, 0xad, 0x25, 0x63, 0x4a, 0xad, 0x7b, 0x39, 0x17, 0x1d, 0xee, 0x36, 0x8e, 0x3d, 0x59,
    0xbf, 0xb7, 0x29, 0x9e, 0x46, 0x01, 0xd4, 0x58, 0x7e, 0x68, 0xca, 0xaf, 0x8d, 0xb4, 0x57, 0xb7,
    0x5a, 0xf4, 0x2f, 0xc0, 0xcf, 0x1a, 0xe7, 0xca, 0xce, 0xd2, 0x86, 0xd7, 0x7f, 0xac, 0x6c, 0xed,
    0xb0, 0x3a, 0xd9, 0x4f, 0x14, 0x33, 0xd2, 0xc9, 0x4d, 0x08, 0xe6, 0x0b, 0xc1, 0xfd, 0xef, 0x05,
    0x43, 0xcd, 0x29, 0x51, 0xe7, 0x65, 0xb3, 0x82, 0x30, 0xfd, 0xd1, 0x8d, 0xe5, 0xd2, 0xca, 0x62,
    0x7d, 0xdc, 0x03, 0x2f, 0xe0, 0x5b, 0xbd, 0x2f, 0xf2, 0x1e, 0x2d, 0xb1, 0xc2, 0xf9, 0x4d, 0x8b,
    };

//
// Public Exponent of RSA Key
//
UINT8 RsaPssE[]={ 0x10, 0xe4, 0x3f };

//
// Private Exponent of RSA Key
//
UINT8 RsaPssD[]={
    0x11, 0xa0, 0xdd, 0x28, 0x5f, 0x66, 0x47, 0x1a, 0x8d, 0xa3, 0x0b, 0xcb, 0x8c, 0x24, 0xa1, 0xd5,
    0xc8, 0xdb, 0x94, 0x2f, 0xc9, 0x92, 0x07, 0x97, 0xca, 0x44, 0x24, 0x60, 0xa8, 0x00, 0xb7, 0x5b,
    0xbc, 0x73, 0x8b, 0xeb, 0x8e, 0xe0, 0xe8, 0x74, 0xb0, 0x53, 0xe6, 0x47, 0x07, 0xdf, 0x4c, 0xfc,
    0x78, 0x37, 0xc4, 0x0e, 0x5b, 0xe6, 0x8b, 0x8a, 0x8e, 0x1d, 0x01, 0x45, 0x16, 0x9c, 0xa6, 0x27,
    0x1d, 0x81, 0x88, 0x7e, 0x19, 0xa1, 0xcd, 0x95, 0xb2, 0xfd, 0x0d, 0xe0, 0xdb, 0xa3, 0x47, 0xfe,
    0x63, 0x7b, 0xcc, 0x6c, 0xdc, 0x24, 0xee, 0xbe, 0x03, 0xc2, 0x4d, 0x4c, 0xf3, 0xa5, 0xc6, 0x15,
    0x4d, 0x78, 0xf1, 0x41, 0xfe, 0x34, 0x16, 0x99, 0x24, 0xd0, 0xf8, 0x95, 0x33, 0x65, 0x8e, 0xac,
    0xfd, 0xea, 0xe9, 0x9c, 0xe1, 0xa8, 0x80, 0x27, 0xc1, 0x8f, 0xf9, 0x26, 0x53, 0xa8, 0x35, 0xaa,
    0x38, 0x91, 0xbf, 0xff, 0xcd, 0x38, 0x8f, 0xfc, 0x23, 0x88, 0xce, 0x2b, 0x10, 0x56, 0x85, 0x43,
    0x75, 0x05, 0x02, 0xcc, 0xbc, 0x69, 0xc0, 0x08, 0x8f, 0x1d, 0x69, 0x0e, 0x97, 0xa5, 0xf5, 0xbd,
    0xd1, 0x88, 0x8c, 0xd2, 0xfa, 0xa4, 0x3c, 0x04, 0xae, 0x24, 0x53, 0x95, 0x22, 0xdd, 0xe2, 0xd9,
    0xc2, 0x02, 0xf6, 0x55, 0xfc, 0x55, 0x75, 0x44, 0x40, 0xb5, 0x3a, 0x15, 0x32, 0xaa, 0xb4, 0x78,
    0x51, 0xf6, 0x0b, 0x7a, 0x06, 0x7e, 0x24, 0x0b, 0x73, 0x8e, 0x1b, 0x1d, 0xaa, 0xe6, 0xca, 0x0d,
    0x59, 0xee, 0xae, 0x27, 0x68, 0x6c, 0xd8, 0x88, 0x57, 0xe9, 0xad, 0xad, 0xc2, 0xd4, 0xb8, 0x2b,
    0x07, 0xa6, 0x1a, 0x35, 0x84, 0x56, 0xaa, 0xf8, 0x07, 0x66, 0x96, 0x93, 0xff, 0xb1, 0x3c, 0x99,
    0x64, 0xa6, 0x36, 0x54, 0xca, 0xdc, 0x81, 0xee, 0x59, 0xdf, 0x51, 0x1c, 0xa3, 0xa4, 0xbd, 0x67,
    };

//
// Binary message to be signed and verified
//
UINT8 PssMessage[]={
    0xe0, 0x02, 0x37, 0x7a, 0xff, 0xb0, 0x4f, 0x0f, 0xe4, 0x59, 0x8d, 0xe9, 0xd9, 0x2d, 0x31, 0xd6,
    0xc7, 0x86, 0x04, 0x0d, 0x57, 0x76, 0x97, 0x65, 0x56, 0xa2, 0xcf, 0xc5, 0x5e, 0x54, 0xa1, 0xdc,
    0xb3, 0xcb, 0x1b, 0x12, 0x6b, 0xd6, 0xa4, 0xbe, 0xd2, 0xa1, 0x84, 0x99, 0x0c, 0xce, 0xa7, 0x73,
    0xfc, 0xc7, 0x9d, 0x24, 0x65, 0x53, 0xe6, 0xc6, 0x4f, 0x68, 0x6d, 0x21, 0xad, 0x41, 0x52, 0x67,
    0x3c, 0xaf, 0xec, 0x22, 0xae, 0xb4, 0x0f, 0x6a, 0x08, 0x4e, 0x8a, 0x5b, 0x49, 0x91, 0xf4, 0xc6,
    0x4c, 0xf8, 0xa9, 0x27, 0xef, 0xfd, 0x0f, 0xd7, 0x75, 0xe7, 0x1e, 0x83, 0x29, 0xe4, 0x1f, 0xdd,
    0x44, 0x57, 0xb3, 0x91, 0x11, 0x73, 0x18, 0x7b, 0x4f, 0x09, 0xa8, 0x17, 0xd7, 0x9e, 0xa2, 0x39,
    0x7f, 0xc1, 0x2d, 0xfe, 0x3d, 0x9c, 0x9a, 0x02, 0x90, 0xc8, 0xea, 0xd3, 0x1b, 0x66, 0x90, 0xa6,
    };

//
// Binary message to be signed and verified
//
UINT8 PssSalt[]={
    0xd6, 0x6f, 0x72, 0xf1, 0x0b, 0x69, 0x00, 0x1a, 0x5b, 0x59, 0xcf, 0x10, 0x92, 0xad, 0x27, 0x4d,
    0x50, 0x56, 0xc4, 0xe9, 0x5c, 0xcc, 0xcf, 0xbe, 0x3b, 0x53, 0x0d, 0xcb, 0x02, 0x7e, 0x57, 0xd6
    };

//
// RSASSA-PSS Signature over above message using above keys, salt and SHA256 digest(and MGF1) algo.
//
UINT8 TestVectorSignature[]={
    0x4f, 0x9b, 0x42, 0x5c, 0x20, 0x58, 0x46, 0x0e, 0x4a, 0xb2, 0xf5, 0xc9, 0x63, 0x84, 0xda, 0x23,
    0x27, 0xfd, 0x29, 0x15, 0x0f, 0x01, 0x95, 0x5a, 0x76, 0xb4, 0xef, 0xe9, 0x56, 0xaf, 0x06, 0xdc,
    0x08, 0x77, 0x9a, 0x37, 0x4e, 0xe4, 0x60, 0x7e, 0xab, 0x61, 0xa9, 0x3a, 0xdc, 0x56, 0x08, 0xf4,
    0xec, 0x36, 0xe4, 0x7f, 0x2a, 0x0f, 0x75, 0x4e, 0x8f, 0xf8, 0x39, 0xa8, 0xa1, 0x9b, 0x1d, 0xb1,
    0xe8, 0x84, 0xea, 0x4c, 0xf3, 0x48, 0xcd, 0x45, 0x50, 0x69, 0xeb, 0x87, 0xaf, 0xd5, 0x36, 0x45,
    0xb4, 0x4e, 0x28, 0xa0, 0xa5, 0x68, 0x08, 0xf5, 0x03, 0x1d, 0xa5, 0xba, 0x91, 0x12, 0x76, 0x8d,
    0xfb, 0xfc, 0xa4, 0x4e, 0xbe, 0x63, 0xa0, 0xc0, 0x57, 0x2b, 0x73, 0x1d, 0x66, 0x12, 0x2f, 0xb7,
    0x16, 0x09, 0xbe, 0x14, 0x80, 0xfa, 0xa4, 0xe4, 0xf7, 0x5e, 0x43, 0x95, 0x51, 0x59, 0xd7, 0x0f,
    0x08, 0x1e, 0x2a, 0x32, 0xfb, 0xb1, 0x9a, 0x48, 0xb9, 0xf1, 0x62, 0xcf, 0x6b, 0x2f, 0xb4, 0x45,
    0xd2, 0xd6, 0x99, 0x4b, 0xc5, 0x89, 0x10, 0xa2, 0x6b, 0x59, 0x43, 0x47, 0x78, 0x03, 0xcd, 0xaa,
    0xa1, 0xbd, 0x74, 0xb0, 0xda, 0x0a, 0x5d, 0x05, 0x3d, 0x8b, 0x1d, 0xc5, 0x93, 0x09, 0x1d, 0xb5,
    0x38, 0x83, 0x83, 0xc2, 0x60, 0x79, 0xf3, 0x44, 0xe2, 0xae, 0xa6, 0x00, 0xd0, 0xe3, 0x24, 0x16,
    0x4b, 0x45, 0x0f, 0x7b, 0x9b, 0x46, 0x51, 0x11, 0xb7, 0x26, 0x5f, 0x3b, 0x1b, 0x06, 0x30, 0x89,
    0xae, 0x7e, 0x26, 0x23, 0xfc, 0x0f, 0xda, 0x80, 0x52, 0xcf, 0x4b, 0xf3, 0x37, 0x91, 0x02, 0xfb,
    0xf7, 0x1d, 0x7c, 0x98, 0xe8, 0x25, 0x86, 0x64, 0xce, 0xed, 0x63, 0x7d, 0x20, 0xf9, 0x5f, 0xf0,
    0x11, 0x18, 0x81, 0xe6, 0x50, 0xce, 0x61, 0xf2, 0x51, 0xd9, 0xc3, 0xa6, 0x29, 0xef, 0x22, 0x2d,
    };


STATIC VOID     *mRsa;

UNIT_TEST_STATUS
EFIAPI
TestVerifyRsaPssPreReq (
  UNIT_TEST_CONTEXT           Context
  )
{
  mRsa = RsaNew ();

  if (mRsa == NULL) {
    return UNIT_TEST_ERROR_TEST_FAILED;
  }

  return UNIT_TEST_PASSED;
}

VOID
EFIAPI
TestVerifyRsaPssCleanUp (
  UNIT_TEST_CONTEXT           Context
  )
{
  if (mRsa != NULL) {
    RsaFree (mRsa);
    mRsa = NULL;
  }
}


UNIT_TEST_STATUS
EFIAPI
TestVerifyRsaPssSignVerify (
  IN UNIT_TEST_CONTEXT           Context
  )
{
  UINT8    *Signature;
  UINTN    SigSize;
  BOOLEAN  Status;

  Status = RsaSetKey (mRsa, RsaKeyN, RsaPssN, sizeof (RsaPssN));
  UT_ASSERT_TRUE (Status);

  Status = RsaSetKey (mRsa, RsaKeyE, RsaPssE, sizeof (RsaPssE));
  UT_ASSERT_TRUE (Status);

  Status = RsaSetKey (mRsa, RsaKeyD, RsaPssD, sizeof (RsaPssD));
  UT_ASSERT_TRUE (Status);

  SigSize = 0;
  Status = RsaPssSign (mRsa, PssMessage, sizeof(PssMessage), SHA256_DIGEST_SIZE, SHA256_DIGEST_SIZE, NULL, &SigSize);
  UT_ASSERT_FALSE (Status);
  UT_ASSERT_NOT_EQUAL (SigSize, 0);

  Signature = AllocatePool (SigSize);
  Status = RsaPssSign (mRsa, PssMessage, sizeof(PssMessage), SHA256_DIGEST_SIZE, SHA256_DIGEST_SIZE, Signature, &SigSize);
  UT_ASSERT_TRUE (Status);

  //
  // Verify RSA PSS encoded Signature generated in above step
  //
  Status = RsaPssVerify (mRsa, PssMessage, sizeof(PssMessage), Signature, SigSize, SHA256_DIGEST_SIZE, SHA256_DIGEST_SIZE);
  UT_ASSERT_TRUE (Status);

  //
  // Verify NIST FIPS 186-3 RSA test vector signature
  //
  Status = RsaPssVerify (mRsa, PssMessage, sizeof(PssMessage), TestVectorSignature, sizeof(TestVectorSignature), SHA256_DIGEST_SIZE, SHA256_DIGEST_SIZE);
  UT_ASSERT_TRUE (Status);

  FreePool(Signature);
  return UNIT_TEST_PASSED;
}


TEST_DESC mRsaPssTest[] = {
    //
    // -----Description--------------------------------------Class----------------------Function---------------------------------Pre---------------------Post---------Context
    //
    {"TestVerifyRsaPssSignVerify()",           "CryptoPkg.BaseCryptLib.Rsa",   TestVerifyRsaPssSignVerify,           TestVerifyRsaPssPreReq, TestVerifyRsaPssCleanUp, NULL},
};

UINTN mRsaPssTestNum = ARRAY_SIZE(mRsaPssTest);

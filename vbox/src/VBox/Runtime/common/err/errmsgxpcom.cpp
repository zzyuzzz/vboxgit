/* $Id: errmsgxpcom.cpp 83743 2020-04-17 11:25:08Z vboxsync $ */
/** @file
 * IPRT - Status code messages for XPCOM.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <iprt/errcore.h>
#include "internal/iprt.h"

#include <iprt/asm.h>
#include <iprt/string.h>
#include <iprt/errcore.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
typedef uint32_t VBOXSTATUSTYPE; /* used by errmsgvboxcomdata.h */


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/** Array of messages.
 * The data is generated by a sed script.
 */
static const RTCOMERRMSG  g_aStatusMsgs[] =
{
#define MY_ERR(_def, _desc, _val) { _desc, _def, _val }

    MY_ERR("NS_OK",                                 "Success",                                      UINT32_C(0x00000000)),
    MY_ERR("NS_ERROR_NOT_IMPLEMENTED",              "Not implemented",                              UINT32_C(0x80004001)),
    MY_ERR("NS_ERROR_NO_INTERFACE",                 "Interface not supported",                      UINT32_C(0x80004002)),
    MY_ERR("NS_ERROR_INVALID_POINTER",              "Invalid pointer value",                        UINT32_C(0x80004003)),
    MY_ERR("NS_ERROR_ABORT",                        "Operation aborted",                            UINT32_C(0x80004004)),
    MY_ERR("NS_ERROR_FAILURE",                      "Operation failed",                             UINT32_C(0x80004005)),
    MY_ERR("NS_ERROR_CALL_FAILED",                  "Call to remote object failed",                 UINT32_C(0x800706be)),
    MY_ERR("NS_ERROR_UNEXPECTED",                   "Unexpected error",                             UINT32_C(0x8000ffff)),

    MY_ERR("NS_ERROR_CANNOT_CONVERT_DATA",          "Cannot convert data",                          UINT32_C(0x80010001)),
    MY_ERR("NS_ERROR_OBJECT_IS_IMMUTABLE",          "Object is immutable",                          UINT32_C(0x80010002)),
    MY_ERR("NS_ERROR_LOSS_OF_SIGNIFICANT_DATA",     "Loss of significant data",                     UINT32_C(0x80010003)),
    MY_ERR("NS_ERROR_PROXY_INVALID_IN_PARAMETER",   "Cannot proxy an IN parameter",                 UINT32_C(0x80010010)),
    MY_ERR("NS_ERROR_PROXY_INVALID_OUT_PARAMETER",  "Cannot proxy an OUT parameter",                UINT32_C(0x80010011)),
    MY_ERR("NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA", "Loss of insignificant data",                   UINT32_C(0x00010001)),

    MY_ERR("E_ACCESSDENIED",                        "Access denied",                                UINT32_C(0x80070005)), /* VirtualBox addition */
    MY_ERR("NS_ERROR_OUT_OF_MEMORY",                "Memory allocation failed",                     UINT32_C(0x8007000e)),
    MY_ERR("NS_ERROR_INVALID_ARG",                  "Invalid argument value",                       UINT32_C(0x80070057)),

    MY_ERR("NS_ERROR_NO_AGGREGATION",               "Class does not allow aggregation",             UINT32_C(0x80040110)),
    MY_ERR("NS_ERROR_NOT_AVAILABLE",                "Resource not available",                       UINT32_C(0x80040111)),
    MY_ERR("NS_ERROR_FACTORY_NOT_REGISTERED",       "Class not registered",                         UINT32_C(0x80040154)),
    MY_ERR("NS_ERROR_FACTORY_REGISTER_AGAIN",       "Cannot be registered, try again later",        UINT32_C(0x80040155)),
    MY_ERR("NS_ERROR_FACTORY_NOT_LOADED",           "Dynamically loaded factory cannot be found",   UINT32_C(0x800401f8)),
    MY_ERR("NS_ERROR_FACTORY_EXISTS",               "Factory already exists",                       UINT32_C(0xc1f30100)),
    MY_ERR("NS_ERROR_FACTORY_NO_SIGNATURE_SUPPORT", "Factory does not support signatures",          UINT32_C(0xc1f30101)),
    MY_ERR("NS_ERROR_NOT_INITIALIZED",              "Instance not initialized",                     UINT32_C(0xc1f30001)),
    MY_ERR("NS_ERROR_ALREADY_INITIALIZED",          "Instance already initialized",                 UINT32_C(0xc1f30002)),

    MY_ERR("NS_BASE_STREAM_CLOSED",                 "Stream closed",                                UINT32_C(0x80470002)),
    MY_ERR("NS_BASE_STREAM_OSERROR",                "Operative system stream error",                UINT32_C(0x80470003)),
    MY_ERR("NS_BASE_STREAM_ILLEGAL_ARGS",           "Illegal argument to stream method",            UINT32_C(0x80470004)),
    MY_ERR("NS_BASE_STREAM_NO_CONVERTER",           "No stream converter",                          UINT32_C(0x80470005)),
    MY_ERR("NS_BASE_STREAM_BAD_CONVERSION",         "Badstream conversion",                         UINT32_C(0x80470006)),
    MY_ERR("NS_BASE_STREAM_WOULD_BLOCK",            "Stream operation would block",                 UINT32_C(0x80470007)),

    MY_ERR("NS_ERROR_FILE_UNRECOGNIZED_PATH",       "Unrecognized path",                            UINT32_C(0x80520001)),
    MY_ERR("NS_ERROR_FILE_UNRESOLVABLE_SYMLINK",    "NS_ERROR_FILE_UNRESOLVABLE_SYMLINK",           UINT32_C(0x80520002)),
    MY_ERR("NS_ERROR_FILE_EXECUTION_FAILED",        "NS_ERROR_FILE_EXECUTION_FAILED",               UINT32_C(0x80520003)),
    MY_ERR("NS_ERROR_FILE_UNKNOWN_TYPE",            "NS_ERROR_FILE_UNKNOWN_TYPE",                   UINT32_C(0x80520004)),
    MY_ERR("NS_ERROR_FILE_DESTINATION_NOT_DIR",     "NS_ERROR_FILE_DESTINATION_NOT_DIR",            UINT32_C(0x80520005)),
    MY_ERR("NS_ERROR_FILE_TARGET_DOES_NOT_EXIST",   "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST",          UINT32_C(0x80520006)),
    MY_ERR("NS_ERROR_FILE_COPY_OR_MOVE_FAILED",     "NS_ERROR_FILE_COPY_OR_MOVE_FAILED",            UINT32_C(0x80520007)),
    MY_ERR("NS_ERROR_FILE_ALREADY_EXISTS",          "NS_ERROR_FILE_ALREADY_EXISTS",                 UINT32_C(0x80520008)),
    MY_ERR("NS_ERROR_FILE_INVALID_PATH",            "NS_ERROR_FILE_INVALID_PATH",                   UINT32_C(0x80520009)),
    MY_ERR("NS_ERROR_FILE_DISK_FULL",               "NS_ERROR_FILE_DISK_FULL",                      UINT32_C(0x8052000a)),
    MY_ERR("NS_ERROR_FILE_CORRUPTED",               "NS_ERROR_FILE_CORRUPTED",                      UINT32_C(0x8052000b)),
    MY_ERR("NS_ERROR_FILE_NOT_DIRECTORY",           "NS_ERROR_FILE_NOT_DIRECTORY",                  UINT32_C(0x8052000c)),
    MY_ERR("NS_ERROR_FILE_IS_DIRECTORY",            "NS_ERROR_FILE_IS_DIRECTORY",                   UINT32_C(0x8052000d)),
    MY_ERR("NS_ERROR_FILE_IS_LOCKED",               "NS_ERROR_FILE_IS_LOCKED",                      UINT32_C(0x8052000e)),
    MY_ERR("NS_ERROR_FILE_TOO_BIG",                 "NS_ERROR_FILE_TOO_BIG",                        UINT32_C(0x8052000f)),
    MY_ERR("NS_ERROR_FILE_NO_DEVICE_SPACE",         "NS_ERROR_FILE_NO_DEVICE_SPACE",                UINT32_C(0x80520010)),
    MY_ERR("NS_ERROR_FILE_NAME_TOO_LONG",           "NS_ERROR_FILE_NAME_TOO_LONG",                  UINT32_C(0x80520011)),
    MY_ERR("NS_ERROR_FILE_NOT_FOUND",               "NS_ERROR_FILE_NOT_FOUND",                      UINT32_C(0x80520012)),
    MY_ERR("NS_ERROR_FILE_READ_ONLY",               "NS_ERROR_FILE_READ_ONLY",                      UINT32_C(0x80520013)),
    MY_ERR("NS_ERROR_FILE_DIR_NOT_EMPTY",           "NS_ERROR_FILE_DIR_NOT_EMPTY",                  UINT32_C(0x80520014)),
    MY_ERR("NS_ERROR_FILE_ACCESS_DENIED",           "NS_ERROR_FILE_ACCESS_DENIED",                  UINT32_C(0x80520015)),
    MY_ERR("NS_SUCCESS_FILE_DIRECTORY_EMPTY",       "NS_SUCCESS_FILE_DIRECTORY_EMPTY",              UINT32_C(0x00520001)),
    MY_ERR("NS_ERROR_SOCKET_FAIL",                  "IPC daemon socket error",                      UINT32_C(0xc1f30200)), /* new XPCOM error code */

#if defined(VBOX) && !defined(IN_GUEST) && !defined(DOXYGEN_RUNNING)
# include "errmsgvboxcomdata.h"
#endif

    { NULL, NULL, 0 }
#undef MY_ERR
};


/** Temporary buffers to format unknown messages in.
 * @{
 */
static char                 g_aszUnknownStr[8][64];
static RTCOMERRMSG          g_aUnknownMsgs[8] =
{
    { &g_aszUnknownStr[0][0], &g_aszUnknownStr[0][0], 0 },
    { &g_aszUnknownStr[1][0], &g_aszUnknownStr[1][0], 0 },
    { &g_aszUnknownStr[2][0], &g_aszUnknownStr[2][0], 0 },
    { &g_aszUnknownStr[3][0], &g_aszUnknownStr[3][0], 0 },
    { &g_aszUnknownStr[4][0], &g_aszUnknownStr[4][0], 0 },
    { &g_aszUnknownStr[5][0], &g_aszUnknownStr[5][0], 0 },
    { &g_aszUnknownStr[6][0], &g_aszUnknownStr[6][0], 0 },
    { &g_aszUnknownStr[7][0], &g_aszUnknownStr[7][0], 0 }
};
/** Last used index in g_aUnknownMsgs. */
static volatile uint32_t    g_iUnknownMsgs;
/** @} */


RTDECL(PCRTCOMERRMSG) RTErrCOMGet(uint32_t rc)
{
    unsigned i;
    for (i = 0; i < RT_ELEMENTS(g_aStatusMsgs) - 1; i++)
        if (g_aStatusMsgs[i].iCode == rc)
            return &g_aStatusMsgs[i];

    /*
     * Need to use the temporary stuff.
     */
    int32_t iMsg = (ASMAtomicIncU32(&g_iUnknownMsgs) - 1) % RT_ELEMENTS(g_aUnknownMsgs);
    RTStrPrintf(&g_aszUnknownStr[iMsg][0], sizeof(g_aszUnknownStr[iMsg]), "Unknown Status 0x%X", rc);
    return &g_aUnknownMsgs[iMsg];
}
RT_EXPORT_SYMBOL(RTErrCOMGet);


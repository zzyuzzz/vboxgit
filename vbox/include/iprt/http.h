/* $Id: http.h 57577 2015-08-28 18:57:36Z vboxsync $ */
/** @file
 * IPRT - Simple HTTP Communication API.
 */

/*
 * Copyright (C) 2012-2015 Oracle Corporation
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

#ifndef ___iprt_http_h
#define ___iprt_http_h

#include <iprt/types.h>

RT_C_DECLS_BEGIN

/** @defgroup grp_rt_http   RTHttp - Simple HTTP API
 * @ingroup grp_rt
 * @{
 */

/** @todo the following three definitions may move the iprt/types.h later. */
/** RTHTTP interface handle. */
typedef R3PTRTYPE(struct RTHTTPINTERNAL *)      RTHTTP;
/** Pointer to a RTHTTP interface handle. */
typedef RTHTTP                                  *PRTHTTP;
/** Nil RTHTTP interface handle. */
#define NIL_RTHTTP                              ((RTHTTP)0)


/**
 * Creates a HTTP interface handle.
 *
 * @returns iprt status code.
 *
 * @param   phHttp      Where to store the HTTP handle.
 */
RTR3DECL(int) RTHttpCreate(PRTHTTP phHttp);

/**
 * Destroys a HTTP interface handle.
 *
 * @param   hHttp       Handle to the HTTP interface.
 */
RTR3DECL(void) RTHttpDestroy(RTHTTP hHttp);


/**
 * Retrieve the redir location for 301 responses.
 *
 * @param   hHttp               Handle to the HTTP interface.
 * @param   ppszRedirLocation   Where to store the string.  To be freed with
 *                              RTStrFree().
 */
RTR3DECL(int) RTHttpGetRedirLocation(RTHTTP hHttp, char **ppszRedirLocation);

/**
 * Perform a simple blocking HTTP request.
 *
 * This is a just a convenient wrapper around RTHttpGetBinary that returns a
 * different type and sheds a parameter.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 * @param   pcszUrl         URL.
 * @param   ppszNotUtf8     Where to return the poitner to the HTTP response.
 *                          The string is of course zero terminated.  Use
 *                          RTHttpFreeReponseText to free.
 *
 * @remarks BIG FAT WARNING!
 *
 *          This function does not guarantee the that returned string is valid UTF-8 or
 *          any other kind of text encoding!
 *
 *          The caller must determine and validate the string encoding _before_
 *          passing it along to functions that expect UTF-8!
 *
 *          Also, this function does not guarantee that the returned string
 *          doesn't have embedded zeros and provides the caller no way of
 *          finding out!  If you are worried about the response from the HTTPD
 *          containing embedded zero's, use RTHttpGetBinary instead.
 */
RTR3DECL(int) RTHttpGetText(RTHTTP hHttp, const char *pcszUrl, char **ppszNotUtf8);

/**
 * Frees memory returned by RTHttpGetText.
 *
 * @param   pszNotUtf8      What RTHttpGetText returned.
 */
RTR3DECL(void) RTHttpFreeResponseText(char *pszNotUtf8);

/**
 * Perform a simple blocking HTTP request.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 * @param   pcszUrl         The URL.
 * @param   ppvResponse     Where to store the HTTP response data.  Use
 *                          RTHttpFreeResponse to free.
 * @param   pcb             Size of the returned buffer.
 */
RTR3DECL(int) RTHttpGetBinary(RTHTTP hHttp, const char *pcszUrl, void **ppvResponse, size_t *pcb);

/**
 * Frees memory returned by RTHttpGetBinary.
 *
 * @param   pvResponse      What RTHttpGetBinary returned.
 */
RTR3DECL(void) RTHttpFreeResponse(void *pvResponse);

/**
 * Perform a simple blocking HTTP request, writing the output to a file.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 * @param   pszUrl          The URL.
 * @param   pszDstFile      The destination file name.
 */
RTR3DECL(int) RTHttpGetFile(RTHTTP hHttp, const char *pszUrl, const char *pszDstFile);

/**
 * Abort a pending HTTP request. A blocking RTHttpGet() call will return with
 * VERR_HTTP_ABORTED. It may take some time (current cURL implementation needs
 * up to 1 second) before the request is aborted.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 */
RTR3DECL(int) RTHttpAbort(RTHTTP hHttp);

/**
 * Tells the HTTP interface to use the system proxy configuration.
 *
 * @returns iprt status code.
 * @param   hHttp           HTTP interface handle.
 */
RTR3DECL(int) RTHttpUseSystemProxySettings(RTHTTP hHttp);

/**
 * Specify proxy settings.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 * @param   pcszProxy       URL of the proxy.
 * @param   uPort           port number of the proxy, use 0 for not specifying a port.
 * @param   pcszUser        Username, pass NULL for no authentication.
 * @param   pcszPwd         Password, pass NULL for no authentication.
 */
RTR3DECL(int) RTHttpSetProxy(RTHTTP hHttp, const char *pcszProxyUrl, uint32_t uPort,
                             const char *pcszProxyUser, const char *pcszProxyPwd);

/**
 * Set custom headers.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 * @param   cHeaders        Number of custom headers.
 * @param   pcszHeaders     Array of headers in form "foo: bar".
 */
RTR3DECL(int) RTHttpSetHeaders(RTHTTP hHttp, size_t cHeaders, const char * const *papszHeaders);

/**
 * Set a custom certification authority file, containing root certificates.
 *
 * @returns iprt status code.
 *
 * @param   hHttp           HTTP interface handle.
 * @param   pcszCAFile      File name containing root certificates.
 */
RTR3DECL(int) RTHttpSetCAFile(RTHTTP hHttp, const char *pcszCAFile);


/** @} */

RT_C_DECLS_END

#endif


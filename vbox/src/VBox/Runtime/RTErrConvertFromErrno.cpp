/* $Id: RTErrConvertFromErrno.cpp 1532 2007-03-16 14:54:40Z vboxsync $ */
/** @file
 * InnoTek Portable Runtime - Convert errno to iprt status codes.
 */

/*
 * Copyright (C) 2006 InnoTek Systemberatung GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <iprt/err.h>
#include <iprt/assert.h>
#include <iprt/err.h>

#if defined(__DARWIN__) && defined(KERNEL)
# include <sys/errno.h>
#else
# include <errno.h>
#endif


RTDECL(int)  RTErrConvertFromErrno(unsigned uNativeCode)
{
    /* very fast check for no error. */
    if (uNativeCode == 0)
        return VINF_SUCCESS;

    /*
     * Process error codes.
     *
     * (Use a switch and not a table since the numbers vary among compilers
     * and OSes. So we let the compiler switch optimizer handle speed issues.)
     *
     * This switch is arranged like the Linux i386 errno.h!
     */
    switch (uNativeCode)
    {                                                                           /* Linux number */
#ifdef EPERM
        case EPERM:             return VERR_ACCESS_DENIED;                      /*   1 */
#endif
#ifdef ENOENT
        case ENOENT:            return VERR_FILE_NOT_FOUND;
#endif
#ifdef ESRCH
        case ESRCH:             return VERR_PROCESS_NOT_FOUND;
#endif
#ifdef EINTR
        case EINTR:             return VERR_INTERRUPTED;
#endif
#ifdef EIO
        case EIO:               return VERR_DEV_IO_ERROR;
#endif
#ifdef ENXIO
        case ENXIO:             return VERR_DEV_IO_ERROR;
#endif
#ifdef E2BIG
        case E2BIG:             return VERR_TOO_MUCH_DATA;
#endif
#ifdef ENOEXEC
        case ENOEXEC:           return VERR_BAD_EXE_FORMAT;
#endif
#ifdef EBADF
        case EBADF:             return VERR_INVALID_HANDLE;
#endif
#ifdef ECHILD
        case ECHILD:            return VERR_PROCESS_NOT_FOUND; //...            /*  10 */
#endif
#ifdef EAGAIN
        case EAGAIN:            return VERR_TRY_AGAIN;
#endif
#ifdef ENOMEM
        case ENOMEM:            return VERR_NO_MEMORY;
#endif
#ifdef EACCES
        case EACCES:            return VERR_ACCESS_DENIED;
#endif
#ifdef EFAULT
        case EFAULT:            return VERR_INVALID_POINTER;
#endif
#ifdef ENOTBLK
        //case ENOTBLK:           return VERR_;
#endif
#ifdef EBUSY
        case EBUSY:             return VERR_DEV_IO_ERROR;
#endif
#ifdef EEXIST
        case EEXIST:            return VERR_ALREADY_EXISTS;
#endif
#ifdef EXDEV
        case EXDEV:             return VERR_NOT_SAME_DEVICE;
#endif
#ifdef ENODEV
        case ENODEV:            return VERR_NOT_SUPPORTED;
#endif
#ifdef ENOTDIR
        case ENOTDIR:           return VERR_PATH_NOT_FOUND;                     /*  20 */
#endif
#ifdef EISDIR
        case EISDIR:            return VERR_IS_A_DIRECTORY;
#endif
#ifdef EINVAL
        case EINVAL:            return VERR_INVALID_PARAMETER;
#endif
#ifdef ENFILE
        case ENFILE:            return VERR_TOO_MANY_OPEN_FILES;
#endif
#ifdef EMFILE
        case EMFILE:            return VERR_TOO_MANY_OPEN_FILES;
#endif
#ifdef ENOTTY
        case ENOTTY:            return VERR_INVALID_FUNCTION;
#endif
#ifdef ETXTBSY
        case ETXTBSY:           return VERR_SHARING_VIOLATION;
#endif
#ifdef EFBIG
        case EFBIG:             return VERR_FILE_TOO_BIG;
#endif
#ifdef ENOSPC
        case ENOSPC:            return VERR_DISK_FULL;
#endif
#ifdef ESPIPE
        case ESPIPE:            return VERR_SEEK_ON_DEVICE;
#endif
#ifdef EROFS
        case EROFS:             return VERR_WRITE_PROTECT;                      /*  30 */
#endif
#ifdef EMLINK
        //case EMLINK:
#endif
#ifdef EPIPE
        case EPIPE:             return VERR_BROKEN_PIPE;
#endif
#ifdef EDOM
        case EDOM:              return VERR_INVALID_PARAMETER;
#endif
#ifdef ERANGE
        case ERANGE:            return VERR_INVALID_PARAMETER;
#endif
#ifdef EDEADLK
        case EDEADLK:           return VERR_DEADLOCK;
#endif
#ifdef ENAMETOOLONG
        case ENAMETOOLONG:      return VERR_FILENAME_TOO_LONG;
#endif
#ifdef ENOLCK
        case ENOLCK:            return VERR_FILE_LOCK_FAILED;
#endif
#ifdef ENOSYS
        case ENOSYS:            return VERR_NOT_SUPPORTED;
#endif
#ifdef ENOTEMPTY
        case ENOTEMPTY:         return VERR_DIR_NOT_EMPTY;
#endif
#ifdef ELOOP
        case ELOOP:             return VERR_TOO_MANY_SYMLINKS;                  /*  40 */
#endif
        //41??
#ifdef ENOMSG
        //case ENOMSG		42	/* No message of desired type */
#endif
#ifdef EIDRM
        //case EIDRM		43	/* Identifier removed */
#endif
#ifdef ECHRNG
        //case ECHRNG		44	/* Channel number out of range */
#endif
#ifdef EL2NSYNC
        //case EL2NSYNC	45	/* Level 2 not synchronized */
#endif
#ifdef EL3HLT
        //case EL3HLT		46	/* Level 3 halted */
#endif
#ifdef EL3RST
        //case EL3RST		47	/* Level 3 reset */
#endif
#ifdef ELNRNG
        //case ELNRNG		48	/* Link number out of range */
#endif
#ifdef EUNATCH
        //case EUNATCH		49	/* Protocol driver not attached */
#endif
#ifdef ENOCSI
        //case ENOCSI		50	/* No CSI structure available */
#endif
#ifdef EL2HLT
        //case EL2HLT		51	/* Level 2 halted */
#endif
#ifdef EBADE
        //case EBADE		52	/* Invalid exchange */
#endif
#ifdef EBADR
        //case EBADR		53	/* Invalid request descriptor */
#endif
#ifdef EXFULL
        //case EXFULL		54	/* Exchange full */
#endif
#ifdef ENOANO
        //case ENOANO		55	/* No anode */
#endif
#ifdef EBADRQC
        //case EBADRQC		56	/* Invalid request code */
#endif
#ifdef EBADSLT
        //case EBADSLT		57	/* Invalid slot */
#endif
        //case 58:
#ifdef EBFONT
        //case EBFONT		59	/* Bad font file format */
#endif
#ifdef ENOSTR
        //case ENOSTR		60	/* Device not a stream */
#endif
#ifdef ENODATA
        case ENODATA:           return  VERR_NO_DATA;
#endif
#ifdef ETIME
        //case ETIME		62	/* Timer expired */
#endif
#ifdef ENOSR
        //case ENOSR		63	/* Out of streams resources */
#endif
#ifdef ENONET
        case ENONET:            return VERR_NET_NO_NETWORK;
#endif
#ifdef ENOPKG
        //case ENOPKG		65	/* Package not installed */
#endif
#ifdef EREMOTE
        //case EREMOTE		66	/* Object is remote */
#endif
#ifdef ENOLINK
        //case ENOLINK		67	/* Link has been severed */
#endif
#ifdef EADV
        //case EADV		68	/* Advertise error */
#endif
#ifdef ESRMNT
        //case ESRMNT		69	/* Srmount error */
#endif
#ifdef ECOMM
        //case ECOMM		70	/* Communication error on send */
#endif
#ifdef EPROTO
        //case EPROTO		71	/* Protocol error */
#endif
#ifdef EMULTIHOP
        //case EMULTIHOP	72	/* Multihop attempted */
#endif
#ifdef EDOTDOT
        //case EDOTDOT		73	/* RFS specific error */
#endif
#ifdef EBADMSG
        //case EBADMSG		74	/* Not a data message */
#endif
#ifdef EOVERFLOW
        case EOVERFLOW:         return VERR_TOO_MUCH_DATA;
#endif
#ifdef ENOTUNIQ
        case ENOTUNIQ:          return VERR_NET_NOT_UNIQUE_NAME;
#endif
#ifdef EBADFD
        case EBADFD:            return VERR_INVALID_HANDLE;
#endif
#ifdef EREMCHG
        //case EREMCHG		78	/* Remote address changed */
#endif
#ifdef ELIBACC
        //case ELIBACC		79	/* Can not access a needed shared library */
#endif
#ifdef ELIBBAD
        //case ELIBBAD		80	/* Accessing a corrupted shared library */
#endif
#ifdef ELIBSCN
        //case ELIBSCN		81	/* .lib section in a.out corrupted */
#endif
#ifdef ELIBMAX
        //case ELIBMAX		82	/* Attempting to link in too many shared libraries */
#endif
#ifdef ELIBEXEC
        //case ELIBEXEC	83	/* Cannot exec a shared library directly */
#endif
#ifdef EILSEQ
        case EILSEQ:            return VERR_NO_TRANSLATION;
#endif
#ifdef ERESTART
        case ERESTART:          return VERR_INTERRUPTED;
#endif
#ifdef ESTRPIPE
        //case ESTRPIPE	86	/* Streams pipe error */
#endif
#ifdef EUSERS
        //case EUSERS		87	/* Too many users */
#endif
#ifdef ENOTSOCK
        case ENOTSOCK:          return VERR_NET_NOT_SOCKET;
#endif
#ifdef EDESTADDRREQ
        case EDESTADDRREQ:      return VERR_NET_DEST_ADDRESS_REQUIRED;
#endif
#ifdef EMSGSIZE
        case EMSGSIZE:          return VERR_NET_MSG_SIZE;
#endif
#ifdef EPROTOTYPE
        case EPROTOTYPE:        return VERR_NET_PROTOCOL_TYPE;
#endif
#ifdef ENOPROTOOPT
        case ENOPROTOOPT:       return VERR_NET_PROTOCOL_NOT_AVAILABLE;
#endif
#ifdef EPROTONOSUPPORT
        case EPROTONOSUPPORT:   return VERR_NET_PROTOCOL_NOT_SUPPORTED;
#endif
#ifdef ESOCKTNOSUPPORT
        case ESOCKTNOSUPPORT:   return VERR_NET_SOCKET_TYPE_NOT_SUPPORTED;
#endif
#ifdef EOPNOTSUPP
        case EOPNOTSUPP:        return VERR_NET_OPERATION_NOT_SUPPORTED;
#endif
#ifdef EPFNOSUPPORT
        case EPFNOSUPPORT:      return VERR_NET_PROTOCOL_FAMILY_NOT_SUPPORTED;
#endif
#ifdef EAFNOSUPPORT
        case EAFNOSUPPORT:      return VERR_NET_ADDRESS_FAMILY_NOT_SUPPORTED;
#endif
#ifdef EADDRINUSE
        case EADDRINUSE:        return VERR_NET_ADDRESS_IN_USE;
#endif
#ifdef EADDRNOTAVAIL
        case EADDRNOTAVAIL:     return VERR_NET_ADDRESS_NOT_AVAILABLE;
#endif
#ifdef ENETDOWN
        case ENETDOWN:          return VERR_NET_DOWN;
#endif
#ifdef ENETUNREACH
        case ENETUNREACH:       return VERR_NET_UNREACHABLE;
#endif
#ifdef ENETRESET
        case ENETRESET:         return VERR_NET_CONNECTION_RESET;
#endif
#ifdef ECONNABORTED
        case ECONNABORTED:      return VERR_NET_CONNECTION_ABORTED;
#endif
#ifdef ECONNRESET
        case ECONNRESET:        return VERR_NET_CONNECTION_RESET_BY_PEER;
#endif
#ifdef ENOBUFS
        case ENOBUFS:           return VERR_NET_NO_BUFFER_SPACE;
#endif
#ifdef EISCONN
        case EISCONN:           return VERR_NET_ALREADY_CONNECTED;
#endif
#ifdef ENOTCONN
        case ENOTCONN:          return VERR_NET_NOT_CONNECTED;
#endif
#ifdef ESHUTDOWN
        case ESHUTDOWN:         return VERR_NET_SHUTDOWN;
#endif
#ifdef ETOOMANYREFS
        case ETOOMANYREFS:      return VERR_NET_TOO_MANY_REFERENCES;
#endif
#ifdef ETIMEDOUT
        case ETIMEDOUT:         return VERR_TIMEOUT;
#endif
#ifdef ECONNREFUSED
        case ECONNREFUSED:      return VERR_NET_CONNECTION_REFUSED;
#endif
#ifdef EHOSTDOWN
        case EHOSTDOWN:         return VERR_NET_HOST_DOWN;
#endif
#ifdef EHOSTUNREACH
        case EHOSTUNREACH:      return VERR_NET_HOST_UNREACHABLE;
#endif
#ifdef EALREADY
        case EALREADY:          return VERR_NET_ALREADY_IN_PROGRESS;
#endif
#ifdef EINPROGRESS
        case EINPROGRESS:       return VERR_NET_IN_PROGRESS;
#endif
#ifdef ESTALE
        //case ESTALE		116	/* Stale NFS file handle */
#endif
#ifdef EUCLEAN
        //case EUCLEAN		117	/* Structure needs cleaning */
#endif
#ifdef ENOTNAM
        //case ENOTNAM		118	/* Not a XENIX named type file */
#endif
#ifdef ENAVAIL
        //case ENAVAIL		119	/* No XENIX semaphores available */
#endif
#ifdef EISNAM
        //case EISNAM		120	/* Is a named type file */
#endif
#ifdef EREMOTEIO
        //case EREMOTEIO	121	/* Remote I/O error */
#endif
#ifdef EDQUOT
        case EDQUOT:            return VERR_DISK_FULL;
#endif
#ifdef ENOMEDIUM
        case ENOMEDIUM:         return VERR_MEDIA_NOT_PRESENT;
#endif
#ifdef EMEDIUMTYPE
        case EMEDIUMTYPE:       return VERR_MEDIA_NOT_RECOGNIZED;
#endif

        /* Non-linux */

#ifdef EPROCLIM
        case EPROCLIM:          return VERR_MAX_PROCS_REACHED;
#endif

        default:
            AssertMsgFailed(("Unhandled error code %d\n", uNativeCode));
            return VERR_UNRESOLVED_ERROR;
    }
}


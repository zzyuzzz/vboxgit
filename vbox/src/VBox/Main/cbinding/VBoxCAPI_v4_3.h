/*
 *  DO NOT EDIT! This is a generated file.
 *
 *  Header file which provides C declarations for VirtualBox Main API
 *  (COM interfaces), generated from XIDL (XML interface definition).
 *  On Windows (which uses COM instead of XPCOM) the native C support
 *  is used, and most of this file is not used.
 *
 *  Source    : src/VBox/Main/idl/VirtualBox.xidl
 *  Generator : src/VBox/Main/cbinding/capiidl.xsl
 *
 *  This file contains portions from the following Mozilla XPCOM files:
 *      xpcom/include/xpcom/nsID.h
 *      xpcom/include/nsIException.h
 *      xpcom/include/nsprpub/prtypes.h
 *      xpcom/include/xpcom/nsISupportsBase.h
 *
 * These files were originally triple-licensed (MPL/GPL2/LGPL2.1). Oracle
 * elects to distribute this derived work under the LGPL2.1 only.
 */

/*
 * Copyright (C) 2008-2016 Oracle Corporation
 *
 * This file is part of a free software library; you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General
 * Public License version 2.1 as published by the Free Software
 * Foundation and shipped in the "COPYING" file with this library.
 * The library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY of any kind.
 *
 * Oracle LGPL Disclaimer: For the avoidance of doubt, except that if
 * any license choice other than GPL or LGPL is available it will
 * apply instead, Oracle elects to use only the Lesser General Public
 * License version 2.1 (LGPLv2) at this time for any software where
 * a choice of LGPL license versions is made available with the
 * language indicating that LGPLv2 or any later version may be used,
 * or where a choice of which version of the LGPL is applied is
 * otherwise unspecified.
 */

#ifndef ___VirtualBox_CAPI_h
#define ___VirtualBox_CAPI_h

#ifdef _WIN32
# undef COBJMACROS
# define COBJMACROS
# include "Windows.h"
#endif /* _WIN32 */

#ifdef WIN32
# ifdef IN_VBOXCAPI
#  define VBOXCAPI_DECL(type) extern __declspec(dllexport) type
# else /* !IN_VBOXCAPI */
#  define VBOXCAPI_DECL(type) __declspec(dllimport) type
# endif /* !IN_VBOXCAPI */
#endif /* WIN32 */

#ifdef __cplusplus
/* The C++ treatment in this file is not meant for SDK users, it only exists
 * so that this file can be used to produce the VBoxCAPI shared library which
 * has to use C++ as it does all the conversion magic. */
# ifdef IN_VBOXCAPI
#  include "VBox/com/VirtualBox.h"
#  ifndef WIN32
#   include "nsIEventQueue.h"
#  endif /* !WIN32 */
# else /* !IN_VBOXCAPI */
#  error Do not include this header file from C++ code
# endif /* !IN_VBOXCAPI */
#endif /* __cplusplus */

#ifdef __GNUC__
# define VBOX_EXTERN_CONST(type, name) extern const type name __attribute__((nocommon))
#else /* !__GNUC__ */
# define VBOX_EXTERN_CONST(type, name) extern const type name
#endif /* !__GNUC__ */

/* Treat WIN32 completely separately, as on Windows VirtualBox uses COM, not
 * XPCOM like on all other platforms. While the code below would also compile
 * on Windows, we need to switch to the native C support provided by the header
 * files produced by the COM IDL compiler. */
#ifdef WIN32
# include "ObjBase.h"
# include "oaidl.h"
# include "VirtualBox.h"

#ifndef __cplusplus
/* Skip this in the C++ case as there's already a definition for CBSTR. */
typedef const BSTR CBSTR;
#endif /* !__cplusplus */

#define VBOX_WINAPI WINAPI

#define ComSafeArrayAsInParam(f) (f)
#define ComSafeArrayAsOutParam(f) (&(f))
#define ComSafeArrayAsOutTypeParam(f,t) (&(f))
#define ComSafeArrayAsOutIfaceParam(f,t) (&(f))

#else /* !WIN32 */

#include <stddef.h>
#include "wchar.h"

#ifdef IN_VBOXCAPI
# define VBOXCAPI_DECL(type) PR_EXPORT(type)
#else /* !IN_VBOXCAPI */
# define VBOXCAPI_DECL(type) PR_IMPORT(type)
#endif /* !IN_VBOXCAPI */

#ifndef __cplusplus

#if defined(WIN32)

#define PR_EXPORT(__type) extern __declspec(dllexport) __type
#define PR_EXPORT_DATA(__type) extern __declspec(dllexport) __type
#define PR_IMPORT(__type) __declspec(dllimport) __type
#define PR_IMPORT_DATA(__type) __declspec(dllimport) __type

#define PR_EXTERN(__type) extern __declspec(dllexport) __type
#define PR_IMPLEMENT(__type) __declspec(dllexport) __type
#define PR_EXTERN_DATA(__type) extern __declspec(dllexport) __type
#define PR_IMPLEMENT_DATA(__type) __declspec(dllexport) __type

#define PR_CALLBACK
#define PR_CALLBACK_DECL
#define PR_STATIC_CALLBACK(__x) static __x

#elif defined(XP_BEOS)

#define PR_EXPORT(__type) extern __declspec(dllexport) __type
#define PR_EXPORT_DATA(__type) extern __declspec(dllexport) __type
#define PR_IMPORT(__type) extern __declspec(dllexport) __type
#define PR_IMPORT_DATA(__type) extern __declspec(dllexport) __type

#define PR_EXTERN(__type) extern __declspec(dllexport) __type
#define PR_IMPLEMENT(__type) __declspec(dllexport) __type
#define PR_EXTERN_DATA(__type) extern __declspec(dllexport) __type
#define PR_IMPLEMENT_DATA(__type) __declspec(dllexport) __type

#define PR_CALLBACK
#define PR_CALLBACK_DECL
#define PR_STATIC_CALLBACK(__x) static __x

#elif defined(WIN16)

#define PR_CALLBACK_DECL        __cdecl

#if defined(_WINDLL)
#define PR_EXPORT(__type) extern __type _cdecl _export _loadds
#define PR_IMPORT(__type) extern __type _cdecl _export _loadds
#define PR_EXPORT_DATA(__type) extern __type _export
#define PR_IMPORT_DATA(__type) extern __type _export

#define PR_EXTERN(__type) extern __type _cdecl _export _loadds
#define PR_IMPLEMENT(__type) __type _cdecl _export _loadds
#define PR_EXTERN_DATA(__type) extern __type _export
#define PR_IMPLEMENT_DATA(__type) __type _export

#define PR_CALLBACK             __cdecl __loadds
#define PR_STATIC_CALLBACK(__x) static __x PR_CALLBACK

#else /* this must be .EXE */
#define PR_EXPORT(__type) extern __type _cdecl _export
#define PR_IMPORT(__type) extern __type _cdecl _export
#define PR_EXPORT_DATA(__type) extern __type _export
#define PR_IMPORT_DATA(__type) extern __type _export

#define PR_EXTERN(__type) extern __type _cdecl _export
#define PR_IMPLEMENT(__type) __type _cdecl _export
#define PR_EXTERN_DATA(__type) extern __type _export
#define PR_IMPLEMENT_DATA(__type) __type _export

#define PR_CALLBACK             __cdecl __loadds
#define PR_STATIC_CALLBACK(__x) __x PR_CALLBACK
#endif /* _WINDLL */

#elif defined(XP_MAC)

#define PR_EXPORT(__type) extern __declspec(export) __type
#define PR_EXPORT_DATA(__type) extern __declspec(export) __type
#define PR_IMPORT(__type) extern __declspec(export) __type
#define PR_IMPORT_DATA(__type) extern __declspec(export) __type

#define PR_EXTERN(__type) extern __declspec(export) __type
#define PR_IMPLEMENT(__type) __declspec(export) __type
#define PR_EXTERN_DATA(__type) extern __declspec(export) __type
#define PR_IMPLEMENT_DATA(__type) __declspec(export) __type

#define PR_CALLBACK
#define PR_CALLBACK_DECL
#define PR_STATIC_CALLBACK(__x) static __x

#elif defined(XP_OS2) && defined(__declspec)

#define PR_EXPORT(__type) extern __declspec(dllexport) __type
#define PR_EXPORT_DATA(__type) extern __declspec(dllexport) __type
#define PR_IMPORT(__type) __declspec(dllimport) __type
#define PR_IMPORT_DATA(__type) __declspec(dllimport) __type

#define PR_EXTERN(__type) extern __declspec(dllexport) __type
#define PR_IMPLEMENT(__type) __declspec(dllexport) __type
#define PR_EXTERN_DATA(__type) extern __declspec(dllexport) __type
#define PR_IMPLEMENT_DATA(__type) __declspec(dllexport) __type

#define PR_CALLBACK
#define PR_CALLBACK_DECL
#define PR_STATIC_CALLBACK(__x) static __x

#elif defined(XP_OS2_VACPP)

#define PR_EXPORT(__type) extern __type
#define PR_EXPORT_DATA(__type) extern __type
#define PR_IMPORT(__type) extern __type
#define PR_IMPORT_DATA(__type) extern __type

#define PR_EXTERN(__type) extern __type
#define PR_IMPLEMENT(__type) __type
#define PR_EXTERN_DATA(__type) extern __type
#define PR_IMPLEMENT_DATA(__type) __type
#define PR_CALLBACK _Optlink
#define PR_CALLBACK_DECL
#define PR_STATIC_CALLBACK(__x) static __x PR_CALLBACK

#else /* Unix */

# ifdef VBOX_HAVE_VISIBILITY_HIDDEN
#  define PR_EXPORT(__type) __attribute__((visibility("default"))) extern __type
#  define PR_EXPORT_DATA(__type) __attribute__((visibility("default"))) extern __type
#  define PR_IMPORT(__type) extern __type
#  define PR_IMPORT_DATA(__type) extern __type
#  define PR_EXTERN(__type) __attribute__((visibility("default"))) extern __type
#  define PR_IMPLEMENT(__type) __attribute__((visibility("default"))) __type
#  define PR_EXTERN_DATA(__type) __attribute__((visibility("default"))) extern __type
#  define PR_IMPLEMENT_DATA(__type) __attribute__((visibility("default"))) __type
#  define PR_CALLBACK
#  define PR_CALLBACK_DECL
#  define PR_STATIC_CALLBACK(__x) static __x
# else
#  define PR_EXPORT(__type) extern __type
#  define PR_EXPORT_DATA(__type) extern __type
#  define PR_IMPORT(__type) extern __type
#  define PR_IMPORT_DATA(__type) extern __type
#  define PR_EXTERN(__type) extern __type
#  define PR_IMPLEMENT(__type) __type
#  define PR_EXTERN_DATA(__type) extern __type
#  define PR_IMPLEMENT_DATA(__type) __type
#  define PR_CALLBACK
#  define PR_CALLBACK_DECL
#  define PR_STATIC_CALLBACK(__x) static __x
# endif
#endif

#if defined(_NSPR_BUILD_)
#define NSPR_API(__type) PR_EXPORT(__type)
#define NSPR_DATA_API(__type) PR_EXPORT_DATA(__type)
#else
#define NSPR_API(__type) PR_IMPORT(__type)
#define NSPR_DATA_API(__type) PR_IMPORT_DATA(__type)
#endif

typedef unsigned char PRUint8;
#if (defined(HPUX) && defined(__cplusplus) \
        && !defined(__GNUC__) && __cplusplus < 199707L) \
    || (defined(SCO) && defined(__cplusplus) \
        && !defined(__GNUC__) && __cplusplus == 1L)
typedef char PRInt8;
#else
typedef signed char PRInt8;
#endif

#define PR_INT8_MAX 127
#define PR_INT8_MIN (-128)
#define PR_UINT8_MAX 255U

typedef unsigned short PRUint16;
typedef short PRInt16;

#define PR_INT16_MAX 32767
#define PR_INT16_MIN (-32768)
#define PR_UINT16_MAX 65535U

typedef unsigned int PRUint32;
typedef int PRInt32;
#define PR_INT32(x)  x
#define PR_UINT32(x) x ## U

#define PR_INT32_MAX PR_INT32(2147483647)
#define PR_INT32_MIN (-PR_INT32_MAX - 1)
#define PR_UINT32_MAX PR_UINT32(4294967295)

typedef long PRInt64;
typedef unsigned long PRUint64;
typedef int PRIntn;
typedef unsigned int PRUintn;

typedef double          PRFloat64;
typedef size_t PRSize;

typedef ptrdiff_t PRPtrdiff;

typedef unsigned long PRUptrdiff;

typedef PRIntn PRBool;

#define PR_TRUE 1
#define PR_FALSE 0

typedef PRUint8 PRPackedBool;

/*
** Status code used by some routines that have a single point of failure or
** special status return.
*/
typedef enum { PR_FAILURE = -1, PR_SUCCESS = 0 } PRStatus;

#ifndef __PRUNICHAR__
#define __PRUNICHAR__
#if defined(WIN32) || defined(XP_MAC)
typedef wchar_t PRUnichar;
#else
typedef PRUint16 PRUnichar;
#endif
typedef PRUnichar *BSTR;
typedef const PRUnichar *CBSTR;
#endif

typedef long PRWord;
typedef unsigned long PRUword;

#define nsnull 0
typedef PRUint32 nsresult;

#if defined(__GNUC__) && (__GNUC__ > 2)
#define NS_LIKELY(x)    (__builtin_expect((x), 1))
#define NS_UNLIKELY(x)  (__builtin_expect((x), 0))
#else
#define NS_LIKELY(x)    (x)
#define NS_UNLIKELY(x)  (x)
#endif

#define NS_FAILED(_nsresult) (NS_UNLIKELY((_nsresult) & 0x80000000))
#define NS_SUCCEEDED(_nsresult) (NS_LIKELY(!((_nsresult) & 0x80000000)))

#ifdef VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
# define PR_IntervalNow VBoxNsprPR_IntervalNow
# define PR_TicksPerSecond VBoxNsprPR_TicksPerSecond
# define PR_SecondsToInterval VBoxNsprPR_SecondsToInterval
# define PR_MillisecondsToInterval VBoxNsprPR_MillisecondsToInterval
# define PR_MicrosecondsToInterval VBoxNsprPR_MicrosecondsToInterval
# define PR_IntervalToSeconds VBoxNsprPR_IntervalToSeconds
# define PR_IntervalToMilliseconds VBoxNsprPR_IntervalToMilliseconds
# define PR_IntervalToMicroseconds VBoxNsprPR_IntervalToMicroseconds
# define PR_EnterMonitor VBoxNsprPR_EnterMonitor
# define PR_ExitMonitor VBoxNsprPR_ExitMonitor
# define PR_Notify VBoxNsprPR_Notify
# define PR_NotifyAll VBoxNsprPR_NotifyAll
# define PR_Wait VBoxNsprPR_Wait
# define PR_NewMonitor VBoxNsprPR_NewMonitor
# define PR_DestroyMonitor VBoxNsprPR_DestroyMonitor
#endif /* VBOX_WITH_XPCOM_NAMESPACE_CLEANUP */

typedef PRUint32 PRIntervalTime;

#define PR_INTERVAL_MIN 1000UL
#define PR_INTERVAL_MAX 100000UL
#define PR_INTERVAL_NO_WAIT 0UL
#define PR_INTERVAL_NO_TIMEOUT 0xffffffffUL

NSPR_API(PRIntervalTime) PR_IntervalNow(void);
NSPR_API(PRUint32) PR_TicksPerSecond(void);
NSPR_API(PRIntervalTime) PR_SecondsToInterval(PRUint32 seconds);
NSPR_API(PRIntervalTime) PR_MillisecondsToInterval(PRUint32 milli);
NSPR_API(PRIntervalTime) PR_MicrosecondsToInterval(PRUint32 micro);
NSPR_API(PRUint32) PR_IntervalToSeconds(PRIntervalTime ticks);
NSPR_API(PRUint32) PR_IntervalToMilliseconds(PRIntervalTime ticks);
NSPR_API(PRUint32) PR_IntervalToMicroseconds(PRIntervalTime ticks);

typedef struct PRMonitor PRMonitor;

NSPR_API(PRMonitor*) PR_NewMonitor(void);
NSPR_API(void) PR_DestroyMonitor(PRMonitor *mon);
NSPR_API(void) PR_EnterMonitor(PRMonitor *mon);
NSPR_API(PRStatus) PR_ExitMonitor(PRMonitor *mon);
NSPR_API(PRStatus) PR_Wait(PRMonitor *mon, PRIntervalTime ticks);
NSPR_API(PRStatus) PR_Notify(PRMonitor *mon);
NSPR_API(PRStatus) PR_NotifyAll(PRMonitor *mon);

#ifdef VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
# define PR_CreateThread VBoxNsprPR_CreateThread
# define PR_JoinThread VBoxNsprPR_JoinThread
# define PR_Sleep VBoxNsprPR_Sleep
# define PR_GetCurrentThread VBoxNsprPR_GetCurrentThread
# define PR_GetThreadState VBoxNsprPR_GetThreadState
# define PR_SetThreadPrivate VBoxNsprPR_SetThreadPrivate
# define PR_GetThreadPrivate VBoxNsprPR_GetThreadPrivate
# define PR_NewThreadPrivateIndex VBoxNsprPR_NewThreadPrivateIndex
# define PR_GetThreadPriority VBoxNsprPR_GetThreadPriority
# define PR_SetThreadPriority VBoxNsprPR_SetThreadPriority
# define PR_Interrupt VBoxNsprPR_Interrupt
# define PR_ClearInterrupt VBoxNsprPR_ClearInterrupt
# define PR_BlockInterrupt VBoxNsprPR_BlockInterrupt
# define PR_UnblockInterrupt VBoxNsprPR_UnblockInterrupt
# define PR_GetThreadScope VBoxNsprPR_GetThreadScope
# define PR_GetThreadType VBoxNsprPR_GetThreadType
#endif /* VBOX_WITH_XPCOM_NAMESPACE_CLEANUP */

typedef struct PRThread PRThread;
typedef struct PRThreadStack PRThreadStack;

typedef enum PRThreadType {
    PR_USER_THREAD,
    PR_SYSTEM_THREAD
} PRThreadType;

typedef enum PRThreadScope {
    PR_LOCAL_THREAD,
    PR_GLOBAL_THREAD,
    PR_GLOBAL_BOUND_THREAD
} PRThreadScope;

typedef enum PRThreadState {
    PR_JOINABLE_THREAD,
    PR_UNJOINABLE_THREAD
} PRThreadState;

typedef enum PRThreadPriority
{
    PR_PRIORITY_FIRST = 0,      /* just a placeholder */
    PR_PRIORITY_LOW = 0,        /* the lowest possible priority */
    PR_PRIORITY_NORMAL = 1,     /* most common expected priority */
    PR_PRIORITY_HIGH = 2,       /* slightly more aggressive scheduling */
    PR_PRIORITY_URGENT = 3,     /* it does little good to have more than one */
    PR_PRIORITY_LAST = 3        /* this is just a placeholder */
} PRThreadPriority;

NSPR_API(PRThread*) PR_CreateThread(PRThreadType type,
                     void (PR_CALLBACK *start)(void *arg),
                     void *arg,
                     PRThreadPriority priority,
                     PRThreadScope scope,
                     PRThreadState state,
                     PRUint32 stackSize);
NSPR_API(PRStatus) PR_JoinThread(PRThread *thread);
NSPR_API(PRThread*) PR_GetCurrentThread(void);
#ifndef NO_NSPR_10_SUPPORT
#define PR_CurrentThread() PR_GetCurrentThread() /* for nspr1.0 compat. */
#endif /* NO_NSPR_10_SUPPORT */
NSPR_API(PRThreadPriority) PR_GetThreadPriority(const PRThread *thread);
NSPR_API(void) PR_SetThreadPriority(PRThread *thread, PRThreadPriority priority);

typedef void (PR_CALLBACK *PRThreadPrivateDTOR)(void *priv);

NSPR_API(PRStatus) PR_NewThreadPrivateIndex(
    PRUintn *newIndex, PRThreadPrivateDTOR destructor);
NSPR_API(PRStatus) PR_SetThreadPrivate(PRUintn tpdIndex, void *priv);
NSPR_API(void*) PR_GetThreadPrivate(PRUintn tpdIndex);
NSPR_API(PRStatus) PR_Interrupt(PRThread *thread);
NSPR_API(void) PR_ClearInterrupt(void);
NSPR_API(void) PR_BlockInterrupt(void);
NSPR_API(void) PR_UnblockInterrupt(void);
NSPR_API(PRStatus) PR_Sleep(PRIntervalTime ticks);
NSPR_API(PRThreadScope) PR_GetThreadScope(const PRThread *thread);
NSPR_API(PRThreadType) PR_GetThreadType(const PRThread *thread);
NSPR_API(PRThreadState) PR_GetThreadState(const PRThread *thread);

#ifdef VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
# define PR_DestroyLock VBoxNsprPR_DestroyLock
# define PR_Lock VBoxNsprPR_Lock
# define PR_NewLock VBoxNsprPR_NewLock
# define PR_Unlock VBoxNsprPR_Unlock
#endif /* VBOX_WITH_XPCOM_NAMESPACE_CLEANUP */

typedef struct PRLock PRLock;

NSPR_API(PRLock*) PR_NewLock(void);
NSPR_API(void) PR_DestroyLock(PRLock *lock);
NSPR_API(void) PR_Lock(PRLock *lock);
NSPR_API(PRStatus) PR_Unlock(PRLock *lock);

#ifdef VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
# define PR_NewCondVar VBoxNsprPR_NewCondVar
# define PR_DestroyCondVar VBoxNsprPR_DestroyCondVar
# define PR_WaitCondVar VBoxNsprPR_WaitCondVar
# define PR_NotifyCondVar VBoxNsprPR_NotifyCondVar
# define PR_NotifyAllCondVar VBoxNsprPR_NotifyAllCondVar
#endif /* VBOX_WITH_XPCOM_NAMESPACE_CLEANUP */

typedef struct PRCondVar PRCondVar;

NSPR_API(PRCondVar*) PR_NewCondVar(PRLock *lock);
NSPR_API(void) PR_DestroyCondVar(PRCondVar *cvar);
NSPR_API(PRStatus) PR_WaitCondVar(PRCondVar *cvar, PRIntervalTime timeout);
NSPR_API(PRStatus) PR_NotifyCondVar(PRCondVar *cvar);
NSPR_API(PRStatus) PR_NotifyAllCondVar(PRCondVar *cvar);

typedef struct PRCListStr PRCList;

struct PRCListStr {
    PRCList *next;
    PRCList *prev;
};

#ifdef VBOX_WITH_XPCOM_NAMESPACE_CLEANUP
# define PL_DestroyEvent VBoxNsplPL_DestroyEvent
# define PL_HandleEvent VBoxNsplPL_HandleEvent
# define PL_InitEvent VBoxNsplPL_InitEvent
# define PL_CreateEventQueue VBoxNsplPL_CreateEventQueue
# define PL_CreateMonitoredEventQueue VBoxNsplPL_CreateMonitoredEventQueue
# define PL_CreateNativeEventQueue VBoxNsplPL_CreateNativeEventQueue
# define PL_DequeueEvent VBoxNsplPL_DequeueEvent
# define PL_DestroyEventQueue VBoxNsplPL_DestroyEventQueue
# define PL_EventAvailable VBoxNsplPL_EventAvailable
# define PL_EventLoop VBoxNsplPL_EventLoop
# define PL_GetEvent VBoxNsplPL_GetEvent
# define PL_GetEventOwner VBoxNsplPL_GetEventOwner
# define PL_GetEventQueueMonitor VBoxNsplPL_GetEventQueueMonitor
# define PL_GetEventQueueSelectFD VBoxNsplPL_GetEventQueueSelectFD
# define PL_MapEvents VBoxNsplPL_MapEvents
# define PL_PostEvent VBoxNsplPL_PostEvent
# define PL_PostSynchronousEvent VBoxNsplPL_PostSynchronousEvent
# define PL_ProcessEventsBeforeID VBoxNsplPL_ProcessEventsBeforeID
# define PL_ProcessPendingEvents VBoxNsplPL_ProcessPendingEvents
# define PL_RegisterEventIDFunc VBoxNsplPL_RegisterEventIDFunc
# define PL_RevokeEvents VBoxNsplPL_RevokeEvents
# define PL_UnregisterEventIDFunc VBoxNsplPL_UnregisterEventIDFunc
# define PL_WaitForEvent VBoxNsplPL_WaitForEvent
# define PL_IsQueueNative VBoxNsplPL_IsQueueNative
# define PL_IsQueueOnCurrentThread VBoxNsplPL_IsQueueOnCurrentThread
# define PL_FavorPerformanceHint VBoxNsplPL_FavorPerformanceHint
#endif /* VBOX_WITH_XPCOM_NAMESPACE_CLEANUP */

typedef struct PLEvent PLEvent;
typedef struct PLEventQueue PLEventQueue;

PR_EXTERN(PLEventQueue*)
PL_CreateEventQueue(const char* name, PRThread* handlerThread);
PR_EXTERN(PLEventQueue *)
    PL_CreateNativeEventQueue(
        const char *name,
        PRThread *handlerThread
    );
PR_EXTERN(PLEventQueue *)
    PL_CreateMonitoredEventQueue(
        const char *name,
        PRThread *handlerThread
    );
PR_EXTERN(void)
PL_DestroyEventQueue(PLEventQueue* self);
PR_EXTERN(PRMonitor*)
PL_GetEventQueueMonitor(PLEventQueue* self);

#define PL_ENTER_EVENT_QUEUE_MONITOR(queue) \
    PR_EnterMonitor(PL_GetEventQueueMonitor(queue))

#define PL_EXIT_EVENT_QUEUE_MONITOR(queue)  \
    PR_ExitMonitor(PL_GetEventQueueMonitor(queue))

PR_EXTERN(PRStatus) PL_PostEvent(PLEventQueue* self, PLEvent* event);
PR_EXTERN(void*) PL_PostSynchronousEvent(PLEventQueue* self, PLEvent* event);
PR_EXTERN(PLEvent*) PL_GetEvent(PLEventQueue* self);
PR_EXTERN(PRBool) PL_EventAvailable(PLEventQueue* self);

typedef void (PR_CALLBACK *PLEventFunProc)(PLEvent* event, void* data, PLEventQueue* queue);

PR_EXTERN(void) PL_MapEvents(PLEventQueue* self, PLEventFunProc fun, void* data);
PR_EXTERN(void) PL_RevokeEvents(PLEventQueue* self, void* owner);
PR_EXTERN(void) PL_ProcessPendingEvents(PLEventQueue* self);
PR_EXTERN(PLEvent*) PL_WaitForEvent(PLEventQueue* self);
PR_EXTERN(void) PL_EventLoop(PLEventQueue* self);
PR_EXTERN(PRInt32) PL_GetEventQueueSelectFD(PLEventQueue* self);
PR_EXTERN(PRBool) PL_IsQueueOnCurrentThread( PLEventQueue *queue );
PR_EXTERN(PRBool) PL_IsQueueNative(PLEventQueue *queue);

typedef void* (PR_CALLBACK *PLHandleEventProc)(PLEvent* self);
typedef void (PR_CALLBACK *PLDestroyEventProc)(PLEvent* self);
PR_EXTERN(void)
PL_InitEvent(PLEvent* self, void* owner,
             PLHandleEventProc handler,
             PLDestroyEventProc destructor);
PR_EXTERN(void*) PL_GetEventOwner(PLEvent* self);
PR_EXTERN(void) PL_HandleEvent(PLEvent* self);
PR_EXTERN(void) PL_DestroyEvent(PLEvent* self);
PR_EXTERN(void) PL_DequeueEvent(PLEvent* self, PLEventQueue* queue);
PR_EXTERN(void) PL_FavorPerformanceHint(PRBool favorPerformanceOverEventStarvation, PRUint32 starvationDelay);

struct PLEvent {
    PRCList             link;
    PLHandleEventProc   handler;
    PLDestroyEventProc  destructor;
    void*               owner;
    void*               synchronousResult;
    PRLock*             lock;
    PRCondVar*          condVar;
    PRBool              handled;
#ifdef PL_POST_TIMINGS
    PRIntervalTime      postTime;
#endif
#ifdef XP_UNIX
    unsigned long       id;
#endif /* XP_UNIX */
    /* other fields follow... */
};

#if defined(XP_WIN) || defined(XP_OS2)

PR_EXTERN(HWND)
    PL_GetNativeEventReceiverWindow(
        PLEventQueue *eqp
    );
#endif /* XP_WIN || XP_OS2 */

#ifdef XP_UNIX

PR_EXTERN(PRInt32)
PL_ProcessEventsBeforeID(PLEventQueue *aSelf, unsigned long aID);

typedef unsigned long (PR_CALLBACK *PLGetEventIDFunc)(void *aClosure);

PR_EXTERN(void)
PL_RegisterEventIDFunc(PLEventQueue *aSelf, PLGetEventIDFunc aFunc,
                       void *aClosure);
PR_EXTERN(void) PL_UnregisterEventIDFunc(PLEventQueue *aSelf);

#endif /* XP_UNIX */

/* Standard "it worked" return value */
#define NS_OK                              0

#define NS_ERROR_BASE                      ((nsresult) 0xC1F30000)

/* Returned when an instance is not initialized */
#define NS_ERROR_NOT_INITIALIZED           (NS_ERROR_BASE + 1)

/* Returned when an instance is already initialized */
#define NS_ERROR_ALREADY_INITIALIZED       (NS_ERROR_BASE + 2)

/* Returned by a not implemented function */
#define NS_ERROR_NOT_IMPLEMENTED           ((nsresult) 0x80004001L)

/* Returned when a given interface is not supported. */
#define NS_NOINTERFACE                     ((nsresult) 0x80004002L)
#define NS_ERROR_NO_INTERFACE              NS_NOINTERFACE

#define NS_ERROR_INVALID_POINTER           ((nsresult) 0x80004003L)
#define NS_ERROR_NULL_POINTER              NS_ERROR_INVALID_POINTER

/* Returned when a function aborts */
#define NS_ERROR_ABORT                     ((nsresult) 0x80004004L)

/* Returned when a function fails */
#define NS_ERROR_FAILURE                   ((nsresult) 0x80004005L)

/* Returned when an unexpected error occurs */
#define NS_ERROR_UNEXPECTED                ((nsresult) 0x8000ffffL)

/* Returned when a memory allocation fails */
#define NS_ERROR_OUT_OF_MEMORY             ((nsresult) 0x8007000eL)

/* Returned when an illegal value is passed */
#define NS_ERROR_ILLEGAL_VALUE             ((nsresult) 0x80070057L)
#define NS_ERROR_INVALID_ARG               NS_ERROR_ILLEGAL_VALUE

/* Returned when a class doesn't allow aggregation */
#define NS_ERROR_NO_AGGREGATION            ((nsresult) 0x80040110L)

/* Returned when an operation can't complete due to an unavailable resource */
#define NS_ERROR_NOT_AVAILABLE             ((nsresult) 0x80040111L)

/* Returned when a class is not registered */
#define NS_ERROR_FACTORY_NOT_REGISTERED    ((nsresult) 0x80040154L)

/* Returned when a class cannot be registered, but may be tried again later */
#define NS_ERROR_FACTORY_REGISTER_AGAIN    ((nsresult) 0x80040155L)

/* Returned when a dynamically loaded factory couldn't be found */
#define NS_ERROR_FACTORY_NOT_LOADED        ((nsresult) 0x800401f8L)

/* Returned when a factory doesn't support signatures */
#define NS_ERROR_FACTORY_NO_SIGNATURE_SUPPORT \
                                           (NS_ERROR_BASE + 0x101)

/* Returned when a factory already is registered */
#define NS_ERROR_FACTORY_EXISTS            (NS_ERROR_BASE + 0x100)

/**
 * An "interface id" which can be used to uniquely identify a given
 * interface.
 * A "unique identifier". This is modeled after OSF DCE UUIDs.
 */

struct nsID {
    PRUint32 m0;
    PRUint16 m1;
    PRUint16 m2;
    PRUint8 m3[8];
};

typedef struct nsID nsID;
typedef nsID nsIID;
typedef nsID nsCID;

#endif /* __cplusplus */

#define VBOX_WINAPI

/* Various COM types defined by their XPCOM equivalent */
typedef PRInt64 LONG64;
typedef PRInt32 LONG;
typedef PRInt32 DWORD;
typedef PRInt16 SHORT;
typedef PRUint64 ULONG64;
typedef PRUint32 ULONG;
typedef PRUint16 USHORT;

typedef PRBool BOOL;

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#define HRESULT nsresult
#define SUCCEEDED NS_SUCCEEDED
#define FAILED NS_FAILED

/* OLE error codes */
#define S_OK                ((nsresult)NS_OK)
#define E_UNEXPECTED        NS_ERROR_UNEXPECTED
#define E_NOTIMPL           NS_ERROR_NOT_IMPLEMENTED
#define E_OUTOFMEMORY       NS_ERROR_OUT_OF_MEMORY
#define E_INVALIDARG        NS_ERROR_INVALID_ARG
#define E_NOINTERFACE       NS_ERROR_NO_INTERFACE
#define E_POINTER           NS_ERROR_NULL_POINTER
#define E_ABORT             NS_ERROR_ABORT
#define E_FAIL              NS_ERROR_FAILURE
/* Note: a better analog for E_ACCESSDENIED would probably be
 * NS_ERROR_NOT_AVAILABLE, but we want binary compatibility for now. */
#define E_ACCESSDENIED      ((nsresult)0x80070005L)

/* Basic vartype for COM compatibility. */
typedef enum VARTYPE
{
    VT_I2 = 2,
    VT_I4 = 3,
    VT_BSTR = 8,
    VT_DISPATCH = 9,
    VT_BOOL = 11,
    VT_UNKNOWN = 13,
    VT_I1 = 16,
    VT_UI1 = 17,
    VT_UI2 = 18,
    VT_UI4 = 19,
    VT_I8 = 20,
    VT_UI8 = 21,
    VT_HRESULT = 25
} VARTYPE;

/* Basic safearray type for COM compatibility. */
typedef struct SAFEARRAY
{
    void *pv;
    ULONG c;
} SAFEARRAY;

#define ComSafeArrayAsInParam(f) ((f)->c), ((f)->pv)
#define ComSafeArrayAsOutParam(f) (&((f)->c)), (&((f)->pv))
#define ComSafeArrayAsOutTypeParam(f,t) (&((f)->c)), (t**)(&((f)->pv))
#define ComSafeArrayAsOutIfaceParam(f,t) (&((f)->c)), (t**)(&((f)->pv))

/* Glossing over differences between COM and XPCOM */
#define IErrorInfo nsIException
#define IUnknown nsISupports
#define IDispatch nsISupports

/* Make things as COM compatible as possible */
#define interface struct
#ifdef CONST_VTABLE
# define CONST_VTBL const
#else /* !CONST_VTABLE */
# define CONST_VTBL
#endif /* !CONST_VTABLE */

#ifndef __cplusplus

/** @todo this first batch of forward declarations (and the corresponding ones
 * generated for each interface) are 100% redundant, remove eventually. */
interface nsISupports;   /* forward declaration */
interface nsIException;  /* forward declaration */
interface nsIStackFrame; /* forward declaration */
interface nsIEventTarget;/* forward declaration */
interface nsIEventQueue; /* forward declaration */

typedef interface nsISupports nsISupports;     /* forward declaration */
typedef interface nsIException nsIException;   /* forward declaration */
typedef interface nsIStackFrame nsIStackFrame; /* forward declaration */
typedef interface nsIEventTarget nsIEventTarget;/* forward declaration */
typedef interface nsIEventQueue nsIEventQueue; /* forward declaration */

/* starting interface:    nsISupports */
#define NS_ISUPPORTS_IID_STR "00000000-0000-0000-c000-000000000046"

#define NS_ISUPPORTS_IID \
    { 0x00000000, 0x0000, 0x0000, \
      {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46} }

/**
 * Reference count values
 *
 * This is the return type for AddRef() and Release() in nsISupports.
 * IUnknown of COM returns an unsigned long from equivalent functions.
 * The following ifdef exists to maintain binary compatibility with
 * IUnknown.
 */
#if defined(XP_WIN) && PR_BYTES_PER_LONG == 4
typedef unsigned long nsrefcnt;
#else
typedef PRUint32 nsrefcnt;
#endif

/**
 * Basic component object model interface. Objects which implement
 * this interface support runtime interface discovery (QueryInterface)
 * and a reference counted memory model (AddRef/Release). This is
 * modelled after the win32 IUnknown API.
 */
#ifndef VBOX_WITH_GLUE
struct nsISupports_vtbl
{
    nsresult (*QueryInterface)(nsISupports *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(nsISupports *pThis);
    nsrefcnt (*Release)(nsISupports *pThis);
};
#else /* !VBOX_WITH_GLUE */
struct nsISupportsVtbl
{
    nsresult (*QueryInterface)(nsISupports *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(nsISupports *pThis);
    nsrefcnt (*Release)(nsISupports *pThis);
};
#define nsISupports_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define nsISupports_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define nsISupports_Release(p) ((p)->lpVtbl->Release(p))
#define IUnknown_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUnknown_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUnknown_Release(p) ((p)->lpVtbl->Release(p))
#define IDispatch_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IDispatch_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IDispatch_Release(p) ((p)->lpVtbl->Release(p))
#endif /* !VBOX_WITH_GLUE */

interface nsISupports
{
#ifndef VBOX_WITH_GLUE
    struct nsISupports_vtbl *vtbl;
#else /* !VBOX_WITH_GLUE */
    CONST_VTBL struct nsISupportsVtbl *lpVtbl;
#endif /* !VBOX_WITH_GLUE */
};

/* starting interface:    nsIException */
#define NS_IEXCEPTION_IID_STR "f3a8d3b4-c424-4edc-8bf6-8974c983ba78"

#define NS_IEXCEPTION_IID \
    {0xf3a8d3b4, 0xc424, 0x4edc, \
      { 0x8b, 0xf6, 0x89, 0x74, 0xc9, 0x83, 0xba, 0x78 }}

#ifndef VBOX_WITH_GLUE
struct nsIException_vtbl
{
    /* Methods from the interface nsISupports */
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetMessage)(nsIException *pThis, PRUnichar * *aMessage);
    nsresult (*GetResult)(nsIException *pThis, nsresult *aResult);
    nsresult (*GetName)(nsIException *pThis, PRUnichar * *aName);
    nsresult (*GetFilename)(nsIException *pThis, PRUnichar * *aFilename);
    nsresult (*GetLineNumber)(nsIException *pThis, PRUint32 *aLineNumber);
    nsresult (*GetColumnNumber)(nsIException *pThis, PRUint32 *aColumnNumber);
    nsresult (*GetLocation)(nsIException *pThis, nsIStackFrame * *aLocation);
    nsresult (*GetInner)(nsIException *pThis, nsIException * *aInner);
    nsresult (*GetData)(nsIException *pThis, nsISupports * *aData);
    nsresult (*ToString)(nsIException *pThis, PRUnichar **_retval);
};
#else /* !VBOX_WITH_GLUE */
struct nsIExceptionVtbl
{
    nsresult (*QueryInterface)(nsIException *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(nsIException *pThis);
    nsrefcnt (*Release)(nsIException *pThis);

    nsresult (*GetMessage)(nsIException *pThis, PRUnichar * *aMessage);
    nsresult (*GetResult)(nsIException *pThis, nsresult *aResult);
    nsresult (*GetName)(nsIException *pThis, PRUnichar * *aName);
    nsresult (*GetFilename)(nsIException *pThis, PRUnichar * *aFilename);
    nsresult (*GetLineNumber)(nsIException *pThis, PRUint32 *aLineNumber);
    nsresult (*GetColumnNumber)(nsIException *pThis, PRUint32 *aColumnNumber);
    nsresult (*GetLocation)(nsIException *pThis, nsIStackFrame * *aLocation);
    nsresult (*GetInner)(nsIException *pThis, nsIException * *aInner);
    nsresult (*GetData)(nsIException *pThis, nsISupports * *aData);
    nsresult (*ToString)(nsIException *pThis, PRUnichar **_retval);
};
#define nsIException_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define nsIException_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define nsIException_Release(p) ((p)->lpVtbl->Release(p))
#define nsIException_get_Message(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define nsIException_GetMessage(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define nsIException_get_Result(p, aResult) ((p)->lpVtbl->GetResult(p, aResult))
#define nsIException_GetResult(p, aResult) ((p)->lpVtbl->GetResult(p, aResult))
#define nsIException_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define nsIException_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define nsIException_get_Filename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define nsIException_GetFilename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define nsIException_get_LineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define nsIException_GetLineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define nsIException_get_ColumnNumber(p, aColumnNumber) ((p)->lpVtbl->GetColumnNumber(p, aColumnNumber))
#define nsIException_GetColumnNumber(p, aColumnNumber) ((p)->lpVtbl->GetColumnNumber(p, aColumnNumber))
#define nsIException_get_Inner(p, aInner) ((p)->lpVtbl->GetInner(p, aInner))
#define nsIException_GetInner(p, aInner) ((p)->lpVtbl->GetInner(p, aInner))
#define nsIException_get_Data(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define nsIException_GetData(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define nsIException_ToString(p, retval) ((p)->lpVtbl->ToString(p, retval))
#define IErrorInfo_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IErrorInfo_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IErrorInfo_Release(p) ((p)->lpVtbl->Release(p))
#define IErrorInfo_get_Message(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define IErrorInfo_GetMessage(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define IErrorInfo_get_Result(p, aResult) ((p)->lpVtbl->GetResult(p, aResult))
#define IErrorInfo_GetResult(p, aResult) ((p)->lpVtbl->GetResult(p, aResult))
#define IErrorInfo_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IErrorInfo_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IErrorInfo_get_Filename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define IErrorInfo_GetFilename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define IErrorInfo_get_LineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define IErrorInfo_GetLineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define IErrorInfo_get_ColumnNumber(p, aColumnNumber) ((p)->lpVtbl->GetColumnNumber(p, aColumnNumber))
#define IErrorInfo_GetColumnNumber(p, aColumnNumber) ((p)->lpVtbl->GetColumnNumber(p, aColumnNumber))
#define IErrorInfo_get_Inner(p, aInner) ((p)->lpVtbl->GetInner(p, aInner))
#define IErrorInfo_GetInner(p, aInner) ((p)->lpVtbl->GetInner(p, aInner))
#define IErrorInfo_get_Data(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define IErrorInfo_GetData(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define IErrorInfo_ToString(p, retval) ((p)->lpVtbl->ToString(p, retval))
#endif /* !VBOX_WITH_GLUE */

interface nsIException
{
#ifndef VBOX_WITH_GLUE
    struct nsIException_vtbl *vtbl;
#else /* !VBOX_WITH_GLUE */
    CONST_VTBL struct nsIExceptionVtbl *lpVtbl;
#endif /* !VBOX_WITH_GLUE */
};

/* starting interface:    nsIStackFrame */
#define NS_ISTACKFRAME_IID_STR "91d82105-7c62-4f8b-9779-154277c0ee90"

#define NS_ISTACKFRAME_IID \
    {0x91d82105, 0x7c62, 0x4f8b, \
      { 0x97, 0x79, 0x15, 0x42, 0x77, 0xc0, 0xee, 0x90 }}

#ifndef VBOX_WITH_GLUE
struct nsIStackFrame_vtbl
{
    /* Methods from the interface nsISupports */
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetLanguage)(nsIStackFrame *pThis, PRUint32 *aLanguage);
    nsresult (*GetLanguageName)(nsIStackFrame *pThis, PRUnichar * *aLanguageName);
    nsresult (*GetFilename)(nsIStackFrame *pThis, PRUnichar * *aFilename);
    nsresult (*GetName)(nsIStackFrame *pThis, PRUnichar * *aName);
    nsresult (*GetLineNumber)(nsIStackFrame *pThis, PRInt32 *aLineNumber);
    nsresult (*GetSourceLine)(nsIStackFrame *pThis, PRUnichar * *aSourceLine);
    nsresult (*GetCaller)(nsIStackFrame *pThis, nsIStackFrame * *aCaller);
    nsresult (*ToString)(nsIStackFrame *pThis, PRUnichar **_retval);
};
#else /* !VBOX_WITH_GLUE */
struct nsIStackFrameVtbl
{
    nsresult (*QueryInterface)(nsIStackFrame *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(nsIStackFrame *pThis);
    nsrefcnt (*Release)(nsIStackFrame *pThis);

    nsresult (*GetLanguage)(nsIStackFrame *pThis, PRUint32 *aLanguage);
    nsresult (*GetLanguageName)(nsIStackFrame *pThis, PRUnichar * *aLanguageName);
    nsresult (*GetFilename)(nsIStackFrame *pThis, PRUnichar * *aFilename);
    nsresult (*GetName)(nsIStackFrame *pThis, PRUnichar * *aName);
    nsresult (*GetLineNumber)(nsIStackFrame *pThis, PRInt32 *aLineNumber);
    nsresult (*GetSourceLine)(nsIStackFrame *pThis, PRUnichar * *aSourceLine);
    nsresult (*GetCaller)(nsIStackFrame *pThis, nsIStackFrame * *aCaller);
    nsresult (*ToString)(nsIStackFrame *pThis, PRUnichar **_retval);
};
#define nsIStackFrame_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define nsIStackFrame_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define nsIStackFrame_Release(p) ((p)->lpVtbl->Release(p))
#define nsIStackFrame_get_Language(p, aLanguage) ((p)->lpVtbl->GetLanguge(p, aLanguage))
#define nsIStackFrame_GetLanguage(p, aLanguage) ((p)->lpVtbl->GetLanguge(p, aLanguage))
#define nsIStackFrame_get_LanguageName(p, aLanguageName) ((p)->lpVtbl->GetLanguageName(p, aLanguageName))
#define nsIStackFrame_GetLanguageName(p, aLanguageName) ((p)->lpVtbl->GetLanguageName(p, aLanguageName))
#define nsIStackFrame_get_Filename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define nsIStackFrame_GetFilename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define nsIStackFrame_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define nsIStackFrame_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define nsIStackFrame_get_LineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define nsIStackFrame_GetLineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define nsIStackFrame_get_SourceLine(p, aSourceLine) ((p)->lpVtbl->GetSourceLine(p, aSourceLine))
#define nsIStackFrame_GetSourceLine(p, aSourceLine) ((p)->lpVtbl->GetSourceLine(p, aSourceLine))
#define nsIStackFrame_get_Caller(p, aCaller) ((p)->lpVtbl->GetCaller(p, aCaller))
#define nsIStackFrame_GetCaller(p, aCaller) ((p)->lpVtbl->GetCaller(p, aCaller))
#define nsIStackFrame_ToString(p, retval) ((p)->lpVtbl->ToString(p, retval))
#endif /* !VBOX_WITH_GLUE */

interface nsIStackFrame
{
#ifndef VBOX_WITH_GLUE
    struct nsIStackFrame_vtbl *vtbl;
#else /* !VBOX_WITH_GLUE */
    CONST_VTBL struct nsIStackFrameVtbl *lpVtbl;
#endif /* !VBOX_WITH_GLUE */
};

/* starting interface:    nsIEventTarget */
#define NS_IEVENTTARGET_IID_STR "ea99ad5b-cc67-4efb-97c9-2ef620a59f2a"

#define NS_IEVENTTARGET_IID \
    {0xea99ad5b, 0xcc67, 0x4efb, \
      { 0x97, 0xc9, 0x2e, 0xf6, 0x20, 0xa5, 0x9f, 0x2a }}

#ifndef VBOX_WITH_GLUE
struct nsIEventTarget_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*PostEvent)(nsIEventTarget *pThis, PLEvent * aEvent);
    nsresult (*IsOnCurrentThread)(nsIEventTarget *pThis, PRBool *_retval);
};
#else /* !VBOX_WITH_GLUE */
struct nsIEventTargetVtbl
{
    nsresult (*QueryInterface)(nsIEventTarget *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(nsIEventTarget *pThis);
    nsrefcnt (*Release)(nsIEventTarget *pThis);

    nsresult (*PostEvent)(nsIEventTarget *pThis, PLEvent * aEvent);
    nsresult (*IsOnCurrentThread)(nsIEventTarget *pThis, PRBool *_retval);
};
#define nsIEventTarget_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define nsIEventTarget_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define nsIEventTarget_Release(p) ((p)->lpVtbl->Release(p))
#define nsIEventTarget_PostEvent(p, aEvent) ((p)->lpVtbl->PostEvent(p, aEvent))
#define nsIEventTarget_IsOnCurrentThread(p, retval) ((p)->lpVtbl->IsOnCurrentThread(p, retval))
#endif /* !VBOX_WITH_GLUE */

interface nsIEventTarget
{
#ifndef VBOX_WITH_GLUE
    struct nsIEventTarget_vtbl *vtbl;
#else /* !VBOX_WITH_GLUE */
    CONST_VTBL struct nsIEventTargetVtbl *lpVtbl;
#endif /* !VBOX_WITH_GLUE */
};

/* starting interface:    nsIEventQueue */
#define NS_IEVENTQUEUE_IID_STR "176afb41-00a4-11d3-9f2a-00400553eef0"

#define NS_IEVENTQUEUE_IID \
  {0x176afb41, 0x00a4, 0x11d3, \
    { 0x9f, 0x2a, 0x00, 0x40, 0x05, 0x53, 0xee, 0xf0 }}

#ifndef VBOX_WITH_GLUE
struct nsIEventQueue_vtbl
{
    struct nsIEventTarget_vtbl nsieventtarget;

    nsresult (*InitEvent)(nsIEventQueue *pThis, PLEvent * aEvent, void * owner, PLHandleEventProc handler, PLDestroyEventProc destructor);
    nsresult (*PostSynchronousEvent)(nsIEventQueue *pThis, PLEvent * aEvent, void * *aResult);
    nsresult (*PendingEvents)(nsIEventQueue *pThis, PRBool *_retval);
    nsresult (*ProcessPendingEvents)(nsIEventQueue *pThis);
    nsresult (*EventLoop)(nsIEventQueue *pThis);
    nsresult (*EventAvailable)(nsIEventQueue *pThis, PRBool *aResult);
    nsresult (*GetEvent)(nsIEventQueue *pThis, PLEvent * *_retval);
    nsresult (*HandleEvent)(nsIEventQueue *pThis, PLEvent * aEvent);
    nsresult (*WaitForEvent)(nsIEventQueue *pThis, PLEvent * *_retval);
    PRInt32 (*GetEventQueueSelectFD)(nsIEventQueue *pThis);
    nsresult (*Init)(nsIEventQueue *pThis, PRBool aNative);
    nsresult (*InitFromPRThread)(nsIEventQueue *pThis, PRThread * thread, PRBool aNative);
    nsresult (*InitFromPLQueue)(nsIEventQueue *pThis, PLEventQueue * aQueue);
    nsresult (*EnterMonitor)(nsIEventQueue *pThis);
    nsresult (*ExitMonitor)(nsIEventQueue *pThis);
    nsresult (*RevokeEvents)(nsIEventQueue *pThis, void * owner);
    nsresult (*GetPLEventQueue)(nsIEventQueue *pThis, PLEventQueue * *_retval);
    nsresult (*IsQueueNative)(nsIEventQueue *pThis, PRBool *_retval);
    nsresult (*StopAcceptingEvents)(nsIEventQueue *pThis);
};
#else /* !VBOX_WITH_GLUE */
struct nsIEventQueueVtbl
{
    nsresult (*QueryInterface)(nsIEventQueue *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(nsIEventQueue *pThis);
    nsrefcnt (*Release)(nsIEventQueue *pThis);

    nsresult (*PostEvent)(nsIEventQueue *pThis, PLEvent * aEvent);
    nsresult (*IsOnCurrentThread)(nsIEventQueue *pThis, PRBool *_retval);

    nsresult (*InitEvent)(nsIEventQueue *pThis, PLEvent * aEvent, void * owner, PLHandleEventProc handler, PLDestroyEventProc destructor);
    nsresult (*PostSynchronousEvent)(nsIEventQueue *pThis, PLEvent * aEvent, void * *aResult);
    nsresult (*PendingEvents)(nsIEventQueue *pThis, PRBool *_retval);
    nsresult (*ProcessPendingEvents)(nsIEventQueue *pThis);
    nsresult (*EventLoop)(nsIEventQueue *pThis);
    nsresult (*EventAvailable)(nsIEventQueue *pThis, PRBool *aResult);
    nsresult (*GetEvent)(nsIEventQueue *pThis, PLEvent * *_retval);
    nsresult (*HandleEvent)(nsIEventQueue *pThis, PLEvent * aEvent);
    nsresult (*WaitForEvent)(nsIEventQueue *pThis, PLEvent * *_retval);
    PRInt32 (*GetEventQueueSelectFD)(nsIEventQueue *pThis);
    nsresult (*Init)(nsIEventQueue *pThis, PRBool aNative);
    nsresult (*InitFromPRThread)(nsIEventQueue *pThis, PRThread * thread, PRBool aNative);
    nsresult (*InitFromPLQueue)(nsIEventQueue *pThis, PLEventQueue * aQueue);
    nsresult (*EnterMonitor)(nsIEventQueue *pThis);
    nsresult (*ExitMonitor)(nsIEventQueue *pThis);
    nsresult (*RevokeEvents)(nsIEventQueue *pThis, void * owner);
    nsresult (*GetPLEventQueue)(nsIEventQueue *pThis, PLEventQueue * *_retval);
    nsresult (*IsQueueNative)(nsIEventQueue *pThis, PRBool *_retval);
    nsresult (*StopAcceptingEvents)(nsIEventQueue *pThis);
};
#define nsIEventQueue_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define nsIEventQueue_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define nsIEventQueue_Release(p) ((p)->lpVtbl->Release(p))
#define nsIEventQueue_PostEvent(p, aEvent) ((p)->lpVtbl->PostEvent(p, aEvent))
#define nsIEventQueue_IsOnCurrentThread(p, retval) ((p)->lpVtbl->IsOnCurrentThread(p, retval))
#define nsIEventQueue_InitEvent(p, aEvent, owner, handler, destructor) ((p)->lpVtbl->InitEvent(p, aEvent, owner, handler, destructor))
#define nsIEventQueue_PostSynchronousEvent(p, aEvent, aResult) ((p)->lpVtbl->PostSynchronousEvent(p, aEvent, aResult))
#define nsIEventQueue_ProcessPendingEvents(p) ((p)->lpVtbl->ProcessPendingEvents(p))
#define nsIEventQueue_EventLoop(p) ((p)->lpVtbl->EventLoop(p))
#define nsIEventQueue_EventAvailable(p, aResult) ((p)->lpVtbl->EventAvailable(p, aResult))
#define nsIEventQueue_get_Event(p, aEvent) ((p)->lpVtbl->GetEvent(p, aEvent))
#define nsIEventQueue_GetEvent(p, aEvent) ((p)->lpVtbl->GetEvent(p, aEvent))
#define nsIEventQueue_HandleEvent(p, aEvent) ((p)->lpVtbl->HandleEvent(p, aEvent))
#define nsIEventQueue_WaitForEvent(p, aEvent) ((p)->lpVtbl->WaitForEvent(p, aEvent))
#define nsIEventQueue_GetEventQueueSelectFD(p) ((p)->lpVtbl->GetEventQueueSelectFD(p))
#define nsIEventQueue_Init(p, aNative) ((p)->lpVtbl->Init(p, aNative))
#define nsIEventQueue_InitFromPLQueue(p, aQueue) ((p)->lpVtbl->InitFromPLQueue(p, aQueue))
#define nsIEventQueue_EnterMonitor(p) ((p)->lpVtbl->EnterMonitor(p))
#define nsIEventQueue_ExitMonitor(p) ((p)->lpVtbl->ExitMonitor(p))
#define nsIEventQueue_RevokeEvents(p, owner) ((p)->lpVtbl->RevokeEvents(p, owner))
#define nsIEventQueue_GetPLEventQueue(p, retval) ((p)->lpVtbl->GetPLEventQueue(p, retval))
#define nsIEventQueue_IsQueueNative(p, retval) ((p)->lpVtbl->IsQueueNative(p, retval))
#define nsIEventQueue_StopAcceptingEvents(p) ((p)->lpVtbl->StopAcceptingEvents(p))
#endif /* !VBOX_WITH_GLUE */

interface nsIEventQueue
{
#ifndef VBOX_WITH_GLUE
    struct nsIEventQueue_vtbl *vtbl;
#else /* !VBOX_WITH_GLUE */
    CONST_VTBL struct nsIEventQueueVtbl *lpVtbl;
#endif /* !VBOX_WITH_GLUE */
};


#define VBOX_E_OBJECT_NOT_FOUND 0x80BB0001
#define VBOX_E_INVALID_VM_STATE 0x80BB0002
#define VBOX_E_VM_ERROR 0x80BB0003
#define VBOX_E_FILE_ERROR 0x80BB0004
#define VBOX_E_IPRT_ERROR 0x80BB0005
#define VBOX_E_PDM_ERROR 0x80BB0006
#define VBOX_E_INVALID_OBJECT_STATE 0x80BB0007
#define VBOX_E_HOST_ERROR 0x80BB0008
#define VBOX_E_NOT_SUPPORTED 0x80BB0009
#define VBOX_E_XML_ERROR 0x80BB000A
#define VBOX_E_INVALID_SESSION_STATE 0x80BB000B
#define VBOX_E_OBJECT_IN_USE 0x80BB000C


interface IVirtualBoxErrorInfo;
interface INATNetwork;
interface IDHCPServer;
interface IVirtualBox;
interface IVFSExplorer;
interface IAppliance;
interface IVirtualSystemDescription;
interface IBIOSSettings;
interface IPCIAddress;
interface IPCIDeviceAttachment;
interface IMachine;
interface IEmulatedUSB;
interface IVRDEServerInfo;
interface IConsole;
interface IHostNetworkInterface;
interface IHostVideoInputDevice;
interface IHost;
interface ISystemProperties;
interface IGuestOSType;
interface IAdditionsFacility;
interface IGuestSession;
interface IProcess;
interface IGuestProcess;
interface IDirectory;
interface IGuestDirectory;
interface IFile;
interface IGuestFile;
interface IFsObjInfo;
interface IGuestFsObjInfo;
interface IGuest;
interface IProgress;
interface ISnapshot;
interface IMediumAttachment;
interface IMedium;
interface IMediumFormat;
interface IToken;
interface IKeyboard;
interface IMouse;
interface IFramebuffer;
interface IFramebufferOverlay;
interface IDisplay;
interface INetworkAdapter;
interface ISerialPort;
interface IParallelPort;
interface IMachineDebugger;
interface IUSBDeviceFilters;
interface IUSBController;
interface IUSBDevice;
interface IUSBDeviceFilter;
interface IHostUSBDevice;
interface IHostUSBDeviceFilter;
interface IAudioAdapter;
interface IVRDEServer;
interface ISharedFolder;
interface ISession;
interface IStorageController;
interface IPerformanceMetric;
interface IPerformanceCollector;
interface INATEngine;
interface IExtPackPlugIn;
interface IExtPackBase;
interface IExtPack;
interface IExtPackFile;
interface IExtPackManager;
interface IBandwidthGroup;
interface IBandwidthControl;
interface IVirtualBoxClient;
interface IEventSource;
interface IEventListener;
interface IEvent;
interface IReusableEvent;
interface IMachineEvent;
interface IMachineStateChangedEvent;
interface IMachineDataChangedEvent;
interface IMediumRegisteredEvent;
interface IMachineRegisteredEvent;
interface ISessionStateChangedEvent;
interface IGuestPropertyChangedEvent;
interface ISnapshotEvent;
interface ISnapshotTakenEvent;
interface ISnapshotDeletedEvent;
interface ISnapshotChangedEvent;
interface IMousePointerShapeChangedEvent;
interface IMouseCapabilityChangedEvent;
interface IKeyboardLedsChangedEvent;
interface IStateChangedEvent;
interface IAdditionsStateChangedEvent;
interface INetworkAdapterChangedEvent;
interface ISerialPortChangedEvent;
interface IParallelPortChangedEvent;
interface IStorageControllerChangedEvent;
interface IMediumChangedEvent;
interface IClipboardModeChangedEvent;
interface IDragAndDropModeChangedEvent;
interface ICPUChangedEvent;
interface ICPUExecutionCapChangedEvent;
interface IGuestKeyboardEvent;
interface IGuestMouseEvent;
interface IGuestMultiTouchEvent;
interface IGuestSessionEvent;
interface IGuestSessionStateChangedEvent;
interface IGuestSessionRegisteredEvent;
interface IGuestProcessEvent;
interface IGuestProcessRegisteredEvent;
interface IGuestProcessStateChangedEvent;
interface IGuestProcessIOEvent;
interface IGuestProcessInputNotifyEvent;
interface IGuestProcessOutputEvent;
interface IGuestFileEvent;
interface IGuestFileRegisteredEvent;
interface IGuestFileStateChangedEvent;
interface IGuestFileIOEvent;
interface IGuestFileOffsetChangedEvent;
interface IGuestFileReadEvent;
interface IGuestFileWriteEvent;
interface IVRDEServerChangedEvent;
interface IVRDEServerInfoChangedEvent;
interface IVideoCaptureChangedEvent;
interface IUSBControllerChangedEvent;
interface IUSBDeviceStateChangedEvent;
interface ISharedFolderChangedEvent;
interface IRuntimeErrorEvent;
interface IEventSourceChangedEvent;
interface IExtraDataChangedEvent;
interface IVetoEvent;
interface IExtraDataCanChangeEvent;
interface ICanShowWindowEvent;
interface IShowWindowEvent;
interface INATRedirectEvent;
interface IHostPCIDevicePlugEvent;
interface IVBoxSVCAvailabilityChangedEvent;
interface IBandwidthGroupChangedEvent;
interface IGuestMonitorChangedEvent;
interface IGuestUserStateChangedEvent;
interface IStorageDeviceChangedEvent;
interface INATNetworkChangedEvent;
interface INATNetworkStartStopEvent;
interface INATNetworkAlterEvent;
interface INATNetworkCreationDeletionEvent;
interface INATNetworkSettingEvent;
interface INATNetworkPortForwardEvent;
interface IHostNameResolutionConfigurationChangeEvent;

typedef interface IVirtualBoxErrorInfo IVirtualBoxErrorInfo;
typedef interface INATNetwork INATNetwork;
typedef interface IDHCPServer IDHCPServer;
typedef interface IVirtualBox IVirtualBox;
typedef interface IVFSExplorer IVFSExplorer;
typedef interface IAppliance IAppliance;
typedef interface IVirtualSystemDescription IVirtualSystemDescription;
typedef interface IBIOSSettings IBIOSSettings;
typedef interface IPCIAddress IPCIAddress;
typedef interface IPCIDeviceAttachment IPCIDeviceAttachment;
typedef interface IMachine IMachine;
typedef interface IEmulatedUSB IEmulatedUSB;
typedef interface IVRDEServerInfo IVRDEServerInfo;
typedef interface IConsole IConsole;
typedef interface IHostNetworkInterface IHostNetworkInterface;
typedef interface IHostVideoInputDevice IHostVideoInputDevice;
typedef interface IHost IHost;
typedef interface ISystemProperties ISystemProperties;
typedef interface IGuestOSType IGuestOSType;
typedef interface IAdditionsFacility IAdditionsFacility;
typedef interface IGuestSession IGuestSession;
typedef interface IProcess IProcess;
typedef interface IGuestProcess IGuestProcess;
typedef interface IDirectory IDirectory;
typedef interface IGuestDirectory IGuestDirectory;
typedef interface IFile IFile;
typedef interface IGuestFile IGuestFile;
typedef interface IFsObjInfo IFsObjInfo;
typedef interface IGuestFsObjInfo IGuestFsObjInfo;
typedef interface IGuest IGuest;
typedef interface IProgress IProgress;
typedef interface ISnapshot ISnapshot;
typedef interface IMediumAttachment IMediumAttachment;
typedef interface IMedium IMedium;
typedef interface IMediumFormat IMediumFormat;
typedef interface IToken IToken;
typedef interface IKeyboard IKeyboard;
typedef interface IMouse IMouse;
typedef interface IFramebuffer IFramebuffer;
typedef interface IFramebufferOverlay IFramebufferOverlay;
typedef interface IDisplay IDisplay;
typedef interface INetworkAdapter INetworkAdapter;
typedef interface ISerialPort ISerialPort;
typedef interface IParallelPort IParallelPort;
typedef interface IMachineDebugger IMachineDebugger;
typedef interface IUSBDeviceFilters IUSBDeviceFilters;
typedef interface IUSBController IUSBController;
typedef interface IUSBDevice IUSBDevice;
typedef interface IUSBDeviceFilter IUSBDeviceFilter;
typedef interface IHostUSBDevice IHostUSBDevice;
typedef interface IHostUSBDeviceFilter IHostUSBDeviceFilter;
typedef interface IAudioAdapter IAudioAdapter;
typedef interface IVRDEServer IVRDEServer;
typedef interface ISharedFolder ISharedFolder;
typedef interface ISession ISession;
typedef interface IStorageController IStorageController;
typedef interface IPerformanceMetric IPerformanceMetric;
typedef interface IPerformanceCollector IPerformanceCollector;
typedef interface INATEngine INATEngine;
typedef interface IExtPackPlugIn IExtPackPlugIn;
typedef interface IExtPackBase IExtPackBase;
typedef interface IExtPack IExtPack;
typedef interface IExtPackFile IExtPackFile;
typedef interface IExtPackManager IExtPackManager;
typedef interface IBandwidthGroup IBandwidthGroup;
typedef interface IBandwidthControl IBandwidthControl;
typedef interface IVirtualBoxClient IVirtualBoxClient;
typedef interface IEventSource IEventSource;
typedef interface IEventListener IEventListener;
typedef interface IEvent IEvent;
typedef interface IReusableEvent IReusableEvent;
typedef interface IMachineEvent IMachineEvent;
typedef interface IMachineStateChangedEvent IMachineStateChangedEvent;
typedef interface IMachineDataChangedEvent IMachineDataChangedEvent;
typedef interface IMediumRegisteredEvent IMediumRegisteredEvent;
typedef interface IMachineRegisteredEvent IMachineRegisteredEvent;
typedef interface ISessionStateChangedEvent ISessionStateChangedEvent;
typedef interface IGuestPropertyChangedEvent IGuestPropertyChangedEvent;
typedef interface ISnapshotEvent ISnapshotEvent;
typedef interface ISnapshotTakenEvent ISnapshotTakenEvent;
typedef interface ISnapshotDeletedEvent ISnapshotDeletedEvent;
typedef interface ISnapshotChangedEvent ISnapshotChangedEvent;
typedef interface IMousePointerShapeChangedEvent IMousePointerShapeChangedEvent;
typedef interface IMouseCapabilityChangedEvent IMouseCapabilityChangedEvent;
typedef interface IKeyboardLedsChangedEvent IKeyboardLedsChangedEvent;
typedef interface IStateChangedEvent IStateChangedEvent;
typedef interface IAdditionsStateChangedEvent IAdditionsStateChangedEvent;
typedef interface INetworkAdapterChangedEvent INetworkAdapterChangedEvent;
typedef interface ISerialPortChangedEvent ISerialPortChangedEvent;
typedef interface IParallelPortChangedEvent IParallelPortChangedEvent;
typedef interface IStorageControllerChangedEvent IStorageControllerChangedEvent;
typedef interface IMediumChangedEvent IMediumChangedEvent;
typedef interface IClipboardModeChangedEvent IClipboardModeChangedEvent;
typedef interface IDragAndDropModeChangedEvent IDragAndDropModeChangedEvent;
typedef interface ICPUChangedEvent ICPUChangedEvent;
typedef interface ICPUExecutionCapChangedEvent ICPUExecutionCapChangedEvent;
typedef interface IGuestKeyboardEvent IGuestKeyboardEvent;
typedef interface IGuestMouseEvent IGuestMouseEvent;
typedef interface IGuestMultiTouchEvent IGuestMultiTouchEvent;
typedef interface IGuestSessionEvent IGuestSessionEvent;
typedef interface IGuestSessionStateChangedEvent IGuestSessionStateChangedEvent;
typedef interface IGuestSessionRegisteredEvent IGuestSessionRegisteredEvent;
typedef interface IGuestProcessEvent IGuestProcessEvent;
typedef interface IGuestProcessRegisteredEvent IGuestProcessRegisteredEvent;
typedef interface IGuestProcessStateChangedEvent IGuestProcessStateChangedEvent;
typedef interface IGuestProcessIOEvent IGuestProcessIOEvent;
typedef interface IGuestProcessInputNotifyEvent IGuestProcessInputNotifyEvent;
typedef interface IGuestProcessOutputEvent IGuestProcessOutputEvent;
typedef interface IGuestFileEvent IGuestFileEvent;
typedef interface IGuestFileRegisteredEvent IGuestFileRegisteredEvent;
typedef interface IGuestFileStateChangedEvent IGuestFileStateChangedEvent;
typedef interface IGuestFileIOEvent IGuestFileIOEvent;
typedef interface IGuestFileOffsetChangedEvent IGuestFileOffsetChangedEvent;
typedef interface IGuestFileReadEvent IGuestFileReadEvent;
typedef interface IGuestFileWriteEvent IGuestFileWriteEvent;
typedef interface IVRDEServerChangedEvent IVRDEServerChangedEvent;
typedef interface IVRDEServerInfoChangedEvent IVRDEServerInfoChangedEvent;
typedef interface IVideoCaptureChangedEvent IVideoCaptureChangedEvent;
typedef interface IUSBControllerChangedEvent IUSBControllerChangedEvent;
typedef interface IUSBDeviceStateChangedEvent IUSBDeviceStateChangedEvent;
typedef interface ISharedFolderChangedEvent ISharedFolderChangedEvent;
typedef interface IRuntimeErrorEvent IRuntimeErrorEvent;
typedef interface IEventSourceChangedEvent IEventSourceChangedEvent;
typedef interface IExtraDataChangedEvent IExtraDataChangedEvent;
typedef interface IVetoEvent IVetoEvent;
typedef interface IExtraDataCanChangeEvent IExtraDataCanChangeEvent;
typedef interface ICanShowWindowEvent ICanShowWindowEvent;
typedef interface IShowWindowEvent IShowWindowEvent;
typedef interface INATRedirectEvent INATRedirectEvent;
typedef interface IHostPCIDevicePlugEvent IHostPCIDevicePlugEvent;
typedef interface IVBoxSVCAvailabilityChangedEvent IVBoxSVCAvailabilityChangedEvent;
typedef interface IBandwidthGroupChangedEvent IBandwidthGroupChangedEvent;
typedef interface IGuestMonitorChangedEvent IGuestMonitorChangedEvent;
typedef interface IGuestUserStateChangedEvent IGuestUserStateChangedEvent;
typedef interface IStorageDeviceChangedEvent IStorageDeviceChangedEvent;
typedef interface INATNetworkChangedEvent INATNetworkChangedEvent;
typedef interface INATNetworkStartStopEvent INATNetworkStartStopEvent;
typedef interface INATNetworkAlterEvent INATNetworkAlterEvent;
typedef interface INATNetworkCreationDeletionEvent INATNetworkCreationDeletionEvent;
typedef interface INATNetworkSettingEvent INATNetworkSettingEvent;
typedef interface INATNetworkPortForwardEvent INATNetworkPortForwardEvent;
typedef interface IHostNameResolutionConfigurationChangeEvent IHostNameResolutionConfigurationChangeEvent;

/* Start of enum SettingsVersion declaration */
#define SETTINGSVERSION_IID_STR "d5b15ca7-3de7-46b2-a63a-ddcce42bfa3f"
#define SETTINGSVERSION_IID { \
    0xd5b15ca7, 0x3de7, 0x46b2, \
    { 0xa6, 0x3a, 0xdd, 0xcc, 0xe4, 0x2b, 0xfa, 0x3f } \
}
typedef enum SettingsVersion
{
    SettingsVersion_Null = 0,
    SettingsVersion_v1_0 = 1,
    SettingsVersion_v1_1 = 2,
    SettingsVersion_v1_2 = 3,
    SettingsVersion_v1_3pre = 4,
    SettingsVersion_v1_3 = 5,
    SettingsVersion_v1_4 = 6,
    SettingsVersion_v1_5 = 7,
    SettingsVersion_v1_6 = 8,
    SettingsVersion_v1_7 = 9,
    SettingsVersion_v1_8 = 10,
    SettingsVersion_v1_9 = 11,
    SettingsVersion_v1_10 = 12,
    SettingsVersion_v1_11 = 13,
    SettingsVersion_v1_12 = 14,
    SettingsVersion_v1_13 = 15,
    SettingsVersion_v1_14 = 16,
    SettingsVersion_Future = 99999
} SettingsVersion;
/* End of enum SettingsVersion declaration */
#define SettingsVersion_T PRUint32


/* Start of enum AccessMode declaration */
#define ACCESSMODE_IID_STR "1da0007c-ddf7-4be8-bcac-d84a1558785f"
#define ACCESSMODE_IID { \
    0x1da0007c, 0xddf7, 0x4be8, \
    { 0xbc, 0xac, 0xd8, 0x4a, 0x15, 0x58, 0x78, 0x5f } \
}
typedef enum AccessMode
{
    AccessMode_ReadOnly = 1,
    AccessMode_ReadWrite = 2
} AccessMode;
/* End of enum AccessMode declaration */
#define AccessMode_T PRUint32


/* Start of enum MachineState declaration */
#define MACHINESTATE_IID_STR "ec6c6a9e-113d-4ff4-b44f-0b69f21c97fe"
#define MACHINESTATE_IID { \
    0xec6c6a9e, 0x113d, 0x4ff4, \
    { 0xb4, 0x4f, 0x0b, 0x69, 0xf2, 0x1c, 0x97, 0xfe } \
}
typedef enum MachineState
{
    MachineState_Null = 0,
    MachineState_PoweredOff = 1,
    MachineState_Saved = 2,
    MachineState_Teleported = 3,
    MachineState_Aborted = 4,
    MachineState_Running = 5,
    MachineState_Paused = 6,
    MachineState_Stuck = 7,
    MachineState_Teleporting = 8,
    MachineState_LiveSnapshotting = 9,
    MachineState_Starting = 10,
    MachineState_Stopping = 11,
    MachineState_Saving = 12,
    MachineState_Restoring = 13,
    MachineState_TeleportingPausedVM = 14,
    MachineState_TeleportingIn = 15,
    MachineState_FaultTolerantSyncing = 16,
    MachineState_DeletingSnapshotOnline = 17,
    MachineState_DeletingSnapshotPaused = 18,
    MachineState_RestoringSnapshot = 19,
    MachineState_DeletingSnapshot = 20,
    MachineState_SettingUp = 21,
    MachineState_FirstOnline = 5,
    MachineState_LastOnline = 18,
    MachineState_FirstTransient = 8,
    MachineState_LastTransient = 21
} MachineState;
/* End of enum MachineState declaration */
#define MachineState_T PRUint32


/* Start of enum SessionState declaration */
#define SESSIONSTATE_IID_STR "cf2700c0-ea4b-47ae-9725-7810114b94d8"
#define SESSIONSTATE_IID { \
    0xcf2700c0, 0xea4b, 0x47ae, \
    { 0x97, 0x25, 0x78, 0x10, 0x11, 0x4b, 0x94, 0xd8 } \
}
typedef enum SessionState
{
    SessionState_Null = 0,
    SessionState_Unlocked = 1,
    SessionState_Locked = 2,
    SessionState_Spawning = 3,
    SessionState_Unlocking = 4
} SessionState;
/* End of enum SessionState declaration */
#define SessionState_T PRUint32


/* Start of enum CPUPropertyType declaration */
#define CPUPROPERTYTYPE_IID_STR "52bc41f4-a279-45da-88ab-3a1d86fb73eb"
#define CPUPROPERTYTYPE_IID { \
    0x52bc41f4, 0xa279, 0x45da, \
    { 0x88, 0xab, 0x3a, 0x1d, 0x86, 0xfb, 0x73, 0xeb } \
}
typedef enum CPUPropertyType
{
    CPUPropertyType_Null = 0,
    CPUPropertyType_PAE = 1,
    CPUPropertyType_Synthetic = 2,
    CPUPropertyType_LongMode = 3,
    CPUPropertyType_TripleFaultReset = 4
} CPUPropertyType;
/* End of enum CPUPropertyType declaration */
#define CPUPropertyType_T PRUint32


/* Start of enum HWVirtExPropertyType declaration */
#define HWVIRTEXPROPERTYTYPE_IID_STR "411ad0ea-aeeb-44cb-9d03-1624d0d025ac"
#define HWVIRTEXPROPERTYTYPE_IID { \
    0x411ad0ea, 0xaeeb, 0x44cb, \
    { 0x9d, 0x03, 0x16, 0x24, 0xd0, 0xd0, 0x25, 0xac } \
}
typedef enum HWVirtExPropertyType
{
    HWVirtExPropertyType_Null = 0,
    HWVirtExPropertyType_Enabled = 1,
    HWVirtExPropertyType_VPID = 2,
    HWVirtExPropertyType_NestedPaging = 3,
    HWVirtExPropertyType_UnrestrictedExecution = 4,
    HWVirtExPropertyType_LargePages = 5,
    HWVirtExPropertyType_Force = 6
} HWVirtExPropertyType;
/* End of enum HWVirtExPropertyType declaration */
#define HWVirtExPropertyType_T PRUint32


/* Start of enum FaultToleranceState declaration */
#define FAULTTOLERANCESTATE_IID_STR "5124f7ec-6b67-493c-9dee-ee45a44114e1"
#define FAULTTOLERANCESTATE_IID { \
    0x5124f7ec, 0x6b67, 0x493c, \
    { 0x9d, 0xee, 0xee, 0x45, 0xa4, 0x41, 0x14, 0xe1 } \
}
typedef enum FaultToleranceState
{
    FaultToleranceState_Inactive = 1,
    FaultToleranceState_Master = 2,
    FaultToleranceState_Standby = 3
} FaultToleranceState;
/* End of enum FaultToleranceState declaration */
#define FaultToleranceState_T PRUint32


/* Start of enum LockType declaration */
#define LOCKTYPE_IID_STR "168a6a8e-12fd-4878-a1f9-38a750a56089"
#define LOCKTYPE_IID { \
    0x168a6a8e, 0x12fd, 0x4878, \
    { 0xa1, 0xf9, 0x38, 0xa7, 0x50, 0xa5, 0x60, 0x89 } \
}
typedef enum LockType
{
    LockType_Write = 2,
    LockType_Shared = 1,
    LockType_VM = 3
} LockType;
/* End of enum LockType declaration */
#define LockType_T PRUint32


/* Start of enum SessionType declaration */
#define SESSIONTYPE_IID_STR "A13C02CB-0C2C-421E-8317-AC0E8AAA153A"
#define SESSIONTYPE_IID { \
    0xA13C02CB, 0x0C2C, 0x421E, \
    { 0x83, 0x17, 0xAC, 0x0E, 0x8A, 0xAA, 0x15, 0x3A } \
}
typedef enum SessionType
{
    SessionType_Null = 0,
    SessionType_WriteLock = 1,
    SessionType_Remote = 2,
    SessionType_Shared = 3
} SessionType;
/* End of enum SessionType declaration */
#define SessionType_T PRUint32


/* Start of enum DeviceType declaration */
#define DEVICETYPE_IID_STR "6d9420f7-0b56-4636-99f9-7346f1b01e57"
#define DEVICETYPE_IID { \
    0x6d9420f7, 0x0b56, 0x4636, \
    { 0x99, 0xf9, 0x73, 0x46, 0xf1, 0xb0, 0x1e, 0x57 } \
}
typedef enum DeviceType
{
    DeviceType_Null = 0,
    DeviceType_Floppy = 1,
    DeviceType_DVD = 2,
    DeviceType_HardDisk = 3,
    DeviceType_Network = 4,
    DeviceType_USB = 5,
    DeviceType_SharedFolder = 6
} DeviceType;
/* End of enum DeviceType declaration */
#define DeviceType_T PRUint32


/* Start of enum DeviceActivity declaration */
#define DEVICEACTIVITY_IID_STR "6FC8AEAA-130A-4eb5-8954-3F921422D707"
#define DEVICEACTIVITY_IID { \
    0x6FC8AEAA, 0x130A, 0x4eb5, \
    { 0x89, 0x54, 0x3F, 0x92, 0x14, 0x22, 0xD7, 0x07 } \
}
typedef enum DeviceActivity
{
    DeviceActivity_Null = 0,
    DeviceActivity_Idle = 1,
    DeviceActivity_Reading = 2,
    DeviceActivity_Writing = 3
} DeviceActivity;
/* End of enum DeviceActivity declaration */
#define DeviceActivity_T PRUint32


/* Start of enum ClipboardMode declaration */
#define CLIPBOARDMODE_IID_STR "33364716-4008-4701-8f14-be0fa3d62950"
#define CLIPBOARDMODE_IID { \
    0x33364716, 0x4008, 0x4701, \
    { 0x8f, 0x14, 0xbe, 0x0f, 0xa3, 0xd6, 0x29, 0x50 } \
}
typedef enum ClipboardMode
{
    ClipboardMode_Disabled = 0,
    ClipboardMode_HostToGuest = 1,
    ClipboardMode_GuestToHost = 2,
    ClipboardMode_Bidirectional = 3
} ClipboardMode;
/* End of enum ClipboardMode declaration */
#define ClipboardMode_T PRUint32


/* Start of enum DragAndDropMode declaration */
#define DRAGANDDROPMODE_IID_STR "b618ea0e-b6fb-4f8d-97f7-5e237e49b547"
#define DRAGANDDROPMODE_IID { \
    0xb618ea0e, 0xb6fb, 0x4f8d, \
    { 0x97, 0xf7, 0x5e, 0x23, 0x7e, 0x49, 0xb5, 0x47 } \
}
typedef enum DragAndDropMode
{
    DragAndDropMode_Disabled = 0,
    DragAndDropMode_HostToGuest = 1,
    DragAndDropMode_GuestToHost = 2,
    DragAndDropMode_Bidirectional = 3
} DragAndDropMode;
/* End of enum DragAndDropMode declaration */
#define DragAndDropMode_T PRUint32


/* Start of enum Scope declaration */
#define SCOPE_IID_STR "7c91096e-499e-4eca-9f9b-9001438d7855"
#define SCOPE_IID { \
    0x7c91096e, 0x499e, 0x4eca, \
    { 0x9f, 0x9b, 0x90, 0x01, 0x43, 0x8d, 0x78, 0x55 } \
}
typedef enum Scope
{
    Scope_Global = 0,
    Scope_Machine = 1,
    Scope_Session = 2
} Scope;
/* End of enum Scope declaration */
#define Scope_T PRUint32


/* Start of enum BIOSBootMenuMode declaration */
#define BIOSBOOTMENUMODE_IID_STR "ae4fb9f7-29d2-45b4-b2c7-d579603135d5"
#define BIOSBOOTMENUMODE_IID { \
    0xae4fb9f7, 0x29d2, 0x45b4, \
    { 0xb2, 0xc7, 0xd5, 0x79, 0x60, 0x31, 0x35, 0xd5 } \
}
typedef enum BIOSBootMenuMode
{
    BIOSBootMenuMode_Disabled = 0,
    BIOSBootMenuMode_MenuOnly = 1,
    BIOSBootMenuMode_MessageAndMenu = 2
} BIOSBootMenuMode;
/* End of enum BIOSBootMenuMode declaration */
#define BIOSBootMenuMode_T PRUint32


/* Start of enum ProcessorFeature declaration */
#define PROCESSORFEATURE_IID_STR "64c38e6b-8bcf-45ad-ac03-9b406287c5bf"
#define PROCESSORFEATURE_IID { \
    0x64c38e6b, 0x8bcf, 0x45ad, \
    { 0xac, 0x03, 0x9b, 0x40, 0x62, 0x87, 0xc5, 0xbf } \
}
typedef enum ProcessorFeature
{
    ProcessorFeature_HWVirtEx = 0,
    ProcessorFeature_PAE = 1,
    ProcessorFeature_LongMode = 2,
    ProcessorFeature_NestedPaging = 3
} ProcessorFeature;
/* End of enum ProcessorFeature declaration */
#define ProcessorFeature_T PRUint32


/* Start of enum FirmwareType declaration */
#define FIRMWARETYPE_IID_STR "b903f264-c230-483e-ac74-2b37ce60d371"
#define FIRMWARETYPE_IID { \
    0xb903f264, 0xc230, 0x483e, \
    { 0xac, 0x74, 0x2b, 0x37, 0xce, 0x60, 0xd3, 0x71 } \
}
typedef enum FirmwareType
{
    FirmwareType_BIOS = 1,
    FirmwareType_EFI = 2,
    FirmwareType_EFI32 = 3,
    FirmwareType_EFI64 = 4,
    FirmwareType_EFIDUAL = 5
} FirmwareType;
/* End of enum FirmwareType declaration */
#define FirmwareType_T PRUint32


/* Start of enum PointingHIDType declaration */
#define POINTINGHIDTYPE_IID_STR "19964e93-0050-45c4-9382-a7bccc53e666"
#define POINTINGHIDTYPE_IID { \
    0x19964e93, 0x0050, 0x45c4, \
    { 0x93, 0x82, 0xa7, 0xbc, 0xcc, 0x53, 0xe6, 0x66 } \
}
typedef enum PointingHIDType
{
    PointingHIDType_None = 1,
    PointingHIDType_PS2Mouse = 2,
    PointingHIDType_USBMouse = 3,
    PointingHIDType_USBTablet = 4,
    PointingHIDType_ComboMouse = 5,
    PointingHIDType_USBMultiTouch = 6
} PointingHIDType;
/* End of enum PointingHIDType declaration */
#define PointingHIDType_T PRUint32


/* Start of enum KeyboardHIDType declaration */
#define KEYBOARDHIDTYPE_IID_STR "383e43d7-5c7c-4ec8-9cb8-eda1bccd6699"
#define KEYBOARDHIDTYPE_IID { \
    0x383e43d7, 0x5c7c, 0x4ec8, \
    { 0x9c, 0xb8, 0xed, 0xa1, 0xbc, 0xcd, 0x66, 0x99 } \
}
typedef enum KeyboardHIDType
{
    KeyboardHIDType_None = 1,
    KeyboardHIDType_PS2Keyboard = 2,
    KeyboardHIDType_USBKeyboard = 3,
    KeyboardHIDType_ComboKeyboard = 4
} KeyboardHIDType;
/* End of enum KeyboardHIDType declaration */
#define KeyboardHIDType_T PRUint32


/* Start of enum DhcpOpt declaration */
#define DHCPOPT_IID_STR "40d99bd3-3ece-44d2-a07e-1085fe9c4f0b"
#define DHCPOPT_IID { \
    0x40d99bd3, 0x3ece, 0x44d2, \
    { 0xa0, 0x7e, 0x10, 0x85, 0xfe, 0x9c, 0x4f, 0x0b } \
}
typedef enum DhcpOpt
{
    DhcpOpt_SubnetMask = 1,
    DhcpOpt_TimeOffset = 2,
    DhcpOpt_Router = 3,
    DhcpOpt_TimeServer = 4,
    DhcpOpt_NameServer = 5,
    DhcpOpt_DomainNameServer = 6,
    DhcpOpt_LogServer = 7,
    DhcpOpt_Cookie = 8,
    DhcpOpt_LPRServer = 9,
    DhcpOpt_ImpressServer = 10,
    DhcpOpt_ResourseLocationServer = 11,
    DhcpOpt_HostName = 12,
    DhcpOpt_BootFileSize = 13,
    DhcpOpt_MeritDumpFile = 14,
    DhcpOpt_DomainName = 15,
    DhcpOpt_SwapServer = 16,
    DhcpOpt_RootPath = 17,
    DhcpOpt_ExtensionPath = 18,
    DhcpOpt_IPForwardingEnableDisable = 19,
    DhcpOpt_NonLocalSourceRoutingEnableDisable = 20,
    DhcpOpt_PolicyFilter = 21,
    DhcpOpt_MaximumDatagramReassemblySize = 22,
    DhcpOpt_DefaultIPTime2Live = 23,
    DhcpOpt_PathMTUAgingTimeout = 24,
    DhcpOpt_IPLayerParametersPerInterface = 25,
    DhcpOpt_InterfaceMTU = 26,
    DhcpOpt_AllSubnetsAreLocal = 27,
    DhcpOpt_BroadcastAddress = 28,
    DhcpOpt_PerformMaskDiscovery = 29,
    DhcpOpt_MaskSupplier = 30,
    DhcpOpt_PerformRouteDiscovery = 31,
    DhcpOpt_RouterSolicitationAddress = 32,
    DhcpOpt_StaticRoute = 33,
    DhcpOpt_TrailerEncapsulation = 34,
    DhcpOpt_ARPCacheTimeout = 35,
    DhcpOpt_EthernetEncapsulation = 36,
    DhcpOpt_TCPDefaultTTL = 37,
    DhcpOpt_TCPKeepAliveInterval = 38,
    DhcpOpt_TCPKeepAliveGarbage = 39,
    DhcpOpt_NetworkInformationServiceDomain = 40,
    DhcpOpt_NetworkInformationServiceServers = 41,
    DhcpOpt_NetworkTimeProtocolServers = 42,
    DhcpOpt_VendorSpecificInformation = 43,
    DhcpOpt_Option_44 = 44,
    DhcpOpt_Option_45 = 45,
    DhcpOpt_Option_46 = 46,
    DhcpOpt_Option_47 = 47,
    DhcpOpt_Option_48 = 48,
    DhcpOpt_Option_49 = 49,
    DhcpOpt_IPAddressLeaseTime = 51,
    DhcpOpt_Option_64 = 64,
    DhcpOpt_Option_65 = 65,
    DhcpOpt_TFTPServerName = 66,
    DhcpOpt_BootfileName = 67,
    DhcpOpt_Option_68 = 68,
    DhcpOpt_Option_69 = 69,
    DhcpOpt_Option_70 = 70,
    DhcpOpt_Option_71 = 71,
    DhcpOpt_Option_72 = 72,
    DhcpOpt_Option_73 = 73,
    DhcpOpt_Option_74 = 74,
    DhcpOpt_Option_75 = 75,
    DhcpOpt_Option_119 = 119
} DhcpOpt;
/* End of enum DhcpOpt declaration */
#define DhcpOpt_T PRUint32


/* Start of enum VFSType declaration */
#define VFSTYPE_IID_STR "813999ba-b949-48a8-9230-aadc6285e2f2"
#define VFSTYPE_IID { \
    0x813999ba, 0xb949, 0x48a8, \
    { 0x92, 0x30, 0xaa, 0xdc, 0x62, 0x85, 0xe2, 0xf2 } \
}
typedef enum VFSType
{
    VFSType_File = 1,
    VFSType_Cloud = 2,
    VFSType_S3 = 3,
    VFSType_WebDav = 4
} VFSType;
/* End of enum VFSType declaration */
#define VFSType_T PRUint32


/* Start of enum VFSFileType declaration */
#define VFSFILETYPE_IID_STR "714333cd-44e2-415f-a245-d378fa9b1242"
#define VFSFILETYPE_IID { \
    0x714333cd, 0x44e2, 0x415f, \
    { 0xa2, 0x45, 0xd3, 0x78, 0xfa, 0x9b, 0x12, 0x42 } \
}
typedef enum VFSFileType
{
    VFSFileType_Unknown = 1,
    VFSFileType_Fifo = 2,
    VFSFileType_DevChar = 3,
    VFSFileType_Directory = 4,
    VFSFileType_DevBlock = 5,
    VFSFileType_File = 6,
    VFSFileType_SymLink = 7,
    VFSFileType_Socket = 8,
    VFSFileType_WhiteOut = 9
} VFSFileType;
/* End of enum VFSFileType declaration */
#define VFSFileType_T PRUint32


/* Start of enum ImportOptions declaration */
#define IMPORTOPTIONS_IID_STR "0a981523-3b20-4004-8ee3-dfd322202ace"
#define IMPORTOPTIONS_IID { \
    0x0a981523, 0x3b20, 0x4004, \
    { 0x8e, 0xe3, 0xdf, 0xd3, 0x22, 0x20, 0x2a, 0xce } \
}
typedef enum ImportOptions
{
    ImportOptions_KeepAllMACs = 1,
    ImportOptions_KeepNATMACs = 2
} ImportOptions;
/* End of enum ImportOptions declaration */
#define ImportOptions_T PRUint32


/* Start of enum ExportOptions declaration */
#define EXPORTOPTIONS_IID_STR "8f45eb08-fd34-41ee-af95-a880bdee5554"
#define EXPORTOPTIONS_IID { \
    0x8f45eb08, 0xfd34, 0x41ee, \
    { 0xaf, 0x95, 0xa8, 0x80, 0xbd, 0xee, 0x55, 0x54 } \
}
typedef enum ExportOptions
{
    ExportOptions_CreateManifest = 1,
    ExportOptions_ExportDVDImages = 2,
    ExportOptions_StripAllMACs = 3,
    ExportOptions_StripAllNonNATMACs = 4
} ExportOptions;
/* End of enum ExportOptions declaration */
#define ExportOptions_T PRUint32


/* Start of enum VirtualSystemDescriptionType declaration */
#define VIRTUALSYSTEMDESCRIPTIONTYPE_IID_STR "303c0900-a746-4612-8c67-79003e91f459"
#define VIRTUALSYSTEMDESCRIPTIONTYPE_IID { \
    0x303c0900, 0xa746, 0x4612, \
    { 0x8c, 0x67, 0x79, 0x00, 0x3e, 0x91, 0xf4, 0x59 } \
}
typedef enum VirtualSystemDescriptionType
{
    VirtualSystemDescriptionType_Ignore = 1,
    VirtualSystemDescriptionType_OS = 2,
    VirtualSystemDescriptionType_Name = 3,
    VirtualSystemDescriptionType_Product = 4,
    VirtualSystemDescriptionType_Vendor = 5,
    VirtualSystemDescriptionType_Version = 6,
    VirtualSystemDescriptionType_ProductUrl = 7,
    VirtualSystemDescriptionType_VendorUrl = 8,
    VirtualSystemDescriptionType_Description = 9,
    VirtualSystemDescriptionType_License = 10,
    VirtualSystemDescriptionType_Miscellaneous = 11,
    VirtualSystemDescriptionType_CPU = 12,
    VirtualSystemDescriptionType_Memory = 13,
    VirtualSystemDescriptionType_HardDiskControllerIDE = 14,
    VirtualSystemDescriptionType_HardDiskControllerSATA = 15,
    VirtualSystemDescriptionType_HardDiskControllerSCSI = 16,
    VirtualSystemDescriptionType_HardDiskControllerSAS = 17,
    VirtualSystemDescriptionType_HardDiskImage = 18,
    VirtualSystemDescriptionType_Floppy = 19,
    VirtualSystemDescriptionType_CDROM = 20,
    VirtualSystemDescriptionType_NetworkAdapter = 21,
    VirtualSystemDescriptionType_USBController = 22,
    VirtualSystemDescriptionType_SoundCard = 23,
    VirtualSystemDescriptionType_SettingsFile = 24
} VirtualSystemDescriptionType;
/* End of enum VirtualSystemDescriptionType declaration */
#define VirtualSystemDescriptionType_T PRUint32


/* Start of enum VirtualSystemDescriptionValueType declaration */
#define VIRTUALSYSTEMDESCRIPTIONVALUETYPE_IID_STR "56d9403f-3425-4118-9919-36f2a9b8c77c"
#define VIRTUALSYSTEMDESCRIPTIONVALUETYPE_IID { \
    0x56d9403f, 0x3425, 0x4118, \
    { 0x99, 0x19, 0x36, 0xf2, 0xa9, 0xb8, 0xc7, 0x7c } \
}
typedef enum VirtualSystemDescriptionValueType
{
    VirtualSystemDescriptionValueType_Reference = 1,
    VirtualSystemDescriptionValueType_Original = 2,
    VirtualSystemDescriptionValueType_Auto = 3,
    VirtualSystemDescriptionValueType_ExtraConfig = 4
} VirtualSystemDescriptionValueType;
/* End of enum VirtualSystemDescriptionValueType declaration */
#define VirtualSystemDescriptionValueType_T PRUint32


/* Start of enum GraphicsControllerType declaration */
#define GRAPHICSCONTROLLERTYPE_IID_STR "79c96ca0-9f39-4900-948e-68c41cbe127a"
#define GRAPHICSCONTROLLERTYPE_IID { \
    0x79c96ca0, 0x9f39, 0x4900, \
    { 0x94, 0x8e, 0x68, 0xc4, 0x1c, 0xbe, 0x12, 0x7a } \
}
typedef enum GraphicsControllerType
{
    GraphicsControllerType_Null = 0,
    GraphicsControllerType_VBoxVGA = 1,
    GraphicsControllerType_VMSVGA = 2
} GraphicsControllerType;
/* End of enum GraphicsControllerType declaration */
#define GraphicsControllerType_T PRUint32


/* Start of enum CleanupMode declaration */
#define CLEANUPMODE_IID_STR "67897c50-7cca-47a9-83f6-ce8fd8eb5441"
#define CLEANUPMODE_IID { \
    0x67897c50, 0x7cca, 0x47a9, \
    { 0x83, 0xf6, 0xce, 0x8f, 0xd8, 0xeb, 0x54, 0x41 } \
}
typedef enum CleanupMode
{
    CleanupMode_UnregisterOnly = 1,
    CleanupMode_DetachAllReturnNone = 2,
    CleanupMode_DetachAllReturnHardDisksOnly = 3,
    CleanupMode_Full = 4
} CleanupMode;
/* End of enum CleanupMode declaration */
#define CleanupMode_T PRUint32


/* Start of enum CloneMode declaration */
#define CLONEMODE_IID_STR "A7A159FE-5096-4B8D-8C3C-D033CB0B35A8"
#define CLONEMODE_IID { \
    0xA7A159FE, 0x5096, 0x4B8D, \
    { 0x8C, 0x3C, 0xD0, 0x33, 0xCB, 0x0B, 0x35, 0xA8 } \
}
typedef enum CloneMode
{
    CloneMode_MachineState = 1,
    CloneMode_MachineAndChildStates = 2,
    CloneMode_AllStates = 3
} CloneMode;
/* End of enum CloneMode declaration */
#define CloneMode_T PRUint32


/* Start of enum CloneOptions declaration */
#define CLONEOPTIONS_IID_STR "22243f8e-96ab-497c-8cf0-f40a566c630b"
#define CLONEOPTIONS_IID { \
    0x22243f8e, 0x96ab, 0x497c, \
    { 0x8c, 0xf0, 0xf4, 0x0a, 0x56, 0x6c, 0x63, 0x0b } \
}
typedef enum CloneOptions
{
    CloneOptions_Link = 1,
    CloneOptions_KeepAllMACs = 2,
    CloneOptions_KeepNATMACs = 3,
    CloneOptions_KeepDiskNames = 4
} CloneOptions;
/* End of enum CloneOptions declaration */
#define CloneOptions_T PRUint32


/* Start of enum AutostopType declaration */
#define AUTOSTOPTYPE_IID_STR "6bb96740-cf34-470d-aab2-2cd48ea2e10e"
#define AUTOSTOPTYPE_IID { \
    0x6bb96740, 0xcf34, 0x470d, \
    { 0xaa, 0xb2, 0x2c, 0xd4, 0x8e, 0xa2, 0xe1, 0x0e } \
}
typedef enum AutostopType
{
    AutostopType_Disabled = 1,
    AutostopType_SaveState = 2,
    AutostopType_PowerOff = 3,
    AutostopType_AcpiShutdown = 4
} AutostopType;
/* End of enum AutostopType declaration */
#define AutostopType_T PRUint32


/* Start of enum HostNetworkInterfaceMediumType declaration */
#define HOSTNETWORKINTERFACEMEDIUMTYPE_IID_STR "1aa54aaf-2497-45a2-bfb1-8eb225e93d5b"
#define HOSTNETWORKINTERFACEMEDIUMTYPE_IID { \
    0x1aa54aaf, 0x2497, 0x45a2, \
    { 0xbf, 0xb1, 0x8e, 0xb2, 0x25, 0xe9, 0x3d, 0x5b } \
}
typedef enum HostNetworkInterfaceMediumType
{
    HostNetworkInterfaceMediumType_Unknown = 0,
    HostNetworkInterfaceMediumType_Ethernet = 1,
    HostNetworkInterfaceMediumType_PPP = 2,
    HostNetworkInterfaceMediumType_SLIP = 3
} HostNetworkInterfaceMediumType;
/* End of enum HostNetworkInterfaceMediumType declaration */
#define HostNetworkInterfaceMediumType_T PRUint32


/* Start of enum HostNetworkInterfaceStatus declaration */
#define HOSTNETWORKINTERFACESTATUS_IID_STR "CC474A69-2710-434B-8D99-C38E5D5A6F41"
#define HOSTNETWORKINTERFACESTATUS_IID { \
    0xCC474A69, 0x2710, 0x434B, \
    { 0x8D, 0x99, 0xC3, 0x8E, 0x5D, 0x5A, 0x6F, 0x41 } \
}
typedef enum HostNetworkInterfaceStatus
{
    HostNetworkInterfaceStatus_Unknown = 0,
    HostNetworkInterfaceStatus_Up = 1,
    HostNetworkInterfaceStatus_Down = 2
} HostNetworkInterfaceStatus;
/* End of enum HostNetworkInterfaceStatus declaration */
#define HostNetworkInterfaceStatus_T PRUint32


/* Start of enum HostNetworkInterfaceType declaration */
#define HOSTNETWORKINTERFACETYPE_IID_STR "67431b00-9946-48a2-bc02-b25c5919f4f3"
#define HOSTNETWORKINTERFACETYPE_IID { \
    0x67431b00, 0x9946, 0x48a2, \
    { 0xbc, 0x02, 0xb2, 0x5c, 0x59, 0x19, 0xf4, 0xf3 } \
}
typedef enum HostNetworkInterfaceType
{
    HostNetworkInterfaceType_Bridged = 1,
    HostNetworkInterfaceType_HostOnly = 2
} HostNetworkInterfaceType;
/* End of enum HostNetworkInterfaceType declaration */
#define HostNetworkInterfaceType_T PRUint32


/* Start of enum AdditionsFacilityType declaration */
#define ADDITIONSFACILITYTYPE_IID_STR "98f7f957-89fb-49b6-a3b1-31e3285eb1d8"
#define ADDITIONSFACILITYTYPE_IID { \
    0x98f7f957, 0x89fb, 0x49b6, \
    { 0xa3, 0xb1, 0x31, 0xe3, 0x28, 0x5e, 0xb1, 0xd8 } \
}
typedef enum AdditionsFacilityType
{
    AdditionsFacilityType_None = 0,
    AdditionsFacilityType_VBoxGuestDriver = 20,
    AdditionsFacilityType_AutoLogon = 90,
    AdditionsFacilityType_VBoxService = 100,
    AdditionsFacilityType_VBoxTrayClient = 101,
    AdditionsFacilityType_Seamless = 1000,
    AdditionsFacilityType_Graphics = 1100,
    AdditionsFacilityType_All = 2147483646
} AdditionsFacilityType;
/* End of enum AdditionsFacilityType declaration */
#define AdditionsFacilityType_T PRUint32


/* Start of enum AdditionsFacilityClass declaration */
#define ADDITIONSFACILITYCLASS_IID_STR "446451b2-c88d-4e5d-84c9-91bc7f533f5f"
#define ADDITIONSFACILITYCLASS_IID { \
    0x446451b2, 0xc88d, 0x4e5d, \
    { 0x84, 0xc9, 0x91, 0xbc, 0x7f, 0x53, 0x3f, 0x5f } \
}
typedef enum AdditionsFacilityClass
{
    AdditionsFacilityClass_None = 0,
    AdditionsFacilityClass_Driver = 10,
    AdditionsFacilityClass_Service = 30,
    AdditionsFacilityClass_Program = 50,
    AdditionsFacilityClass_Feature = 100,
    AdditionsFacilityClass_ThirdParty = 999,
    AdditionsFacilityClass_All = 2147483646
} AdditionsFacilityClass;
/* End of enum AdditionsFacilityClass declaration */
#define AdditionsFacilityClass_T PRUint32


/* Start of enum AdditionsFacilityStatus declaration */
#define ADDITIONSFACILITYSTATUS_IID_STR "ce06f9e1-394e-4fe9-9368-5a88c567dbde"
#define ADDITIONSFACILITYSTATUS_IID { \
    0xce06f9e1, 0x394e, 0x4fe9, \
    { 0x93, 0x68, 0x5a, 0x88, 0xc5, 0x67, 0xdb, 0xde } \
}
typedef enum AdditionsFacilityStatus
{
    AdditionsFacilityStatus_Inactive = 0,
    AdditionsFacilityStatus_Paused = 1,
    AdditionsFacilityStatus_PreInit = 20,
    AdditionsFacilityStatus_Init = 30,
    AdditionsFacilityStatus_Active = 50,
    AdditionsFacilityStatus_Terminating = 100,
    AdditionsFacilityStatus_Terminated = 101,
    AdditionsFacilityStatus_Failed = 800,
    AdditionsFacilityStatus_Unknown = 999
} AdditionsFacilityStatus;
/* End of enum AdditionsFacilityStatus declaration */
#define AdditionsFacilityStatus_T PRUint32


/* Start of enum AdditionsRunLevelType declaration */
#define ADDITIONSRUNLEVELTYPE_IID_STR "a25417ee-a9dd-4f5b-b0dc-377860087754"
#define ADDITIONSRUNLEVELTYPE_IID { \
    0xa25417ee, 0xa9dd, 0x4f5b, \
    { 0xb0, 0xdc, 0x37, 0x78, 0x60, 0x08, 0x77, 0x54 } \
}
typedef enum AdditionsRunLevelType
{
    AdditionsRunLevelType_None = 0,
    AdditionsRunLevelType_System = 1,
    AdditionsRunLevelType_Userland = 2,
    AdditionsRunLevelType_Desktop = 3
} AdditionsRunLevelType;
/* End of enum AdditionsRunLevelType declaration */
#define AdditionsRunLevelType_T PRUint32


/* Start of enum AdditionsUpdateFlag declaration */
#define ADDITIONSUPDATEFLAG_IID_STR "726a818d-18d6-4389-94e8-3e9e6826171a"
#define ADDITIONSUPDATEFLAG_IID { \
    0x726a818d, 0x18d6, 0x4389, \
    { 0x94, 0xe8, 0x3e, 0x9e, 0x68, 0x26, 0x17, 0x1a } \
}
typedef enum AdditionsUpdateFlag
{
    AdditionsUpdateFlag_None = 0,
    AdditionsUpdateFlag_WaitForUpdateStartOnly = 1
} AdditionsUpdateFlag;
/* End of enum AdditionsUpdateFlag declaration */
#define AdditionsUpdateFlag_T PRUint32


/* Start of enum GuestSessionStatus declaration */
#define GUESTSESSIONSTATUS_IID_STR "ac2669da-4624-44f2-85b5-0b0bfb8d8673"
#define GUESTSESSIONSTATUS_IID { \
    0xac2669da, 0x4624, 0x44f2, \
    { 0x85, 0xb5, 0x0b, 0x0b, 0xfb, 0x8d, 0x86, 0x73 } \
}
typedef enum GuestSessionStatus
{
    GuestSessionStatus_Undefined = 0,
    GuestSessionStatus_Starting = 10,
    GuestSessionStatus_Started = 100,
    GuestSessionStatus_Terminating = 480,
    GuestSessionStatus_Terminated = 500,
    GuestSessionStatus_TimedOutKilled = 512,
    GuestSessionStatus_TimedOutAbnormally = 513,
    GuestSessionStatus_Down = 600,
    GuestSessionStatus_Error = 800
} GuestSessionStatus;
/* End of enum GuestSessionStatus declaration */
#define GuestSessionStatus_T PRUint32


/* Start of enum GuestSessionWaitForFlag declaration */
#define GUESTSESSIONWAITFORFLAG_IID_STR "bb7a372a-f635-4e11-a81a-e707f3a52ef5"
#define GUESTSESSIONWAITFORFLAG_IID { \
    0xbb7a372a, 0xf635, 0x4e11, \
    { 0xa8, 0x1a, 0xe7, 0x07, 0xf3, 0xa5, 0x2e, 0xf5 } \
}
typedef enum GuestSessionWaitForFlag
{
    GuestSessionWaitForFlag_None = 0,
    GuestSessionWaitForFlag_Start = 1,
    GuestSessionWaitForFlag_Terminate = 2,
    GuestSessionWaitForFlag_Status = 4
} GuestSessionWaitForFlag;
/* End of enum GuestSessionWaitForFlag declaration */
#define GuestSessionWaitForFlag_T PRUint32


/* Start of enum GuestSessionWaitResult declaration */
#define GUESTSESSIONWAITRESULT_IID_STR "c0f6a8a5-fdb6-42bf-a582-56c6f82bcd2d"
#define GUESTSESSIONWAITRESULT_IID { \
    0xc0f6a8a5, 0xfdb6, 0x42bf, \
    { 0xa5, 0x82, 0x56, 0xc6, 0xf8, 0x2b, 0xcd, 0x2d } \
}
typedef enum GuestSessionWaitResult
{
    GuestSessionWaitResult_None = 0,
    GuestSessionWaitResult_Start = 1,
    GuestSessionWaitResult_Terminate = 2,
    GuestSessionWaitResult_Status = 3,
    GuestSessionWaitResult_Error = 4,
    GuestSessionWaitResult_Timeout = 5,
    GuestSessionWaitResult_WaitFlagNotSupported = 6
} GuestSessionWaitResult;
/* End of enum GuestSessionWaitResult declaration */
#define GuestSessionWaitResult_T PRUint32


/* Start of enum GuestUserState declaration */
#define GUESTUSERSTATE_IID_STR "b2a82b02-fd3d-4fc2-ba84-6ba5ac8be198"
#define GUESTUSERSTATE_IID { \
    0xb2a82b02, 0xfd3d, 0x4fc2, \
    { 0xba, 0x84, 0x6b, 0xa5, 0xac, 0x8b, 0xe1, 0x98 } \
}
typedef enum GuestUserState
{
    GuestUserState_Unknown = 0,
    GuestUserState_LoggedIn = 1,
    GuestUserState_LoggedOut = 2,
    GuestUserState_Locked = 3,
    GuestUserState_Unlocked = 4,
    GuestUserState_Disabled = 5,
    GuestUserState_Idle = 6,
    GuestUserState_InUse = 7,
    GuestUserState_Created = 8,
    GuestUserState_Deleted = 9,
    GuestUserState_SessionChanged = 10,
    GuestUserState_CredentialsChanged = 11,
    GuestUserState_RoleChanged = 12,
    GuestUserState_GroupAdded = 13,
    GuestUserState_GroupRemoved = 14,
    GuestUserState_Elevated = 15
} GuestUserState;
/* End of enum GuestUserState declaration */
#define GuestUserState_T PRUint32


/* Start of enum FileSeekType declaration */
#define FILESEEKTYPE_IID_STR "1b73f4f3-3515-4073-a506-76878d9e2541"
#define FILESEEKTYPE_IID { \
    0x1b73f4f3, 0x3515, 0x4073, \
    { 0xa5, 0x06, 0x76, 0x87, 0x8d, 0x9e, 0x25, 0x41 } \
}
typedef enum FileSeekType
{
    FileSeekType_Set = 0,
    FileSeekType_Current = 1
} FileSeekType;
/* End of enum FileSeekType declaration */
#define FileSeekType_T PRUint32


/* Start of enum ProcessInputFlag declaration */
#define PROCESSINPUTFLAG_IID_STR "5d38c1dd-2604-4ddf-92e5-0c0cdd3bdbd5"
#define PROCESSINPUTFLAG_IID { \
    0x5d38c1dd, 0x2604, 0x4ddf, \
    { 0x92, 0xe5, 0x0c, 0x0c, 0xdd, 0x3b, 0xdb, 0xd5 } \
}
typedef enum ProcessInputFlag
{
    ProcessInputFlag_None = 0,
    ProcessInputFlag_EndOfFile = 1
} ProcessInputFlag;
/* End of enum ProcessInputFlag declaration */
#define ProcessInputFlag_T PRUint32


/* Start of enum ProcessOutputFlag declaration */
#define PROCESSOUTPUTFLAG_IID_STR "9979e85a-52bb-40b7-870c-57115e27e0f1"
#define PROCESSOUTPUTFLAG_IID { \
    0x9979e85a, 0x52bb, 0x40b7, \
    { 0x87, 0x0c, 0x57, 0x11, 0x5e, 0x27, 0xe0, 0xf1 } \
}
typedef enum ProcessOutputFlag
{
    ProcessOutputFlag_None = 0,
    ProcessOutputFlag_StdErr = 1
} ProcessOutputFlag;
/* End of enum ProcessOutputFlag declaration */
#define ProcessOutputFlag_T PRUint32


/* Start of enum ProcessWaitForFlag declaration */
#define PROCESSWAITFORFLAG_IID_STR "23b550c7-78e1-437e-98f0-65fd9757bcd2"
#define PROCESSWAITFORFLAG_IID { \
    0x23b550c7, 0x78e1, 0x437e, \
    { 0x98, 0xf0, 0x65, 0xfd, 0x97, 0x57, 0xbc, 0xd2 } \
}
typedef enum ProcessWaitForFlag
{
    ProcessWaitForFlag_None = 0,
    ProcessWaitForFlag_Start = 1,
    ProcessWaitForFlag_Terminate = 2,
    ProcessWaitForFlag_StdIn = 4,
    ProcessWaitForFlag_StdOut = 8,
    ProcessWaitForFlag_StdErr = 16
} ProcessWaitForFlag;
/* End of enum ProcessWaitForFlag declaration */
#define ProcessWaitForFlag_T PRUint32


/* Start of enum ProcessWaitResult declaration */
#define PROCESSWAITRESULT_IID_STR "40719cbe-f192-4fe9-a231-6697b3c8e2b4"
#define PROCESSWAITRESULT_IID { \
    0x40719cbe, 0xf192, 0x4fe9, \
    { 0xa2, 0x31, 0x66, 0x97, 0xb3, 0xc8, 0xe2, 0xb4 } \
}
typedef enum ProcessWaitResult
{
    ProcessWaitResult_None = 0,
    ProcessWaitResult_Start = 1,
    ProcessWaitResult_Terminate = 2,
    ProcessWaitResult_Status = 3,
    ProcessWaitResult_Error = 4,
    ProcessWaitResult_Timeout = 5,
    ProcessWaitResult_StdIn = 6,
    ProcessWaitResult_StdOut = 7,
    ProcessWaitResult_StdErr = 8,
    ProcessWaitResult_WaitFlagNotSupported = 9
} ProcessWaitResult;
/* End of enum ProcessWaitResult declaration */
#define ProcessWaitResult_T PRUint32


/* Start of enum CopyFileFlag declaration */
#define COPYFILEFLAG_IID_STR "23f79fdf-738a-493d-b80b-42d607c9b916"
#define COPYFILEFLAG_IID { \
    0x23f79fdf, 0x738a, 0x493d, \
    { 0xb8, 0x0b, 0x42, 0xd6, 0x07, 0xc9, 0xb9, 0x16 } \
}
typedef enum CopyFileFlag
{
    CopyFileFlag_None = 0,
    CopyFileFlag_Recursive = 1,
    CopyFileFlag_Update = 2,
    CopyFileFlag_FollowLinks = 4
} CopyFileFlag;
/* End of enum CopyFileFlag declaration */
#define CopyFileFlag_T PRUint32


/* Start of enum DirectoryCreateFlag declaration */
#define DIRECTORYCREATEFLAG_IID_STR "bd721b0e-ced5-4f79-b368-249897c32a36"
#define DIRECTORYCREATEFLAG_IID { \
    0xbd721b0e, 0xced5, 0x4f79, \
    { 0xb3, 0x68, 0x24, 0x98, 0x97, 0xc3, 0x2a, 0x36 } \
}
typedef enum DirectoryCreateFlag
{
    DirectoryCreateFlag_None = 0,
    DirectoryCreateFlag_Parents = 1
} DirectoryCreateFlag;
/* End of enum DirectoryCreateFlag declaration */
#define DirectoryCreateFlag_T PRUint32


/* Start of enum DirectoryRemoveRecFlag declaration */
#define DIRECTORYREMOVERECFLAG_IID_STR "455aabf0-7692-48f6-9061-f21579b65769"
#define DIRECTORYREMOVERECFLAG_IID { \
    0x455aabf0, 0x7692, 0x48f6, \
    { 0x90, 0x61, 0xf2, 0x15, 0x79, 0xb6, 0x57, 0x69 } \
}
typedef enum DirectoryRemoveRecFlag
{
    DirectoryRemoveRecFlag_None = 0,
    DirectoryRemoveRecFlag_ContentAndDir = 1,
    DirectoryRemoveRecFlag_ContentOnly = 2
} DirectoryRemoveRecFlag;
/* End of enum DirectoryRemoveRecFlag declaration */
#define DirectoryRemoveRecFlag_T PRUint32


/* Start of enum PathRenameFlag declaration */
#define PATHRENAMEFLAG_IID_STR "f3baa09f-c758-453d-b91c-c7787d76351d"
#define PATHRENAMEFLAG_IID { \
    0xf3baa09f, 0xc758, 0x453d, \
    { 0xb9, 0x1c, 0xc7, 0x78, 0x7d, 0x76, 0x35, 0x1d } \
}
typedef enum PathRenameFlag
{
    PathRenameFlag_None = 0,
    PathRenameFlag_NoReplace = 1,
    PathRenameFlag_Replace = 2,
    PathRenameFlag_NoSymlinks = 4
} PathRenameFlag;
/* End of enum PathRenameFlag declaration */
#define PathRenameFlag_T PRUint32


/* Start of enum ProcessCreateFlag declaration */
#define PROCESSCREATEFLAG_IID_STR "35192799-bfde-405d-9bea-c735ab9998e4"
#define PROCESSCREATEFLAG_IID { \
    0x35192799, 0xbfde, 0x405d, \
    { 0x9b, 0xea, 0xc7, 0x35, 0xab, 0x99, 0x98, 0xe4 } \
}
typedef enum ProcessCreateFlag
{
    ProcessCreateFlag_None = 0,
    ProcessCreateFlag_WaitForProcessStartOnly = 1,
    ProcessCreateFlag_IgnoreOrphanedProcesses = 2,
    ProcessCreateFlag_Hidden = 4,
    ProcessCreateFlag_NoProfile = 8,
    ProcessCreateFlag_WaitForStdOut = 16,
    ProcessCreateFlag_WaitForStdErr = 32,
    ProcessCreateFlag_ExpandArguments = 64
} ProcessCreateFlag;
/* End of enum ProcessCreateFlag declaration */
#define ProcessCreateFlag_T PRUint32


/* Start of enum ProcessPriority declaration */
#define PROCESSPRIORITY_IID_STR "ee8cac50-e232-49fe-806b-d1214d9c2e49"
#define PROCESSPRIORITY_IID { \
    0xee8cac50, 0xe232, 0x49fe, \
    { 0x80, 0x6b, 0xd1, 0x21, 0x4d, 0x9c, 0x2e, 0x49 } \
}
typedef enum ProcessPriority
{
    ProcessPriority_Invalid = 0,
    ProcessPriority_Default = 1
} ProcessPriority;
/* End of enum ProcessPriority declaration */
#define ProcessPriority_T PRUint32


/* Start of enum SymlinkType declaration */
#define SYMLINKTYPE_IID_STR "37794668-f8f1-4714-98a5-6f8fa2ed0118"
#define SYMLINKTYPE_IID { \
    0x37794668, 0xf8f1, 0x4714, \
    { 0x98, 0xa5, 0x6f, 0x8f, 0xa2, 0xed, 0x01, 0x18 } \
}
typedef enum SymlinkType
{
    SymlinkType_Unknown = 0,
    SymlinkType_Directory = 1,
    SymlinkType_File = 2
} SymlinkType;
/* End of enum SymlinkType declaration */
#define SymlinkType_T PRUint32


/* Start of enum SymlinkReadFlag declaration */
#define SYMLINKREADFLAG_IID_STR "b7fe2b9d-790e-4b25-8adf-1ca33026931f"
#define SYMLINKREADFLAG_IID { \
    0xb7fe2b9d, 0x790e, 0x4b25, \
    { 0x8a, 0xdf, 0x1c, 0xa3, 0x30, 0x26, 0x93, 0x1f } \
}
typedef enum SymlinkReadFlag
{
    SymlinkReadFlag_None = 0,
    SymlinkReadFlag_NoSymlinks = 1
} SymlinkReadFlag;
/* End of enum SymlinkReadFlag declaration */
#define SymlinkReadFlag_T PRUint32


/* Start of enum ProcessStatus declaration */
#define PROCESSSTATUS_IID_STR "4d52368f-5b48-4bfe-b486-acf89139b52f"
#define PROCESSSTATUS_IID { \
    0x4d52368f, 0x5b48, 0x4bfe, \
    { 0xb4, 0x86, 0xac, 0xf8, 0x91, 0x39, 0xb5, 0x2f } \
}
typedef enum ProcessStatus
{
    ProcessStatus_Undefined = 0,
    ProcessStatus_Starting = 10,
    ProcessStatus_Started = 100,
    ProcessStatus_Paused = 110,
    ProcessStatus_Terminating = 480,
    ProcessStatus_TerminatedNormally = 500,
    ProcessStatus_TerminatedSignal = 510,
    ProcessStatus_TerminatedAbnormally = 511,
    ProcessStatus_TimedOutKilled = 512,
    ProcessStatus_TimedOutAbnormally = 513,
    ProcessStatus_Down = 600,
    ProcessStatus_Error = 800
} ProcessStatus;
/* End of enum ProcessStatus declaration */
#define ProcessStatus_T PRUint32


/* Start of enum ProcessInputStatus declaration */
#define PROCESSINPUTSTATUS_IID_STR "a4a0ef9c-29cc-4805-9803-c8215ae9da6c"
#define PROCESSINPUTSTATUS_IID { \
    0xa4a0ef9c, 0x29cc, 0x4805, \
    { 0x98, 0x03, 0xc8, 0x21, 0x5a, 0xe9, 0xda, 0x6c } \
}
typedef enum ProcessInputStatus
{
    ProcessInputStatus_Undefined = 0,
    ProcessInputStatus_Broken = 1,
    ProcessInputStatus_Available = 10,
    ProcessInputStatus_Written = 50,
    ProcessInputStatus_Overflow = 100
} ProcessInputStatus;
/* End of enum ProcessInputStatus declaration */
#define ProcessInputStatus_T PRUint32


/* Start of enum FileStatus declaration */
#define FILESTATUS_IID_STR "8c86468b-b97b-4080-8914-e29f5b0abd2c"
#define FILESTATUS_IID { \
    0x8c86468b, 0xb97b, 0x4080, \
    { 0x89, 0x14, 0xe2, 0x9f, 0x5b, 0x0a, 0xbd, 0x2c } \
}
typedef enum FileStatus
{
    FileStatus_Undefined = 0,
    FileStatus_Opening = 10,
    FileStatus_Open = 100,
    FileStatus_Closing = 150,
    FileStatus_Closed = 200,
    FileStatus_Down = 600,
    FileStatus_Error = 800
} FileStatus;
/* End of enum FileStatus declaration */
#define FileStatus_T PRUint32


/* Start of enum FsObjType declaration */
#define FSOBJTYPE_IID_STR "a1ed437c-b3c3-4ca2-b19c-4239d658d5e8"
#define FSOBJTYPE_IID { \
    0xa1ed437c, 0xb3c3, 0x4ca2, \
    { 0xb1, 0x9c, 0x42, 0x39, 0xd6, 0x58, 0xd5, 0xe8 } \
}
typedef enum FsObjType
{
    FsObjType_Undefined = 0,
    FsObjType_FIFO = 1,
    FsObjType_DevChar = 10,
    FsObjType_DevBlock = 11,
    FsObjType_Directory = 50,
    FsObjType_File = 80,
    FsObjType_Symlink = 100,
    FsObjType_Socket = 200,
    FsObjType_Whiteout = 400
} FsObjType;
/* End of enum FsObjType declaration */
#define FsObjType_T PRUint32


/* Start of enum DragAndDropAction declaration */
#define DRAGANDDROPACTION_IID_STR "47f3b162-c107-4fcd-bfa7-54b8135c441e"
#define DRAGANDDROPACTION_IID { \
    0x47f3b162, 0xc107, 0x4fcd, \
    { 0xbf, 0xa7, 0x54, 0xb8, 0x13, 0x5c, 0x44, 0x1e } \
}
typedef enum DragAndDropAction
{
    DragAndDropAction_Ignore = 0,
    DragAndDropAction_Copy = 1,
    DragAndDropAction_Move = 2,
    DragAndDropAction_Link = 3
} DragAndDropAction;
/* End of enum DragAndDropAction declaration */
#define DragAndDropAction_T PRUint32


/* Start of enum DirectoryOpenFlag declaration */
#define DIRECTORYOPENFLAG_IID_STR "5138837a-8fd2-4194-a1b0-08f7bc3949d0"
#define DIRECTORYOPENFLAG_IID { \
    0x5138837a, 0x8fd2, 0x4194, \
    { 0xa1, 0xb0, 0x08, 0xf7, 0xbc, 0x39, 0x49, 0xd0 } \
}
typedef enum DirectoryOpenFlag
{
    DirectoryOpenFlag_None = 0,
    DirectoryOpenFlag_NoSymlinks = 1
} DirectoryOpenFlag;
/* End of enum DirectoryOpenFlag declaration */
#define DirectoryOpenFlag_T PRUint32


/* Start of enum MediumState declaration */
#define MEDIUMSTATE_IID_STR "ef41e980-e012-43cd-9dea-479d4ef14d13"
#define MEDIUMSTATE_IID { \
    0xef41e980, 0xe012, 0x43cd, \
    { 0x9d, 0xea, 0x47, 0x9d, 0x4e, 0xf1, 0x4d, 0x13 } \
}
typedef enum MediumState
{
    MediumState_NotCreated = 0,
    MediumState_Created = 1,
    MediumState_LockedRead = 2,
    MediumState_LockedWrite = 3,
    MediumState_Inaccessible = 4,
    MediumState_Creating = 5,
    MediumState_Deleting = 6
} MediumState;
/* End of enum MediumState declaration */
#define MediumState_T PRUint32


/* Start of enum MediumType declaration */
#define MEDIUMTYPE_IID_STR "fe663fb5-c244-4e1b-9d81-c628b417dd04"
#define MEDIUMTYPE_IID { \
    0xfe663fb5, 0xc244, 0x4e1b, \
    { 0x9d, 0x81, 0xc6, 0x28, 0xb4, 0x17, 0xdd, 0x04 } \
}
typedef enum MediumType
{
    MediumType_Normal = 0,
    MediumType_Immutable = 1,
    MediumType_Writethrough = 2,
    MediumType_Shareable = 3,
    MediumType_Readonly = 4,
    MediumType_MultiAttach = 5
} MediumType;
/* End of enum MediumType declaration */
#define MediumType_T PRUint32


/* Start of enum MediumVariant declaration */
#define MEDIUMVARIANT_IID_STR "80685b6b-e42f-497d-8271-e77bf3c61ada"
#define MEDIUMVARIANT_IID { \
    0x80685b6b, 0xe42f, 0x497d, \
    { 0x82, 0x71, 0xe7, 0x7b, 0xf3, 0xc6, 0x1a, 0xda } \
}
typedef enum MediumVariant
{
    MediumVariant_Standard = 0,
    MediumVariant_VmdkSplit2G = 0x01,
    MediumVariant_VmdkRawDisk = 0x02,
    MediumVariant_VmdkStreamOptimized = 0x04,
    MediumVariant_VmdkESX = 0x08,
    MediumVariant_Fixed = 0x10000,
    MediumVariant_Diff = 0x20000,
    MediumVariant_NoCreateDir = 0x40000000
} MediumVariant;
/* End of enum MediumVariant declaration */
#define MediumVariant_T PRUint32


/* Start of enum DataType declaration */
#define DATATYPE_IID_STR "d90ea51e-a3f1-4a01-beb1-c1723c0d3ba7"
#define DATATYPE_IID { \
    0xd90ea51e, 0xa3f1, 0x4a01, \
    { 0xbe, 0xb1, 0xc1, 0x72, 0x3c, 0x0d, 0x3b, 0xa7 } \
}
typedef enum DataType
{
    DataType_Int32 = 0,
    DataType_Int8 = 1,
    DataType_String = 2
} DataType;
/* End of enum DataType declaration */
#define DataType_T PRUint32


/* Start of enum DataFlags declaration */
#define DATAFLAGS_IID_STR "86884dcf-1d6b-4f1b-b4bf-f5aa44959d60"
#define DATAFLAGS_IID { \
    0x86884dcf, 0x1d6b, 0x4f1b, \
    { 0xb4, 0xbf, 0xf5, 0xaa, 0x44, 0x95, 0x9d, 0x60 } \
}
typedef enum DataFlags
{
    DataFlags_None = 0x00,
    DataFlags_Mandatory = 0x01,
    DataFlags_Expert = 0x02,
    DataFlags_Array = 0x04,
    DataFlags_FlagMask = 0x07
} DataFlags;
/* End of enum DataFlags declaration */
#define DataFlags_T PRUint32


/* Start of enum MediumFormatCapabilities declaration */
#define MEDIUMFORMATCAPABILITIES_IID_STR "7342ba79-7ce0-4d94-8f86-5ed5a185d9bd"
#define MEDIUMFORMATCAPABILITIES_IID { \
    0x7342ba79, 0x7ce0, 0x4d94, \
    { 0x8f, 0x86, 0x5e, 0xd5, 0xa1, 0x85, 0xd9, 0xbd } \
}
typedef enum MediumFormatCapabilities
{
    MediumFormatCapabilities_Uuid = 0x01,
    MediumFormatCapabilities_CreateFixed = 0x02,
    MediumFormatCapabilities_CreateDynamic = 0x04,
    MediumFormatCapabilities_CreateSplit2G = 0x08,
    MediumFormatCapabilities_Differencing = 0x10,
    MediumFormatCapabilities_Asynchronous = 0x20,
    MediumFormatCapabilities_File = 0x40,
    MediumFormatCapabilities_Properties = 0x80,
    MediumFormatCapabilities_TcpNetworking = 0x100,
    MediumFormatCapabilities_VFS = 0x200,
    MediumFormatCapabilities_CapabilityMask = 0x3FF
} MediumFormatCapabilities;
/* End of enum MediumFormatCapabilities declaration */
#define MediumFormatCapabilities_T PRUint32


/* Start of enum MouseButtonState declaration */
#define MOUSEBUTTONSTATE_IID_STR "9ee094b8-b28a-4d56-a166-973cb588d7f8"
#define MOUSEBUTTONSTATE_IID { \
    0x9ee094b8, 0xb28a, 0x4d56, \
    { 0xa1, 0x66, 0x97, 0x3c, 0xb5, 0x88, 0xd7, 0xf8 } \
}
typedef enum MouseButtonState
{
    MouseButtonState_LeftButton = 0x01,
    MouseButtonState_RightButton = 0x02,
    MouseButtonState_MiddleButton = 0x04,
    MouseButtonState_WheelUp = 0x08,
    MouseButtonState_WheelDown = 0x10,
    MouseButtonState_XButton1 = 0x20,
    MouseButtonState_XButton2 = 0x40,
    MouseButtonState_MouseStateMask = 0x7F
} MouseButtonState;
/* End of enum MouseButtonState declaration */
#define MouseButtonState_T PRUint32


/* Start of enum TouchContactState declaration */
#define TOUCHCONTACTSTATE_IID_STR "3f942686-2506-421c-927c-90d4b45f4a38"
#define TOUCHCONTACTSTATE_IID { \
    0x3f942686, 0x2506, 0x421c, \
    { 0x92, 0x7c, 0x90, 0xd4, 0xb4, 0x5f, 0x4a, 0x38 } \
}
typedef enum TouchContactState
{
    TouchContactState_None = 0x00,
    TouchContactState_InContact = 0x01,
    TouchContactState_InRange = 0x02,
    TouchContactState_ContactStateMask = 0x03
} TouchContactState;
/* End of enum TouchContactState declaration */
#define TouchContactState_T PRUint32


/* Start of enum FramebufferPixelFormat declaration */
#define FRAMEBUFFERPIXELFORMAT_IID_STR "7acfd5ed-29e3-45e3-8136-73c9224f3d2d"
#define FRAMEBUFFERPIXELFORMAT_IID { \
    0x7acfd5ed, 0x29e3, 0x45e3, \
    { 0x81, 0x36, 0x73, 0xc9, 0x22, 0x4f, 0x3d, 0x2d } \
}
typedef enum FramebufferPixelFormat
{
    FramebufferPixelFormat_Opaque = 0,
    FramebufferPixelFormat_FOURCC_RGB = 0x32424752
} FramebufferPixelFormat;
/* End of enum FramebufferPixelFormat declaration */
#define FramebufferPixelFormat_T PRUint32


/* Start of enum NetworkAttachmentType declaration */
#define NETWORKATTACHMENTTYPE_IID_STR "524a8f9d-4b86-4b51-877d-1aa27c4ebeac"
#define NETWORKATTACHMENTTYPE_IID { \
    0x524a8f9d, 0x4b86, 0x4b51, \
    { 0x87, 0x7d, 0x1a, 0xa2, 0x7c, 0x4e, 0xbe, 0xac } \
}
typedef enum NetworkAttachmentType
{
    NetworkAttachmentType_Null = 0,
    NetworkAttachmentType_NAT = 1,
    NetworkAttachmentType_Bridged = 2,
    NetworkAttachmentType_Internal = 3,
    NetworkAttachmentType_HostOnly = 4,
    NetworkAttachmentType_Generic = 5,
    NetworkAttachmentType_NATNetwork = 6
} NetworkAttachmentType;
/* End of enum NetworkAttachmentType declaration */
#define NetworkAttachmentType_T PRUint32


/* Start of enum NetworkAdapterType declaration */
#define NETWORKADAPTERTYPE_IID_STR "3c2281e4-d952-4e87-8c7d-24379cb6a81c"
#define NETWORKADAPTERTYPE_IID { \
    0x3c2281e4, 0xd952, 0x4e87, \
    { 0x8c, 0x7d, 0x24, 0x37, 0x9c, 0xb6, 0xa8, 0x1c } \
}
typedef enum NetworkAdapterType
{
    NetworkAdapterType_Null = 0,
    NetworkAdapterType_Am79C970A = 1,
    NetworkAdapterType_Am79C973 = 2,
    NetworkAdapterType_I82540EM = 3,
    NetworkAdapterType_I82543GC = 4,
    NetworkAdapterType_I82545EM = 5,
    NetworkAdapterType_Virtio = 6
} NetworkAdapterType;
/* End of enum NetworkAdapterType declaration */
#define NetworkAdapterType_T PRUint32


/* Start of enum NetworkAdapterPromiscModePolicy declaration */
#define NETWORKADAPTERPROMISCMODEPOLICY_IID_STR "c963768a-376f-4c85-8d84-d8ced4b7269e"
#define NETWORKADAPTERPROMISCMODEPOLICY_IID { \
    0xc963768a, 0x376f, 0x4c85, \
    { 0x8d, 0x84, 0xd8, 0xce, 0xd4, 0xb7, 0x26, 0x9e } \
}
typedef enum NetworkAdapterPromiscModePolicy
{
    NetworkAdapterPromiscModePolicy_Deny = 1,
    NetworkAdapterPromiscModePolicy_AllowNetwork = 2,
    NetworkAdapterPromiscModePolicy_AllowAll = 3
} NetworkAdapterPromiscModePolicy;
/* End of enum NetworkAdapterPromiscModePolicy declaration */
#define NetworkAdapterPromiscModePolicy_T PRUint32


/* Start of enum PortMode declaration */
#define PORTMODE_IID_STR "533b5fe3-0185-4197-86a7-17e37dd39d76"
#define PORTMODE_IID { \
    0x533b5fe3, 0x0185, 0x4197, \
    { 0x86, 0xa7, 0x17, 0xe3, 0x7d, 0xd3, 0x9d, 0x76 } \
}
typedef enum PortMode
{
    PortMode_Disconnected = 0,
    PortMode_HostPipe = 1,
    PortMode_HostDevice = 2,
    PortMode_RawFile = 3
} PortMode;
/* End of enum PortMode declaration */
#define PortMode_T PRUint32


/* Start of enum USBControllerType declaration */
#define USBCONTROLLERTYPE_IID_STR "8fdd1c6a-5412-41da-ab07-7baed7d6e18e"
#define USBCONTROLLERTYPE_IID { \
    0x8fdd1c6a, 0x5412, 0x41da, \
    { 0xab, 0x07, 0x7b, 0xae, 0xd7, 0xd6, 0xe1, 0x8e } \
}
typedef enum USBControllerType
{
    USBControllerType_Null = 0,
    USBControllerType_OHCI = 1,
    USBControllerType_EHCI = 2,
    USBControllerType_Last = 3
} USBControllerType;
/* End of enum USBControllerType declaration */
#define USBControllerType_T PRUint32


/* Start of enum USBDeviceState declaration */
#define USBDEVICESTATE_IID_STR "b99a2e65-67fb-4882-82fd-f3e5e8193ab4"
#define USBDEVICESTATE_IID { \
    0xb99a2e65, 0x67fb, 0x4882, \
    { 0x82, 0xfd, 0xf3, 0xe5, 0xe8, 0x19, 0x3a, 0xb4 } \
}
typedef enum USBDeviceState
{
    USBDeviceState_NotSupported = 0,
    USBDeviceState_Unavailable = 1,
    USBDeviceState_Busy = 2,
    USBDeviceState_Available = 3,
    USBDeviceState_Held = 4,
    USBDeviceState_Captured = 5
} USBDeviceState;
/* End of enum USBDeviceState declaration */
#define USBDeviceState_T PRUint32


/* Start of enum USBDeviceFilterAction declaration */
#define USBDEVICEFILTERACTION_IID_STR "cbc30a49-2f4e-43b5-9da6-121320475933"
#define USBDEVICEFILTERACTION_IID { \
    0xcbc30a49, 0x2f4e, 0x43b5, \
    { 0x9d, 0xa6, 0x12, 0x13, 0x20, 0x47, 0x59, 0x33 } \
}
typedef enum USBDeviceFilterAction
{
    USBDeviceFilterAction_Null = 0,
    USBDeviceFilterAction_Ignore = 1,
    USBDeviceFilterAction_Hold = 2
} USBDeviceFilterAction;
/* End of enum USBDeviceFilterAction declaration */
#define USBDeviceFilterAction_T PRUint32


/* Start of enum AudioDriverType declaration */
#define AUDIODRIVERTYPE_IID_STR "4bcc3d73-c2fe-40db-b72f-0c2ca9d68496"
#define AUDIODRIVERTYPE_IID { \
    0x4bcc3d73, 0xc2fe, 0x40db, \
    { 0xb7, 0x2f, 0x0c, 0x2c, 0xa9, 0xd6, 0x84, 0x96 } \
}
typedef enum AudioDriverType
{
    AudioDriverType_Null = 0,
    AudioDriverType_WinMM = 1,
    AudioDriverType_OSS = 2,
    AudioDriverType_ALSA = 3,
    AudioDriverType_DirectSound = 4,
    AudioDriverType_CoreAudio = 5,
    AudioDriverType_MMPM = 6,
    AudioDriverType_Pulse = 7,
    AudioDriverType_SolAudio = 8
} AudioDriverType;
/* End of enum AudioDriverType declaration */
#define AudioDriverType_T PRUint32


/* Start of enum AudioControllerType declaration */
#define AUDIOCONTROLLERTYPE_IID_STR "7afd395c-42c3-444e-8788-3ce80292f36c"
#define AUDIOCONTROLLERTYPE_IID { \
    0x7afd395c, 0x42c3, 0x444e, \
    { 0x87, 0x88, 0x3c, 0xe8, 0x02, 0x92, 0xf3, 0x6c } \
}
typedef enum AudioControllerType
{
    AudioControllerType_AC97 = 0,
    AudioControllerType_SB16 = 1,
    AudioControllerType_HDA = 2
} AudioControllerType;
/* End of enum AudioControllerType declaration */
#define AudioControllerType_T PRUint32


/* Start of enum AuthType declaration */
#define AUTHTYPE_IID_STR "7eef6ef6-98c2-4dc2-ab35-10d2b292028d"
#define AUTHTYPE_IID { \
    0x7eef6ef6, 0x98c2, 0x4dc2, \
    { 0xab, 0x35, 0x10, 0xd2, 0xb2, 0x92, 0x02, 0x8d } \
}
typedef enum AuthType
{
    AuthType_Null = 0,
    AuthType_External = 1,
    AuthType_Guest = 2
} AuthType;
/* End of enum AuthType declaration */
#define AuthType_T PRUint32


/* Start of enum Reason declaration */
#define REASON_IID_STR "e7e8e097-299d-4e98-8bbc-c31c2d47d0cc"
#define REASON_IID { \
    0xe7e8e097, 0x299d, 0x4e98, \
    { 0x8b, 0xbc, 0xc3, 0x1c, 0x2d, 0x47, 0xd0, 0xcc } \
}
typedef enum Reason
{
    Reason_Unspecified = 0,
    Reason_HostSuspend = 1,
    Reason_HostResume = 2,
    Reason_HostBatteryLow = 3
} Reason;
/* End of enum Reason declaration */
#define Reason_T PRUint32


/* Start of enum StorageBus declaration */
#define STORAGEBUS_IID_STR "eee67ab3-668d-4ef5-91e0-7025fe4a0d7a"
#define STORAGEBUS_IID { \
    0xeee67ab3, 0x668d, 0x4ef5, \
    { 0x91, 0xe0, 0x70, 0x25, 0xfe, 0x4a, 0x0d, 0x7a } \
}
typedef enum StorageBus
{
    StorageBus_Null = 0,
    StorageBus_IDE = 1,
    StorageBus_SATA = 2,
    StorageBus_SCSI = 3,
    StorageBus_Floppy = 4,
    StorageBus_SAS = 5
} StorageBus;
/* End of enum StorageBus declaration */
#define StorageBus_T PRUint32


/* Start of enum StorageControllerType declaration */
#define STORAGECONTROLLERTYPE_IID_STR "8a412b8a-f43e-4456-bd37-b474f0879a58"
#define STORAGECONTROLLERTYPE_IID { \
    0x8a412b8a, 0xf43e, 0x4456, \
    { 0xbd, 0x37, 0xb4, 0x74, 0xf0, 0x87, 0x9a, 0x58 } \
}
typedef enum StorageControllerType
{
    StorageControllerType_Null = 0,
    StorageControllerType_LsiLogic = 1,
    StorageControllerType_BusLogic = 2,
    StorageControllerType_IntelAhci = 3,
    StorageControllerType_PIIX3 = 4,
    StorageControllerType_PIIX4 = 5,
    StorageControllerType_ICH6 = 6,
    StorageControllerType_I82078 = 7,
    StorageControllerType_LsiLogicSas = 8
} StorageControllerType;
/* End of enum StorageControllerType declaration */
#define StorageControllerType_T PRUint32


/* Start of enum ChipsetType declaration */
#define CHIPSETTYPE_IID_STR "8b4096a8-a7c3-4d3b-bbb1-05a0a51ec394"
#define CHIPSETTYPE_IID { \
    0x8b4096a8, 0xa7c3, 0x4d3b, \
    { 0xbb, 0xb1, 0x05, 0xa0, 0xa5, 0x1e, 0xc3, 0x94 } \
}
typedef enum ChipsetType
{
    ChipsetType_Null = 0,
    ChipsetType_PIIX3 = 1,
    ChipsetType_ICH9 = 2
} ChipsetType;
/* End of enum ChipsetType declaration */
#define ChipsetType_T PRUint32


/* Start of enum NATAliasMode declaration */
#define NATALIASMODE_IID_STR "67772168-50d9-11df-9669-7fb714ee4fa1"
#define NATALIASMODE_IID { \
    0x67772168, 0x50d9, 0x11df, \
    { 0x96, 0x69, 0x7f, 0xb7, 0x14, 0xee, 0x4f, 0xa1 } \
}
typedef enum NATAliasMode
{
    NATAliasMode_AliasLog = 0x1,
    NATAliasMode_AliasProxyOnly = 0x02,
    NATAliasMode_AliasUseSamePorts = 0x04
} NATAliasMode;
/* End of enum NATAliasMode declaration */
#define NATAliasMode_T PRUint32


/* Start of enum NATProtocol declaration */
#define NATPROTOCOL_IID_STR "e90164be-eb03-11de-94af-fff9b1c1b19f"
#define NATPROTOCOL_IID { \
    0xe90164be, 0xeb03, 0x11de, \
    { 0x94, 0xaf, 0xff, 0xf9, 0xb1, 0xc1, 0xb1, 0x9f } \
}
typedef enum NATProtocol
{
    NATProtocol_UDP = 0,
    NATProtocol_TCP = 1
} NATProtocol;
/* End of enum NATProtocol declaration */
#define NATProtocol_T PRUint32


/* Start of enum BandwidthGroupType declaration */
#define BANDWIDTHGROUPTYPE_IID_STR "1d92b67d-dc69-4be9-ad4c-93a01e1e0c8e"
#define BANDWIDTHGROUPTYPE_IID { \
    0x1d92b67d, 0xdc69, 0x4be9, \
    { 0xad, 0x4c, 0x93, 0xa0, 0x1e, 0x1e, 0x0c, 0x8e } \
}
typedef enum BandwidthGroupType
{
    BandwidthGroupType_Null = 0,
    BandwidthGroupType_Disk = 1,
    BandwidthGroupType_Network = 2
} BandwidthGroupType;
/* End of enum BandwidthGroupType declaration */
#define BandwidthGroupType_T PRUint32


/* Start of enum VBoxEventType declaration */
#define VBOXEVENTTYPE_IID_STR "5248e377-e578-47d7-b07b-84b1db6db8a8"
#define VBOXEVENTTYPE_IID { \
    0x5248e377, 0xe578, 0x47d7, \
    { 0xb0, 0x7b, 0x84, 0xb1, 0xdb, 0x6d, 0xb8, 0xa8 } \
}
typedef enum VBoxEventType
{
    VBoxEventType_Invalid = 0,
    VBoxEventType_Any = 1,
    VBoxEventType_Vetoable = 2,
    VBoxEventType_MachineEvent = 3,
    VBoxEventType_SnapshotEvent = 4,
    VBoxEventType_InputEvent = 5,
    VBoxEventType_LastWildcard = 31,
    VBoxEventType_OnMachineStateChanged = 32,
    VBoxEventType_OnMachineDataChanged = 33,
    VBoxEventType_OnExtraDataChanged = 34,
    VBoxEventType_OnExtraDataCanChange = 35,
    VBoxEventType_OnMediumRegistered = 36,
    VBoxEventType_OnMachineRegistered = 37,
    VBoxEventType_OnSessionStateChanged = 38,
    VBoxEventType_OnSnapshotTaken = 39,
    VBoxEventType_OnSnapshotDeleted = 40,
    VBoxEventType_OnSnapshotChanged = 41,
    VBoxEventType_OnGuestPropertyChanged = 42,
    VBoxEventType_OnMousePointerShapeChanged = 43,
    VBoxEventType_OnMouseCapabilityChanged = 44,
    VBoxEventType_OnKeyboardLedsChanged = 45,
    VBoxEventType_OnStateChanged = 46,
    VBoxEventType_OnAdditionsStateChanged = 47,
    VBoxEventType_OnNetworkAdapterChanged = 48,
    VBoxEventType_OnSerialPortChanged = 49,
    VBoxEventType_OnParallelPortChanged = 50,
    VBoxEventType_OnStorageControllerChanged = 51,
    VBoxEventType_OnMediumChanged = 52,
    VBoxEventType_OnVRDEServerChanged = 53,
    VBoxEventType_OnUSBControllerChanged = 54,
    VBoxEventType_OnUSBDeviceStateChanged = 55,
    VBoxEventType_OnSharedFolderChanged = 56,
    VBoxEventType_OnRuntimeError = 57,
    VBoxEventType_OnCanShowWindow = 58,
    VBoxEventType_OnShowWindow = 59,
    VBoxEventType_OnCPUChanged = 60,
    VBoxEventType_OnVRDEServerInfoChanged = 61,
    VBoxEventType_OnEventSourceChanged = 62,
    VBoxEventType_OnCPUExecutionCapChanged = 63,
    VBoxEventType_OnGuestKeyboard = 64,
    VBoxEventType_OnGuestMouse = 65,
    VBoxEventType_OnNATRedirect = 66,
    VBoxEventType_OnHostPCIDevicePlug = 67,
    VBoxEventType_OnVBoxSVCAvailabilityChanged = 68,
    VBoxEventType_OnBandwidthGroupChanged = 69,
    VBoxEventType_OnGuestMonitorChanged = 70,
    VBoxEventType_OnStorageDeviceChanged = 71,
    VBoxEventType_OnClipboardModeChanged = 72,
    VBoxEventType_OnDragAndDropModeChanged = 73,
    VBoxEventType_OnNATNetworkChanged = 74,
    VBoxEventType_OnNATNetworkStartStop = 75,
    VBoxEventType_OnNATNetworkAlter = 76,
    VBoxEventType_OnNATNetworkCreationDeletion = 77,
    VBoxEventType_OnNATNetworkSetting = 78,
    VBoxEventType_OnNATNetworkPortForward = 79,
    VBoxEventType_OnGuestSessionStateChanged = 80,
    VBoxEventType_OnGuestSessionRegistered = 81,
    VBoxEventType_OnGuestProcessRegistered = 82,
    VBoxEventType_OnGuestProcessStateChanged = 83,
    VBoxEventType_OnGuestProcessInputNotify = 84,
    VBoxEventType_OnGuestProcessOutput = 85,
    VBoxEventType_OnGuestFileRegistered = 86,
    VBoxEventType_OnGuestFileStateChanged = 87,
    VBoxEventType_OnGuestFileOffsetChanged = 88,
    VBoxEventType_OnGuestFileRead = 89,
    VBoxEventType_OnGuestFileWrite = 90,
    VBoxEventType_OnVideoCaptureChanged = 91,
    VBoxEventType_OnGuestUserStateChanged = 92,
    VBoxEventType_OnGuestMultiTouch = 93,
    VBoxEventType_OnHostNameResolutionConfigurationChange = 94,
    VBoxEventType_Last = 95
} VBoxEventType;
/* End of enum VBoxEventType declaration */
#define VBoxEventType_T PRUint32


/* Start of enum GuestMouseEventMode declaration */
#define GUESTMOUSEEVENTMODE_IID_STR "4b500146-ebba-4b7c-bc29-69c2d57a5caf"
#define GUESTMOUSEEVENTMODE_IID { \
    0x4b500146, 0xebba, 0x4b7c, \
    { 0xbc, 0x29, 0x69, 0xc2, 0xd5, 0x7a, 0x5c, 0xaf } \
}
typedef enum GuestMouseEventMode
{
    GuestMouseEventMode_Relative = 0,
    GuestMouseEventMode_Absolute = 1
} GuestMouseEventMode;
/* End of enum GuestMouseEventMode declaration */
#define GuestMouseEventMode_T PRUint32


/* Start of enum GuestMonitorChangedEventType declaration */
#define GUESTMONITORCHANGEDEVENTTYPE_IID_STR "ef172985-7e36-4297-95be-e46396968d66"
#define GUESTMONITORCHANGEDEVENTTYPE_IID { \
    0xef172985, 0x7e36, 0x4297, \
    { 0x95, 0xbe, 0xe4, 0x63, 0x96, 0x96, 0x8d, 0x66 } \
}
typedef enum GuestMonitorChangedEventType
{
    GuestMonitorChangedEventType_Enabled = 0,
    GuestMonitorChangedEventType_Disabled = 1,
    GuestMonitorChangedEventType_NewOrigin = 2
} GuestMonitorChangedEventType;
/* End of enum GuestMonitorChangedEventType declaration */
#define GuestMonitorChangedEventType_T PRUint32


/* Start of struct IVirtualBoxErrorInfo declaration */
#define IVIRTUALBOXERRORINFO_IID_STR "c1bcc6d5-7966-481d-ab0b-d0ed73e28135"
#define IVIRTUALBOXERRORINFO_IID { \
    0xc1bcc6d5, 0x7966, 0x481d, \
    { 0xab, 0x0b, 0xd0, 0xed, 0x73, 0xe2, 0x81, 0x35 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVirtualBoxErrorInfo);
#ifndef VBOX_WITH_GLUE
struct IVirtualBoxErrorInfo_vtbl
{
    struct nsIException_vtbl nsiexception;

    nsresult (*GetResultCode)(IVirtualBoxErrorInfo *pThis, PRInt32 *resultCode);

    nsresult (*GetResultDetail)(IVirtualBoxErrorInfo *pThis, PRInt32 *resultDetail);

    nsresult (*GetInterfaceID)(IVirtualBoxErrorInfo *pThis, PRUnichar * *interfaceID);

    nsresult (*GetComponent)(IVirtualBoxErrorInfo *pThis, PRUnichar * *component);

    nsresult (*GetText)(IVirtualBoxErrorInfo *pThis, PRUnichar * *text);

    nsresult (*GetNext)(IVirtualBoxErrorInfo *pThis, IVirtualBoxErrorInfo * *next);

};
#else /* VBOX_WITH_GLUE */
struct IVirtualBoxErrorInfoVtbl
{
    nsresult (*QueryInterface)(IVirtualBoxErrorInfo *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVirtualBoxErrorInfo *pThis);
    nsrefcnt (*Release)(IVirtualBoxErrorInfo *pThis);
    nsresult (*GetMessage)(IVirtualBoxErrorInfo *pThis, PRUnichar * *aMessage);
    nsresult (*GetResult)(IVirtualBoxErrorInfo *pThis, nsresult *aResult);
    nsresult (*GetName)(IVirtualBoxErrorInfo*pThis, PRUnichar * *aName);
    nsresult (*GetFilename)(IVirtualBoxErrorInfo *pThis, PRUnichar * *aFilename);
    nsresult (*GetLineNumber)(IVirtualBoxErrorInfo *pThis, PRUint32 *aLineNumber);
    nsresult (*GetColumnNumber)(IVirtualBoxErrorInfo *pThis, PRUint32 *aColumnNumber);
    nsresult (*GetLocation)(IVirtualBoxErrorInfo *pThis, nsIStackFrame * *aLocation);
    nsresult (*GetInner)(IVirtualBoxErrorInfo *pThis, nsIException * *aInner);
    nsresult (*GetData)(IVirtualBoxErrorInfo *pThis, nsISupports * *aData);
    nsresult (*ToString)(IVirtualBoxErrorInfo *pThis, PRUnichar **_retval);
    nsresult (*GetResultCode)(IVirtualBoxErrorInfo *pThis, PRInt32 *resultCode);

    nsresult (*GetResultDetail)(IVirtualBoxErrorInfo *pThis, PRInt32 *resultDetail);

    nsresult (*GetInterfaceID)(IVirtualBoxErrorInfo *pThis, PRUnichar * *interfaceID);

    nsresult (*GetComponent)(IVirtualBoxErrorInfo *pThis, PRUnichar * *component);

    nsresult (*GetText)(IVirtualBoxErrorInfo *pThis, PRUnichar * *text);

    nsresult (*GetNext)(IVirtualBoxErrorInfo *pThis, IVirtualBoxErrorInfo * *next);

};
#define IVirtualBoxErrorInfo_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVirtualBoxErrorInfo_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVirtualBoxErrorInfo_Release(p) ((p)->lpVtbl->Release(p))
#define IVirtualBoxErrorInfo_get_Message(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define IVirtualBoxErrorInfo_GetMessage(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define IVirtualBoxErrorInfo_get_Result(p, aResult) ((p)->lpVtbl->GetResult(p, aResult))
#define IVirtualBoxErrorInfo_GetResult(p, aResult) ((p)->lpVtbl->GetResult(p, aResult))
#define IVirtualBoxErrorInfo_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IVirtualBoxErrorInfo_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IVirtualBoxErrorInfo_get_Filename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define IVirtualBoxErrorInfo_GetFilename(p, aFilename) ((p)->lpVtbl->GetFilename(p, aFilename))
#define IVirtualBoxErrorInfo_get_LineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define IVirtualBoxErrorInfo_GetLineNumber(p, aLineNumber) ((p)->lpVtbl->GetLineNumber(p, aLineNumber))
#define IVirtualBoxErrorInfo_get_ColumnNumber(p, aColumnNumber) ((p)->lpVtbl->GetColumnNumber(p, aColumnNumber))
#define IVirtualBoxErrorInfo_GetColumnNumber(p, aColumnNumber) ((p)->lpVtbl->GetColumnNumber(p, aColumnNumber))
#define IVirtualBoxErrorInfo_get_Location(p, aLocation) ((p)->lpVtbl->GetLocation(p, aLocation))
#define IVirtualBoxErrorInfo_GetLocation(p, aLocation) ((p)->lpVtbl->GetLocation(p, aLocation))
#define IVirtualBoxErrorInfo_get_Inner(p, aInner) ((p)->lpVtbl->GetInner(p, aInner))
#define IVirtualBoxErrorInfo_GetInner(p, aInner) ((p)->lpVtbl->GetInner(p, aInner))
#define IVirtualBoxErrorInfo_get_Data(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define IVirtualBoxErrorInfo_GetData(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define IVirtualBoxErrorInfo_ToString(p, retval) ((p)->lpVtbl->ToString(p, retval))
#define IVirtualBoxErrorInfo_get_ResultCode(p, aResultCode) ((p)->lpVtbl->GetResultCode(p, aResultCode))
#define IVirtualBoxErrorInfo_GetResultCode(p, aResultCode) ((p)->lpVtbl->GetResultCode(p, aResultCode))
#define IVirtualBoxErrorInfo_get_ResultDetail(p, aResultDetail) ((p)->lpVtbl->GetResultDetail(p, aResultDetail))
#define IVirtualBoxErrorInfo_GetResultDetail(p, aResultDetail) ((p)->lpVtbl->GetResultDetail(p, aResultDetail))
#define IVirtualBoxErrorInfo_get_InterfaceID(p, aInterfaceID) ((p)->lpVtbl->GetInterfaceID(p, aInterfaceID))
#define IVirtualBoxErrorInfo_GetInterfaceID(p, aInterfaceID) ((p)->lpVtbl->GetInterfaceID(p, aInterfaceID))
#define IVirtualBoxErrorInfo_get_Component(p, aComponent) ((p)->lpVtbl->GetComponent(p, aComponent))
#define IVirtualBoxErrorInfo_GetComponent(p, aComponent) ((p)->lpVtbl->GetComponent(p, aComponent))
#define IVirtualBoxErrorInfo_get_Text(p, aText) ((p)->lpVtbl->GetText(p, aText))
#define IVirtualBoxErrorInfo_GetText(p, aText) ((p)->lpVtbl->GetText(p, aText))
#define IVirtualBoxErrorInfo_get_Next(p, aNext) ((p)->lpVtbl->GetNext(p, aNext))
#define IVirtualBoxErrorInfo_GetNext(p, aNext) ((p)->lpVtbl->GetNext(p, aNext))
#endif /* VBOX_WITH_GLUE */

interface IVirtualBoxErrorInfo
{
#ifndef VBOX_WITH_GLUE
    struct IVirtualBoxErrorInfo_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVirtualBoxErrorInfoVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVirtualBoxErrorInfo declaration */


/* Start of struct INATNetwork declaration */
#define INATNETWORK_IID_STR "a63c75da-4c99-4e9d-8351-eb73651c18cc"
#define INATNETWORK_IID { \
    0xa63c75da, 0x4c99, 0x4e9d, \
    { 0x83, 0x51, 0xeb, 0x73, 0x65, 0x1c, 0x18, 0xcc } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetwork);
#ifndef VBOX_WITH_GLUE
struct INATNetwork_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetNetworkName)(INATNetwork *pThis, PRUnichar * *networkName);
    nsresult (*SetNetworkName)(INATNetwork *pThis, PRUnichar * networkName);

    nsresult (*GetEnabled)(INATNetwork *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(INATNetwork *pThis, PRBool enabled);

    nsresult (*GetNetwork)(INATNetwork *pThis, PRUnichar * *network);
    nsresult (*SetNetwork)(INATNetwork *pThis, PRUnichar * network);

    nsresult (*GetGateway)(INATNetwork *pThis, PRUnichar * *gateway);

    nsresult (*GetIPv6Enabled)(INATNetwork *pThis, PRBool *IPv6Enabled);
    nsresult (*SetIPv6Enabled)(INATNetwork *pThis, PRBool IPv6Enabled);

    nsresult (*GetIPv6Prefix)(INATNetwork *pThis, PRUnichar * *IPv6Prefix);
    nsresult (*SetIPv6Prefix)(INATNetwork *pThis, PRUnichar * IPv6Prefix);

    nsresult (*GetAdvertiseDefaultIPv6RouteEnabled)(INATNetwork *pThis, PRBool *advertiseDefaultIPv6RouteEnabled);
    nsresult (*SetAdvertiseDefaultIPv6RouteEnabled)(INATNetwork *pThis, PRBool advertiseDefaultIPv6RouteEnabled);

    nsresult (*GetNeedDhcpServer)(INATNetwork *pThis, PRBool *needDhcpServer);
    nsresult (*SetNeedDhcpServer)(INATNetwork *pThis, PRBool needDhcpServer);

    nsresult (*GetEventSource)(INATNetwork *pThis, IEventSource * *eventSource);

    nsresult (*GetPortForwardRules4)(INATNetwork *pThis, PRUint32 *portForwardRules4Size, PRUnichar * **portForwardRules4);

    nsresult (*GetLocalMappings)(INATNetwork *pThis, PRUint32 *localMappingsSize, PRUnichar * **localMappings);

    nsresult (*GetLoopbackIp6)(INATNetwork *pThis, PRInt32 *loopbackIp6);
    nsresult (*SetLoopbackIp6)(INATNetwork *pThis, PRInt32 loopbackIp6);

    nsresult (*GetPortForwardRules6)(INATNetwork *pThis, PRUint32 *portForwardRules6Size, PRUnichar * **portForwardRules6);

    nsresult (*AddLocalMapping)(
        INATNetwork *pThis,
        PRUnichar * hostid,
        PRInt32 offset
    );

    nsresult (*AddPortForwardRule)(
        INATNetwork *pThis,
        PRBool isIpv6,
        PRUnichar * ruleName,
        PRUint32 proto,
        PRUnichar * hostIP,
        PRUint16 hostPort,
        PRUnichar * guestIP,
        PRUint16 guestPort
    );

    nsresult (*RemovePortForwardRule)(
        INATNetwork *pThis,
        PRBool iSipv6,
        PRUnichar * ruleName
    );

    nsresult (*Start)(
        INATNetwork *pThis,
        PRUnichar * trunkType
    );

    nsresult (*Stop)(INATNetwork *pThis );

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkVtbl
{
    nsresult (*QueryInterface)(INATNetwork *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetwork *pThis);
    nsrefcnt (*Release)(INATNetwork *pThis);
    nsresult (*GetNetworkName)(INATNetwork *pThis, PRUnichar * *networkName);
    nsresult (*SetNetworkName)(INATNetwork *pThis, PRUnichar * networkName);

    nsresult (*GetEnabled)(INATNetwork *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(INATNetwork *pThis, PRBool enabled);

    nsresult (*GetNetwork)(INATNetwork *pThis, PRUnichar * *network);
    nsresult (*SetNetwork)(INATNetwork *pThis, PRUnichar * network);

    nsresult (*GetGateway)(INATNetwork *pThis, PRUnichar * *gateway);

    nsresult (*GetIPv6Enabled)(INATNetwork *pThis, PRBool *IPv6Enabled);
    nsresult (*SetIPv6Enabled)(INATNetwork *pThis, PRBool IPv6Enabled);

    nsresult (*GetIPv6Prefix)(INATNetwork *pThis, PRUnichar * *IPv6Prefix);
    nsresult (*SetIPv6Prefix)(INATNetwork *pThis, PRUnichar * IPv6Prefix);

    nsresult (*GetAdvertiseDefaultIPv6RouteEnabled)(INATNetwork *pThis, PRBool *advertiseDefaultIPv6RouteEnabled);
    nsresult (*SetAdvertiseDefaultIPv6RouteEnabled)(INATNetwork *pThis, PRBool advertiseDefaultIPv6RouteEnabled);

    nsresult (*GetNeedDhcpServer)(INATNetwork *pThis, PRBool *needDhcpServer);
    nsresult (*SetNeedDhcpServer)(INATNetwork *pThis, PRBool needDhcpServer);

    nsresult (*GetEventSource)(INATNetwork *pThis, IEventSource * *eventSource);

    nsresult (*GetPortForwardRules4)(INATNetwork *pThis, PRUint32 *portForwardRules4Size, PRUnichar * **portForwardRules4);

    nsresult (*GetLocalMappings)(INATNetwork *pThis, PRUint32 *localMappingsSize, PRUnichar * **localMappings);

    nsresult (*GetLoopbackIp6)(INATNetwork *pThis, PRInt32 *loopbackIp6);
    nsresult (*SetLoopbackIp6)(INATNetwork *pThis, PRInt32 loopbackIp6);

    nsresult (*GetPortForwardRules6)(INATNetwork *pThis, PRUint32 *portForwardRules6Size, PRUnichar * **portForwardRules6);

    nsresult (*AddLocalMapping)(
        INATNetwork *pThis,
        PRUnichar * hostid,
        PRInt32 offset
    );

    nsresult (*AddPortForwardRule)(
        INATNetwork *pThis,
        PRBool isIpv6,
        PRUnichar * ruleName,
        PRUint32 proto,
        PRUnichar * hostIP,
        PRUint16 hostPort,
        PRUnichar * guestIP,
        PRUint16 guestPort
    );

    nsresult (*RemovePortForwardRule)(
        INATNetwork *pThis,
        PRBool iSipv6,
        PRUnichar * ruleName
    );

    nsresult (*Start)(
        INATNetwork *pThis,
        PRUnichar * trunkType
    );

    nsresult (*Stop)(INATNetwork *pThis );

};
#define INATNetwork_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetwork_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetwork_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetwork_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetwork_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetwork_put_NetworkName(p, aNetworkName) ((p)->lpVtbl->SetNetworkName(p, aNetworkName))
#define INATNetwork_SetNetworkName(p, aNetworkName) ((p)->lpVtbl->SetNetworkName(p, aNetworkName))
#define INATNetwork_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define INATNetwork_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define INATNetwork_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define INATNetwork_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define INATNetwork_get_Network(p, aNetwork) ((p)->lpVtbl->GetNetwork(p, aNetwork))
#define INATNetwork_GetNetwork(p, aNetwork) ((p)->lpVtbl->GetNetwork(p, aNetwork))
#define INATNetwork_put_Network(p, aNetwork) ((p)->lpVtbl->SetNetwork(p, aNetwork))
#define INATNetwork_SetNetwork(p, aNetwork) ((p)->lpVtbl->SetNetwork(p, aNetwork))
#define INATNetwork_get_Gateway(p, aGateway) ((p)->lpVtbl->GetGateway(p, aGateway))
#define INATNetwork_GetGateway(p, aGateway) ((p)->lpVtbl->GetGateway(p, aGateway))
#define INATNetwork_get_IPv6Enabled(p, aIPv6Enabled) ((p)->lpVtbl->GetIPv6Enabled(p, aIPv6Enabled))
#define INATNetwork_GetIPv6Enabled(p, aIPv6Enabled) ((p)->lpVtbl->GetIPv6Enabled(p, aIPv6Enabled))
#define INATNetwork_put_IPv6Enabled(p, aIPv6Enabled) ((p)->lpVtbl->SetIPv6Enabled(p, aIPv6Enabled))
#define INATNetwork_SetIPv6Enabled(p, aIPv6Enabled) ((p)->lpVtbl->SetIPv6Enabled(p, aIPv6Enabled))
#define INATNetwork_get_IPv6Prefix(p, aIPv6Prefix) ((p)->lpVtbl->GetIPv6Prefix(p, aIPv6Prefix))
#define INATNetwork_GetIPv6Prefix(p, aIPv6Prefix) ((p)->lpVtbl->GetIPv6Prefix(p, aIPv6Prefix))
#define INATNetwork_put_IPv6Prefix(p, aIPv6Prefix) ((p)->lpVtbl->SetIPv6Prefix(p, aIPv6Prefix))
#define INATNetwork_SetIPv6Prefix(p, aIPv6Prefix) ((p)->lpVtbl->SetIPv6Prefix(p, aIPv6Prefix))
#define INATNetwork_get_AdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled) ((p)->lpVtbl->GetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled))
#define INATNetwork_GetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled) ((p)->lpVtbl->GetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled))
#define INATNetwork_put_AdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled) ((p)->lpVtbl->SetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled))
#define INATNetwork_SetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled) ((p)->lpVtbl->SetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled))
#define INATNetwork_get_NeedDhcpServer(p, aNeedDhcpServer) ((p)->lpVtbl->GetNeedDhcpServer(p, aNeedDhcpServer))
#define INATNetwork_GetNeedDhcpServer(p, aNeedDhcpServer) ((p)->lpVtbl->GetNeedDhcpServer(p, aNeedDhcpServer))
#define INATNetwork_put_NeedDhcpServer(p, aNeedDhcpServer) ((p)->lpVtbl->SetNeedDhcpServer(p, aNeedDhcpServer))
#define INATNetwork_SetNeedDhcpServer(p, aNeedDhcpServer) ((p)->lpVtbl->SetNeedDhcpServer(p, aNeedDhcpServer))
#define INATNetwork_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define INATNetwork_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define INATNetwork_get_PortForwardRules4(p, aPortForwardRules4) ((p)->lpVtbl->GetPortForwardRules4(p, aPortForwardRules4))
#define INATNetwork_GetPortForwardRules4(p, aPortForwardRules4) ((p)->lpVtbl->GetPortForwardRules4(p, aPortForwardRules4))
#define INATNetwork_get_LocalMappings(p, aLocalMappings) ((p)->lpVtbl->GetLocalMappings(p, aLocalMappings))
#define INATNetwork_GetLocalMappings(p, aLocalMappings) ((p)->lpVtbl->GetLocalMappings(p, aLocalMappings))
#define INATNetwork_get_LoopbackIp6(p, aLoopbackIp6) ((p)->lpVtbl->GetLoopbackIp6(p, aLoopbackIp6))
#define INATNetwork_GetLoopbackIp6(p, aLoopbackIp6) ((p)->lpVtbl->GetLoopbackIp6(p, aLoopbackIp6))
#define INATNetwork_put_LoopbackIp6(p, aLoopbackIp6) ((p)->lpVtbl->SetLoopbackIp6(p, aLoopbackIp6))
#define INATNetwork_SetLoopbackIp6(p, aLoopbackIp6) ((p)->lpVtbl->SetLoopbackIp6(p, aLoopbackIp6))
#define INATNetwork_get_PortForwardRules6(p, aPortForwardRules6) ((p)->lpVtbl->GetPortForwardRules6(p, aPortForwardRules6))
#define INATNetwork_GetPortForwardRules6(p, aPortForwardRules6) ((p)->lpVtbl->GetPortForwardRules6(p, aPortForwardRules6))
#define INATNetwork_AddLocalMapping(p, aHostid, aOffset) ((p)->lpVtbl->AddLocalMapping(p, aHostid, aOffset))
#define INATNetwork_AddPortForwardRule(p, aIsIpv6, aRuleName, aProto, aHostIP, aHostPort, aGuestIP, aGuestPort) ((p)->lpVtbl->AddPortForwardRule(p, aIsIpv6, aRuleName, aProto, aHostIP, aHostPort, aGuestIP, aGuestPort))
#define INATNetwork_RemovePortForwardRule(p, aISipv6, aRuleName) ((p)->lpVtbl->RemovePortForwardRule(p, aISipv6, aRuleName))
#define INATNetwork_Start(p, aTrunkType) ((p)->lpVtbl->Start(p, aTrunkType))
#define INATNetwork_Stop(p) ((p)->lpVtbl->Stop(p))
#endif /* VBOX_WITH_GLUE */

interface INATNetwork
{
#ifndef VBOX_WITH_GLUE
    struct INATNetwork_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetwork declaration */


/* Start of struct IDHCPServer declaration */
#define IDHCPSERVER_IID_STR "ff0774c5-1f62-4bc3-919c-7fc942bf1d25"
#define IDHCPSERVER_IID { \
    0xff0774c5, 0x1f62, 0x4bc3, \
    { 0x91, 0x9c, 0x7f, 0xc9, 0x42, 0xbf, 0x1d, 0x25 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IDHCPServer);
#ifndef VBOX_WITH_GLUE
struct IDHCPServer_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetEventSource)(IDHCPServer *pThis, IEventSource * *eventSource);

    nsresult (*GetEnabled)(IDHCPServer *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IDHCPServer *pThis, PRBool enabled);

    nsresult (*GetIPAddress)(IDHCPServer *pThis, PRUnichar * *IPAddress);

    nsresult (*GetNetworkMask)(IDHCPServer *pThis, PRUnichar * *networkMask);

    nsresult (*GetNetworkName)(IDHCPServer *pThis, PRUnichar * *networkName);

    nsresult (*GetLowerIP)(IDHCPServer *pThis, PRUnichar * *lowerIP);

    nsresult (*GetUpperIP)(IDHCPServer *pThis, PRUnichar * *upperIP);

    nsresult (*GetGlobalOptions)(IDHCPServer *pThis, PRUint32 *globalOptionsSize, PRUnichar * **globalOptions);

    nsresult (*GetVmConfigs)(IDHCPServer *pThis, PRUint32 *vmConfigsSize, PRUnichar * **vmConfigs);

    nsresult (*AddGlobalOption)(
        IDHCPServer *pThis,
        PRUint32 option,
        PRUnichar * value
    );

    nsresult (*AddVmSlotOption)(
        IDHCPServer *pThis,
        PRUnichar * vmname,
        PRInt32 slot,
        PRUint32 option,
        PRUnichar * value
    );

    nsresult (*RemoveVmSlotOptions)(
        IDHCPServer *pThis,
        PRUnichar * vmname,
        PRInt32 slot
    );

    nsresult (*GetVmSlotOptions)(
        IDHCPServer *pThis,
        PRUnichar * vmname,
        PRInt32 slot,
        PRUint32 *optionSize,
        PRUnichar *** option
    );

    nsresult (*GetMacOptions)(
        IDHCPServer *pThis,
        PRUnichar * mac,
        PRUint32 *optionSize,
        PRUnichar *** option
    );

    nsresult (*SetConfiguration)(
        IDHCPServer *pThis,
        PRUnichar * IPAddress,
        PRUnichar * networkMask,
        PRUnichar * FromIPAddress,
        PRUnichar * ToIPAddress
    );

    nsresult (*Start)(
        IDHCPServer *pThis,
        PRUnichar * networkName,
        PRUnichar * trunkName,
        PRUnichar * trunkType
    );

    nsresult (*Stop)(IDHCPServer *pThis );

};
#else /* VBOX_WITH_GLUE */
struct IDHCPServerVtbl
{
    nsresult (*QueryInterface)(IDHCPServer *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IDHCPServer *pThis);
    nsrefcnt (*Release)(IDHCPServer *pThis);
    nsresult (*GetEventSource)(IDHCPServer *pThis, IEventSource * *eventSource);

    nsresult (*GetEnabled)(IDHCPServer *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IDHCPServer *pThis, PRBool enabled);

    nsresult (*GetIPAddress)(IDHCPServer *pThis, PRUnichar * *IPAddress);

    nsresult (*GetNetworkMask)(IDHCPServer *pThis, PRUnichar * *networkMask);

    nsresult (*GetNetworkName)(IDHCPServer *pThis, PRUnichar * *networkName);

    nsresult (*GetLowerIP)(IDHCPServer *pThis, PRUnichar * *lowerIP);

    nsresult (*GetUpperIP)(IDHCPServer *pThis, PRUnichar * *upperIP);

    nsresult (*GetGlobalOptions)(IDHCPServer *pThis, PRUint32 *globalOptionsSize, PRUnichar * **globalOptions);

    nsresult (*GetVmConfigs)(IDHCPServer *pThis, PRUint32 *vmConfigsSize, PRUnichar * **vmConfigs);

    nsresult (*AddGlobalOption)(
        IDHCPServer *pThis,
        PRUint32 option,
        PRUnichar * value
    );

    nsresult (*AddVmSlotOption)(
        IDHCPServer *pThis,
        PRUnichar * vmname,
        PRInt32 slot,
        PRUint32 option,
        PRUnichar * value
    );

    nsresult (*RemoveVmSlotOptions)(
        IDHCPServer *pThis,
        PRUnichar * vmname,
        PRInt32 slot
    );

    nsresult (*GetVmSlotOptions)(
        IDHCPServer *pThis,
        PRUnichar * vmname,
        PRInt32 slot,
        PRUint32 *optionSize,
        PRUnichar *** option
    );

    nsresult (*GetMacOptions)(
        IDHCPServer *pThis,
        PRUnichar * mac,
        PRUint32 *optionSize,
        PRUnichar *** option
    );

    nsresult (*SetConfiguration)(
        IDHCPServer *pThis,
        PRUnichar * IPAddress,
        PRUnichar * networkMask,
        PRUnichar * FromIPAddress,
        PRUnichar * ToIPAddress
    );

    nsresult (*Start)(
        IDHCPServer *pThis,
        PRUnichar * networkName,
        PRUnichar * trunkName,
        PRUnichar * trunkType
    );

    nsresult (*Stop)(IDHCPServer *pThis );

};
#define IDHCPServer_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IDHCPServer_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IDHCPServer_Release(p) ((p)->lpVtbl->Release(p))
#define IDHCPServer_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IDHCPServer_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IDHCPServer_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IDHCPServer_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IDHCPServer_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IDHCPServer_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IDHCPServer_get_IPAddress(p, aIPAddress) ((p)->lpVtbl->GetIPAddress(p, aIPAddress))
#define IDHCPServer_GetIPAddress(p, aIPAddress) ((p)->lpVtbl->GetIPAddress(p, aIPAddress))
#define IDHCPServer_get_NetworkMask(p, aNetworkMask) ((p)->lpVtbl->GetNetworkMask(p, aNetworkMask))
#define IDHCPServer_GetNetworkMask(p, aNetworkMask) ((p)->lpVtbl->GetNetworkMask(p, aNetworkMask))
#define IDHCPServer_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define IDHCPServer_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define IDHCPServer_get_LowerIP(p, aLowerIP) ((p)->lpVtbl->GetLowerIP(p, aLowerIP))
#define IDHCPServer_GetLowerIP(p, aLowerIP) ((p)->lpVtbl->GetLowerIP(p, aLowerIP))
#define IDHCPServer_get_UpperIP(p, aUpperIP) ((p)->lpVtbl->GetUpperIP(p, aUpperIP))
#define IDHCPServer_GetUpperIP(p, aUpperIP) ((p)->lpVtbl->GetUpperIP(p, aUpperIP))
#define IDHCPServer_get_GlobalOptions(p, aGlobalOptions) ((p)->lpVtbl->GetGlobalOptions(p, aGlobalOptions))
#define IDHCPServer_GetGlobalOptions(p, aGlobalOptions) ((p)->lpVtbl->GetGlobalOptions(p, aGlobalOptions))
#define IDHCPServer_get_VmConfigs(p, aVmConfigs) ((p)->lpVtbl->GetVmConfigs(p, aVmConfigs))
#define IDHCPServer_GetVmConfigs(p, aVmConfigs) ((p)->lpVtbl->GetVmConfigs(p, aVmConfigs))
#define IDHCPServer_AddGlobalOption(p, aOption, aValue) ((p)->lpVtbl->AddGlobalOption(p, aOption, aValue))
#define IDHCPServer_AddVmSlotOption(p, aVmname, aSlot, aOption, aValue) ((p)->lpVtbl->AddVmSlotOption(p, aVmname, aSlot, aOption, aValue))
#define IDHCPServer_RemoveVmSlotOptions(p, aVmname, aSlot) ((p)->lpVtbl->RemoveVmSlotOptions(p, aVmname, aSlot))
#define IDHCPServer_GetVmSlotOptions(p, aVmname, aSlot, aOption) ((p)->lpVtbl->GetVmSlotOptions(p, aVmname, aSlot, aOption))
#define IDHCPServer_GetMacOptions(p, aMac, aOption) ((p)->lpVtbl->GetMacOptions(p, aMac, aOption))
#define IDHCPServer_SetConfiguration(p, aIPAddress, aNetworkMask, aFromIPAddress, aToIPAddress) ((p)->lpVtbl->SetConfiguration(p, aIPAddress, aNetworkMask, aFromIPAddress, aToIPAddress))
#define IDHCPServer_Start(p, aNetworkName, aTrunkName, aTrunkType) ((p)->lpVtbl->Start(p, aNetworkName, aTrunkName, aTrunkType))
#define IDHCPServer_Stop(p) ((p)->lpVtbl->Stop(p))
#endif /* VBOX_WITH_GLUE */

interface IDHCPServer
{
#ifndef VBOX_WITH_GLUE
    struct IDHCPServer_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IDHCPServerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IDHCPServer declaration */


/* Start of struct IVirtualBox declaration */
#define IVIRTUALBOX_IID_STR "fafa4e17-1ee2-4905-a10e-fe7c18bf5554"
#define IVIRTUALBOX_IID { \
    0xfafa4e17, 0x1ee2, 0x4905, \
    { 0xa1, 0x0e, 0xfe, 0x7c, 0x18, 0xbf, 0x55, 0x54 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVirtualBox);
#ifndef VBOX_WITH_GLUE
struct IVirtualBox_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetVersion)(IVirtualBox *pThis, PRUnichar * *version);

    nsresult (*GetVersionNormalized)(IVirtualBox *pThis, PRUnichar * *versionNormalized);

    nsresult (*GetRevision)(IVirtualBox *pThis, PRUint32 *revision);

    nsresult (*GetPackageType)(IVirtualBox *pThis, PRUnichar * *packageType);

    nsresult (*GetAPIVersion)(IVirtualBox *pThis, PRUnichar * *APIVersion);

    nsresult (*GetHomeFolder)(IVirtualBox *pThis, PRUnichar * *homeFolder);

    nsresult (*GetSettingsFilePath)(IVirtualBox *pThis, PRUnichar * *settingsFilePath);

    nsresult (*GetHost)(IVirtualBox *pThis, IHost * *host);

    nsresult (*GetSystemProperties)(IVirtualBox *pThis, ISystemProperties * *systemProperties);

    nsresult (*GetMachines)(IVirtualBox *pThis, PRUint32 *machinesSize, IMachine * **machines);

    nsresult (*GetMachineGroups)(IVirtualBox *pThis, PRUint32 *machineGroupsSize, PRUnichar * **machineGroups);

    nsresult (*GetHardDisks)(IVirtualBox *pThis, PRUint32 *hardDisksSize, IMedium * **hardDisks);

    nsresult (*GetDVDImages)(IVirtualBox *pThis, PRUint32 *DVDImagesSize, IMedium * **DVDImages);

    nsresult (*GetFloppyImages)(IVirtualBox *pThis, PRUint32 *floppyImagesSize, IMedium * **floppyImages);

    nsresult (*GetProgressOperations)(IVirtualBox *pThis, PRUint32 *progressOperationsSize, IProgress * **progressOperations);

    nsresult (*GetGuestOSTypes)(IVirtualBox *pThis, PRUint32 *guestOSTypesSize, IGuestOSType * **guestOSTypes);

    nsresult (*GetSharedFolders)(IVirtualBox *pThis, PRUint32 *sharedFoldersSize, ISharedFolder * **sharedFolders);

    nsresult (*GetPerformanceCollector)(IVirtualBox *pThis, IPerformanceCollector * *performanceCollector);

    nsresult (*GetDHCPServers)(IVirtualBox *pThis, PRUint32 *DHCPServersSize, IDHCPServer * **DHCPServers);

    nsresult (*GetNATNetworks)(IVirtualBox *pThis, PRUint32 *NATNetworksSize, INATNetwork * **NATNetworks);

    nsresult (*GetEventSource)(IVirtualBox *pThis, IEventSource * *eventSource);

    nsresult (*GetExtensionPackManager)(IVirtualBox *pThis, IExtPackManager * *extensionPackManager);

    nsresult (*GetInternalNetworks)(IVirtualBox *pThis, PRUint32 *internalNetworksSize, PRUnichar * **internalNetworks);

    nsresult (*GetGenericNetworkDrivers)(IVirtualBox *pThis, PRUint32 *genericNetworkDriversSize, PRUnichar * **genericNetworkDrivers);

    nsresult (*ComposeMachineFilename)(
        IVirtualBox *pThis,
        PRUnichar * name,
        PRUnichar * group,
        PRUnichar * createFlags,
        PRUnichar * baseFolder,
        PRUnichar * * file
    );

    nsresult (*CreateMachine)(
        IVirtualBox *pThis,
        PRUnichar * settingsFile,
        PRUnichar * name,
        PRUint32 groupsSize,
        PRUnichar ** groups,
        PRUnichar * osTypeId,
        PRUnichar * flags,
        IMachine * * machine
    );

    nsresult (*OpenMachine)(
        IVirtualBox *pThis,
        PRUnichar * settingsFile,
        IMachine * * machine
    );

    nsresult (*RegisterMachine)(
        IVirtualBox *pThis,
        IMachine * machine
    );

    nsresult (*FindMachine)(
        IVirtualBox *pThis,
        PRUnichar * nameOrId,
        IMachine * * machine
    );

    nsresult (*GetMachinesByGroups)(
        IVirtualBox *pThis,
        PRUint32 groupsSize,
        PRUnichar ** groups,
        PRUint32 *machinesSize,
        IMachine *** machines
    );

    nsresult (*GetMachineStates)(
        IVirtualBox *pThis,
        PRUint32 machinesSize,
        IMachine ** machines,
        PRUint32 *statesSize,
        PRUint32** states
    );

    nsresult (*CreateAppliance)(
        IVirtualBox *pThis,
        IAppliance * * appliance
    );

    nsresult (*CreateHardDisk)(
        IVirtualBox *pThis,
        PRUnichar * format,
        PRUnichar * location,
        IMedium * * medium
    );

    nsresult (*OpenMedium)(
        IVirtualBox *pThis,
        PRUnichar * location,
        PRUint32 deviceType,
        PRUint32 accessMode,
        PRBool forceNewUuid,
        IMedium * * medium
    );

    nsresult (*GetGuestOSType)(
        IVirtualBox *pThis,
        PRUnichar * id,
        IGuestOSType * * type
    );

    nsresult (*CreateSharedFolder)(
        IVirtualBox *pThis,
        PRUnichar * name,
        PRUnichar * hostPath,
        PRBool writable,
        PRBool automount
    );

    nsresult (*RemoveSharedFolder)(
        IVirtualBox *pThis,
        PRUnichar * name
    );

    nsresult (*GetExtraDataKeys)(
        IVirtualBox *pThis,
        PRUint32 *keysSize,
        PRUnichar *** keys
    );

    nsresult (*GetExtraData)(
        IVirtualBox *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

    nsresult (*SetExtraData)(
        IVirtualBox *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*SetSettingsSecret)(
        IVirtualBox *pThis,
        PRUnichar * password
    );

    nsresult (*CreateDHCPServer)(
        IVirtualBox *pThis,
        PRUnichar * name,
        IDHCPServer * * server
    );

    nsresult (*FindDHCPServerByNetworkName)(
        IVirtualBox *pThis,
        PRUnichar * name,
        IDHCPServer * * server
    );

    nsresult (*RemoveDHCPServer)(
        IVirtualBox *pThis,
        IDHCPServer * server
    );

    nsresult (*CreateNATNetwork)(
        IVirtualBox *pThis,
        PRUnichar * networkName,
        INATNetwork * * network
    );

    nsresult (*FindNATNetworkByName)(
        IVirtualBox *pThis,
        PRUnichar * networkName,
        INATNetwork * * network
    );

    nsresult (*RemoveNATNetwork)(
        IVirtualBox *pThis,
        INATNetwork * network
    );

    nsresult (*CheckFirmwarePresent)(
        IVirtualBox *pThis,
        PRUint32 firmwareType,
        PRUnichar * version,
        PRUnichar * * url,
        PRUnichar * * file,
        PRBool * result
    );

};
#else /* VBOX_WITH_GLUE */
struct IVirtualBoxVtbl
{
    nsresult (*QueryInterface)(IVirtualBox *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVirtualBox *pThis);
    nsrefcnt (*Release)(IVirtualBox *pThis);
    nsresult (*GetVersion)(IVirtualBox *pThis, PRUnichar * *version);

    nsresult (*GetVersionNormalized)(IVirtualBox *pThis, PRUnichar * *versionNormalized);

    nsresult (*GetRevision)(IVirtualBox *pThis, PRUint32 *revision);

    nsresult (*GetPackageType)(IVirtualBox *pThis, PRUnichar * *packageType);

    nsresult (*GetAPIVersion)(IVirtualBox *pThis, PRUnichar * *APIVersion);

    nsresult (*GetHomeFolder)(IVirtualBox *pThis, PRUnichar * *homeFolder);

    nsresult (*GetSettingsFilePath)(IVirtualBox *pThis, PRUnichar * *settingsFilePath);

    nsresult (*GetHost)(IVirtualBox *pThis, IHost * *host);

    nsresult (*GetSystemProperties)(IVirtualBox *pThis, ISystemProperties * *systemProperties);

    nsresult (*GetMachines)(IVirtualBox *pThis, PRUint32 *machinesSize, IMachine * **machines);

    nsresult (*GetMachineGroups)(IVirtualBox *pThis, PRUint32 *machineGroupsSize, PRUnichar * **machineGroups);

    nsresult (*GetHardDisks)(IVirtualBox *pThis, PRUint32 *hardDisksSize, IMedium * **hardDisks);

    nsresult (*GetDVDImages)(IVirtualBox *pThis, PRUint32 *DVDImagesSize, IMedium * **DVDImages);

    nsresult (*GetFloppyImages)(IVirtualBox *pThis, PRUint32 *floppyImagesSize, IMedium * **floppyImages);

    nsresult (*GetProgressOperations)(IVirtualBox *pThis, PRUint32 *progressOperationsSize, IProgress * **progressOperations);

    nsresult (*GetGuestOSTypes)(IVirtualBox *pThis, PRUint32 *guestOSTypesSize, IGuestOSType * **guestOSTypes);

    nsresult (*GetSharedFolders)(IVirtualBox *pThis, PRUint32 *sharedFoldersSize, ISharedFolder * **sharedFolders);

    nsresult (*GetPerformanceCollector)(IVirtualBox *pThis, IPerformanceCollector * *performanceCollector);

    nsresult (*GetDHCPServers)(IVirtualBox *pThis, PRUint32 *DHCPServersSize, IDHCPServer * **DHCPServers);

    nsresult (*GetNATNetworks)(IVirtualBox *pThis, PRUint32 *NATNetworksSize, INATNetwork * **NATNetworks);

    nsresult (*GetEventSource)(IVirtualBox *pThis, IEventSource * *eventSource);

    nsresult (*GetExtensionPackManager)(IVirtualBox *pThis, IExtPackManager * *extensionPackManager);

    nsresult (*GetInternalNetworks)(IVirtualBox *pThis, PRUint32 *internalNetworksSize, PRUnichar * **internalNetworks);

    nsresult (*GetGenericNetworkDrivers)(IVirtualBox *pThis, PRUint32 *genericNetworkDriversSize, PRUnichar * **genericNetworkDrivers);

    nsresult (*ComposeMachineFilename)(
        IVirtualBox *pThis,
        PRUnichar * name,
        PRUnichar * group,
        PRUnichar * createFlags,
        PRUnichar * baseFolder,
        PRUnichar * * file
    );

    nsresult (*CreateMachine)(
        IVirtualBox *pThis,
        PRUnichar * settingsFile,
        PRUnichar * name,
        PRUint32 groupsSize,
        PRUnichar ** groups,
        PRUnichar * osTypeId,
        PRUnichar * flags,
        IMachine * * machine
    );

    nsresult (*OpenMachine)(
        IVirtualBox *pThis,
        PRUnichar * settingsFile,
        IMachine * * machine
    );

    nsresult (*RegisterMachine)(
        IVirtualBox *pThis,
        IMachine * machine
    );

    nsresult (*FindMachine)(
        IVirtualBox *pThis,
        PRUnichar * nameOrId,
        IMachine * * machine
    );

    nsresult (*GetMachinesByGroups)(
        IVirtualBox *pThis,
        PRUint32 groupsSize,
        PRUnichar ** groups,
        PRUint32 *machinesSize,
        IMachine *** machines
    );

    nsresult (*GetMachineStates)(
        IVirtualBox *pThis,
        PRUint32 machinesSize,
        IMachine ** machines,
        PRUint32 *statesSize,
        PRUint32** states
    );

    nsresult (*CreateAppliance)(
        IVirtualBox *pThis,
        IAppliance * * appliance
    );

    nsresult (*CreateHardDisk)(
        IVirtualBox *pThis,
        PRUnichar * format,
        PRUnichar * location,
        IMedium * * medium
    );

    nsresult (*OpenMedium)(
        IVirtualBox *pThis,
        PRUnichar * location,
        PRUint32 deviceType,
        PRUint32 accessMode,
        PRBool forceNewUuid,
        IMedium * * medium
    );

    nsresult (*GetGuestOSType)(
        IVirtualBox *pThis,
        PRUnichar * id,
        IGuestOSType * * type
    );

    nsresult (*CreateSharedFolder)(
        IVirtualBox *pThis,
        PRUnichar * name,
        PRUnichar * hostPath,
        PRBool writable,
        PRBool automount
    );

    nsresult (*RemoveSharedFolder)(
        IVirtualBox *pThis,
        PRUnichar * name
    );

    nsresult (*GetExtraDataKeys)(
        IVirtualBox *pThis,
        PRUint32 *keysSize,
        PRUnichar *** keys
    );

    nsresult (*GetExtraData)(
        IVirtualBox *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

    nsresult (*SetExtraData)(
        IVirtualBox *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*SetSettingsSecret)(
        IVirtualBox *pThis,
        PRUnichar * password
    );

    nsresult (*CreateDHCPServer)(
        IVirtualBox *pThis,
        PRUnichar * name,
        IDHCPServer * * server
    );

    nsresult (*FindDHCPServerByNetworkName)(
        IVirtualBox *pThis,
        PRUnichar * name,
        IDHCPServer * * server
    );

    nsresult (*RemoveDHCPServer)(
        IVirtualBox *pThis,
        IDHCPServer * server
    );

    nsresult (*CreateNATNetwork)(
        IVirtualBox *pThis,
        PRUnichar * networkName,
        INATNetwork * * network
    );

    nsresult (*FindNATNetworkByName)(
        IVirtualBox *pThis,
        PRUnichar * networkName,
        INATNetwork * * network
    );

    nsresult (*RemoveNATNetwork)(
        IVirtualBox *pThis,
        INATNetwork * network
    );

    nsresult (*CheckFirmwarePresent)(
        IVirtualBox *pThis,
        PRUint32 firmwareType,
        PRUnichar * version,
        PRUnichar * * url,
        PRUnichar * * file,
        PRBool * result
    );

};
#define IVirtualBox_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVirtualBox_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVirtualBox_Release(p) ((p)->lpVtbl->Release(p))
#define IVirtualBox_get_Version(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IVirtualBox_GetVersion(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IVirtualBox_get_VersionNormalized(p, aVersionNormalized) ((p)->lpVtbl->GetVersionNormalized(p, aVersionNormalized))
#define IVirtualBox_GetVersionNormalized(p, aVersionNormalized) ((p)->lpVtbl->GetVersionNormalized(p, aVersionNormalized))
#define IVirtualBox_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IVirtualBox_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IVirtualBox_get_PackageType(p, aPackageType) ((p)->lpVtbl->GetPackageType(p, aPackageType))
#define IVirtualBox_GetPackageType(p, aPackageType) ((p)->lpVtbl->GetPackageType(p, aPackageType))
#define IVirtualBox_get_APIVersion(p, aAPIVersion) ((p)->lpVtbl->GetAPIVersion(p, aAPIVersion))
#define IVirtualBox_GetAPIVersion(p, aAPIVersion) ((p)->lpVtbl->GetAPIVersion(p, aAPIVersion))
#define IVirtualBox_get_HomeFolder(p, aHomeFolder) ((p)->lpVtbl->GetHomeFolder(p, aHomeFolder))
#define IVirtualBox_GetHomeFolder(p, aHomeFolder) ((p)->lpVtbl->GetHomeFolder(p, aHomeFolder))
#define IVirtualBox_get_SettingsFilePath(p, aSettingsFilePath) ((p)->lpVtbl->GetSettingsFilePath(p, aSettingsFilePath))
#define IVirtualBox_GetSettingsFilePath(p, aSettingsFilePath) ((p)->lpVtbl->GetSettingsFilePath(p, aSettingsFilePath))
#define IVirtualBox_get_Host(p, aHost) ((p)->lpVtbl->GetHost(p, aHost))
#define IVirtualBox_GetHost(p, aHost) ((p)->lpVtbl->GetHost(p, aHost))
#define IVirtualBox_get_SystemProperties(p, aSystemProperties) ((p)->lpVtbl->GetSystemProperties(p, aSystemProperties))
#define IVirtualBox_GetSystemProperties(p, aSystemProperties) ((p)->lpVtbl->GetSystemProperties(p, aSystemProperties))
#define IVirtualBox_get_Machines(p, aMachines) ((p)->lpVtbl->GetMachines(p, aMachines))
#define IVirtualBox_GetMachines(p, aMachines) ((p)->lpVtbl->GetMachines(p, aMachines))
#define IVirtualBox_get_MachineGroups(p, aMachineGroups) ((p)->lpVtbl->GetMachineGroups(p, aMachineGroups))
#define IVirtualBox_GetMachineGroups(p, aMachineGroups) ((p)->lpVtbl->GetMachineGroups(p, aMachineGroups))
#define IVirtualBox_get_HardDisks(p, aHardDisks) ((p)->lpVtbl->GetHardDisks(p, aHardDisks))
#define IVirtualBox_GetHardDisks(p, aHardDisks) ((p)->lpVtbl->GetHardDisks(p, aHardDisks))
#define IVirtualBox_get_DVDImages(p, aDVDImages) ((p)->lpVtbl->GetDVDImages(p, aDVDImages))
#define IVirtualBox_GetDVDImages(p, aDVDImages) ((p)->lpVtbl->GetDVDImages(p, aDVDImages))
#define IVirtualBox_get_FloppyImages(p, aFloppyImages) ((p)->lpVtbl->GetFloppyImages(p, aFloppyImages))
#define IVirtualBox_GetFloppyImages(p, aFloppyImages) ((p)->lpVtbl->GetFloppyImages(p, aFloppyImages))
#define IVirtualBox_get_ProgressOperations(p, aProgressOperations) ((p)->lpVtbl->GetProgressOperations(p, aProgressOperations))
#define IVirtualBox_GetProgressOperations(p, aProgressOperations) ((p)->lpVtbl->GetProgressOperations(p, aProgressOperations))
#define IVirtualBox_get_GuestOSTypes(p, aGuestOSTypes) ((p)->lpVtbl->GetGuestOSTypes(p, aGuestOSTypes))
#define IVirtualBox_GetGuestOSTypes(p, aGuestOSTypes) ((p)->lpVtbl->GetGuestOSTypes(p, aGuestOSTypes))
#define IVirtualBox_get_SharedFolders(p, aSharedFolders) ((p)->lpVtbl->GetSharedFolders(p, aSharedFolders))
#define IVirtualBox_GetSharedFolders(p, aSharedFolders) ((p)->lpVtbl->GetSharedFolders(p, aSharedFolders))
#define IVirtualBox_get_PerformanceCollector(p, aPerformanceCollector) ((p)->lpVtbl->GetPerformanceCollector(p, aPerformanceCollector))
#define IVirtualBox_GetPerformanceCollector(p, aPerformanceCollector) ((p)->lpVtbl->GetPerformanceCollector(p, aPerformanceCollector))
#define IVirtualBox_get_DHCPServers(p, aDHCPServers) ((p)->lpVtbl->GetDHCPServers(p, aDHCPServers))
#define IVirtualBox_GetDHCPServers(p, aDHCPServers) ((p)->lpVtbl->GetDHCPServers(p, aDHCPServers))
#define IVirtualBox_get_NATNetworks(p, aNATNetworks) ((p)->lpVtbl->GetNATNetworks(p, aNATNetworks))
#define IVirtualBox_GetNATNetworks(p, aNATNetworks) ((p)->lpVtbl->GetNATNetworks(p, aNATNetworks))
#define IVirtualBox_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IVirtualBox_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IVirtualBox_get_ExtensionPackManager(p, aExtensionPackManager) ((p)->lpVtbl->GetExtensionPackManager(p, aExtensionPackManager))
#define IVirtualBox_GetExtensionPackManager(p, aExtensionPackManager) ((p)->lpVtbl->GetExtensionPackManager(p, aExtensionPackManager))
#define IVirtualBox_get_InternalNetworks(p, aInternalNetworks) ((p)->lpVtbl->GetInternalNetworks(p, aInternalNetworks))
#define IVirtualBox_GetInternalNetworks(p, aInternalNetworks) ((p)->lpVtbl->GetInternalNetworks(p, aInternalNetworks))
#define IVirtualBox_get_GenericNetworkDrivers(p, aGenericNetworkDrivers) ((p)->lpVtbl->GetGenericNetworkDrivers(p, aGenericNetworkDrivers))
#define IVirtualBox_GetGenericNetworkDrivers(p, aGenericNetworkDrivers) ((p)->lpVtbl->GetGenericNetworkDrivers(p, aGenericNetworkDrivers))
#define IVirtualBox_ComposeMachineFilename(p, aName, aGroup, aCreateFlags, aBaseFolder, aFile) ((p)->lpVtbl->ComposeMachineFilename(p, aName, aGroup, aCreateFlags, aBaseFolder, aFile))
#define IVirtualBox_CreateMachine(p, aSettingsFile, aName, aGroups, aOsTypeId, aFlags, aMachine) ((p)->lpVtbl->CreateMachine(p, aSettingsFile, aName, aGroups, aOsTypeId, aFlags, aMachine))
#define IVirtualBox_OpenMachine(p, aSettingsFile, aMachine) ((p)->lpVtbl->OpenMachine(p, aSettingsFile, aMachine))
#define IVirtualBox_RegisterMachine(p, aMachine) ((p)->lpVtbl->RegisterMachine(p, aMachine))
#define IVirtualBox_FindMachine(p, aNameOrId, aMachine) ((p)->lpVtbl->FindMachine(p, aNameOrId, aMachine))
#define IVirtualBox_GetMachinesByGroups(p, aGroups, aMachines) ((p)->lpVtbl->GetMachinesByGroups(p, aGroups, aMachines))
#define IVirtualBox_GetMachineStates(p, aMachines, aStates) ((p)->lpVtbl->GetMachineStates(p, aMachines, aStates))
#define IVirtualBox_CreateAppliance(p, aAppliance) ((p)->lpVtbl->CreateAppliance(p, aAppliance))
#define IVirtualBox_CreateHardDisk(p, aFormat, aLocation, aMedium) ((p)->lpVtbl->CreateHardDisk(p, aFormat, aLocation, aMedium))
#define IVirtualBox_OpenMedium(p, aLocation, aDeviceType, aAccessMode, aForceNewUuid, aMedium) ((p)->lpVtbl->OpenMedium(p, aLocation, aDeviceType, aAccessMode, aForceNewUuid, aMedium))
#define IVirtualBox_GetGuestOSType(p, aId, aType) ((p)->lpVtbl->GetGuestOSType(p, aId, aType))
#define IVirtualBox_CreateSharedFolder(p, aName, aHostPath, aWritable, aAutomount) ((p)->lpVtbl->CreateSharedFolder(p, aName, aHostPath, aWritable, aAutomount))
#define IVirtualBox_RemoveSharedFolder(p, aName) ((p)->lpVtbl->RemoveSharedFolder(p, aName))
#define IVirtualBox_GetExtraDataKeys(p, aKeys) ((p)->lpVtbl->GetExtraDataKeys(p, aKeys))
#define IVirtualBox_GetExtraData(p, aKey, aValue) ((p)->lpVtbl->GetExtraData(p, aKey, aValue))
#define IVirtualBox_SetExtraData(p, aKey, aValue) ((p)->lpVtbl->SetExtraData(p, aKey, aValue))
#define IVirtualBox_SetSettingsSecret(p, aPassword) ((p)->lpVtbl->SetSettingsSecret(p, aPassword))
#define IVirtualBox_CreateDHCPServer(p, aName, aServer) ((p)->lpVtbl->CreateDHCPServer(p, aName, aServer))
#define IVirtualBox_FindDHCPServerByNetworkName(p, aName, aServer) ((p)->lpVtbl->FindDHCPServerByNetworkName(p, aName, aServer))
#define IVirtualBox_RemoveDHCPServer(p, aServer) ((p)->lpVtbl->RemoveDHCPServer(p, aServer))
#define IVirtualBox_CreateNATNetwork(p, aNetworkName, aNetwork) ((p)->lpVtbl->CreateNATNetwork(p, aNetworkName, aNetwork))
#define IVirtualBox_FindNATNetworkByName(p, aNetworkName, aNetwork) ((p)->lpVtbl->FindNATNetworkByName(p, aNetworkName, aNetwork))
#define IVirtualBox_RemoveNATNetwork(p, aNetwork) ((p)->lpVtbl->RemoveNATNetwork(p, aNetwork))
#define IVirtualBox_CheckFirmwarePresent(p, aFirmwareType, aVersion, aUrl, aFile, aResult) ((p)->lpVtbl->CheckFirmwarePresent(p, aFirmwareType, aVersion, aUrl, aFile, aResult))
#endif /* VBOX_WITH_GLUE */

interface IVirtualBox
{
#ifndef VBOX_WITH_GLUE
    struct IVirtualBox_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVirtualBoxVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVirtualBox declaration */


/* Start of struct IVFSExplorer declaration */
#define IVFSEXPLORER_IID_STR "fb220201-2fd3-47e2-a5dc-2c2431d833cc"
#define IVFSEXPLORER_IID { \
    0xfb220201, 0x2fd3, 0x47e2, \
    { 0xa5, 0xdc, 0x2c, 0x24, 0x31, 0xd8, 0x33, 0xcc } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVFSExplorer);
#ifndef VBOX_WITH_GLUE
struct IVFSExplorer_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetPath)(IVFSExplorer *pThis, PRUnichar * *path);

    nsresult (*GetType)(IVFSExplorer *pThis, PRUint32 *type);

    nsresult (*Update)(
        IVFSExplorer *pThis,
        IProgress * * progress
    );

    nsresult (*Cd)(
        IVFSExplorer *pThis,
        PRUnichar * dir,
        IProgress * * progress
    );

    nsresult (*CdUp)(
        IVFSExplorer *pThis,
        IProgress * * progress
    );

    nsresult (*EntryList)(
        IVFSExplorer *pThis,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *sizesSize,
        PRInt64** sizes,
        PRUint32 *modesSize,
        PRUint32** modes
    );

    nsresult (*Exists)(
        IVFSExplorer *pThis,
        PRUint32 namesSize,
        PRUnichar ** names,
        PRUint32 *existsSize,
        PRUnichar *** exists
    );

    nsresult (*Remove)(
        IVFSExplorer *pThis,
        PRUint32 namesSize,
        PRUnichar ** names,
        IProgress * * progress
    );

};
#else /* VBOX_WITH_GLUE */
struct IVFSExplorerVtbl
{
    nsresult (*QueryInterface)(IVFSExplorer *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVFSExplorer *pThis);
    nsrefcnt (*Release)(IVFSExplorer *pThis);
    nsresult (*GetPath)(IVFSExplorer *pThis, PRUnichar * *path);

    nsresult (*GetType)(IVFSExplorer *pThis, PRUint32 *type);

    nsresult (*Update)(
        IVFSExplorer *pThis,
        IProgress * * progress
    );

    nsresult (*Cd)(
        IVFSExplorer *pThis,
        PRUnichar * dir,
        IProgress * * progress
    );

    nsresult (*CdUp)(
        IVFSExplorer *pThis,
        IProgress * * progress
    );

    nsresult (*EntryList)(
        IVFSExplorer *pThis,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *sizesSize,
        PRInt64** sizes,
        PRUint32 *modesSize,
        PRUint32** modes
    );

    nsresult (*Exists)(
        IVFSExplorer *pThis,
        PRUint32 namesSize,
        PRUnichar ** names,
        PRUint32 *existsSize,
        PRUnichar *** exists
    );

    nsresult (*Remove)(
        IVFSExplorer *pThis,
        PRUint32 namesSize,
        PRUnichar ** names,
        IProgress * * progress
    );

};
#define IVFSExplorer_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVFSExplorer_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVFSExplorer_Release(p) ((p)->lpVtbl->Release(p))
#define IVFSExplorer_get_Path(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IVFSExplorer_GetPath(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IVFSExplorer_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVFSExplorer_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVFSExplorer_Update(p, aProgress) ((p)->lpVtbl->Update(p, aProgress))
#define IVFSExplorer_Cd(p, aDir, aProgress) ((p)->lpVtbl->Cd(p, aDir, aProgress))
#define IVFSExplorer_CdUp(p, aProgress) ((p)->lpVtbl->CdUp(p, aProgress))
#define IVFSExplorer_EntryList(p, aNames, aTypes, aSizes, aModes) ((p)->lpVtbl->EntryList(p, aNames, aTypes, aSizes, aModes))
#define IVFSExplorer_Exists(p, aNames, aExists) ((p)->lpVtbl->Exists(p, aNames, aExists))
#define IVFSExplorer_Remove(p, aNames, aProgress) ((p)->lpVtbl->Remove(p, aNames, aProgress))
#endif /* VBOX_WITH_GLUE */

interface IVFSExplorer
{
#ifndef VBOX_WITH_GLUE
    struct IVFSExplorer_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVFSExplorerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVFSExplorer declaration */


/* Start of struct IAppliance declaration */
#define IAPPLIANCE_IID_STR "3059cf9e-25c7-4f0b-9fa5-3c42e441670b"
#define IAPPLIANCE_IID { \
    0x3059cf9e, 0x25c7, 0x4f0b, \
    { 0x9f, 0xa5, 0x3c, 0x42, 0xe4, 0x41, 0x67, 0x0b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IAppliance);
#ifndef VBOX_WITH_GLUE
struct IAppliance_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetPath)(IAppliance *pThis, PRUnichar * *path);

    nsresult (*GetDisks)(IAppliance *pThis, PRUint32 *disksSize, PRUnichar * **disks);

    nsresult (*GetVirtualSystemDescriptions)(IAppliance *pThis, PRUint32 *virtualSystemDescriptionsSize, IVirtualSystemDescription * **virtualSystemDescriptions);

    nsresult (*GetMachines)(IAppliance *pThis, PRUint32 *machinesSize, PRUnichar * **machines);

    nsresult (*Read)(
        IAppliance *pThis,
        PRUnichar * file,
        IProgress * * progress
    );

    nsresult (*Interpret)(IAppliance *pThis );

    nsresult (*ImportMachines)(
        IAppliance *pThis,
        PRUint32 optionsSize,
        PRUint32* options,
        IProgress * * progress
    );

    nsresult (*CreateVFSExplorer)(
        IAppliance *pThis,
        PRUnichar * URI,
        IVFSExplorer * * explorer
    );

    nsresult (*Write)(
        IAppliance *pThis,
        PRUnichar * format,
        PRUint32 optionsSize,
        PRUint32* options,
        PRUnichar * path,
        IProgress * * progress
    );

    nsresult (*GetWarnings)(
        IAppliance *pThis,
        PRUint32 *warningsSize,
        PRUnichar *** warnings
    );

};
#else /* VBOX_WITH_GLUE */
struct IApplianceVtbl
{
    nsresult (*QueryInterface)(IAppliance *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IAppliance *pThis);
    nsrefcnt (*Release)(IAppliance *pThis);
    nsresult (*GetPath)(IAppliance *pThis, PRUnichar * *path);

    nsresult (*GetDisks)(IAppliance *pThis, PRUint32 *disksSize, PRUnichar * **disks);

    nsresult (*GetVirtualSystemDescriptions)(IAppliance *pThis, PRUint32 *virtualSystemDescriptionsSize, IVirtualSystemDescription * **virtualSystemDescriptions);

    nsresult (*GetMachines)(IAppliance *pThis, PRUint32 *machinesSize, PRUnichar * **machines);

    nsresult (*Read)(
        IAppliance *pThis,
        PRUnichar * file,
        IProgress * * progress
    );

    nsresult (*Interpret)(IAppliance *pThis );

    nsresult (*ImportMachines)(
        IAppliance *pThis,
        PRUint32 optionsSize,
        PRUint32* options,
        IProgress * * progress
    );

    nsresult (*CreateVFSExplorer)(
        IAppliance *pThis,
        PRUnichar * URI,
        IVFSExplorer * * explorer
    );

    nsresult (*Write)(
        IAppliance *pThis,
        PRUnichar * format,
        PRUint32 optionsSize,
        PRUint32* options,
        PRUnichar * path,
        IProgress * * progress
    );

    nsresult (*GetWarnings)(
        IAppliance *pThis,
        PRUint32 *warningsSize,
        PRUnichar *** warnings
    );

};
#define IAppliance_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IAppliance_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IAppliance_Release(p) ((p)->lpVtbl->Release(p))
#define IAppliance_get_Path(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IAppliance_GetPath(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IAppliance_get_Disks(p, aDisks) ((p)->lpVtbl->GetDisks(p, aDisks))
#define IAppliance_GetDisks(p, aDisks) ((p)->lpVtbl->GetDisks(p, aDisks))
#define IAppliance_get_VirtualSystemDescriptions(p, aVirtualSystemDescriptions) ((p)->lpVtbl->GetVirtualSystemDescriptions(p, aVirtualSystemDescriptions))
#define IAppliance_GetVirtualSystemDescriptions(p, aVirtualSystemDescriptions) ((p)->lpVtbl->GetVirtualSystemDescriptions(p, aVirtualSystemDescriptions))
#define IAppliance_get_Machines(p, aMachines) ((p)->lpVtbl->GetMachines(p, aMachines))
#define IAppliance_GetMachines(p, aMachines) ((p)->lpVtbl->GetMachines(p, aMachines))
#define IAppliance_Read(p, aFile, aProgress) ((p)->lpVtbl->Read(p, aFile, aProgress))
#define IAppliance_Interpret(p) ((p)->lpVtbl->Interpret(p))
#define IAppliance_ImportMachines(p, aOptions, aProgress) ((p)->lpVtbl->ImportMachines(p, aOptions, aProgress))
#define IAppliance_CreateVFSExplorer(p, aURI, aExplorer) ((p)->lpVtbl->CreateVFSExplorer(p, aURI, aExplorer))
#define IAppliance_Write(p, aFormat, aOptions, aPath, aProgress) ((p)->lpVtbl->Write(p, aFormat, aOptions, aPath, aProgress))
#define IAppliance_GetWarnings(p, aWarnings) ((p)->lpVtbl->GetWarnings(p, aWarnings))
#endif /* VBOX_WITH_GLUE */

interface IAppliance
{
#ifndef VBOX_WITH_GLUE
    struct IAppliance_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IApplianceVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IAppliance declaration */


/* Start of struct IVirtualSystemDescription declaration */
#define IVIRTUALSYSTEMDESCRIPTION_IID_STR "d7525e6c-531a-4c51-8e04-41235083a3d8"
#define IVIRTUALSYSTEMDESCRIPTION_IID { \
    0xd7525e6c, 0x531a, 0x4c51, \
    { 0x8e, 0x04, 0x41, 0x23, 0x50, 0x83, 0xa3, 0xd8 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVirtualSystemDescription);
#ifndef VBOX_WITH_GLUE
struct IVirtualSystemDescription_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetCount)(IVirtualSystemDescription *pThis, PRUint32 *count);

    nsresult (*GetDescription)(
        IVirtualSystemDescription *pThis,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *refsSize,
        PRUnichar *** refs,
        PRUint32 *OVFValuesSize,
        PRUnichar *** OVFValues,
        PRUint32 *VBoxValuesSize,
        PRUnichar *** VBoxValues,
        PRUint32 *extraConfigValuesSize,
        PRUnichar *** extraConfigValues
    );

    nsresult (*GetDescriptionByType)(
        IVirtualSystemDescription *pThis,
        PRUint32 type,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *refsSize,
        PRUnichar *** refs,
        PRUint32 *OVFValuesSize,
        PRUnichar *** OVFValues,
        PRUint32 *VBoxValuesSize,
        PRUnichar *** VBoxValues,
        PRUint32 *extraConfigValuesSize,
        PRUnichar *** extraConfigValues
    );

    nsresult (*GetValuesByType)(
        IVirtualSystemDescription *pThis,
        PRUint32 type,
        PRUint32 which,
        PRUint32 *valuesSize,
        PRUnichar *** values
    );

    nsresult (*SetFinalValues)(
        IVirtualSystemDescription *pThis,
        PRUint32 enabledSize,
        PRBool* enabled,
        PRUint32 VBoxValuesSize,
        PRUnichar ** VBoxValues,
        PRUint32 extraConfigValuesSize,
        PRUnichar ** extraConfigValues
    );

    nsresult (*AddDescription)(
        IVirtualSystemDescription *pThis,
        PRUint32 type,
        PRUnichar * VBoxValue,
        PRUnichar * extraConfigValue
    );

};
#else /* VBOX_WITH_GLUE */
struct IVirtualSystemDescriptionVtbl
{
    nsresult (*QueryInterface)(IVirtualSystemDescription *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVirtualSystemDescription *pThis);
    nsrefcnt (*Release)(IVirtualSystemDescription *pThis);
    nsresult (*GetCount)(IVirtualSystemDescription *pThis, PRUint32 *count);

    nsresult (*GetDescription)(
        IVirtualSystemDescription *pThis,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *refsSize,
        PRUnichar *** refs,
        PRUint32 *OVFValuesSize,
        PRUnichar *** OVFValues,
        PRUint32 *VBoxValuesSize,
        PRUnichar *** VBoxValues,
        PRUint32 *extraConfigValuesSize,
        PRUnichar *** extraConfigValues
    );

    nsresult (*GetDescriptionByType)(
        IVirtualSystemDescription *pThis,
        PRUint32 type,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *refsSize,
        PRUnichar *** refs,
        PRUint32 *OVFValuesSize,
        PRUnichar *** OVFValues,
        PRUint32 *VBoxValuesSize,
        PRUnichar *** VBoxValues,
        PRUint32 *extraConfigValuesSize,
        PRUnichar *** extraConfigValues
    );

    nsresult (*GetValuesByType)(
        IVirtualSystemDescription *pThis,
        PRUint32 type,
        PRUint32 which,
        PRUint32 *valuesSize,
        PRUnichar *** values
    );

    nsresult (*SetFinalValues)(
        IVirtualSystemDescription *pThis,
        PRUint32 enabledSize,
        PRBool* enabled,
        PRUint32 VBoxValuesSize,
        PRUnichar ** VBoxValues,
        PRUint32 extraConfigValuesSize,
        PRUnichar ** extraConfigValues
    );

    nsresult (*AddDescription)(
        IVirtualSystemDescription *pThis,
        PRUint32 type,
        PRUnichar * VBoxValue,
        PRUnichar * extraConfigValue
    );

};
#define IVirtualSystemDescription_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVirtualSystemDescription_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVirtualSystemDescription_Release(p) ((p)->lpVtbl->Release(p))
#define IVirtualSystemDescription_get_Count(p, aCount) ((p)->lpVtbl->GetCount(p, aCount))
#define IVirtualSystemDescription_GetCount(p, aCount) ((p)->lpVtbl->GetCount(p, aCount))
#define IVirtualSystemDescription_GetDescription(p, aTypes, aRefs, aOVFValues, aVBoxValues, aExtraConfigValues) ((p)->lpVtbl->GetDescription(p, aTypes, aRefs, aOVFValues, aVBoxValues, aExtraConfigValues))
#define IVirtualSystemDescription_GetDescriptionByType(p, aType, aTypes, aRefs, aOVFValues, aVBoxValues, aExtraConfigValues) ((p)->lpVtbl->GetDescriptionByType(p, aType, aTypes, aRefs, aOVFValues, aVBoxValues, aExtraConfigValues))
#define IVirtualSystemDescription_GetValuesByType(p, aType, aWhich, aValues) ((p)->lpVtbl->GetValuesByType(p, aType, aWhich, aValues))
#define IVirtualSystemDescription_SetFinalValues(p, aEnabled, aVBoxValues, aExtraConfigValues) ((p)->lpVtbl->SetFinalValues(p, aEnabled, aVBoxValues, aExtraConfigValues))
#define IVirtualSystemDescription_AddDescription(p, aType, aVBoxValue, aExtraConfigValue) ((p)->lpVtbl->AddDescription(p, aType, aVBoxValue, aExtraConfigValue))
#endif /* VBOX_WITH_GLUE */

interface IVirtualSystemDescription
{
#ifndef VBOX_WITH_GLUE
    struct IVirtualSystemDescription_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVirtualSystemDescriptionVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVirtualSystemDescription declaration */


/* Start of struct IBIOSSettings declaration */
#define IBIOSSETTINGS_IID_STR "38b54279-dc35-4f5e-a431-835b867c6b5e"
#define IBIOSSETTINGS_IID { \
    0x38b54279, 0xdc35, 0x4f5e, \
    { 0xa4, 0x31, 0x83, 0x5b, 0x86, 0x7c, 0x6b, 0x5e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IBIOSSettings);
#ifndef VBOX_WITH_GLUE
struct IBIOSSettings_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetLogoFadeIn)(IBIOSSettings *pThis, PRBool *logoFadeIn);
    nsresult (*SetLogoFadeIn)(IBIOSSettings *pThis, PRBool logoFadeIn);

    nsresult (*GetLogoFadeOut)(IBIOSSettings *pThis, PRBool *logoFadeOut);
    nsresult (*SetLogoFadeOut)(IBIOSSettings *pThis, PRBool logoFadeOut);

    nsresult (*GetLogoDisplayTime)(IBIOSSettings *pThis, PRUint32 *logoDisplayTime);
    nsresult (*SetLogoDisplayTime)(IBIOSSettings *pThis, PRUint32 logoDisplayTime);

    nsresult (*GetLogoImagePath)(IBIOSSettings *pThis, PRUnichar * *logoImagePath);
    nsresult (*SetLogoImagePath)(IBIOSSettings *pThis, PRUnichar * logoImagePath);

    nsresult (*GetBootMenuMode)(IBIOSSettings *pThis, PRUint32 *bootMenuMode);
    nsresult (*SetBootMenuMode)(IBIOSSettings *pThis, PRUint32 bootMenuMode);

    nsresult (*GetACPIEnabled)(IBIOSSettings *pThis, PRBool *ACPIEnabled);
    nsresult (*SetACPIEnabled)(IBIOSSettings *pThis, PRBool ACPIEnabled);

    nsresult (*GetIOAPICEnabled)(IBIOSSettings *pThis, PRBool *IOAPICEnabled);
    nsresult (*SetIOAPICEnabled)(IBIOSSettings *pThis, PRBool IOAPICEnabled);

    nsresult (*GetTimeOffset)(IBIOSSettings *pThis, PRInt64 *timeOffset);
    nsresult (*SetTimeOffset)(IBIOSSettings *pThis, PRInt64 timeOffset);

    nsresult (*GetPXEDebugEnabled)(IBIOSSettings *pThis, PRBool *PXEDebugEnabled);
    nsresult (*SetPXEDebugEnabled)(IBIOSSettings *pThis, PRBool PXEDebugEnabled);

    nsresult (*GetNonVolatileStorageFile)(IBIOSSettings *pThis, PRUnichar * *nonVolatileStorageFile);

};
#else /* VBOX_WITH_GLUE */
struct IBIOSSettingsVtbl
{
    nsresult (*QueryInterface)(IBIOSSettings *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IBIOSSettings *pThis);
    nsrefcnt (*Release)(IBIOSSettings *pThis);
    nsresult (*GetLogoFadeIn)(IBIOSSettings *pThis, PRBool *logoFadeIn);
    nsresult (*SetLogoFadeIn)(IBIOSSettings *pThis, PRBool logoFadeIn);

    nsresult (*GetLogoFadeOut)(IBIOSSettings *pThis, PRBool *logoFadeOut);
    nsresult (*SetLogoFadeOut)(IBIOSSettings *pThis, PRBool logoFadeOut);

    nsresult (*GetLogoDisplayTime)(IBIOSSettings *pThis, PRUint32 *logoDisplayTime);
    nsresult (*SetLogoDisplayTime)(IBIOSSettings *pThis, PRUint32 logoDisplayTime);

    nsresult (*GetLogoImagePath)(IBIOSSettings *pThis, PRUnichar * *logoImagePath);
    nsresult (*SetLogoImagePath)(IBIOSSettings *pThis, PRUnichar * logoImagePath);

    nsresult (*GetBootMenuMode)(IBIOSSettings *pThis, PRUint32 *bootMenuMode);
    nsresult (*SetBootMenuMode)(IBIOSSettings *pThis, PRUint32 bootMenuMode);

    nsresult (*GetACPIEnabled)(IBIOSSettings *pThis, PRBool *ACPIEnabled);
    nsresult (*SetACPIEnabled)(IBIOSSettings *pThis, PRBool ACPIEnabled);

    nsresult (*GetIOAPICEnabled)(IBIOSSettings *pThis, PRBool *IOAPICEnabled);
    nsresult (*SetIOAPICEnabled)(IBIOSSettings *pThis, PRBool IOAPICEnabled);

    nsresult (*GetTimeOffset)(IBIOSSettings *pThis, PRInt64 *timeOffset);
    nsresult (*SetTimeOffset)(IBIOSSettings *pThis, PRInt64 timeOffset);

    nsresult (*GetPXEDebugEnabled)(IBIOSSettings *pThis, PRBool *PXEDebugEnabled);
    nsresult (*SetPXEDebugEnabled)(IBIOSSettings *pThis, PRBool PXEDebugEnabled);

    nsresult (*GetNonVolatileStorageFile)(IBIOSSettings *pThis, PRUnichar * *nonVolatileStorageFile);

};
#define IBIOSSettings_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IBIOSSettings_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IBIOSSettings_Release(p) ((p)->lpVtbl->Release(p))
#define IBIOSSettings_get_LogoFadeIn(p, aLogoFadeIn) ((p)->lpVtbl->GetLogoFadeIn(p, aLogoFadeIn))
#define IBIOSSettings_GetLogoFadeIn(p, aLogoFadeIn) ((p)->lpVtbl->GetLogoFadeIn(p, aLogoFadeIn))
#define IBIOSSettings_put_LogoFadeIn(p, aLogoFadeIn) ((p)->lpVtbl->SetLogoFadeIn(p, aLogoFadeIn))
#define IBIOSSettings_SetLogoFadeIn(p, aLogoFadeIn) ((p)->lpVtbl->SetLogoFadeIn(p, aLogoFadeIn))
#define IBIOSSettings_get_LogoFadeOut(p, aLogoFadeOut) ((p)->lpVtbl->GetLogoFadeOut(p, aLogoFadeOut))
#define IBIOSSettings_GetLogoFadeOut(p, aLogoFadeOut) ((p)->lpVtbl->GetLogoFadeOut(p, aLogoFadeOut))
#define IBIOSSettings_put_LogoFadeOut(p, aLogoFadeOut) ((p)->lpVtbl->SetLogoFadeOut(p, aLogoFadeOut))
#define IBIOSSettings_SetLogoFadeOut(p, aLogoFadeOut) ((p)->lpVtbl->SetLogoFadeOut(p, aLogoFadeOut))
#define IBIOSSettings_get_LogoDisplayTime(p, aLogoDisplayTime) ((p)->lpVtbl->GetLogoDisplayTime(p, aLogoDisplayTime))
#define IBIOSSettings_GetLogoDisplayTime(p, aLogoDisplayTime) ((p)->lpVtbl->GetLogoDisplayTime(p, aLogoDisplayTime))
#define IBIOSSettings_put_LogoDisplayTime(p, aLogoDisplayTime) ((p)->lpVtbl->SetLogoDisplayTime(p, aLogoDisplayTime))
#define IBIOSSettings_SetLogoDisplayTime(p, aLogoDisplayTime) ((p)->lpVtbl->SetLogoDisplayTime(p, aLogoDisplayTime))
#define IBIOSSettings_get_LogoImagePath(p, aLogoImagePath) ((p)->lpVtbl->GetLogoImagePath(p, aLogoImagePath))
#define IBIOSSettings_GetLogoImagePath(p, aLogoImagePath) ((p)->lpVtbl->GetLogoImagePath(p, aLogoImagePath))
#define IBIOSSettings_put_LogoImagePath(p, aLogoImagePath) ((p)->lpVtbl->SetLogoImagePath(p, aLogoImagePath))
#define IBIOSSettings_SetLogoImagePath(p, aLogoImagePath) ((p)->lpVtbl->SetLogoImagePath(p, aLogoImagePath))
#define IBIOSSettings_get_BootMenuMode(p, aBootMenuMode) ((p)->lpVtbl->GetBootMenuMode(p, aBootMenuMode))
#define IBIOSSettings_GetBootMenuMode(p, aBootMenuMode) ((p)->lpVtbl->GetBootMenuMode(p, aBootMenuMode))
#define IBIOSSettings_put_BootMenuMode(p, aBootMenuMode) ((p)->lpVtbl->SetBootMenuMode(p, aBootMenuMode))
#define IBIOSSettings_SetBootMenuMode(p, aBootMenuMode) ((p)->lpVtbl->SetBootMenuMode(p, aBootMenuMode))
#define IBIOSSettings_get_ACPIEnabled(p, aACPIEnabled) ((p)->lpVtbl->GetACPIEnabled(p, aACPIEnabled))
#define IBIOSSettings_GetACPIEnabled(p, aACPIEnabled) ((p)->lpVtbl->GetACPIEnabled(p, aACPIEnabled))
#define IBIOSSettings_put_ACPIEnabled(p, aACPIEnabled) ((p)->lpVtbl->SetACPIEnabled(p, aACPIEnabled))
#define IBIOSSettings_SetACPIEnabled(p, aACPIEnabled) ((p)->lpVtbl->SetACPIEnabled(p, aACPIEnabled))
#define IBIOSSettings_get_IOAPICEnabled(p, aIOAPICEnabled) ((p)->lpVtbl->GetIOAPICEnabled(p, aIOAPICEnabled))
#define IBIOSSettings_GetIOAPICEnabled(p, aIOAPICEnabled) ((p)->lpVtbl->GetIOAPICEnabled(p, aIOAPICEnabled))
#define IBIOSSettings_put_IOAPICEnabled(p, aIOAPICEnabled) ((p)->lpVtbl->SetIOAPICEnabled(p, aIOAPICEnabled))
#define IBIOSSettings_SetIOAPICEnabled(p, aIOAPICEnabled) ((p)->lpVtbl->SetIOAPICEnabled(p, aIOAPICEnabled))
#define IBIOSSettings_get_TimeOffset(p, aTimeOffset) ((p)->lpVtbl->GetTimeOffset(p, aTimeOffset))
#define IBIOSSettings_GetTimeOffset(p, aTimeOffset) ((p)->lpVtbl->GetTimeOffset(p, aTimeOffset))
#define IBIOSSettings_put_TimeOffset(p, aTimeOffset) ((p)->lpVtbl->SetTimeOffset(p, aTimeOffset))
#define IBIOSSettings_SetTimeOffset(p, aTimeOffset) ((p)->lpVtbl->SetTimeOffset(p, aTimeOffset))
#define IBIOSSettings_get_PXEDebugEnabled(p, aPXEDebugEnabled) ((p)->lpVtbl->GetPXEDebugEnabled(p, aPXEDebugEnabled))
#define IBIOSSettings_GetPXEDebugEnabled(p, aPXEDebugEnabled) ((p)->lpVtbl->GetPXEDebugEnabled(p, aPXEDebugEnabled))
#define IBIOSSettings_put_PXEDebugEnabled(p, aPXEDebugEnabled) ((p)->lpVtbl->SetPXEDebugEnabled(p, aPXEDebugEnabled))
#define IBIOSSettings_SetPXEDebugEnabled(p, aPXEDebugEnabled) ((p)->lpVtbl->SetPXEDebugEnabled(p, aPXEDebugEnabled))
#define IBIOSSettings_get_NonVolatileStorageFile(p, aNonVolatileStorageFile) ((p)->lpVtbl->GetNonVolatileStorageFile(p, aNonVolatileStorageFile))
#define IBIOSSettings_GetNonVolatileStorageFile(p, aNonVolatileStorageFile) ((p)->lpVtbl->GetNonVolatileStorageFile(p, aNonVolatileStorageFile))
#endif /* VBOX_WITH_GLUE */

interface IBIOSSettings
{
#ifndef VBOX_WITH_GLUE
    struct IBIOSSettings_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IBIOSSettingsVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IBIOSSettings declaration */


/* Start of struct IPCIAddress declaration */
#define IPCIADDRESS_IID_STR "c984d15f-e191-400b-840e-970f3dad7296"
#define IPCIADDRESS_IID { \
    0xc984d15f, 0xe191, 0x400b, \
    { 0x84, 0x0e, 0x97, 0x0f, 0x3d, 0xad, 0x72, 0x96 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IPCIAddress);
#ifndef VBOX_WITH_GLUE
struct IPCIAddress_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetBus)(IPCIAddress *pThis, PRInt16 *bus);
    nsresult (*SetBus)(IPCIAddress *pThis, PRInt16 bus);

    nsresult (*GetDevice)(IPCIAddress *pThis, PRInt16 *device);
    nsresult (*SetDevice)(IPCIAddress *pThis, PRInt16 device);

    nsresult (*GetDevFunction)(IPCIAddress *pThis, PRInt16 *devFunction);
    nsresult (*SetDevFunction)(IPCIAddress *pThis, PRInt16 devFunction);

    nsresult (*AsLong)(
        IPCIAddress *pThis,
        PRInt32 * result
    );

    nsresult (*FromLong)(
        IPCIAddress *pThis,
        PRInt32 number
    );

};
#else /* VBOX_WITH_GLUE */
struct IPCIAddressVtbl
{
    nsresult (*QueryInterface)(IPCIAddress *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IPCIAddress *pThis);
    nsrefcnt (*Release)(IPCIAddress *pThis);
    nsresult (*GetBus)(IPCIAddress *pThis, PRInt16 *bus);
    nsresult (*SetBus)(IPCIAddress *pThis, PRInt16 bus);

    nsresult (*GetDevice)(IPCIAddress *pThis, PRInt16 *device);
    nsresult (*SetDevice)(IPCIAddress *pThis, PRInt16 device);

    nsresult (*GetDevFunction)(IPCIAddress *pThis, PRInt16 *devFunction);
    nsresult (*SetDevFunction)(IPCIAddress *pThis, PRInt16 devFunction);

    nsresult (*AsLong)(
        IPCIAddress *pThis,
        PRInt32 * result
    );

    nsresult (*FromLong)(
        IPCIAddress *pThis,
        PRInt32 number
    );

};
#define IPCIAddress_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IPCIAddress_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IPCIAddress_Release(p) ((p)->lpVtbl->Release(p))
#define IPCIAddress_get_Bus(p, aBus) ((p)->lpVtbl->GetBus(p, aBus))
#define IPCIAddress_GetBus(p, aBus) ((p)->lpVtbl->GetBus(p, aBus))
#define IPCIAddress_put_Bus(p, aBus) ((p)->lpVtbl->SetBus(p, aBus))
#define IPCIAddress_SetBus(p, aBus) ((p)->lpVtbl->SetBus(p, aBus))
#define IPCIAddress_get_Device(p, aDevice) ((p)->lpVtbl->GetDevice(p, aDevice))
#define IPCIAddress_GetDevice(p, aDevice) ((p)->lpVtbl->GetDevice(p, aDevice))
#define IPCIAddress_put_Device(p, aDevice) ((p)->lpVtbl->SetDevice(p, aDevice))
#define IPCIAddress_SetDevice(p, aDevice) ((p)->lpVtbl->SetDevice(p, aDevice))
#define IPCIAddress_get_DevFunction(p, aDevFunction) ((p)->lpVtbl->GetDevFunction(p, aDevFunction))
#define IPCIAddress_GetDevFunction(p, aDevFunction) ((p)->lpVtbl->GetDevFunction(p, aDevFunction))
#define IPCIAddress_put_DevFunction(p, aDevFunction) ((p)->lpVtbl->SetDevFunction(p, aDevFunction))
#define IPCIAddress_SetDevFunction(p, aDevFunction) ((p)->lpVtbl->SetDevFunction(p, aDevFunction))
#define IPCIAddress_AsLong(p, aResult) ((p)->lpVtbl->AsLong(p, aResult))
#define IPCIAddress_FromLong(p, aNumber) ((p)->lpVtbl->FromLong(p, aNumber))
#endif /* VBOX_WITH_GLUE */

interface IPCIAddress
{
#ifndef VBOX_WITH_GLUE
    struct IPCIAddress_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IPCIAddressVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IPCIAddress declaration */


/* Start of struct IPCIDeviceAttachment declaration */
#define IPCIDEVICEATTACHMENT_IID_STR "91f33d6f-e621-4f70-a77e-15f0e3c714d5"
#define IPCIDEVICEATTACHMENT_IID { \
    0x91f33d6f, 0xe621, 0x4f70, \
    { 0xa7, 0x7e, 0x15, 0xf0, 0xe3, 0xc7, 0x14, 0xd5 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IPCIDeviceAttachment);
#ifndef VBOX_WITH_GLUE
struct IPCIDeviceAttachment_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IPCIDeviceAttachment *pThis, PRUnichar * *name);

    nsresult (*GetIsPhysicalDevice)(IPCIDeviceAttachment *pThis, PRBool *isPhysicalDevice);

    nsresult (*GetHostAddress)(IPCIDeviceAttachment *pThis, PRInt32 *hostAddress);

    nsresult (*GetGuestAddress)(IPCIDeviceAttachment *pThis, PRInt32 *guestAddress);

};
#else /* VBOX_WITH_GLUE */
struct IPCIDeviceAttachmentVtbl
{
    nsresult (*QueryInterface)(IPCIDeviceAttachment *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IPCIDeviceAttachment *pThis);
    nsrefcnt (*Release)(IPCIDeviceAttachment *pThis);
    nsresult (*GetName)(IPCIDeviceAttachment *pThis, PRUnichar * *name);

    nsresult (*GetIsPhysicalDevice)(IPCIDeviceAttachment *pThis, PRBool *isPhysicalDevice);

    nsresult (*GetHostAddress)(IPCIDeviceAttachment *pThis, PRInt32 *hostAddress);

    nsresult (*GetGuestAddress)(IPCIDeviceAttachment *pThis, PRInt32 *guestAddress);

};
#define IPCIDeviceAttachment_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IPCIDeviceAttachment_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IPCIDeviceAttachment_Release(p) ((p)->lpVtbl->Release(p))
#define IPCIDeviceAttachment_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IPCIDeviceAttachment_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IPCIDeviceAttachment_get_IsPhysicalDevice(p, aIsPhysicalDevice) ((p)->lpVtbl->GetIsPhysicalDevice(p, aIsPhysicalDevice))
#define IPCIDeviceAttachment_GetIsPhysicalDevice(p, aIsPhysicalDevice) ((p)->lpVtbl->GetIsPhysicalDevice(p, aIsPhysicalDevice))
#define IPCIDeviceAttachment_get_HostAddress(p, aHostAddress) ((p)->lpVtbl->GetHostAddress(p, aHostAddress))
#define IPCIDeviceAttachment_GetHostAddress(p, aHostAddress) ((p)->lpVtbl->GetHostAddress(p, aHostAddress))
#define IPCIDeviceAttachment_get_GuestAddress(p, aGuestAddress) ((p)->lpVtbl->GetGuestAddress(p, aGuestAddress))
#define IPCIDeviceAttachment_GetGuestAddress(p, aGuestAddress) ((p)->lpVtbl->GetGuestAddress(p, aGuestAddress))
#endif /* VBOX_WITH_GLUE */

interface IPCIDeviceAttachment
{
#ifndef VBOX_WITH_GLUE
    struct IPCIDeviceAttachment_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IPCIDeviceAttachmentVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IPCIDeviceAttachment declaration */


/* Start of struct IMachine declaration */
#define IMACHINE_IID_STR "480cf695-2d8d-4256-9c7c-cce4184fa048"
#define IMACHINE_IID { \
    0x480cf695, 0x2d8d, 0x4256, \
    { 0x9c, 0x7c, 0xcc, 0xe4, 0x18, 0x4f, 0xa0, 0x48 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMachine);
#ifndef VBOX_WITH_GLUE
struct IMachine_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetParent)(IMachine *pThis, IVirtualBox * *parent);

    nsresult (*GetIcon)(IMachine *pThis, PRUint32 *iconSize, PRUint8 **icon);
    nsresult (*SetIcon)(IMachine *pThis, PRUint32 iconSize, PRUint8 *icon);

    nsresult (*GetAccessible)(IMachine *pThis, PRBool *accessible);

    nsresult (*GetAccessError)(IMachine *pThis, IVirtualBoxErrorInfo * *accessError);

    nsresult (*GetName)(IMachine *pThis, PRUnichar * *name);
    nsresult (*SetName)(IMachine *pThis, PRUnichar * name);

    nsresult (*GetDescription)(IMachine *pThis, PRUnichar * *description);
    nsresult (*SetDescription)(IMachine *pThis, PRUnichar * description);

    nsresult (*GetId)(IMachine *pThis, PRUnichar * *id);

    nsresult (*GetGroups)(IMachine *pThis, PRUint32 *groupsSize, PRUnichar * **groups);
    nsresult (*SetGroups)(IMachine *pThis, PRUint32 groupsSize, PRUnichar * *groups);

    nsresult (*GetOSTypeId)(IMachine *pThis, PRUnichar * *OSTypeId);
    nsresult (*SetOSTypeId)(IMachine *pThis, PRUnichar * OSTypeId);

    nsresult (*GetHardwareVersion)(IMachine *pThis, PRUnichar * *hardwareVersion);
    nsresult (*SetHardwareVersion)(IMachine *pThis, PRUnichar * hardwareVersion);

    nsresult (*GetHardwareUUID)(IMachine *pThis, PRUnichar * *hardwareUUID);
    nsresult (*SetHardwareUUID)(IMachine *pThis, PRUnichar * hardwareUUID);

    nsresult (*GetCPUCount)(IMachine *pThis, PRUint32 *CPUCount);
    nsresult (*SetCPUCount)(IMachine *pThis, PRUint32 CPUCount);

    nsresult (*GetCPUHotPlugEnabled)(IMachine *pThis, PRBool *CPUHotPlugEnabled);
    nsresult (*SetCPUHotPlugEnabled)(IMachine *pThis, PRBool CPUHotPlugEnabled);

    nsresult (*GetCPUExecutionCap)(IMachine *pThis, PRUint32 *CPUExecutionCap);
    nsresult (*SetCPUExecutionCap)(IMachine *pThis, PRUint32 CPUExecutionCap);

    nsresult (*GetMemorySize)(IMachine *pThis, PRUint32 *memorySize);
    nsresult (*SetMemorySize)(IMachine *pThis, PRUint32 memorySize);

    nsresult (*GetMemoryBalloonSize)(IMachine *pThis, PRUint32 *memoryBalloonSize);
    nsresult (*SetMemoryBalloonSize)(IMachine *pThis, PRUint32 memoryBalloonSize);

    nsresult (*GetPageFusionEnabled)(IMachine *pThis, PRBool *pageFusionEnabled);
    nsresult (*SetPageFusionEnabled)(IMachine *pThis, PRBool pageFusionEnabled);

    nsresult (*GetGraphicsControllerType)(IMachine *pThis, PRUint32 *graphicsControllerType);
    nsresult (*SetGraphicsControllerType)(IMachine *pThis, PRUint32 graphicsControllerType);

    nsresult (*GetVRAMSize)(IMachine *pThis, PRUint32 *VRAMSize);
    nsresult (*SetVRAMSize)(IMachine *pThis, PRUint32 VRAMSize);

    nsresult (*GetAccelerate3DEnabled)(IMachine *pThis, PRBool *accelerate3DEnabled);
    nsresult (*SetAccelerate3DEnabled)(IMachine *pThis, PRBool accelerate3DEnabled);

    nsresult (*GetAccelerate2DVideoEnabled)(IMachine *pThis, PRBool *accelerate2DVideoEnabled);
    nsresult (*SetAccelerate2DVideoEnabled)(IMachine *pThis, PRBool accelerate2DVideoEnabled);

    nsresult (*GetMonitorCount)(IMachine *pThis, PRUint32 *monitorCount);
    nsresult (*SetMonitorCount)(IMachine *pThis, PRUint32 monitorCount);

    nsresult (*GetVideoCaptureEnabled)(IMachine *pThis, PRBool *videoCaptureEnabled);
    nsresult (*SetVideoCaptureEnabled)(IMachine *pThis, PRBool videoCaptureEnabled);

    nsresult (*GetVideoCaptureScreens)(IMachine *pThis, PRUint32 *videoCaptureScreensSize, PRBool **videoCaptureScreens);
    nsresult (*SetVideoCaptureScreens)(IMachine *pThis, PRUint32 videoCaptureScreensSize, PRBool *videoCaptureScreens);

    nsresult (*GetVideoCaptureFile)(IMachine *pThis, PRUnichar * *videoCaptureFile);
    nsresult (*SetVideoCaptureFile)(IMachine *pThis, PRUnichar * videoCaptureFile);

    nsresult (*GetVideoCaptureWidth)(IMachine *pThis, PRUint32 *videoCaptureWidth);
    nsresult (*SetVideoCaptureWidth)(IMachine *pThis, PRUint32 videoCaptureWidth);

    nsresult (*GetVideoCaptureHeight)(IMachine *pThis, PRUint32 *videoCaptureHeight);
    nsresult (*SetVideoCaptureHeight)(IMachine *pThis, PRUint32 videoCaptureHeight);

    nsresult (*GetVideoCaptureRate)(IMachine *pThis, PRUint32 *videoCaptureRate);
    nsresult (*SetVideoCaptureRate)(IMachine *pThis, PRUint32 videoCaptureRate);

    nsresult (*GetVideoCaptureFPS)(IMachine *pThis, PRUint32 *videoCaptureFPS);
    nsresult (*SetVideoCaptureFPS)(IMachine *pThis, PRUint32 videoCaptureFPS);

    nsresult (*GetBIOSSettings)(IMachine *pThis, IBIOSSettings * *BIOSSettings);

    nsresult (*GetFirmwareType)(IMachine *pThis, PRUint32 *firmwareType);
    nsresult (*SetFirmwareType)(IMachine *pThis, PRUint32 firmwareType);

    nsresult (*GetPointingHIDType)(IMachine *pThis, PRUint32 *pointingHIDType);
    nsresult (*SetPointingHIDType)(IMachine *pThis, PRUint32 pointingHIDType);

    nsresult (*GetKeyboardHIDType)(IMachine *pThis, PRUint32 *keyboardHIDType);
    nsresult (*SetKeyboardHIDType)(IMachine *pThis, PRUint32 keyboardHIDType);

    nsresult (*GetHPETEnabled)(IMachine *pThis, PRBool *HPETEnabled);
    nsresult (*SetHPETEnabled)(IMachine *pThis, PRBool HPETEnabled);

    nsresult (*GetChipsetType)(IMachine *pThis, PRUint32 *chipsetType);
    nsresult (*SetChipsetType)(IMachine *pThis, PRUint32 chipsetType);

    nsresult (*GetSnapshotFolder)(IMachine *pThis, PRUnichar * *snapshotFolder);
    nsresult (*SetSnapshotFolder)(IMachine *pThis, PRUnichar * snapshotFolder);

    nsresult (*GetVRDEServer)(IMachine *pThis, IVRDEServer * *VRDEServer);

    nsresult (*GetEmulatedUSBCardReaderEnabled)(IMachine *pThis, PRBool *emulatedUSBCardReaderEnabled);
    nsresult (*SetEmulatedUSBCardReaderEnabled)(IMachine *pThis, PRBool emulatedUSBCardReaderEnabled);

    nsresult (*GetMediumAttachments)(IMachine *pThis, PRUint32 *mediumAttachmentsSize, IMediumAttachment * **mediumAttachments);

    nsresult (*GetUSBControllers)(IMachine *pThis, PRUint32 *USBControllersSize, IUSBController * **USBControllers);

    nsresult (*GetUSBDeviceFilters)(IMachine *pThis, IUSBDeviceFilters * *USBDeviceFilters);

    nsresult (*GetAudioAdapter)(IMachine *pThis, IAudioAdapter * *audioAdapter);

    nsresult (*GetStorageControllers)(IMachine *pThis, PRUint32 *storageControllersSize, IStorageController * **storageControllers);

    nsresult (*GetSettingsFilePath)(IMachine *pThis, PRUnichar * *settingsFilePath);

    nsresult (*GetSettingsModified)(IMachine *pThis, PRBool *settingsModified);

    nsresult (*GetSessionState)(IMachine *pThis, PRUint32 *sessionState);

    nsresult (*GetSessionType)(IMachine *pThis, PRUnichar * *sessionType);

    nsresult (*GetSessionPID)(IMachine *pThis, PRUint32 *sessionPID);

    nsresult (*GetState)(IMachine *pThis, PRUint32 *state);

    nsresult (*GetLastStateChange)(IMachine *pThis, PRInt64 *lastStateChange);

    nsresult (*GetStateFilePath)(IMachine *pThis, PRUnichar * *stateFilePath);

    nsresult (*GetLogFolder)(IMachine *pThis, PRUnichar * *logFolder);

    nsresult (*GetCurrentSnapshot)(IMachine *pThis, ISnapshot * *currentSnapshot);

    nsresult (*GetSnapshotCount)(IMachine *pThis, PRUint32 *snapshotCount);

    nsresult (*GetCurrentStateModified)(IMachine *pThis, PRBool *currentStateModified);

    nsresult (*GetSharedFolders)(IMachine *pThis, PRUint32 *sharedFoldersSize, ISharedFolder * **sharedFolders);

    nsresult (*GetClipboardMode)(IMachine *pThis, PRUint32 *clipboardMode);
    nsresult (*SetClipboardMode)(IMachine *pThis, PRUint32 clipboardMode);

    nsresult (*GetDragAndDropMode)(IMachine *pThis, PRUint32 *dragAndDropMode);
    nsresult (*SetDragAndDropMode)(IMachine *pThis, PRUint32 dragAndDropMode);

    nsresult (*GetGuestPropertyNotificationPatterns)(IMachine *pThis, PRUnichar * *guestPropertyNotificationPatterns);
    nsresult (*SetGuestPropertyNotificationPatterns)(IMachine *pThis, PRUnichar * guestPropertyNotificationPatterns);

    nsresult (*GetTeleporterEnabled)(IMachine *pThis, PRBool *teleporterEnabled);
    nsresult (*SetTeleporterEnabled)(IMachine *pThis, PRBool teleporterEnabled);

    nsresult (*GetTeleporterPort)(IMachine *pThis, PRUint32 *teleporterPort);
    nsresult (*SetTeleporterPort)(IMachine *pThis, PRUint32 teleporterPort);

    nsresult (*GetTeleporterAddress)(IMachine *pThis, PRUnichar * *teleporterAddress);
    nsresult (*SetTeleporterAddress)(IMachine *pThis, PRUnichar * teleporterAddress);

    nsresult (*GetTeleporterPassword)(IMachine *pThis, PRUnichar * *teleporterPassword);
    nsresult (*SetTeleporterPassword)(IMachine *pThis, PRUnichar * teleporterPassword);

    nsresult (*GetFaultToleranceState)(IMachine *pThis, PRUint32 *faultToleranceState);
    nsresult (*SetFaultToleranceState)(IMachine *pThis, PRUint32 faultToleranceState);

    nsresult (*GetFaultTolerancePort)(IMachine *pThis, PRUint32 *faultTolerancePort);
    nsresult (*SetFaultTolerancePort)(IMachine *pThis, PRUint32 faultTolerancePort);

    nsresult (*GetFaultToleranceAddress)(IMachine *pThis, PRUnichar * *faultToleranceAddress);
    nsresult (*SetFaultToleranceAddress)(IMachine *pThis, PRUnichar * faultToleranceAddress);

    nsresult (*GetFaultTolerancePassword)(IMachine *pThis, PRUnichar * *faultTolerancePassword);
    nsresult (*SetFaultTolerancePassword)(IMachine *pThis, PRUnichar * faultTolerancePassword);

    nsresult (*GetFaultToleranceSyncInterval)(IMachine *pThis, PRUint32 *faultToleranceSyncInterval);
    nsresult (*SetFaultToleranceSyncInterval)(IMachine *pThis, PRUint32 faultToleranceSyncInterval);

    nsresult (*GetRTCUseUTC)(IMachine *pThis, PRBool *RTCUseUTC);
    nsresult (*SetRTCUseUTC)(IMachine *pThis, PRBool RTCUseUTC);

    nsresult (*GetIOCacheEnabled)(IMachine *pThis, PRBool *IOCacheEnabled);
    nsresult (*SetIOCacheEnabled)(IMachine *pThis, PRBool IOCacheEnabled);

    nsresult (*GetIOCacheSize)(IMachine *pThis, PRUint32 *IOCacheSize);
    nsresult (*SetIOCacheSize)(IMachine *pThis, PRUint32 IOCacheSize);

    nsresult (*GetPCIDeviceAssignments)(IMachine *pThis, PRUint32 *PCIDeviceAssignmentsSize, IPCIDeviceAttachment * **PCIDeviceAssignments);

    nsresult (*GetBandwidthControl)(IMachine *pThis, IBandwidthControl * *bandwidthControl);

    nsresult (*GetTracingEnabled)(IMachine *pThis, PRBool *tracingEnabled);
    nsresult (*SetTracingEnabled)(IMachine *pThis, PRBool tracingEnabled);

    nsresult (*GetTracingConfig)(IMachine *pThis, PRUnichar * *tracingConfig);
    nsresult (*SetTracingConfig)(IMachine *pThis, PRUnichar * tracingConfig);

    nsresult (*GetAllowTracingToAccessVM)(IMachine *pThis, PRBool *allowTracingToAccessVM);
    nsresult (*SetAllowTracingToAccessVM)(IMachine *pThis, PRBool allowTracingToAccessVM);

    nsresult (*GetAutostartEnabled)(IMachine *pThis, PRBool *autostartEnabled);
    nsresult (*SetAutostartEnabled)(IMachine *pThis, PRBool autostartEnabled);

    nsresult (*GetAutostartDelay)(IMachine *pThis, PRUint32 *autostartDelay);
    nsresult (*SetAutostartDelay)(IMachine *pThis, PRUint32 autostartDelay);

    nsresult (*GetAutostopType)(IMachine *pThis, PRUint32 *autostopType);
    nsresult (*SetAutostopType)(IMachine *pThis, PRUint32 autostopType);

    nsresult (*GetDefaultFrontend)(IMachine *pThis, PRUnichar * *defaultFrontend);
    nsresult (*SetDefaultFrontend)(IMachine *pThis, PRUnichar * defaultFrontend);

    nsresult (*GetUSBProxyAvailable)(IMachine *pThis, PRBool *USBProxyAvailable);

    nsresult (*LockMachine)(
        IMachine *pThis,
        ISession * session,
        PRUint32 lockType
    );

    nsresult (*LaunchVMProcess)(
        IMachine *pThis,
        ISession * session,
        PRUnichar * type,
        PRUnichar * environment,
        IProgress * * progress
    );

    nsresult (*SetBootOrder)(
        IMachine *pThis,
        PRUint32 position,
        PRUint32 device
    );

    nsresult (*GetBootOrder)(
        IMachine *pThis,
        PRUint32 position,
        PRUint32 * device
    );

    nsresult (*AttachDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRUint32 type,
        IMedium * medium
    );

    nsresult (*AttachDeviceWithoutMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRUint32 type
    );

    nsresult (*DetachDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device
    );

    nsresult (*PassthroughDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool passthrough
    );

    nsresult (*TemporaryEjectDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool temporaryEject
    );

    nsresult (*NonRotationalDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool nonRotational
    );

    nsresult (*SetAutoDiscardForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool discard
    );

    nsresult (*SetHotPluggableForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool hotPluggable
    );

    nsresult (*SetBandwidthGroupForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IBandwidthGroup * bandwidthGroup
    );

    nsresult (*SetNoBandwidthGroupForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device
    );

    nsresult (*UnmountMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool force
    );

    nsresult (*MountMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IMedium * medium,
        PRBool force
    );

    nsresult (*GetMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IMedium * * medium
    );

    nsresult (*GetMediumAttachmentsOfController)(
        IMachine *pThis,
        PRUnichar * name,
        PRUint32 *mediumAttachmentsSize,
        IMediumAttachment *** mediumAttachments
    );

    nsresult (*GetMediumAttachment)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IMediumAttachment * * attachment
    );

    nsresult (*AttachHostPCIDevice)(
        IMachine *pThis,
        PRInt32 hostAddress,
        PRInt32 desiredGuestAddress,
        PRBool tryToUnbind
    );

    nsresult (*DetachHostPCIDevice)(
        IMachine *pThis,
        PRInt32 hostAddress
    );

    nsresult (*GetNetworkAdapter)(
        IMachine *pThis,
        PRUint32 slot,
        INetworkAdapter * * adapter
    );

    nsresult (*AddStorageController)(
        IMachine *pThis,
        PRUnichar * name,
        PRUint32 connectionType,
        IStorageController * * controller
    );

    nsresult (*GetStorageControllerByName)(
        IMachine *pThis,
        PRUnichar * name,
        IStorageController * * storageController
    );

    nsresult (*GetStorageControllerByInstance)(
        IMachine *pThis,
        PRUint32 instance,
        IStorageController * * storageController
    );

    nsresult (*RemoveStorageController)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*SetStorageControllerBootable)(
        IMachine *pThis,
        PRUnichar * name,
        PRBool bootable
    );

    nsresult (*AddUSBController)(
        IMachine *pThis,
        PRUnichar * name,
        PRUint32 type,
        IUSBController * * controller
    );

    nsresult (*RemoveUSBController)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*GetUSBControllerByName)(
        IMachine *pThis,
        PRUnichar * name,
        IUSBController * * controller
    );

    nsresult (*GetUSBControllerCountByType)(
        IMachine *pThis,
        PRUint32 type,
        PRUint32 * controllers
    );

    nsresult (*GetSerialPort)(
        IMachine *pThis,
        PRUint32 slot,
        ISerialPort * * port
    );

    nsresult (*GetParallelPort)(
        IMachine *pThis,
        PRUint32 slot,
        IParallelPort * * port
    );

    nsresult (*GetExtraDataKeys)(
        IMachine *pThis,
        PRUint32 *keysSize,
        PRUnichar *** keys
    );

    nsresult (*GetExtraData)(
        IMachine *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

    nsresult (*SetExtraData)(
        IMachine *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*GetCPUProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool * value
    );

    nsresult (*SetCPUProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool value
    );

    nsresult (*GetCPUIDLeaf)(
        IMachine *pThis,
        PRUint32 id,
        PRUint32 * valEax,
        PRUint32 * valEbx,
        PRUint32 * valEcx,
        PRUint32 * valEdx
    );

    nsresult (*SetCPUIDLeaf)(
        IMachine *pThis,
        PRUint32 id,
        PRUint32 valEax,
        PRUint32 valEbx,
        PRUint32 valEcx,
        PRUint32 valEdx
    );

    nsresult (*RemoveCPUIDLeaf)(
        IMachine *pThis,
        PRUint32 id
    );

    nsresult (*RemoveAllCPUIDLeaves)(IMachine *pThis );

    nsresult (*GetHWVirtExProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool * value
    );

    nsresult (*SetHWVirtExProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool value
    );

    nsresult (*SetSettingsFilePath)(
        IMachine *pThis,
        PRUnichar * settingsFilePath,
        IProgress * * progress
    );

    nsresult (*SaveSettings)(IMachine *pThis );

    nsresult (*DiscardSettings)(IMachine *pThis );

    nsresult (*Unregister)(
        IMachine *pThis,
        PRUint32 cleanupMode,
        PRUint32 *mediaSize,
        IMedium *** media
    );

    nsresult (*DeleteConfig)(
        IMachine *pThis,
        PRUint32 mediaSize,
        IMedium ** media,
        IProgress * * progress
    );

    nsresult (*ExportTo)(
        IMachine *pThis,
        IAppliance * appliance,
        PRUnichar * location,
        IVirtualSystemDescription * * description
    );

    nsresult (*FindSnapshot)(
        IMachine *pThis,
        PRUnichar * nameOrId,
        ISnapshot * * snapshot
    );

    nsresult (*CreateSharedFolder)(
        IMachine *pThis,
        PRUnichar * name,
        PRUnichar * hostPath,
        PRBool writable,
        PRBool automount
    );

    nsresult (*RemoveSharedFolder)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*CanShowConsoleWindow)(
        IMachine *pThis,
        PRBool * canShow
    );

    nsresult (*ShowConsoleWindow)(
        IMachine *pThis,
        PRInt64 * winId
    );

    nsresult (*GetGuestProperty)(
        IMachine *pThis,
        PRUnichar * name,
        PRUnichar * * value,
        PRInt64 * timestamp,
        PRUnichar * * flags
    );

    nsresult (*GetGuestPropertyValue)(
        IMachine *pThis,
        PRUnichar * property,
        PRUnichar * * value
    );

    nsresult (*GetGuestPropertyTimestamp)(
        IMachine *pThis,
        PRUnichar * property,
        PRInt64 * value
    );

    nsresult (*SetGuestProperty)(
        IMachine *pThis,
        PRUnichar * property,
        PRUnichar * value,
        PRUnichar * flags
    );

    nsresult (*SetGuestPropertyValue)(
        IMachine *pThis,
        PRUnichar * property,
        PRUnichar * value
    );

    nsresult (*DeleteGuestProperty)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*EnumerateGuestProperties)(
        IMachine *pThis,
        PRUnichar * patterns,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *valuesSize,
        PRUnichar *** values,
        PRUint32 *timestampsSize,
        PRInt64** timestamps,
        PRUint32 *flagsSize,
        PRUnichar *** flags
    );

    nsresult (*QuerySavedGuestScreenInfo)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * originX,
        PRUint32 * originY,
        PRUint32 * width,
        PRUint32 * height,
        PRBool * enabled
    );

    nsresult (*QuerySavedThumbnailSize)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * size,
        PRUint32 * width,
        PRUint32 * height
    );

    nsresult (*ReadSavedThumbnailToArray)(
        IMachine *pThis,
        PRUint32 screenId,
        PRBool BGR,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*ReadSavedThumbnailPNGToArray)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*QuerySavedScreenshotPNGSize)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * size,
        PRUint32 * width,
        PRUint32 * height
    );

    nsresult (*ReadSavedScreenshotPNGToArray)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*HotPlugCPU)(
        IMachine *pThis,
        PRUint32 cpu
    );

    nsresult (*HotUnplugCPU)(
        IMachine *pThis,
        PRUint32 cpu
    );

    nsresult (*GetCPUStatus)(
        IMachine *pThis,
        PRUint32 cpu,
        PRBool * attached
    );

    nsresult (*QueryLogFilename)(
        IMachine *pThis,
        PRUint32 idx,
        PRUnichar * * filename
    );

    nsresult (*ReadLog)(
        IMachine *pThis,
        PRUint32 idx,
        PRInt64 offset,
        PRInt64 size,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*CloneTo)(
        IMachine *pThis,
        IMachine * target,
        PRUint32 mode,
        PRUint32 optionsSize,
        PRUint32* options,
        IProgress * * progress
    );

};
#else /* VBOX_WITH_GLUE */
struct IMachineVtbl
{
    nsresult (*QueryInterface)(IMachine *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMachine *pThis);
    nsrefcnt (*Release)(IMachine *pThis);
    nsresult (*GetParent)(IMachine *pThis, IVirtualBox * *parent);

    nsresult (*GetIcon)(IMachine *pThis, PRUint32 *iconSize, PRUint8 **icon);
    nsresult (*SetIcon)(IMachine *pThis, PRUint32 iconSize, PRUint8 *icon);

    nsresult (*GetAccessible)(IMachine *pThis, PRBool *accessible);

    nsresult (*GetAccessError)(IMachine *pThis, IVirtualBoxErrorInfo * *accessError);

    nsresult (*GetName)(IMachine *pThis, PRUnichar * *name);
    nsresult (*SetName)(IMachine *pThis, PRUnichar * name);

    nsresult (*GetDescription)(IMachine *pThis, PRUnichar * *description);
    nsresult (*SetDescription)(IMachine *pThis, PRUnichar * description);

    nsresult (*GetId)(IMachine *pThis, PRUnichar * *id);

    nsresult (*GetGroups)(IMachine *pThis, PRUint32 *groupsSize, PRUnichar * **groups);
    nsresult (*SetGroups)(IMachine *pThis, PRUint32 groupsSize, PRUnichar * *groups);

    nsresult (*GetOSTypeId)(IMachine *pThis, PRUnichar * *OSTypeId);
    nsresult (*SetOSTypeId)(IMachine *pThis, PRUnichar * OSTypeId);

    nsresult (*GetHardwareVersion)(IMachine *pThis, PRUnichar * *hardwareVersion);
    nsresult (*SetHardwareVersion)(IMachine *pThis, PRUnichar * hardwareVersion);

    nsresult (*GetHardwareUUID)(IMachine *pThis, PRUnichar * *hardwareUUID);
    nsresult (*SetHardwareUUID)(IMachine *pThis, PRUnichar * hardwareUUID);

    nsresult (*GetCPUCount)(IMachine *pThis, PRUint32 *CPUCount);
    nsresult (*SetCPUCount)(IMachine *pThis, PRUint32 CPUCount);

    nsresult (*GetCPUHotPlugEnabled)(IMachine *pThis, PRBool *CPUHotPlugEnabled);
    nsresult (*SetCPUHotPlugEnabled)(IMachine *pThis, PRBool CPUHotPlugEnabled);

    nsresult (*GetCPUExecutionCap)(IMachine *pThis, PRUint32 *CPUExecutionCap);
    nsresult (*SetCPUExecutionCap)(IMachine *pThis, PRUint32 CPUExecutionCap);

    nsresult (*GetMemorySize)(IMachine *pThis, PRUint32 *memorySize);
    nsresult (*SetMemorySize)(IMachine *pThis, PRUint32 memorySize);

    nsresult (*GetMemoryBalloonSize)(IMachine *pThis, PRUint32 *memoryBalloonSize);
    nsresult (*SetMemoryBalloonSize)(IMachine *pThis, PRUint32 memoryBalloonSize);

    nsresult (*GetPageFusionEnabled)(IMachine *pThis, PRBool *pageFusionEnabled);
    nsresult (*SetPageFusionEnabled)(IMachine *pThis, PRBool pageFusionEnabled);

    nsresult (*GetGraphicsControllerType)(IMachine *pThis, PRUint32 *graphicsControllerType);
    nsresult (*SetGraphicsControllerType)(IMachine *pThis, PRUint32 graphicsControllerType);

    nsresult (*GetVRAMSize)(IMachine *pThis, PRUint32 *VRAMSize);
    nsresult (*SetVRAMSize)(IMachine *pThis, PRUint32 VRAMSize);

    nsresult (*GetAccelerate3DEnabled)(IMachine *pThis, PRBool *accelerate3DEnabled);
    nsresult (*SetAccelerate3DEnabled)(IMachine *pThis, PRBool accelerate3DEnabled);

    nsresult (*GetAccelerate2DVideoEnabled)(IMachine *pThis, PRBool *accelerate2DVideoEnabled);
    nsresult (*SetAccelerate2DVideoEnabled)(IMachine *pThis, PRBool accelerate2DVideoEnabled);

    nsresult (*GetMonitorCount)(IMachine *pThis, PRUint32 *monitorCount);
    nsresult (*SetMonitorCount)(IMachine *pThis, PRUint32 monitorCount);

    nsresult (*GetVideoCaptureEnabled)(IMachine *pThis, PRBool *videoCaptureEnabled);
    nsresult (*SetVideoCaptureEnabled)(IMachine *pThis, PRBool videoCaptureEnabled);

    nsresult (*GetVideoCaptureScreens)(IMachine *pThis, PRUint32 *videoCaptureScreensSize, PRBool **videoCaptureScreens);
    nsresult (*SetVideoCaptureScreens)(IMachine *pThis, PRUint32 videoCaptureScreensSize, PRBool *videoCaptureScreens);

    nsresult (*GetVideoCaptureFile)(IMachine *pThis, PRUnichar * *videoCaptureFile);
    nsresult (*SetVideoCaptureFile)(IMachine *pThis, PRUnichar * videoCaptureFile);

    nsresult (*GetVideoCaptureWidth)(IMachine *pThis, PRUint32 *videoCaptureWidth);
    nsresult (*SetVideoCaptureWidth)(IMachine *pThis, PRUint32 videoCaptureWidth);

    nsresult (*GetVideoCaptureHeight)(IMachine *pThis, PRUint32 *videoCaptureHeight);
    nsresult (*SetVideoCaptureHeight)(IMachine *pThis, PRUint32 videoCaptureHeight);

    nsresult (*GetVideoCaptureRate)(IMachine *pThis, PRUint32 *videoCaptureRate);
    nsresult (*SetVideoCaptureRate)(IMachine *pThis, PRUint32 videoCaptureRate);

    nsresult (*GetVideoCaptureFPS)(IMachine *pThis, PRUint32 *videoCaptureFPS);
    nsresult (*SetVideoCaptureFPS)(IMachine *pThis, PRUint32 videoCaptureFPS);

    nsresult (*GetBIOSSettings)(IMachine *pThis, IBIOSSettings * *BIOSSettings);

    nsresult (*GetFirmwareType)(IMachine *pThis, PRUint32 *firmwareType);
    nsresult (*SetFirmwareType)(IMachine *pThis, PRUint32 firmwareType);

    nsresult (*GetPointingHIDType)(IMachine *pThis, PRUint32 *pointingHIDType);
    nsresult (*SetPointingHIDType)(IMachine *pThis, PRUint32 pointingHIDType);

    nsresult (*GetKeyboardHIDType)(IMachine *pThis, PRUint32 *keyboardHIDType);
    nsresult (*SetKeyboardHIDType)(IMachine *pThis, PRUint32 keyboardHIDType);

    nsresult (*GetHPETEnabled)(IMachine *pThis, PRBool *HPETEnabled);
    nsresult (*SetHPETEnabled)(IMachine *pThis, PRBool HPETEnabled);

    nsresult (*GetChipsetType)(IMachine *pThis, PRUint32 *chipsetType);
    nsresult (*SetChipsetType)(IMachine *pThis, PRUint32 chipsetType);

    nsresult (*GetSnapshotFolder)(IMachine *pThis, PRUnichar * *snapshotFolder);
    nsresult (*SetSnapshotFolder)(IMachine *pThis, PRUnichar * snapshotFolder);

    nsresult (*GetVRDEServer)(IMachine *pThis, IVRDEServer * *VRDEServer);

    nsresult (*GetEmulatedUSBCardReaderEnabled)(IMachine *pThis, PRBool *emulatedUSBCardReaderEnabled);
    nsresult (*SetEmulatedUSBCardReaderEnabled)(IMachine *pThis, PRBool emulatedUSBCardReaderEnabled);

    nsresult (*GetMediumAttachments)(IMachine *pThis, PRUint32 *mediumAttachmentsSize, IMediumAttachment * **mediumAttachments);

    nsresult (*GetUSBControllers)(IMachine *pThis, PRUint32 *USBControllersSize, IUSBController * **USBControllers);

    nsresult (*GetUSBDeviceFilters)(IMachine *pThis, IUSBDeviceFilters * *USBDeviceFilters);

    nsresult (*GetAudioAdapter)(IMachine *pThis, IAudioAdapter * *audioAdapter);

    nsresult (*GetStorageControllers)(IMachine *pThis, PRUint32 *storageControllersSize, IStorageController * **storageControllers);

    nsresult (*GetSettingsFilePath)(IMachine *pThis, PRUnichar * *settingsFilePath);

    nsresult (*GetSettingsModified)(IMachine *pThis, PRBool *settingsModified);

    nsresult (*GetSessionState)(IMachine *pThis, PRUint32 *sessionState);

    nsresult (*GetSessionType)(IMachine *pThis, PRUnichar * *sessionType);

    nsresult (*GetSessionPID)(IMachine *pThis, PRUint32 *sessionPID);

    nsresult (*GetState)(IMachine *pThis, PRUint32 *state);

    nsresult (*GetLastStateChange)(IMachine *pThis, PRInt64 *lastStateChange);

    nsresult (*GetStateFilePath)(IMachine *pThis, PRUnichar * *stateFilePath);

    nsresult (*GetLogFolder)(IMachine *pThis, PRUnichar * *logFolder);

    nsresult (*GetCurrentSnapshot)(IMachine *pThis, ISnapshot * *currentSnapshot);

    nsresult (*GetSnapshotCount)(IMachine *pThis, PRUint32 *snapshotCount);

    nsresult (*GetCurrentStateModified)(IMachine *pThis, PRBool *currentStateModified);

    nsresult (*GetSharedFolders)(IMachine *pThis, PRUint32 *sharedFoldersSize, ISharedFolder * **sharedFolders);

    nsresult (*GetClipboardMode)(IMachine *pThis, PRUint32 *clipboardMode);
    nsresult (*SetClipboardMode)(IMachine *pThis, PRUint32 clipboardMode);

    nsresult (*GetDragAndDropMode)(IMachine *pThis, PRUint32 *dragAndDropMode);
    nsresult (*SetDragAndDropMode)(IMachine *pThis, PRUint32 dragAndDropMode);

    nsresult (*GetGuestPropertyNotificationPatterns)(IMachine *pThis, PRUnichar * *guestPropertyNotificationPatterns);
    nsresult (*SetGuestPropertyNotificationPatterns)(IMachine *pThis, PRUnichar * guestPropertyNotificationPatterns);

    nsresult (*GetTeleporterEnabled)(IMachine *pThis, PRBool *teleporterEnabled);
    nsresult (*SetTeleporterEnabled)(IMachine *pThis, PRBool teleporterEnabled);

    nsresult (*GetTeleporterPort)(IMachine *pThis, PRUint32 *teleporterPort);
    nsresult (*SetTeleporterPort)(IMachine *pThis, PRUint32 teleporterPort);

    nsresult (*GetTeleporterAddress)(IMachine *pThis, PRUnichar * *teleporterAddress);
    nsresult (*SetTeleporterAddress)(IMachine *pThis, PRUnichar * teleporterAddress);

    nsresult (*GetTeleporterPassword)(IMachine *pThis, PRUnichar * *teleporterPassword);
    nsresult (*SetTeleporterPassword)(IMachine *pThis, PRUnichar * teleporterPassword);

    nsresult (*GetFaultToleranceState)(IMachine *pThis, PRUint32 *faultToleranceState);
    nsresult (*SetFaultToleranceState)(IMachine *pThis, PRUint32 faultToleranceState);

    nsresult (*GetFaultTolerancePort)(IMachine *pThis, PRUint32 *faultTolerancePort);
    nsresult (*SetFaultTolerancePort)(IMachine *pThis, PRUint32 faultTolerancePort);

    nsresult (*GetFaultToleranceAddress)(IMachine *pThis, PRUnichar * *faultToleranceAddress);
    nsresult (*SetFaultToleranceAddress)(IMachine *pThis, PRUnichar * faultToleranceAddress);

    nsresult (*GetFaultTolerancePassword)(IMachine *pThis, PRUnichar * *faultTolerancePassword);
    nsresult (*SetFaultTolerancePassword)(IMachine *pThis, PRUnichar * faultTolerancePassword);

    nsresult (*GetFaultToleranceSyncInterval)(IMachine *pThis, PRUint32 *faultToleranceSyncInterval);
    nsresult (*SetFaultToleranceSyncInterval)(IMachine *pThis, PRUint32 faultToleranceSyncInterval);

    nsresult (*GetRTCUseUTC)(IMachine *pThis, PRBool *RTCUseUTC);
    nsresult (*SetRTCUseUTC)(IMachine *pThis, PRBool RTCUseUTC);

    nsresult (*GetIOCacheEnabled)(IMachine *pThis, PRBool *IOCacheEnabled);
    nsresult (*SetIOCacheEnabled)(IMachine *pThis, PRBool IOCacheEnabled);

    nsresult (*GetIOCacheSize)(IMachine *pThis, PRUint32 *IOCacheSize);
    nsresult (*SetIOCacheSize)(IMachine *pThis, PRUint32 IOCacheSize);

    nsresult (*GetPCIDeviceAssignments)(IMachine *pThis, PRUint32 *PCIDeviceAssignmentsSize, IPCIDeviceAttachment * **PCIDeviceAssignments);

    nsresult (*GetBandwidthControl)(IMachine *pThis, IBandwidthControl * *bandwidthControl);

    nsresult (*GetTracingEnabled)(IMachine *pThis, PRBool *tracingEnabled);
    nsresult (*SetTracingEnabled)(IMachine *pThis, PRBool tracingEnabled);

    nsresult (*GetTracingConfig)(IMachine *pThis, PRUnichar * *tracingConfig);
    nsresult (*SetTracingConfig)(IMachine *pThis, PRUnichar * tracingConfig);

    nsresult (*GetAllowTracingToAccessVM)(IMachine *pThis, PRBool *allowTracingToAccessVM);
    nsresult (*SetAllowTracingToAccessVM)(IMachine *pThis, PRBool allowTracingToAccessVM);

    nsresult (*GetAutostartEnabled)(IMachine *pThis, PRBool *autostartEnabled);
    nsresult (*SetAutostartEnabled)(IMachine *pThis, PRBool autostartEnabled);

    nsresult (*GetAutostartDelay)(IMachine *pThis, PRUint32 *autostartDelay);
    nsresult (*SetAutostartDelay)(IMachine *pThis, PRUint32 autostartDelay);

    nsresult (*GetAutostopType)(IMachine *pThis, PRUint32 *autostopType);
    nsresult (*SetAutostopType)(IMachine *pThis, PRUint32 autostopType);

    nsresult (*GetDefaultFrontend)(IMachine *pThis, PRUnichar * *defaultFrontend);
    nsresult (*SetDefaultFrontend)(IMachine *pThis, PRUnichar * defaultFrontend);

    nsresult (*GetUSBProxyAvailable)(IMachine *pThis, PRBool *USBProxyAvailable);

    nsresult (*LockMachine)(
        IMachine *pThis,
        ISession * session,
        PRUint32 lockType
    );

    nsresult (*LaunchVMProcess)(
        IMachine *pThis,
        ISession * session,
        PRUnichar * type,
        PRUnichar * environment,
        IProgress * * progress
    );

    nsresult (*SetBootOrder)(
        IMachine *pThis,
        PRUint32 position,
        PRUint32 device
    );

    nsresult (*GetBootOrder)(
        IMachine *pThis,
        PRUint32 position,
        PRUint32 * device
    );

    nsresult (*AttachDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRUint32 type,
        IMedium * medium
    );

    nsresult (*AttachDeviceWithoutMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRUint32 type
    );

    nsresult (*DetachDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device
    );

    nsresult (*PassthroughDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool passthrough
    );

    nsresult (*TemporaryEjectDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool temporaryEject
    );

    nsresult (*NonRotationalDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool nonRotational
    );

    nsresult (*SetAutoDiscardForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool discard
    );

    nsresult (*SetHotPluggableForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool hotPluggable
    );

    nsresult (*SetBandwidthGroupForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IBandwidthGroup * bandwidthGroup
    );

    nsresult (*SetNoBandwidthGroupForDevice)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device
    );

    nsresult (*UnmountMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        PRBool force
    );

    nsresult (*MountMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IMedium * medium,
        PRBool force
    );

    nsresult (*GetMedium)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IMedium * * medium
    );

    nsresult (*GetMediumAttachmentsOfController)(
        IMachine *pThis,
        PRUnichar * name,
        PRUint32 *mediumAttachmentsSize,
        IMediumAttachment *** mediumAttachments
    );

    nsresult (*GetMediumAttachment)(
        IMachine *pThis,
        PRUnichar * name,
        PRInt32 controllerPort,
        PRInt32 device,
        IMediumAttachment * * attachment
    );

    nsresult (*AttachHostPCIDevice)(
        IMachine *pThis,
        PRInt32 hostAddress,
        PRInt32 desiredGuestAddress,
        PRBool tryToUnbind
    );

    nsresult (*DetachHostPCIDevice)(
        IMachine *pThis,
        PRInt32 hostAddress
    );

    nsresult (*GetNetworkAdapter)(
        IMachine *pThis,
        PRUint32 slot,
        INetworkAdapter * * adapter
    );

    nsresult (*AddStorageController)(
        IMachine *pThis,
        PRUnichar * name,
        PRUint32 connectionType,
        IStorageController * * controller
    );

    nsresult (*GetStorageControllerByName)(
        IMachine *pThis,
        PRUnichar * name,
        IStorageController * * storageController
    );

    nsresult (*GetStorageControllerByInstance)(
        IMachine *pThis,
        PRUint32 instance,
        IStorageController * * storageController
    );

    nsresult (*RemoveStorageController)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*SetStorageControllerBootable)(
        IMachine *pThis,
        PRUnichar * name,
        PRBool bootable
    );

    nsresult (*AddUSBController)(
        IMachine *pThis,
        PRUnichar * name,
        PRUint32 type,
        IUSBController * * controller
    );

    nsresult (*RemoveUSBController)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*GetUSBControllerByName)(
        IMachine *pThis,
        PRUnichar * name,
        IUSBController * * controller
    );

    nsresult (*GetUSBControllerCountByType)(
        IMachine *pThis,
        PRUint32 type,
        PRUint32 * controllers
    );

    nsresult (*GetSerialPort)(
        IMachine *pThis,
        PRUint32 slot,
        ISerialPort * * port
    );

    nsresult (*GetParallelPort)(
        IMachine *pThis,
        PRUint32 slot,
        IParallelPort * * port
    );

    nsresult (*GetExtraDataKeys)(
        IMachine *pThis,
        PRUint32 *keysSize,
        PRUnichar *** keys
    );

    nsresult (*GetExtraData)(
        IMachine *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

    nsresult (*SetExtraData)(
        IMachine *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*GetCPUProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool * value
    );

    nsresult (*SetCPUProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool value
    );

    nsresult (*GetCPUIDLeaf)(
        IMachine *pThis,
        PRUint32 id,
        PRUint32 * valEax,
        PRUint32 * valEbx,
        PRUint32 * valEcx,
        PRUint32 * valEdx
    );

    nsresult (*SetCPUIDLeaf)(
        IMachine *pThis,
        PRUint32 id,
        PRUint32 valEax,
        PRUint32 valEbx,
        PRUint32 valEcx,
        PRUint32 valEdx
    );

    nsresult (*RemoveCPUIDLeaf)(
        IMachine *pThis,
        PRUint32 id
    );

    nsresult (*RemoveAllCPUIDLeaves)(IMachine *pThis );

    nsresult (*GetHWVirtExProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool * value
    );

    nsresult (*SetHWVirtExProperty)(
        IMachine *pThis,
        PRUint32 property,
        PRBool value
    );

    nsresult (*SetSettingsFilePath)(
        IMachine *pThis,
        PRUnichar * settingsFilePath,
        IProgress * * progress
    );

    nsresult (*SaveSettings)(IMachine *pThis );

    nsresult (*DiscardSettings)(IMachine *pThis );

    nsresult (*Unregister)(
        IMachine *pThis,
        PRUint32 cleanupMode,
        PRUint32 *mediaSize,
        IMedium *** media
    );

    nsresult (*DeleteConfig)(
        IMachine *pThis,
        PRUint32 mediaSize,
        IMedium ** media,
        IProgress * * progress
    );

    nsresult (*ExportTo)(
        IMachine *pThis,
        IAppliance * appliance,
        PRUnichar * location,
        IVirtualSystemDescription * * description
    );

    nsresult (*FindSnapshot)(
        IMachine *pThis,
        PRUnichar * nameOrId,
        ISnapshot * * snapshot
    );

    nsresult (*CreateSharedFolder)(
        IMachine *pThis,
        PRUnichar * name,
        PRUnichar * hostPath,
        PRBool writable,
        PRBool automount
    );

    nsresult (*RemoveSharedFolder)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*CanShowConsoleWindow)(
        IMachine *pThis,
        PRBool * canShow
    );

    nsresult (*ShowConsoleWindow)(
        IMachine *pThis,
        PRInt64 * winId
    );

    nsresult (*GetGuestProperty)(
        IMachine *pThis,
        PRUnichar * name,
        PRUnichar * * value,
        PRInt64 * timestamp,
        PRUnichar * * flags
    );

    nsresult (*GetGuestPropertyValue)(
        IMachine *pThis,
        PRUnichar * property,
        PRUnichar * * value
    );

    nsresult (*GetGuestPropertyTimestamp)(
        IMachine *pThis,
        PRUnichar * property,
        PRInt64 * value
    );

    nsresult (*SetGuestProperty)(
        IMachine *pThis,
        PRUnichar * property,
        PRUnichar * value,
        PRUnichar * flags
    );

    nsresult (*SetGuestPropertyValue)(
        IMachine *pThis,
        PRUnichar * property,
        PRUnichar * value
    );

    nsresult (*DeleteGuestProperty)(
        IMachine *pThis,
        PRUnichar * name
    );

    nsresult (*EnumerateGuestProperties)(
        IMachine *pThis,
        PRUnichar * patterns,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *valuesSize,
        PRUnichar *** values,
        PRUint32 *timestampsSize,
        PRInt64** timestamps,
        PRUint32 *flagsSize,
        PRUnichar *** flags
    );

    nsresult (*QuerySavedGuestScreenInfo)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * originX,
        PRUint32 * originY,
        PRUint32 * width,
        PRUint32 * height,
        PRBool * enabled
    );

    nsresult (*QuerySavedThumbnailSize)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * size,
        PRUint32 * width,
        PRUint32 * height
    );

    nsresult (*ReadSavedThumbnailToArray)(
        IMachine *pThis,
        PRUint32 screenId,
        PRBool BGR,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*ReadSavedThumbnailPNGToArray)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*QuerySavedScreenshotPNGSize)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * size,
        PRUint32 * width,
        PRUint32 * height
    );

    nsresult (*ReadSavedScreenshotPNGToArray)(
        IMachine *pThis,
        PRUint32 screenId,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*HotPlugCPU)(
        IMachine *pThis,
        PRUint32 cpu
    );

    nsresult (*HotUnplugCPU)(
        IMachine *pThis,
        PRUint32 cpu
    );

    nsresult (*GetCPUStatus)(
        IMachine *pThis,
        PRUint32 cpu,
        PRBool * attached
    );

    nsresult (*QueryLogFilename)(
        IMachine *pThis,
        PRUint32 idx,
        PRUnichar * * filename
    );

    nsresult (*ReadLog)(
        IMachine *pThis,
        PRUint32 idx,
        PRInt64 offset,
        PRInt64 size,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*CloneTo)(
        IMachine *pThis,
        IMachine * target,
        PRUint32 mode,
        PRUint32 optionsSize,
        PRUint32* options,
        IProgress * * progress
    );

};
#define IMachine_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMachine_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMachine_Release(p) ((p)->lpVtbl->Release(p))
#define IMachine_get_Parent(p, aParent) ((p)->lpVtbl->GetParent(p, aParent))
#define IMachine_GetParent(p, aParent) ((p)->lpVtbl->GetParent(p, aParent))
#define IMachine_get_Icon(p, aIcon) ((p)->lpVtbl->GetIcon(p, aIcon))
#define IMachine_GetIcon(p, aIcon) ((p)->lpVtbl->GetIcon(p, aIcon))
#define IMachine_put_Icon(p, aIcon) ((p)->lpVtbl->SetIcon(p, aIcon))
#define IMachine_SetIcon(p, aIcon) ((p)->lpVtbl->SetIcon(p, aIcon))
#define IMachine_get_Accessible(p, aAccessible) ((p)->lpVtbl->GetAccessible(p, aAccessible))
#define IMachine_GetAccessible(p, aAccessible) ((p)->lpVtbl->GetAccessible(p, aAccessible))
#define IMachine_get_AccessError(p, aAccessError) ((p)->lpVtbl->GetAccessError(p, aAccessError))
#define IMachine_GetAccessError(p, aAccessError) ((p)->lpVtbl->GetAccessError(p, aAccessError))
#define IMachine_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IMachine_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IMachine_put_Name(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define IMachine_SetName(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define IMachine_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IMachine_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IMachine_put_Description(p, aDescription) ((p)->lpVtbl->SetDescription(p, aDescription))
#define IMachine_SetDescription(p, aDescription) ((p)->lpVtbl->SetDescription(p, aDescription))
#define IMachine_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IMachine_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IMachine_get_Groups(p, aGroups) ((p)->lpVtbl->GetGroups(p, aGroups))
#define IMachine_GetGroups(p, aGroups) ((p)->lpVtbl->GetGroups(p, aGroups))
#define IMachine_put_Groups(p, aGroups) ((p)->lpVtbl->SetGroups(p, aGroups))
#define IMachine_SetGroups(p, aGroups) ((p)->lpVtbl->SetGroups(p, aGroups))
#define IMachine_get_OSTypeId(p, aOSTypeId) ((p)->lpVtbl->GetOSTypeId(p, aOSTypeId))
#define IMachine_GetOSTypeId(p, aOSTypeId) ((p)->lpVtbl->GetOSTypeId(p, aOSTypeId))
#define IMachine_put_OSTypeId(p, aOSTypeId) ((p)->lpVtbl->SetOSTypeId(p, aOSTypeId))
#define IMachine_SetOSTypeId(p, aOSTypeId) ((p)->lpVtbl->SetOSTypeId(p, aOSTypeId))
#define IMachine_get_HardwareVersion(p, aHardwareVersion) ((p)->lpVtbl->GetHardwareVersion(p, aHardwareVersion))
#define IMachine_GetHardwareVersion(p, aHardwareVersion) ((p)->lpVtbl->GetHardwareVersion(p, aHardwareVersion))
#define IMachine_put_HardwareVersion(p, aHardwareVersion) ((p)->lpVtbl->SetHardwareVersion(p, aHardwareVersion))
#define IMachine_SetHardwareVersion(p, aHardwareVersion) ((p)->lpVtbl->SetHardwareVersion(p, aHardwareVersion))
#define IMachine_get_HardwareUUID(p, aHardwareUUID) ((p)->lpVtbl->GetHardwareUUID(p, aHardwareUUID))
#define IMachine_GetHardwareUUID(p, aHardwareUUID) ((p)->lpVtbl->GetHardwareUUID(p, aHardwareUUID))
#define IMachine_put_HardwareUUID(p, aHardwareUUID) ((p)->lpVtbl->SetHardwareUUID(p, aHardwareUUID))
#define IMachine_SetHardwareUUID(p, aHardwareUUID) ((p)->lpVtbl->SetHardwareUUID(p, aHardwareUUID))
#define IMachine_get_CPUCount(p, aCPUCount) ((p)->lpVtbl->GetCPUCount(p, aCPUCount))
#define IMachine_GetCPUCount(p, aCPUCount) ((p)->lpVtbl->GetCPUCount(p, aCPUCount))
#define IMachine_put_CPUCount(p, aCPUCount) ((p)->lpVtbl->SetCPUCount(p, aCPUCount))
#define IMachine_SetCPUCount(p, aCPUCount) ((p)->lpVtbl->SetCPUCount(p, aCPUCount))
#define IMachine_get_CPUHotPlugEnabled(p, aCPUHotPlugEnabled) ((p)->lpVtbl->GetCPUHotPlugEnabled(p, aCPUHotPlugEnabled))
#define IMachine_GetCPUHotPlugEnabled(p, aCPUHotPlugEnabled) ((p)->lpVtbl->GetCPUHotPlugEnabled(p, aCPUHotPlugEnabled))
#define IMachine_put_CPUHotPlugEnabled(p, aCPUHotPlugEnabled) ((p)->lpVtbl->SetCPUHotPlugEnabled(p, aCPUHotPlugEnabled))
#define IMachine_SetCPUHotPlugEnabled(p, aCPUHotPlugEnabled) ((p)->lpVtbl->SetCPUHotPlugEnabled(p, aCPUHotPlugEnabled))
#define IMachine_get_CPUExecutionCap(p, aCPUExecutionCap) ((p)->lpVtbl->GetCPUExecutionCap(p, aCPUExecutionCap))
#define IMachine_GetCPUExecutionCap(p, aCPUExecutionCap) ((p)->lpVtbl->GetCPUExecutionCap(p, aCPUExecutionCap))
#define IMachine_put_CPUExecutionCap(p, aCPUExecutionCap) ((p)->lpVtbl->SetCPUExecutionCap(p, aCPUExecutionCap))
#define IMachine_SetCPUExecutionCap(p, aCPUExecutionCap) ((p)->lpVtbl->SetCPUExecutionCap(p, aCPUExecutionCap))
#define IMachine_get_MemorySize(p, aMemorySize) ((p)->lpVtbl->GetMemorySize(p, aMemorySize))
#define IMachine_GetMemorySize(p, aMemorySize) ((p)->lpVtbl->GetMemorySize(p, aMemorySize))
#define IMachine_put_MemorySize(p, aMemorySize) ((p)->lpVtbl->SetMemorySize(p, aMemorySize))
#define IMachine_SetMemorySize(p, aMemorySize) ((p)->lpVtbl->SetMemorySize(p, aMemorySize))
#define IMachine_get_MemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->GetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IMachine_GetMemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->GetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IMachine_put_MemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->SetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IMachine_SetMemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->SetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IMachine_get_PageFusionEnabled(p, aPageFusionEnabled) ((p)->lpVtbl->GetPageFusionEnabled(p, aPageFusionEnabled))
#define IMachine_GetPageFusionEnabled(p, aPageFusionEnabled) ((p)->lpVtbl->GetPageFusionEnabled(p, aPageFusionEnabled))
#define IMachine_put_PageFusionEnabled(p, aPageFusionEnabled) ((p)->lpVtbl->SetPageFusionEnabled(p, aPageFusionEnabled))
#define IMachine_SetPageFusionEnabled(p, aPageFusionEnabled) ((p)->lpVtbl->SetPageFusionEnabled(p, aPageFusionEnabled))
#define IMachine_get_GraphicsControllerType(p, aGraphicsControllerType) ((p)->lpVtbl->GetGraphicsControllerType(p, aGraphicsControllerType))
#define IMachine_GetGraphicsControllerType(p, aGraphicsControllerType) ((p)->lpVtbl->GetGraphicsControllerType(p, aGraphicsControllerType))
#define IMachine_put_GraphicsControllerType(p, aGraphicsControllerType) ((p)->lpVtbl->SetGraphicsControllerType(p, aGraphicsControllerType))
#define IMachine_SetGraphicsControllerType(p, aGraphicsControllerType) ((p)->lpVtbl->SetGraphicsControllerType(p, aGraphicsControllerType))
#define IMachine_get_VRAMSize(p, aVRAMSize) ((p)->lpVtbl->GetVRAMSize(p, aVRAMSize))
#define IMachine_GetVRAMSize(p, aVRAMSize) ((p)->lpVtbl->GetVRAMSize(p, aVRAMSize))
#define IMachine_put_VRAMSize(p, aVRAMSize) ((p)->lpVtbl->SetVRAMSize(p, aVRAMSize))
#define IMachine_SetVRAMSize(p, aVRAMSize) ((p)->lpVtbl->SetVRAMSize(p, aVRAMSize))
#define IMachine_get_Accelerate3DEnabled(p, aAccelerate3DEnabled) ((p)->lpVtbl->GetAccelerate3DEnabled(p, aAccelerate3DEnabled))
#define IMachine_GetAccelerate3DEnabled(p, aAccelerate3DEnabled) ((p)->lpVtbl->GetAccelerate3DEnabled(p, aAccelerate3DEnabled))
#define IMachine_put_Accelerate3DEnabled(p, aAccelerate3DEnabled) ((p)->lpVtbl->SetAccelerate3DEnabled(p, aAccelerate3DEnabled))
#define IMachine_SetAccelerate3DEnabled(p, aAccelerate3DEnabled) ((p)->lpVtbl->SetAccelerate3DEnabled(p, aAccelerate3DEnabled))
#define IMachine_get_Accelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled) ((p)->lpVtbl->GetAccelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled))
#define IMachine_GetAccelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled) ((p)->lpVtbl->GetAccelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled))
#define IMachine_put_Accelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled) ((p)->lpVtbl->SetAccelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled))
#define IMachine_SetAccelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled) ((p)->lpVtbl->SetAccelerate2DVideoEnabled(p, aAccelerate2DVideoEnabled))
#define IMachine_get_MonitorCount(p, aMonitorCount) ((p)->lpVtbl->GetMonitorCount(p, aMonitorCount))
#define IMachine_GetMonitorCount(p, aMonitorCount) ((p)->lpVtbl->GetMonitorCount(p, aMonitorCount))
#define IMachine_put_MonitorCount(p, aMonitorCount) ((p)->lpVtbl->SetMonitorCount(p, aMonitorCount))
#define IMachine_SetMonitorCount(p, aMonitorCount) ((p)->lpVtbl->SetMonitorCount(p, aMonitorCount))
#define IMachine_get_VideoCaptureEnabled(p, aVideoCaptureEnabled) ((p)->lpVtbl->GetVideoCaptureEnabled(p, aVideoCaptureEnabled))
#define IMachine_GetVideoCaptureEnabled(p, aVideoCaptureEnabled) ((p)->lpVtbl->GetVideoCaptureEnabled(p, aVideoCaptureEnabled))
#define IMachine_put_VideoCaptureEnabled(p, aVideoCaptureEnabled) ((p)->lpVtbl->SetVideoCaptureEnabled(p, aVideoCaptureEnabled))
#define IMachine_SetVideoCaptureEnabled(p, aVideoCaptureEnabled) ((p)->lpVtbl->SetVideoCaptureEnabled(p, aVideoCaptureEnabled))
#define IMachine_get_VideoCaptureScreens(p, aVideoCaptureScreens) ((p)->lpVtbl->GetVideoCaptureScreens(p, aVideoCaptureScreens))
#define IMachine_GetVideoCaptureScreens(p, aVideoCaptureScreens) ((p)->lpVtbl->GetVideoCaptureScreens(p, aVideoCaptureScreens))
#define IMachine_put_VideoCaptureScreens(p, aVideoCaptureScreens) ((p)->lpVtbl->SetVideoCaptureScreens(p, aVideoCaptureScreens))
#define IMachine_SetVideoCaptureScreens(p, aVideoCaptureScreens) ((p)->lpVtbl->SetVideoCaptureScreens(p, aVideoCaptureScreens))
#define IMachine_get_VideoCaptureFile(p, aVideoCaptureFile) ((p)->lpVtbl->GetVideoCaptureFile(p, aVideoCaptureFile))
#define IMachine_GetVideoCaptureFile(p, aVideoCaptureFile) ((p)->lpVtbl->GetVideoCaptureFile(p, aVideoCaptureFile))
#define IMachine_put_VideoCaptureFile(p, aVideoCaptureFile) ((p)->lpVtbl->SetVideoCaptureFile(p, aVideoCaptureFile))
#define IMachine_SetVideoCaptureFile(p, aVideoCaptureFile) ((p)->lpVtbl->SetVideoCaptureFile(p, aVideoCaptureFile))
#define IMachine_get_VideoCaptureWidth(p, aVideoCaptureWidth) ((p)->lpVtbl->GetVideoCaptureWidth(p, aVideoCaptureWidth))
#define IMachine_GetVideoCaptureWidth(p, aVideoCaptureWidth) ((p)->lpVtbl->GetVideoCaptureWidth(p, aVideoCaptureWidth))
#define IMachine_put_VideoCaptureWidth(p, aVideoCaptureWidth) ((p)->lpVtbl->SetVideoCaptureWidth(p, aVideoCaptureWidth))
#define IMachine_SetVideoCaptureWidth(p, aVideoCaptureWidth) ((p)->lpVtbl->SetVideoCaptureWidth(p, aVideoCaptureWidth))
#define IMachine_get_VideoCaptureHeight(p, aVideoCaptureHeight) ((p)->lpVtbl->GetVideoCaptureHeight(p, aVideoCaptureHeight))
#define IMachine_GetVideoCaptureHeight(p, aVideoCaptureHeight) ((p)->lpVtbl->GetVideoCaptureHeight(p, aVideoCaptureHeight))
#define IMachine_put_VideoCaptureHeight(p, aVideoCaptureHeight) ((p)->lpVtbl->SetVideoCaptureHeight(p, aVideoCaptureHeight))
#define IMachine_SetVideoCaptureHeight(p, aVideoCaptureHeight) ((p)->lpVtbl->SetVideoCaptureHeight(p, aVideoCaptureHeight))
#define IMachine_get_VideoCaptureRate(p, aVideoCaptureRate) ((p)->lpVtbl->GetVideoCaptureRate(p, aVideoCaptureRate))
#define IMachine_GetVideoCaptureRate(p, aVideoCaptureRate) ((p)->lpVtbl->GetVideoCaptureRate(p, aVideoCaptureRate))
#define IMachine_put_VideoCaptureRate(p, aVideoCaptureRate) ((p)->lpVtbl->SetVideoCaptureRate(p, aVideoCaptureRate))
#define IMachine_SetVideoCaptureRate(p, aVideoCaptureRate) ((p)->lpVtbl->SetVideoCaptureRate(p, aVideoCaptureRate))
#define IMachine_get_VideoCaptureFPS(p, aVideoCaptureFPS) ((p)->lpVtbl->GetVideoCaptureFPS(p, aVideoCaptureFPS))
#define IMachine_GetVideoCaptureFPS(p, aVideoCaptureFPS) ((p)->lpVtbl->GetVideoCaptureFPS(p, aVideoCaptureFPS))
#define IMachine_put_VideoCaptureFPS(p, aVideoCaptureFPS) ((p)->lpVtbl->SetVideoCaptureFPS(p, aVideoCaptureFPS))
#define IMachine_SetVideoCaptureFPS(p, aVideoCaptureFPS) ((p)->lpVtbl->SetVideoCaptureFPS(p, aVideoCaptureFPS))
#define IMachine_get_BIOSSettings(p, aBIOSSettings) ((p)->lpVtbl->GetBIOSSettings(p, aBIOSSettings))
#define IMachine_GetBIOSSettings(p, aBIOSSettings) ((p)->lpVtbl->GetBIOSSettings(p, aBIOSSettings))
#define IMachine_get_FirmwareType(p, aFirmwareType) ((p)->lpVtbl->GetFirmwareType(p, aFirmwareType))
#define IMachine_GetFirmwareType(p, aFirmwareType) ((p)->lpVtbl->GetFirmwareType(p, aFirmwareType))
#define IMachine_put_FirmwareType(p, aFirmwareType) ((p)->lpVtbl->SetFirmwareType(p, aFirmwareType))
#define IMachine_SetFirmwareType(p, aFirmwareType) ((p)->lpVtbl->SetFirmwareType(p, aFirmwareType))
#define IMachine_get_PointingHIDType(p, aPointingHIDType) ((p)->lpVtbl->GetPointingHIDType(p, aPointingHIDType))
#define IMachine_GetPointingHIDType(p, aPointingHIDType) ((p)->lpVtbl->GetPointingHIDType(p, aPointingHIDType))
#define IMachine_put_PointingHIDType(p, aPointingHIDType) ((p)->lpVtbl->SetPointingHIDType(p, aPointingHIDType))
#define IMachine_SetPointingHIDType(p, aPointingHIDType) ((p)->lpVtbl->SetPointingHIDType(p, aPointingHIDType))
#define IMachine_get_KeyboardHIDType(p, aKeyboardHIDType) ((p)->lpVtbl->GetKeyboardHIDType(p, aKeyboardHIDType))
#define IMachine_GetKeyboardHIDType(p, aKeyboardHIDType) ((p)->lpVtbl->GetKeyboardHIDType(p, aKeyboardHIDType))
#define IMachine_put_KeyboardHIDType(p, aKeyboardHIDType) ((p)->lpVtbl->SetKeyboardHIDType(p, aKeyboardHIDType))
#define IMachine_SetKeyboardHIDType(p, aKeyboardHIDType) ((p)->lpVtbl->SetKeyboardHIDType(p, aKeyboardHIDType))
#define IMachine_get_HPETEnabled(p, aHPETEnabled) ((p)->lpVtbl->GetHPETEnabled(p, aHPETEnabled))
#define IMachine_GetHPETEnabled(p, aHPETEnabled) ((p)->lpVtbl->GetHPETEnabled(p, aHPETEnabled))
#define IMachine_put_HPETEnabled(p, aHPETEnabled) ((p)->lpVtbl->SetHPETEnabled(p, aHPETEnabled))
#define IMachine_SetHPETEnabled(p, aHPETEnabled) ((p)->lpVtbl->SetHPETEnabled(p, aHPETEnabled))
#define IMachine_get_ChipsetType(p, aChipsetType) ((p)->lpVtbl->GetChipsetType(p, aChipsetType))
#define IMachine_GetChipsetType(p, aChipsetType) ((p)->lpVtbl->GetChipsetType(p, aChipsetType))
#define IMachine_put_ChipsetType(p, aChipsetType) ((p)->lpVtbl->SetChipsetType(p, aChipsetType))
#define IMachine_SetChipsetType(p, aChipsetType) ((p)->lpVtbl->SetChipsetType(p, aChipsetType))
#define IMachine_get_SnapshotFolder(p, aSnapshotFolder) ((p)->lpVtbl->GetSnapshotFolder(p, aSnapshotFolder))
#define IMachine_GetSnapshotFolder(p, aSnapshotFolder) ((p)->lpVtbl->GetSnapshotFolder(p, aSnapshotFolder))
#define IMachine_put_SnapshotFolder(p, aSnapshotFolder) ((p)->lpVtbl->SetSnapshotFolder(p, aSnapshotFolder))
#define IMachine_SetSnapshotFolder(p, aSnapshotFolder) ((p)->lpVtbl->SetSnapshotFolder(p, aSnapshotFolder))
#define IMachine_get_VRDEServer(p, aVRDEServer) ((p)->lpVtbl->GetVRDEServer(p, aVRDEServer))
#define IMachine_GetVRDEServer(p, aVRDEServer) ((p)->lpVtbl->GetVRDEServer(p, aVRDEServer))
#define IMachine_get_EmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled) ((p)->lpVtbl->GetEmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled))
#define IMachine_GetEmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled) ((p)->lpVtbl->GetEmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled))
#define IMachine_put_EmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled) ((p)->lpVtbl->SetEmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled))
#define IMachine_SetEmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled) ((p)->lpVtbl->SetEmulatedUSBCardReaderEnabled(p, aEmulatedUSBCardReaderEnabled))
#define IMachine_get_MediumAttachments(p, aMediumAttachments) ((p)->lpVtbl->GetMediumAttachments(p, aMediumAttachments))
#define IMachine_GetMediumAttachments(p, aMediumAttachments) ((p)->lpVtbl->GetMediumAttachments(p, aMediumAttachments))
#define IMachine_get_USBControllers(p, aUSBControllers) ((p)->lpVtbl->GetUSBControllers(p, aUSBControllers))
#define IMachine_GetUSBControllers(p, aUSBControllers) ((p)->lpVtbl->GetUSBControllers(p, aUSBControllers))
#define IMachine_get_USBDeviceFilters(p, aUSBDeviceFilters) ((p)->lpVtbl->GetUSBDeviceFilters(p, aUSBDeviceFilters))
#define IMachine_GetUSBDeviceFilters(p, aUSBDeviceFilters) ((p)->lpVtbl->GetUSBDeviceFilters(p, aUSBDeviceFilters))
#define IMachine_get_AudioAdapter(p, aAudioAdapter) ((p)->lpVtbl->GetAudioAdapter(p, aAudioAdapter))
#define IMachine_GetAudioAdapter(p, aAudioAdapter) ((p)->lpVtbl->GetAudioAdapter(p, aAudioAdapter))
#define IMachine_get_StorageControllers(p, aStorageControllers) ((p)->lpVtbl->GetStorageControllers(p, aStorageControllers))
#define IMachine_GetStorageControllers(p, aStorageControllers) ((p)->lpVtbl->GetStorageControllers(p, aStorageControllers))
#define IMachine_get_SettingsFilePath(p, aSettingsFilePath) ((p)->lpVtbl->GetSettingsFilePath(p, aSettingsFilePath))
#define IMachine_GetSettingsFilePath(p, aSettingsFilePath) ((p)->lpVtbl->GetSettingsFilePath(p, aSettingsFilePath))
#define IMachine_get_SettingsModified(p, aSettingsModified) ((p)->lpVtbl->GetSettingsModified(p, aSettingsModified))
#define IMachine_GetSettingsModified(p, aSettingsModified) ((p)->lpVtbl->GetSettingsModified(p, aSettingsModified))
#define IMachine_get_SessionState(p, aSessionState) ((p)->lpVtbl->GetSessionState(p, aSessionState))
#define IMachine_GetSessionState(p, aSessionState) ((p)->lpVtbl->GetSessionState(p, aSessionState))
#define IMachine_get_SessionType(p, aSessionType) ((p)->lpVtbl->GetSessionType(p, aSessionType))
#define IMachine_GetSessionType(p, aSessionType) ((p)->lpVtbl->GetSessionType(p, aSessionType))
#define IMachine_get_SessionPID(p, aSessionPID) ((p)->lpVtbl->GetSessionPID(p, aSessionPID))
#define IMachine_GetSessionPID(p, aSessionPID) ((p)->lpVtbl->GetSessionPID(p, aSessionPID))
#define IMachine_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IMachine_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IMachine_get_LastStateChange(p, aLastStateChange) ((p)->lpVtbl->GetLastStateChange(p, aLastStateChange))
#define IMachine_GetLastStateChange(p, aLastStateChange) ((p)->lpVtbl->GetLastStateChange(p, aLastStateChange))
#define IMachine_get_StateFilePath(p, aStateFilePath) ((p)->lpVtbl->GetStateFilePath(p, aStateFilePath))
#define IMachine_GetStateFilePath(p, aStateFilePath) ((p)->lpVtbl->GetStateFilePath(p, aStateFilePath))
#define IMachine_get_LogFolder(p, aLogFolder) ((p)->lpVtbl->GetLogFolder(p, aLogFolder))
#define IMachine_GetLogFolder(p, aLogFolder) ((p)->lpVtbl->GetLogFolder(p, aLogFolder))
#define IMachine_get_CurrentSnapshot(p, aCurrentSnapshot) ((p)->lpVtbl->GetCurrentSnapshot(p, aCurrentSnapshot))
#define IMachine_GetCurrentSnapshot(p, aCurrentSnapshot) ((p)->lpVtbl->GetCurrentSnapshot(p, aCurrentSnapshot))
#define IMachine_get_SnapshotCount(p, aSnapshotCount) ((p)->lpVtbl->GetSnapshotCount(p, aSnapshotCount))
#define IMachine_GetSnapshotCount(p, aSnapshotCount) ((p)->lpVtbl->GetSnapshotCount(p, aSnapshotCount))
#define IMachine_get_CurrentStateModified(p, aCurrentStateModified) ((p)->lpVtbl->GetCurrentStateModified(p, aCurrentStateModified))
#define IMachine_GetCurrentStateModified(p, aCurrentStateModified) ((p)->lpVtbl->GetCurrentStateModified(p, aCurrentStateModified))
#define IMachine_get_SharedFolders(p, aSharedFolders) ((p)->lpVtbl->GetSharedFolders(p, aSharedFolders))
#define IMachine_GetSharedFolders(p, aSharedFolders) ((p)->lpVtbl->GetSharedFolders(p, aSharedFolders))
#define IMachine_get_ClipboardMode(p, aClipboardMode) ((p)->lpVtbl->GetClipboardMode(p, aClipboardMode))
#define IMachine_GetClipboardMode(p, aClipboardMode) ((p)->lpVtbl->GetClipboardMode(p, aClipboardMode))
#define IMachine_put_ClipboardMode(p, aClipboardMode) ((p)->lpVtbl->SetClipboardMode(p, aClipboardMode))
#define IMachine_SetClipboardMode(p, aClipboardMode) ((p)->lpVtbl->SetClipboardMode(p, aClipboardMode))
#define IMachine_get_DragAndDropMode(p, aDragAndDropMode) ((p)->lpVtbl->GetDragAndDropMode(p, aDragAndDropMode))
#define IMachine_GetDragAndDropMode(p, aDragAndDropMode) ((p)->lpVtbl->GetDragAndDropMode(p, aDragAndDropMode))
#define IMachine_put_DragAndDropMode(p, aDragAndDropMode) ((p)->lpVtbl->SetDragAndDropMode(p, aDragAndDropMode))
#define IMachine_SetDragAndDropMode(p, aDragAndDropMode) ((p)->lpVtbl->SetDragAndDropMode(p, aDragAndDropMode))
#define IMachine_get_GuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns) ((p)->lpVtbl->GetGuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns))
#define IMachine_GetGuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns) ((p)->lpVtbl->GetGuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns))
#define IMachine_put_GuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns) ((p)->lpVtbl->SetGuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns))
#define IMachine_SetGuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns) ((p)->lpVtbl->SetGuestPropertyNotificationPatterns(p, aGuestPropertyNotificationPatterns))
#define IMachine_get_TeleporterEnabled(p, aTeleporterEnabled) ((p)->lpVtbl->GetTeleporterEnabled(p, aTeleporterEnabled))
#define IMachine_GetTeleporterEnabled(p, aTeleporterEnabled) ((p)->lpVtbl->GetTeleporterEnabled(p, aTeleporterEnabled))
#define IMachine_put_TeleporterEnabled(p, aTeleporterEnabled) ((p)->lpVtbl->SetTeleporterEnabled(p, aTeleporterEnabled))
#define IMachine_SetTeleporterEnabled(p, aTeleporterEnabled) ((p)->lpVtbl->SetTeleporterEnabled(p, aTeleporterEnabled))
#define IMachine_get_TeleporterPort(p, aTeleporterPort) ((p)->lpVtbl->GetTeleporterPort(p, aTeleporterPort))
#define IMachine_GetTeleporterPort(p, aTeleporterPort) ((p)->lpVtbl->GetTeleporterPort(p, aTeleporterPort))
#define IMachine_put_TeleporterPort(p, aTeleporterPort) ((p)->lpVtbl->SetTeleporterPort(p, aTeleporterPort))
#define IMachine_SetTeleporterPort(p, aTeleporterPort) ((p)->lpVtbl->SetTeleporterPort(p, aTeleporterPort))
#define IMachine_get_TeleporterAddress(p, aTeleporterAddress) ((p)->lpVtbl->GetTeleporterAddress(p, aTeleporterAddress))
#define IMachine_GetTeleporterAddress(p, aTeleporterAddress) ((p)->lpVtbl->GetTeleporterAddress(p, aTeleporterAddress))
#define IMachine_put_TeleporterAddress(p, aTeleporterAddress) ((p)->lpVtbl->SetTeleporterAddress(p, aTeleporterAddress))
#define IMachine_SetTeleporterAddress(p, aTeleporterAddress) ((p)->lpVtbl->SetTeleporterAddress(p, aTeleporterAddress))
#define IMachine_get_TeleporterPassword(p, aTeleporterPassword) ((p)->lpVtbl->GetTeleporterPassword(p, aTeleporterPassword))
#define IMachine_GetTeleporterPassword(p, aTeleporterPassword) ((p)->lpVtbl->GetTeleporterPassword(p, aTeleporterPassword))
#define IMachine_put_TeleporterPassword(p, aTeleporterPassword) ((p)->lpVtbl->SetTeleporterPassword(p, aTeleporterPassword))
#define IMachine_SetTeleporterPassword(p, aTeleporterPassword) ((p)->lpVtbl->SetTeleporterPassword(p, aTeleporterPassword))
#define IMachine_get_FaultToleranceState(p, aFaultToleranceState) ((p)->lpVtbl->GetFaultToleranceState(p, aFaultToleranceState))
#define IMachine_GetFaultToleranceState(p, aFaultToleranceState) ((p)->lpVtbl->GetFaultToleranceState(p, aFaultToleranceState))
#define IMachine_put_FaultToleranceState(p, aFaultToleranceState) ((p)->lpVtbl->SetFaultToleranceState(p, aFaultToleranceState))
#define IMachine_SetFaultToleranceState(p, aFaultToleranceState) ((p)->lpVtbl->SetFaultToleranceState(p, aFaultToleranceState))
#define IMachine_get_FaultTolerancePort(p, aFaultTolerancePort) ((p)->lpVtbl->GetFaultTolerancePort(p, aFaultTolerancePort))
#define IMachine_GetFaultTolerancePort(p, aFaultTolerancePort) ((p)->lpVtbl->GetFaultTolerancePort(p, aFaultTolerancePort))
#define IMachine_put_FaultTolerancePort(p, aFaultTolerancePort) ((p)->lpVtbl->SetFaultTolerancePort(p, aFaultTolerancePort))
#define IMachine_SetFaultTolerancePort(p, aFaultTolerancePort) ((p)->lpVtbl->SetFaultTolerancePort(p, aFaultTolerancePort))
#define IMachine_get_FaultToleranceAddress(p, aFaultToleranceAddress) ((p)->lpVtbl->GetFaultToleranceAddress(p, aFaultToleranceAddress))
#define IMachine_GetFaultToleranceAddress(p, aFaultToleranceAddress) ((p)->lpVtbl->GetFaultToleranceAddress(p, aFaultToleranceAddress))
#define IMachine_put_FaultToleranceAddress(p, aFaultToleranceAddress) ((p)->lpVtbl->SetFaultToleranceAddress(p, aFaultToleranceAddress))
#define IMachine_SetFaultToleranceAddress(p, aFaultToleranceAddress) ((p)->lpVtbl->SetFaultToleranceAddress(p, aFaultToleranceAddress))
#define IMachine_get_FaultTolerancePassword(p, aFaultTolerancePassword) ((p)->lpVtbl->GetFaultTolerancePassword(p, aFaultTolerancePassword))
#define IMachine_GetFaultTolerancePassword(p, aFaultTolerancePassword) ((p)->lpVtbl->GetFaultTolerancePassword(p, aFaultTolerancePassword))
#define IMachine_put_FaultTolerancePassword(p, aFaultTolerancePassword) ((p)->lpVtbl->SetFaultTolerancePassword(p, aFaultTolerancePassword))
#define IMachine_SetFaultTolerancePassword(p, aFaultTolerancePassword) ((p)->lpVtbl->SetFaultTolerancePassword(p, aFaultTolerancePassword))
#define IMachine_get_FaultToleranceSyncInterval(p, aFaultToleranceSyncInterval) ((p)->lpVtbl->GetFaultToleranceSyncInterval(p, aFaultToleranceSyncInterval))
#define IMachine_GetFaultToleranceSyncInterval(p, aFaultToleranceSyncInterval) ((p)->lpVtbl->GetFaultToleranceSyncInterval(p, aFaultToleranceSyncInterval))
#define IMachine_put_FaultToleranceSyncInterval(p, aFaultToleranceSyncInterval) ((p)->lpVtbl->SetFaultToleranceSyncInterval(p, aFaultToleranceSyncInterval))
#define IMachine_SetFaultToleranceSyncInterval(p, aFaultToleranceSyncInterval) ((p)->lpVtbl->SetFaultToleranceSyncInterval(p, aFaultToleranceSyncInterval))
#define IMachine_get_RTCUseUTC(p, aRTCUseUTC) ((p)->lpVtbl->GetRTCUseUTC(p, aRTCUseUTC))
#define IMachine_GetRTCUseUTC(p, aRTCUseUTC) ((p)->lpVtbl->GetRTCUseUTC(p, aRTCUseUTC))
#define IMachine_put_RTCUseUTC(p, aRTCUseUTC) ((p)->lpVtbl->SetRTCUseUTC(p, aRTCUseUTC))
#define IMachine_SetRTCUseUTC(p, aRTCUseUTC) ((p)->lpVtbl->SetRTCUseUTC(p, aRTCUseUTC))
#define IMachine_get_IOCacheEnabled(p, aIOCacheEnabled) ((p)->lpVtbl->GetIOCacheEnabled(p, aIOCacheEnabled))
#define IMachine_GetIOCacheEnabled(p, aIOCacheEnabled) ((p)->lpVtbl->GetIOCacheEnabled(p, aIOCacheEnabled))
#define IMachine_put_IOCacheEnabled(p, aIOCacheEnabled) ((p)->lpVtbl->SetIOCacheEnabled(p, aIOCacheEnabled))
#define IMachine_SetIOCacheEnabled(p, aIOCacheEnabled) ((p)->lpVtbl->SetIOCacheEnabled(p, aIOCacheEnabled))
#define IMachine_get_IOCacheSize(p, aIOCacheSize) ((p)->lpVtbl->GetIOCacheSize(p, aIOCacheSize))
#define IMachine_GetIOCacheSize(p, aIOCacheSize) ((p)->lpVtbl->GetIOCacheSize(p, aIOCacheSize))
#define IMachine_put_IOCacheSize(p, aIOCacheSize) ((p)->lpVtbl->SetIOCacheSize(p, aIOCacheSize))
#define IMachine_SetIOCacheSize(p, aIOCacheSize) ((p)->lpVtbl->SetIOCacheSize(p, aIOCacheSize))
#define IMachine_get_PCIDeviceAssignments(p, aPCIDeviceAssignments) ((p)->lpVtbl->GetPCIDeviceAssignments(p, aPCIDeviceAssignments))
#define IMachine_GetPCIDeviceAssignments(p, aPCIDeviceAssignments) ((p)->lpVtbl->GetPCIDeviceAssignments(p, aPCIDeviceAssignments))
#define IMachine_get_BandwidthControl(p, aBandwidthControl) ((p)->lpVtbl->GetBandwidthControl(p, aBandwidthControl))
#define IMachine_GetBandwidthControl(p, aBandwidthControl) ((p)->lpVtbl->GetBandwidthControl(p, aBandwidthControl))
#define IMachine_get_TracingEnabled(p, aTracingEnabled) ((p)->lpVtbl->GetTracingEnabled(p, aTracingEnabled))
#define IMachine_GetTracingEnabled(p, aTracingEnabled) ((p)->lpVtbl->GetTracingEnabled(p, aTracingEnabled))
#define IMachine_put_TracingEnabled(p, aTracingEnabled) ((p)->lpVtbl->SetTracingEnabled(p, aTracingEnabled))
#define IMachine_SetTracingEnabled(p, aTracingEnabled) ((p)->lpVtbl->SetTracingEnabled(p, aTracingEnabled))
#define IMachine_get_TracingConfig(p, aTracingConfig) ((p)->lpVtbl->GetTracingConfig(p, aTracingConfig))
#define IMachine_GetTracingConfig(p, aTracingConfig) ((p)->lpVtbl->GetTracingConfig(p, aTracingConfig))
#define IMachine_put_TracingConfig(p, aTracingConfig) ((p)->lpVtbl->SetTracingConfig(p, aTracingConfig))
#define IMachine_SetTracingConfig(p, aTracingConfig) ((p)->lpVtbl->SetTracingConfig(p, aTracingConfig))
#define IMachine_get_AllowTracingToAccessVM(p, aAllowTracingToAccessVM) ((p)->lpVtbl->GetAllowTracingToAccessVM(p, aAllowTracingToAccessVM))
#define IMachine_GetAllowTracingToAccessVM(p, aAllowTracingToAccessVM) ((p)->lpVtbl->GetAllowTracingToAccessVM(p, aAllowTracingToAccessVM))
#define IMachine_put_AllowTracingToAccessVM(p, aAllowTracingToAccessVM) ((p)->lpVtbl->SetAllowTracingToAccessVM(p, aAllowTracingToAccessVM))
#define IMachine_SetAllowTracingToAccessVM(p, aAllowTracingToAccessVM) ((p)->lpVtbl->SetAllowTracingToAccessVM(p, aAllowTracingToAccessVM))
#define IMachine_get_AutostartEnabled(p, aAutostartEnabled) ((p)->lpVtbl->GetAutostartEnabled(p, aAutostartEnabled))
#define IMachine_GetAutostartEnabled(p, aAutostartEnabled) ((p)->lpVtbl->GetAutostartEnabled(p, aAutostartEnabled))
#define IMachine_put_AutostartEnabled(p, aAutostartEnabled) ((p)->lpVtbl->SetAutostartEnabled(p, aAutostartEnabled))
#define IMachine_SetAutostartEnabled(p, aAutostartEnabled) ((p)->lpVtbl->SetAutostartEnabled(p, aAutostartEnabled))
#define IMachine_get_AutostartDelay(p, aAutostartDelay) ((p)->lpVtbl->GetAutostartDelay(p, aAutostartDelay))
#define IMachine_GetAutostartDelay(p, aAutostartDelay) ((p)->lpVtbl->GetAutostartDelay(p, aAutostartDelay))
#define IMachine_put_AutostartDelay(p, aAutostartDelay) ((p)->lpVtbl->SetAutostartDelay(p, aAutostartDelay))
#define IMachine_SetAutostartDelay(p, aAutostartDelay) ((p)->lpVtbl->SetAutostartDelay(p, aAutostartDelay))
#define IMachine_get_AutostopType(p, aAutostopType) ((p)->lpVtbl->GetAutostopType(p, aAutostopType))
#define IMachine_GetAutostopType(p, aAutostopType) ((p)->lpVtbl->GetAutostopType(p, aAutostopType))
#define IMachine_put_AutostopType(p, aAutostopType) ((p)->lpVtbl->SetAutostopType(p, aAutostopType))
#define IMachine_SetAutostopType(p, aAutostopType) ((p)->lpVtbl->SetAutostopType(p, aAutostopType))
#define IMachine_get_DefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->GetDefaultFrontend(p, aDefaultFrontend))
#define IMachine_GetDefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->GetDefaultFrontend(p, aDefaultFrontend))
#define IMachine_put_DefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->SetDefaultFrontend(p, aDefaultFrontend))
#define IMachine_SetDefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->SetDefaultFrontend(p, aDefaultFrontend))
#define IMachine_get_USBProxyAvailable(p, aUSBProxyAvailable) ((p)->lpVtbl->GetUSBProxyAvailable(p, aUSBProxyAvailable))
#define IMachine_GetUSBProxyAvailable(p, aUSBProxyAvailable) ((p)->lpVtbl->GetUSBProxyAvailable(p, aUSBProxyAvailable))
#define IMachine_LockMachine(p, aSession, aLockType) ((p)->lpVtbl->LockMachine(p, aSession, aLockType))
#define IMachine_LaunchVMProcess(p, aSession, aType, aEnvironment, aProgress) ((p)->lpVtbl->LaunchVMProcess(p, aSession, aType, aEnvironment, aProgress))
#define IMachine_SetBootOrder(p, aPosition, aDevice) ((p)->lpVtbl->SetBootOrder(p, aPosition, aDevice))
#define IMachine_GetBootOrder(p, aPosition, aDevice) ((p)->lpVtbl->GetBootOrder(p, aPosition, aDevice))
#define IMachine_AttachDevice(p, aName, aControllerPort, aDevice, aType, aMedium) ((p)->lpVtbl->AttachDevice(p, aName, aControllerPort, aDevice, aType, aMedium))
#define IMachine_AttachDeviceWithoutMedium(p, aName, aControllerPort, aDevice, aType) ((p)->lpVtbl->AttachDeviceWithoutMedium(p, aName, aControllerPort, aDevice, aType))
#define IMachine_DetachDevice(p, aName, aControllerPort, aDevice) ((p)->lpVtbl->DetachDevice(p, aName, aControllerPort, aDevice))
#define IMachine_PassthroughDevice(p, aName, aControllerPort, aDevice, aPassthrough) ((p)->lpVtbl->PassthroughDevice(p, aName, aControllerPort, aDevice, aPassthrough))
#define IMachine_TemporaryEjectDevice(p, aName, aControllerPort, aDevice, aTemporaryEject) ((p)->lpVtbl->TemporaryEjectDevice(p, aName, aControllerPort, aDevice, aTemporaryEject))
#define IMachine_NonRotationalDevice(p, aName, aControllerPort, aDevice, aNonRotational) ((p)->lpVtbl->NonRotationalDevice(p, aName, aControllerPort, aDevice, aNonRotational))
#define IMachine_SetAutoDiscardForDevice(p, aName, aControllerPort, aDevice, aDiscard) ((p)->lpVtbl->SetAutoDiscardForDevice(p, aName, aControllerPort, aDevice, aDiscard))
#define IMachine_SetHotPluggableForDevice(p, aName, aControllerPort, aDevice, aHotPluggable) ((p)->lpVtbl->SetHotPluggableForDevice(p, aName, aControllerPort, aDevice, aHotPluggable))
#define IMachine_SetBandwidthGroupForDevice(p, aName, aControllerPort, aDevice, aBandwidthGroup) ((p)->lpVtbl->SetBandwidthGroupForDevice(p, aName, aControllerPort, aDevice, aBandwidthGroup))
#define IMachine_SetNoBandwidthGroupForDevice(p, aName, aControllerPort, aDevice) ((p)->lpVtbl->SetNoBandwidthGroupForDevice(p, aName, aControllerPort, aDevice))
#define IMachine_UnmountMedium(p, aName, aControllerPort, aDevice, aForce) ((p)->lpVtbl->UnmountMedium(p, aName, aControllerPort, aDevice, aForce))
#define IMachine_MountMedium(p, aName, aControllerPort, aDevice, aMedium, aForce) ((p)->lpVtbl->MountMedium(p, aName, aControllerPort, aDevice, aMedium, aForce))
#define IMachine_GetMedium(p, aName, aControllerPort, aDevice, aMedium) ((p)->lpVtbl->GetMedium(p, aName, aControllerPort, aDevice, aMedium))
#define IMachine_GetMediumAttachmentsOfController(p, aName, aMediumAttachments) ((p)->lpVtbl->GetMediumAttachmentsOfController(p, aName, aMediumAttachments))
#define IMachine_GetMediumAttachment(p, aName, aControllerPort, aDevice, aAttachment) ((p)->lpVtbl->GetMediumAttachment(p, aName, aControllerPort, aDevice, aAttachment))
#define IMachine_AttachHostPCIDevice(p, aHostAddress, aDesiredGuestAddress, aTryToUnbind) ((p)->lpVtbl->AttachHostPCIDevice(p, aHostAddress, aDesiredGuestAddress, aTryToUnbind))
#define IMachine_DetachHostPCIDevice(p, aHostAddress) ((p)->lpVtbl->DetachHostPCIDevice(p, aHostAddress))
#define IMachine_GetNetworkAdapter(p, aSlot, aAdapter) ((p)->lpVtbl->GetNetworkAdapter(p, aSlot, aAdapter))
#define IMachine_AddStorageController(p, aName, aConnectionType, aController) ((p)->lpVtbl->AddStorageController(p, aName, aConnectionType, aController))
#define IMachine_GetStorageControllerByName(p, aName, aStorageController) ((p)->lpVtbl->GetStorageControllerByName(p, aName, aStorageController))
#define IMachine_GetStorageControllerByInstance(p, aInstance, aStorageController) ((p)->lpVtbl->GetStorageControllerByInstance(p, aInstance, aStorageController))
#define IMachine_RemoveStorageController(p, aName) ((p)->lpVtbl->RemoveStorageController(p, aName))
#define IMachine_SetStorageControllerBootable(p, aName, aBootable) ((p)->lpVtbl->SetStorageControllerBootable(p, aName, aBootable))
#define IMachine_AddUSBController(p, aName, aType, aController) ((p)->lpVtbl->AddUSBController(p, aName, aType, aController))
#define IMachine_RemoveUSBController(p, aName) ((p)->lpVtbl->RemoveUSBController(p, aName))
#define IMachine_GetUSBControllerByName(p, aName, aController) ((p)->lpVtbl->GetUSBControllerByName(p, aName, aController))
#define IMachine_GetUSBControllerCountByType(p, aType, aControllers) ((p)->lpVtbl->GetUSBControllerCountByType(p, aType, aControllers))
#define IMachine_GetSerialPort(p, aSlot, aPort) ((p)->lpVtbl->GetSerialPort(p, aSlot, aPort))
#define IMachine_GetParallelPort(p, aSlot, aPort) ((p)->lpVtbl->GetParallelPort(p, aSlot, aPort))
#define IMachine_GetExtraDataKeys(p, aKeys) ((p)->lpVtbl->GetExtraDataKeys(p, aKeys))
#define IMachine_GetExtraData(p, aKey, aValue) ((p)->lpVtbl->GetExtraData(p, aKey, aValue))
#define IMachine_SetExtraData(p, aKey, aValue) ((p)->lpVtbl->SetExtraData(p, aKey, aValue))
#define IMachine_GetCPUProperty(p, aProperty, aValue) ((p)->lpVtbl->GetCPUProperty(p, aProperty, aValue))
#define IMachine_SetCPUProperty(p, aProperty, aValue) ((p)->lpVtbl->SetCPUProperty(p, aProperty, aValue))
#define IMachine_GetCPUIDLeaf(p, aId, aValEax, aValEbx, aValEcx, aValEdx) ((p)->lpVtbl->GetCPUIDLeaf(p, aId, aValEax, aValEbx, aValEcx, aValEdx))
#define IMachine_SetCPUIDLeaf(p, aId, aValEax, aValEbx, aValEcx, aValEdx) ((p)->lpVtbl->SetCPUIDLeaf(p, aId, aValEax, aValEbx, aValEcx, aValEdx))
#define IMachine_RemoveCPUIDLeaf(p, aId) ((p)->lpVtbl->RemoveCPUIDLeaf(p, aId))
#define IMachine_RemoveAllCPUIDLeaves(p) ((p)->lpVtbl->RemoveAllCPUIDLeaves(p))
#define IMachine_GetHWVirtExProperty(p, aProperty, aValue) ((p)->lpVtbl->GetHWVirtExProperty(p, aProperty, aValue))
#define IMachine_SetHWVirtExProperty(p, aProperty, aValue) ((p)->lpVtbl->SetHWVirtExProperty(p, aProperty, aValue))
#define IMachine_SetSettingsFilePath(p, aSettingsFilePath, aProgress) ((p)->lpVtbl->SetSettingsFilePath(p, aSettingsFilePath, aProgress))
#define IMachine_SaveSettings(p) ((p)->lpVtbl->SaveSettings(p))
#define IMachine_DiscardSettings(p) ((p)->lpVtbl->DiscardSettings(p))
#define IMachine_Unregister(p, aCleanupMode, aMedia) ((p)->lpVtbl->Unregister(p, aCleanupMode, aMedia))
#define IMachine_DeleteConfig(p, aMedia, aProgress) ((p)->lpVtbl->DeleteConfig(p, aMedia, aProgress))
#define IMachine_ExportTo(p, aAppliance, aLocation, aDescription) ((p)->lpVtbl->ExportTo(p, aAppliance, aLocation, aDescription))
#define IMachine_FindSnapshot(p, aNameOrId, aSnapshot) ((p)->lpVtbl->FindSnapshot(p, aNameOrId, aSnapshot))
#define IMachine_CreateSharedFolder(p, aName, aHostPath, aWritable, aAutomount) ((p)->lpVtbl->CreateSharedFolder(p, aName, aHostPath, aWritable, aAutomount))
#define IMachine_RemoveSharedFolder(p, aName) ((p)->lpVtbl->RemoveSharedFolder(p, aName))
#define IMachine_CanShowConsoleWindow(p, aCanShow) ((p)->lpVtbl->CanShowConsoleWindow(p, aCanShow))
#define IMachine_ShowConsoleWindow(p, aWinId) ((p)->lpVtbl->ShowConsoleWindow(p, aWinId))
#define IMachine_GetGuestProperty(p, aName, aValue, aTimestamp, aFlags) ((p)->lpVtbl->GetGuestProperty(p, aName, aValue, aTimestamp, aFlags))
#define IMachine_GetGuestPropertyValue(p, aProperty, aValue) ((p)->lpVtbl->GetGuestPropertyValue(p, aProperty, aValue))
#define IMachine_GetGuestPropertyTimestamp(p, aProperty, aValue) ((p)->lpVtbl->GetGuestPropertyTimestamp(p, aProperty, aValue))
#define IMachine_SetGuestProperty(p, aProperty, aValue, aFlags) ((p)->lpVtbl->SetGuestProperty(p, aProperty, aValue, aFlags))
#define IMachine_SetGuestPropertyValue(p, aProperty, aValue) ((p)->lpVtbl->SetGuestPropertyValue(p, aProperty, aValue))
#define IMachine_DeleteGuestProperty(p, aName) ((p)->lpVtbl->DeleteGuestProperty(p, aName))
#define IMachine_EnumerateGuestProperties(p, aPatterns, aNames, aValues, aTimestamps, aFlags) ((p)->lpVtbl->EnumerateGuestProperties(p, aPatterns, aNames, aValues, aTimestamps, aFlags))
#define IMachine_QuerySavedGuestScreenInfo(p, aScreenId, aOriginX, aOriginY, aWidth, aHeight, aEnabled) ((p)->lpVtbl->QuerySavedGuestScreenInfo(p, aScreenId, aOriginX, aOriginY, aWidth, aHeight, aEnabled))
#define IMachine_QuerySavedThumbnailSize(p, aScreenId, aSize, aWidth, aHeight) ((p)->lpVtbl->QuerySavedThumbnailSize(p, aScreenId, aSize, aWidth, aHeight))
#define IMachine_ReadSavedThumbnailToArray(p, aScreenId, aBGR, aWidth, aHeight, aData) ((p)->lpVtbl->ReadSavedThumbnailToArray(p, aScreenId, aBGR, aWidth, aHeight, aData))
#define IMachine_ReadSavedThumbnailPNGToArray(p, aScreenId, aWidth, aHeight, aData) ((p)->lpVtbl->ReadSavedThumbnailPNGToArray(p, aScreenId, aWidth, aHeight, aData))
#define IMachine_QuerySavedScreenshotPNGSize(p, aScreenId, aSize, aWidth, aHeight) ((p)->lpVtbl->QuerySavedScreenshotPNGSize(p, aScreenId, aSize, aWidth, aHeight))
#define IMachine_ReadSavedScreenshotPNGToArray(p, aScreenId, aWidth, aHeight, aData) ((p)->lpVtbl->ReadSavedScreenshotPNGToArray(p, aScreenId, aWidth, aHeight, aData))
#define IMachine_HotPlugCPU(p, aCpu) ((p)->lpVtbl->HotPlugCPU(p, aCpu))
#define IMachine_HotUnplugCPU(p, aCpu) ((p)->lpVtbl->HotUnplugCPU(p, aCpu))
#define IMachine_GetCPUStatus(p, aCpu, aAttached) ((p)->lpVtbl->GetCPUStatus(p, aCpu, aAttached))
#define IMachine_QueryLogFilename(p, aIdx, aFilename) ((p)->lpVtbl->QueryLogFilename(p, aIdx, aFilename))
#define IMachine_ReadLog(p, aIdx, aOffset, aSize, aData) ((p)->lpVtbl->ReadLog(p, aIdx, aOffset, aSize, aData))
#define IMachine_CloneTo(p, aTarget, aMode, aOptions, aProgress) ((p)->lpVtbl->CloneTo(p, aTarget, aMode, aOptions, aProgress))
#endif /* VBOX_WITH_GLUE */

interface IMachine
{
#ifndef VBOX_WITH_GLUE
    struct IMachine_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMachineVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMachine declaration */


/* Start of struct IEmulatedUSB declaration */
#define IEMULATEDUSB_IID_STR "38cc4dfd-8bb2-4d40-aebe-699eead8c2dd"
#define IEMULATEDUSB_IID { \
    0x38cc4dfd, 0x8bb2, 0x4d40, \
    { 0xae, 0xbe, 0x69, 0x9e, 0xea, 0xd8, 0xc2, 0xdd } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IEmulatedUSB);
#ifndef VBOX_WITH_GLUE
struct IEmulatedUSB_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetWebcams)(IEmulatedUSB *pThis, PRUint32 *webcamsSize, PRUnichar * **webcams);

    nsresult (*WebcamAttach)(
        IEmulatedUSB *pThis,
        PRUnichar * path,
        PRUnichar * settings
    );

    nsresult (*WebcamDetach)(
        IEmulatedUSB *pThis,
        PRUnichar * path
    );

};
#else /* VBOX_WITH_GLUE */
struct IEmulatedUSBVtbl
{
    nsresult (*QueryInterface)(IEmulatedUSB *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IEmulatedUSB *pThis);
    nsrefcnt (*Release)(IEmulatedUSB *pThis);
    nsresult (*GetWebcams)(IEmulatedUSB *pThis, PRUint32 *webcamsSize, PRUnichar * **webcams);

    nsresult (*WebcamAttach)(
        IEmulatedUSB *pThis,
        PRUnichar * path,
        PRUnichar * settings
    );

    nsresult (*WebcamDetach)(
        IEmulatedUSB *pThis,
        PRUnichar * path
    );

};
#define IEmulatedUSB_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IEmulatedUSB_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IEmulatedUSB_Release(p) ((p)->lpVtbl->Release(p))
#define IEmulatedUSB_get_Webcams(p, aWebcams) ((p)->lpVtbl->GetWebcams(p, aWebcams))
#define IEmulatedUSB_GetWebcams(p, aWebcams) ((p)->lpVtbl->GetWebcams(p, aWebcams))
#define IEmulatedUSB_WebcamAttach(p, aPath, aSettings) ((p)->lpVtbl->WebcamAttach(p, aPath, aSettings))
#define IEmulatedUSB_WebcamDetach(p, aPath) ((p)->lpVtbl->WebcamDetach(p, aPath))
#endif /* VBOX_WITH_GLUE */

interface IEmulatedUSB
{
#ifndef VBOX_WITH_GLUE
    struct IEmulatedUSB_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IEmulatedUSBVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IEmulatedUSB declaration */


/* Start of struct IVRDEServerInfo declaration */
#define IVRDESERVERINFO_IID_STR "714434a1-58c3-4aab-9049-7652c5df113b"
#define IVRDESERVERINFO_IID { \
    0x714434a1, 0x58c3, 0x4aab, \
    { 0x90, 0x49, 0x76, 0x52, 0xc5, 0xdf, 0x11, 0x3b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVRDEServerInfo);
#ifndef VBOX_WITH_GLUE
struct IVRDEServerInfo_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetActive)(IVRDEServerInfo *pThis, PRBool *active);

    nsresult (*GetPort)(IVRDEServerInfo *pThis, PRInt32 *port);

    nsresult (*GetNumberOfClients)(IVRDEServerInfo *pThis, PRUint32 *numberOfClients);

    nsresult (*GetBeginTime)(IVRDEServerInfo *pThis, PRInt64 *beginTime);

    nsresult (*GetEndTime)(IVRDEServerInfo *pThis, PRInt64 *endTime);

    nsresult (*GetBytesSent)(IVRDEServerInfo *pThis, PRInt64 *bytesSent);

    nsresult (*GetBytesSentTotal)(IVRDEServerInfo *pThis, PRInt64 *bytesSentTotal);

    nsresult (*GetBytesReceived)(IVRDEServerInfo *pThis, PRInt64 *bytesReceived);

    nsresult (*GetBytesReceivedTotal)(IVRDEServerInfo *pThis, PRInt64 *bytesReceivedTotal);

    nsresult (*GetUser)(IVRDEServerInfo *pThis, PRUnichar * *user);

    nsresult (*GetDomain)(IVRDEServerInfo *pThis, PRUnichar * *domain);

    nsresult (*GetClientName)(IVRDEServerInfo *pThis, PRUnichar * *clientName);

    nsresult (*GetClientIP)(IVRDEServerInfo *pThis, PRUnichar * *clientIP);

    nsresult (*GetClientVersion)(IVRDEServerInfo *pThis, PRUint32 *clientVersion);

    nsresult (*GetEncryptionStyle)(IVRDEServerInfo *pThis, PRUint32 *encryptionStyle);

};
#else /* VBOX_WITH_GLUE */
struct IVRDEServerInfoVtbl
{
    nsresult (*QueryInterface)(IVRDEServerInfo *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVRDEServerInfo *pThis);
    nsrefcnt (*Release)(IVRDEServerInfo *pThis);
    nsresult (*GetActive)(IVRDEServerInfo *pThis, PRBool *active);

    nsresult (*GetPort)(IVRDEServerInfo *pThis, PRInt32 *port);

    nsresult (*GetNumberOfClients)(IVRDEServerInfo *pThis, PRUint32 *numberOfClients);

    nsresult (*GetBeginTime)(IVRDEServerInfo *pThis, PRInt64 *beginTime);

    nsresult (*GetEndTime)(IVRDEServerInfo *pThis, PRInt64 *endTime);

    nsresult (*GetBytesSent)(IVRDEServerInfo *pThis, PRInt64 *bytesSent);

    nsresult (*GetBytesSentTotal)(IVRDEServerInfo *pThis, PRInt64 *bytesSentTotal);

    nsresult (*GetBytesReceived)(IVRDEServerInfo *pThis, PRInt64 *bytesReceived);

    nsresult (*GetBytesReceivedTotal)(IVRDEServerInfo *pThis, PRInt64 *bytesReceivedTotal);

    nsresult (*GetUser)(IVRDEServerInfo *pThis, PRUnichar * *user);

    nsresult (*GetDomain)(IVRDEServerInfo *pThis, PRUnichar * *domain);

    nsresult (*GetClientName)(IVRDEServerInfo *pThis, PRUnichar * *clientName);

    nsresult (*GetClientIP)(IVRDEServerInfo *pThis, PRUnichar * *clientIP);

    nsresult (*GetClientVersion)(IVRDEServerInfo *pThis, PRUint32 *clientVersion);

    nsresult (*GetEncryptionStyle)(IVRDEServerInfo *pThis, PRUint32 *encryptionStyle);

};
#define IVRDEServerInfo_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVRDEServerInfo_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVRDEServerInfo_Release(p) ((p)->lpVtbl->Release(p))
#define IVRDEServerInfo_get_Active(p, aActive) ((p)->lpVtbl->GetActive(p, aActive))
#define IVRDEServerInfo_GetActive(p, aActive) ((p)->lpVtbl->GetActive(p, aActive))
#define IVRDEServerInfo_get_Port(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IVRDEServerInfo_GetPort(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IVRDEServerInfo_get_NumberOfClients(p, aNumberOfClients) ((p)->lpVtbl->GetNumberOfClients(p, aNumberOfClients))
#define IVRDEServerInfo_GetNumberOfClients(p, aNumberOfClients) ((p)->lpVtbl->GetNumberOfClients(p, aNumberOfClients))
#define IVRDEServerInfo_get_BeginTime(p, aBeginTime) ((p)->lpVtbl->GetBeginTime(p, aBeginTime))
#define IVRDEServerInfo_GetBeginTime(p, aBeginTime) ((p)->lpVtbl->GetBeginTime(p, aBeginTime))
#define IVRDEServerInfo_get_EndTime(p, aEndTime) ((p)->lpVtbl->GetEndTime(p, aEndTime))
#define IVRDEServerInfo_GetEndTime(p, aEndTime) ((p)->lpVtbl->GetEndTime(p, aEndTime))
#define IVRDEServerInfo_get_BytesSent(p, aBytesSent) ((p)->lpVtbl->GetBytesSent(p, aBytesSent))
#define IVRDEServerInfo_GetBytesSent(p, aBytesSent) ((p)->lpVtbl->GetBytesSent(p, aBytesSent))
#define IVRDEServerInfo_get_BytesSentTotal(p, aBytesSentTotal) ((p)->lpVtbl->GetBytesSentTotal(p, aBytesSentTotal))
#define IVRDEServerInfo_GetBytesSentTotal(p, aBytesSentTotal) ((p)->lpVtbl->GetBytesSentTotal(p, aBytesSentTotal))
#define IVRDEServerInfo_get_BytesReceived(p, aBytesReceived) ((p)->lpVtbl->GetBytesReceived(p, aBytesReceived))
#define IVRDEServerInfo_GetBytesReceived(p, aBytesReceived) ((p)->lpVtbl->GetBytesReceived(p, aBytesReceived))
#define IVRDEServerInfo_get_BytesReceivedTotal(p, aBytesReceivedTotal) ((p)->lpVtbl->GetBytesReceivedTotal(p, aBytesReceivedTotal))
#define IVRDEServerInfo_GetBytesReceivedTotal(p, aBytesReceivedTotal) ((p)->lpVtbl->GetBytesReceivedTotal(p, aBytesReceivedTotal))
#define IVRDEServerInfo_get_User(p, aUser) ((p)->lpVtbl->GetUser(p, aUser))
#define IVRDEServerInfo_GetUser(p, aUser) ((p)->lpVtbl->GetUser(p, aUser))
#define IVRDEServerInfo_get_Domain(p, aDomain) ((p)->lpVtbl->GetDomain(p, aDomain))
#define IVRDEServerInfo_GetDomain(p, aDomain) ((p)->lpVtbl->GetDomain(p, aDomain))
#define IVRDEServerInfo_get_ClientName(p, aClientName) ((p)->lpVtbl->GetClientName(p, aClientName))
#define IVRDEServerInfo_GetClientName(p, aClientName) ((p)->lpVtbl->GetClientName(p, aClientName))
#define IVRDEServerInfo_get_ClientIP(p, aClientIP) ((p)->lpVtbl->GetClientIP(p, aClientIP))
#define IVRDEServerInfo_GetClientIP(p, aClientIP) ((p)->lpVtbl->GetClientIP(p, aClientIP))
#define IVRDEServerInfo_get_ClientVersion(p, aClientVersion) ((p)->lpVtbl->GetClientVersion(p, aClientVersion))
#define IVRDEServerInfo_GetClientVersion(p, aClientVersion) ((p)->lpVtbl->GetClientVersion(p, aClientVersion))
#define IVRDEServerInfo_get_EncryptionStyle(p, aEncryptionStyle) ((p)->lpVtbl->GetEncryptionStyle(p, aEncryptionStyle))
#define IVRDEServerInfo_GetEncryptionStyle(p, aEncryptionStyle) ((p)->lpVtbl->GetEncryptionStyle(p, aEncryptionStyle))
#endif /* VBOX_WITH_GLUE */

interface IVRDEServerInfo
{
#ifndef VBOX_WITH_GLUE
    struct IVRDEServerInfo_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVRDEServerInfoVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVRDEServerInfo declaration */


/* Start of struct IConsole declaration */
#define ICONSOLE_IID_STR "8ab7c520-2442-4b66-8d74-4ff1e195d2b6"
#define ICONSOLE_IID { \
    0x8ab7c520, 0x2442, 0x4b66, \
    { 0x8d, 0x74, 0x4f, 0xf1, 0xe1, 0x95, 0xd2, 0xb6 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IConsole);
#ifndef VBOX_WITH_GLUE
struct IConsole_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetMachine)(IConsole *pThis, IMachine * *machine);

    nsresult (*GetState)(IConsole *pThis, PRUint32 *state);

    nsresult (*GetGuest)(IConsole *pThis, IGuest * *guest);

    nsresult (*GetKeyboard)(IConsole *pThis, IKeyboard * *keyboard);

    nsresult (*GetMouse)(IConsole *pThis, IMouse * *mouse);

    nsresult (*GetDisplay)(IConsole *pThis, IDisplay * *display);

    nsresult (*GetDebugger)(IConsole *pThis, IMachineDebugger * *debugger);

    nsresult (*GetUSBDevices)(IConsole *pThis, PRUint32 *USBDevicesSize, IUSBDevice * **USBDevices);

    nsresult (*GetRemoteUSBDevices)(IConsole *pThis, PRUint32 *remoteUSBDevicesSize, IHostUSBDevice * **remoteUSBDevices);

    nsresult (*GetSharedFolders)(IConsole *pThis, PRUint32 *sharedFoldersSize, ISharedFolder * **sharedFolders);

    nsresult (*GetVRDEServerInfo)(IConsole *pThis, IVRDEServerInfo * *VRDEServerInfo);

    nsresult (*GetEventSource)(IConsole *pThis, IEventSource * *eventSource);

    nsresult (*GetAttachedPCIDevices)(IConsole *pThis, PRUint32 *attachedPCIDevicesSize, IPCIDeviceAttachment * **attachedPCIDevices);

    nsresult (*GetUseHostClipboard)(IConsole *pThis, PRBool *useHostClipboard);
    nsresult (*SetUseHostClipboard)(IConsole *pThis, PRBool useHostClipboard);

    nsresult (*GetEmulatedUSB)(IConsole *pThis, IEmulatedUSB * *emulatedUSB);

    nsresult (*PowerUp)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*PowerUpPaused)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*PowerDown)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*Reset)(IConsole *pThis );

    nsresult (*Pause)(IConsole *pThis );

    nsresult (*Resume)(IConsole *pThis );

    nsresult (*PowerButton)(IConsole *pThis );

    nsresult (*SleepButton)(IConsole *pThis );

    nsresult (*GetPowerButtonHandled)(
        IConsole *pThis,
        PRBool * handled
    );

    nsresult (*GetGuestEnteredACPIMode)(
        IConsole *pThis,
        PRBool * entered
    );

    nsresult (*SaveState)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*AdoptSavedState)(
        IConsole *pThis,
        PRUnichar * savedStateFile
    );

    nsresult (*DiscardSavedState)(
        IConsole *pThis,
        PRBool fRemoveFile
    );

    nsresult (*GetDeviceActivity)(
        IConsole *pThis,
        PRUint32 type,
        PRUint32 * activity
    );

    nsresult (*AttachUSBDevice)(
        IConsole *pThis,
        PRUnichar * id
    );

    nsresult (*DetachUSBDevice)(
        IConsole *pThis,
        PRUnichar * id,
        IUSBDevice * * device
    );

    nsresult (*FindUSBDeviceByAddress)(
        IConsole *pThis,
        PRUnichar * name,
        IUSBDevice * * device
    );

    nsresult (*FindUSBDeviceById)(
        IConsole *pThis,
        PRUnichar * id,
        IUSBDevice * * device
    );

    nsresult (*CreateSharedFolder)(
        IConsole *pThis,
        PRUnichar * name,
        PRUnichar * hostPath,
        PRBool writable,
        PRBool automount
    );

    nsresult (*RemoveSharedFolder)(
        IConsole *pThis,
        PRUnichar * name
    );

    nsresult (*TakeSnapshot)(
        IConsole *pThis,
        PRUnichar * name,
        PRUnichar * description,
        IProgress * * progress
    );

    nsresult (*DeleteSnapshot)(
        IConsole *pThis,
        PRUnichar * id,
        IProgress * * progress
    );

    nsresult (*DeleteSnapshotAndAllChildren)(
        IConsole *pThis,
        PRUnichar * id,
        IProgress * * progress
    );

    nsresult (*DeleteSnapshotRange)(
        IConsole *pThis,
        PRUnichar * startId,
        PRUnichar * endId,
        IProgress * * progress
    );

    nsresult (*RestoreSnapshot)(
        IConsole *pThis,
        ISnapshot * snapshot,
        IProgress * * progress
    );

    nsresult (*Teleport)(
        IConsole *pThis,
        PRUnichar * hostname,
        PRUint32 tcpport,
        PRUnichar * password,
        PRUint32 maxDowntime,
        IProgress * * progress
    );

};
#else /* VBOX_WITH_GLUE */
struct IConsoleVtbl
{
    nsresult (*QueryInterface)(IConsole *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IConsole *pThis);
    nsrefcnt (*Release)(IConsole *pThis);
    nsresult (*GetMachine)(IConsole *pThis, IMachine * *machine);

    nsresult (*GetState)(IConsole *pThis, PRUint32 *state);

    nsresult (*GetGuest)(IConsole *pThis, IGuest * *guest);

    nsresult (*GetKeyboard)(IConsole *pThis, IKeyboard * *keyboard);

    nsresult (*GetMouse)(IConsole *pThis, IMouse * *mouse);

    nsresult (*GetDisplay)(IConsole *pThis, IDisplay * *display);

    nsresult (*GetDebugger)(IConsole *pThis, IMachineDebugger * *debugger);

    nsresult (*GetUSBDevices)(IConsole *pThis, PRUint32 *USBDevicesSize, IUSBDevice * **USBDevices);

    nsresult (*GetRemoteUSBDevices)(IConsole *pThis, PRUint32 *remoteUSBDevicesSize, IHostUSBDevice * **remoteUSBDevices);

    nsresult (*GetSharedFolders)(IConsole *pThis, PRUint32 *sharedFoldersSize, ISharedFolder * **sharedFolders);

    nsresult (*GetVRDEServerInfo)(IConsole *pThis, IVRDEServerInfo * *VRDEServerInfo);

    nsresult (*GetEventSource)(IConsole *pThis, IEventSource * *eventSource);

    nsresult (*GetAttachedPCIDevices)(IConsole *pThis, PRUint32 *attachedPCIDevicesSize, IPCIDeviceAttachment * **attachedPCIDevices);

    nsresult (*GetUseHostClipboard)(IConsole *pThis, PRBool *useHostClipboard);
    nsresult (*SetUseHostClipboard)(IConsole *pThis, PRBool useHostClipboard);

    nsresult (*GetEmulatedUSB)(IConsole *pThis, IEmulatedUSB * *emulatedUSB);

    nsresult (*PowerUp)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*PowerUpPaused)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*PowerDown)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*Reset)(IConsole *pThis );

    nsresult (*Pause)(IConsole *pThis );

    nsresult (*Resume)(IConsole *pThis );

    nsresult (*PowerButton)(IConsole *pThis );

    nsresult (*SleepButton)(IConsole *pThis );

    nsresult (*GetPowerButtonHandled)(
        IConsole *pThis,
        PRBool * handled
    );

    nsresult (*GetGuestEnteredACPIMode)(
        IConsole *pThis,
        PRBool * entered
    );

    nsresult (*SaveState)(
        IConsole *pThis,
        IProgress * * progress
    );

    nsresult (*AdoptSavedState)(
        IConsole *pThis,
        PRUnichar * savedStateFile
    );

    nsresult (*DiscardSavedState)(
        IConsole *pThis,
        PRBool fRemoveFile
    );

    nsresult (*GetDeviceActivity)(
        IConsole *pThis,
        PRUint32 type,
        PRUint32 * activity
    );

    nsresult (*AttachUSBDevice)(
        IConsole *pThis,
        PRUnichar * id
    );

    nsresult (*DetachUSBDevice)(
        IConsole *pThis,
        PRUnichar * id,
        IUSBDevice * * device
    );

    nsresult (*FindUSBDeviceByAddress)(
        IConsole *pThis,
        PRUnichar * name,
        IUSBDevice * * device
    );

    nsresult (*FindUSBDeviceById)(
        IConsole *pThis,
        PRUnichar * id,
        IUSBDevice * * device
    );

    nsresult (*CreateSharedFolder)(
        IConsole *pThis,
        PRUnichar * name,
        PRUnichar * hostPath,
        PRBool writable,
        PRBool automount
    );

    nsresult (*RemoveSharedFolder)(
        IConsole *pThis,
        PRUnichar * name
    );

    nsresult (*TakeSnapshot)(
        IConsole *pThis,
        PRUnichar * name,
        PRUnichar * description,
        IProgress * * progress
    );

    nsresult (*DeleteSnapshot)(
        IConsole *pThis,
        PRUnichar * id,
        IProgress * * progress
    );

    nsresult (*DeleteSnapshotAndAllChildren)(
        IConsole *pThis,
        PRUnichar * id,
        IProgress * * progress
    );

    nsresult (*DeleteSnapshotRange)(
        IConsole *pThis,
        PRUnichar * startId,
        PRUnichar * endId,
        IProgress * * progress
    );

    nsresult (*RestoreSnapshot)(
        IConsole *pThis,
        ISnapshot * snapshot,
        IProgress * * progress
    );

    nsresult (*Teleport)(
        IConsole *pThis,
        PRUnichar * hostname,
        PRUint32 tcpport,
        PRUnichar * password,
        PRUint32 maxDowntime,
        IProgress * * progress
    );

};
#define IConsole_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IConsole_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IConsole_Release(p) ((p)->lpVtbl->Release(p))
#define IConsole_get_Machine(p, aMachine) ((p)->lpVtbl->GetMachine(p, aMachine))
#define IConsole_GetMachine(p, aMachine) ((p)->lpVtbl->GetMachine(p, aMachine))
#define IConsole_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IConsole_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IConsole_get_Guest(p, aGuest) ((p)->lpVtbl->GetGuest(p, aGuest))
#define IConsole_GetGuest(p, aGuest) ((p)->lpVtbl->GetGuest(p, aGuest))
#define IConsole_get_Keyboard(p, aKeyboard) ((p)->lpVtbl->GetKeyboard(p, aKeyboard))
#define IConsole_GetKeyboard(p, aKeyboard) ((p)->lpVtbl->GetKeyboard(p, aKeyboard))
#define IConsole_get_Mouse(p, aMouse) ((p)->lpVtbl->GetMouse(p, aMouse))
#define IConsole_GetMouse(p, aMouse) ((p)->lpVtbl->GetMouse(p, aMouse))
#define IConsole_get_Display(p, aDisplay) ((p)->lpVtbl->GetDisplay(p, aDisplay))
#define IConsole_GetDisplay(p, aDisplay) ((p)->lpVtbl->GetDisplay(p, aDisplay))
#define IConsole_get_Debugger(p, aDebugger) ((p)->lpVtbl->GetDebugger(p, aDebugger))
#define IConsole_GetDebugger(p, aDebugger) ((p)->lpVtbl->GetDebugger(p, aDebugger))
#define IConsole_get_USBDevices(p, aUSBDevices) ((p)->lpVtbl->GetUSBDevices(p, aUSBDevices))
#define IConsole_GetUSBDevices(p, aUSBDevices) ((p)->lpVtbl->GetUSBDevices(p, aUSBDevices))
#define IConsole_get_RemoteUSBDevices(p, aRemoteUSBDevices) ((p)->lpVtbl->GetRemoteUSBDevices(p, aRemoteUSBDevices))
#define IConsole_GetRemoteUSBDevices(p, aRemoteUSBDevices) ((p)->lpVtbl->GetRemoteUSBDevices(p, aRemoteUSBDevices))
#define IConsole_get_SharedFolders(p, aSharedFolders) ((p)->lpVtbl->GetSharedFolders(p, aSharedFolders))
#define IConsole_GetSharedFolders(p, aSharedFolders) ((p)->lpVtbl->GetSharedFolders(p, aSharedFolders))
#define IConsole_get_VRDEServerInfo(p, aVRDEServerInfo) ((p)->lpVtbl->GetVRDEServerInfo(p, aVRDEServerInfo))
#define IConsole_GetVRDEServerInfo(p, aVRDEServerInfo) ((p)->lpVtbl->GetVRDEServerInfo(p, aVRDEServerInfo))
#define IConsole_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IConsole_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IConsole_get_AttachedPCIDevices(p, aAttachedPCIDevices) ((p)->lpVtbl->GetAttachedPCIDevices(p, aAttachedPCIDevices))
#define IConsole_GetAttachedPCIDevices(p, aAttachedPCIDevices) ((p)->lpVtbl->GetAttachedPCIDevices(p, aAttachedPCIDevices))
#define IConsole_get_UseHostClipboard(p, aUseHostClipboard) ((p)->lpVtbl->GetUseHostClipboard(p, aUseHostClipboard))
#define IConsole_GetUseHostClipboard(p, aUseHostClipboard) ((p)->lpVtbl->GetUseHostClipboard(p, aUseHostClipboard))
#define IConsole_put_UseHostClipboard(p, aUseHostClipboard) ((p)->lpVtbl->SetUseHostClipboard(p, aUseHostClipboard))
#define IConsole_SetUseHostClipboard(p, aUseHostClipboard) ((p)->lpVtbl->SetUseHostClipboard(p, aUseHostClipboard))
#define IConsole_get_EmulatedUSB(p, aEmulatedUSB) ((p)->lpVtbl->GetEmulatedUSB(p, aEmulatedUSB))
#define IConsole_GetEmulatedUSB(p, aEmulatedUSB) ((p)->lpVtbl->GetEmulatedUSB(p, aEmulatedUSB))
#define IConsole_PowerUp(p, aProgress) ((p)->lpVtbl->PowerUp(p, aProgress))
#define IConsole_PowerUpPaused(p, aProgress) ((p)->lpVtbl->PowerUpPaused(p, aProgress))
#define IConsole_PowerDown(p, aProgress) ((p)->lpVtbl->PowerDown(p, aProgress))
#define IConsole_Reset(p) ((p)->lpVtbl->Reset(p))
#define IConsole_Pause(p) ((p)->lpVtbl->Pause(p))
#define IConsole_Resume(p) ((p)->lpVtbl->Resume(p))
#define IConsole_PowerButton(p) ((p)->lpVtbl->PowerButton(p))
#define IConsole_SleepButton(p) ((p)->lpVtbl->SleepButton(p))
#define IConsole_GetPowerButtonHandled(p, aHandled) ((p)->lpVtbl->GetPowerButtonHandled(p, aHandled))
#define IConsole_GetGuestEnteredACPIMode(p, aEntered) ((p)->lpVtbl->GetGuestEnteredACPIMode(p, aEntered))
#define IConsole_SaveState(p, aProgress) ((p)->lpVtbl->SaveState(p, aProgress))
#define IConsole_AdoptSavedState(p, aSavedStateFile) ((p)->lpVtbl->AdoptSavedState(p, aSavedStateFile))
#define IConsole_DiscardSavedState(p, aFRemoveFile) ((p)->lpVtbl->DiscardSavedState(p, aFRemoveFile))
#define IConsole_GetDeviceActivity(p, aType, aActivity) ((p)->lpVtbl->GetDeviceActivity(p, aType, aActivity))
#define IConsole_AttachUSBDevice(p, aId) ((p)->lpVtbl->AttachUSBDevice(p, aId))
#define IConsole_DetachUSBDevice(p, aId, aDevice) ((p)->lpVtbl->DetachUSBDevice(p, aId, aDevice))
#define IConsole_FindUSBDeviceByAddress(p, aName, aDevice) ((p)->lpVtbl->FindUSBDeviceByAddress(p, aName, aDevice))
#define IConsole_FindUSBDeviceById(p, aId, aDevice) ((p)->lpVtbl->FindUSBDeviceById(p, aId, aDevice))
#define IConsole_CreateSharedFolder(p, aName, aHostPath, aWritable, aAutomount) ((p)->lpVtbl->CreateSharedFolder(p, aName, aHostPath, aWritable, aAutomount))
#define IConsole_RemoveSharedFolder(p, aName) ((p)->lpVtbl->RemoveSharedFolder(p, aName))
#define IConsole_TakeSnapshot(p, aName, aDescription, aProgress) ((p)->lpVtbl->TakeSnapshot(p, aName, aDescription, aProgress))
#define IConsole_DeleteSnapshot(p, aId, aProgress) ((p)->lpVtbl->DeleteSnapshot(p, aId, aProgress))
#define IConsole_DeleteSnapshotAndAllChildren(p, aId, aProgress) ((p)->lpVtbl->DeleteSnapshotAndAllChildren(p, aId, aProgress))
#define IConsole_DeleteSnapshotRange(p, aStartId, aEndId, aProgress) ((p)->lpVtbl->DeleteSnapshotRange(p, aStartId, aEndId, aProgress))
#define IConsole_RestoreSnapshot(p, aSnapshot, aProgress) ((p)->lpVtbl->RestoreSnapshot(p, aSnapshot, aProgress))
#define IConsole_Teleport(p, aHostname, aTcpport, aPassword, aMaxDowntime, aProgress) ((p)->lpVtbl->Teleport(p, aHostname, aTcpport, aPassword, aMaxDowntime, aProgress))
#endif /* VBOX_WITH_GLUE */

interface IConsole
{
#ifndef VBOX_WITH_GLUE
    struct IConsole_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IConsoleVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IConsole declaration */


/* Start of struct IHostNetworkInterface declaration */
#define IHOSTNETWORKINTERFACE_IID_STR "f6e556f9-d598-409b-898c-8ba99d9b05ae"
#define IHOSTNETWORKINTERFACE_IID { \
    0xf6e556f9, 0xd598, 0x409b, \
    { 0x89, 0x8c, 0x8b, 0xa9, 0x9d, 0x9b, 0x05, 0xae } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHostNetworkInterface);
#ifndef VBOX_WITH_GLUE
struct IHostNetworkInterface_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IHostNetworkInterface *pThis, PRUnichar * *name);

    nsresult (*GetShortName)(IHostNetworkInterface *pThis, PRUnichar * *shortName);

    nsresult (*GetId)(IHostNetworkInterface *pThis, PRUnichar * *id);

    nsresult (*GetNetworkName)(IHostNetworkInterface *pThis, PRUnichar * *networkName);

    nsresult (*GetDHCPEnabled)(IHostNetworkInterface *pThis, PRBool *DHCPEnabled);

    nsresult (*GetIPAddress)(IHostNetworkInterface *pThis, PRUnichar * *IPAddress);

    nsresult (*GetNetworkMask)(IHostNetworkInterface *pThis, PRUnichar * *networkMask);

    nsresult (*GetIPV6Supported)(IHostNetworkInterface *pThis, PRBool *IPV6Supported);

    nsresult (*GetIPV6Address)(IHostNetworkInterface *pThis, PRUnichar * *IPV6Address);

    nsresult (*GetIPV6NetworkMaskPrefixLength)(IHostNetworkInterface *pThis, PRUint32 *IPV6NetworkMaskPrefixLength);

    nsresult (*GetHardwareAddress)(IHostNetworkInterface *pThis, PRUnichar * *hardwareAddress);

    nsresult (*GetMediumType)(IHostNetworkInterface *pThis, PRUint32 *mediumType);

    nsresult (*GetStatus)(IHostNetworkInterface *pThis, PRUint32 *status);

    nsresult (*GetInterfaceType)(IHostNetworkInterface *pThis, PRUint32 *interfaceType);

    nsresult (*EnableStaticIPConfig)(
        IHostNetworkInterface *pThis,
        PRUnichar * IPAddress,
        PRUnichar * networkMask
    );

    nsresult (*EnableStaticIPConfigV6)(
        IHostNetworkInterface *pThis,
        PRUnichar * IPV6Address,
        PRUint32 IPV6NetworkMaskPrefixLength
    );

    nsresult (*EnableDynamicIPConfig)(IHostNetworkInterface *pThis );

    nsresult (*DHCPRediscover)(IHostNetworkInterface *pThis );

};
#else /* VBOX_WITH_GLUE */
struct IHostNetworkInterfaceVtbl
{
    nsresult (*QueryInterface)(IHostNetworkInterface *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHostNetworkInterface *pThis);
    nsrefcnt (*Release)(IHostNetworkInterface *pThis);
    nsresult (*GetName)(IHostNetworkInterface *pThis, PRUnichar * *name);

    nsresult (*GetShortName)(IHostNetworkInterface *pThis, PRUnichar * *shortName);

    nsresult (*GetId)(IHostNetworkInterface *pThis, PRUnichar * *id);

    nsresult (*GetNetworkName)(IHostNetworkInterface *pThis, PRUnichar * *networkName);

    nsresult (*GetDHCPEnabled)(IHostNetworkInterface *pThis, PRBool *DHCPEnabled);

    nsresult (*GetIPAddress)(IHostNetworkInterface *pThis, PRUnichar * *IPAddress);

    nsresult (*GetNetworkMask)(IHostNetworkInterface *pThis, PRUnichar * *networkMask);

    nsresult (*GetIPV6Supported)(IHostNetworkInterface *pThis, PRBool *IPV6Supported);

    nsresult (*GetIPV6Address)(IHostNetworkInterface *pThis, PRUnichar * *IPV6Address);

    nsresult (*GetIPV6NetworkMaskPrefixLength)(IHostNetworkInterface *pThis, PRUint32 *IPV6NetworkMaskPrefixLength);

    nsresult (*GetHardwareAddress)(IHostNetworkInterface *pThis, PRUnichar * *hardwareAddress);

    nsresult (*GetMediumType)(IHostNetworkInterface *pThis, PRUint32 *mediumType);

    nsresult (*GetStatus)(IHostNetworkInterface *pThis, PRUint32 *status);

    nsresult (*GetInterfaceType)(IHostNetworkInterface *pThis, PRUint32 *interfaceType);

    nsresult (*EnableStaticIPConfig)(
        IHostNetworkInterface *pThis,
        PRUnichar * IPAddress,
        PRUnichar * networkMask
    );

    nsresult (*EnableStaticIPConfigV6)(
        IHostNetworkInterface *pThis,
        PRUnichar * IPV6Address,
        PRUint32 IPV6NetworkMaskPrefixLength
    );

    nsresult (*EnableDynamicIPConfig)(IHostNetworkInterface *pThis );

    nsresult (*DHCPRediscover)(IHostNetworkInterface *pThis );

};
#define IHostNetworkInterface_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHostNetworkInterface_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHostNetworkInterface_Release(p) ((p)->lpVtbl->Release(p))
#define IHostNetworkInterface_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IHostNetworkInterface_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IHostNetworkInterface_get_ShortName(p, aShortName) ((p)->lpVtbl->GetShortName(p, aShortName))
#define IHostNetworkInterface_GetShortName(p, aShortName) ((p)->lpVtbl->GetShortName(p, aShortName))
#define IHostNetworkInterface_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IHostNetworkInterface_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IHostNetworkInterface_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define IHostNetworkInterface_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define IHostNetworkInterface_get_DHCPEnabled(p, aDHCPEnabled) ((p)->lpVtbl->GetDHCPEnabled(p, aDHCPEnabled))
#define IHostNetworkInterface_GetDHCPEnabled(p, aDHCPEnabled) ((p)->lpVtbl->GetDHCPEnabled(p, aDHCPEnabled))
#define IHostNetworkInterface_get_IPAddress(p, aIPAddress) ((p)->lpVtbl->GetIPAddress(p, aIPAddress))
#define IHostNetworkInterface_GetIPAddress(p, aIPAddress) ((p)->lpVtbl->GetIPAddress(p, aIPAddress))
#define IHostNetworkInterface_get_NetworkMask(p, aNetworkMask) ((p)->lpVtbl->GetNetworkMask(p, aNetworkMask))
#define IHostNetworkInterface_GetNetworkMask(p, aNetworkMask) ((p)->lpVtbl->GetNetworkMask(p, aNetworkMask))
#define IHostNetworkInterface_get_IPV6Supported(p, aIPV6Supported) ((p)->lpVtbl->GetIPV6Supported(p, aIPV6Supported))
#define IHostNetworkInterface_GetIPV6Supported(p, aIPV6Supported) ((p)->lpVtbl->GetIPV6Supported(p, aIPV6Supported))
#define IHostNetworkInterface_get_IPV6Address(p, aIPV6Address) ((p)->lpVtbl->GetIPV6Address(p, aIPV6Address))
#define IHostNetworkInterface_GetIPV6Address(p, aIPV6Address) ((p)->lpVtbl->GetIPV6Address(p, aIPV6Address))
#define IHostNetworkInterface_get_IPV6NetworkMaskPrefixLength(p, aIPV6NetworkMaskPrefixLength) ((p)->lpVtbl->GetIPV6NetworkMaskPrefixLength(p, aIPV6NetworkMaskPrefixLength))
#define IHostNetworkInterface_GetIPV6NetworkMaskPrefixLength(p, aIPV6NetworkMaskPrefixLength) ((p)->lpVtbl->GetIPV6NetworkMaskPrefixLength(p, aIPV6NetworkMaskPrefixLength))
#define IHostNetworkInterface_get_HardwareAddress(p, aHardwareAddress) ((p)->lpVtbl->GetHardwareAddress(p, aHardwareAddress))
#define IHostNetworkInterface_GetHardwareAddress(p, aHardwareAddress) ((p)->lpVtbl->GetHardwareAddress(p, aHardwareAddress))
#define IHostNetworkInterface_get_MediumType(p, aMediumType) ((p)->lpVtbl->GetMediumType(p, aMediumType))
#define IHostNetworkInterface_GetMediumType(p, aMediumType) ((p)->lpVtbl->GetMediumType(p, aMediumType))
#define IHostNetworkInterface_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IHostNetworkInterface_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IHostNetworkInterface_get_InterfaceType(p, aInterfaceType) ((p)->lpVtbl->GetInterfaceType(p, aInterfaceType))
#define IHostNetworkInterface_GetInterfaceType(p, aInterfaceType) ((p)->lpVtbl->GetInterfaceType(p, aInterfaceType))
#define IHostNetworkInterface_EnableStaticIPConfig(p, aIPAddress, aNetworkMask) ((p)->lpVtbl->EnableStaticIPConfig(p, aIPAddress, aNetworkMask))
#define IHostNetworkInterface_EnableStaticIPConfigV6(p, aIPV6Address, aIPV6NetworkMaskPrefixLength) ((p)->lpVtbl->EnableStaticIPConfigV6(p, aIPV6Address, aIPV6NetworkMaskPrefixLength))
#define IHostNetworkInterface_EnableDynamicIPConfig(p) ((p)->lpVtbl->EnableDynamicIPConfig(p))
#define IHostNetworkInterface_DHCPRediscover(p) ((p)->lpVtbl->DHCPRediscover(p))
#endif /* VBOX_WITH_GLUE */

interface IHostNetworkInterface
{
#ifndef VBOX_WITH_GLUE
    struct IHostNetworkInterface_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostNetworkInterfaceVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHostNetworkInterface declaration */


/* Start of struct IHostVideoInputDevice declaration */
#define IHOSTVIDEOINPUTDEVICE_IID_STR "a1ceae44-d65e-4156-9359-d390f93ee9a0"
#define IHOSTVIDEOINPUTDEVICE_IID { \
    0xa1ceae44, 0xd65e, 0x4156, \
    { 0x93, 0x59, 0xd3, 0x90, 0xf9, 0x3e, 0xe9, 0xa0 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHostVideoInputDevice);
#ifndef VBOX_WITH_GLUE
struct IHostVideoInputDevice_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IHostVideoInputDevice *pThis, PRUnichar * *name);

    nsresult (*GetPath)(IHostVideoInputDevice *pThis, PRUnichar * *path);

    nsresult (*GetAlias)(IHostVideoInputDevice *pThis, PRUnichar * *alias);

};
#else /* VBOX_WITH_GLUE */
struct IHostVideoInputDeviceVtbl
{
    nsresult (*QueryInterface)(IHostVideoInputDevice *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHostVideoInputDevice *pThis);
    nsrefcnt (*Release)(IHostVideoInputDevice *pThis);
    nsresult (*GetName)(IHostVideoInputDevice *pThis, PRUnichar * *name);

    nsresult (*GetPath)(IHostVideoInputDevice *pThis, PRUnichar * *path);

    nsresult (*GetAlias)(IHostVideoInputDevice *pThis, PRUnichar * *alias);

};
#define IHostVideoInputDevice_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHostVideoInputDevice_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHostVideoInputDevice_Release(p) ((p)->lpVtbl->Release(p))
#define IHostVideoInputDevice_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IHostVideoInputDevice_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IHostVideoInputDevice_get_Path(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IHostVideoInputDevice_GetPath(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IHostVideoInputDevice_get_Alias(p, aAlias) ((p)->lpVtbl->GetAlias(p, aAlias))
#define IHostVideoInputDevice_GetAlias(p, aAlias) ((p)->lpVtbl->GetAlias(p, aAlias))
#endif /* VBOX_WITH_GLUE */

interface IHostVideoInputDevice
{
#ifndef VBOX_WITH_GLUE
    struct IHostVideoInputDevice_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostVideoInputDeviceVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHostVideoInputDevice declaration */


/* Start of struct IHost declaration */
#define IHOST_IID_STR "93269330-48ca-4096-b4a2-1189df336267"
#define IHOST_IID { \
    0x93269330, 0x48ca, 0x4096, \
    { 0xb4, 0xa2, 0x11, 0x89, 0xdf, 0x33, 0x62, 0x67 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHost);
#ifndef VBOX_WITH_GLUE
struct IHost_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetDVDDrives)(IHost *pThis, PRUint32 *DVDDrivesSize, IMedium * **DVDDrives);

    nsresult (*GetFloppyDrives)(IHost *pThis, PRUint32 *floppyDrivesSize, IMedium * **floppyDrives);

    nsresult (*GetUSBDevices)(IHost *pThis, PRUint32 *USBDevicesSize, IHostUSBDevice * **USBDevices);

    nsresult (*GetUSBDeviceFilters)(IHost *pThis, PRUint32 *USBDeviceFiltersSize, IHostUSBDeviceFilter * **USBDeviceFilters);

    nsresult (*GetNetworkInterfaces)(IHost *pThis, PRUint32 *networkInterfacesSize, IHostNetworkInterface * **networkInterfaces);

    nsresult (*GetNameServers)(IHost *pThis, PRUint32 *nameServersSize, PRUnichar * **nameServers);

    nsresult (*GetDomainName)(IHost *pThis, PRUnichar * *domainName);

    nsresult (*GetSearchStrings)(IHost *pThis, PRUint32 *searchStringsSize, PRUnichar * **searchStrings);

    nsresult (*GetProcessorCount)(IHost *pThis, PRUint32 *processorCount);

    nsresult (*GetProcessorOnlineCount)(IHost *pThis, PRUint32 *processorOnlineCount);

    nsresult (*GetProcessorCoreCount)(IHost *pThis, PRUint32 *processorCoreCount);

    nsresult (*GetProcessorOnlineCoreCount)(IHost *pThis, PRUint32 *processorOnlineCoreCount);

    nsresult (*GetMemorySize)(IHost *pThis, PRUint32 *memorySize);

    nsresult (*GetMemoryAvailable)(IHost *pThis, PRUint32 *memoryAvailable);

    nsresult (*GetOperatingSystem)(IHost *pThis, PRUnichar * *operatingSystem);

    nsresult (*GetOSVersion)(IHost *pThis, PRUnichar * *OSVersion);

    nsresult (*GetUTCTime)(IHost *pThis, PRInt64 *UTCTime);

    nsresult (*GetAcceleration3DAvailable)(IHost *pThis, PRBool *acceleration3DAvailable);

    nsresult (*GetVideoInputDevices)(IHost *pThis, PRUint32 *videoInputDevicesSize, IHostVideoInputDevice * **videoInputDevices);

    nsresult (*GetProcessorSpeed)(
        IHost *pThis,
        PRUint32 cpuId,
        PRUint32 * speed
    );

    nsresult (*GetProcessorFeature)(
        IHost *pThis,
        PRUint32 feature,
        PRBool * supported
    );

    nsresult (*GetProcessorDescription)(
        IHost *pThis,
        PRUint32 cpuId,
        PRUnichar * * description
    );

    nsresult (*GetProcessorCPUIDLeaf)(
        IHost *pThis,
        PRUint32 cpuId,
        PRUint32 leaf,
        PRUint32 subLeaf,
        PRUint32 * valEax,
        PRUint32 * valEbx,
        PRUint32 * valEcx,
        PRUint32 * valEdx
    );

    nsresult (*CreateHostOnlyNetworkInterface)(
        IHost *pThis,
        IHostNetworkInterface * * hostInterface,
        IProgress * * progress
    );

    nsresult (*RemoveHostOnlyNetworkInterface)(
        IHost *pThis,
        PRUnichar * id,
        IProgress * * progress
    );

    nsresult (*CreateUSBDeviceFilter)(
        IHost *pThis,
        PRUnichar * name,
        IHostUSBDeviceFilter * * filter
    );

    nsresult (*InsertUSBDeviceFilter)(
        IHost *pThis,
        PRUint32 position,
        IHostUSBDeviceFilter * filter
    );

    nsresult (*RemoveUSBDeviceFilter)(
        IHost *pThis,
        PRUint32 position
    );

    nsresult (*FindHostDVDDrive)(
        IHost *pThis,
        PRUnichar * name,
        IMedium * * drive
    );

    nsresult (*FindHostFloppyDrive)(
        IHost *pThis,
        PRUnichar * name,
        IMedium * * drive
    );

    nsresult (*FindHostNetworkInterfaceByName)(
        IHost *pThis,
        PRUnichar * name,
        IHostNetworkInterface * * networkInterface
    );

    nsresult (*FindHostNetworkInterfaceById)(
        IHost *pThis,
        PRUnichar * id,
        IHostNetworkInterface * * networkInterface
    );

    nsresult (*FindHostNetworkInterfacesOfType)(
        IHost *pThis,
        PRUint32 type,
        PRUint32 *networkInterfacesSize,
        IHostNetworkInterface *** networkInterfaces
    );

    nsresult (*FindUSBDeviceById)(
        IHost *pThis,
        PRUnichar * id,
        IHostUSBDevice * * device
    );

    nsresult (*FindUSBDeviceByAddress)(
        IHost *pThis,
        PRUnichar * name,
        IHostUSBDevice * * device
    );

    nsresult (*GenerateMACAddress)(
        IHost *pThis,
        PRUnichar * * address
    );

};
#else /* VBOX_WITH_GLUE */
struct IHostVtbl
{
    nsresult (*QueryInterface)(IHost *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHost *pThis);
    nsrefcnt (*Release)(IHost *pThis);
    nsresult (*GetDVDDrives)(IHost *pThis, PRUint32 *DVDDrivesSize, IMedium * **DVDDrives);

    nsresult (*GetFloppyDrives)(IHost *pThis, PRUint32 *floppyDrivesSize, IMedium * **floppyDrives);

    nsresult (*GetUSBDevices)(IHost *pThis, PRUint32 *USBDevicesSize, IHostUSBDevice * **USBDevices);

    nsresult (*GetUSBDeviceFilters)(IHost *pThis, PRUint32 *USBDeviceFiltersSize, IHostUSBDeviceFilter * **USBDeviceFilters);

    nsresult (*GetNetworkInterfaces)(IHost *pThis, PRUint32 *networkInterfacesSize, IHostNetworkInterface * **networkInterfaces);

    nsresult (*GetNameServers)(IHost *pThis, PRUint32 *nameServersSize, PRUnichar * **nameServers);

    nsresult (*GetDomainName)(IHost *pThis, PRUnichar * *domainName);

    nsresult (*GetSearchStrings)(IHost *pThis, PRUint32 *searchStringsSize, PRUnichar * **searchStrings);

    nsresult (*GetProcessorCount)(IHost *pThis, PRUint32 *processorCount);

    nsresult (*GetProcessorOnlineCount)(IHost *pThis, PRUint32 *processorOnlineCount);

    nsresult (*GetProcessorCoreCount)(IHost *pThis, PRUint32 *processorCoreCount);

    nsresult (*GetProcessorOnlineCoreCount)(IHost *pThis, PRUint32 *processorOnlineCoreCount);

    nsresult (*GetMemorySize)(IHost *pThis, PRUint32 *memorySize);

    nsresult (*GetMemoryAvailable)(IHost *pThis, PRUint32 *memoryAvailable);

    nsresult (*GetOperatingSystem)(IHost *pThis, PRUnichar * *operatingSystem);

    nsresult (*GetOSVersion)(IHost *pThis, PRUnichar * *OSVersion);

    nsresult (*GetUTCTime)(IHost *pThis, PRInt64 *UTCTime);

    nsresult (*GetAcceleration3DAvailable)(IHost *pThis, PRBool *acceleration3DAvailable);

    nsresult (*GetVideoInputDevices)(IHost *pThis, PRUint32 *videoInputDevicesSize, IHostVideoInputDevice * **videoInputDevices);

    nsresult (*GetProcessorSpeed)(
        IHost *pThis,
        PRUint32 cpuId,
        PRUint32 * speed
    );

    nsresult (*GetProcessorFeature)(
        IHost *pThis,
        PRUint32 feature,
        PRBool * supported
    );

    nsresult (*GetProcessorDescription)(
        IHost *pThis,
        PRUint32 cpuId,
        PRUnichar * * description
    );

    nsresult (*GetProcessorCPUIDLeaf)(
        IHost *pThis,
        PRUint32 cpuId,
        PRUint32 leaf,
        PRUint32 subLeaf,
        PRUint32 * valEax,
        PRUint32 * valEbx,
        PRUint32 * valEcx,
        PRUint32 * valEdx
    );

    nsresult (*CreateHostOnlyNetworkInterface)(
        IHost *pThis,
        IHostNetworkInterface * * hostInterface,
        IProgress * * progress
    );

    nsresult (*RemoveHostOnlyNetworkInterface)(
        IHost *pThis,
        PRUnichar * id,
        IProgress * * progress
    );

    nsresult (*CreateUSBDeviceFilter)(
        IHost *pThis,
        PRUnichar * name,
        IHostUSBDeviceFilter * * filter
    );

    nsresult (*InsertUSBDeviceFilter)(
        IHost *pThis,
        PRUint32 position,
        IHostUSBDeviceFilter * filter
    );

    nsresult (*RemoveUSBDeviceFilter)(
        IHost *pThis,
        PRUint32 position
    );

    nsresult (*FindHostDVDDrive)(
        IHost *pThis,
        PRUnichar * name,
        IMedium * * drive
    );

    nsresult (*FindHostFloppyDrive)(
        IHost *pThis,
        PRUnichar * name,
        IMedium * * drive
    );

    nsresult (*FindHostNetworkInterfaceByName)(
        IHost *pThis,
        PRUnichar * name,
        IHostNetworkInterface * * networkInterface
    );

    nsresult (*FindHostNetworkInterfaceById)(
        IHost *pThis,
        PRUnichar * id,
        IHostNetworkInterface * * networkInterface
    );

    nsresult (*FindHostNetworkInterfacesOfType)(
        IHost *pThis,
        PRUint32 type,
        PRUint32 *networkInterfacesSize,
        IHostNetworkInterface *** networkInterfaces
    );

    nsresult (*FindUSBDeviceById)(
        IHost *pThis,
        PRUnichar * id,
        IHostUSBDevice * * device
    );

    nsresult (*FindUSBDeviceByAddress)(
        IHost *pThis,
        PRUnichar * name,
        IHostUSBDevice * * device
    );

    nsresult (*GenerateMACAddress)(
        IHost *pThis,
        PRUnichar * * address
    );

};
#define IHost_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHost_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHost_Release(p) ((p)->lpVtbl->Release(p))
#define IHost_get_DVDDrives(p, aDVDDrives) ((p)->lpVtbl->GetDVDDrives(p, aDVDDrives))
#define IHost_GetDVDDrives(p, aDVDDrives) ((p)->lpVtbl->GetDVDDrives(p, aDVDDrives))
#define IHost_get_FloppyDrives(p, aFloppyDrives) ((p)->lpVtbl->GetFloppyDrives(p, aFloppyDrives))
#define IHost_GetFloppyDrives(p, aFloppyDrives) ((p)->lpVtbl->GetFloppyDrives(p, aFloppyDrives))
#define IHost_get_USBDevices(p, aUSBDevices) ((p)->lpVtbl->GetUSBDevices(p, aUSBDevices))
#define IHost_GetUSBDevices(p, aUSBDevices) ((p)->lpVtbl->GetUSBDevices(p, aUSBDevices))
#define IHost_get_USBDeviceFilters(p, aUSBDeviceFilters) ((p)->lpVtbl->GetUSBDeviceFilters(p, aUSBDeviceFilters))
#define IHost_GetUSBDeviceFilters(p, aUSBDeviceFilters) ((p)->lpVtbl->GetUSBDeviceFilters(p, aUSBDeviceFilters))
#define IHost_get_NetworkInterfaces(p, aNetworkInterfaces) ((p)->lpVtbl->GetNetworkInterfaces(p, aNetworkInterfaces))
#define IHost_GetNetworkInterfaces(p, aNetworkInterfaces) ((p)->lpVtbl->GetNetworkInterfaces(p, aNetworkInterfaces))
#define IHost_get_NameServers(p, aNameServers) ((p)->lpVtbl->GetNameServers(p, aNameServers))
#define IHost_GetNameServers(p, aNameServers) ((p)->lpVtbl->GetNameServers(p, aNameServers))
#define IHost_get_DomainName(p, aDomainName) ((p)->lpVtbl->GetDomainName(p, aDomainName))
#define IHost_GetDomainName(p, aDomainName) ((p)->lpVtbl->GetDomainName(p, aDomainName))
#define IHost_get_SearchStrings(p, aSearchStrings) ((p)->lpVtbl->GetSearchStrings(p, aSearchStrings))
#define IHost_GetSearchStrings(p, aSearchStrings) ((p)->lpVtbl->GetSearchStrings(p, aSearchStrings))
#define IHost_get_ProcessorCount(p, aProcessorCount) ((p)->lpVtbl->GetProcessorCount(p, aProcessorCount))
#define IHost_GetProcessorCount(p, aProcessorCount) ((p)->lpVtbl->GetProcessorCount(p, aProcessorCount))
#define IHost_get_ProcessorOnlineCount(p, aProcessorOnlineCount) ((p)->lpVtbl->GetProcessorOnlineCount(p, aProcessorOnlineCount))
#define IHost_GetProcessorOnlineCount(p, aProcessorOnlineCount) ((p)->lpVtbl->GetProcessorOnlineCount(p, aProcessorOnlineCount))
#define IHost_get_ProcessorCoreCount(p, aProcessorCoreCount) ((p)->lpVtbl->GetProcessorCoreCount(p, aProcessorCoreCount))
#define IHost_GetProcessorCoreCount(p, aProcessorCoreCount) ((p)->lpVtbl->GetProcessorCoreCount(p, aProcessorCoreCount))
#define IHost_get_ProcessorOnlineCoreCount(p, aProcessorOnlineCoreCount) ((p)->lpVtbl->GetProcessorOnlineCoreCount(p, aProcessorOnlineCoreCount))
#define IHost_GetProcessorOnlineCoreCount(p, aProcessorOnlineCoreCount) ((p)->lpVtbl->GetProcessorOnlineCoreCount(p, aProcessorOnlineCoreCount))
#define IHost_get_MemorySize(p, aMemorySize) ((p)->lpVtbl->GetMemorySize(p, aMemorySize))
#define IHost_GetMemorySize(p, aMemorySize) ((p)->lpVtbl->GetMemorySize(p, aMemorySize))
#define IHost_get_MemoryAvailable(p, aMemoryAvailable) ((p)->lpVtbl->GetMemoryAvailable(p, aMemoryAvailable))
#define IHost_GetMemoryAvailable(p, aMemoryAvailable) ((p)->lpVtbl->GetMemoryAvailable(p, aMemoryAvailable))
#define IHost_get_OperatingSystem(p, aOperatingSystem) ((p)->lpVtbl->GetOperatingSystem(p, aOperatingSystem))
#define IHost_GetOperatingSystem(p, aOperatingSystem) ((p)->lpVtbl->GetOperatingSystem(p, aOperatingSystem))
#define IHost_get_OSVersion(p, aOSVersion) ((p)->lpVtbl->GetOSVersion(p, aOSVersion))
#define IHost_GetOSVersion(p, aOSVersion) ((p)->lpVtbl->GetOSVersion(p, aOSVersion))
#define IHost_get_UTCTime(p, aUTCTime) ((p)->lpVtbl->GetUTCTime(p, aUTCTime))
#define IHost_GetUTCTime(p, aUTCTime) ((p)->lpVtbl->GetUTCTime(p, aUTCTime))
#define IHost_get_Acceleration3DAvailable(p, aAcceleration3DAvailable) ((p)->lpVtbl->GetAcceleration3DAvailable(p, aAcceleration3DAvailable))
#define IHost_GetAcceleration3DAvailable(p, aAcceleration3DAvailable) ((p)->lpVtbl->GetAcceleration3DAvailable(p, aAcceleration3DAvailable))
#define IHost_get_VideoInputDevices(p, aVideoInputDevices) ((p)->lpVtbl->GetVideoInputDevices(p, aVideoInputDevices))
#define IHost_GetVideoInputDevices(p, aVideoInputDevices) ((p)->lpVtbl->GetVideoInputDevices(p, aVideoInputDevices))
#define IHost_GetProcessorSpeed(p, aCpuId, aSpeed) ((p)->lpVtbl->GetProcessorSpeed(p, aCpuId, aSpeed))
#define IHost_GetProcessorFeature(p, aFeature, aSupported) ((p)->lpVtbl->GetProcessorFeature(p, aFeature, aSupported))
#define IHost_GetProcessorDescription(p, aCpuId, aDescription) ((p)->lpVtbl->GetProcessorDescription(p, aCpuId, aDescription))
#define IHost_GetProcessorCPUIDLeaf(p, aCpuId, aLeaf, aSubLeaf, aValEax, aValEbx, aValEcx, aValEdx) ((p)->lpVtbl->GetProcessorCPUIDLeaf(p, aCpuId, aLeaf, aSubLeaf, aValEax, aValEbx, aValEcx, aValEdx))
#define IHost_CreateHostOnlyNetworkInterface(p, aHostInterface, aProgress) ((p)->lpVtbl->CreateHostOnlyNetworkInterface(p, aHostInterface, aProgress))
#define IHost_RemoveHostOnlyNetworkInterface(p, aId, aProgress) ((p)->lpVtbl->RemoveHostOnlyNetworkInterface(p, aId, aProgress))
#define IHost_CreateUSBDeviceFilter(p, aName, aFilter) ((p)->lpVtbl->CreateUSBDeviceFilter(p, aName, aFilter))
#define IHost_InsertUSBDeviceFilter(p, aPosition, aFilter) ((p)->lpVtbl->InsertUSBDeviceFilter(p, aPosition, aFilter))
#define IHost_RemoveUSBDeviceFilter(p, aPosition) ((p)->lpVtbl->RemoveUSBDeviceFilter(p, aPosition))
#define IHost_FindHostDVDDrive(p, aName, aDrive) ((p)->lpVtbl->FindHostDVDDrive(p, aName, aDrive))
#define IHost_FindHostFloppyDrive(p, aName, aDrive) ((p)->lpVtbl->FindHostFloppyDrive(p, aName, aDrive))
#define IHost_FindHostNetworkInterfaceByName(p, aName, aNetworkInterface) ((p)->lpVtbl->FindHostNetworkInterfaceByName(p, aName, aNetworkInterface))
#define IHost_FindHostNetworkInterfaceById(p, aId, aNetworkInterface) ((p)->lpVtbl->FindHostNetworkInterfaceById(p, aId, aNetworkInterface))
#define IHost_FindHostNetworkInterfacesOfType(p, aType, aNetworkInterfaces) ((p)->lpVtbl->FindHostNetworkInterfacesOfType(p, aType, aNetworkInterfaces))
#define IHost_FindUSBDeviceById(p, aId, aDevice) ((p)->lpVtbl->FindUSBDeviceById(p, aId, aDevice))
#define IHost_FindUSBDeviceByAddress(p, aName, aDevice) ((p)->lpVtbl->FindUSBDeviceByAddress(p, aName, aDevice))
#define IHost_GenerateMACAddress(p, aAddress) ((p)->lpVtbl->GenerateMACAddress(p, aAddress))
#endif /* VBOX_WITH_GLUE */

interface IHost
{
#ifndef VBOX_WITH_GLUE
    struct IHost_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHost declaration */


/* Start of struct ISystemProperties declaration */
#define ISYSTEMPROPERTIES_IID_STR "1254a96a-ae57-4484-946a-22d86c1f98af"
#define ISYSTEMPROPERTIES_IID { \
    0x1254a96a, 0xae57, 0x4484, \
    { 0x94, 0x6a, 0x22, 0xd8, 0x6c, 0x1f, 0x98, 0xaf } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISystemProperties);
#ifndef VBOX_WITH_GLUE
struct ISystemProperties_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetMinGuestRAM)(ISystemProperties *pThis, PRUint32 *minGuestRAM);

    nsresult (*GetMaxGuestRAM)(ISystemProperties *pThis, PRUint32 *maxGuestRAM);

    nsresult (*GetMinGuestVRAM)(ISystemProperties *pThis, PRUint32 *minGuestVRAM);

    nsresult (*GetMaxGuestVRAM)(ISystemProperties *pThis, PRUint32 *maxGuestVRAM);

    nsresult (*GetMinGuestCPUCount)(ISystemProperties *pThis, PRUint32 *minGuestCPUCount);

    nsresult (*GetMaxGuestCPUCount)(ISystemProperties *pThis, PRUint32 *maxGuestCPUCount);

    nsresult (*GetMaxGuestMonitors)(ISystemProperties *pThis, PRUint32 *maxGuestMonitors);

    nsresult (*GetInfoVDSize)(ISystemProperties *pThis, PRInt64 *infoVDSize);

    nsresult (*GetSerialPortCount)(ISystemProperties *pThis, PRUint32 *serialPortCount);

    nsresult (*GetParallelPortCount)(ISystemProperties *pThis, PRUint32 *parallelPortCount);

    nsresult (*GetMaxBootPosition)(ISystemProperties *pThis, PRUint32 *maxBootPosition);

    nsresult (*GetExclusiveHwVirt)(ISystemProperties *pThis, PRBool *exclusiveHwVirt);
    nsresult (*SetExclusiveHwVirt)(ISystemProperties *pThis, PRBool exclusiveHwVirt);

    nsresult (*GetDefaultMachineFolder)(ISystemProperties *pThis, PRUnichar * *defaultMachineFolder);
    nsresult (*SetDefaultMachineFolder)(ISystemProperties *pThis, PRUnichar * defaultMachineFolder);

    nsresult (*GetLoggingLevel)(ISystemProperties *pThis, PRUnichar * *loggingLevel);
    nsresult (*SetLoggingLevel)(ISystemProperties *pThis, PRUnichar * loggingLevel);

    nsresult (*GetMediumFormats)(ISystemProperties *pThis, PRUint32 *mediumFormatsSize, IMediumFormat * **mediumFormats);

    nsresult (*GetDefaultHardDiskFormat)(ISystemProperties *pThis, PRUnichar * *defaultHardDiskFormat);
    nsresult (*SetDefaultHardDiskFormat)(ISystemProperties *pThis, PRUnichar * defaultHardDiskFormat);

    nsresult (*GetFreeDiskSpaceWarning)(ISystemProperties *pThis, PRInt64 *freeDiskSpaceWarning);
    nsresult (*SetFreeDiskSpaceWarning)(ISystemProperties *pThis, PRInt64 freeDiskSpaceWarning);

    nsresult (*GetFreeDiskSpacePercentWarning)(ISystemProperties *pThis, PRUint32 *freeDiskSpacePercentWarning);
    nsresult (*SetFreeDiskSpacePercentWarning)(ISystemProperties *pThis, PRUint32 freeDiskSpacePercentWarning);

    nsresult (*GetFreeDiskSpaceError)(ISystemProperties *pThis, PRInt64 *freeDiskSpaceError);
    nsresult (*SetFreeDiskSpaceError)(ISystemProperties *pThis, PRInt64 freeDiskSpaceError);

    nsresult (*GetFreeDiskSpacePercentError)(ISystemProperties *pThis, PRUint32 *freeDiskSpacePercentError);
    nsresult (*SetFreeDiskSpacePercentError)(ISystemProperties *pThis, PRUint32 freeDiskSpacePercentError);

    nsresult (*GetVRDEAuthLibrary)(ISystemProperties *pThis, PRUnichar * *VRDEAuthLibrary);
    nsresult (*SetVRDEAuthLibrary)(ISystemProperties *pThis, PRUnichar * VRDEAuthLibrary);

    nsresult (*GetWebServiceAuthLibrary)(ISystemProperties *pThis, PRUnichar * *webServiceAuthLibrary);
    nsresult (*SetWebServiceAuthLibrary)(ISystemProperties *pThis, PRUnichar * webServiceAuthLibrary);

    nsresult (*GetDefaultVRDEExtPack)(ISystemProperties *pThis, PRUnichar * *defaultVRDEExtPack);
    nsresult (*SetDefaultVRDEExtPack)(ISystemProperties *pThis, PRUnichar * defaultVRDEExtPack);

    nsresult (*GetLogHistoryCount)(ISystemProperties *pThis, PRUint32 *logHistoryCount);
    nsresult (*SetLogHistoryCount)(ISystemProperties *pThis, PRUint32 logHistoryCount);

    nsresult (*GetDefaultAudioDriver)(ISystemProperties *pThis, PRUint32 *defaultAudioDriver);

    nsresult (*GetAutostartDatabasePath)(ISystemProperties *pThis, PRUnichar * *autostartDatabasePath);
    nsresult (*SetAutostartDatabasePath)(ISystemProperties *pThis, PRUnichar * autostartDatabasePath);

    nsresult (*GetDefaultAdditionsISO)(ISystemProperties *pThis, PRUnichar * *defaultAdditionsISO);
    nsresult (*SetDefaultAdditionsISO)(ISystemProperties *pThis, PRUnichar * defaultAdditionsISO);

    nsresult (*GetDefaultFrontend)(ISystemProperties *pThis, PRUnichar * *defaultFrontend);
    nsresult (*SetDefaultFrontend)(ISystemProperties *pThis, PRUnichar * defaultFrontend);

    nsresult (*GetMaxNetworkAdapters)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 * maxNetworkAdapters
    );

    nsresult (*GetMaxNetworkAdaptersOfType)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 type,
        PRUint32 * maxNetworkAdapters
    );

    nsresult (*GetMaxDevicesPerPortForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 * maxDevicesPerPort
    );

    nsresult (*GetMinPortCountForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 * minPortCount
    );

    nsresult (*GetMaxPortCountForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 * maxPortCount
    );

    nsresult (*GetMaxInstancesOfStorageBus)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 bus,
        PRUint32 * maxInstances
    );

    nsresult (*GetDeviceTypesForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 *deviceTypesSize,
        PRUint32** deviceTypes
    );

    nsresult (*GetDefaultIoCacheSettingForStorageController)(
        ISystemProperties *pThis,
        PRUint32 controllerType,
        PRBool * enabled
    );

    nsresult (*GetMaxInstancesOfUSBControllerType)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 type,
        PRUint32 * maxInstances
    );

};
#else /* VBOX_WITH_GLUE */
struct ISystemPropertiesVtbl
{
    nsresult (*QueryInterface)(ISystemProperties *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISystemProperties *pThis);
    nsrefcnt (*Release)(ISystemProperties *pThis);
    nsresult (*GetMinGuestRAM)(ISystemProperties *pThis, PRUint32 *minGuestRAM);

    nsresult (*GetMaxGuestRAM)(ISystemProperties *pThis, PRUint32 *maxGuestRAM);

    nsresult (*GetMinGuestVRAM)(ISystemProperties *pThis, PRUint32 *minGuestVRAM);

    nsresult (*GetMaxGuestVRAM)(ISystemProperties *pThis, PRUint32 *maxGuestVRAM);

    nsresult (*GetMinGuestCPUCount)(ISystemProperties *pThis, PRUint32 *minGuestCPUCount);

    nsresult (*GetMaxGuestCPUCount)(ISystemProperties *pThis, PRUint32 *maxGuestCPUCount);

    nsresult (*GetMaxGuestMonitors)(ISystemProperties *pThis, PRUint32 *maxGuestMonitors);

    nsresult (*GetInfoVDSize)(ISystemProperties *pThis, PRInt64 *infoVDSize);

    nsresult (*GetSerialPortCount)(ISystemProperties *pThis, PRUint32 *serialPortCount);

    nsresult (*GetParallelPortCount)(ISystemProperties *pThis, PRUint32 *parallelPortCount);

    nsresult (*GetMaxBootPosition)(ISystemProperties *pThis, PRUint32 *maxBootPosition);

    nsresult (*GetExclusiveHwVirt)(ISystemProperties *pThis, PRBool *exclusiveHwVirt);
    nsresult (*SetExclusiveHwVirt)(ISystemProperties *pThis, PRBool exclusiveHwVirt);

    nsresult (*GetDefaultMachineFolder)(ISystemProperties *pThis, PRUnichar * *defaultMachineFolder);
    nsresult (*SetDefaultMachineFolder)(ISystemProperties *pThis, PRUnichar * defaultMachineFolder);

    nsresult (*GetLoggingLevel)(ISystemProperties *pThis, PRUnichar * *loggingLevel);
    nsresult (*SetLoggingLevel)(ISystemProperties *pThis, PRUnichar * loggingLevel);

    nsresult (*GetMediumFormats)(ISystemProperties *pThis, PRUint32 *mediumFormatsSize, IMediumFormat * **mediumFormats);

    nsresult (*GetDefaultHardDiskFormat)(ISystemProperties *pThis, PRUnichar * *defaultHardDiskFormat);
    nsresult (*SetDefaultHardDiskFormat)(ISystemProperties *pThis, PRUnichar * defaultHardDiskFormat);

    nsresult (*GetFreeDiskSpaceWarning)(ISystemProperties *pThis, PRInt64 *freeDiskSpaceWarning);
    nsresult (*SetFreeDiskSpaceWarning)(ISystemProperties *pThis, PRInt64 freeDiskSpaceWarning);

    nsresult (*GetFreeDiskSpacePercentWarning)(ISystemProperties *pThis, PRUint32 *freeDiskSpacePercentWarning);
    nsresult (*SetFreeDiskSpacePercentWarning)(ISystemProperties *pThis, PRUint32 freeDiskSpacePercentWarning);

    nsresult (*GetFreeDiskSpaceError)(ISystemProperties *pThis, PRInt64 *freeDiskSpaceError);
    nsresult (*SetFreeDiskSpaceError)(ISystemProperties *pThis, PRInt64 freeDiskSpaceError);

    nsresult (*GetFreeDiskSpacePercentError)(ISystemProperties *pThis, PRUint32 *freeDiskSpacePercentError);
    nsresult (*SetFreeDiskSpacePercentError)(ISystemProperties *pThis, PRUint32 freeDiskSpacePercentError);

    nsresult (*GetVRDEAuthLibrary)(ISystemProperties *pThis, PRUnichar * *VRDEAuthLibrary);
    nsresult (*SetVRDEAuthLibrary)(ISystemProperties *pThis, PRUnichar * VRDEAuthLibrary);

    nsresult (*GetWebServiceAuthLibrary)(ISystemProperties *pThis, PRUnichar * *webServiceAuthLibrary);
    nsresult (*SetWebServiceAuthLibrary)(ISystemProperties *pThis, PRUnichar * webServiceAuthLibrary);

    nsresult (*GetDefaultVRDEExtPack)(ISystemProperties *pThis, PRUnichar * *defaultVRDEExtPack);
    nsresult (*SetDefaultVRDEExtPack)(ISystemProperties *pThis, PRUnichar * defaultVRDEExtPack);

    nsresult (*GetLogHistoryCount)(ISystemProperties *pThis, PRUint32 *logHistoryCount);
    nsresult (*SetLogHistoryCount)(ISystemProperties *pThis, PRUint32 logHistoryCount);

    nsresult (*GetDefaultAudioDriver)(ISystemProperties *pThis, PRUint32 *defaultAudioDriver);

    nsresult (*GetAutostartDatabasePath)(ISystemProperties *pThis, PRUnichar * *autostartDatabasePath);
    nsresult (*SetAutostartDatabasePath)(ISystemProperties *pThis, PRUnichar * autostartDatabasePath);

    nsresult (*GetDefaultAdditionsISO)(ISystemProperties *pThis, PRUnichar * *defaultAdditionsISO);
    nsresult (*SetDefaultAdditionsISO)(ISystemProperties *pThis, PRUnichar * defaultAdditionsISO);

    nsresult (*GetDefaultFrontend)(ISystemProperties *pThis, PRUnichar * *defaultFrontend);
    nsresult (*SetDefaultFrontend)(ISystemProperties *pThis, PRUnichar * defaultFrontend);

    nsresult (*GetMaxNetworkAdapters)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 * maxNetworkAdapters
    );

    nsresult (*GetMaxNetworkAdaptersOfType)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 type,
        PRUint32 * maxNetworkAdapters
    );

    nsresult (*GetMaxDevicesPerPortForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 * maxDevicesPerPort
    );

    nsresult (*GetMinPortCountForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 * minPortCount
    );

    nsresult (*GetMaxPortCountForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 * maxPortCount
    );

    nsresult (*GetMaxInstancesOfStorageBus)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 bus,
        PRUint32 * maxInstances
    );

    nsresult (*GetDeviceTypesForStorageBus)(
        ISystemProperties *pThis,
        PRUint32 bus,
        PRUint32 *deviceTypesSize,
        PRUint32** deviceTypes
    );

    nsresult (*GetDefaultIoCacheSettingForStorageController)(
        ISystemProperties *pThis,
        PRUint32 controllerType,
        PRBool * enabled
    );

    nsresult (*GetMaxInstancesOfUSBControllerType)(
        ISystemProperties *pThis,
        PRUint32 chipset,
        PRUint32 type,
        PRUint32 * maxInstances
    );

};
#define ISystemProperties_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISystemProperties_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISystemProperties_Release(p) ((p)->lpVtbl->Release(p))
#define ISystemProperties_get_MinGuestRAM(p, aMinGuestRAM) ((p)->lpVtbl->GetMinGuestRAM(p, aMinGuestRAM))
#define ISystemProperties_GetMinGuestRAM(p, aMinGuestRAM) ((p)->lpVtbl->GetMinGuestRAM(p, aMinGuestRAM))
#define ISystemProperties_get_MaxGuestRAM(p, aMaxGuestRAM) ((p)->lpVtbl->GetMaxGuestRAM(p, aMaxGuestRAM))
#define ISystemProperties_GetMaxGuestRAM(p, aMaxGuestRAM) ((p)->lpVtbl->GetMaxGuestRAM(p, aMaxGuestRAM))
#define ISystemProperties_get_MinGuestVRAM(p, aMinGuestVRAM) ((p)->lpVtbl->GetMinGuestVRAM(p, aMinGuestVRAM))
#define ISystemProperties_GetMinGuestVRAM(p, aMinGuestVRAM) ((p)->lpVtbl->GetMinGuestVRAM(p, aMinGuestVRAM))
#define ISystemProperties_get_MaxGuestVRAM(p, aMaxGuestVRAM) ((p)->lpVtbl->GetMaxGuestVRAM(p, aMaxGuestVRAM))
#define ISystemProperties_GetMaxGuestVRAM(p, aMaxGuestVRAM) ((p)->lpVtbl->GetMaxGuestVRAM(p, aMaxGuestVRAM))
#define ISystemProperties_get_MinGuestCPUCount(p, aMinGuestCPUCount) ((p)->lpVtbl->GetMinGuestCPUCount(p, aMinGuestCPUCount))
#define ISystemProperties_GetMinGuestCPUCount(p, aMinGuestCPUCount) ((p)->lpVtbl->GetMinGuestCPUCount(p, aMinGuestCPUCount))
#define ISystemProperties_get_MaxGuestCPUCount(p, aMaxGuestCPUCount) ((p)->lpVtbl->GetMaxGuestCPUCount(p, aMaxGuestCPUCount))
#define ISystemProperties_GetMaxGuestCPUCount(p, aMaxGuestCPUCount) ((p)->lpVtbl->GetMaxGuestCPUCount(p, aMaxGuestCPUCount))
#define ISystemProperties_get_MaxGuestMonitors(p, aMaxGuestMonitors) ((p)->lpVtbl->GetMaxGuestMonitors(p, aMaxGuestMonitors))
#define ISystemProperties_GetMaxGuestMonitors(p, aMaxGuestMonitors) ((p)->lpVtbl->GetMaxGuestMonitors(p, aMaxGuestMonitors))
#define ISystemProperties_get_InfoVDSize(p, aInfoVDSize) ((p)->lpVtbl->GetInfoVDSize(p, aInfoVDSize))
#define ISystemProperties_GetInfoVDSize(p, aInfoVDSize) ((p)->lpVtbl->GetInfoVDSize(p, aInfoVDSize))
#define ISystemProperties_get_SerialPortCount(p, aSerialPortCount) ((p)->lpVtbl->GetSerialPortCount(p, aSerialPortCount))
#define ISystemProperties_GetSerialPortCount(p, aSerialPortCount) ((p)->lpVtbl->GetSerialPortCount(p, aSerialPortCount))
#define ISystemProperties_get_ParallelPortCount(p, aParallelPortCount) ((p)->lpVtbl->GetParallelPortCount(p, aParallelPortCount))
#define ISystemProperties_GetParallelPortCount(p, aParallelPortCount) ((p)->lpVtbl->GetParallelPortCount(p, aParallelPortCount))
#define ISystemProperties_get_MaxBootPosition(p, aMaxBootPosition) ((p)->lpVtbl->GetMaxBootPosition(p, aMaxBootPosition))
#define ISystemProperties_GetMaxBootPosition(p, aMaxBootPosition) ((p)->lpVtbl->GetMaxBootPosition(p, aMaxBootPosition))
#define ISystemProperties_get_ExclusiveHwVirt(p, aExclusiveHwVirt) ((p)->lpVtbl->GetExclusiveHwVirt(p, aExclusiveHwVirt))
#define ISystemProperties_GetExclusiveHwVirt(p, aExclusiveHwVirt) ((p)->lpVtbl->GetExclusiveHwVirt(p, aExclusiveHwVirt))
#define ISystemProperties_put_ExclusiveHwVirt(p, aExclusiveHwVirt) ((p)->lpVtbl->SetExclusiveHwVirt(p, aExclusiveHwVirt))
#define ISystemProperties_SetExclusiveHwVirt(p, aExclusiveHwVirt) ((p)->lpVtbl->SetExclusiveHwVirt(p, aExclusiveHwVirt))
#define ISystemProperties_get_DefaultMachineFolder(p, aDefaultMachineFolder) ((p)->lpVtbl->GetDefaultMachineFolder(p, aDefaultMachineFolder))
#define ISystemProperties_GetDefaultMachineFolder(p, aDefaultMachineFolder) ((p)->lpVtbl->GetDefaultMachineFolder(p, aDefaultMachineFolder))
#define ISystemProperties_put_DefaultMachineFolder(p, aDefaultMachineFolder) ((p)->lpVtbl->SetDefaultMachineFolder(p, aDefaultMachineFolder))
#define ISystemProperties_SetDefaultMachineFolder(p, aDefaultMachineFolder) ((p)->lpVtbl->SetDefaultMachineFolder(p, aDefaultMachineFolder))
#define ISystemProperties_get_LoggingLevel(p, aLoggingLevel) ((p)->lpVtbl->GetLoggingLevel(p, aLoggingLevel))
#define ISystemProperties_GetLoggingLevel(p, aLoggingLevel) ((p)->lpVtbl->GetLoggingLevel(p, aLoggingLevel))
#define ISystemProperties_put_LoggingLevel(p, aLoggingLevel) ((p)->lpVtbl->SetLoggingLevel(p, aLoggingLevel))
#define ISystemProperties_SetLoggingLevel(p, aLoggingLevel) ((p)->lpVtbl->SetLoggingLevel(p, aLoggingLevel))
#define ISystemProperties_get_MediumFormats(p, aMediumFormats) ((p)->lpVtbl->GetMediumFormats(p, aMediumFormats))
#define ISystemProperties_GetMediumFormats(p, aMediumFormats) ((p)->lpVtbl->GetMediumFormats(p, aMediumFormats))
#define ISystemProperties_get_DefaultHardDiskFormat(p, aDefaultHardDiskFormat) ((p)->lpVtbl->GetDefaultHardDiskFormat(p, aDefaultHardDiskFormat))
#define ISystemProperties_GetDefaultHardDiskFormat(p, aDefaultHardDiskFormat) ((p)->lpVtbl->GetDefaultHardDiskFormat(p, aDefaultHardDiskFormat))
#define ISystemProperties_put_DefaultHardDiskFormat(p, aDefaultHardDiskFormat) ((p)->lpVtbl->SetDefaultHardDiskFormat(p, aDefaultHardDiskFormat))
#define ISystemProperties_SetDefaultHardDiskFormat(p, aDefaultHardDiskFormat) ((p)->lpVtbl->SetDefaultHardDiskFormat(p, aDefaultHardDiskFormat))
#define ISystemProperties_get_FreeDiskSpaceWarning(p, aFreeDiskSpaceWarning) ((p)->lpVtbl->GetFreeDiskSpaceWarning(p, aFreeDiskSpaceWarning))
#define ISystemProperties_GetFreeDiskSpaceWarning(p, aFreeDiskSpaceWarning) ((p)->lpVtbl->GetFreeDiskSpaceWarning(p, aFreeDiskSpaceWarning))
#define ISystemProperties_put_FreeDiskSpaceWarning(p, aFreeDiskSpaceWarning) ((p)->lpVtbl->SetFreeDiskSpaceWarning(p, aFreeDiskSpaceWarning))
#define ISystemProperties_SetFreeDiskSpaceWarning(p, aFreeDiskSpaceWarning) ((p)->lpVtbl->SetFreeDiskSpaceWarning(p, aFreeDiskSpaceWarning))
#define ISystemProperties_get_FreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning) ((p)->lpVtbl->GetFreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning))
#define ISystemProperties_GetFreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning) ((p)->lpVtbl->GetFreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning))
#define ISystemProperties_put_FreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning) ((p)->lpVtbl->SetFreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning))
#define ISystemProperties_SetFreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning) ((p)->lpVtbl->SetFreeDiskSpacePercentWarning(p, aFreeDiskSpacePercentWarning))
#define ISystemProperties_get_FreeDiskSpaceError(p, aFreeDiskSpaceError) ((p)->lpVtbl->GetFreeDiskSpaceError(p, aFreeDiskSpaceError))
#define ISystemProperties_GetFreeDiskSpaceError(p, aFreeDiskSpaceError) ((p)->lpVtbl->GetFreeDiskSpaceError(p, aFreeDiskSpaceError))
#define ISystemProperties_put_FreeDiskSpaceError(p, aFreeDiskSpaceError) ((p)->lpVtbl->SetFreeDiskSpaceError(p, aFreeDiskSpaceError))
#define ISystemProperties_SetFreeDiskSpaceError(p, aFreeDiskSpaceError) ((p)->lpVtbl->SetFreeDiskSpaceError(p, aFreeDiskSpaceError))
#define ISystemProperties_get_FreeDiskSpacePercentError(p, aFreeDiskSpacePercentError) ((p)->lpVtbl->GetFreeDiskSpacePercentError(p, aFreeDiskSpacePercentError))
#define ISystemProperties_GetFreeDiskSpacePercentError(p, aFreeDiskSpacePercentError) ((p)->lpVtbl->GetFreeDiskSpacePercentError(p, aFreeDiskSpacePercentError))
#define ISystemProperties_put_FreeDiskSpacePercentError(p, aFreeDiskSpacePercentError) ((p)->lpVtbl->SetFreeDiskSpacePercentError(p, aFreeDiskSpacePercentError))
#define ISystemProperties_SetFreeDiskSpacePercentError(p, aFreeDiskSpacePercentError) ((p)->lpVtbl->SetFreeDiskSpacePercentError(p, aFreeDiskSpacePercentError))
#define ISystemProperties_get_VRDEAuthLibrary(p, aVRDEAuthLibrary) ((p)->lpVtbl->GetVRDEAuthLibrary(p, aVRDEAuthLibrary))
#define ISystemProperties_GetVRDEAuthLibrary(p, aVRDEAuthLibrary) ((p)->lpVtbl->GetVRDEAuthLibrary(p, aVRDEAuthLibrary))
#define ISystemProperties_put_VRDEAuthLibrary(p, aVRDEAuthLibrary) ((p)->lpVtbl->SetVRDEAuthLibrary(p, aVRDEAuthLibrary))
#define ISystemProperties_SetVRDEAuthLibrary(p, aVRDEAuthLibrary) ((p)->lpVtbl->SetVRDEAuthLibrary(p, aVRDEAuthLibrary))
#define ISystemProperties_get_WebServiceAuthLibrary(p, aWebServiceAuthLibrary) ((p)->lpVtbl->GetWebServiceAuthLibrary(p, aWebServiceAuthLibrary))
#define ISystemProperties_GetWebServiceAuthLibrary(p, aWebServiceAuthLibrary) ((p)->lpVtbl->GetWebServiceAuthLibrary(p, aWebServiceAuthLibrary))
#define ISystemProperties_put_WebServiceAuthLibrary(p, aWebServiceAuthLibrary) ((p)->lpVtbl->SetWebServiceAuthLibrary(p, aWebServiceAuthLibrary))
#define ISystemProperties_SetWebServiceAuthLibrary(p, aWebServiceAuthLibrary) ((p)->lpVtbl->SetWebServiceAuthLibrary(p, aWebServiceAuthLibrary))
#define ISystemProperties_get_DefaultVRDEExtPack(p, aDefaultVRDEExtPack) ((p)->lpVtbl->GetDefaultVRDEExtPack(p, aDefaultVRDEExtPack))
#define ISystemProperties_GetDefaultVRDEExtPack(p, aDefaultVRDEExtPack) ((p)->lpVtbl->GetDefaultVRDEExtPack(p, aDefaultVRDEExtPack))
#define ISystemProperties_put_DefaultVRDEExtPack(p, aDefaultVRDEExtPack) ((p)->lpVtbl->SetDefaultVRDEExtPack(p, aDefaultVRDEExtPack))
#define ISystemProperties_SetDefaultVRDEExtPack(p, aDefaultVRDEExtPack) ((p)->lpVtbl->SetDefaultVRDEExtPack(p, aDefaultVRDEExtPack))
#define ISystemProperties_get_LogHistoryCount(p, aLogHistoryCount) ((p)->lpVtbl->GetLogHistoryCount(p, aLogHistoryCount))
#define ISystemProperties_GetLogHistoryCount(p, aLogHistoryCount) ((p)->lpVtbl->GetLogHistoryCount(p, aLogHistoryCount))
#define ISystemProperties_put_LogHistoryCount(p, aLogHistoryCount) ((p)->lpVtbl->SetLogHistoryCount(p, aLogHistoryCount))
#define ISystemProperties_SetLogHistoryCount(p, aLogHistoryCount) ((p)->lpVtbl->SetLogHistoryCount(p, aLogHistoryCount))
#define ISystemProperties_get_DefaultAudioDriver(p, aDefaultAudioDriver) ((p)->lpVtbl->GetDefaultAudioDriver(p, aDefaultAudioDriver))
#define ISystemProperties_GetDefaultAudioDriver(p, aDefaultAudioDriver) ((p)->lpVtbl->GetDefaultAudioDriver(p, aDefaultAudioDriver))
#define ISystemProperties_get_AutostartDatabasePath(p, aAutostartDatabasePath) ((p)->lpVtbl->GetAutostartDatabasePath(p, aAutostartDatabasePath))
#define ISystemProperties_GetAutostartDatabasePath(p, aAutostartDatabasePath) ((p)->lpVtbl->GetAutostartDatabasePath(p, aAutostartDatabasePath))
#define ISystemProperties_put_AutostartDatabasePath(p, aAutostartDatabasePath) ((p)->lpVtbl->SetAutostartDatabasePath(p, aAutostartDatabasePath))
#define ISystemProperties_SetAutostartDatabasePath(p, aAutostartDatabasePath) ((p)->lpVtbl->SetAutostartDatabasePath(p, aAutostartDatabasePath))
#define ISystemProperties_get_DefaultAdditionsISO(p, aDefaultAdditionsISO) ((p)->lpVtbl->GetDefaultAdditionsISO(p, aDefaultAdditionsISO))
#define ISystemProperties_GetDefaultAdditionsISO(p, aDefaultAdditionsISO) ((p)->lpVtbl->GetDefaultAdditionsISO(p, aDefaultAdditionsISO))
#define ISystemProperties_put_DefaultAdditionsISO(p, aDefaultAdditionsISO) ((p)->lpVtbl->SetDefaultAdditionsISO(p, aDefaultAdditionsISO))
#define ISystemProperties_SetDefaultAdditionsISO(p, aDefaultAdditionsISO) ((p)->lpVtbl->SetDefaultAdditionsISO(p, aDefaultAdditionsISO))
#define ISystemProperties_get_DefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->GetDefaultFrontend(p, aDefaultFrontend))
#define ISystemProperties_GetDefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->GetDefaultFrontend(p, aDefaultFrontend))
#define ISystemProperties_put_DefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->SetDefaultFrontend(p, aDefaultFrontend))
#define ISystemProperties_SetDefaultFrontend(p, aDefaultFrontend) ((p)->lpVtbl->SetDefaultFrontend(p, aDefaultFrontend))
#define ISystemProperties_GetMaxNetworkAdapters(p, aChipset, aMaxNetworkAdapters) ((p)->lpVtbl->GetMaxNetworkAdapters(p, aChipset, aMaxNetworkAdapters))
#define ISystemProperties_GetMaxNetworkAdaptersOfType(p, aChipset, aType, aMaxNetworkAdapters) ((p)->lpVtbl->GetMaxNetworkAdaptersOfType(p, aChipset, aType, aMaxNetworkAdapters))
#define ISystemProperties_GetMaxDevicesPerPortForStorageBus(p, aBus, aMaxDevicesPerPort) ((p)->lpVtbl->GetMaxDevicesPerPortForStorageBus(p, aBus, aMaxDevicesPerPort))
#define ISystemProperties_GetMinPortCountForStorageBus(p, aBus, aMinPortCount) ((p)->lpVtbl->GetMinPortCountForStorageBus(p, aBus, aMinPortCount))
#define ISystemProperties_GetMaxPortCountForStorageBus(p, aBus, aMaxPortCount) ((p)->lpVtbl->GetMaxPortCountForStorageBus(p, aBus, aMaxPortCount))
#define ISystemProperties_GetMaxInstancesOfStorageBus(p, aChipset, aBus, aMaxInstances) ((p)->lpVtbl->GetMaxInstancesOfStorageBus(p, aChipset, aBus, aMaxInstances))
#define ISystemProperties_GetDeviceTypesForStorageBus(p, aBus, aDeviceTypes) ((p)->lpVtbl->GetDeviceTypesForStorageBus(p, aBus, aDeviceTypes))
#define ISystemProperties_GetDefaultIoCacheSettingForStorageController(p, aControllerType, aEnabled) ((p)->lpVtbl->GetDefaultIoCacheSettingForStorageController(p, aControllerType, aEnabled))
#define ISystemProperties_GetMaxInstancesOfUSBControllerType(p, aChipset, aType, aMaxInstances) ((p)->lpVtbl->GetMaxInstancesOfUSBControllerType(p, aChipset, aType, aMaxInstances))
#endif /* VBOX_WITH_GLUE */

interface ISystemProperties
{
#ifndef VBOX_WITH_GLUE
    struct ISystemProperties_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISystemPropertiesVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISystemProperties declaration */


/* Start of struct IGuestOSType declaration */
#define IGUESTOSTYPE_IID_STR "6d968f9a-858b-4c50-bf17-241f069e94c2"
#define IGUESTOSTYPE_IID { \
    0x6d968f9a, 0x858b, 0x4c50, \
    { 0xbf, 0x17, 0x24, 0x1f, 0x06, 0x9e, 0x94, 0xc2 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestOSType);
#ifndef VBOX_WITH_GLUE
struct IGuestOSType_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetFamilyId)(IGuestOSType *pThis, PRUnichar * *familyId);

    nsresult (*GetFamilyDescription)(IGuestOSType *pThis, PRUnichar * *familyDescription);

    nsresult (*GetId)(IGuestOSType *pThis, PRUnichar * *id);

    nsresult (*GetDescription)(IGuestOSType *pThis, PRUnichar * *description);

    nsresult (*GetIs64Bit)(IGuestOSType *pThis, PRBool *is64Bit);

    nsresult (*GetRecommendedIOAPIC)(IGuestOSType *pThis, PRBool *recommendedIOAPIC);

    nsresult (*GetRecommendedVirtEx)(IGuestOSType *pThis, PRBool *recommendedVirtEx);

    nsresult (*GetRecommendedRAM)(IGuestOSType *pThis, PRUint32 *recommendedRAM);

    nsresult (*GetRecommendedVRAM)(IGuestOSType *pThis, PRUint32 *recommendedVRAM);

    nsresult (*GetRecommended2DVideoAcceleration)(IGuestOSType *pThis, PRBool *recommended2DVideoAcceleration);

    nsresult (*GetRecommended3DAcceleration)(IGuestOSType *pThis, PRBool *recommended3DAcceleration);

    nsresult (*GetRecommendedHDD)(IGuestOSType *pThis, PRInt64 *recommendedHDD);

    nsresult (*GetAdapterType)(IGuestOSType *pThis, PRUint32 *adapterType);

    nsresult (*GetRecommendedPAE)(IGuestOSType *pThis, PRBool *recommendedPAE);

    nsresult (*GetRecommendedDVDStorageController)(IGuestOSType *pThis, PRUint32 *recommendedDVDStorageController);

    nsresult (*GetRecommendedDVDStorageBus)(IGuestOSType *pThis, PRUint32 *recommendedDVDStorageBus);

    nsresult (*GetRecommendedHDStorageController)(IGuestOSType *pThis, PRUint32 *recommendedHDStorageController);

    nsresult (*GetRecommendedHDStorageBus)(IGuestOSType *pThis, PRUint32 *recommendedHDStorageBus);

    nsresult (*GetRecommendedFirmware)(IGuestOSType *pThis, PRUint32 *recommendedFirmware);

    nsresult (*GetRecommendedUSBHID)(IGuestOSType *pThis, PRBool *recommendedUSBHID);

    nsresult (*GetRecommendedHPET)(IGuestOSType *pThis, PRBool *recommendedHPET);

    nsresult (*GetRecommendedUSBTablet)(IGuestOSType *pThis, PRBool *recommendedUSBTablet);

    nsresult (*GetRecommendedRTCUseUTC)(IGuestOSType *pThis, PRBool *recommendedRTCUseUTC);

    nsresult (*GetRecommendedChipset)(IGuestOSType *pThis, PRUint32 *recommendedChipset);

    nsresult (*GetRecommendedAudioController)(IGuestOSType *pThis, PRUint32 *recommendedAudioController);

    nsresult (*GetRecommendedFloppy)(IGuestOSType *pThis, PRBool *recommendedFloppy);

    nsresult (*GetRecommendedUSB)(IGuestOSType *pThis, PRBool *recommendedUSB);

};
#else /* VBOX_WITH_GLUE */
struct IGuestOSTypeVtbl
{
    nsresult (*QueryInterface)(IGuestOSType *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestOSType *pThis);
    nsrefcnt (*Release)(IGuestOSType *pThis);
    nsresult (*GetFamilyId)(IGuestOSType *pThis, PRUnichar * *familyId);

    nsresult (*GetFamilyDescription)(IGuestOSType *pThis, PRUnichar * *familyDescription);

    nsresult (*GetId)(IGuestOSType *pThis, PRUnichar * *id);

    nsresult (*GetDescription)(IGuestOSType *pThis, PRUnichar * *description);

    nsresult (*GetIs64Bit)(IGuestOSType *pThis, PRBool *is64Bit);

    nsresult (*GetRecommendedIOAPIC)(IGuestOSType *pThis, PRBool *recommendedIOAPIC);

    nsresult (*GetRecommendedVirtEx)(IGuestOSType *pThis, PRBool *recommendedVirtEx);

    nsresult (*GetRecommendedRAM)(IGuestOSType *pThis, PRUint32 *recommendedRAM);

    nsresult (*GetRecommendedVRAM)(IGuestOSType *pThis, PRUint32 *recommendedVRAM);

    nsresult (*GetRecommended2DVideoAcceleration)(IGuestOSType *pThis, PRBool *recommended2DVideoAcceleration);

    nsresult (*GetRecommended3DAcceleration)(IGuestOSType *pThis, PRBool *recommended3DAcceleration);

    nsresult (*GetRecommendedHDD)(IGuestOSType *pThis, PRInt64 *recommendedHDD);

    nsresult (*GetAdapterType)(IGuestOSType *pThis, PRUint32 *adapterType);

    nsresult (*GetRecommendedPAE)(IGuestOSType *pThis, PRBool *recommendedPAE);

    nsresult (*GetRecommendedDVDStorageController)(IGuestOSType *pThis, PRUint32 *recommendedDVDStorageController);

    nsresult (*GetRecommendedDVDStorageBus)(IGuestOSType *pThis, PRUint32 *recommendedDVDStorageBus);

    nsresult (*GetRecommendedHDStorageController)(IGuestOSType *pThis, PRUint32 *recommendedHDStorageController);

    nsresult (*GetRecommendedHDStorageBus)(IGuestOSType *pThis, PRUint32 *recommendedHDStorageBus);

    nsresult (*GetRecommendedFirmware)(IGuestOSType *pThis, PRUint32 *recommendedFirmware);

    nsresult (*GetRecommendedUSBHID)(IGuestOSType *pThis, PRBool *recommendedUSBHID);

    nsresult (*GetRecommendedHPET)(IGuestOSType *pThis, PRBool *recommendedHPET);

    nsresult (*GetRecommendedUSBTablet)(IGuestOSType *pThis, PRBool *recommendedUSBTablet);

    nsresult (*GetRecommendedRTCUseUTC)(IGuestOSType *pThis, PRBool *recommendedRTCUseUTC);

    nsresult (*GetRecommendedChipset)(IGuestOSType *pThis, PRUint32 *recommendedChipset);

    nsresult (*GetRecommendedAudioController)(IGuestOSType *pThis, PRUint32 *recommendedAudioController);

    nsresult (*GetRecommendedFloppy)(IGuestOSType *pThis, PRBool *recommendedFloppy);

    nsresult (*GetRecommendedUSB)(IGuestOSType *pThis, PRBool *recommendedUSB);

};
#define IGuestOSType_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestOSType_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestOSType_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestOSType_get_FamilyId(p, aFamilyId) ((p)->lpVtbl->GetFamilyId(p, aFamilyId))
#define IGuestOSType_GetFamilyId(p, aFamilyId) ((p)->lpVtbl->GetFamilyId(p, aFamilyId))
#define IGuestOSType_get_FamilyDescription(p, aFamilyDescription) ((p)->lpVtbl->GetFamilyDescription(p, aFamilyDescription))
#define IGuestOSType_GetFamilyDescription(p, aFamilyDescription) ((p)->lpVtbl->GetFamilyDescription(p, aFamilyDescription))
#define IGuestOSType_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestOSType_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestOSType_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IGuestOSType_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IGuestOSType_get_Is64Bit(p, aIs64Bit) ((p)->lpVtbl->GetIs64Bit(p, aIs64Bit))
#define IGuestOSType_GetIs64Bit(p, aIs64Bit) ((p)->lpVtbl->GetIs64Bit(p, aIs64Bit))
#define IGuestOSType_get_RecommendedIOAPIC(p, aRecommendedIOAPIC) ((p)->lpVtbl->GetRecommendedIOAPIC(p, aRecommendedIOAPIC))
#define IGuestOSType_GetRecommendedIOAPIC(p, aRecommendedIOAPIC) ((p)->lpVtbl->GetRecommendedIOAPIC(p, aRecommendedIOAPIC))
#define IGuestOSType_get_RecommendedVirtEx(p, aRecommendedVirtEx) ((p)->lpVtbl->GetRecommendedVirtEx(p, aRecommendedVirtEx))
#define IGuestOSType_GetRecommendedVirtEx(p, aRecommendedVirtEx) ((p)->lpVtbl->GetRecommendedVirtEx(p, aRecommendedVirtEx))
#define IGuestOSType_get_RecommendedRAM(p, aRecommendedRAM) ((p)->lpVtbl->GetRecommendedRAM(p, aRecommendedRAM))
#define IGuestOSType_GetRecommendedRAM(p, aRecommendedRAM) ((p)->lpVtbl->GetRecommendedRAM(p, aRecommendedRAM))
#define IGuestOSType_get_RecommendedVRAM(p, aRecommendedVRAM) ((p)->lpVtbl->GetRecommendedVRAM(p, aRecommendedVRAM))
#define IGuestOSType_GetRecommendedVRAM(p, aRecommendedVRAM) ((p)->lpVtbl->GetRecommendedVRAM(p, aRecommendedVRAM))
#define IGuestOSType_get_Recommended2DVideoAcceleration(p, aRecommended2DVideoAcceleration) ((p)->lpVtbl->GetRecommended2DVideoAcceleration(p, aRecommended2DVideoAcceleration))
#define IGuestOSType_GetRecommended2DVideoAcceleration(p, aRecommended2DVideoAcceleration) ((p)->lpVtbl->GetRecommended2DVideoAcceleration(p, aRecommended2DVideoAcceleration))
#define IGuestOSType_get_Recommended3DAcceleration(p, aRecommended3DAcceleration) ((p)->lpVtbl->GetRecommended3DAcceleration(p, aRecommended3DAcceleration))
#define IGuestOSType_GetRecommended3DAcceleration(p, aRecommended3DAcceleration) ((p)->lpVtbl->GetRecommended3DAcceleration(p, aRecommended3DAcceleration))
#define IGuestOSType_get_RecommendedHDD(p, aRecommendedHDD) ((p)->lpVtbl->GetRecommendedHDD(p, aRecommendedHDD))
#define IGuestOSType_GetRecommendedHDD(p, aRecommendedHDD) ((p)->lpVtbl->GetRecommendedHDD(p, aRecommendedHDD))
#define IGuestOSType_get_AdapterType(p, aAdapterType) ((p)->lpVtbl->GetAdapterType(p, aAdapterType))
#define IGuestOSType_GetAdapterType(p, aAdapterType) ((p)->lpVtbl->GetAdapterType(p, aAdapterType))
#define IGuestOSType_get_RecommendedPAE(p, aRecommendedPAE) ((p)->lpVtbl->GetRecommendedPAE(p, aRecommendedPAE))
#define IGuestOSType_GetRecommendedPAE(p, aRecommendedPAE) ((p)->lpVtbl->GetRecommendedPAE(p, aRecommendedPAE))
#define IGuestOSType_get_RecommendedDVDStorageController(p, aRecommendedDVDStorageController) ((p)->lpVtbl->GetRecommendedDVDStorageController(p, aRecommendedDVDStorageController))
#define IGuestOSType_GetRecommendedDVDStorageController(p, aRecommendedDVDStorageController) ((p)->lpVtbl->GetRecommendedDVDStorageController(p, aRecommendedDVDStorageController))
#define IGuestOSType_get_RecommendedDVDStorageBus(p, aRecommendedDVDStorageBus) ((p)->lpVtbl->GetRecommendedDVDStorageBus(p, aRecommendedDVDStorageBus))
#define IGuestOSType_GetRecommendedDVDStorageBus(p, aRecommendedDVDStorageBus) ((p)->lpVtbl->GetRecommendedDVDStorageBus(p, aRecommendedDVDStorageBus))
#define IGuestOSType_get_RecommendedHDStorageController(p, aRecommendedHDStorageController) ((p)->lpVtbl->GetRecommendedHDStorageController(p, aRecommendedHDStorageController))
#define IGuestOSType_GetRecommendedHDStorageController(p, aRecommendedHDStorageController) ((p)->lpVtbl->GetRecommendedHDStorageController(p, aRecommendedHDStorageController))
#define IGuestOSType_get_RecommendedHDStorageBus(p, aRecommendedHDStorageBus) ((p)->lpVtbl->GetRecommendedHDStorageBus(p, aRecommendedHDStorageBus))
#define IGuestOSType_GetRecommendedHDStorageBus(p, aRecommendedHDStorageBus) ((p)->lpVtbl->GetRecommendedHDStorageBus(p, aRecommendedHDStorageBus))
#define IGuestOSType_get_RecommendedFirmware(p, aRecommendedFirmware) ((p)->lpVtbl->GetRecommendedFirmware(p, aRecommendedFirmware))
#define IGuestOSType_GetRecommendedFirmware(p, aRecommendedFirmware) ((p)->lpVtbl->GetRecommendedFirmware(p, aRecommendedFirmware))
#define IGuestOSType_get_RecommendedUSBHID(p, aRecommendedUSBHID) ((p)->lpVtbl->GetRecommendedUSBHID(p, aRecommendedUSBHID))
#define IGuestOSType_GetRecommendedUSBHID(p, aRecommendedUSBHID) ((p)->lpVtbl->GetRecommendedUSBHID(p, aRecommendedUSBHID))
#define IGuestOSType_get_RecommendedHPET(p, aRecommendedHPET) ((p)->lpVtbl->GetRecommendedHPET(p, aRecommendedHPET))
#define IGuestOSType_GetRecommendedHPET(p, aRecommendedHPET) ((p)->lpVtbl->GetRecommendedHPET(p, aRecommendedHPET))
#define IGuestOSType_get_RecommendedUSBTablet(p, aRecommendedUSBTablet) ((p)->lpVtbl->GetRecommendedUSBTablet(p, aRecommendedUSBTablet))
#define IGuestOSType_GetRecommendedUSBTablet(p, aRecommendedUSBTablet) ((p)->lpVtbl->GetRecommendedUSBTablet(p, aRecommendedUSBTablet))
#define IGuestOSType_get_RecommendedRTCUseUTC(p, aRecommendedRTCUseUTC) ((p)->lpVtbl->GetRecommendedRTCUseUTC(p, aRecommendedRTCUseUTC))
#define IGuestOSType_GetRecommendedRTCUseUTC(p, aRecommendedRTCUseUTC) ((p)->lpVtbl->GetRecommendedRTCUseUTC(p, aRecommendedRTCUseUTC))
#define IGuestOSType_get_RecommendedChipset(p, aRecommendedChipset) ((p)->lpVtbl->GetRecommendedChipset(p, aRecommendedChipset))
#define IGuestOSType_GetRecommendedChipset(p, aRecommendedChipset) ((p)->lpVtbl->GetRecommendedChipset(p, aRecommendedChipset))
#define IGuestOSType_get_RecommendedAudioController(p, aRecommendedAudioController) ((p)->lpVtbl->GetRecommendedAudioController(p, aRecommendedAudioController))
#define IGuestOSType_GetRecommendedAudioController(p, aRecommendedAudioController) ((p)->lpVtbl->GetRecommendedAudioController(p, aRecommendedAudioController))
#define IGuestOSType_get_RecommendedFloppy(p, aRecommendedFloppy) ((p)->lpVtbl->GetRecommendedFloppy(p, aRecommendedFloppy))
#define IGuestOSType_GetRecommendedFloppy(p, aRecommendedFloppy) ((p)->lpVtbl->GetRecommendedFloppy(p, aRecommendedFloppy))
#define IGuestOSType_get_RecommendedUSB(p, aRecommendedUSB) ((p)->lpVtbl->GetRecommendedUSB(p, aRecommendedUSB))
#define IGuestOSType_GetRecommendedUSB(p, aRecommendedUSB) ((p)->lpVtbl->GetRecommendedUSB(p, aRecommendedUSB))
#endif /* VBOX_WITH_GLUE */

interface IGuestOSType
{
#ifndef VBOX_WITH_GLUE
    struct IGuestOSType_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestOSTypeVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestOSType declaration */


/* Start of struct IAdditionsFacility declaration */
#define IADDITIONSFACILITY_IID_STR "54992946-6af1-4e49-98ec-58b558b7291e"
#define IADDITIONSFACILITY_IID { \
    0x54992946, 0x6af1, 0x4e49, \
    { 0x98, 0xec, 0x58, 0xb5, 0x58, 0xb7, 0x29, 0x1e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IAdditionsFacility);
#ifndef VBOX_WITH_GLUE
struct IAdditionsFacility_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetClassType)(IAdditionsFacility *pThis, PRUint32 *classType);

    nsresult (*GetLastUpdated)(IAdditionsFacility *pThis, PRInt64 *lastUpdated);

    nsresult (*GetName)(IAdditionsFacility *pThis, PRUnichar * *name);

    nsresult (*GetStatus)(IAdditionsFacility *pThis, PRUint32 *status);

    nsresult (*GetType)(IAdditionsFacility *pThis, PRUint32 *type);

};
#else /* VBOX_WITH_GLUE */
struct IAdditionsFacilityVtbl
{
    nsresult (*QueryInterface)(IAdditionsFacility *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IAdditionsFacility *pThis);
    nsrefcnt (*Release)(IAdditionsFacility *pThis);
    nsresult (*GetClassType)(IAdditionsFacility *pThis, PRUint32 *classType);

    nsresult (*GetLastUpdated)(IAdditionsFacility *pThis, PRInt64 *lastUpdated);

    nsresult (*GetName)(IAdditionsFacility *pThis, PRUnichar * *name);

    nsresult (*GetStatus)(IAdditionsFacility *pThis, PRUint32 *status);

    nsresult (*GetType)(IAdditionsFacility *pThis, PRUint32 *type);

};
#define IAdditionsFacility_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IAdditionsFacility_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IAdditionsFacility_Release(p) ((p)->lpVtbl->Release(p))
#define IAdditionsFacility_get_ClassType(p, aClassType) ((p)->lpVtbl->GetClassType(p, aClassType))
#define IAdditionsFacility_GetClassType(p, aClassType) ((p)->lpVtbl->GetClassType(p, aClassType))
#define IAdditionsFacility_get_LastUpdated(p, aLastUpdated) ((p)->lpVtbl->GetLastUpdated(p, aLastUpdated))
#define IAdditionsFacility_GetLastUpdated(p, aLastUpdated) ((p)->lpVtbl->GetLastUpdated(p, aLastUpdated))
#define IAdditionsFacility_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IAdditionsFacility_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IAdditionsFacility_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IAdditionsFacility_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IAdditionsFacility_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IAdditionsFacility_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#endif /* VBOX_WITH_GLUE */

interface IAdditionsFacility
{
#ifndef VBOX_WITH_GLUE
    struct IAdditionsFacility_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IAdditionsFacilityVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IAdditionsFacility declaration */


/* Start of struct IGuestSession declaration */
#define IGUESTSESSION_IID_STR "5b28703c-07b6-4fcb-afba-ac199b309752"
#define IGUESTSESSION_IID { \
    0x5b28703c, 0x07b6, 0x4fcb, \
    { 0xaf, 0xba, 0xac, 0x19, 0x9b, 0x30, 0x97, 0x52 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestSession);
#ifndef VBOX_WITH_GLUE
struct IGuestSession_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetUser)(IGuestSession *pThis, PRUnichar * *user);

    nsresult (*GetDomain)(IGuestSession *pThis, PRUnichar * *domain);

    nsresult (*GetName)(IGuestSession *pThis, PRUnichar * *name);

    nsresult (*GetId)(IGuestSession *pThis, PRUint32 *id);

    nsresult (*GetTimeout)(IGuestSession *pThis, PRUint32 *timeout);
    nsresult (*SetTimeout)(IGuestSession *pThis, PRUint32 timeout);

    nsresult (*GetProtocolVersion)(IGuestSession *pThis, PRUint32 *protocolVersion);

    nsresult (*GetStatus)(IGuestSession *pThis, PRUint32 *status);

    nsresult (*GetEnvironment)(IGuestSession *pThis, PRUint32 *environmentSize, PRUnichar * **environment);
    nsresult (*SetEnvironment)(IGuestSession *pThis, PRUint32 environmentSize, PRUnichar * *environment);

    nsresult (*GetProcesses)(IGuestSession *pThis, PRUint32 *processesSize, IGuestProcess * **processes);

    nsresult (*GetDirectories)(IGuestSession *pThis, PRUint32 *directoriesSize, IGuestDirectory * **directories);

    nsresult (*GetFiles)(IGuestSession *pThis, PRUint32 *filesSize, IGuestFile * **files);

    nsresult (*GetEventSource)(IGuestSession *pThis, IEventSource * *eventSource);

    nsresult (*Close)(IGuestSession *pThis );

    nsresult (*CopyFrom)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

    nsresult (*CopyTo)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

    nsresult (*DirectoryCreate)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUint32 mode,
        PRUint32 flagsSize,
        PRUint32* flags
    );

    nsresult (*DirectoryCreateTemp)(
        IGuestSession *pThis,
        PRUnichar * templateName,
        PRUint32 mode,
        PRUnichar * path,
        PRBool secure,
        PRUnichar * * directory
    );

    nsresult (*DirectoryExists)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRBool * exists
    );

    nsresult (*DirectoryOpen)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * filter,
        PRUint32 flagsSize,
        PRUint32* flags,
        IGuestDirectory * * directory
    );

    nsresult (*DirectoryQueryInfo)(
        IGuestSession *pThis,
        PRUnichar * path,
        IGuestFsObjInfo * * info
    );

    nsresult (*DirectoryRemove)(
        IGuestSession *pThis,
        PRUnichar * path
    );

    nsresult (*DirectoryRemoveRecursive)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

    nsresult (*DirectoryRename)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags
    );

    nsresult (*DirectorySetACL)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * acl
    );

    nsresult (*EnvironmentClear)(IGuestSession *pThis );

    nsresult (*EnvironmentGet)(
        IGuestSession *pThis,
        PRUnichar * name,
        PRUnichar * * value
    );

    nsresult (*EnvironmentSet)(
        IGuestSession *pThis,
        PRUnichar * name,
        PRUnichar * value
    );

    nsresult (*EnvironmentUnset)(
        IGuestSession *pThis,
        PRUnichar * name
    );

    nsresult (*FileCreateTemp)(
        IGuestSession *pThis,
        PRUnichar * templateName,
        PRUint32 mode,
        PRUnichar * path,
        PRBool secure,
        IGuestFile * * file
    );

    nsresult (*FileExists)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRBool * exists
    );

    nsresult (*FileRemove)(
        IGuestSession *pThis,
        PRUnichar * path
    );

    nsresult (*FileOpen)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * openMode,
        PRUnichar * disposition,
        PRUint32 creationMode,
        IGuestFile * * file
    );

    nsresult (*FileOpenEx)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * openMode,
        PRUnichar * disposition,
        PRUnichar * sharingMode,
        PRUint32 creationMode,
        PRInt64 offset,
        IGuestFile * * file
    );

    nsresult (*FileQueryInfo)(
        IGuestSession *pThis,
        PRUnichar * path,
        IGuestFsObjInfo * * info
    );

    nsresult (*FileQuerySize)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRInt64 * size
    );

    nsresult (*FileRename)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags
    );

    nsresult (*FileSetACL)(
        IGuestSession *pThis,
        PRUnichar * file,
        PRUnichar * acl
    );

    nsresult (*ProcessCreate)(
        IGuestSession *pThis,
        PRUnichar * command,
        PRUint32 argumentsSize,
        PRUnichar ** arguments,
        PRUint32 environmentSize,
        PRUnichar ** environment,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 timeoutMS,
        IGuestProcess * * guestProcess
    );

    nsresult (*ProcessCreateEx)(
        IGuestSession *pThis,
        PRUnichar * command,
        PRUint32 argumentsSize,
        PRUnichar ** arguments,
        PRUint32 environmentSize,
        PRUnichar ** environment,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 timeoutMS,
        PRUint32 priority,
        PRUint32 affinitySize,
        PRInt32* affinity,
        IGuestProcess * * guestProcess
    );

    nsresult (*ProcessGet)(
        IGuestSession *pThis,
        PRUint32 pid,
        IGuestProcess * * guestProcess
    );

    nsresult (*SymlinkCreate)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * target,
        PRUint32 type
    );

    nsresult (*SymlinkExists)(
        IGuestSession *pThis,
        PRUnichar * symlink,
        PRBool * exists
    );

    nsresult (*SymlinkRead)(
        IGuestSession *pThis,
        PRUnichar * symlink,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUnichar * * target
    );

    nsresult (*SymlinkRemoveDirectory)(
        IGuestSession *pThis,
        PRUnichar * path
    );

    nsresult (*SymlinkRemoveFile)(
        IGuestSession *pThis,
        PRUnichar * file
    );

    nsresult (*WaitFor)(
        IGuestSession *pThis,
        PRUint32 waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*WaitForArray)(
        IGuestSession *pThis,
        PRUint32 waitForSize,
        PRUint32* waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

};
#else /* VBOX_WITH_GLUE */
struct IGuestSessionVtbl
{
    nsresult (*QueryInterface)(IGuestSession *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestSession *pThis);
    nsrefcnt (*Release)(IGuestSession *pThis);
    nsresult (*GetUser)(IGuestSession *pThis, PRUnichar * *user);

    nsresult (*GetDomain)(IGuestSession *pThis, PRUnichar * *domain);

    nsresult (*GetName)(IGuestSession *pThis, PRUnichar * *name);

    nsresult (*GetId)(IGuestSession *pThis, PRUint32 *id);

    nsresult (*GetTimeout)(IGuestSession *pThis, PRUint32 *timeout);
    nsresult (*SetTimeout)(IGuestSession *pThis, PRUint32 timeout);

    nsresult (*GetProtocolVersion)(IGuestSession *pThis, PRUint32 *protocolVersion);

    nsresult (*GetStatus)(IGuestSession *pThis, PRUint32 *status);

    nsresult (*GetEnvironment)(IGuestSession *pThis, PRUint32 *environmentSize, PRUnichar * **environment);
    nsresult (*SetEnvironment)(IGuestSession *pThis, PRUint32 environmentSize, PRUnichar * *environment);

    nsresult (*GetProcesses)(IGuestSession *pThis, PRUint32 *processesSize, IGuestProcess * **processes);

    nsresult (*GetDirectories)(IGuestSession *pThis, PRUint32 *directoriesSize, IGuestDirectory * **directories);

    nsresult (*GetFiles)(IGuestSession *pThis, PRUint32 *filesSize, IGuestFile * **files);

    nsresult (*GetEventSource)(IGuestSession *pThis, IEventSource * *eventSource);

    nsresult (*Close)(IGuestSession *pThis );

    nsresult (*CopyFrom)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

    nsresult (*CopyTo)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

    nsresult (*DirectoryCreate)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUint32 mode,
        PRUint32 flagsSize,
        PRUint32* flags
    );

    nsresult (*DirectoryCreateTemp)(
        IGuestSession *pThis,
        PRUnichar * templateName,
        PRUint32 mode,
        PRUnichar * path,
        PRBool secure,
        PRUnichar * * directory
    );

    nsresult (*DirectoryExists)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRBool * exists
    );

    nsresult (*DirectoryOpen)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * filter,
        PRUint32 flagsSize,
        PRUint32* flags,
        IGuestDirectory * * directory
    );

    nsresult (*DirectoryQueryInfo)(
        IGuestSession *pThis,
        PRUnichar * path,
        IGuestFsObjInfo * * info
    );

    nsresult (*DirectoryRemove)(
        IGuestSession *pThis,
        PRUnichar * path
    );

    nsresult (*DirectoryRemoveRecursive)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

    nsresult (*DirectoryRename)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags
    );

    nsresult (*DirectorySetACL)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * acl
    );

    nsresult (*EnvironmentClear)(IGuestSession *pThis );

    nsresult (*EnvironmentGet)(
        IGuestSession *pThis,
        PRUnichar * name,
        PRUnichar * * value
    );

    nsresult (*EnvironmentSet)(
        IGuestSession *pThis,
        PRUnichar * name,
        PRUnichar * value
    );

    nsresult (*EnvironmentUnset)(
        IGuestSession *pThis,
        PRUnichar * name
    );

    nsresult (*FileCreateTemp)(
        IGuestSession *pThis,
        PRUnichar * templateName,
        PRUint32 mode,
        PRUnichar * path,
        PRBool secure,
        IGuestFile * * file
    );

    nsresult (*FileExists)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRBool * exists
    );

    nsresult (*FileRemove)(
        IGuestSession *pThis,
        PRUnichar * path
    );

    nsresult (*FileOpen)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * openMode,
        PRUnichar * disposition,
        PRUint32 creationMode,
        IGuestFile * * file
    );

    nsresult (*FileOpenEx)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRUnichar * openMode,
        PRUnichar * disposition,
        PRUnichar * sharingMode,
        PRUint32 creationMode,
        PRInt64 offset,
        IGuestFile * * file
    );

    nsresult (*FileQueryInfo)(
        IGuestSession *pThis,
        PRUnichar * path,
        IGuestFsObjInfo * * info
    );

    nsresult (*FileQuerySize)(
        IGuestSession *pThis,
        PRUnichar * path,
        PRInt64 * size
    );

    nsresult (*FileRename)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * dest,
        PRUint32 flagsSize,
        PRUint32* flags
    );

    nsresult (*FileSetACL)(
        IGuestSession *pThis,
        PRUnichar * file,
        PRUnichar * acl
    );

    nsresult (*ProcessCreate)(
        IGuestSession *pThis,
        PRUnichar * command,
        PRUint32 argumentsSize,
        PRUnichar ** arguments,
        PRUint32 environmentSize,
        PRUnichar ** environment,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 timeoutMS,
        IGuestProcess * * guestProcess
    );

    nsresult (*ProcessCreateEx)(
        IGuestSession *pThis,
        PRUnichar * command,
        PRUint32 argumentsSize,
        PRUnichar ** arguments,
        PRUint32 environmentSize,
        PRUnichar ** environment,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 timeoutMS,
        PRUint32 priority,
        PRUint32 affinitySize,
        PRInt32* affinity,
        IGuestProcess * * guestProcess
    );

    nsresult (*ProcessGet)(
        IGuestSession *pThis,
        PRUint32 pid,
        IGuestProcess * * guestProcess
    );

    nsresult (*SymlinkCreate)(
        IGuestSession *pThis,
        PRUnichar * source,
        PRUnichar * target,
        PRUint32 type
    );

    nsresult (*SymlinkExists)(
        IGuestSession *pThis,
        PRUnichar * symlink,
        PRBool * exists
    );

    nsresult (*SymlinkRead)(
        IGuestSession *pThis,
        PRUnichar * symlink,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUnichar * * target
    );

    nsresult (*SymlinkRemoveDirectory)(
        IGuestSession *pThis,
        PRUnichar * path
    );

    nsresult (*SymlinkRemoveFile)(
        IGuestSession *pThis,
        PRUnichar * file
    );

    nsresult (*WaitFor)(
        IGuestSession *pThis,
        PRUint32 waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*WaitForArray)(
        IGuestSession *pThis,
        PRUint32 waitForSize,
        PRUint32* waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

};
#define IGuestSession_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestSession_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestSession_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestSession_get_User(p, aUser) ((p)->lpVtbl->GetUser(p, aUser))
#define IGuestSession_GetUser(p, aUser) ((p)->lpVtbl->GetUser(p, aUser))
#define IGuestSession_get_Domain(p, aDomain) ((p)->lpVtbl->GetDomain(p, aDomain))
#define IGuestSession_GetDomain(p, aDomain) ((p)->lpVtbl->GetDomain(p, aDomain))
#define IGuestSession_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestSession_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestSession_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestSession_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestSession_get_Timeout(p, aTimeout) ((p)->lpVtbl->GetTimeout(p, aTimeout))
#define IGuestSession_GetTimeout(p, aTimeout) ((p)->lpVtbl->GetTimeout(p, aTimeout))
#define IGuestSession_put_Timeout(p, aTimeout) ((p)->lpVtbl->SetTimeout(p, aTimeout))
#define IGuestSession_SetTimeout(p, aTimeout) ((p)->lpVtbl->SetTimeout(p, aTimeout))
#define IGuestSession_get_ProtocolVersion(p, aProtocolVersion) ((p)->lpVtbl->GetProtocolVersion(p, aProtocolVersion))
#define IGuestSession_GetProtocolVersion(p, aProtocolVersion) ((p)->lpVtbl->GetProtocolVersion(p, aProtocolVersion))
#define IGuestSession_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestSession_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestSession_get_Environment(p, aEnvironment) ((p)->lpVtbl->GetEnvironment(p, aEnvironment))
#define IGuestSession_GetEnvironment(p, aEnvironment) ((p)->lpVtbl->GetEnvironment(p, aEnvironment))
#define IGuestSession_put_Environment(p, aEnvironment) ((p)->lpVtbl->SetEnvironment(p, aEnvironment))
#define IGuestSession_SetEnvironment(p, aEnvironment) ((p)->lpVtbl->SetEnvironment(p, aEnvironment))
#define IGuestSession_get_Processes(p, aProcesses) ((p)->lpVtbl->GetProcesses(p, aProcesses))
#define IGuestSession_GetProcesses(p, aProcesses) ((p)->lpVtbl->GetProcesses(p, aProcesses))
#define IGuestSession_get_Directories(p, aDirectories) ((p)->lpVtbl->GetDirectories(p, aDirectories))
#define IGuestSession_GetDirectories(p, aDirectories) ((p)->lpVtbl->GetDirectories(p, aDirectories))
#define IGuestSession_get_Files(p, aFiles) ((p)->lpVtbl->GetFiles(p, aFiles))
#define IGuestSession_GetFiles(p, aFiles) ((p)->lpVtbl->GetFiles(p, aFiles))
#define IGuestSession_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuestSession_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuestSession_Close(p) ((p)->lpVtbl->Close(p))
#define IGuestSession_CopyFrom(p, aSource, aDest, aFlags, aProgress) ((p)->lpVtbl->CopyFrom(p, aSource, aDest, aFlags, aProgress))
#define IGuestSession_CopyTo(p, aSource, aDest, aFlags, aProgress) ((p)->lpVtbl->CopyTo(p, aSource, aDest, aFlags, aProgress))
#define IGuestSession_DirectoryCreate(p, aPath, aMode, aFlags) ((p)->lpVtbl->DirectoryCreate(p, aPath, aMode, aFlags))
#define IGuestSession_DirectoryCreateTemp(p, aTemplateName, aMode, aPath, aSecure, aDirectory) ((p)->lpVtbl->DirectoryCreateTemp(p, aTemplateName, aMode, aPath, aSecure, aDirectory))
#define IGuestSession_DirectoryExists(p, aPath, aExists) ((p)->lpVtbl->DirectoryExists(p, aPath, aExists))
#define IGuestSession_DirectoryOpen(p, aPath, aFilter, aFlags, aDirectory) ((p)->lpVtbl->DirectoryOpen(p, aPath, aFilter, aFlags, aDirectory))
#define IGuestSession_DirectoryQueryInfo(p, aPath, aInfo) ((p)->lpVtbl->DirectoryQueryInfo(p, aPath, aInfo))
#define IGuestSession_DirectoryRemove(p, aPath) ((p)->lpVtbl->DirectoryRemove(p, aPath))
#define IGuestSession_DirectoryRemoveRecursive(p, aPath, aFlags, aProgress) ((p)->lpVtbl->DirectoryRemoveRecursive(p, aPath, aFlags, aProgress))
#define IGuestSession_DirectoryRename(p, aSource, aDest, aFlags) ((p)->lpVtbl->DirectoryRename(p, aSource, aDest, aFlags))
#define IGuestSession_DirectorySetACL(p, aPath, aAcl) ((p)->lpVtbl->DirectorySetACL(p, aPath, aAcl))
#define IGuestSession_EnvironmentClear(p) ((p)->lpVtbl->EnvironmentClear(p))
#define IGuestSession_EnvironmentGet(p, aName, aValue) ((p)->lpVtbl->EnvironmentGet(p, aName, aValue))
#define IGuestSession_EnvironmentSet(p, aName, aValue) ((p)->lpVtbl->EnvironmentSet(p, aName, aValue))
#define IGuestSession_EnvironmentUnset(p, aName) ((p)->lpVtbl->EnvironmentUnset(p, aName))
#define IGuestSession_FileCreateTemp(p, aTemplateName, aMode, aPath, aSecure, aFile) ((p)->lpVtbl->FileCreateTemp(p, aTemplateName, aMode, aPath, aSecure, aFile))
#define IGuestSession_FileExists(p, aPath, aExists) ((p)->lpVtbl->FileExists(p, aPath, aExists))
#define IGuestSession_FileRemove(p, aPath) ((p)->lpVtbl->FileRemove(p, aPath))
#define IGuestSession_FileOpen(p, aPath, aOpenMode, aDisposition, aCreationMode, aFile) ((p)->lpVtbl->FileOpen(p, aPath, aOpenMode, aDisposition, aCreationMode, aFile))
#define IGuestSession_FileOpenEx(p, aPath, aOpenMode, aDisposition, aSharingMode, aCreationMode, aOffset, aFile) ((p)->lpVtbl->FileOpenEx(p, aPath, aOpenMode, aDisposition, aSharingMode, aCreationMode, aOffset, aFile))
#define IGuestSession_FileQueryInfo(p, aPath, aInfo) ((p)->lpVtbl->FileQueryInfo(p, aPath, aInfo))
#define IGuestSession_FileQuerySize(p, aPath, aSize) ((p)->lpVtbl->FileQuerySize(p, aPath, aSize))
#define IGuestSession_FileRename(p, aSource, aDest, aFlags) ((p)->lpVtbl->FileRename(p, aSource, aDest, aFlags))
#define IGuestSession_FileSetACL(p, aFile, aAcl) ((p)->lpVtbl->FileSetACL(p, aFile, aAcl))
#define IGuestSession_ProcessCreate(p, aCommand, aArguments, aEnvironment, aFlags, aTimeoutMS, aGuestProcess) ((p)->lpVtbl->ProcessCreate(p, aCommand, aArguments, aEnvironment, aFlags, aTimeoutMS, aGuestProcess))
#define IGuestSession_ProcessCreateEx(p, aCommand, aArguments, aEnvironment, aFlags, aTimeoutMS, aPriority, aAffinity, aGuestProcess) ((p)->lpVtbl->ProcessCreateEx(p, aCommand, aArguments, aEnvironment, aFlags, aTimeoutMS, aPriority, aAffinity, aGuestProcess))
#define IGuestSession_ProcessGet(p, aPid, aGuestProcess) ((p)->lpVtbl->ProcessGet(p, aPid, aGuestProcess))
#define IGuestSession_SymlinkCreate(p, aSource, aTarget, aType) ((p)->lpVtbl->SymlinkCreate(p, aSource, aTarget, aType))
#define IGuestSession_SymlinkExists(p, aSymlink, aExists) ((p)->lpVtbl->SymlinkExists(p, aSymlink, aExists))
#define IGuestSession_SymlinkRead(p, aSymlink, aFlags, aTarget) ((p)->lpVtbl->SymlinkRead(p, aSymlink, aFlags, aTarget))
#define IGuestSession_SymlinkRemoveDirectory(p, aPath) ((p)->lpVtbl->SymlinkRemoveDirectory(p, aPath))
#define IGuestSession_SymlinkRemoveFile(p, aFile) ((p)->lpVtbl->SymlinkRemoveFile(p, aFile))
#define IGuestSession_WaitFor(p, aWaitFor, aTimeoutMS, aReason) ((p)->lpVtbl->WaitFor(p, aWaitFor, aTimeoutMS, aReason))
#define IGuestSession_WaitForArray(p, aWaitFor, aTimeoutMS, aReason) ((p)->lpVtbl->WaitForArray(p, aWaitFor, aTimeoutMS, aReason))
#endif /* VBOX_WITH_GLUE */

interface IGuestSession
{
#ifndef VBOX_WITH_GLUE
    struct IGuestSession_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestSessionVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestSession declaration */


/* Start of struct IProcess declaration */
#define IPROCESS_IID_STR "5a4fe06d-8cb1-40ff-ac9e-9676e32f706e"
#define IPROCESS_IID { \
    0x5a4fe06d, 0x8cb1, 0x40ff, \
    { 0xac, 0x9e, 0x96, 0x76, 0xe3, 0x2f, 0x70, 0x6e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IProcess);
#ifndef VBOX_WITH_GLUE
struct IProcess_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetArguments)(IProcess *pThis, PRUint32 *argumentsSize, PRUnichar * **arguments);

    nsresult (*GetEnvironment)(IProcess *pThis, PRUint32 *environmentSize, PRUnichar * **environment);

    nsresult (*GetEventSource)(IProcess *pThis, IEventSource * *eventSource);

    nsresult (*GetExecutablePath)(IProcess *pThis, PRUnichar * *executablePath);

    nsresult (*GetExitCode)(IProcess *pThis, PRInt32 *exitCode);

    nsresult (*GetName)(IProcess *pThis, PRUnichar * *name);

    nsresult (*GetPID)(IProcess *pThis, PRUint32 *PID);

    nsresult (*GetStatus)(IProcess *pThis, PRUint32 *status);

    nsresult (*WaitFor)(
        IProcess *pThis,
        PRUint32 waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*WaitForArray)(
        IProcess *pThis,
        PRUint32 waitForSize,
        PRUint32* waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*Read)(
        IProcess *pThis,
        PRUint32 handle,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*Write)(
        IProcess *pThis,
        PRUint32 handle,
        PRUint32 flags,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*WriteArray)(
        IProcess *pThis,
        PRUint32 handle,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*Terminate)(IProcess *pThis );

};
#else /* VBOX_WITH_GLUE */
struct IProcessVtbl
{
    nsresult (*QueryInterface)(IProcess *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IProcess *pThis);
    nsrefcnt (*Release)(IProcess *pThis);
    nsresult (*GetArguments)(IProcess *pThis, PRUint32 *argumentsSize, PRUnichar * **arguments);

    nsresult (*GetEnvironment)(IProcess *pThis, PRUint32 *environmentSize, PRUnichar * **environment);

    nsresult (*GetEventSource)(IProcess *pThis, IEventSource * *eventSource);

    nsresult (*GetExecutablePath)(IProcess *pThis, PRUnichar * *executablePath);

    nsresult (*GetExitCode)(IProcess *pThis, PRInt32 *exitCode);

    nsresult (*GetName)(IProcess *pThis, PRUnichar * *name);

    nsresult (*GetPID)(IProcess *pThis, PRUint32 *PID);

    nsresult (*GetStatus)(IProcess *pThis, PRUint32 *status);

    nsresult (*WaitFor)(
        IProcess *pThis,
        PRUint32 waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*WaitForArray)(
        IProcess *pThis,
        PRUint32 waitForSize,
        PRUint32* waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*Read)(
        IProcess *pThis,
        PRUint32 handle,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*Write)(
        IProcess *pThis,
        PRUint32 handle,
        PRUint32 flags,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*WriteArray)(
        IProcess *pThis,
        PRUint32 handle,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*Terminate)(IProcess *pThis );

};
#define IProcess_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IProcess_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IProcess_Release(p) ((p)->lpVtbl->Release(p))
#define IProcess_get_Arguments(p, aArguments) ((p)->lpVtbl->GetArguments(p, aArguments))
#define IProcess_GetArguments(p, aArguments) ((p)->lpVtbl->GetArguments(p, aArguments))
#define IProcess_get_Environment(p, aEnvironment) ((p)->lpVtbl->GetEnvironment(p, aEnvironment))
#define IProcess_GetEnvironment(p, aEnvironment) ((p)->lpVtbl->GetEnvironment(p, aEnvironment))
#define IProcess_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IProcess_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IProcess_get_ExecutablePath(p, aExecutablePath) ((p)->lpVtbl->GetExecutablePath(p, aExecutablePath))
#define IProcess_GetExecutablePath(p, aExecutablePath) ((p)->lpVtbl->GetExecutablePath(p, aExecutablePath))
#define IProcess_get_ExitCode(p, aExitCode) ((p)->lpVtbl->GetExitCode(p, aExitCode))
#define IProcess_GetExitCode(p, aExitCode) ((p)->lpVtbl->GetExitCode(p, aExitCode))
#define IProcess_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IProcess_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IProcess_get_PID(p, aPID) ((p)->lpVtbl->GetPID(p, aPID))
#define IProcess_GetPID(p, aPID) ((p)->lpVtbl->GetPID(p, aPID))
#define IProcess_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IProcess_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IProcess_WaitFor(p, aWaitFor, aTimeoutMS, aReason) ((p)->lpVtbl->WaitFor(p, aWaitFor, aTimeoutMS, aReason))
#define IProcess_WaitForArray(p, aWaitFor, aTimeoutMS, aReason) ((p)->lpVtbl->WaitForArray(p, aWaitFor, aTimeoutMS, aReason))
#define IProcess_Read(p, aHandle, aToRead, aTimeoutMS, aData) ((p)->lpVtbl->Read(p, aHandle, aToRead, aTimeoutMS, aData))
#define IProcess_Write(p, aHandle, aFlags, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->Write(p, aHandle, aFlags, aData, aTimeoutMS, aWritten))
#define IProcess_WriteArray(p, aHandle, aFlags, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->WriteArray(p, aHandle, aFlags, aData, aTimeoutMS, aWritten))
#define IProcess_Terminate(p) ((p)->lpVtbl->Terminate(p))
#endif /* VBOX_WITH_GLUE */

interface IProcess
{
#ifndef VBOX_WITH_GLUE
    struct IProcess_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IProcessVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IProcess declaration */


/* Start of struct IGuestProcess declaration */
#define IGUESTPROCESS_IID_STR "dfa39a36-5d43-4840-a025-67ea956b3111"
#define IGUESTPROCESS_IID { \
    0xdfa39a36, 0x5d43, 0x4840, \
    { 0xa0, 0x25, 0x67, 0xea, 0x95, 0x6b, 0x31, 0x11 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcess);
#ifndef VBOX_WITH_GLUE
struct IGuestProcess_vtbl
{
    struct IProcess_vtbl iprocess;

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessVtbl
{
    nsresult (*QueryInterface)(IGuestProcess *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcess *pThis);
    nsrefcnt (*Release)(IGuestProcess *pThis);
    nsresult (*GetArguments)(IGuestProcess *pThis, PRUint32 *argumentsSize, PRUnichar * **arguments);

    nsresult (*GetEnvironment)(IGuestProcess *pThis, PRUint32 *environmentSize, PRUnichar * **environment);

    nsresult (*GetEventSource)(IGuestProcess *pThis, IEventSource * *eventSource);

    nsresult (*GetExecutablePath)(IGuestProcess *pThis, PRUnichar * *executablePath);

    nsresult (*GetExitCode)(IGuestProcess *pThis, PRInt32 *exitCode);

    nsresult (*GetName)(IGuestProcess *pThis, PRUnichar * *name);

    nsresult (*GetPID)(IGuestProcess *pThis, PRUint32 *PID);

    nsresult (*GetStatus)(IGuestProcess *pThis, PRUint32 *status);

    nsresult (*WaitFor)(
        IGuestProcess *pThis,
        PRUint32 waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*WaitForArray)(
        IGuestProcess *pThis,
        PRUint32 waitForSize,
        PRUint32* waitFor,
        PRUint32 timeoutMS,
        PRUint32 * reason
    );

    nsresult (*Read)(
        IGuestProcess *pThis,
        PRUint32 handle,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*Write)(
        IGuestProcess *pThis,
        PRUint32 handle,
        PRUint32 flags,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*WriteArray)(
        IGuestProcess *pThis,
        PRUint32 handle,
        PRUint32 flagsSize,
        PRUint32* flags,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*Terminate)(IGuestProcess *pThis );

};
#define IGuestProcess_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcess_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcess_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcess_get_Arguments(p, aArguments) ((p)->lpVtbl->GetArguments(p, aArguments))
#define IGuestProcess_GetArguments(p, aArguments) ((p)->lpVtbl->GetArguments(p, aArguments))
#define IGuestProcess_get_Environment(p, aEnvironment) ((p)->lpVtbl->GetEnvironment(p, aEnvironment))
#define IGuestProcess_GetEnvironment(p, aEnvironment) ((p)->lpVtbl->GetEnvironment(p, aEnvironment))
#define IGuestProcess_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuestProcess_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuestProcess_get_ExecutablePath(p, aExecutablePath) ((p)->lpVtbl->GetExecutablePath(p, aExecutablePath))
#define IGuestProcess_GetExecutablePath(p, aExecutablePath) ((p)->lpVtbl->GetExecutablePath(p, aExecutablePath))
#define IGuestProcess_get_ExitCode(p, aExitCode) ((p)->lpVtbl->GetExitCode(p, aExitCode))
#define IGuestProcess_GetExitCode(p, aExitCode) ((p)->lpVtbl->GetExitCode(p, aExitCode))
#define IGuestProcess_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestProcess_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestProcess_get_PID(p, aPID) ((p)->lpVtbl->GetPID(p, aPID))
#define IGuestProcess_GetPID(p, aPID) ((p)->lpVtbl->GetPID(p, aPID))
#define IGuestProcess_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestProcess_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestProcess_WaitFor(p, aWaitFor, aTimeoutMS, aReason) ((p)->lpVtbl->WaitFor(p, aWaitFor, aTimeoutMS, aReason))
#define IGuestProcess_WaitForArray(p, aWaitFor, aTimeoutMS, aReason) ((p)->lpVtbl->WaitForArray(p, aWaitFor, aTimeoutMS, aReason))
#define IGuestProcess_Read(p, aHandle, aToRead, aTimeoutMS, aData) ((p)->lpVtbl->Read(p, aHandle, aToRead, aTimeoutMS, aData))
#define IGuestProcess_Write(p, aHandle, aFlags, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->Write(p, aHandle, aFlags, aData, aTimeoutMS, aWritten))
#define IGuestProcess_WriteArray(p, aHandle, aFlags, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->WriteArray(p, aHandle, aFlags, aData, aTimeoutMS, aWritten))
#define IGuestProcess_Terminate(p) ((p)->lpVtbl->Terminate(p))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcess
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcess_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcess declaration */


/* Start of struct IDirectory declaration */
#define IDIRECTORY_IID_STR "1b70dd03-26d7-483a-8877-89bbb0f87b70"
#define IDIRECTORY_IID { \
    0x1b70dd03, 0x26d7, 0x483a, \
    { 0x88, 0x77, 0x89, 0xbb, 0xb0, 0xf8, 0x7b, 0x70 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IDirectory);
#ifndef VBOX_WITH_GLUE
struct IDirectory_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetDirectoryName)(IDirectory *pThis, PRUnichar * *directoryName);

    nsresult (*GetFilter)(IDirectory *pThis, PRUnichar * *filter);

    nsresult (*Close)(IDirectory *pThis );

    nsresult (*Read)(
        IDirectory *pThis,
        IFsObjInfo * * objInfo
    );

};
#else /* VBOX_WITH_GLUE */
struct IDirectoryVtbl
{
    nsresult (*QueryInterface)(IDirectory *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IDirectory *pThis);
    nsrefcnt (*Release)(IDirectory *pThis);
    nsresult (*GetDirectoryName)(IDirectory *pThis, PRUnichar * *directoryName);

    nsresult (*GetFilter)(IDirectory *pThis, PRUnichar * *filter);

    nsresult (*Close)(IDirectory *pThis );

    nsresult (*Read)(
        IDirectory *pThis,
        IFsObjInfo * * objInfo
    );

};
#define IDirectory_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IDirectory_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IDirectory_Release(p) ((p)->lpVtbl->Release(p))
#define IDirectory_get_DirectoryName(p, aDirectoryName) ((p)->lpVtbl->GetDirectoryName(p, aDirectoryName))
#define IDirectory_GetDirectoryName(p, aDirectoryName) ((p)->lpVtbl->GetDirectoryName(p, aDirectoryName))
#define IDirectory_get_Filter(p, aFilter) ((p)->lpVtbl->GetFilter(p, aFilter))
#define IDirectory_GetFilter(p, aFilter) ((p)->lpVtbl->GetFilter(p, aFilter))
#define IDirectory_Close(p) ((p)->lpVtbl->Close(p))
#define IDirectory_Read(p, aObjInfo) ((p)->lpVtbl->Read(p, aObjInfo))
#endif /* VBOX_WITH_GLUE */

interface IDirectory
{
#ifndef VBOX_WITH_GLUE
    struct IDirectory_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IDirectoryVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IDirectory declaration */


/* Start of struct IGuestDirectory declaration */
#define IGUESTDIRECTORY_IID_STR "af4a8ce0-0725-42b7-8826-46e3c7ba7357"
#define IGUESTDIRECTORY_IID { \
    0xaf4a8ce0, 0x0725, 0x42b7, \
    { 0x88, 0x26, 0x46, 0xe3, 0xc7, 0xba, 0x73, 0x57 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestDirectory);
#ifndef VBOX_WITH_GLUE
struct IGuestDirectory_vtbl
{
    struct IDirectory_vtbl idirectory;

};
#else /* VBOX_WITH_GLUE */
struct IGuestDirectoryVtbl
{
    nsresult (*QueryInterface)(IGuestDirectory *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestDirectory *pThis);
    nsrefcnt (*Release)(IGuestDirectory *pThis);
    nsresult (*GetDirectoryName)(IGuestDirectory *pThis, PRUnichar * *directoryName);

    nsresult (*GetFilter)(IGuestDirectory *pThis, PRUnichar * *filter);

    nsresult (*Close)(IGuestDirectory *pThis );

    nsresult (*Read)(
        IGuestDirectory *pThis,
        IFsObjInfo * * objInfo
    );

};
#define IGuestDirectory_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestDirectory_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestDirectory_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestDirectory_get_DirectoryName(p, aDirectoryName) ((p)->lpVtbl->GetDirectoryName(p, aDirectoryName))
#define IGuestDirectory_GetDirectoryName(p, aDirectoryName) ((p)->lpVtbl->GetDirectoryName(p, aDirectoryName))
#define IGuestDirectory_get_Filter(p, aFilter) ((p)->lpVtbl->GetFilter(p, aFilter))
#define IGuestDirectory_GetFilter(p, aFilter) ((p)->lpVtbl->GetFilter(p, aFilter))
#define IGuestDirectory_Close(p) ((p)->lpVtbl->Close(p))
#define IGuestDirectory_Read(p, aObjInfo) ((p)->lpVtbl->Read(p, aObjInfo))
#endif /* VBOX_WITH_GLUE */

interface IGuestDirectory
{
#ifndef VBOX_WITH_GLUE
    struct IGuestDirectory_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestDirectoryVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestDirectory declaration */


/* Start of struct IFile declaration */
#define IFILE_IID_STR "5ec56ea3-b55d-4bdb-8c4f-5f9fb26b894b"
#define IFILE_IID { \
    0x5ec56ea3, 0xb55d, 0x4bdb, \
    { 0x8c, 0x4f, 0x5f, 0x9f, 0xb2, 0x6b, 0x89, 0x4b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IFile);
#ifndef VBOX_WITH_GLUE
struct IFile_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetCreationMode)(IFile *pThis, PRUint32 *creationMode);

    nsresult (*GetDisposition)(IFile *pThis, PRUnichar * *disposition);

    nsresult (*GetEventSource)(IFile *pThis, IEventSource * *eventSource);

    nsresult (*GetFileName)(IFile *pThis, PRUnichar * *fileName);

    nsresult (*GetId)(IFile *pThis, PRUint32 *id);

    nsresult (*GetInitialSize)(IFile *pThis, PRInt64 *initialSize);

    nsresult (*GetOpenMode)(IFile *pThis, PRUnichar * *openMode);

    nsresult (*GetOffset)(IFile *pThis, PRInt64 *offset);

    nsresult (*GetStatus)(IFile *pThis, PRUint32 *status);

    nsresult (*Close)(IFile *pThis );

    nsresult (*QueryInfo)(
        IFile *pThis,
        IFsObjInfo * * objInfo
    );

    nsresult (*Read)(
        IFile *pThis,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*ReadAt)(
        IFile *pThis,
        PRInt64 offset,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*Seek)(
        IFile *pThis,
        PRInt64 offset,
        PRUint32 whence
    );

    nsresult (*SetACL)(
        IFile *pThis,
        PRUnichar * acl
    );

    nsresult (*Write)(
        IFile *pThis,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*WriteAt)(
        IFile *pThis,
        PRInt64 offset,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

};
#else /* VBOX_WITH_GLUE */
struct IFileVtbl
{
    nsresult (*QueryInterface)(IFile *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IFile *pThis);
    nsrefcnt (*Release)(IFile *pThis);
    nsresult (*GetCreationMode)(IFile *pThis, PRUint32 *creationMode);

    nsresult (*GetDisposition)(IFile *pThis, PRUnichar * *disposition);

    nsresult (*GetEventSource)(IFile *pThis, IEventSource * *eventSource);

    nsresult (*GetFileName)(IFile *pThis, PRUnichar * *fileName);

    nsresult (*GetId)(IFile *pThis, PRUint32 *id);

    nsresult (*GetInitialSize)(IFile *pThis, PRInt64 *initialSize);

    nsresult (*GetOpenMode)(IFile *pThis, PRUnichar * *openMode);

    nsresult (*GetOffset)(IFile *pThis, PRInt64 *offset);

    nsresult (*GetStatus)(IFile *pThis, PRUint32 *status);

    nsresult (*Close)(IFile *pThis );

    nsresult (*QueryInfo)(
        IFile *pThis,
        IFsObjInfo * * objInfo
    );

    nsresult (*Read)(
        IFile *pThis,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*ReadAt)(
        IFile *pThis,
        PRInt64 offset,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*Seek)(
        IFile *pThis,
        PRInt64 offset,
        PRUint32 whence
    );

    nsresult (*SetACL)(
        IFile *pThis,
        PRUnichar * acl
    );

    nsresult (*Write)(
        IFile *pThis,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*WriteAt)(
        IFile *pThis,
        PRInt64 offset,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

};
#define IFile_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IFile_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IFile_Release(p) ((p)->lpVtbl->Release(p))
#define IFile_get_CreationMode(p, aCreationMode) ((p)->lpVtbl->GetCreationMode(p, aCreationMode))
#define IFile_GetCreationMode(p, aCreationMode) ((p)->lpVtbl->GetCreationMode(p, aCreationMode))
#define IFile_get_Disposition(p, aDisposition) ((p)->lpVtbl->GetDisposition(p, aDisposition))
#define IFile_GetDisposition(p, aDisposition) ((p)->lpVtbl->GetDisposition(p, aDisposition))
#define IFile_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IFile_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IFile_get_FileName(p, aFileName) ((p)->lpVtbl->GetFileName(p, aFileName))
#define IFile_GetFileName(p, aFileName) ((p)->lpVtbl->GetFileName(p, aFileName))
#define IFile_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IFile_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IFile_get_InitialSize(p, aInitialSize) ((p)->lpVtbl->GetInitialSize(p, aInitialSize))
#define IFile_GetInitialSize(p, aInitialSize) ((p)->lpVtbl->GetInitialSize(p, aInitialSize))
#define IFile_get_OpenMode(p, aOpenMode) ((p)->lpVtbl->GetOpenMode(p, aOpenMode))
#define IFile_GetOpenMode(p, aOpenMode) ((p)->lpVtbl->GetOpenMode(p, aOpenMode))
#define IFile_get_Offset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IFile_GetOffset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IFile_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IFile_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IFile_Close(p) ((p)->lpVtbl->Close(p))
#define IFile_QueryInfo(p, aObjInfo) ((p)->lpVtbl->QueryInfo(p, aObjInfo))
#define IFile_Read(p, aToRead, aTimeoutMS, aData) ((p)->lpVtbl->Read(p, aToRead, aTimeoutMS, aData))
#define IFile_ReadAt(p, aOffset, aToRead, aTimeoutMS, aData) ((p)->lpVtbl->ReadAt(p, aOffset, aToRead, aTimeoutMS, aData))
#define IFile_Seek(p, aOffset, aWhence) ((p)->lpVtbl->Seek(p, aOffset, aWhence))
#define IFile_SetACL(p, aAcl) ((p)->lpVtbl->SetACL(p, aAcl))
#define IFile_Write(p, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->Write(p, aData, aTimeoutMS, aWritten))
#define IFile_WriteAt(p, aOffset, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->WriteAt(p, aOffset, aData, aTimeoutMS, aWritten))
#endif /* VBOX_WITH_GLUE */

interface IFile
{
#ifndef VBOX_WITH_GLUE
    struct IFile_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IFileVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IFile declaration */


/* Start of struct IGuestFile declaration */
#define IGUESTFILE_IID_STR "60661aec-145f-4d11-b80e-8ea151598093"
#define IGUESTFILE_IID { \
    0x60661aec, 0x145f, 0x4d11, \
    { 0xb8, 0x0e, 0x8e, 0xa1, 0x51, 0x59, 0x80, 0x93 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFile);
#ifndef VBOX_WITH_GLUE
struct IGuestFile_vtbl
{
    struct IFile_vtbl ifile;

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileVtbl
{
    nsresult (*QueryInterface)(IGuestFile *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFile *pThis);
    nsrefcnt (*Release)(IGuestFile *pThis);
    nsresult (*GetCreationMode)(IGuestFile *pThis, PRUint32 *creationMode);

    nsresult (*GetDisposition)(IGuestFile *pThis, PRUnichar * *disposition);

    nsresult (*GetEventSource)(IGuestFile *pThis, IEventSource * *eventSource);

    nsresult (*GetFileName)(IGuestFile *pThis, PRUnichar * *fileName);

    nsresult (*GetId)(IGuestFile *pThis, PRUint32 *id);

    nsresult (*GetInitialSize)(IGuestFile *pThis, PRInt64 *initialSize);

    nsresult (*GetOpenMode)(IGuestFile *pThis, PRUnichar * *openMode);

    nsresult (*GetOffset)(IGuestFile *pThis, PRInt64 *offset);

    nsresult (*GetStatus)(IGuestFile *pThis, PRUint32 *status);

    nsresult (*Close)(IGuestFile *pThis );

    nsresult (*QueryInfo)(
        IGuestFile *pThis,
        IFsObjInfo * * objInfo
    );

    nsresult (*Read)(
        IGuestFile *pThis,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*ReadAt)(
        IGuestFile *pThis,
        PRInt64 offset,
        PRUint32 toRead,
        PRUint32 timeoutMS,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*Seek)(
        IGuestFile *pThis,
        PRInt64 offset,
        PRUint32 whence
    );

    nsresult (*SetACL)(
        IGuestFile *pThis,
        PRUnichar * acl
    );

    nsresult (*Write)(
        IGuestFile *pThis,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

    nsresult (*WriteAt)(
        IGuestFile *pThis,
        PRInt64 offset,
        PRUint32 dataSize,
        PRUint8* data,
        PRUint32 timeoutMS,
        PRUint32 * written
    );

};
#define IGuestFile_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFile_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFile_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFile_get_CreationMode(p, aCreationMode) ((p)->lpVtbl->GetCreationMode(p, aCreationMode))
#define IGuestFile_GetCreationMode(p, aCreationMode) ((p)->lpVtbl->GetCreationMode(p, aCreationMode))
#define IGuestFile_get_Disposition(p, aDisposition) ((p)->lpVtbl->GetDisposition(p, aDisposition))
#define IGuestFile_GetDisposition(p, aDisposition) ((p)->lpVtbl->GetDisposition(p, aDisposition))
#define IGuestFile_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuestFile_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuestFile_get_FileName(p, aFileName) ((p)->lpVtbl->GetFileName(p, aFileName))
#define IGuestFile_GetFileName(p, aFileName) ((p)->lpVtbl->GetFileName(p, aFileName))
#define IGuestFile_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestFile_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestFile_get_InitialSize(p, aInitialSize) ((p)->lpVtbl->GetInitialSize(p, aInitialSize))
#define IGuestFile_GetInitialSize(p, aInitialSize) ((p)->lpVtbl->GetInitialSize(p, aInitialSize))
#define IGuestFile_get_OpenMode(p, aOpenMode) ((p)->lpVtbl->GetOpenMode(p, aOpenMode))
#define IGuestFile_GetOpenMode(p, aOpenMode) ((p)->lpVtbl->GetOpenMode(p, aOpenMode))
#define IGuestFile_get_Offset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFile_GetOffset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFile_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestFile_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestFile_Close(p) ((p)->lpVtbl->Close(p))
#define IGuestFile_QueryInfo(p, aObjInfo) ((p)->lpVtbl->QueryInfo(p, aObjInfo))
#define IGuestFile_Read(p, aToRead, aTimeoutMS, aData) ((p)->lpVtbl->Read(p, aToRead, aTimeoutMS, aData))
#define IGuestFile_ReadAt(p, aOffset, aToRead, aTimeoutMS, aData) ((p)->lpVtbl->ReadAt(p, aOffset, aToRead, aTimeoutMS, aData))
#define IGuestFile_Seek(p, aOffset, aWhence) ((p)->lpVtbl->Seek(p, aOffset, aWhence))
#define IGuestFile_SetACL(p, aAcl) ((p)->lpVtbl->SetACL(p, aAcl))
#define IGuestFile_Write(p, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->Write(p, aData, aTimeoutMS, aWritten))
#define IGuestFile_WriteAt(p, aOffset, aData, aTimeoutMS, aWritten) ((p)->lpVtbl->WriteAt(p, aOffset, aData, aTimeoutMS, aWritten))
#endif /* VBOX_WITH_GLUE */

interface IGuestFile
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFile_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFile declaration */


/* Start of struct IFsObjInfo declaration */
#define IFSOBJINFO_IID_STR "4047ba30-7006-4966-ae86-94164e5e20eb"
#define IFSOBJINFO_IID { \
    0x4047ba30, 0x7006, 0x4966, \
    { 0xae, 0x86, 0x94, 0x16, 0x4e, 0x5e, 0x20, 0xeb } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IFsObjInfo);
#ifndef VBOX_WITH_GLUE
struct IFsObjInfo_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetAccessTime)(IFsObjInfo *pThis, PRInt64 *accessTime);

    nsresult (*GetAllocatedSize)(IFsObjInfo *pThis, PRInt64 *allocatedSize);

    nsresult (*GetBirthTime)(IFsObjInfo *pThis, PRInt64 *birthTime);

    nsresult (*GetChangeTime)(IFsObjInfo *pThis, PRInt64 *changeTime);

    nsresult (*GetDeviceNumber)(IFsObjInfo *pThis, PRUint32 *deviceNumber);

    nsresult (*GetFileAttributes)(IFsObjInfo *pThis, PRUnichar * *fileAttributes);

    nsresult (*GetGenerationId)(IFsObjInfo *pThis, PRUint32 *generationId);

    nsresult (*GetGID)(IFsObjInfo *pThis, PRUint32 *GID);

    nsresult (*GetGroupName)(IFsObjInfo *pThis, PRUnichar * *groupName);

    nsresult (*GetHardLinks)(IFsObjInfo *pThis, PRUint32 *hardLinks);

    nsresult (*GetModificationTime)(IFsObjInfo *pThis, PRInt64 *modificationTime);

    nsresult (*GetName)(IFsObjInfo *pThis, PRUnichar * *name);

    nsresult (*GetNodeId)(IFsObjInfo *pThis, PRInt64 *nodeId);

    nsresult (*GetNodeIdDevice)(IFsObjInfo *pThis, PRUint32 *nodeIdDevice);

    nsresult (*GetObjectSize)(IFsObjInfo *pThis, PRInt64 *objectSize);

    nsresult (*GetType)(IFsObjInfo *pThis, PRUint32 *type);

    nsresult (*GetUID)(IFsObjInfo *pThis, PRUint32 *UID);

    nsresult (*GetUserFlags)(IFsObjInfo *pThis, PRUint32 *userFlags);

    nsresult (*GetUserName)(IFsObjInfo *pThis, PRUnichar * *userName);

};
#else /* VBOX_WITH_GLUE */
struct IFsObjInfoVtbl
{
    nsresult (*QueryInterface)(IFsObjInfo *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IFsObjInfo *pThis);
    nsrefcnt (*Release)(IFsObjInfo *pThis);
    nsresult (*GetAccessTime)(IFsObjInfo *pThis, PRInt64 *accessTime);

    nsresult (*GetAllocatedSize)(IFsObjInfo *pThis, PRInt64 *allocatedSize);

    nsresult (*GetBirthTime)(IFsObjInfo *pThis, PRInt64 *birthTime);

    nsresult (*GetChangeTime)(IFsObjInfo *pThis, PRInt64 *changeTime);

    nsresult (*GetDeviceNumber)(IFsObjInfo *pThis, PRUint32 *deviceNumber);

    nsresult (*GetFileAttributes)(IFsObjInfo *pThis, PRUnichar * *fileAttributes);

    nsresult (*GetGenerationId)(IFsObjInfo *pThis, PRUint32 *generationId);

    nsresult (*GetGID)(IFsObjInfo *pThis, PRUint32 *GID);

    nsresult (*GetGroupName)(IFsObjInfo *pThis, PRUnichar * *groupName);

    nsresult (*GetHardLinks)(IFsObjInfo *pThis, PRUint32 *hardLinks);

    nsresult (*GetModificationTime)(IFsObjInfo *pThis, PRInt64 *modificationTime);

    nsresult (*GetName)(IFsObjInfo *pThis, PRUnichar * *name);

    nsresult (*GetNodeId)(IFsObjInfo *pThis, PRInt64 *nodeId);

    nsresult (*GetNodeIdDevice)(IFsObjInfo *pThis, PRUint32 *nodeIdDevice);

    nsresult (*GetObjectSize)(IFsObjInfo *pThis, PRInt64 *objectSize);

    nsresult (*GetType)(IFsObjInfo *pThis, PRUint32 *type);

    nsresult (*GetUID)(IFsObjInfo *pThis, PRUint32 *UID);

    nsresult (*GetUserFlags)(IFsObjInfo *pThis, PRUint32 *userFlags);

    nsresult (*GetUserName)(IFsObjInfo *pThis, PRUnichar * *userName);

};
#define IFsObjInfo_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IFsObjInfo_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IFsObjInfo_Release(p) ((p)->lpVtbl->Release(p))
#define IFsObjInfo_get_AccessTime(p, aAccessTime) ((p)->lpVtbl->GetAccessTime(p, aAccessTime))
#define IFsObjInfo_GetAccessTime(p, aAccessTime) ((p)->lpVtbl->GetAccessTime(p, aAccessTime))
#define IFsObjInfo_get_AllocatedSize(p, aAllocatedSize) ((p)->lpVtbl->GetAllocatedSize(p, aAllocatedSize))
#define IFsObjInfo_GetAllocatedSize(p, aAllocatedSize) ((p)->lpVtbl->GetAllocatedSize(p, aAllocatedSize))
#define IFsObjInfo_get_BirthTime(p, aBirthTime) ((p)->lpVtbl->GetBirthTime(p, aBirthTime))
#define IFsObjInfo_GetBirthTime(p, aBirthTime) ((p)->lpVtbl->GetBirthTime(p, aBirthTime))
#define IFsObjInfo_get_ChangeTime(p, aChangeTime) ((p)->lpVtbl->GetChangeTime(p, aChangeTime))
#define IFsObjInfo_GetChangeTime(p, aChangeTime) ((p)->lpVtbl->GetChangeTime(p, aChangeTime))
#define IFsObjInfo_get_DeviceNumber(p, aDeviceNumber) ((p)->lpVtbl->GetDeviceNumber(p, aDeviceNumber))
#define IFsObjInfo_GetDeviceNumber(p, aDeviceNumber) ((p)->lpVtbl->GetDeviceNumber(p, aDeviceNumber))
#define IFsObjInfo_get_FileAttributes(p, aFileAttributes) ((p)->lpVtbl->GetFileAttributes(p, aFileAttributes))
#define IFsObjInfo_GetFileAttributes(p, aFileAttributes) ((p)->lpVtbl->GetFileAttributes(p, aFileAttributes))
#define IFsObjInfo_get_GenerationId(p, aGenerationId) ((p)->lpVtbl->GetGenerationId(p, aGenerationId))
#define IFsObjInfo_GetGenerationId(p, aGenerationId) ((p)->lpVtbl->GetGenerationId(p, aGenerationId))
#define IFsObjInfo_get_GID(p, aGID) ((p)->lpVtbl->GetGID(p, aGID))
#define IFsObjInfo_GetGID(p, aGID) ((p)->lpVtbl->GetGID(p, aGID))
#define IFsObjInfo_get_GroupName(p, aGroupName) ((p)->lpVtbl->GetGroupName(p, aGroupName))
#define IFsObjInfo_GetGroupName(p, aGroupName) ((p)->lpVtbl->GetGroupName(p, aGroupName))
#define IFsObjInfo_get_HardLinks(p, aHardLinks) ((p)->lpVtbl->GetHardLinks(p, aHardLinks))
#define IFsObjInfo_GetHardLinks(p, aHardLinks) ((p)->lpVtbl->GetHardLinks(p, aHardLinks))
#define IFsObjInfo_get_ModificationTime(p, aModificationTime) ((p)->lpVtbl->GetModificationTime(p, aModificationTime))
#define IFsObjInfo_GetModificationTime(p, aModificationTime) ((p)->lpVtbl->GetModificationTime(p, aModificationTime))
#define IFsObjInfo_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IFsObjInfo_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IFsObjInfo_get_NodeId(p, aNodeId) ((p)->lpVtbl->GetNodeId(p, aNodeId))
#define IFsObjInfo_GetNodeId(p, aNodeId) ((p)->lpVtbl->GetNodeId(p, aNodeId))
#define IFsObjInfo_get_NodeIdDevice(p, aNodeIdDevice) ((p)->lpVtbl->GetNodeIdDevice(p, aNodeIdDevice))
#define IFsObjInfo_GetNodeIdDevice(p, aNodeIdDevice) ((p)->lpVtbl->GetNodeIdDevice(p, aNodeIdDevice))
#define IFsObjInfo_get_ObjectSize(p, aObjectSize) ((p)->lpVtbl->GetObjectSize(p, aObjectSize))
#define IFsObjInfo_GetObjectSize(p, aObjectSize) ((p)->lpVtbl->GetObjectSize(p, aObjectSize))
#define IFsObjInfo_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IFsObjInfo_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IFsObjInfo_get_UID(p, aUID) ((p)->lpVtbl->GetUID(p, aUID))
#define IFsObjInfo_GetUID(p, aUID) ((p)->lpVtbl->GetUID(p, aUID))
#define IFsObjInfo_get_UserFlags(p, aUserFlags) ((p)->lpVtbl->GetUserFlags(p, aUserFlags))
#define IFsObjInfo_GetUserFlags(p, aUserFlags) ((p)->lpVtbl->GetUserFlags(p, aUserFlags))
#define IFsObjInfo_get_UserName(p, aUserName) ((p)->lpVtbl->GetUserName(p, aUserName))
#define IFsObjInfo_GetUserName(p, aUserName) ((p)->lpVtbl->GetUserName(p, aUserName))
#endif /* VBOX_WITH_GLUE */

interface IFsObjInfo
{
#ifndef VBOX_WITH_GLUE
    struct IFsObjInfo_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IFsObjInfoVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IFsObjInfo declaration */


/* Start of struct IGuestFsObjInfo declaration */
#define IGUESTFSOBJINFO_IID_STR "d5cf678e-3484-4e4a-ac55-329e15462e18"
#define IGUESTFSOBJINFO_IID { \
    0xd5cf678e, 0x3484, 0x4e4a, \
    { 0xac, 0x55, 0x32, 0x9e, 0x15, 0x46, 0x2e, 0x18 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFsObjInfo);
#ifndef VBOX_WITH_GLUE
struct IGuestFsObjInfo_vtbl
{
    struct IFsObjInfo_vtbl ifsobjinfo;

};
#else /* VBOX_WITH_GLUE */
struct IGuestFsObjInfoVtbl
{
    nsresult (*QueryInterface)(IGuestFsObjInfo *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFsObjInfo *pThis);
    nsrefcnt (*Release)(IGuestFsObjInfo *pThis);
    nsresult (*GetAccessTime)(IGuestFsObjInfo *pThis, PRInt64 *accessTime);

    nsresult (*GetAllocatedSize)(IGuestFsObjInfo *pThis, PRInt64 *allocatedSize);

    nsresult (*GetBirthTime)(IGuestFsObjInfo *pThis, PRInt64 *birthTime);

    nsresult (*GetChangeTime)(IGuestFsObjInfo *pThis, PRInt64 *changeTime);

    nsresult (*GetDeviceNumber)(IGuestFsObjInfo *pThis, PRUint32 *deviceNumber);

    nsresult (*GetFileAttributes)(IGuestFsObjInfo *pThis, PRUnichar * *fileAttributes);

    nsresult (*GetGenerationId)(IGuestFsObjInfo *pThis, PRUint32 *generationId);

    nsresult (*GetGID)(IGuestFsObjInfo *pThis, PRUint32 *GID);

    nsresult (*GetGroupName)(IGuestFsObjInfo *pThis, PRUnichar * *groupName);

    nsresult (*GetHardLinks)(IGuestFsObjInfo *pThis, PRUint32 *hardLinks);

    nsresult (*GetModificationTime)(IGuestFsObjInfo *pThis, PRInt64 *modificationTime);

    nsresult (*GetName)(IGuestFsObjInfo *pThis, PRUnichar * *name);

    nsresult (*GetNodeId)(IGuestFsObjInfo *pThis, PRInt64 *nodeId);

    nsresult (*GetNodeIdDevice)(IGuestFsObjInfo *pThis, PRUint32 *nodeIdDevice);

    nsresult (*GetObjectSize)(IGuestFsObjInfo *pThis, PRInt64 *objectSize);

    nsresult (*GetType)(IGuestFsObjInfo *pThis, PRUint32 *type);

    nsresult (*GetUID)(IGuestFsObjInfo *pThis, PRUint32 *UID);

    nsresult (*GetUserFlags)(IGuestFsObjInfo *pThis, PRUint32 *userFlags);

    nsresult (*GetUserName)(IGuestFsObjInfo *pThis, PRUnichar * *userName);

};
#define IGuestFsObjInfo_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFsObjInfo_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFsObjInfo_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFsObjInfo_get_AccessTime(p, aAccessTime) ((p)->lpVtbl->GetAccessTime(p, aAccessTime))
#define IGuestFsObjInfo_GetAccessTime(p, aAccessTime) ((p)->lpVtbl->GetAccessTime(p, aAccessTime))
#define IGuestFsObjInfo_get_AllocatedSize(p, aAllocatedSize) ((p)->lpVtbl->GetAllocatedSize(p, aAllocatedSize))
#define IGuestFsObjInfo_GetAllocatedSize(p, aAllocatedSize) ((p)->lpVtbl->GetAllocatedSize(p, aAllocatedSize))
#define IGuestFsObjInfo_get_BirthTime(p, aBirthTime) ((p)->lpVtbl->GetBirthTime(p, aBirthTime))
#define IGuestFsObjInfo_GetBirthTime(p, aBirthTime) ((p)->lpVtbl->GetBirthTime(p, aBirthTime))
#define IGuestFsObjInfo_get_ChangeTime(p, aChangeTime) ((p)->lpVtbl->GetChangeTime(p, aChangeTime))
#define IGuestFsObjInfo_GetChangeTime(p, aChangeTime) ((p)->lpVtbl->GetChangeTime(p, aChangeTime))
#define IGuestFsObjInfo_get_DeviceNumber(p, aDeviceNumber) ((p)->lpVtbl->GetDeviceNumber(p, aDeviceNumber))
#define IGuestFsObjInfo_GetDeviceNumber(p, aDeviceNumber) ((p)->lpVtbl->GetDeviceNumber(p, aDeviceNumber))
#define IGuestFsObjInfo_get_FileAttributes(p, aFileAttributes) ((p)->lpVtbl->GetFileAttributes(p, aFileAttributes))
#define IGuestFsObjInfo_GetFileAttributes(p, aFileAttributes) ((p)->lpVtbl->GetFileAttributes(p, aFileAttributes))
#define IGuestFsObjInfo_get_GenerationId(p, aGenerationId) ((p)->lpVtbl->GetGenerationId(p, aGenerationId))
#define IGuestFsObjInfo_GetGenerationId(p, aGenerationId) ((p)->lpVtbl->GetGenerationId(p, aGenerationId))
#define IGuestFsObjInfo_get_GID(p, aGID) ((p)->lpVtbl->GetGID(p, aGID))
#define IGuestFsObjInfo_GetGID(p, aGID) ((p)->lpVtbl->GetGID(p, aGID))
#define IGuestFsObjInfo_get_GroupName(p, aGroupName) ((p)->lpVtbl->GetGroupName(p, aGroupName))
#define IGuestFsObjInfo_GetGroupName(p, aGroupName) ((p)->lpVtbl->GetGroupName(p, aGroupName))
#define IGuestFsObjInfo_get_HardLinks(p, aHardLinks) ((p)->lpVtbl->GetHardLinks(p, aHardLinks))
#define IGuestFsObjInfo_GetHardLinks(p, aHardLinks) ((p)->lpVtbl->GetHardLinks(p, aHardLinks))
#define IGuestFsObjInfo_get_ModificationTime(p, aModificationTime) ((p)->lpVtbl->GetModificationTime(p, aModificationTime))
#define IGuestFsObjInfo_GetModificationTime(p, aModificationTime) ((p)->lpVtbl->GetModificationTime(p, aModificationTime))
#define IGuestFsObjInfo_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestFsObjInfo_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestFsObjInfo_get_NodeId(p, aNodeId) ((p)->lpVtbl->GetNodeId(p, aNodeId))
#define IGuestFsObjInfo_GetNodeId(p, aNodeId) ((p)->lpVtbl->GetNodeId(p, aNodeId))
#define IGuestFsObjInfo_get_NodeIdDevice(p, aNodeIdDevice) ((p)->lpVtbl->GetNodeIdDevice(p, aNodeIdDevice))
#define IGuestFsObjInfo_GetNodeIdDevice(p, aNodeIdDevice) ((p)->lpVtbl->GetNodeIdDevice(p, aNodeIdDevice))
#define IGuestFsObjInfo_get_ObjectSize(p, aObjectSize) ((p)->lpVtbl->GetObjectSize(p, aObjectSize))
#define IGuestFsObjInfo_GetObjectSize(p, aObjectSize) ((p)->lpVtbl->GetObjectSize(p, aObjectSize))
#define IGuestFsObjInfo_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFsObjInfo_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFsObjInfo_get_UID(p, aUID) ((p)->lpVtbl->GetUID(p, aUID))
#define IGuestFsObjInfo_GetUID(p, aUID) ((p)->lpVtbl->GetUID(p, aUID))
#define IGuestFsObjInfo_get_UserFlags(p, aUserFlags) ((p)->lpVtbl->GetUserFlags(p, aUserFlags))
#define IGuestFsObjInfo_GetUserFlags(p, aUserFlags) ((p)->lpVtbl->GetUserFlags(p, aUserFlags))
#define IGuestFsObjInfo_get_UserName(p, aUserName) ((p)->lpVtbl->GetUserName(p, aUserName))
#define IGuestFsObjInfo_GetUserName(p, aUserName) ((p)->lpVtbl->GetUserName(p, aUserName))
#endif /* VBOX_WITH_GLUE */

interface IGuestFsObjInfo
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFsObjInfo_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFsObjInfoVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFsObjInfo declaration */


/* Start of struct IGuest declaration */
#define IGUEST_IID_STR "8011a1b1-6adb-4ffb-a37e-20abdaee4650"
#define IGUEST_IID { \
    0x8011a1b1, 0x6adb, 0x4ffb, \
    { 0xa3, 0x7e, 0x20, 0xab, 0xda, 0xee, 0x46, 0x50 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuest);
#ifndef VBOX_WITH_GLUE
struct IGuest_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetOSTypeId)(IGuest *pThis, PRUnichar * *OSTypeId);

    nsresult (*GetAdditionsRunLevel)(IGuest *pThis, PRUint32 *additionsRunLevel);

    nsresult (*GetAdditionsVersion)(IGuest *pThis, PRUnichar * *additionsVersion);

    nsresult (*GetAdditionsRevision)(IGuest *pThis, PRUint32 *additionsRevision);

    nsresult (*GetEventSource)(IGuest *pThis, IEventSource * *eventSource);

    nsresult (*GetFacilities)(IGuest *pThis, PRUint32 *facilitiesSize, IAdditionsFacility * **facilities);

    nsresult (*GetSessions)(IGuest *pThis, PRUint32 *sessionsSize, IGuestSession * **sessions);

    nsresult (*GetMemoryBalloonSize)(IGuest *pThis, PRUint32 *memoryBalloonSize);
    nsresult (*SetMemoryBalloonSize)(IGuest *pThis, PRUint32 memoryBalloonSize);

    nsresult (*GetStatisticsUpdateInterval)(IGuest *pThis, PRUint32 *statisticsUpdateInterval);
    nsresult (*SetStatisticsUpdateInterval)(IGuest *pThis, PRUint32 statisticsUpdateInterval);

    nsresult (*InternalGetStatistics)(
        IGuest *pThis,
        PRUint32 * cpuUser,
        PRUint32 * cpuKernel,
        PRUint32 * cpuIdle,
        PRUint32 * memTotal,
        PRUint32 * memFree,
        PRUint32 * memBalloon,
        PRUint32 * memShared,
        PRUint32 * memCache,
        PRUint32 * pagedTotal,
        PRUint32 * memAllocTotal,
        PRUint32 * memFreeTotal,
        PRUint32 * memBalloonTotal,
        PRUint32 * memSharedTotal
    );

    nsresult (*GetFacilityStatus)(
        IGuest *pThis,
        PRUint32 facility,
        PRInt64 * timestamp,
        PRUint32 * status
    );

    nsresult (*GetAdditionsStatus)(
        IGuest *pThis,
        PRUint32 level,
        PRBool * active
    );

    nsresult (*SetCredentials)(
        IGuest *pThis,
        PRUnichar * userName,
        PRUnichar * password,
        PRUnichar * domain,
        PRBool allowInteractiveLogon
    );

    nsresult (*DragHGEnter)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 y,
        PRUint32 x,
        PRUint32 defaultAction,
        PRUint32 allowedActionsSize,
        PRUint32* allowedActions,
        PRUint32 formatsSize,
        PRUnichar ** formats,
        PRUint32 * resultAction
    );

    nsresult (*DragHGMove)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 x,
        PRUint32 y,
        PRUint32 defaultAction,
        PRUint32 allowedActionsSize,
        PRUint32* allowedActions,
        PRUint32 formatsSize,
        PRUnichar ** formats,
        PRUint32 * resultAction
    );

    nsresult (*DragHGLeave)(
        IGuest *pThis,
        PRUint32 screenId
    );

    nsresult (*DragHGDrop)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 x,
        PRUint32 y,
        PRUint32 defaultAction,
        PRUint32 allowedActionsSize,
        PRUint32* allowedActions,
        PRUint32 formatsSize,
        PRUnichar ** formats,
        PRUnichar * * format,
        PRUint32 * resultAction
    );

    nsresult (*DragHGPutData)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUnichar * format,
        PRUint32 dataSize,
        PRUint8* data,
        IProgress * * progress
    );

    nsresult (*DragGHPending)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 *formatsSize,
        PRUnichar *** formats,
        PRUint32 *allowedActionsSize,
        PRUint32** allowedActions,
        PRUint32 * defaultAction
    );

    nsresult (*DragGHDropped)(
        IGuest *pThis,
        PRUnichar * format,
        PRUint32 action,
        IProgress * * progress
    );

    nsresult (*DragGHGetData)(
        IGuest *pThis,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*CreateSession)(
        IGuest *pThis,
        PRUnichar * user,
        PRUnichar * password,
        PRUnichar * domain,
        PRUnichar * sessionName,
        IGuestSession * * guestSession
    );

    nsresult (*FindSession)(
        IGuest *pThis,
        PRUnichar * sessionName,
        PRUint32 *sessionsSize,
        IGuestSession *** sessions
    );

    nsresult (*UpdateGuestAdditions)(
        IGuest *pThis,
        PRUnichar * source,
        PRUint32 argumentsSize,
        PRUnichar ** arguments,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

};
#else /* VBOX_WITH_GLUE */
struct IGuestVtbl
{
    nsresult (*QueryInterface)(IGuest *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuest *pThis);
    nsrefcnt (*Release)(IGuest *pThis);
    nsresult (*GetOSTypeId)(IGuest *pThis, PRUnichar * *OSTypeId);

    nsresult (*GetAdditionsRunLevel)(IGuest *pThis, PRUint32 *additionsRunLevel);

    nsresult (*GetAdditionsVersion)(IGuest *pThis, PRUnichar * *additionsVersion);

    nsresult (*GetAdditionsRevision)(IGuest *pThis, PRUint32 *additionsRevision);

    nsresult (*GetEventSource)(IGuest *pThis, IEventSource * *eventSource);

    nsresult (*GetFacilities)(IGuest *pThis, PRUint32 *facilitiesSize, IAdditionsFacility * **facilities);

    nsresult (*GetSessions)(IGuest *pThis, PRUint32 *sessionsSize, IGuestSession * **sessions);

    nsresult (*GetMemoryBalloonSize)(IGuest *pThis, PRUint32 *memoryBalloonSize);
    nsresult (*SetMemoryBalloonSize)(IGuest *pThis, PRUint32 memoryBalloonSize);

    nsresult (*GetStatisticsUpdateInterval)(IGuest *pThis, PRUint32 *statisticsUpdateInterval);
    nsresult (*SetStatisticsUpdateInterval)(IGuest *pThis, PRUint32 statisticsUpdateInterval);

    nsresult (*InternalGetStatistics)(
        IGuest *pThis,
        PRUint32 * cpuUser,
        PRUint32 * cpuKernel,
        PRUint32 * cpuIdle,
        PRUint32 * memTotal,
        PRUint32 * memFree,
        PRUint32 * memBalloon,
        PRUint32 * memShared,
        PRUint32 * memCache,
        PRUint32 * pagedTotal,
        PRUint32 * memAllocTotal,
        PRUint32 * memFreeTotal,
        PRUint32 * memBalloonTotal,
        PRUint32 * memSharedTotal
    );

    nsresult (*GetFacilityStatus)(
        IGuest *pThis,
        PRUint32 facility,
        PRInt64 * timestamp,
        PRUint32 * status
    );

    nsresult (*GetAdditionsStatus)(
        IGuest *pThis,
        PRUint32 level,
        PRBool * active
    );

    nsresult (*SetCredentials)(
        IGuest *pThis,
        PRUnichar * userName,
        PRUnichar * password,
        PRUnichar * domain,
        PRBool allowInteractiveLogon
    );

    nsresult (*DragHGEnter)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 y,
        PRUint32 x,
        PRUint32 defaultAction,
        PRUint32 allowedActionsSize,
        PRUint32* allowedActions,
        PRUint32 formatsSize,
        PRUnichar ** formats,
        PRUint32 * resultAction
    );

    nsresult (*DragHGMove)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 x,
        PRUint32 y,
        PRUint32 defaultAction,
        PRUint32 allowedActionsSize,
        PRUint32* allowedActions,
        PRUint32 formatsSize,
        PRUnichar ** formats,
        PRUint32 * resultAction
    );

    nsresult (*DragHGLeave)(
        IGuest *pThis,
        PRUint32 screenId
    );

    nsresult (*DragHGDrop)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 x,
        PRUint32 y,
        PRUint32 defaultAction,
        PRUint32 allowedActionsSize,
        PRUint32* allowedActions,
        PRUint32 formatsSize,
        PRUnichar ** formats,
        PRUnichar * * format,
        PRUint32 * resultAction
    );

    nsresult (*DragHGPutData)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUnichar * format,
        PRUint32 dataSize,
        PRUint8* data,
        IProgress * * progress
    );

    nsresult (*DragGHPending)(
        IGuest *pThis,
        PRUint32 screenId,
        PRUint32 *formatsSize,
        PRUnichar *** formats,
        PRUint32 *allowedActionsSize,
        PRUint32** allowedActions,
        PRUint32 * defaultAction
    );

    nsresult (*DragGHDropped)(
        IGuest *pThis,
        PRUnichar * format,
        PRUint32 action,
        IProgress * * progress
    );

    nsresult (*DragGHGetData)(
        IGuest *pThis,
        PRUint32 *dataSize,
        PRUint8** data
    );

    nsresult (*CreateSession)(
        IGuest *pThis,
        PRUnichar * user,
        PRUnichar * password,
        PRUnichar * domain,
        PRUnichar * sessionName,
        IGuestSession * * guestSession
    );

    nsresult (*FindSession)(
        IGuest *pThis,
        PRUnichar * sessionName,
        PRUint32 *sessionsSize,
        IGuestSession *** sessions
    );

    nsresult (*UpdateGuestAdditions)(
        IGuest *pThis,
        PRUnichar * source,
        PRUint32 argumentsSize,
        PRUnichar ** arguments,
        PRUint32 flagsSize,
        PRUint32* flags,
        IProgress * * progress
    );

};
#define IGuest_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuest_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuest_Release(p) ((p)->lpVtbl->Release(p))
#define IGuest_get_OSTypeId(p, aOSTypeId) ((p)->lpVtbl->GetOSTypeId(p, aOSTypeId))
#define IGuest_GetOSTypeId(p, aOSTypeId) ((p)->lpVtbl->GetOSTypeId(p, aOSTypeId))
#define IGuest_get_AdditionsRunLevel(p, aAdditionsRunLevel) ((p)->lpVtbl->GetAdditionsRunLevel(p, aAdditionsRunLevel))
#define IGuest_GetAdditionsRunLevel(p, aAdditionsRunLevel) ((p)->lpVtbl->GetAdditionsRunLevel(p, aAdditionsRunLevel))
#define IGuest_get_AdditionsVersion(p, aAdditionsVersion) ((p)->lpVtbl->GetAdditionsVersion(p, aAdditionsVersion))
#define IGuest_GetAdditionsVersion(p, aAdditionsVersion) ((p)->lpVtbl->GetAdditionsVersion(p, aAdditionsVersion))
#define IGuest_get_AdditionsRevision(p, aAdditionsRevision) ((p)->lpVtbl->GetAdditionsRevision(p, aAdditionsRevision))
#define IGuest_GetAdditionsRevision(p, aAdditionsRevision) ((p)->lpVtbl->GetAdditionsRevision(p, aAdditionsRevision))
#define IGuest_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuest_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IGuest_get_Facilities(p, aFacilities) ((p)->lpVtbl->GetFacilities(p, aFacilities))
#define IGuest_GetFacilities(p, aFacilities) ((p)->lpVtbl->GetFacilities(p, aFacilities))
#define IGuest_get_Sessions(p, aSessions) ((p)->lpVtbl->GetSessions(p, aSessions))
#define IGuest_GetSessions(p, aSessions) ((p)->lpVtbl->GetSessions(p, aSessions))
#define IGuest_get_MemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->GetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IGuest_GetMemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->GetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IGuest_put_MemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->SetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IGuest_SetMemoryBalloonSize(p, aMemoryBalloonSize) ((p)->lpVtbl->SetMemoryBalloonSize(p, aMemoryBalloonSize))
#define IGuest_get_StatisticsUpdateInterval(p, aStatisticsUpdateInterval) ((p)->lpVtbl->GetStatisticsUpdateInterval(p, aStatisticsUpdateInterval))
#define IGuest_GetStatisticsUpdateInterval(p, aStatisticsUpdateInterval) ((p)->lpVtbl->GetStatisticsUpdateInterval(p, aStatisticsUpdateInterval))
#define IGuest_put_StatisticsUpdateInterval(p, aStatisticsUpdateInterval) ((p)->lpVtbl->SetStatisticsUpdateInterval(p, aStatisticsUpdateInterval))
#define IGuest_SetStatisticsUpdateInterval(p, aStatisticsUpdateInterval) ((p)->lpVtbl->SetStatisticsUpdateInterval(p, aStatisticsUpdateInterval))
#define IGuest_InternalGetStatistics(p, aCpuUser, aCpuKernel, aCpuIdle, aMemTotal, aMemFree, aMemBalloon, aMemShared, aMemCache, aPagedTotal, aMemAllocTotal, aMemFreeTotal, aMemBalloonTotal, aMemSharedTotal) ((p)->lpVtbl->InternalGetStatistics(p, aCpuUser, aCpuKernel, aCpuIdle, aMemTotal, aMemFree, aMemBalloon, aMemShared, aMemCache, aPagedTotal, aMemAllocTotal, aMemFreeTotal, aMemBalloonTotal, aMemSharedTotal))
#define IGuest_GetFacilityStatus(p, aFacility, aTimestamp, aStatus) ((p)->lpVtbl->GetFacilityStatus(p, aFacility, aTimestamp, aStatus))
#define IGuest_GetAdditionsStatus(p, aLevel, aActive) ((p)->lpVtbl->GetAdditionsStatus(p, aLevel, aActive))
#define IGuest_SetCredentials(p, aUserName, aPassword, aDomain, aAllowInteractiveLogon) ((p)->lpVtbl->SetCredentials(p, aUserName, aPassword, aDomain, aAllowInteractiveLogon))
#define IGuest_DragHGEnter(p, aScreenId, aY, aX, aDefaultAction, aAllowedActions, aFormats, aResultAction) ((p)->lpVtbl->DragHGEnter(p, aScreenId, aY, aX, aDefaultAction, aAllowedActions, aFormats, aResultAction))
#define IGuest_DragHGMove(p, aScreenId, aX, aY, aDefaultAction, aAllowedActions, aFormats, aResultAction) ((p)->lpVtbl->DragHGMove(p, aScreenId, aX, aY, aDefaultAction, aAllowedActions, aFormats, aResultAction))
#define IGuest_DragHGLeave(p, aScreenId) ((p)->lpVtbl->DragHGLeave(p, aScreenId))
#define IGuest_DragHGDrop(p, aScreenId, aX, aY, aDefaultAction, aAllowedActions, aFormats, aFormat, aResultAction) ((p)->lpVtbl->DragHGDrop(p, aScreenId, aX, aY, aDefaultAction, aAllowedActions, aFormats, aFormat, aResultAction))
#define IGuest_DragHGPutData(p, aScreenId, aFormat, aData, aProgress) ((p)->lpVtbl->DragHGPutData(p, aScreenId, aFormat, aData, aProgress))
#define IGuest_DragGHPending(p, aScreenId, aFormats, aAllowedActions, aDefaultAction) ((p)->lpVtbl->DragGHPending(p, aScreenId, aFormats, aAllowedActions, aDefaultAction))
#define IGuest_DragGHDropped(p, aFormat, aAction, aProgress) ((p)->lpVtbl->DragGHDropped(p, aFormat, aAction, aProgress))
#define IGuest_DragGHGetData(p, aData) ((p)->lpVtbl->DragGHGetData(p, aData))
#define IGuest_CreateSession(p, aUser, aPassword, aDomain, aSessionName, aGuestSession) ((p)->lpVtbl->CreateSession(p, aUser, aPassword, aDomain, aSessionName, aGuestSession))
#define IGuest_FindSession(p, aSessionName, aSessions) ((p)->lpVtbl->FindSession(p, aSessionName, aSessions))
#define IGuest_UpdateGuestAdditions(p, aSource, aArguments, aFlags, aProgress) ((p)->lpVtbl->UpdateGuestAdditions(p, aSource, aArguments, aFlags, aProgress))
#endif /* VBOX_WITH_GLUE */

interface IGuest
{
#ifndef VBOX_WITH_GLUE
    struct IGuest_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuest declaration */


/* Start of struct IProgress declaration */
#define IPROGRESS_IID_STR "c20238e4-3221-4d3f-8891-81ce92d9f913"
#define IPROGRESS_IID { \
    0xc20238e4, 0x3221, 0x4d3f, \
    { 0x88, 0x91, 0x81, 0xce, 0x92, 0xd9, 0xf9, 0x13 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IProgress);
#ifndef VBOX_WITH_GLUE
struct IProgress_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetId)(IProgress *pThis, PRUnichar * *id);

    nsresult (*GetDescription)(IProgress *pThis, PRUnichar * *description);

    nsresult (*GetInitiator)(IProgress *pThis, nsISupports * *initiator);

    nsresult (*GetCancelable)(IProgress *pThis, PRBool *cancelable);

    nsresult (*GetPercent)(IProgress *pThis, PRUint32 *percent);

    nsresult (*GetTimeRemaining)(IProgress *pThis, PRInt32 *timeRemaining);

    nsresult (*GetCompleted)(IProgress *pThis, PRBool *completed);

    nsresult (*GetCanceled)(IProgress *pThis, PRBool *canceled);

    nsresult (*GetResultCode)(IProgress *pThis, PRInt32 *resultCode);

    nsresult (*GetErrorInfo)(IProgress *pThis, IVirtualBoxErrorInfo * *errorInfo);

    nsresult (*GetOperationCount)(IProgress *pThis, PRUint32 *operationCount);

    nsresult (*GetOperation)(IProgress *pThis, PRUint32 *operation);

    nsresult (*GetOperationDescription)(IProgress *pThis, PRUnichar * *operationDescription);

    nsresult (*GetOperationPercent)(IProgress *pThis, PRUint32 *operationPercent);

    nsresult (*GetOperationWeight)(IProgress *pThis, PRUint32 *operationWeight);

    nsresult (*GetTimeout)(IProgress *pThis, PRUint32 *timeout);
    nsresult (*SetTimeout)(IProgress *pThis, PRUint32 timeout);

    nsresult (*SetCurrentOperationProgress)(
        IProgress *pThis,
        PRUint32 percent
    );

    nsresult (*SetNextOperation)(
        IProgress *pThis,
        PRUnichar * nextOperationDescription,
        PRUint32 nextOperationsWeight
    );

    nsresult (*WaitForCompletion)(
        IProgress *pThis,
        PRInt32 timeout
    );

    nsresult (*WaitForOperationCompletion)(
        IProgress *pThis,
        PRUint32 operation,
        PRInt32 timeout
    );

    nsresult (*WaitForAsyncProgressCompletion)(
        IProgress *pThis,
        IProgress * pProgressAsync
    );

    nsresult (*Cancel)(IProgress *pThis );

};
#else /* VBOX_WITH_GLUE */
struct IProgressVtbl
{
    nsresult (*QueryInterface)(IProgress *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IProgress *pThis);
    nsrefcnt (*Release)(IProgress *pThis);
    nsresult (*GetId)(IProgress *pThis, PRUnichar * *id);

    nsresult (*GetDescription)(IProgress *pThis, PRUnichar * *description);

    nsresult (*GetInitiator)(IProgress *pThis, nsISupports * *initiator);

    nsresult (*GetCancelable)(IProgress *pThis, PRBool *cancelable);

    nsresult (*GetPercent)(IProgress *pThis, PRUint32 *percent);

    nsresult (*GetTimeRemaining)(IProgress *pThis, PRInt32 *timeRemaining);

    nsresult (*GetCompleted)(IProgress *pThis, PRBool *completed);

    nsresult (*GetCanceled)(IProgress *pThis, PRBool *canceled);

    nsresult (*GetResultCode)(IProgress *pThis, PRInt32 *resultCode);

    nsresult (*GetErrorInfo)(IProgress *pThis, IVirtualBoxErrorInfo * *errorInfo);

    nsresult (*GetOperationCount)(IProgress *pThis, PRUint32 *operationCount);

    nsresult (*GetOperation)(IProgress *pThis, PRUint32 *operation);

    nsresult (*GetOperationDescription)(IProgress *pThis, PRUnichar * *operationDescription);

    nsresult (*GetOperationPercent)(IProgress *pThis, PRUint32 *operationPercent);

    nsresult (*GetOperationWeight)(IProgress *pThis, PRUint32 *operationWeight);

    nsresult (*GetTimeout)(IProgress *pThis, PRUint32 *timeout);
    nsresult (*SetTimeout)(IProgress *pThis, PRUint32 timeout);

    nsresult (*SetCurrentOperationProgress)(
        IProgress *pThis,
        PRUint32 percent
    );

    nsresult (*SetNextOperation)(
        IProgress *pThis,
        PRUnichar * nextOperationDescription,
        PRUint32 nextOperationsWeight
    );

    nsresult (*WaitForCompletion)(
        IProgress *pThis,
        PRInt32 timeout
    );

    nsresult (*WaitForOperationCompletion)(
        IProgress *pThis,
        PRUint32 operation,
        PRInt32 timeout
    );

    nsresult (*WaitForAsyncProgressCompletion)(
        IProgress *pThis,
        IProgress * pProgressAsync
    );

    nsresult (*Cancel)(IProgress *pThis );

};
#define IProgress_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IProgress_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IProgress_Release(p) ((p)->lpVtbl->Release(p))
#define IProgress_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IProgress_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IProgress_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IProgress_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IProgress_get_Initiator(p, aInitiator) ((p)->lpVtbl->GetInitiator(p, aInitiator))
#define IProgress_GetInitiator(p, aInitiator) ((p)->lpVtbl->GetInitiator(p, aInitiator))
#define IProgress_get_Cancelable(p, aCancelable) ((p)->lpVtbl->GetCancelable(p, aCancelable))
#define IProgress_GetCancelable(p, aCancelable) ((p)->lpVtbl->GetCancelable(p, aCancelable))
#define IProgress_get_Percent(p, aPercent) ((p)->lpVtbl->GetPercent(p, aPercent))
#define IProgress_GetPercent(p, aPercent) ((p)->lpVtbl->GetPercent(p, aPercent))
#define IProgress_get_TimeRemaining(p, aTimeRemaining) ((p)->lpVtbl->GetTimeRemaining(p, aTimeRemaining))
#define IProgress_GetTimeRemaining(p, aTimeRemaining) ((p)->lpVtbl->GetTimeRemaining(p, aTimeRemaining))
#define IProgress_get_Completed(p, aCompleted) ((p)->lpVtbl->GetCompleted(p, aCompleted))
#define IProgress_GetCompleted(p, aCompleted) ((p)->lpVtbl->GetCompleted(p, aCompleted))
#define IProgress_get_Canceled(p, aCanceled) ((p)->lpVtbl->GetCanceled(p, aCanceled))
#define IProgress_GetCanceled(p, aCanceled) ((p)->lpVtbl->GetCanceled(p, aCanceled))
#define IProgress_get_ResultCode(p, aResultCode) ((p)->lpVtbl->GetResultCode(p, aResultCode))
#define IProgress_GetResultCode(p, aResultCode) ((p)->lpVtbl->GetResultCode(p, aResultCode))
#define IProgress_get_ErrorInfo(p, aErrorInfo) ((p)->lpVtbl->GetErrorInfo(p, aErrorInfo))
#define IProgress_GetErrorInfo(p, aErrorInfo) ((p)->lpVtbl->GetErrorInfo(p, aErrorInfo))
#define IProgress_get_OperationCount(p, aOperationCount) ((p)->lpVtbl->GetOperationCount(p, aOperationCount))
#define IProgress_GetOperationCount(p, aOperationCount) ((p)->lpVtbl->GetOperationCount(p, aOperationCount))
#define IProgress_get_Operation(p, aOperation) ((p)->lpVtbl->GetOperation(p, aOperation))
#define IProgress_GetOperation(p, aOperation) ((p)->lpVtbl->GetOperation(p, aOperation))
#define IProgress_get_OperationDescription(p, aOperationDescription) ((p)->lpVtbl->GetOperationDescription(p, aOperationDescription))
#define IProgress_GetOperationDescription(p, aOperationDescription) ((p)->lpVtbl->GetOperationDescription(p, aOperationDescription))
#define IProgress_get_OperationPercent(p, aOperationPercent) ((p)->lpVtbl->GetOperationPercent(p, aOperationPercent))
#define IProgress_GetOperationPercent(p, aOperationPercent) ((p)->lpVtbl->GetOperationPercent(p, aOperationPercent))
#define IProgress_get_OperationWeight(p, aOperationWeight) ((p)->lpVtbl->GetOperationWeight(p, aOperationWeight))
#define IProgress_GetOperationWeight(p, aOperationWeight) ((p)->lpVtbl->GetOperationWeight(p, aOperationWeight))
#define IProgress_get_Timeout(p, aTimeout) ((p)->lpVtbl->GetTimeout(p, aTimeout))
#define IProgress_GetTimeout(p, aTimeout) ((p)->lpVtbl->GetTimeout(p, aTimeout))
#define IProgress_put_Timeout(p, aTimeout) ((p)->lpVtbl->SetTimeout(p, aTimeout))
#define IProgress_SetTimeout(p, aTimeout) ((p)->lpVtbl->SetTimeout(p, aTimeout))
#define IProgress_SetCurrentOperationProgress(p, aPercent) ((p)->lpVtbl->SetCurrentOperationProgress(p, aPercent))
#define IProgress_SetNextOperation(p, aNextOperationDescription, aNextOperationsWeight) ((p)->lpVtbl->SetNextOperation(p, aNextOperationDescription, aNextOperationsWeight))
#define IProgress_WaitForCompletion(p, aTimeout) ((p)->lpVtbl->WaitForCompletion(p, aTimeout))
#define IProgress_WaitForOperationCompletion(p, aOperation, aTimeout) ((p)->lpVtbl->WaitForOperationCompletion(p, aOperation, aTimeout))
#define IProgress_WaitForAsyncProgressCompletion(p, aPProgressAsync) ((p)->lpVtbl->WaitForAsyncProgressCompletion(p, aPProgressAsync))
#define IProgress_Cancel(p) ((p)->lpVtbl->Cancel(p))
#endif /* VBOX_WITH_GLUE */

interface IProgress
{
#ifndef VBOX_WITH_GLUE
    struct IProgress_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IProgressVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IProgress declaration */


/* Start of struct ISnapshot declaration */
#define ISNAPSHOT_IID_STR "0472823b-c6e7-472a-8e9f-d732e86b8463"
#define ISNAPSHOT_IID { \
    0x0472823b, 0xc6e7, 0x472a, \
    { 0x8e, 0x9f, 0xd7, 0x32, 0xe8, 0x6b, 0x84, 0x63 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISnapshot);
#ifndef VBOX_WITH_GLUE
struct ISnapshot_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetId)(ISnapshot *pThis, PRUnichar * *id);

    nsresult (*GetName)(ISnapshot *pThis, PRUnichar * *name);
    nsresult (*SetName)(ISnapshot *pThis, PRUnichar * name);

    nsresult (*GetDescription)(ISnapshot *pThis, PRUnichar * *description);
    nsresult (*SetDescription)(ISnapshot *pThis, PRUnichar * description);

    nsresult (*GetTimeStamp)(ISnapshot *pThis, PRInt64 *timeStamp);

    nsresult (*GetOnline)(ISnapshot *pThis, PRBool *online);

    nsresult (*GetMachine)(ISnapshot *pThis, IMachine * *machine);

    nsresult (*GetParent)(ISnapshot *pThis, ISnapshot * *parent);

    nsresult (*GetChildren)(ISnapshot *pThis, PRUint32 *childrenSize, ISnapshot * **children);

    nsresult (*GetChildrenCount)(
        ISnapshot *pThis,
        PRUint32 * childrenCount
    );

};
#else /* VBOX_WITH_GLUE */
struct ISnapshotVtbl
{
    nsresult (*QueryInterface)(ISnapshot *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISnapshot *pThis);
    nsrefcnt (*Release)(ISnapshot *pThis);
    nsresult (*GetId)(ISnapshot *pThis, PRUnichar * *id);

    nsresult (*GetName)(ISnapshot *pThis, PRUnichar * *name);
    nsresult (*SetName)(ISnapshot *pThis, PRUnichar * name);

    nsresult (*GetDescription)(ISnapshot *pThis, PRUnichar * *description);
    nsresult (*SetDescription)(ISnapshot *pThis, PRUnichar * description);

    nsresult (*GetTimeStamp)(ISnapshot *pThis, PRInt64 *timeStamp);

    nsresult (*GetOnline)(ISnapshot *pThis, PRBool *online);

    nsresult (*GetMachine)(ISnapshot *pThis, IMachine * *machine);

    nsresult (*GetParent)(ISnapshot *pThis, ISnapshot * *parent);

    nsresult (*GetChildren)(ISnapshot *pThis, PRUint32 *childrenSize, ISnapshot * **children);

    nsresult (*GetChildrenCount)(
        ISnapshot *pThis,
        PRUint32 * childrenCount
    );

};
#define ISnapshot_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISnapshot_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISnapshot_Release(p) ((p)->lpVtbl->Release(p))
#define ISnapshot_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define ISnapshot_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define ISnapshot_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define ISnapshot_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define ISnapshot_put_Name(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define ISnapshot_SetName(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define ISnapshot_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define ISnapshot_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define ISnapshot_put_Description(p, aDescription) ((p)->lpVtbl->SetDescription(p, aDescription))
#define ISnapshot_SetDescription(p, aDescription) ((p)->lpVtbl->SetDescription(p, aDescription))
#define ISnapshot_get_TimeStamp(p, aTimeStamp) ((p)->lpVtbl->GetTimeStamp(p, aTimeStamp))
#define ISnapshot_GetTimeStamp(p, aTimeStamp) ((p)->lpVtbl->GetTimeStamp(p, aTimeStamp))
#define ISnapshot_get_Online(p, aOnline) ((p)->lpVtbl->GetOnline(p, aOnline))
#define ISnapshot_GetOnline(p, aOnline) ((p)->lpVtbl->GetOnline(p, aOnline))
#define ISnapshot_get_Machine(p, aMachine) ((p)->lpVtbl->GetMachine(p, aMachine))
#define ISnapshot_GetMachine(p, aMachine) ((p)->lpVtbl->GetMachine(p, aMachine))
#define ISnapshot_get_Parent(p, aParent) ((p)->lpVtbl->GetParent(p, aParent))
#define ISnapshot_GetParent(p, aParent) ((p)->lpVtbl->GetParent(p, aParent))
#define ISnapshot_get_Children(p, aChildren) ((p)->lpVtbl->GetChildren(p, aChildren))
#define ISnapshot_GetChildren(p, aChildren) ((p)->lpVtbl->GetChildren(p, aChildren))
#define ISnapshot_GetChildrenCount(p, aChildrenCount) ((p)->lpVtbl->GetChildrenCount(p, aChildrenCount))
#endif /* VBOX_WITH_GLUE */

interface ISnapshot
{
#ifndef VBOX_WITH_GLUE
    struct ISnapshot_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISnapshotVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISnapshot declaration */


/* Start of struct IMediumAttachment declaration */
#define IMEDIUMATTACHMENT_IID_STR "4b252567-5d4e-4db8-b3c8-569ec1c9236c"
#define IMEDIUMATTACHMENT_IID { \
    0x4b252567, 0x5d4e, 0x4db8, \
    { 0xb3, 0xc8, 0x56, 0x9e, 0xc1, 0xc9, 0x23, 0x6c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMediumAttachment);
#ifndef VBOX_WITH_GLUE
struct IMediumAttachment_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetMedium)(IMediumAttachment *pThis, IMedium * *medium);

    nsresult (*GetController)(IMediumAttachment *pThis, PRUnichar * *controller);

    nsresult (*GetPort)(IMediumAttachment *pThis, PRInt32 *port);

    nsresult (*GetDevice)(IMediumAttachment *pThis, PRInt32 *device);

    nsresult (*GetType)(IMediumAttachment *pThis, PRUint32 *type);

    nsresult (*GetPassthrough)(IMediumAttachment *pThis, PRBool *passthrough);

    nsresult (*GetTemporaryEject)(IMediumAttachment *pThis, PRBool *temporaryEject);

    nsresult (*GetIsEjected)(IMediumAttachment *pThis, PRBool *isEjected);

    nsresult (*GetNonRotational)(IMediumAttachment *pThis, PRBool *nonRotational);

    nsresult (*GetDiscard)(IMediumAttachment *pThis, PRBool *discard);

    nsresult (*GetHotPluggable)(IMediumAttachment *pThis, PRBool *hotPluggable);

    nsresult (*GetBandwidthGroup)(IMediumAttachment *pThis, IBandwidthGroup * *bandwidthGroup);

};
#else /* VBOX_WITH_GLUE */
struct IMediumAttachmentVtbl
{
    nsresult (*QueryInterface)(IMediumAttachment *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMediumAttachment *pThis);
    nsrefcnt (*Release)(IMediumAttachment *pThis);
    nsresult (*GetMedium)(IMediumAttachment *pThis, IMedium * *medium);

    nsresult (*GetController)(IMediumAttachment *pThis, PRUnichar * *controller);

    nsresult (*GetPort)(IMediumAttachment *pThis, PRInt32 *port);

    nsresult (*GetDevice)(IMediumAttachment *pThis, PRInt32 *device);

    nsresult (*GetType)(IMediumAttachment *pThis, PRUint32 *type);

    nsresult (*GetPassthrough)(IMediumAttachment *pThis, PRBool *passthrough);

    nsresult (*GetTemporaryEject)(IMediumAttachment *pThis, PRBool *temporaryEject);

    nsresult (*GetIsEjected)(IMediumAttachment *pThis, PRBool *isEjected);

    nsresult (*GetNonRotational)(IMediumAttachment *pThis, PRBool *nonRotational);

    nsresult (*GetDiscard)(IMediumAttachment *pThis, PRBool *discard);

    nsresult (*GetHotPluggable)(IMediumAttachment *pThis, PRBool *hotPluggable);

    nsresult (*GetBandwidthGroup)(IMediumAttachment *pThis, IBandwidthGroup * *bandwidthGroup);

};
#define IMediumAttachment_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMediumAttachment_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMediumAttachment_Release(p) ((p)->lpVtbl->Release(p))
#define IMediumAttachment_get_Medium(p, aMedium) ((p)->lpVtbl->GetMedium(p, aMedium))
#define IMediumAttachment_GetMedium(p, aMedium) ((p)->lpVtbl->GetMedium(p, aMedium))
#define IMediumAttachment_get_Controller(p, aController) ((p)->lpVtbl->GetController(p, aController))
#define IMediumAttachment_GetController(p, aController) ((p)->lpVtbl->GetController(p, aController))
#define IMediumAttachment_get_Port(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IMediumAttachment_GetPort(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IMediumAttachment_get_Device(p, aDevice) ((p)->lpVtbl->GetDevice(p, aDevice))
#define IMediumAttachment_GetDevice(p, aDevice) ((p)->lpVtbl->GetDevice(p, aDevice))
#define IMediumAttachment_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMediumAttachment_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMediumAttachment_get_Passthrough(p, aPassthrough) ((p)->lpVtbl->GetPassthrough(p, aPassthrough))
#define IMediumAttachment_GetPassthrough(p, aPassthrough) ((p)->lpVtbl->GetPassthrough(p, aPassthrough))
#define IMediumAttachment_get_TemporaryEject(p, aTemporaryEject) ((p)->lpVtbl->GetTemporaryEject(p, aTemporaryEject))
#define IMediumAttachment_GetTemporaryEject(p, aTemporaryEject) ((p)->lpVtbl->GetTemporaryEject(p, aTemporaryEject))
#define IMediumAttachment_get_IsEjected(p, aIsEjected) ((p)->lpVtbl->GetIsEjected(p, aIsEjected))
#define IMediumAttachment_GetIsEjected(p, aIsEjected) ((p)->lpVtbl->GetIsEjected(p, aIsEjected))
#define IMediumAttachment_get_NonRotational(p, aNonRotational) ((p)->lpVtbl->GetNonRotational(p, aNonRotational))
#define IMediumAttachment_GetNonRotational(p, aNonRotational) ((p)->lpVtbl->GetNonRotational(p, aNonRotational))
#define IMediumAttachment_get_Discard(p, aDiscard) ((p)->lpVtbl->GetDiscard(p, aDiscard))
#define IMediumAttachment_GetDiscard(p, aDiscard) ((p)->lpVtbl->GetDiscard(p, aDiscard))
#define IMediumAttachment_get_HotPluggable(p, aHotPluggable) ((p)->lpVtbl->GetHotPluggable(p, aHotPluggable))
#define IMediumAttachment_GetHotPluggable(p, aHotPluggable) ((p)->lpVtbl->GetHotPluggable(p, aHotPluggable))
#define IMediumAttachment_get_BandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aBandwidthGroup))
#define IMediumAttachment_GetBandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aBandwidthGroup))
#endif /* VBOX_WITH_GLUE */

interface IMediumAttachment
{
#ifndef VBOX_WITH_GLUE
    struct IMediumAttachment_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMediumAttachmentVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMediumAttachment declaration */


/* Start of struct IMedium declaration */
#define IMEDIUM_IID_STR "05f2bbb6-a3a6-4fb9-9b49-6d0dda7142ac"
#define IMEDIUM_IID { \
    0x05f2bbb6, 0xa3a6, 0x4fb9, \
    { 0x9b, 0x49, 0x6d, 0x0d, 0xda, 0x71, 0x42, 0xac } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMedium);
#ifndef VBOX_WITH_GLUE
struct IMedium_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetId)(IMedium *pThis, PRUnichar * *id);

    nsresult (*GetDescription)(IMedium *pThis, PRUnichar * *description);
    nsresult (*SetDescription)(IMedium *pThis, PRUnichar * description);

    nsresult (*GetState)(IMedium *pThis, PRUint32 *state);

    nsresult (*GetVariant)(IMedium *pThis, PRUint32 *variantSize, PRUint32 **variant);

    nsresult (*GetLocation)(IMedium *pThis, PRUnichar * *location);

    nsresult (*GetName)(IMedium *pThis, PRUnichar * *name);

    nsresult (*GetDeviceType)(IMedium *pThis, PRUint32 *deviceType);

    nsresult (*GetHostDrive)(IMedium *pThis, PRBool *hostDrive);

    nsresult (*GetSize)(IMedium *pThis, PRInt64 *size);

    nsresult (*GetFormat)(IMedium *pThis, PRUnichar * *format);

    nsresult (*GetMediumFormat)(IMedium *pThis, IMediumFormat * *mediumFormat);

    nsresult (*GetType)(IMedium *pThis, PRUint32 *type);
    nsresult (*SetType)(IMedium *pThis, PRUint32 type);

    nsresult (*GetAllowedTypes)(IMedium *pThis, PRUint32 *allowedTypesSize, PRUint32 **allowedTypes);

    nsresult (*GetParent)(IMedium *pThis, IMedium * *parent);

    nsresult (*GetChildren)(IMedium *pThis, PRUint32 *childrenSize, IMedium * **children);

    nsresult (*GetBase)(IMedium *pThis, IMedium * *base);

    nsresult (*GetReadOnly)(IMedium *pThis, PRBool *readOnly);

    nsresult (*GetLogicalSize)(IMedium *pThis, PRInt64 *logicalSize);

    nsresult (*GetAutoReset)(IMedium *pThis, PRBool *autoReset);
    nsresult (*SetAutoReset)(IMedium *pThis, PRBool autoReset);

    nsresult (*GetLastAccessError)(IMedium *pThis, PRUnichar * *lastAccessError);

    nsresult (*GetMachineIds)(IMedium *pThis, PRUint32 *machineIdsSize, PRUnichar * **machineIds);

    nsresult (*SetIds)(
        IMedium *pThis,
        PRBool setImageId,
        PRUnichar * imageId,
        PRBool setParentId,
        PRUnichar * parentId
    );

    nsresult (*RefreshState)(
        IMedium *pThis,
        PRUint32 * state
    );

    nsresult (*GetSnapshotIds)(
        IMedium *pThis,
        PRUnichar * machineId,
        PRUint32 *snapshotIdsSize,
        PRUnichar *** snapshotIds
    );

    nsresult (*LockRead)(
        IMedium *pThis,
        IToken * * token
    );

    nsresult (*LockWrite)(
        IMedium *pThis,
        IToken * * token
    );

    nsresult (*Close)(IMedium *pThis );

    nsresult (*GetProperty)(
        IMedium *pThis,
        PRUnichar * name,
        PRUnichar * * value
    );

    nsresult (*SetProperty)(
        IMedium *pThis,
        PRUnichar * name,
        PRUnichar * value
    );

    nsresult (*GetProperties)(
        IMedium *pThis,
        PRUnichar * names,
        PRUint32 *returnNamesSize,
        PRUnichar *** returnNames,
        PRUint32 *returnValuesSize,
        PRUnichar *** returnValues
    );

    nsresult (*SetProperties)(
        IMedium *pThis,
        PRUint32 namesSize,
        PRUnichar ** names,
        PRUint32 valuesSize,
        PRUnichar ** values
    );

    nsresult (*CreateBaseStorage)(
        IMedium *pThis,
        PRInt64 logicalSize,
        PRUint32 variantSize,
        PRUint32* variant,
        IProgress * * progress
    );

    nsresult (*DeleteStorage)(
        IMedium *pThis,
        IProgress * * progress
    );

    nsresult (*CreateDiffStorage)(
        IMedium *pThis,
        IMedium * target,
        PRUint32 variantSize,
        PRUint32* variant,
        IProgress * * progress
    );

    nsresult (*MergeTo)(
        IMedium *pThis,
        IMedium * target,
        IProgress * * progress
    );

    nsresult (*CloneTo)(
        IMedium *pThis,
        IMedium * target,
        PRUint32 variantSize,
        PRUint32* variant,
        IMedium * parent,
        IProgress * * progress
    );

    nsresult (*CloneToBase)(
        IMedium *pThis,
        IMedium * target,
        PRUint32 variantSize,
        PRUint32* variant,
        IProgress * * progress
    );

    nsresult (*SetLocation)(
        IMedium *pThis,
        PRUnichar * location,
        IProgress * * progress
    );

    nsresult (*Compact)(
        IMedium *pThis,
        IProgress * * progress
    );

    nsresult (*Resize)(
        IMedium *pThis,
        PRInt64 logicalSize,
        IProgress * * progress
    );

    nsresult (*Reset)(
        IMedium *pThis,
        IProgress * * progress
    );

};
#else /* VBOX_WITH_GLUE */
struct IMediumVtbl
{
    nsresult (*QueryInterface)(IMedium *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMedium *pThis);
    nsrefcnt (*Release)(IMedium *pThis);
    nsresult (*GetId)(IMedium *pThis, PRUnichar * *id);

    nsresult (*GetDescription)(IMedium *pThis, PRUnichar * *description);
    nsresult (*SetDescription)(IMedium *pThis, PRUnichar * description);

    nsresult (*GetState)(IMedium *pThis, PRUint32 *state);

    nsresult (*GetVariant)(IMedium *pThis, PRUint32 *variantSize, PRUint32 **variant);

    nsresult (*GetLocation)(IMedium *pThis, PRUnichar * *location);

    nsresult (*GetName)(IMedium *pThis, PRUnichar * *name);

    nsresult (*GetDeviceType)(IMedium *pThis, PRUint32 *deviceType);

    nsresult (*GetHostDrive)(IMedium *pThis, PRBool *hostDrive);

    nsresult (*GetSize)(IMedium *pThis, PRInt64 *size);

    nsresult (*GetFormat)(IMedium *pThis, PRUnichar * *format);

    nsresult (*GetMediumFormat)(IMedium *pThis, IMediumFormat * *mediumFormat);

    nsresult (*GetType)(IMedium *pThis, PRUint32 *type);
    nsresult (*SetType)(IMedium *pThis, PRUint32 type);

    nsresult (*GetAllowedTypes)(IMedium *pThis, PRUint32 *allowedTypesSize, PRUint32 **allowedTypes);

    nsresult (*GetParent)(IMedium *pThis, IMedium * *parent);

    nsresult (*GetChildren)(IMedium *pThis, PRUint32 *childrenSize, IMedium * **children);

    nsresult (*GetBase)(IMedium *pThis, IMedium * *base);

    nsresult (*GetReadOnly)(IMedium *pThis, PRBool *readOnly);

    nsresult (*GetLogicalSize)(IMedium *pThis, PRInt64 *logicalSize);

    nsresult (*GetAutoReset)(IMedium *pThis, PRBool *autoReset);
    nsresult (*SetAutoReset)(IMedium *pThis, PRBool autoReset);

    nsresult (*GetLastAccessError)(IMedium *pThis, PRUnichar * *lastAccessError);

    nsresult (*GetMachineIds)(IMedium *pThis, PRUint32 *machineIdsSize, PRUnichar * **machineIds);

    nsresult (*SetIds)(
        IMedium *pThis,
        PRBool setImageId,
        PRUnichar * imageId,
        PRBool setParentId,
        PRUnichar * parentId
    );

    nsresult (*RefreshState)(
        IMedium *pThis,
        PRUint32 * state
    );

    nsresult (*GetSnapshotIds)(
        IMedium *pThis,
        PRUnichar * machineId,
        PRUint32 *snapshotIdsSize,
        PRUnichar *** snapshotIds
    );

    nsresult (*LockRead)(
        IMedium *pThis,
        IToken * * token
    );

    nsresult (*LockWrite)(
        IMedium *pThis,
        IToken * * token
    );

    nsresult (*Close)(IMedium *pThis );

    nsresult (*GetProperty)(
        IMedium *pThis,
        PRUnichar * name,
        PRUnichar * * value
    );

    nsresult (*SetProperty)(
        IMedium *pThis,
        PRUnichar * name,
        PRUnichar * value
    );

    nsresult (*GetProperties)(
        IMedium *pThis,
        PRUnichar * names,
        PRUint32 *returnNamesSize,
        PRUnichar *** returnNames,
        PRUint32 *returnValuesSize,
        PRUnichar *** returnValues
    );

    nsresult (*SetProperties)(
        IMedium *pThis,
        PRUint32 namesSize,
        PRUnichar ** names,
        PRUint32 valuesSize,
        PRUnichar ** values
    );

    nsresult (*CreateBaseStorage)(
        IMedium *pThis,
        PRInt64 logicalSize,
        PRUint32 variantSize,
        PRUint32* variant,
        IProgress * * progress
    );

    nsresult (*DeleteStorage)(
        IMedium *pThis,
        IProgress * * progress
    );

    nsresult (*CreateDiffStorage)(
        IMedium *pThis,
        IMedium * target,
        PRUint32 variantSize,
        PRUint32* variant,
        IProgress * * progress
    );

    nsresult (*MergeTo)(
        IMedium *pThis,
        IMedium * target,
        IProgress * * progress
    );

    nsresult (*CloneTo)(
        IMedium *pThis,
        IMedium * target,
        PRUint32 variantSize,
        PRUint32* variant,
        IMedium * parent,
        IProgress * * progress
    );

    nsresult (*CloneToBase)(
        IMedium *pThis,
        IMedium * target,
        PRUint32 variantSize,
        PRUint32* variant,
        IProgress * * progress
    );

    nsresult (*SetLocation)(
        IMedium *pThis,
        PRUnichar * location,
        IProgress * * progress
    );

    nsresult (*Compact)(
        IMedium *pThis,
        IProgress * * progress
    );

    nsresult (*Resize)(
        IMedium *pThis,
        PRInt64 logicalSize,
        IProgress * * progress
    );

    nsresult (*Reset)(
        IMedium *pThis,
        IProgress * * progress
    );

};
#define IMedium_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMedium_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMedium_Release(p) ((p)->lpVtbl->Release(p))
#define IMedium_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IMedium_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IMedium_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IMedium_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IMedium_put_Description(p, aDescription) ((p)->lpVtbl->SetDescription(p, aDescription))
#define IMedium_SetDescription(p, aDescription) ((p)->lpVtbl->SetDescription(p, aDescription))
#define IMedium_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IMedium_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IMedium_get_Variant(p, aVariant) ((p)->lpVtbl->GetVariant(p, aVariant))
#define IMedium_GetVariant(p, aVariant) ((p)->lpVtbl->GetVariant(p, aVariant))
#define IMedium_get_Location(p, aLocation) ((p)->lpVtbl->GetLocation(p, aLocation))
#define IMedium_GetLocation(p, aLocation) ((p)->lpVtbl->GetLocation(p, aLocation))
#define IMedium_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IMedium_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IMedium_get_DeviceType(p, aDeviceType) ((p)->lpVtbl->GetDeviceType(p, aDeviceType))
#define IMedium_GetDeviceType(p, aDeviceType) ((p)->lpVtbl->GetDeviceType(p, aDeviceType))
#define IMedium_get_HostDrive(p, aHostDrive) ((p)->lpVtbl->GetHostDrive(p, aHostDrive))
#define IMedium_GetHostDrive(p, aHostDrive) ((p)->lpVtbl->GetHostDrive(p, aHostDrive))
#define IMedium_get_Size(p, aSize) ((p)->lpVtbl->GetSize(p, aSize))
#define IMedium_GetSize(p, aSize) ((p)->lpVtbl->GetSize(p, aSize))
#define IMedium_get_Format(p, aFormat) ((p)->lpVtbl->GetFormat(p, aFormat))
#define IMedium_GetFormat(p, aFormat) ((p)->lpVtbl->GetFormat(p, aFormat))
#define IMedium_get_MediumFormat(p, aMediumFormat) ((p)->lpVtbl->GetMediumFormat(p, aMediumFormat))
#define IMedium_GetMediumFormat(p, aMediumFormat) ((p)->lpVtbl->GetMediumFormat(p, aMediumFormat))
#define IMedium_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMedium_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMedium_put_Type(p, aType) ((p)->lpVtbl->SetType(p, aType))
#define IMedium_SetType(p, aType) ((p)->lpVtbl->SetType(p, aType))
#define IMedium_get_AllowedTypes(p, aAllowedTypes) ((p)->lpVtbl->GetAllowedTypes(p, aAllowedTypes))
#define IMedium_GetAllowedTypes(p, aAllowedTypes) ((p)->lpVtbl->GetAllowedTypes(p, aAllowedTypes))
#define IMedium_get_Parent(p, aParent) ((p)->lpVtbl->GetParent(p, aParent))
#define IMedium_GetParent(p, aParent) ((p)->lpVtbl->GetParent(p, aParent))
#define IMedium_get_Children(p, aChildren) ((p)->lpVtbl->GetChildren(p, aChildren))
#define IMedium_GetChildren(p, aChildren) ((p)->lpVtbl->GetChildren(p, aChildren))
#define IMedium_get_Base(p, aBase) ((p)->lpVtbl->GetBase(p, aBase))
#define IMedium_GetBase(p, aBase) ((p)->lpVtbl->GetBase(p, aBase))
#define IMedium_get_ReadOnly(p, aReadOnly) ((p)->lpVtbl->GetReadOnly(p, aReadOnly))
#define IMedium_GetReadOnly(p, aReadOnly) ((p)->lpVtbl->GetReadOnly(p, aReadOnly))
#define IMedium_get_LogicalSize(p, aLogicalSize) ((p)->lpVtbl->GetLogicalSize(p, aLogicalSize))
#define IMedium_GetLogicalSize(p, aLogicalSize) ((p)->lpVtbl->GetLogicalSize(p, aLogicalSize))
#define IMedium_get_AutoReset(p, aAutoReset) ((p)->lpVtbl->GetAutoReset(p, aAutoReset))
#define IMedium_GetAutoReset(p, aAutoReset) ((p)->lpVtbl->GetAutoReset(p, aAutoReset))
#define IMedium_put_AutoReset(p, aAutoReset) ((p)->lpVtbl->SetAutoReset(p, aAutoReset))
#define IMedium_SetAutoReset(p, aAutoReset) ((p)->lpVtbl->SetAutoReset(p, aAutoReset))
#define IMedium_get_LastAccessError(p, aLastAccessError) ((p)->lpVtbl->GetLastAccessError(p, aLastAccessError))
#define IMedium_GetLastAccessError(p, aLastAccessError) ((p)->lpVtbl->GetLastAccessError(p, aLastAccessError))
#define IMedium_get_MachineIds(p, aMachineIds) ((p)->lpVtbl->GetMachineIds(p, aMachineIds))
#define IMedium_GetMachineIds(p, aMachineIds) ((p)->lpVtbl->GetMachineIds(p, aMachineIds))
#define IMedium_SetIds(p, aSetImageId, aImageId, aSetParentId, aParentId) ((p)->lpVtbl->SetIds(p, aSetImageId, aImageId, aSetParentId, aParentId))
#define IMedium_RefreshState(p, aState) ((p)->lpVtbl->RefreshState(p, aState))
#define IMedium_GetSnapshotIds(p, aMachineId, aSnapshotIds) ((p)->lpVtbl->GetSnapshotIds(p, aMachineId, aSnapshotIds))
#define IMedium_LockRead(p, aToken) ((p)->lpVtbl->LockRead(p, aToken))
#define IMedium_LockWrite(p, aToken) ((p)->lpVtbl->LockWrite(p, aToken))
#define IMedium_Close(p) ((p)->lpVtbl->Close(p))
#define IMedium_GetProperty(p, aName, aValue) ((p)->lpVtbl->GetProperty(p, aName, aValue))
#define IMedium_SetProperty(p, aName, aValue) ((p)->lpVtbl->SetProperty(p, aName, aValue))
#define IMedium_GetProperties(p, aNames, aReturnNames, aReturnValues) ((p)->lpVtbl->GetProperties(p, aNames, aReturnNames, aReturnValues))
#define IMedium_SetProperties(p, aNames, aValues) ((p)->lpVtbl->SetProperties(p, aNames, aValues))
#define IMedium_CreateBaseStorage(p, aLogicalSize, aVariant, aProgress) ((p)->lpVtbl->CreateBaseStorage(p, aLogicalSize, aVariant, aProgress))
#define IMedium_DeleteStorage(p, aProgress) ((p)->lpVtbl->DeleteStorage(p, aProgress))
#define IMedium_CreateDiffStorage(p, aTarget, aVariant, aProgress) ((p)->lpVtbl->CreateDiffStorage(p, aTarget, aVariant, aProgress))
#define IMedium_MergeTo(p, aTarget, aProgress) ((p)->lpVtbl->MergeTo(p, aTarget, aProgress))
#define IMedium_CloneTo(p, aTarget, aVariant, aParent, aProgress) ((p)->lpVtbl->CloneTo(p, aTarget, aVariant, aParent, aProgress))
#define IMedium_CloneToBase(p, aTarget, aVariant, aProgress) ((p)->lpVtbl->CloneToBase(p, aTarget, aVariant, aProgress))
#define IMedium_SetLocation(p, aLocation, aProgress) ((p)->lpVtbl->SetLocation(p, aLocation, aProgress))
#define IMedium_Compact(p, aProgress) ((p)->lpVtbl->Compact(p, aProgress))
#define IMedium_Resize(p, aLogicalSize, aProgress) ((p)->lpVtbl->Resize(p, aLogicalSize, aProgress))
#define IMedium_Reset(p, aProgress) ((p)->lpVtbl->Reset(p, aProgress))
#endif /* VBOX_WITH_GLUE */

interface IMedium
{
#ifndef VBOX_WITH_GLUE
    struct IMedium_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMediumVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMedium declaration */


/* Start of struct IMediumFormat declaration */
#define IMEDIUMFORMAT_IID_STR "6238e1cf-a17d-4ec1-8172-418bfb22b93a"
#define IMEDIUMFORMAT_IID { \
    0x6238e1cf, 0xa17d, 0x4ec1, \
    { 0x81, 0x72, 0x41, 0x8b, 0xfb, 0x22, 0xb9, 0x3a } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMediumFormat);
#ifndef VBOX_WITH_GLUE
struct IMediumFormat_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetId)(IMediumFormat *pThis, PRUnichar * *id);

    nsresult (*GetName)(IMediumFormat *pThis, PRUnichar * *name);

    nsresult (*GetCapabilities)(IMediumFormat *pThis, PRUint32 *capabilitiesSize, PRUint32 **capabilities);

    nsresult (*DescribeFileExtensions)(
        IMediumFormat *pThis,
        PRUint32 *extensionsSize,
        PRUnichar *** extensions,
        PRUint32 *typesSize,
        PRUint32** types
    );

    nsresult (*DescribeProperties)(
        IMediumFormat *pThis,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *descriptionsSize,
        PRUnichar *** descriptions,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *flagsSize,
        PRUint32** flags,
        PRUint32 *defaultsSize,
        PRUnichar *** defaults
    );

};
#else /* VBOX_WITH_GLUE */
struct IMediumFormatVtbl
{
    nsresult (*QueryInterface)(IMediumFormat *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMediumFormat *pThis);
    nsrefcnt (*Release)(IMediumFormat *pThis);
    nsresult (*GetId)(IMediumFormat *pThis, PRUnichar * *id);

    nsresult (*GetName)(IMediumFormat *pThis, PRUnichar * *name);

    nsresult (*GetCapabilities)(IMediumFormat *pThis, PRUint32 *capabilitiesSize, PRUint32 **capabilities);

    nsresult (*DescribeFileExtensions)(
        IMediumFormat *pThis,
        PRUint32 *extensionsSize,
        PRUnichar *** extensions,
        PRUint32 *typesSize,
        PRUint32** types
    );

    nsresult (*DescribeProperties)(
        IMediumFormat *pThis,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *descriptionsSize,
        PRUnichar *** descriptions,
        PRUint32 *typesSize,
        PRUint32** types,
        PRUint32 *flagsSize,
        PRUint32** flags,
        PRUint32 *defaultsSize,
        PRUnichar *** defaults
    );

};
#define IMediumFormat_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMediumFormat_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMediumFormat_Release(p) ((p)->lpVtbl->Release(p))
#define IMediumFormat_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IMediumFormat_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IMediumFormat_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IMediumFormat_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IMediumFormat_get_Capabilities(p, aCapabilities) ((p)->lpVtbl->GetCapabilities(p, aCapabilities))
#define IMediumFormat_GetCapabilities(p, aCapabilities) ((p)->lpVtbl->GetCapabilities(p, aCapabilities))
#define IMediumFormat_DescribeFileExtensions(p, aExtensions, aTypes) ((p)->lpVtbl->DescribeFileExtensions(p, aExtensions, aTypes))
#define IMediumFormat_DescribeProperties(p, aNames, aDescriptions, aTypes, aFlags, aDefaults) ((p)->lpVtbl->DescribeProperties(p, aNames, aDescriptions, aTypes, aFlags, aDefaults))
#endif /* VBOX_WITH_GLUE */

interface IMediumFormat
{
#ifndef VBOX_WITH_GLUE
    struct IMediumFormat_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMediumFormatVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMediumFormat declaration */


/* Start of struct IToken declaration */
#define ITOKEN_IID_STR "3b1c4797-e289-4d4c-b74c-50c9b86a36f8"
#define ITOKEN_IID { \
    0x3b1c4797, 0xe289, 0x4d4c, \
    { 0xb7, 0x4c, 0x50, 0xc9, 0xb8, 0x6a, 0x36, 0xf8 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IToken);
#ifndef VBOX_WITH_GLUE
struct IToken_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*Abandon)(IToken *pThis );

    nsresult (*Dummy)(IToken *pThis );

};
#else /* VBOX_WITH_GLUE */
struct ITokenVtbl
{
    nsresult (*QueryInterface)(IToken *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IToken *pThis);
    nsrefcnt (*Release)(IToken *pThis);
    nsresult (*Abandon)(IToken *pThis );

    nsresult (*Dummy)(IToken *pThis );

};
#define IToken_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IToken_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IToken_Release(p) ((p)->lpVtbl->Release(p))
#define IToken_Abandon(p) ((p)->lpVtbl->Abandon(p))
#define IToken_Dummy(p) ((p)->lpVtbl->Dummy(p))
#endif /* VBOX_WITH_GLUE */

interface IToken
{
#ifndef VBOX_WITH_GLUE
    struct IToken_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ITokenVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IToken declaration */


/* Start of struct IKeyboard declaration */
#define IKEYBOARD_IID_STR "f6916ec5-a881-4237-898f-7de58cf88672"
#define IKEYBOARD_IID { \
    0xf6916ec5, 0xa881, 0x4237, \
    { 0x89, 0x8f, 0x7d, 0xe5, 0x8c, 0xf8, 0x86, 0x72 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IKeyboard);
#ifndef VBOX_WITH_GLUE
struct IKeyboard_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetEventSource)(IKeyboard *pThis, IEventSource * *eventSource);

    nsresult (*PutScancode)(
        IKeyboard *pThis,
        PRInt32 scancode
    );

    nsresult (*PutScancodes)(
        IKeyboard *pThis,
        PRUint32 scancodesSize,
        PRInt32* scancodes,
        PRUint32 * codesStored
    );

    nsresult (*PutCAD)(IKeyboard *pThis );

};
#else /* VBOX_WITH_GLUE */
struct IKeyboardVtbl
{
    nsresult (*QueryInterface)(IKeyboard *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IKeyboard *pThis);
    nsrefcnt (*Release)(IKeyboard *pThis);
    nsresult (*GetEventSource)(IKeyboard *pThis, IEventSource * *eventSource);

    nsresult (*PutScancode)(
        IKeyboard *pThis,
        PRInt32 scancode
    );

    nsresult (*PutScancodes)(
        IKeyboard *pThis,
        PRUint32 scancodesSize,
        PRInt32* scancodes,
        PRUint32 * codesStored
    );

    nsresult (*PutCAD)(IKeyboard *pThis );

};
#define IKeyboard_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IKeyboard_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IKeyboard_Release(p) ((p)->lpVtbl->Release(p))
#define IKeyboard_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IKeyboard_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IKeyboard_PutScancode(p, aScancode) ((p)->lpVtbl->PutScancode(p, aScancode))
#define IKeyboard_PutScancodes(p, aScancodes, aCodesStored) ((p)->lpVtbl->PutScancodes(p, aScancodes, aCodesStored))
#define IKeyboard_PutCAD(p) ((p)->lpVtbl->PutCAD(p))
#endif /* VBOX_WITH_GLUE */

interface IKeyboard
{
#ifndef VBOX_WITH_GLUE
    struct IKeyboard_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IKeyboardVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IKeyboard declaration */


/* Start of struct IMouse declaration */
#define IMOUSE_IID_STR "ee770393-415f-4421-b2d5-28b73cacf86a"
#define IMOUSE_IID { \
    0xee770393, 0x415f, 0x4421, \
    { 0xb2, 0xd5, 0x28, 0xb7, 0x3c, 0xac, 0xf8, 0x6a } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMouse);
#ifndef VBOX_WITH_GLUE
struct IMouse_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetAbsoluteSupported)(IMouse *pThis, PRBool *absoluteSupported);

    nsresult (*GetRelativeSupported)(IMouse *pThis, PRBool *relativeSupported);

    nsresult (*GetMultiTouchSupported)(IMouse *pThis, PRBool *multiTouchSupported);

    nsresult (*GetNeedsHostCursor)(IMouse *pThis, PRBool *needsHostCursor);

    nsresult (*GetEventSource)(IMouse *pThis, IEventSource * *eventSource);

    nsresult (*PutMouseEvent)(
        IMouse *pThis,
        PRInt32 dx,
        PRInt32 dy,
        PRInt32 dz,
        PRInt32 dw,
        PRInt32 buttonState
    );

    nsresult (*PutMouseEventAbsolute)(
        IMouse *pThis,
        PRInt32 x,
        PRInt32 y,
        PRInt32 dz,
        PRInt32 dw,
        PRInt32 buttonState
    );

    nsresult (*PutEventMultiTouch)(
        IMouse *pThis,
        PRInt32 count,
        PRUint32 contactsSize,
        PRInt64* contacts,
        PRUint32 scanTime
    );

    nsresult (*PutEventMultiTouchString)(
        IMouse *pThis,
        PRInt32 count,
        PRUnichar * contacts,
        PRUint32 scanTime
    );

};
#else /* VBOX_WITH_GLUE */
struct IMouseVtbl
{
    nsresult (*QueryInterface)(IMouse *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMouse *pThis);
    nsrefcnt (*Release)(IMouse *pThis);
    nsresult (*GetAbsoluteSupported)(IMouse *pThis, PRBool *absoluteSupported);

    nsresult (*GetRelativeSupported)(IMouse *pThis, PRBool *relativeSupported);

    nsresult (*GetMultiTouchSupported)(IMouse *pThis, PRBool *multiTouchSupported);

    nsresult (*GetNeedsHostCursor)(IMouse *pThis, PRBool *needsHostCursor);

    nsresult (*GetEventSource)(IMouse *pThis, IEventSource * *eventSource);

    nsresult (*PutMouseEvent)(
        IMouse *pThis,
        PRInt32 dx,
        PRInt32 dy,
        PRInt32 dz,
        PRInt32 dw,
        PRInt32 buttonState
    );

    nsresult (*PutMouseEventAbsolute)(
        IMouse *pThis,
        PRInt32 x,
        PRInt32 y,
        PRInt32 dz,
        PRInt32 dw,
        PRInt32 buttonState
    );

    nsresult (*PutEventMultiTouch)(
        IMouse *pThis,
        PRInt32 count,
        PRUint32 contactsSize,
        PRInt64* contacts,
        PRUint32 scanTime
    );

    nsresult (*PutEventMultiTouchString)(
        IMouse *pThis,
        PRInt32 count,
        PRUnichar * contacts,
        PRUint32 scanTime
    );

};
#define IMouse_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMouse_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMouse_Release(p) ((p)->lpVtbl->Release(p))
#define IMouse_get_AbsoluteSupported(p, aAbsoluteSupported) ((p)->lpVtbl->GetAbsoluteSupported(p, aAbsoluteSupported))
#define IMouse_GetAbsoluteSupported(p, aAbsoluteSupported) ((p)->lpVtbl->GetAbsoluteSupported(p, aAbsoluteSupported))
#define IMouse_get_RelativeSupported(p, aRelativeSupported) ((p)->lpVtbl->GetRelativeSupported(p, aRelativeSupported))
#define IMouse_GetRelativeSupported(p, aRelativeSupported) ((p)->lpVtbl->GetRelativeSupported(p, aRelativeSupported))
#define IMouse_get_MultiTouchSupported(p, aMultiTouchSupported) ((p)->lpVtbl->GetMultiTouchSupported(p, aMultiTouchSupported))
#define IMouse_GetMultiTouchSupported(p, aMultiTouchSupported) ((p)->lpVtbl->GetMultiTouchSupported(p, aMultiTouchSupported))
#define IMouse_get_NeedsHostCursor(p, aNeedsHostCursor) ((p)->lpVtbl->GetNeedsHostCursor(p, aNeedsHostCursor))
#define IMouse_GetNeedsHostCursor(p, aNeedsHostCursor) ((p)->lpVtbl->GetNeedsHostCursor(p, aNeedsHostCursor))
#define IMouse_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IMouse_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IMouse_PutMouseEvent(p, aDx, aDy, aDz, aDw, aButtonState) ((p)->lpVtbl->PutMouseEvent(p, aDx, aDy, aDz, aDw, aButtonState))
#define IMouse_PutMouseEventAbsolute(p, aX, aY, aDz, aDw, aButtonState) ((p)->lpVtbl->PutMouseEventAbsolute(p, aX, aY, aDz, aDw, aButtonState))
#define IMouse_PutEventMultiTouch(p, aCount, aContacts, aScanTime) ((p)->lpVtbl->PutEventMultiTouch(p, aCount, aContacts, aScanTime))
#define IMouse_PutEventMultiTouchString(p, aCount, aContacts, aScanTime) ((p)->lpVtbl->PutEventMultiTouchString(p, aCount, aContacts, aScanTime))
#endif /* VBOX_WITH_GLUE */

interface IMouse
{
#ifndef VBOX_WITH_GLUE
    struct IMouse_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMouseVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMouse declaration */


/* Start of struct IFramebuffer declaration */
#define IFRAMEBUFFER_IID_STR "e3f122c0-adab-4fc9-a8dc-da112fb48428"
#define IFRAMEBUFFER_IID { \
    0xe3f122c0, 0xadab, 0x4fc9, \
    { 0xa8, 0xdc, 0xda, 0x11, 0x2f, 0xb4, 0x84, 0x28 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IFramebuffer);
#ifndef VBOX_WITH_GLUE
struct IFramebuffer_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetAddress)(IFramebuffer *pThis, PRUint8 * *address);

    nsresult (*GetWidth)(IFramebuffer *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IFramebuffer *pThis, PRUint32 *height);

    nsresult (*GetBitsPerPixel)(IFramebuffer *pThis, PRUint32 *bitsPerPixel);

    nsresult (*GetBytesPerLine)(IFramebuffer *pThis, PRUint32 *bytesPerLine);

    nsresult (*GetPixelFormat)(IFramebuffer *pThis, PRUint32 *pixelFormat);

    nsresult (*GetUsesGuestVRAM)(IFramebuffer *pThis, PRBool *usesGuestVRAM);

    nsresult (*GetHeightReduction)(IFramebuffer *pThis, PRUint32 *heightReduction);

    nsresult (*GetOverlay)(IFramebuffer *pThis, IFramebufferOverlay * *overlay);

    nsresult (*GetWinId)(IFramebuffer *pThis, PRInt64 *winId);

    nsresult (*Lock)(IFramebuffer *pThis );

    nsresult (*Unlock)(IFramebuffer *pThis );

    nsresult (*NotifyUpdate)(
        IFramebuffer *pThis,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*RequestResize)(
        IFramebuffer *pThis,
        PRUint32 screenId,
        PRUint32 pixelFormat,
        PRUint8 * VRAM,
        PRUint32 bitsPerPixel,
        PRUint32 bytesPerLine,
        PRUint32 width,
        PRUint32 height,
        PRBool * finished
    );

    nsresult (*VideoModeSupported)(
        IFramebuffer *pThis,
        PRUint32 width,
        PRUint32 height,
        PRUint32 bpp,
        PRBool * supported
    );

    nsresult (*GetVisibleRegion)(
        IFramebuffer *pThis,
        PRUint8 * rectangles,
        PRUint32 count,
        PRUint32 * countCopied
    );

    nsresult (*SetVisibleRegion)(
        IFramebuffer *pThis,
        PRUint8 * rectangles,
        PRUint32 count
    );

    nsresult (*ProcessVHWACommand)(
        IFramebuffer *pThis,
        PRUint8 * command
    );

    nsresult (*Notify3DEvent)(
        IFramebuffer *pThis,
        PRUint32 type,
        PRUint8 * data
    );

};
#else /* VBOX_WITH_GLUE */
struct IFramebufferVtbl
{
    nsresult (*QueryInterface)(IFramebuffer *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IFramebuffer *pThis);
    nsrefcnt (*Release)(IFramebuffer *pThis);
    nsresult (*GetAddress)(IFramebuffer *pThis, PRUint8 * *address);

    nsresult (*GetWidth)(IFramebuffer *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IFramebuffer *pThis, PRUint32 *height);

    nsresult (*GetBitsPerPixel)(IFramebuffer *pThis, PRUint32 *bitsPerPixel);

    nsresult (*GetBytesPerLine)(IFramebuffer *pThis, PRUint32 *bytesPerLine);

    nsresult (*GetPixelFormat)(IFramebuffer *pThis, PRUint32 *pixelFormat);

    nsresult (*GetUsesGuestVRAM)(IFramebuffer *pThis, PRBool *usesGuestVRAM);

    nsresult (*GetHeightReduction)(IFramebuffer *pThis, PRUint32 *heightReduction);

    nsresult (*GetOverlay)(IFramebuffer *pThis, IFramebufferOverlay * *overlay);

    nsresult (*GetWinId)(IFramebuffer *pThis, PRInt64 *winId);

    nsresult (*Lock)(IFramebuffer *pThis );

    nsresult (*Unlock)(IFramebuffer *pThis );

    nsresult (*NotifyUpdate)(
        IFramebuffer *pThis,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*RequestResize)(
        IFramebuffer *pThis,
        PRUint32 screenId,
        PRUint32 pixelFormat,
        PRUint8 * VRAM,
        PRUint32 bitsPerPixel,
        PRUint32 bytesPerLine,
        PRUint32 width,
        PRUint32 height,
        PRBool * finished
    );

    nsresult (*VideoModeSupported)(
        IFramebuffer *pThis,
        PRUint32 width,
        PRUint32 height,
        PRUint32 bpp,
        PRBool * supported
    );

    nsresult (*GetVisibleRegion)(
        IFramebuffer *pThis,
        PRUint8 * rectangles,
        PRUint32 count,
        PRUint32 * countCopied
    );

    nsresult (*SetVisibleRegion)(
        IFramebuffer *pThis,
        PRUint8 * rectangles,
        PRUint32 count
    );

    nsresult (*ProcessVHWACommand)(
        IFramebuffer *pThis,
        PRUint8 * command
    );

    nsresult (*Notify3DEvent)(
        IFramebuffer *pThis,
        PRUint32 type,
        PRUint8 * data
    );

};
#define IFramebuffer_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IFramebuffer_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IFramebuffer_Release(p) ((p)->lpVtbl->Release(p))
#define IFramebuffer_get_Address(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IFramebuffer_GetAddress(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IFramebuffer_get_Width(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IFramebuffer_GetWidth(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IFramebuffer_get_Height(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IFramebuffer_GetHeight(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IFramebuffer_get_BitsPerPixel(p, aBitsPerPixel) ((p)->lpVtbl->GetBitsPerPixel(p, aBitsPerPixel))
#define IFramebuffer_GetBitsPerPixel(p, aBitsPerPixel) ((p)->lpVtbl->GetBitsPerPixel(p, aBitsPerPixel))
#define IFramebuffer_get_BytesPerLine(p, aBytesPerLine) ((p)->lpVtbl->GetBytesPerLine(p, aBytesPerLine))
#define IFramebuffer_GetBytesPerLine(p, aBytesPerLine) ((p)->lpVtbl->GetBytesPerLine(p, aBytesPerLine))
#define IFramebuffer_get_PixelFormat(p, aPixelFormat) ((p)->lpVtbl->GetPixelFormat(p, aPixelFormat))
#define IFramebuffer_GetPixelFormat(p, aPixelFormat) ((p)->lpVtbl->GetPixelFormat(p, aPixelFormat))
#define IFramebuffer_get_UsesGuestVRAM(p, aUsesGuestVRAM) ((p)->lpVtbl->GetUsesGuestVRAM(p, aUsesGuestVRAM))
#define IFramebuffer_GetUsesGuestVRAM(p, aUsesGuestVRAM) ((p)->lpVtbl->GetUsesGuestVRAM(p, aUsesGuestVRAM))
#define IFramebuffer_get_HeightReduction(p, aHeightReduction) ((p)->lpVtbl->GetHeightReduction(p, aHeightReduction))
#define IFramebuffer_GetHeightReduction(p, aHeightReduction) ((p)->lpVtbl->GetHeightReduction(p, aHeightReduction))
#define IFramebuffer_get_Overlay(p, aOverlay) ((p)->lpVtbl->GetOverlay(p, aOverlay))
#define IFramebuffer_GetOverlay(p, aOverlay) ((p)->lpVtbl->GetOverlay(p, aOverlay))
#define IFramebuffer_get_WinId(p, aWinId) ((p)->lpVtbl->GetWinId(p, aWinId))
#define IFramebuffer_GetWinId(p, aWinId) ((p)->lpVtbl->GetWinId(p, aWinId))
#define IFramebuffer_Lock(p) ((p)->lpVtbl->Lock(p))
#define IFramebuffer_Unlock(p) ((p)->lpVtbl->Unlock(p))
#define IFramebuffer_NotifyUpdate(p, aX, aY, aWidth, aHeight) ((p)->lpVtbl->NotifyUpdate(p, aX, aY, aWidth, aHeight))
#define IFramebuffer_RequestResize(p, aScreenId, aPixelFormat, aVRAM, aBitsPerPixel, aBytesPerLine, aWidth, aHeight, aFinished) ((p)->lpVtbl->RequestResize(p, aScreenId, aPixelFormat, aVRAM, aBitsPerPixel, aBytesPerLine, aWidth, aHeight, aFinished))
#define IFramebuffer_VideoModeSupported(p, aWidth, aHeight, aBpp, aSupported) ((p)->lpVtbl->VideoModeSupported(p, aWidth, aHeight, aBpp, aSupported))
#define IFramebuffer_GetVisibleRegion(p, aRectangles, aCount, aCountCopied) ((p)->lpVtbl->GetVisibleRegion(p, aRectangles, aCount, aCountCopied))
#define IFramebuffer_SetVisibleRegion(p, aRectangles, aCount) ((p)->lpVtbl->SetVisibleRegion(p, aRectangles, aCount))
#define IFramebuffer_ProcessVHWACommand(p, aCommand) ((p)->lpVtbl->ProcessVHWACommand(p, aCommand))
#define IFramebuffer_Notify3DEvent(p, aType, aData) ((p)->lpVtbl->Notify3DEvent(p, aType, aData))
#endif /* VBOX_WITH_GLUE */

interface IFramebuffer
{
#ifndef VBOX_WITH_GLUE
    struct IFramebuffer_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IFramebufferVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IFramebuffer declaration */


/* Start of struct IFramebufferOverlay declaration */
#define IFRAMEBUFFEROVERLAY_IID_STR "0bcc1c7e-e415-47d2-bfdb-e4c705fb0f47"
#define IFRAMEBUFFEROVERLAY_IID { \
    0x0bcc1c7e, 0xe415, 0x47d2, \
    { 0xbf, 0xdb, 0xe4, 0xc7, 0x05, 0xfb, 0x0f, 0x47 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IFramebufferOverlay);
#ifndef VBOX_WITH_GLUE
struct IFramebufferOverlay_vtbl
{
    struct IFramebuffer_vtbl iframebuffer;

    nsresult (*GetX)(IFramebufferOverlay *pThis, PRUint32 *x);

    nsresult (*GetY)(IFramebufferOverlay *pThis, PRUint32 *y);

    nsresult (*GetVisible)(IFramebufferOverlay *pThis, PRBool *visible);
    nsresult (*SetVisible)(IFramebufferOverlay *pThis, PRBool visible);

    nsresult (*GetAlpha)(IFramebufferOverlay *pThis, PRUint32 *alpha);
    nsresult (*SetAlpha)(IFramebufferOverlay *pThis, PRUint32 alpha);

    nsresult (*Move)(
        IFramebufferOverlay *pThis,
        PRUint32 x,
        PRUint32 y
    );

};
#else /* VBOX_WITH_GLUE */
struct IFramebufferOverlayVtbl
{
    nsresult (*QueryInterface)(IFramebufferOverlay *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IFramebufferOverlay *pThis);
    nsrefcnt (*Release)(IFramebufferOverlay *pThis);
    nsresult (*GetAddress)(IFramebufferOverlay *pThis, PRUint8 * *address);

    nsresult (*GetWidth)(IFramebufferOverlay *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IFramebufferOverlay *pThis, PRUint32 *height);

    nsresult (*GetBitsPerPixel)(IFramebufferOverlay *pThis, PRUint32 *bitsPerPixel);

    nsresult (*GetBytesPerLine)(IFramebufferOverlay *pThis, PRUint32 *bytesPerLine);

    nsresult (*GetPixelFormat)(IFramebufferOverlay *pThis, PRUint32 *pixelFormat);

    nsresult (*GetUsesGuestVRAM)(IFramebufferOverlay *pThis, PRBool *usesGuestVRAM);

    nsresult (*GetHeightReduction)(IFramebufferOverlay *pThis, PRUint32 *heightReduction);

    nsresult (*GetOverlay)(IFramebufferOverlay *pThis, IFramebufferOverlay * *overlay);

    nsresult (*GetWinId)(IFramebufferOverlay *pThis, PRInt64 *winId);

    nsresult (*Lock)(IFramebufferOverlay *pThis );

    nsresult (*Unlock)(IFramebufferOverlay *pThis );

    nsresult (*NotifyUpdate)(
        IFramebufferOverlay *pThis,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*RequestResize)(
        IFramebufferOverlay *pThis,
        PRUint32 screenId,
        PRUint32 pixelFormat,
        PRUint8 * VRAM,
        PRUint32 bitsPerPixel,
        PRUint32 bytesPerLine,
        PRUint32 width,
        PRUint32 height,
        PRBool * finished
    );

    nsresult (*VideoModeSupported)(
        IFramebufferOverlay *pThis,
        PRUint32 width,
        PRUint32 height,
        PRUint32 bpp,
        PRBool * supported
    );

    nsresult (*GetVisibleRegion)(
        IFramebufferOverlay *pThis,
        PRUint8 * rectangles,
        PRUint32 count,
        PRUint32 * countCopied
    );

    nsresult (*SetVisibleRegion)(
        IFramebufferOverlay *pThis,
        PRUint8 * rectangles,
        PRUint32 count
    );

    nsresult (*ProcessVHWACommand)(
        IFramebufferOverlay *pThis,
        PRUint8 * command
    );

    nsresult (*Notify3DEvent)(
        IFramebufferOverlay *pThis,
        PRUint32 type,
        PRUint8 * data
    );

    nsresult (*GetX)(IFramebufferOverlay *pThis, PRUint32 *x);

    nsresult (*GetY)(IFramebufferOverlay *pThis, PRUint32 *y);

    nsresult (*GetVisible)(IFramebufferOverlay *pThis, PRBool *visible);
    nsresult (*SetVisible)(IFramebufferOverlay *pThis, PRBool visible);

    nsresult (*GetAlpha)(IFramebufferOverlay *pThis, PRUint32 *alpha);
    nsresult (*SetAlpha)(IFramebufferOverlay *pThis, PRUint32 alpha);

    nsresult (*Move)(
        IFramebufferOverlay *pThis,
        PRUint32 x,
        PRUint32 y
    );

};
#define IFramebufferOverlay_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IFramebufferOverlay_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IFramebufferOverlay_Release(p) ((p)->lpVtbl->Release(p))
#define IFramebufferOverlay_get_Address(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IFramebufferOverlay_GetAddress(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IFramebufferOverlay_get_Width(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IFramebufferOverlay_GetWidth(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IFramebufferOverlay_get_Height(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IFramebufferOverlay_GetHeight(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IFramebufferOverlay_get_BitsPerPixel(p, aBitsPerPixel) ((p)->lpVtbl->GetBitsPerPixel(p, aBitsPerPixel))
#define IFramebufferOverlay_GetBitsPerPixel(p, aBitsPerPixel) ((p)->lpVtbl->GetBitsPerPixel(p, aBitsPerPixel))
#define IFramebufferOverlay_get_BytesPerLine(p, aBytesPerLine) ((p)->lpVtbl->GetBytesPerLine(p, aBytesPerLine))
#define IFramebufferOverlay_GetBytesPerLine(p, aBytesPerLine) ((p)->lpVtbl->GetBytesPerLine(p, aBytesPerLine))
#define IFramebufferOverlay_get_PixelFormat(p, aPixelFormat) ((p)->lpVtbl->GetPixelFormat(p, aPixelFormat))
#define IFramebufferOverlay_GetPixelFormat(p, aPixelFormat) ((p)->lpVtbl->GetPixelFormat(p, aPixelFormat))
#define IFramebufferOverlay_get_UsesGuestVRAM(p, aUsesGuestVRAM) ((p)->lpVtbl->GetUsesGuestVRAM(p, aUsesGuestVRAM))
#define IFramebufferOverlay_GetUsesGuestVRAM(p, aUsesGuestVRAM) ((p)->lpVtbl->GetUsesGuestVRAM(p, aUsesGuestVRAM))
#define IFramebufferOverlay_get_HeightReduction(p, aHeightReduction) ((p)->lpVtbl->GetHeightReduction(p, aHeightReduction))
#define IFramebufferOverlay_GetHeightReduction(p, aHeightReduction) ((p)->lpVtbl->GetHeightReduction(p, aHeightReduction))
#define IFramebufferOverlay_get_Overlay(p, aOverlay) ((p)->lpVtbl->GetOverlay(p, aOverlay))
#define IFramebufferOverlay_GetOverlay(p, aOverlay) ((p)->lpVtbl->GetOverlay(p, aOverlay))
#define IFramebufferOverlay_get_WinId(p, aWinId) ((p)->lpVtbl->GetWinId(p, aWinId))
#define IFramebufferOverlay_GetWinId(p, aWinId) ((p)->lpVtbl->GetWinId(p, aWinId))
#define IFramebufferOverlay_Lock(p) ((p)->lpVtbl->Lock(p))
#define IFramebufferOverlay_Unlock(p) ((p)->lpVtbl->Unlock(p))
#define IFramebufferOverlay_NotifyUpdate(p, aX, aY, aWidth, aHeight) ((p)->lpVtbl->NotifyUpdate(p, aX, aY, aWidth, aHeight))
#define IFramebufferOverlay_RequestResize(p, aScreenId, aPixelFormat, aVRAM, aBitsPerPixel, aBytesPerLine, aWidth, aHeight, aFinished) ((p)->lpVtbl->RequestResize(p, aScreenId, aPixelFormat, aVRAM, aBitsPerPixel, aBytesPerLine, aWidth, aHeight, aFinished))
#define IFramebufferOverlay_VideoModeSupported(p, aWidth, aHeight, aBpp, aSupported) ((p)->lpVtbl->VideoModeSupported(p, aWidth, aHeight, aBpp, aSupported))
#define IFramebufferOverlay_GetVisibleRegion(p, aRectangles, aCount, aCountCopied) ((p)->lpVtbl->GetVisibleRegion(p, aRectangles, aCount, aCountCopied))
#define IFramebufferOverlay_SetVisibleRegion(p, aRectangles, aCount) ((p)->lpVtbl->SetVisibleRegion(p, aRectangles, aCount))
#define IFramebufferOverlay_ProcessVHWACommand(p, aCommand) ((p)->lpVtbl->ProcessVHWACommand(p, aCommand))
#define IFramebufferOverlay_Notify3DEvent(p, aType, aData) ((p)->lpVtbl->Notify3DEvent(p, aType, aData))
#define IFramebufferOverlay_get_X(p, aX) ((p)->lpVtbl->GetX(p, aX))
#define IFramebufferOverlay_GetX(p, aX) ((p)->lpVtbl->GetX(p, aX))
#define IFramebufferOverlay_get_Y(p, aY) ((p)->lpVtbl->GetY(p, aY))
#define IFramebufferOverlay_GetY(p, aY) ((p)->lpVtbl->GetY(p, aY))
#define IFramebufferOverlay_get_Visible(p, aVisible) ((p)->lpVtbl->GetVisible(p, aVisible))
#define IFramebufferOverlay_GetVisible(p, aVisible) ((p)->lpVtbl->GetVisible(p, aVisible))
#define IFramebufferOverlay_put_Visible(p, aVisible) ((p)->lpVtbl->SetVisible(p, aVisible))
#define IFramebufferOverlay_SetVisible(p, aVisible) ((p)->lpVtbl->SetVisible(p, aVisible))
#define IFramebufferOverlay_get_Alpha(p, aAlpha) ((p)->lpVtbl->GetAlpha(p, aAlpha))
#define IFramebufferOverlay_GetAlpha(p, aAlpha) ((p)->lpVtbl->GetAlpha(p, aAlpha))
#define IFramebufferOverlay_put_Alpha(p, aAlpha) ((p)->lpVtbl->SetAlpha(p, aAlpha))
#define IFramebufferOverlay_SetAlpha(p, aAlpha) ((p)->lpVtbl->SetAlpha(p, aAlpha))
#define IFramebufferOverlay_Move(p, aX, aY) ((p)->lpVtbl->Move(p, aX, aY))
#endif /* VBOX_WITH_GLUE */

interface IFramebufferOverlay
{
#ifndef VBOX_WITH_GLUE
    struct IFramebufferOverlay_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IFramebufferOverlayVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IFramebufferOverlay declaration */


/* Start of struct IDisplay declaration */
#define IDISPLAY_IID_STR "480b372c-c0b5-4c23-9bd7-dcbb85b1594c"
#define IDISPLAY_IID { \
    0x480b372c, 0xc0b5, 0x4c23, \
    { 0x9b, 0xd7, 0xdc, 0xbb, 0x85, 0xb1, 0x59, 0x4c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IDisplay);
#ifndef VBOX_WITH_GLUE
struct IDisplay_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetScreenResolution)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 * bitsPerPixel,
        PRInt32 * xOrigin,
        PRInt32 * yOrigin
    );

    nsresult (*SetFramebuffer)(
        IDisplay *pThis,
        PRUint32 screenId,
        IFramebuffer * framebuffer
    );

    nsresult (*GetFramebuffer)(
        IDisplay *pThis,
        PRUint32 screenId,
        IFramebuffer * * framebuffer,
        PRInt32 * xOrigin,
        PRInt32 * yOrigin
    );

    nsresult (*SetVideoModeHint)(
        IDisplay *pThis,
        PRUint32 display,
        PRBool enabled,
        PRBool changeOrigin,
        PRInt32 originX,
        PRInt32 originY,
        PRUint32 width,
        PRUint32 height,
        PRUint32 bitsPerPixel
    );

    nsresult (*SetSeamlessMode)(
        IDisplay *pThis,
        PRBool enabled
    );

    nsresult (*TakeScreenShot)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint8 * address,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*TakeScreenShotToArray)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 width,
        PRUint32 height,
        PRUint32 *screenDataSize,
        PRUint8** screenData
    );

    nsresult (*TakeScreenShotPNGToArray)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 width,
        PRUint32 height,
        PRUint32 *screenDataSize,
        PRUint8** screenData
    );

    nsresult (*DrawToScreen)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint8 * address,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*InvalidateAndUpdate)(IDisplay *pThis );

    nsresult (*ResizeCompleted)(
        IDisplay *pThis,
        PRUint32 screenId
    );

    nsresult (*CompleteVHWACommand)(
        IDisplay *pThis,
        PRUint8 * command
    );

    nsresult (*ViewportChanged)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

};
#else /* VBOX_WITH_GLUE */
struct IDisplayVtbl
{
    nsresult (*QueryInterface)(IDisplay *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IDisplay *pThis);
    nsrefcnt (*Release)(IDisplay *pThis);
    nsresult (*GetScreenResolution)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 * width,
        PRUint32 * height,
        PRUint32 * bitsPerPixel,
        PRInt32 * xOrigin,
        PRInt32 * yOrigin
    );

    nsresult (*SetFramebuffer)(
        IDisplay *pThis,
        PRUint32 screenId,
        IFramebuffer * framebuffer
    );

    nsresult (*GetFramebuffer)(
        IDisplay *pThis,
        PRUint32 screenId,
        IFramebuffer * * framebuffer,
        PRInt32 * xOrigin,
        PRInt32 * yOrigin
    );

    nsresult (*SetVideoModeHint)(
        IDisplay *pThis,
        PRUint32 display,
        PRBool enabled,
        PRBool changeOrigin,
        PRInt32 originX,
        PRInt32 originY,
        PRUint32 width,
        PRUint32 height,
        PRUint32 bitsPerPixel
    );

    nsresult (*SetSeamlessMode)(
        IDisplay *pThis,
        PRBool enabled
    );

    nsresult (*TakeScreenShot)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint8 * address,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*TakeScreenShotToArray)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 width,
        PRUint32 height,
        PRUint32 *screenDataSize,
        PRUint8** screenData
    );

    nsresult (*TakeScreenShotPNGToArray)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 width,
        PRUint32 height,
        PRUint32 *screenDataSize,
        PRUint8** screenData
    );

    nsresult (*DrawToScreen)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint8 * address,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

    nsresult (*InvalidateAndUpdate)(IDisplay *pThis );

    nsresult (*ResizeCompleted)(
        IDisplay *pThis,
        PRUint32 screenId
    );

    nsresult (*CompleteVHWACommand)(
        IDisplay *pThis,
        PRUint8 * command
    );

    nsresult (*ViewportChanged)(
        IDisplay *pThis,
        PRUint32 screenId,
        PRUint32 x,
        PRUint32 y,
        PRUint32 width,
        PRUint32 height
    );

};
#define IDisplay_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IDisplay_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IDisplay_Release(p) ((p)->lpVtbl->Release(p))
#define IDisplay_GetScreenResolution(p, aScreenId, aWidth, aHeight, aBitsPerPixel, aXOrigin, aYOrigin) ((p)->lpVtbl->GetScreenResolution(p, aScreenId, aWidth, aHeight, aBitsPerPixel, aXOrigin, aYOrigin))
#define IDisplay_SetFramebuffer(p, aScreenId, aFramebuffer) ((p)->lpVtbl->SetFramebuffer(p, aScreenId, aFramebuffer))
#define IDisplay_GetFramebuffer(p, aScreenId, aFramebuffer, aXOrigin, aYOrigin) ((p)->lpVtbl->GetFramebuffer(p, aScreenId, aFramebuffer, aXOrigin, aYOrigin))
#define IDisplay_SetVideoModeHint(p, aDisplay, aEnabled, aChangeOrigin, aOriginX, aOriginY, aWidth, aHeight, aBitsPerPixel) ((p)->lpVtbl->SetVideoModeHint(p, aDisplay, aEnabled, aChangeOrigin, aOriginX, aOriginY, aWidth, aHeight, aBitsPerPixel))
#define IDisplay_SetSeamlessMode(p, aEnabled) ((p)->lpVtbl->SetSeamlessMode(p, aEnabled))
#define IDisplay_TakeScreenShot(p, aScreenId, aAddress, aWidth, aHeight) ((p)->lpVtbl->TakeScreenShot(p, aScreenId, aAddress, aWidth, aHeight))
#define IDisplay_TakeScreenShotToArray(p, aScreenId, aWidth, aHeight, aScreenData) ((p)->lpVtbl->TakeScreenShotToArray(p, aScreenId, aWidth, aHeight, aScreenData))
#define IDisplay_TakeScreenShotPNGToArray(p, aScreenId, aWidth, aHeight, aScreenData) ((p)->lpVtbl->TakeScreenShotPNGToArray(p, aScreenId, aWidth, aHeight, aScreenData))
#define IDisplay_DrawToScreen(p, aScreenId, aAddress, aX, aY, aWidth, aHeight) ((p)->lpVtbl->DrawToScreen(p, aScreenId, aAddress, aX, aY, aWidth, aHeight))
#define IDisplay_InvalidateAndUpdate(p) ((p)->lpVtbl->InvalidateAndUpdate(p))
#define IDisplay_ResizeCompleted(p, aScreenId) ((p)->lpVtbl->ResizeCompleted(p, aScreenId))
#define IDisplay_CompleteVHWACommand(p, aCommand) ((p)->lpVtbl->CompleteVHWACommand(p, aCommand))
#define IDisplay_ViewportChanged(p, aScreenId, aX, aY, aWidth, aHeight) ((p)->lpVtbl->ViewportChanged(p, aScreenId, aX, aY, aWidth, aHeight))
#endif /* VBOX_WITH_GLUE */

interface IDisplay
{
#ifndef VBOX_WITH_GLUE
    struct IDisplay_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IDisplayVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IDisplay declaration */


/* Start of struct INetworkAdapter declaration */
#define INETWORKADAPTER_IID_STR "efa0f965-63c7-4c60-afdf-b1cc9943b9c0"
#define INETWORKADAPTER_IID { \
    0xefa0f965, 0x63c7, 0x4c60, \
    { 0xaf, 0xdf, 0xb1, 0xcc, 0x99, 0x43, 0xb9, 0xc0 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INetworkAdapter);
#ifndef VBOX_WITH_GLUE
struct INetworkAdapter_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetAdapterType)(INetworkAdapter *pThis, PRUint32 *adapterType);
    nsresult (*SetAdapterType)(INetworkAdapter *pThis, PRUint32 adapterType);

    nsresult (*GetSlot)(INetworkAdapter *pThis, PRUint32 *slot);

    nsresult (*GetEnabled)(INetworkAdapter *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(INetworkAdapter *pThis, PRBool enabled);

    nsresult (*GetMACAddress)(INetworkAdapter *pThis, PRUnichar * *MACAddress);
    nsresult (*SetMACAddress)(INetworkAdapter *pThis, PRUnichar * MACAddress);

    nsresult (*GetAttachmentType)(INetworkAdapter *pThis, PRUint32 *attachmentType);
    nsresult (*SetAttachmentType)(INetworkAdapter *pThis, PRUint32 attachmentType);

    nsresult (*GetBridgedInterface)(INetworkAdapter *pThis, PRUnichar * *bridgedInterface);
    nsresult (*SetBridgedInterface)(INetworkAdapter *pThis, PRUnichar * bridgedInterface);

    nsresult (*GetHostOnlyInterface)(INetworkAdapter *pThis, PRUnichar * *hostOnlyInterface);
    nsresult (*SetHostOnlyInterface)(INetworkAdapter *pThis, PRUnichar * hostOnlyInterface);

    nsresult (*GetInternalNetwork)(INetworkAdapter *pThis, PRUnichar * *internalNetwork);
    nsresult (*SetInternalNetwork)(INetworkAdapter *pThis, PRUnichar * internalNetwork);

    nsresult (*GetNATNetwork)(INetworkAdapter *pThis, PRUnichar * *NATNetwork);
    nsresult (*SetNATNetwork)(INetworkAdapter *pThis, PRUnichar * NATNetwork);

    nsresult (*GetGenericDriver)(INetworkAdapter *pThis, PRUnichar * *genericDriver);
    nsresult (*SetGenericDriver)(INetworkAdapter *pThis, PRUnichar * genericDriver);

    nsresult (*GetCableConnected)(INetworkAdapter *pThis, PRBool *cableConnected);
    nsresult (*SetCableConnected)(INetworkAdapter *pThis, PRBool cableConnected);

    nsresult (*GetLineSpeed)(INetworkAdapter *pThis, PRUint32 *lineSpeed);
    nsresult (*SetLineSpeed)(INetworkAdapter *pThis, PRUint32 lineSpeed);

    nsresult (*GetPromiscModePolicy)(INetworkAdapter *pThis, PRUint32 *promiscModePolicy);
    nsresult (*SetPromiscModePolicy)(INetworkAdapter *pThis, PRUint32 promiscModePolicy);

    nsresult (*GetTraceEnabled)(INetworkAdapter *pThis, PRBool *traceEnabled);
    nsresult (*SetTraceEnabled)(INetworkAdapter *pThis, PRBool traceEnabled);

    nsresult (*GetTraceFile)(INetworkAdapter *pThis, PRUnichar * *traceFile);
    nsresult (*SetTraceFile)(INetworkAdapter *pThis, PRUnichar * traceFile);

    nsresult (*GetNATEngine)(INetworkAdapter *pThis, INATEngine * *NATEngine);

    nsresult (*GetBootPriority)(INetworkAdapter *pThis, PRUint32 *bootPriority);
    nsresult (*SetBootPriority)(INetworkAdapter *pThis, PRUint32 bootPriority);

    nsresult (*GetBandwidthGroup)(INetworkAdapter *pThis, IBandwidthGroup * *bandwidthGroup);
    nsresult (*SetBandwidthGroup)(INetworkAdapter *pThis, IBandwidthGroup * bandwidthGroup);

    nsresult (*GetProperty)(
        INetworkAdapter *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

    nsresult (*SetProperty)(
        INetworkAdapter *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*GetProperties)(
        INetworkAdapter *pThis,
        PRUnichar * names,
        PRUint32 *returnNamesSize,
        PRUnichar *** returnNames,
        PRUint32 *returnValuesSize,
        PRUnichar *** returnValues
    );

};
#else /* VBOX_WITH_GLUE */
struct INetworkAdapterVtbl
{
    nsresult (*QueryInterface)(INetworkAdapter *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INetworkAdapter *pThis);
    nsrefcnt (*Release)(INetworkAdapter *pThis);
    nsresult (*GetAdapterType)(INetworkAdapter *pThis, PRUint32 *adapterType);
    nsresult (*SetAdapterType)(INetworkAdapter *pThis, PRUint32 adapterType);

    nsresult (*GetSlot)(INetworkAdapter *pThis, PRUint32 *slot);

    nsresult (*GetEnabled)(INetworkAdapter *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(INetworkAdapter *pThis, PRBool enabled);

    nsresult (*GetMACAddress)(INetworkAdapter *pThis, PRUnichar * *MACAddress);
    nsresult (*SetMACAddress)(INetworkAdapter *pThis, PRUnichar * MACAddress);

    nsresult (*GetAttachmentType)(INetworkAdapter *pThis, PRUint32 *attachmentType);
    nsresult (*SetAttachmentType)(INetworkAdapter *pThis, PRUint32 attachmentType);

    nsresult (*GetBridgedInterface)(INetworkAdapter *pThis, PRUnichar * *bridgedInterface);
    nsresult (*SetBridgedInterface)(INetworkAdapter *pThis, PRUnichar * bridgedInterface);

    nsresult (*GetHostOnlyInterface)(INetworkAdapter *pThis, PRUnichar * *hostOnlyInterface);
    nsresult (*SetHostOnlyInterface)(INetworkAdapter *pThis, PRUnichar * hostOnlyInterface);

    nsresult (*GetInternalNetwork)(INetworkAdapter *pThis, PRUnichar * *internalNetwork);
    nsresult (*SetInternalNetwork)(INetworkAdapter *pThis, PRUnichar * internalNetwork);

    nsresult (*GetNATNetwork)(INetworkAdapter *pThis, PRUnichar * *NATNetwork);
    nsresult (*SetNATNetwork)(INetworkAdapter *pThis, PRUnichar * NATNetwork);

    nsresult (*GetGenericDriver)(INetworkAdapter *pThis, PRUnichar * *genericDriver);
    nsresult (*SetGenericDriver)(INetworkAdapter *pThis, PRUnichar * genericDriver);

    nsresult (*GetCableConnected)(INetworkAdapter *pThis, PRBool *cableConnected);
    nsresult (*SetCableConnected)(INetworkAdapter *pThis, PRBool cableConnected);

    nsresult (*GetLineSpeed)(INetworkAdapter *pThis, PRUint32 *lineSpeed);
    nsresult (*SetLineSpeed)(INetworkAdapter *pThis, PRUint32 lineSpeed);

    nsresult (*GetPromiscModePolicy)(INetworkAdapter *pThis, PRUint32 *promiscModePolicy);
    nsresult (*SetPromiscModePolicy)(INetworkAdapter *pThis, PRUint32 promiscModePolicy);

    nsresult (*GetTraceEnabled)(INetworkAdapter *pThis, PRBool *traceEnabled);
    nsresult (*SetTraceEnabled)(INetworkAdapter *pThis, PRBool traceEnabled);

    nsresult (*GetTraceFile)(INetworkAdapter *pThis, PRUnichar * *traceFile);
    nsresult (*SetTraceFile)(INetworkAdapter *pThis, PRUnichar * traceFile);

    nsresult (*GetNATEngine)(INetworkAdapter *pThis, INATEngine * *NATEngine);

    nsresult (*GetBootPriority)(INetworkAdapter *pThis, PRUint32 *bootPriority);
    nsresult (*SetBootPriority)(INetworkAdapter *pThis, PRUint32 bootPriority);

    nsresult (*GetBandwidthGroup)(INetworkAdapter *pThis, IBandwidthGroup * *bandwidthGroup);
    nsresult (*SetBandwidthGroup)(INetworkAdapter *pThis, IBandwidthGroup * bandwidthGroup);

    nsresult (*GetProperty)(
        INetworkAdapter *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

    nsresult (*SetProperty)(
        INetworkAdapter *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*GetProperties)(
        INetworkAdapter *pThis,
        PRUnichar * names,
        PRUint32 *returnNamesSize,
        PRUnichar *** returnNames,
        PRUint32 *returnValuesSize,
        PRUnichar *** returnValues
    );

};
#define INetworkAdapter_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INetworkAdapter_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INetworkAdapter_Release(p) ((p)->lpVtbl->Release(p))
#define INetworkAdapter_get_AdapterType(p, aAdapterType) ((p)->lpVtbl->GetAdapterType(p, aAdapterType))
#define INetworkAdapter_GetAdapterType(p, aAdapterType) ((p)->lpVtbl->GetAdapterType(p, aAdapterType))
#define INetworkAdapter_put_AdapterType(p, aAdapterType) ((p)->lpVtbl->SetAdapterType(p, aAdapterType))
#define INetworkAdapter_SetAdapterType(p, aAdapterType) ((p)->lpVtbl->SetAdapterType(p, aAdapterType))
#define INetworkAdapter_get_Slot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define INetworkAdapter_GetSlot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define INetworkAdapter_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define INetworkAdapter_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define INetworkAdapter_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define INetworkAdapter_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define INetworkAdapter_get_MACAddress(p, aMACAddress) ((p)->lpVtbl->GetMACAddress(p, aMACAddress))
#define INetworkAdapter_GetMACAddress(p, aMACAddress) ((p)->lpVtbl->GetMACAddress(p, aMACAddress))
#define INetworkAdapter_put_MACAddress(p, aMACAddress) ((p)->lpVtbl->SetMACAddress(p, aMACAddress))
#define INetworkAdapter_SetMACAddress(p, aMACAddress) ((p)->lpVtbl->SetMACAddress(p, aMACAddress))
#define INetworkAdapter_get_AttachmentType(p, aAttachmentType) ((p)->lpVtbl->GetAttachmentType(p, aAttachmentType))
#define INetworkAdapter_GetAttachmentType(p, aAttachmentType) ((p)->lpVtbl->GetAttachmentType(p, aAttachmentType))
#define INetworkAdapter_put_AttachmentType(p, aAttachmentType) ((p)->lpVtbl->SetAttachmentType(p, aAttachmentType))
#define INetworkAdapter_SetAttachmentType(p, aAttachmentType) ((p)->lpVtbl->SetAttachmentType(p, aAttachmentType))
#define INetworkAdapter_get_BridgedInterface(p, aBridgedInterface) ((p)->lpVtbl->GetBridgedInterface(p, aBridgedInterface))
#define INetworkAdapter_GetBridgedInterface(p, aBridgedInterface) ((p)->lpVtbl->GetBridgedInterface(p, aBridgedInterface))
#define INetworkAdapter_put_BridgedInterface(p, aBridgedInterface) ((p)->lpVtbl->SetBridgedInterface(p, aBridgedInterface))
#define INetworkAdapter_SetBridgedInterface(p, aBridgedInterface) ((p)->lpVtbl->SetBridgedInterface(p, aBridgedInterface))
#define INetworkAdapter_get_HostOnlyInterface(p, aHostOnlyInterface) ((p)->lpVtbl->GetHostOnlyInterface(p, aHostOnlyInterface))
#define INetworkAdapter_GetHostOnlyInterface(p, aHostOnlyInterface) ((p)->lpVtbl->GetHostOnlyInterface(p, aHostOnlyInterface))
#define INetworkAdapter_put_HostOnlyInterface(p, aHostOnlyInterface) ((p)->lpVtbl->SetHostOnlyInterface(p, aHostOnlyInterface))
#define INetworkAdapter_SetHostOnlyInterface(p, aHostOnlyInterface) ((p)->lpVtbl->SetHostOnlyInterface(p, aHostOnlyInterface))
#define INetworkAdapter_get_InternalNetwork(p, aInternalNetwork) ((p)->lpVtbl->GetInternalNetwork(p, aInternalNetwork))
#define INetworkAdapter_GetInternalNetwork(p, aInternalNetwork) ((p)->lpVtbl->GetInternalNetwork(p, aInternalNetwork))
#define INetworkAdapter_put_InternalNetwork(p, aInternalNetwork) ((p)->lpVtbl->SetInternalNetwork(p, aInternalNetwork))
#define INetworkAdapter_SetInternalNetwork(p, aInternalNetwork) ((p)->lpVtbl->SetInternalNetwork(p, aInternalNetwork))
#define INetworkAdapter_get_NATNetwork(p, aNATNetwork) ((p)->lpVtbl->GetNATNetwork(p, aNATNetwork))
#define INetworkAdapter_GetNATNetwork(p, aNATNetwork) ((p)->lpVtbl->GetNATNetwork(p, aNATNetwork))
#define INetworkAdapter_put_NATNetwork(p, aNATNetwork) ((p)->lpVtbl->SetNATNetwork(p, aNATNetwork))
#define INetworkAdapter_SetNATNetwork(p, aNATNetwork) ((p)->lpVtbl->SetNATNetwork(p, aNATNetwork))
#define INetworkAdapter_get_GenericDriver(p, aGenericDriver) ((p)->lpVtbl->GetGenericDriver(p, aGenericDriver))
#define INetworkAdapter_GetGenericDriver(p, aGenericDriver) ((p)->lpVtbl->GetGenericDriver(p, aGenericDriver))
#define INetworkAdapter_put_GenericDriver(p, aGenericDriver) ((p)->lpVtbl->SetGenericDriver(p, aGenericDriver))
#define INetworkAdapter_SetGenericDriver(p, aGenericDriver) ((p)->lpVtbl->SetGenericDriver(p, aGenericDriver))
#define INetworkAdapter_get_CableConnected(p, aCableConnected) ((p)->lpVtbl->GetCableConnected(p, aCableConnected))
#define INetworkAdapter_GetCableConnected(p, aCableConnected) ((p)->lpVtbl->GetCableConnected(p, aCableConnected))
#define INetworkAdapter_put_CableConnected(p, aCableConnected) ((p)->lpVtbl->SetCableConnected(p, aCableConnected))
#define INetworkAdapter_SetCableConnected(p, aCableConnected) ((p)->lpVtbl->SetCableConnected(p, aCableConnected))
#define INetworkAdapter_get_LineSpeed(p, aLineSpeed) ((p)->lpVtbl->GetLineSpeed(p, aLineSpeed))
#define INetworkAdapter_GetLineSpeed(p, aLineSpeed) ((p)->lpVtbl->GetLineSpeed(p, aLineSpeed))
#define INetworkAdapter_put_LineSpeed(p, aLineSpeed) ((p)->lpVtbl->SetLineSpeed(p, aLineSpeed))
#define INetworkAdapter_SetLineSpeed(p, aLineSpeed) ((p)->lpVtbl->SetLineSpeed(p, aLineSpeed))
#define INetworkAdapter_get_PromiscModePolicy(p, aPromiscModePolicy) ((p)->lpVtbl->GetPromiscModePolicy(p, aPromiscModePolicy))
#define INetworkAdapter_GetPromiscModePolicy(p, aPromiscModePolicy) ((p)->lpVtbl->GetPromiscModePolicy(p, aPromiscModePolicy))
#define INetworkAdapter_put_PromiscModePolicy(p, aPromiscModePolicy) ((p)->lpVtbl->SetPromiscModePolicy(p, aPromiscModePolicy))
#define INetworkAdapter_SetPromiscModePolicy(p, aPromiscModePolicy) ((p)->lpVtbl->SetPromiscModePolicy(p, aPromiscModePolicy))
#define INetworkAdapter_get_TraceEnabled(p, aTraceEnabled) ((p)->lpVtbl->GetTraceEnabled(p, aTraceEnabled))
#define INetworkAdapter_GetTraceEnabled(p, aTraceEnabled) ((p)->lpVtbl->GetTraceEnabled(p, aTraceEnabled))
#define INetworkAdapter_put_TraceEnabled(p, aTraceEnabled) ((p)->lpVtbl->SetTraceEnabled(p, aTraceEnabled))
#define INetworkAdapter_SetTraceEnabled(p, aTraceEnabled) ((p)->lpVtbl->SetTraceEnabled(p, aTraceEnabled))
#define INetworkAdapter_get_TraceFile(p, aTraceFile) ((p)->lpVtbl->GetTraceFile(p, aTraceFile))
#define INetworkAdapter_GetTraceFile(p, aTraceFile) ((p)->lpVtbl->GetTraceFile(p, aTraceFile))
#define INetworkAdapter_put_TraceFile(p, aTraceFile) ((p)->lpVtbl->SetTraceFile(p, aTraceFile))
#define INetworkAdapter_SetTraceFile(p, aTraceFile) ((p)->lpVtbl->SetTraceFile(p, aTraceFile))
#define INetworkAdapter_get_NATEngine(p, aNATEngine) ((p)->lpVtbl->GetNATEngine(p, aNATEngine))
#define INetworkAdapter_GetNATEngine(p, aNATEngine) ((p)->lpVtbl->GetNATEngine(p, aNATEngine))
#define INetworkAdapter_get_BootPriority(p, aBootPriority) ((p)->lpVtbl->GetBootPriority(p, aBootPriority))
#define INetworkAdapter_GetBootPriority(p, aBootPriority) ((p)->lpVtbl->GetBootPriority(p, aBootPriority))
#define INetworkAdapter_put_BootPriority(p, aBootPriority) ((p)->lpVtbl->SetBootPriority(p, aBootPriority))
#define INetworkAdapter_SetBootPriority(p, aBootPriority) ((p)->lpVtbl->SetBootPriority(p, aBootPriority))
#define INetworkAdapter_get_BandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aBandwidthGroup))
#define INetworkAdapter_GetBandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aBandwidthGroup))
#define INetworkAdapter_put_BandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->SetBandwidthGroup(p, aBandwidthGroup))
#define INetworkAdapter_SetBandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->SetBandwidthGroup(p, aBandwidthGroup))
#define INetworkAdapter_GetProperty(p, aKey, aValue) ((p)->lpVtbl->GetProperty(p, aKey, aValue))
#define INetworkAdapter_SetProperty(p, aKey, aValue) ((p)->lpVtbl->SetProperty(p, aKey, aValue))
#define INetworkAdapter_GetProperties(p, aNames, aReturnNames, aReturnValues) ((p)->lpVtbl->GetProperties(p, aNames, aReturnNames, aReturnValues))
#endif /* VBOX_WITH_GLUE */

interface INetworkAdapter
{
#ifndef VBOX_WITH_GLUE
    struct INetworkAdapter_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INetworkAdapterVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INetworkAdapter declaration */


/* Start of struct ISerialPort declaration */
#define ISERIALPORT_IID_STR "937f6970-5103-4745-b78e-d28dcf1479a8"
#define ISERIALPORT_IID { \
    0x937f6970, 0x5103, 0x4745, \
    { 0xb7, 0x8e, 0xd2, 0x8d, 0xcf, 0x14, 0x79, 0xa8 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISerialPort);
#ifndef VBOX_WITH_GLUE
struct ISerialPort_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetSlot)(ISerialPort *pThis, PRUint32 *slot);

    nsresult (*GetEnabled)(ISerialPort *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(ISerialPort *pThis, PRBool enabled);

    nsresult (*GetIOBase)(ISerialPort *pThis, PRUint32 *IOBase);
    nsresult (*SetIOBase)(ISerialPort *pThis, PRUint32 IOBase);

    nsresult (*GetIRQ)(ISerialPort *pThis, PRUint32 *IRQ);
    nsresult (*SetIRQ)(ISerialPort *pThis, PRUint32 IRQ);

    nsresult (*GetHostMode)(ISerialPort *pThis, PRUint32 *hostMode);
    nsresult (*SetHostMode)(ISerialPort *pThis, PRUint32 hostMode);

    nsresult (*GetServer)(ISerialPort *pThis, PRBool *server);
    nsresult (*SetServer)(ISerialPort *pThis, PRBool server);

    nsresult (*GetPath)(ISerialPort *pThis, PRUnichar * *path);
    nsresult (*SetPath)(ISerialPort *pThis, PRUnichar * path);

};
#else /* VBOX_WITH_GLUE */
struct ISerialPortVtbl
{
    nsresult (*QueryInterface)(ISerialPort *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISerialPort *pThis);
    nsrefcnt (*Release)(ISerialPort *pThis);
    nsresult (*GetSlot)(ISerialPort *pThis, PRUint32 *slot);

    nsresult (*GetEnabled)(ISerialPort *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(ISerialPort *pThis, PRBool enabled);

    nsresult (*GetIOBase)(ISerialPort *pThis, PRUint32 *IOBase);
    nsresult (*SetIOBase)(ISerialPort *pThis, PRUint32 IOBase);

    nsresult (*GetIRQ)(ISerialPort *pThis, PRUint32 *IRQ);
    nsresult (*SetIRQ)(ISerialPort *pThis, PRUint32 IRQ);

    nsresult (*GetHostMode)(ISerialPort *pThis, PRUint32 *hostMode);
    nsresult (*SetHostMode)(ISerialPort *pThis, PRUint32 hostMode);

    nsresult (*GetServer)(ISerialPort *pThis, PRBool *server);
    nsresult (*SetServer)(ISerialPort *pThis, PRBool server);

    nsresult (*GetPath)(ISerialPort *pThis, PRUnichar * *path);
    nsresult (*SetPath)(ISerialPort *pThis, PRUnichar * path);

};
#define ISerialPort_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISerialPort_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISerialPort_Release(p) ((p)->lpVtbl->Release(p))
#define ISerialPort_get_Slot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define ISerialPort_GetSlot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define ISerialPort_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define ISerialPort_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define ISerialPort_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define ISerialPort_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define ISerialPort_get_IOBase(p, aIOBase) ((p)->lpVtbl->GetIOBase(p, aIOBase))
#define ISerialPort_GetIOBase(p, aIOBase) ((p)->lpVtbl->GetIOBase(p, aIOBase))
#define ISerialPort_put_IOBase(p, aIOBase) ((p)->lpVtbl->SetIOBase(p, aIOBase))
#define ISerialPort_SetIOBase(p, aIOBase) ((p)->lpVtbl->SetIOBase(p, aIOBase))
#define ISerialPort_get_IRQ(p, aIRQ) ((p)->lpVtbl->GetIRQ(p, aIRQ))
#define ISerialPort_GetIRQ(p, aIRQ) ((p)->lpVtbl->GetIRQ(p, aIRQ))
#define ISerialPort_put_IRQ(p, aIRQ) ((p)->lpVtbl->SetIRQ(p, aIRQ))
#define ISerialPort_SetIRQ(p, aIRQ) ((p)->lpVtbl->SetIRQ(p, aIRQ))
#define ISerialPort_get_HostMode(p, aHostMode) ((p)->lpVtbl->GetHostMode(p, aHostMode))
#define ISerialPort_GetHostMode(p, aHostMode) ((p)->lpVtbl->GetHostMode(p, aHostMode))
#define ISerialPort_put_HostMode(p, aHostMode) ((p)->lpVtbl->SetHostMode(p, aHostMode))
#define ISerialPort_SetHostMode(p, aHostMode) ((p)->lpVtbl->SetHostMode(p, aHostMode))
#define ISerialPort_get_Server(p, aServer) ((p)->lpVtbl->GetServer(p, aServer))
#define ISerialPort_GetServer(p, aServer) ((p)->lpVtbl->GetServer(p, aServer))
#define ISerialPort_put_Server(p, aServer) ((p)->lpVtbl->SetServer(p, aServer))
#define ISerialPort_SetServer(p, aServer) ((p)->lpVtbl->SetServer(p, aServer))
#define ISerialPort_get_Path(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define ISerialPort_GetPath(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define ISerialPort_put_Path(p, aPath) ((p)->lpVtbl->SetPath(p, aPath))
#define ISerialPort_SetPath(p, aPath) ((p)->lpVtbl->SetPath(p, aPath))
#endif /* VBOX_WITH_GLUE */

interface ISerialPort
{
#ifndef VBOX_WITH_GLUE
    struct ISerialPort_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISerialPortVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISerialPort declaration */


/* Start of struct IParallelPort declaration */
#define IPARALLELPORT_IID_STR "0c925f06-dd10-4b77-8de8-294d738c3214"
#define IPARALLELPORT_IID { \
    0x0c925f06, 0xdd10, 0x4b77, \
    { 0x8d, 0xe8, 0x29, 0x4d, 0x73, 0x8c, 0x32, 0x14 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IParallelPort);
#ifndef VBOX_WITH_GLUE
struct IParallelPort_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetSlot)(IParallelPort *pThis, PRUint32 *slot);

    nsresult (*GetEnabled)(IParallelPort *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IParallelPort *pThis, PRBool enabled);

    nsresult (*GetIOBase)(IParallelPort *pThis, PRUint32 *IOBase);
    nsresult (*SetIOBase)(IParallelPort *pThis, PRUint32 IOBase);

    nsresult (*GetIRQ)(IParallelPort *pThis, PRUint32 *IRQ);
    nsresult (*SetIRQ)(IParallelPort *pThis, PRUint32 IRQ);

    nsresult (*GetPath)(IParallelPort *pThis, PRUnichar * *path);
    nsresult (*SetPath)(IParallelPort *pThis, PRUnichar * path);

};
#else /* VBOX_WITH_GLUE */
struct IParallelPortVtbl
{
    nsresult (*QueryInterface)(IParallelPort *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IParallelPort *pThis);
    nsrefcnt (*Release)(IParallelPort *pThis);
    nsresult (*GetSlot)(IParallelPort *pThis, PRUint32 *slot);

    nsresult (*GetEnabled)(IParallelPort *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IParallelPort *pThis, PRBool enabled);

    nsresult (*GetIOBase)(IParallelPort *pThis, PRUint32 *IOBase);
    nsresult (*SetIOBase)(IParallelPort *pThis, PRUint32 IOBase);

    nsresult (*GetIRQ)(IParallelPort *pThis, PRUint32 *IRQ);
    nsresult (*SetIRQ)(IParallelPort *pThis, PRUint32 IRQ);

    nsresult (*GetPath)(IParallelPort *pThis, PRUnichar * *path);
    nsresult (*SetPath)(IParallelPort *pThis, PRUnichar * path);

};
#define IParallelPort_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IParallelPort_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IParallelPort_Release(p) ((p)->lpVtbl->Release(p))
#define IParallelPort_get_Slot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define IParallelPort_GetSlot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define IParallelPort_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IParallelPort_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IParallelPort_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IParallelPort_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IParallelPort_get_IOBase(p, aIOBase) ((p)->lpVtbl->GetIOBase(p, aIOBase))
#define IParallelPort_GetIOBase(p, aIOBase) ((p)->lpVtbl->GetIOBase(p, aIOBase))
#define IParallelPort_put_IOBase(p, aIOBase) ((p)->lpVtbl->SetIOBase(p, aIOBase))
#define IParallelPort_SetIOBase(p, aIOBase) ((p)->lpVtbl->SetIOBase(p, aIOBase))
#define IParallelPort_get_IRQ(p, aIRQ) ((p)->lpVtbl->GetIRQ(p, aIRQ))
#define IParallelPort_GetIRQ(p, aIRQ) ((p)->lpVtbl->GetIRQ(p, aIRQ))
#define IParallelPort_put_IRQ(p, aIRQ) ((p)->lpVtbl->SetIRQ(p, aIRQ))
#define IParallelPort_SetIRQ(p, aIRQ) ((p)->lpVtbl->SetIRQ(p, aIRQ))
#define IParallelPort_get_Path(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IParallelPort_GetPath(p, aPath) ((p)->lpVtbl->GetPath(p, aPath))
#define IParallelPort_put_Path(p, aPath) ((p)->lpVtbl->SetPath(p, aPath))
#define IParallelPort_SetPath(p, aPath) ((p)->lpVtbl->SetPath(p, aPath))
#endif /* VBOX_WITH_GLUE */

interface IParallelPort
{
#ifndef VBOX_WITH_GLUE
    struct IParallelPort_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IParallelPortVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IParallelPort declaration */


/* Start of struct IMachineDebugger declaration */
#define IMACHINEDEBUGGER_IID_STR "5e4534dc-21b8-4f6b-8a08-eef50e1a0aa1"
#define IMACHINEDEBUGGER_IID { \
    0x5e4534dc, 0x21b8, 0x4f6b, \
    { 0x8a, 0x08, 0xee, 0xf5, 0x0e, 0x1a, 0x0a, 0xa1 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMachineDebugger);
#ifndef VBOX_WITH_GLUE
struct IMachineDebugger_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetSingleStep)(IMachineDebugger *pThis, PRBool *singleStep);
    nsresult (*SetSingleStep)(IMachineDebugger *pThis, PRBool singleStep);

    nsresult (*GetRecompileUser)(IMachineDebugger *pThis, PRBool *recompileUser);
    nsresult (*SetRecompileUser)(IMachineDebugger *pThis, PRBool recompileUser);

    nsresult (*GetRecompileSupervisor)(IMachineDebugger *pThis, PRBool *recompileSupervisor);
    nsresult (*SetRecompileSupervisor)(IMachineDebugger *pThis, PRBool recompileSupervisor);

    nsresult (*GetExecuteAllInIEM)(IMachineDebugger *pThis, PRBool *executeAllInIEM);
    nsresult (*SetExecuteAllInIEM)(IMachineDebugger *pThis, PRBool executeAllInIEM);

    nsresult (*GetPATMEnabled)(IMachineDebugger *pThis, PRBool *PATMEnabled);
    nsresult (*SetPATMEnabled)(IMachineDebugger *pThis, PRBool PATMEnabled);

    nsresult (*GetCSAMEnabled)(IMachineDebugger *pThis, PRBool *CSAMEnabled);
    nsresult (*SetCSAMEnabled)(IMachineDebugger *pThis, PRBool CSAMEnabled);

    nsresult (*GetLogEnabled)(IMachineDebugger *pThis, PRBool *logEnabled);
    nsresult (*SetLogEnabled)(IMachineDebugger *pThis, PRBool logEnabled);

    nsresult (*GetLogDbgFlags)(IMachineDebugger *pThis, PRUnichar * *logDbgFlags);

    nsresult (*GetLogDbgGroups)(IMachineDebugger *pThis, PRUnichar * *logDbgGroups);

    nsresult (*GetLogDbgDestinations)(IMachineDebugger *pThis, PRUnichar * *logDbgDestinations);

    nsresult (*GetLogRelFlags)(IMachineDebugger *pThis, PRUnichar * *logRelFlags);

    nsresult (*GetLogRelGroups)(IMachineDebugger *pThis, PRUnichar * *logRelGroups);

    nsresult (*GetLogRelDestinations)(IMachineDebugger *pThis, PRUnichar * *logRelDestinations);

    nsresult (*GetHWVirtExEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExEnabled);

    nsresult (*GetHWVirtExNestedPagingEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExNestedPagingEnabled);

    nsresult (*GetHWVirtExVPIDEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExVPIDEnabled);

    nsresult (*GetHWVirtExUXEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExUXEnabled);

    nsresult (*GetOSName)(IMachineDebugger *pThis, PRUnichar * *OSName);

    nsresult (*GetOSVersion)(IMachineDebugger *pThis, PRUnichar * *OSVersion);

    nsresult (*GetPAEEnabled)(IMachineDebugger *pThis, PRBool *PAEEnabled);

    nsresult (*GetVirtualTimeRate)(IMachineDebugger *pThis, PRUint32 *virtualTimeRate);
    nsresult (*SetVirtualTimeRate)(IMachineDebugger *pThis, PRUint32 virtualTimeRate);

    nsresult (*GetVM)(IMachineDebugger *pThis, PRInt64 *VM);

    nsresult (*DumpGuestCore)(
        IMachineDebugger *pThis,
        PRUnichar * filename,
        PRUnichar * compression
    );

    nsresult (*DumpHostProcessCore)(
        IMachineDebugger *pThis,
        PRUnichar * filename,
        PRUnichar * compression
    );

    nsresult (*Info)(
        IMachineDebugger *pThis,
        PRUnichar * name,
        PRUnichar * args,
        PRUnichar * * info
    );

    nsresult (*InjectNMI)(IMachineDebugger *pThis );

    nsresult (*ModifyLogGroups)(
        IMachineDebugger *pThis,
        PRUnichar * settings
    );

    nsresult (*ModifyLogFlags)(
        IMachineDebugger *pThis,
        PRUnichar * settings
    );

    nsresult (*ModifyLogDestinations)(
        IMachineDebugger *pThis,
        PRUnichar * settings
    );

    nsresult (*ReadPhysicalMemory)(
        IMachineDebugger *pThis,
        PRInt64 address,
        PRUint32 size,
        PRUint32 *bytesSize,
        PRUint8** bytes
    );

    nsresult (*WritePhysicalMemory)(
        IMachineDebugger *pThis,
        PRInt64 address,
        PRUint32 size,
        PRUint32 bytesSize,
        PRUint8* bytes
    );

    nsresult (*ReadVirtualMemory)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRInt64 address,
        PRUint32 size,
        PRUint32 *bytesSize,
        PRUint8** bytes
    );

    nsresult (*WriteVirtualMemory)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRInt64 address,
        PRUint32 size,
        PRUint32 bytesSize,
        PRUint8* bytes
    );

    nsresult (*DetectOS)(
        IMachineDebugger *pThis,
        PRUnichar * * os
    );

    nsresult (*GetRegister)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUnichar * name,
        PRUnichar * * value
    );

    nsresult (*GetRegisters)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *valuesSize,
        PRUnichar *** values
    );

    nsresult (*SetRegister)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUnichar * name,
        PRUnichar * value
    );

    nsresult (*SetRegisters)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUint32 namesSize,
        PRUnichar ** names,
        PRUint32 valuesSize,
        PRUnichar ** values
    );

    nsresult (*DumpGuestStack)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUnichar * * stack
    );

    nsresult (*ResetStats)(
        IMachineDebugger *pThis,
        PRUnichar * pattern
    );

    nsresult (*DumpStats)(
        IMachineDebugger *pThis,
        PRUnichar * pattern
    );

    nsresult (*GetStats)(
        IMachineDebugger *pThis,
        PRUnichar * pattern,
        PRBool withDescriptions,
        PRUnichar * * stats
    );

};
#else /* VBOX_WITH_GLUE */
struct IMachineDebuggerVtbl
{
    nsresult (*QueryInterface)(IMachineDebugger *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMachineDebugger *pThis);
    nsrefcnt (*Release)(IMachineDebugger *pThis);
    nsresult (*GetSingleStep)(IMachineDebugger *pThis, PRBool *singleStep);
    nsresult (*SetSingleStep)(IMachineDebugger *pThis, PRBool singleStep);

    nsresult (*GetRecompileUser)(IMachineDebugger *pThis, PRBool *recompileUser);
    nsresult (*SetRecompileUser)(IMachineDebugger *pThis, PRBool recompileUser);

    nsresult (*GetRecompileSupervisor)(IMachineDebugger *pThis, PRBool *recompileSupervisor);
    nsresult (*SetRecompileSupervisor)(IMachineDebugger *pThis, PRBool recompileSupervisor);

    nsresult (*GetExecuteAllInIEM)(IMachineDebugger *pThis, PRBool *executeAllInIEM);
    nsresult (*SetExecuteAllInIEM)(IMachineDebugger *pThis, PRBool executeAllInIEM);

    nsresult (*GetPATMEnabled)(IMachineDebugger *pThis, PRBool *PATMEnabled);
    nsresult (*SetPATMEnabled)(IMachineDebugger *pThis, PRBool PATMEnabled);

    nsresult (*GetCSAMEnabled)(IMachineDebugger *pThis, PRBool *CSAMEnabled);
    nsresult (*SetCSAMEnabled)(IMachineDebugger *pThis, PRBool CSAMEnabled);

    nsresult (*GetLogEnabled)(IMachineDebugger *pThis, PRBool *logEnabled);
    nsresult (*SetLogEnabled)(IMachineDebugger *pThis, PRBool logEnabled);

    nsresult (*GetLogDbgFlags)(IMachineDebugger *pThis, PRUnichar * *logDbgFlags);

    nsresult (*GetLogDbgGroups)(IMachineDebugger *pThis, PRUnichar * *logDbgGroups);

    nsresult (*GetLogDbgDestinations)(IMachineDebugger *pThis, PRUnichar * *logDbgDestinations);

    nsresult (*GetLogRelFlags)(IMachineDebugger *pThis, PRUnichar * *logRelFlags);

    nsresult (*GetLogRelGroups)(IMachineDebugger *pThis, PRUnichar * *logRelGroups);

    nsresult (*GetLogRelDestinations)(IMachineDebugger *pThis, PRUnichar * *logRelDestinations);

    nsresult (*GetHWVirtExEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExEnabled);

    nsresult (*GetHWVirtExNestedPagingEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExNestedPagingEnabled);

    nsresult (*GetHWVirtExVPIDEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExVPIDEnabled);

    nsresult (*GetHWVirtExUXEnabled)(IMachineDebugger *pThis, PRBool *HWVirtExUXEnabled);

    nsresult (*GetOSName)(IMachineDebugger *pThis, PRUnichar * *OSName);

    nsresult (*GetOSVersion)(IMachineDebugger *pThis, PRUnichar * *OSVersion);

    nsresult (*GetPAEEnabled)(IMachineDebugger *pThis, PRBool *PAEEnabled);

    nsresult (*GetVirtualTimeRate)(IMachineDebugger *pThis, PRUint32 *virtualTimeRate);
    nsresult (*SetVirtualTimeRate)(IMachineDebugger *pThis, PRUint32 virtualTimeRate);

    nsresult (*GetVM)(IMachineDebugger *pThis, PRInt64 *VM);

    nsresult (*DumpGuestCore)(
        IMachineDebugger *pThis,
        PRUnichar * filename,
        PRUnichar * compression
    );

    nsresult (*DumpHostProcessCore)(
        IMachineDebugger *pThis,
        PRUnichar * filename,
        PRUnichar * compression
    );

    nsresult (*Info)(
        IMachineDebugger *pThis,
        PRUnichar * name,
        PRUnichar * args,
        PRUnichar * * info
    );

    nsresult (*InjectNMI)(IMachineDebugger *pThis );

    nsresult (*ModifyLogGroups)(
        IMachineDebugger *pThis,
        PRUnichar * settings
    );

    nsresult (*ModifyLogFlags)(
        IMachineDebugger *pThis,
        PRUnichar * settings
    );

    nsresult (*ModifyLogDestinations)(
        IMachineDebugger *pThis,
        PRUnichar * settings
    );

    nsresult (*ReadPhysicalMemory)(
        IMachineDebugger *pThis,
        PRInt64 address,
        PRUint32 size,
        PRUint32 *bytesSize,
        PRUint8** bytes
    );

    nsresult (*WritePhysicalMemory)(
        IMachineDebugger *pThis,
        PRInt64 address,
        PRUint32 size,
        PRUint32 bytesSize,
        PRUint8* bytes
    );

    nsresult (*ReadVirtualMemory)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRInt64 address,
        PRUint32 size,
        PRUint32 *bytesSize,
        PRUint8** bytes
    );

    nsresult (*WriteVirtualMemory)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRInt64 address,
        PRUint32 size,
        PRUint32 bytesSize,
        PRUint8* bytes
    );

    nsresult (*DetectOS)(
        IMachineDebugger *pThis,
        PRUnichar * * os
    );

    nsresult (*GetRegister)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUnichar * name,
        PRUnichar * * value
    );

    nsresult (*GetRegisters)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUint32 *namesSize,
        PRUnichar *** names,
        PRUint32 *valuesSize,
        PRUnichar *** values
    );

    nsresult (*SetRegister)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUnichar * name,
        PRUnichar * value
    );

    nsresult (*SetRegisters)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUint32 namesSize,
        PRUnichar ** names,
        PRUint32 valuesSize,
        PRUnichar ** values
    );

    nsresult (*DumpGuestStack)(
        IMachineDebugger *pThis,
        PRUint32 cpuId,
        PRUnichar * * stack
    );

    nsresult (*ResetStats)(
        IMachineDebugger *pThis,
        PRUnichar * pattern
    );

    nsresult (*DumpStats)(
        IMachineDebugger *pThis,
        PRUnichar * pattern
    );

    nsresult (*GetStats)(
        IMachineDebugger *pThis,
        PRUnichar * pattern,
        PRBool withDescriptions,
        PRUnichar * * stats
    );

};
#define IMachineDebugger_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMachineDebugger_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMachineDebugger_Release(p) ((p)->lpVtbl->Release(p))
#define IMachineDebugger_get_SingleStep(p, aSingleStep) ((p)->lpVtbl->GetSingleStep(p, aSingleStep))
#define IMachineDebugger_GetSingleStep(p, aSingleStep) ((p)->lpVtbl->GetSingleStep(p, aSingleStep))
#define IMachineDebugger_put_SingleStep(p, aSingleStep) ((p)->lpVtbl->SetSingleStep(p, aSingleStep))
#define IMachineDebugger_SetSingleStep(p, aSingleStep) ((p)->lpVtbl->SetSingleStep(p, aSingleStep))
#define IMachineDebugger_get_RecompileUser(p, aRecompileUser) ((p)->lpVtbl->GetRecompileUser(p, aRecompileUser))
#define IMachineDebugger_GetRecompileUser(p, aRecompileUser) ((p)->lpVtbl->GetRecompileUser(p, aRecompileUser))
#define IMachineDebugger_put_RecompileUser(p, aRecompileUser) ((p)->lpVtbl->SetRecompileUser(p, aRecompileUser))
#define IMachineDebugger_SetRecompileUser(p, aRecompileUser) ((p)->lpVtbl->SetRecompileUser(p, aRecompileUser))
#define IMachineDebugger_get_RecompileSupervisor(p, aRecompileSupervisor) ((p)->lpVtbl->GetRecompileSupervisor(p, aRecompileSupervisor))
#define IMachineDebugger_GetRecompileSupervisor(p, aRecompileSupervisor) ((p)->lpVtbl->GetRecompileSupervisor(p, aRecompileSupervisor))
#define IMachineDebugger_put_RecompileSupervisor(p, aRecompileSupervisor) ((p)->lpVtbl->SetRecompileSupervisor(p, aRecompileSupervisor))
#define IMachineDebugger_SetRecompileSupervisor(p, aRecompileSupervisor) ((p)->lpVtbl->SetRecompileSupervisor(p, aRecompileSupervisor))
#define IMachineDebugger_get_ExecuteAllInIEM(p, aExecuteAllInIEM) ((p)->lpVtbl->GetExecuteAllInIEM(p, aExecuteAllInIEM))
#define IMachineDebugger_GetExecuteAllInIEM(p, aExecuteAllInIEM) ((p)->lpVtbl->GetExecuteAllInIEM(p, aExecuteAllInIEM))
#define IMachineDebugger_put_ExecuteAllInIEM(p, aExecuteAllInIEM) ((p)->lpVtbl->SetExecuteAllInIEM(p, aExecuteAllInIEM))
#define IMachineDebugger_SetExecuteAllInIEM(p, aExecuteAllInIEM) ((p)->lpVtbl->SetExecuteAllInIEM(p, aExecuteAllInIEM))
#define IMachineDebugger_get_PATMEnabled(p, aPATMEnabled) ((p)->lpVtbl->GetPATMEnabled(p, aPATMEnabled))
#define IMachineDebugger_GetPATMEnabled(p, aPATMEnabled) ((p)->lpVtbl->GetPATMEnabled(p, aPATMEnabled))
#define IMachineDebugger_put_PATMEnabled(p, aPATMEnabled) ((p)->lpVtbl->SetPATMEnabled(p, aPATMEnabled))
#define IMachineDebugger_SetPATMEnabled(p, aPATMEnabled) ((p)->lpVtbl->SetPATMEnabled(p, aPATMEnabled))
#define IMachineDebugger_get_CSAMEnabled(p, aCSAMEnabled) ((p)->lpVtbl->GetCSAMEnabled(p, aCSAMEnabled))
#define IMachineDebugger_GetCSAMEnabled(p, aCSAMEnabled) ((p)->lpVtbl->GetCSAMEnabled(p, aCSAMEnabled))
#define IMachineDebugger_put_CSAMEnabled(p, aCSAMEnabled) ((p)->lpVtbl->SetCSAMEnabled(p, aCSAMEnabled))
#define IMachineDebugger_SetCSAMEnabled(p, aCSAMEnabled) ((p)->lpVtbl->SetCSAMEnabled(p, aCSAMEnabled))
#define IMachineDebugger_get_LogEnabled(p, aLogEnabled) ((p)->lpVtbl->GetLogEnabled(p, aLogEnabled))
#define IMachineDebugger_GetLogEnabled(p, aLogEnabled) ((p)->lpVtbl->GetLogEnabled(p, aLogEnabled))
#define IMachineDebugger_put_LogEnabled(p, aLogEnabled) ((p)->lpVtbl->SetLogEnabled(p, aLogEnabled))
#define IMachineDebugger_SetLogEnabled(p, aLogEnabled) ((p)->lpVtbl->SetLogEnabled(p, aLogEnabled))
#define IMachineDebugger_get_LogDbgFlags(p, aLogDbgFlags) ((p)->lpVtbl->GetLogDbgFlags(p, aLogDbgFlags))
#define IMachineDebugger_GetLogDbgFlags(p, aLogDbgFlags) ((p)->lpVtbl->GetLogDbgFlags(p, aLogDbgFlags))
#define IMachineDebugger_get_LogDbgGroups(p, aLogDbgGroups) ((p)->lpVtbl->GetLogDbgGroups(p, aLogDbgGroups))
#define IMachineDebugger_GetLogDbgGroups(p, aLogDbgGroups) ((p)->lpVtbl->GetLogDbgGroups(p, aLogDbgGroups))
#define IMachineDebugger_get_LogDbgDestinations(p, aLogDbgDestinations) ((p)->lpVtbl->GetLogDbgDestinations(p, aLogDbgDestinations))
#define IMachineDebugger_GetLogDbgDestinations(p, aLogDbgDestinations) ((p)->lpVtbl->GetLogDbgDestinations(p, aLogDbgDestinations))
#define IMachineDebugger_get_LogRelFlags(p, aLogRelFlags) ((p)->lpVtbl->GetLogRelFlags(p, aLogRelFlags))
#define IMachineDebugger_GetLogRelFlags(p, aLogRelFlags) ((p)->lpVtbl->GetLogRelFlags(p, aLogRelFlags))
#define IMachineDebugger_get_LogRelGroups(p, aLogRelGroups) ((p)->lpVtbl->GetLogRelGroups(p, aLogRelGroups))
#define IMachineDebugger_GetLogRelGroups(p, aLogRelGroups) ((p)->lpVtbl->GetLogRelGroups(p, aLogRelGroups))
#define IMachineDebugger_get_LogRelDestinations(p, aLogRelDestinations) ((p)->lpVtbl->GetLogRelDestinations(p, aLogRelDestinations))
#define IMachineDebugger_GetLogRelDestinations(p, aLogRelDestinations) ((p)->lpVtbl->GetLogRelDestinations(p, aLogRelDestinations))
#define IMachineDebugger_get_HWVirtExEnabled(p, aHWVirtExEnabled) ((p)->lpVtbl->GetHWVirtExEnabled(p, aHWVirtExEnabled))
#define IMachineDebugger_GetHWVirtExEnabled(p, aHWVirtExEnabled) ((p)->lpVtbl->GetHWVirtExEnabled(p, aHWVirtExEnabled))
#define IMachineDebugger_get_HWVirtExNestedPagingEnabled(p, aHWVirtExNestedPagingEnabled) ((p)->lpVtbl->GetHWVirtExNestedPagingEnabled(p, aHWVirtExNestedPagingEnabled))
#define IMachineDebugger_GetHWVirtExNestedPagingEnabled(p, aHWVirtExNestedPagingEnabled) ((p)->lpVtbl->GetHWVirtExNestedPagingEnabled(p, aHWVirtExNestedPagingEnabled))
#define IMachineDebugger_get_HWVirtExVPIDEnabled(p, aHWVirtExVPIDEnabled) ((p)->lpVtbl->GetHWVirtExVPIDEnabled(p, aHWVirtExVPIDEnabled))
#define IMachineDebugger_GetHWVirtExVPIDEnabled(p, aHWVirtExVPIDEnabled) ((p)->lpVtbl->GetHWVirtExVPIDEnabled(p, aHWVirtExVPIDEnabled))
#define IMachineDebugger_get_HWVirtExUXEnabled(p, aHWVirtExUXEnabled) ((p)->lpVtbl->GetHWVirtExUXEnabled(p, aHWVirtExUXEnabled))
#define IMachineDebugger_GetHWVirtExUXEnabled(p, aHWVirtExUXEnabled) ((p)->lpVtbl->GetHWVirtExUXEnabled(p, aHWVirtExUXEnabled))
#define IMachineDebugger_get_OSName(p, aOSName) ((p)->lpVtbl->GetOSName(p, aOSName))
#define IMachineDebugger_GetOSName(p, aOSName) ((p)->lpVtbl->GetOSName(p, aOSName))
#define IMachineDebugger_get_OSVersion(p, aOSVersion) ((p)->lpVtbl->GetOSVersion(p, aOSVersion))
#define IMachineDebugger_GetOSVersion(p, aOSVersion) ((p)->lpVtbl->GetOSVersion(p, aOSVersion))
#define IMachineDebugger_get_PAEEnabled(p, aPAEEnabled) ((p)->lpVtbl->GetPAEEnabled(p, aPAEEnabled))
#define IMachineDebugger_GetPAEEnabled(p, aPAEEnabled) ((p)->lpVtbl->GetPAEEnabled(p, aPAEEnabled))
#define IMachineDebugger_get_VirtualTimeRate(p, aVirtualTimeRate) ((p)->lpVtbl->GetVirtualTimeRate(p, aVirtualTimeRate))
#define IMachineDebugger_GetVirtualTimeRate(p, aVirtualTimeRate) ((p)->lpVtbl->GetVirtualTimeRate(p, aVirtualTimeRate))
#define IMachineDebugger_put_VirtualTimeRate(p, aVirtualTimeRate) ((p)->lpVtbl->SetVirtualTimeRate(p, aVirtualTimeRate))
#define IMachineDebugger_SetVirtualTimeRate(p, aVirtualTimeRate) ((p)->lpVtbl->SetVirtualTimeRate(p, aVirtualTimeRate))
#define IMachineDebugger_get_VM(p, aVM) ((p)->lpVtbl->GetVM(p, aVM))
#define IMachineDebugger_GetVM(p, aVM) ((p)->lpVtbl->GetVM(p, aVM))
#define IMachineDebugger_DumpGuestCore(p, aFilename, aCompression) ((p)->lpVtbl->DumpGuestCore(p, aFilename, aCompression))
#define IMachineDebugger_DumpHostProcessCore(p, aFilename, aCompression) ((p)->lpVtbl->DumpHostProcessCore(p, aFilename, aCompression))
#define IMachineDebugger_Info(p, aName, aArgs, aInfo) ((p)->lpVtbl->Info(p, aName, aArgs, aInfo))
#define IMachineDebugger_InjectNMI(p) ((p)->lpVtbl->InjectNMI(p))
#define IMachineDebugger_ModifyLogGroups(p, aSettings) ((p)->lpVtbl->ModifyLogGroups(p, aSettings))
#define IMachineDebugger_ModifyLogFlags(p, aSettings) ((p)->lpVtbl->ModifyLogFlags(p, aSettings))
#define IMachineDebugger_ModifyLogDestinations(p, aSettings) ((p)->lpVtbl->ModifyLogDestinations(p, aSettings))
#define IMachineDebugger_ReadPhysicalMemory(p, aAddress, aSize, aBytes) ((p)->lpVtbl->ReadPhysicalMemory(p, aAddress, aSize, aBytes))
#define IMachineDebugger_WritePhysicalMemory(p, aAddress, aSize, aBytes) ((p)->lpVtbl->WritePhysicalMemory(p, aAddress, aSize, aBytes))
#define IMachineDebugger_ReadVirtualMemory(p, aCpuId, aAddress, aSize, aBytes) ((p)->lpVtbl->ReadVirtualMemory(p, aCpuId, aAddress, aSize, aBytes))
#define IMachineDebugger_WriteVirtualMemory(p, aCpuId, aAddress, aSize, aBytes) ((p)->lpVtbl->WriteVirtualMemory(p, aCpuId, aAddress, aSize, aBytes))
#define IMachineDebugger_DetectOS(p, aOs) ((p)->lpVtbl->DetectOS(p, aOs))
#define IMachineDebugger_GetRegister(p, aCpuId, aName, aValue) ((p)->lpVtbl->GetRegister(p, aCpuId, aName, aValue))
#define IMachineDebugger_GetRegisters(p, aCpuId, aNames, aValues) ((p)->lpVtbl->GetRegisters(p, aCpuId, aNames, aValues))
#define IMachineDebugger_SetRegister(p, aCpuId, aName, aValue) ((p)->lpVtbl->SetRegister(p, aCpuId, aName, aValue))
#define IMachineDebugger_SetRegisters(p, aCpuId, aNames, aValues) ((p)->lpVtbl->SetRegisters(p, aCpuId, aNames, aValues))
#define IMachineDebugger_DumpGuestStack(p, aCpuId, aStack) ((p)->lpVtbl->DumpGuestStack(p, aCpuId, aStack))
#define IMachineDebugger_ResetStats(p, aPattern) ((p)->lpVtbl->ResetStats(p, aPattern))
#define IMachineDebugger_DumpStats(p, aPattern) ((p)->lpVtbl->DumpStats(p, aPattern))
#define IMachineDebugger_GetStats(p, aPattern, aWithDescriptions, aStats) ((p)->lpVtbl->GetStats(p, aPattern, aWithDescriptions, aStats))
#endif /* VBOX_WITH_GLUE */

interface IMachineDebugger
{
#ifndef VBOX_WITH_GLUE
    struct IMachineDebugger_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMachineDebuggerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMachineDebugger declaration */


/* Start of struct IUSBDeviceFilters declaration */
#define IUSBDEVICEFILTERS_IID_STR "2ab550b2-53cc-4c2e-ae07-0adf4114e75c"
#define IUSBDEVICEFILTERS_IID { \
    0x2ab550b2, 0x53cc, 0x4c2e, \
    { 0xae, 0x07, 0x0a, 0xdf, 0x41, 0x14, 0xe7, 0x5c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IUSBDeviceFilters);
#ifndef VBOX_WITH_GLUE
struct IUSBDeviceFilters_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetDeviceFilters)(IUSBDeviceFilters *pThis, PRUint32 *deviceFiltersSize, IUSBDeviceFilter * **deviceFilters);

    nsresult (*CreateDeviceFilter)(
        IUSBDeviceFilters *pThis,
        PRUnichar * name,
        IUSBDeviceFilter * * filter
    );

    nsresult (*InsertDeviceFilter)(
        IUSBDeviceFilters *pThis,
        PRUint32 position,
        IUSBDeviceFilter * filter
    );

    nsresult (*RemoveDeviceFilter)(
        IUSBDeviceFilters *pThis,
        PRUint32 position,
        IUSBDeviceFilter * * filter
    );

};
#else /* VBOX_WITH_GLUE */
struct IUSBDeviceFiltersVtbl
{
    nsresult (*QueryInterface)(IUSBDeviceFilters *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IUSBDeviceFilters *pThis);
    nsrefcnt (*Release)(IUSBDeviceFilters *pThis);
    nsresult (*GetDeviceFilters)(IUSBDeviceFilters *pThis, PRUint32 *deviceFiltersSize, IUSBDeviceFilter * **deviceFilters);

    nsresult (*CreateDeviceFilter)(
        IUSBDeviceFilters *pThis,
        PRUnichar * name,
        IUSBDeviceFilter * * filter
    );

    nsresult (*InsertDeviceFilter)(
        IUSBDeviceFilters *pThis,
        PRUint32 position,
        IUSBDeviceFilter * filter
    );

    nsresult (*RemoveDeviceFilter)(
        IUSBDeviceFilters *pThis,
        PRUint32 position,
        IUSBDeviceFilter * * filter
    );

};
#define IUSBDeviceFilters_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUSBDeviceFilters_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUSBDeviceFilters_Release(p) ((p)->lpVtbl->Release(p))
#define IUSBDeviceFilters_get_DeviceFilters(p, aDeviceFilters) ((p)->lpVtbl->GetDeviceFilters(p, aDeviceFilters))
#define IUSBDeviceFilters_GetDeviceFilters(p, aDeviceFilters) ((p)->lpVtbl->GetDeviceFilters(p, aDeviceFilters))
#define IUSBDeviceFilters_CreateDeviceFilter(p, aName, aFilter) ((p)->lpVtbl->CreateDeviceFilter(p, aName, aFilter))
#define IUSBDeviceFilters_InsertDeviceFilter(p, aPosition, aFilter) ((p)->lpVtbl->InsertDeviceFilter(p, aPosition, aFilter))
#define IUSBDeviceFilters_RemoveDeviceFilter(p, aPosition, aFilter) ((p)->lpVtbl->RemoveDeviceFilter(p, aPosition, aFilter))
#endif /* VBOX_WITH_GLUE */

interface IUSBDeviceFilters
{
#ifndef VBOX_WITH_GLUE
    struct IUSBDeviceFilters_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IUSBDeviceFiltersVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IUSBDeviceFilters declaration */


/* Start of struct IUSBController declaration */
#define IUSBCONTROLLER_IID_STR "d2745291-65f7-4d75-9556-38047d802319"
#define IUSBCONTROLLER_IID { \
    0xd2745291, 0x65f7, 0x4d75, \
    { 0x95, 0x56, 0x38, 0x04, 0x7d, 0x80, 0x23, 0x19 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IUSBController);
#ifndef VBOX_WITH_GLUE
struct IUSBController_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IUSBController *pThis, PRUnichar * *name);

    nsresult (*GetType)(IUSBController *pThis, PRUint32 *type);

    nsresult (*GetUSBStandard)(IUSBController *pThis, PRUint16 *USBStandard);

};
#else /* VBOX_WITH_GLUE */
struct IUSBControllerVtbl
{
    nsresult (*QueryInterface)(IUSBController *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IUSBController *pThis);
    nsrefcnt (*Release)(IUSBController *pThis);
    nsresult (*GetName)(IUSBController *pThis, PRUnichar * *name);

    nsresult (*GetType)(IUSBController *pThis, PRUint32 *type);

    nsresult (*GetUSBStandard)(IUSBController *pThis, PRUint16 *USBStandard);

};
#define IUSBController_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUSBController_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUSBController_Release(p) ((p)->lpVtbl->Release(p))
#define IUSBController_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IUSBController_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IUSBController_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IUSBController_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IUSBController_get_USBStandard(p, aUSBStandard) ((p)->lpVtbl->GetUSBStandard(p, aUSBStandard))
#define IUSBController_GetUSBStandard(p, aUSBStandard) ((p)->lpVtbl->GetUSBStandard(p, aUSBStandard))
#endif /* VBOX_WITH_GLUE */

interface IUSBController
{
#ifndef VBOX_WITH_GLUE
    struct IUSBController_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IUSBControllerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IUSBController declaration */


/* Start of struct IUSBDevice declaration */
#define IUSBDEVICE_IID_STR "f8967b0b-4483-400f-92b5-8b675d98a85b"
#define IUSBDEVICE_IID { \
    0xf8967b0b, 0x4483, 0x400f, \
    { 0x92, 0xb5, 0x8b, 0x67, 0x5d, 0x98, 0xa8, 0x5b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IUSBDevice);
#ifndef VBOX_WITH_GLUE
struct IUSBDevice_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetId)(IUSBDevice *pThis, PRUnichar * *id);

    nsresult (*GetVendorId)(IUSBDevice *pThis, PRUint16 *vendorId);

    nsresult (*GetProductId)(IUSBDevice *pThis, PRUint16 *productId);

    nsresult (*GetRevision)(IUSBDevice *pThis, PRUint16 *revision);

    nsresult (*GetManufacturer)(IUSBDevice *pThis, PRUnichar * *manufacturer);

    nsresult (*GetProduct)(IUSBDevice *pThis, PRUnichar * *product);

    nsresult (*GetSerialNumber)(IUSBDevice *pThis, PRUnichar * *serialNumber);

    nsresult (*GetAddress)(IUSBDevice *pThis, PRUnichar * *address);

    nsresult (*GetPort)(IUSBDevice *pThis, PRUint16 *port);

    nsresult (*GetVersion)(IUSBDevice *pThis, PRUint16 *version);

    nsresult (*GetPortVersion)(IUSBDevice *pThis, PRUint16 *portVersion);

    nsresult (*GetRemote)(IUSBDevice *pThis, PRBool *remote);

};
#else /* VBOX_WITH_GLUE */
struct IUSBDeviceVtbl
{
    nsresult (*QueryInterface)(IUSBDevice *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IUSBDevice *pThis);
    nsrefcnt (*Release)(IUSBDevice *pThis);
    nsresult (*GetId)(IUSBDevice *pThis, PRUnichar * *id);

    nsresult (*GetVendorId)(IUSBDevice *pThis, PRUint16 *vendorId);

    nsresult (*GetProductId)(IUSBDevice *pThis, PRUint16 *productId);

    nsresult (*GetRevision)(IUSBDevice *pThis, PRUint16 *revision);

    nsresult (*GetManufacturer)(IUSBDevice *pThis, PRUnichar * *manufacturer);

    nsresult (*GetProduct)(IUSBDevice *pThis, PRUnichar * *product);

    nsresult (*GetSerialNumber)(IUSBDevice *pThis, PRUnichar * *serialNumber);

    nsresult (*GetAddress)(IUSBDevice *pThis, PRUnichar * *address);

    nsresult (*GetPort)(IUSBDevice *pThis, PRUint16 *port);

    nsresult (*GetVersion)(IUSBDevice *pThis, PRUint16 *version);

    nsresult (*GetPortVersion)(IUSBDevice *pThis, PRUint16 *portVersion);

    nsresult (*GetRemote)(IUSBDevice *pThis, PRBool *remote);

};
#define IUSBDevice_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUSBDevice_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUSBDevice_Release(p) ((p)->lpVtbl->Release(p))
#define IUSBDevice_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IUSBDevice_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IUSBDevice_get_VendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IUSBDevice_GetVendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IUSBDevice_get_ProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IUSBDevice_GetProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IUSBDevice_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IUSBDevice_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IUSBDevice_get_Manufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IUSBDevice_GetManufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IUSBDevice_get_Product(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IUSBDevice_GetProduct(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IUSBDevice_get_SerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IUSBDevice_GetSerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IUSBDevice_get_Address(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IUSBDevice_GetAddress(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IUSBDevice_get_Port(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IUSBDevice_GetPort(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IUSBDevice_get_Version(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IUSBDevice_GetVersion(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IUSBDevice_get_PortVersion(p, aPortVersion) ((p)->lpVtbl->GetPortVersion(p, aPortVersion))
#define IUSBDevice_GetPortVersion(p, aPortVersion) ((p)->lpVtbl->GetPortVersion(p, aPortVersion))
#define IUSBDevice_get_Remote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IUSBDevice_GetRemote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#endif /* VBOX_WITH_GLUE */

interface IUSBDevice
{
#ifndef VBOX_WITH_GLUE
    struct IUSBDevice_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IUSBDeviceVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IUSBDevice declaration */


/* Start of struct IUSBDeviceFilter declaration */
#define IUSBDEVICEFILTER_IID_STR "d6831fb4-1a94-4c2c-96ef-8d0d6192066d"
#define IUSBDEVICEFILTER_IID { \
    0xd6831fb4, 0x1a94, 0x4c2c, \
    { 0x96, 0xef, 0x8d, 0x0d, 0x61, 0x92, 0x06, 0x6d } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IUSBDeviceFilter);
#ifndef VBOX_WITH_GLUE
struct IUSBDeviceFilter_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IUSBDeviceFilter *pThis, PRUnichar * *name);
    nsresult (*SetName)(IUSBDeviceFilter *pThis, PRUnichar * name);

    nsresult (*GetActive)(IUSBDeviceFilter *pThis, PRBool *active);
    nsresult (*SetActive)(IUSBDeviceFilter *pThis, PRBool active);

    nsresult (*GetVendorId)(IUSBDeviceFilter *pThis, PRUnichar * *vendorId);
    nsresult (*SetVendorId)(IUSBDeviceFilter *pThis, PRUnichar * vendorId);

    nsresult (*GetProductId)(IUSBDeviceFilter *pThis, PRUnichar * *productId);
    nsresult (*SetProductId)(IUSBDeviceFilter *pThis, PRUnichar * productId);

    nsresult (*GetRevision)(IUSBDeviceFilter *pThis, PRUnichar * *revision);
    nsresult (*SetRevision)(IUSBDeviceFilter *pThis, PRUnichar * revision);

    nsresult (*GetManufacturer)(IUSBDeviceFilter *pThis, PRUnichar * *manufacturer);
    nsresult (*SetManufacturer)(IUSBDeviceFilter *pThis, PRUnichar * manufacturer);

    nsresult (*GetProduct)(IUSBDeviceFilter *pThis, PRUnichar * *product);
    nsresult (*SetProduct)(IUSBDeviceFilter *pThis, PRUnichar * product);

    nsresult (*GetSerialNumber)(IUSBDeviceFilter *pThis, PRUnichar * *serialNumber);
    nsresult (*SetSerialNumber)(IUSBDeviceFilter *pThis, PRUnichar * serialNumber);

    nsresult (*GetPort)(IUSBDeviceFilter *pThis, PRUnichar * *port);
    nsresult (*SetPort)(IUSBDeviceFilter *pThis, PRUnichar * port);

    nsresult (*GetRemote)(IUSBDeviceFilter *pThis, PRUnichar * *remote);
    nsresult (*SetRemote)(IUSBDeviceFilter *pThis, PRUnichar * remote);

    nsresult (*GetMaskedInterfaces)(IUSBDeviceFilter *pThis, PRUint32 *maskedInterfaces);
    nsresult (*SetMaskedInterfaces)(IUSBDeviceFilter *pThis, PRUint32 maskedInterfaces);

};
#else /* VBOX_WITH_GLUE */
struct IUSBDeviceFilterVtbl
{
    nsresult (*QueryInterface)(IUSBDeviceFilter *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IUSBDeviceFilter *pThis);
    nsrefcnt (*Release)(IUSBDeviceFilter *pThis);
    nsresult (*GetName)(IUSBDeviceFilter *pThis, PRUnichar * *name);
    nsresult (*SetName)(IUSBDeviceFilter *pThis, PRUnichar * name);

    nsresult (*GetActive)(IUSBDeviceFilter *pThis, PRBool *active);
    nsresult (*SetActive)(IUSBDeviceFilter *pThis, PRBool active);

    nsresult (*GetVendorId)(IUSBDeviceFilter *pThis, PRUnichar * *vendorId);
    nsresult (*SetVendorId)(IUSBDeviceFilter *pThis, PRUnichar * vendorId);

    nsresult (*GetProductId)(IUSBDeviceFilter *pThis, PRUnichar * *productId);
    nsresult (*SetProductId)(IUSBDeviceFilter *pThis, PRUnichar * productId);

    nsresult (*GetRevision)(IUSBDeviceFilter *pThis, PRUnichar * *revision);
    nsresult (*SetRevision)(IUSBDeviceFilter *pThis, PRUnichar * revision);

    nsresult (*GetManufacturer)(IUSBDeviceFilter *pThis, PRUnichar * *manufacturer);
    nsresult (*SetManufacturer)(IUSBDeviceFilter *pThis, PRUnichar * manufacturer);

    nsresult (*GetProduct)(IUSBDeviceFilter *pThis, PRUnichar * *product);
    nsresult (*SetProduct)(IUSBDeviceFilter *pThis, PRUnichar * product);

    nsresult (*GetSerialNumber)(IUSBDeviceFilter *pThis, PRUnichar * *serialNumber);
    nsresult (*SetSerialNumber)(IUSBDeviceFilter *pThis, PRUnichar * serialNumber);

    nsresult (*GetPort)(IUSBDeviceFilter *pThis, PRUnichar * *port);
    nsresult (*SetPort)(IUSBDeviceFilter *pThis, PRUnichar * port);

    nsresult (*GetRemote)(IUSBDeviceFilter *pThis, PRUnichar * *remote);
    nsresult (*SetRemote)(IUSBDeviceFilter *pThis, PRUnichar * remote);

    nsresult (*GetMaskedInterfaces)(IUSBDeviceFilter *pThis, PRUint32 *maskedInterfaces);
    nsresult (*SetMaskedInterfaces)(IUSBDeviceFilter *pThis, PRUint32 maskedInterfaces);

};
#define IUSBDeviceFilter_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUSBDeviceFilter_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUSBDeviceFilter_Release(p) ((p)->lpVtbl->Release(p))
#define IUSBDeviceFilter_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IUSBDeviceFilter_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IUSBDeviceFilter_put_Name(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define IUSBDeviceFilter_SetName(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define IUSBDeviceFilter_get_Active(p, aActive) ((p)->lpVtbl->GetActive(p, aActive))
#define IUSBDeviceFilter_GetActive(p, aActive) ((p)->lpVtbl->GetActive(p, aActive))
#define IUSBDeviceFilter_put_Active(p, aActive) ((p)->lpVtbl->SetActive(p, aActive))
#define IUSBDeviceFilter_SetActive(p, aActive) ((p)->lpVtbl->SetActive(p, aActive))
#define IUSBDeviceFilter_get_VendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IUSBDeviceFilter_GetVendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IUSBDeviceFilter_put_VendorId(p, aVendorId) ((p)->lpVtbl->SetVendorId(p, aVendorId))
#define IUSBDeviceFilter_SetVendorId(p, aVendorId) ((p)->lpVtbl->SetVendorId(p, aVendorId))
#define IUSBDeviceFilter_get_ProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IUSBDeviceFilter_GetProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IUSBDeviceFilter_put_ProductId(p, aProductId) ((p)->lpVtbl->SetProductId(p, aProductId))
#define IUSBDeviceFilter_SetProductId(p, aProductId) ((p)->lpVtbl->SetProductId(p, aProductId))
#define IUSBDeviceFilter_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IUSBDeviceFilter_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IUSBDeviceFilter_put_Revision(p, aRevision) ((p)->lpVtbl->SetRevision(p, aRevision))
#define IUSBDeviceFilter_SetRevision(p, aRevision) ((p)->lpVtbl->SetRevision(p, aRevision))
#define IUSBDeviceFilter_get_Manufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IUSBDeviceFilter_GetManufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IUSBDeviceFilter_put_Manufacturer(p, aManufacturer) ((p)->lpVtbl->SetManufacturer(p, aManufacturer))
#define IUSBDeviceFilter_SetManufacturer(p, aManufacturer) ((p)->lpVtbl->SetManufacturer(p, aManufacturer))
#define IUSBDeviceFilter_get_Product(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IUSBDeviceFilter_GetProduct(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IUSBDeviceFilter_put_Product(p, aProduct) ((p)->lpVtbl->SetProduct(p, aProduct))
#define IUSBDeviceFilter_SetProduct(p, aProduct) ((p)->lpVtbl->SetProduct(p, aProduct))
#define IUSBDeviceFilter_get_SerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IUSBDeviceFilter_GetSerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IUSBDeviceFilter_put_SerialNumber(p, aSerialNumber) ((p)->lpVtbl->SetSerialNumber(p, aSerialNumber))
#define IUSBDeviceFilter_SetSerialNumber(p, aSerialNumber) ((p)->lpVtbl->SetSerialNumber(p, aSerialNumber))
#define IUSBDeviceFilter_get_Port(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IUSBDeviceFilter_GetPort(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IUSBDeviceFilter_put_Port(p, aPort) ((p)->lpVtbl->SetPort(p, aPort))
#define IUSBDeviceFilter_SetPort(p, aPort) ((p)->lpVtbl->SetPort(p, aPort))
#define IUSBDeviceFilter_get_Remote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IUSBDeviceFilter_GetRemote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IUSBDeviceFilter_put_Remote(p, aRemote) ((p)->lpVtbl->SetRemote(p, aRemote))
#define IUSBDeviceFilter_SetRemote(p, aRemote) ((p)->lpVtbl->SetRemote(p, aRemote))
#define IUSBDeviceFilter_get_MaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->GetMaskedInterfaces(p, aMaskedInterfaces))
#define IUSBDeviceFilter_GetMaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->GetMaskedInterfaces(p, aMaskedInterfaces))
#define IUSBDeviceFilter_put_MaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->SetMaskedInterfaces(p, aMaskedInterfaces))
#define IUSBDeviceFilter_SetMaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->SetMaskedInterfaces(p, aMaskedInterfaces))
#endif /* VBOX_WITH_GLUE */

interface IUSBDeviceFilter
{
#ifndef VBOX_WITH_GLUE
    struct IUSBDeviceFilter_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IUSBDeviceFilterVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IUSBDeviceFilter declaration */


/* Start of struct IHostUSBDevice declaration */
#define IHOSTUSBDEVICE_IID_STR "173b4b44-d268-4334-a00d-b6521c9a740a"
#define IHOSTUSBDEVICE_IID { \
    0x173b4b44, 0xd268, 0x4334, \
    { 0xa0, 0x0d, 0xb6, 0x52, 0x1c, 0x9a, 0x74, 0x0a } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHostUSBDevice);
#ifndef VBOX_WITH_GLUE
struct IHostUSBDevice_vtbl
{
    struct IUSBDevice_vtbl iusbdevice;

    nsresult (*GetState)(IHostUSBDevice *pThis, PRUint32 *state);

};
#else /* VBOX_WITH_GLUE */
struct IHostUSBDeviceVtbl
{
    nsresult (*QueryInterface)(IHostUSBDevice *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHostUSBDevice *pThis);
    nsrefcnt (*Release)(IHostUSBDevice *pThis);
    nsresult (*GetId)(IHostUSBDevice *pThis, PRUnichar * *id);

    nsresult (*GetVendorId)(IHostUSBDevice *pThis, PRUint16 *vendorId);

    nsresult (*GetProductId)(IHostUSBDevice *pThis, PRUint16 *productId);

    nsresult (*GetRevision)(IHostUSBDevice *pThis, PRUint16 *revision);

    nsresult (*GetManufacturer)(IHostUSBDevice *pThis, PRUnichar * *manufacturer);

    nsresult (*GetProduct)(IHostUSBDevice *pThis, PRUnichar * *product);

    nsresult (*GetSerialNumber)(IHostUSBDevice *pThis, PRUnichar * *serialNumber);

    nsresult (*GetAddress)(IHostUSBDevice *pThis, PRUnichar * *address);

    nsresult (*GetPort)(IHostUSBDevice *pThis, PRUint16 *port);

    nsresult (*GetVersion)(IHostUSBDevice *pThis, PRUint16 *version);

    nsresult (*GetPortVersion)(IHostUSBDevice *pThis, PRUint16 *portVersion);

    nsresult (*GetRemote)(IHostUSBDevice *pThis, PRBool *remote);

    nsresult (*GetState)(IHostUSBDevice *pThis, PRUint32 *state);

};
#define IHostUSBDevice_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHostUSBDevice_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHostUSBDevice_Release(p) ((p)->lpVtbl->Release(p))
#define IHostUSBDevice_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IHostUSBDevice_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IHostUSBDevice_get_VendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IHostUSBDevice_GetVendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IHostUSBDevice_get_ProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IHostUSBDevice_GetProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IHostUSBDevice_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IHostUSBDevice_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IHostUSBDevice_get_Manufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IHostUSBDevice_GetManufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IHostUSBDevice_get_Product(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IHostUSBDevice_GetProduct(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IHostUSBDevice_get_SerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IHostUSBDevice_GetSerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IHostUSBDevice_get_Address(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IHostUSBDevice_GetAddress(p, aAddress) ((p)->lpVtbl->GetAddress(p, aAddress))
#define IHostUSBDevice_get_Port(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IHostUSBDevice_GetPort(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IHostUSBDevice_get_Version(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IHostUSBDevice_GetVersion(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IHostUSBDevice_get_PortVersion(p, aPortVersion) ((p)->lpVtbl->GetPortVersion(p, aPortVersion))
#define IHostUSBDevice_GetPortVersion(p, aPortVersion) ((p)->lpVtbl->GetPortVersion(p, aPortVersion))
#define IHostUSBDevice_get_Remote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IHostUSBDevice_GetRemote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IHostUSBDevice_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IHostUSBDevice_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#endif /* VBOX_WITH_GLUE */

interface IHostUSBDevice
{
#ifndef VBOX_WITH_GLUE
    struct IHostUSBDevice_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostUSBDeviceVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHostUSBDevice declaration */


/* Start of struct IHostUSBDeviceFilter declaration */
#define IHOSTUSBDEVICEFILTER_IID_STR "4cc70246-d74a-400f-8222-3900489c0374"
#define IHOSTUSBDEVICEFILTER_IID { \
    0x4cc70246, 0xd74a, 0x400f, \
    { 0x82, 0x22, 0x39, 0x00, 0x48, 0x9c, 0x03, 0x74 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHostUSBDeviceFilter);
#ifndef VBOX_WITH_GLUE
struct IHostUSBDeviceFilter_vtbl
{
    struct IUSBDeviceFilter_vtbl iusbdevicefilter;

    nsresult (*GetAction)(IHostUSBDeviceFilter *pThis, PRUint32 *action);
    nsresult (*SetAction)(IHostUSBDeviceFilter *pThis, PRUint32 action);

};
#else /* VBOX_WITH_GLUE */
struct IHostUSBDeviceFilterVtbl
{
    nsresult (*QueryInterface)(IHostUSBDeviceFilter *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHostUSBDeviceFilter *pThis);
    nsrefcnt (*Release)(IHostUSBDeviceFilter *pThis);
    nsresult (*GetName)(IHostUSBDeviceFilter *pThis, PRUnichar * *name);
    nsresult (*SetName)(IHostUSBDeviceFilter *pThis, PRUnichar * name);

    nsresult (*GetActive)(IHostUSBDeviceFilter *pThis, PRBool *active);
    nsresult (*SetActive)(IHostUSBDeviceFilter *pThis, PRBool active);

    nsresult (*GetVendorId)(IHostUSBDeviceFilter *pThis, PRUnichar * *vendorId);
    nsresult (*SetVendorId)(IHostUSBDeviceFilter *pThis, PRUnichar * vendorId);

    nsresult (*GetProductId)(IHostUSBDeviceFilter *pThis, PRUnichar * *productId);
    nsresult (*SetProductId)(IHostUSBDeviceFilter *pThis, PRUnichar * productId);

    nsresult (*GetRevision)(IHostUSBDeviceFilter *pThis, PRUnichar * *revision);
    nsresult (*SetRevision)(IHostUSBDeviceFilter *pThis, PRUnichar * revision);

    nsresult (*GetManufacturer)(IHostUSBDeviceFilter *pThis, PRUnichar * *manufacturer);
    nsresult (*SetManufacturer)(IHostUSBDeviceFilter *pThis, PRUnichar * manufacturer);

    nsresult (*GetProduct)(IHostUSBDeviceFilter *pThis, PRUnichar * *product);
    nsresult (*SetProduct)(IHostUSBDeviceFilter *pThis, PRUnichar * product);

    nsresult (*GetSerialNumber)(IHostUSBDeviceFilter *pThis, PRUnichar * *serialNumber);
    nsresult (*SetSerialNumber)(IHostUSBDeviceFilter *pThis, PRUnichar * serialNumber);

    nsresult (*GetPort)(IHostUSBDeviceFilter *pThis, PRUnichar * *port);
    nsresult (*SetPort)(IHostUSBDeviceFilter *pThis, PRUnichar * port);

    nsresult (*GetRemote)(IHostUSBDeviceFilter *pThis, PRUnichar * *remote);
    nsresult (*SetRemote)(IHostUSBDeviceFilter *pThis, PRUnichar * remote);

    nsresult (*GetMaskedInterfaces)(IHostUSBDeviceFilter *pThis, PRUint32 *maskedInterfaces);
    nsresult (*SetMaskedInterfaces)(IHostUSBDeviceFilter *pThis, PRUint32 maskedInterfaces);

    nsresult (*GetAction)(IHostUSBDeviceFilter *pThis, PRUint32 *action);
    nsresult (*SetAction)(IHostUSBDeviceFilter *pThis, PRUint32 action);

};
#define IHostUSBDeviceFilter_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHostUSBDeviceFilter_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHostUSBDeviceFilter_Release(p) ((p)->lpVtbl->Release(p))
#define IHostUSBDeviceFilter_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IHostUSBDeviceFilter_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IHostUSBDeviceFilter_put_Name(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define IHostUSBDeviceFilter_SetName(p, aName) ((p)->lpVtbl->SetName(p, aName))
#define IHostUSBDeviceFilter_get_Active(p, aActive) ((p)->lpVtbl->GetActive(p, aActive))
#define IHostUSBDeviceFilter_GetActive(p, aActive) ((p)->lpVtbl->GetActive(p, aActive))
#define IHostUSBDeviceFilter_put_Active(p, aActive) ((p)->lpVtbl->SetActive(p, aActive))
#define IHostUSBDeviceFilter_SetActive(p, aActive) ((p)->lpVtbl->SetActive(p, aActive))
#define IHostUSBDeviceFilter_get_VendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IHostUSBDeviceFilter_GetVendorId(p, aVendorId) ((p)->lpVtbl->GetVendorId(p, aVendorId))
#define IHostUSBDeviceFilter_put_VendorId(p, aVendorId) ((p)->lpVtbl->SetVendorId(p, aVendorId))
#define IHostUSBDeviceFilter_SetVendorId(p, aVendorId) ((p)->lpVtbl->SetVendorId(p, aVendorId))
#define IHostUSBDeviceFilter_get_ProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IHostUSBDeviceFilter_GetProductId(p, aProductId) ((p)->lpVtbl->GetProductId(p, aProductId))
#define IHostUSBDeviceFilter_put_ProductId(p, aProductId) ((p)->lpVtbl->SetProductId(p, aProductId))
#define IHostUSBDeviceFilter_SetProductId(p, aProductId) ((p)->lpVtbl->SetProductId(p, aProductId))
#define IHostUSBDeviceFilter_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IHostUSBDeviceFilter_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IHostUSBDeviceFilter_put_Revision(p, aRevision) ((p)->lpVtbl->SetRevision(p, aRevision))
#define IHostUSBDeviceFilter_SetRevision(p, aRevision) ((p)->lpVtbl->SetRevision(p, aRevision))
#define IHostUSBDeviceFilter_get_Manufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IHostUSBDeviceFilter_GetManufacturer(p, aManufacturer) ((p)->lpVtbl->GetManufacturer(p, aManufacturer))
#define IHostUSBDeviceFilter_put_Manufacturer(p, aManufacturer) ((p)->lpVtbl->SetManufacturer(p, aManufacturer))
#define IHostUSBDeviceFilter_SetManufacturer(p, aManufacturer) ((p)->lpVtbl->SetManufacturer(p, aManufacturer))
#define IHostUSBDeviceFilter_get_Product(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IHostUSBDeviceFilter_GetProduct(p, aProduct) ((p)->lpVtbl->GetProduct(p, aProduct))
#define IHostUSBDeviceFilter_put_Product(p, aProduct) ((p)->lpVtbl->SetProduct(p, aProduct))
#define IHostUSBDeviceFilter_SetProduct(p, aProduct) ((p)->lpVtbl->SetProduct(p, aProduct))
#define IHostUSBDeviceFilter_get_SerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IHostUSBDeviceFilter_GetSerialNumber(p, aSerialNumber) ((p)->lpVtbl->GetSerialNumber(p, aSerialNumber))
#define IHostUSBDeviceFilter_put_SerialNumber(p, aSerialNumber) ((p)->lpVtbl->SetSerialNumber(p, aSerialNumber))
#define IHostUSBDeviceFilter_SetSerialNumber(p, aSerialNumber) ((p)->lpVtbl->SetSerialNumber(p, aSerialNumber))
#define IHostUSBDeviceFilter_get_Port(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IHostUSBDeviceFilter_GetPort(p, aPort) ((p)->lpVtbl->GetPort(p, aPort))
#define IHostUSBDeviceFilter_put_Port(p, aPort) ((p)->lpVtbl->SetPort(p, aPort))
#define IHostUSBDeviceFilter_SetPort(p, aPort) ((p)->lpVtbl->SetPort(p, aPort))
#define IHostUSBDeviceFilter_get_Remote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IHostUSBDeviceFilter_GetRemote(p, aRemote) ((p)->lpVtbl->GetRemote(p, aRemote))
#define IHostUSBDeviceFilter_put_Remote(p, aRemote) ((p)->lpVtbl->SetRemote(p, aRemote))
#define IHostUSBDeviceFilter_SetRemote(p, aRemote) ((p)->lpVtbl->SetRemote(p, aRemote))
#define IHostUSBDeviceFilter_get_MaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->GetMaskedInterfaces(p, aMaskedInterfaces))
#define IHostUSBDeviceFilter_GetMaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->GetMaskedInterfaces(p, aMaskedInterfaces))
#define IHostUSBDeviceFilter_put_MaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->SetMaskedInterfaces(p, aMaskedInterfaces))
#define IHostUSBDeviceFilter_SetMaskedInterfaces(p, aMaskedInterfaces) ((p)->lpVtbl->SetMaskedInterfaces(p, aMaskedInterfaces))
#define IHostUSBDeviceFilter_get_Action(p, aAction) ((p)->lpVtbl->GetAction(p, aAction))
#define IHostUSBDeviceFilter_GetAction(p, aAction) ((p)->lpVtbl->GetAction(p, aAction))
#define IHostUSBDeviceFilter_put_Action(p, aAction) ((p)->lpVtbl->SetAction(p, aAction))
#define IHostUSBDeviceFilter_SetAction(p, aAction) ((p)->lpVtbl->SetAction(p, aAction))
#endif /* VBOX_WITH_GLUE */

interface IHostUSBDeviceFilter
{
#ifndef VBOX_WITH_GLUE
    struct IHostUSBDeviceFilter_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostUSBDeviceFilterVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHostUSBDeviceFilter declaration */


/* Start of struct IAudioAdapter declaration */
#define IAUDIOADAPTER_IID_STR "921873db-5f3f-4b69-91f9-7be9e535a2cb"
#define IAUDIOADAPTER_IID { \
    0x921873db, 0x5f3f, 0x4b69, \
    { 0x91, 0xf9, 0x7b, 0xe9, 0xe5, 0x35, 0xa2, 0xcb } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IAudioAdapter);
#ifndef VBOX_WITH_GLUE
struct IAudioAdapter_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetEnabled)(IAudioAdapter *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IAudioAdapter *pThis, PRBool enabled);

    nsresult (*GetAudioController)(IAudioAdapter *pThis, PRUint32 *audioController);
    nsresult (*SetAudioController)(IAudioAdapter *pThis, PRUint32 audioController);

    nsresult (*GetAudioDriver)(IAudioAdapter *pThis, PRUint32 *audioDriver);
    nsresult (*SetAudioDriver)(IAudioAdapter *pThis, PRUint32 audioDriver);

};
#else /* VBOX_WITH_GLUE */
struct IAudioAdapterVtbl
{
    nsresult (*QueryInterface)(IAudioAdapter *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IAudioAdapter *pThis);
    nsrefcnt (*Release)(IAudioAdapter *pThis);
    nsresult (*GetEnabled)(IAudioAdapter *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IAudioAdapter *pThis, PRBool enabled);

    nsresult (*GetAudioController)(IAudioAdapter *pThis, PRUint32 *audioController);
    nsresult (*SetAudioController)(IAudioAdapter *pThis, PRUint32 audioController);

    nsresult (*GetAudioDriver)(IAudioAdapter *pThis, PRUint32 *audioDriver);
    nsresult (*SetAudioDriver)(IAudioAdapter *pThis, PRUint32 audioDriver);

};
#define IAudioAdapter_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IAudioAdapter_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IAudioAdapter_Release(p) ((p)->lpVtbl->Release(p))
#define IAudioAdapter_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IAudioAdapter_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IAudioAdapter_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IAudioAdapter_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IAudioAdapter_get_AudioController(p, aAudioController) ((p)->lpVtbl->GetAudioController(p, aAudioController))
#define IAudioAdapter_GetAudioController(p, aAudioController) ((p)->lpVtbl->GetAudioController(p, aAudioController))
#define IAudioAdapter_put_AudioController(p, aAudioController) ((p)->lpVtbl->SetAudioController(p, aAudioController))
#define IAudioAdapter_SetAudioController(p, aAudioController) ((p)->lpVtbl->SetAudioController(p, aAudioController))
#define IAudioAdapter_get_AudioDriver(p, aAudioDriver) ((p)->lpVtbl->GetAudioDriver(p, aAudioDriver))
#define IAudioAdapter_GetAudioDriver(p, aAudioDriver) ((p)->lpVtbl->GetAudioDriver(p, aAudioDriver))
#define IAudioAdapter_put_AudioDriver(p, aAudioDriver) ((p)->lpVtbl->SetAudioDriver(p, aAudioDriver))
#define IAudioAdapter_SetAudioDriver(p, aAudioDriver) ((p)->lpVtbl->SetAudioDriver(p, aAudioDriver))
#endif /* VBOX_WITH_GLUE */

interface IAudioAdapter
{
#ifndef VBOX_WITH_GLUE
    struct IAudioAdapter_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IAudioAdapterVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IAudioAdapter declaration */


/* Start of struct IVRDEServer declaration */
#define IVRDESERVER_IID_STR "d38de40a-c2c1-4e95-b5a4-167b05f5694c"
#define IVRDESERVER_IID { \
    0xd38de40a, 0xc2c1, 0x4e95, \
    { 0xb5, 0xa4, 0x16, 0x7b, 0x05, 0xf5, 0x69, 0x4c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVRDEServer);
#ifndef VBOX_WITH_GLUE
struct IVRDEServer_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetEnabled)(IVRDEServer *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IVRDEServer *pThis, PRBool enabled);

    nsresult (*GetAuthType)(IVRDEServer *pThis, PRUint32 *authType);
    nsresult (*SetAuthType)(IVRDEServer *pThis, PRUint32 authType);

    nsresult (*GetAuthTimeout)(IVRDEServer *pThis, PRUint32 *authTimeout);
    nsresult (*SetAuthTimeout)(IVRDEServer *pThis, PRUint32 authTimeout);

    nsresult (*GetAllowMultiConnection)(IVRDEServer *pThis, PRBool *allowMultiConnection);
    nsresult (*SetAllowMultiConnection)(IVRDEServer *pThis, PRBool allowMultiConnection);

    nsresult (*GetReuseSingleConnection)(IVRDEServer *pThis, PRBool *reuseSingleConnection);
    nsresult (*SetReuseSingleConnection)(IVRDEServer *pThis, PRBool reuseSingleConnection);

    nsresult (*GetVRDEExtPack)(IVRDEServer *pThis, PRUnichar * *VRDEExtPack);
    nsresult (*SetVRDEExtPack)(IVRDEServer *pThis, PRUnichar * VRDEExtPack);

    nsresult (*GetAuthLibrary)(IVRDEServer *pThis, PRUnichar * *authLibrary);
    nsresult (*SetAuthLibrary)(IVRDEServer *pThis, PRUnichar * authLibrary);

    nsresult (*GetVRDEProperties)(IVRDEServer *pThis, PRUint32 *VRDEPropertiesSize, PRUnichar * **VRDEProperties);

    nsresult (*SetVRDEProperty)(
        IVRDEServer *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*GetVRDEProperty)(
        IVRDEServer *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

};
#else /* VBOX_WITH_GLUE */
struct IVRDEServerVtbl
{
    nsresult (*QueryInterface)(IVRDEServer *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVRDEServer *pThis);
    nsrefcnt (*Release)(IVRDEServer *pThis);
    nsresult (*GetEnabled)(IVRDEServer *pThis, PRBool *enabled);
    nsresult (*SetEnabled)(IVRDEServer *pThis, PRBool enabled);

    nsresult (*GetAuthType)(IVRDEServer *pThis, PRUint32 *authType);
    nsresult (*SetAuthType)(IVRDEServer *pThis, PRUint32 authType);

    nsresult (*GetAuthTimeout)(IVRDEServer *pThis, PRUint32 *authTimeout);
    nsresult (*SetAuthTimeout)(IVRDEServer *pThis, PRUint32 authTimeout);

    nsresult (*GetAllowMultiConnection)(IVRDEServer *pThis, PRBool *allowMultiConnection);
    nsresult (*SetAllowMultiConnection)(IVRDEServer *pThis, PRBool allowMultiConnection);

    nsresult (*GetReuseSingleConnection)(IVRDEServer *pThis, PRBool *reuseSingleConnection);
    nsresult (*SetReuseSingleConnection)(IVRDEServer *pThis, PRBool reuseSingleConnection);

    nsresult (*GetVRDEExtPack)(IVRDEServer *pThis, PRUnichar * *VRDEExtPack);
    nsresult (*SetVRDEExtPack)(IVRDEServer *pThis, PRUnichar * VRDEExtPack);

    nsresult (*GetAuthLibrary)(IVRDEServer *pThis, PRUnichar * *authLibrary);
    nsresult (*SetAuthLibrary)(IVRDEServer *pThis, PRUnichar * authLibrary);

    nsresult (*GetVRDEProperties)(IVRDEServer *pThis, PRUint32 *VRDEPropertiesSize, PRUnichar * **VRDEProperties);

    nsresult (*SetVRDEProperty)(
        IVRDEServer *pThis,
        PRUnichar * key,
        PRUnichar * value
    );

    nsresult (*GetVRDEProperty)(
        IVRDEServer *pThis,
        PRUnichar * key,
        PRUnichar * * value
    );

};
#define IVRDEServer_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVRDEServer_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVRDEServer_Release(p) ((p)->lpVtbl->Release(p))
#define IVRDEServer_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IVRDEServer_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define IVRDEServer_put_Enabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IVRDEServer_SetEnabled(p, aEnabled) ((p)->lpVtbl->SetEnabled(p, aEnabled))
#define IVRDEServer_get_AuthType(p, aAuthType) ((p)->lpVtbl->GetAuthType(p, aAuthType))
#define IVRDEServer_GetAuthType(p, aAuthType) ((p)->lpVtbl->GetAuthType(p, aAuthType))
#define IVRDEServer_put_AuthType(p, aAuthType) ((p)->lpVtbl->SetAuthType(p, aAuthType))
#define IVRDEServer_SetAuthType(p, aAuthType) ((p)->lpVtbl->SetAuthType(p, aAuthType))
#define IVRDEServer_get_AuthTimeout(p, aAuthTimeout) ((p)->lpVtbl->GetAuthTimeout(p, aAuthTimeout))
#define IVRDEServer_GetAuthTimeout(p, aAuthTimeout) ((p)->lpVtbl->GetAuthTimeout(p, aAuthTimeout))
#define IVRDEServer_put_AuthTimeout(p, aAuthTimeout) ((p)->lpVtbl->SetAuthTimeout(p, aAuthTimeout))
#define IVRDEServer_SetAuthTimeout(p, aAuthTimeout) ((p)->lpVtbl->SetAuthTimeout(p, aAuthTimeout))
#define IVRDEServer_get_AllowMultiConnection(p, aAllowMultiConnection) ((p)->lpVtbl->GetAllowMultiConnection(p, aAllowMultiConnection))
#define IVRDEServer_GetAllowMultiConnection(p, aAllowMultiConnection) ((p)->lpVtbl->GetAllowMultiConnection(p, aAllowMultiConnection))
#define IVRDEServer_put_AllowMultiConnection(p, aAllowMultiConnection) ((p)->lpVtbl->SetAllowMultiConnection(p, aAllowMultiConnection))
#define IVRDEServer_SetAllowMultiConnection(p, aAllowMultiConnection) ((p)->lpVtbl->SetAllowMultiConnection(p, aAllowMultiConnection))
#define IVRDEServer_get_ReuseSingleConnection(p, aReuseSingleConnection) ((p)->lpVtbl->GetReuseSingleConnection(p, aReuseSingleConnection))
#define IVRDEServer_GetReuseSingleConnection(p, aReuseSingleConnection) ((p)->lpVtbl->GetReuseSingleConnection(p, aReuseSingleConnection))
#define IVRDEServer_put_ReuseSingleConnection(p, aReuseSingleConnection) ((p)->lpVtbl->SetReuseSingleConnection(p, aReuseSingleConnection))
#define IVRDEServer_SetReuseSingleConnection(p, aReuseSingleConnection) ((p)->lpVtbl->SetReuseSingleConnection(p, aReuseSingleConnection))
#define IVRDEServer_get_VRDEExtPack(p, aVRDEExtPack) ((p)->lpVtbl->GetVRDEExtPack(p, aVRDEExtPack))
#define IVRDEServer_GetVRDEExtPack(p, aVRDEExtPack) ((p)->lpVtbl->GetVRDEExtPack(p, aVRDEExtPack))
#define IVRDEServer_put_VRDEExtPack(p, aVRDEExtPack) ((p)->lpVtbl->SetVRDEExtPack(p, aVRDEExtPack))
#define IVRDEServer_SetVRDEExtPack(p, aVRDEExtPack) ((p)->lpVtbl->SetVRDEExtPack(p, aVRDEExtPack))
#define IVRDEServer_get_AuthLibrary(p, aAuthLibrary) ((p)->lpVtbl->GetAuthLibrary(p, aAuthLibrary))
#define IVRDEServer_GetAuthLibrary(p, aAuthLibrary) ((p)->lpVtbl->GetAuthLibrary(p, aAuthLibrary))
#define IVRDEServer_put_AuthLibrary(p, aAuthLibrary) ((p)->lpVtbl->SetAuthLibrary(p, aAuthLibrary))
#define IVRDEServer_SetAuthLibrary(p, aAuthLibrary) ((p)->lpVtbl->SetAuthLibrary(p, aAuthLibrary))
#define IVRDEServer_get_VRDEProperties(p, aVRDEProperties) ((p)->lpVtbl->GetVRDEProperties(p, aVRDEProperties))
#define IVRDEServer_GetVRDEProperties(p, aVRDEProperties) ((p)->lpVtbl->GetVRDEProperties(p, aVRDEProperties))
#define IVRDEServer_SetVRDEProperty(p, aKey, aValue) ((p)->lpVtbl->SetVRDEProperty(p, aKey, aValue))
#define IVRDEServer_GetVRDEProperty(p, aKey, aValue) ((p)->lpVtbl->GetVRDEProperty(p, aKey, aValue))
#endif /* VBOX_WITH_GLUE */

interface IVRDEServer
{
#ifndef VBOX_WITH_GLUE
    struct IVRDEServer_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVRDEServerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVRDEServer declaration */


/* Start of struct ISharedFolder declaration */
#define ISHAREDFOLDER_IID_STR "8388da11-b559-4574-a5b7-2bd7acd5cef8"
#define ISHAREDFOLDER_IID { \
    0x8388da11, 0xb559, 0x4574, \
    { 0xa5, 0xb7, 0x2b, 0xd7, 0xac, 0xd5, 0xce, 0xf8 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISharedFolder);
#ifndef VBOX_WITH_GLUE
struct ISharedFolder_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(ISharedFolder *pThis, PRUnichar * *name);

    nsresult (*GetHostPath)(ISharedFolder *pThis, PRUnichar * *hostPath);

    nsresult (*GetAccessible)(ISharedFolder *pThis, PRBool *accessible);

    nsresult (*GetWritable)(ISharedFolder *pThis, PRBool *writable);

    nsresult (*GetAutoMount)(ISharedFolder *pThis, PRBool *autoMount);

    nsresult (*GetLastAccessError)(ISharedFolder *pThis, PRUnichar * *lastAccessError);

};
#else /* VBOX_WITH_GLUE */
struct ISharedFolderVtbl
{
    nsresult (*QueryInterface)(ISharedFolder *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISharedFolder *pThis);
    nsrefcnt (*Release)(ISharedFolder *pThis);
    nsresult (*GetName)(ISharedFolder *pThis, PRUnichar * *name);

    nsresult (*GetHostPath)(ISharedFolder *pThis, PRUnichar * *hostPath);

    nsresult (*GetAccessible)(ISharedFolder *pThis, PRBool *accessible);

    nsresult (*GetWritable)(ISharedFolder *pThis, PRBool *writable);

    nsresult (*GetAutoMount)(ISharedFolder *pThis, PRBool *autoMount);

    nsresult (*GetLastAccessError)(ISharedFolder *pThis, PRUnichar * *lastAccessError);

};
#define ISharedFolder_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISharedFolder_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISharedFolder_Release(p) ((p)->lpVtbl->Release(p))
#define ISharedFolder_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define ISharedFolder_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define ISharedFolder_get_HostPath(p, aHostPath) ((p)->lpVtbl->GetHostPath(p, aHostPath))
#define ISharedFolder_GetHostPath(p, aHostPath) ((p)->lpVtbl->GetHostPath(p, aHostPath))
#define ISharedFolder_get_Accessible(p, aAccessible) ((p)->lpVtbl->GetAccessible(p, aAccessible))
#define ISharedFolder_GetAccessible(p, aAccessible) ((p)->lpVtbl->GetAccessible(p, aAccessible))
#define ISharedFolder_get_Writable(p, aWritable) ((p)->lpVtbl->GetWritable(p, aWritable))
#define ISharedFolder_GetWritable(p, aWritable) ((p)->lpVtbl->GetWritable(p, aWritable))
#define ISharedFolder_get_AutoMount(p, aAutoMount) ((p)->lpVtbl->GetAutoMount(p, aAutoMount))
#define ISharedFolder_GetAutoMount(p, aAutoMount) ((p)->lpVtbl->GetAutoMount(p, aAutoMount))
#define ISharedFolder_get_LastAccessError(p, aLastAccessError) ((p)->lpVtbl->GetLastAccessError(p, aLastAccessError))
#define ISharedFolder_GetLastAccessError(p, aLastAccessError) ((p)->lpVtbl->GetLastAccessError(p, aLastAccessError))
#endif /* VBOX_WITH_GLUE */

interface ISharedFolder
{
#ifndef VBOX_WITH_GLUE
    struct ISharedFolder_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISharedFolderVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISharedFolder declaration */


/* Start of struct ISession declaration */
#define ISESSION_IID_STR "12F4DCDB-12B2-4EC1-B7CD-DDD9F6C5BF4D"
#define ISESSION_IID { \
    0x12F4DCDB, 0x12B2, 0x4EC1, \
    { 0xB7, 0xCD, 0xDD, 0xD9, 0xF6, 0xC5, 0xBF, 0x4D } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISession);
#ifndef VBOX_WITH_GLUE
struct ISession_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetState)(ISession *pThis, PRUint32 *state);

    nsresult (*GetType)(ISession *pThis, PRUint32 *type);

    nsresult (*GetMachine)(ISession *pThis, IMachine * *machine);

    nsresult (*GetConsole)(ISession *pThis, IConsole * *console);

    nsresult (*UnlockMachine)(ISession *pThis );

};
#else /* VBOX_WITH_GLUE */
struct ISessionVtbl
{
    nsresult (*QueryInterface)(ISession *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISession *pThis);
    nsrefcnt (*Release)(ISession *pThis);
    nsresult (*GetState)(ISession *pThis, PRUint32 *state);

    nsresult (*GetType)(ISession *pThis, PRUint32 *type);

    nsresult (*GetMachine)(ISession *pThis, IMachine * *machine);

    nsresult (*GetConsole)(ISession *pThis, IConsole * *console);

    nsresult (*UnlockMachine)(ISession *pThis );

};
#define ISession_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISession_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISession_Release(p) ((p)->lpVtbl->Release(p))
#define ISession_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define ISession_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define ISession_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISession_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISession_get_Machine(p, aMachine) ((p)->lpVtbl->GetMachine(p, aMachine))
#define ISession_GetMachine(p, aMachine) ((p)->lpVtbl->GetMachine(p, aMachine))
#define ISession_get_Console(p, aConsole) ((p)->lpVtbl->GetConsole(p, aConsole))
#define ISession_GetConsole(p, aConsole) ((p)->lpVtbl->GetConsole(p, aConsole))
#define ISession_UnlockMachine(p) ((p)->lpVtbl->UnlockMachine(p))
#endif /* VBOX_WITH_GLUE */

interface ISession
{
#ifndef VBOX_WITH_GLUE
    struct ISession_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISessionVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISession declaration */


/* Start of struct IStorageController declaration */
#define ISTORAGECONTROLLER_IID_STR "a1556333-09b6-46d9-bfb7-fc239b7fbe1e"
#define ISTORAGECONTROLLER_IID { \
    0xa1556333, 0x09b6, 0x46d9, \
    { 0xbf, 0xb7, 0xfc, 0x23, 0x9b, 0x7f, 0xbe, 0x1e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IStorageController);
#ifndef VBOX_WITH_GLUE
struct IStorageController_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IStorageController *pThis, PRUnichar * *name);

    nsresult (*GetMaxDevicesPerPortCount)(IStorageController *pThis, PRUint32 *maxDevicesPerPortCount);

    nsresult (*GetMinPortCount)(IStorageController *pThis, PRUint32 *minPortCount);

    nsresult (*GetMaxPortCount)(IStorageController *pThis, PRUint32 *maxPortCount);

    nsresult (*GetInstance)(IStorageController *pThis, PRUint32 *instance);
    nsresult (*SetInstance)(IStorageController *pThis, PRUint32 instance);

    nsresult (*GetPortCount)(IStorageController *pThis, PRUint32 *portCount);
    nsresult (*SetPortCount)(IStorageController *pThis, PRUint32 portCount);

    nsresult (*GetBus)(IStorageController *pThis, PRUint32 *bus);

    nsresult (*GetControllerType)(IStorageController *pThis, PRUint32 *controllerType);
    nsresult (*SetControllerType)(IStorageController *pThis, PRUint32 controllerType);

    nsresult (*GetUseHostIOCache)(IStorageController *pThis, PRBool *useHostIOCache);
    nsresult (*SetUseHostIOCache)(IStorageController *pThis, PRBool useHostIOCache);

    nsresult (*GetBootable)(IStorageController *pThis, PRBool *bootable);

};
#else /* VBOX_WITH_GLUE */
struct IStorageControllerVtbl
{
    nsresult (*QueryInterface)(IStorageController *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IStorageController *pThis);
    nsrefcnt (*Release)(IStorageController *pThis);
    nsresult (*GetName)(IStorageController *pThis, PRUnichar * *name);

    nsresult (*GetMaxDevicesPerPortCount)(IStorageController *pThis, PRUint32 *maxDevicesPerPortCount);

    nsresult (*GetMinPortCount)(IStorageController *pThis, PRUint32 *minPortCount);

    nsresult (*GetMaxPortCount)(IStorageController *pThis, PRUint32 *maxPortCount);

    nsresult (*GetInstance)(IStorageController *pThis, PRUint32 *instance);
    nsresult (*SetInstance)(IStorageController *pThis, PRUint32 instance);

    nsresult (*GetPortCount)(IStorageController *pThis, PRUint32 *portCount);
    nsresult (*SetPortCount)(IStorageController *pThis, PRUint32 portCount);

    nsresult (*GetBus)(IStorageController *pThis, PRUint32 *bus);

    nsresult (*GetControllerType)(IStorageController *pThis, PRUint32 *controllerType);
    nsresult (*SetControllerType)(IStorageController *pThis, PRUint32 controllerType);

    nsresult (*GetUseHostIOCache)(IStorageController *pThis, PRBool *useHostIOCache);
    nsresult (*SetUseHostIOCache)(IStorageController *pThis, PRBool useHostIOCache);

    nsresult (*GetBootable)(IStorageController *pThis, PRBool *bootable);

};
#define IStorageController_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IStorageController_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IStorageController_Release(p) ((p)->lpVtbl->Release(p))
#define IStorageController_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IStorageController_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IStorageController_get_MaxDevicesPerPortCount(p, aMaxDevicesPerPortCount) ((p)->lpVtbl->GetMaxDevicesPerPortCount(p, aMaxDevicesPerPortCount))
#define IStorageController_GetMaxDevicesPerPortCount(p, aMaxDevicesPerPortCount) ((p)->lpVtbl->GetMaxDevicesPerPortCount(p, aMaxDevicesPerPortCount))
#define IStorageController_get_MinPortCount(p, aMinPortCount) ((p)->lpVtbl->GetMinPortCount(p, aMinPortCount))
#define IStorageController_GetMinPortCount(p, aMinPortCount) ((p)->lpVtbl->GetMinPortCount(p, aMinPortCount))
#define IStorageController_get_MaxPortCount(p, aMaxPortCount) ((p)->lpVtbl->GetMaxPortCount(p, aMaxPortCount))
#define IStorageController_GetMaxPortCount(p, aMaxPortCount) ((p)->lpVtbl->GetMaxPortCount(p, aMaxPortCount))
#define IStorageController_get_Instance(p, aInstance) ((p)->lpVtbl->GetInstance(p, aInstance))
#define IStorageController_GetInstance(p, aInstance) ((p)->lpVtbl->GetInstance(p, aInstance))
#define IStorageController_put_Instance(p, aInstance) ((p)->lpVtbl->SetInstance(p, aInstance))
#define IStorageController_SetInstance(p, aInstance) ((p)->lpVtbl->SetInstance(p, aInstance))
#define IStorageController_get_PortCount(p, aPortCount) ((p)->lpVtbl->GetPortCount(p, aPortCount))
#define IStorageController_GetPortCount(p, aPortCount) ((p)->lpVtbl->GetPortCount(p, aPortCount))
#define IStorageController_put_PortCount(p, aPortCount) ((p)->lpVtbl->SetPortCount(p, aPortCount))
#define IStorageController_SetPortCount(p, aPortCount) ((p)->lpVtbl->SetPortCount(p, aPortCount))
#define IStorageController_get_Bus(p, aBus) ((p)->lpVtbl->GetBus(p, aBus))
#define IStorageController_GetBus(p, aBus) ((p)->lpVtbl->GetBus(p, aBus))
#define IStorageController_get_ControllerType(p, aControllerType) ((p)->lpVtbl->GetControllerType(p, aControllerType))
#define IStorageController_GetControllerType(p, aControllerType) ((p)->lpVtbl->GetControllerType(p, aControllerType))
#define IStorageController_put_ControllerType(p, aControllerType) ((p)->lpVtbl->SetControllerType(p, aControllerType))
#define IStorageController_SetControllerType(p, aControllerType) ((p)->lpVtbl->SetControllerType(p, aControllerType))
#define IStorageController_get_UseHostIOCache(p, aUseHostIOCache) ((p)->lpVtbl->GetUseHostIOCache(p, aUseHostIOCache))
#define IStorageController_GetUseHostIOCache(p, aUseHostIOCache) ((p)->lpVtbl->GetUseHostIOCache(p, aUseHostIOCache))
#define IStorageController_put_UseHostIOCache(p, aUseHostIOCache) ((p)->lpVtbl->SetUseHostIOCache(p, aUseHostIOCache))
#define IStorageController_SetUseHostIOCache(p, aUseHostIOCache) ((p)->lpVtbl->SetUseHostIOCache(p, aUseHostIOCache))
#define IStorageController_get_Bootable(p, aBootable) ((p)->lpVtbl->GetBootable(p, aBootable))
#define IStorageController_GetBootable(p, aBootable) ((p)->lpVtbl->GetBootable(p, aBootable))
#endif /* VBOX_WITH_GLUE */

interface IStorageController
{
#ifndef VBOX_WITH_GLUE
    struct IStorageController_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IStorageControllerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IStorageController declaration */


/* Start of struct IPerformanceMetric declaration */
#define IPERFORMANCEMETRIC_IID_STR "2a1a60ae-9345-4019-ad53-d34ba41cbfe9"
#define IPERFORMANCEMETRIC_IID { \
    0x2a1a60ae, 0x9345, 0x4019, \
    { 0xad, 0x53, 0xd3, 0x4b, 0xa4, 0x1c, 0xbf, 0xe9 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IPerformanceMetric);
#ifndef VBOX_WITH_GLUE
struct IPerformanceMetric_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetMetricName)(IPerformanceMetric *pThis, PRUnichar * *metricName);

    nsresult (*GetObject)(IPerformanceMetric *pThis, nsISupports * *object);

    nsresult (*GetDescription)(IPerformanceMetric *pThis, PRUnichar * *description);

    nsresult (*GetPeriod)(IPerformanceMetric *pThis, PRUint32 *period);

    nsresult (*GetCount)(IPerformanceMetric *pThis, PRUint32 *count);

    nsresult (*GetUnit)(IPerformanceMetric *pThis, PRUnichar * *unit);

    nsresult (*GetMinimumValue)(IPerformanceMetric *pThis, PRInt32 *minimumValue);

    nsresult (*GetMaximumValue)(IPerformanceMetric *pThis, PRInt32 *maximumValue);

};
#else /* VBOX_WITH_GLUE */
struct IPerformanceMetricVtbl
{
    nsresult (*QueryInterface)(IPerformanceMetric *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IPerformanceMetric *pThis);
    nsrefcnt (*Release)(IPerformanceMetric *pThis);
    nsresult (*GetMetricName)(IPerformanceMetric *pThis, PRUnichar * *metricName);

    nsresult (*GetObject)(IPerformanceMetric *pThis, nsISupports * *object);

    nsresult (*GetDescription)(IPerformanceMetric *pThis, PRUnichar * *description);

    nsresult (*GetPeriod)(IPerformanceMetric *pThis, PRUint32 *period);

    nsresult (*GetCount)(IPerformanceMetric *pThis, PRUint32 *count);

    nsresult (*GetUnit)(IPerformanceMetric *pThis, PRUnichar * *unit);

    nsresult (*GetMinimumValue)(IPerformanceMetric *pThis, PRInt32 *minimumValue);

    nsresult (*GetMaximumValue)(IPerformanceMetric *pThis, PRInt32 *maximumValue);

};
#define IPerformanceMetric_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IPerformanceMetric_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IPerformanceMetric_Release(p) ((p)->lpVtbl->Release(p))
#define IPerformanceMetric_get_MetricName(p, aMetricName) ((p)->lpVtbl->GetMetricName(p, aMetricName))
#define IPerformanceMetric_GetMetricName(p, aMetricName) ((p)->lpVtbl->GetMetricName(p, aMetricName))
#define IPerformanceMetric_get_Object(p, aObject) ((p)->lpVtbl->GetObject(p, aObject))
#define IPerformanceMetric_GetObject(p, aObject) ((p)->lpVtbl->GetObject(p, aObject))
#define IPerformanceMetric_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IPerformanceMetric_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IPerformanceMetric_get_Period(p, aPeriod) ((p)->lpVtbl->GetPeriod(p, aPeriod))
#define IPerformanceMetric_GetPeriod(p, aPeriod) ((p)->lpVtbl->GetPeriod(p, aPeriod))
#define IPerformanceMetric_get_Count(p, aCount) ((p)->lpVtbl->GetCount(p, aCount))
#define IPerformanceMetric_GetCount(p, aCount) ((p)->lpVtbl->GetCount(p, aCount))
#define IPerformanceMetric_get_Unit(p, aUnit) ((p)->lpVtbl->GetUnit(p, aUnit))
#define IPerformanceMetric_GetUnit(p, aUnit) ((p)->lpVtbl->GetUnit(p, aUnit))
#define IPerformanceMetric_get_MinimumValue(p, aMinimumValue) ((p)->lpVtbl->GetMinimumValue(p, aMinimumValue))
#define IPerformanceMetric_GetMinimumValue(p, aMinimumValue) ((p)->lpVtbl->GetMinimumValue(p, aMinimumValue))
#define IPerformanceMetric_get_MaximumValue(p, aMaximumValue) ((p)->lpVtbl->GetMaximumValue(p, aMaximumValue))
#define IPerformanceMetric_GetMaximumValue(p, aMaximumValue) ((p)->lpVtbl->GetMaximumValue(p, aMaximumValue))
#endif /* VBOX_WITH_GLUE */

interface IPerformanceMetric
{
#ifndef VBOX_WITH_GLUE
    struct IPerformanceMetric_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IPerformanceMetricVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IPerformanceMetric declaration */


/* Start of struct IPerformanceCollector declaration */
#define IPERFORMANCECOLLECTOR_IID_STR "e22e1acb-ac4a-43bb-a31c-17321659b0c6"
#define IPERFORMANCECOLLECTOR_IID { \
    0xe22e1acb, 0xac4a, 0x43bb, \
    { 0xa3, 0x1c, 0x17, 0x32, 0x16, 0x59, 0xb0, 0xc6 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IPerformanceCollector);
#ifndef VBOX_WITH_GLUE
struct IPerformanceCollector_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetMetricNames)(IPerformanceCollector *pThis, PRUint32 *metricNamesSize, PRUnichar * **metricNames);

    nsresult (*GetMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *metricsSize,
        IPerformanceMetric *** metrics
    );

    nsresult (*SetupMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 period,
        PRUint32 count,
        PRUint32 *affectedMetricsSize,
        IPerformanceMetric *** affectedMetrics
    );

    nsresult (*EnableMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *affectedMetricsSize,
        IPerformanceMetric *** affectedMetrics
    );

    nsresult (*DisableMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *affectedMetricsSize,
        IPerformanceMetric *** affectedMetrics
    );

    nsresult (*QueryMetricsData)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *returnMetricNamesSize,
        PRUnichar *** returnMetricNames,
        PRUint32 *returnObjectsSize,
        nsISupports *** returnObjects,
        PRUint32 *returnUnitsSize,
        PRUnichar *** returnUnits,
        PRUint32 *returnScalesSize,
        PRUint32** returnScales,
        PRUint32 *returnSequenceNumbersSize,
        PRUint32** returnSequenceNumbers,
        PRUint32 *returnDataIndicesSize,
        PRUint32** returnDataIndices,
        PRUint32 *returnDataLengthsSize,
        PRUint32** returnDataLengths,
        PRUint32 *returnDataSize,
        PRInt32** returnData
    );

};
#else /* VBOX_WITH_GLUE */
struct IPerformanceCollectorVtbl
{
    nsresult (*QueryInterface)(IPerformanceCollector *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IPerformanceCollector *pThis);
    nsrefcnt (*Release)(IPerformanceCollector *pThis);
    nsresult (*GetMetricNames)(IPerformanceCollector *pThis, PRUint32 *metricNamesSize, PRUnichar * **metricNames);

    nsresult (*GetMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *metricsSize,
        IPerformanceMetric *** metrics
    );

    nsresult (*SetupMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 period,
        PRUint32 count,
        PRUint32 *affectedMetricsSize,
        IPerformanceMetric *** affectedMetrics
    );

    nsresult (*EnableMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *affectedMetricsSize,
        IPerformanceMetric *** affectedMetrics
    );

    nsresult (*DisableMetrics)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *affectedMetricsSize,
        IPerformanceMetric *** affectedMetrics
    );

    nsresult (*QueryMetricsData)(
        IPerformanceCollector *pThis,
        PRUint32 metricNamesSize,
        PRUnichar ** metricNames,
        PRUint32 objectsSize,
        nsISupports ** objects,
        PRUint32 *returnMetricNamesSize,
        PRUnichar *** returnMetricNames,
        PRUint32 *returnObjectsSize,
        nsISupports *** returnObjects,
        PRUint32 *returnUnitsSize,
        PRUnichar *** returnUnits,
        PRUint32 *returnScalesSize,
        PRUint32** returnScales,
        PRUint32 *returnSequenceNumbersSize,
        PRUint32** returnSequenceNumbers,
        PRUint32 *returnDataIndicesSize,
        PRUint32** returnDataIndices,
        PRUint32 *returnDataLengthsSize,
        PRUint32** returnDataLengths,
        PRUint32 *returnDataSize,
        PRInt32** returnData
    );

};
#define IPerformanceCollector_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IPerformanceCollector_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IPerformanceCollector_Release(p) ((p)->lpVtbl->Release(p))
#define IPerformanceCollector_get_MetricNames(p, aMetricNames) ((p)->lpVtbl->GetMetricNames(p, aMetricNames))
#define IPerformanceCollector_GetMetricNames(p, aMetricNames) ((p)->lpVtbl->GetMetricNames(p, aMetricNames))
#define IPerformanceCollector_GetMetrics(p, aMetricNames, aObjects, aMetrics) ((p)->lpVtbl->GetMetrics(p, aMetricNames, aObjects, aMetrics))
#define IPerformanceCollector_SetupMetrics(p, aMetricNames, aObjects, aPeriod, aCount, aAffectedMetrics) ((p)->lpVtbl->SetupMetrics(p, aMetricNames, aObjects, aPeriod, aCount, aAffectedMetrics))
#define IPerformanceCollector_EnableMetrics(p, aMetricNames, aObjects, aAffectedMetrics) ((p)->lpVtbl->EnableMetrics(p, aMetricNames, aObjects, aAffectedMetrics))
#define IPerformanceCollector_DisableMetrics(p, aMetricNames, aObjects, aAffectedMetrics) ((p)->lpVtbl->DisableMetrics(p, aMetricNames, aObjects, aAffectedMetrics))
#define IPerformanceCollector_QueryMetricsData(p, aMetricNames, aObjects, aReturnMetricNames, aReturnObjects, aReturnUnits, aReturnScales, aReturnSequenceNumbers, aReturnDataIndices, aReturnDataLengths, aReturnData) ((p)->lpVtbl->QueryMetricsData(p, aMetricNames, aObjects, aReturnMetricNames, aReturnObjects, aReturnUnits, aReturnScales, aReturnSequenceNumbers, aReturnDataIndices, aReturnDataLengths, aReturnData))
#endif /* VBOX_WITH_GLUE */

interface IPerformanceCollector
{
#ifndef VBOX_WITH_GLUE
    struct IPerformanceCollector_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IPerformanceCollectorVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IPerformanceCollector declaration */


/* Start of struct INATEngine declaration */
#define INATENGINE_IID_STR "26451b99-3b2d-4dcb-8e4b-d63654218175"
#define INATENGINE_IID { \
    0x26451b99, 0x3b2d, 0x4dcb, \
    { 0x8e, 0x4b, 0xd6, 0x36, 0x54, 0x21, 0x81, 0x75 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATEngine);
#ifndef VBOX_WITH_GLUE
struct INATEngine_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetNetwork)(INATEngine *pThis, PRUnichar * *network);
    nsresult (*SetNetwork)(INATEngine *pThis, PRUnichar * network);

    nsresult (*GetHostIP)(INATEngine *pThis, PRUnichar * *hostIP);
    nsresult (*SetHostIP)(INATEngine *pThis, PRUnichar * hostIP);

    nsresult (*GetTFTPPrefix)(INATEngine *pThis, PRUnichar * *TFTPPrefix);
    nsresult (*SetTFTPPrefix)(INATEngine *pThis, PRUnichar * TFTPPrefix);

    nsresult (*GetTFTPBootFile)(INATEngine *pThis, PRUnichar * *TFTPBootFile);
    nsresult (*SetTFTPBootFile)(INATEngine *pThis, PRUnichar * TFTPBootFile);

    nsresult (*GetTFTPNextServer)(INATEngine *pThis, PRUnichar * *TFTPNextServer);
    nsresult (*SetTFTPNextServer)(INATEngine *pThis, PRUnichar * TFTPNextServer);

    nsresult (*GetAliasMode)(INATEngine *pThis, PRUint32 *aliasMode);
    nsresult (*SetAliasMode)(INATEngine *pThis, PRUint32 aliasMode);

    nsresult (*GetDNSPassDomain)(INATEngine *pThis, PRBool *DNSPassDomain);
    nsresult (*SetDNSPassDomain)(INATEngine *pThis, PRBool DNSPassDomain);

    nsresult (*GetDNSProxy)(INATEngine *pThis, PRBool *DNSProxy);
    nsresult (*SetDNSProxy)(INATEngine *pThis, PRBool DNSProxy);

    nsresult (*GetDNSUseHostResolver)(INATEngine *pThis, PRBool *DNSUseHostResolver);
    nsresult (*SetDNSUseHostResolver)(INATEngine *pThis, PRBool DNSUseHostResolver);

    nsresult (*GetRedirects)(INATEngine *pThis, PRUint32 *redirectsSize, PRUnichar * **redirects);

    nsresult (*SetNetworkSettings)(
        INATEngine *pThis,
        PRUint32 mtu,
        PRUint32 sockSnd,
        PRUint32 sockRcv,
        PRUint32 TcpWndSnd,
        PRUint32 TcpWndRcv
    );

    nsresult (*GetNetworkSettings)(
        INATEngine *pThis,
        PRUint32 * mtu,
        PRUint32 * sockSnd,
        PRUint32 * sockRcv,
        PRUint32 * TcpWndSnd,
        PRUint32 * TcpWndRcv
    );

    nsresult (*AddRedirect)(
        INATEngine *pThis,
        PRUnichar * name,
        PRUint32 proto,
        PRUnichar * hostIP,
        PRUint16 hostPort,
        PRUnichar * guestIP,
        PRUint16 guestPort
    );

    nsresult (*RemoveRedirect)(
        INATEngine *pThis,
        PRUnichar * name
    );

};
#else /* VBOX_WITH_GLUE */
struct INATEngineVtbl
{
    nsresult (*QueryInterface)(INATEngine *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATEngine *pThis);
    nsrefcnt (*Release)(INATEngine *pThis);
    nsresult (*GetNetwork)(INATEngine *pThis, PRUnichar * *network);
    nsresult (*SetNetwork)(INATEngine *pThis, PRUnichar * network);

    nsresult (*GetHostIP)(INATEngine *pThis, PRUnichar * *hostIP);
    nsresult (*SetHostIP)(INATEngine *pThis, PRUnichar * hostIP);

    nsresult (*GetTFTPPrefix)(INATEngine *pThis, PRUnichar * *TFTPPrefix);
    nsresult (*SetTFTPPrefix)(INATEngine *pThis, PRUnichar * TFTPPrefix);

    nsresult (*GetTFTPBootFile)(INATEngine *pThis, PRUnichar * *TFTPBootFile);
    nsresult (*SetTFTPBootFile)(INATEngine *pThis, PRUnichar * TFTPBootFile);

    nsresult (*GetTFTPNextServer)(INATEngine *pThis, PRUnichar * *TFTPNextServer);
    nsresult (*SetTFTPNextServer)(INATEngine *pThis, PRUnichar * TFTPNextServer);

    nsresult (*GetAliasMode)(INATEngine *pThis, PRUint32 *aliasMode);
    nsresult (*SetAliasMode)(INATEngine *pThis, PRUint32 aliasMode);

    nsresult (*GetDNSPassDomain)(INATEngine *pThis, PRBool *DNSPassDomain);
    nsresult (*SetDNSPassDomain)(INATEngine *pThis, PRBool DNSPassDomain);

    nsresult (*GetDNSProxy)(INATEngine *pThis, PRBool *DNSProxy);
    nsresult (*SetDNSProxy)(INATEngine *pThis, PRBool DNSProxy);

    nsresult (*GetDNSUseHostResolver)(INATEngine *pThis, PRBool *DNSUseHostResolver);
    nsresult (*SetDNSUseHostResolver)(INATEngine *pThis, PRBool DNSUseHostResolver);

    nsresult (*GetRedirects)(INATEngine *pThis, PRUint32 *redirectsSize, PRUnichar * **redirects);

    nsresult (*SetNetworkSettings)(
        INATEngine *pThis,
        PRUint32 mtu,
        PRUint32 sockSnd,
        PRUint32 sockRcv,
        PRUint32 TcpWndSnd,
        PRUint32 TcpWndRcv
    );

    nsresult (*GetNetworkSettings)(
        INATEngine *pThis,
        PRUint32 * mtu,
        PRUint32 * sockSnd,
        PRUint32 * sockRcv,
        PRUint32 * TcpWndSnd,
        PRUint32 * TcpWndRcv
    );

    nsresult (*AddRedirect)(
        INATEngine *pThis,
        PRUnichar * name,
        PRUint32 proto,
        PRUnichar * hostIP,
        PRUint16 hostPort,
        PRUnichar * guestIP,
        PRUint16 guestPort
    );

    nsresult (*RemoveRedirect)(
        INATEngine *pThis,
        PRUnichar * name
    );

};
#define INATEngine_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATEngine_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATEngine_Release(p) ((p)->lpVtbl->Release(p))
#define INATEngine_get_Network(p, aNetwork) ((p)->lpVtbl->GetNetwork(p, aNetwork))
#define INATEngine_GetNetwork(p, aNetwork) ((p)->lpVtbl->GetNetwork(p, aNetwork))
#define INATEngine_put_Network(p, aNetwork) ((p)->lpVtbl->SetNetwork(p, aNetwork))
#define INATEngine_SetNetwork(p, aNetwork) ((p)->lpVtbl->SetNetwork(p, aNetwork))
#define INATEngine_get_HostIP(p, aHostIP) ((p)->lpVtbl->GetHostIP(p, aHostIP))
#define INATEngine_GetHostIP(p, aHostIP) ((p)->lpVtbl->GetHostIP(p, aHostIP))
#define INATEngine_put_HostIP(p, aHostIP) ((p)->lpVtbl->SetHostIP(p, aHostIP))
#define INATEngine_SetHostIP(p, aHostIP) ((p)->lpVtbl->SetHostIP(p, aHostIP))
#define INATEngine_get_TFTPPrefix(p, aTFTPPrefix) ((p)->lpVtbl->GetTFTPPrefix(p, aTFTPPrefix))
#define INATEngine_GetTFTPPrefix(p, aTFTPPrefix) ((p)->lpVtbl->GetTFTPPrefix(p, aTFTPPrefix))
#define INATEngine_put_TFTPPrefix(p, aTFTPPrefix) ((p)->lpVtbl->SetTFTPPrefix(p, aTFTPPrefix))
#define INATEngine_SetTFTPPrefix(p, aTFTPPrefix) ((p)->lpVtbl->SetTFTPPrefix(p, aTFTPPrefix))
#define INATEngine_get_TFTPBootFile(p, aTFTPBootFile) ((p)->lpVtbl->GetTFTPBootFile(p, aTFTPBootFile))
#define INATEngine_GetTFTPBootFile(p, aTFTPBootFile) ((p)->lpVtbl->GetTFTPBootFile(p, aTFTPBootFile))
#define INATEngine_put_TFTPBootFile(p, aTFTPBootFile) ((p)->lpVtbl->SetTFTPBootFile(p, aTFTPBootFile))
#define INATEngine_SetTFTPBootFile(p, aTFTPBootFile) ((p)->lpVtbl->SetTFTPBootFile(p, aTFTPBootFile))
#define INATEngine_get_TFTPNextServer(p, aTFTPNextServer) ((p)->lpVtbl->GetTFTPNextServer(p, aTFTPNextServer))
#define INATEngine_GetTFTPNextServer(p, aTFTPNextServer) ((p)->lpVtbl->GetTFTPNextServer(p, aTFTPNextServer))
#define INATEngine_put_TFTPNextServer(p, aTFTPNextServer) ((p)->lpVtbl->SetTFTPNextServer(p, aTFTPNextServer))
#define INATEngine_SetTFTPNextServer(p, aTFTPNextServer) ((p)->lpVtbl->SetTFTPNextServer(p, aTFTPNextServer))
#define INATEngine_get_AliasMode(p, aAliasMode) ((p)->lpVtbl->GetAliasMode(p, aAliasMode))
#define INATEngine_GetAliasMode(p, aAliasMode) ((p)->lpVtbl->GetAliasMode(p, aAliasMode))
#define INATEngine_put_AliasMode(p, aAliasMode) ((p)->lpVtbl->SetAliasMode(p, aAliasMode))
#define INATEngine_SetAliasMode(p, aAliasMode) ((p)->lpVtbl->SetAliasMode(p, aAliasMode))
#define INATEngine_get_DNSPassDomain(p, aDNSPassDomain) ((p)->lpVtbl->GetDNSPassDomain(p, aDNSPassDomain))
#define INATEngine_GetDNSPassDomain(p, aDNSPassDomain) ((p)->lpVtbl->GetDNSPassDomain(p, aDNSPassDomain))
#define INATEngine_put_DNSPassDomain(p, aDNSPassDomain) ((p)->lpVtbl->SetDNSPassDomain(p, aDNSPassDomain))
#define INATEngine_SetDNSPassDomain(p, aDNSPassDomain) ((p)->lpVtbl->SetDNSPassDomain(p, aDNSPassDomain))
#define INATEngine_get_DNSProxy(p, aDNSProxy) ((p)->lpVtbl->GetDNSProxy(p, aDNSProxy))
#define INATEngine_GetDNSProxy(p, aDNSProxy) ((p)->lpVtbl->GetDNSProxy(p, aDNSProxy))
#define INATEngine_put_DNSProxy(p, aDNSProxy) ((p)->lpVtbl->SetDNSProxy(p, aDNSProxy))
#define INATEngine_SetDNSProxy(p, aDNSProxy) ((p)->lpVtbl->SetDNSProxy(p, aDNSProxy))
#define INATEngine_get_DNSUseHostResolver(p, aDNSUseHostResolver) ((p)->lpVtbl->GetDNSUseHostResolver(p, aDNSUseHostResolver))
#define INATEngine_GetDNSUseHostResolver(p, aDNSUseHostResolver) ((p)->lpVtbl->GetDNSUseHostResolver(p, aDNSUseHostResolver))
#define INATEngine_put_DNSUseHostResolver(p, aDNSUseHostResolver) ((p)->lpVtbl->SetDNSUseHostResolver(p, aDNSUseHostResolver))
#define INATEngine_SetDNSUseHostResolver(p, aDNSUseHostResolver) ((p)->lpVtbl->SetDNSUseHostResolver(p, aDNSUseHostResolver))
#define INATEngine_get_Redirects(p, aRedirects) ((p)->lpVtbl->GetRedirects(p, aRedirects))
#define INATEngine_GetRedirects(p, aRedirects) ((p)->lpVtbl->GetRedirects(p, aRedirects))
#define INATEngine_SetNetworkSettings(p, aMtu, aSockSnd, aSockRcv, aTcpWndSnd, aTcpWndRcv) ((p)->lpVtbl->SetNetworkSettings(p, aMtu, aSockSnd, aSockRcv, aTcpWndSnd, aTcpWndRcv))
#define INATEngine_GetNetworkSettings(p, aMtu, aSockSnd, aSockRcv, aTcpWndSnd, aTcpWndRcv) ((p)->lpVtbl->GetNetworkSettings(p, aMtu, aSockSnd, aSockRcv, aTcpWndSnd, aTcpWndRcv))
#define INATEngine_AddRedirect(p, aName, aProto, aHostIP, aHostPort, aGuestIP, aGuestPort) ((p)->lpVtbl->AddRedirect(p, aName, aProto, aHostIP, aHostPort, aGuestIP, aGuestPort))
#define INATEngine_RemoveRedirect(p, aName) ((p)->lpVtbl->RemoveRedirect(p, aName))
#endif /* VBOX_WITH_GLUE */

interface INATEngine
{
#ifndef VBOX_WITH_GLUE
    struct INATEngine_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATEngineVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATEngine declaration */


/* Start of struct IExtPackPlugIn declaration */
#define IEXTPACKPLUGIN_IID_STR "58000040-e718-4746-bbce-4b86d96da461"
#define IEXTPACKPLUGIN_IID { \
    0x58000040, 0xe718, 0x4746, \
    { 0xbb, 0xce, 0x4b, 0x86, 0xd9, 0x6d, 0xa4, 0x61 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtPackPlugIn);
#ifndef VBOX_WITH_GLUE
struct IExtPackPlugIn_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IExtPackPlugIn *pThis, PRUnichar * *name);

    nsresult (*GetDescription)(IExtPackPlugIn *pThis, PRUnichar * *description);

    nsresult (*GetFrontend)(IExtPackPlugIn *pThis, PRUnichar * *frontend);

    nsresult (*GetModulePath)(IExtPackPlugIn *pThis, PRUnichar * *modulePath);

};
#else /* VBOX_WITH_GLUE */
struct IExtPackPlugInVtbl
{
    nsresult (*QueryInterface)(IExtPackPlugIn *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtPackPlugIn *pThis);
    nsrefcnt (*Release)(IExtPackPlugIn *pThis);
    nsresult (*GetName)(IExtPackPlugIn *pThis, PRUnichar * *name);

    nsresult (*GetDescription)(IExtPackPlugIn *pThis, PRUnichar * *description);

    nsresult (*GetFrontend)(IExtPackPlugIn *pThis, PRUnichar * *frontend);

    nsresult (*GetModulePath)(IExtPackPlugIn *pThis, PRUnichar * *modulePath);

};
#define IExtPackPlugIn_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtPackPlugIn_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtPackPlugIn_Release(p) ((p)->lpVtbl->Release(p))
#define IExtPackPlugIn_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPackPlugIn_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPackPlugIn_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPackPlugIn_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPackPlugIn_get_Frontend(p, aFrontend) ((p)->lpVtbl->GetFrontend(p, aFrontend))
#define IExtPackPlugIn_GetFrontend(p, aFrontend) ((p)->lpVtbl->GetFrontend(p, aFrontend))
#define IExtPackPlugIn_get_ModulePath(p, aModulePath) ((p)->lpVtbl->GetModulePath(p, aModulePath))
#define IExtPackPlugIn_GetModulePath(p, aModulePath) ((p)->lpVtbl->GetModulePath(p, aModulePath))
#endif /* VBOX_WITH_GLUE */

interface IExtPackPlugIn
{
#ifndef VBOX_WITH_GLUE
    struct IExtPackPlugIn_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtPackPlugInVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtPackPlugIn declaration */


/* Start of struct IExtPackBase declaration */
#define IEXTPACKBASE_IID_STR "f79b75d8-2890-4f34-ffff-ffffa144e82c"
#define IEXTPACKBASE_IID { \
    0xf79b75d8, 0x2890, 0x4f34, \
    { 0xff, 0xff, 0xff, 0xff, 0xa1, 0x44, 0xe8, 0x2c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtPackBase);
#ifndef VBOX_WITH_GLUE
struct IExtPackBase_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IExtPackBase *pThis, PRUnichar * *name);

    nsresult (*GetDescription)(IExtPackBase *pThis, PRUnichar * *description);

    nsresult (*GetVersion)(IExtPackBase *pThis, PRUnichar * *version);

    nsresult (*GetRevision)(IExtPackBase *pThis, PRUint32 *revision);

    nsresult (*GetEdition)(IExtPackBase *pThis, PRUnichar * *edition);

    nsresult (*GetVRDEModule)(IExtPackBase *pThis, PRUnichar * *VRDEModule);

    nsresult (*GetPlugIns)(IExtPackBase *pThis, PRUint32 *plugInsSize, IExtPackPlugIn * **plugIns);

    nsresult (*GetUsable)(IExtPackBase *pThis, PRBool *usable);

    nsresult (*GetWhyUnusable)(IExtPackBase *pThis, PRUnichar * *whyUnusable);

    nsresult (*GetShowLicense)(IExtPackBase *pThis, PRBool *showLicense);

    nsresult (*GetLicense)(IExtPackBase *pThis, PRUnichar * *license);

    nsresult (*QueryLicense)(
        IExtPackBase *pThis,
        PRUnichar * preferredLocale,
        PRUnichar * preferredLanguage,
        PRUnichar * format,
        PRUnichar * * licenseText
    );

};
#else /* VBOX_WITH_GLUE */
struct IExtPackBaseVtbl
{
    nsresult (*QueryInterface)(IExtPackBase *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtPackBase *pThis);
    nsrefcnt (*Release)(IExtPackBase *pThis);
    nsresult (*GetName)(IExtPackBase *pThis, PRUnichar * *name);

    nsresult (*GetDescription)(IExtPackBase *pThis, PRUnichar * *description);

    nsresult (*GetVersion)(IExtPackBase *pThis, PRUnichar * *version);

    nsresult (*GetRevision)(IExtPackBase *pThis, PRUint32 *revision);

    nsresult (*GetEdition)(IExtPackBase *pThis, PRUnichar * *edition);

    nsresult (*GetVRDEModule)(IExtPackBase *pThis, PRUnichar * *VRDEModule);

    nsresult (*GetPlugIns)(IExtPackBase *pThis, PRUint32 *plugInsSize, IExtPackPlugIn * **plugIns);

    nsresult (*GetUsable)(IExtPackBase *pThis, PRBool *usable);

    nsresult (*GetWhyUnusable)(IExtPackBase *pThis, PRUnichar * *whyUnusable);

    nsresult (*GetShowLicense)(IExtPackBase *pThis, PRBool *showLicense);

    nsresult (*GetLicense)(IExtPackBase *pThis, PRUnichar * *license);

    nsresult (*QueryLicense)(
        IExtPackBase *pThis,
        PRUnichar * preferredLocale,
        PRUnichar * preferredLanguage,
        PRUnichar * format,
        PRUnichar * * licenseText
    );

};
#define IExtPackBase_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtPackBase_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtPackBase_Release(p) ((p)->lpVtbl->Release(p))
#define IExtPackBase_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPackBase_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPackBase_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPackBase_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPackBase_get_Version(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IExtPackBase_GetVersion(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IExtPackBase_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IExtPackBase_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IExtPackBase_get_Edition(p, aEdition) ((p)->lpVtbl->GetEdition(p, aEdition))
#define IExtPackBase_GetEdition(p, aEdition) ((p)->lpVtbl->GetEdition(p, aEdition))
#define IExtPackBase_get_VRDEModule(p, aVRDEModule) ((p)->lpVtbl->GetVRDEModule(p, aVRDEModule))
#define IExtPackBase_GetVRDEModule(p, aVRDEModule) ((p)->lpVtbl->GetVRDEModule(p, aVRDEModule))
#define IExtPackBase_get_PlugIns(p, aPlugIns) ((p)->lpVtbl->GetPlugIns(p, aPlugIns))
#define IExtPackBase_GetPlugIns(p, aPlugIns) ((p)->lpVtbl->GetPlugIns(p, aPlugIns))
#define IExtPackBase_get_Usable(p, aUsable) ((p)->lpVtbl->GetUsable(p, aUsable))
#define IExtPackBase_GetUsable(p, aUsable) ((p)->lpVtbl->GetUsable(p, aUsable))
#define IExtPackBase_get_WhyUnusable(p, aWhyUnusable) ((p)->lpVtbl->GetWhyUnusable(p, aWhyUnusable))
#define IExtPackBase_GetWhyUnusable(p, aWhyUnusable) ((p)->lpVtbl->GetWhyUnusable(p, aWhyUnusable))
#define IExtPackBase_get_ShowLicense(p, aShowLicense) ((p)->lpVtbl->GetShowLicense(p, aShowLicense))
#define IExtPackBase_GetShowLicense(p, aShowLicense) ((p)->lpVtbl->GetShowLicense(p, aShowLicense))
#define IExtPackBase_get_License(p, aLicense) ((p)->lpVtbl->GetLicense(p, aLicense))
#define IExtPackBase_GetLicense(p, aLicense) ((p)->lpVtbl->GetLicense(p, aLicense))
#define IExtPackBase_QueryLicense(p, aPreferredLocale, aPreferredLanguage, aFormat, aLicenseText) ((p)->lpVtbl->QueryLicense(p, aPreferredLocale, aPreferredLanguage, aFormat, aLicenseText))
#endif /* VBOX_WITH_GLUE */

interface IExtPackBase
{
#ifndef VBOX_WITH_GLUE
    struct IExtPackBase_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtPackBaseVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtPackBase declaration */


/* Start of struct IExtPack declaration */
#define IEXTPACK_IID_STR "431685da-3618-4ebc-b038-833ba829b4b2"
#define IEXTPACK_IID { \
    0x431685da, 0x3618, 0x4ebc, \
    { 0xb0, 0x38, 0x83, 0x3b, 0xa8, 0x29, 0xb4, 0xb2 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtPack);
#ifndef VBOX_WITH_GLUE
struct IExtPack_vtbl
{
    struct IExtPackBase_vtbl iextpackbase;

    nsresult (*QueryObject)(
        IExtPack *pThis,
        PRUnichar * objUuid,
        nsISupports * * returnInterface
    );

};
#else /* VBOX_WITH_GLUE */
struct IExtPackVtbl
{
    nsresult (*QueryInterface)(IExtPack *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtPack *pThis);
    nsrefcnt (*Release)(IExtPack *pThis);
    nsresult (*GetName)(IExtPack *pThis, PRUnichar * *name);

    nsresult (*GetDescription)(IExtPack *pThis, PRUnichar * *description);

    nsresult (*GetVersion)(IExtPack *pThis, PRUnichar * *version);

    nsresult (*GetRevision)(IExtPack *pThis, PRUint32 *revision);

    nsresult (*GetEdition)(IExtPack *pThis, PRUnichar * *edition);

    nsresult (*GetVRDEModule)(IExtPack *pThis, PRUnichar * *VRDEModule);

    nsresult (*GetPlugIns)(IExtPack *pThis, PRUint32 *plugInsSize, IExtPackPlugIn * **plugIns);

    nsresult (*GetUsable)(IExtPack *pThis, PRBool *usable);

    nsresult (*GetWhyUnusable)(IExtPack *pThis, PRUnichar * *whyUnusable);

    nsresult (*GetShowLicense)(IExtPack *pThis, PRBool *showLicense);

    nsresult (*GetLicense)(IExtPack *pThis, PRUnichar * *license);

    nsresult (*QueryLicense)(
        IExtPack *pThis,
        PRUnichar * preferredLocale,
        PRUnichar * preferredLanguage,
        PRUnichar * format,
        PRUnichar * * licenseText
    );

    nsresult (*QueryObject)(
        IExtPack *pThis,
        PRUnichar * objUuid,
        nsISupports * * returnInterface
    );

};
#define IExtPack_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtPack_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtPack_Release(p) ((p)->lpVtbl->Release(p))
#define IExtPack_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPack_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPack_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPack_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPack_get_Version(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IExtPack_GetVersion(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IExtPack_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IExtPack_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IExtPack_get_Edition(p, aEdition) ((p)->lpVtbl->GetEdition(p, aEdition))
#define IExtPack_GetEdition(p, aEdition) ((p)->lpVtbl->GetEdition(p, aEdition))
#define IExtPack_get_VRDEModule(p, aVRDEModule) ((p)->lpVtbl->GetVRDEModule(p, aVRDEModule))
#define IExtPack_GetVRDEModule(p, aVRDEModule) ((p)->lpVtbl->GetVRDEModule(p, aVRDEModule))
#define IExtPack_get_PlugIns(p, aPlugIns) ((p)->lpVtbl->GetPlugIns(p, aPlugIns))
#define IExtPack_GetPlugIns(p, aPlugIns) ((p)->lpVtbl->GetPlugIns(p, aPlugIns))
#define IExtPack_get_Usable(p, aUsable) ((p)->lpVtbl->GetUsable(p, aUsable))
#define IExtPack_GetUsable(p, aUsable) ((p)->lpVtbl->GetUsable(p, aUsable))
#define IExtPack_get_WhyUnusable(p, aWhyUnusable) ((p)->lpVtbl->GetWhyUnusable(p, aWhyUnusable))
#define IExtPack_GetWhyUnusable(p, aWhyUnusable) ((p)->lpVtbl->GetWhyUnusable(p, aWhyUnusable))
#define IExtPack_get_ShowLicense(p, aShowLicense) ((p)->lpVtbl->GetShowLicense(p, aShowLicense))
#define IExtPack_GetShowLicense(p, aShowLicense) ((p)->lpVtbl->GetShowLicense(p, aShowLicense))
#define IExtPack_get_License(p, aLicense) ((p)->lpVtbl->GetLicense(p, aLicense))
#define IExtPack_GetLicense(p, aLicense) ((p)->lpVtbl->GetLicense(p, aLicense))
#define IExtPack_QueryLicense(p, aPreferredLocale, aPreferredLanguage, aFormat, aLicenseText) ((p)->lpVtbl->QueryLicense(p, aPreferredLocale, aPreferredLanguage, aFormat, aLicenseText))
#define IExtPack_QueryObject(p, aObjUuid, aReturnInterface) ((p)->lpVtbl->QueryObject(p, aObjUuid, aReturnInterface))
#endif /* VBOX_WITH_GLUE */

interface IExtPack
{
#ifndef VBOX_WITH_GLUE
    struct IExtPack_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtPackVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtPack declaration */


/* Start of struct IExtPackFile declaration */
#define IEXTPACKFILE_IID_STR "b6b49f55-efcc-4f08-b486-56e8d8afb10b"
#define IEXTPACKFILE_IID { \
    0xb6b49f55, 0xefcc, 0x4f08, \
    { 0xb4, 0x86, 0x56, 0xe8, 0xd8, 0xaf, 0xb1, 0x0b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtPackFile);
#ifndef VBOX_WITH_GLUE
struct IExtPackFile_vtbl
{
    struct IExtPackBase_vtbl iextpackbase;

    nsresult (*GetFilePath)(IExtPackFile *pThis, PRUnichar * *filePath);

    nsresult (*Install)(
        IExtPackFile *pThis,
        PRBool replace,
        PRUnichar * displayInfo,
        IProgress * * progess
    );

};
#else /* VBOX_WITH_GLUE */
struct IExtPackFileVtbl
{
    nsresult (*QueryInterface)(IExtPackFile *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtPackFile *pThis);
    nsrefcnt (*Release)(IExtPackFile *pThis);
    nsresult (*GetName)(IExtPackFile *pThis, PRUnichar * *name);

    nsresult (*GetDescription)(IExtPackFile *pThis, PRUnichar * *description);

    nsresult (*GetVersion)(IExtPackFile *pThis, PRUnichar * *version);

    nsresult (*GetRevision)(IExtPackFile *pThis, PRUint32 *revision);

    nsresult (*GetEdition)(IExtPackFile *pThis, PRUnichar * *edition);

    nsresult (*GetVRDEModule)(IExtPackFile *pThis, PRUnichar * *VRDEModule);

    nsresult (*GetPlugIns)(IExtPackFile *pThis, PRUint32 *plugInsSize, IExtPackPlugIn * **plugIns);

    nsresult (*GetUsable)(IExtPackFile *pThis, PRBool *usable);

    nsresult (*GetWhyUnusable)(IExtPackFile *pThis, PRUnichar * *whyUnusable);

    nsresult (*GetShowLicense)(IExtPackFile *pThis, PRBool *showLicense);

    nsresult (*GetLicense)(IExtPackFile *pThis, PRUnichar * *license);

    nsresult (*QueryLicense)(
        IExtPackFile *pThis,
        PRUnichar * preferredLocale,
        PRUnichar * preferredLanguage,
        PRUnichar * format,
        PRUnichar * * licenseText
    );

    nsresult (*GetFilePath)(IExtPackFile *pThis, PRUnichar * *filePath);

    nsresult (*Install)(
        IExtPackFile *pThis,
        PRBool replace,
        PRUnichar * displayInfo,
        IProgress * * progess
    );

};
#define IExtPackFile_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtPackFile_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtPackFile_Release(p) ((p)->lpVtbl->Release(p))
#define IExtPackFile_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPackFile_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IExtPackFile_get_Description(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPackFile_GetDescription(p, aDescription) ((p)->lpVtbl->GetDescription(p, aDescription))
#define IExtPackFile_get_Version(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IExtPackFile_GetVersion(p, aVersion) ((p)->lpVtbl->GetVersion(p, aVersion))
#define IExtPackFile_get_Revision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IExtPackFile_GetRevision(p, aRevision) ((p)->lpVtbl->GetRevision(p, aRevision))
#define IExtPackFile_get_Edition(p, aEdition) ((p)->lpVtbl->GetEdition(p, aEdition))
#define IExtPackFile_GetEdition(p, aEdition) ((p)->lpVtbl->GetEdition(p, aEdition))
#define IExtPackFile_get_VRDEModule(p, aVRDEModule) ((p)->lpVtbl->GetVRDEModule(p, aVRDEModule))
#define IExtPackFile_GetVRDEModule(p, aVRDEModule) ((p)->lpVtbl->GetVRDEModule(p, aVRDEModule))
#define IExtPackFile_get_PlugIns(p, aPlugIns) ((p)->lpVtbl->GetPlugIns(p, aPlugIns))
#define IExtPackFile_GetPlugIns(p, aPlugIns) ((p)->lpVtbl->GetPlugIns(p, aPlugIns))
#define IExtPackFile_get_Usable(p, aUsable) ((p)->lpVtbl->GetUsable(p, aUsable))
#define IExtPackFile_GetUsable(p, aUsable) ((p)->lpVtbl->GetUsable(p, aUsable))
#define IExtPackFile_get_WhyUnusable(p, aWhyUnusable) ((p)->lpVtbl->GetWhyUnusable(p, aWhyUnusable))
#define IExtPackFile_GetWhyUnusable(p, aWhyUnusable) ((p)->lpVtbl->GetWhyUnusable(p, aWhyUnusable))
#define IExtPackFile_get_ShowLicense(p, aShowLicense) ((p)->lpVtbl->GetShowLicense(p, aShowLicense))
#define IExtPackFile_GetShowLicense(p, aShowLicense) ((p)->lpVtbl->GetShowLicense(p, aShowLicense))
#define IExtPackFile_get_License(p, aLicense) ((p)->lpVtbl->GetLicense(p, aLicense))
#define IExtPackFile_GetLicense(p, aLicense) ((p)->lpVtbl->GetLicense(p, aLicense))
#define IExtPackFile_QueryLicense(p, aPreferredLocale, aPreferredLanguage, aFormat, aLicenseText) ((p)->lpVtbl->QueryLicense(p, aPreferredLocale, aPreferredLanguage, aFormat, aLicenseText))
#define IExtPackFile_get_FilePath(p, aFilePath) ((p)->lpVtbl->GetFilePath(p, aFilePath))
#define IExtPackFile_GetFilePath(p, aFilePath) ((p)->lpVtbl->GetFilePath(p, aFilePath))
#define IExtPackFile_Install(p, aReplace, aDisplayInfo, aProgess) ((p)->lpVtbl->Install(p, aReplace, aDisplayInfo, aProgess))
#endif /* VBOX_WITH_GLUE */

interface IExtPackFile
{
#ifndef VBOX_WITH_GLUE
    struct IExtPackFile_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtPackFileVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtPackFile declaration */


/* Start of struct IExtPackManager declaration */
#define IEXTPACKMANAGER_IID_STR "3295e6ce-b051-47b2-9514-2c588bfe7554"
#define IEXTPACKMANAGER_IID { \
    0x3295e6ce, 0xb051, 0x47b2, \
    { 0x95, 0x14, 0x2c, 0x58, 0x8b, 0xfe, 0x75, 0x54 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtPackManager);
#ifndef VBOX_WITH_GLUE
struct IExtPackManager_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetInstalledExtPacks)(IExtPackManager *pThis, PRUint32 *installedExtPacksSize, IExtPack * **installedExtPacks);

    nsresult (*Find)(
        IExtPackManager *pThis,
        PRUnichar * name,
        IExtPack * * returnData
    );

    nsresult (*OpenExtPackFile)(
        IExtPackManager *pThis,
        PRUnichar * path,
        IExtPackFile * * file
    );

    nsresult (*Uninstall)(
        IExtPackManager *pThis,
        PRUnichar * name,
        PRBool forcedRemoval,
        PRUnichar * displayInfo,
        IProgress * * progess
    );

    nsresult (*Cleanup)(IExtPackManager *pThis );

    nsresult (*QueryAllPlugInsForFrontend)(
        IExtPackManager *pThis,
        PRUnichar * frontendName,
        PRUint32 *plugInModulesSize,
        PRUnichar *** plugInModules
    );

    nsresult (*IsExtPackUsable)(
        IExtPackManager *pThis,
        PRUnichar * name,
        PRBool * usable
    );

};
#else /* VBOX_WITH_GLUE */
struct IExtPackManagerVtbl
{
    nsresult (*QueryInterface)(IExtPackManager *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtPackManager *pThis);
    nsrefcnt (*Release)(IExtPackManager *pThis);
    nsresult (*GetInstalledExtPacks)(IExtPackManager *pThis, PRUint32 *installedExtPacksSize, IExtPack * **installedExtPacks);

    nsresult (*Find)(
        IExtPackManager *pThis,
        PRUnichar * name,
        IExtPack * * returnData
    );

    nsresult (*OpenExtPackFile)(
        IExtPackManager *pThis,
        PRUnichar * path,
        IExtPackFile * * file
    );

    nsresult (*Uninstall)(
        IExtPackManager *pThis,
        PRUnichar * name,
        PRBool forcedRemoval,
        PRUnichar * displayInfo,
        IProgress * * progess
    );

    nsresult (*Cleanup)(IExtPackManager *pThis );

    nsresult (*QueryAllPlugInsForFrontend)(
        IExtPackManager *pThis,
        PRUnichar * frontendName,
        PRUint32 *plugInModulesSize,
        PRUnichar *** plugInModules
    );

    nsresult (*IsExtPackUsable)(
        IExtPackManager *pThis,
        PRUnichar * name,
        PRBool * usable
    );

};
#define IExtPackManager_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtPackManager_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtPackManager_Release(p) ((p)->lpVtbl->Release(p))
#define IExtPackManager_get_InstalledExtPacks(p, aInstalledExtPacks) ((p)->lpVtbl->GetInstalledExtPacks(p, aInstalledExtPacks))
#define IExtPackManager_GetInstalledExtPacks(p, aInstalledExtPacks) ((p)->lpVtbl->GetInstalledExtPacks(p, aInstalledExtPacks))
#define IExtPackManager_Find(p, aName, aReturnData) ((p)->lpVtbl->Find(p, aName, aReturnData))
#define IExtPackManager_OpenExtPackFile(p, aPath, aFile) ((p)->lpVtbl->OpenExtPackFile(p, aPath, aFile))
#define IExtPackManager_Uninstall(p, aName, aForcedRemoval, aDisplayInfo, aProgess) ((p)->lpVtbl->Uninstall(p, aName, aForcedRemoval, aDisplayInfo, aProgess))
#define IExtPackManager_Cleanup(p) ((p)->lpVtbl->Cleanup(p))
#define IExtPackManager_QueryAllPlugInsForFrontend(p, aFrontendName, aPlugInModules) ((p)->lpVtbl->QueryAllPlugInsForFrontend(p, aFrontendName, aPlugInModules))
#define IExtPackManager_IsExtPackUsable(p, aName, aUsable) ((p)->lpVtbl->IsExtPackUsable(p, aName, aUsable))
#endif /* VBOX_WITH_GLUE */

interface IExtPackManager
{
#ifndef VBOX_WITH_GLUE
    struct IExtPackManager_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtPackManagerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtPackManager declaration */


/* Start of struct IBandwidthGroup declaration */
#define IBANDWIDTHGROUP_IID_STR "badea2d7-0261-4146-89f0-6a57cc34833d"
#define IBANDWIDTHGROUP_IID { \
    0xbadea2d7, 0x0261, 0x4146, \
    { 0x89, 0xf0, 0x6a, 0x57, 0xcc, 0x34, 0x83, 0x3d } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IBandwidthGroup);
#ifndef VBOX_WITH_GLUE
struct IBandwidthGroup_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetName)(IBandwidthGroup *pThis, PRUnichar * *name);

    nsresult (*GetType)(IBandwidthGroup *pThis, PRUint32 *type);

    nsresult (*GetReference)(IBandwidthGroup *pThis, PRUint32 *reference);

    nsresult (*GetMaxBytesPerSec)(IBandwidthGroup *pThis, PRInt64 *maxBytesPerSec);
    nsresult (*SetMaxBytesPerSec)(IBandwidthGroup *pThis, PRInt64 maxBytesPerSec);

};
#else /* VBOX_WITH_GLUE */
struct IBandwidthGroupVtbl
{
    nsresult (*QueryInterface)(IBandwidthGroup *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IBandwidthGroup *pThis);
    nsrefcnt (*Release)(IBandwidthGroup *pThis);
    nsresult (*GetName)(IBandwidthGroup *pThis, PRUnichar * *name);

    nsresult (*GetType)(IBandwidthGroup *pThis, PRUint32 *type);

    nsresult (*GetReference)(IBandwidthGroup *pThis, PRUint32 *reference);

    nsresult (*GetMaxBytesPerSec)(IBandwidthGroup *pThis, PRInt64 *maxBytesPerSec);
    nsresult (*SetMaxBytesPerSec)(IBandwidthGroup *pThis, PRInt64 maxBytesPerSec);

};
#define IBandwidthGroup_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IBandwidthGroup_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IBandwidthGroup_Release(p) ((p)->lpVtbl->Release(p))
#define IBandwidthGroup_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IBandwidthGroup_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IBandwidthGroup_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IBandwidthGroup_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IBandwidthGroup_get_Reference(p, aReference) ((p)->lpVtbl->GetReference(p, aReference))
#define IBandwidthGroup_GetReference(p, aReference) ((p)->lpVtbl->GetReference(p, aReference))
#define IBandwidthGroup_get_MaxBytesPerSec(p, aMaxBytesPerSec) ((p)->lpVtbl->GetMaxBytesPerSec(p, aMaxBytesPerSec))
#define IBandwidthGroup_GetMaxBytesPerSec(p, aMaxBytesPerSec) ((p)->lpVtbl->GetMaxBytesPerSec(p, aMaxBytesPerSec))
#define IBandwidthGroup_put_MaxBytesPerSec(p, aMaxBytesPerSec) ((p)->lpVtbl->SetMaxBytesPerSec(p, aMaxBytesPerSec))
#define IBandwidthGroup_SetMaxBytesPerSec(p, aMaxBytesPerSec) ((p)->lpVtbl->SetMaxBytesPerSec(p, aMaxBytesPerSec))
#endif /* VBOX_WITH_GLUE */

interface IBandwidthGroup
{
#ifndef VBOX_WITH_GLUE
    struct IBandwidthGroup_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IBandwidthGroupVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IBandwidthGroup declaration */


/* Start of struct IBandwidthControl declaration */
#define IBANDWIDTHCONTROL_IID_STR "e2eb3930-d2f4-4f87-be17-0707e30f019f"
#define IBANDWIDTHCONTROL_IID { \
    0xe2eb3930, 0xd2f4, 0x4f87, \
    { 0xbe, 0x17, 0x07, 0x07, 0xe3, 0x0f, 0x01, 0x9f } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IBandwidthControl);
#ifndef VBOX_WITH_GLUE
struct IBandwidthControl_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetNumGroups)(IBandwidthControl *pThis, PRUint32 *numGroups);

    nsresult (*CreateBandwidthGroup)(
        IBandwidthControl *pThis,
        PRUnichar * name,
        PRUint32 type,
        PRInt64 maxBytesPerSec
    );

    nsresult (*DeleteBandwidthGroup)(
        IBandwidthControl *pThis,
        PRUnichar * name
    );

    nsresult (*GetBandwidthGroup)(
        IBandwidthControl *pThis,
        PRUnichar * name,
        IBandwidthGroup * * bandwidthGroup
    );

    nsresult (*GetAllBandwidthGroups)(
        IBandwidthControl *pThis,
        PRUint32 *bandwidthGroupsSize,
        IBandwidthGroup *** bandwidthGroups
    );

};
#else /* VBOX_WITH_GLUE */
struct IBandwidthControlVtbl
{
    nsresult (*QueryInterface)(IBandwidthControl *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IBandwidthControl *pThis);
    nsrefcnt (*Release)(IBandwidthControl *pThis);
    nsresult (*GetNumGroups)(IBandwidthControl *pThis, PRUint32 *numGroups);

    nsresult (*CreateBandwidthGroup)(
        IBandwidthControl *pThis,
        PRUnichar * name,
        PRUint32 type,
        PRInt64 maxBytesPerSec
    );

    nsresult (*DeleteBandwidthGroup)(
        IBandwidthControl *pThis,
        PRUnichar * name
    );

    nsresult (*GetBandwidthGroup)(
        IBandwidthControl *pThis,
        PRUnichar * name,
        IBandwidthGroup * * bandwidthGroup
    );

    nsresult (*GetAllBandwidthGroups)(
        IBandwidthControl *pThis,
        PRUint32 *bandwidthGroupsSize,
        IBandwidthGroup *** bandwidthGroups
    );

};
#define IBandwidthControl_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IBandwidthControl_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IBandwidthControl_Release(p) ((p)->lpVtbl->Release(p))
#define IBandwidthControl_get_NumGroups(p, aNumGroups) ((p)->lpVtbl->GetNumGroups(p, aNumGroups))
#define IBandwidthControl_GetNumGroups(p, aNumGroups) ((p)->lpVtbl->GetNumGroups(p, aNumGroups))
#define IBandwidthControl_CreateBandwidthGroup(p, aName, aType, aMaxBytesPerSec) ((p)->lpVtbl->CreateBandwidthGroup(p, aName, aType, aMaxBytesPerSec))
#define IBandwidthControl_DeleteBandwidthGroup(p, aName) ((p)->lpVtbl->DeleteBandwidthGroup(p, aName))
#define IBandwidthControl_GetBandwidthGroup(p, aName, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aName, aBandwidthGroup))
#define IBandwidthControl_GetAllBandwidthGroups(p, aBandwidthGroups) ((p)->lpVtbl->GetAllBandwidthGroups(p, aBandwidthGroups))
#endif /* VBOX_WITH_GLUE */

interface IBandwidthControl
{
#ifndef VBOX_WITH_GLUE
    struct IBandwidthControl_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IBandwidthControlVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IBandwidthControl declaration */


/* Start of struct IVirtualBoxClient declaration */
#define IVIRTUALBOXCLIENT_IID_STR "d191281f-b0cb-4d83-a8fa-0d9fd6ba234c"
#define IVIRTUALBOXCLIENT_IID { \
    0xd191281f, 0xb0cb, 0x4d83, \
    { 0xa8, 0xfa, 0x0d, 0x9f, 0xd6, 0xba, 0x23, 0x4c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVirtualBoxClient);
#ifndef VBOX_WITH_GLUE
struct IVirtualBoxClient_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetVirtualBox)(IVirtualBoxClient *pThis, IVirtualBox * *virtualBox);

    nsresult (*GetSession)(IVirtualBoxClient *pThis, ISession * *session);

    nsresult (*GetEventSource)(IVirtualBoxClient *pThis, IEventSource * *eventSource);

    nsresult (*CheckMachineError)(
        IVirtualBoxClient *pThis,
        IMachine * machine
    );

};
#else /* VBOX_WITH_GLUE */
struct IVirtualBoxClientVtbl
{
    nsresult (*QueryInterface)(IVirtualBoxClient *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVirtualBoxClient *pThis);
    nsrefcnt (*Release)(IVirtualBoxClient *pThis);
    nsresult (*GetVirtualBox)(IVirtualBoxClient *pThis, IVirtualBox * *virtualBox);

    nsresult (*GetSession)(IVirtualBoxClient *pThis, ISession * *session);

    nsresult (*GetEventSource)(IVirtualBoxClient *pThis, IEventSource * *eventSource);

    nsresult (*CheckMachineError)(
        IVirtualBoxClient *pThis,
        IMachine * machine
    );

};
#define IVirtualBoxClient_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVirtualBoxClient_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVirtualBoxClient_Release(p) ((p)->lpVtbl->Release(p))
#define IVirtualBoxClient_get_VirtualBox(p, aVirtualBox) ((p)->lpVtbl->GetVirtualBox(p, aVirtualBox))
#define IVirtualBoxClient_GetVirtualBox(p, aVirtualBox) ((p)->lpVtbl->GetVirtualBox(p, aVirtualBox))
#define IVirtualBoxClient_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IVirtualBoxClient_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IVirtualBoxClient_get_EventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IVirtualBoxClient_GetEventSource(p, aEventSource) ((p)->lpVtbl->GetEventSource(p, aEventSource))
#define IVirtualBoxClient_CheckMachineError(p, aMachine) ((p)->lpVtbl->CheckMachineError(p, aMachine))
#endif /* VBOX_WITH_GLUE */

interface IVirtualBoxClient
{
#ifndef VBOX_WITH_GLUE
    struct IVirtualBoxClient_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVirtualBoxClientVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVirtualBoxClient declaration */


/* Start of struct IEventSource declaration */
#define IEVENTSOURCE_IID_STR "9b6e1aee-35f3-4f4d-b5bb-ed0ecefd8538"
#define IEVENTSOURCE_IID { \
    0x9b6e1aee, 0x35f3, 0x4f4d, \
    { 0xb5, 0xbb, 0xed, 0x0e, 0xce, 0xfd, 0x85, 0x38 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IEventSource);
#ifndef VBOX_WITH_GLUE
struct IEventSource_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*CreateListener)(
        IEventSource *pThis,
        IEventListener * * listener
    );

    nsresult (*CreateAggregator)(
        IEventSource *pThis,
        PRUint32 subordinatesSize,
        IEventSource ** subordinates,
        IEventSource * * result
    );

    nsresult (*RegisterListener)(
        IEventSource *pThis,
        IEventListener * listener,
        PRUint32 interestingSize,
        PRUint32* interesting,
        PRBool active
    );

    nsresult (*UnregisterListener)(
        IEventSource *pThis,
        IEventListener * listener
    );

    nsresult (*FireEvent)(
        IEventSource *pThis,
        IEvent * event,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetEvent)(
        IEventSource *pThis,
        IEventListener * listener,
        PRInt32 timeout,
        IEvent * * event
    );

    nsresult (*EventProcessed)(
        IEventSource *pThis,
        IEventListener * listener,
        IEvent * event
    );

};
#else /* VBOX_WITH_GLUE */
struct IEventSourceVtbl
{
    nsresult (*QueryInterface)(IEventSource *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IEventSource *pThis);
    nsrefcnt (*Release)(IEventSource *pThis);
    nsresult (*CreateListener)(
        IEventSource *pThis,
        IEventListener * * listener
    );

    nsresult (*CreateAggregator)(
        IEventSource *pThis,
        PRUint32 subordinatesSize,
        IEventSource ** subordinates,
        IEventSource * * result
    );

    nsresult (*RegisterListener)(
        IEventSource *pThis,
        IEventListener * listener,
        PRUint32 interestingSize,
        PRUint32* interesting,
        PRBool active
    );

    nsresult (*UnregisterListener)(
        IEventSource *pThis,
        IEventListener * listener
    );

    nsresult (*FireEvent)(
        IEventSource *pThis,
        IEvent * event,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetEvent)(
        IEventSource *pThis,
        IEventListener * listener,
        PRInt32 timeout,
        IEvent * * event
    );

    nsresult (*EventProcessed)(
        IEventSource *pThis,
        IEventListener * listener,
        IEvent * event
    );

};
#define IEventSource_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IEventSource_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IEventSource_Release(p) ((p)->lpVtbl->Release(p))
#define IEventSource_CreateListener(p, aListener) ((p)->lpVtbl->CreateListener(p, aListener))
#define IEventSource_CreateAggregator(p, aSubordinates, aResult) ((p)->lpVtbl->CreateAggregator(p, aSubordinates, aResult))
#define IEventSource_RegisterListener(p, aListener, aInteresting, aActive) ((p)->lpVtbl->RegisterListener(p, aListener, aInteresting, aActive))
#define IEventSource_UnregisterListener(p, aListener) ((p)->lpVtbl->UnregisterListener(p, aListener))
#define IEventSource_FireEvent(p, aEvent, aTimeout, aResult) ((p)->lpVtbl->FireEvent(p, aEvent, aTimeout, aResult))
#define IEventSource_GetEvent(p, aListener, aTimeout, aEvent) ((p)->lpVtbl->GetEvent(p, aListener, aTimeout, aEvent))
#define IEventSource_EventProcessed(p, aListener, aEvent) ((p)->lpVtbl->EventProcessed(p, aListener, aEvent))
#endif /* VBOX_WITH_GLUE */

interface IEventSource
{
#ifndef VBOX_WITH_GLUE
    struct IEventSource_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IEventSourceVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IEventSource declaration */


/* Start of struct IEventListener declaration */
#define IEVENTLISTENER_IID_STR "67099191-32e7-4f6c-85ee-422304c71b90"
#define IEVENTLISTENER_IID { \
    0x67099191, 0x32e7, 0x4f6c, \
    { 0x85, 0xee, 0x42, 0x23, 0x04, 0xc7, 0x1b, 0x90 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IEventListener);
#ifndef VBOX_WITH_GLUE
struct IEventListener_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*HandleEvent)(
        IEventListener *pThis,
        IEvent * event
    );

};
#else /* VBOX_WITH_GLUE */
struct IEventListenerVtbl
{
    nsresult (*QueryInterface)(IEventListener *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IEventListener *pThis);
    nsrefcnt (*Release)(IEventListener *pThis);
    nsresult (*HandleEvent)(
        IEventListener *pThis,
        IEvent * event
    );

};
#define IEventListener_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IEventListener_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IEventListener_Release(p) ((p)->lpVtbl->Release(p))
#define IEventListener_HandleEvent(p, aEvent) ((p)->lpVtbl->HandleEvent(p, aEvent))
#endif /* VBOX_WITH_GLUE */

interface IEventListener
{
#ifndef VBOX_WITH_GLUE
    struct IEventListener_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IEventListenerVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IEventListener declaration */


/* Start of struct IEvent declaration */
#define IEVENT_IID_STR "0ca2adba-8f30-401b-a8cd-fe31dbe839c0"
#define IEVENT_IID { \
    0x0ca2adba, 0x8f30, 0x401b, \
    { 0xa8, 0xcd, 0xfe, 0x31, 0xdb, 0xe8, 0x39, 0xc0 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IEvent);
#ifndef VBOX_WITH_GLUE
struct IEvent_vtbl
{
    struct nsISupports_vtbl nsisupports;

    nsresult (*GetType)(IEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IEvent *pThis );

    nsresult (*WaitProcessed)(
        IEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#else /* VBOX_WITH_GLUE */
struct IEventVtbl
{
    nsresult (*QueryInterface)(IEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IEvent *pThis);
    nsrefcnt (*Release)(IEvent *pThis);
    nsresult (*GetType)(IEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IEvent *pThis );

    nsresult (*WaitProcessed)(
        IEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IEvent
{
#ifndef VBOX_WITH_GLUE
    struct IEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IEvent declaration */


/* Start of struct IReusableEvent declaration */
#define IREUSABLEEVENT_IID_STR "69bfb134-80f6-4266-8e20-16371f68fa25"
#define IREUSABLEEVENT_IID { \
    0x69bfb134, 0x80f6, 0x4266, \
    { 0x8e, 0x20, 0x16, 0x37, 0x1f, 0x68, 0xfa, 0x25 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IReusableEvent);
#ifndef VBOX_WITH_GLUE
struct IReusableEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetGeneration)(IReusableEvent *pThis, PRUint32 *generation);

    nsresult (*Reuse)(IReusableEvent *pThis );

};
#else /* VBOX_WITH_GLUE */
struct IReusableEventVtbl
{
    nsresult (*QueryInterface)(IReusableEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IReusableEvent *pThis);
    nsrefcnt (*Release)(IReusableEvent *pThis);
    nsresult (*GetType)(IReusableEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IReusableEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IReusableEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IReusableEvent *pThis );

    nsresult (*WaitProcessed)(
        IReusableEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetGeneration)(IReusableEvent *pThis, PRUint32 *generation);

    nsresult (*Reuse)(IReusableEvent *pThis );

};
#define IReusableEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IReusableEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IReusableEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IReusableEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IReusableEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IReusableEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IReusableEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IReusableEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IReusableEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IReusableEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IReusableEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IReusableEvent_get_Generation(p, aGeneration) ((p)->lpVtbl->GetGeneration(p, aGeneration))
#define IReusableEvent_GetGeneration(p, aGeneration) ((p)->lpVtbl->GetGeneration(p, aGeneration))
#define IReusableEvent_Reuse(p) ((p)->lpVtbl->Reuse(p))
#endif /* VBOX_WITH_GLUE */

interface IReusableEvent
{
#ifndef VBOX_WITH_GLUE
    struct IReusableEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IReusableEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IReusableEvent declaration */


/* Start of struct IMachineEvent declaration */
#define IMACHINEEVENT_IID_STR "92ed7b1a-0d96-40ed-ae46-a564d484325e"
#define IMACHINEEVENT_IID { \
    0x92ed7b1a, 0x0d96, 0x40ed, \
    { 0xae, 0x46, 0xa5, 0x64, 0xd4, 0x84, 0x32, 0x5e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMachineEvent);
#ifndef VBOX_WITH_GLUE
struct IMachineEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetMachineId)(IMachineEvent *pThis, PRUnichar * *machineId);

};
#else /* VBOX_WITH_GLUE */
struct IMachineEventVtbl
{
    nsresult (*QueryInterface)(IMachineEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMachineEvent *pThis);
    nsrefcnt (*Release)(IMachineEvent *pThis);
    nsresult (*GetType)(IMachineEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMachineEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMachineEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMachineEvent *pThis );

    nsresult (*WaitProcessed)(
        IMachineEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IMachineEvent *pThis, PRUnichar * *machineId);

};
#define IMachineEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMachineEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMachineEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMachineEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMachineEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMachineEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#endif /* VBOX_WITH_GLUE */

interface IMachineEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMachineEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMachineEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMachineEvent declaration */


/* Start of struct IMachineStateChangedEvent declaration */
#define IMACHINESTATECHANGEDEVENT_IID_STR "5748F794-48DF-438D-85EB-98FFD70D18C9"
#define IMACHINESTATECHANGEDEVENT_IID { \
    0x5748F794, 0x48DF, 0x438D, \
    { 0x85, 0xEB, 0x98, 0xFF, 0xD7, 0x0D, 0x18, 0xC9 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMachineStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IMachineStateChangedEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetState)(IMachineStateChangedEvent *pThis, PRUint32 *state);

};
#else /* VBOX_WITH_GLUE */
struct IMachineStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IMachineStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMachineStateChangedEvent *pThis);
    nsrefcnt (*Release)(IMachineStateChangedEvent *pThis);
    nsresult (*GetType)(IMachineStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMachineStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMachineStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMachineStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IMachineStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IMachineStateChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetState)(IMachineStateChangedEvent *pThis, PRUint32 *state);

};
#define IMachineStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMachineStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMachineStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMachineStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMachineStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMachineStateChangedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineStateChangedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineStateChangedEvent_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IMachineStateChangedEvent_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#endif /* VBOX_WITH_GLUE */

interface IMachineStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMachineStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMachineStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMachineStateChangedEvent declaration */


/* Start of struct IMachineDataChangedEvent declaration */
#define IMACHINEDATACHANGEDEVENT_IID_STR "abe94809-2e88-4436-83d7-50f3e64d0503"
#define IMACHINEDATACHANGEDEVENT_IID { \
    0xabe94809, 0x2e88, 0x4436, \
    { 0x83, 0xd7, 0x50, 0xf3, 0xe6, 0x4d, 0x05, 0x03 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMachineDataChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IMachineDataChangedEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetTemporary)(IMachineDataChangedEvent *pThis, PRBool *temporary);

};
#else /* VBOX_WITH_GLUE */
struct IMachineDataChangedEventVtbl
{
    nsresult (*QueryInterface)(IMachineDataChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMachineDataChangedEvent *pThis);
    nsrefcnt (*Release)(IMachineDataChangedEvent *pThis);
    nsresult (*GetType)(IMachineDataChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMachineDataChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMachineDataChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMachineDataChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IMachineDataChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IMachineDataChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetTemporary)(IMachineDataChangedEvent *pThis, PRBool *temporary);

};
#define IMachineDataChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMachineDataChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMachineDataChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMachineDataChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineDataChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineDataChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineDataChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineDataChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineDataChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineDataChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMachineDataChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMachineDataChangedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineDataChangedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineDataChangedEvent_get_Temporary(p, aTemporary) ((p)->lpVtbl->GetTemporary(p, aTemporary))
#define IMachineDataChangedEvent_GetTemporary(p, aTemporary) ((p)->lpVtbl->GetTemporary(p, aTemporary))
#endif /* VBOX_WITH_GLUE */

interface IMachineDataChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMachineDataChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMachineDataChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMachineDataChangedEvent declaration */


/* Start of struct IMediumRegisteredEvent declaration */
#define IMEDIUMREGISTEREDEVENT_IID_STR "53fac49a-b7f1-4a5a-a4ef-a11dd9c2a458"
#define IMEDIUMREGISTEREDEVENT_IID { \
    0x53fac49a, 0xb7f1, 0x4a5a, \
    { 0xa4, 0xef, 0xa1, 0x1d, 0xd9, 0xc2, 0xa4, 0x58 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMediumRegisteredEvent);
#ifndef VBOX_WITH_GLUE
struct IMediumRegisteredEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetMediumId)(IMediumRegisteredEvent *pThis, PRUnichar * *mediumId);

    nsresult (*GetMediumType)(IMediumRegisteredEvent *pThis, PRUint32 *mediumType);

    nsresult (*GetRegistered)(IMediumRegisteredEvent *pThis, PRBool *registered);

};
#else /* VBOX_WITH_GLUE */
struct IMediumRegisteredEventVtbl
{
    nsresult (*QueryInterface)(IMediumRegisteredEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMediumRegisteredEvent *pThis);
    nsrefcnt (*Release)(IMediumRegisteredEvent *pThis);
    nsresult (*GetType)(IMediumRegisteredEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMediumRegisteredEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMediumRegisteredEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMediumRegisteredEvent *pThis );

    nsresult (*WaitProcessed)(
        IMediumRegisteredEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMediumId)(IMediumRegisteredEvent *pThis, PRUnichar * *mediumId);

    nsresult (*GetMediumType)(IMediumRegisteredEvent *pThis, PRUint32 *mediumType);

    nsresult (*GetRegistered)(IMediumRegisteredEvent *pThis, PRBool *registered);

};
#define IMediumRegisteredEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMediumRegisteredEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMediumRegisteredEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMediumRegisteredEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMediumRegisteredEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMediumRegisteredEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMediumRegisteredEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMediumRegisteredEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMediumRegisteredEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMediumRegisteredEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMediumRegisteredEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMediumRegisteredEvent_get_MediumId(p, aMediumId) ((p)->lpVtbl->GetMediumId(p, aMediumId))
#define IMediumRegisteredEvent_GetMediumId(p, aMediumId) ((p)->lpVtbl->GetMediumId(p, aMediumId))
#define IMediumRegisteredEvent_get_MediumType(p, aMediumType) ((p)->lpVtbl->GetMediumType(p, aMediumType))
#define IMediumRegisteredEvent_GetMediumType(p, aMediumType) ((p)->lpVtbl->GetMediumType(p, aMediumType))
#define IMediumRegisteredEvent_get_Registered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#define IMediumRegisteredEvent_GetRegistered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#endif /* VBOX_WITH_GLUE */

interface IMediumRegisteredEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMediumRegisteredEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMediumRegisteredEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMediumRegisteredEvent declaration */


/* Start of struct IMachineRegisteredEvent declaration */
#define IMACHINEREGISTEREDEVENT_IID_STR "c354a762-3ff2-4f2e-8f09-07382ee25088"
#define IMACHINEREGISTEREDEVENT_IID { \
    0xc354a762, 0x3ff2, 0x4f2e, \
    { 0x8f, 0x09, 0x07, 0x38, 0x2e, 0xe2, 0x50, 0x88 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMachineRegisteredEvent);
#ifndef VBOX_WITH_GLUE
struct IMachineRegisteredEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetRegistered)(IMachineRegisteredEvent *pThis, PRBool *registered);

};
#else /* VBOX_WITH_GLUE */
struct IMachineRegisteredEventVtbl
{
    nsresult (*QueryInterface)(IMachineRegisteredEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMachineRegisteredEvent *pThis);
    nsrefcnt (*Release)(IMachineRegisteredEvent *pThis);
    nsresult (*GetType)(IMachineRegisteredEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMachineRegisteredEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMachineRegisteredEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMachineRegisteredEvent *pThis );

    nsresult (*WaitProcessed)(
        IMachineRegisteredEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IMachineRegisteredEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetRegistered)(IMachineRegisteredEvent *pThis, PRBool *registered);

};
#define IMachineRegisteredEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMachineRegisteredEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMachineRegisteredEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMachineRegisteredEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineRegisteredEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMachineRegisteredEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineRegisteredEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMachineRegisteredEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineRegisteredEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMachineRegisteredEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMachineRegisteredEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMachineRegisteredEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineRegisteredEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IMachineRegisteredEvent_get_Registered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#define IMachineRegisteredEvent_GetRegistered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#endif /* VBOX_WITH_GLUE */

interface IMachineRegisteredEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMachineRegisteredEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMachineRegisteredEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMachineRegisteredEvent declaration */


/* Start of struct ISessionStateChangedEvent declaration */
#define ISESSIONSTATECHANGEDEVENT_IID_STR "714a3eef-799a-4489-86cd-fe8e45b2ff8e"
#define ISESSIONSTATECHANGEDEVENT_IID { \
    0x714a3eef, 0x799a, 0x4489, \
    { 0x86, 0xcd, 0xfe, 0x8e, 0x45, 0xb2, 0xff, 0x8e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISessionStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct ISessionStateChangedEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetState)(ISessionStateChangedEvent *pThis, PRUint32 *state);

};
#else /* VBOX_WITH_GLUE */
struct ISessionStateChangedEventVtbl
{
    nsresult (*QueryInterface)(ISessionStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISessionStateChangedEvent *pThis);
    nsrefcnt (*Release)(ISessionStateChangedEvent *pThis);
    nsresult (*GetType)(ISessionStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISessionStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISessionStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISessionStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        ISessionStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(ISessionStateChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetState)(ISessionStateChangedEvent *pThis, PRUint32 *state);

};
#define ISessionStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISessionStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISessionStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISessionStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISessionStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISessionStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISessionStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISessionStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISessionStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISessionStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISessionStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISessionStateChangedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISessionStateChangedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISessionStateChangedEvent_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define ISessionStateChangedEvent_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#endif /* VBOX_WITH_GLUE */

interface ISessionStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISessionStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISessionStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISessionStateChangedEvent declaration */


/* Start of struct IGuestPropertyChangedEvent declaration */
#define IGUESTPROPERTYCHANGEDEVENT_IID_STR "3f63597a-26f1-4edb-8dd2-6bddd0912368"
#define IGUESTPROPERTYCHANGEDEVENT_IID { \
    0x3f63597a, 0x26f1, 0x4edb, \
    { 0x8d, 0xd2, 0x6b, 0xdd, 0xd0, 0x91, 0x23, 0x68 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestPropertyChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestPropertyChangedEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetName)(IGuestPropertyChangedEvent *pThis, PRUnichar * *name);

    nsresult (*GetValue)(IGuestPropertyChangedEvent *pThis, PRUnichar * *value);

    nsresult (*GetFlags)(IGuestPropertyChangedEvent *pThis, PRUnichar * *flags);

};
#else /* VBOX_WITH_GLUE */
struct IGuestPropertyChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestPropertyChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestPropertyChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestPropertyChangedEvent *pThis);
    nsresult (*GetType)(IGuestPropertyChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestPropertyChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestPropertyChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestPropertyChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestPropertyChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IGuestPropertyChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetName)(IGuestPropertyChangedEvent *pThis, PRUnichar * *name);

    nsresult (*GetValue)(IGuestPropertyChangedEvent *pThis, PRUnichar * *value);

    nsresult (*GetFlags)(IGuestPropertyChangedEvent *pThis, PRUnichar * *flags);

};
#define IGuestPropertyChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestPropertyChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestPropertyChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestPropertyChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestPropertyChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestPropertyChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestPropertyChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestPropertyChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestPropertyChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestPropertyChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestPropertyChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestPropertyChangedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IGuestPropertyChangedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IGuestPropertyChangedEvent_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestPropertyChangedEvent_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestPropertyChangedEvent_get_Value(p, aValue) ((p)->lpVtbl->GetValue(p, aValue))
#define IGuestPropertyChangedEvent_GetValue(p, aValue) ((p)->lpVtbl->GetValue(p, aValue))
#define IGuestPropertyChangedEvent_get_Flags(p, aFlags) ((p)->lpVtbl->GetFlags(p, aFlags))
#define IGuestPropertyChangedEvent_GetFlags(p, aFlags) ((p)->lpVtbl->GetFlags(p, aFlags))
#endif /* VBOX_WITH_GLUE */

interface IGuestPropertyChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestPropertyChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestPropertyChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestPropertyChangedEvent declaration */


/* Start of struct ISnapshotEvent declaration */
#define ISNAPSHOTEVENT_IID_STR "21637b0e-34b8-42d3-acfb-7e96daf77c22"
#define ISNAPSHOTEVENT_IID { \
    0x21637b0e, 0x34b8, 0x42d3, \
    { 0xac, 0xfb, 0x7e, 0x96, 0xda, 0xf7, 0x7c, 0x22 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISnapshotEvent);
#ifndef VBOX_WITH_GLUE
struct ISnapshotEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetSnapshotId)(ISnapshotEvent *pThis, PRUnichar * *snapshotId);

};
#else /* VBOX_WITH_GLUE */
struct ISnapshotEventVtbl
{
    nsresult (*QueryInterface)(ISnapshotEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISnapshotEvent *pThis);
    nsrefcnt (*Release)(ISnapshotEvent *pThis);
    nsresult (*GetType)(ISnapshotEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISnapshotEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISnapshotEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISnapshotEvent *pThis );

    nsresult (*WaitProcessed)(
        ISnapshotEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(ISnapshotEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetSnapshotId)(ISnapshotEvent *pThis, PRUnichar * *snapshotId);

};
#define ISnapshotEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISnapshotEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISnapshotEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISnapshotEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISnapshotEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISnapshotEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotEvent_get_SnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#define ISnapshotEvent_GetSnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#endif /* VBOX_WITH_GLUE */

interface ISnapshotEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISnapshotEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISnapshotEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISnapshotEvent declaration */


/* Start of struct ISnapshotTakenEvent declaration */
#define ISNAPSHOTTAKENEVENT_IID_STR "d27c0b3d-6038-422c-b45e-6d4a0503d9f1"
#define ISNAPSHOTTAKENEVENT_IID { \
    0xd27c0b3d, 0x6038, 0x422c, \
    { 0xb4, 0x5e, 0x6d, 0x4a, 0x05, 0x03, 0xd9, 0xf1 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISnapshotTakenEvent);
#ifndef VBOX_WITH_GLUE
struct ISnapshotTakenEvent_vtbl
{
    struct ISnapshotEvent_vtbl isnapshotevent;

};
#else /* VBOX_WITH_GLUE */
struct ISnapshotTakenEventVtbl
{
    nsresult (*QueryInterface)(ISnapshotTakenEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISnapshotTakenEvent *pThis);
    nsrefcnt (*Release)(ISnapshotTakenEvent *pThis);
    nsresult (*GetType)(ISnapshotTakenEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISnapshotTakenEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISnapshotTakenEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISnapshotTakenEvent *pThis );

    nsresult (*WaitProcessed)(
        ISnapshotTakenEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(ISnapshotTakenEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetSnapshotId)(ISnapshotTakenEvent *pThis, PRUnichar * *snapshotId);

};
#define ISnapshotTakenEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISnapshotTakenEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISnapshotTakenEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISnapshotTakenEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotTakenEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotTakenEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotTakenEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotTakenEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotTakenEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotTakenEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISnapshotTakenEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISnapshotTakenEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotTakenEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotTakenEvent_get_SnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#define ISnapshotTakenEvent_GetSnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#endif /* VBOX_WITH_GLUE */

interface ISnapshotTakenEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISnapshotTakenEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISnapshotTakenEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISnapshotTakenEvent declaration */


/* Start of struct ISnapshotDeletedEvent declaration */
#define ISNAPSHOTDELETEDEVENT_IID_STR "c48f3401-4a9e-43f4-b7a7-54bd285e22f4"
#define ISNAPSHOTDELETEDEVENT_IID { \
    0xc48f3401, 0x4a9e, 0x43f4, \
    { 0xb7, 0xa7, 0x54, 0xbd, 0x28, 0x5e, 0x22, 0xf4 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISnapshotDeletedEvent);
#ifndef VBOX_WITH_GLUE
struct ISnapshotDeletedEvent_vtbl
{
    struct ISnapshotEvent_vtbl isnapshotevent;

};
#else /* VBOX_WITH_GLUE */
struct ISnapshotDeletedEventVtbl
{
    nsresult (*QueryInterface)(ISnapshotDeletedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISnapshotDeletedEvent *pThis);
    nsrefcnt (*Release)(ISnapshotDeletedEvent *pThis);
    nsresult (*GetType)(ISnapshotDeletedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISnapshotDeletedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISnapshotDeletedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISnapshotDeletedEvent *pThis );

    nsresult (*WaitProcessed)(
        ISnapshotDeletedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(ISnapshotDeletedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetSnapshotId)(ISnapshotDeletedEvent *pThis, PRUnichar * *snapshotId);

};
#define ISnapshotDeletedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISnapshotDeletedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISnapshotDeletedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISnapshotDeletedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotDeletedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotDeletedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotDeletedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotDeletedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotDeletedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotDeletedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISnapshotDeletedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISnapshotDeletedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotDeletedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotDeletedEvent_get_SnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#define ISnapshotDeletedEvent_GetSnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#endif /* VBOX_WITH_GLUE */

interface ISnapshotDeletedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISnapshotDeletedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISnapshotDeletedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISnapshotDeletedEvent declaration */


/* Start of struct ISnapshotChangedEvent declaration */
#define ISNAPSHOTCHANGEDEVENT_IID_STR "07541941-8079-447a-a33e-47a69c7980db"
#define ISNAPSHOTCHANGEDEVENT_IID { \
    0x07541941, 0x8079, 0x447a, \
    { 0xa3, 0x3e, 0x47, 0xa6, 0x9c, 0x79, 0x80, 0xdb } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISnapshotChangedEvent);
#ifndef VBOX_WITH_GLUE
struct ISnapshotChangedEvent_vtbl
{
    struct ISnapshotEvent_vtbl isnapshotevent;

};
#else /* VBOX_WITH_GLUE */
struct ISnapshotChangedEventVtbl
{
    nsresult (*QueryInterface)(ISnapshotChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISnapshotChangedEvent *pThis);
    nsrefcnt (*Release)(ISnapshotChangedEvent *pThis);
    nsresult (*GetType)(ISnapshotChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISnapshotChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISnapshotChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISnapshotChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        ISnapshotChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(ISnapshotChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetSnapshotId)(ISnapshotChangedEvent *pThis, PRUnichar * *snapshotId);

};
#define ISnapshotChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISnapshotChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISnapshotChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISnapshotChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISnapshotChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISnapshotChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISnapshotChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISnapshotChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISnapshotChangedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotChangedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define ISnapshotChangedEvent_get_SnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#define ISnapshotChangedEvent_GetSnapshotId(p, aSnapshotId) ((p)->lpVtbl->GetSnapshotId(p, aSnapshotId))
#endif /* VBOX_WITH_GLUE */

interface ISnapshotChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISnapshotChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISnapshotChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISnapshotChangedEvent declaration */


/* Start of struct IMousePointerShapeChangedEvent declaration */
#define IMOUSEPOINTERSHAPECHANGEDEVENT_IID_STR "a6dcf6e8-416b-4181-8c4a-45ec95177aef"
#define IMOUSEPOINTERSHAPECHANGEDEVENT_IID { \
    0xa6dcf6e8, 0x416b, 0x4181, \
    { 0x8c, 0x4a, 0x45, 0xec, 0x95, 0x17, 0x7a, 0xef } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMousePointerShapeChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IMousePointerShapeChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetVisible)(IMousePointerShapeChangedEvent *pThis, PRBool *visible);

    nsresult (*GetAlpha)(IMousePointerShapeChangedEvent *pThis, PRBool *alpha);

    nsresult (*GetXhot)(IMousePointerShapeChangedEvent *pThis, PRUint32 *xhot);

    nsresult (*GetYhot)(IMousePointerShapeChangedEvent *pThis, PRUint32 *yhot);

    nsresult (*GetWidth)(IMousePointerShapeChangedEvent *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IMousePointerShapeChangedEvent *pThis, PRUint32 *height);

    nsresult (*GetShape)(IMousePointerShapeChangedEvent *pThis, PRUint32 *shapeSize, PRUint8 **shape);

};
#else /* VBOX_WITH_GLUE */
struct IMousePointerShapeChangedEventVtbl
{
    nsresult (*QueryInterface)(IMousePointerShapeChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMousePointerShapeChangedEvent *pThis);
    nsrefcnt (*Release)(IMousePointerShapeChangedEvent *pThis);
    nsresult (*GetType)(IMousePointerShapeChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMousePointerShapeChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMousePointerShapeChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMousePointerShapeChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IMousePointerShapeChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetVisible)(IMousePointerShapeChangedEvent *pThis, PRBool *visible);

    nsresult (*GetAlpha)(IMousePointerShapeChangedEvent *pThis, PRBool *alpha);

    nsresult (*GetXhot)(IMousePointerShapeChangedEvent *pThis, PRUint32 *xhot);

    nsresult (*GetYhot)(IMousePointerShapeChangedEvent *pThis, PRUint32 *yhot);

    nsresult (*GetWidth)(IMousePointerShapeChangedEvent *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IMousePointerShapeChangedEvent *pThis, PRUint32 *height);

    nsresult (*GetShape)(IMousePointerShapeChangedEvent *pThis, PRUint32 *shapeSize, PRUint8 **shape);

};
#define IMousePointerShapeChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMousePointerShapeChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMousePointerShapeChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMousePointerShapeChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMousePointerShapeChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMousePointerShapeChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMousePointerShapeChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMousePointerShapeChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMousePointerShapeChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMousePointerShapeChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMousePointerShapeChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMousePointerShapeChangedEvent_get_Visible(p, aVisible) ((p)->lpVtbl->GetVisible(p, aVisible))
#define IMousePointerShapeChangedEvent_GetVisible(p, aVisible) ((p)->lpVtbl->GetVisible(p, aVisible))
#define IMousePointerShapeChangedEvent_get_Alpha(p, aAlpha) ((p)->lpVtbl->GetAlpha(p, aAlpha))
#define IMousePointerShapeChangedEvent_GetAlpha(p, aAlpha) ((p)->lpVtbl->GetAlpha(p, aAlpha))
#define IMousePointerShapeChangedEvent_get_Xhot(p, aXhot) ((p)->lpVtbl->GetXhot(p, aXhot))
#define IMousePointerShapeChangedEvent_GetXhot(p, aXhot) ((p)->lpVtbl->GetXhot(p, aXhot))
#define IMousePointerShapeChangedEvent_get_Yhot(p, aYhot) ((p)->lpVtbl->GetYhot(p, aYhot))
#define IMousePointerShapeChangedEvent_GetYhot(p, aYhot) ((p)->lpVtbl->GetYhot(p, aYhot))
#define IMousePointerShapeChangedEvent_get_Width(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IMousePointerShapeChangedEvent_GetWidth(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IMousePointerShapeChangedEvent_get_Height(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IMousePointerShapeChangedEvent_GetHeight(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IMousePointerShapeChangedEvent_get_Shape(p, aShape) ((p)->lpVtbl->GetShape(p, aShape))
#define IMousePointerShapeChangedEvent_GetShape(p, aShape) ((p)->lpVtbl->GetShape(p, aShape))
#endif /* VBOX_WITH_GLUE */

interface IMousePointerShapeChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMousePointerShapeChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMousePointerShapeChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMousePointerShapeChangedEvent declaration */


/* Start of struct IMouseCapabilityChangedEvent declaration */
#define IMOUSECAPABILITYCHANGEDEVENT_IID_STR "70e7779a-e64a-4908-804e-371cad23a756"
#define IMOUSECAPABILITYCHANGEDEVENT_IID { \
    0x70e7779a, 0xe64a, 0x4908, \
    { 0x80, 0x4e, 0x37, 0x1c, 0xad, 0x23, 0xa7, 0x56 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMouseCapabilityChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IMouseCapabilityChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetSupportsAbsolute)(IMouseCapabilityChangedEvent *pThis, PRBool *supportsAbsolute);

    nsresult (*GetSupportsRelative)(IMouseCapabilityChangedEvent *pThis, PRBool *supportsRelative);

    nsresult (*GetSupportsMultiTouch)(IMouseCapabilityChangedEvent *pThis, PRBool *supportsMultiTouch);

    nsresult (*GetNeedsHostCursor)(IMouseCapabilityChangedEvent *pThis, PRBool *needsHostCursor);

};
#else /* VBOX_WITH_GLUE */
struct IMouseCapabilityChangedEventVtbl
{
    nsresult (*QueryInterface)(IMouseCapabilityChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMouseCapabilityChangedEvent *pThis);
    nsrefcnt (*Release)(IMouseCapabilityChangedEvent *pThis);
    nsresult (*GetType)(IMouseCapabilityChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMouseCapabilityChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMouseCapabilityChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMouseCapabilityChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IMouseCapabilityChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSupportsAbsolute)(IMouseCapabilityChangedEvent *pThis, PRBool *supportsAbsolute);

    nsresult (*GetSupportsRelative)(IMouseCapabilityChangedEvent *pThis, PRBool *supportsRelative);

    nsresult (*GetSupportsMultiTouch)(IMouseCapabilityChangedEvent *pThis, PRBool *supportsMultiTouch);

    nsresult (*GetNeedsHostCursor)(IMouseCapabilityChangedEvent *pThis, PRBool *needsHostCursor);

};
#define IMouseCapabilityChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMouseCapabilityChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMouseCapabilityChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMouseCapabilityChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMouseCapabilityChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMouseCapabilityChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMouseCapabilityChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMouseCapabilityChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMouseCapabilityChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMouseCapabilityChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMouseCapabilityChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMouseCapabilityChangedEvent_get_SupportsAbsolute(p, aSupportsAbsolute) ((p)->lpVtbl->GetSupportsAbsolute(p, aSupportsAbsolute))
#define IMouseCapabilityChangedEvent_GetSupportsAbsolute(p, aSupportsAbsolute) ((p)->lpVtbl->GetSupportsAbsolute(p, aSupportsAbsolute))
#define IMouseCapabilityChangedEvent_get_SupportsRelative(p, aSupportsRelative) ((p)->lpVtbl->GetSupportsRelative(p, aSupportsRelative))
#define IMouseCapabilityChangedEvent_GetSupportsRelative(p, aSupportsRelative) ((p)->lpVtbl->GetSupportsRelative(p, aSupportsRelative))
#define IMouseCapabilityChangedEvent_get_SupportsMultiTouch(p, aSupportsMultiTouch) ((p)->lpVtbl->GetSupportsMultiTouch(p, aSupportsMultiTouch))
#define IMouseCapabilityChangedEvent_GetSupportsMultiTouch(p, aSupportsMultiTouch) ((p)->lpVtbl->GetSupportsMultiTouch(p, aSupportsMultiTouch))
#define IMouseCapabilityChangedEvent_get_NeedsHostCursor(p, aNeedsHostCursor) ((p)->lpVtbl->GetNeedsHostCursor(p, aNeedsHostCursor))
#define IMouseCapabilityChangedEvent_GetNeedsHostCursor(p, aNeedsHostCursor) ((p)->lpVtbl->GetNeedsHostCursor(p, aNeedsHostCursor))
#endif /* VBOX_WITH_GLUE */

interface IMouseCapabilityChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMouseCapabilityChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMouseCapabilityChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMouseCapabilityChangedEvent declaration */


/* Start of struct IKeyboardLedsChangedEvent declaration */
#define IKEYBOARDLEDSCHANGEDEVENT_IID_STR "6DDEF35E-4737-457B-99FC-BC52C851A44F"
#define IKEYBOARDLEDSCHANGEDEVENT_IID { \
    0x6DDEF35E, 0x4737, 0x457B, \
    { 0x99, 0xFC, 0xBC, 0x52, 0xC8, 0x51, 0xA4, 0x4F } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IKeyboardLedsChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IKeyboardLedsChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetNumLock)(IKeyboardLedsChangedEvent *pThis, PRBool *numLock);

    nsresult (*GetCapsLock)(IKeyboardLedsChangedEvent *pThis, PRBool *capsLock);

    nsresult (*GetScrollLock)(IKeyboardLedsChangedEvent *pThis, PRBool *scrollLock);

};
#else /* VBOX_WITH_GLUE */
struct IKeyboardLedsChangedEventVtbl
{
    nsresult (*QueryInterface)(IKeyboardLedsChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IKeyboardLedsChangedEvent *pThis);
    nsrefcnt (*Release)(IKeyboardLedsChangedEvent *pThis);
    nsresult (*GetType)(IKeyboardLedsChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IKeyboardLedsChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IKeyboardLedsChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IKeyboardLedsChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IKeyboardLedsChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNumLock)(IKeyboardLedsChangedEvent *pThis, PRBool *numLock);

    nsresult (*GetCapsLock)(IKeyboardLedsChangedEvent *pThis, PRBool *capsLock);

    nsresult (*GetScrollLock)(IKeyboardLedsChangedEvent *pThis, PRBool *scrollLock);

};
#define IKeyboardLedsChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IKeyboardLedsChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IKeyboardLedsChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IKeyboardLedsChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IKeyboardLedsChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IKeyboardLedsChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IKeyboardLedsChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IKeyboardLedsChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IKeyboardLedsChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IKeyboardLedsChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IKeyboardLedsChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IKeyboardLedsChangedEvent_get_NumLock(p, aNumLock) ((p)->lpVtbl->GetNumLock(p, aNumLock))
#define IKeyboardLedsChangedEvent_GetNumLock(p, aNumLock) ((p)->lpVtbl->GetNumLock(p, aNumLock))
#define IKeyboardLedsChangedEvent_get_CapsLock(p, aCapsLock) ((p)->lpVtbl->GetCapsLock(p, aCapsLock))
#define IKeyboardLedsChangedEvent_GetCapsLock(p, aCapsLock) ((p)->lpVtbl->GetCapsLock(p, aCapsLock))
#define IKeyboardLedsChangedEvent_get_ScrollLock(p, aScrollLock) ((p)->lpVtbl->GetScrollLock(p, aScrollLock))
#define IKeyboardLedsChangedEvent_GetScrollLock(p, aScrollLock) ((p)->lpVtbl->GetScrollLock(p, aScrollLock))
#endif /* VBOX_WITH_GLUE */

interface IKeyboardLedsChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IKeyboardLedsChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IKeyboardLedsChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IKeyboardLedsChangedEvent declaration */


/* Start of struct IStateChangedEvent declaration */
#define ISTATECHANGEDEVENT_IID_STR "4376693C-CF37-453B-9289-3B0F521CAF27"
#define ISTATECHANGEDEVENT_IID { \
    0x4376693C, 0xCF37, 0x453B, \
    { 0x92, 0x89, 0x3B, 0x0F, 0x52, 0x1C, 0xAF, 0x27 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IStateChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetState)(IStateChangedEvent *pThis, PRUint32 *state);

};
#else /* VBOX_WITH_GLUE */
struct IStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IStateChangedEvent *pThis);
    nsrefcnt (*Release)(IStateChangedEvent *pThis);
    nsresult (*GetType)(IStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetState)(IStateChangedEvent *pThis, PRUint32 *state);

};
#define IStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IStateChangedEvent_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IStateChangedEvent_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#endif /* VBOX_WITH_GLUE */

interface IStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IStateChangedEvent declaration */


/* Start of struct IAdditionsStateChangedEvent declaration */
#define IADDITIONSSTATECHANGEDEVENT_IID_STR "D70F7915-DA7C-44C8-A7AC-9F173490446A"
#define IADDITIONSSTATECHANGEDEVENT_IID { \
    0xD70F7915, 0xDA7C, 0x44C8, \
    { 0xA7, 0xAC, 0x9F, 0x17, 0x34, 0x90, 0x44, 0x6A } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IAdditionsStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IAdditionsStateChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IAdditionsStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IAdditionsStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IAdditionsStateChangedEvent *pThis);
    nsrefcnt (*Release)(IAdditionsStateChangedEvent *pThis);
    nsresult (*GetType)(IAdditionsStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IAdditionsStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IAdditionsStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IAdditionsStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IAdditionsStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IAdditionsStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IAdditionsStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IAdditionsStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IAdditionsStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IAdditionsStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IAdditionsStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IAdditionsStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IAdditionsStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IAdditionsStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IAdditionsStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IAdditionsStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IAdditionsStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IAdditionsStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IAdditionsStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IAdditionsStateChangedEvent declaration */


/* Start of struct INetworkAdapterChangedEvent declaration */
#define INETWORKADAPTERCHANGEDEVENT_IID_STR "08889892-1EC6-4883-801D-77F56CFD0103"
#define INETWORKADAPTERCHANGEDEVENT_IID { \
    0x08889892, 0x1EC6, 0x4883, \
    { 0x80, 0x1D, 0x77, 0xF5, 0x6C, 0xFD, 0x01, 0x03 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INetworkAdapterChangedEvent);
#ifndef VBOX_WITH_GLUE
struct INetworkAdapterChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetNetworkAdapter)(INetworkAdapterChangedEvent *pThis, INetworkAdapter * *networkAdapter);

};
#else /* VBOX_WITH_GLUE */
struct INetworkAdapterChangedEventVtbl
{
    nsresult (*QueryInterface)(INetworkAdapterChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INetworkAdapterChangedEvent *pThis);
    nsrefcnt (*Release)(INetworkAdapterChangedEvent *pThis);
    nsresult (*GetType)(INetworkAdapterChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INetworkAdapterChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INetworkAdapterChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INetworkAdapterChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        INetworkAdapterChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkAdapter)(INetworkAdapterChangedEvent *pThis, INetworkAdapter * *networkAdapter);

};
#define INetworkAdapterChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INetworkAdapterChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INetworkAdapterChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INetworkAdapterChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INetworkAdapterChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INetworkAdapterChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INetworkAdapterChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INetworkAdapterChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INetworkAdapterChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INetworkAdapterChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INetworkAdapterChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INetworkAdapterChangedEvent_get_NetworkAdapter(p, aNetworkAdapter) ((p)->lpVtbl->GetNetworkAdapter(p, aNetworkAdapter))
#define INetworkAdapterChangedEvent_GetNetworkAdapter(p, aNetworkAdapter) ((p)->lpVtbl->GetNetworkAdapter(p, aNetworkAdapter))
#endif /* VBOX_WITH_GLUE */

interface INetworkAdapterChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct INetworkAdapterChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INetworkAdapterChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INetworkAdapterChangedEvent declaration */


/* Start of struct ISerialPortChangedEvent declaration */
#define ISERIALPORTCHANGEDEVENT_IID_STR "3BA329DC-659C-488B-835C-4ECA7AE71C6C"
#define ISERIALPORTCHANGEDEVENT_IID { \
    0x3BA329DC, 0x659C, 0x488B, \
    { 0x83, 0x5C, 0x4E, 0xCA, 0x7A, 0xE7, 0x1C, 0x6C } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISerialPortChangedEvent);
#ifndef VBOX_WITH_GLUE
struct ISerialPortChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetSerialPort)(ISerialPortChangedEvent *pThis, ISerialPort * *serialPort);

};
#else /* VBOX_WITH_GLUE */
struct ISerialPortChangedEventVtbl
{
    nsresult (*QueryInterface)(ISerialPortChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISerialPortChangedEvent *pThis);
    nsrefcnt (*Release)(ISerialPortChangedEvent *pThis);
    nsresult (*GetType)(ISerialPortChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISerialPortChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISerialPortChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISerialPortChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        ISerialPortChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSerialPort)(ISerialPortChangedEvent *pThis, ISerialPort * *serialPort);

};
#define ISerialPortChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISerialPortChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISerialPortChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISerialPortChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISerialPortChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISerialPortChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISerialPortChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISerialPortChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISerialPortChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISerialPortChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISerialPortChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISerialPortChangedEvent_get_SerialPort(p, aSerialPort) ((p)->lpVtbl->GetSerialPort(p, aSerialPort))
#define ISerialPortChangedEvent_GetSerialPort(p, aSerialPort) ((p)->lpVtbl->GetSerialPort(p, aSerialPort))
#endif /* VBOX_WITH_GLUE */

interface ISerialPortChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISerialPortChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISerialPortChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISerialPortChangedEvent declaration */


/* Start of struct IParallelPortChangedEvent declaration */
#define IPARALLELPORTCHANGEDEVENT_IID_STR "813C99FC-9849-4F47-813E-24A75DC85615"
#define IPARALLELPORTCHANGEDEVENT_IID { \
    0x813C99FC, 0x9849, 0x4F47, \
    { 0x81, 0x3E, 0x24, 0xA7, 0x5D, 0xC8, 0x56, 0x15 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IParallelPortChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IParallelPortChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetParallelPort)(IParallelPortChangedEvent *pThis, IParallelPort * *parallelPort);

};
#else /* VBOX_WITH_GLUE */
struct IParallelPortChangedEventVtbl
{
    nsresult (*QueryInterface)(IParallelPortChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IParallelPortChangedEvent *pThis);
    nsrefcnt (*Release)(IParallelPortChangedEvent *pThis);
    nsresult (*GetType)(IParallelPortChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IParallelPortChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IParallelPortChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IParallelPortChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IParallelPortChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetParallelPort)(IParallelPortChangedEvent *pThis, IParallelPort * *parallelPort);

};
#define IParallelPortChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IParallelPortChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IParallelPortChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IParallelPortChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IParallelPortChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IParallelPortChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IParallelPortChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IParallelPortChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IParallelPortChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IParallelPortChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IParallelPortChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IParallelPortChangedEvent_get_ParallelPort(p, aParallelPort) ((p)->lpVtbl->GetParallelPort(p, aParallelPort))
#define IParallelPortChangedEvent_GetParallelPort(p, aParallelPort) ((p)->lpVtbl->GetParallelPort(p, aParallelPort))
#endif /* VBOX_WITH_GLUE */

interface IParallelPortChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IParallelPortChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IParallelPortChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IParallelPortChangedEvent declaration */


/* Start of struct IStorageControllerChangedEvent declaration */
#define ISTORAGECONTROLLERCHANGEDEVENT_IID_STR "715212BF-DA59-426E-8230-3831FAA52C56"
#define ISTORAGECONTROLLERCHANGEDEVENT_IID { \
    0x715212BF, 0xDA59, 0x426E, \
    { 0x82, 0x30, 0x38, 0x31, 0xFA, 0xA5, 0x2C, 0x56 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IStorageControllerChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IStorageControllerChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IStorageControllerChangedEventVtbl
{
    nsresult (*QueryInterface)(IStorageControllerChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IStorageControllerChangedEvent *pThis);
    nsrefcnt (*Release)(IStorageControllerChangedEvent *pThis);
    nsresult (*GetType)(IStorageControllerChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IStorageControllerChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IStorageControllerChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IStorageControllerChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IStorageControllerChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IStorageControllerChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IStorageControllerChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IStorageControllerChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IStorageControllerChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IStorageControllerChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IStorageControllerChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IStorageControllerChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IStorageControllerChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IStorageControllerChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IStorageControllerChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IStorageControllerChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IStorageControllerChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IStorageControllerChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IStorageControllerChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IStorageControllerChangedEvent declaration */


/* Start of struct IMediumChangedEvent declaration */
#define IMEDIUMCHANGEDEVENT_IID_STR "0FE2DA40-5637-472A-9736-72019EABD7DE"
#define IMEDIUMCHANGEDEVENT_IID { \
    0x0FE2DA40, 0x5637, 0x472A, \
    { 0x97, 0x36, 0x72, 0x01, 0x9E, 0xAB, 0xD7, 0xDE } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IMediumChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IMediumChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetMediumAttachment)(IMediumChangedEvent *pThis, IMediumAttachment * *mediumAttachment);

};
#else /* VBOX_WITH_GLUE */
struct IMediumChangedEventVtbl
{
    nsresult (*QueryInterface)(IMediumChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IMediumChangedEvent *pThis);
    nsrefcnt (*Release)(IMediumChangedEvent *pThis);
    nsresult (*GetType)(IMediumChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IMediumChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IMediumChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IMediumChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IMediumChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMediumAttachment)(IMediumChangedEvent *pThis, IMediumAttachment * *mediumAttachment);

};
#define IMediumChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IMediumChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IMediumChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IMediumChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMediumChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IMediumChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMediumChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IMediumChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMediumChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IMediumChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IMediumChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IMediumChangedEvent_get_MediumAttachment(p, aMediumAttachment) ((p)->lpVtbl->GetMediumAttachment(p, aMediumAttachment))
#define IMediumChangedEvent_GetMediumAttachment(p, aMediumAttachment) ((p)->lpVtbl->GetMediumAttachment(p, aMediumAttachment))
#endif /* VBOX_WITH_GLUE */

interface IMediumChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IMediumChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IMediumChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IMediumChangedEvent declaration */


/* Start of struct IClipboardModeChangedEvent declaration */
#define ICLIPBOARDMODECHANGEDEVENT_IID_STR "cac21692-7997-4595-a731-3a509db604e5"
#define ICLIPBOARDMODECHANGEDEVENT_IID { \
    0xcac21692, 0x7997, 0x4595, \
    { 0xa7, 0x31, 0x3a, 0x50, 0x9d, 0xb6, 0x04, 0xe5 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IClipboardModeChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IClipboardModeChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetClipboardMode)(IClipboardModeChangedEvent *pThis, PRUint32 *clipboardMode);

};
#else /* VBOX_WITH_GLUE */
struct IClipboardModeChangedEventVtbl
{
    nsresult (*QueryInterface)(IClipboardModeChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IClipboardModeChangedEvent *pThis);
    nsrefcnt (*Release)(IClipboardModeChangedEvent *pThis);
    nsresult (*GetType)(IClipboardModeChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IClipboardModeChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IClipboardModeChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IClipboardModeChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IClipboardModeChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetClipboardMode)(IClipboardModeChangedEvent *pThis, PRUint32 *clipboardMode);

};
#define IClipboardModeChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IClipboardModeChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IClipboardModeChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IClipboardModeChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IClipboardModeChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IClipboardModeChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IClipboardModeChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IClipboardModeChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IClipboardModeChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IClipboardModeChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IClipboardModeChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IClipboardModeChangedEvent_get_ClipboardMode(p, aClipboardMode) ((p)->lpVtbl->GetClipboardMode(p, aClipboardMode))
#define IClipboardModeChangedEvent_GetClipboardMode(p, aClipboardMode) ((p)->lpVtbl->GetClipboardMode(p, aClipboardMode))
#endif /* VBOX_WITH_GLUE */

interface IClipboardModeChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IClipboardModeChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IClipboardModeChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IClipboardModeChangedEvent declaration */


/* Start of struct IDragAndDropModeChangedEvent declaration */
#define IDRAGANDDROPMODECHANGEDEVENT_IID_STR "e90b8850-ac8e-4dff-8059-4100ae2c3c3d"
#define IDRAGANDDROPMODECHANGEDEVENT_IID { \
    0xe90b8850, 0xac8e, 0x4dff, \
    { 0x80, 0x59, 0x41, 0x00, 0xae, 0x2c, 0x3c, 0x3d } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IDragAndDropModeChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IDragAndDropModeChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetDragAndDropMode)(IDragAndDropModeChangedEvent *pThis, PRUint32 *dragAndDropMode);

};
#else /* VBOX_WITH_GLUE */
struct IDragAndDropModeChangedEventVtbl
{
    nsresult (*QueryInterface)(IDragAndDropModeChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IDragAndDropModeChangedEvent *pThis);
    nsrefcnt (*Release)(IDragAndDropModeChangedEvent *pThis);
    nsresult (*GetType)(IDragAndDropModeChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IDragAndDropModeChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IDragAndDropModeChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IDragAndDropModeChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IDragAndDropModeChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetDragAndDropMode)(IDragAndDropModeChangedEvent *pThis, PRUint32 *dragAndDropMode);

};
#define IDragAndDropModeChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IDragAndDropModeChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IDragAndDropModeChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IDragAndDropModeChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IDragAndDropModeChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IDragAndDropModeChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IDragAndDropModeChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IDragAndDropModeChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IDragAndDropModeChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IDragAndDropModeChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IDragAndDropModeChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IDragAndDropModeChangedEvent_get_DragAndDropMode(p, aDragAndDropMode) ((p)->lpVtbl->GetDragAndDropMode(p, aDragAndDropMode))
#define IDragAndDropModeChangedEvent_GetDragAndDropMode(p, aDragAndDropMode) ((p)->lpVtbl->GetDragAndDropMode(p, aDragAndDropMode))
#endif /* VBOX_WITH_GLUE */

interface IDragAndDropModeChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IDragAndDropModeChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IDragAndDropModeChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IDragAndDropModeChangedEvent declaration */


/* Start of struct ICPUChangedEvent declaration */
#define ICPUCHANGEDEVENT_IID_STR "4da2dec7-71b2-4817-9a64-4ed12c17388e"
#define ICPUCHANGEDEVENT_IID { \
    0x4da2dec7, 0x71b2, 0x4817, \
    { 0x9a, 0x64, 0x4e, 0xd1, 0x2c, 0x17, 0x38, 0x8e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ICPUChangedEvent);
#ifndef VBOX_WITH_GLUE
struct ICPUChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetCPU)(ICPUChangedEvent *pThis, PRUint32 *CPU);

    nsresult (*GetAdd)(ICPUChangedEvent *pThis, PRBool *add);

};
#else /* VBOX_WITH_GLUE */
struct ICPUChangedEventVtbl
{
    nsresult (*QueryInterface)(ICPUChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ICPUChangedEvent *pThis);
    nsrefcnt (*Release)(ICPUChangedEvent *pThis);
    nsresult (*GetType)(ICPUChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ICPUChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ICPUChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ICPUChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        ICPUChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetCPU)(ICPUChangedEvent *pThis, PRUint32 *CPU);

    nsresult (*GetAdd)(ICPUChangedEvent *pThis, PRBool *add);

};
#define ICPUChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ICPUChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ICPUChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ICPUChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ICPUChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ICPUChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ICPUChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ICPUChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ICPUChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ICPUChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ICPUChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ICPUChangedEvent_get_CPU(p, aCPU) ((p)->lpVtbl->GetCPU(p, aCPU))
#define ICPUChangedEvent_GetCPU(p, aCPU) ((p)->lpVtbl->GetCPU(p, aCPU))
#define ICPUChangedEvent_get_Add(p, aAdd) ((p)->lpVtbl->GetAdd(p, aAdd))
#define ICPUChangedEvent_GetAdd(p, aAdd) ((p)->lpVtbl->GetAdd(p, aAdd))
#endif /* VBOX_WITH_GLUE */

interface ICPUChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ICPUChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ICPUChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ICPUChangedEvent declaration */


/* Start of struct ICPUExecutionCapChangedEvent declaration */
#define ICPUEXECUTIONCAPCHANGEDEVENT_IID_STR "dfa7e4f5-b4a4-44ce-85a8-127ac5eb59dc"
#define ICPUEXECUTIONCAPCHANGEDEVENT_IID { \
    0xdfa7e4f5, 0xb4a4, 0x44ce, \
    { 0x85, 0xa8, 0x12, 0x7a, 0xc5, 0xeb, 0x59, 0xdc } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ICPUExecutionCapChangedEvent);
#ifndef VBOX_WITH_GLUE
struct ICPUExecutionCapChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetExecutionCap)(ICPUExecutionCapChangedEvent *pThis, PRUint32 *executionCap);

};
#else /* VBOX_WITH_GLUE */
struct ICPUExecutionCapChangedEventVtbl
{
    nsresult (*QueryInterface)(ICPUExecutionCapChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ICPUExecutionCapChangedEvent *pThis);
    nsrefcnt (*Release)(ICPUExecutionCapChangedEvent *pThis);
    nsresult (*GetType)(ICPUExecutionCapChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ICPUExecutionCapChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ICPUExecutionCapChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ICPUExecutionCapChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        ICPUExecutionCapChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetExecutionCap)(ICPUExecutionCapChangedEvent *pThis, PRUint32 *executionCap);

};
#define ICPUExecutionCapChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ICPUExecutionCapChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ICPUExecutionCapChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ICPUExecutionCapChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ICPUExecutionCapChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ICPUExecutionCapChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ICPUExecutionCapChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ICPUExecutionCapChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ICPUExecutionCapChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ICPUExecutionCapChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ICPUExecutionCapChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ICPUExecutionCapChangedEvent_get_ExecutionCap(p, aExecutionCap) ((p)->lpVtbl->GetExecutionCap(p, aExecutionCap))
#define ICPUExecutionCapChangedEvent_GetExecutionCap(p, aExecutionCap) ((p)->lpVtbl->GetExecutionCap(p, aExecutionCap))
#endif /* VBOX_WITH_GLUE */

interface ICPUExecutionCapChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ICPUExecutionCapChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ICPUExecutionCapChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ICPUExecutionCapChangedEvent declaration */


/* Start of struct IGuestKeyboardEvent declaration */
#define IGUESTKEYBOARDEVENT_IID_STR "88394258-7006-40d4-b339-472ee3801844"
#define IGUESTKEYBOARDEVENT_IID { \
    0x88394258, 0x7006, 0x40d4, \
    { 0xb3, 0x39, 0x47, 0x2e, 0xe3, 0x80, 0x18, 0x44 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestKeyboardEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestKeyboardEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetScancodes)(IGuestKeyboardEvent *pThis, PRUint32 *scancodesSize, PRInt32 **scancodes);

};
#else /* VBOX_WITH_GLUE */
struct IGuestKeyboardEventVtbl
{
    nsresult (*QueryInterface)(IGuestKeyboardEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestKeyboardEvent *pThis);
    nsrefcnt (*Release)(IGuestKeyboardEvent *pThis);
    nsresult (*GetType)(IGuestKeyboardEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestKeyboardEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestKeyboardEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestKeyboardEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestKeyboardEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetScancodes)(IGuestKeyboardEvent *pThis, PRUint32 *scancodesSize, PRInt32 **scancodes);

};
#define IGuestKeyboardEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestKeyboardEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestKeyboardEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestKeyboardEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestKeyboardEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestKeyboardEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestKeyboardEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestKeyboardEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestKeyboardEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestKeyboardEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestKeyboardEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestKeyboardEvent_get_Scancodes(p, aScancodes) ((p)->lpVtbl->GetScancodes(p, aScancodes))
#define IGuestKeyboardEvent_GetScancodes(p, aScancodes) ((p)->lpVtbl->GetScancodes(p, aScancodes))
#endif /* VBOX_WITH_GLUE */

interface IGuestKeyboardEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestKeyboardEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestKeyboardEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestKeyboardEvent declaration */


/* Start of struct IGuestMouseEvent declaration */
#define IGUESTMOUSEEVENT_IID_STR "179f8647-319c-4e7e-8150-c5837bd265f6"
#define IGUESTMOUSEEVENT_IID { \
    0x179f8647, 0x319c, 0x4e7e, \
    { 0x81, 0x50, 0xc5, 0x83, 0x7b, 0xd2, 0x65, 0xf6 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestMouseEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestMouseEvent_vtbl
{
    struct IReusableEvent_vtbl ireusableevent;

    nsresult (*GetMode)(IGuestMouseEvent *pThis, PRUint32 *mode);

    nsresult (*GetX)(IGuestMouseEvent *pThis, PRInt32 *x);

    nsresult (*GetY)(IGuestMouseEvent *pThis, PRInt32 *y);

    nsresult (*GetZ)(IGuestMouseEvent *pThis, PRInt32 *z);

    nsresult (*GetW)(IGuestMouseEvent *pThis, PRInt32 *w);

    nsresult (*GetButtons)(IGuestMouseEvent *pThis, PRInt32 *buttons);

};
#else /* VBOX_WITH_GLUE */
struct IGuestMouseEventVtbl
{
    nsresult (*QueryInterface)(IGuestMouseEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestMouseEvent *pThis);
    nsrefcnt (*Release)(IGuestMouseEvent *pThis);
    nsresult (*GetType)(IGuestMouseEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestMouseEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestMouseEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestMouseEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestMouseEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetGeneration)(IGuestMouseEvent *pThis, PRUint32 *generation);

    nsresult (*Reuse)(IGuestMouseEvent *pThis );

    nsresult (*GetMode)(IGuestMouseEvent *pThis, PRUint32 *mode);

    nsresult (*GetX)(IGuestMouseEvent *pThis, PRInt32 *x);

    nsresult (*GetY)(IGuestMouseEvent *pThis, PRInt32 *y);

    nsresult (*GetZ)(IGuestMouseEvent *pThis, PRInt32 *z);

    nsresult (*GetW)(IGuestMouseEvent *pThis, PRInt32 *w);

    nsresult (*GetButtons)(IGuestMouseEvent *pThis, PRInt32 *buttons);

};
#define IGuestMouseEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestMouseEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestMouseEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestMouseEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestMouseEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestMouseEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestMouseEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestMouseEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestMouseEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestMouseEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestMouseEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestMouseEvent_get_Generation(p, aGeneration) ((p)->lpVtbl->GetGeneration(p, aGeneration))
#define IGuestMouseEvent_GetGeneration(p, aGeneration) ((p)->lpVtbl->GetGeneration(p, aGeneration))
#define IGuestMouseEvent_Reuse(p) ((p)->lpVtbl->Reuse(p))
#define IGuestMouseEvent_get_Mode(p, aMode) ((p)->lpVtbl->GetMode(p, aMode))
#define IGuestMouseEvent_GetMode(p, aMode) ((p)->lpVtbl->GetMode(p, aMode))
#define IGuestMouseEvent_get_X(p, aX) ((p)->lpVtbl->GetX(p, aX))
#define IGuestMouseEvent_GetX(p, aX) ((p)->lpVtbl->GetX(p, aX))
#define IGuestMouseEvent_get_Y(p, aY) ((p)->lpVtbl->GetY(p, aY))
#define IGuestMouseEvent_GetY(p, aY) ((p)->lpVtbl->GetY(p, aY))
#define IGuestMouseEvent_get_Z(p, aZ) ((p)->lpVtbl->GetZ(p, aZ))
#define IGuestMouseEvent_GetZ(p, aZ) ((p)->lpVtbl->GetZ(p, aZ))
#define IGuestMouseEvent_get_W(p, aW) ((p)->lpVtbl->GetW(p, aW))
#define IGuestMouseEvent_GetW(p, aW) ((p)->lpVtbl->GetW(p, aW))
#define IGuestMouseEvent_get_Buttons(p, aButtons) ((p)->lpVtbl->GetButtons(p, aButtons))
#define IGuestMouseEvent_GetButtons(p, aButtons) ((p)->lpVtbl->GetButtons(p, aButtons))
#endif /* VBOX_WITH_GLUE */

interface IGuestMouseEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestMouseEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestMouseEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestMouseEvent declaration */


/* Start of struct IGuestMultiTouchEvent declaration */
#define IGUESTMULTITOUCHEVENT_IID_STR "be8a0eb5-f4f4-4dd0-9d30-c89b873247ec"
#define IGUESTMULTITOUCHEVENT_IID { \
    0xbe8a0eb5, 0xf4f4, 0x4dd0, \
    { 0x9d, 0x30, 0xc8, 0x9b, 0x87, 0x32, 0x47, 0xec } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestMultiTouchEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestMultiTouchEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetContactCount)(IGuestMultiTouchEvent *pThis, PRInt32 *contactCount);

    nsresult (*GetXPositions)(IGuestMultiTouchEvent *pThis, PRUint32 *xPositionsSize, PRInt16 **xPositions);

    nsresult (*GetYPositions)(IGuestMultiTouchEvent *pThis, PRUint32 *yPositionsSize, PRInt16 **yPositions);

    nsresult (*GetContactIds)(IGuestMultiTouchEvent *pThis, PRUint32 *contactIdsSize, PRUint16 **contactIds);

    nsresult (*GetContactFlags)(IGuestMultiTouchEvent *pThis, PRUint32 *contactFlagsSize, PRUint16 **contactFlags);

    nsresult (*GetScanTime)(IGuestMultiTouchEvent *pThis, PRUint32 *scanTime);

};
#else /* VBOX_WITH_GLUE */
struct IGuestMultiTouchEventVtbl
{
    nsresult (*QueryInterface)(IGuestMultiTouchEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestMultiTouchEvent *pThis);
    nsrefcnt (*Release)(IGuestMultiTouchEvent *pThis);
    nsresult (*GetType)(IGuestMultiTouchEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestMultiTouchEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestMultiTouchEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestMultiTouchEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestMultiTouchEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetContactCount)(IGuestMultiTouchEvent *pThis, PRInt32 *contactCount);

    nsresult (*GetXPositions)(IGuestMultiTouchEvent *pThis, PRUint32 *xPositionsSize, PRInt16 **xPositions);

    nsresult (*GetYPositions)(IGuestMultiTouchEvent *pThis, PRUint32 *yPositionsSize, PRInt16 **yPositions);

    nsresult (*GetContactIds)(IGuestMultiTouchEvent *pThis, PRUint32 *contactIdsSize, PRUint16 **contactIds);

    nsresult (*GetContactFlags)(IGuestMultiTouchEvent *pThis, PRUint32 *contactFlagsSize, PRUint16 **contactFlags);

    nsresult (*GetScanTime)(IGuestMultiTouchEvent *pThis, PRUint32 *scanTime);

};
#define IGuestMultiTouchEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestMultiTouchEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestMultiTouchEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestMultiTouchEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestMultiTouchEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestMultiTouchEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestMultiTouchEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestMultiTouchEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestMultiTouchEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestMultiTouchEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestMultiTouchEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestMultiTouchEvent_get_ContactCount(p, aContactCount) ((p)->lpVtbl->GetContactCount(p, aContactCount))
#define IGuestMultiTouchEvent_GetContactCount(p, aContactCount) ((p)->lpVtbl->GetContactCount(p, aContactCount))
#define IGuestMultiTouchEvent_get_XPositions(p, aXPositions) ((p)->lpVtbl->GetXPositions(p, aXPositions))
#define IGuestMultiTouchEvent_GetXPositions(p, aXPositions) ((p)->lpVtbl->GetXPositions(p, aXPositions))
#define IGuestMultiTouchEvent_get_YPositions(p, aYPositions) ((p)->lpVtbl->GetYPositions(p, aYPositions))
#define IGuestMultiTouchEvent_GetYPositions(p, aYPositions) ((p)->lpVtbl->GetYPositions(p, aYPositions))
#define IGuestMultiTouchEvent_get_ContactIds(p, aContactIds) ((p)->lpVtbl->GetContactIds(p, aContactIds))
#define IGuestMultiTouchEvent_GetContactIds(p, aContactIds) ((p)->lpVtbl->GetContactIds(p, aContactIds))
#define IGuestMultiTouchEvent_get_ContactFlags(p, aContactFlags) ((p)->lpVtbl->GetContactFlags(p, aContactFlags))
#define IGuestMultiTouchEvent_GetContactFlags(p, aContactFlags) ((p)->lpVtbl->GetContactFlags(p, aContactFlags))
#define IGuestMultiTouchEvent_get_ScanTime(p, aScanTime) ((p)->lpVtbl->GetScanTime(p, aScanTime))
#define IGuestMultiTouchEvent_GetScanTime(p, aScanTime) ((p)->lpVtbl->GetScanTime(p, aScanTime))
#endif /* VBOX_WITH_GLUE */

interface IGuestMultiTouchEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestMultiTouchEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestMultiTouchEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestMultiTouchEvent declaration */


/* Start of struct IGuestSessionEvent declaration */
#define IGUESTSESSIONEVENT_IID_STR "b9acd33f-647d-45ac-8fe9-f49b3183ba37"
#define IGUESTSESSIONEVENT_IID { \
    0xb9acd33f, 0x647d, 0x45ac, \
    { 0x8f, 0xe9, 0xf4, 0x9b, 0x31, 0x83, 0xba, 0x37 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestSessionEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestSessionEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetSession)(IGuestSessionEvent *pThis, IGuestSession * *session);

};
#else /* VBOX_WITH_GLUE */
struct IGuestSessionEventVtbl
{
    nsresult (*QueryInterface)(IGuestSessionEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestSessionEvent *pThis);
    nsrefcnt (*Release)(IGuestSessionEvent *pThis);
    nsresult (*GetType)(IGuestSessionEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestSessionEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestSessionEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestSessionEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestSessionEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestSessionEvent *pThis, IGuestSession * *session);

};
#define IGuestSessionEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestSessionEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestSessionEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestSessionEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestSessionEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestSessionEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestSessionEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestSessionEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestSessionEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestSessionEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestSessionEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestSessionEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestSessionEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#endif /* VBOX_WITH_GLUE */

interface IGuestSessionEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestSessionEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestSessionEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestSessionEvent declaration */


/* Start of struct IGuestSessionStateChangedEvent declaration */
#define IGUESTSESSIONSTATECHANGEDEVENT_IID_STR "327e3c00-ee61-462f-aed3-0dff6cbf9904"
#define IGUESTSESSIONSTATECHANGEDEVENT_IID { \
    0x327e3c00, 0xee61, 0x462f, \
    { 0xae, 0xd3, 0x0d, 0xff, 0x6c, 0xbf, 0x99, 0x04 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestSessionStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestSessionStateChangedEvent_vtbl
{
    struct IGuestSessionEvent_vtbl iguestsessionevent;

    nsresult (*GetId)(IGuestSessionStateChangedEvent *pThis, PRUint32 *id);

    nsresult (*GetStatus)(IGuestSessionStateChangedEvent *pThis, PRUint32 *status);

    nsresult (*GetError)(IGuestSessionStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#else /* VBOX_WITH_GLUE */
struct IGuestSessionStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestSessionStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestSessionStateChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestSessionStateChangedEvent *pThis);
    nsresult (*GetType)(IGuestSessionStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestSessionStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestSessionStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestSessionStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestSessionStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestSessionStateChangedEvent *pThis, IGuestSession * *session);

    nsresult (*GetId)(IGuestSessionStateChangedEvent *pThis, PRUint32 *id);

    nsresult (*GetStatus)(IGuestSessionStateChangedEvent *pThis, PRUint32 *status);

    nsresult (*GetError)(IGuestSessionStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#define IGuestSessionStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestSessionStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestSessionStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestSessionStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestSessionStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestSessionStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestSessionStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestSessionStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestSessionStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestSessionStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestSessionStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestSessionStateChangedEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestSessionStateChangedEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestSessionStateChangedEvent_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestSessionStateChangedEvent_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IGuestSessionStateChangedEvent_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestSessionStateChangedEvent_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestSessionStateChangedEvent_get_Error(p, aError) ((p)->lpVtbl->GetError(p, aError))
#define IGuestSessionStateChangedEvent_GetError(p, aError) ((p)->lpVtbl->GetError(p, aError))
#endif /* VBOX_WITH_GLUE */

interface IGuestSessionStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestSessionStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestSessionStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestSessionStateChangedEvent declaration */


/* Start of struct IGuestSessionRegisteredEvent declaration */
#define IGUESTSESSIONREGISTEREDEVENT_IID_STR "b79de686-eabd-4fa6-960a-f1756c99ea1c"
#define IGUESTSESSIONREGISTEREDEVENT_IID { \
    0xb79de686, 0xeabd, 0x4fa6, \
    { 0x96, 0x0a, 0xf1, 0x75, 0x6c, 0x99, 0xea, 0x1c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestSessionRegisteredEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestSessionRegisteredEvent_vtbl
{
    struct IGuestSessionEvent_vtbl iguestsessionevent;

    nsresult (*GetRegistered)(IGuestSessionRegisteredEvent *pThis, PRBool *registered);

};
#else /* VBOX_WITH_GLUE */
struct IGuestSessionRegisteredEventVtbl
{
    nsresult (*QueryInterface)(IGuestSessionRegisteredEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestSessionRegisteredEvent *pThis);
    nsrefcnt (*Release)(IGuestSessionRegisteredEvent *pThis);
    nsresult (*GetType)(IGuestSessionRegisteredEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestSessionRegisteredEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestSessionRegisteredEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestSessionRegisteredEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestSessionRegisteredEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestSessionRegisteredEvent *pThis, IGuestSession * *session);

    nsresult (*GetRegistered)(IGuestSessionRegisteredEvent *pThis, PRBool *registered);

};
#define IGuestSessionRegisteredEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestSessionRegisteredEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestSessionRegisteredEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestSessionRegisteredEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestSessionRegisteredEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestSessionRegisteredEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestSessionRegisteredEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestSessionRegisteredEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestSessionRegisteredEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestSessionRegisteredEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestSessionRegisteredEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestSessionRegisteredEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestSessionRegisteredEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestSessionRegisteredEvent_get_Registered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#define IGuestSessionRegisteredEvent_GetRegistered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#endif /* VBOX_WITH_GLUE */

interface IGuestSessionRegisteredEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestSessionRegisteredEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestSessionRegisteredEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestSessionRegisteredEvent declaration */


/* Start of struct IGuestProcessEvent declaration */
#define IGUESTPROCESSEVENT_IID_STR "2405f0e5-6588-40a3-9b0a-68c05ba52c4b"
#define IGUESTPROCESSEVENT_IID { \
    0x2405f0e5, 0x6588, 0x40a3, \
    { 0x9b, 0x0a, 0x68, 0xc0, 0x5b, 0xa5, 0x2c, 0x4b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcessEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestProcessEvent_vtbl
{
    struct IGuestSessionEvent_vtbl iguestsessionevent;

    nsresult (*GetProcess)(IGuestProcessEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessEvent *pThis, PRUint32 *pid);

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessEventVtbl
{
    nsresult (*QueryInterface)(IGuestProcessEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcessEvent *pThis);
    nsrefcnt (*Release)(IGuestProcessEvent *pThis);
    nsresult (*GetType)(IGuestProcessEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestProcessEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestProcessEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestProcessEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestProcessEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestProcessEvent *pThis, IGuestSession * *session);

    nsresult (*GetProcess)(IGuestProcessEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessEvent *pThis, PRUint32 *pid);

};
#define IGuestProcessEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcessEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcessEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcessEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestProcessEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestProcessEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessEvent_get_Process(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessEvent_GetProcess(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessEvent_get_Pid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessEvent_GetPid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcessEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcessEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcessEvent declaration */


/* Start of struct IGuestProcessRegisteredEvent declaration */
#define IGUESTPROCESSREGISTEREDEVENT_IID_STR "1d89e2b3-c6ea-45b6-9d43-dc6f70cc9f02"
#define IGUESTPROCESSREGISTEREDEVENT_IID { \
    0x1d89e2b3, 0xc6ea, 0x45b6, \
    { 0x9d, 0x43, 0xdc, 0x6f, 0x70, 0xcc, 0x9f, 0x02 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcessRegisteredEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestProcessRegisteredEvent_vtbl
{
    struct IGuestProcessEvent_vtbl iguestprocessevent;

    nsresult (*GetRegistered)(IGuestProcessRegisteredEvent *pThis, PRBool *registered);

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessRegisteredEventVtbl
{
    nsresult (*QueryInterface)(IGuestProcessRegisteredEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcessRegisteredEvent *pThis);
    nsrefcnt (*Release)(IGuestProcessRegisteredEvent *pThis);
    nsresult (*GetType)(IGuestProcessRegisteredEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestProcessRegisteredEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestProcessRegisteredEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestProcessRegisteredEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestProcessRegisteredEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestProcessRegisteredEvent *pThis, IGuestSession * *session);

    nsresult (*GetProcess)(IGuestProcessRegisteredEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessRegisteredEvent *pThis, PRUint32 *pid);

    nsresult (*GetRegistered)(IGuestProcessRegisteredEvent *pThis, PRBool *registered);

};
#define IGuestProcessRegisteredEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcessRegisteredEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcessRegisteredEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcessRegisteredEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessRegisteredEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessRegisteredEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessRegisteredEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessRegisteredEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessRegisteredEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessRegisteredEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestProcessRegisteredEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestProcessRegisteredEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessRegisteredEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessRegisteredEvent_get_Process(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessRegisteredEvent_GetProcess(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessRegisteredEvent_get_Pid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessRegisteredEvent_GetPid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessRegisteredEvent_get_Registered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#define IGuestProcessRegisteredEvent_GetRegistered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcessRegisteredEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcessRegisteredEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessRegisteredEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcessRegisteredEvent declaration */


/* Start of struct IGuestProcessStateChangedEvent declaration */
#define IGUESTPROCESSSTATECHANGEDEVENT_IID_STR "c365fb7b-4430-499f-92c8-8bed814a567a"
#define IGUESTPROCESSSTATECHANGEDEVENT_IID { \
    0xc365fb7b, 0x4430, 0x499f, \
    { 0x92, 0xc8, 0x8b, 0xed, 0x81, 0x4a, 0x56, 0x7a } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcessStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestProcessStateChangedEvent_vtbl
{
    struct IGuestProcessEvent_vtbl iguestprocessevent;

    nsresult (*GetStatus)(IGuestProcessStateChangedEvent *pThis, PRUint32 *status);

    nsresult (*GetError)(IGuestProcessStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestProcessStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcessStateChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestProcessStateChangedEvent *pThis);
    nsresult (*GetType)(IGuestProcessStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestProcessStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestProcessStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestProcessStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestProcessStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestProcessStateChangedEvent *pThis, IGuestSession * *session);

    nsresult (*GetProcess)(IGuestProcessStateChangedEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessStateChangedEvent *pThis, PRUint32 *pid);

    nsresult (*GetStatus)(IGuestProcessStateChangedEvent *pThis, PRUint32 *status);

    nsresult (*GetError)(IGuestProcessStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#define IGuestProcessStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcessStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcessStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcessStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestProcessStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestProcessStateChangedEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessStateChangedEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessStateChangedEvent_get_Process(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessStateChangedEvent_GetProcess(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessStateChangedEvent_get_Pid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessStateChangedEvent_GetPid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessStateChangedEvent_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestProcessStateChangedEvent_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestProcessStateChangedEvent_get_Error(p, aError) ((p)->lpVtbl->GetError(p, aError))
#define IGuestProcessStateChangedEvent_GetError(p, aError) ((p)->lpVtbl->GetError(p, aError))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcessStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcessStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcessStateChangedEvent declaration */


/* Start of struct IGuestProcessIOEvent declaration */
#define IGUESTPROCESSIOEVENT_IID_STR "9ea9227c-e9bb-49b3-bfc7-c5171e93ef38"
#define IGUESTPROCESSIOEVENT_IID { \
    0x9ea9227c, 0xe9bb, 0x49b3, \
    { 0xbf, 0xc7, 0xc5, 0x17, 0x1e, 0x93, 0xef, 0x38 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcessIOEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestProcessIOEvent_vtbl
{
    struct IGuestProcessEvent_vtbl iguestprocessevent;

    nsresult (*GetHandle)(IGuestProcessIOEvent *pThis, PRUint32 *handle);

    nsresult (*GetProcessed)(IGuestProcessIOEvent *pThis, PRUint32 *processed);

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessIOEventVtbl
{
    nsresult (*QueryInterface)(IGuestProcessIOEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcessIOEvent *pThis);
    nsrefcnt (*Release)(IGuestProcessIOEvent *pThis);
    nsresult (*GetType)(IGuestProcessIOEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestProcessIOEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestProcessIOEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestProcessIOEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestProcessIOEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestProcessIOEvent *pThis, IGuestSession * *session);

    nsresult (*GetProcess)(IGuestProcessIOEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessIOEvent *pThis, PRUint32 *pid);

    nsresult (*GetHandle)(IGuestProcessIOEvent *pThis, PRUint32 *handle);

    nsresult (*GetProcessed)(IGuestProcessIOEvent *pThis, PRUint32 *processed);

};
#define IGuestProcessIOEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcessIOEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcessIOEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcessIOEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessIOEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessIOEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessIOEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessIOEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessIOEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessIOEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestProcessIOEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestProcessIOEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessIOEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessIOEvent_get_Process(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessIOEvent_GetProcess(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessIOEvent_get_Pid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessIOEvent_GetPid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessIOEvent_get_Handle(p, aHandle) ((p)->lpVtbl->GetHandle(p, aHandle))
#define IGuestProcessIOEvent_GetHandle(p, aHandle) ((p)->lpVtbl->GetHandle(p, aHandle))
#define IGuestProcessIOEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestProcessIOEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcessIOEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcessIOEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessIOEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcessIOEvent declaration */


/* Start of struct IGuestProcessInputNotifyEvent declaration */
#define IGUESTPROCESSINPUTNOTIFYEVENT_IID_STR "0de887f2-b7db-4616-aac6-cfb94d89ba78"
#define IGUESTPROCESSINPUTNOTIFYEVENT_IID { \
    0x0de887f2, 0xb7db, 0x4616, \
    { 0xaa, 0xc6, 0xcf, 0xb9, 0x4d, 0x89, 0xba, 0x78 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcessInputNotifyEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestProcessInputNotifyEvent_vtbl
{
    struct IGuestProcessIOEvent_vtbl iguestprocessioevent;

    nsresult (*GetStatus)(IGuestProcessInputNotifyEvent *pThis, PRUint32 *status);

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessInputNotifyEventVtbl
{
    nsresult (*QueryInterface)(IGuestProcessInputNotifyEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcessInputNotifyEvent *pThis);
    nsrefcnt (*Release)(IGuestProcessInputNotifyEvent *pThis);
    nsresult (*GetType)(IGuestProcessInputNotifyEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestProcessInputNotifyEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestProcessInputNotifyEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestProcessInputNotifyEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestProcessInputNotifyEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestProcessInputNotifyEvent *pThis, IGuestSession * *session);

    nsresult (*GetProcess)(IGuestProcessInputNotifyEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessInputNotifyEvent *pThis, PRUint32 *pid);

    nsresult (*GetHandle)(IGuestProcessInputNotifyEvent *pThis, PRUint32 *handle);

    nsresult (*GetProcessed)(IGuestProcessInputNotifyEvent *pThis, PRUint32 *processed);

    nsresult (*GetStatus)(IGuestProcessInputNotifyEvent *pThis, PRUint32 *status);

};
#define IGuestProcessInputNotifyEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcessInputNotifyEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcessInputNotifyEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcessInputNotifyEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessInputNotifyEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessInputNotifyEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessInputNotifyEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessInputNotifyEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessInputNotifyEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessInputNotifyEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestProcessInputNotifyEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestProcessInputNotifyEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessInputNotifyEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessInputNotifyEvent_get_Process(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessInputNotifyEvent_GetProcess(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessInputNotifyEvent_get_Pid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessInputNotifyEvent_GetPid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessInputNotifyEvent_get_Handle(p, aHandle) ((p)->lpVtbl->GetHandle(p, aHandle))
#define IGuestProcessInputNotifyEvent_GetHandle(p, aHandle) ((p)->lpVtbl->GetHandle(p, aHandle))
#define IGuestProcessInputNotifyEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestProcessInputNotifyEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestProcessInputNotifyEvent_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestProcessInputNotifyEvent_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcessInputNotifyEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcessInputNotifyEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessInputNotifyEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcessInputNotifyEvent declaration */


/* Start of struct IGuestProcessOutputEvent declaration */
#define IGUESTPROCESSOUTPUTEVENT_IID_STR "d3d5f1ee-bcb2-4905-a7ab-cc85448a742b"
#define IGUESTPROCESSOUTPUTEVENT_IID { \
    0xd3d5f1ee, 0xbcb2, 0x4905, \
    { 0xa7, 0xab, 0xcc, 0x85, 0x44, 0x8a, 0x74, 0x2b } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestProcessOutputEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestProcessOutputEvent_vtbl
{
    struct IGuestProcessIOEvent_vtbl iguestprocessioevent;

    nsresult (*GetData)(IGuestProcessOutputEvent *pThis, PRUint32 *dataSize, PRUint8 **data);

};
#else /* VBOX_WITH_GLUE */
struct IGuestProcessOutputEventVtbl
{
    nsresult (*QueryInterface)(IGuestProcessOutputEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestProcessOutputEvent *pThis);
    nsrefcnt (*Release)(IGuestProcessOutputEvent *pThis);
    nsresult (*GetType)(IGuestProcessOutputEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestProcessOutputEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestProcessOutputEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestProcessOutputEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestProcessOutputEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestProcessOutputEvent *pThis, IGuestSession * *session);

    nsresult (*GetProcess)(IGuestProcessOutputEvent *pThis, IGuestProcess * *process);

    nsresult (*GetPid)(IGuestProcessOutputEvent *pThis, PRUint32 *pid);

    nsresult (*GetHandle)(IGuestProcessOutputEvent *pThis, PRUint32 *handle);

    nsresult (*GetProcessed)(IGuestProcessOutputEvent *pThis, PRUint32 *processed);

    nsresult (*GetData)(IGuestProcessOutputEvent *pThis, PRUint32 *dataSize, PRUint8 **data);

};
#define IGuestProcessOutputEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestProcessOutputEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestProcessOutputEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestProcessOutputEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessOutputEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestProcessOutputEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessOutputEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestProcessOutputEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessOutputEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestProcessOutputEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestProcessOutputEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestProcessOutputEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessOutputEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestProcessOutputEvent_get_Process(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessOutputEvent_GetProcess(p, aProcess) ((p)->lpVtbl->GetProcess(p, aProcess))
#define IGuestProcessOutputEvent_get_Pid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessOutputEvent_GetPid(p, aPid) ((p)->lpVtbl->GetPid(p, aPid))
#define IGuestProcessOutputEvent_get_Handle(p, aHandle) ((p)->lpVtbl->GetHandle(p, aHandle))
#define IGuestProcessOutputEvent_GetHandle(p, aHandle) ((p)->lpVtbl->GetHandle(p, aHandle))
#define IGuestProcessOutputEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestProcessOutputEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestProcessOutputEvent_get_Data(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define IGuestProcessOutputEvent_GetData(p, aData) ((p)->lpVtbl->GetData(p, aData))
#endif /* VBOX_WITH_GLUE */

interface IGuestProcessOutputEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestProcessOutputEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestProcessOutputEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestProcessOutputEvent declaration */


/* Start of struct IGuestFileEvent declaration */
#define IGUESTFILEEVENT_IID_STR "c8adb7b0-057d-4391-b928-f14b06b710c5"
#define IGUESTFILEEVENT_IID { \
    0xc8adb7b0, 0x057d, 0x4391, \
    { 0xb9, 0x28, 0xf1, 0x4b, 0x06, 0xb7, 0x10, 0xc5 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileEvent_vtbl
{
    struct IGuestSessionEvent_vtbl iguestsessionevent;

    nsresult (*GetFile)(IGuestFileEvent *pThis, IGuestFile * *file);

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileEvent *pThis);
    nsrefcnt (*Release)(IGuestFileEvent *pThis);
    nsresult (*GetType)(IGuestFileEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileEvent *pThis, IGuestFile * *file);

};
#define IGuestFileEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileEvent declaration */


/* Start of struct IGuestFileRegisteredEvent declaration */
#define IGUESTFILEREGISTEREDEVENT_IID_STR "d0d93830-70a2-487e-895e-d3fc9679f7b3"
#define IGUESTFILEREGISTEREDEVENT_IID { \
    0xd0d93830, 0x70a2, 0x487e, \
    { 0x89, 0x5e, 0xd3, 0xfc, 0x96, 0x79, 0xf7, 0xb3 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileRegisteredEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileRegisteredEvent_vtbl
{
    struct IGuestFileEvent_vtbl iguestfileevent;

    nsresult (*GetRegistered)(IGuestFileRegisteredEvent *pThis, PRBool *registered);

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileRegisteredEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileRegisteredEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileRegisteredEvent *pThis);
    nsrefcnt (*Release)(IGuestFileRegisteredEvent *pThis);
    nsresult (*GetType)(IGuestFileRegisteredEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileRegisteredEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileRegisteredEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileRegisteredEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileRegisteredEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileRegisteredEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileRegisteredEvent *pThis, IGuestFile * *file);

    nsresult (*GetRegistered)(IGuestFileRegisteredEvent *pThis, PRBool *registered);

};
#define IGuestFileRegisteredEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileRegisteredEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileRegisteredEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileRegisteredEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileRegisteredEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileRegisteredEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileRegisteredEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileRegisteredEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileRegisteredEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileRegisteredEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileRegisteredEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileRegisteredEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileRegisteredEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileRegisteredEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileRegisteredEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileRegisteredEvent_get_Registered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#define IGuestFileRegisteredEvent_GetRegistered(p, aRegistered) ((p)->lpVtbl->GetRegistered(p, aRegistered))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileRegisteredEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileRegisteredEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileRegisteredEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileRegisteredEvent declaration */


/* Start of struct IGuestFileStateChangedEvent declaration */
#define IGUESTFILESTATECHANGEDEVENT_IID_STR "d37fe88f-0979-486c-baa1-3abb144dc82d"
#define IGUESTFILESTATECHANGEDEVENT_IID { \
    0xd37fe88f, 0x0979, 0x486c, \
    { 0xba, 0xa1, 0x3a, 0xbb, 0x14, 0x4d, 0xc8, 0x2d } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileStateChangedEvent_vtbl
{
    struct IGuestFileEvent_vtbl iguestfileevent;

    nsresult (*GetStatus)(IGuestFileStateChangedEvent *pThis, PRUint32 *status);

    nsresult (*GetError)(IGuestFileStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileStateChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestFileStateChangedEvent *pThis);
    nsresult (*GetType)(IGuestFileStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileStateChangedEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileStateChangedEvent *pThis, IGuestFile * *file);

    nsresult (*GetStatus)(IGuestFileStateChangedEvent *pThis, PRUint32 *status);

    nsresult (*GetError)(IGuestFileStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#define IGuestFileStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileStateChangedEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileStateChangedEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileStateChangedEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileStateChangedEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileStateChangedEvent_get_Status(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestFileStateChangedEvent_GetStatus(p, aStatus) ((p)->lpVtbl->GetStatus(p, aStatus))
#define IGuestFileStateChangedEvent_get_Error(p, aError) ((p)->lpVtbl->GetError(p, aError))
#define IGuestFileStateChangedEvent_GetError(p, aError) ((p)->lpVtbl->GetError(p, aError))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileStateChangedEvent declaration */


/* Start of struct IGuestFileIOEvent declaration */
#define IGUESTFILEIOEVENT_IID_STR "b5191a7c-9536-4ef8-820e-3b0e17e5bbc8"
#define IGUESTFILEIOEVENT_IID { \
    0xb5191a7c, 0x9536, 0x4ef8, \
    { 0x82, 0x0e, 0x3b, 0x0e, 0x17, 0xe5, 0xbb, 0xc8 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileIOEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileIOEvent_vtbl
{
    struct IGuestFileEvent_vtbl iguestfileevent;

    nsresult (*GetOffset)(IGuestFileIOEvent *pThis, PRInt64 *offset);

    nsresult (*GetProcessed)(IGuestFileIOEvent *pThis, PRUint32 *processed);

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileIOEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileIOEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileIOEvent *pThis);
    nsrefcnt (*Release)(IGuestFileIOEvent *pThis);
    nsresult (*GetType)(IGuestFileIOEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileIOEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileIOEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileIOEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileIOEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileIOEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileIOEvent *pThis, IGuestFile * *file);

    nsresult (*GetOffset)(IGuestFileIOEvent *pThis, PRInt64 *offset);

    nsresult (*GetProcessed)(IGuestFileIOEvent *pThis, PRUint32 *processed);

};
#define IGuestFileIOEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileIOEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileIOEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileIOEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileIOEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileIOEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileIOEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileIOEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileIOEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileIOEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileIOEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileIOEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileIOEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileIOEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileIOEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileIOEvent_get_Offset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileIOEvent_GetOffset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileIOEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestFileIOEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileIOEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileIOEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileIOEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileIOEvent declaration */


/* Start of struct IGuestFileOffsetChangedEvent declaration */
#define IGUESTFILEOFFSETCHANGEDEVENT_IID_STR "e8f79a21-1207-4179-94cf-ca250036308f"
#define IGUESTFILEOFFSETCHANGEDEVENT_IID { \
    0xe8f79a21, 0x1207, 0x4179, \
    { 0x94, 0xcf, 0xca, 0x25, 0x00, 0x36, 0x30, 0x8f } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileOffsetChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileOffsetChangedEvent_vtbl
{
    struct IGuestFileIOEvent_vtbl iguestfileioevent;

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileOffsetChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileOffsetChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileOffsetChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestFileOffsetChangedEvent *pThis);
    nsresult (*GetType)(IGuestFileOffsetChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileOffsetChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileOffsetChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileOffsetChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileOffsetChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileOffsetChangedEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileOffsetChangedEvent *pThis, IGuestFile * *file);

    nsresult (*GetOffset)(IGuestFileOffsetChangedEvent *pThis, PRInt64 *offset);

    nsresult (*GetProcessed)(IGuestFileOffsetChangedEvent *pThis, PRUint32 *processed);

};
#define IGuestFileOffsetChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileOffsetChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileOffsetChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileOffsetChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileOffsetChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileOffsetChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileOffsetChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileOffsetChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileOffsetChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileOffsetChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileOffsetChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileOffsetChangedEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileOffsetChangedEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileOffsetChangedEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileOffsetChangedEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileOffsetChangedEvent_get_Offset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileOffsetChangedEvent_GetOffset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileOffsetChangedEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestFileOffsetChangedEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileOffsetChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileOffsetChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileOffsetChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileOffsetChangedEvent declaration */


/* Start of struct IGuestFileReadEvent declaration */
#define IGUESTFILEREADEVENT_IID_STR "4ee3cbcb-486f-40db-9150-deee3fd24189"
#define IGUESTFILEREADEVENT_IID { \
    0x4ee3cbcb, 0x486f, 0x40db, \
    { 0x91, 0x50, 0xde, 0xee, 0x3f, 0xd2, 0x41, 0x89 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileReadEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileReadEvent_vtbl
{
    struct IGuestFileIOEvent_vtbl iguestfileioevent;

    nsresult (*GetData)(IGuestFileReadEvent *pThis, PRUint32 *dataSize, PRUint8 **data);

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileReadEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileReadEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileReadEvent *pThis);
    nsrefcnt (*Release)(IGuestFileReadEvent *pThis);
    nsresult (*GetType)(IGuestFileReadEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileReadEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileReadEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileReadEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileReadEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileReadEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileReadEvent *pThis, IGuestFile * *file);

    nsresult (*GetOffset)(IGuestFileReadEvent *pThis, PRInt64 *offset);

    nsresult (*GetProcessed)(IGuestFileReadEvent *pThis, PRUint32 *processed);

    nsresult (*GetData)(IGuestFileReadEvent *pThis, PRUint32 *dataSize, PRUint8 **data);

};
#define IGuestFileReadEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileReadEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileReadEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileReadEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileReadEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileReadEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileReadEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileReadEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileReadEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileReadEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileReadEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileReadEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileReadEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileReadEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileReadEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileReadEvent_get_Offset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileReadEvent_GetOffset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileReadEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestFileReadEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestFileReadEvent_get_Data(p, aData) ((p)->lpVtbl->GetData(p, aData))
#define IGuestFileReadEvent_GetData(p, aData) ((p)->lpVtbl->GetData(p, aData))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileReadEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileReadEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileReadEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileReadEvent declaration */


/* Start of struct IGuestFileWriteEvent declaration */
#define IGUESTFILEWRITEEVENT_IID_STR "e062a915-3cf5-4c0a-bc90-9b8d4cc94d89"
#define IGUESTFILEWRITEEVENT_IID { \
    0xe062a915, 0x3cf5, 0x4c0a, \
    { 0xbc, 0x90, 0x9b, 0x8d, 0x4c, 0xc9, 0x4d, 0x89 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestFileWriteEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestFileWriteEvent_vtbl
{
    struct IGuestFileIOEvent_vtbl iguestfileioevent;

};
#else /* VBOX_WITH_GLUE */
struct IGuestFileWriteEventVtbl
{
    nsresult (*QueryInterface)(IGuestFileWriteEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestFileWriteEvent *pThis);
    nsrefcnt (*Release)(IGuestFileWriteEvent *pThis);
    nsresult (*GetType)(IGuestFileWriteEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestFileWriteEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestFileWriteEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestFileWriteEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestFileWriteEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetSession)(IGuestFileWriteEvent *pThis, IGuestSession * *session);

    nsresult (*GetFile)(IGuestFileWriteEvent *pThis, IGuestFile * *file);

    nsresult (*GetOffset)(IGuestFileWriteEvent *pThis, PRInt64 *offset);

    nsresult (*GetProcessed)(IGuestFileWriteEvent *pThis, PRUint32 *processed);

};
#define IGuestFileWriteEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestFileWriteEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestFileWriteEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestFileWriteEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileWriteEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestFileWriteEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileWriteEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestFileWriteEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileWriteEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestFileWriteEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestFileWriteEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestFileWriteEvent_get_Session(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileWriteEvent_GetSession(p, aSession) ((p)->lpVtbl->GetSession(p, aSession))
#define IGuestFileWriteEvent_get_File(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileWriteEvent_GetFile(p, aFile) ((p)->lpVtbl->GetFile(p, aFile))
#define IGuestFileWriteEvent_get_Offset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileWriteEvent_GetOffset(p, aOffset) ((p)->lpVtbl->GetOffset(p, aOffset))
#define IGuestFileWriteEvent_get_Processed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#define IGuestFileWriteEvent_GetProcessed(p, aProcessed) ((p)->lpVtbl->GetProcessed(p, aProcessed))
#endif /* VBOX_WITH_GLUE */

interface IGuestFileWriteEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestFileWriteEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestFileWriteEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestFileWriteEvent declaration */


/* Start of struct IVRDEServerChangedEvent declaration */
#define IVRDESERVERCHANGEDEVENT_IID_STR "a06fd66a-3188-4c8c-8756-1395e8cb691c"
#define IVRDESERVERCHANGEDEVENT_IID { \
    0xa06fd66a, 0x3188, 0x4c8c, \
    { 0x87, 0x56, 0x13, 0x95, 0xe8, 0xcb, 0x69, 0x1c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVRDEServerChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IVRDEServerChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IVRDEServerChangedEventVtbl
{
    nsresult (*QueryInterface)(IVRDEServerChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVRDEServerChangedEvent *pThis);
    nsrefcnt (*Release)(IVRDEServerChangedEvent *pThis);
    nsresult (*GetType)(IVRDEServerChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IVRDEServerChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IVRDEServerChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IVRDEServerChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IVRDEServerChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IVRDEServerChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVRDEServerChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVRDEServerChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IVRDEServerChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVRDEServerChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVRDEServerChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVRDEServerChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVRDEServerChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVRDEServerChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVRDEServerChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IVRDEServerChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IVRDEServerChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IVRDEServerChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVRDEServerChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVRDEServerChangedEvent declaration */


/* Start of struct IVRDEServerInfoChangedEvent declaration */
#define IVRDESERVERINFOCHANGEDEVENT_IID_STR "dd6a1080-e1b7-4339-a549-f0878115596e"
#define IVRDESERVERINFOCHANGEDEVENT_IID { \
    0xdd6a1080, 0xe1b7, 0x4339, \
    { 0xa5, 0x49, 0xf0, 0x87, 0x81, 0x15, 0x59, 0x6e } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVRDEServerInfoChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IVRDEServerInfoChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IVRDEServerInfoChangedEventVtbl
{
    nsresult (*QueryInterface)(IVRDEServerInfoChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVRDEServerInfoChangedEvent *pThis);
    nsrefcnt (*Release)(IVRDEServerInfoChangedEvent *pThis);
    nsresult (*GetType)(IVRDEServerInfoChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IVRDEServerInfoChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IVRDEServerInfoChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IVRDEServerInfoChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IVRDEServerInfoChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IVRDEServerInfoChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVRDEServerInfoChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVRDEServerInfoChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IVRDEServerInfoChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVRDEServerInfoChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVRDEServerInfoChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVRDEServerInfoChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVRDEServerInfoChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVRDEServerInfoChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVRDEServerInfoChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IVRDEServerInfoChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IVRDEServerInfoChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IVRDEServerInfoChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVRDEServerInfoChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVRDEServerInfoChangedEvent declaration */


/* Start of struct IVideoCaptureChangedEvent declaration */
#define IVIDEOCAPTURECHANGEDEVENT_IID_STR "6215d169-25dd-4719-ab34-c908701efb58"
#define IVIDEOCAPTURECHANGEDEVENT_IID { \
    0x6215d169, 0x25dd, 0x4719, \
    { 0xab, 0x34, 0xc9, 0x08, 0x70, 0x1e, 0xfb, 0x58 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVideoCaptureChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IVideoCaptureChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IVideoCaptureChangedEventVtbl
{
    nsresult (*QueryInterface)(IVideoCaptureChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVideoCaptureChangedEvent *pThis);
    nsrefcnt (*Release)(IVideoCaptureChangedEvent *pThis);
    nsresult (*GetType)(IVideoCaptureChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IVideoCaptureChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IVideoCaptureChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IVideoCaptureChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IVideoCaptureChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IVideoCaptureChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVideoCaptureChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVideoCaptureChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IVideoCaptureChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVideoCaptureChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVideoCaptureChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVideoCaptureChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVideoCaptureChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVideoCaptureChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVideoCaptureChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IVideoCaptureChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IVideoCaptureChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IVideoCaptureChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVideoCaptureChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVideoCaptureChangedEvent declaration */


/* Start of struct IUSBControllerChangedEvent declaration */
#define IUSBCONTROLLERCHANGEDEVENT_IID_STR "93BADC0C-61D9-4940-A084-E6BB29AF3D83"
#define IUSBCONTROLLERCHANGEDEVENT_IID { \
    0x93BADC0C, 0x61D9, 0x4940, \
    { 0xA0, 0x84, 0xE6, 0xBB, 0x29, 0xAF, 0x3D, 0x83 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IUSBControllerChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IUSBControllerChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IUSBControllerChangedEventVtbl
{
    nsresult (*QueryInterface)(IUSBControllerChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IUSBControllerChangedEvent *pThis);
    nsrefcnt (*Release)(IUSBControllerChangedEvent *pThis);
    nsresult (*GetType)(IUSBControllerChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IUSBControllerChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IUSBControllerChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IUSBControllerChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IUSBControllerChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IUSBControllerChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUSBControllerChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUSBControllerChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IUSBControllerChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IUSBControllerChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IUSBControllerChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IUSBControllerChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IUSBControllerChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IUSBControllerChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IUSBControllerChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IUSBControllerChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IUSBControllerChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IUSBControllerChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IUSBControllerChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IUSBControllerChangedEvent declaration */


/* Start of struct IUSBDeviceStateChangedEvent declaration */
#define IUSBDEVICESTATECHANGEDEVENT_IID_STR "806da61b-6679-422a-b629-51b06b0c6d93"
#define IUSBDEVICESTATECHANGEDEVENT_IID { \
    0x806da61b, 0x6679, 0x422a, \
    { 0xb6, 0x29, 0x51, 0xb0, 0x6b, 0x0c, 0x6d, 0x93 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IUSBDeviceStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IUSBDeviceStateChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetDevice)(IUSBDeviceStateChangedEvent *pThis, IUSBDevice * *device);

    nsresult (*GetAttached)(IUSBDeviceStateChangedEvent *pThis, PRBool *attached);

    nsresult (*GetError)(IUSBDeviceStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#else /* VBOX_WITH_GLUE */
struct IUSBDeviceStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IUSBDeviceStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IUSBDeviceStateChangedEvent *pThis);
    nsrefcnt (*Release)(IUSBDeviceStateChangedEvent *pThis);
    nsresult (*GetType)(IUSBDeviceStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IUSBDeviceStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IUSBDeviceStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IUSBDeviceStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IUSBDeviceStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetDevice)(IUSBDeviceStateChangedEvent *pThis, IUSBDevice * *device);

    nsresult (*GetAttached)(IUSBDeviceStateChangedEvent *pThis, PRBool *attached);

    nsresult (*GetError)(IUSBDeviceStateChangedEvent *pThis, IVirtualBoxErrorInfo * *error);

};
#define IUSBDeviceStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IUSBDeviceStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IUSBDeviceStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IUSBDeviceStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IUSBDeviceStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IUSBDeviceStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IUSBDeviceStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IUSBDeviceStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IUSBDeviceStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IUSBDeviceStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IUSBDeviceStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IUSBDeviceStateChangedEvent_get_Device(p, aDevice) ((p)->lpVtbl->GetDevice(p, aDevice))
#define IUSBDeviceStateChangedEvent_GetDevice(p, aDevice) ((p)->lpVtbl->GetDevice(p, aDevice))
#define IUSBDeviceStateChangedEvent_get_Attached(p, aAttached) ((p)->lpVtbl->GetAttached(p, aAttached))
#define IUSBDeviceStateChangedEvent_GetAttached(p, aAttached) ((p)->lpVtbl->GetAttached(p, aAttached))
#define IUSBDeviceStateChangedEvent_get_Error(p, aError) ((p)->lpVtbl->GetError(p, aError))
#define IUSBDeviceStateChangedEvent_GetError(p, aError) ((p)->lpVtbl->GetError(p, aError))
#endif /* VBOX_WITH_GLUE */

interface IUSBDeviceStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IUSBDeviceStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IUSBDeviceStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IUSBDeviceStateChangedEvent declaration */


/* Start of struct ISharedFolderChangedEvent declaration */
#define ISHAREDFOLDERCHANGEDEVENT_IID_STR "B66349B5-3534-4239-B2DE-8E1535D94C0B"
#define ISHAREDFOLDERCHANGEDEVENT_IID { \
    0xB66349B5, 0x3534, 0x4239, \
    { 0xB2, 0xDE, 0x8E, 0x15, 0x35, 0xD9, 0x4C, 0x0B } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ISharedFolderChangedEvent);
#ifndef VBOX_WITH_GLUE
struct ISharedFolderChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetScope)(ISharedFolderChangedEvent *pThis, PRUint32 *scope);

};
#else /* VBOX_WITH_GLUE */
struct ISharedFolderChangedEventVtbl
{
    nsresult (*QueryInterface)(ISharedFolderChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ISharedFolderChangedEvent *pThis);
    nsrefcnt (*Release)(ISharedFolderChangedEvent *pThis);
    nsresult (*GetType)(ISharedFolderChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ISharedFolderChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ISharedFolderChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ISharedFolderChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        ISharedFolderChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetScope)(ISharedFolderChangedEvent *pThis, PRUint32 *scope);

};
#define ISharedFolderChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ISharedFolderChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ISharedFolderChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ISharedFolderChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISharedFolderChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ISharedFolderChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISharedFolderChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ISharedFolderChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISharedFolderChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ISharedFolderChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ISharedFolderChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ISharedFolderChangedEvent_get_Scope(p, aScope) ((p)->lpVtbl->GetScope(p, aScope))
#define ISharedFolderChangedEvent_GetScope(p, aScope) ((p)->lpVtbl->GetScope(p, aScope))
#endif /* VBOX_WITH_GLUE */

interface ISharedFolderChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct ISharedFolderChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ISharedFolderChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ISharedFolderChangedEvent declaration */


/* Start of struct IRuntimeErrorEvent declaration */
#define IRUNTIMEERROREVENT_IID_STR "883DD18B-0721-4CDE-867C-1A82ABAF914C"
#define IRUNTIMEERROREVENT_IID { \
    0x883DD18B, 0x0721, 0x4CDE, \
    { 0x86, 0x7C, 0x1A, 0x82, 0xAB, 0xAF, 0x91, 0x4C } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IRuntimeErrorEvent);
#ifndef VBOX_WITH_GLUE
struct IRuntimeErrorEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetFatal)(IRuntimeErrorEvent *pThis, PRBool *fatal);

    nsresult (*GetId)(IRuntimeErrorEvent *pThis, PRUnichar * *id);

    nsresult (*GetMessage)(IRuntimeErrorEvent *pThis, PRUnichar * *message);

};
#else /* VBOX_WITH_GLUE */
struct IRuntimeErrorEventVtbl
{
    nsresult (*QueryInterface)(IRuntimeErrorEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IRuntimeErrorEvent *pThis);
    nsrefcnt (*Release)(IRuntimeErrorEvent *pThis);
    nsresult (*GetType)(IRuntimeErrorEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IRuntimeErrorEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IRuntimeErrorEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IRuntimeErrorEvent *pThis );

    nsresult (*WaitProcessed)(
        IRuntimeErrorEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetFatal)(IRuntimeErrorEvent *pThis, PRBool *fatal);

    nsresult (*GetId)(IRuntimeErrorEvent *pThis, PRUnichar * *id);

    nsresult (*GetMessage)(IRuntimeErrorEvent *pThis, PRUnichar * *message);

};
#define IRuntimeErrorEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IRuntimeErrorEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IRuntimeErrorEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IRuntimeErrorEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IRuntimeErrorEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IRuntimeErrorEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IRuntimeErrorEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IRuntimeErrorEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IRuntimeErrorEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IRuntimeErrorEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IRuntimeErrorEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IRuntimeErrorEvent_get_Fatal(p, aFatal) ((p)->lpVtbl->GetFatal(p, aFatal))
#define IRuntimeErrorEvent_GetFatal(p, aFatal) ((p)->lpVtbl->GetFatal(p, aFatal))
#define IRuntimeErrorEvent_get_Id(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IRuntimeErrorEvent_GetId(p, aId) ((p)->lpVtbl->GetId(p, aId))
#define IRuntimeErrorEvent_get_Message(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define IRuntimeErrorEvent_GetMessage(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#endif /* VBOX_WITH_GLUE */

interface IRuntimeErrorEvent
{
#ifndef VBOX_WITH_GLUE
    struct IRuntimeErrorEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IRuntimeErrorEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IRuntimeErrorEvent declaration */


/* Start of struct IEventSourceChangedEvent declaration */
#define IEVENTSOURCECHANGEDEVENT_IID_STR "e7932cb8-f6d4-4ab6-9cbf-558eb8959a6a"
#define IEVENTSOURCECHANGEDEVENT_IID { \
    0xe7932cb8, 0xf6d4, 0x4ab6, \
    { 0x9c, 0xbf, 0x55, 0x8e, 0xb8, 0x95, 0x9a, 0x6a } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IEventSourceChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IEventSourceChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetListener)(IEventSourceChangedEvent *pThis, IEventListener * *listener);

    nsresult (*GetAdd)(IEventSourceChangedEvent *pThis, PRBool *add);

};
#else /* VBOX_WITH_GLUE */
struct IEventSourceChangedEventVtbl
{
    nsresult (*QueryInterface)(IEventSourceChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IEventSourceChangedEvent *pThis);
    nsrefcnt (*Release)(IEventSourceChangedEvent *pThis);
    nsresult (*GetType)(IEventSourceChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IEventSourceChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IEventSourceChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IEventSourceChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IEventSourceChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetListener)(IEventSourceChangedEvent *pThis, IEventListener * *listener);

    nsresult (*GetAdd)(IEventSourceChangedEvent *pThis, PRBool *add);

};
#define IEventSourceChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IEventSourceChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IEventSourceChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IEventSourceChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IEventSourceChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IEventSourceChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IEventSourceChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IEventSourceChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IEventSourceChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IEventSourceChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IEventSourceChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IEventSourceChangedEvent_get_Listener(p, aListener) ((p)->lpVtbl->GetListener(p, aListener))
#define IEventSourceChangedEvent_GetListener(p, aListener) ((p)->lpVtbl->GetListener(p, aListener))
#define IEventSourceChangedEvent_get_Add(p, aAdd) ((p)->lpVtbl->GetAdd(p, aAdd))
#define IEventSourceChangedEvent_GetAdd(p, aAdd) ((p)->lpVtbl->GetAdd(p, aAdd))
#endif /* VBOX_WITH_GLUE */

interface IEventSourceChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IEventSourceChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IEventSourceChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IEventSourceChangedEvent declaration */


/* Start of struct IExtraDataChangedEvent declaration */
#define IEXTRADATACHANGEDEVENT_IID_STR "024F00CE-6E0B-492A-A8D0-968472A94DC7"
#define IEXTRADATACHANGEDEVENT_IID { \
    0x024F00CE, 0x6E0B, 0x492A, \
    { 0xA8, 0xD0, 0x96, 0x84, 0x72, 0xA9, 0x4D, 0xC7 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtraDataChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IExtraDataChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetMachineId)(IExtraDataChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetKey)(IExtraDataChangedEvent *pThis, PRUnichar * *key);

    nsresult (*GetValue)(IExtraDataChangedEvent *pThis, PRUnichar * *value);

};
#else /* VBOX_WITH_GLUE */
struct IExtraDataChangedEventVtbl
{
    nsresult (*QueryInterface)(IExtraDataChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtraDataChangedEvent *pThis);
    nsrefcnt (*Release)(IExtraDataChangedEvent *pThis);
    nsresult (*GetType)(IExtraDataChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IExtraDataChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IExtraDataChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IExtraDataChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IExtraDataChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IExtraDataChangedEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetKey)(IExtraDataChangedEvent *pThis, PRUnichar * *key);

    nsresult (*GetValue)(IExtraDataChangedEvent *pThis, PRUnichar * *value);

};
#define IExtraDataChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtraDataChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtraDataChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IExtraDataChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IExtraDataChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IExtraDataChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IExtraDataChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IExtraDataChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IExtraDataChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IExtraDataChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IExtraDataChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IExtraDataChangedEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IExtraDataChangedEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IExtraDataChangedEvent_get_Key(p, aKey) ((p)->lpVtbl->GetKey(p, aKey))
#define IExtraDataChangedEvent_GetKey(p, aKey) ((p)->lpVtbl->GetKey(p, aKey))
#define IExtraDataChangedEvent_get_Value(p, aValue) ((p)->lpVtbl->GetValue(p, aValue))
#define IExtraDataChangedEvent_GetValue(p, aValue) ((p)->lpVtbl->GetValue(p, aValue))
#endif /* VBOX_WITH_GLUE */

interface IExtraDataChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IExtraDataChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtraDataChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtraDataChangedEvent declaration */


/* Start of struct IVetoEvent declaration */
#define IVETOEVENT_IID_STR "9a1a4130-69fe-472f-ac10-c6fa25d75007"
#define IVETOEVENT_IID { \
    0x9a1a4130, 0x69fe, 0x472f, \
    { 0xac, 0x10, 0xc6, 0xfa, 0x25, 0xd7, 0x50, 0x07 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVetoEvent);
#ifndef VBOX_WITH_GLUE
struct IVetoEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*AddVeto)(
        IVetoEvent *pThis,
        PRUnichar * reason
    );

    nsresult (*IsVetoed)(
        IVetoEvent *pThis,
        PRBool * result
    );

    nsresult (*GetVetos)(
        IVetoEvent *pThis,
        PRUint32 *resultSize,
        PRUnichar *** result
    );

};
#else /* VBOX_WITH_GLUE */
struct IVetoEventVtbl
{
    nsresult (*QueryInterface)(IVetoEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVetoEvent *pThis);
    nsrefcnt (*Release)(IVetoEvent *pThis);
    nsresult (*GetType)(IVetoEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IVetoEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IVetoEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IVetoEvent *pThis );

    nsresult (*WaitProcessed)(
        IVetoEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*AddVeto)(
        IVetoEvent *pThis,
        PRUnichar * reason
    );

    nsresult (*IsVetoed)(
        IVetoEvent *pThis,
        PRBool * result
    );

    nsresult (*GetVetos)(
        IVetoEvent *pThis,
        PRUint32 *resultSize,
        PRUnichar *** result
    );

};
#define IVetoEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVetoEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVetoEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IVetoEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVetoEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVetoEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVetoEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVetoEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVetoEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVetoEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IVetoEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IVetoEvent_AddVeto(p, aReason) ((p)->lpVtbl->AddVeto(p, aReason))
#define IVetoEvent_IsVetoed(p, aResult) ((p)->lpVtbl->IsVetoed(p, aResult))
#define IVetoEvent_GetVetos(p, aResult) ((p)->lpVtbl->GetVetos(p, aResult))
#endif /* VBOX_WITH_GLUE */

interface IVetoEvent
{
#ifndef VBOX_WITH_GLUE
    struct IVetoEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVetoEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVetoEvent declaration */


/* Start of struct IExtraDataCanChangeEvent declaration */
#define IEXTRADATACANCHANGEEVENT_IID_STR "245d88bd-800a-40f8-87a6-170d02249a55"
#define IEXTRADATACANCHANGEEVENT_IID { \
    0x245d88bd, 0x800a, 0x40f8, \
    { 0x87, 0xa6, 0x17, 0x0d, 0x02, 0x24, 0x9a, 0x55 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IExtraDataCanChangeEvent);
#ifndef VBOX_WITH_GLUE
struct IExtraDataCanChangeEvent_vtbl
{
    struct IVetoEvent_vtbl ivetoevent;

    nsresult (*GetMachineId)(IExtraDataCanChangeEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetKey)(IExtraDataCanChangeEvent *pThis, PRUnichar * *key);

    nsresult (*GetValue)(IExtraDataCanChangeEvent *pThis, PRUnichar * *value);

};
#else /* VBOX_WITH_GLUE */
struct IExtraDataCanChangeEventVtbl
{
    nsresult (*QueryInterface)(IExtraDataCanChangeEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IExtraDataCanChangeEvent *pThis);
    nsrefcnt (*Release)(IExtraDataCanChangeEvent *pThis);
    nsresult (*GetType)(IExtraDataCanChangeEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IExtraDataCanChangeEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IExtraDataCanChangeEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IExtraDataCanChangeEvent *pThis );

    nsresult (*WaitProcessed)(
        IExtraDataCanChangeEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*AddVeto)(
        IExtraDataCanChangeEvent *pThis,
        PRUnichar * reason
    );

    nsresult (*IsVetoed)(
        IExtraDataCanChangeEvent *pThis,
        PRBool * result
    );

    nsresult (*GetVetos)(
        IExtraDataCanChangeEvent *pThis,
        PRUint32 *resultSize,
        PRUnichar *** result
    );

    nsresult (*GetMachineId)(IExtraDataCanChangeEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetKey)(IExtraDataCanChangeEvent *pThis, PRUnichar * *key);

    nsresult (*GetValue)(IExtraDataCanChangeEvent *pThis, PRUnichar * *value);

};
#define IExtraDataCanChangeEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IExtraDataCanChangeEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IExtraDataCanChangeEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IExtraDataCanChangeEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IExtraDataCanChangeEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IExtraDataCanChangeEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IExtraDataCanChangeEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IExtraDataCanChangeEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IExtraDataCanChangeEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IExtraDataCanChangeEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IExtraDataCanChangeEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IExtraDataCanChangeEvent_AddVeto(p, aReason) ((p)->lpVtbl->AddVeto(p, aReason))
#define IExtraDataCanChangeEvent_IsVetoed(p, aResult) ((p)->lpVtbl->IsVetoed(p, aResult))
#define IExtraDataCanChangeEvent_GetVetos(p, aResult) ((p)->lpVtbl->GetVetos(p, aResult))
#define IExtraDataCanChangeEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IExtraDataCanChangeEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IExtraDataCanChangeEvent_get_Key(p, aKey) ((p)->lpVtbl->GetKey(p, aKey))
#define IExtraDataCanChangeEvent_GetKey(p, aKey) ((p)->lpVtbl->GetKey(p, aKey))
#define IExtraDataCanChangeEvent_get_Value(p, aValue) ((p)->lpVtbl->GetValue(p, aValue))
#define IExtraDataCanChangeEvent_GetValue(p, aValue) ((p)->lpVtbl->GetValue(p, aValue))
#endif /* VBOX_WITH_GLUE */

interface IExtraDataCanChangeEvent
{
#ifndef VBOX_WITH_GLUE
    struct IExtraDataCanChangeEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IExtraDataCanChangeEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IExtraDataCanChangeEvent declaration */


/* Start of struct ICanShowWindowEvent declaration */
#define ICANSHOWWINDOWEVENT_IID_STR "adf292b0-92c9-4a77-9d35-e058b39fe0b9"
#define ICANSHOWWINDOWEVENT_IID { \
    0xadf292b0, 0x92c9, 0x4a77, \
    { 0x9d, 0x35, 0xe0, 0x58, 0xb3, 0x9f, 0xe0, 0xb9 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_ICanShowWindowEvent);
#ifndef VBOX_WITH_GLUE
struct ICanShowWindowEvent_vtbl
{
    struct IVetoEvent_vtbl ivetoevent;

};
#else /* VBOX_WITH_GLUE */
struct ICanShowWindowEventVtbl
{
    nsresult (*QueryInterface)(ICanShowWindowEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(ICanShowWindowEvent *pThis);
    nsrefcnt (*Release)(ICanShowWindowEvent *pThis);
    nsresult (*GetType)(ICanShowWindowEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(ICanShowWindowEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(ICanShowWindowEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(ICanShowWindowEvent *pThis );

    nsresult (*WaitProcessed)(
        ICanShowWindowEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*AddVeto)(
        ICanShowWindowEvent *pThis,
        PRUnichar * reason
    );

    nsresult (*IsVetoed)(
        ICanShowWindowEvent *pThis,
        PRBool * result
    );

    nsresult (*GetVetos)(
        ICanShowWindowEvent *pThis,
        PRUint32 *resultSize,
        PRUnichar *** result
    );

};
#define ICanShowWindowEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define ICanShowWindowEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define ICanShowWindowEvent_Release(p) ((p)->lpVtbl->Release(p))
#define ICanShowWindowEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ICanShowWindowEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define ICanShowWindowEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ICanShowWindowEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define ICanShowWindowEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ICanShowWindowEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define ICanShowWindowEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define ICanShowWindowEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define ICanShowWindowEvent_AddVeto(p, aReason) ((p)->lpVtbl->AddVeto(p, aReason))
#define ICanShowWindowEvent_IsVetoed(p, aResult) ((p)->lpVtbl->IsVetoed(p, aResult))
#define ICanShowWindowEvent_GetVetos(p, aResult) ((p)->lpVtbl->GetVetos(p, aResult))
#endif /* VBOX_WITH_GLUE */

interface ICanShowWindowEvent
{
#ifndef VBOX_WITH_GLUE
    struct ICanShowWindowEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct ICanShowWindowEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct ICanShowWindowEvent declaration */


/* Start of struct IShowWindowEvent declaration */
#define ISHOWWINDOWEVENT_IID_STR "B0A0904D-2F05-4D28-855F-488F96BAD2B2"
#define ISHOWWINDOWEVENT_IID { \
    0xB0A0904D, 0x2F05, 0x4D28, \
    { 0x85, 0x5F, 0x48, 0x8F, 0x96, 0xBA, 0xD2, 0xB2 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IShowWindowEvent);
#ifndef VBOX_WITH_GLUE
struct IShowWindowEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetWinId)(IShowWindowEvent *pThis, PRInt64 *winId);
    nsresult (*SetWinId)(IShowWindowEvent *pThis, PRInt64 winId);

};
#else /* VBOX_WITH_GLUE */
struct IShowWindowEventVtbl
{
    nsresult (*QueryInterface)(IShowWindowEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IShowWindowEvent *pThis);
    nsrefcnt (*Release)(IShowWindowEvent *pThis);
    nsresult (*GetType)(IShowWindowEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IShowWindowEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IShowWindowEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IShowWindowEvent *pThis );

    nsresult (*WaitProcessed)(
        IShowWindowEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetWinId)(IShowWindowEvent *pThis, PRInt64 *winId);
    nsresult (*SetWinId)(IShowWindowEvent *pThis, PRInt64 winId);

};
#define IShowWindowEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IShowWindowEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IShowWindowEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IShowWindowEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IShowWindowEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IShowWindowEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IShowWindowEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IShowWindowEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IShowWindowEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IShowWindowEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IShowWindowEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IShowWindowEvent_get_WinId(p, aWinId) ((p)->lpVtbl->GetWinId(p, aWinId))
#define IShowWindowEvent_GetWinId(p, aWinId) ((p)->lpVtbl->GetWinId(p, aWinId))
#define IShowWindowEvent_put_WinId(p, aWinId) ((p)->lpVtbl->SetWinId(p, aWinId))
#define IShowWindowEvent_SetWinId(p, aWinId) ((p)->lpVtbl->SetWinId(p, aWinId))
#endif /* VBOX_WITH_GLUE */

interface IShowWindowEvent
{
#ifndef VBOX_WITH_GLUE
    struct IShowWindowEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IShowWindowEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IShowWindowEvent declaration */


/* Start of struct INATRedirectEvent declaration */
#define INATREDIRECTEVENT_IID_STR "24eef068-c380-4510-bc7c-19314a7352f1"
#define INATREDIRECTEVENT_IID { \
    0x24eef068, 0xc380, 0x4510, \
    { 0xbc, 0x7c, 0x19, 0x31, 0x4a, 0x73, 0x52, 0xf1 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATRedirectEvent);
#ifndef VBOX_WITH_GLUE
struct INATRedirectEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetSlot)(INATRedirectEvent *pThis, PRUint32 *slot);

    nsresult (*GetRemove)(INATRedirectEvent *pThis, PRBool *remove);

    nsresult (*GetName)(INATRedirectEvent *pThis, PRUnichar * *name);

    nsresult (*GetProto)(INATRedirectEvent *pThis, PRUint32 *proto);

    nsresult (*GetHostIP)(INATRedirectEvent *pThis, PRUnichar * *hostIP);

    nsresult (*GetHostPort)(INATRedirectEvent *pThis, PRInt32 *hostPort);

    nsresult (*GetGuestIP)(INATRedirectEvent *pThis, PRUnichar * *guestIP);

    nsresult (*GetGuestPort)(INATRedirectEvent *pThis, PRInt32 *guestPort);

};
#else /* VBOX_WITH_GLUE */
struct INATRedirectEventVtbl
{
    nsresult (*QueryInterface)(INATRedirectEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATRedirectEvent *pThis);
    nsrefcnt (*Release)(INATRedirectEvent *pThis);
    nsresult (*GetType)(INATRedirectEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATRedirectEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATRedirectEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATRedirectEvent *pThis );

    nsresult (*WaitProcessed)(
        INATRedirectEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(INATRedirectEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetSlot)(INATRedirectEvent *pThis, PRUint32 *slot);

    nsresult (*GetRemove)(INATRedirectEvent *pThis, PRBool *remove);

    nsresult (*GetName)(INATRedirectEvent *pThis, PRUnichar * *name);

    nsresult (*GetProto)(INATRedirectEvent *pThis, PRUint32 *proto);

    nsresult (*GetHostIP)(INATRedirectEvent *pThis, PRUnichar * *hostIP);

    nsresult (*GetHostPort)(INATRedirectEvent *pThis, PRInt32 *hostPort);

    nsresult (*GetGuestIP)(INATRedirectEvent *pThis, PRUnichar * *guestIP);

    nsresult (*GetGuestPort)(INATRedirectEvent *pThis, PRInt32 *guestPort);

};
#define INATRedirectEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATRedirectEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATRedirectEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATRedirectEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATRedirectEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATRedirectEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATRedirectEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATRedirectEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATRedirectEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATRedirectEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATRedirectEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATRedirectEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define INATRedirectEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define INATRedirectEvent_get_Slot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define INATRedirectEvent_GetSlot(p, aSlot) ((p)->lpVtbl->GetSlot(p, aSlot))
#define INATRedirectEvent_get_Remove(p, aRemove) ((p)->lpVtbl->GetRemove(p, aRemove))
#define INATRedirectEvent_GetRemove(p, aRemove) ((p)->lpVtbl->GetRemove(p, aRemove))
#define INATRedirectEvent_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define INATRedirectEvent_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define INATRedirectEvent_get_Proto(p, aProto) ((p)->lpVtbl->GetProto(p, aProto))
#define INATRedirectEvent_GetProto(p, aProto) ((p)->lpVtbl->GetProto(p, aProto))
#define INATRedirectEvent_get_HostIP(p, aHostIP) ((p)->lpVtbl->GetHostIP(p, aHostIP))
#define INATRedirectEvent_GetHostIP(p, aHostIP) ((p)->lpVtbl->GetHostIP(p, aHostIP))
#define INATRedirectEvent_get_HostPort(p, aHostPort) ((p)->lpVtbl->GetHostPort(p, aHostPort))
#define INATRedirectEvent_GetHostPort(p, aHostPort) ((p)->lpVtbl->GetHostPort(p, aHostPort))
#define INATRedirectEvent_get_GuestIP(p, aGuestIP) ((p)->lpVtbl->GetGuestIP(p, aGuestIP))
#define INATRedirectEvent_GetGuestIP(p, aGuestIP) ((p)->lpVtbl->GetGuestIP(p, aGuestIP))
#define INATRedirectEvent_get_GuestPort(p, aGuestPort) ((p)->lpVtbl->GetGuestPort(p, aGuestPort))
#define INATRedirectEvent_GetGuestPort(p, aGuestPort) ((p)->lpVtbl->GetGuestPort(p, aGuestPort))
#endif /* VBOX_WITH_GLUE */

interface INATRedirectEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATRedirectEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATRedirectEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATRedirectEvent declaration */


/* Start of struct IHostPCIDevicePlugEvent declaration */
#define IHOSTPCIDEVICEPLUGEVENT_IID_STR "a0bad6df-d612-47d3-89d4-db3992533948"
#define IHOSTPCIDEVICEPLUGEVENT_IID { \
    0xa0bad6df, 0xd612, 0x47d3, \
    { 0x89, 0xd4, 0xdb, 0x39, 0x92, 0x53, 0x39, 0x48 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHostPCIDevicePlugEvent);
#ifndef VBOX_WITH_GLUE
struct IHostPCIDevicePlugEvent_vtbl
{
    struct IMachineEvent_vtbl imachineevent;

    nsresult (*GetPlugged)(IHostPCIDevicePlugEvent *pThis, PRBool *plugged);

    nsresult (*GetSuccess)(IHostPCIDevicePlugEvent *pThis, PRBool *success);

    nsresult (*GetAttachment)(IHostPCIDevicePlugEvent *pThis, IPCIDeviceAttachment * *attachment);

    nsresult (*GetMessage)(IHostPCIDevicePlugEvent *pThis, PRUnichar * *message);

};
#else /* VBOX_WITH_GLUE */
struct IHostPCIDevicePlugEventVtbl
{
    nsresult (*QueryInterface)(IHostPCIDevicePlugEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHostPCIDevicePlugEvent *pThis);
    nsrefcnt (*Release)(IHostPCIDevicePlugEvent *pThis);
    nsresult (*GetType)(IHostPCIDevicePlugEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IHostPCIDevicePlugEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IHostPCIDevicePlugEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IHostPCIDevicePlugEvent *pThis );

    nsresult (*WaitProcessed)(
        IHostPCIDevicePlugEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetMachineId)(IHostPCIDevicePlugEvent *pThis, PRUnichar * *machineId);

    nsresult (*GetPlugged)(IHostPCIDevicePlugEvent *pThis, PRBool *plugged);

    nsresult (*GetSuccess)(IHostPCIDevicePlugEvent *pThis, PRBool *success);

    nsresult (*GetAttachment)(IHostPCIDevicePlugEvent *pThis, IPCIDeviceAttachment * *attachment);

    nsresult (*GetMessage)(IHostPCIDevicePlugEvent *pThis, PRUnichar * *message);

};
#define IHostPCIDevicePlugEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHostPCIDevicePlugEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHostPCIDevicePlugEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IHostPCIDevicePlugEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IHostPCIDevicePlugEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IHostPCIDevicePlugEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IHostPCIDevicePlugEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IHostPCIDevicePlugEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IHostPCIDevicePlugEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IHostPCIDevicePlugEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IHostPCIDevicePlugEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IHostPCIDevicePlugEvent_get_MachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IHostPCIDevicePlugEvent_GetMachineId(p, aMachineId) ((p)->lpVtbl->GetMachineId(p, aMachineId))
#define IHostPCIDevicePlugEvent_get_Plugged(p, aPlugged) ((p)->lpVtbl->GetPlugged(p, aPlugged))
#define IHostPCIDevicePlugEvent_GetPlugged(p, aPlugged) ((p)->lpVtbl->GetPlugged(p, aPlugged))
#define IHostPCIDevicePlugEvent_get_Success(p, aSuccess) ((p)->lpVtbl->GetSuccess(p, aSuccess))
#define IHostPCIDevicePlugEvent_GetSuccess(p, aSuccess) ((p)->lpVtbl->GetSuccess(p, aSuccess))
#define IHostPCIDevicePlugEvent_get_Attachment(p, aAttachment) ((p)->lpVtbl->GetAttachment(p, aAttachment))
#define IHostPCIDevicePlugEvent_GetAttachment(p, aAttachment) ((p)->lpVtbl->GetAttachment(p, aAttachment))
#define IHostPCIDevicePlugEvent_get_Message(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#define IHostPCIDevicePlugEvent_GetMessage(p, aMessage) ((p)->lpVtbl->GetMessage(p, aMessage))
#endif /* VBOX_WITH_GLUE */

interface IHostPCIDevicePlugEvent
{
#ifndef VBOX_WITH_GLUE
    struct IHostPCIDevicePlugEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostPCIDevicePlugEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHostPCIDevicePlugEvent declaration */


/* Start of struct IVBoxSVCAvailabilityChangedEvent declaration */
#define IVBOXSVCAVAILABILITYCHANGEDEVENT_IID_STR "97c78fcd-d4fc-485f-8613-5af88bfcfcdc"
#define IVBOXSVCAVAILABILITYCHANGEDEVENT_IID { \
    0x97c78fcd, 0xd4fc, 0x485f, \
    { 0x86, 0x13, 0x5a, 0xf8, 0x8b, 0xfc, 0xfc, 0xdc } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IVBoxSVCAvailabilityChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IVBoxSVCAvailabilityChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetAvailable)(IVBoxSVCAvailabilityChangedEvent *pThis, PRBool *available);

};
#else /* VBOX_WITH_GLUE */
struct IVBoxSVCAvailabilityChangedEventVtbl
{
    nsresult (*QueryInterface)(IVBoxSVCAvailabilityChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IVBoxSVCAvailabilityChangedEvent *pThis);
    nsrefcnt (*Release)(IVBoxSVCAvailabilityChangedEvent *pThis);
    nsresult (*GetType)(IVBoxSVCAvailabilityChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IVBoxSVCAvailabilityChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IVBoxSVCAvailabilityChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IVBoxSVCAvailabilityChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IVBoxSVCAvailabilityChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetAvailable)(IVBoxSVCAvailabilityChangedEvent *pThis, PRBool *available);

};
#define IVBoxSVCAvailabilityChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IVBoxSVCAvailabilityChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IVBoxSVCAvailabilityChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IVBoxSVCAvailabilityChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVBoxSVCAvailabilityChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IVBoxSVCAvailabilityChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVBoxSVCAvailabilityChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IVBoxSVCAvailabilityChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVBoxSVCAvailabilityChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IVBoxSVCAvailabilityChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IVBoxSVCAvailabilityChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IVBoxSVCAvailabilityChangedEvent_get_Available(p, aAvailable) ((p)->lpVtbl->GetAvailable(p, aAvailable))
#define IVBoxSVCAvailabilityChangedEvent_GetAvailable(p, aAvailable) ((p)->lpVtbl->GetAvailable(p, aAvailable))
#endif /* VBOX_WITH_GLUE */

interface IVBoxSVCAvailabilityChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IVBoxSVCAvailabilityChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IVBoxSVCAvailabilityChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IVBoxSVCAvailabilityChangedEvent declaration */


/* Start of struct IBandwidthGroupChangedEvent declaration */
#define IBANDWIDTHGROUPCHANGEDEVENT_IID_STR "334df94a-7556-4cbc-8c04-043096b02d82"
#define IBANDWIDTHGROUPCHANGEDEVENT_IID { \
    0x334df94a, 0x7556, 0x4cbc, \
    { 0x8c, 0x04, 0x04, 0x30, 0x96, 0xb0, 0x2d, 0x82 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IBandwidthGroupChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IBandwidthGroupChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetBandwidthGroup)(IBandwidthGroupChangedEvent *pThis, IBandwidthGroup * *bandwidthGroup);

};
#else /* VBOX_WITH_GLUE */
struct IBandwidthGroupChangedEventVtbl
{
    nsresult (*QueryInterface)(IBandwidthGroupChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IBandwidthGroupChangedEvent *pThis);
    nsrefcnt (*Release)(IBandwidthGroupChangedEvent *pThis);
    nsresult (*GetType)(IBandwidthGroupChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IBandwidthGroupChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IBandwidthGroupChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IBandwidthGroupChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IBandwidthGroupChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetBandwidthGroup)(IBandwidthGroupChangedEvent *pThis, IBandwidthGroup * *bandwidthGroup);

};
#define IBandwidthGroupChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IBandwidthGroupChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IBandwidthGroupChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IBandwidthGroupChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IBandwidthGroupChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IBandwidthGroupChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IBandwidthGroupChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IBandwidthGroupChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IBandwidthGroupChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IBandwidthGroupChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IBandwidthGroupChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IBandwidthGroupChangedEvent_get_BandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aBandwidthGroup))
#define IBandwidthGroupChangedEvent_GetBandwidthGroup(p, aBandwidthGroup) ((p)->lpVtbl->GetBandwidthGroup(p, aBandwidthGroup))
#endif /* VBOX_WITH_GLUE */

interface IBandwidthGroupChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IBandwidthGroupChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IBandwidthGroupChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IBandwidthGroupChangedEvent declaration */


/* Start of struct IGuestMonitorChangedEvent declaration */
#define IGUESTMONITORCHANGEDEVENT_IID_STR "0f7b8a22-c71f-4a36-8e5f-a77d01d76090"
#define IGUESTMONITORCHANGEDEVENT_IID { \
    0x0f7b8a22, 0xc71f, 0x4a36, \
    { 0x8e, 0x5f, 0xa7, 0x7d, 0x01, 0xd7, 0x60, 0x90 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestMonitorChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestMonitorChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetChangeType)(IGuestMonitorChangedEvent *pThis, PRUint32 *changeType);

    nsresult (*GetScreenId)(IGuestMonitorChangedEvent *pThis, PRUint32 *screenId);

    nsresult (*GetOriginX)(IGuestMonitorChangedEvent *pThis, PRUint32 *originX);

    nsresult (*GetOriginY)(IGuestMonitorChangedEvent *pThis, PRUint32 *originY);

    nsresult (*GetWidth)(IGuestMonitorChangedEvent *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IGuestMonitorChangedEvent *pThis, PRUint32 *height);

};
#else /* VBOX_WITH_GLUE */
struct IGuestMonitorChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestMonitorChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestMonitorChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestMonitorChangedEvent *pThis);
    nsresult (*GetType)(IGuestMonitorChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestMonitorChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestMonitorChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestMonitorChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestMonitorChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetChangeType)(IGuestMonitorChangedEvent *pThis, PRUint32 *changeType);

    nsresult (*GetScreenId)(IGuestMonitorChangedEvent *pThis, PRUint32 *screenId);

    nsresult (*GetOriginX)(IGuestMonitorChangedEvent *pThis, PRUint32 *originX);

    nsresult (*GetOriginY)(IGuestMonitorChangedEvent *pThis, PRUint32 *originY);

    nsresult (*GetWidth)(IGuestMonitorChangedEvent *pThis, PRUint32 *width);

    nsresult (*GetHeight)(IGuestMonitorChangedEvent *pThis, PRUint32 *height);

};
#define IGuestMonitorChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestMonitorChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestMonitorChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestMonitorChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestMonitorChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestMonitorChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestMonitorChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestMonitorChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestMonitorChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestMonitorChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestMonitorChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestMonitorChangedEvent_get_ChangeType(p, aChangeType) ((p)->lpVtbl->GetChangeType(p, aChangeType))
#define IGuestMonitorChangedEvent_GetChangeType(p, aChangeType) ((p)->lpVtbl->GetChangeType(p, aChangeType))
#define IGuestMonitorChangedEvent_get_ScreenId(p, aScreenId) ((p)->lpVtbl->GetScreenId(p, aScreenId))
#define IGuestMonitorChangedEvent_GetScreenId(p, aScreenId) ((p)->lpVtbl->GetScreenId(p, aScreenId))
#define IGuestMonitorChangedEvent_get_OriginX(p, aOriginX) ((p)->lpVtbl->GetOriginX(p, aOriginX))
#define IGuestMonitorChangedEvent_GetOriginX(p, aOriginX) ((p)->lpVtbl->GetOriginX(p, aOriginX))
#define IGuestMonitorChangedEvent_get_OriginY(p, aOriginY) ((p)->lpVtbl->GetOriginY(p, aOriginY))
#define IGuestMonitorChangedEvent_GetOriginY(p, aOriginY) ((p)->lpVtbl->GetOriginY(p, aOriginY))
#define IGuestMonitorChangedEvent_get_Width(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IGuestMonitorChangedEvent_GetWidth(p, aWidth) ((p)->lpVtbl->GetWidth(p, aWidth))
#define IGuestMonitorChangedEvent_get_Height(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#define IGuestMonitorChangedEvent_GetHeight(p, aHeight) ((p)->lpVtbl->GetHeight(p, aHeight))
#endif /* VBOX_WITH_GLUE */

interface IGuestMonitorChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestMonitorChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestMonitorChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestMonitorChangedEvent declaration */


/* Start of struct IGuestUserStateChangedEvent declaration */
#define IGUESTUSERSTATECHANGEDEVENT_IID_STR "39b4e759-1ec0-4c0f-857f-fbe2a737a256"
#define IGUESTUSERSTATECHANGEDEVENT_IID { \
    0x39b4e759, 0x1ec0, 0x4c0f, \
    { 0x85, 0x7f, 0xfb, 0xe2, 0xa7, 0x37, 0xa2, 0x56 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IGuestUserStateChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IGuestUserStateChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetName)(IGuestUserStateChangedEvent *pThis, PRUnichar * *name);

    nsresult (*GetDomain)(IGuestUserStateChangedEvent *pThis, PRUnichar * *domain);

    nsresult (*GetState)(IGuestUserStateChangedEvent *pThis, PRUint32 *state);

    nsresult (*GetStateDetails)(IGuestUserStateChangedEvent *pThis, PRUnichar * *stateDetails);

};
#else /* VBOX_WITH_GLUE */
struct IGuestUserStateChangedEventVtbl
{
    nsresult (*QueryInterface)(IGuestUserStateChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IGuestUserStateChangedEvent *pThis);
    nsrefcnt (*Release)(IGuestUserStateChangedEvent *pThis);
    nsresult (*GetType)(IGuestUserStateChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IGuestUserStateChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IGuestUserStateChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IGuestUserStateChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IGuestUserStateChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetName)(IGuestUserStateChangedEvent *pThis, PRUnichar * *name);

    nsresult (*GetDomain)(IGuestUserStateChangedEvent *pThis, PRUnichar * *domain);

    nsresult (*GetState)(IGuestUserStateChangedEvent *pThis, PRUint32 *state);

    nsresult (*GetStateDetails)(IGuestUserStateChangedEvent *pThis, PRUnichar * *stateDetails);

};
#define IGuestUserStateChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IGuestUserStateChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IGuestUserStateChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IGuestUserStateChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestUserStateChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IGuestUserStateChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestUserStateChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IGuestUserStateChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestUserStateChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IGuestUserStateChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IGuestUserStateChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IGuestUserStateChangedEvent_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestUserStateChangedEvent_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define IGuestUserStateChangedEvent_get_Domain(p, aDomain) ((p)->lpVtbl->GetDomain(p, aDomain))
#define IGuestUserStateChangedEvent_GetDomain(p, aDomain) ((p)->lpVtbl->GetDomain(p, aDomain))
#define IGuestUserStateChangedEvent_get_State(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IGuestUserStateChangedEvent_GetState(p, aState) ((p)->lpVtbl->GetState(p, aState))
#define IGuestUserStateChangedEvent_get_StateDetails(p, aStateDetails) ((p)->lpVtbl->GetStateDetails(p, aStateDetails))
#define IGuestUserStateChangedEvent_GetStateDetails(p, aStateDetails) ((p)->lpVtbl->GetStateDetails(p, aStateDetails))
#endif /* VBOX_WITH_GLUE */

interface IGuestUserStateChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IGuestUserStateChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IGuestUserStateChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IGuestUserStateChangedEvent declaration */


/* Start of struct IStorageDeviceChangedEvent declaration */
#define ISTORAGEDEVICECHANGEDEVENT_IID_STR "232e9151-ae84-4b8e-b0f3-5c20c35caac9"
#define ISTORAGEDEVICECHANGEDEVENT_IID { \
    0x232e9151, 0xae84, 0x4b8e, \
    { 0xb0, 0xf3, 0x5c, 0x20, 0xc3, 0x5c, 0xaa, 0xc9 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IStorageDeviceChangedEvent);
#ifndef VBOX_WITH_GLUE
struct IStorageDeviceChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetStorageDevice)(IStorageDeviceChangedEvent *pThis, IMediumAttachment * *storageDevice);

    nsresult (*GetRemoved)(IStorageDeviceChangedEvent *pThis, PRBool *removed);

    nsresult (*GetSilent)(IStorageDeviceChangedEvent *pThis, PRBool *silent);

};
#else /* VBOX_WITH_GLUE */
struct IStorageDeviceChangedEventVtbl
{
    nsresult (*QueryInterface)(IStorageDeviceChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IStorageDeviceChangedEvent *pThis);
    nsrefcnt (*Release)(IStorageDeviceChangedEvent *pThis);
    nsresult (*GetType)(IStorageDeviceChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IStorageDeviceChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IStorageDeviceChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IStorageDeviceChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        IStorageDeviceChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetStorageDevice)(IStorageDeviceChangedEvent *pThis, IMediumAttachment * *storageDevice);

    nsresult (*GetRemoved)(IStorageDeviceChangedEvent *pThis, PRBool *removed);

    nsresult (*GetSilent)(IStorageDeviceChangedEvent *pThis, PRBool *silent);

};
#define IStorageDeviceChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IStorageDeviceChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IStorageDeviceChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IStorageDeviceChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IStorageDeviceChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IStorageDeviceChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IStorageDeviceChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IStorageDeviceChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IStorageDeviceChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IStorageDeviceChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IStorageDeviceChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define IStorageDeviceChangedEvent_get_StorageDevice(p, aStorageDevice) ((p)->lpVtbl->GetStorageDevice(p, aStorageDevice))
#define IStorageDeviceChangedEvent_GetStorageDevice(p, aStorageDevice) ((p)->lpVtbl->GetStorageDevice(p, aStorageDevice))
#define IStorageDeviceChangedEvent_get_Removed(p, aRemoved) ((p)->lpVtbl->GetRemoved(p, aRemoved))
#define IStorageDeviceChangedEvent_GetRemoved(p, aRemoved) ((p)->lpVtbl->GetRemoved(p, aRemoved))
#define IStorageDeviceChangedEvent_get_Silent(p, aSilent) ((p)->lpVtbl->GetSilent(p, aSilent))
#define IStorageDeviceChangedEvent_GetSilent(p, aSilent) ((p)->lpVtbl->GetSilent(p, aSilent))
#endif /* VBOX_WITH_GLUE */

interface IStorageDeviceChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct IStorageDeviceChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IStorageDeviceChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IStorageDeviceChangedEvent declaration */


/* Start of struct INATNetworkChangedEvent declaration */
#define INATNETWORKCHANGEDEVENT_IID_STR "101ae042-1a29-4a19-92cf-02285773f3b5"
#define INATNETWORKCHANGEDEVENT_IID { \
    0x101ae042, 0x1a29, 0x4a19, \
    { 0x92, 0xcf, 0x02, 0x28, 0x57, 0x73, 0xf3, 0xb5 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetworkChangedEvent);
#ifndef VBOX_WITH_GLUE
struct INATNetworkChangedEvent_vtbl
{
    struct IEvent_vtbl ievent;

    nsresult (*GetNetworkName)(INATNetworkChangedEvent *pThis, PRUnichar * *networkName);

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkChangedEventVtbl
{
    nsresult (*QueryInterface)(INATNetworkChangedEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetworkChangedEvent *pThis);
    nsrefcnt (*Release)(INATNetworkChangedEvent *pThis);
    nsresult (*GetType)(INATNetworkChangedEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATNetworkChangedEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATNetworkChangedEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATNetworkChangedEvent *pThis );

    nsresult (*WaitProcessed)(
        INATNetworkChangedEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkName)(INATNetworkChangedEvent *pThis, PRUnichar * *networkName);

};
#define INATNetworkChangedEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetworkChangedEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetworkChangedEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetworkChangedEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkChangedEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkChangedEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkChangedEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkChangedEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkChangedEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkChangedEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATNetworkChangedEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATNetworkChangedEvent_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkChangedEvent_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#endif /* VBOX_WITH_GLUE */

interface INATNetworkChangedEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATNetworkChangedEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkChangedEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetworkChangedEvent declaration */


/* Start of struct INATNetworkStartStopEvent declaration */
#define INATNETWORKSTARTSTOPEVENT_IID_STR "269d8f6b-fa1e-4cee-91c7-6d8496bea3c1"
#define INATNETWORKSTARTSTOPEVENT_IID { \
    0x269d8f6b, 0xfa1e, 0x4cee, \
    { 0x91, 0xc7, 0x6d, 0x84, 0x96, 0xbe, 0xa3, 0xc1 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetworkStartStopEvent);
#ifndef VBOX_WITH_GLUE
struct INATNetworkStartStopEvent_vtbl
{
    struct INATNetworkChangedEvent_vtbl inatnetworkchangedevent;

    nsresult (*GetStartEvent)(INATNetworkStartStopEvent *pThis, PRBool *startEvent);

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkStartStopEventVtbl
{
    nsresult (*QueryInterface)(INATNetworkStartStopEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetworkStartStopEvent *pThis);
    nsrefcnt (*Release)(INATNetworkStartStopEvent *pThis);
    nsresult (*GetType)(INATNetworkStartStopEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATNetworkStartStopEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATNetworkStartStopEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATNetworkStartStopEvent *pThis );

    nsresult (*WaitProcessed)(
        INATNetworkStartStopEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkName)(INATNetworkStartStopEvent *pThis, PRUnichar * *networkName);

    nsresult (*GetStartEvent)(INATNetworkStartStopEvent *pThis, PRBool *startEvent);

};
#define INATNetworkStartStopEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetworkStartStopEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetworkStartStopEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetworkStartStopEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkStartStopEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkStartStopEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkStartStopEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkStartStopEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkStartStopEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkStartStopEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATNetworkStartStopEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATNetworkStartStopEvent_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkStartStopEvent_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkStartStopEvent_get_StartEvent(p, aStartEvent) ((p)->lpVtbl->GetStartEvent(p, aStartEvent))
#define INATNetworkStartStopEvent_GetStartEvent(p, aStartEvent) ((p)->lpVtbl->GetStartEvent(p, aStartEvent))
#endif /* VBOX_WITH_GLUE */

interface INATNetworkStartStopEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATNetworkStartStopEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkStartStopEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetworkStartStopEvent declaration */


/* Start of struct INATNetworkAlterEvent declaration */
#define INATNETWORKALTEREVENT_IID_STR "3f5a0822-163a-43b1-ad16-8d58b0ef6e75"
#define INATNETWORKALTEREVENT_IID { \
    0x3f5a0822, 0x163a, 0x43b1, \
    { 0xad, 0x16, 0x8d, 0x58, 0xb0, 0xef, 0x6e, 0x75 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetworkAlterEvent);
#ifndef VBOX_WITH_GLUE
struct INATNetworkAlterEvent_vtbl
{
    struct INATNetworkChangedEvent_vtbl inatnetworkchangedevent;

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkAlterEventVtbl
{
    nsresult (*QueryInterface)(INATNetworkAlterEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetworkAlterEvent *pThis);
    nsrefcnt (*Release)(INATNetworkAlterEvent *pThis);
    nsresult (*GetType)(INATNetworkAlterEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATNetworkAlterEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATNetworkAlterEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATNetworkAlterEvent *pThis );

    nsresult (*WaitProcessed)(
        INATNetworkAlterEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkName)(INATNetworkAlterEvent *pThis, PRUnichar * *networkName);

};
#define INATNetworkAlterEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetworkAlterEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetworkAlterEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetworkAlterEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkAlterEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkAlterEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkAlterEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkAlterEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkAlterEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkAlterEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATNetworkAlterEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATNetworkAlterEvent_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkAlterEvent_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#endif /* VBOX_WITH_GLUE */

interface INATNetworkAlterEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATNetworkAlterEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkAlterEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetworkAlterEvent declaration */


/* Start of struct INATNetworkCreationDeletionEvent declaration */
#define INATNETWORKCREATIONDELETIONEVENT_IID_STR "8d984a7e-b855-40b8-ab0c-44d3515b4528"
#define INATNETWORKCREATIONDELETIONEVENT_IID { \
    0x8d984a7e, 0xb855, 0x40b8, \
    { 0xab, 0x0c, 0x44, 0xd3, 0x51, 0x5b, 0x45, 0x28 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetworkCreationDeletionEvent);
#ifndef VBOX_WITH_GLUE
struct INATNetworkCreationDeletionEvent_vtbl
{
    struct INATNetworkAlterEvent_vtbl inatnetworkalterevent;

    nsresult (*GetCreationEvent)(INATNetworkCreationDeletionEvent *pThis, PRBool *creationEvent);

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkCreationDeletionEventVtbl
{
    nsresult (*QueryInterface)(INATNetworkCreationDeletionEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetworkCreationDeletionEvent *pThis);
    nsrefcnt (*Release)(INATNetworkCreationDeletionEvent *pThis);
    nsresult (*GetType)(INATNetworkCreationDeletionEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATNetworkCreationDeletionEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATNetworkCreationDeletionEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATNetworkCreationDeletionEvent *pThis );

    nsresult (*WaitProcessed)(
        INATNetworkCreationDeletionEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkName)(INATNetworkCreationDeletionEvent *pThis, PRUnichar * *networkName);

    nsresult (*GetCreationEvent)(INATNetworkCreationDeletionEvent *pThis, PRBool *creationEvent);

};
#define INATNetworkCreationDeletionEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetworkCreationDeletionEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetworkCreationDeletionEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetworkCreationDeletionEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkCreationDeletionEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkCreationDeletionEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkCreationDeletionEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkCreationDeletionEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkCreationDeletionEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkCreationDeletionEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATNetworkCreationDeletionEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATNetworkCreationDeletionEvent_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkCreationDeletionEvent_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkCreationDeletionEvent_get_CreationEvent(p, aCreationEvent) ((p)->lpVtbl->GetCreationEvent(p, aCreationEvent))
#define INATNetworkCreationDeletionEvent_GetCreationEvent(p, aCreationEvent) ((p)->lpVtbl->GetCreationEvent(p, aCreationEvent))
#endif /* VBOX_WITH_GLUE */

interface INATNetworkCreationDeletionEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATNetworkCreationDeletionEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkCreationDeletionEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetworkCreationDeletionEvent declaration */


/* Start of struct INATNetworkSettingEvent declaration */
#define INATNETWORKSETTINGEVENT_IID_STR "9db3a9e6-7f29-4aae-a627-5a282c83092c"
#define INATNETWORKSETTINGEVENT_IID { \
    0x9db3a9e6, 0x7f29, 0x4aae, \
    { 0xa6, 0x27, 0x5a, 0x28, 0x2c, 0x83, 0x09, 0x2c } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetworkSettingEvent);
#ifndef VBOX_WITH_GLUE
struct INATNetworkSettingEvent_vtbl
{
    struct INATNetworkAlterEvent_vtbl inatnetworkalterevent;

    nsresult (*GetEnabled)(INATNetworkSettingEvent *pThis, PRBool *enabled);

    nsresult (*GetNetwork)(INATNetworkSettingEvent *pThis, PRUnichar * *network);

    nsresult (*GetGateway)(INATNetworkSettingEvent *pThis, PRUnichar * *gateway);

    nsresult (*GetAdvertiseDefaultIPv6RouteEnabled)(INATNetworkSettingEvent *pThis, PRBool *advertiseDefaultIPv6RouteEnabled);

    nsresult (*GetNeedDhcpServer)(INATNetworkSettingEvent *pThis, PRBool *needDhcpServer);

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkSettingEventVtbl
{
    nsresult (*QueryInterface)(INATNetworkSettingEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetworkSettingEvent *pThis);
    nsrefcnt (*Release)(INATNetworkSettingEvent *pThis);
    nsresult (*GetType)(INATNetworkSettingEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATNetworkSettingEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATNetworkSettingEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATNetworkSettingEvent *pThis );

    nsresult (*WaitProcessed)(
        INATNetworkSettingEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkName)(INATNetworkSettingEvent *pThis, PRUnichar * *networkName);

    nsresult (*GetEnabled)(INATNetworkSettingEvent *pThis, PRBool *enabled);

    nsresult (*GetNetwork)(INATNetworkSettingEvent *pThis, PRUnichar * *network);

    nsresult (*GetGateway)(INATNetworkSettingEvent *pThis, PRUnichar * *gateway);

    nsresult (*GetAdvertiseDefaultIPv6RouteEnabled)(INATNetworkSettingEvent *pThis, PRBool *advertiseDefaultIPv6RouteEnabled);

    nsresult (*GetNeedDhcpServer)(INATNetworkSettingEvent *pThis, PRBool *needDhcpServer);

};
#define INATNetworkSettingEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetworkSettingEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetworkSettingEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetworkSettingEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkSettingEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkSettingEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkSettingEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkSettingEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkSettingEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkSettingEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATNetworkSettingEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATNetworkSettingEvent_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkSettingEvent_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkSettingEvent_get_Enabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define INATNetworkSettingEvent_GetEnabled(p, aEnabled) ((p)->lpVtbl->GetEnabled(p, aEnabled))
#define INATNetworkSettingEvent_get_Network(p, aNetwork) ((p)->lpVtbl->GetNetwork(p, aNetwork))
#define INATNetworkSettingEvent_GetNetwork(p, aNetwork) ((p)->lpVtbl->GetNetwork(p, aNetwork))
#define INATNetworkSettingEvent_get_Gateway(p, aGateway) ((p)->lpVtbl->GetGateway(p, aGateway))
#define INATNetworkSettingEvent_GetGateway(p, aGateway) ((p)->lpVtbl->GetGateway(p, aGateway))
#define INATNetworkSettingEvent_get_AdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled) ((p)->lpVtbl->GetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled))
#define INATNetworkSettingEvent_GetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled) ((p)->lpVtbl->GetAdvertiseDefaultIPv6RouteEnabled(p, aAdvertiseDefaultIPv6RouteEnabled))
#define INATNetworkSettingEvent_get_NeedDhcpServer(p, aNeedDhcpServer) ((p)->lpVtbl->GetNeedDhcpServer(p, aNeedDhcpServer))
#define INATNetworkSettingEvent_GetNeedDhcpServer(p, aNeedDhcpServer) ((p)->lpVtbl->GetNeedDhcpServer(p, aNeedDhcpServer))
#endif /* VBOX_WITH_GLUE */

interface INATNetworkSettingEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATNetworkSettingEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkSettingEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetworkSettingEvent declaration */


/* Start of struct INATNetworkPortForwardEvent declaration */
#define INATNETWORKPORTFORWARDEVENT_IID_STR "2514881b-23d0-430a-a7ff-7ed7f05534bc"
#define INATNETWORKPORTFORWARDEVENT_IID { \
    0x2514881b, 0x23d0, 0x430a, \
    { 0xa7, 0xff, 0x7e, 0xd7, 0xf0, 0x55, 0x34, 0xbc } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_INATNetworkPortForwardEvent);
#ifndef VBOX_WITH_GLUE
struct INATNetworkPortForwardEvent_vtbl
{
    struct INATNetworkAlterEvent_vtbl inatnetworkalterevent;

    nsresult (*GetCreate)(INATNetworkPortForwardEvent *pThis, PRBool *create);

    nsresult (*GetIpv6)(INATNetworkPortForwardEvent *pThis, PRBool *ipv6);

    nsresult (*GetName)(INATNetworkPortForwardEvent *pThis, PRUnichar * *name);

    nsresult (*GetProto)(INATNetworkPortForwardEvent *pThis, PRUint32 *proto);

    nsresult (*GetHostIp)(INATNetworkPortForwardEvent *pThis, PRUnichar * *hostIp);

    nsresult (*GetHostPort)(INATNetworkPortForwardEvent *pThis, PRInt32 *hostPort);

    nsresult (*GetGuestIp)(INATNetworkPortForwardEvent *pThis, PRUnichar * *guestIp);

    nsresult (*GetGuestPort)(INATNetworkPortForwardEvent *pThis, PRInt32 *guestPort);

};
#else /* VBOX_WITH_GLUE */
struct INATNetworkPortForwardEventVtbl
{
    nsresult (*QueryInterface)(INATNetworkPortForwardEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(INATNetworkPortForwardEvent *pThis);
    nsrefcnt (*Release)(INATNetworkPortForwardEvent *pThis);
    nsresult (*GetType)(INATNetworkPortForwardEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(INATNetworkPortForwardEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(INATNetworkPortForwardEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(INATNetworkPortForwardEvent *pThis );

    nsresult (*WaitProcessed)(
        INATNetworkPortForwardEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

    nsresult (*GetNetworkName)(INATNetworkPortForwardEvent *pThis, PRUnichar * *networkName);

    nsresult (*GetCreate)(INATNetworkPortForwardEvent *pThis, PRBool *create);

    nsresult (*GetIpv6)(INATNetworkPortForwardEvent *pThis, PRBool *ipv6);

    nsresult (*GetName)(INATNetworkPortForwardEvent *pThis, PRUnichar * *name);

    nsresult (*GetProto)(INATNetworkPortForwardEvent *pThis, PRUint32 *proto);

    nsresult (*GetHostIp)(INATNetworkPortForwardEvent *pThis, PRUnichar * *hostIp);

    nsresult (*GetHostPort)(INATNetworkPortForwardEvent *pThis, PRInt32 *hostPort);

    nsresult (*GetGuestIp)(INATNetworkPortForwardEvent *pThis, PRUnichar * *guestIp);

    nsresult (*GetGuestPort)(INATNetworkPortForwardEvent *pThis, PRInt32 *guestPort);

};
#define INATNetworkPortForwardEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define INATNetworkPortForwardEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define INATNetworkPortForwardEvent_Release(p) ((p)->lpVtbl->Release(p))
#define INATNetworkPortForwardEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkPortForwardEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define INATNetworkPortForwardEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkPortForwardEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define INATNetworkPortForwardEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkPortForwardEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define INATNetworkPortForwardEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define INATNetworkPortForwardEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#define INATNetworkPortForwardEvent_get_NetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkPortForwardEvent_GetNetworkName(p, aNetworkName) ((p)->lpVtbl->GetNetworkName(p, aNetworkName))
#define INATNetworkPortForwardEvent_get_Create(p, aCreate) ((p)->lpVtbl->GetCreate(p, aCreate))
#define INATNetworkPortForwardEvent_GetCreate(p, aCreate) ((p)->lpVtbl->GetCreate(p, aCreate))
#define INATNetworkPortForwardEvent_get_Ipv6(p, aIpv6) ((p)->lpVtbl->GetIpv6(p, aIpv6))
#define INATNetworkPortForwardEvent_GetIpv6(p, aIpv6) ((p)->lpVtbl->GetIpv6(p, aIpv6))
#define INATNetworkPortForwardEvent_get_Name(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define INATNetworkPortForwardEvent_GetName(p, aName) ((p)->lpVtbl->GetName(p, aName))
#define INATNetworkPortForwardEvent_get_Proto(p, aProto) ((p)->lpVtbl->GetProto(p, aProto))
#define INATNetworkPortForwardEvent_GetProto(p, aProto) ((p)->lpVtbl->GetProto(p, aProto))
#define INATNetworkPortForwardEvent_get_HostIp(p, aHostIp) ((p)->lpVtbl->GetHostIp(p, aHostIp))
#define INATNetworkPortForwardEvent_GetHostIp(p, aHostIp) ((p)->lpVtbl->GetHostIp(p, aHostIp))
#define INATNetworkPortForwardEvent_get_HostPort(p, aHostPort) ((p)->lpVtbl->GetHostPort(p, aHostPort))
#define INATNetworkPortForwardEvent_GetHostPort(p, aHostPort) ((p)->lpVtbl->GetHostPort(p, aHostPort))
#define INATNetworkPortForwardEvent_get_GuestIp(p, aGuestIp) ((p)->lpVtbl->GetGuestIp(p, aGuestIp))
#define INATNetworkPortForwardEvent_GetGuestIp(p, aGuestIp) ((p)->lpVtbl->GetGuestIp(p, aGuestIp))
#define INATNetworkPortForwardEvent_get_GuestPort(p, aGuestPort) ((p)->lpVtbl->GetGuestPort(p, aGuestPort))
#define INATNetworkPortForwardEvent_GetGuestPort(p, aGuestPort) ((p)->lpVtbl->GetGuestPort(p, aGuestPort))
#endif /* VBOX_WITH_GLUE */

interface INATNetworkPortForwardEvent
{
#ifndef VBOX_WITH_GLUE
    struct INATNetworkPortForwardEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct INATNetworkPortForwardEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct INATNetworkPortForwardEvent declaration */


/* Start of struct IHostNameResolutionConfigurationChangeEvent declaration */
#define IHOSTNAMERESOLUTIONCONFIGURATIONCHANGEEVENT_IID_STR "f9b9e1cf-cb63-47a1-84fb-02c4894b89a9"
#define IHOSTNAMERESOLUTIONCONFIGURATIONCHANGEEVENT_IID { \
    0xf9b9e1cf, 0xcb63, 0x47a1, \
    { 0x84, 0xfb, 0x02, 0xc4, 0x89, 0x4b, 0x89, 0xa9 } \
}
/* COM compatibility */
VBOX_EXTERN_CONST(nsIID, IID_IHostNameResolutionConfigurationChangeEvent);
#ifndef VBOX_WITH_GLUE
struct IHostNameResolutionConfigurationChangeEvent_vtbl
{
    struct IEvent_vtbl ievent;

};
#else /* VBOX_WITH_GLUE */
struct IHostNameResolutionConfigurationChangeEventVtbl
{
    nsresult (*QueryInterface)(IHostNameResolutionConfigurationChangeEvent *pThis, const nsID *iid, void **resultp);
    nsrefcnt (*AddRef)(IHostNameResolutionConfigurationChangeEvent *pThis);
    nsrefcnt (*Release)(IHostNameResolutionConfigurationChangeEvent *pThis);
    nsresult (*GetType)(IHostNameResolutionConfigurationChangeEvent *pThis, PRUint32 *type);

    nsresult (*GetSource)(IHostNameResolutionConfigurationChangeEvent *pThis, IEventSource * *source);

    nsresult (*GetWaitable)(IHostNameResolutionConfigurationChangeEvent *pThis, PRBool *waitable);

    nsresult (*SetProcessed)(IHostNameResolutionConfigurationChangeEvent *pThis );

    nsresult (*WaitProcessed)(
        IHostNameResolutionConfigurationChangeEvent *pThis,
        PRInt32 timeout,
        PRBool * result
    );

};
#define IHostNameResolutionConfigurationChangeEvent_QueryInterface(p, iid, resultp) ((p)->lpVtbl->QueryInterface(p, iid, resultp))
#define IHostNameResolutionConfigurationChangeEvent_AddRef(p) ((p)->lpVtbl->AddRef(p))
#define IHostNameResolutionConfigurationChangeEvent_Release(p) ((p)->lpVtbl->Release(p))
#define IHostNameResolutionConfigurationChangeEvent_get_Type(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IHostNameResolutionConfigurationChangeEvent_GetType(p, aType) ((p)->lpVtbl->GetType(p, aType))
#define IHostNameResolutionConfigurationChangeEvent_get_Source(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IHostNameResolutionConfigurationChangeEvent_GetSource(p, aSource) ((p)->lpVtbl->GetSource(p, aSource))
#define IHostNameResolutionConfigurationChangeEvent_get_Waitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IHostNameResolutionConfigurationChangeEvent_GetWaitable(p, aWaitable) ((p)->lpVtbl->GetWaitable(p, aWaitable))
#define IHostNameResolutionConfigurationChangeEvent_SetProcessed(p) ((p)->lpVtbl->SetProcessed(p))
#define IHostNameResolutionConfigurationChangeEvent_WaitProcessed(p, aTimeout, aResult) ((p)->lpVtbl->WaitProcessed(p, aTimeout, aResult))
#endif /* VBOX_WITH_GLUE */

interface IHostNameResolutionConfigurationChangeEvent
{
#ifndef VBOX_WITH_GLUE
    struct IHostNameResolutionConfigurationChangeEvent_vtbl *vtbl;
#else /* VBOX_WITH_GLUE */
    CONST_VTBL struct IHostNameResolutionConfigurationChangeEventVtbl *lpVtbl;
#endif /* VBOX_WITH_GLUE */
};
/* End of struct IHostNameResolutionConfigurationChangeEvent declaration */



#define NS_VIRTUALBOX_CID { \
    0xB1A7A4F2, 0x47B9, 0x4A1E, \
    { 0x82, 0xB2, 0x07, 0xCC, 0xD5, 0x32, 0x3C, 0x3F } \
}
#define NS_VIRTUALBOX_CONTRACTID "@virtualbox.org/VirtualBox;1"
/* COM compatibility */
VBOX_EXTERN_CONST(nsCID, CLSID_VirtualBox);



#define NS_VIRTUALBOXCLIENT_CID { \
    0xdd3fc71d, 0x26c0, 0x4fe1, \
    { 0xbf, 0x6f, 0x67, 0xf6, 0x33, 0x26, 0x5b, 0xba } \
}
#define NS_VIRTUALBOXCLIENT_CONTRACTID "@virtualbox.org/VirtualBoxClient;1"
/* COM compatibility */
VBOX_EXTERN_CONST(nsCID, CLSID_VirtualBoxClient);



#define NS_SESSION_CID { \
    0x3C02F46D, 0xC9D2, 0x4F11, \
    { 0xA3, 0x84, 0x53, 0xF0, 0xCF, 0x91, 0x72, 0x14 } \
}
#define NS_SESSION_CONTRACTID "@virtualbox.org/Session;1"
/* COM compatibility */
VBOX_EXTERN_CONST(nsCID, CLSID_Session);




#endif /* __cplusplus */

#endif /* !WIN32 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */


/**
 * Function table for dynamic linking.
 * Use VBoxGetCAPIFunctions() to obtain the pointer to it.
 */
typedef struct VBOXCAPI
{
    /** The size of the structure. */
    unsigned cb;
    /** The structure version. */
    unsigned uVersion;

    /** Gets the VirtualBox version, major * 1000000 + minor * 1000 + patch. */
    unsigned int (*pfnGetVersion)(void);

    /** Gets the VirtualBox API version, major * 1000 + minor, e.g. 4003. */
    unsigned int (*pfnGetAPIVersion)(void);

    /**
     * New and preferred way to initialize the C bindings for an API client.
     *
     * This way is much more flexible, as it can easily handle multiple
     * sessions (important with more complicated API clients, including
     * multithreaded ones), and even VBoxSVC crashes can be detected and
     * processed appropriately by listening for events from the associated
     * event source in VirtualBoxClient. It is completely up to the client
     * to decide what to do (terminate or continue after getting new
     * object references to server-side objects). Must be called in the
     * primary thread of the client, later API use can be done in any
     * thread.
     *
     * Note that the returned reference is owned by the caller, and thus it's
     * the caller's responsibility to handle the reference count appropriately.
     *
     * @param pszVirtualBoxClientIID    pass IVIRTUALBOXCLIENT_IID_STR
     * @param ppVirtualBoxClient        output parameter for VirtualBoxClient
     *              reference, handled as usual with COM/XPCOM.
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnClientInitialize)(const char *pszVirtualBoxClientIID,
                                   IVirtualBoxClient **ppVirtualBoxClient);
    /**
     * Initialize the use of the C bindings in a non-primary thread.
     *
     * Must be called on any newly created thread which wants to use the
     * VirtualBox API.
     *
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnClientThreadInitialize)(void);
    /**
     * Uninitialize the use of the C bindings in a non-primary thread.
     *
     * Should be called before terminating the thread which initialized the
     * C bindings using pfnClientThreadInitialize.
     *
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnClientThreadUninitialize)(void);
    /**
     * Uninitialize the C bindings for an API client.
     *
     * Should be called when the API client is about to terminate and does
     * not want to use the C bindings any more. It will invalidate all
     * object references. It is possible, however, to change one's mind,
     * and call pfnClientInitialize again to continue using the API, as long
     * as none of the object references from before the re-initialization
     * are used. Must be called from the primary thread of the client.
     */
    void (*pfnClientUninitialize)(void);

    /**
     * Deprecated way to initialize the C bindings and getting important
     * object references. Kept for backwards compatibility.
     *
     * If any returned reference is NULL then the initialization failed.
     * Note that the returned references are owned by the C bindings. The
     * number of calls to Release in the client code must match the number
     * of calls to AddRef, and additionally at no point in time there can
     * be more Release calls than AddRef calls.
     *
     * @param pszVirtualBoxIID      pass IVIRTUALBOX_IID_STR
     * @param ppVirtualBox          output parameter for VirtualBox reference,
     *          owned by C bindings
     * @param pszSessionIID         pass ISESSION_IID_STR
     * @param ppSession             output parameter for Session reference,
     *          owned by C bindings
     */
    void (*pfnComInitialize)(const char *pszVirtualBoxIID,
                             IVirtualBox **ppVirtualBox,
                             const char *pszSessionIID,
                             ISession **ppSession);
    /**
     * Deprecated way to uninitialize the C bindings for an API client.
     * Kept for backwards compatibility and must be used if the C bindings
     * were initialized using pfnComInitialize. */
    void (*pfnComUninitialize)(void);

    /**
     * Free string managed by COM/XPCOM.
     *
     * @param pwsz          pointer to string to be freed
     */
    void (*pfnComUnallocString)(BSTR pwsz);
#ifndef WIN32
    /** Legacy function, was always for freeing strings only. */
#define pfnComUnallocMem(pv) pfnComUnallocString((BSTR)(pv))
#endif /* !WIN32 */

    /**
     * Convert string from UTF-16 encoding to UTF-8 encoding.
     *
     * @param pwszString    input string
     * @param ppszString    output string
     * @returns IPRT status code
     */
    int (*pfnUtf16ToUtf8)(CBSTR pwszString, char **ppszString);
    /**
     * Convert string from UTF-8 encoding to UTF-16 encoding.
     *
     * @param pszString     input string
     * @param ppwszString   output string
     * @returns IPRT status code
     */
    int (*pfnUtf8ToUtf16)(const char *pszString, BSTR *ppwszString);
    /**
     * Free memory returned by pfnUtf16ToUtf8. Do not use for anything else.
     *
     * @param pszString     string to be freed.
     */
    void (*pfnUtf8Free)(char *pszString);
    /**
     * Free memory returned by pfnUtf8ToUtf16. Do not use for anything else.
     *
     * @param pwszString    string to be freed.
     */
    void (*pfnUtf16Free)(BSTR pwszString);

    /**
     * Create a safearray (used for passing arrays to COM/XPCOM)
     *
     * Must be freed by pfnSafeArrayDestroy.
     *
     * @param vt            variant type, defines the size of the elements
     * @param lLbound       lower bound of the index, should be 0
     * @param cElements     number of elements
     * @returns pointer to safearray
     */
    SAFEARRAY *(*pfnSafeArrayCreateVector)(VARTYPE vt, LONG lLbound, ULONG cElements);
    /**
     * Pre-allocate a safearray to be used by an out safearray parameter
     *
     * Must be freed by pfnSafeArrayDestroy.
     *
     * @returns pointer to safearray (system dependent, may be NULL if
     *    there is no need to pre-allocate a safearray)
     */
    SAFEARRAY *(*pfnSafeArrayOutParamAlloc)(void);
    /**
     * Copy a C array into a safearray (for passing as an input parameter)
     *
     * @param psa           pointer to already created safearray.
     * @param pv            pointer to memory block to copy into safearray.
     * @param cb            number of bytes to copy.
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnSafeArrayCopyInParamHelper)(SAFEARRAY *psa, const void *pv, ULONG cb);
    /**
     * Copy a safearray into a C array (for getting an output parameter)
     *
     * @param ppv           output pointer to newly created array, which has to
     *          be freed with pfnArrayOutFree.
     * @param pcb           number of bytes in the output buffer.
     * @param vt            variant type, defines the size of the elements
     * @param psa           pointer to safearray for getting the data
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnSafeArrayCopyOutParamHelper)(void **ppv, ULONG *pcb, VARTYPE vt, SAFEARRAY *psa);
    /**
     * Copy a safearray into a C array (special variant for interface pointers)
     *
     * @param ppaObj        output pointer to newly created array, which has
     *          to be freed with pfnArrayOutFree. Note that it's the caller's
     *          responsibility to call Release() on each non-NULL interface
     *          pointer before freeing.
     * @param pcObj         number of pointers in the output buffer.
     * @param psa           pointer to safearray for getting the data
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnSafeArrayCopyOutIfaceParamHelper)(IUnknown ***ppaObj, ULONG *pcObj, SAFEARRAY *psa);
    /**
     * Free a safearray
     *
     * @param psa           pointer to safearray
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnSafeArrayDestroy)(SAFEARRAY *psa);
    /**
     * Free an out array created by pfnSafeArrayCopyOutParamHelper or
     * pdnSafeArrayCopyOutIfaceParamHelper.
     *
     * @param psa           pointer to memory block
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnArrayOutFree)(void *pv);

#ifndef WIN32
    /**
     * Get XPCOM event queue. Deprecated!
     *
     * @param ppEventQueue      output parameter for nsIEventQueue reference,
     *              owned by C bindings.
     */
    void (*pfnGetEventQueue)(nsIEventQueue **ppEventQueue);
#endif /* !WIN32 */

    /**
     * Get current COM/XPCOM exception.
     *
     * @param ppException       output parameter for exception info reference,
     *              may be @c NULL if no exception object has been created by
     *              a previous COM/XPCOM call.
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnGetException)(IErrorInfo **ppException);
    /**
     * Clears current COM/XPCOM exception.
     *
     * @returns COM/XPCOM error code
     */
    HRESULT (*pfnClearException)(void);

    /**
     * Process the event queue for a given amount of time.
     *
     * Must be called on the primary thread. Typical timeouts are from 200 to
     * 5000 msecs, to allow for checking a volatile variable if the event queue
     * processing should be terminated (,
     * or 0 if only the pending events should be processed, without waiting.
     *
     * @param iTimeoutMS        how long to process the event queue, -1 means
     *              infinitely long
     * @returns status code
     * @retval 0 if at least one event has been processed
     * @retval 1 if any signal interrupted the native system call (or returned
     *      otherwise)
     * @retval 2 if the event queue was explicitly interrupted
     * @retval 3 if the timeout expired
     * @retval 4 if the function was called from the wrong thread
     * @retval 5 for all other (unexpected) errors
     */
    int (*pfnProcessEventQueue)(LONG64 iTimeoutMS);
    /**
     * Interrupt event queue processing.
     *
     * Can be called on any thread. Note that this function is not async-signal
     * safe, so never use it in such a context, instead use a volatile global
     * variable and a sensible timeout.
     * @returns 0 if successful, 1 otherwise.
     */
    int (*pfnInterruptEventQueueProcessing)(void);

    /** Tail version, same as uVersion. */
    unsigned uEndVersion;
} VBOXCAPI;
/** Pointer to a const VBOXCAPI function table. */
typedef VBOXCAPI const *PCVBOXCAPI;
#ifndef WIN32
/** Backwards compatibility: Pointer to a const VBOXCAPI function table.
 * Use PCVBOXCAPI instead. */
typedef VBOXCAPI const *PCVBOXXPCOM;
#endif /* !WIN32 */

#ifndef WIN32
/** Backwards compatibility: make sure old code using VBOXXPCOMC still compiles.
 * Use VBOXCAPI instead. */
#define VBOXXPCOMC VBOXCAPI
#endif /* !WIN32 */

/** The current interface version.
 * For use with VBoxGetCAPIFunctions and to be found in VBOXCAPI::uVersion. */
#define VBOX_CAPI_VERSION 0x00040000U

#ifndef WIN32
/** Backwards compatibility: The current interface version.
 * Use VBOX_CAPI_VERSION instead. */
#define VBOX_XPCOMC_VERSION VBOX_CAPI_VERSION
#endif /* !WIN32 */

/** VBoxGetCAPIFunctions. */
VBOXCAPI_DECL(PCVBOXCAPI) VBoxGetCAPIFunctions(unsigned uVersion);
#ifndef WIN32
/** Backwards compatibility: VBoxGetXPCOMCFunctions.
 * Use VBoxGetCAPIFunctions instead. */
VBOXCAPI_DECL(PCVBOXCAPI) VBoxGetXPCOMCFunctions(unsigned uVersion);
#endif /* !WIN32 */

/** Typedef for VBoxGetCAPIFunctions. */
typedef PCVBOXCAPI (*PFNVBOXGETCAPIFUNCTIONS)(unsigned uVersion);
#ifndef WIN32
/** Backwards compatibility: Typedef for VBoxGetXPCOMCFunctions.
 * Use PFNVBOXGETCAPIFUNCTIONS instead. */
typedef PCVBOXCAPI (*PFNVBOXGETXPCOMCFUNCTIONS)(unsigned uVersion);
#endif /* !WIN32 */

/** The symbol name of VBoxGetCAPIFunctions. */
#ifdef __OS2__
# define VBOX_GET_CAPI_FUNCTIONS_SYMBOL_NAME "_VBoxGetCAPIFunctions"
#else /* !__OS2__ */
# define VBOX_GET_CAPI_FUNCTIONS_SYMBOL_NAME "VBoxGetCAPIFunctions"
#endif /* !__OS2__ */
#ifndef WIN32
/** Backwards compatibility: The symbol name of VBoxGetXPCOMCFunctions.
 * Use VBOX_GET_CAPI_FUNCTIONS_SYMBOL_NAME instead. */
# ifdef __OS2__
#  define VBOX_GET_XPCOMC_FUNCTIONS_SYMBOL_NAME "_VBoxGetXPCOMCFunctions"
# else /* !__OS2__ */
#  define VBOX_GET_XPCOMC_FUNCTIONS_SYMBOL_NAME "VBoxGetXPCOMCFunctions"
# endif /* !__OS2__ */
#endif /* !WIN32 */


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !___VirtualBox_CAPI_h */

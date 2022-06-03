#ifndef QEMU_OSDEP_H
#define QEMU_OSDEP_H

#ifdef VBOX /** @todo clean up this, it's not fully synched. */

#include <iprt/alloc.h>
#ifndef RT_OS_WINDOWS
# include <iprt/alloca.h>
#endif
#include <iprt/stdarg.h>
#include <iprt/string.h>

#include "config.h"

#define VBOX_ONLY(x) x

#define qemu_snprintf(pszBuf, cbBuf, ...) RTStrPrintf((pszBuf), (cbBuf), __VA_ARGS__)
#define qemu_vsnprintf(pszBuf, cbBuf, pszFormat, args) \
                                RTStrPrintfV((pszBuf), (cbBuf), (pszFormat), (args))
#define qemu_vprintf(pszFormat, args) \
                                RTLogPrintfV((pszFormat), (args))
#define qemu_printf             RTLogPrintf
#define qemu_malloc(cb)         RTMemAlloc(cb)
#define qemu_mallocz(cb)        RTMemAllocZ(cb)
#define qemu_realloc(ptr, cb)   RTMemRealloc(ptr, cb)

#define qemu_free(pv)           RTMemFree(pv)
#define qemu_strdup(psz)        RTStrDup(psz)

#define qemu_vmalloc(cb)        RTMemPageAlloc(cb)
#define qemu_vfree(pv)          RTMemPageFree(pv, missing_size_parameter)

#ifndef NULL
# define NULL 0
#endif

#define fflush(file)            RTLogFlush(NULL)
#define printf(...)             LogIt(LOG_INSTANCE, 0, LOG_GROUP_REM_PRINTF, (__VA_ARGS__))
/* If DEBUG_TMP_LOGGING - goes to QEMU log file */
#ifndef DEBUG_TMP_LOGGING
# define fprintf(logfile, ...)  LogIt(LOG_INSTANCE, 0, LOG_GROUP_REM_PRINTF, (__VA_ARGS__))
#endif

#define assert(cond) Assert(cond)

#else /* !VBOX */

#include <stdarg.h>

#define VBOX_ONLY(x)

#define qemu_snprintf snprintf   /* bird */
#define qemu_vsnprintf vsnprintf /* bird */
#define qemu_vprintf vprintf     /* bird */

#define qemu_printf printf

void *qemu_malloc(size_t size);
void *qemu_mallocz(size_t size);
void qemu_free(void *ptr);
char *qemu_strdup(const char *str);

void *qemu_vmalloc(size_t size);
void qemu_vfree(void *ptr);

void *get_mmap_addr(unsigned long size);

#endif /* !VBOX */

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sys/signal.h>
#endif

#ifndef _WIN32
#include <sys/time.h>
#endif

#ifndef glue
#define xglue(x, y) x ## y
#define glue(x, y) xglue(x, y)
#define stringify(s)	tostring(s)
#define tostring(s)	#s
#endif

#ifndef likely
#if __GNUC__ < 3
#define __builtin_expect(x, n) (x)
#endif

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x)   __builtin_expect(!!(x), 0)
#endif

#ifndef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *) 0)->MEMBER)
#endif
#ifndef container_of
#define container_of(ptr, type, member) ({                      \
        const typeof(((type *) 0)->member) *__mptr = (ptr);     \
        (type *) ((char *) __mptr - offsetof(type, member));})
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

#ifndef always_inline
#if (__GNUC__ < 3) || defined(__APPLE__)
#define always_inline inline
#else
#define always_inline __attribute__ (( always_inline )) __inline__
#ifdef __OPTIMIZE__
#define inline always_inline
#endif
#endif
#else
#define inline always_inline
#endif

#ifdef __i386__
#define REGPARM __attribute((regparm(3)))
#else
#define REGPARM
#endif

#ifndef VBOX
#define qemu_printf printf
#endif

#if defined (__GNUC__) && defined (__GNUC_MINOR__)
# define QEMU_GNUC_PREREQ(maj, min) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
# define QEMU_GNUC_PREREQ(maj, min) 0
#endif

#ifndef VBOX
void *qemu_memalign(size_t alignment, size_t size);
void *qemu_vmalloc(size_t size);
void qemu_vfree(void *ptr);

int qemu_create_pidfile(const char *filename);

#ifdef _WIN32
int ffs(int i);

typedef struct {
    long tv_sec;
    long tv_usec;
} qemu_timeval;
int qemu_gettimeofday(qemu_timeval *tp);
#else
typedef struct timeval qemu_timeval;
#define qemu_gettimeofday(tp) gettimeofday(tp, NULL);
#endif /* !_WIN32 */
#else  /* VBOX */
# define qemu_memalign(alignment, size)  ( (alignment) <= PAGE_SIZE ? RTMemPageAlloc((size)) : NULL )
#endif /* VBOX */

#endif

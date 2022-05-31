/* $Id: the-freebsd-kernel.h 22819 2009-09-07 19:10:55Z vboxsync $ */
/** @file
 * IPRT - Ring-0 Driver, The FreeBSD Kernel Headers.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ___the_freebsd_kernel_h
#define ___the_freebsd_kernel_h

#include <iprt/types.h>

/* Deal with conflicts first. */
#include <sys/param.h>
#undef PVM
#include <sys/bus.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/kernel.h>
#include <sys/uio.h>
#include <sys/libkern.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/pcpu.h>
#include <sys/proc.h>
#include <sys/limits.h>
#include <sys/unistd.h>
#include <sys/kthread.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sched.h>
#include <sys/callout.h>
#include <sys/cpu.h>
#include <sys/smp.h>
#include <sys/sleepqueue.h>
#include <sys/sx.h>
#include <vm/vm.h>
#include <vm/pmap.h>            /* for vtophys */
#include <vm/vm_map.h>
#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_param.h>        /* KERN_SUCCESS ++ */
#include <vm/vm_page.h>
#include <sys/resourcevar.h>
#include <machine/cpu.h>

/*#ifdef __cplusplus
# error "This header doesn't work for C++ code. Sorry, typical kernel crap."
#endif*/

#ifdef __FreeBSD_version >= 800026
# define SLEEPQ_TIMEDWAIT(EventInt) sleepq_timedwait(EventInt, 0)
# define SLEEPQ_TIMEDWAIT_SIG(EventInt) sleepq_timedwait_sig(EventInt, 0)
# define SLEEPQ_WAIT(EventInt) sleepq_wait(EventInt, 0)
# define SLEEPQ_WAIT_SIG(EventInt) sleepq_wait_sig(EventInt, 0)
#elif
# define SLEEPQ_TIMEDWAIT(EventInt) sleepq_timedwait(EventInt)
# define SLEEPQ_TIMEDWAIT_SIG(EventInt) sleepq_timedwait_sig(EventInt)
# define SLEEPQ_WAIT(EventInt) sleepq_wait(EventInt)
# define SLEEPQ_WAIT_SIG(EventInt) sleepq_wait_sig(EventInt)
#endif

#endif

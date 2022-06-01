/** @file
 * IPRT - Lock Validator.
 */

/*
 * Copyright (C) 2009 Sun Microsystems, Inc.
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
 *
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 USA or visit http://www.sun.com if you need
 * additional information or have any questions.
 */

#ifndef ___iprt_lockvalidator_h
#define ___iprt_lockvalidator_h

#include <iprt/cdefs.h>
#include <iprt/types.h>
#include <iprt/assert.h>


/** @defgroup grp_ldr       RTLockValidator - Lock Validator
 * @ingroup grp_rt
 * @{
 */

RT_C_DECLS_BEGIN

/**
 * Record recording the ownership of a lock.
 *
 * This is typically part of the per-lock data structure when compiling with
 * the lock validator.
 */
typedef struct RTLOCKVALIDATORREC
{
    /** Magic value (RTLOCKVALIDATORREC_MAGIC). */
    uint32_t                            u32Magic;
    /** The line number in the file. */
    uint32_t volatile                   uLine;
    /** The file where the lock was taken. */
    R3R0PTRTYPE(const char * volatile)  pszFile;
    /** The function where the lock was taken. */
    R3R0PTRTYPE(const char * volatile)  pszFunction;
    /** Some ID indicating where the lock was taken, typically an address. */
    RTHCUINTPTR volatile                uId;
    /** The current owner thread. */
    RTTHREAD volatile                   hThread;
    /** Pointer to the lock record below us. Only accessed by the owner. */
    R3R0PTRTYPE(PRTLOCKVALIDATORREC)    pDown;
    /** The lock class. */
    RTLOCKVALIDATORCLASS                hClass;
    /** The lock sub-class. */
    uint32_t volatile                   uSubClass;
    /** Reserved for future use. */
    uint32_t                            u32Reserved;
    /** Pointer to the lock. */
    RTHCPTR                             hLock;
    /** The lock name. */
    R3R0PTRTYPE(const char *)           pszName;
} RTLOCKVALIDATORREC;
AssertCompileSize(RTLOCKVALIDATORREC, HC_ARCH_BITS == 32 ? 48 : 80);
/* The pointer is defined in iprt/types.h */


/** @name   Special sub-class values.
 * The range 16..UINT32_MAX is available to the user, the range 0..15 is
 * reserved for the lock validator.
 * @{ */
/** Not allowed to be taken with any other locks in the same class.
  * This is the recommended value.  */
#define RTLOCKVALIDATOR_SUB_CLASS_NONE  UINT32_C(0)
/** Any order is allowed within the class. */
#define RTLOCKVALIDATOR_SUB_CLASS_ANY   UINT32_C(1)
/** The first user value. */
#define RTLOCKVALIDATOR_SUB_CLASS_USER  UINT32_C(16)
/** @} */

/**
 * Initialize a lock validator record.
 *
 * Use RTLockValidatorDelete to deinitialize it.
 *
 * @param   pRec                The record.
 * @param   hClass              The class. If NIL, the no lock order
 *                              validation will be performed on this lock.
 * @param   uSubClass           The sub-class.  This is used to define lock
 *                              order inside the same class.  If you don't know,
 *                              then pass RTLOCKVALIDATOR_SUB_CLASS_NONE.
 * @param   pszName             The lock name (optional).
 * @param   hLock               The lock handle.
 */
RTDECL(void) RTLockValidatorInit(PRTLOCKVALIDATORREC pRec, RTLOCKVALIDATORCLASS hClass,
                                 uint32_t uSubClass, const char *pszName, void *hLock);
/**
 * Uninitialize a lock validator record previously initialized by
 * RTLockValidatorInit.
 *
 * @param   pRec                The record.  Must be valid.
 */
RTDECL(void) RTLockValidatorDelete(PRTLOCKVALIDATORREC pRec);

/**
 * Create and initialize a lock validator record.
 *
 * Use RTLockValidatorDestroy to deinitialize and destroy the returned record.
 *
 * @return VINF_SUCCESS or VERR_NO_MEMORY.
 * @param   ppRec               Where to return the record pointer.
 * @param   hClass              The class. If NIL, the no lock order
 *                              validation will be performed on this lock.
 * @param   uSubClass           The sub-class.  This is used to define lock
 *                              order inside the same class.  If you don't know,
 *                              then pass RTLOCKVALIDATOR_SUB_CLASS_NONE.
 * @param   pszName             The lock name (optional).
 * @param   hLock               The lock handle.
 */
RTDECL(int)  RTLockValidatorCreate(PRTLOCKVALIDATORREC *ppRec, RTLOCKVALIDATORCLASS hClass,
                                   uint32_t uSubClass, const char *pszName, void *hLock);

/**
 * Deinitialize and destroy a record created by RTLockValidatorCreate.
 *
 * @param   ppRec               Pointer to the record pointer.  Will be set to
 *                              NULL.
 */
RTDECL(void) RTLockValidatorDestroy(PRTLOCKVALIDATORREC *ppRec);

/**
 * Check the locking order.
 *
 * This is called by routines implementing lock acquisition.
 *
 * @retval  VINF_SUCCESS on success.
 * @retval  VERR_DEADLOCK if the order is wrong, after having whined and
 *          asserted.
 *
 * @param   pRec                The validator record.
 * @param   hThread             The handle of the calling thread.  If not known,
 *                              pass NIL_RTTHREAD and this method will figure it
 *                              out.
 * @param   uId                 Some kind of locking location ID.  Typically a
 *                              return address up the stack.  Optional (0).
 * @param   pszFile             The file where the lock is being acquired from.
 *                              Optional.
 * @param   iLine               The line number in that file.  Optional (0).
 * @param   pszFunction         The functionn where the lock is being acquired
 *                              from.  Optional.
 */
RTDECL(int)  RTLockValidatorCheckOrder(PRTLOCKVALIDATORREC pRec, RTTHREAD hThread, RTHCUINTPTR uId, RT_SRC_POS_DECL);


/**
 * Record the specified thread as lock owner.
 *
 * This is typically called after acquiring the lock.
 *
 * @returns hThread resolved.  Can return NIL_RTHREAD iff we fail to adopt the
 *          alien thread or if pRec is invalid.
 *
 * @param   pRec                The validator record.
 * @param   hThread             The handle of the calling thread.  If not known,
 *                              pass NIL_RTTHREAD and this method will figure it
 *                              out.
 * @param   uId                 Some kind of locking location ID.  Typically a
 *                              return address up the stack.  Optional (0).
 * @param   pszFile             The file where the lock is being acquired from.
 *                              Optional.
 * @param   iLine               The line number in that file.  Optional (0).
 * @param   pszFunction         The functionn where the lock is being acquired
 *                              from.  Optional.
 */
RTDECL(RTTHREAD) RTLockValidatorSetOwner(PRTLOCKVALIDATORREC pRec, RTTHREAD hThread, RTHCUINTPTR uId, RT_SRC_POS_DECL);


/**
 * Clear the lock ownership.
 *
 * This is typically called before release the lock.
 *
 * @returns The thread handle of the previous owner.  NIL_RTTHREAD if the record
 *          is invalid or didn't have any owner.
 * @param   pRec                The validator record.
 */
RTDECL(RTTHREAD) RTLockValidatorUnsetOwner(PRTLOCKVALIDATORREC pRec);


/*RTDECL(int) RTLockValidatorClassCreate();*/

RT_C_DECLS_END

/** @} */

#endif



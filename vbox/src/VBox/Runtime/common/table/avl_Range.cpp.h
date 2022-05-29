/* $Id: avl_Range.cpp.h 8155 2008-04-18 15:16:47Z vboxsync $ */
/** @file
 * kAVLRange  - Range routines for AVL trees.
 */

/*
 * Copyright (C) 1999-2006 knut st. osmundsen (bird-src-spam@anduin.net)
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

#ifndef _kAVLRange_h_
#define _kAVLRange_h_

/**
 * Finds the range containing the specified key.
 *
 * @returns   Pointer to the matching range.
 *
 * @param     ppTree  Pointer to Pointer to the tree root node.
 * @param     Key     The Key to find matching range for.
 */
RTDECL(PKAVLNODECORE) KAVL_FN(RangeGet)(PPKAVLNODECORE ppTree, register KAVLKEY Key)
{
    register PKAVLNODECORE  pNode = KAVL_GET_POINTER_NULL(ppTree);
    if (pNode)
    {
        for (;;)
        {
            if (KAVL_R_IS_IN_RANGE(pNode->Key, pNode->KeyLast, Key))
                return pNode;
            if (KAVL_G(pNode->Key, Key))
            {
                if (pNode->pLeft != KAVL_NULL)
                    pNode = KAVL_GET_POINTER(&pNode->pLeft);
                else
                    return NULL;
            }
            else
            {
                if (pNode->pRight != KAVL_NULL)
                    pNode = KAVL_GET_POINTER(&pNode->pRight);
                else
                    return NULL;
            }
        }
    }

    return NULL;
}


/**
 * Removes the range containing the specified key.
 *
 * @returns   Pointer to the matching range.
 *
 * @param     ppTree  Pointer to Pointer to the tree root node.
 * @param     Key     The Key to remove matching range for.
 */
RTDECL(PKAVLNODECORE) KAVL_FN(RangeRemove)(PPKAVLNODECORE ppTree, KAVLKEY Key)
{
    PKAVLNODECORE pNode = KAVL_FN(RangeGet)(ppTree, Key);
    if (pNode)
        return KAVL_FN(Remove)(ppTree, pNode->Key);
    return NULL;
}

#endif

/* $Id: avl_Destroy.cpp.h 65208 2017-01-09 14:39:54Z vboxsync $ */
/** @file
 * kAVLDestroy - Walk the tree calling a callback to destroy all the nodes.
 */

/*
 * Copyright (C) 1999-2011 knut st. osmundsen (bird-src-spam@anduin.net)
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

#ifndef _kAVLDestroy_h_
#define _kAVLDestroy_h_


/**
 * Destroys the specified tree, starting with the root node and working our way down.
 *
 * @returns 0 on success.
 * @returns Return value from callback on failure. On failure, the tree will be in
 *          an unbalanced condition and only further calls to the Destroy should be
 *          made on it. Note that the node we fail on will be considered dead and
 *          no action is taken to link it back into the tree.
 * @param   ppTree          Pointer to the AVL-tree root node pointer.
 * @param   pfnCallBack     Pointer to callback function.
 * @param   pvUser          User parameter passed on to the callback function.
 */
KAVL_DECL(int) KAVL_FN(Destroy)(PPKAVLNODECORE ppTree, PKAVLCALLBACK pfnCallBack, void *pvUser)
{
    unsigned        cEntries;
    PKAVLNODECORE   apEntries[KAVL_MAX_STACK];
    int             rc;

    if (*ppTree == KAVL_NULL)
        return VINF_SUCCESS;

    cEntries = 1;
    apEntries[0] = KAVL_GET_POINTER(ppTree);
    while (cEntries > 0)
    {
        /*
         * Process the subtrees first.
         */
        PKAVLNODECORE pNode = apEntries[cEntries - 1];
        if (pNode->pLeft != KAVL_NULL)
            apEntries[cEntries++] = KAVL_GET_POINTER(&pNode->pLeft);
        else if (pNode->pRight != KAVL_NULL)
            apEntries[cEntries++] = KAVL_GET_POINTER(&pNode->pRight);
        else
        {
#ifdef KAVL_EQUAL_ALLOWED
            /*
             * Process nodes with the same key.
             */
            while (pNode->pList != KAVL_NULL)
            {
                PKAVLNODECORE pEqual = KAVL_GET_POINTER(&pNode->pList);
                KAVL_SET_POINTER(&pNode->pList, KAVL_GET_POINTER_NULL(&pEqual->pList));
                pEqual->pList = KAVL_NULL;

                rc = pfnCallBack(pEqual, pvUser);
                if (rc != VINF_SUCCESS)
                    return rc;
            }
#endif

            /*
             * Unlink the node.
             */
            if (--cEntries > 0)
            {
                PKAVLNODECORE pParent = apEntries[cEntries - 1];
                if (KAVL_GET_POINTER(&pParent->pLeft) == pNode)
                    pParent->pLeft = KAVL_NULL;
                else
                    pParent->pRight = KAVL_NULL;
            }
            else
                *ppTree = KAVL_NULL;

            kASSERT(pNode->pLeft == KAVL_NULL);
            kASSERT(pNode->pRight == KAVL_NULL);
            rc = pfnCallBack(pNode, pvUser);
            if (rc != VINF_SUCCESS)
                return rc;
        }
    } /* while */

    kASSERT(*ppTree == KAVL_NULL);

    return VINF_SUCCESS;
}

#endif


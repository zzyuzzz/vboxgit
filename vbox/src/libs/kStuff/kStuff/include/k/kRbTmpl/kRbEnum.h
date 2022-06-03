/* $Id: kRbEnum.h 35 2009-11-08 19:39:03Z bird $ */
/** @file
 * kRbTmpl - Templated Red-Black Trees, Node Enumeration.
 */

/*
 * Copyright (c) 1999-2009 Knut St. Osmundsen <bird-kStuff-spamix@anduin.net>
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

/*******************************************************************************
*   Structures and Typedefs                                                    *
*******************************************************************************/
/**
 * Enumeration control data.
 *
 * This is initialized by BeginEnum and used by GetNext to figure out what
 * to do next.
 */
typedef struct KRB_TYPE(,ENUMDATA)
{
    KBOOL               fFromLeft;
    KI8                 cEntries;
    KU8                 achFlags[KRB_MAX_STACK];
    KRBNODE *           aEntries[KRB_MAX_STACK];
} KRB_TYPE(,ENUMDATA), *KRB_TYPE(P,ENUMDATA);


/**
 * Ends an enumeration.
 *
 * The purpose of this function is to unlock the tree should the Red-Black tree
 * implementation include locking.  It's good practice to call it anyway even if
 * the tree doesn't do any locking.
 *
 * @param   pEnumData   Pointer to enumeration control data.
 */
KRB_DECL(void) KRB_FN(EndEnum)(KRB_TYPE(,ENUMDATA) *pEnumData)
{
    KRBROOT pRoot = pEnumData->pRoot;
    pEnumData->pRoot = NULL;
    if (pRoot)
        KRB_READ_UNLOCK(pEnumData->pRoot);
}


/**
 * Get the next node in the tree enumeration.
 *
 * The current implementation of this function willl not walk the mpList
 * chain like the DoWithAll function does. This may be changed later.
 *
 * @returns Pointer to the next node in the tree.
 *          NULL is returned when the end of the tree has been reached,
 *          it is not necessary to call EndEnum in this case.
 * @param   pEnumData   Pointer to enumeration control data.
 */
KRB_DECL(KRBNODE *) KRB_FN(GetNext)(KRB_TYPE(,ENUMDATA) *pEnumData)
{
    if (pEnumData->fFromLeft)
    {   /* from left */
        while (pEnumData->cEntries > 0)
        {
            KRBNODE *pNode = pEnumData->aEntries[pEnumData->cEntries - 1];

            /* left */
            if (pEnumData->achFlags[pEnumData->cEntries - 1] == 0)
            {
                pEnumData->achFlags[pEnumData->cEntries - 1]++;
                if (pNode->mpLeft != KRB_NULL)
                {
                    pEnumData->achFlags[pEnumData->cEntries] = 0; /* 0 left, 1 center, 2 right */
                    pEnumData->aEntries[pEnumData->cEntries++] = KRB_GET_POINTER(&pNode->mpLeft);
                    continue;
                }
            }

            /* center */
            if (pEnumData->achFlags[pEnumData->cEntries - 1] == 1)
            {
                pEnumData->achFlags[pEnumData->cEntries - 1]++;
                return pNode;
            }

            /* right */
            pEnumData->cEntries--;
            if (pNode->mpRight != KRB_NULL)
            {
                pEnumData->achFlags[pEnumData->cEntries] = 0;
                pEnumData->aEntries[pEnumData->cEntries++] = KRB_GET_POINTER(&pNode->mpRight);
            }
        } /* while */
    }
    else
    {   /* from right */
        while (pEnumData->cEntries > 0)
        {
            KRBNODE *pNode = pEnumData->aEntries[pEnumData->cEntries - 1];

            /* right */
            if (pEnumData->achFlags[pEnumData->cEntries - 1] == 0)
            {
                pEnumData->achFlags[pEnumData->cEntries - 1]++;
                if (pNode->mpRight != KRB_NULL)
                {
                    pEnumData->achFlags[pEnumData->cEntries] = 0;  /* 0 right, 1 center, 2 left */
                    pEnumData->aEntries[pEnumData->cEntries++] = KRB_GET_POINTER(&pNode->mpRight);
                    continue;
                }
            }

            /* center */
            if (pEnumData->achFlags[pEnumData->cEntries - 1] == 1)
            {
                pEnumData->achFlags[pEnumData->cEntries - 1]++;
                return pNode;
            }

            /* left */
            pEnumData->cEntries--;
            if (pNode->mpLeft != KRB_NULL)
            {
                pEnumData->achFlags[pEnumData->cEntries] = 0;
                pEnumData->aEntries[pEnumData->cEntries++] = KRB_GET_POINTER(&pNode->mpLeft);
            }
        } /* while */
    }

    /*
     * Call EndEnum.
     */
    KRB_FN(EndEnum)(pEnumData);
    return NULL;
}


/**
 * Starts an enumeration of all nodes in the given tree.
 *
 * The current implementation of this function will not walk the mpList
 * chain like the DoWithAll function does. This may be changed later.
 *
 * @returns Pointer to the first node in the enumeration.
 *          If NULL is returned the tree is empty calling EndEnum isn't
 *          strictly necessary (although it will do no harm).
 * @param   pRoot           Pointer to the Red-Back tree's root structure.
 * @param   pEnumData       Pointer to enumeration control data.
 * @param   fFromLeft       K_TRUE:  Left to right.
 *                          K_FALSE: Right to left.
 */
KRB_DECL(KRBNODE *) KRB_FN(BeginEnum)(KRBROOT *pRoot, KRB_TYPE(,ENUMDATA) *pEnumData, KBOOL fFromLeft)
{
    KRB_READ_LOCK(pRoot);
    pEnumData->pRoot = pRoot;
    if (pRoot->mpRoot != KRB_NULL)
    {
        pEnumData->fFromLeft = fFromLeft;
        pEnumData->cEntries = 1;
        pEnumData->aEntries[0] = KRB_GET_POINTER(pRoot->mpRoot);
        pEnumData->achFlags[0] = 0;
    }
    else
        pEnumData->cEntries = 0;

    return KRB_FN(GetNext)(pEnumData);
}


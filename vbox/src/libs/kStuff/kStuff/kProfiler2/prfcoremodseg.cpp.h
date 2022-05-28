/* $Id: prfcoremodseg.cpp.h 2 2007-11-16 16:07:14Z bird $ */
/** @file
 * kProfiler Mark 2 - Core Module Segment Code Template.
 */

/*
 * Copyright (c) 2006-2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * This file is part of kProfiler.
 *
 * kProfiler is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * kProfiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with kProfiler; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


/**
 * Adds a module segment.
 *
 * @returns Offset to the module if existing or successfully added
 * @returns 0 if not found.
 *
 * @param   pHdr        The profiler header.
 * @param   pModSeg     Pointer to the module segment to insert (it's copied of course).
 * @param   off         The offset into the modseg area which has been searched.
 *                      (This is relative to the first moddule segment record (at pHdr->offModSegs).)
 */
static KU32 KPRF_NAME(InsertModSeg)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(PC,MODSEG) pModSeg, KU32 off)
{
    /*
     * Lookup the module segment, inserting it if not found (and there is room).
     */
    for (;;)
    {
        if (off >= pHdr->cbModSegs)
        {
            /*
             * It was the end, let's try insert it.
             *
             * This is where we lock the modseg stuff. The deal is that we
             * serialize the actual inserting without blocking lookups. This
             * means that we may end up with potential racing inserts, but
             * unless there is a large amount of modules being profiled that's
             * probably not going to be much of a problem. Anyway if we race,
             * we'll simply have to search the new additions before we add our
             * own stuff.
             */
            KPRF_MODSEGS_LOCK();
            if (off >= pHdr->cbModSegs)
            {
                KU32 cbModSeg = KPRF_OFFSETOF(MODSEG, szPath[pModSeg->cchPath + 1]);
                cbModSeg = KPRF_ALIGN(cbModSeg, KPRF_SIZEOF(UPTR));
                if (off + cbModSeg <= pHdr->cbMaxModSegs)
                {
                    KPRF_TYPE(P,MODSEG) pNew = KPRF_OFF2PTR(P,MODSEG, off + pHdr->offModSegs, pHdr);
                    pNew->uBasePtr          = pModSeg->uBasePtr;
                    pNew->cbSegmentMinusOne = pModSeg->cbSegmentMinusOne;
                    pNew->iSegment          = pModSeg->iSegment;
                    pNew->fLoaded           = pModSeg->fLoaded;
                    pNew->cchPath           = pModSeg->cchPath;

                    KI32 iPath = pModSeg->cchPath;
                    do  pNew->szPath[iPath] = pModSeg->szPath[iPath];
                    while (--iPath >= 0);

                    /* commit it */
                    KPRF_ATOMIC_SET32(&pHdr->cbModSegs, off + cbModSeg);
                    off += pHdr->offModSegs;
                }
                else
                    off = 0;
                KPRF_MODSEGS_UNLOCK();
                return off;
            }
            KPRF_MODSEGS_UNLOCK();
            /* someone raced us, check the new entries. */
        }

        /*
         * Match?
         */
        KPRF_TYPE(PC,MODSEG) pCur = KPRF_OFF2PTR(P,MODSEG, off + pHdr->offModSegs, pHdr);
        if (    pCur->uBasePtr          == pModSeg->uBasePtr
            &&  pCur->fLoaded           == pModSeg->fLoaded
            &&  pCur->cchPath           == pModSeg->cchPath
            &&  pCur->iSegment          == pModSeg->iSegment
            &&  pCur->cbSegmentMinusOne == pModSeg->cbSegmentMinusOne
           )
        {
            KI32 iPath = pModSeg->cchPath;
            for (;;)
            {
                if (!iPath--)
                    return off + pHdr->offModSegs;
                if (pModSeg->szPath[iPath] != pCur->szPath[iPath])
                    break;
            }
            /* didn't match, continue searching */
        }
        KU32 cbCur = KPRF_OFFSETOF(MODSEG, szPath[pCur->cchPath + 1]);
        off += KPRF_ALIGN(cbCur, KPRF_SIZEOF(UPTR));
    }
}


/**
 * Queries data for and inserts a new module segment.
 *
 *
 * @returns Offset to the module if existing or successfully added
 * @returns 0 if not found.
 *
 * @param   pHdr        The profiler header.
 * @param   uPC         Address within the module.
 * @param   off         The offset into the modseg area which has been searched.
 *                      (This is relative to the first moddule segment record (at pHdr->offModSegs).)
 */
static KU32 KPRF_NAME(NewModSeg)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(,UPTR) uPC, KU32 off)
{
    /*
     * Query the module name and object of the function.
     */
#pragma pack(1)
    struct
    {
        KPRF_TYPE(,MODSEG) ModSeg;
        char               szMorePath[260];
    } s;
#pragma pack()
    if (KPRF_GET_MODSEG(uPC + pHdr->uBasePtr, s.ModSeg.szPath, sizeof(s.ModSeg.szPath) + sizeof(s.szMorePath),
                        &s.ModSeg.iSegment, &s.ModSeg.uBasePtr, &s.ModSeg.cbSegmentMinusOne))
        return 0;
    s.ModSeg.uBasePtr -= pHdr->uBasePtr;
    s.ModSeg.fLoaded = 1;
    s.ModSeg.cchPath = 0;
    while (s.ModSeg.szPath[s.ModSeg.cchPath])
        s.ModSeg.cchPath++;

    return KPRF_NAME(InsertModSeg)(pHdr, &s.ModSeg, off);
}


/**
 * Record a module segment.
 *
 * This is an internal worker for recording a module segment when adding
 * a new function.
 *
 * @returns Offset to the module if existing or successfully added
 * @returns 0 if not found.
 *
 * @param   pHdr        The profiler header.
 * @param   uPC         Address within the module.
 */
static KU32 KPRF_NAME(RecordModSeg)(KPRF_TYPE(P,HDR) pHdr, KPRF_TYPE(,UPTR) uPC)
{
    /*
     * Lookup the module segment, inserting it if not found (and there is room).
     */
    KU32 off = 0;
    KPRF_TYPE(PC,MODSEG) pCur = KPRF_OFF2PTR(P,MODSEG, pHdr->offModSegs, pHdr);
    const KU32 cbModSegs = pHdr->cbModSegs;
    for (;;)
    {
        /* done and not found? */
        if (off >= cbModSegs)
            return KPRF_NAME(NewModSeg)(pHdr, uPC, off);

        /*
         * Match?
         */
        if (    pCur->fLoaded
            &&  uPC - pCur->uBasePtr <= pCur->cbSegmentMinusOne)
            return off + pHdr->offModSegs;

        KU32 cbCur = KPRF_OFFSETOF(MODSEG, szPath[pCur->cchPath + 1]);
        cbCur = KPRF_ALIGN(cbCur, KPRF_SIZEOF(UPTR));
        off += cbCur;
        pCur = (KPRF_TYPE(PC,MODSEG))((KU8 *)pCur + cbCur);
    }
}


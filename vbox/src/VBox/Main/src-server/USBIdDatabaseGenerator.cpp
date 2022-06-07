/* $Id: USBIdDatabaseGenerator.cpp 58019 2015-10-03 19:31:41Z vboxsync $ */
/** @file
 * USB device vendor and product ID database - generator.
 */

/*
 * Copyright (C) 2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#include <stdio.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <iprt/initterm.h>
#include <iprt/message.h>
#include <iprt/string.h>
#include <iprt/stream.h>
#include "../../Runtime/include/internal/strhash.h" /** @todo make this one public */

#include "../include/USBIdDatabase.h"


/** For verbose output.   */
static bool g_fVerbose = false;
/** Output prefix for informational output. */
#define INFO_PREF "USBIdDatabaseGenerator: Info: "


using namespace std;

static const char * const header =
    "/** @file\n"
    " * USB device vendor and product ID database - Autogenerated from <stupid C++ cannot do %s>\n"
    " */\n"
    "\n"
    "/*\n"
    " * Copyright (C) 2015 Oracle Corporation\n"
    " *\n"
    " * This file is part of VirtualBox Open Source Edition(OSE), as\n"
    " * available from http ://www.virtualbox.org. This file is free software;\n"
    " * you can redistribute it and / or modify it under the terms of the GNU\n"
    " * General Public License(GPL) as published by the Free Software\n"
    " * Foundation, in version 2 as it comes in the \"COPYING\" file of the\n"
    " * VirtualBox OSE distribution.VirtualBox OSE is distributed in the\n"
    " * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.\n"
    " */"
    "\n"
    "\n"
    "#include \"USBIdDatabase.h\"\n"
    "\n";
static const char * const product_header =
    "/**\n"
    " * USB devices aliases array.\n"
    " * Format: VendorId, ProductId, Vendor Name, Product Name\n"
    " * The source of the list is http://www.linux-usb.org/usb.ids\n"
    " */\n"
    "USBIDDBPROD const USBIdDatabase::s_aProducts[] =\n"
    "{\n";
const char *product_part2 =
    "};\n"
    "\n"
    "\nconst USBIDDBSTR USBIdDatabase::s_aProductNames[] =\n"
    "{\n";
const char *product_footer =
    "};\n"
    "\n"
    "const size_t USBIdDatabase::s_cProducts = RT_ELEMENTS(USBIdDatabase::s_aProducts);\n";

const char *vendor_header =
    "\nUSBIDDBVENDOR const USBIdDatabase::s_aVendors[] =\n"
    "{\n";
const char *vendor_part2 =
    "};\n"
    "\n"
    "\nconst USBIDDBSTR USBIdDatabase::s_aVendorNames[] =\n"
    "{\n";
const char *vendor_footer =
    "};\n"
    "\n"
    "const size_t USBIdDatabase::s_cVendors = RT_ELEMENTS(USBIdDatabase::s_aVendors);\n";

const char *start_block = "# Vendors, devices and interfaces. Please keep sorted.";
const char *end_block = "# List of known device classes, subclasses and protocols";


// error codes (complements RTEXITCODE_XXX).
#define ERROR_OPEN_FILE         (12)
#define ERROR_IN_PARSE_LINE     (13)
#define ERROR_DUPLICATE_ENTRY   (14)
#define ERROR_WRONG_FILE_FORMAT (15)
#define ERROR_TOO_MANY_PRODUCTS (16)


/**
 * String that will end up in the string table.
 */
struct StrTabString
{
    /** The string. */
    std::string str;
    /** The string hash value. */
    uint32_t    uHash;
    /** The string table reference. */
    USBIDDBSTR  StrRef;
    /** Pointer to the next string reference (same string table entry). */
    struct StrTabString *pNextRef;
    /** Pointer to the next string with the same hash value (collision). */
    struct StrTabString *pNextCollision;

    void printRef(ostream &rStrm) const
    {
        rStrm << "    { 0x" << setfill('0') << setw(6) << hex << StrRef.off << ", 0x"
              << setfill('0') << setw(2) << hex << StrRef.cch << " }, ";
    }

    void printRefLine(ostream &rStrm) const
    {
        printRef(rStrm);
        rStrm << endl;
    }
};
typedef struct StrTabString *PSTRTABSTRING;

struct VendorRecord
{
    size_t vendorID;
    size_t iProduct;
    size_t cProducts;
    StrTabString vendor;
};

struct ProductRecord
{
    size_t key;
    size_t vendorID;
    size_t productID;
    StrTabString product;
};

bool operator < (const ProductRecord& lh, const ProductRecord& rh)
{
    return lh.key < rh.key;
}

bool operator < (const VendorRecord& lh, const VendorRecord& rh)
{
    return lh.vendorID < rh.vendorID;
}

bool operator == (const ProductRecord& lh, const ProductRecord& rh)
{
    return lh.key == rh.key;
}

bool operator == (const VendorRecord& lh, const VendorRecord& rh)
{
    return lh.vendorID == rh.vendorID;
}

ostream& operator <<(ostream& stream, const ProductRecord product)
{
    stream << "    { 0x" << setfill('0') << setw(4) << product.productID << " }, " << endl;
    return stream;
}

ostream& operator <<(ostream& stream, const VendorRecord vendor)
{
    stream << "    { 0x" << setfill('0') << setw(4) << hex << vendor.vendorID
           << ", 0x"  << setfill('0') << setw(4) << hex << vendor.iProduct
           << ", 0x"  << setfill('0') << setw(4) << hex << vendor.cProducts << " }, " << endl;
    return stream;
}

namespace State
{
    typedef int Value;
    enum
    {
        lookForStartBlock,
        lookForEndBlock,
        finished
    };
}

typedef vector<ProductRecord> ProductsSet;
typedef vector<VendorRecord> VendorsSet;
ProductsSet g_products;
VendorsSet g_vendors;



/*
 * String "compression".  We replace the 127 most used words with references.
 */
#ifdef USB_ID_DATABASE_WITH_COMPRESSION

typedef std::map<std::string, size_t> WORDFREQMAP;
typedef WORDFREQMAP::value_type WORDFREQPAIR;

/** The 127 words we've picked to be indexed by reference.  */
static StrTabString g_aCompDict[127];

/**
 * For sorting the frequency fidning in descending order.
 *
 * Comparison operators are put outside to make older gcc versions (like 4.1.1
 * on lnx64-rel) happy.
 */
class WordFreqSortEntry
{
public:
    WORDFREQPAIR const *m_pPair;

public:
    WordFreqSortEntry(WORDFREQPAIR const *pPair) : m_pPair(pPair) {}
};

bool operator == (WordFreqSortEntry const &rLeft, WordFreqSortEntry const &rRight)
{
    return rLeft.m_pPair->second == rRight.m_pPair->second;
}

bool operator <  (WordFreqSortEntry const &rLeft, WordFreqSortEntry const &rRight)
{
    return rLeft.m_pPair->second >  rRight.m_pPair->second;
}


/**
 * Replaces the dictionary words and escapes non-ascii chars in a string.
 *
 * @param   pString     The string to fixup.
 * @param   pcchOld     The old string length is added to this (stats)
 * @param   pcchNew     The new string length is added to this (stats)
 */
static void FixupString(std::string *pString, size_t *pcchOld, size_t *pcchNew)
{
    char        szNew[USB_ID_DATABASE_MAX_STRING * 2];
    char       *pszDst = szNew;
    const char *pszSrc = pString->c_str();
    const char *pszSrcEnd = strchr(pszSrc, '\0');

    *pcchOld += pszSrcEnd - pszSrc;

    char ch;
    while ((ch = *pszSrc) != '\0')
    {
        /* Spaces. */
        while (ch == ' ')
        {
            *pszDst++ = ' ';
            ch = *++pszSrc;
        }
        if (!ch)
            break;

        /* Find the end of the current word. */
        size_t cchWord = 1;
        while ((ch = pszSrc[cchWord]) != ' ' && ch != '\0')
            cchWord++;

        /* Check for g_aWord matches. */
        size_t cchMax = pszSrcEnd - pszSrc;
        for (unsigned i = 0; i < RT_ELEMENTS(g_aCompDict); i++)
        {
            size_t cchLen = g_aCompDict[i].str.length();
            if (   cchLen >= cchWord
                && cchLen <= cchMax
                && g_aCompDict[i].str.compare(0, cchLen, pszSrc, cchLen) == 0)
            {
                *pszDst++ = (unsigned char)(0x80 | i);
                pszSrc += cchLen;
                cchWord = 0;
                break;
            }
        }

        if (cchWord)
        {
            /* Copy the current word. */
            ch = *pszSrc;
            do
            {
                if (!((unsigned char)ch & 0x80))
                {
                    *pszDst++ = ch;
                    pszSrc++;
                }
                else
                {
                    RTUNICP uc;
                    int rc = RTStrGetCpEx(&pszSrc, &uc);
                    if (RT_SUCCESS(rc))
                    {
                        *pszDst++ = (unsigned char)0xff; /* escape single code point. */
                        pszDst = RTStrPutCp(pszDst, uc);
                    }
                    else
                    {
                        cerr << "Error: RTStrGetCpEx failed with rc=" << rc << endl;
                        exit(3);
                    }
                }
            } while ((ch = *pszSrc) != '\0' && ch != ' ');
        }
    }
    *pszDst = '\0';
    *pcchNew += pszDst - &szNew[0];

    *pString = szNew;
}


/**
 * Analyzes a string.
 *
 * @param   pFrequencies    The word frequency map.
 * @param   rString         The string to analyze.
 */
static void AnalyzeString(WORDFREQMAP *pFrequencies, std::string const &rString)
{
    const char *psz = rString.c_str();

    /*
     * For now we just consider words.
     */
    char ch;
    while ((ch = *psz) != '\0')
    {
        /* Skip leading spaces. */
        while (ch == ' ')
            ch = *++psz;
        if (!ch)
            return;

        /* Find end of word. */
        size_t cchWord = 1;
        while ((ch = psz[cchWord]) != ' ' && ch != '\0')
            cchWord++;
        if (cchWord > 1)
        {
            std::string strWord(psz, cchWord);
            WORDFREQMAP::iterator it = pFrequencies->find(strWord);
            if (it != pFrequencies->end())
                it->second += cchWord - 1;
            else
                (*pFrequencies)[strWord] = 0;
            /** @todo could gain hits by including the space after the word, but that
             *        has the same accounting problems as the two words scenario below. */

# if 0 /** @todo need better accounting for overlapping alternatives before this can be enabled. */
            /* Two words - immediate yields calc may lie when this enabled and we may pick the wrong words. */
            if (ch == ' ')
            {
                ch = psz[++cchWord];
                if (ch != ' ' && ch != '\0')
                {
                    size_t const cchSaved = cchWord;

                    do
                        cchWord++;
                    while ((ch = psz[cchWord]) != ' ' && ch != '\0');

                    strWord = std::string(psz, cchWord);
                    WORDFREQMAP::iterator it = pFrequencies->find(strWord);
                    if (it != pFrequencies->end())
                        it->second += cchWord - 1;
                    else
                        (*pFrequencies)[strWord] = 0;

                    cchWord = cchSaved;
                }
            }
# endif
        }

        /* Advance. */
        psz += cchWord;
    }
}


/**
 * Compresses the vendor and product strings.
 *
 * This is very very simple (a lot less work that the string table for
 * instance).
 */
static void DoStringCompression(void)
{
    /*
     * Analyze the strings collecting stats on potential sequences to replace.
     */
    WORDFREQMAP Frequencies;

    uint32_t    cProducts = 0;
    for (ProductsSet::iterator it = g_products.begin(); it != g_products.end(); ++it, cProducts++)
        AnalyzeString(&Frequencies, it->product.str);

    uint32_t    cVendors = 0;
    for (VendorsSet::iterator it = g_vendors.begin(); it != g_vendors.end(); ++it, cVendors++)
        AnalyzeString(&Frequencies, it->vendor.str);

    if (g_fVerbose)
    {
        size_t const cbVendorEntry = sizeof(USBIdDatabase::s_aVendors[0]) + sizeof(USBIdDatabase::s_aVendorNames[0]);
        size_t const cbVendors = cVendors * cbVendorEntry;
        cout << INFO_PREF << cVendors  << " vendors (" << cbVendors << " bytes)" << endl;

        size_t const cbProductEntry = sizeof(USBIdDatabase::s_aProducts[0]) + sizeof(USBIdDatabase::s_aProductNames[0]);
        size_t const cbProducts = cProducts * cbProductEntry;
        cout << INFO_PREF << cProducts << " products (" << cbProducts << " bytes)" << endl;
    }

    /*
     * Sort the result and pick the top 127 ones.
     */
    std::vector<WordFreqSortEntry> SortMap;
    for (WORDFREQMAP::iterator it = Frequencies.begin(); it != Frequencies.end(); ++it)
    {
        WORDFREQPAIR const &rPair = *it;
        SortMap.push_back(WordFreqSortEntry(&rPair));
    }

    sort(SortMap.begin(), SortMap.end());

    size_t   cb = 0;
    unsigned i  = 0;
    for (std::vector<WordFreqSortEntry>::iterator it = SortMap.begin();
         it != SortMap.end() && i < RT_ELEMENTS(g_aCompDict);
         ++it, i++)
    {
        g_aCompDict[i].str = it->m_pPair->first;
        cb += it->m_pPair->second;
    }

    if (g_fVerbose)
        cout << INFO_PREF "Estimated compression saving " << cb << " bytes" << endl;

    /*
     * Rework the strings.
     */
    size_t cchNew = 0;
    size_t cchOld = 0;
    for (ProductsSet::iterator it = g_products.begin(); it != g_products.end(); ++it)
        FixupString(&it->product.str, &cchOld, &cchNew);
    for (VendorsSet::iterator it = g_vendors.begin(); it != g_vendors.end(); ++it)
        FixupString(&it->vendor.str, &cchOld, &cchNew);

    for (i = 0; i < RT_ELEMENTS(g_aCompDict); i++)
        cchNew += g_aCompDict[i].str.length() + 1;

    if (g_fVerbose)
    {
        cout << INFO_PREF "Strings: original: " << cchOld << " bytes;  compressed: " << cchNew << " bytes;";
        if (cchNew < cchOld)
            cout << "  saving " << (cchOld - cchNew) << " bytes (" << ((cchOld - cchNew) * 100 / cchOld) << "%)" << endl;
        else
            cout << "  wasting " << (cchOld - cchNew) << " bytes!" << endl;
        cout << INFO_PREF "Average string length is " << (cchOld / (cVendors + cProducts)) << endl;
    }
}


/**
 * Writes the compression dictionary to the output stream.
 *
 * @param   rStrm   The output stream.
 */
static void WriteCompressionDictionary(ostream &rStrm)
{
    rStrm << "const USBIDDBSTR USBIdDatabase::s_aCompDict[" << dec << RT_ELEMENTS(g_aCompDict) << "] = " << endl;
    rStrm << "{" << endl;
    for (unsigned i = 0; i < RT_ELEMENTS(g_aCompDict); i++)
    {
        g_aCompDict[i].printRef(rStrm);
        rStrm << " // " << g_aCompDict[i].str << endl;
    }
    rStrm << "};" << endl << endl;
}

#endif /* USB_ID_DATABASE_WITH_COMPRESSION */


/*
 * Compile a string table.
 */

/** The size of g_papStrHash. */
static size_t           g_cStrHash = 0;
/** String hash table. */
static PSTRTABSTRING   *g_papStrHash = NULL;
/** Duplicate strings found by AddString. */
static size_t           g_cDuplicateStrings = 0;
/** Total length of the unique strings (no terminators). */
static size_t           g_cchUniqueStrings = 0;
/** Number of unique strings after AddString. */
static size_t           g_cUniqueStrings = 0;
/** Number of collisions. */
static size_t           g_cCollisions = 0;

/** Number of entries in g_apSortedStrings. */
static size_t           g_cSortedStrings = 0;
/** The sorted string table. */
static PSTRTABSTRING   *g_papSortedStrings = NULL;

/** The string table. */
static char            *g_pachStrTab = NULL;
/** The actual string table size. */
static size_t           g_cchStrTab = 0;


/**
 * Adds a string to the hash table.
 * @param   pStr    The string.
 */
static void AddString(PSTRTABSTRING pStr)
{
    pStr->pNextRef       = NULL;
    pStr->pNextCollision = NULL;
    pStr->StrRef.off     = 0;
    pStr->StrRef.cch     = pStr->str.length();
    size_t cchIgnored;
    pStr->uHash          = sdbm(pStr->str.c_str(), &cchIgnored);
    Assert(cchIgnored == pStr->str.length());

    size_t idxHash = pStr->uHash % g_cStrHash;
    PSTRTABSTRING pCur = g_papStrHash[idxHash];
    if (!pCur)
        g_papStrHash[idxHash] = pStr;
    else
    {
        /* Look for matching string. */
        do
        {
            if (   pCur->uHash      == pStr->uHash
                && pCur->StrRef.cch == pStr->StrRef.cch
                && pCur->str        == pStr->str)
            {
                pStr->pNextRef = pCur->pNextRef;
                pCur->pNextRef = pStr;
                g_cDuplicateStrings++;
                return;
            }
            pCur = pCur->pNextCollision;
        } while (pCur != NULL);

        /* No matching string, insert. */
        g_cCollisions++;
        pStr->pNextCollision = g_papStrHash[idxHash];
        g_papStrHash[idxHash] = pStr;
    }

    g_cUniqueStrings++;
    g_cchUniqueStrings += pStr->StrRef.cch;
}


/**
 * Inserts a string into g_apUniqueStrings.
 * @param   pStr    The string.
 */
static void InsertUniqueString(PSTRTABSTRING pStr)
{
    size_t iIdx;
    size_t iEnd = g_cSortedStrings;
    if (iEnd)
    {
        size_t iStart = 0;
        for (;;)
        {
            iIdx = iStart + (iEnd - iStart) / 2;
            if (g_papSortedStrings[iIdx]->StrRef.cch < pStr->StrRef.cch)
            {
                if (iIdx <= iStart)
                    break;
                iEnd = iIdx;
            }
            else if (g_papSortedStrings[iIdx]->StrRef.cch > pStr->StrRef.cch)
            {
                if (++iIdx >= iEnd)
                    break;
                iStart = iIdx;
            }
            else
                break;
        }

        if (iIdx != g_cSortedStrings)
            memmove(&g_papSortedStrings[iIdx + 1], &g_papSortedStrings[iIdx],
                    (g_cSortedStrings - iIdx) * sizeof(g_papSortedStrings[iIdx]));
    }
    else
        iIdx = 0;

    g_papSortedStrings[iIdx] = pStr;
    g_cSortedStrings++;
}


/**
 * Creates a string table.
 *
 * This will save space by dropping string terminators, eliminating duplicates
 * and try find strings that are sub-strings of others.
 *
 * Will initialize the StrRef of all StrTabString instances.
 */
static void CreateStringTable(void)
{
    /*
     * Allocate a hash table double the size of all strings (to avoid too
     * many collisions).  Add all strings to it, finding duplicates in the
     * process.
     */
    size_t cMaxStrings = g_products.size() + g_vendors.size();
#ifdef USB_ID_DATABASE_WITH_COMPRESSION
    cMaxStrings += RT_ELEMENTS(g_aCompDict);
#endif
    cMaxStrings *= 2;
    g_papStrHash = new PSTRTABSTRING[cMaxStrings];
    g_cStrHash   = cMaxStrings;
    memset(g_papStrHash, 0, cMaxStrings * sizeof(g_papStrHash[0]));

    for (ProductsSet::iterator it = g_products.begin(); it != g_products.end(); ++it)
        AddString(&it->product);
    for (VendorsSet::iterator it = g_vendors.begin(); it != g_vendors.end(); ++it)
        AddString(&it->vendor);
#ifdef USB_ID_DATABASE_WITH_COMPRESSION
    for (unsigned i = 0; i < RT_ELEMENTS(g_aCompDict); i++)
        AddString(&g_aCompDict[i]);
#endif
    if (g_fVerbose)
        cout << INFO_PREF "" << g_cUniqueStrings << " unique string (" << g_cchUniqueStrings << " bytes), "
             << g_cDuplicateStrings << " duplicates, " << g_cCollisions << " collisions" << endl;

    /*
     * Create g_papSortedStrings from the hash table.  The table is sorted by
     * string length, with the longer strings first.
     */
    g_papSortedStrings = new PSTRTABSTRING[g_cUniqueStrings];
    g_cSortedStrings   = 0;
    size_t idxHash = g_cStrHash;
    while (idxHash-- > 0)
    {
        PSTRTABSTRING pCur = g_papStrHash[idxHash];
        if (pCur)
        {
            do
            {
                InsertUniqueString(pCur);
                pCur = pCur->pNextCollision;
            } while (pCur);
        }
    }

    /*
     * Create the actual string table.
     */
    g_pachStrTab = new char [g_cchUniqueStrings + 1];
    g_cchStrTab  = 0;
    for (size_t i = 0; i < g_cSortedStrings; i++)
    {
        PSTRTABSTRING       pCur      = g_papSortedStrings[i];
        const char * const  pszCur    = pCur->str.c_str();
        size_t       const  cchCur    = pCur->StrRef.cch;
        size_t              offStrTab = g_cchStrTab;

        /*
         * See if the string is a substring already found in the string table.
         * Excluding the zero terminator increases the chances for this.
         */
        size_t      cchLeft   = g_cchStrTab >= cchCur ? g_cchStrTab - cchCur : 0;
        const char *pchLeft   = g_pachStrTab;
        char const  chFirst   = *pszCur;
        while (cchLeft > 0)
        {
            const char *pchCandidate = (const char *)memchr(pchLeft, chFirst, cchLeft);
            if (!pchCandidate)
                break;
            if (memcmp(pchCandidate, pszCur,  cchCur) == 0)
            {
                offStrTab = pchCandidate - g_pachStrTab;
                break;
            }

            cchLeft -= pchCandidate + 1 - pchLeft;
            pchLeft  = pchCandidate + 1;
        }

        if (offStrTab == g_cchStrTab)
        {
            /*
             * See if the start of the string overlaps the end of the string table.
             */
            if (g_cchStrTab && cchCur > 1)
            {
                cchLeft = RT_MIN(g_cchStrTab, cchCur - 1);
                pchLeft = &g_pachStrTab[g_cchStrTab - cchLeft];
                while (cchLeft > 0)
                {
                    const char *pchCandidate = (const char *)memchr(pchLeft, chFirst, cchLeft);
                    if (!pchCandidate)
                        break;
                    cchLeft -= pchCandidate - pchLeft;
                    pchLeft  = pchCandidate;
                    if (memcmp(pchLeft, pszCur, cchLeft) == 0)
                    {
                        size_t cchToCopy = cchCur - cchLeft;
                        memcpy(&g_pachStrTab[offStrTab], &pszCur[cchLeft], cchToCopy);
                        g_cchStrTab += cchToCopy;
                        offStrTab = pchCandidate - g_pachStrTab;
                        break;
                    }
                    cchLeft--;
                    pchLeft++;
                }
            }

            /*
             * If we didn't have any luck above, just append the string.
             */
            if (offStrTab == g_cchStrTab)
            {
                memcpy(&g_pachStrTab[offStrTab], pszCur, cchCur);
                g_cchStrTab += cchCur;
            }
        }

        /*
         * Set the string table offset for all the references to this string.
         */
        do
        {
            pCur->StrRef.off = (uint32_t)offStrTab;
            pCur = pCur->pNextRef;
        } while (pCur != NULL);
    }

    if (g_fVerbose)
        cout << INFO_PREF "String table: " << g_cchStrTab << " bytes" << endl;
}


#ifdef VBOX_STRICT
/**
 * Sanity checks a string table string.
 * @param   pStr    The string to check.
 */
static void CheckStrTabString(PSTRTABSTRING pStr)
{
    AssertFailed();
    Assert(pStr->StrRef.cch == pStr->str.length());
    Assert(pStr->StrRef.off < g_cchStrTab);
    Assert(pStr->StrRef.off + pStr->StrRef.cch <= g_cchStrTab);
    Assert(memcmp(pStr->str.c_str(), &g_pachStrTab[pStr->StrRef.off], pStr->str.length()) == 0);
}
#endif


/**
 * Writes the string table code to the output stream.
 *
 * @param   rStrm   The output stream.
 */
static void WriteStringTable(ostream &rStrm)
{
#ifdef VBOX_STRICT
    /*
     * Do some quick sanity checks while we're here.
     */
    for (ProductsSet::iterator it = g_products.begin(); it != g_products.end(); ++it)
        CheckStrTabString(&it->product);
    for (VendorsSet::iterator it = g_vendors.begin(); it != g_vendors.end(); ++it)
        CheckStrTabString(&it->vendor);
# ifdef USB_ID_DATABASE_WITH_COMPRESSION
    for (unsigned i = 0; i < RT_ELEMENTS(g_aCompDict); i++)
        CheckStrTabString(&g_aCompDict[i]);
# endif
#endif

    /*
     * Create a table for speeding up the character categorization.
     */
    uint8_t abCharCat[256];
    RT_ZERO(abCharCat);
    abCharCat[(unsigned char)'\\'] = 1;
    abCharCat[(unsigned char)'\''] = 1;
    for (unsigned i = 0; i < 0x20; i++)
        abCharCat[i] = 2;
    for (unsigned i = 0x7f; i < 0x100; i++)
        abCharCat[i] = 2;

    /*
     * We follow the sorted string table, one string per line.
     */
    rStrm << endl;
    rStrm << "const size_t USBIdDatabase::s_cchStrTab   = " <<  g_cchStrTab << ";" << endl;
    rStrm << "const char   USBIdDatabase::s_achStrTab[] =" << endl;
    rStrm << "{" << endl;

    uint32_t off = 0;
    for (uint32_t i = 0; i < g_cSortedStrings; i++)
    {
        PSTRTABSTRING pCur = g_papSortedStrings[i];
        uint32_t      offEnd = pCur->StrRef.off + pCur->StrRef.cch;
        if (offEnd > off)
        {
            /* Comment with a more readable version of the string. */
            if (off == pCur->StrRef.off)
                rStrm << " /* 0x";
            else
                rStrm << " /* 0X";
            rStrm << hex << setfill('0') << setw(5) << off << " = \"";
            for (uint32_t offTmp = off; offTmp < offEnd; offTmp++)
            {
                unsigned char uch = g_pachStrTab[offTmp];
                if (abCharCat[uch] == 0)
                    rStrm << (char)uch;
                else if (abCharCat[uch] != 1)
                    rStrm << "\\x" << setw(2) << hex << (size_t)uch;
                else
                    rStrm << "\\" << (char)uch;
            }
            rStrm << "\" */" << endl;

            /* Must use char by char here or we may trigger the max string
               length limit in the compiler, */
            rStrm << "    ";
            for (; off < offEnd; off++)
            {
                unsigned char uch = g_pachStrTab[off];
                rStrm << "'";
                if (abCharCat[uch] == 0)
                    rStrm << (char)uch;
                else if (abCharCat[uch] != 1)
                    rStrm << "\\x" << setw(2) << hex << (size_t)uch;
                else
                    rStrm << "\\" << (char)uch;
                rStrm << "',";
            }
            rStrm << endl;
        }
    }

    rStrm << "};" << endl;
    rStrm << "AssertCompile(sizeof(USBIdDatabase::s_achStrTab) == 0x" << hex << g_cchStrTab << ");" << endl << endl;
}


/*
 * Input file parsing.
 */

/** The size of all the raw strings, including terminators. */
static size_t g_cbRawStrings = 0;

int ParseAlias(const string& src, size_t& id, string& desc)
{
    unsigned int i = 0;
    if (sscanf(src.c_str(), "%x", &i) != 1)
        return ERROR_IN_PARSE_LINE;

    /* skip the number and following whitespace. */
    size_t offNext = src.find_first_of(" \t", 1);
    offNext = src.find_first_not_of(" \t", offNext);
    if (offNext != string::npos)
    {
        size_t cchLength = src.length() - offNext;
        if (cchLength <= USB_ID_DATABASE_MAX_STRING)
        {
            id = i;
            desc = src.substr(offNext);

            /* Check the string encoding. */
            int rc = RTStrValidateEncoding(desc.c_str());
            if (RT_SUCCESS(rc))
            {
                g_cbRawStrings += desc.length() + 1;
                return RTEXITCODE_SUCCESS;
            }

            cerr << "Error: Invalid encoding: '" << desc << "' (rc=" << rc << ")" << endl;
        }
        cerr << "Error: String to long (" << cchLength << ")" << endl;
    }
    else
        cerr << "Error: Error parsing \"" << src << "\"" << endl;
    return ERROR_IN_PARSE_LINE;
}

bool IsCommentOrEmptyLine(const string& str)
{
    size_t index = str.find_first_not_of(" \t");// skip left spaces
    return index == string::npos || str[index] == '#';
}

bool getline(PRTSTREAM instream, string& resString)
{
    const size_t szBuf = 4096;
    char buf[szBuf] = { 0 };

    int rc = RTStrmGetLine(instream, buf, szBuf);
    if (RT_SUCCESS(rc))
    {
        resString = buf;
        return true;
    }
    else if (rc != VERR_EOF)
    {
        cerr << "Warning: Invalid line in file. Error: " << rc << endl;
    }
    return false;
}

int ParseUsbIds(PRTSTREAM instream)
{
    State::Value state = State::lookForStartBlock;
    string line;
    int res = 0;
    VendorRecord vendor = { 0, 0, 0, "" };

    while (state != State::finished && getline(instream, line))
    {
        switch (state)
        {
            case State::lookForStartBlock:
            {
                if (line.find(start_block) != string::npos)
                    state = State::lookForEndBlock;
                break;
            }
            case State::lookForEndBlock:
            {
                if (line.find(end_block) != string::npos)
                    state = State::finished;
                else
                {
                    if (!IsCommentOrEmptyLine(line))
                    {
                        if (line[0] == '\t')
                        {
                            // Parse Product line
                            // first line should be vendor
                            if (vendor.vendorID == 0)
                            {
                                cerr << "Wrong file format. Product before vendor: " << line.c_str() << "'" << endl;
                                return ERROR_WRONG_FILE_FORMAT;
                            }
                            ProductRecord product = { 0, vendor.vendorID, 0, "" };
                            if (ParseAlias(line.substr(1), product.productID, product.product.str) != 0)
                            {
                                cerr << "Error in parsing product line: '" << line.c_str() << "'" << endl;
                                return ERROR_IN_PARSE_LINE;
                            }
                            product.key = RT_MAKE_U32(product.productID, product.vendorID);
                            Assert(product.vendorID == vendor.vendorID);
                            g_products.push_back(product);
                        }
                        else
                        {
                            // Parse vendor line
                            if (ParseAlias(line, vendor.vendorID, vendor.vendor.str) != 0)
                            {
                                cerr << "Error in parsing vendor line: '"
                                    << line.c_str() << "'" << endl;
                                return ERROR_IN_PARSE_LINE;
                            }
                            g_vendors.push_back(vendor);
                        }
                    }
                }
                break;
            }
        }
    }
    if (state == State::lookForStartBlock)
    {
        cerr << "Error: wrong format of input file. Start line is not found." << endl;
        return ERROR_WRONG_FILE_FORMAT;
    }
    return 0;
}


static int usage(ostream &rOut, const char *argv0)
{
    rOut << "Usage: " << argv0
         << " [linux.org usb list file] [custom usb list file] [-o output file]" << endl;
    return RTEXITCODE_SYNTAX;
}

int main(int argc, char *argv[])
{
    /*
     * Initialize IPRT and convert argv to UTF-8.
     */
    int rc = RTR3InitExe(argc, &argv, 0);
    if (RT_FAILURE(rc))
        return RTMsgInitFailure(rc);

    /*
     * Parse arguments and read input files.
     */
    if (argc < 4)
    {
        usage(cerr, argv[0]);
        cerr << "Error: Not enough arguments." << endl;
        return RTEXITCODE_SYNTAX;
    }
    ofstream fout;
    PRTSTREAM fin;
    g_products.reserve(20000);
    g_vendors.reserve(3500);

    const char *outName = NULL;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0)
        {
            outName = argv[++i];
            continue;
        }
        if (   strcmp(argv[i], "-h") == 0
            || strcmp(argv[i], "-?") == 0
            || strcmp(argv[i], "--help") == 0)
        {
            usage(cout, argv[0]);
            return RTEXITCODE_SUCCESS;
        }

        rc = RTStrmOpen(argv[i], "r", &fin);
        if (RT_FAILURE(rc))
        {
            cerr << "Error: Failed to open file '" << argv[i] << "' for reading. rc=" << rc << endl;
            return ERROR_OPEN_FILE;
        }

        rc = ParseUsbIds(fin);
        if (rc != 0)
        {
            cerr << "Error: Failed parsing USB devices file '" << argv[i] << "'" << endl;
            RTStrmClose(fin);
            return rc;
        }
        RTStrmClose(fin);
    }

    /*
     * Due to USBIDDBVENDOR::iProduct, there is currently a max of 64KB products.
     * (Not a problem as we've only have less that 54K products currently.)
     */
    if (g_products.size() > _64K)
    {
        cerr << "Error: More than 64K products is not supported (input: " << g_products.size() << ")" << endl;
        return ERROR_TOO_MANY_PRODUCTS;
    }

    /*
     * Sort the IDs and fill in the iProduct and cProduct members.
     */
    sort(g_products.begin(), g_products.end());
    sort(g_vendors.begin(), g_vendors.end());

    size_t iProduct = 0;
    for (size_t iVendor = 0; iVendor < g_vendors.size(); iVendor++)
    {
        size_t const idVendor = g_vendors[iVendor].vendorID;
        g_vendors[iVendor].iProduct = iProduct;
        if (   iProduct < g_products.size()
            && g_products[iProduct].vendorID <= idVendor)
        {
            if (g_products[iProduct].vendorID == idVendor)
                do
                    iProduct++;
                while (   iProduct < g_products.size()
                       && g_products[iProduct].vendorID == idVendor);
            else
            {
                cerr << "Error: product without vendor after sorting. impossible!" << endl;
                return ERROR_IN_PARSE_LINE;
            }
        }
        g_vendors[iVendor].cProducts = iProduct - g_vendors[iVendor].iProduct;
    }

    /*
     * Verify that all IDs are unique.
     */
    ProductsSet::iterator ita = adjacent_find(g_products.begin(), g_products.end());
    if (ita != g_products.end())
    {
        cerr << "Error: Duplicate alias detected. " << *ita << endl;
        return ERROR_DUPLICATE_ENTRY;
    }

    /*
     * Do string compression and create the string table.
     */
#ifdef USB_ID_DATABASE_WITH_COMPRESSION
    DoStringCompression();
#endif
    CreateStringTable();

    /*
     * Print stats.
     */
    size_t const cbVendorEntry  = sizeof(USBIdDatabase::s_aVendors[0]) + sizeof(USBIdDatabase::s_aVendorNames[0]);
    size_t const cbProductEntry = sizeof(USBIdDatabase::s_aProducts[0]) + sizeof(USBIdDatabase::s_aProductNames[0]);

    size_t cbOldRaw = (g_products.size() + g_vendors.size()) * sizeof(const char *) * 2 + g_cbRawStrings;
    size_t cbRaw    = g_vendors.size() * cbVendorEntry + g_products.size() * cbProductEntry + g_cbRawStrings;
    size_t cbActual = g_vendors.size() * cbVendorEntry + g_products.size() * cbProductEntry + g_cchStrTab;
#ifdef USB_ID_DATABASE_WITH_COMPRESSION
    cbActual += sizeof(USBIdDatabase::s_aCompDict);
#endif
    cout << INFO_PREF "Total " << dec << cbActual << " bytes";
    if (cbActual < cbRaw)
        cout << " saving " << dec << ((cbRaw - cbActual) * 100 / cbRaw) << "% (" << (cbRaw - cbActual) << " bytes)";
    else
        cout << " wasting " << dec << (cbActual - cbRaw) << " bytes";
    cout << "; old version " << cbOldRaw << " bytes + relocs ("
         << ((cbOldRaw - cbActual) * 100 / cbOldRaw) << "% save)." << endl;


    /*
     * Produce the source file.
     */
    if (!outName)
    {
        cerr << "Error: Output file is not specified." << endl;
        return ERROR_OPEN_FILE;
    }

    fout.open(outName);
    if (!fout.is_open())
    {
        cerr << "Error: Can not open file to write '" << outName << "'." << endl;
        return ERROR_OPEN_FILE;
    }

    fout << header;

    WriteStringTable(fout);
#ifdef USB_ID_DATABASE_WITH_COMPRESSION
    WriteCompressionDictionary(fout);
#endif

    fout << product_header;
    for (ProductsSet::iterator itp = g_products.begin(); itp != g_products.end(); ++itp)
        fout << *itp;
    fout << product_part2;
    for (ProductsSet::iterator itp = g_products.begin(); itp != g_products.end(); ++itp)
        itp->product.printRefLine(fout);
    fout << product_footer;

    fout << vendor_header;
    for (VendorsSet::iterator itv = g_vendors.begin(); itv != g_vendors.end(); ++itv)
        fout << *itv;
    fout << vendor_part2;
    for (VendorsSet::iterator itv = g_vendors.begin(); itv != g_vendors.end(); ++itv)
        itv->vendor.printRefLine(fout);
    fout << vendor_footer;

    fout.close();


    return RTEXITCODE_SUCCESS;
}


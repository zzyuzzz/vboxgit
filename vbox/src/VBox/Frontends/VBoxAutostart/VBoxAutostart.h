/* $Id: VBoxAutostart.h 42732 2012-08-09 22:32:48Z vboxsync $ */
/** @file
 * VBoxAutostart - VirtualBox Autostart service.
 */

/*
 * Copyright (C) 2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef __VBoxAutostart_h__
#define __VBoxAutostart_h__

/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <VBox/cdefs.h>
#include <VBox/types.h>

#include <VBox/com/com.h>
#include <VBox/com/VirtualBox.h>

/*******************************************************************************
*   Constants And Macros, Structures and Typedefs                              *
*******************************************************************************/

/**
 * Config AST node types.
 */
typedef enum CFGASTNODETYPE
{
    /** Invalid. */
    CFGASTNODETYPE_INVALID = 0,
    /** Key/Value pair. */
    CFGASTNODETYPE_KEYVALUE,
    /** Compound type. */
    CFGASTNODETYPE_COMPOUND,
    /** List type. */
    CFGASTNODETYPE_LIST,
    /** 32bit hack. */
    CFGASTNODETYPE_32BIT_HACK = 0x7fffffff
} CFGASTNODETYPE;
/** Pointer to a config AST node type. */
typedef CFGASTNODETYPE *PCFGASTNODETYPE;
/** Pointer to a const config AST node type. */
typedef const CFGASTNODETYPE *PCCFGASTNODETYPE;

/**
 * Config AST.
 */
typedef struct CFGAST
{
    /** AST node type. */
    CFGASTNODETYPE        enmType;
    /** Key or scope id. */
    char                 *pszKey;
    /** Type dependent data. */
    union
    {
        /** Key value pair. */
        struct
        {
            /** Number of characters in the value - excluding terminator. */
            size_t        cchValue;
            /** Value string - variable in size. */
            char          aszValue[1];
        } KeyValue;
        /** Compound type. */
        struct
        {
            /** Number of AST node entries in the array. */
            unsigned       cAstNodes;
            /** AST node array - variable in size. */
            struct CFGAST *apAstNodes[1];
        } Compound;
        /** List type. */
        struct
        {
            /** Number of entries in the list. */
            unsigned       cListEntries;
            /** Array of list entries - variable in size. */
            char          *apszEntries[1];
        } List;
    } u;
} CFGAST, *PCFGAST;

/** Flag whether we are in verbose logging mode. */
extern bool                g_fVerbose;
/** Handle to the VirtualBox interface. */
extern ComPtr<IVirtualBox> g_pVirtualBox;
/** Handle to the session interface. */
extern ComPtr<ISession>    g_pSession;

/**
 * Log information in verbose mode.
 */
#define serviceLogVerbose(a) if (g_fVerbose) { serviceLog a; }

/**
 * Log messages to the release log.
 *
 * @returns nothing.
 * @param   pszFormat    Format string.
 */
DECLHIDDEN(void) serviceLog(const char *pszFormat, ...);

/**
 * Parse the given configuration file and return the interesting config parameters.
 *
 * @returns VBox status code.
 * @param   pszFilename    The config file to parse.
 * @param   ppCfgAst       Where to store the pointer to the root AST node on success.
 */
DECLHIDDEN(int) autostartParseConfig(const char *pszFilename, PCFGAST *ppCfgAst);

/**
 * Destroys the config AST and frees all resources.
 *
 * @returns nothing.
 * @param   pCfgAst        The config AST.
 */
DECLHIDDEN(void) autostartConfigAstDestroy(PCFGAST pCfgAst);

/**
 * Return the config AST node with the given name or NULL if it doesn't exist.
 *
 * @returns Matching config AST node for the given name or NULL if not found.
 * @param   pCfgAst    The config ASt to search.
 * @param   pszName    The name to search for.
 */
DECLHIDDEN(PCFGAST) autostartConfigAstGetByName(PCFGAST pCfgAst, const char *pszName);

/**
 * Main routine for the autostart daemon.
 *
 * @returns exit status code.
 * @param   pCfgAst        Config AST for the startup part of the autostart daemon.
 */
DECLHIDDEN(RTEXITCODE) autostartStartMain(PCFGAST pCfgAst);

/**
 * Main routine for the autostart daemon when stopping virtual machines
 * during system shutdown.
 *
 * @returns exit status code.
 * @param   pCfgAst        Config AST for the shutdown part of the autostart daemon.
 */
DECLHIDDEN(RTEXITCODE) autostartStopMain(PCFGAST pCfgAst);

#endif /* __VBoxAutostart_h__ */


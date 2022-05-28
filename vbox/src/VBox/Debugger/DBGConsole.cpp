/** $Id: DBGConsole.cpp 5669 2007-11-11 05:05:02Z vboxsync $ */
/** @file
 * DBGC - Debugger Console.
 */

/*
 * Copyright (C) 2006-2007 innotek GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/** @page pg_dbgc                       DBGC - The Debug Console
 *
 * The debugger console is a first attempt to make some interactive
 * debugging facilities for the VirtualBox backend (i.e. the VM). At a later
 * stage we'll make a fancy gui around this, but for the present a telnet (or
 * serial terminal) will have to suffice.
 *
 * The debugger is only built into the VM with debug builds or when
 * VBOX_WITH_DEBUGGER is defined. There might be need for \#ifdef'ing on this
 * define to enable special debugger hooks, but the general approach is to
 * make generic interfaces. The individual components also can register
 * external commands, and such code must be within \#ifdef.
 *
 *
 * @section sec_dbgc_op                 Operation (intentions)
 *
 * The console will process commands in a manner similar to the OS/2 and
 * windows kernel debuggers. This means ';' is a command separator and
 * that when possible we'll use the same command names as these two uses.
 *
 *
 * @subsection sec_dbg_op_numbers       Numbers
 *
 * Numbers are hexadecimal unless specified with a prefix indicating
 * elsewise. Prefixes:
 *      - '0x' - hexadecimal.
 *      - '0i' - decimal
 *      - '0t' - octal.
 *      - '0y' - binary.
 *
 *
 * @subsection sec_dbg_op_address       Addressing modes
 *
 *      - Default is flat. For compatability '%' also means flat.
 *      - Segmented addresses are specified selector:offset.
 *      - Physical addresses are specified using '%%'.
 *      - The default target for the addressing is the guest context, the '#'
 *        will override this and set it to the host.
 *
 *
 * @subsection sec_dbg_op_evalution     Evaluation
 *
 * As time permits support will be implemented support for a subset of the C
 * binary operators, starting with '+', '-', '*' and '/'. Support for variables
 * are provided thru commands 'set' and 'unset' and the unary operator '$'. The
 * unary '@' operator will indicate function calls. The debugger needs a set of
 * memory read functions, but we might later extend this to allow registration of
 * external functions too.
 *
 * A special command '?' will then be added which evalutates a given expression
 * and prints it in all the different formats.
 *
 *
 * @subsection sec_dbg_op_registers     Registers
 *
 * Registers are addressed using their name. Some registers which have several fields
 * (like gdtr) will have separate names indicating the different fields. The default
 * register set is the guest one. To access the hypervisor register one have to
 * prefix the register names with '.'.
 *
 *
 * @subsection sec_dbg_op_commands      Commands
 *
 * The commands are all lowercase, case sensitive, and starting with a letter. We will
 * later add some special commands ('?' for evaulation) and perhaps command classes ('.', '!')
 *
 *
 * @section sec_dbg_tasks               Tasks
 *
 * To implement DBGT and instrument VMM for basic state inspection and log
 * viewing, the follwing task must be executed:
 *
 *      -# Basic threading layer in RT.
 *      -# Basic tcpip server abstration in RT.
 *      -# Write DBGC.
 *      -# Write DBCTCP.
 *      -# Integrate with VMM and the rest.
 *      -# Start writing DBGF (VMM).
 */




/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#define LOG_GROUP LOG_GROUP_DBGC
#include <VBox/dbg.h>
#include <VBox/dbgf.h>
#include <VBox/vm.h>
#include <VBox/vmm.h>
#include <VBox/mm.h>
#include <VBox/pgm.h>
#include <VBox/selm.h>
#include <VBox/dis.h>
#include <VBox/param.h>
#include <VBox/err.h>
#include <VBox/log.h>

#include <iprt/alloc.h>
#include <iprt/alloca.h>
#include <iprt/string.h>
#include <iprt/assert.h>
#include <iprt/ctype.h>

#include <stdlib.h>
#include <stdio.h>

#include "DBGCInternal.h"


/*******************************************************************************
*   Internal Functions                                                         *
*******************************************************************************/
static DECLCALLBACK(int) dbgcCmdHelp(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdQuit(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdStop(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdInfo(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdLog(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdLogDest(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdLogFlags(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdFormat(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdLoadSyms(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdSet(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdUnset(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdLoadVars(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdShowVars(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdHarakiri(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdEcho(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcCmdRunScript(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult);

static DECLCALLBACK(int) dbgcOpMinus(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpPluss(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBooleanNot(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBitwiseNot(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpAddrFlat(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpAddrPhys(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpAddrHost(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpAddrHostPhys(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpVar(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult);

static DECLCALLBACK(int) dbgcOpAddrFar(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpMult(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpDiv(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpMod(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpAdd(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpSub(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBitwiseShiftLeft(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBitwiseShiftRight(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBitwiseAnd(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBitwiseXor(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBitwiseOr(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBooleanAnd(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpBooleanOr(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpRangeLength(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpRangeLengthBytes(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcOpRangeTo(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult);

static DECLCALLBACK(int) dbgcSymGetReg(PCDBGCSYM pSymDesc, PDBGCCMDHLP pCmdHlp, DBGCVARTYPE enmType, PDBGCVAR pResult);
static DECLCALLBACK(int) dbgcSymSetReg(PCDBGCSYM pSymDesc, PDBGCCMDHLP pCmdHlp, PCDBGCVAR pValue);

static int dbgcSymbolGet(PDBGC pDbgc, const char *pszSymbol, DBGCVARTYPE enmType, PDBGCVAR pResult);
static int dbgcEvalSub(PDBGC pDbgc, char *pszExpr, size_t cchExpr, PDBGCVAR pResult);
static int dbgcProcessCommand(PDBGC pDbgc, char *pszCmd, size_t cchCmd);


/*******************************************************************************
*   Global Variables                                                           *
*******************************************************************************/
/**
 * Pointer to head of the list of external commands.
 */
static PDBGCEXTCMDS     g_pExtCmdsHead;     /** @todo rw protect g_pExtCmdsHead! */
/** Locks the g_pExtCmdsHead list for reading. */
#define DBGCEXTCMDS_LOCK_RD()       do { } while (0)
/** Locks the g_pExtCmdsHead list for writing. */
#define DBGCEXTCMDS_LOCK_WR()       do { } while (0)
/** UnLocks the g_pExtCmdsHead list after reading. */
#define DBGCEXTCMDS_UNLOCK_RD()     do { } while (0)
/** UnLocks the g_pExtCmdsHead list after writing. */
#define DBGCEXTCMDS_UNLOCK_WR()     do { } while (0)


/** One argument of any kind. */
static const DBGCVARDESC    g_aArgAny[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  0,           1,          DBGCVAR_CAT_ANY,        0,                              "var",          "Any type of argument." },
};

/** Multiple string arguments (min 1). */
static const DBGCVARDESC    g_aArgMultiStr[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           ~0,         DBGCVAR_CAT_STRING,     0,                              "strings",      "One or more strings." },
};

/** Filename string. */
static const DBGCVARDESC    g_aArgFilename[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "path",         "Filename string." },
};


/** 'help' arguments. */
static const DBGCVARDESC    g_aArgHelp[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  0,           ~0,         DBGCVAR_CAT_STRING,     0,                              "cmd/op",       "Zero or more command or operator names." },
};


/** 'info' arguments. */
static const DBGCVARDESC    g_aArgInfo[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "info",         "The name of the info to display." },
    {  0,           1,          DBGCVAR_CAT_STRING,     0,                              "args",         "String arguments to the handler." },
};


/** loadsyms arguments. */
static const DBGCVARDESC    g_aArgLoadSyms[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "path",         "Filename string." },
    {  0,           1,          DBGCVAR_CAT_NUMBER,     0,                              "delta",        "Delta to add to the loaded symbols. (optional)" },
    {  0,           1,          DBGCVAR_CAT_STRING,     0,                              "module name",  "Module name." },
    {  0,           1,          DBGCVAR_CAT_POINTER,    DBGCVD_FLAGS_DEP_PREV,          "module address", "Module address." },
    {  0,           1,          DBGCVAR_CAT_NUMBER,     0,                              "module size",  "The module size. (optional)" },
};


/** log arguments. */
static const DBGCVARDESC    g_aArgLog[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "groups",       "Group modifier string (quote it!)." }
};


/** logdest arguments. */
static const DBGCVARDESC    g_aArgLogDest[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "dests",        "Destination modifier string (quote it!)." }
};


/** logflags arguments. */
static const DBGCVARDESC    g_aArgLogFlags[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "flags",        "Flag modifier string (quote it!)." }
};


/** 'set' arguments */
static const DBGCVARDESC    g_aArgSet[] =
{
    /* cTimesMin,   cTimesMax,  enmCategory,            fFlags,                         pszName,        pszDescription */
    {  1,           1,          DBGCVAR_CAT_STRING,     0,                              "var",          "Variable name." },
    {  1,           1,          DBGCVAR_CAT_ANY,        0,                              "value",        "Value to assign to the variable." },
};





/** Command descriptors for the basic commands. */
static const DBGCCMD    g_aCmds[] =
{
    /* pszCmd,      cArgsMin, cArgsMax, paArgDescs,         cArgDescs,                  pResultDesc,        fFlags,     pfnHandler          pszSyntax,          ....pszDescription */
    { "bye",        0,        0,        NULL,               0,                          NULL,               0,          dbgcCmdQuit,        "",                     "Exits the debugger." },
    { "echo",       1,        ~0,       &g_aArgMultiStr[0], ELEMENTS(g_aArgMultiStr),   NULL,               0,          dbgcCmdEcho,        "<str1> [str2..[strN]]", "Displays the strings separated by one blank space and the last one followed by a newline." },
    { "exit",       0,        0,        NULL,               0,                          NULL,               0,          dbgcCmdQuit,        "",                     "Exits the debugger." },
    { "format",     1,        1,        &g_aArgAny[0],      ELEMENTS(g_aArgAny),        NULL,               0,          dbgcCmdFormat,      "",                     "Evaluates an expression and formats it." },
    { "harakiri",   0,        0,        NULL,               0,                          NULL,               0,          dbgcCmdHarakiri,    "",                     "Kills debugger process." },
    { "help",       0,        ~0,       &g_aArgHelp[0],     ELEMENTS(g_aArgHelp),       NULL,               0,          dbgcCmdHelp,        "[cmd/op [..]]",        "Display help. For help about info items try 'info help'." },
    { "info",       1,        2,        &g_aArgInfo[0],     ELEMENTS(g_aArgInfo),       NULL,               0,          dbgcCmdInfo,        "<info> [args]",        "Display info register in the DBGF. For a list of info items try 'info help'." },
    { "loadsyms",   1,        5,        &g_aArgLoadSyms[0], ELEMENTS(g_aArgLoadSyms),   NULL,               0,          dbgcCmdLoadSyms,    "<filename> [delta] [module] [module address]", "Loads symbols from a text file. Optionally giving a delta and a module." },
    { "loadvars",   1,        1,        &g_aArgFilename[0], ELEMENTS(g_aArgFilename),   NULL,               0,          dbgcCmdLoadVars,    "<filename>",           "Load variables from file. One per line, same as the args to the set command." },
    { "log",        1,        1,        &g_aArgLog[0],      ELEMENTS(g_aArgLog),        NULL,               0,          dbgcCmdLog,         "<group string>",       "Modifies the logging group settings (VBOX_LOG)" },
    { "logdest",    1,        1,        &g_aArgLogDest[0],  ELEMENTS(g_aArgLogDest),    NULL,               0,          dbgcCmdLogDest,     "<dest string>",        "Modifies the logging destination (VBOX_LOG_DEST)." },
    { "logflags",   1,        1,        &g_aArgLogFlags[0], ELEMENTS(g_aArgLogFlags),   NULL,               0,          dbgcCmdLogFlags,    "<flags string>",       "Modifies the logging flags (VBOX_LOG_FLAGS)." },
    { "quit",       0,        0,        NULL,               0,                          NULL,               0,          dbgcCmdQuit,        "",                     "Exits the debugger." },
    { "runscript",  1,        1,        &g_aArgFilename[0], ELEMENTS(g_aArgFilename),   NULL,               0,          dbgcCmdRunScript,   "<filename>",           "Runs the command listed in the script. Lines starting with '#' (after removing blanks) are comment. blank lines are ignored. Stops on failure." },
    { "set",        2,        2,        &g_aArgSet[0],      ELEMENTS(g_aArgSet),        NULL,               0,          dbgcCmdSet,         "<var> <value>",        "Sets a global variable." },
    { "showvars",   0,        0,        NULL,               0,                          NULL,               0,          dbgcCmdShowVars,    "",                     "List all the defined variables." },
    { "stop",       0,        0,        NULL,               0,                          NULL,               0,          dbgcCmdStop,        "",                     "Stop execution." },
    { "unset",      1,        ~0,       &g_aArgMultiStr[0], ELEMENTS(g_aArgMultiStr),   NULL,               0,          dbgcCmdUnset,       "<var1> [var1..[varN]]", "Unsets (delete) one or more global variables." },
};


/** Operators. */
static const DBGCOP g_aOps[] =
{
    /* szName is initialized as a 4 char array because of M$C elsewise optimizing it away in /Ox mode (the 'const char' vs 'char' problem). */
    /* szName,          cchName, fBinary,    iPrecedence,    pfnHandlerUnary,    pfnHandlerBitwise  */
    { {'-'},            1,       false,      1,              dbgcOpMinus,        NULL,                       "Unary minus." },
    { {'+'},            1,       false,      1,              dbgcOpPluss,        NULL,                       "Unary pluss." },
    { {'!'},            1,       false,      1,              dbgcOpBooleanNot,   NULL,                       "Boolean not." },
    { {'~'},            1,       false,      1,              dbgcOpBitwiseNot,   NULL,                       "Bitwise complement." },
    { {':'},            1,       true,       2,              NULL,               dbgcOpAddrFar,              "Far pointer." },
    { {'%'},            1,       false,      3,              dbgcOpAddrFlat,     NULL,                       "Flat address." },
    { {'%','%'},        2,       false,      3,              dbgcOpAddrPhys,     NULL,                       "Physical address." },
    { {'#'},            1,       false,      3,              dbgcOpAddrHost,     NULL,                       "Flat host address." },
    { {'#','%','%'},    3,       false,      3,              dbgcOpAddrHostPhys, NULL,                       "Physical host address." },
    { {'$'},            1,       false,      3,              dbgcOpVar,          NULL,                       "Reference a variable." },
    { {'*'},            1,       true,       10,             NULL,               dbgcOpMult,                 "Multiplication." },
    { {'/'},            1,       true,       11,             NULL,               dbgcOpDiv,                  "Division." },
    { {'%'},            1,       true,       12,             NULL,               dbgcOpMod,                  "Modulus." },
    { {'+'},            1,       true,       13,             NULL,               dbgcOpAdd,                  "Addition." },
    { {'-'},            1,       true,       14,             NULL,               dbgcOpSub,                  "Subtraction." },
    { {'<','<'},        2,       true,       15,             NULL,               dbgcOpBitwiseShiftLeft,     "Bitwise left shift." },
    { {'>','>'},        2,       true,       16,             NULL,               dbgcOpBitwiseShiftRight,    "Bitwise right shift." },
    { {'&'},            1,       true,       17,             NULL,               dbgcOpBitwiseAnd,           "Bitwise and." },
    { {'^'},            1,       true,       18,             NULL,               dbgcOpBitwiseXor,           "Bitwise exclusiv or." },
    { {'|'},            1,       true,       19,             NULL,               dbgcOpBitwiseOr,            "Bitwise inclusive or." },
    { {'&','&'},        2,       true,       20,             NULL,               dbgcOpBooleanAnd,           "Boolean and." },
    { {'|','|'},        2,       true,       21,             NULL,               dbgcOpBooleanOr,            "Boolean or." },
    { {'L'},            1,       true,       22,             NULL,               dbgcOpRangeLength,          "Range elements." },
    { {'L','B'},        2,       true,       23,             NULL,               dbgcOpRangeLengthBytes,     "Range bytes." },
    { {'T'},            1,       true,       24,             NULL,               dbgcOpRangeTo,              "Range to." }
};

/** Bitmap where set bits indicates the characters the may start an operator name. */
static uint32_t g_bmOperatorChars[256 / (4*8)];

/** Register symbol uUser value.
 * @{
 */
/** If set the register set is the hypervisor and not the guest one. */
#define SYMREG_FLAGS_HYPER      RT_BIT(20)
/** If set a far conversion of the value will use the high 16 bit for the selector.
 * If clear the low 16 bit will be used. */
#define SYMREG_FLAGS_HIGH_SEL   RT_BIT(21)
/** The shift value to calc the size of a register symbol from the uUser value. */
#define SYMREG_SIZE_SHIFT       (24)
/** Get the offset */
#define SYMREG_OFFSET(uUser)    (uUser & ((1 << 20) - 1))
/** Get the size. */
#define SYMREG_SIZE(uUser)      ((uUser >> SYMREG_SIZE_SHIFT) & 0xff)
/** 1 byte. */
#define SYMREG_SIZE_1           ( 1 << SYMREG_SIZE_SHIFT)
/** 2 byte. */
#define SYMREG_SIZE_2           ( 2 << SYMREG_SIZE_SHIFT)
/** 4 byte. */
#define SYMREG_SIZE_4           ( 4 << SYMREG_SIZE_SHIFT)
/** 6 byte. */
#define SYMREG_SIZE_6           ( 6 << SYMREG_SIZE_SHIFT)
/** 8 byte. */
#define SYMREG_SIZE_8           ( 8 << SYMREG_SIZE_SHIFT)
/** 12 byte. */
#define SYMREG_SIZE_12          (12 << SYMREG_SIZE_SHIFT)
/** 16 byte. */
#define SYMREG_SIZE_16          (16 << SYMREG_SIZE_SHIFT)
/** @} */

/** Builtin Symbols.
 * ASSUMES little endian register representation!
 */
static const DBGCSYM    g_aSyms[] =
{
    { "eax",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax)      | SYMREG_SIZE_4 },
    { "ax",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax)      | SYMREG_SIZE_2 },
    { "al",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax)      | SYMREG_SIZE_1 },
    { "ah",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax) + 1  | SYMREG_SIZE_1 },

    { "ebx",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx)      | SYMREG_SIZE_4 },
    { "bx",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx)      | SYMREG_SIZE_2 },
    { "bl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx)      | SYMREG_SIZE_1 },
    { "bh",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx) + 1  | SYMREG_SIZE_1 },

    { "ecx",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx)      | SYMREG_SIZE_4 },
    { "cx",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx)      | SYMREG_SIZE_2 },
    { "cl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx)      | SYMREG_SIZE_1 },
    { "ch",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx) + 1  | SYMREG_SIZE_1 },

    { "edx",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx)      | SYMREG_SIZE_4 },
    { "dx",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx)      | SYMREG_SIZE_2 },
    { "dl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx)      | SYMREG_SIZE_1 },
    { "dh",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx) + 1  | SYMREG_SIZE_1 },

    { "edi",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edi)      | SYMREG_SIZE_4 },
    { "di",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edi)      | SYMREG_SIZE_2 },

    { "esi",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esi)      | SYMREG_SIZE_4 },
    { "si",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esi)      | SYMREG_SIZE_2 },

    { "ebp",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebp)      | SYMREG_SIZE_4 },
    { "bp",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebp)      | SYMREG_SIZE_2 },

    { "esp",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esp)      | SYMREG_SIZE_4 },
    { "sp",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esp)      | SYMREG_SIZE_2 },

    { "eip",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eip)      | SYMREG_SIZE_4 },
    { "ip",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eip)      | SYMREG_SIZE_2 },

    { "efl",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_4 },
    { "eflags", dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_4 },
    { "fl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_2 },
    { "flags",  dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_2 },

    { "cs",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cs)       | SYMREG_SIZE_2 },
    { "ds",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ds)       | SYMREG_SIZE_2 },
    { "es",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, es)       | SYMREG_SIZE_2 },
    { "fs",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, fs)       | SYMREG_SIZE_2 },
    { "gs",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, gs)       | SYMREG_SIZE_2 },
    { "ss",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ss)       | SYMREG_SIZE_2 },

    { "cr0",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr0)      | SYMREG_SIZE_4 },
    { "cr2",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr2)      | SYMREG_SIZE_4 },
    { "cr3",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr3)      | SYMREG_SIZE_4 },
    { "cr4",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr4)      | SYMREG_SIZE_4 },

    { "tr",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, tr)       | SYMREG_SIZE_2 },
    { "ldtr",   dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ldtr)     | SYMREG_SIZE_2 },

    { "gdtr",   dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, gdtr)     | SYMREG_SIZE_6 },
    { "gdtr.limit", dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, gdtr.cbGdt)| SYMREG_SIZE_2 },
    { "gdtr.base",  dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, gdtr.pGdt)| SYMREG_SIZE_4 },

    { "idtr",   dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, idtr)     | SYMREG_SIZE_6 },
    { "idtr.limit", dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, idtr.cbIdt)| SYMREG_SIZE_2 },
    { "idtr.base",  dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, idtr.pIdt)| SYMREG_SIZE_4 },

    /* hypervisor */

    {".eax",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".ax",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".al",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax)      | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },
    {".ah",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eax) + 1  | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },

    {".ebx",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".bx",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".bl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx)      | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },
    {".bh",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebx) + 1  | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },

    {".ecx",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".cx",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".cl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx)      | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },
    {".ch",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ecx) + 1  | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },

    {".edx",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".dx",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".dl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx)      | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },
    {".dh",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edx) + 1  | SYMREG_SIZE_1 | SYMREG_FLAGS_HYPER },

    {".edi",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edi)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".di",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, edi)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".esi",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esi)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".si",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esi)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".ebp",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebp)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".bp",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ebp)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".esp",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esp)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".sp",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, esp)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".eip",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eip)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".ip",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eip)      | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".efl",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".eflags", dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".fl",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".flags",  dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, eflags)   | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".cs",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cs)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".ds",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ds)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".es",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, es)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".fs",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, fs)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".gs",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, gs)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".ss",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ss)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".cr0",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr0)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".cr2",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr2)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".cr3",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr3)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },
    {".cr4",    dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, cr4)      | SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },

    {".tr",     dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, tr)       | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },
    {".ldtr",   dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, ldtr)     | SYMREG_SIZE_2 | SYMREG_FLAGS_HYPER },

    {".gdtr",   dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, gdtr)     | SYMREG_SIZE_6 | SYMREG_FLAGS_HYPER },
    {".gdtr.limit", dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, gdtr.cbGdt)| SYMREG_SIZE_2| SYMREG_FLAGS_HYPER },
    {".gdtr.base",  dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, gdtr.pGdt)| SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },

    {".idtr",   dbgcSymGetReg,          dbgcSymSetReg,      offsetof(CPUMCTX, idtr)     | SYMREG_SIZE_6 | SYMREG_FLAGS_HYPER },
    {".idtr.limit", dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, idtr.cbIdt)| SYMREG_SIZE_2| SYMREG_FLAGS_HYPER },
    {".idtr.base",  dbgcSymGetReg,      dbgcSymSetReg,      offsetof(CPUMCTX, idtr.pIdt)| SYMREG_SIZE_4 | SYMREG_FLAGS_HYPER },

};


/**
 * Prints full command help.
 */
static int dbgcPrintHelp(PDBGCCMDHLP pCmdHlp, PCDBGCCMD pCmd, bool fExternal)
{
    int rc;

    /* the command */
    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                            "%s%-*s %-30s %s",
                            fExternal ? "." : "",
                            fExternal ? 10 : 11,
                            pCmd->pszCmd,
                            pCmd->pszSyntax,
                            pCmd->pszDescription);
    if (!pCmd->cArgsMin && pCmd->cArgsMin == pCmd->cArgsMax)
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <no args>\n");
    else if (pCmd->cArgsMin == pCmd->cArgsMax)
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <%u args>\n", pCmd->cArgsMin);
    else if (pCmd->cArgsMax == ~0U)
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <%u+ args>\n", pCmd->cArgsMin);
    else
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <%u to %u args>\n", pCmd->cArgsMin, pCmd->cArgsMax);

    /* argument descriptions. */
    for (unsigned i = 0; i < pCmd->cArgDescs; i++)
    {
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                "    %-12s %s",
                                pCmd->paArgDescs[i].pszName,
                                pCmd->paArgDescs[i].pszDescription);
        if (!pCmd->paArgDescs[i].cTimesMin)
        {
            if (pCmd->paArgDescs[i].cTimesMax == ~0U)
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <optional+>\n");
            else
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <optional-%u>\n", pCmd->paArgDescs[i].cTimesMax);
        }
        else
        {
            if (pCmd->paArgDescs[i].cTimesMax == ~0U)
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <%u+>\n", pCmd->paArgDescs[i].cTimesMin);
            else
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, " <%u-%u>\n", pCmd->paArgDescs[i].cTimesMin, pCmd->paArgDescs[i].cTimesMax);
        }
    }
    return rc;
}


/**
 * The 'help' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdHelp(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    PDBGC       pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    int         rc = VINF_SUCCESS;
    unsigned    i;

    if (!cArgs)
    {
        /*
         * All the stuff.
         */
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                "VirtualBox Debugger\n"
                                "-------------------\n"
                                "\n"
                                "Commands and Functions:\n");
        for (i = 0; i < ELEMENTS(g_aCmds); i++)
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                    "%-11s %-30s %s\n",
                                    g_aCmds[i].pszCmd,
                                    g_aCmds[i].pszSyntax,
                                    g_aCmds[i].pszDescription);
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                "\n"
                                "Emulation: %s\n", pDbgc->pszEmulation);
        PCDBGCCMD pCmd = pDbgc->paEmulationCmds;
        for (i = 0; i < pDbgc->cEmulationCmds; i++, pCmd++)
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                    "%-11s %-30s %s\n",
                                    pCmd->pszCmd,
                                    pCmd->pszSyntax,
                                    pCmd->pszDescription);

        if (g_pExtCmdsHead)
        {
            DBGCEXTCMDS_LOCK_RD();
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                    "\n"
                                    "External Commands and Functions:\n");
            for (PDBGCEXTCMDS pExtCmd = g_pExtCmdsHead; pExtCmd; pExtCmd = pExtCmd->pNext)
                for (i = 0; i < pExtCmd->cCmds; i++)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                            ".%-10s %-30s %s\n",
                                            pExtCmd->paCmds[i].pszCmd,
                                            pExtCmd->paCmds[i].pszSyntax,
                                            pExtCmd->paCmds[i].pszDescription);
            DBGCEXTCMDS_UNLOCK_RD();
        }

        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                "\n"
                                "Operators:\n");
        unsigned iPrecedence = 0;
        unsigned cLeft = ELEMENTS(g_aOps);
        while (cLeft > 0)
        {
            for (i = 0; i < ELEMENTS(g_aOps); i++)
                if (g_aOps[i].iPrecedence == iPrecedence)
                {
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                            "%-10s  %s  %s\n",
                                            g_aOps[i].szName,
                                            g_aOps[i].fBinary ? "Binary" : "Unary ",
                                            g_aOps[i].pszDescription);
                    cLeft--;
                }
            iPrecedence++;
        }
    }
    else
    {
        /*
         * Search for the arguments (strings).
         */
        for (unsigned iArg = 0; iArg < cArgs; iArg++)
        {
            Assert(paArgs[iArg].enmType == DBGCVAR_TYPE_STRING);
            bool fFound = false;

            /* lookup in the emulation command list first */
            for (i = 0; i < pDbgc->cEmulationCmds; i++)
                if (!strcmp(pDbgc->paEmulationCmds[i].pszCmd, paArgs[iArg].u.pszString))
                {
                    rc = dbgcPrintHelp(pCmdHlp, &pDbgc->paEmulationCmds[i], false);
                    fFound = true;
                    break;
                }

            /* lookup in the command list (even when found in the emulation) */
            for (i = 0; i < ELEMENTS(g_aCmds); i++)
                if (!strcmp(g_aCmds[i].pszCmd, paArgs[iArg].u.pszString))
                {
                    rc = dbgcPrintHelp(pCmdHlp, &g_aCmds[i], false);
                    fFound = true;
                    break;
                }

           /* external commands */
           if (     !fFound
               &&   g_pExtCmdsHead
               &&   paArgs[iArg].u.pszString[0] == '.')
           {
               DBGCEXTCMDS_LOCK_RD();
               for (PDBGCEXTCMDS pExtCmd = g_pExtCmdsHead; pExtCmd; pExtCmd = pExtCmd->pNext)
                   for (i = 0; i < pExtCmd->cCmds; i++)
                       if (!strcmp(pExtCmd->paCmds[i].pszCmd, paArgs[iArg].u.pszString + 1))
                       {
                           rc = dbgcPrintHelp(pCmdHlp, &g_aCmds[i], true);
                           fFound = true;
                           break;
                       }
               DBGCEXTCMDS_UNLOCK_RD();
           }

           /* operators */
           if (!fFound && strlen(paArgs[iArg].u.pszString) < sizeof(g_aOps[i].szName))
           {
               for (i = 0; i < ELEMENTS(g_aOps); i++)
                   if (!strcmp(g_aOps[i].szName, paArgs[iArg].u.pszString))
                   {
                       rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                               "%-10s  %s  %s\n",
                                               g_aOps[i].szName,
                                               g_aOps[i].fBinary ? "Binary" : "Unary ",
                                               g_aOps[i].pszDescription);
                       fFound = true;
                       break;
                   }
           }

           /* found? */
           if (!fFound)
               rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                                       "error: '%s' was not found!\n",
                                       paArgs[iArg].u.pszString);
        } /* foreach argument */
    }

    NOREF(pCmd);
    NOREF(pVM);
    NOREF(pResult);
    return rc;
}


/**
 * The 'quit', 'exit' and 'bye' commands.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdQuit(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Quitting console...\n");
    NOREF(pCmd);
    NOREF(pVM);
    NOREF(paArgs);
    NOREF(cArgs);
    NOREF(pResult);
    return VERR_DBGC_QUIT;
}


/**
 * The 'stop' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdStop(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    /*
     * Check if the VM is halted or not before trying to halt it.
     */
    int rc;
    if (DBGFR3IsHalted(pVM))
        rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "warning: The VM is already halted...\n");
    else
    {
        rc = DBGFR3Halt(pVM);
        if (VBOX_SUCCESS(rc))
            rc = VWRN_DBGC_CMD_PENDING;
        else
            rc = pCmdHlp->pfnVBoxError(pCmdHlp, rc, "Executing DBGFR3Halt().");
    }

    NOREF(pCmd); NOREF(paArgs); NOREF(cArgs); NOREF(pResult);
    return rc;
}


/**
 * The 'echo' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdEcho(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    /*
     * Loop thru the arguments and print them with one space between.
     */
    int rc = 0;
    for (unsigned i = 0; i < cArgs; i++)
    {
        if (paArgs[i].enmType == DBGCVAR_TYPE_STRING)
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, i ? " %s" : "%s", paArgs[i].u.pszString);
        else
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, i ? " <parser error>" : "<parser error>");
        if (VBOX_FAILURE(rc))
            return rc;
    }
    NOREF(pCmd); NOREF(pResult); NOREF(pVM);
    return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "\n");
}


/**
 * The 'runscript' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdRunScript(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    /* check that the parser did what it's supposed to do. */
    if (    cArgs != 1
        ||  paArgs[0].enmType != DBGCVAR_TYPE_STRING)
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "parser error\n");

    /*
     * Try open the script.
     */
    const char *pszFilename = paArgs[0].u.pszString;
    FILE *pFile = fopen(pszFilename, "r");
    if (!pFile)
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Failed to open '%s'.\n", pszFilename);

    /*
     * Execute it line by line.
     */
    int rc = 0;
    unsigned iLine = 0;
    char szLine[8192];
    while (fgets(szLine, sizeof(szLine), pFile))
    {
        /* check that the line isn't too long. */
        char *pszEnd = strchr(szLine, '\0');
        if (pszEnd == &szLine[sizeof(szLine) - 1])
        {
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "runscript error: Line #%u is too long\n", iLine);
            break;
        }
        iLine++;

        /* strip leading blanks and check for comment / blank line. */
        char *psz = RTStrStripL(szLine);
        if (    *psz == '\0'
            ||  *psz == '\n'
            ||  *psz == '#')
            continue;

        /* strip trailing blanks and check for empty line (\r case). */
        while (     pszEnd > psz
               &&   isspace(pszEnd[-1])) /* isspace includes \n and \r normally. */
            *--pszEnd = '\0';

        /** @todo check for Control-C / Cancel at this point... */

        /*
         * Execute the command.
         *
         * This is a bit wasteful with scratch space btw., can fix it later.
         * The whole return code crap should be fixed too, so that it's possible
         * to know whether a command succeeded (VBOX_SUCCESS()) or failed, and
         * more importantly why it failed.
         */
        rc = pCmdHlp->pfnExec(pCmdHlp, "%s", psz);
        if (VBOX_FAILURE(rc))
        {
            if (rc == VERR_BUFFER_OVERFLOW)
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "runscript error: Line #%u is too long (exec overflowed)\n", iLine);
            break;
        }
        if (rc == VWRN_DBGC_CMD_PENDING)
        {
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "runscript error: VWRN_DBGC_CMD_PENDING on line #%u, script terminated\n", iLine);
            break;
        }
    }

    fclose(pFile);

    NOREF(pCmd); NOREF(pResult); NOREF(pVM);
    return rc;
}


/**
 * Print formatted string.
 *
 * @param   pHlp        Pointer to this structure.
 * @param   pszFormat   The format string.
 * @param   ...         Arguments.
 */
static DECLCALLBACK(void) dbgcCmdInfo_Printf(PCDBGFINFOHLP pHlp, const char *pszFormat, ...)
{
    PDBGCCMDHLP pCmdHlp = *(PDBGCCMDHLP *)(pHlp + 1);
    va_list args;
    va_start(args,  pszFormat);
    pCmdHlp->pfnPrintfV(pCmdHlp, NULL, pszFormat, args);
    va_end(args);
}


/**
 * Print formatted string.
 *
 * @param   pHlp        Pointer to this structure.
 * @param   pszFormat   The format string.
 * @param   args        Argument list.
 */
static DECLCALLBACK(void) dbgcCmdInfo_PrintfV(PCDBGFINFOHLP pHlp, const char *pszFormat, va_list args)
{
    PDBGCCMDHLP pCmdHlp = *(PDBGCCMDHLP *)(pHlp + 1);
    pCmdHlp->pfnPrintfV(pCmdHlp, NULL, pszFormat, args);
}


/**
 * The 'info' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdInfo(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    /*
     * Validate input.
     */
    if (    cArgs < 1
        ||  cArgs > 2
        ||  paArgs[0].enmType != DBGCVAR_TYPE_STRING
        ||  paArgs[cArgs - 1].enmType != DBGCVAR_TYPE_STRING)
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "internal error: The parser doesn't do its job properly yet.. quote the string.\n");
    if (!pVM)
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "error: No VM.\n");

    /*
     * Dump it.
     */
    struct
    {
        DBGFINFOHLP Hlp;
        PDBGCCMDHLP pCmdHlp;
    } Hlp = { { dbgcCmdInfo_Printf, dbgcCmdInfo_PrintfV }, pCmdHlp };
    int rc = DBGFR3Info(pVM, paArgs[0].u.pszString, cArgs == 2 ? paArgs[1].u.pszString : NULL, &Hlp.Hlp);
    if (VBOX_FAILURE(rc))
        return pCmdHlp->pfnVBoxError(pCmdHlp, rc, "DBGFR3Info()\n");

    NOREF(pCmd); NOREF(pResult);
    return 0;
}


/**
 * The 'log' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdLog(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    int rc = DBGFR3LogModifyGroups(pVM, paArgs[0].u.pszString);
    if (VBOX_SUCCESS(rc))
        return VINF_SUCCESS;
    NOREF(pCmd); NOREF(cArgs); NOREF(pResult);
    return pCmdHlp->pfnVBoxError(pCmdHlp, rc, "DBGFR3LogModifyGroups(%p,'%s')\n", pVM, paArgs[0].u.pszString);
}


/**
 * The 'logdest' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdLogDest(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    int rc = DBGFR3LogModifyDestinations(pVM, paArgs[0].u.pszString);
    if (VBOX_SUCCESS(rc))
        return VINF_SUCCESS;
    NOREF(pCmd); NOREF(cArgs); NOREF(pResult);
    return pCmdHlp->pfnVBoxError(pCmdHlp, rc, "DBGFR3LogModifyDestinations(%p,'%s')\n", pVM, paArgs[0].u.pszString);
}


/**
 * The 'logflags' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdLogFlags(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    int rc = DBGFR3LogModifyFlags(pVM, paArgs[0].u.pszString);
    if (VBOX_SUCCESS(rc))
        return VINF_SUCCESS;
    NOREF(pCmd); NOREF(cArgs); NOREF(pResult);
    return pCmdHlp->pfnVBoxError(pCmdHlp, rc, "DBGFR3LogModifyFlags(%p,'%s')\n", pVM, paArgs[0].u.pszString);
}


/**
 * The 'format' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdFormat(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    LogFlow(("dbgcCmdFormat\n"));
    static const char *apszRangeDesc[] =
    {
        "none", "bytes", "elements"
    };
    int rc;

    for (unsigned iArg = 0; iArg < cArgs; iArg++)
    {
        switch (paArgs[iArg].enmType)
        {
            case DBGCVAR_TYPE_UNKNOWN:
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                    "Unknown variable type!\n");
                break;
            case DBGCVAR_TYPE_GC_FLAT:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Guest flat address: %%%08x range %lld %s\n",
                        paArgs[iArg].u.GCFlat,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Guest flat address: %%%08x\n",
                        paArgs[iArg].u.GCFlat);
                break;
            case DBGCVAR_TYPE_GC_FAR:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Guest far address: %04x:%08x range %lld %s\n",
                        paArgs[iArg].u.GCFar.sel,
                        paArgs[iArg].u.GCFar.off,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Guest far address: %04x:%08x\n",
                        paArgs[iArg].u.GCFar.sel,
                        paArgs[iArg].u.GCFar.off);
                break;
            case DBGCVAR_TYPE_GC_PHYS:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Guest physical address: %%%%%08x range %lld %s\n",
                        paArgs[iArg].u.GCPhys,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Guest physical address: %%%%%08x\n",
                        paArgs[iArg].u.GCPhys);
                break;
            case DBGCVAR_TYPE_HC_FLAT:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Host flat address: %%%08x range %lld %s\n",
                        paArgs[iArg].u.pvHCFlat,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Host flat address: %%%08x\n",
                        paArgs[iArg].u.pvHCFlat);
                break;
            case DBGCVAR_TYPE_HC_FAR:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Host far address: %04x:%08x range %lld %s\n",
                        paArgs[iArg].u.HCFar.sel,
                        paArgs[iArg].u.HCFar.off,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Host far address: %04x:%08x\n",
                        paArgs[iArg].u.HCFar.sel,
                        paArgs[iArg].u.HCFar.off);
                break;
            case DBGCVAR_TYPE_HC_PHYS:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Host physical address: %VHp range %lld %s\n",
                        paArgs[iArg].u.HCPhys,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Host physical address: %VHp\n",
                        paArgs[iArg].u.HCPhys);
                break;

            case DBGCVAR_TYPE_STRING:
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                    "String, %lld bytes long: %s\n",
                    paArgs[iArg].u64Range,
                    paArgs[iArg].u.pszString);
                break;

            case DBGCVAR_TYPE_NUMBER:
                if (paArgs[iArg].enmRangeType != DBGCVAR_RANGE_NONE)
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Number: hex %llx  dec 0i%lld  oct 0t%llo  range %lld %s\n",
                        paArgs[iArg].u.u64Number,
                        paArgs[iArg].u.u64Number,
                        paArgs[iArg].u.u64Number,
                        paArgs[iArg].u64Range,
                        apszRangeDesc[paArgs[iArg].enmRangeType]);
                else
                    rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                        "Number: hex %llx  dec 0i%lld  oct 0t%llo\n",
                        paArgs[iArg].u.u64Number,
                        paArgs[iArg].u.u64Number,
                        paArgs[iArg].u.u64Number);
                break;

            default:
                rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL,
                    "Invalid argument type %d\n",
                    paArgs[iArg].enmType);
                break;
        }
    } /* arg loop */

    NOREF(pCmd); NOREF(pVM); NOREF(pResult);
    return 0;
}


/**
 * The 'loadsyms' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdLoadSyms(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    /*
     * Validate the parsing and make sense of the input.
     * This is a mess as usual because we don't trust the parser yet.
     */
    if (    cArgs < 1
        ||  paArgs[0].enmType != DBGCVAR_TYPE_STRING)
    {
        AssertMsgFailed(("Parse error, first argument required to be string!\n"));
        return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    DBGCVAR     AddrVar;
    RTGCUINTPTR Delta = 0;
    const char *pszModule = NULL;
    RTGCUINTPTR ModuleAddress = 0;
    unsigned    cbModule = 0;
    if (cArgs > 1)
    {
        unsigned iArg = 1;
        if (paArgs[iArg].enmType == DBGCVAR_TYPE_NUMBER)
        {
            Delta = (RTGCUINTPTR)paArgs[iArg].u.u64Number;
            iArg++;
        }
        if (iArg < cArgs)
        {
            if (paArgs[iArg].enmType != DBGCVAR_TYPE_STRING)
            {
                AssertMsgFailed(("Parse error, module argument required to be string!\n"));
                return VERR_PARSE_INCORRECT_ARG_TYPE;
            }
            pszModule = paArgs[iArg].u.pszString;
            iArg++;
            if (iArg < cArgs)
            {
                if (DBGCVAR_ISPOINTER(paArgs[iArg].enmType))
                {
                    AssertMsgFailed(("Parse error, module argument required to be GC pointer!\n"));
                    return VERR_PARSE_INCORRECT_ARG_TYPE;
                }
                int rc = pCmdHlp->pfnEval(pCmdHlp, &AddrVar, "%%(%Dv)", &paArgs[iArg]);
                if (VBOX_FAILURE(rc))
                    return pCmdHlp->pfnVBoxError(pCmdHlp, rc, "Module address cast %%(%Dv) failed.", &paArgs[iArg]);
                ModuleAddress = paArgs[iArg].u.GCFlat;
                iArg++;
                if (iArg < cArgs)
                {
                    if (paArgs[iArg].enmType != DBGCVAR_TYPE_NUMBER)
                    {
                        AssertMsgFailed(("Parse error, module argument required to be an interger!\n"));
                        return VERR_PARSE_INCORRECT_ARG_TYPE;
                    }
                    cbModule = (unsigned)paArgs[iArg].u.u64Number;
                    iArg++;
                    if (iArg < cArgs)
                    {
                        AssertMsgFailed(("Parse error, too many arguments!\n"));
                        return VERR_PARSE_TOO_MANY_ARGUMENTS;
                    }
                }
            }
        }
    }

    /*
     * Call the debug info manager about this loading...
     */
    int rc = DBGFR3ModuleLoad(pVM, paArgs[0].u.pszString, Delta, pszModule, ModuleAddress, cbModule);
    if (VBOX_FAILURE(rc))
        return pCmdHlp->pfnVBoxError(pCmdHlp, rc, "DBGInfoSymbolLoad(, '%s', %VGv, '%s', %VGv, 0)\n",
                                     paArgs[0].u.pszString, Delta, pszModule, ModuleAddress);

    NOREF(pCmd); NOREF(pResult);
    return VINF_SUCCESS;
}


/**
 * The 'set' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdSet(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /* parse sanity check. */
    AssertMsg(paArgs[0].enmType == DBGCVAR_TYPE_STRING, ("expected string not %d as first arg!\n", paArgs[0].enmType));
    if (paArgs[0].enmType != DBGCVAR_TYPE_STRING)
        return VERR_PARSE_INCORRECT_ARG_TYPE;


    /*
     * A variable must start with an alpha chars and only contain alpha numerical chars.
     */
    const char *pszVar = paArgs[0].u.pszString;
    if (!isalpha(*pszVar) || *pszVar == '_')
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL,
            "syntax error: Invalid variable name '%s'. Variable names must match regex '[_a-zA-Z][_a-zA-Z0-9*'!", paArgs[0].u.pszString);

    while (isalnum(*pszVar) || *pszVar == '_')
        *pszVar++;
    if (*pszVar)
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL,
            "syntax error: Invalid variable name '%s'. Variable names must match regex '[_a-zA-Z][_a-zA-Z0-9*]'!", paArgs[0].u.pszString);


    /*
     * Calc variable size.
     */
    size_t  cbVar = (size_t)paArgs[0].u64Range + sizeof(DBGCNAMEDVAR);
    if (paArgs[1].enmType == DBGCVAR_TYPE_STRING)
        cbVar += 1 + (size_t)paArgs[1].u64Range;

    /*
     * Look for existing one.
     */
    pszVar = paArgs[0].u.pszString;
    for (unsigned iVar = 0; iVar < pDbgc->cVars; iVar++)
    {
        if (!strcmp(pszVar, pDbgc->papVars[iVar]->szName))
        {
            /*
             * Update existing variable.
             */
            void *pv = RTMemRealloc(pDbgc->papVars[iVar], cbVar);
            if (!pv)
                return VERR_PARSE_NO_MEMORY;
            PDBGCNAMEDVAR pVar = pDbgc->papVars[iVar] = (PDBGCNAMEDVAR)pv;

            pVar->Var = paArgs[1];
            memcpy(pVar->szName, paArgs[0].u.pszString, (size_t)paArgs[0].u64Range + 1);
            if (paArgs[1].enmType == DBGCVAR_TYPE_STRING)
                pVar->Var.u.pszString = (char *)memcpy(&pVar->szName[paArgs[0].u64Range + 1], paArgs[1].u.pszString, (size_t)paArgs[1].u64Range + 1);
            return 0;
        }
    }

    /*
     * Allocate another.
     */
    PDBGCNAMEDVAR pVar = (PDBGCNAMEDVAR)RTMemAlloc(cbVar);

    pVar->Var = paArgs[1];
    memcpy(pVar->szName, pszVar, (size_t)paArgs[0].u64Range + 1);
    if (paArgs[1].enmType == DBGCVAR_TYPE_STRING)
        pVar->Var.u.pszString = (char *)memcpy(&pVar->szName[paArgs[0].u64Range + 1], paArgs[1].u.pszString, (size_t)paArgs[1].u64Range + 1);

    /* need to reallocate the pointer array too? */
    if (!(pDbgc->cVars % 0x20))
    {
        void *pv = RTMemRealloc(pDbgc->papVars, (pDbgc->cVars + 0x20) * sizeof(pDbgc->papVars[0]));
        if (!pv)
        {
            RTMemFree(pVar);
            return VERR_PARSE_NO_MEMORY;
        }
        pDbgc->papVars = (PDBGCNAMEDVAR *)pv;
    }
    pDbgc->papVars[pDbgc->cVars++] = pVar;

    NOREF(pCmd); NOREF(pVM); NOREF(cArgs); NOREF(pResult);
    return 0;
}


/**
 * The 'unset' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdUnset(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Don't trust the parser.
     */
    for (unsigned  i = 0; i < cArgs; i++)
        if (paArgs[i].enmType != DBGCVAR_TYPE_STRING)
        {
            AssertMsgFailed(("expected strings only. (arg=%d)!\n", i));
            return VERR_PARSE_INCORRECT_ARG_TYPE;
        }

    /*
     * Iterate the variables and unset them.
     */
    for (unsigned iArg = 0; iArg < cArgs; iArg++)
    {
        const char *pszVar = paArgs[iArg].u.pszString;

        /*
         * Look up the variable.
         */
        for (unsigned iVar = 0; iVar < pDbgc->cVars; iVar++)
        {
            if (!strcmp(pszVar, pDbgc->papVars[iVar]->szName))
            {
                /*
                 * Shuffle the array removing this entry.
                 */
                void *pvFree = pDbgc->papVars[iVar];
                if (iVar + 1 < pDbgc->cVars)
                    memmove(&pDbgc->papVars[iVar],
                            &pDbgc->papVars[iVar + 1],
                            (pDbgc->cVars - iVar - 1) * sizeof(pDbgc->papVars[0]));
                pDbgc->papVars[--pDbgc->cVars] = NULL;

                RTMemFree(pvFree);
            }
        } /* lookup */
    } /* arg loop */

    NOREF(pCmd); NOREF(pVM); NOREF(pResult);
    return 0;
}


/**
 * The 'loadvars' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdLoadVars(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    /*
     * Don't trust the parser.
     */
    if (    cArgs != 1
        ||  paArgs[0].enmType != DBGCVAR_TYPE_STRING)
    {
        AssertMsgFailed(("Expected one string exactly!\n"));
        return VERR_PARSE_INCORRECT_ARG_TYPE;
    }

    /*
     * Iterate the variables and unset them.
     */
    FILE *pFile = fopen(paArgs[0].u.pszString, "r");
    if (pFile)
    {
        char szLine[4096];
        while (fgets(szLine, sizeof(szLine), pFile))
        {
            /* Strip it. */
            char *psz = szLine;
            while (isblank(*psz))
                psz++;
            int i = (int)strlen(psz) - 1;
            while (i >= 0 && isspace(psz[i]))
                psz[i--] ='\0';
            /* Execute it if not comment or empty line. */
            if (    *psz != '\0'
                &&  *psz != '#'
                &&  *psz != ';')
            {
                pCmdHlp->pfnPrintf(pCmdHlp, NULL, "dbg: set %s", psz);
                pCmdHlp->pfnExec(pCmdHlp, "set %s", psz);
            }
        }
        fclose(pFile);
    }
    else
        return pCmdHlp->pfnPrintf(pCmdHlp, NULL, "Failed to open file '%s'.\n", paArgs[0].u.pszString);

    NOREF(pCmd); NOREF(pVM); NOREF(pResult);
    return 0;
}


/**
 * The 'showvars' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdShowVars(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    for (unsigned iVar = 0; iVar < pDbgc->cVars; iVar++)
    {
        int rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "%-20s ", &pDbgc->papVars[iVar]->szName);
        if (!rc)
            rc = dbgcCmdFormat(pCmd, pCmdHlp, pVM, &pDbgc->papVars[iVar]->Var, 1, NULL);
        if (rc)
            return rc;
    }

    NOREF(paArgs); NOREF(cArgs); NOREF(pResult);
    return 0;
}


/**
 * The 'harakiri' command.
 *
 * @returns VBox status.
 * @param   pCmd        Pointer to the command descriptor (as registered).
 * @param   pCmdHlp     Pointer to command helper functions.
 * @param   pVM         Pointer to the current VM (if any).
 * @param   paArgs      Pointer to (readonly) array of arguments.
 * @param   cArgs       Number of arguments in the array.
 */
static DECLCALLBACK(int) dbgcCmdHarakiri(PCDBGCCMD pCmd, PDBGCCMDHLP pCmdHlp, PVM pVM, PCDBGCVAR paArgs, unsigned cArgs, PDBGCVAR pResult)
{
    Log(("dbgcCmdHarakiri\n"));
    for (;;)
        exit(126);
    NOREF(pCmd); NOREF(pCmdHlp); NOREF(pVM); NOREF(paArgs); NOREF(cArgs); NOREF(pResult);
}





//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//
//      B u l t i n   S y m b o l s
//
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//



/**
 * Get builtin register symbol.
 *
 * The uUser is special for these symbol descriptors. See the SYMREG_* \#defines.
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pSymDesc    Pointer to the symbol descriptor.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   enmType     The result type.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcSymGetReg(PCDBGCSYM pSymDesc, PDBGCCMDHLP pCmdHlp, DBGCVARTYPE enmType, PDBGCVAR pResult)
{
    LogFlow(("dbgcSymSetReg: pSymDesc->pszName=%d\n", pSymDesc->pszName));

    /*
     * pVM is required.
     */
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    Assert(pDbgc->pVM);

    /*
     * Get the right CPU context.
     */
    PCPUMCTX    pCtx;
    int         rc;
    if (!(pSymDesc->uUser & SYMREG_FLAGS_HYPER))
        rc = CPUMQueryGuestCtxPtr(pDbgc->pVM, &pCtx);
    else
        rc = CPUMQueryHyperCtxPtr(pDbgc->pVM, &pCtx);
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Get the value.
     */
    void       *pvValue = (char *)pCtx + SYMREG_OFFSET(pSymDesc->uUser);
    uint64_t    u64;
    switch (SYMREG_SIZE(pSymDesc->uUser))
    {
        case 1: u64 = *(uint8_t *)pvValue; break;
        case 2: u64 = *(uint16_t *)pvValue; break;
        case 4: u64 = *(uint32_t *)pvValue; break;
        case 6: u64 = *(uint32_t *)pvValue | ((uint64_t)*(uint16_t *)((char *)pvValue + sizeof(uint32_t)) << 32); break;
        case 8: u64 = *(uint64_t *)pvValue; break;
        default:
            return VERR_PARSE_NOT_IMPLEMENTED;
    }

    /*
     * Construct the desired result.
     */
    if (enmType == DBGCVAR_TYPE_ANY)
        enmType = DBGCVAR_TYPE_NUMBER;
    pResult->pDesc          = NULL;
    pResult->pNext          = NULL;
    pResult->enmType        = enmType;
    pResult->enmRangeType   = DBGCVAR_RANGE_NONE;
    pResult->u64Range       = 0;

    switch (enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            pResult->u.GCFlat = (RTGCPTR)u64;
            break;

        case DBGCVAR_TYPE_GC_FAR:
            switch (SYMREG_SIZE(pSymDesc->uUser))
            {
                case 4:
                    if (!(pSymDesc->uUser & SYMREG_FLAGS_HIGH_SEL))
                    {
                        pResult->u.GCFar.off = (uint16_t)u64;
                        pResult->u.GCFar.sel = (uint16_t)(u64 >> 16);
                    }
                    else
                    {
                        pResult->u.GCFar.sel = (uint16_t)u64;
                        pResult->u.GCFar.off = (uint16_t)(u64 >> 16);
                    }
                    break;
                case 6:
                    if (!(pSymDesc->uUser & SYMREG_FLAGS_HIGH_SEL))
                    {
                        pResult->u.GCFar.off = (uint32_t)u64;
                        pResult->u.GCFar.sel = (uint16_t)(u64 >> 32);
                    }
                    else
                    {
                        pResult->u.GCFar.sel = (uint32_t)u64;
                        pResult->u.GCFar.off = (uint16_t)(u64 >> 32);
                    }
                    break;

                default:
                    return VERR_PARSE_BAD_RESULT_TYPE;
            }
            break;

        case DBGCVAR_TYPE_GC_PHYS:
            pResult->u.GCPhys = (RTGCPHYS)u64;
            break;

        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u.pvHCFlat = (void *)(uintptr_t)u64;
            break;

        case DBGCVAR_TYPE_HC_FAR:
            switch (SYMREG_SIZE(pSymDesc->uUser))
            {
                case 4:
                    if (!(pSymDesc->uUser & SYMREG_FLAGS_HIGH_SEL))
                    {
                        pResult->u.HCFar.off = (uint16_t)u64;
                        pResult->u.HCFar.sel = (uint16_t)(u64 >> 16);
                    }
                    else
                    {
                        pResult->u.HCFar.sel = (uint16_t)u64;
                        pResult->u.HCFar.off = (uint16_t)(u64 >> 16);
                    }
                    break;
                case 6:
                    if (!(pSymDesc->uUser & SYMREG_FLAGS_HIGH_SEL))
                    {
                        pResult->u.HCFar.off = (uint32_t)u64;
                        pResult->u.HCFar.sel = (uint16_t)(u64 >> 32);
                    }
                    else
                    {
                        pResult->u.HCFar.sel = (uint32_t)u64;
                        pResult->u.HCFar.off = (uint16_t)(u64 >> 32);
                    }
                    break;

                default:
                    return VERR_PARSE_BAD_RESULT_TYPE;
            }
            break;

        case DBGCVAR_TYPE_HC_PHYS:
            pResult->u.GCPhys = (RTGCPHYS)u64;
            break;

        case DBGCVAR_TYPE_NUMBER:
            pResult->u.u64Number = u64;
            break;

        case DBGCVAR_TYPE_STRING:
        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_BAD_RESULT_TYPE;

    }

    return 0;
}


/**
 * Set builtin register symbol.
 *
 * The uUser is special for these symbol descriptors. See the SYMREG_* #defines.
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pSymDesc    Pointer to the symbol descriptor.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pValue      The value to assign the symbol.
 */
static DECLCALLBACK(int) dbgcSymSetReg(PCDBGCSYM pSymDesc, PDBGCCMDHLP pCmdHlp, PCDBGCVAR pValue)
{
    LogFlow(("dbgcSymSetReg: pSymDesc->pszName=%d\n", pSymDesc->pszName));

    /*
     * pVM is required.
     */
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    Assert(pDbgc->pVM);

    /*
     * Get the right CPU context.
     */
    PCPUMCTX    pCtx;
    int         rc;
    if (!(pSymDesc->uUser & SYMREG_FLAGS_HYPER))
        rc = CPUMQueryGuestCtxPtr(pDbgc->pVM, &pCtx);
    else
        rc = CPUMQueryHyperCtxPtr(pDbgc->pVM, &pCtx);
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Check the new value.
     */
    if (pValue->enmType != DBGCVAR_TYPE_NUMBER)
        return VERR_PARSE_ARGUMENT_TYPE_MISMATCH;

    /*
     * Set the value.
     */
    void       *pvValue = (char *)pCtx + SYMREG_OFFSET(pSymDesc->uUser);
    switch (SYMREG_SIZE(pSymDesc->uUser))
    {
        case 1:
            *(uint8_t *)pvValue  = (uint8_t)pValue->u.u64Number;
            break;
        case 2:
            *(uint16_t *)pvValue = (uint16_t)pValue->u.u64Number;
            break;
        case 4:
            *(uint32_t *)pvValue = (uint32_t)pValue->u.u64Number;
            break;
        case 6:
            *(uint32_t *)pvValue = (uint32_t)pValue->u.u64Number;
            ((uint16_t *)pvValue)[3] = (uint16_t)(pValue->u.u64Number >> 32);
            break;
        case 8:
            *(uint64_t *)pvValue = pValue->u.u64Number;
            break;
        default:
            return VERR_PARSE_NOT_IMPLEMENTED;
    }

    return VINF_SUCCESS;
}







//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//
//      O p e r a t o r s
//
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//


/**
 * Minus (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpMinus(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpMinus\n"));
    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            pResult->u.GCFlat       = -(RTGCINTPTR)pResult->u.GCFlat;
            break;
        case DBGCVAR_TYPE_GC_FAR:
            pResult->u.GCFar.off    = -(int32_t)pResult->u.GCFar.off;
            break;
        case DBGCVAR_TYPE_GC_PHYS:
            pResult->u.GCPhys       = (RTGCPHYS) -(int64_t)pResult->u.GCPhys;
            break;
        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u.pvHCFlat     = (void *) -(intptr_t)pResult->u.pvHCFlat;
            break;
        case DBGCVAR_TYPE_HC_FAR:
            pResult->u.HCFar.off    = -(int32_t)pResult->u.HCFar.off;
            break;
        case DBGCVAR_TYPE_HC_PHYS:
            pResult->u.HCPhys       = (RTHCPHYS) -(int64_t)pResult->u.HCPhys;
            break;
        case DBGCVAR_TYPE_NUMBER:
            pResult->u.u64Number    = -(int64_t)pResult->u.u64Number;
            break;

        case DBGCVAR_TYPE_UNKNOWN:
        case DBGCVAR_TYPE_STRING:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    NOREF(pDbgc);
    return 0;
}


/**
 * Pluss (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpPluss(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpPluss\n"));
    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
        case DBGCVAR_TYPE_GC_FAR:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_FAR:
        case DBGCVAR_TYPE_HC_PHYS:
        case DBGCVAR_TYPE_NUMBER:
            break;

        case DBGCVAR_TYPE_UNKNOWN:
        case DBGCVAR_TYPE_STRING:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    NOREF(pDbgc);
    return 0;
}


/**
 * Boolean not (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBooleanNot(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpBooleanNot\n"));
    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            pResult->u.u64Number    = !pResult->u.GCFlat;
            break;
        case DBGCVAR_TYPE_GC_FAR:
            pResult->u.u64Number    = !pResult->u.GCFar.off && pResult->u.GCFar.sel <= 3;
            break;
        case DBGCVAR_TYPE_GC_PHYS:
            pResult->u.u64Number    = !pResult->u.GCPhys;
            break;
        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u.u64Number    = !pResult->u.pvHCFlat;
            break;
        case DBGCVAR_TYPE_HC_FAR:
            pResult->u.u64Number    = !pResult->u.HCFar.off && pResult->u.HCFar.sel <= 3;
            break;
        case DBGCVAR_TYPE_HC_PHYS:
            pResult->u.u64Number    = !pResult->u.HCPhys;
            break;
        case DBGCVAR_TYPE_NUMBER:
            pResult->u.u64Number    = !pResult->u.u64Number;
            break;
        case DBGCVAR_TYPE_STRING:
            pResult->u.u64Number    = !pResult->u64Range;
            break;

        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    pResult->enmType = DBGCVAR_TYPE_NUMBER;
    NOREF(pDbgc);
    return 0;
}


/**
 * Bitwise not (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBitwiseNot(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpBitwiseNot\n"));
    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            pResult->u.GCFlat       = ~pResult->u.GCFlat;
            break;
        case DBGCVAR_TYPE_GC_FAR:
            pResult->u.GCFar.off    = ~pResult->u.GCFar.off;
            break;
        case DBGCVAR_TYPE_GC_PHYS:
            pResult->u.GCPhys       = ~pResult->u.GCPhys;
            break;
        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u.pvHCFlat     = (void *)~(uintptr_t)pResult->u.pvHCFlat;
            break;
        case DBGCVAR_TYPE_HC_FAR:
            pResult->u.HCFar.off= ~pResult->u.HCFar.off;
            break;
        case DBGCVAR_TYPE_HC_PHYS:
            pResult->u.HCPhys       = ~pResult->u.HCPhys;
            break;
        case DBGCVAR_TYPE_NUMBER:
            pResult->u.u64Number    = ~pResult->u.u64Number;
            break;

        case DBGCVAR_TYPE_UNKNOWN:
        case DBGCVAR_TYPE_STRING:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    NOREF(pDbgc);
    return 0;
}


/**
 * Reference variable (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpVar(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpVar: %s\n", pArg->u.pszString));
    /*
     * Parse sanity.
     */
    if (pArg->enmType != DBGCVAR_TYPE_STRING)
        return VERR_PARSE_INCORRECT_ARG_TYPE;

    /*
     * Lookup the variable.
     */
    const char *pszVar = pArg->u.pszString;
    for (unsigned iVar = 0; iVar < pDbgc->cVars; iVar++)
    {
        if (!strcmp(pszVar, pDbgc->papVars[iVar]->szName))
        {
            *pResult = pDbgc->papVars[iVar]->Var;
            return 0;
        }
    }

    return VERR_PARSE_VARIABLE_NOT_FOUND;
}


/**
 * Flat address (unary).
 *
 * @returns VINF_SUCCESS on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpAddrFlat(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpAddrFlat\n"));
    int     rc;
    *pResult = *pArg;

    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_GC_FAR:
        {
            Assert(pDbgc->pVM);
            DBGFADDRESS Address;
            rc = DBGFR3AddrFromSelOff(pDbgc->pVM, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
            if (VBOX_SUCCESS(rc))
            {
                pResult->enmType = DBGCVAR_TYPE_GC_FLAT;
                pResult->u.GCFlat = Address.FlatPtr;
                return VINF_SUCCESS;
            }
            return VERR_PARSE_CONVERSION_FAILED;
        }

        case DBGCVAR_TYPE_GC_PHYS:
            //rc = MMR3PhysGCPhys2GCVirtEx(pDbgc->pVM, pResult->u.GCPhys, ..., &pResult->u.GCFlat); - yea, sure.
            return VERR_PARSE_INCORRECT_ARG_TYPE;

        case DBGCVAR_TYPE_HC_FLAT:
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_HC_FAR:
            return VERR_PARSE_INCORRECT_ARG_TYPE;

        case DBGCVAR_TYPE_HC_PHYS:
            Assert(pDbgc->pVM);
            pResult->enmType        = DBGCVAR_TYPE_HC_FLAT;
            rc = MMR3HCPhys2HCVirt(pDbgc->pVM, pResult->u.HCPhys, &pResult->u.pvHCFlat);
            if (VBOX_SUCCESS(rc))
                return VINF_SUCCESS;
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_NUMBER:
            pResult->enmType    = DBGCVAR_TYPE_GC_FLAT;
            pResult->u.GCFlat   = (RTGCPTR)pResult->u.u64Number;
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_STRING:
            return dbgcSymbolGet(pDbgc, pArg->u.pszString, DBGCVAR_TYPE_GC_FLAT, pResult);

        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
}


/**
 * Physical address (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpAddrPhys(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpAddrPhys\n"));
    int     rc;

    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
            rc = PGMPhysGCPtr2GCPhys(pDbgc->pVM, pArg->u.GCFlat, &pResult->u.GCPhys);
            if (VBOX_SUCCESS(rc))
                return 0;
            /** @todo more memory types! */
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_GC_FAR:
        {
            Assert(pDbgc->pVM);
            DBGFADDRESS Address;
            rc = DBGFR3AddrFromSelOff(pDbgc->pVM, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
            if (VBOX_SUCCESS(rc))
            {
                pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
                rc = PGMPhysGCPtr2GCPhys(pDbgc->pVM, Address.FlatPtr, &pResult->u.GCPhys);
                if (VBOX_SUCCESS(rc))
                    return 0;
                /** @todo more memory types! */
            }
            return VERR_PARSE_CONVERSION_FAILED;
        }

        case DBGCVAR_TYPE_GC_PHYS:
            return 0;

        case DBGCVAR_TYPE_HC_FLAT:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_GC_PHYS;
            rc = PGMR3DbgHCPtr2GCPhys(pDbgc->pVM, pArg->u.pvHCFlat, &pResult->u.GCPhys);
            if (VBOX_SUCCESS(rc))
                return 0;
            /** @todo more memory types! */
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_HC_FAR:
            return VERR_PARSE_INCORRECT_ARG_TYPE;

        case DBGCVAR_TYPE_HC_PHYS:
            return 0;

        case DBGCVAR_TYPE_NUMBER:
            pResult->enmType    = DBGCVAR_TYPE_GC_PHYS;
            pResult->u.GCPhys   = (RTGCPHYS)pResult->u.u64Number;
            return 0;

        case DBGCVAR_TYPE_STRING:
            return dbgcSymbolGet(pDbgc, pArg->u.pszString, DBGCVAR_TYPE_GC_PHYS, pResult);

        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    return 0;
}


/**
 * Physical host address (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpAddrHostPhys(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpAddrPhys\n"));
    int     rc;

    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
            rc = PGMPhysGCPtr2HCPhys(pDbgc->pVM, pArg->u.GCFlat, &pResult->u.HCPhys);
            if (VBOX_SUCCESS(rc))
                return 0;
            /** @todo more memory types. */
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_GC_FAR:
        {
            Assert(pDbgc->pVM);
            DBGFADDRESS Address;
            rc = DBGFR3AddrFromSelOff(pDbgc->pVM, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
            if (VBOX_SUCCESS(rc))
            {
                pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
                rc = PGMPhysGCPtr2HCPhys(pDbgc->pVM, Address.FlatPtr, &pResult->u.HCPhys);
                if (VBOX_SUCCESS(rc))
                    return 0;
                /** @todo more memory types. */
            }
            return VERR_PARSE_CONVERSION_FAILED;
        }

        case DBGCVAR_TYPE_GC_PHYS:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
            rc = PGMPhysGCPhys2HCPhys(pDbgc->pVM, pArg->u.GCFlat, &pResult->u.HCPhys);
            if (VBOX_SUCCESS(rc))
                return 0;
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_HC_FLAT:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_HC_PHYS;
            rc = PGMR3DbgHCPtr2HCPhys(pDbgc->pVM, pArg->u.pvHCFlat, &pResult->u.HCPhys);
            if (VBOX_SUCCESS(rc))
                return 0;
            /** @todo more memory types! */
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_HC_FAR:
            return VERR_PARSE_INCORRECT_ARG_TYPE;

        case DBGCVAR_TYPE_HC_PHYS:
            return 0;

        case DBGCVAR_TYPE_NUMBER:
            pResult->enmType    = DBGCVAR_TYPE_HC_PHYS;
            pResult->u.HCPhys   = (RTGCPHYS)pResult->u.u64Number;
            return 0;

        case DBGCVAR_TYPE_STRING:
            return dbgcSymbolGet(pDbgc, pArg->u.pszString, DBGCVAR_TYPE_HC_PHYS, pResult);

        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    return 0;
}


/**
 * Host address (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpAddrHost(PDBGC pDbgc, PCDBGCVAR pArg, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpAddrHost\n"));
    int     rc;

    *pResult = *pArg;
    switch (pArg->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_HC_FLAT;
            rc = PGMPhysGCPtr2HCPtr(pDbgc->pVM, pArg->u.GCFlat, &pResult->u.pvHCFlat);
            if (VBOX_SUCCESS(rc))
                return 0;
            /** @todo more memory types. */
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_GC_FAR:
        {
            Assert(pDbgc->pVM);
            DBGFADDRESS Address;
            rc = DBGFR3AddrFromSelOff(pDbgc->pVM, &Address, pArg->u.GCFar.sel, pArg->u.GCFar.off);
            if (VBOX_SUCCESS(rc))
            {
                pResult->enmType = DBGCVAR_TYPE_HC_FLAT;
                rc = PGMPhysGCPtr2HCPtr(pDbgc->pVM, Address.FlatPtr, &pResult->u.pvHCFlat);
                if (VBOX_SUCCESS(rc))
                    return 0;
                /** @todo more memory types. */
            }
            return VERR_PARSE_CONVERSION_FAILED;
        }

        case DBGCVAR_TYPE_GC_PHYS:
            Assert(pDbgc->pVM);
            pResult->enmType = DBGCVAR_TYPE_HC_FLAT;
            rc = PGMPhysGCPhys2HCPtr(pDbgc->pVM, pArg->u.GCPhys, 1, &pResult->u.pvHCFlat);
            if (VBOX_SUCCESS(rc))
                return 0;
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_HC_FLAT:
            return 0;

        case DBGCVAR_TYPE_HC_FAR:
        case DBGCVAR_TYPE_HC_PHYS:
            /** @todo !*/
            return VERR_PARSE_CONVERSION_FAILED;

        case DBGCVAR_TYPE_NUMBER:
            pResult->enmType    = DBGCVAR_TYPE_HC_FLAT;
            pResult->u.pvHCFlat = (void *)(uintptr_t)pResult->u.u64Number;
            return 0;

        case DBGCVAR_TYPE_STRING:
            return dbgcSymbolGet(pDbgc, pArg->u.pszString, DBGCVAR_TYPE_HC_FLAT, pResult);

        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
}

/**
 * Bitwise not (unary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg        The argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpAddrFar(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpAddrFar\n"));
    int     rc;

    switch (pArg1->enmType)
    {
        case DBGCVAR_TYPE_STRING:
            rc = dbgcSymbolGet(pDbgc, pArg1->u.pszString, DBGCVAR_TYPE_NUMBER, pResult);
            if (VBOX_FAILURE(rc))
                return rc;
            break;
        case DBGCVAR_TYPE_NUMBER:
            *pResult = *pArg1;
            break;

        case DBGCVAR_TYPE_GC_FLAT:
        case DBGCVAR_TYPE_GC_FAR:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_FAR:
        case DBGCVAR_TYPE_HC_PHYS:
        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    pResult->u.GCFar.sel = (RTSEL)pResult->u.u64Number;

    /* common code for the two types we support. */
    switch (pArg2->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            pResult->u.GCFar.off = pArg2->u.GCFlat;
            pResult->enmType    = DBGCVAR_TYPE_GC_FAR;
            break;

        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u.HCFar.off = pArg2->u.GCFlat;
            pResult->enmType    = DBGCVAR_TYPE_GC_FAR;
            break;

        case DBGCVAR_TYPE_NUMBER:
            pResult->u.GCFar.off = (RTGCPTR)pArg2->u.u64Number;
            pResult->enmType    = DBGCVAR_TYPE_GC_FAR;
            break;

        case DBGCVAR_TYPE_STRING:
        {
            DBGCVAR Var;
            rc = dbgcSymbolGet(pDbgc, pArg2->u.pszString, DBGCVAR_TYPE_NUMBER, &Var);
            if (VBOX_FAILURE(rc))
                return rc;
            pResult->u.GCFar.off = (RTGCPTR)Var.u.u64Number;
            pResult->enmType    = DBGCVAR_TYPE_GC_FAR;
            break;
        }

        case DBGCVAR_TYPE_GC_FAR:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FAR:
        case DBGCVAR_TYPE_HC_PHYS:
        case DBGCVAR_TYPE_UNKNOWN:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    return 0;

}


/**
 * Multiplication operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpMult(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpMult\n"));
    int     rc;

    /*
     * Switch the factors so we preserve pointers, far pointers are considered more
     * important that physical and flat pointers.
     */
    if (    DBGCVAR_ISPOINTER(pArg2->enmType)
        &&  (   !DBGCVAR_ISPOINTER(pArg1->enmType)
             || (   DBGCVAR_IS_FAR_PTR(pArg2->enmType)
                 && !DBGCVAR_IS_FAR_PTR(pArg1->enmType))))
    {
        PCDBGCVAR pTmp = pArg1;
        pArg2 = pArg1;
        pArg1 = pTmp;
    }

    /*
     * Convert the 2nd number into a number we use multiply the first with.
     */
    DBGCVAR Factor2 = *pArg2;
    if (    Factor2.enmType == DBGCVAR_TYPE_STRING
        ||  Factor2.enmType == DBGCVAR_TYPE_SYMBOL)
    {
        rc = dbgcSymbolGet(pDbgc, pArg2->u.pszString, DBGCVAR_TYPE_NUMBER, &Factor2);
        if (VBOX_FAILURE(rc))
            return rc;
    }
    uint64_t u64Factor;
    switch (Factor2.enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:  u64Factor = Factor2.u.GCFlat; break;
        case DBGCVAR_TYPE_GC_FAR:   u64Factor = Factor2.u.GCFar.off; break;
        case DBGCVAR_TYPE_GC_PHYS:  u64Factor = Factor2.u.GCPhys; break;
        case DBGCVAR_TYPE_HC_FLAT:  u64Factor = (uintptr_t)Factor2.u.pvHCFlat; break;
        case DBGCVAR_TYPE_HC_FAR:   u64Factor = Factor2.u.HCFar.off; break;
        case DBGCVAR_TYPE_HC_PHYS:  u64Factor = Factor2.u.HCPhys; break;
        case DBGCVAR_TYPE_NUMBER:   u64Factor = Factor2.u.u64Number; break;
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }

    /*
     * Fix symbols in the 1st factor.
     */
    *pResult = *pArg1;
    if (    pResult->enmType == DBGCVAR_TYPE_STRING
        ||  pResult->enmType == DBGCVAR_TYPE_SYMBOL)
    {
        rc = dbgcSymbolGet(pDbgc, pArg1->u.pszString, DBGCVAR_TYPE_ANY, pResult);
        if (VBOX_FAILURE(rc))
            return rc;
    }

    /*
     * Do the multiplication.
     */
    switch (pArg1->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:  pResult->u.GCFlat *= u64Factor; break;
        case DBGCVAR_TYPE_GC_FAR:   pResult->u.GCFar.off *= u64Factor; break;
        case DBGCVAR_TYPE_GC_PHYS:  pResult->u.GCPhys *= u64Factor; break;
        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u.pvHCFlat = (void *)(uintptr_t)((uintptr_t)pResult->u.pvHCFlat * u64Factor);
            break;
        case DBGCVAR_TYPE_HC_FAR:   pResult->u.HCFar.off *= u64Factor; break;
        case DBGCVAR_TYPE_HC_PHYS:  pResult->u.HCPhys *= u64Factor; break;
        case DBGCVAR_TYPE_NUMBER:   pResult->u.u64Number *= u64Factor; break;
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
    return 0;
}


/**
 * Division operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpDiv(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpDiv\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Modulus operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpMod(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpMod\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Addition operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpAdd(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpAdd\n"));

    /*
     * An addition operation will return (when possible) the left side type in the
     * expression. We make an omission for numbers, where we'll take the right side
     * type instead. An expression where only the left hand side is a string we'll
     * use the right hand type assuming that the string is a symbol.
     */
    if (    (pArg1->enmType == DBGCVAR_TYPE_NUMBER && pArg2->enmType != DBGCVAR_TYPE_STRING)
        ||  (pArg1->enmType == DBGCVAR_TYPE_STRING && pArg2->enmType != DBGCVAR_TYPE_STRING))
    {
        PCDBGCVAR pTmp = pArg2;
        pArg2 = pArg1;
        pArg1 = pTmp;
    }
    DBGCVAR     Sym1, Sym2;
    if (pArg1->enmType == DBGCVAR_TYPE_STRING)
    {
        int rc = dbgcSymbolGet(pDbgc, pArg1->u.pszString, DBGCVAR_TYPE_ANY, &Sym1);
        if (VBOX_FAILURE(rc))
            return rc;
        pArg1 = &Sym1;

        rc = dbgcSymbolGet(pDbgc, pArg2->u.pszString, DBGCVAR_TYPE_ANY, &Sym2);
        if (VBOX_FAILURE(rc))
            return rc;
        pArg2 = &Sym2;
    }

    int         rc;
    DBGCVAR     Var;
    DBGCVAR     Var2;
    switch (pArg1->enmType)
    {
        /*
         * GC Flat
         */
        case DBGCVAR_TYPE_GC_FLAT:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_INVALID_OPERATION;
                default:
                    *pResult = *pArg1;
                    rc = dbgcOpAddrFlat(pDbgc, pArg2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    pResult->u.GCFlat += pArg2->u.GCFlat;
                    break;
            }
            break;

        /*
         * GC Far
         */
        case DBGCVAR_TYPE_GC_FAR:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_INVALID_OPERATION;
                case DBGCVAR_TYPE_NUMBER:
                    *pResult = *pArg1;
                    pResult->u.GCFar.off += (RTGCPTR)pArg2->u.u64Number;
                    break;
                default:
                    rc = dbgcOpAddrFlat(pDbgc, pArg1, pResult);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    rc = dbgcOpAddrFlat(pDbgc, pArg2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    pResult->u.GCFlat += pArg2->u.GCFlat;
                    break;
            }
            break;

        /*
         * GC Phys
         */
        case DBGCVAR_TYPE_GC_PHYS:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_INVALID_OPERATION;
                default:
                    *pResult = *pArg1;
                    rc = dbgcOpAddrPhys(pDbgc, pArg2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    if (Var.enmType != DBGCVAR_TYPE_GC_PHYS)
                        return VERR_PARSE_INVALID_OPERATION;
                    pResult->u.GCPhys += Var.u.GCPhys;
                    break;
            }
            break;

        /*
         * HC Flat
         */
        case DBGCVAR_TYPE_HC_FLAT:
            *pResult = *pArg1;
            rc = dbgcOpAddrHost(pDbgc, pArg2, &Var2);
            if (VBOX_FAILURE(rc))
                return rc;
            rc = dbgcOpAddrFlat(pDbgc, &Var2, &Var);
            if (VBOX_FAILURE(rc))
                return rc;
            pResult->u.pvHCFlat = (char *)pResult->u.pvHCFlat + (uintptr_t)Var.u.pvHCFlat;
            break;

        /*
         * HC Far
         */
        case DBGCVAR_TYPE_HC_FAR:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_NUMBER:
                    *pResult = *pArg1;
                    pResult->u.HCFar.off += (uintptr_t)pArg2->u.u64Number;
                    break;

                default:
                    rc = dbgcOpAddrFlat(pDbgc, pArg1, pResult);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    rc = dbgcOpAddrHost(pDbgc, pArg2, &Var2);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    rc = dbgcOpAddrFlat(pDbgc, &Var2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    pResult->u.pvHCFlat = (char *)pResult->u.pvHCFlat + (uintptr_t)Var.u.pvHCFlat;
                    break;
            }
            break;

        /*
         * HC Phys
         */
        case DBGCVAR_TYPE_HC_PHYS:
            *pResult = *pArg1;
            rc = dbgcOpAddrHostPhys(pDbgc, pArg2, &Var);
            if (VBOX_FAILURE(rc))
                return rc;
            pResult->u.HCPhys += Var.u.HCPhys;
            break;

        /*
         * Numbers (see start of function)
         */
        case DBGCVAR_TYPE_NUMBER:
            *pResult = *pArg1;
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_SYMBOL:
                case DBGCVAR_TYPE_STRING:
                    rc = dbgcSymbolGet(pDbgc, pArg2->u.pszString, DBGCVAR_TYPE_NUMBER, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                case DBGCVAR_TYPE_NUMBER:
                    pResult->u.u64Number += pArg2->u.u64Number;
                    break;
                default:
                    return VERR_PARSE_INVALID_OPERATION;
            }
            break;

        default:
            return VERR_PARSE_INVALID_OPERATION;

    }
    return 0;
}


/**
 * Subtration operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpSub(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpSub\n"));

    /*
     * An subtraction operation will return the left side type in the expression.
     * However, if the left hand side is a number and the right hand a pointer of
     * some kind we'll convert the left hand side to the same type as the right hand.
     * Any strings will be attempted resolved as symbols.
     */
    DBGCVAR     Sym1, Sym2;
    if (    pArg2->enmType == DBGCVAR_TYPE_STRING
        &&  (   pArg1->enmType == DBGCVAR_TYPE_NUMBER
             || pArg1->enmType == DBGCVAR_TYPE_STRING))
    {
        int rc = dbgcSymbolGet(pDbgc, pArg2->u.pszString, DBGCVAR_TYPE_ANY, &Sym2);
        if (VBOX_FAILURE(rc))
            return rc;
        pArg2 = &Sym2;
    }

    if (pArg1->enmType == DBGCVAR_TYPE_STRING)
    {
        DBGCVARTYPE enmType;
        switch (pArg2->enmType)
        {
            case DBGCVAR_TYPE_NUMBER:
                enmType = DBGCVAR_TYPE_ANY;
                break;
            case DBGCVAR_TYPE_GC_FLAT:
            case DBGCVAR_TYPE_GC_PHYS:
            case DBGCVAR_TYPE_HC_FLAT:
            case DBGCVAR_TYPE_HC_PHYS:
                enmType = pArg2->enmType;
                break;
            case DBGCVAR_TYPE_GC_FAR:
                enmType = DBGCVAR_TYPE_GC_FLAT;
                break;
            case DBGCVAR_TYPE_HC_FAR:
                enmType = DBGCVAR_TYPE_HC_FLAT;
                break;

            default:
            case DBGCVAR_TYPE_STRING:
                AssertMsgFailed(("Can't happen\n"));
                enmType = DBGCVAR_TYPE_STRING;
                break;
        }
        if (enmType != DBGCVAR_TYPE_STRING)
        {
            int rc = dbgcSymbolGet(pDbgc, pArg1->u.pszString, DBGCVAR_TYPE_ANY, &Sym1);
            if (VBOX_FAILURE(rc))
                return rc;
            pArg1 = &Sym1;
        }
    }
    else if (pArg1->enmType == DBGCVAR_TYPE_NUMBER)
    {
        PFNDBGCOPUNARY pOp = NULL;
        switch (pArg2->enmType)
        {
            case DBGCVAR_TYPE_GC_FAR:
            case DBGCVAR_TYPE_GC_FLAT:
                pOp = dbgcOpAddrFlat;
                break;
            case DBGCVAR_TYPE_GC_PHYS:
                pOp = dbgcOpAddrPhys;
                break;
            case DBGCVAR_TYPE_HC_FAR:
            case DBGCVAR_TYPE_HC_FLAT:
                pOp = dbgcOpAddrHost;
                break;
            case DBGCVAR_TYPE_HC_PHYS:
                pOp = dbgcOpAddrHostPhys;
                break;
            case DBGCVAR_TYPE_NUMBER:
                break;
            default:
            case DBGCVAR_TYPE_STRING:
                AssertMsgFailed(("Can't happen\n"));
                break;
        }
        if (pOp)
        {
            int rc = pOp(pDbgc, pArg1, &Sym1);
            if (VBOX_FAILURE(rc))
                return rc;
            pArg1 = &Sym1;
        }
    }


    /*
     * Normal processing.
     */
    int         rc;
    DBGCVAR     Var;
    DBGCVAR     Var2;
    switch (pArg1->enmType)
    {
        /*
         * GC Flat
         */
        case DBGCVAR_TYPE_GC_FLAT:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_INVALID_OPERATION;
                default:
                    *pResult = *pArg1;
                    rc = dbgcOpAddrFlat(pDbgc, pArg2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    pResult->u.GCFlat -= pArg2->u.GCFlat;
                    break;
            }
            break;

        /*
         * GC Far
         */
        case DBGCVAR_TYPE_GC_FAR:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_INVALID_OPERATION;
                case DBGCVAR_TYPE_NUMBER:
                    *pResult = *pArg1;
                    pResult->u.GCFar.off -= (RTGCPTR)pArg2->u.u64Number;
                    break;
                default:
                    rc = dbgcOpAddrFlat(pDbgc, pArg1, pResult);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    rc = dbgcOpAddrFlat(pDbgc, pArg2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    pResult->u.GCFlat -= pArg2->u.GCFlat;
                    break;
            }
            break;

        /*
         * GC Phys
         */
        case DBGCVAR_TYPE_GC_PHYS:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_INVALID_OPERATION;
                default:
                    *pResult = *pArg1;
                    rc = dbgcOpAddrPhys(pDbgc, pArg2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    if (Var.enmType != DBGCVAR_TYPE_GC_PHYS)
                        return VERR_PARSE_INVALID_OPERATION;
                    pResult->u.GCPhys -= Var.u.GCPhys;
                    break;
            }
            break;

        /*
         * HC Flat
         */
        case DBGCVAR_TYPE_HC_FLAT:
            *pResult = *pArg1;
            rc = dbgcOpAddrHost(pDbgc, pArg2, &Var2);
            if (VBOX_FAILURE(rc))
                return rc;
            rc = dbgcOpAddrFlat(pDbgc, &Var2, &Var);
            if (VBOX_FAILURE(rc))
                return rc;
            pResult->u.pvHCFlat = (char *)pResult->u.pvHCFlat - (uintptr_t)Var.u.pvHCFlat;
            break;

        /*
         * HC Far
         */
        case DBGCVAR_TYPE_HC_FAR:
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_NUMBER:
                    *pResult = *pArg1;
                    pResult->u.HCFar.off -= (uintptr_t)pArg2->u.u64Number;
                    break;

                default:
                    rc = dbgcOpAddrFlat(pDbgc, pArg1, pResult);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    rc = dbgcOpAddrHost(pDbgc, pArg2, &Var2);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    rc = dbgcOpAddrFlat(pDbgc, &Var2, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                    pResult->u.pvHCFlat = (char *)pResult->u.pvHCFlat - (uintptr_t)Var.u.pvHCFlat;
                    break;
            }
            break;

        /*
         * HC Phys
         */
        case DBGCVAR_TYPE_HC_PHYS:
            *pResult = *pArg1;
            rc = dbgcOpAddrHostPhys(pDbgc, pArg2, &Var);
            if (VBOX_FAILURE(rc))
                return rc;
            pResult->u.HCPhys -= Var.u.HCPhys;
            break;

        /*
         * Numbers (see start of function)
         */
        case DBGCVAR_TYPE_NUMBER:
            *pResult = *pArg1;
            switch (pArg2->enmType)
            {
                case DBGCVAR_TYPE_SYMBOL:
                case DBGCVAR_TYPE_STRING:
                    rc = dbgcSymbolGet(pDbgc, pArg2->u.pszString, DBGCVAR_TYPE_NUMBER, &Var);
                    if (VBOX_FAILURE(rc))
                        return rc;
                case DBGCVAR_TYPE_NUMBER:
                    pResult->u.u64Number -= pArg2->u.u64Number;
                    break;
                default:
                    return VERR_PARSE_INVALID_OPERATION;
            }
            break;

        default:
            return VERR_PARSE_INVALID_OPERATION;

    }
    return 0;
}


/**
 * Bitwise shift left operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBitwiseShiftLeft(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBitwiseShiftLeft\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Bitwise shift right operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBitwiseShiftRight(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBitwiseShiftRight\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Bitwise and operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBitwiseAnd(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBitwiseAnd\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Bitwise exclusive or operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBitwiseXor(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBitwiseXor\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Bitwise inclusive or operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBitwiseOr(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBitwiseOr\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Boolean and operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBooleanAnd(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBooleanAnd\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Boolean or operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpBooleanOr(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
    LogFlow(("dbgcOpBooleanOr\n"));
    NOREF(pDbgc); NOREF(pArg1); NOREF(pArg2); NOREF(pResult);
    return -1;
}


/**
 * Range to operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpRangeLength(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpRangeLength\n"));
    /*
     * Make result. Strings needs to be resolved into symbols.
     */
    if (pArg1->enmType == DBGCVAR_TYPE_STRING)
    {
        int rc = dbgcSymbolGet(pDbgc, pArg1->u.pszString, DBGCVAR_TYPE_ANY, pResult);
        if (VBOX_FAILURE(rc))
            return rc;
    }
    else
        *pResult = *pArg1;

    /*
     * Convert 2nd argument to element count.
     */
    pResult->enmRangeType = DBGCVAR_RANGE_ELEMENTS;
    switch (pArg2->enmType)
    {
        case DBGCVAR_TYPE_NUMBER:
            pResult->u64Range = pArg2->u.u64Number;
            break;

        case DBGCVAR_TYPE_STRING:
        {
            int rc = dbgcSymbolGet(pDbgc, pArg1->u.pszString, DBGCVAR_TYPE_NUMBER, pResult);
            if (VBOX_FAILURE(rc))
                return rc;
            pResult->u64Range = pArg2->u.u64Number;
            break;
        }

        default:
            return VERR_PARSE_INVALID_OPERATION;
    }

    return VINF_SUCCESS;
}


/**
 * Range to operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpRangeLengthBytes(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpRangeLengthBytes\n"));
    int rc = dbgcOpRangeLength(pDbgc, pArg1, pArg2, pResult);
    if (VBOX_SUCCESS(rc))
        pResult->enmRangeType = DBGCVAR_RANGE_BYTES;
    return rc;
}


/**
 * Range to operator (binary).
 *
 * @returns 0 on success.
 * @returns VBox evaluation / parsing error code on failure.
 *          The caller does the bitching.
 * @param   pDbgc       Debugger console instance data.
 * @param   pArg1       The first argument.
 * @param   pArg2       The 2nd argument.
 * @param   pResult     Where to store the result.
 */
static DECLCALLBACK(int) dbgcOpRangeTo(PDBGC pDbgc, PCDBGCVAR pArg1, PCDBGCVAR pArg2, PDBGCVAR pResult)
{
//    LogFlow(("dbgcOpRangeTo\n"));
    /*
     * Calc number of bytes between the two args.
     */
    DBGCVAR Diff;
    int rc = dbgcOpSub(pDbgc, pArg2, pArg1, &Diff);
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Use the diff as the range of Arg1.
     */
    *pResult = *pArg1;
    pResult->enmRangeType = DBGCVAR_RANGE_BYTES;
    switch (Diff.enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            pResult->u64Range = (RTGCUINTPTR)Diff.u.GCFlat;
            break;
        case DBGCVAR_TYPE_GC_PHYS:
            pResult->u64Range = Diff.u.GCPhys;
            break;
        case DBGCVAR_TYPE_HC_FLAT:
            pResult->u64Range = (uintptr_t)Diff.u.pvHCFlat;
            break;
        case DBGCVAR_TYPE_HC_PHYS:
            pResult->u64Range = Diff.u.HCPhys;
            break;
        case DBGCVAR_TYPE_NUMBER:
            pResult->u64Range = Diff.u.u64Number;
            break;

        case DBGCVAR_TYPE_GC_FAR:
        case DBGCVAR_TYPE_STRING:
        case DBGCVAR_TYPE_HC_FAR:
        default:
            AssertMsgFailed(("Impossible!\n"));
            return VERR_PARSE_INVALID_OPERATION;
    }

    return 0;
}





/**
 * Output callback.
 *
 * @returns number of bytes written.
 * @param   pvArg       User argument.
 * @param   pachChars   Pointer to an array of utf-8 characters.
 * @param   cbChars     Number of bytes in the character array pointed to by pachChars.
 */
static DECLCALLBACK(size_t) dbgcFormatOutput(void *pvArg, const char *pachChars, size_t cbChars)
{
    PDBGC   pDbgc = (PDBGC)pvArg;
    if (cbChars)
    {
        int rc = pDbgc->pBack->pfnWrite(pDbgc->pBack, pachChars, cbChars, NULL);
        if (VBOX_FAILURE(rc))
        {
            pDbgc->rcOutput = rc;
            cbChars = 0;
        }
    }

    return cbChars;
}









//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//
//      C a l l b a c k   H e l p e r s
//
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//



/**
 * Command helper for writing text to the debug console.
 *
 * @returns VBox status.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pvBuf       What to write.
 * @param   cbBuf       Number of bytes to write.
 * @param   pcbWritten  Where to store the number of bytes actually written.
 *                      If NULL the entire buffer must be successfully written.
 */
static DECLCALLBACK(int) dbgcHlpWrite(PDBGCCMDHLP pCmdHlp, const void *pvBuf, size_t cbBuf, size_t *pcbWritten)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    return pDbgc->pBack->pfnWrite(pDbgc->pBack, pvBuf, cbBuf, pcbWritten);
}


/**
 * Command helper for writing formatted text to the debug console.
 *
 * @returns VBox status.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pcb         Where to store the number of bytes written.
 * @param   pszFormat   The format string.
 *                      This is using the log formatter, so it's format extensions can be used.
 * @param   ...         Arguments specified in the format string.
 */
static DECLCALLBACK(int) dbgcHlpPrintf(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten, const char *pszFormat, ...)
{
    /*
     * Do the formatting and output.
     */
    va_list args;
    va_start(args, pszFormat);
    int rc = pCmdHlp->pfnPrintfV(pCmdHlp, pcbWritten, pszFormat, args);
    va_end(args);

    return rc;
}

/**
 * Callback to format non-standard format specifiers.
 *
 * @returns The number of bytes formatted.
 * @param   pvArg           Formatter argument.
 * @param   pfnOutput       Pointer to output function.
 * @param   pvArgOutput     Argument for the output function.
 * @param   ppszFormat      Pointer to the format string pointer. Advance this till the char
 *                          after the format specifier.
 * @param   pArgs           Pointer to the argument list. Use this to fetch the arguments.
 * @param   cchWidth        Format Width. -1 if not specified.
 * @param   cchPrecision    Format Precision. -1 if not specified.
 * @param   fFlags          Flags (RTSTR_NTFS_*).
 * @param   chArgSize       The argument size specifier, 'l' or 'L'.
 */
static DECLCALLBACK(size_t) dbgcStringFormatter(void *pvArg, PFNRTSTROUTPUT pfnOutput, void *pvArgOutput,
                                                const char **ppszFormat, va_list *pArgs, int cchWidth,
                                                int cchPrecision, unsigned fFlags, char chArgSize)
{
    NOREF(cchWidth); NOREF(cchPrecision); NOREF(fFlags); NOREF(chArgSize); NOREF(pvArg);
    if (**ppszFormat != 'D')
    {
        (*ppszFormat)++;
        return 0;
    }

    (*ppszFormat)++;
    switch (**ppszFormat)
    {
        /*
         * Print variable without range.
         * The argument is a const pointer to the variable.
         */
        case 'V':
        {
            (*ppszFormat)++;
            PCDBGCVAR   pVar = va_arg(*pArgs, PCDBGCVAR);
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%VGv", pVar->u.GCFlat);
                case DBGCVAR_TYPE_GC_FAR:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%04x:%08x", pVar->u.GCFar.sel, pVar->u.GCFar.off);
                case DBGCVAR_TYPE_GC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%%%VGp", pVar->u.GCPhys);
                case DBGCVAR_TYPE_HC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%#%VHv", (uintptr_t)pVar->u.pvHCFlat);
                case DBGCVAR_TYPE_HC_FAR:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "#%04x:%08x", pVar->u.HCFar.sel, pVar->u.HCFar.off);
                case DBGCVAR_TYPE_HC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "#%%%%%VHp", pVar->u.HCPhys);
                case DBGCVAR_TYPE_STRING:
                    return pfnOutput(pvArgOutput, pVar->u.pszString, (size_t)pVar->u64Range);
                case DBGCVAR_TYPE_NUMBER:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%llx", pVar->u.u64Number);

                case DBGCVAR_TYPE_UNKNOWN:
                default:
                    return pfnOutput(pvArgOutput, "??", 2);
            }
        }

        /*
         * Print variable with range.
         * The argument is a const pointer to the variable.
         */
        case 'v':
        {
            (*ppszFormat)++;
            PCDBGCVAR   pVar = va_arg(*pArgs, PCDBGCVAR);

            char szRange[32];
            switch (pVar->enmRangeType)
            {
                case DBGCVAR_RANGE_NONE:
                    szRange[0] = '\0';
                    break;
                case DBGCVAR_RANGE_ELEMENTS:
                    RTStrPrintf(szRange, sizeof(szRange), " L %llx", pVar->u64Range);
                    break;
                case DBGCVAR_RANGE_BYTES:
                    RTStrPrintf(szRange, sizeof(szRange), " LB %llx", pVar->u64Range);
                    break;
            }

            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%VGv%s", pVar->u.GCFlat, szRange);
                case DBGCVAR_TYPE_GC_FAR:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%04x:%08x%s", pVar->u.GCFar.sel, pVar->u.GCFar.off, szRange);
                case DBGCVAR_TYPE_GC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%%%%VGp%s", pVar->u.GCPhys, szRange);
                case DBGCVAR_TYPE_HC_FLAT:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%%#%VHv%s", (uintptr_t)pVar->u.pvHCFlat, szRange);
                case DBGCVAR_TYPE_HC_FAR:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "#%04x:%08x%s", pVar->u.HCFar.sel, pVar->u.HCFar.off, szRange);
                case DBGCVAR_TYPE_HC_PHYS:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "#%%%%%VHp%s", pVar->u.HCPhys, szRange);
                case DBGCVAR_TYPE_STRING:
                    return pfnOutput(pvArgOutput, pVar->u.pszString, (size_t)pVar->u64Range);
                case DBGCVAR_TYPE_NUMBER:
                    return RTStrFormat(pfnOutput, pvArgOutput, NULL, 0, "%llx%s", pVar->u.u64Number, szRange);

                case DBGCVAR_TYPE_UNKNOWN:
                default:
                    return pfnOutput(pvArgOutput, "??", 2);
            }
        }

        default:
            AssertMsgFailed(("Invalid format type '%s'!\n", **ppszFormat));
            return 0;
    }
}


/**
 * Command helper for writing formatted text to the debug console.
 *
 * @returns VBox status.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pcb         Where to store the number of bytes written.
 * @param   pszFormat   The format string.
 *                      This is using the log formatter, so it's format extensions can be used.
 * @param   args        Arguments specified in the format string.
 */
static DECLCALLBACK(int) dbgcHlpPrintfV(PDBGCCMDHLP pCmdHlp, size_t *pcbWritten, const char *pszFormat, va_list args)
{
    PDBGC   pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Do the formatting and output.
     */
    pDbgc->rcOutput = 0;
    size_t cb = RTStrFormatV(dbgcFormatOutput, pDbgc, dbgcStringFormatter, pDbgc, pszFormat, args);

    if (pcbWritten)
        *pcbWritten = cb;

    return pDbgc->rcOutput;
}


/**
 * Reports an error from a DBGF call.
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to command helpers.
 * @param   rc          The VBox status code returned by a DBGF call.
 * @param   pszFormat   Format string for additional messages. Can be NULL.
 * @param   ...         Format arguments, optional.
 */
static DECLCALLBACK(int) dbgcHlpVBoxErrorV(PDBGCCMDHLP pCmdHlp, int rc, const char *pszFormat, va_list args)
{
    switch (rc)
    {
        case VINF_SUCCESS:
            break;

        default:
            rc = pCmdHlp->pfnPrintf(pCmdHlp, NULL, "error: %Vrc: %s", rc, pszFormat ? " " : "\n");
            if (VBOX_SUCCESS(rc) && pszFormat)
                rc = pCmdHlp->pfnPrintfV(pCmdHlp, NULL, pszFormat, args);
            break;
    }
    return rc;
}


/**
 * Reports an error from a DBGF call.
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to command helpers.
 * @param   rc          The VBox status code returned by a DBGF call.
 * @param   pszFormat   Format string for additional messages. Can be NULL.
 * @param   ...         Format arguments, optional.
 */
static DECLCALLBACK(int) dbgcHlpVBoxError(PDBGCCMDHLP pCmdHlp, int rc, const char *pszFormat, ...)
{
    va_list args;
    va_start(args, pszFormat);
    int rcRet = pCmdHlp->pfnVBoxErrorV(pCmdHlp, rc, pszFormat, args);
    va_end(args);
    return rcRet;
}


/**
 * Command helper for reading memory specified by a DBGC variable.
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pVM         VM handle if GC or physical HC address.
 * @param   pvBuffer    Where to store the read data.
 * @param   cbRead      Number of bytes to read.
 * @param   pVarPointer DBGC variable specifying where to start reading.
 * @param   pcbRead     Where to store the number of bytes actually read.
 *                      This optional, but it's useful when read GC virtual memory where a
 *                      page in the requested range might not be present.
 *                      If not specified not-present failure or end of a HC physical page
 *                      will cause failure.
 */
static DECLCALLBACK(int) dbgcHlpMemRead(PDBGCCMDHLP pCmdHlp, PVM pVM, void *pvBuffer, size_t cbRead, PCDBGCVAR pVarPointer, size_t *pcbRead)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Dummy check.
     */
    if (cbRead == 0)
    {
        if (*pcbRead)
            *pcbRead = 0;
        return VINF_SUCCESS;
    }

    /*
     * Convert Far addresses getting size and the correct base address.
     * Getting and checking the size is what makes this messy and slow.
     */
    DBGCVAR Var = *pVarPointer;
    switch (pVarPointer->enmType)
    {
        case DBGCVAR_TYPE_GC_FAR:
        {
            /* Use DBGFR3AddrFromSelOff for the conversion. */
            Assert(pDbgc->pVM);
            DBGFADDRESS Address;
            int rc = DBGFR3AddrFromSelOff(pDbgc->pVM, &Address, Var.u.GCFar.sel, Var.u.GCFar.off);
            if (VBOX_FAILURE(rc))
                return rc;

            /* don't bother with flat selectors (for now). */
            if (!DBGFADDRESS_IS_FLAT(&Address))
            {
                SELMSELINFO SelInfo;
                rc = SELMR3GetSelectorInfo(pDbgc->pVM, Address.Sel, &SelInfo);
                if (VBOX_SUCCESS(rc))
                {
                    RTGCUINTPTR cb; /* -1 byte */
                    if (SELMSelInfoIsExpandDown(&SelInfo))
                    {
                        if (    !SelInfo.Raw.Gen.u1Granularity
                            &&  Address.off > UINT16_C(0xffff))
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        if (Address.off <= SelInfo.cbLimit)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cb = (SelInfo.Raw.Gen.u1Granularity ? UINT32_C(0xffffffff) : UINT32_C(0xffff)) - Address.off;
                    }
                    else
                    {
                        if (Address.off > SelInfo.cbLimit)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cb = SelInfo.cbLimit - Address.off;
                    }
                    if (cbRead - 1 > cb)
                    {
                        if (!pcbRead)
                            return VERR_OUT_OF_SELECTOR_BOUNDS;
                        cbRead = cb + 1;
                    }
                }

                Var.enmType = DBGCVAR_TYPE_GC_FLAT;
                Var.u.GCFlat = Address.FlatPtr;
            }
            break;
        }

        case DBGCVAR_TYPE_GC_FLAT:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_PHYS:
            break;

        case DBGCVAR_TYPE_HC_FAR: /* not supported yet! */
        default:
            return VERR_NOT_IMPLEMENTED;
    }



    /*
     * Copy page by page.
     */
    size_t cbLeft = cbRead;
    for (;;)
    {
        /*
         * Calc read size.
         */
        size_t cb = RT_MIN(PAGE_SIZE, cbLeft);
        switch (pVarPointer->enmType)
        {
            case DBGCVAR_TYPE_GC_FLAT: cb = RT_MIN(cb, PAGE_SIZE - (Var.u.GCFlat & PAGE_OFFSET_MASK)); break;
            case DBGCVAR_TYPE_GC_PHYS: cb = RT_MIN(cb, PAGE_SIZE - (Var.u.GCPhys & PAGE_OFFSET_MASK)); break;
            case DBGCVAR_TYPE_HC_FLAT: cb = RT_MIN(cb, PAGE_SIZE - ((uintptr_t)Var.u.pvHCFlat & PAGE_OFFSET_MASK)); break;
            case DBGCVAR_TYPE_HC_PHYS: cb = RT_MIN(cb, PAGE_SIZE - ((size_t)Var.u.HCPhys & PAGE_OFFSET_MASK)); break; /* size_t: MSC has braindead loss of data warnings! */
            default: break;
        }

        /*
         * Perform read.
         */
        int rc;
        switch (Var.enmType)
        {
            case DBGCVAR_TYPE_GC_FLAT:
                rc = MMR3ReadGCVirt(pVM, pvBuffer, Var.u.GCFlat, cb);
                break;
            case DBGCVAR_TYPE_GC_PHYS:
                rc = PGMPhysReadGCPhys(pVM, pvBuffer, Var.u.GCPhys, cb);
                break;

            case DBGCVAR_TYPE_HC_PHYS:
            case DBGCVAR_TYPE_HC_FLAT:
            case DBGCVAR_TYPE_HC_FAR:
            {
                DBGCVAR Var2;
                rc = dbgcOpAddrFlat(pDbgc, &Var, &Var2);
                if (VBOX_SUCCESS(rc))
                {
                    /** @todo protect this!!! */
                    memcpy(pvBuffer, Var2.u.pvHCFlat, cb);
                    rc = 0;
                }
                else
                    rc = VERR_INVALID_POINTER;
                break;
            }

            default:
                rc = VERR_PARSE_INCORRECT_ARG_TYPE;
        }

        /*
         * Check for failure.
         */
        if (VBOX_FAILURE(rc))
        {
            if (pcbRead && (*pcbRead = cbRead - cbLeft) > 0)
                return VINF_SUCCESS;
            return rc;
        }

        /*
         * Next.
         */
        cbLeft -= cb;
        if (!cbLeft)
            break;
        pvBuffer = (char *)pvBuffer + cb;
        rc = pCmdHlp->pfnEval(pCmdHlp, &Var, "%DV + %d", &Var, cb);
        if (VBOX_FAILURE(rc))
        {
            if (pcbRead && (*pcbRead = cbRead - cbLeft) > 0)
                return VINF_SUCCESS;
            return rc;
        }
    }

    /*
     * Done
     */
    if (pcbRead)
        *pcbRead = cbRead;
    return 0;
}

/**
 * Command helper for writing memory specified by a DBGC variable.
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pVM         VM handle if GC or physical HC address.
 * @param   pvBuffer    What to write.
 * @param   cbWrite     Number of bytes to write.
 * @param   pVarPointer DBGC variable specifying where to start reading.
 * @param   pcbWritten  Where to store the number of bytes written.
 *                      This is optional. If NULL be aware that some of the buffer
 *                      might have been written to the specified address.
 */
static DECLCALLBACK(int) dbgcHlpMemWrite(PDBGCCMDHLP pCmdHlp, PVM pVM, const void *pvBuffer, size_t cbWrite, PCDBGCVAR pVarPointer, size_t *pcbWritten)
{
    NOREF(pCmdHlp); NOREF(pVM); NOREF(pvBuffer); NOREF(cbWrite); NOREF(pVarPointer); NOREF(pcbWritten);
    return VERR_NOT_IMPLEMENTED;
}


/**
 * Evaluates an expression.
 * (Hopefully the parser and functions are fully reentrant.)
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pResult     Where to store the result.
 * @param   pszExpr     The expression. Format string with the format DBGC extensions.
 * @param   ...         Format arguments.
 */
static DECLCALLBACK(int) dbgcHlpEval(PDBGCCMDHLP pCmdHlp, PDBGCVAR pResult, const char *pszExpr, ...)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);

    /*
     * Format the expression.
     */
    char szExprFormatted[2048];
    va_list args;
    va_start(args, pszExpr);
    size_t cb = RTStrPrintfExV(dbgcStringFormatter, pDbgc, szExprFormatted, sizeof(szExprFormatted), pszExpr, args);
    va_end(args);
    /* ignore overflows. */

    return dbgcEvalSub(pDbgc, &szExprFormatted[0], cb, pResult);
}


/**
 * Executes one command expression.
 * (Hopefully the parser and functions are fully reentrant.)
 *
 * @returns VBox status code appropriate to return from a command.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pszExpr     The expression. Format string with the format DBGC extensions.
 * @param   ...         Format arguments.
 */
static DECLCALLBACK(int) dbgcHlpExec(PDBGCCMDHLP pCmdHlp, const char *pszExpr, ...)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    /* Save the scratch state. */
    char       *pszScratch  = pDbgc->pszScratch;
    unsigned    iArg        = pDbgc->iArg;

    /*
     * Format the expression.
     */
    va_list args;
    va_start(args, pszExpr);
    size_t cbScratch = sizeof(pDbgc->achScratch) - (pDbgc->pszScratch - &pDbgc->achScratch[0]);
    size_t cb = RTStrPrintfExV(dbgcStringFormatter, pDbgc, pDbgc->pszScratch, cbScratch, pszExpr, args);
    va_end(args);
    if (cb >= cbScratch)
        return VERR_BUFFER_OVERFLOW;

    /*
     * Execute the command.
     * We save and restore the arg index and scratch buffer pointer.
     */
    pDbgc->pszScratch = pDbgc->pszScratch + cb + 1;
    int rc = dbgcProcessCommand(pDbgc, pszScratch, cb);

    /* Restore the scratch state. */
    pDbgc->iArg         = iArg;
    pDbgc->pszScratch   = pszScratch;

    return rc;
}


/**
 * Converts a DBGC variable to a DBGF address structure.
 *
 * @returns VBox status code.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pVar        The variable to convert.
 * @param   pAddress    The target address.
 */
static DECLCALLBACK(int) dbgcHlpVarToDbgfAddr(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, PDBGFADDRESS pAddress)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    return dbgcVarToDbgfAddr(pDbgc, pVar, pAddress);
}


/**
 * Converts a DBGC variable to a boolean.
 *
 * @returns VBox status code.
 * @param   pCmdHlp     Pointer to the command callback structure.
 * @param   pVar        The variable to convert.
 * @param   pf          Where to store the boolean.
 */
static DECLCALLBACK(int) dbgcHlpVarToBool(PDBGCCMDHLP pCmdHlp, PCDBGCVAR pVar, bool *pf)
{
    PDBGC pDbgc = DBGC_CMDHLP2DBGC(pCmdHlp);
    NOREF(pDbgc);

    switch (pVar->enmType)
    {
        case DBGCVAR_TYPE_STRING:
            /** @todo add strcasecmp / stricmp wrappers to iprt/string.h. */
            if (    !strcmp(pVar->u.pszString, "true")
                ||  !strcmp(pVar->u.pszString, "True")
                ||  !strcmp(pVar->u.pszString, "TRUE")
                ||  !strcmp(pVar->u.pszString, "on")
                ||  !strcmp(pVar->u.pszString, "On")
                ||  !strcmp(pVar->u.pszString, "oN")
                ||  !strcmp(pVar->u.pszString, "ON")
                ||  !strcmp(pVar->u.pszString, "enabled")
                ||  !strcmp(pVar->u.pszString, "Enabled")
                ||  !strcmp(pVar->u.pszString, "DISABLED"))
            {
                *pf = true;
                return VINF_SUCCESS;
            }
            if (    !strcmp(pVar->u.pszString, "false")
                ||  !strcmp(pVar->u.pszString, "False")
                ||  !strcmp(pVar->u.pszString, "FALSE")
                ||  !strcmp(pVar->u.pszString, "off")
                ||  !strcmp(pVar->u.pszString, "Off")
                ||  !strcmp(pVar->u.pszString, "OFF")
                ||  !strcmp(pVar->u.pszString, "disabled")
                ||  !strcmp(pVar->u.pszString, "Disabled")
                ||  !strcmp(pVar->u.pszString, "DISABLED"))
            {
                *pf = false;
                return VINF_SUCCESS;
            }
            return VERR_PARSE_INCORRECT_ARG_TYPE; /** @todo better error code! */

        case DBGCVAR_TYPE_GC_FLAT:
        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_PHYS:
        case DBGCVAR_TYPE_NUMBER:
            *pf = pVar->u.u64Number != 0;
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_HC_FAR:
        case DBGCVAR_TYPE_GC_FAR:
        case DBGCVAR_TYPE_SYMBOL:
        default:
            return VERR_PARSE_INCORRECT_ARG_TYPE;
    }
}





//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//
//      V a r i a b l e   M a n i p u l a t i o n
//
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//



/** @todo move me!*/
void dbgcVarSetGCFlat(PDBGCVAR pVar, RTGCPTR GCFlat)
{
    if (pVar)
    {
        pVar->enmType  = DBGCVAR_TYPE_GC_FLAT;
        pVar->u.GCFlat = GCFlat;
        pVar->enmRangeType = DBGCVAR_RANGE_NONE;
        pVar->u64Range  = 0;
    }
}


/** @todo move me!*/
void dbgcVarSetGCFlatByteRange(PDBGCVAR pVar, RTGCPTR GCFlat, uint64_t cb)
{
    if (pVar)
    {
        pVar->enmType  = DBGCVAR_TYPE_GC_FLAT;
        pVar->u.GCFlat = GCFlat;
        pVar->enmRangeType = DBGCVAR_RANGE_BYTES;
        pVar->u64Range  = cb;
    }
}


/** @todo move me!*/
void dbgcVarSetVar(PDBGCVAR pVar, PCDBGCVAR pVar2)
{
    if (pVar)
    {
        if (pVar2)
            *pVar = *pVar2;
        else
        {
            pVar->enmType = DBGCVAR_TYPE_UNKNOWN;
            memset(&pVar->u, 0, sizeof(pVar->u));
            pVar->enmRangeType = DBGCVAR_RANGE_NONE;
            pVar->u64Range  = 0;
        }
    }
}


/** @todo move me!*/
void dbgcVarSetByteRange(PDBGCVAR pVar, uint64_t cb)
{
    if (pVar)
    {
        pVar->enmRangeType = DBGCVAR_RANGE_BYTES;
        pVar->u64Range  = cb;
    }
}


/** @todo move me!*/
void dbgcVarSetNoRange(PDBGCVAR pVar)
{
    if (pVar)
    {
        pVar->enmRangeType = DBGCVAR_RANGE_NONE;
        pVar->u64Range  = 0;
    }
}


/**
 * Converts a DBGC variable to a DBGF address.
 *
 * @returns VBox status code.
 * @param   pDbgc       The DBGC instance.
 * @param   pVar        The variable.
 * @param   pAddress    Where to store the address.
 */
static int dbgcVarToDbgfAddr(PDBGC pDbgc, PCDBGCVAR pVar, PDBGFADDRESS pAddress)
{
    AssertReturn(pVar, VERR_INVALID_PARAMETER);
    switch (pVar->enmType)
    {
        case DBGCVAR_TYPE_GC_FLAT:
            DBGFR3AddrFromFlat(pDbgc->pVM, pAddress, pVar->u.GCFlat);
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_NUMBER:
            DBGFR3AddrFromFlat(pDbgc->pVM, pAddress, (RTGCUINTPTR)pVar->u.u64Number);
            return VINF_SUCCESS;

        case DBGCVAR_TYPE_GC_FAR:
            return DBGFR3AddrFromSelOff(pDbgc->pVM, pAddress, pVar->u.GCFar.sel, pVar->u.GCFar.sel);

        case DBGCVAR_TYPE_STRING:
        case DBGCVAR_TYPE_SYMBOL:
        {
            DBGCVAR Var;
            int rc = pDbgc->CmdHlp.pfnEval(&pDbgc->CmdHlp, &Var, "%%(%DV)", pVar);
            if (VBOX_FAILURE(rc))
                return rc;
            return dbgcVarToDbgfAddr(pDbgc, &Var, pAddress);
        }

        case DBGCVAR_TYPE_GC_PHYS:
        case DBGCVAR_TYPE_HC_FLAT:
        case DBGCVAR_TYPE_HC_FAR:
        case DBGCVAR_TYPE_HC_PHYS:
        default:
            return VERR_PARSE_CONVERSION_FAILED;
    }
}



//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//
//      B r e a k p o i n t   M a n a g e m e n t
//
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//


/**
 * Adds a breakpoint to the DBGC breakpoint list.
 */
int dbgcBpAdd(PDBGC pDbgc, RTUINT iBp, const char *pszCmd)
{
    /*
     * Check if it already exists.
     */
    PDBGCBP pBp = dbgcBpGet(pDbgc, iBp);
    if (pBp)
        return VERR_DBGC_BP_EXISTS;

    /*
     * Add the breakpoint.
     */
    if (pszCmd)
        pszCmd = RTStrStripL(pszCmd);
    size_t cchCmd = pszCmd ? strlen(pszCmd) : 0;
    pBp = (PDBGCBP)RTMemAlloc(RT_OFFSETOF(DBGCBP, szCmd[cchCmd + 1]));
    if (!pBp)
        return VERR_NO_MEMORY;
    if (cchCmd)
        memcpy(pBp->szCmd, pszCmd, cchCmd + 1);
    else
        pBp->szCmd[0] = '\0';
    pBp->cchCmd = cchCmd;
    pBp->iBp    = iBp;
    pBp->pNext  = pDbgc->pFirstBp;
    pDbgc->pFirstBp = pBp;

    return VINF_SUCCESS;
}

/**
 * Updates the a breakpoint.
 *
 * @returns VBox status code.
 * @param   pDbgc       The DBGC instance.
 * @param   iBp         The breakpoint to update.
 * @param   pszCmd      The new command.
 */
int dbgcBpUpdate(PDBGC pDbgc, RTUINT iBp, const char *pszCmd)
{
    /*
     * Find the breakpoint.
     */
    PDBGCBP pBp = dbgcBpGet(pDbgc, iBp);
    if (!pBp)
        return VERR_DBGC_BP_NOT_FOUND;

    /*
     * Do we need to reallocate?
     */
    if (pszCmd)
        pszCmd = RTStrStripL(pszCmd);
    if (!pszCmd || !*pszCmd)
        pBp->szCmd[0] = '\0';
    else
    {
        size_t cchCmd = strlen(pszCmd);
        if (strlen(pBp->szCmd) >= cchCmd)
        {
            memcpy(pBp->szCmd, pszCmd, cchCmd + 1);
            pBp->cchCmd = cchCmd;
        }
        else
        {
            /*
             * Yes, let's do it the simple way...
             */
            int rc = dbgcBpDelete(pDbgc, iBp);
            AssertRC(rc);
            return dbgcBpAdd(pDbgc, iBp, pszCmd);
        }
    }
    return VINF_SUCCESS;
}


/**
 * Deletes a breakpoint.
 *
 * @returns VBox status code.
 * @param   pDbgc       The DBGC instance.
 * @param   iBp         The breakpoint to delete.
 */
int dbgcBpDelete(PDBGC pDbgc, RTUINT iBp)
{
    /*
     * Search thru the list, when found unlink and free it.
     */
    PDBGCBP pBpPrev = NULL;
    PDBGCBP pBp = pDbgc->pFirstBp;
    for (; pBp; pBp = pBp->pNext)
    {
        if (pBp->iBp == iBp)
        {
            if (pBpPrev)
                pBpPrev->pNext = pBp->pNext;
            else
                pDbgc->pFirstBp = pBp->pNext;
            RTMemFree(pBp);
            return VINF_SUCCESS;
        }
        pBpPrev = pBp;
    }

    return VERR_DBGC_BP_NOT_FOUND;
}


/**
 * Get a breakpoint.
 *
 * @returns Pointer to the breakpoint.
 * @returns NULL if the breakpoint wasn't found.
 * @param   pDbgc       The DBGC instance.
 * @param   iBp         The breakpoint to get.
 */
PDBGCBP dbgcBpGet(PDBGC pDbgc, RTUINT iBp)
{
    /*
     * Enumerate the list.
     */
    PDBGCBP pBp = pDbgc->pFirstBp;
    for (; pBp; pBp = pBp->pNext)
        if (pBp->iBp == iBp)
            return pBp;
    return NULL;
}


/**
 * Executes the command of a breakpoint.
 *
 * @returns VINF_DBGC_BP_NO_COMMAND if there is no command associated with the breakpoint.
 * @returns VERR_DBGC_BP_NOT_FOUND if the breakpoint wasn't found.
 * @returns VERR_BUFFER_OVERFLOW if the is not enough space in the scratch buffer for the command.
 * @returns VBox status code from dbgcProcessCommand() other wise.
 * @param   pDbgc       The DBGC instance.
 * @param   iBp         The breakpoint to execute.
 */
int dbgcBpExec(PDBGC pDbgc, RTUINT iBp)
{
    /*
     * Find the breakpoint.
     */
    PDBGCBP pBp = dbgcBpGet(pDbgc, iBp);
    if (!pBp)
        return VERR_DBGC_BP_NOT_FOUND;

    /*
     * Anything to do?
     */
    if (!pBp->cchCmd)
        return VINF_DBGC_BP_NO_COMMAND;

    /*
     * Execute the command.
     * This means copying it to the scratch buffer and process it as if it
     * were user input. We must save and restore the state of the scratch buffer.
     */
    /* Save the scratch state. */
    char       *pszScratch  = pDbgc->pszScratch;
    unsigned    iArg        = pDbgc->iArg;

    /* Copy the command to the scratch buffer. */
    size_t cbScratch = sizeof(pDbgc->achScratch) - (pDbgc->pszScratch - &pDbgc->achScratch[0]);
    if (pBp->cchCmd >= cbScratch)
        return VERR_BUFFER_OVERFLOW;
    memcpy(pDbgc->pszScratch, pBp->szCmd, pBp->cchCmd + 1);

    /* Execute the command. */
    pDbgc->pszScratch = pDbgc->pszScratch + pBp->cchCmd + 1;
    int rc = dbgcProcessCommand(pDbgc, pszScratch, pBp->cchCmd);

    /* Restore the scratch state. */
    pDbgc->iArg         = iArg;
    pDbgc->pszScratch   = pszScratch;

    return rc;
}





//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
//
//
//      I n p u t ,   p a r s i n g   a n d   l o g g i n g
//
//
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//



/**
 * Prints any log lines from the log buffer.
 *
 * The caller must not call function this unless pDbgc->fLog is set.
 *
 * @returns VBox status. (output related)
 * @param   pDbgc   Debugger console instance data.
 */
static int dbgcProcessLog(PDBGC pDbgc)
{
    /** @todo */
    NOREF(pDbgc);
    return 0;
}



/**
 * Handle input buffer overflow.
 *
 * Will read any available input looking for a '\n' to reset the buffer on.
 *
 * @returns VBox status.
 * @param   pDbgc   Debugger console instance data.
 */
static int dbgcInputOverflow(PDBGC pDbgc)
{
    /*
     * Assert overflow status and reset the input buffer.
     */
    if (!pDbgc->fInputOverflow)
    {
        pDbgc->fInputOverflow = true;
        pDbgc->iRead = pDbgc->iWrite = 0;
        pDbgc->cInputLines = 0;
        pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "Input overflow!!\n");
    }

    /*
     * Eat input till no more or there is a '\n'.
     * When finding a '\n' we'll continue normal processing.
     */
    while (pDbgc->pBack->pfnInput(pDbgc->pBack, 0))
    {
        size_t cbRead;
        int rc = pDbgc->pBack->pfnRead(pDbgc->pBack, &pDbgc->achInput[0], sizeof(pDbgc->achInput) - 1, &cbRead);
        if (VBOX_FAILURE(rc))
            return rc;
        char *psz = (char *)memchr(&pDbgc->achInput[0], '\n', cbRead);
        if (psz)
        {
            pDbgc->fInputOverflow = false;
            pDbgc->iRead = psz - &pDbgc->achInput[0] + 1;
            pDbgc->iWrite = (unsigned)cbRead;
            pDbgc->cInputLines = 0;
            break;
        }
    }

    return 0;
}



/**
 * Read input and do some preprocessing.
 *
 * @returns VBox status.
 *          In addition to the iWrite and achInput, cInputLines is maintained.
 *          In case of an input overflow the fInputOverflow flag will be set.
 * @param   pDbgc   Debugger console instance data.
 */
static int dbgcInputRead(PDBGC pDbgc)
{
    /*
     * We have ready input.
     * Read it till we don't have any or we have a full input buffer.
     */
    int     rc = 0;
    do
    {
        /*
         * More available buffer space?
         */
        size_t cbLeft;
        if (pDbgc->iWrite > pDbgc->iRead)
            cbLeft = sizeof(pDbgc->achInput) - pDbgc->iWrite - (pDbgc->iRead == 0);
        else
            cbLeft = pDbgc->iRead - pDbgc->iWrite - 1;
        if (!cbLeft)
        {
            /* overflow? */
            if (!pDbgc->cInputLines)
                rc = dbgcInputOverflow(pDbgc);
            break;
        }

        /*
         * Read one char and interpret it.
         */
        char    achRead[128];
        size_t  cbRead;
        rc = pDbgc->pBack->pfnRead(pDbgc->pBack, &achRead[0], RT_MIN(cbLeft, sizeof(achRead)), &cbRead);
        if (VBOX_FAILURE(rc))
            return rc;
        char *psz = &achRead[0];
        while (cbRead-- > 0)
        {
            char ch = *psz++;
            switch (ch)
            {
                /*
                 * Ignore.
                 */
                case '\0':
                case '\r':
                case '\a':
                    break;

                /*
                 * Backspace.
                 */
                case '\b':
                    Log2(("DBGC: backspace\n"));
                    if (pDbgc->iRead != pDbgc->iWrite)
                    {
                        unsigned iWriteUndo = pDbgc->iWrite;
                        if (pDbgc->iWrite)
                            pDbgc->iWrite--;
                        else
                            pDbgc->iWrite = sizeof(pDbgc->achInput) - 1;

                        if (pDbgc->achInput[pDbgc->iWrite] == '\n')
                            pDbgc->iWrite = iWriteUndo;
                    }
                    break;

                /*
                 * Add char to buffer.
                 */
                case '\t':
                case '\n':
                case ';':
                    switch (ch)
                    {
                        case '\t': ch = ' '; break;
                        case '\n': pDbgc->cInputLines++; break;
                    }
                default:
                    Log2(("DBGC: ch=%02x\n", (unsigned char)ch));
                    pDbgc->achInput[pDbgc->iWrite] = ch;
                    if (++pDbgc->iWrite >= sizeof(pDbgc->achInput))
                        pDbgc->iWrite = 0;
                    break;
            }
        }

        /* Terminate it to make it easier to read in the debugger. */
        pDbgc->achInput[pDbgc->iWrite] = '\0';
    } while (pDbgc->pBack->pfnInput(pDbgc->pBack, 0));

    return rc;
}


/**
 * Finds a builtin symbol.
 * @returns Pointer to symbol descriptor on success.
 * @returns NULL on failure.
 * @param   pDbgc       The debug console instance.
 * @param   pszSymbol   The symbol name.
 */
PCDBGCSYM dbgcLookupRegisterSymbol(PDBGC pDbgc, const char *pszSymbol)
{
    for (unsigned iSym = 0; iSym < ELEMENTS(g_aSyms); iSym++)
        if (!strcmp(pszSymbol, g_aSyms[iSym].pszName))
            return &g_aSyms[iSym];

    /** @todo externally registered symbols. */
    NOREF(pDbgc);
    return NULL;
}


/**
 * Resolves a symbol (or tries to do so at least).
 *
 * @returns 0 on success.
 * @returns VBox status on failure.
 * @param   pDbgc       The debug console instance.
 * @param   pszSymbol   The symbol name.
 * @param   enmType     The result type.
 * @param   pResult     Where to store the result.
 */
static int dbgcSymbolGet(PDBGC pDbgc, const char *pszSymbol, DBGCVARTYPE enmType, PDBGCVAR pResult)
{
    /*
     * Builtin?
     */
    PCDBGCSYM   pSymDesc = dbgcLookupRegisterSymbol(pDbgc, pszSymbol);
    if (pSymDesc)
    {
        if (!pSymDesc->pfnGet)
            return VERR_PARSE_WRITEONLY_SYMBOL;
        return pSymDesc->pfnGet(pSymDesc, &pDbgc->CmdHlp, enmType, pResult);
    }


    /*
     * Ask PDM.
     */
    /** @todo resolve symbols using PDM. */


    /*
     * Ask the debug info manager.
     */
    DBGFSYMBOL Symbol;
    int rc = DBGFR3SymbolByName(pDbgc->pVM, pszSymbol, &Symbol);
    if (VBOX_SUCCESS(rc))
    {
        /*
         * Default return is a flat gc address.
         */
        memset(pResult, 0,  sizeof(*pResult));
        pResult->enmRangeType = Symbol.cb ? DBGCVAR_RANGE_BYTES : DBGCVAR_RANGE_NONE;
        pResult->u64Range     = Symbol.cb;
        pResult->enmType      = DBGCVAR_TYPE_GC_FLAT;
        pResult->u.GCFlat     = Symbol.Value;
        DBGCVAR VarTmp;
        switch (enmType)
        {
            /* nothing to do. */
            case DBGCVAR_TYPE_GC_FLAT:
            case DBGCVAR_TYPE_GC_FAR:
            case DBGCVAR_TYPE_ANY:
                return VINF_SUCCESS;

            /* simply make it numeric. */
            case DBGCVAR_TYPE_NUMBER:
                pResult->enmType = DBGCVAR_TYPE_NUMBER;
                pResult->u.u64Number = Symbol.Value;
                return VINF_SUCCESS;

            /* cast it. */

            case DBGCVAR_TYPE_GC_PHYS:
                VarTmp = *pResult;
                return dbgcOpAddrPhys(pDbgc, &VarTmp, pResult);

            case DBGCVAR_TYPE_HC_FAR:
            case DBGCVAR_TYPE_HC_FLAT:
                VarTmp = *pResult;
                return dbgcOpAddrHost(pDbgc, &VarTmp, pResult);

            case DBGCVAR_TYPE_HC_PHYS:
                VarTmp = *pResult;
                return dbgcOpAddrHostPhys(pDbgc, &VarTmp, pResult);

            default:
                AssertMsgFailed(("Internal error enmType=%d\n", enmType));
                return VERR_INVALID_PARAMETER;
        }
    }

    return VERR_PARSE_NOT_IMPLEMENTED;
}



/**
 * Finds a routine.
 *
 * @returns Pointer to the command descriptor.
 *          If the request was for an external command, the caller is responsible for
 *          unlocking the external command list.
 * @returns NULL if not found.
 * @param   pDbgc       The debug console instance.
 * @param   pachName    Pointer to the routine string (not terminated).
 * @param   cchName     Length of the routine name.
 * @param   fExternal   Whether or not the routine is external.
 */
static PCDBGCCMD dbgcRoutineLookup(PDBGC pDbgc, const char *pachName, size_t cchName, bool fExternal)
{
    if (!fExternal)
    {
        /* emulation first, so commands can be overloaded (info ++). */
        PCDBGCCMD pCmd = pDbgc->paEmulationCmds;
        unsigned cLeft = pDbgc->cEmulationCmds;
        while (cLeft-- > 0)
        {
            if (    !strncmp(pachName, pCmd->pszCmd, cchName)
                &&  !pCmd->pszCmd[cchName])
                return pCmd;
            pCmd++;
        }

        for (unsigned iCmd = 0; iCmd < ELEMENTS(g_aCmds); iCmd++)
        {
            if (    !strncmp(pachName, g_aCmds[iCmd].pszCmd, cchName)
                &&  !g_aCmds[iCmd].pszCmd[cchName])
                return &g_aCmds[iCmd];
        }
    }
    else
    {
        DBGCEXTCMDS_LOCK_RD();
        for (PDBGCEXTCMDS pExtCmds = g_pExtCmdsHead; pExtCmds; pExtCmds = pExtCmds->pNext)
        {
            for (unsigned iCmd = 0; iCmd < pExtCmds->cCmds; iCmd++)
            {
                if (    !strncmp(pachName, pExtCmds->paCmds[iCmd].pszCmd, cchName)
                    &&  !pExtCmds->paCmds[iCmd].pszCmd[cchName])
                    return &pExtCmds->paCmds[iCmd];
            }
        }
        DBGCEXTCMDS_UNLOCK_RD();
    }

    NOREF(pDbgc);
    return NULL;
}


/**
 * Searches for an operator descriptor which matches the start of
 * the expression given us.
 *
 * @returns Pointer to the operator on success.
 * @param   pDbgc           The debug console instance.
 * @param   pszExpr         Pointer to the expression string which might start with an operator.
 * @param   fPreferBinary   Whether to favour binary or unary operators.
 *                          Caller must assert that it's the disired type! Both types will still
 *                          be returned, this is only for resolving duplicates.
 * @param   chPrev          The previous char. Some operators requires a blank in front of it.
 */
static PCDBGCOP dbgcOperatorLookup(PDBGC pDbgc, const char *pszExpr, bool fPreferBinary, char chPrev)
{
    PCDBGCOP    pOp = NULL;
    for (unsigned iOp = 0; iOp < ELEMENTS(g_aOps); iOp++)
    {
        if (     g_aOps[iOp].szName[0] == pszExpr[0]
            &&  (!g_aOps[iOp].szName[1] || g_aOps[iOp].szName[1] == pszExpr[1])
            &&  (!g_aOps[iOp].szName[2] || g_aOps[iOp].szName[2] == pszExpr[2]))
        {
            /*
             * Check that we don't mistake it for some other operator which have more chars.
             */
            unsigned j;
            for (j = iOp + 1; j < ELEMENTS(g_aOps); j++)
                if (    g_aOps[j].cchName > g_aOps[iOp].cchName
                    &&  g_aOps[j].szName[0] == pszExpr[0]
                    &&  (!g_aOps[j].szName[1] || g_aOps[j].szName[1] == pszExpr[1])
                    &&  (!g_aOps[j].szName[2] || g_aOps[j].szName[2] == pszExpr[2]) )
                    break;
            if (j < ELEMENTS(g_aOps))
                continue;       /* we'll catch it later. (for theoretical +,++,+++ cases.) */
            pOp = &g_aOps[iOp];

            /*
             * Prefered type?
             */
            if (g_aOps[iOp].fBinary == fPreferBinary)
                break;
        }
    }

    if (pOp)
        Log2(("dbgcOperatorLookup: pOp=%p %s\n", pOp, pOp->szName));
    NOREF(pDbgc); NOREF(chPrev);
    return pOp;
}


/**
 * Initalizes g_bmOperatorChars.
 */
static void dbgcInitOpCharBitMap(void)
{
    memset(g_bmOperatorChars, 0, sizeof(g_bmOperatorChars));
    for (unsigned iOp = 0; iOp < RT_ELEMENTS(g_aOps); iOp++)
        ASMBitSet(&g_bmOperatorChars[0], (uint8_t)g_aOps[iOp].szName[0]);
}


/**
 * Checks whether the character may be the start of an operator.
 *
 * @returns true/false.
 * @param   ch      The character.
 */
DECLINLINE(bool) dbgcIsOpChar(char ch)
{
    return ASMBitTest(&g_bmOperatorChars[0], (uint8_t)ch);
}


static int dbgcEvalSubString(PDBGC pDbgc, char *pszExpr, size_t cchExpr, PDBGCVAR pArg)
{
    Log2(("dbgcEvalSubString: cchExpr=%d pszExpr=%s\n", cchExpr, pszExpr));

    /*
     * Removing any quoting and escapings.
     */
    char ch = *pszExpr;
    if (ch == '"' || ch == '\'' || ch == '`')
    {
        if (pszExpr[--cchExpr] != ch)
            return VERR_PARSE_UNBALANCED_QUOTE;
        cchExpr--;
        pszExpr++;

        /** @todo string unescaping. */
    }
    pszExpr[cchExpr] = '\0';

    /*
     * Make the argument.
     */
    pArg->pDesc         = NULL;
    pArg->pNext         = NULL;
    pArg->enmType       = DBGCVAR_TYPE_STRING;
    pArg->u.pszString   = pszExpr;
    pArg->enmRangeType  = DBGCVAR_RANGE_BYTES;
    pArg->u64Range      = cchExpr;

    NOREF(pDbgc);
    return 0;
}


static int dbgcEvalSubNum(char *pszExpr, unsigned uBase, PDBGCVAR pArg)
{
    Log2(("dbgcEvalSubNum: uBase=%d pszExpr=%s\n", uBase, pszExpr));
    /*
     * Convert to number.
     */
    uint64_t    u64 = 0;
    char        ch;
    while ((ch = *pszExpr) != '\0')
    {
        uint64_t    u64Prev = u64;
        unsigned    u = ch - '0';
        if (u < 10 && u < uBase)
            u64 = u64 * uBase + u;
        else if (ch >= 'a' && (u = ch - ('a' - 10)) < uBase)
            u64 = u64 * uBase + u;
        else if (ch >= 'A' && (u = ch - ('A' - 10)) < uBase)
            u64 = u64 * uBase + u;
        else
            return VERR_PARSE_INVALID_NUMBER;

        /* check for overflow - ARG!!! How to detect overflow correctly!?!?!? */
        if (u64Prev != u64 / uBase)
            return VERR_PARSE_NUMBER_TOO_BIG;

        /* next */
        pszExpr++;
    }

    /*
     * Initialize the argument.
     */
    pArg->pDesc         = NULL;
    pArg->pNext         = NULL;
    pArg->enmType       = DBGCVAR_TYPE_NUMBER;
    pArg->u.u64Number   = u64;
    pArg->enmRangeType  = DBGCVAR_RANGE_NONE;
    pArg->u64Range      = 0;

    return 0;
}


/**
 * Match variable and variable descriptor, promoting the variable if necessary.
 *
 * @returns VBox status code.
 * @param   pDbgc       Debug console instanace.
 * @param   pVar        Variable.
 * @param   pVarDesc    Variable descriptor.
 */
static int dbgcEvalSubMatchVar(PDBGC pDbgc, PDBGCVAR pVar, PCDBGCVARDESC pVarDesc)
{
    /*
     * (If match or promoted to match, return, else break.)
     */
    switch (pVarDesc->enmCategory)
    {
        /*
         * Anything goes
         */
        case DBGCVAR_CAT_ANY:
            return VINF_SUCCESS;

        /*
         * Pointer with and without range.
         * We can try resolve strings and symbols as symbols and
         * promote numbers to flat GC pointers.
         */
        case DBGCVAR_CAT_POINTER_NO_RANGE:
            if (pVar->enmRangeType != DBGCVAR_RANGE_NONE)
                return VERR_PARSE_NO_RANGE_ALLOWED;
            /* fallthru */
        case DBGCVAR_CAT_POINTER:
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                case DBGCVAR_TYPE_GC_FAR:
                case DBGCVAR_TYPE_GC_PHYS:
                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_SYMBOL:
                case DBGCVAR_TYPE_STRING:
                {
                    DBGCVAR Var;
                    int rc = dbgcSymbolGet(pDbgc, pVar->u.pszString, DBGCVAR_TYPE_GC_FLAT, &Var);
                    if (VBOX_SUCCESS(rc))
                    {
                        /* deal with range */
                        if (pVar->enmRangeType != DBGCVAR_RANGE_NONE)
                        {
                            Var.enmRangeType = pVar->enmRangeType;
                            Var.u64Range = pVar->u64Range;
                        }
                        else if (pVarDesc->enmCategory == DBGCVAR_CAT_POINTER_NO_RANGE)
                            Var.enmRangeType = DBGCVAR_RANGE_NONE;
                        *pVar = Var;
                        return rc;
                    }
                    break;
                }

                case DBGCVAR_TYPE_NUMBER:
                {
                    RTGCPTR GCPtr = (RTGCPTR)pVar->u.u64Number;
                    pVar->enmType = DBGCVAR_TYPE_GC_FLAT;
                    pVar->u.GCFlat = GCPtr;
                    return VINF_SUCCESS;
                }

                default:
                    break;
            }
            break;

        /*
         * GC pointer with and without range.
         * We can try resolve strings and symbols as symbols and
         * promote numbers to flat GC pointers.
         */
        case DBGCVAR_CAT_GC_POINTER_NO_RANGE:
            if (pVar->enmRangeType != DBGCVAR_RANGE_NONE)
                return VERR_PARSE_NO_RANGE_ALLOWED;
            /* fallthru */
        case DBGCVAR_CAT_GC_POINTER:
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_GC_FLAT:
                case DBGCVAR_TYPE_GC_FAR:
                case DBGCVAR_TYPE_GC_PHYS:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_HC_FLAT:
                case DBGCVAR_TYPE_HC_FAR:
                case DBGCVAR_TYPE_HC_PHYS:
                    return VERR_PARSE_CONVERSION_FAILED;

                case DBGCVAR_TYPE_SYMBOL:
                case DBGCVAR_TYPE_STRING:
                {
                    DBGCVAR Var;
                    int rc = dbgcSymbolGet(pDbgc, pVar->u.pszString, DBGCVAR_TYPE_GC_FLAT, &Var);
                    if (VBOX_SUCCESS(rc))
                    {
                        /* deal with range */
                        if (pVar->enmRangeType != DBGCVAR_RANGE_NONE)
                        {
                            Var.enmRangeType = pVar->enmRangeType;
                            Var.u64Range = pVar->u64Range;
                        }
                        else if (pVarDesc->enmCategory == DBGCVAR_CAT_POINTER_NO_RANGE)
                            Var.enmRangeType = DBGCVAR_RANGE_NONE;
                        *pVar = Var;
                        return rc;
                    }
                    break;
                }

                case DBGCVAR_TYPE_NUMBER:
                {
                    RTGCPTR GCPtr = (RTGCPTR)pVar->u.u64Number;
                    pVar->enmType = DBGCVAR_TYPE_GC_FLAT;
                    pVar->u.GCFlat = GCPtr;
                    return VINF_SUCCESS;
                }

                default:
                    break;
            }
            break;

        /*
         * Number with or without a range.
         * Numbers can be resolved from symbols, but we cannot demote a pointer
         * to a number.
         */
        case DBGCVAR_CAT_NUMBER_NO_RANGE:
            if (pVar->enmRangeType != DBGCVAR_RANGE_NONE)
                return VERR_PARSE_NO_RANGE_ALLOWED;
            /* fallthru */
        case DBGCVAR_CAT_NUMBER:
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_NUMBER:
                    return VINF_SUCCESS;

                case DBGCVAR_TYPE_SYMBOL:
                case DBGCVAR_TYPE_STRING:
                {
                    DBGCVAR Var;
                    int rc = dbgcSymbolGet(pDbgc, pVar->u.pszString, DBGCVAR_TYPE_NUMBER, &Var);
                    if (VBOX_SUCCESS(rc))
                    {
                        *pVar = Var;
                        return rc;
                    }
                    break;
                }
                default:
                    break;
            }
            break;

        /*
         * Strings can easily be made from symbols (and of course strings).
         * We could consider reformatting the addresses and numbers into strings later...
         */
        case DBGCVAR_CAT_STRING:
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_SYMBOL:
                    pVar->enmType = DBGCVAR_TYPE_STRING;
                    /* fallthru */
                case DBGCVAR_TYPE_STRING:
                    return VINF_SUCCESS;
                default:
                    break;
            }
            break;

        /*
         * Symol is pretty much the same thing as a string (at least until we actually implement it).
         */
        case DBGCVAR_CAT_SYMBOL:
            switch (pVar->enmType)
            {
                case DBGCVAR_TYPE_STRING:
                    pVar->enmType = DBGCVAR_TYPE_SYMBOL;
                    /* fallthru */
                case DBGCVAR_TYPE_SYMBOL:
                    return VINF_SUCCESS;
                default:
                    break;
            }
            break;

        /*
         * Anything else is illegal.
         */
        default:
            AssertMsgFailed(("enmCategory=%d\n", pVar->enmType));
            break;
    }

    return VERR_PARSE_NO_ARGUMENT_MATCH;
}


/**
 * Matches a set of variables with a description set.
 *
 * This is typically used for routine arguments before a call. The effects in
 * addition to the validation, is that some variables might be propagated to
 * other types in order to match the description. The following transformations
 * are supported:
 *      - String reinterpreted as a symbol and resolved to a number or pointer.
 *      - Number to a pointer.
 *      - Pointer to a number.
 * @returns 0 on success with paVars.
 * @returns VBox error code for match errors.
 */
static int dbgcEvalSubMatchVars(PDBGC pDbgc, unsigned cVarsMin, unsigned cVarsMax,
                                PCDBGCVARDESC paVarDescs, unsigned cVarDescs,
                                PDBGCVAR paVars, unsigned cVars)
{
    /*
     * Just do basic min / max checks first.
     */
    if (cVars < cVarsMin)
        return VERR_PARSE_TOO_FEW_ARGUMENTS;
    if (cVars > cVarsMax)
        return VERR_PARSE_TOO_MANY_ARGUMENTS;

    /*
     * Match the descriptors and actual variables.
     */
    PCDBGCVARDESC   pPrevDesc = NULL;
    unsigned        cCurDesc = 0;
    unsigned        iVar = 0;
    unsigned        iVarDesc = 0;
    while (iVar < cVars)
    {
        /* walk the descriptors */
        if (iVarDesc >= cVarDescs)
            return VERR_PARSE_TOO_MANY_ARGUMENTS;
        if (    (    paVarDescs[iVarDesc].fFlags & DBGCVD_FLAGS_DEP_PREV
                &&  &paVarDescs[iVarDesc - 1] != pPrevDesc)
            ||  cCurDesc >= paVarDescs[iVarDesc].cTimesMax)
        {
            iVarDesc++;
            if (iVarDesc >= cVarDescs)
                return VERR_PARSE_TOO_MANY_ARGUMENTS;
            cCurDesc = 0;
        }

        /*
         * Skip thru optional arguments until we find something which matches
         * or can easily be promoted to what the descriptor want.
         */
        for (;;)
        {
            int rc = dbgcEvalSubMatchVar(pDbgc, &paVars[iVar], &paVarDescs[iVarDesc]);
            if (VBOX_SUCCESS(rc))
            {
                paVars[iVar].pDesc = &paVarDescs[iVarDesc];
                cCurDesc++;
                break;
            }

            /* can we advance? */
            if (paVarDescs[iVarDesc].cTimesMin > cCurDesc)
                return VERR_PARSE_ARGUMENT_TYPE_MISMATCH;
            if (++iVarDesc >= cVarDescs)
                return VERR_PARSE_ARGUMENT_TYPE_MISMATCH;
            cCurDesc = 0;
        }

        /* next var */
        iVar++;
    }

    /*
     * Check that the rest of the descriptors are optional.
     */
    while (iVarDesc < cVarDescs)
    {
        if (paVarDescs[iVarDesc].cTimesMin > cCurDesc)
            return VERR_PARSE_TOO_FEW_ARGUMENTS;
        cCurDesc = 0;

        /* next */
        iVarDesc++;
    }

    return 0;
}


/**
 * Evaluates one argument with respect to unary operators.
 *
 * @returns 0 on success. pResult contains the result.
 * @returns VBox error code on parse or other evaluation error.
 *
 * @param   pDbgc       Debugger console instance data.
 * @param   pszExpr     The expression string.
 * @param   pResult     Where to store the result of the expression evaluation.
 */
static int dbgcEvalSubUnary(PDBGC pDbgc, char *pszExpr, size_t cchExpr, PDBGCVAR pResult)
{
    Log2(("dbgcEvalSubUnary: cchExpr=%d pszExpr=%s\n", cchExpr, pszExpr));

    /*
     * The state of the expression is now such that it will start by zero or more
     * unary operators and being followed by an expression of some kind.
     * The expression is either plain or in parenthesis.
     *
     * Being in a lazy, recursive mode today, the parsing is done as simple as possible. :-)
     * ASSUME: unary operators are all of equal precedence.
     */
    int         rc = 0;
    PCDBGCOP    pOp = dbgcOperatorLookup(pDbgc, pszExpr, false, ' ');
    if (pOp)
    {
        /* binary operators means syntax error. */
        if (pOp->fBinary)
            return VERR_PARSE_UNEXPECTED_OPERATOR;

        /*
         * If the next expression (the one following the unary operator) is in a
         * parenthesis a full eval is needed. If not the unary eval will suffice.
         */
        /* calc and strip next expr. */
        char *pszExpr2 = pszExpr + pOp->cchName;
        while (isblank(*pszExpr2))
            pszExpr2++;

        if (!*pszExpr2)
            rc = VERR_PARSE_EMPTY_ARGUMENT;
        else
        {
            DBGCVAR Arg;
            if (*pszExpr2 == '(')
                rc = dbgcEvalSub(pDbgc, pszExpr2, cchExpr - (pszExpr2 - pszExpr), &Arg);
            else
                rc = dbgcEvalSubUnary(pDbgc, pszExpr2, cchExpr - (pszExpr2 - pszExpr), &Arg);
            if (VBOX_SUCCESS(rc))
                rc = pOp->pfnHandlerUnary(pDbgc, &Arg, pResult);
        }
    }
    else
    {
        /*
         * Didn't find any operators, so it we have to check if this can be an
         * function call before assuming numeric or string expression.
         *
         * (ASSUMPTIONS:)
         * A function name only contains alphanumerical chars and it can not start
         * with a numerical character.
         * Immediately following the name is a parenthesis which must over
         * the remaining part of the expression.
         */
        bool    fExternal = *pszExpr == '.';
        char   *pszFun    = fExternal ? pszExpr + 1 : pszExpr;
        char   *pszFunEnd = NULL;
        if (pszExpr[cchExpr - 1] == ')' && isalpha(*pszFun))
        {
            pszFunEnd = pszExpr + 1;
            while (*pszFunEnd != '(' && isalnum(*pszFunEnd))
                pszFunEnd++;
            if (*pszFunEnd != '(')
                pszFunEnd = NULL;
        }

        if (pszFunEnd)
        {
            /*
             * Ok, it's a function call.
             */
            if (fExternal)
                pszExpr++, cchExpr--;
            PCDBGCCMD pFun = dbgcRoutineLookup(pDbgc, pszExpr, pszFunEnd - pszExpr, fExternal);
            if (!pFun)
                return VERR_PARSE_FUNCTION_NOT_FOUND;
            if (!pFun->pResultDesc)
                return VERR_PARSE_NOT_A_FUNCTION;

            /*
             * Parse the expression in parenthesis.
             */
            cchExpr -= pszFunEnd - pszExpr;
            pszExpr = pszFunEnd;
            /** @todo implement multiple arguments. */
            DBGCVAR     Arg;
            rc = dbgcEvalSub(pDbgc, pszExpr, cchExpr, &Arg);
            if (!rc)
            {
                rc = dbgcEvalSubMatchVars(pDbgc, pFun->cArgsMin, pFun->cArgsMax, pFun->paArgDescs, pFun->cArgDescs, &Arg, 1);
                if (!rc)
                    rc = pFun->pfnHandler(pFun, &pDbgc->CmdHlp, pDbgc->pVM, &Arg, 1, pResult);
            }
            else if (rc == VERR_PARSE_EMPTY_ARGUMENT && pFun->cArgsMin == 0)
                rc = pFun->pfnHandler(pFun, &pDbgc->CmdHlp, pDbgc->pVM, NULL, 0, pResult);
        }
        else
        {
            /*
             * Didn't find any operators, so it must be a plain expression.
             * This might be numeric or a string expression.
             */
            char ch  = pszExpr[0];
            char ch2 = pszExpr[1];
            if (ch == '0' && (ch2 == 'x' || ch2 == 'X'))
                rc = dbgcEvalSubNum(pszExpr + 2, 16, pResult);
            else if (ch == '0' && (ch2 == 'i' || ch2 == 'i'))
                rc = dbgcEvalSubNum(pszExpr + 2, 10, pResult);
            else if (ch == '0' && (ch2 == 't' || ch2 == 'T'))
                rc = dbgcEvalSubNum(pszExpr + 2, 8, pResult);
            /// @todo 0b doesn't work as a binary prefix, we confuse it with 0bf8:0123 and stuff.
            //else if (ch == '0' && (ch2 == 'b' || ch2 == 'b'))
            //    rc = dbgcEvalSubNum(pszExpr + 2, 2, pResult);
            else
            {
                /*
                 * Hexadecimal number or a string?
                 */
                char *psz = pszExpr;
                while (isxdigit(*psz))
                    psz++;
                if (!*psz)
                    rc = dbgcEvalSubNum(pszExpr, 16, pResult);
                else if ((*psz == 'h' || *psz == 'H') && !psz[1])
                {
                    *psz = '\0';
                    rc = dbgcEvalSubNum(pszExpr, 16, pResult);
                }
                else
                    rc = dbgcEvalSubString(pDbgc, pszExpr, cchExpr, pResult);
            }
        }
    }

    return rc;
}


/**
 * Evaluates one argument.
 *
 * @returns 0 on success. pResult contains the result.
 * @returns VBox error code on parse or other evaluation error.
 *
 * @param   pDbgc       Debugger console instance data.
 * @param   pszExpr     The expression string.
 * @param   pResult     Where to store the result of the expression evaluation.
 */
static int dbgcEvalSub(PDBGC pDbgc, char *pszExpr, size_t cchExpr, PDBGCVAR pResult)
{
    Log2(("dbgcEvalSub: cchExpr=%d pszExpr=%s\n", cchExpr, pszExpr));
    /*
     * First we need to remove blanks in both ends.
     * ASSUMES: There is no quoting unless the entire expression is a string.
     */

    /* stripping. */
    while (cchExpr > 0 && isblank(pszExpr[cchExpr - 1]))
        pszExpr[--cchExpr] = '\0';
    while (isblank(*pszExpr))
        pszExpr++, cchExpr--;
    if (!*pszExpr)
        return VERR_PARSE_EMPTY_ARGUMENT;

    /* it there is any kind of quoting in the expression, it's string meat. */
    if (strpbrk(pszExpr, "\"'`"))
        return dbgcEvalSubString(pDbgc, pszExpr, cchExpr, pResult);

    /*
     * Check if there are any parenthesis which needs removing.
     */
    if (pszExpr[0] == '(' && pszExpr[cchExpr - 1] == ')')
    {
        do
        {
            unsigned cPar = 1;
            char    *psz = pszExpr + 1;
            char     ch;
            while ((ch = *psz) != '\0')
            {
                if (ch == '(')
                    cPar++;
                else if (ch == ')')
                {
                    if (cPar <= 0)
                        return VERR_PARSE_UNBALANCED_PARENTHESIS;
                    cPar--;
                    if (cPar == 0 && psz[1]) /* If not at end, there's nothing to do. */
                        break;
                }
                /* next */
                psz++;
            }
            if (ch)
                break;

            /* remove the parenthesis. */
            pszExpr++;
            cchExpr -= 2;
            pszExpr[cchExpr] = '\0';

            /* strip blanks. */
            while (cchExpr > 0 && isblank(pszExpr[cchExpr - 1]))
                pszExpr[--cchExpr] = '\0';
            while (isblank(*pszExpr))
                pszExpr++, cchExpr--;
            if (!*pszExpr)
                return VERR_PARSE_EMPTY_ARGUMENT;
        } while (pszExpr[0] == '(' && pszExpr[cchExpr - 1] == ')');
    }

    /* tabs to spaces. */
    char *psz = pszExpr;
    while ((psz = strchr(psz, '\t')) != NULL)
        *psz = ' ';

    /*
     * Now, we need to look for the binary operator with the lowest precedence.
     *
     * If there are no operators we're left with a simple expression which we
     * evaluate with respect to unary operators
     */
    char       *pszOpSplit = NULL;
    PCDBGCOP    pOpSplit = NULL;
    unsigned    cBinaryOps = 0;
    unsigned    cPar = 0;
    char        ch;
    char        chPrev = ' ';
    bool        fBinary = false;
    psz = pszExpr;

    while ((ch = *psz) != '\0')
    {
        //Log2(("ch=%c cPar=%d fBinary=%d\n", ch, cPar, fBinary));
        /*
         * Parenthesis.
         */
        if (ch == '(')
        {
            cPar++;
            fBinary = false;
        }
        else if (ch == ')')
        {
            if (cPar <= 0)
                return VERR_PARSE_UNBALANCED_PARENTHESIS;
            cPar--;
            fBinary = true;
        }
        /*
         * Potential operator.
         */
        else if (cPar == 0 && !isblank(ch))
        {
            PCDBGCOP pOp = dbgcIsOpChar(ch)
                         ? dbgcOperatorLookup(pDbgc, psz, fBinary, chPrev)
                         : NULL;
            if (pOp)
            {
                /* If not the right kind of operator we've got a syntax error. */
                if (pOp->fBinary != fBinary)
                    return VERR_PARSE_UNEXPECTED_OPERATOR;

                /*
                 * Update the parse state and skip the operator.
                 */
                if (!pOpSplit)
                {
                    pOpSplit = pOp;
                    pszOpSplit = psz;
                    cBinaryOps = fBinary;
                }
                else if (fBinary)
                {
                    cBinaryOps++;
                    if (pOp->iPrecedence >= pOpSplit->iPrecedence)
                    {
                        pOpSplit = pOp;
                        pszOpSplit = psz;
                    }
                }

                psz += pOp->cchName - 1;
                fBinary = false;
            }
            else
                fBinary = true;
        }

        /* next */
        psz++;
        chPrev = ch;
    } /* parse loop. */


    /*
     * Either we found an operator to divide the expression by
     * or we didn't find any. In the first case it's divide and
     * conquer. In the latter it's a single expression which
     * needs dealing with its unary operators if any.
     */
    int rc;
    if (    cBinaryOps
        &&  pOpSplit->fBinary)
    {
        /* process 1st sub expression. */
        *pszOpSplit = '\0';
        DBGCVAR     Arg1;
        rc = dbgcEvalSub(pDbgc, pszExpr, pszOpSplit - pszExpr, &Arg1);
        if (VBOX_SUCCESS(rc))
        {
            /* process 2nd sub expression. */
            char       *psz2 = pszOpSplit + pOpSplit->cchName;
            DBGCVAR     Arg2;
            rc = dbgcEvalSub(pDbgc, psz2, cchExpr - (psz2 - pszExpr), &Arg2);
            if (VBOX_SUCCESS(rc))
                /* apply the operator. */
                rc = pOpSplit->pfnHandlerBinary(pDbgc, &Arg1, &Arg2, pResult);
        }
    }
    else if (cBinaryOps)
    {
        /* process sub expression. */
        pszOpSplit += pOpSplit->cchName;
        DBGCVAR     Arg;
        rc = dbgcEvalSub(pDbgc, pszOpSplit, cchExpr - (pszOpSplit - pszExpr), &Arg);
        if (VBOX_SUCCESS(rc))
            /* apply the operator. */
            rc = pOpSplit->pfnHandlerUnary(pDbgc, &Arg, pResult);
    }
    else
        /* plain expression or using unary operators perhaps with paratheses. */
        rc = dbgcEvalSubUnary(pDbgc, pszExpr, cchExpr, pResult);

    return rc;
}


/**
 * Parses the arguments of one command.
 *
 * @returns 0 on success.
 * @returns VBox error code on parse error with *pcArg containing the argument causing trouble.
 * @param   pDbgc       Debugger console instance data.
 * @param   pCmd        Pointer to the command descriptor.
 * @param   pszArg      Pointer to the arguments to parse.
 * @param   paArgs      Where to store the parsed arguments.
 * @param   cArgs       Size of the paArgs array.
 * @param   pcArgs      Where to store the number of arguments.
 *                      In the event of an error this is used to store the index of the offending argument.
 */
static int dbgcProcessArguments(PDBGC pDbgc, PCDBGCCMD pCmd, char *pszArgs, PDBGCVAR paArgs, unsigned cArgs, unsigned *pcArgs)
{
    Log2(("dbgcProcessArguments: pCmd=%s pszArgs='%s'\n", pCmd->pszCmd, pszArgs));
    /*
     * Check if we have any argument and if the command takes any.
     */
    *pcArgs = 0;
    /* strip leading blanks. */
    while (*pszArgs && isblank(*pszArgs))
        pszArgs++;
    if (!*pszArgs)
    {
        if (!pCmd->cArgsMin)
            return 0;
        return VERR_PARSE_TOO_FEW_ARGUMENTS;
    }
    /** @todo fixme - foo() doesn't work. */
    if (!pCmd->cArgsMax)
        return VERR_PARSE_TOO_MANY_ARGUMENTS;

    /*
     * This is a hack, it's "temporary" and should go away "when" the parser is
     * modified to match arguments while parsing.
     */
    if (    pCmd->cArgsMax == 1
        &&  pCmd->cArgsMin == 1
        &&  pCmd->cArgDescs == 1
        &&  pCmd->paArgDescs[0].enmCategory == DBGCVAR_CAT_STRING
        &&  cArgs >= 1)
    {
        *pcArgs = 1;
        RTStrStripR(pszArgs);
        return dbgcEvalSubString(pDbgc, pszArgs, strlen(pszArgs), &paArgs[0]);
    }


    /*
     * The parse loop.
     */
    PDBGCVAR        pArg0 = &paArgs[0];
    PDBGCVAR        pArg = pArg0;
    *pcArgs = 0;
    do
    {
        /*
         * Can we have another argument?
         */
        if (*pcArgs >= pCmd->cArgsMax)
            return VERR_PARSE_TOO_MANY_ARGUMENTS;
        if (pArg >= &paArgs[cArgs])
            return VERR_PARSE_ARGUMENT_OVERFLOW;

        /*
         * Find the end of the argument.
         */
        int     cPar    = 0;
        char    chQuote = '\0';
        char   *pszEnd  = NULL;
        char   *psz     = pszArgs;
        char    ch;
        bool    fBinary = false;
        for (;;)
        {
            /*
             * Check for the end.
             */
            if ((ch = *psz) == '\0')
            {
                if (chQuote)
                    return VERR_PARSE_UNBALANCED_QUOTE;
                if (cPar)
                    return VERR_PARSE_UNBALANCED_PARENTHESIS;
                pszEnd = psz;
                break;
            }
            /*
             * When quoted we ignore everything but the quotation char.
             * We use the REXX way of escaping the quotation char, i.e. double occurence.
             */
            else if (ch == '\'' || ch == '"' || ch == '`')
            {
                if (chQuote)
                {
                    /* end quote? */
                    if (ch == chQuote)
                    {
                        if (psz[1] == ch)
                            psz++;          /* skip the escaped quote char */
                        else
                            chQuote = '\0'; /* end of quoted string. */
                    }
                }
                else
                    chQuote = ch;           /* open new quote */
            }
            /*
             * Parenthesis can of course be nested.
             */
            else if (ch == '(')
            {
                cPar++;
                fBinary = false;
            }
            else if (ch == ')')
            {
                if (!cPar)
                    return VERR_PARSE_UNBALANCED_PARENTHESIS;
                cPar--;
                fBinary = true;
            }
            else if (!chQuote && !cPar)
            {
                /*
                 * Encountering blanks may mean the end of it all. A binary operator
                 * will force continued parsing.
                 */
                if (isblank(*psz))
                {
                    pszEnd = psz++;         /* just in case. */
                    while (isblank(*psz))
                        psz++;
                    PCDBGCOP pOp = dbgcOperatorLookup(pDbgc, psz, fBinary, ' ');
                    if (!pOp || pOp->fBinary != fBinary)
                        break;              /* the end. */
                    psz += pOp->cchName;
                    while (isblank(*psz))   /* skip blanks so we don't get here again */
                        psz++;
                    fBinary = false;
                    continue;
                }

                /*
                 * Look for operators without a space up front.
                 */
                if (dbgcIsOpChar(*psz))
                {
                    PCDBGCOP pOp = dbgcOperatorLookup(pDbgc, psz, fBinary, ' ');
                    if (pOp)
                    {
                        if (pOp->fBinary != fBinary)
                        {
                            pszEnd = psz;
                            /** @todo this is a parsing error really. */
                            break;              /* the end. */
                        }
                        psz += pOp->cchName;
                        while (isblank(*psz))   /* skip blanks so we don't get here again */
                            psz++;
                        fBinary = false;
                        continue;
                    }
                }
                fBinary = true;
            }

            /* next char */
            psz++;
        }
        *pszEnd = '\0';
        /* (psz = next char to process) */

        /*
         * Parse and evaluate the argument.
         */
        int rc = dbgcEvalSub(pDbgc, pszArgs, strlen(pszArgs), pArg);
        if (VBOX_FAILURE(rc))
            return rc;

        /*
         * Next.
         */
        pArg++;
        (*pcArgs)++;
        pszArgs = psz;
        while (*pszArgs && isblank(*pszArgs))
            pszArgs++;
    } while (*pszArgs);

    /*
     * Match the arguments.
     */
    return dbgcEvalSubMatchVars(pDbgc, pCmd->cArgsMin, pCmd->cArgsMax, pCmd->paArgDescs, pCmd->cArgDescs, pArg0, pArg - pArg0);
}


/**
 * Process one command.
 *
 * @returns VBox status code. Any error indicates the termination of the console session.
 * @param   pDbgc   Debugger console instance data.
 * @param   pszCmd  Pointer to the command.
 * @param   cchCmd  Length of the command.
 */
static int dbgcProcessCommand(PDBGC pDbgc, char *pszCmd, size_t cchCmd)
{
    char *pszCmdInput = pszCmd;

    /*
     * Skip blanks.
     */
    while (isblank(*pszCmd))
        pszCmd++, cchCmd--;

    /* external command? */
    bool fExternal = *pszCmd == '.';
    if (fExternal)
        pszCmd++, cchCmd--;

    /*
     * Find arguments.
     */
    char *pszArgs = pszCmd;
    while (isalnum(*pszArgs))
        pszArgs++;
    if (*pszArgs && (!isblank(*pszArgs) || pszArgs == pszCmd))
    {
        pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "Syntax error in command '%s'!\n", pszCmdInput);
        return 0;
    }

    /*
     * Find the command.
     */
    PCDBGCCMD pCmd = dbgcRoutineLookup(pDbgc, pszCmd, pszArgs - pszCmd, fExternal);
    if (!pCmd || (pCmd->fFlags & DBGCCMD_FLAGS_FUNCTION))
        return pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "Unknown command '%s'!\n", pszCmdInput);

    /*
     * Parse arguments (if any).
     */
    unsigned    cArgs;
    int rc = dbgcProcessArguments(pDbgc, pCmd, pszArgs, &pDbgc->aArgs[pDbgc->iArg], ELEMENTS(pDbgc->aArgs) - pDbgc->iArg, &cArgs);

    /*
     * Execute the command.
     */
    if (!rc)
    {
        rc = pCmd->pfnHandler(pCmd, &pDbgc->CmdHlp, pDbgc->pVM, &pDbgc->aArgs[0], cArgs, NULL);
    }
    else
    {
        /* report parse / eval error. */
        switch (rc)
        {
            case VERR_PARSE_TOO_FEW_ARGUMENTS:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Too few arguments. Minimum is %d for command '%s'.\n", pCmd->cArgsMin, pCmd->pszCmd);
                break;
            case VERR_PARSE_TOO_MANY_ARGUMENTS:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Too many arguments. Maximum is %d for command '%s'.\n", pCmd->cArgsMax, pCmd->pszCmd);
                break;
            case VERR_PARSE_ARGUMENT_OVERFLOW:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Too many arguments.\n");
                break;
            case VERR_PARSE_UNBALANCED_QUOTE:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Unbalanced quote (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_UNBALANCED_PARENTHESIS:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Unbalanced parenthesis (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_EMPTY_ARGUMENT:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: An argument or subargument contains nothing useful (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_UNEXPECTED_OPERATOR:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Invalid operator usage (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_INVALID_NUMBER:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Syntax error: Ivalid numeric value (argument %d). If a string was the intention, then quote it.\n", cArgs);
                break;
            case VERR_PARSE_NUMBER_TOO_BIG:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Numeric overflow (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_INVALID_OPERATION:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Invalid operation attempted (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_FUNCTION_NOT_FOUND:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Function not found (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_NOT_A_FUNCTION:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: The function specified is not a function (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_NO_MEMORY:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Out memory in the regular heap! Expect odd stuff to happen...\n", cArgs);
                break;
            case VERR_PARSE_INCORRECT_ARG_TYPE:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Incorrect argument type (argument %d?).\n", cArgs);
                break;
            case VERR_PARSE_VARIABLE_NOT_FOUND:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: An undefined variable was referenced (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_CONVERSION_FAILED:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: A conversion between two types failed (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_NOT_IMPLEMENTED:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: You hit a debugger feature which isn't implemented yet (argument %d).\n", cArgs);
                break;
            case VERR_PARSE_BAD_RESULT_TYPE:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Couldn't satisfy a request for a specific result type (argument %d). (Usually applies to symbols)\n", cArgs);
                break;
            case VERR_PARSE_WRITEONLY_SYMBOL:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Cannot get symbol, it's set only (argument %d).\n", cArgs);
                break;

            default:
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                    "Error: Unknown error %d!\n", rc);
                return rc;
        }

        /*
         * Parse errors are non fatal.
         */
        if (rc >= VERR_PARSE_FIRST && rc < VERR_PARSE_LAST)
            rc = 0;
    }

    return rc;
}


/**
 * Process all commands current in the buffer.
 *
 * @returns VBox status code. Any error indicates the termination of the console session.
 * @param   pDbgc   Debugger console instance data.
 */
static int dbgcProcessCommands(PDBGC pDbgc)
{
    int rc = 0;
    while (pDbgc->cInputLines)
    {
        /*
         * Empty the log buffer if we're hooking the log.
         */
        if (pDbgc->fLog)
        {
            rc = dbgcProcessLog(pDbgc);
            if (VBOX_FAILURE(rc))
                break;
        }

        if (pDbgc->iRead == pDbgc->iWrite)
        {
            AssertMsgFailed(("The input buffer is empty while cInputLines=%d!\n", pDbgc->cInputLines));
            pDbgc->cInputLines = 0;
            return 0;
        }

        /*
         * Copy the command to the parse buffer.
         */
        char    ch;
        char   *psz = &pDbgc->achInput[pDbgc->iRead];
        char   *pszTrg = &pDbgc->achScratch[0];
        while ((*pszTrg = ch = *psz++) != ';' && ch != '\n' )
        {
            if (psz == &pDbgc->achInput[sizeof(pDbgc->achInput)])
                psz = &pDbgc->achInput[0];

            if (psz == &pDbgc->achInput[pDbgc->iWrite])
            {
                AssertMsgFailed(("The buffer contains no commands while cInputLines=%d!\n", pDbgc->cInputLines));
                pDbgc->cInputLines = 0;
                return 0;
            }

            pszTrg++;
        }
        *pszTrg = '\0';

        /*
         * Advance the buffer.
         */
        pDbgc->iRead = psz - &pDbgc->achInput[0];
        if (ch == '\n')
            pDbgc->cInputLines--;

        /*
         * Parse and execute this command.
         */
        pDbgc->pszScratch = psz;
        pDbgc->iArg       = 0;
        rc = dbgcProcessCommand(pDbgc, &pDbgc->achScratch[0], psz - &pDbgc->achScratch[0] - 1);
        if (rc)
            break;
    }

    return rc;
}


/**
 * Reads input, parses it and executes commands on '\n'.
 *
 * @returns VBox status.
 * @param   pDbgc   Debugger console instance data.
 */
static int dbgcProcessInput(PDBGC pDbgc)
{
    /*
     * We know there's input ready, so let's read it first.
     */
    int rc = dbgcInputRead(pDbgc);
    if (VBOX_FAILURE(rc))
        return rc;

    /*
     * Now execute any ready commands.
     */
    if (pDbgc->cInputLines)
    {
        /** @todo this fReady stuff is broken. */
        pDbgc->fReady = false;
        rc = dbgcProcessCommands(pDbgc);
        if (VBOX_SUCCESS(rc) && rc != VWRN_DBGC_CMD_PENDING)
            pDbgc->fReady = true;
        if (    VBOX_SUCCESS(rc)
            &&  pDbgc->iRead == pDbgc->iWrite
            &&  pDbgc->fReady)
            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "VBoxDbg> ");
    }

    return rc;
}


/**
 * Gets the event context identifier string.
 * @returns Read only string.
 * @param   enmCtx          The context.
 */
static const char *dbgcGetEventCtx(DBGFEVENTCTX enmCtx)
{
    switch (enmCtx)
    {
        case DBGFEVENTCTX_RAW:      return "raw";
        case DBGFEVENTCTX_REM:      return "rem";
        case DBGFEVENTCTX_HWACCL:   return "hwaccl";
        case DBGFEVENTCTX_HYPER:    return "hyper";
        case DBGFEVENTCTX_OTHER:    return "other";

        case DBGFEVENTCTX_INVALID:  return "!Invalid Event Ctx!";
        default:
            AssertMsgFailed(("enmCtx=%d\n", enmCtx));
            return "!Unknown Event Ctx!";
    }
}


/**
 * Processes debugger events.
 *
 * @returns VBox status.
 * @param   pDbgc   DBGC Instance data.
 * @param   pEvent  Pointer to event data.
 */
static int dbgcProcessEvent(PDBGC pDbgc, PCDBGFEVENT pEvent)
{
    /*
     * Flush log first.
     */
    if (pDbgc->fLog)
    {
        int rc = dbgcProcessLog(pDbgc);
        if (VBOX_FAILURE(rc))
            return rc;
    }

    /*
     * Process the event.
     */
    pDbgc->pszScratch = &pDbgc->achInput[0];
    pDbgc->iArg       = 0;
    bool fPrintPrompt = true;
    int rc = VINF_SUCCESS;
    switch (pEvent->enmType)
    {
        /*
         * The first part is events we have initiated with commands.
         */
        case DBGFEVENT_HALT_DONE:
        {
            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf event: VM %p is halted! (%s)\n",
                                         pDbgc->pVM, dbgcGetEventCtx(pEvent->enmCtx));
            pDbgc->fRegCtxGuest = true; /* we're always in guest context when halted. */
            if (VBOX_SUCCESS(rc))
                rc = pDbgc->CmdHlp.pfnExec(&pDbgc->CmdHlp, "r");
            break;
        }


        /*
         * The second part is events which can occur at any time.
         */
        case DBGFEVENT_FATAL_ERROR:
        {
            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbf event: Fatal error! (%s)\n",
                                         dbgcGetEventCtx(pEvent->enmCtx));
            pDbgc->fRegCtxGuest = false; /* fatal errors are always in hypervisor. */
            if (VBOX_SUCCESS(rc))
                rc = pDbgc->CmdHlp.pfnExec(&pDbgc->CmdHlp, "r");
            break;
        }

        case DBGFEVENT_BREAKPOINT:
        case DBGFEVENT_BREAKPOINT_HYPER:
        {
            bool fRegCtxGuest = pDbgc->fRegCtxGuest;
            pDbgc->fRegCtxGuest = pEvent->enmType == DBGFEVENT_BREAKPOINT;

            rc = dbgcBpExec(pDbgc, pEvent->u.Bp.iBp);
            switch (rc)
            {
                case VERR_DBGC_BP_NOT_FOUND:
                    rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf event: Unknown breakpoint %u! (%s)\n",
                                                 pEvent->u.Bp.iBp, dbgcGetEventCtx(pEvent->enmCtx));
                    break;

                case VINF_DBGC_BP_NO_COMMAND:
                    rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf event: Breakpoint %u! (%s)\n",
                                                 pEvent->u.Bp.iBp, dbgcGetEventCtx(pEvent->enmCtx));
                    break;

                case VINF_BUFFER_OVERFLOW:
                    rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf event: Breakpoint %u! Command too long to execute! (%s)\n",
                                                 pEvent->u.Bp.iBp, dbgcGetEventCtx(pEvent->enmCtx));
                    break;

                default:
                    break;
            }
            if (VBOX_SUCCESS(rc) && DBGFR3IsHalted(pDbgc->pVM))
                rc = pDbgc->CmdHlp.pfnExec(&pDbgc->CmdHlp, "r");
            else
                pDbgc->fRegCtxGuest = fRegCtxGuest;
            break;
        }

        case DBGFEVENT_STEPPED:
        case DBGFEVENT_STEPPED_HYPER:
        {
            pDbgc->fRegCtxGuest = pEvent->enmType == DBGFEVENT_STEPPED;

            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf event: Single step! (%s)\n", dbgcGetEventCtx(pEvent->enmCtx));
            if (VBOX_SUCCESS(rc))
                rc = pDbgc->CmdHlp.pfnExec(&pDbgc->CmdHlp, "r");
            break;
        }

        case DBGFEVENT_ASSERTION_HYPER:
        {
            pDbgc->fRegCtxGuest = false;

            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                                         "\ndbgf event: Hypervisor Assertion! (%s)\n"
                                         "%s"
                                         "%s"
                                         "\n",
                                         dbgcGetEventCtx(pEvent->enmCtx),
                                         pEvent->u.Assert.pszMsg1,
                                         pEvent->u.Assert.pszMsg2);
            if (VBOX_SUCCESS(rc))
                rc = pDbgc->CmdHlp.pfnExec(&pDbgc->CmdHlp, "r");
            break;
        }

        case DBGFEVENT_DEV_STOP:
        {
            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                                         "\n"
                                         "dbgf event: DBGFSTOP (%s)\n"
                                         "File:     %s\n"
                                         "Line:     %d\n"
                                         "Function: %s\n",
                                         dbgcGetEventCtx(pEvent->enmCtx),
                                         pEvent->u.Src.pszFile,
                                         pEvent->u.Src.uLine,
                                         pEvent->u.Src.pszFunction);
            if (VBOX_SUCCESS(rc) && pEvent->u.Src.pszMessage && *pEvent->u.Src.pszMessage)
                rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
                                         "Message:  %s\n",
                                             pEvent->u.Src.pszMessage);
            if (VBOX_SUCCESS(rc))
                rc = pDbgc->CmdHlp.pfnExec(&pDbgc->CmdHlp, "r");
            break;
        }


        case DBGFEVENT_INVALID_COMMAND:
        {
            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf/dbgc error: Invalid command event!\n");
            fPrintPrompt = !pDbgc->fReady;
            break;
        }

        case DBGFEVENT_TERMINATING:
        {
            pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\nVM is terminating!\n");
            rc = VERR_GENERAL_FAILURE;
            break;
        }


        default:
        {
            rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "\ndbgf/dbgc error: Unknown event %d!\n", pEvent->enmType);
            fPrintPrompt = !pDbgc->fReady;
            break;
        }
    }

    /*
     * Prompt, anyone?
     */
    if (fPrintPrompt && VBOX_SUCCESS(rc))
    {
        rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL, "VBoxDbg> ");
    }

    return rc;
}





/**
 * Make a console instance.
 *
 * This will not return until either an 'exit' command is issued or a error code
 * indicating connection loss is encountered.
 *
 * @returns VINF_SUCCESS if console termination caused by the 'exit' command.
 * @returns The VBox status code causing the console termination.
 *
 * @param   pVM         VM Handle.
 * @param   pBack       Pointer to the backend structure. This must contain
 *                      a full set of function pointers to service the console.
 * @param   fFlags      Reserved, must be zero.
 * @remark  A forced termination of the console is easiest done by forcing the
 *          callbacks to return fatal failures.
 */
DBGDECL(int)    DBGCCreate(PVM pVM, PDBGCBACK pBack, unsigned fFlags)
{
    /*
     * Validate input.
     */
    AssertReturn(VALID_PTR(pVM), VERR_INVALID_PARAMETER);
    AssertReturn(VALID_PTR(pBack), VERR_INVALID_PARAMETER);
    AssertMsgReturn(!fFlags, ("%#x", fFlags), VERR_INVALID_PARAMETER);

    /*
     * Allocate and initialize instance data
     */
    PDBGC   pDbgc = (PDBGC)RTMemAllocZ(sizeof(*pDbgc));
    if (!pDbgc)
        return VERR_NO_MEMORY;

    pDbgc->CmdHlp.pfnWrite      = dbgcHlpWrite;
    pDbgc->CmdHlp.pfnPrintfV    = dbgcHlpPrintfV;
    pDbgc->CmdHlp.pfnPrintf     = dbgcHlpPrintf;
    pDbgc->CmdHlp.pfnVBoxErrorV = dbgcHlpVBoxErrorV;
    pDbgc->CmdHlp.pfnVBoxError  = dbgcHlpVBoxError;
    pDbgc->CmdHlp.pfnMemRead    = dbgcHlpMemRead;
    pDbgc->CmdHlp.pfnMemWrite   = dbgcHlpMemWrite;
    pDbgc->CmdHlp.pfnEval       = dbgcHlpEval;
    pDbgc->CmdHlp.pfnExec       = dbgcHlpExec;
    pDbgc->CmdHlp.pfnVarToDbgfAddr = dbgcHlpVarToDbgfAddr;
    pDbgc->CmdHlp.pfnVarToBool = dbgcHlpVarToBool;
    pDbgc->pBack            = pBack;
    pDbgc->pVM              = NULL;
    pDbgc->pszEmulation     = "CodeView/WinDbg";
    pDbgc->paEmulationCmds  = &g_aCmdsCodeView[0];
    pDbgc->cEmulationCmds   = g_cCmdsCodeView;
    //pDbgc->fLog             = false;
    pDbgc->fRegCtxGuest     = true;
    pDbgc->fRegTerse        = true;
    //pDbgc->DisasmPos        = {0};
    //pDbgc->SourcePos        = {0};
    //pDbgc->DumpPos          = {0};
    //pDbgc->cbDumpElement    = 0;
    //pDbgc->cVars            = 0;
    //pDbgc->paVars           = NULL;
    //pDbgc->pFirstBp         = NULL;
    //pDbgc->uInputZero       = 0;
    //pDbgc->iRead            = 0;
    //pDbgc->iWrite           = 0;
    //pDbgc->cInputLines      = 0;
    //pDbgc->fInputOverflow   = false;
    pDbgc->fReady           = true;
    pDbgc->pszScratch       = &pDbgc->achScratch[0];
    //pDbgc->iArg             = 0;
    //pDbgc->rcOutput         = 0;

    dbgcInitOpCharBitMap();

    /*
     * Print welcome message.
     */
    int rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
        "Welcome to the VirtualBox Debugger!\n");
    if (VBOX_FAILURE(rc))
        goto l_failure;

    /*
     * Attach to the VM.
     */
    rc = DBGFR3Attach(pVM);
    if (VBOX_FAILURE(rc))
    {
        rc = pDbgc->CmdHlp.pfnVBoxError(&pDbgc->CmdHlp, rc, "When trying to attach to VM %p\n", pDbgc->pVM);
        goto l_failure;
    }
    pDbgc->pVM = pVM;

    /*
     * Print commandline and auto select result.
     */
    rc = pDbgc->CmdHlp.pfnPrintf(&pDbgc->CmdHlp, NULL,
        "Current VM is %08x\n" /** @todo get and print the VM name! */
        "VBoxDbg> ",
        pDbgc->pVM);
    if (VBOX_FAILURE(rc))
        goto l_failure;

    /*
     * Main Debugger Loop.
     *
     * This loop will either block on waiting for input or on waiting on
     * debug events. If we're forwarding the log we cannot wait for long
     * before we must flush the log.
     */
    for (rc = 0;;)
    {
        if (pDbgc->pVM && DBGFR3CanWait(pDbgc->pVM))
        {
            /*
             * Wait for a debug event.
             */
            PCDBGFEVENT pEvent;
            rc = DBGFR3EventWait(pDbgc->pVM, pDbgc->fLog ? 1 : 32, &pEvent);
            if (VBOX_SUCCESS(rc))
            {
                rc = dbgcProcessEvent(pDbgc, pEvent);
                if (VBOX_FAILURE(rc))
                    break;
            }
            else if (rc != VERR_TIMEOUT)
                break;

            /*
             * Check for input.
             */
            if (pBack->pfnInput(pDbgc->pBack, 0))
            {
                rc = dbgcProcessInput(pDbgc);
                if (VBOX_FAILURE(rc))
                    break;
            }
        }
        else
        {
            /*
             * Wait for input. If Logging is enabled we'll only wait very briefly.
             */
            if (pBack->pfnInput(pDbgc->pBack, pDbgc->fLog ? 1 : 1000))
            {
                rc = dbgcProcessInput(pDbgc);
                if (VBOX_FAILURE(rc))
                    break;
            }
        }

        /*
         * Forward log output.
         */
        if (pDbgc->fLog)
        {
            rc = dbgcProcessLog(pDbgc);
            if (VBOX_FAILURE(rc))
                break;
        }
    }


l_failure:
    /*
     * Cleanup console debugger session.
     */
    /* Disable log hook. */
    if (pDbgc->fLog)
    {

    }

    /* Detach from the VM. */
    if (pDbgc->pVM)
        DBGFR3Detach(pDbgc->pVM);

    /* finally, free the instance memory. */
    RTMemFree(pDbgc);

    return rc;
}



/**
 * Register one or more external commands.
 *
 * @returns VBox status.
 * @param   paCommands      Pointer to an array of command descriptors.
 *                          The commands must be unique. It's not possible
 *                          to register the same commands more than once.
 * @param   cCommands       Number of commands.
 */
DBGDECL(int)    DBGCRegisterCommands(PCDBGCCMD paCommands, unsigned cCommands)
{
    /*
     * Lock the list.
     */
    DBGCEXTCMDS_LOCK_WR();
    PDBGCEXTCMDS pCur = g_pExtCmdsHead;
    while (pCur)
    {
        if (paCommands == pCur->paCmds)
        {
            DBGCEXTCMDS_UNLOCK_WR();
            AssertMsgFailed(("Attempt at re-registering %d command(s)!\n", cCommands));
            return VWRN_DBGC_ALREADY_REGISTERED;
        }
        pCur = pCur->pNext;
    }

    /*
     * Allocate new chunk.
     */
    int rc = 0;
    pCur = (PDBGCEXTCMDS)RTMemAlloc(sizeof(*pCur));
    if (pCur)
    {
        pCur->cCmds  = cCommands;
        pCur->paCmds = paCommands;
        pCur->pNext = g_pExtCmdsHead;
        g_pExtCmdsHead = pCur;
    }
    else
        rc = VERR_NO_MEMORY;
    DBGCEXTCMDS_UNLOCK_WR();

    return rc;
}


/**
 * Deregister one or more external commands previously registered by
 * DBGCRegisterCommands().
 *
 * @returns VBox status.
 * @param   paCommands      Pointer to an array of command descriptors
 *                          as given to DBGCRegisterCommands().
 * @param   cCommands       Number of commands.
 */
DBGDECL(int)    DBGCDeregisterCommands(PCDBGCCMD paCommands, unsigned cCommands)
{
    /*
     * Lock the list.
     */
    DBGCEXTCMDS_LOCK_WR();
    PDBGCEXTCMDS pPrev = NULL;
    PDBGCEXTCMDS pCur = g_pExtCmdsHead;
    while (pCur)
    {
        if (paCommands == pCur->paCmds)
        {
            if (pPrev)
                pPrev->pNext = pCur->pNext;
            else
                g_pExtCmdsHead = pCur->pNext;
            DBGCEXTCMDS_UNLOCK_WR();

            RTMemFree(pCur);
            return VINF_SUCCESS;
        }
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    DBGCEXTCMDS_UNLOCK_WR();

    NOREF(cCommands);
    return VERR_DBGC_COMMANDS_NOT_REGISTERED;
}


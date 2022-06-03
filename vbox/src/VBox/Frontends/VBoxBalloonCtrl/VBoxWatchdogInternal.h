/* $Id: VBoxWatchdogInternal.h 40877 2012-04-11 20:43:50Z vboxsync $ */
/** @file
 * VBoxWatchdog - VirtualBox Watchdog Service.
 */

/*
 * Copyright (C) 2011-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___H_VBOXWATCHDOG
#define ___H_VBOXWATCHDOG

#ifndef VBOX_ONLY_DOCS
# include <iprt/getopt.h>
# include <iprt/time.h>

# include <VBox/err.h>
# include <VBox/com/com.h>
# include <VBox/com/string.h>
# include <VBox/com/Guid.h>
# include <VBox/com/array.h>
# include <VBox/com/ErrorInfo.h>
# include <VBox/com/VirtualBox.h>
#endif /* !VBOX_ONLY_DOCS */

#include <map>
#include <vector>

using namespace com;

////////////////////////////////////////////////////////////////////////////////
//
// definitions
//
////////////////////////////////////////////////////////////////////////////////

/** Command handler argument. */
struct HandlerArg
{
    int argc;
    char **argv;
};

/**
 * A module's payload for a machine entry.
 * The payload data is not (yet) thread safe -- so only
 * use this in one module at a time only!
 */
typedef struct VBOXWATCHDOG_MODULE_PAYLOAD
{
    /** Pointer to allocated payload. Can be NULL if
     * a module doesn't have an own payload. */
    void *pvData;
    /** Size of payload (in bytes). */
    size_t cbData;
    /** @todo Add mutex for locking + getPayloadLocked(). */
} VBOXWATCHDOG_MODULE_PAYLOAD, *PVBOXWATCHDOG_MODULE_PAYLOAD;

/**
 * Map containing a module's individual payload -- the module itself
 * is responsible for allocating/handling/destroying this payload.
 * Primary key is the module name.
 */
typedef std::map<const char*, VBOXWATCHDOG_MODULE_PAYLOAD> mapPayload;
typedef std::map<const char*, VBOXWATCHDOG_MODULE_PAYLOAD>::iterator mapPayloadIter;
typedef std::map<const char*, VBOXWATCHDOG_MODULE_PAYLOAD>::const_iterator mapPayloadIterConst;

struct VBOXWATCHDOG_VM_GROUP;

/** A machine's internal entry.
 *  Primary key is the machine's UUID. */
typedef struct VBOXWATCHDOG_MACHINE
{
    ComPtr<IMachine> machine;
#ifndef VBOX_WATCHDOG_GLOBAL_PERFCOL
    ComPtr<IPerformanceCollector> collector;
#endif
    /** The machine's VM group(s). */
    Bstr group;
    /** Map containing the individual
     *  module payloads. */
    mapPayload payload;
} VBOXWATCHDOG_MACHINE, *PVBOXWATCHDOG_MACHINE;
typedef std::map<Bstr, VBOXWATCHDOG_MACHINE> mapVM;
typedef std::map<Bstr, VBOXWATCHDOG_MACHINE>::iterator mapVMIter;
typedef std::map<Bstr, VBOXWATCHDOG_MACHINE>::const_iterator mapVMIterConst;

/** Members of a VM group; currently only represented by the machine's UUID. */
typedef std::vector<Bstr> vecGroupMembers;
typedef std::vector<Bstr>::iterator vecGroupMembersIter;
typedef std::vector<Bstr>::const_iterator vecGroupMembersIterConst;

/** A VM group. Can contain none, one or more group members. */
typedef std::map<Bstr, vecGroupMembers> mapGroup;
typedef std::map<Bstr, vecGroupMembers>::iterator mapGroupIter;
typedef std::map<Bstr, vecGroupMembers>::const_iterator mapGroupIterConst;

/**
 * A module descriptor.
 */
typedef struct
{
    /** The short module name. */
    const char *pszName;
    /** The longer module name. */
    const char *pszDescription;
    /** A comma-separated list of modules this module
     *  depends on. Might be NULL if no dependencies. */
    const char *pszDepends;
    /** Priority (lower is higher, 0 is invalid) of
     *  module execution. */
    uint32_t    uPriority;
    /** The usage options stuff for the --help screen. */
    const char *pszUsage;
    /** The option descriptions for the --help screen. */
    const char *pszOptions;

    /**
     * Called before parsing arguments.
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnPreInit)(void);

    /**
     * Tries to parse the given command line options.
     *
     * @returns 0 if we parsed, -1 if it didn't and anything else means exit.
     * @param   argc        Argument count.
     * @param   argv        Arguments.
     */
    DECLCALLBACKMEMBER(int, pfnOption)(int argc, char **argv);

    /**
     * Called before parsing arguments.
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnInit)(void);

    /** Called from the watchdog's main function. Non-blocking.
     *
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnMain)(void);

    /**
     * Stop the module.
     */
    DECLCALLBACKMEMBER(int, pfnStop)(void);

    /**
     * Does termination cleanups.
     *
     * @remarks This may be called even if pfnInit hasn't been called!
     */
    DECLCALLBACKMEMBER(void, pfnTerm)(void);

    /** @name  Callbacks.
     * @{
     */

    /**
     *
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnOnMachineRegistered)(const Bstr &strUuid);

    /**
     *
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnOnMachineUnregistered)(const Bstr &strUuid);

    /**
     *
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnOnMachineStateChanged)(const Bstr &strUuid, MachineState_T enmState);

    /**
     *
     * @returns VBox status code.
     */
    DECLCALLBACKMEMBER(int, pfnOnServiceStateChanged)(bool fAvailable);

    /** @} */
} VBOXMODULE;
/** Pointer to a VBOXMODULE. */
typedef VBOXMODULE *PVBOXMODULE;
/** Pointer to a const VBOXMODULE. */
typedef VBOXMODULE const *PCVBOXMODULE;

RT_C_DECLS_BEGIN

extern bool                             g_fDryrun;
extern bool                             g_fVerbose;
extern ComPtr<IVirtualBox>              g_pVirtualBox;
extern ComPtr<ISession>                 g_pSession;
extern mapVM                            g_mapVM;
extern mapGroup                         g_mapGroup;
# ifdef VBOX_WATCHDOG_GLOBAL_PERFCOL
extern ComPtr<IPerformanceCollector>    g_pPerfCollector;
# endif

extern VBOXMODULE g_ModBallooning;
extern VBOXMODULE g_ModAPIMonitor;

extern void serviceLog(const char *pszFormat, ...);
#define serviceLogVerbose(a) if (g_fVerbose) { serviceLog a; }

extern int getMetric(PVBOXWATCHDOG_MACHINE pMachine, const Bstr& strName, LONG *pulData);
void* getPayload(PVBOXWATCHDOG_MACHINE pMachine, const char *pszModule);
int payloadAlloc(PVBOXWATCHDOG_MACHINE pMachine, const char *pszModule, size_t cbSize, void **ppszPayload);
void payloadFree(PVBOXWATCHDOG_MACHINE pMachine, const char *pszModule);
PVBOXWATCHDOG_MACHINE getMachine(const Bstr& strUuid);
MachineState_T getMachineState(const PVBOXWATCHDOG_MACHINE pMachine);

RT_C_DECLS_END

#endif /* !___H_VBOXWATCHDOG */


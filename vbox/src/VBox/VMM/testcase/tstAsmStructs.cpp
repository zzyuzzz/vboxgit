/* $Id: tstAsmStructs.cpp 93115 2022-01-01 11:31:46Z vboxsync $ */
/** @file
 * Testcase for checking offsets in the assembly structures shared with C/C++.
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
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
#include <VBox/vmm/cpum.h>
#include "CPUMInternal.h"
#define IPRT_WITHOUT_NAMED_UNIONS_AND_STRUCTS 1 /* For HMInternal */
#include "HMInternal.h"
#undef  IPRT_WITHOUT_NAMED_UNIONS_AND_STRUCTS /* probably not necessary */
#include "VMMInternal.h"
#include <VBox/vmm/vm.h>
#define GVM_C_STYLE_STRUCTURES
#include <VBox/vmm/gvm.h>
#include <VBox/vmm/hm_vmx.h>

#include "tstHelp.h"
#include <stdio.h>

/* Hack for validating nested HMCPU structures. */
typedef HMCPU::HMCPUVMX HMCPUVMX;
typedef HMCPU::HMCPUSVM HMCPUSVM;
typedef HMR0PERVCPU::HMR0CPUVMX HMR0CPUVMX;
typedef HMR0PERVCPU::HMR0CPUSVM HMR0CPUSVM;

/* For sup.mac simplifications. */
#define SUPDRVTRACERUSRCTX32    SUPDRVTRACERUSRCTX
#define SUPDRVTRACERUSRCTX64    SUPDRVTRACERUSRCTX


int main()
{
    int rc = 0;
    printf("tstAsmStructs: TESTING\n");

#ifdef IN_RING3
# include "tstAsmStructsHC.h"
#else
# include "tstAsmStructsRC.h"
#endif

    if (rc)
        printf("tstAsmStructs: FAILURE - %d errors \n", rc);
    else
        printf("tstAsmStructs: SUCCESS\n");
    return rc;
}

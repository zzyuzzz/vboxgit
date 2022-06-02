/** @file
 *
 * VBox Debugger GUI, dummy testcase.
 */

/*
 * Copyright (C) 2006-2007 Oracle Corporation
 *
 * Oracle Corporation confidential
 * All rights reserved
 */


/*******************************************************************************
*   Header Files                                                               *
*******************************************************************************/
#include <qapplication.h>
#include <VBox/dbggui.h>
#include <VBox/vm.h>
#include <VBox/err.h>
#include <iprt/initterm.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>
#include <iprt/semaphore.h>
#include <iprt/stream.h>


#define TESTCASE "tstVBoxDbg"


int main(int argc, char **argv)
{
    int     cErrors = 0;                  /* error count. */

    RTR3InitAndSUPLib();
    RTPrintf(TESTCASE ": TESTING...\n");

    /*
     * Create empty VM.
     */
    PVM pVM;
    int rc = VMR3Create(1, NULL, NULL, NULL, NULL, &pVM);
    if (RT_SUCCESS(rc))
    {
        /*
         * Instantiate the debugger GUI bits and run them.
         */
        QApplication App(argc, argv);
        PDBGGUI pGui;
        PCDBGGUIVT pGuiVT;
        rc = DBGGuiCreateForVM(pVM, &pGui, &pGuiVT);
        if (RT_SUCCESS(rc))
        {
            if (argc <= 1 || argc == 2)
            {
                RTPrintf(TESTCASE ": calling pfnShowCommandLine...\n");
                rc = pGuiVT->pfnShowCommandLine(pGui);
                if (RT_FAILURE(rc))
                {
                    RTPrintf(TESTCASE ": error: pfnShowCommandLine failed! rc=%Rrc\n", rc);
                    cErrors++;
                }
            }

            if (argc <= 1 || argc == 3)
            {
                RTPrintf(TESTCASE ": calling pfnShowStatistics...\n");
                pGuiVT->pfnShowStatistics(pGui);
                if (RT_FAILURE(rc))
                {
                    RTPrintf(TESTCASE ": error: pfnShowStatistics failed! rc=%Rrc\n", rc);
                    cErrors++;
                }
            }

            pGuiVT->pfnAdjustRelativePos(pGui, 0, 0, 640, 480);
            RTPrintf(TESTCASE ": calling App.exec()...\n");
            App.exec();
        }
        else
        {
            RTPrintf(TESTCASE ": error: DBGGuiCreateForVM failed! rc=%Rrc\n", rc);
            cErrors++;
        }

        /*
         * Cleanup.
         */
        rc = VMR3Destroy(pVM);
        if (!RT_SUCCESS(rc))
        {
            RTPrintf(TESTCASE ": error: failed to destroy vm! rc=%Rrc\n", rc);
            cErrors++;
        }
    }
    else
    {
        RTPrintf(TESTCASE ": fatal error: failed to create vm! rc=%Rrc\n", rc);
        cErrors++;
    }

    /*
     * Summay and exit.
     */
    if (!cErrors)
        RTPrintf(TESTCASE ": SUCCESS\n");
    else
        RTPrintf(TESTCASE ": FAILURE - %d errors\n", cErrors);
    return !!cErrors;
}


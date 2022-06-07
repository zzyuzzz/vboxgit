#!/usr/bin/env python
# -*- coding: utf-8 -*-
# $Id: virtual_test_sheriff.py 61282 2016-05-29 19:49:31Z vboxsync $
# pylint: disable=C0301

"""
Virtual Test Sheriff.

Duties:
    - Try to a assign failure reasons to recently failed tests.
    - Reboot or disable bad test boxes.

"""

__copyright__ = \
"""
Copyright (C) 2012-2016 Oracle Corporation

This file is part of VirtualBox Open Source Edition (OSE), as
available from http://www.virtualbox.org. This file is free software;
you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software
Foundation, in version 2 as it comes in the "COPYING" file of the
VirtualBox OSE distribution. VirtualBox OSE is distributed in the
hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.

The contents of this file may alternatively be used under the terms
of the Common Development and Distribution License Version 1.0
(CDDL) only, as it comes in the "COPYING.CDDL" file of the
VirtualBox OSE distribution, in which case the provisions of the
CDDL are applicable instead of those of the GPL.

You may elect to license modified versions of this file under the
terms and conditions of either the GPL or the CDDL or both.
"""
__version__ = "$Revision: 61282 $"


# Standard python imports
import sys;
import os;
from optparse import OptionParser;

# Add Test Manager's modules path
g_ksTestManagerDir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))));
sys.path.append(g_ksTestManagerDir);

# Test Manager imports
from testmanager.core.db            import TMDatabaseConnection;
from testmanager.core.testbox       import TestBoxLogic, TestBoxData;
from testmanager.core.testset       import TestSetLogic, TestSetData;
from testmanager.core.testresults   import TestResultLogic;
from testmanager.core.useraccount   import UserAccountLogic;


class VirtualTestSheriff(object): # pylint: disable=R0903
    """
    Add build info into Test Manager database.
    """

    ## The user account for the virtual sheriff.
    ksLoginName = 'vsheriff';


    def __init__(self):
        """
        Parse command line.
        """
        self.oDb = None;
        self.tsNow = None;
        self.oResultLogic = None;
        self.oTestSetLogic = None;
        self.oLogin = None;
        self.uidSelf = -1;

        oParser = OptionParser();
        oParser.add_option('--start-hours-ago', dest = 'cStartHoursAgo', metavar = '<hours>', default = 0,
                           help = 'When to start specified as hours relative to current time.  Defauls is right now.', );
        oParser.add_option('--hours-period', dest = 'cPeriodInHours', metavar = '<period-in-hours>', default = 2,
                           help = 'Work period specified in hours.  Defauls is 2 hours.');
        oParser.add_option('--real-run-back', dest = 'fRealRun', action = 'store_true', default = False,
                           help = 'Whether to commit the findings to the database. Default is a dry run.');
        oParser.add_option('-q', '--quiet', dest = 'fQuiet', action = 'store_true', default = False,
                           help = 'Quiet execution');
        oParser.add_option('-l', '--log', dest = 'sBuildLogPath', metavar = '<url>',
                           help = 'URL to the build logs (optional).');
        oParser.add_option('--debug', dest = 'fDebug', action = 'store_true', default = False,
                           help = 'Enables debug mode.');

        (self.oConfig, _) = oParser.parse_args();

    def eprint(self, sText):
        """
        Prints error messages.
        Returns 1 (for exit code usage.)
        """
        print 'error: %s' % (sText,);
        return 1;

    def dprint(self, sText):
        """
        Prints debug info.
        """
        if not self.oConfig.fDebug:
            print 'debug: %s' % (sText, );
        return 0;

    def vprint(self, sText):
        """
        Prints verbose info.
        """
        if not self.oConfig.fQuiet:
            print 'info: %s' % (sText,);
        return 0;


    def badTestBoxManagement(self):
        """
        Looks for bad test boxes and first tries once to reboot them then disables them.
        """
        rcExit = 0;

        #
        # We skip this entirely if we're running in the past and not in harmless debug mode.
        #
        if    self.oConfig.cStartHoursAgo != 0 \
          and (not self.oConfig.fDebug or self.oConfig.fRealRun):
            return rcExit;
        tsNow = self.oConfig.tsNow if self.oConfig.fDebug else None;
        oTestBoxLogic = TestBoxLogic(self.oDb);

        #
        # Get list of bad test boxes for given period and check them out individually.
        #
        aidBadTestBoxes = self.oTestSetLogic.fetchBadTestBoxIds();
        for idTestBox in aidBadTestBoxes:
            # Skip if the testbox is already disabled or has a pending reboot command.
            try:
                oTestBox = TestBoxData().initFromDbWithId(self.oDb, idTestBox);
            except Exception as oXcpt:
                rcExit = self.eprint('Failed to get data for test box #%u in badTestBoxManagement: %s' % (idTestBox, oXcpt,));
                continue;
            if not oTestBox.fEnabled:
                self.dprint('badTestBoxManagement: Skipping test box #%u (%s) as it has been disabled already.'
                            % ( idTestBox, oTestBox.sName, ));
                continue;
            if oTestBox.enmPendingCmd != TestBoxData.ksTestBoxCmd_None:
                self.dprint('badTestBoxManagement: Skipping test box #%u (%s) as it has a command pending: %s'
                            % ( idTestBox, oTestBox.sName, oTestBox.enmPendingCmd));
                continue;

            # Get the most recent testsets for this box (descending on tsDone) and see how bad it is.
            aoSets  = self.oTestSetLogic.fetchResultForTestBox(idTestBox, cHoursBack = 2, tsNow = tsNow);
            cOkay      = 0;
            cBad       = 0;
            iFirstOkay = len(aoSets);
            for iSet, oSet in enumerate(aoSets):
                if oSet.enmStatus == TestSetData.ksTestStatus_BadTestBox:
                    cBad += 1;
                else:
                    ## @todo maybe check the elapsed time here, it could still be a bad run.
                    iFirstOkay = iSet;
                    cOkay += 1;
                if iSet > 10:
                    break;

            # We react if there are two or more bad-testbox statuses at the head of the
            # history and at least three in the last 10 results.
            if iFirstOkay >= 2 and cBad > 2:
                if oTestBoxLogic.hasTestBoxRecentlyBeenRebooted(idTestBox, cHoursBack = 2, tsNow = tsNow):
                    self.vprint('Disabling testbox #%u (%s) - iFirstOkay=%u cBad=%u cOkay=%u'
                                % ( idTestBox, oTestBox.sName, iFirstOkay, cBad, cOkay));
                    if self.oConfig.fRealRun:
                        try:
                            oTestBoxLogic.disableTestBox(idTestBox, self.uidSelf, fCommit = True,
                                                         sComment = 'Automatically disabled (iFirstOkay=%u cBad=%u cOkay=%u)'
                                                                  % (iFirstOkay, cBad, cOkay),);
                        except Exception as oXcpt:
                            rcExit = self.eprint('Error disabling testbox #%u (%u): %s\n' % (idTestBox, oTestBox.sName, oXcpt,));
                else:
                    self.vprint('Rebooting testbox #%u (%s) - iFirstOkay=%u cBad=%u cOkay=%u'
                                % ( idTestBox, oTestBox.sName, iFirstOkay, cBad, cOkay));
                    if self.oConfig.fRealRun:
                        try:
                            oTestBoxLogic.rebootTestBox(idTestBox, self.uidSelf, fCommit = True,
                                                        sComment = 'Automatically rebooted (iFirstOkay=%u cBad=%u cOkay=%u)'
                                                                 % (iFirstOkay, cBad, cOkay),);
                        except Exception as oXcpt:
                            rcExit = self.eprint('Error rebooting testbox #%u (%u): %s\n' % (idTestBox, oTestBox.sName, oXcpt,));
        return rcExit;


    def reasoningFailures(self):
        """
        Guess the reason for failures.
        """

        return 0;


    def main(self):
        """
        The 'main' function.
        Return exit code (0, 1, etc).
        """
        # Database stuff.
        self.oDb = TMDatabaseConnection()
        self.oResultLogic = TestResultLogic(self.oDb);
        self.oTestSetLogic = TestSetLogic(self.oDb);

        # Get a fix on our 'now' before we do anything..
        self.oDb.execute('SELECT CURRENT_TIMESTAMP - interval \'%s hours\'', (self.oConfig.cStartHoursAgo,));
        self.tsNow = self.oDb.fetchOne();

        # If we're suppost to commit anything we need to get our user ID.
        rcExit = 0;
        if self.oConfig.fRealRun:
            self.oLogin = UserAccountLogic(self.oDb).tryFetchAccountByLoginName(VirtualTestSheriff.ksLoginName);
            if self.oLogin is None:
                rcExit = self.eprint('Cannot find my user account "%s"!' % (VirtualTestSheriff.ksLoginName,));
            else:
                self.uidSelf = self.oLogin.uid;

        if rcExit == 0:
            # Do the stuff.
            rcExit  = self.badTestBoxManagement();
            rcExit2 = self.reasoningFailures();
            if rcExit == 0:
                rcExit = rcExit2;

        # Cleanup.
        self.oTestSetLogic = None;
        self.oResultLogic = None;
        self.oDb.close();
        self.oDb = None;
        return rcExit;

if __name__ == '__main__':
    sys.exit(VirtualTestSheriff().main());


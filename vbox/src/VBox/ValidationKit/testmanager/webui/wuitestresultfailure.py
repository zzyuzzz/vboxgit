# -*- coding: utf-8 -*-
# $Id: wuitestresultfailure.py 61217 2016-05-26 20:04:05Z vboxsync $

"""
Test Manager WUI - Dummy Test Result Failure Reason Edit Dialog - just for error handling!
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
__version__ = "$Revision: 61217 $"

# Validation Kit imports.
from testmanager.webui.wuicontentbase   import WuiFormContentBase;
from testmanager.core.testresults       import TestResultFailureData;
from testmanager.core.failurereason     import FailureReasonLogic;


class WuiTestResultFailure(WuiFormContentBase):
    """
    WUI test result failure error form generator.
    """
    def __init__(self, oData, sMode, oDisp):
        if sMode == WuiFormContentBase.ksMode_Add:
            sTitle = 'Add Test Result Failure Reason';
        elif sMode == WuiFormContentBase.ksMode_Edit:
            sTitle = 'Modify Test Result Failure Reason';
        else:
            assert sMode == WuiFormContentBase.ksMode_Show;
            sTitle = 'Test Result Failure Reason';
        ## submit access.
        WuiFormContentBase.__init__(self, oData, sMode, 'TestResultFailure', oDisp, sTitle);

    def _populateForm(self, oForm, oData):

        aoFailureReasons = FailureReasonLogic(self._oDisp.getDb()).fetchForCombo('Todo: Figure out why');
        oForm.addComboBox(TestResultFailureData.ksParam_idFailureReason, oData.idFailureReason,
                          'Reason', aoFailureReasons)
        oForm.addMultilineText(TestResultFailureData.ksParam_sComment,   oData.sComment,     'Comment');
        oForm.addIntRO(      TestResultFailureData.ksParam_idTestResult, oData.idTestResult, 'Test Result ID');
        oForm.addTimestampRO(TestResultFailureData.ksParam_tsEffective,  oData.tsEffective,  'Effective Date');
        oForm.addTimestampRO(TestResultFailureData.ksParam_tsExpire,     oData.tsExpire,     'Expire (excl)');
        oForm.addIntRO(      TestResultFailureData.ksParam_uidAuthor,    oData.uidAuthor,    'Changed by UID');
        if self._sMode != WuiFormContentBase.ksMode_Show:
            oForm.addSubmit('Add' if self._sMode == WuiFormContentBase.ksMode_Add else 'Modify');
        return True;


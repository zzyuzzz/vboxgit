/* $Id: UIWizardNewCloudVM.cpp 91825 2021-10-18 12:26:24Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardNewCloudVM class implementation.
 */

/*
 * Copyright (C) 2009-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* GUI includes: */
#include "UIMessageCenter.h"
#include "UINotificationCenter.h"
#include "UIProgressObject.h"
#include "UIWizardNewCloudVM.h"
#include "UIWizardNewCloudVMPageSource.h"
#include "UIWizardNewCloudVMPageProperties.h"
#include "UIWizardNewCloudVMPageExpert.h"

/* COM includes: */
#include "CCloudMachine.h"


UIWizardNewCloudVM::UIWizardNewCloudVM(QWidget *pParent,
                                       const QString &strFullGroupName /* = QString() */)
    : UINativeWizard(pParent, WizardType_NewCloudVM)
{
#ifndef VBOX_WS_MAC
    /* Assign watermark: */
    setPixmapName(":/wizard_new_cloud_vm.png");
#else
    /* Assign background image: */
    setPixmapName(":/wizard_new_cloud_vm_bg.png");
#endif

    /* Parse passed full group name: */
    const QString strProviderShortName = strFullGroupName.section('/', 1, 1);
    const QString strProfileName = strFullGroupName.section('/', 2, 2);
    if (!strProviderShortName.isEmpty() && !strProfileName.isEmpty())
    {
        m_strProviderShortName = strProviderShortName;
        m_strProfileName = strProfileName;
    }
}

void UIWizardNewCloudVM::createVSDForm()
{
    /* Acquire prepared client and description: */
    CCloudClient comClient = client();
    CVirtualSystemDescription comVSD = vsd();
    AssertReturnVoid(comClient.isNotNull() && comVSD.isNotNull());

    /* Create launch VSD form: */
    UINotificationProgressLaunchVSDFormCreate *pNotification = new UINotificationProgressLaunchVSDFormCreate(comClient,
                                                                                                             comVSD,
                                                                                                             providerShortName(),
                                                                                                             profileName());
    connect(pNotification, &UINotificationProgressLaunchVSDFormCreate::sigVSDFormCreated,
            this, &UIWizardNewCloudVM::setVSDForm);
    handleNotificationProgressNow(pNotification);
}

bool UIWizardNewCloudVM::createCloudVM()
{
    /* Prepare result: */
    bool fResult = false;

    /* Acquire prepared client and description: */
    CCloudClient comClient = client();
    CVirtualSystemDescription comVSD = vsd();
    AssertReturn(comClient.isNotNull() && comVSD.isNotNull(), false);

    /* Initiate cloud VM creation procedure: */
    CCloudMachine comMachine;

    /* Create cloud VM: */
    UINotificationProgressCloudMachineCreate *pNotification = new UINotificationProgressCloudMachineCreate(comClient,
                                                                                                           comMachine,
                                                                                                           comVSD,
                                                                                                           providerShortName(),
                                                                                                           profileName());
    connect(pNotification, &UINotificationProgressCloudMachineCreate::sigCloudMachineCreated,
            &uiCommon(), &UICommon::sltHandleCloudMachineAdded);
    gpNotificationCenter->append(pNotification);

    /* Positive: */
    fResult = true;

    /* Return result: */
    return fResult;
}

void UIWizardNewCloudVM::populatePages()
{
    /* Create corresponding pages: */
    switch (mode())
    {
        case WizardMode_Basic:
        {
            addPage(new UIWizardNewCloudVMPageSource);
            addPage(new UIWizardNewCloudVMPageProperties);
            break;
        }
        case WizardMode_Expert:
        {
            addPage(new UIWizardNewCloudVMPageExpert);
            break;
        }
        default:
        {
            AssertMsgFailed(("Invalid mode: %d", mode()));
            break;
        }
    }
}

void UIWizardNewCloudVM::retranslateUi()
{
    /* Call to base-class: */
    UINativeWizard::retranslateUi();

    /* Translate wizard: */
    setWindowTitle(tr("Create Cloud Virtual Machine"));
    /// @todo implement this?
    //setButtonText(QWizard::FinishButton, tr("Create"));
}

/* $Id: UIWizardCloneVDPageBasic4.cpp 41021 2012-04-23 11:02:30Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * UIWizardCloneVDPageBasic4 class implementation
 */

/*
 * Copyright (C) 2006-2012 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Global includes: */
#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLineEdit>

/* Local includes: */
#include "UIWizardCloneVDPageBasic4.h"
#include "UIWizardCloneVD.h"
#include "COMDefs.h"
#include "VBoxGlobal.h"
#include "UIMessageCenter.h"
#include "UIIconPool.h"
#include "QIFileDialog.h"
#include "QIRichTextLabel.h"
#include "QIToolButton.h"

UIWizardCloneVDPage4::UIWizardCloneVDPage4()
{
}

void UIWizardCloneVDPage4::onSelectLocationButtonClicked()
{
    /* Get current folder and filename: */
    QFileInfo fullFilePath(mediumPath());
    QDir folder = fullFilePath.path();
    QString strFileName = fullFilePath.fileName();

    /* Set the first parent folder that exists as the current: */
    while (!folder.exists() && !folder.isRoot())
    {
        QFileInfo folderInfo(folder.absolutePath());
        if (folder == QDir(folderInfo.absolutePath()))
            break;
        folder = folderInfo.absolutePath();
    }

    /* But if it doesn't exists at all: */
    if (!folder.exists() || folder.isRoot())
    {
        /* Use recommended one folder: */
        QFileInfo defaultFilePath(absoluteFilePath(strFileName, m_strDefaultPath));
        folder = defaultFilePath.path();
    }

    /* Prepare backends list: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    CMediumFormat mediumFormat = fieldImp("mediumFormat").value<CMediumFormat>();
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    QStringList validExtensionList;
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == KDeviceType_HardDisk)
            validExtensionList << QString("*.%1").arg(fileExtensions[i]);
    /* Compose full filter list: */
    QString strBackendsList = QString("%1 (%2)").arg(mediumFormat.GetName()).arg(validExtensionList.join(" "));

    /* Open corresponding file-dialog: */
    QString strChosenFilePath = QIFileDialog::getSaveFileName(folder.absoluteFilePath(strFileName),
                                                              strBackendsList, thisImp(),
                                                              VBoxGlobal::tr("Choose a virtual hard disk file"));

    /* If there was something really chosen: */
    if (!strChosenFilePath.isEmpty())
    {
        /* If valid file extension is missed, append it: */
        if (QFileInfo(strChosenFilePath).suffix().isEmpty())
            strChosenFilePath += QString(".%1").arg(m_strDefaultExtension);
        m_pDestinationDiskEditor->setText(QDir::toNativeSeparators(strChosenFilePath));
        m_pDestinationDiskEditor->selectAll();
        m_pDestinationDiskEditor->setFocus();
    }
}

/* static */
QString UIWizardCloneVDPage4::toFileName(const QString &strName, const QString &strExtension)
{
    /* Convert passed name to native separators (it can be full, actually): */
    QString strFileName = QDir::toNativeSeparators(strName);

    /* Remove all trailing dots to avoid multiple dots before extension: */
    int iLen;
    while (iLen = strFileName.length(), iLen > 0 && strFileName[iLen - 1] == '.')
        strFileName.truncate(iLen - 1);

    /* Add passed extension if its not done yet: */
    if (QFileInfo(strFileName).suffix().toLower() != strExtension)
        strFileName += QString(".%1").arg(strExtension);

    /* Return result: */
    return strFileName;
}

/* static */
QString UIWizardCloneVDPage4::absoluteFilePath(const QString &strFileName, const QString &strDefaultPath)
{
    /* Wrap file-info around received file name: */
    QFileInfo fileInfo(strFileName);
    /* If path-info is relative or there is no path-info at all: */
    if (fileInfo.fileName() == strFileName || fileInfo.isRelative())
    {
        /* Resolve path on the basis of default path we have: */
        fileInfo = QFileInfo(strDefaultPath, strFileName);
    }
    /* Return full absolute hard disk file path: */
    return QDir::toNativeSeparators(fileInfo.absoluteFilePath());
}

/* static */
QString UIWizardCloneVDPage4::defaultExtension(const CMediumFormat &mediumFormatRef)
{
    /* Load extension / device list: */
    QVector<QString> fileExtensions;
    QVector<KDeviceType> deviceTypes;
    CMediumFormat mediumFormat(mediumFormatRef);
    mediumFormat.DescribeFileExtensions(fileExtensions, deviceTypes);
    for (int i = 0; i < fileExtensions.size(); ++i)
        if (deviceTypes[i] == KDeviceType_HardDisk)
            return fileExtensions[i].toLower();
    AssertMsgFailed(("Extension can't be NULL!\n"));
    return QString();
}

QString UIWizardCloneVDPage4::mediumPath() const
{
    return absoluteFilePath(toFileName(m_pDestinationDiskEditor->text(), m_strDefaultExtension), m_strDefaultPath);
}

qulonglong UIWizardCloneVDPage4::mediumSize() const
{
    const CMedium &sourceVirtualDisk = fieldImp("sourceVirtualDisk").value<CMedium>();
    return sourceVirtualDisk.isNull() ? 0 : sourceVirtualDisk.GetLogicalSize();
}

UIWizardCloneVDPageBasic4::UIWizardCloneVDPageBasic4()
{
    /* Create widgets: */
    QVBoxLayout *pMainLayout = new QVBoxLayout(this);
    {
        m_pLabel = new QIRichTextLabel(this);
        m_pDestinationCnt = new QGroupBox(this);
        {
            m_pDestinationCnt->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            QHBoxLayout *pLocationCntLayout = new QHBoxLayout(m_pDestinationCnt);
            {
                m_pDestinationDiskEditor = new QLineEdit(m_pDestinationCnt);
                m_pDestinationDiskOpenButton = new QIToolButton(m_pDestinationCnt);
                {
                    m_pDestinationDiskOpenButton->setAutoRaise(true);
                    m_pDestinationDiskOpenButton->setIcon(UIIconPool::iconSet(":/select_file_16px.png", "select_file_dis_16px.png"));
                }
                pLocationCntLayout->addWidget(m_pDestinationDiskEditor);
                pLocationCntLayout->addWidget(m_pDestinationDiskOpenButton);
            }
        }
        pMainLayout->addWidget(m_pLabel);
        pMainLayout->addWidget(m_pDestinationCnt);
        pMainLayout->addStretch();
    }

    /* Setup page connections: */
    connect(m_pDestinationDiskEditor, SIGNAL(textChanged(const QString &)), this, SIGNAL(completeChanged()));
    connect(m_pDestinationDiskOpenButton, SIGNAL(clicked()), this, SLOT(sltSelectLocationButtonClicked()));

    /* Register fields: */
    registerField("mediumPath", this, "mediumPath");
    registerField("mediumSize", this, "mediumSize");
}

void UIWizardCloneVDPageBasic4::sltSelectLocationButtonClicked()
{
    /* Call to base-class: */
    onSelectLocationButtonClicked();
}

void UIWizardCloneVDPageBasic4::retranslateUi()
{
    /* Translate page: */
    setTitle(UIWizardCloneVD::tr("Virtual disk file location"));

    /* Translate widgets: */
    m_pLabel->setText(UIWizardCloneVD::tr("Please type the name of the new virtual disk file into the box below or "
                                          "click on the folder icon to select a different folder to create the file in."));
    m_pDestinationCnt->setTitle(UIWizardCloneVD::tr("&Location"));
    m_pDestinationDiskOpenButton->setToolTip(UIWizardCloneVD::tr("Choose a virtual hard disk file..."));
}

void UIWizardCloneVDPageBasic4::initializePage()
{
    /* Translate page: */
    retranslateUi();

    /* Get source virtual-disk file-information: */
    QFileInfo sourceFileInfo(field("sourceVirtualDisk").value<CMedium>().GetLocation());
    /* Get default path for virtual-disk copy: */
    m_strDefaultPath = sourceFileInfo.absolutePath();
    /* Get default extension for virtual-disk copy: */
    m_strDefaultExtension = defaultExtension(field("mediumFormat").value<CMediumFormat>());
    /* Compose default-name for virtual-disk copy: */
    QString strMediumName = UIWizardCloneVD::tr("%1_copy", "copied virtual disk name").arg(sourceFileInfo.baseName());
    /* Set default-name as text for location editor: */
    m_pDestinationDiskEditor->setText(strMediumName);
}

bool UIWizardCloneVDPageBasic4::isComplete() const
{
    /* Make sure current name is not empty: */
    return !m_pDestinationDiskEditor->text().trimmed().isEmpty();
}

bool UIWizardCloneVDPageBasic4::validatePage()
{
    /* Make sure such virtual-disk doesn't exists already: */
    QString strMediumPath(mediumPath());
    if (QFileInfo(strMediumPath).exists())
    {
        msgCenter().sayCannotOverwriteHardDiskStorage(this, strMediumPath);
        return false;
    }
    return true;
}


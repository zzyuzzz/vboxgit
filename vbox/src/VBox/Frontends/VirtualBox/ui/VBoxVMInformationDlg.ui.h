/**
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * "VirtualBox Information Dialog" dialog UI include (Qt Designer)
 */

/*
 * Copyright (C) 2006 innotek GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


VBoxVMInformationDlg::InfoDlgMap VBoxVMInformationDlg::mSelfArray = InfoDlgMap();

void VBoxVMInformationDlg::createInformationDlg (const CSession &aSession,
                                                 VBoxConsoleView *aConsole)
{
    CMachine machine = aSession.GetMachine();
    if (mSelfArray.find (machine.GetName()) == mSelfArray.end())
    {
        /* creating new information dialog if there is no one existing */
        mSelfArray [machine.GetName()] = new VBoxVMInformationDlg (
            aConsole,
            "VBoxVMInformationDlg", WType_TopLevel | WDestructiveClose);
        /* read new machine data for this information dialog */
        mSelfArray [machine.GetName()]->setup (aSession, aConsole);
    }

    VBoxVMInformationDlg *info = mSelfArray [machine.GetName()];
    info->show();
    info->raise();
    info->setWindowState (info->windowState() & ~WindowMinimized);
    info->setActiveWindow();
}


void VBoxVMInformationDlg::init()
{
    /* dialog initially is not polished */
    mIsPolished = false;

    /* search the default button */
    mDefaultButton = searchDefaultButton();
    qApp->installEventFilter (this);

    /* setup a dialog icon */
    setIcon (QPixmap::fromMimeSource ("description_16px.png"));

    /* statusbar initially disabled */
    statusBar()->setHidden (true);

    /* setup size grip */
    mSizeGrip = new QSizeGrip (centralWidget(), "mSizeGrip");
    mSizeGrip->resize (mSizeGrip->sizeHint());
    mSizeGrip->stackUnder (mCloseButton);

    /* logs list creation */
    mInfoStack = new QTabWidget (mInfoFrame, "mInfoStack");
    mInfoStack->setMargin (10);
    QVBoxLayout *infoFrameLayout = new QVBoxLayout (mInfoFrame);
    infoFrameLayout->addWidget (mInfoStack);

    /* details view creation */
    mDetailsText = new QTextBrowser();
    mDetailsText->setFrameShape (QFrame::NoFrame);
    mDetailsText->setPaper (backgroundBrush());
    mInfoStack->addTab (mDetailsText,
                        VBoxGlobal::iconSet ("settings_16px.png"),
                        QString::null);

    /* statistic view creation */
    mStatisticText = new QTextBrowser();
    mStatisticText->setFrameShape (QFrame::NoFrame);
    mStatisticText->setPaper (backgroundBrush());
    mInfoStack->addTab (mStatisticText,
                        VBoxGlobal::iconSet ("state_running_16px.png"),
                        QString::null);

    /* full list of statistics counters to get total info */
    // mDefStatText = new QTextBrowser();
    // mDefStatText->setFrameShape (QFrame::NoFrame);
    // mDefStatText->setPaper (backgroundBrush());
    // mInfoStack->addTab (mDefStatText,
    //                     VBoxGlobal::iconSet ("show_logs_16px.png"),
    //                     QString::null);

    /* applying language settings */
    languageChangeImp();

    /* show statistics page and make it focused */
    connect (mInfoStack, SIGNAL (currentChanged (QWidget*)),
             this, SLOT (onPageChanged (QWidget*)));
    mInfoStack->showPage (mStatisticText);
}


void VBoxVMInformationDlg::destroy()
{
    /* save dialog attributes for this vm */
    QString dlgsize ("%1,%2,%3");
    mSession.GetMachine().SetExtraData (VBoxDefs::GUI_InfoDlgState,
        dlgsize.arg (mWidth).arg (mHeight).arg (isMaximized() ? "max" : "normal"));

    if (!mSession.isNull() && !mSession.GetMachine().isNull())
        mSelfArray.erase (mSession.GetMachine().GetName());
}


void VBoxVMInformationDlg::setup (const CSession &aSession,
                                  VBoxConsoleView *aConsole)
{
    /* store related machine pointers */
    mSession = aSession;
    mConsole = aConsole;

    /* loading language constants */
    languageChangeImp();

    /* details page update */
    updateDetails();

    /* statistics page update */
    processStatistics();
    mStatTimer.start (5000);

    /* setup handlers */
    connect (&vboxGlobal(), SIGNAL (mediaEnumFinished (const VBoxMediaList &)),
             this, SLOT (updateDetails()));
    connect (mConsole, SIGNAL (mediaChanged (VBoxDefs::DiskType)),
             this, SLOT (updateDetails()));
    connect (mConsole, SIGNAL (sharedFoldersChanged()),
             this, SLOT (updateDetails()));

    connect (&mStatTimer, SIGNAL (timeout()), this, SLOT (processStatistics()));
    connect (mConsole, SIGNAL (resizeHintDone()), this, SLOT (processStatistics()));

    /* preload dialog attributes for this vm */
    QString dlgsize = mSession.GetMachine().GetExtraData (VBoxDefs::GUI_InfoDlgState);
    if (dlgsize.isNull())
    {
        mWidth = 400;
        mHeight = 450;
        mMax = false;
    }
    else
    {
        QStringList list = QStringList::split (',', dlgsize);
        mWidth = list [0].toInt(), mHeight = list [1].toInt();
        mMax = list [2] == "max";
    }
}


void VBoxVMInformationDlg::languageChangeImp()
{
    /* Setup a dialog caption. */
    if (!mSession.isNull() && !mSession.GetMachine().isNull())
        setCaption (tr ("%1 - Session Information")
        .arg (mSession.GetMachine().GetName()));

    /* Setup a tabwidget page names. */
    mInfoStack->changeTab (mDetailsText, tr ("&Details"));
    mInfoStack->changeTab (mStatisticText, tr ("&Runtime"));
    // mInfoStack->changeTab (mDefStatText, tr ("De&fault Stat"));

    /* Clear counter names initially. */
    mNamesMap.clear();

    /* HD statistics: */
    mNamesMap ["/Devices/ATA0/Unit0/*DMA"] = tr ("DMA Transfers");
    mNamesMap ["/Devices/ATA0/Unit0/*PIO"] = tr ("PIO Transfers");
    mNamesMap ["/Devices/ATA0/Unit0/ReadBytes"] = tr ("Data Read");
    mNamesMap ["/Devices/ATA0/Unit0/WrittenBytes"] = tr ("Data Written");

    mNamesMap ["/Devices/ATA0/Unit1/*DMA"] = tr ("DMA Transfers");
    mNamesMap ["/Devices/ATA0/Unit1/*PIO"] = tr ("PIO Transfers");
    mNamesMap ["/Devices/ATA0/Unit1/ReadBytes"] = tr ("Data Read");
    mNamesMap ["/Devices/ATA0/Unit1/WrittenBytes"] = tr ("Data Written");

    mNamesMap ["/Devices/ATA1/Unit0/*DMA"] = tr ("DMA Transfers");
    mNamesMap ["/Devices/ATA1/Unit0/*PIO"] = tr ("PIO Transfers");
    mNamesMap ["/Devices/ATA1/Unit0/ReadBytes"] = tr ("Data Read");
    mNamesMap ["/Devices/ATA1/Unit0/WrittenBytes"] = tr ("Data Written");

    mNamesMap ["/Devices/ATA1/Unit1/*DMA"] = tr ("DMA Transfers");
    mNamesMap ["/Devices/ATA1/Unit1/*PIO"] = tr ("PIO Transfers");
    mNamesMap ["/Devices/ATA1/Unit1/ReadBytes"] = tr ("Data Read");
    mNamesMap ["/Devices/ATA1/Unit1/WrittenBytes"] = tr ("Data Written");

    mNamesMap ["/Devices/PCNet0/TransmitBytes"] = tr ("Data Transmitted");
    mNamesMap ["/Devices/PCNet0/ReceiveBytes"] = tr ("Data Received");

    mNamesMap ["/Devices/PCNet1/TransmitBytes"] = tr ("Data Transmitted");
    mNamesMap ["/Devices/PCNet1/ReceiveBytes"] = tr ("Data Received");

    mNamesMap ["/Devices/PCNet2/TransmitBytes"] = tr ("Data Transmitted");
    mNamesMap ["/Devices/PCNet2/ReceiveBytes"] = tr ("Data Received");

    mNamesMap ["/Devices/PCNet3/TransmitBytes"] = tr ("Data Transmitted");
    mNamesMap ["/Devices/PCNet3/ReceiveBytes"] = tr ("Data Received");

    /* Statistics page update. */
    refreshStatistics();
}


QPushButton* VBoxVMInformationDlg::searchDefaultButton()
{
    /* this mechanism is used for searching the default dialog button
     * and similar the same mechanism in Qt::QDialog inner source */
    QPushButton *button = 0;
    QObjectList *list = queryList ("QPushButton");
    QObjectListIt it (*list);
    while ((button = (QPushButton*)it.current()) && !button->isDefault())
        ++ it;
    return button;
}


bool VBoxVMInformationDlg::eventFilter (QObject *aObject, QEvent *aEvent)
{
    switch (aEvent->type())
    {
        /* auto-default button focus-in processor used to move the "default"
         * button property into the currently focused button */
        case QEvent::FocusIn:
        {
            if (aObject->inherits ("QPushButton") &&
                aObject->parent() == centralWidget())
            {
                ((QPushButton*)aObject)->setDefault (aObject != mDefaultButton);
                if (mDefaultButton)
                    mDefaultButton->setDefault (aObject == mDefaultButton);
            }
            break;
        }
        /* auto-default button focus-out processor used to remove the "default"
         * button property from the previously focused button */
        case QEvent::FocusOut:
        {
            if (aObject->inherits ("QPushButton") &&
                aObject->parent() == centralWidget())
            {
                if (mDefaultButton)
                    mDefaultButton->setDefault (aObject != mDefaultButton);
                ((QPushButton*)aObject)->setDefault (aObject == mDefaultButton);
            }
            break;
        }
        default:
            break;
    }
    return QMainWindow::eventFilter (aObject, aEvent);
}


bool VBoxVMInformationDlg::event (QEvent *aEvent)
{
    bool result = QMainWindow::event (aEvent);
    switch (aEvent->type())
    {
        case QEvent::LanguageChange:
        {
            languageChangeImp();
            break;
        }
        case QEvent::WindowStateChange:
        {
            if (mIsPolished)
                mMax = isMaximized();
            else if (mMax == isMaximized())
                mIsPolished = true;
            break;
        }
        default:
            break;
    }
    return result;
}


void VBoxVMInformationDlg::keyPressEvent (QKeyEvent *aEvent)
{
    if (aEvent->state() == 0 ||
        (aEvent->state() & Keypad && aEvent->key() == Key_Enter))
    {
        switch (aEvent->key())
        {
            /* processing the return keypress for the auto-default button */
            case Key_Enter:
            case Key_Return:
            {
                QPushButton *currentDefault = searchDefaultButton();
                if (currentDefault)
                    currentDefault->animateClick();
                break;
            }
            /* processing the escape keypress as the close dialog action */
            case Key_Escape:
            {
                close();
                break;
            }
        }
    }
    else
        aEvent->ignore();
}


void VBoxVMInformationDlg::showEvent (QShowEvent *aEvent)
{
    QMainWindow::showEvent (aEvent);

    /* one may think that QWidget::polish() is the right place to do things
     * below, but apparently, by the time when QWidget::polish() is called,
     * the widget style & layout are not fully done, at least the minimum
     * size hint is not properly calculated. Since this is sometimes necessary,
     * we provide our own "polish" implementation. */

    if (mIsPolished)
        return;

    /* load window size and state */
    resize (mWidth, mHeight);
    if (mMax)
        QTimer::singleShot (0, this, SLOT (showMaximized()));
    else
        mIsPolished = true;

    VBoxGlobal::centerWidget (this, parentWidget());
}


void VBoxVMInformationDlg::resizeEvent (QResizeEvent*)
{
    /* adjust the size-grip location for the current resize event */
    mSizeGrip->move (centralWidget()->rect().bottomRight() -
                     QPoint (mSizeGrip->rect().width() - 1,
                     mSizeGrip->rect().height() - 1));

    /* store dialog size for this vm */
    if (mIsPolished && !isMaximized())
    {
        mWidth = width();
        mHeight = height();
    }
}


void VBoxVMInformationDlg::updateDetails()
{
    /* details page update */
    mDetailsText->setText (
        vboxGlobal().detailsReport (mSession.GetMachine(), false /* isNewVM */,
                                    false /* withLinks */, false /* refresh */));
}


void VBoxVMInformationDlg::onPageChanged (QWidget *aPage)
{
    /* focusing the browser on shown page */
    aPage->setFocus();
}


void VBoxVMInformationDlg::processStatistics()
{
    CMachineDebugger dbg = mSession.GetConsole().GetDebugger();
    QString info;

    /* Look for all statistics for filtering purposes. */
    // dbg.GetStats ("*", false, info);
    // mDefStatText->setText (info);

    /* Process selected statistics: */
    for (DataMapType::const_iterator it = mNamesMap.begin();
         it != mNamesMap.end(); ++ it)
    {
        dbg.GetStats (it.key(), true, info);
        mValuesMap [it.key()] = parseStatistics (info);
    }

    /* Statistics page update. */
    refreshStatistics();
}


QString VBoxVMInformationDlg::parseStatistics (const QString &aText)
{
    /* Filters the statistic counters body. */
    QRegExp query ("^.+<Statistics>\n(.+)\n</Statistics>.*$");
    if (query.search (aText) == -1)
        return QString::null;

    QStringList wholeList = QStringList::split ("\n", query.cap (1));

    ULONG64 summa = 0;
    for (QStringList::Iterator lineIt = wholeList.begin();
         lineIt != wholeList.end(); ++ lineIt)
    {
        QString text = *lineIt;
        text.remove (1, 1);
        text.remove (text.length() - 2, 2);

        /* Parse incoming counter and fill the counter-element values. */
        CounterElementType counter;
        counter.type = text.section (" ", 0, 0);
        text = text.section (" ", 1);
        QStringList list = QStringList::split ("\" ", text);
        for (QStringList::Iterator it = list.begin(); it != list.end(); ++ it)
        {
            QString pair = *it;
            QRegExp regExp ("^(.+)=\"([^\"]*)\"?$");
            regExp.search (pair);
            counter.list.insert (regExp.cap (1), regExp.cap (2));
        }

        /* Fill the output with the necessary counter's value.
         * Currently we are using "c" field of simple counter only. */
        QString result = counter.list.contains ("c") ? counter.list ["c"] : "0";
        summa += result.toULongLong();
    }

    return QString::number (summa);
}


void VBoxVMInformationDlg::refreshStatistics()
{
    if (mSession.isNull())
        return;

    QString table = "<p><table border=0 cellspacing=0 cellpadding=0 width=100%>%1</table></p>";
    QString hdrRow = "<tr><td align=left><img src='%1'></td><td colspan=3><b>%2</b></td></tr>";
    QString bdyRow = "<tr><td></td><td><nobr>%1</nobr></td><td colspan=2><nobr>%2</nobr></td></tr>";
    QString paragraph = "<tr><td colspan=4></td></tr>";
    QString interline = "<tr><td colspan=4><font size=1>&nbsp;</font></td></tr>";
    QString result;

    /* Screen & VT-X Runtime Parameters */
    {
        CConsole console = mSession.GetConsole();
        ULONG bpp = console.GetDisplay().GetBitsPerPixel();
        QString resolution = QString ("%1x%2")
            .arg (console.GetDisplay().GetWidth())
            .arg (console.GetDisplay().GetHeight());
        if (bpp)
            resolution += QString ("x%1").arg (bpp);
        QString virt = console.GetDebugger().GetHWVirtExEnabled() ?
            tr ("Enabled") : tr ("Disabled");

        result += hdrRow.arg ("state_running_16px.png").arg (tr ("Runtime Attributes"));
        result += bdyRow.arg (tr ("Screen Resolution")).arg (resolution) +
                  bdyRow.arg (tr ("Hardware Virtualization")).arg (virt);
        result += paragraph;
    }

    /* Hard Disk Statistics. */
    QString primaryMaster = QString ("%1 %2")
        .arg (vboxGlobal().toString (KDiskControllerType_IDE0))
        .arg (vboxGlobal().toString (KDiskControllerType_IDE0, 0));
    QString primarySlave = QString ("%1 %2")
        .arg (vboxGlobal().toString (KDiskControllerType_IDE0))
        .arg (vboxGlobal().toString (KDiskControllerType_IDE0, 1));
    QString secondarySlave = QString ("%1 %2")
        .arg (vboxGlobal().toString (KDiskControllerType_IDE1))
        .arg (vboxGlobal().toString (KDiskControllerType_IDE1, 1));

    result += hdrRow.arg ("hd_16px.png").arg (tr ("IDE Hard Disk Statistics"));
    result += formatHardDisk (primaryMaster, KDiskControllerType_IDE0, 0, 0, 1);
    result += interline;
    result += formatHardDisk (primarySlave, KDiskControllerType_IDE0, 1, 4, 5);
    result += interline;
    result += formatHardDisk (secondarySlave, KDiskControllerType_IDE1, 1, 12, 13);
    result += paragraph;

    /* CD/DVD-ROM Statistics. */
    result += hdrRow.arg ("cd_16px.png").arg (tr ("CD/DVD-ROM Statistics"));
    result += formatHardDisk (QString::null /* tr ("Secondary Master") */,
                              KDiskControllerType_IDE1, 0, 8, 9);
    result += paragraph;

    /* Network Adapters Statistics. */
    result += hdrRow.arg ("nw_16px.png").arg (tr ("Network Adapter Statistics"));
    result += formatAdapter (tr ("Adapter 1"), 0, 16, 17);
    result += interline;
    result += formatAdapter (tr ("Adapter 2"), 1, 18, 19);
    result += interline;
    result += formatAdapter (tr ("Adapter 3"), 2, 20, 21);
    result += interline;
    result += formatAdapter (tr ("Adapter 4"), 3, 22, 23);

    /* Show full composed page. */
    mStatisticText->setText (table.arg (result));
}


QString VBoxVMInformationDlg::formatHardDisk (const QString &aName,
                                              KDiskControllerType aType,
                                              LONG aSlot, int aStart, int aFinish)
{
    if (mSession.isNull())
        return QString::null;

    QString header = "<tr><td></td><td colspan=3><nobr><u>%1</u></nobr></td></tr>";
    CMachine machine = mSession.GetMachine();

    QString result = aName.isNull() ? QString::null : header.arg (aName);
    CHardDisk hd = machine.GetHardDisk (aType, aSlot);
    if (!hd.isNull() || (aType == KDiskControllerType_IDE1 && aSlot == 0))
    {
        result += composeArticle (QString::null, aStart, aFinish);
        result += composeArticle ("B", aStart + 2, aFinish + 2);
    }
    else
        result += composeArticle (tr ("Not attached"), -1, -1);
    return result;
}

QString VBoxVMInformationDlg::formatAdapter (const QString &aName,
                                             ULONG aSlot,
                                             int aStart, int aFinish)

{
    if (mSession.isNull())
        return QString::null;

    QString header = "<tr><td></td><td colspan=3><nobr><u>%1</u></nobr></td></tr>";
    CMachine machine = mSession.GetMachine();

    QString result = header.arg (aName);
    CNetworkAdapter na = machine.GetNetworkAdapter (aSlot);
    result += na.GetEnabled() ?
        composeArticle ("B", aStart, aFinish) :
        composeArticle (tr ("Disabled"), -1, -1);
    return result;
}


QString VBoxVMInformationDlg::composeArticle (const QString &aUnits,
                                              int aStart, int aFinish)
{
    QString body = "<tr><td></td><td><nobr>%1</nobr></td><td align=right><nobr>%2%3</nobr></td><td width=100%></td></tr>";

    QString result;

    if (aStart == -1 && aFinish == -1)
        result += body.arg (aUnits).arg (QString::null).arg (QString::null);
    else for (int id = aStart; id <= aFinish; ++ id)
    {
        QString line = body;
        if (mValuesMap.contains (mNamesMap.keys() [id]))
        {
            ULONG64 value = mValuesMap.values() [id].toULongLong();
            line = line.arg (mNamesMap.values() [id])
                       .arg (QString ("%L1").arg (value));
            line = aUnits.isNull() ?
                line.arg (QString ("<img src=tpixel.png width=%1 height=1>")
                          .arg (QApplication::fontMetrics().width (" B"))) :
                line.arg (QString (" %1").arg (aUnits));
        }
        result += line;
    }

    return result;
}


/* Old code for two columns support */
#if 0
void VBoxVMInformationDlg::refreshStatistics()
{
    QString table = "<p><table border=0 cellspacing=0 cellpadding=0 width=100%>%1</table></p>";
    QString hdrRow = "<tr><td align=left><img src='%1'></td><td colspan=4><b>%2</b></td></tr>";
    QString subRow = "<tr><td></td><td colspan=2><nobr><u>%1</u></nobr></td>"
                                  "<td colspan=2><nobr><u>%2</u></nobr></td></tr>";
    QString bdyRow = "<tr><td></td><td><nobr>%1</nobr></td><td width=50%><nobr>%2</nobr></td>"
                                  "<td><nobr>%3</nobr></td><td width=50%><nobr>%4</nobr></td></tr>";
    QString paragraph = "<tr><td colspan=5></td></tr>";
    QString interline = "<tr><td colspan=5><font size=1>&nbsp;</font></td></tr>";
    QString result;

    /* Screen & VT-X Runtime Parameters */
    if (!mSession.isNull())
    {
        CConsole console = mSession.GetConsole();
        ULONG bpp = console.GetDisplay().GetBitsPerPixel();
        QString resolution = QString ("%1x%2")
            .arg (console.GetDisplay().GetWidth())
            .arg (console.GetDisplay().GetHeight());
        if (bpp)
            resolution += QString ("x%1").arg (bpp);
        QString virt = console.GetDebugger().GetHWVirtExEnabled() ?
            tr ("Enabled") : tr ("Disabled");

        result += hdrRow.arg ("state_running_16px.png").arg (tr ("Runtime Attributes"));
        result += bdyRow.arg (tr ("Screen Resolution")) .arg (resolution)
                        .arg (tr ("Hardware Virtualization")).arg (virt);
        result += paragraph;
    }

    /* Hard Disk Statistics. */
    result += hdrRow.arg ("hd_16px.png").arg (tr ("Hard Disks Statistics"));

    result += subRow.arg (tr ("Primary Master")).arg (tr ("Primary Slave"));
    result += composeArticle (QString::null, 0, 1, 4, 5);
    result += composeArticle ("B", 2, 3, 6, 7);
    result += interline;

    result += subRow.arg (tr ("Secondary Master")).arg (tr ("Secondary Slave"));
    result += composeArticle (QString::null, 8, 9, 12, 13);
    result += composeArticle ("B", 10, 11, 14, 15);
    result += paragraph;

    /* Network Adapters Statistics. Counters are currently missed. */
    result += hdrRow.arg ("nw_16px.png").arg (tr ("Network Adapter Statistics"));
    result += subRow.arg (tr ("Adapter 1")).arg (tr ("Adapter 2"));
    result += composeArticle ("B", 16, 17, 18, 19);

    /* Show full composed page. */
    mStatisticText->setText (table.arg (result));
}


QString VBoxVMInformationDlg::composeArticle (const QString &aUnits,
                                              int aStart1, int aFinish1,
                                              int aStart2, int aFinish2)
{
    QString bdyRow = "<tr><td></td><td><nobr>%1</nobr></td><td width=50%><nobr>%2</nobr></td>"
                                  "<td><nobr>%3</nobr></td><td width=50%><nobr>%4</nobr></td></tr>";

    QString result;

    int id1 = aStart1, id2 = aStart2;
    while (id1 <= aFinish1 || id2 <= aFinish2)
    {
        QString line = bdyRow;
        /* Processing first column */
        if (id1 > aFinish1)
        {
            line = line.arg (QString::null).arg (QString::null)
                       .arg (QString::null).arg (QString::null);
        }
        else if (mValuesMap.contains (mNamesMap.keys() [id1]))
        {
            line = line.arg (mNamesMap.values() [id1]);
            ULONG64 value = mValuesMap.values() [id1].toULongLong();
            line = aUnits.isNull() ?
                line.arg (QString ("%L1").arg (value)) :
                line.arg (QString ("%L1 %2").arg (value).arg (aUnits));
        }
        /* Processing second column */
        if (id2 > aFinish2)
        {
            line = line.arg (QString::null).arg (QString::null)
                       .arg (QString::null).arg (QString::null);
        }
        else if (mValuesMap.contains (mNamesMap.keys() [id2]))
        {
            line = line.arg (mNamesMap.values() [id2]);
            ULONG64 value = mValuesMap.values() [id2].toULongLong();
            line = aUnits.isNull() ?
                line.arg (QString ("%L1").arg (value)) :
                line.arg (QString ("%L1 %2").arg (value).arg (aUnits));
        }
        result += line;
        ++ id1; ++ id2;
    }

    return result;
}
#endif


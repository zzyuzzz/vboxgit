/** @file
 *
 * VBox frontends: Qt GUI ("VirtualBox"):
 * VBoxProblemReporter class implementation
 */

/*
 * Copyright (C) 2006 InnoTek Systemberatung GmbH
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation,
 * in version 2 as it comes in the "COPYING" file of the VirtualBox OSE
 * distribution. VirtualBox OSE is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * If you received this file as part of a commercial VirtualBox
 * distribution, then only the terms of your commercial VirtualBox
 * license agreement apply instead of the previous paragraph.
 */

#include "VBoxProblemReporter.h"

#include "VBoxGlobal.h"
#include "VBoxSelectorWnd.h"
#include "VBoxConsoleWnd.h"

#include "VBoxAboutDlg.h"

#include <qmessagebox.h>
#include <qprogressdialog.h>
#include <qcursor.h>
#include <qprocess.h>
#include <qeventloop.h>
#include <qregexp.h>

#include <iprt/err.h>

/**
 *  A QProgressDialog enhancement that allows to:
 *
 *  1) prevent closing the dialog when it has no cancel button;
 *  2) effectively track the IProgress object completion
 *     (w/o using IProgress::waitForCompletion() and w/o blocking the UI thread
 *     in any other way for too long).
 *
 *  @note The CProgress instance is passed as a non-const reference to the
 *        constructor (to memorize COM errors if they happen), and therefore
 *        must not be destroyed before the created VBoxProgressDialog instance
 *        is destroyed.
 */
class VBoxProgressDialog : public QProgressDialog
{
public:

    VBoxProgressDialog (CProgress &aProgress, const QString &aTitle,
                        int aMinDuration = 2000, QWidget *aCreator = 0,
                        const char *aName = 0)
        : QProgressDialog (aCreator, aName, true,
                           WStyle_Customize | WStyle_DialogBorder | WStyle_Title)
        , mProgress (aProgress)
        , mCalcelEnabled (true)
        , mOpCount (mProgress.GetOperationCount())
        , mCurOp (mProgress.GetOperation() + 1)
        , mLoopLevel (-1)
        , mEnded (false)
    {
        if (mOpCount > 1)
            setLabelText (QString (sOpDescTpl)
                          .arg (mProgress.GetOperationDescription())
                          .arg (mCurOp).arg (mOpCount));
        else
            setLabelText (QString ("%1...")
                          .arg (mProgress.GetOperationDescription()));
        setCancelButtonText (QString::null);
        setTotalSteps (100);
        setCaption (QString ("%1: %2")
                    .arg (aTitle, mProgress.GetDescription()));
        setMinimumDuration (aMinDuration);
        setCancelEnabled (false);
        setProgress (0);
    }

    int run (int aRefreshInterval);

    bool cancelEnabled() const { return mCalcelEnabled; }
    void setCancelEnabled (bool aEnabled) { mCalcelEnabled = aEnabled; }

protected:

    virtual void timerEvent (QTimerEvent *e);

    virtual void reject() { if (mCalcelEnabled) QProgressDialog::reject(); };

    virtual void closeEvent (QCloseEvent *e)
    {
        if (mCalcelEnabled)
            QProgressDialog::closeEvent (e);
        else
            e->ignore();
    }

private:

    CProgress &mProgress;
    bool mCalcelEnabled;
    const ulong mOpCount;
    ulong mCurOp;
    int mLoopLevel;
    bool mEnded;

    static const char *sOpDescTpl;
};

//static
const char *VBoxProgressDialog::sOpDescTpl = "%1... (%2/%3)";

int VBoxProgressDialog::run (int aRefreshInterval)
{
    if (mProgress.isOk())
    {
        /* start a refresh timer */
        startTimer (aRefreshInterval);
        mLoopLevel = qApp->eventLoop()->loopLevel();
        /* enter the modal loop */
        qApp->eventLoop()->enterLoop();
        killTimers();
        mLoopLevel = -1;
        mEnded = false;
        return result();
    }
    return Rejected;
}

//virtual
void VBoxProgressDialog::timerEvent (QTimerEvent *e)
{
    bool justEnded = false;

    if (!mEnded && (!mProgress.isOk() || mProgress.GetCompleted()))
    {
        /* dismiss the dialog -- the progress is no more valid */
        killTimer (e->timerId());
        if (mProgress.isOk())
        {
            setProgress (100);
            setResult (Accepted);
        }
        else
            setResult (Rejected);
        mEnded = justEnded = true;
    }

    if (mEnded)
    {
        if (mLoopLevel != -1)
        {
            /* we've entered the loop in run() */
            if (mLoopLevel + 1 == qApp->eventLoop()->loopLevel())
            {
                /* it's our loop, exit it */
                qApp->eventLoop()->exitLoop();
            }
            else
            {
                Assert (mLoopLevel + 1 < qApp->eventLoop()->loopLevel());
                /* restart the timer to watch for the loop level to drop */
                if (justEnded)
                    startTimer (50);
            }
        }
        else
            Assert (justEnded);
        return;
    }

    /* update the progress dialog */
    ulong newOp = mProgress.GetOperation() + 1;
    if (newOp != mCurOp)
    {
        mCurOp = newOp;
        setLabelText (QString (sOpDescTpl)
            .arg (mProgress.GetOperationDescription())
            .arg (mCurOp).arg (mOpCount));
    }
    setProgress (mProgress.GetPercent());
}


static const char *GUI_SuppressMessages = "GUI/SuppressMessages";

/** @class VBoxProblemReporter
 *
 *  The VBoxProblemReporter class is a central place to handle all
 *  problem/error situations that happen during application
 *  runtime and require the user's attention. Its role is to
 *  describe the problem and/or the cause of the error to the user and give
 *  him the opportunity to select an action (when appropriate).
 *
 *  Every problem sutiation has its own (correspondingly named) method in
 *  this class that takes a list of arguments necessary to describe the
 *  situation and to provide the appropriate actions. The method then
 *  returns the choice to the caller.
 */

/**
 *  Returns a reference to the global VirtualBox problem reporter instance.
 */
VBoxProblemReporter &VBoxProblemReporter::instance()
{
    static VBoxProblemReporter vboxProblem_instance;
    return vboxProblem_instance;
}

bool VBoxProblemReporter::isValid()
{
    return qApp != 0;
}

// Helpers
/////////////////////////////////////////////////////////////////////////////

/**
 *  Shows a message box of the given type with the given text and with buttons
 *  according to arguments b1, b2 and b3 (in the same way as QMessageBox does
 *  it), and returns the user's choice.
 *
 *  When all button arguments are zero, a single 'Ok' button is shown.
 *
 *  If autoConfirmId is not null, then the message box will contain a
 *  checkbox "Don't show this message again" below the message text and its
 *  state will be saved in the global config. When the checkbox is in the
 *  checked state, this method doesn't actually show the message box but
 *  returns immediately. The return value in this case is the same as if the
 *  user has pressed the Enter key or the default button, but with
 *  AutoConfirmed bit set (AutoConfirmed alone is returned when no default
 *  button is defined in button arguments).
 *
 *  @param  parent
 *      parent widget or 0 to use the desktop as the parent. Also,
 *      #mainWindowShown can be used to determine the currently shown VBox
 *      main window (Selector or Console)
 *  @param  type
 *      one of values of the Type enum, that defines the message box
 *      title and icon
 *  @param  msg
 *      message text to display (can contain sinmple Qt-html tags)
 *  @param  details
 *      detailed message description displayed under the main message text using
 *      QTextEdit (that provides rich text support and scrollbars when necessary).
 *      If equals to QString::null, no details text box is shown
 *  @param  autoConfirmId
 *      ID used to save the auto confirmation state across calls. If null,
 *      the auto confirmation feature is turned off (and no checkbox is shown)
 *  @param  b1
 *      first button code or 0, see QIMessageBox
 *  @param  b2
 *      second button code or 0, see QIMessageBox
 *  @param  b3
 *      third button code or 0, see QIMessageBox
 *  @param  name
 *      name of the underlying QIMessageBox object. If null, a value of
 *      autoConfirmId is used.
 *
 *  @return
 *      code of the button pressed by the user
 */
int VBoxProblemReporter::message (
    QWidget *parent, Type type, const QString &msg,
    const QString &details,
    const char *autoConfirmId,
    int b1, int b2, int b3,
    const char *name
) {
    if (b1 == 0 && b2 == 0 && b3 == 0)
        b1 = QIMessageBox::Ok | QIMessageBox::Default;

    CVirtualBox vbox;
    QStringList msgs;

    if (autoConfirmId)
    {
        vbox = vboxGlobal().virtualBox();
        msgs = QStringList::split (',', vbox.GetExtraData (GUI_SuppressMessages));
        if (msgs.findIndex (autoConfirmId) >= 0) {
            int rc = AutoConfirmed;
            if (b1 & QIMessageBox::Default) rc |= (b1 & QIMessageBox::ButtonMask);
            if (b2 & QIMessageBox::Default) rc |= (b2 & QIMessageBox::ButtonMask);
            if (b3 & QIMessageBox::Default) rc |= (b3 & QIMessageBox::ButtonMask);
            return rc;
        }
    }

    QString title;
    QIMessageBox::Icon icon;

    switch (type)
    {
        default:
        case Info:
            title = tr ("VirtualBox - Information", "msg box title");
            icon = QIMessageBox::Information;
            break;
        case Question:
            title = tr ("VirtualBox - Question", "msg box title");
            icon = QIMessageBox::Question;
            break;
        case Warning:
            title = tr ("VirtualBox - Warning", "msg box title");
            icon = QIMessageBox::Warning;
            break;
        case Error:
            title = tr ("VirtualBox - Error", "msg box title");
            icon = QIMessageBox::Critical;
            break;
        case Critical:
            title = tr ("VirtualBox - Critical Error", "msg box title");
            icon = QIMessageBox::Critical;
            break;
    }

    if (!name)
        name = autoConfirmId;
    QIMessageBox *box = new QIMessageBox (title, msg, icon, b1, b2, b3, parent, name);

    if (details)
    {
        box->setDetailsText (details);
        box->setDetailsShown (true);
    }

    if (autoConfirmId)
    {
        box->setFlagText (tr ("Do not show this message again", "msg box flag"));
        box->setFlagChecked (false);
    }

    int rc = box->exec();

    if (autoConfirmId)
    {
        if (box->isFlagChecked())
        {
            msgs << autoConfirmId;
            vbox.SetExtraData (GUI_SuppressMessages, msgs.join (","));
        }
    }

    delete box;

    return rc;
}

/** @fn message (QWidget *, Type, const QString &, const char *, int, int, int, const char *)
 *
 *  A shortcut to #message() that doesn't require to specify the details
 *  text (QString::null is assumed).
 */

/**
 *  A shortcut to #message() that shows 'Yes' and 'No' buttons ('Yes' is the
 *  default) and returns true when the user selects Yes.
 */
bool VBoxProblemReporter::messageYesNo (
    QWidget *parent, Type type, const QString &msg,
    const QString &details,
    const char *autoConfirmId,
    const char *name
)
{
    return (message (parent, type, msg, details, autoConfirmId,
                     QIMessageBox::Yes | QIMessageBox::Default,
                     QIMessageBox::No | QIMessageBox::Escape,
                     0, name) & QIMessageBox::ButtonMask) == QIMessageBox::Yes;
}

/** @fn messageYesNo (QWidget *, Type, const QString &, const char *, const char *)
 *
 *  A shortcut to #messageYesNo() that doesn't require to specify the details
 *  text (QString::null is assumed).
 */

/**
 *  Shows a modal progress dialog using a CProgress object passed as an
 *  argument to track the progress.
 *
 *  @param aProgress    progress object to track
 *  @param aTitle       title prefix
 *  @param aParent      parent widget
 *  @param aMinDuration time (ms) that must pass before the dialog appears
 */
bool VBoxProblemReporter::showModalProgressDialog (
    CProgress &aProgress, const QString &aTitle, QWidget *aParent,
    int aMinDuration)
{
    QApplication::setOverrideCursor (QCursor (WaitCursor));

    VBoxProgressDialog progressDlg (aProgress, aTitle, aMinDuration,
                                    aParent, "progressDlg");

    /* run the dialog with the 100 ms refresh interval */
    progressDlg.run (100);

    QApplication::restoreOverrideCursor();

    return true;
}

/**
 *  Returns what main window (selector or console) is now shown, or
 *  zero if none of them. The selector window takes precedence.
 */
QWidget *VBoxProblemReporter::mainWindowShown()
{
#if defined (VBOX_GUI_SEPARATE_VM_PROCESS)
    if (vboxGlobal().isVMConsoleProcess())
    {
        if (vboxGlobal().consoleWnd().isShown())
            return &vboxGlobal().consoleWnd();
    }
    else
    {
        if (vboxGlobal().selectorWnd().isShown())
            return &vboxGlobal().selectorWnd();
    }
#else
    if (vboxGlobal().consoleWnd().isShown())
        return &vboxGlobal().consoleWnd();
    if (vboxGlobal().selectorWnd().isShown())
        return &vboxGlobal().selectorWnd();
#endif
    return 0;
}

// Problem handlers
/////////////////////////////////////////////////////////////////////////////

void VBoxProblemReporter::cannotInitCOM (HRESULT rc)
{
    message (
        0,
        Critical,
        tr ("<p>Failed to initialize COM or to find the VirtualBox COM server. "
            "Most likely, the VirtualBox server is not running "
            "or failed to start.</p>"
            "<p>The application will now terminate.</p>"),
        formatErrorInfo (COMErrorInfo(), rc)
    );
}

void VBoxProblemReporter::cannotCreateVirtualBox (const CVirtualBox &vbox)
{
    message (
        0,
        Critical,
        tr ("<p>Failed to create the VirtualBox COM object.</p>"
            "<p>The application will now terminate.</p>"),
        formatErrorInfo (vbox)
    );
}

void VBoxProblemReporter::cannotLoadGlobalConfig (const CVirtualBox &vbox,
                                                  const QString &error)
{
    message (
        mainWindowShown(),
        Critical,
        tr ("<p>Failed to load the global GUI configuration.</p>"
            "<p>The application will now terminate.</p>"),
        !vbox.isOk() ? formatErrorInfo (vbox) : highlight (error)
    );
}

void VBoxProblemReporter::cannotSaveGlobalConfig (const CVirtualBox &vbox)
{
    message (
        mainWindowShown(),
        Critical,
        tr ("<p>Failed to save the global GUI configuration.<p>"),
        formatErrorInfo (vbox)
    );
}

void VBoxProblemReporter::cannotSetSystemProperties (const CSystemProperties &props)
{
    message (
        mainWindowShown(),
        Critical,
        tr ("Failed to set global VirtualBox properties."),
        formatErrorInfo (props)
    );
}

void VBoxProblemReporter::cannotCreateMachine (const CVirtualBox &vbox,
                                               QWidget *parent /* = 0 */)
{
    message (
        parent ? parent : mainWindowShown(),
        Error,
        tr ("Failed to create a new virtual machine."),
        formatErrorInfo (vbox)
    );
}

void VBoxProblemReporter::cannotCreateMachine (const CVirtualBox &vbox,
                                               const CMachine &machine,
                                               QWidget *parent /* = 0 */)
{
    message (
        parent ? parent : mainWindowShown(),
        Error,
        tr ("Failed to create a new virtual machine <b>%1</b>.")
            .arg (machine.GetName()),
        formatErrorInfo (vbox)
    );
}

void VBoxProblemReporter::
cannotApplyMachineSettings (const CMachine &machine, const COMResult &res)
{
    message (
        mainWindowShown(),
        Error,
        tr ("Failed to apply the settings to the virtual machine <b>%1</b>.")
            .arg (machine.GetName()),
        formatErrorInfo (res)
    );
}

void VBoxProblemReporter::cannotSaveMachineSettings (const CMachine &machine,
                                                     QWidget *parent /* = 0 */)
{
    /* preserve the current error info before calling the object again */
    COMErrorInfo errInfo = machine.errorInfo();
     
    message (parent ? parent : mainWindowShown(), Error,
             tr ("Failed to save the settings of the virtual machine <b>%1</b>.")
                 .arg (machine.GetName()),
             formatErrorInfo (errInfo));
}

/**
 *  @param  strict  if |true| then show the message even if there is no basic
 *                  error info available
 */
void VBoxProblemReporter::cannotLoadMachineSettings (const CMachine &machine,
                                                     bool strict /* = true */,
                                                     QWidget *parent /* = 0 */)
{
    COMErrorInfo errInfo = machine.errorInfo();
    if (!strict && !errInfo.isBasicAvailable())
        return;

    message (parent ? parent : mainWindowShown(), Error,
             tr ("Failed to load the settings of the virtual machine <b>%1</b>.")
                 .arg (machine.GetName()),
             formatErrorInfo (errInfo));
}

void VBoxProblemReporter::cannotStartMachine (const CConsole &console)
{
    // below, we use CConsole (console) to preserve current error info
    // for formatErrorInfo()

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to start the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotStartMachine (const CProgress &progress)
{
    AssertWrapperOk (progress);
    CConsole console = CProgress (progress).GetInitiator();
    AssertWrapperOk (console);

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to start the virtual machine <b>%1</b>.")
            .arg (console.GetMachine().GetName()),
        formatErrorInfo (progress.GetErrorInfo())
    );
}

void VBoxProblemReporter::cannotPauseMachine (const CConsole &console)
{
    /* below, we use CConsole (console) to preserve current error info
     * for formatErrorInfo() */

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to pause the execution of the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotResumeMachine (const CConsole &console)
{
    /* below, we use CConsole (console) to preserve current error info
     * for formatErrorInfo() */

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to resume the execution of the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotSaveMachineState (const CConsole &console)
{
    // below, we use CConsole (console) to preserve current error info
    // for formatErrorInfo()

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to save the state of the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotSaveMachineState (const CProgress &progress)
{
    AssertWrapperOk (progress);
    CConsole console = CProgress (progress).GetInitiator();
    AssertWrapperOk (console);

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to save the state of the virtual machine <b>%1</b>.")
            .arg (console.GetMachine().GetName()),
        formatErrorInfo (progress.GetErrorInfo())
    );
}

void VBoxProblemReporter::cannotTakeSnapshot (const CConsole &console)
{
    // below, we use CConsole (console) to preserve current error info
    // for formatErrorInfo()

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to create a snapshot of the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotTakeSnapshot (const CProgress &progress)
{
    AssertWrapperOk (progress);
    CConsole console = CProgress (progress).GetInitiator();
    AssertWrapperOk (console);

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to create a snapshot of the virtual machine <b>%1</b>.")
            .arg (console.GetMachine().GetName()),
        formatErrorInfo (progress.GetErrorInfo())
    );
}

void VBoxProblemReporter::cannotStopMachine (const CConsole &console)
{
    // below, we use CConsole (console) to preserve current error info
    // for formatErrorInfo()

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to stop the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotDeleteMachine (const CVirtualBox &vbox,
                                               const CMachine &machine)
{
    // below, we use CMachine (machine) to preserve current error info
    // for formatErrorInfo()

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to remove the virtual machine <b>%1</b>.")
            .arg (CMachine (machine).GetName()),
        !vbox.isOk() ? formatErrorInfo (vbox) : formatErrorInfo (machine)
    );
}

void VBoxProblemReporter::cannotDiscardSavedState (const CConsole &console)
{
    // below, we use CConsole (console) to preserve current error info
    // for formatErrorInfo()

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the saved state of the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console)
    );
}

void VBoxProblemReporter::cannotDiscardSnapshot (const CConsole &console,
                                                 const CSnapshot &snapshot)
{
    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the snapshot <b>%1</b> of the virtual "
            "machine <b>%1</b>.")
            .arg (snapshot.GetName())
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console));
}

void VBoxProblemReporter::cannotDiscardSnapshot (const CProgress &progress,
                                                 const CSnapshot &snapshot)
{
    CConsole console = CProgress (progress).GetInitiator();

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the snapshot <b>%1</b> of the virtual "
            "machine <b>%1</b>.")
            .arg (snapshot.GetName())
            .arg (console.GetMachine().GetName()),
        formatErrorInfo (progress.GetErrorInfo()));
}

void VBoxProblemReporter::cannotDiscardCurrentState (const CConsole &console)
{
    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the current state of the virtual "
            "machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console));
}

void VBoxProblemReporter::cannotDiscardCurrentState (const CProgress &progress)
{
    CConsole console = CProgress (progress).GetInitiator();

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the current state of the virtual "
            "machine <b>%1</b>.")
            .arg (console.GetMachine().GetName()),
        formatErrorInfo (progress.GetErrorInfo()));
}

void VBoxProblemReporter::cannotDiscardCurrentSnapshotAndState (const CConsole &console)
{
    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the current snapshot and the current state "
            "of the virtual machine <b>%1</b>.")
            .arg (CConsole (console).GetMachine().GetName()),
        formatErrorInfo (console));
}

void VBoxProblemReporter::cannotDiscardCurrentSnapshotAndState (const CProgress &progress)
{
    CConsole console = CProgress (progress).GetInitiator();

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to discard the current snapshot and the current state "
            "of the virtual machine <b>%1</b>.")
            .arg (console.GetMachine().GetName()),
        formatErrorInfo (progress.GetErrorInfo()));
}

void VBoxProblemReporter::cannotFindMachineByName (const CVirtualBox &vbox,
                                                   const QString &name)
{
    message (
        mainWindowShown(),
        Error,
        tr ("There is no virtual machine named <b>%1</b>.")
            .arg (name),
        formatErrorInfo (vbox)
    );
}

bool VBoxProblemReporter::confirmMachineDeletion (const CMachine &machine)
{
    QString msg;
    QString name;
    
    if (machine.GetAccessible())
    {
        name = machine.GetName();
        msg = tr ("<p>Are you sure you want to permanently delete "
                  "the virtual machine <b>%1</b>?</p>"
                  "<p>This operation cannot be undone.</p>")
                  .arg (name);
    }
    else
    {
        /* this should be in sync with VBoxVMListBoxItem::recache() */
        QFileInfo fi (machine.GetSettingsFilePath());
        name = fi.extension().lower() == "xml" ?
               fi.baseName (true) : fi.fileName();
        msg = tr ("<p>Are you sure you want to unregister the inaccessible "
                  "virtual machine <b>%1</b>?</p>"
                  "<p>You will no longer be able to register it back from "
                  "GUI.</p>")
                  .arg (name);
    }
    
    return messageYesNo (&vboxGlobal().selectorWnd(), Question, msg);
}

bool VBoxProblemReporter::confirmDiscardSavedState (const CMachine &machine)
{
    return messageYesNo (
        &vboxGlobal().selectorWnd(),
        Question,
        tr ("<p>Are you sure you want to discard the saved state of "
            "the virtual machine <b>%1</b>?</p>"
            "<p>This operation is equivalent to resetting or powering off "
            "the machine without doing a proper shutdown by means of the "
            "guest OS.</p>")
            .arg (machine.GetName())
    );
}

bool VBoxProblemReporter::confirmReleaseImage (QWidget* parent, QString usage)
{
    return messageYesNo (
        parent,
        Question,
        tr ("<p>Releasing this media image will detach it from the "
            "following virtual machine(s): <b>%1</b>.</p>"
            "<p>Continue?</p>")
            .arg (usage),
        "confirmReleaseImage"
    );
}

void VBoxProblemReporter::sayCannotOverwriteHardDiskImage (QWidget *parent,
                                                           const QString &src)
{
    message (
        parent,
        Info,
        tr (
            "<p>The image file <b>%1</b> already exists. "
            "You cannot create a new virtual hard disk that uses this file, "
            "because it can be already used by another virtual hard disk.</p>"
            "<p>Please specify a different image file name.</p>"
        )
            .arg (src)
    );
}

int VBoxProblemReporter::confirmHardDiskImageDeletion (QWidget *parent,
                                                        const QString &src)
{
    return message (parent, Question,
        tr ("<p>Do you want to delete this hard disk's image file "
            "<nobr><b>%1</b>?</nobr></p>"
            "<p>If you select <b>No</b> then the virtual hard disk will be "
            "unregistered and removed from the collection, but the image file " 
            "will be left on your physical disk.</p>"
            "<p>If you select <b>Yes</b> then the image file will be permanently "
            "deleted after unregistering the hard disk. This operation "
            "cannot be undone.</p>")
            .arg (src),
        0, /* autoConfirmId */
        QIMessageBox::Yes,
        QIMessageBox::No | QIMessageBox::Default,
        QIMessageBox::Cancel | QIMessageBox::Escape);
}

void VBoxProblemReporter::cannotDeleteHardDiskImage (QWidget *parent,
                                                     const CVirtualDiskImage &vdi)
{
    /* below, we use CHardDisk (hd) to preserve current error info
     * for formatErrorInfo() */

    message (parent, Error,
        tr ("Failed to delete the virtual hard disk image <b>%1</b>.")
            .arg (CVirtualDiskImage (vdi).GetFilePath()),
        formatErrorInfo (vdi));
}

int VBoxProblemReporter::confirmHardDiskUnregister (QWidget *parent,
                                                    const QString &src)
{
    return message (parent, Question,
        tr ("<p>Do you want to remove (unregister) the virtual hard disk "
            "<nobr><b>%1</b>?</nobr></p>")
            .arg (src),
        0, /* autoConfirmId */
        QIMessageBox::Ok | QIMessageBox::Default,
        QIMessageBox::Cancel | QIMessageBox::Escape);
}


void VBoxProblemReporter::cannotCreateHardDiskImage (
    QWidget *parent, const CVirtualBox &vbox, const QString &src,
    const CVirtualDiskImage &vdi, const CProgress &progress
) {
    message (parent,Error,
        tr ("Failed to create the virtual hard disk image <nobr><b>%1</b>.</nobr>")
            .arg (src),
        !vbox.isOk() ? formatErrorInfo (vbox) :
        !vdi.isOk() ? formatErrorInfo (vdi) :
        formatErrorInfo (progress.GetErrorInfo()));
}

void VBoxProblemReporter::cannotAttachHardDisk (
    QWidget *parent, const CMachine &m, const QUuid &id,
    CEnums::DiskControllerType ctl, LONG dev)
{
    message (parent, Error,
        tr ("Failed to attach a hard disk image with UUID %1 "
            "to the device slot %2 of the controller %3 of the machine <b>%4</b>.")
            .arg (id).arg (vboxGlobal().toString (ctl, dev))
            .arg (vboxGlobal().toString (ctl))
            .arg (CMachine (m).GetName()),
        formatErrorInfo (m));
}

void VBoxProblemReporter::cannotDetachHardDisk (
    QWidget *parent, const CMachine &m,
    CEnums::DiskControllerType ctl, LONG dev)
{
    message (parent, Error,
        tr ("Failed to detach a hard disk image "
            "from the device slot %1 of the controller %2 of the machine <b>%3</b>.")
            .arg (vboxGlobal().toString (ctl, dev))
            .arg (vboxGlobal().toString (ctl))
            .arg (CMachine (m).GetName()),
        formatErrorInfo (m));
}

void VBoxProblemReporter::cannotRegisterMedia (
    QWidget *parent, const CVirtualBox &vbox,
    VBoxDefs::DiskType type, const QString &src)
{
    QString media = type == VBoxDefs::HD ? tr ("hard disk") :
                    type == VBoxDefs::CD ? tr ("CD/DVD image") :
                    type == VBoxDefs::FD ? tr ("floppy image") :
                    QString::null;

    Assert (!media.isNull());
    
    message (parent, Error,
        tr ("Failed to register the %1 <nobr><b>%2</b></nobr>.")
            .arg (media)
            .arg (src),
        formatErrorInfo (vbox));
}

void VBoxProblemReporter::cannotUnregisterMedia (
    QWidget *parent, const CVirtualBox &vbox,
    VBoxDefs::DiskType type, const QString &src)
{
    QString media = type == VBoxDefs::HD ? tr ("hard disk") :
                    type == VBoxDefs::CD ? tr ("CD/DVD image") :
                    type == VBoxDefs::FD ? tr ("floppy image") :
                    QString::null;

    Assert (!media.isNull());

    message (parent, Error,
        tr ("Failed to unregister the %1 <nobr><b>%2</b></nobr>.")
            .arg (media)
            .arg (src),
        formatErrorInfo (vbox));
}

void VBoxProblemReporter::cannotOpenSession (const CSession &session)
{
    Assert (session.isNull());

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to create a new session."),
        formatErrorInfo (session)
    );
}

void VBoxProblemReporter::cannotOpenSession (const CVirtualBox &vbox,
                                             const QUuid &id
) {
    message (
        mainWindowShown(),
        Error,
        tr ("Failed to open a session for a virtual machine with UUID <b>%1</b>.")
            .arg (id),
        formatErrorInfo (vbox)
    );
}

void VBoxProblemReporter::cannotOpenSession (
    const CVirtualBox &vbox, const CMachine &machine,
    const CProgress &progress
) {
    Assert (!vbox.isOk() || progress.isOk());

    message (
        mainWindowShown(),
        Error,
        tr ("Failed to open a session for the virtual machine <b>%1</b>.")
            .arg (machine.GetName()),
        !vbox.isOk() ? formatErrorInfo (vbox) :
                       formatErrorInfo (progress.GetErrorInfo())
    );
}

void VBoxProblemReporter::cannotGetMediaAccessibility (const CUnknown &unk)
{
    QString src;
    CHardDisk hd;
    CDVDImage dvd;
    CFloppyImage floppy;

    if (!(hd = unk).isNull())
        src = hd.GetLocation();
    else
    if (!(dvd = unk).isNull())
        src = dvd.GetFilePath();
    else
    if (!(floppy = unk).isNull())
        src = floppy.GetFilePath();
    else
        AssertMsgFailed (("Not a valid CUnknown\n"));

    message (qApp->activeWindow(), Error,
        tr ("Failed to get the accessibility state of the media "
            "<nobr><b>%1</b></nobr>. Some of the registered media may "
            "become inaccessible.")
            .arg (src),
        formatErrorInfo (unk));
}

#if defined Q_WS_WIN

void VBoxProblemReporter::cannotCreateHostInterface (
    const CHost &host, const QString &name, QWidget *parent)
{
    message (parent ? parent : mainWindowShown(), Error,
        tr ("Failed to create the host network interface <b>%1</b>'.")
            .arg (name),
        formatErrorInfo (host));
}

void VBoxProblemReporter::cannotCreateHostInterface (
    const CProgress &progress, const QString &name, QWidget *parent)
{
    message (parent ? parent : mainWindowShown(), Error,
        tr ("Failed to create the host network interface <b>%1</b>'.")
            .arg (name),
        formatErrorInfo (progress.GetErrorInfo()));
}

void VBoxProblemReporter::cannotRemoveHostInterface (
    const CHost &host, const CHostNetworkInterface &iface, QWidget *parent)
{
    message (parent ? parent : mainWindowShown(), Error,
        tr ("Failed to remove the host network interface <b>%1</b>'.")
            .arg (iface.GetName()),
        formatErrorInfo (host));
}

void VBoxProblemReporter::cannotRemoveHostInterface (
    const CProgress &progress, const CHostNetworkInterface &iface, QWidget *parent)
{
    message (parent ? parent : mainWindowShown(), Error,
        tr ("Failed to remove the host network interface <b>%1</b>'.")
            .arg (iface.GetName()),
        formatErrorInfo (progress.GetErrorInfo()));
}

#endif

/** @return false if the dialog wasn't actually shown (i.e. it was autoconfirmed) */
bool VBoxProblemReporter::remindAboutInputCapture()
{
    int rc = message (
        &vboxGlobal().consoleWnd(),
        Info,
        tr (
            "<p>You have <b>clicked the mouse</b> inside the Virtual Machine display "
            "or pressed the <b>host key</b>. This will cause the Virtual Machine to "
            "<b>capture</b> the host mouse pointer (only if the mouse pointer "
            "integration is not currently supported by the guest OS) and the "
            "keyboard, which will make them unavailable to other applications "
            "running on your host machine.</p>"
            "<p>You can press the host key at any time to <b>uncapture</b> the "
            "keyboard and mouse (if it is captured) and return them to normal "
            "operation. The currently assigned host key is shown on the status bar "
            "at the bottom of the Virtual Machine window. "
            "There are also a keyboard icon and a mouse icon indicating the "
            "current keyboard and mouse capture state.</p>"
        ),
        "remindAboutInputCapture"
    );
    return !(rc & AutoConfirmed);
}

/** @return false if the dialog wasn't actually shown (i.e. it was autoconfirmed) */
bool VBoxProblemReporter::remindAboutAutoCapture()
{
    int rc = message (
        &vboxGlobal().consoleWnd(),
        Info,
        tr (
            "<p>You have the <b>Auto capture keyboard</b> option turned on. "
            "This will cause the Virtual Machine to automatically <b>capture</b> "
            "the keyboard every time the VM window is activated and make it "
            "unavailable to other applications running on your host machine.</p>"
            "<p>You can press the <b>host key</b> at any time to <b>uncapture</b> the "
            "keyboard and mouse (if it is captured) and return them to normal "
            "operation. The currently assigned host key is shown on the status bar "
            "at the bottom of the Virtual Machine window. "
            "There are also a keyboard icon and a mouse icon indicating the "
            "current keyboard and mouse capture state.</p>"
        ),
        "remindAboutAutoCapture"
    );
    return !(rc & AutoConfirmed);
}

/** @return false if the dialog wasn't actually shown (i.e. it was autoconfirmed) */
bool VBoxProblemReporter::remindAboutMouseIntegration (bool supportsAbsolute)
{
    static const char *names [2] =
    {
        "remindAboutMouseIntegrationOff",
        "remindAboutMouseIntegrationOn"
    };

    // close the previous reminder if it is still active -- already outdated
    // (the name of the modal window will correspond to autoConfirmId if
    // it is our reminder)
    QWidget *modal = QApplication::activeModalWidget();
    if (modal && !strcmp (modal->name(), names [int (!supportsAbsolute)]))
        modal->close();

    if (supportsAbsolute)
    {
        int rc = message (
            &vboxGlobal().consoleWnd(),
            Info,
            tr (
                "<p>The Virtual Machine reports that the guest OS supports the "
                "<b>mouse pointer integration</b>. This means that you do not need "
                "to capture the host mouse pointer to be able to use it in "
                "your guest OS -- all "
                "mouse actions you perform when the mouse pointer is over the "
                "Virtual Machine's display are directly sent to the guest OS."
                "If the mouse is currently captured, it will be automatically "
                "uncaptured.</p>"
                "<p>The mouse icon on the status bar displays arrows around "
                "the mouse when the guest OS supports the pointer integration.</p>"
                "<p><b>Note</b>: some applications may behave incorrectly in "
                "mouse pointer integration mode. You can always disable it for "
                "the current session (and enable again) by selecting the "
                "corresponding action from the menu bar.</p>"
            ),
            names [1] // autoConfirmId
        );
        return !(rc & AutoConfirmed);
    }
    else
    {
        int rc = message (
            &vboxGlobal().consoleWnd(),
            Info,
            tr (
                "<p>The Virtual Machine reports that the guest OS does not "
                "support <b>mouse pointer integration</b> in the current video "
                "mode. You need to capture the mouse (by clicking over the VM "
                "display or pressing the host key) in order to use the "
                "mouse inside the guest OS.</p>"
            ),
            names [0] // autoConfirmId
        );
        return !(rc & AutoConfirmed);
    }
}

/** @return false if the dialog wasn't actually shown (i.e. it was autoconfirmed) */
bool VBoxProblemReporter::remindAboutPausedVMInput()
{
    int rc = message (
        &vboxGlobal().consoleWnd(),
        Info,
        tr (
            "<p>The Virtual Machine is currently in the <b>Paused</b> state and "
            "therefore does not accept any keyboard or mouse input. If you want to "
            "continue to work inside the VM, you need to resume it by selecting the "
            "corresponding action from the menu bar.</p>"
        ),
        "remindAboutPausedVMInput"
    );
    return !(rc & AutoConfirmed);
}

/** @return true if the user has chosen to show the Disk Manager Window */
bool VBoxProblemReporter::remindAboutInaccessibleMedia()
{
    int rc = message (
        &vboxGlobal().selectorWnd(),
        Warning,
        tr ("<p>One or more of the registered virtual hard disks, CD/DVD or "
            "floppy media are not currently accessible. As a result, you will "
            "not be able to operate virtual machines that use these media until "
            "they become accessible later.</p>"
            "<p>Press <b>OK</b> to open the Virtual Disk Manager window and "
            "see what media are inaccessible, or press <b>Ignore</b> to "
            "ignore this message.</p>"),
        "remindAboutInaccessibleMedia",
        QIMessageBox::Ok | QIMessageBox::Default,
        QIMessageBox::Ignore | QIMessageBox::Escape
    );

    return rc == QIMessageBox::Ok && !(rc & AutoConfirmed);
}

/**
 *  @param  fullscreen hot key as defined in the menu
 *  @param  current host key as in the global settings
 *  @return true if the user has chosen to go fullscreen.
 */
void VBoxProblemReporter::remindAboutGoingFullscreen (const QString &hotKey,
                                                      const QString &hostKey)
{
    int rc = message (&vboxGlobal().consoleWnd(), Info,
        tr ("<p>The virtual machine window will be now switched to "
            "<b>fullscreen</b> mode. "
            "You can go back to windowed mode at any time by pressing "
            "Host-<b>%1</b>. Note that the current "
            "Host key is defined as <b>%1</b>.</p>")
            .arg (hotKey).arg (hostKey),
        "remindAboutGoingFullscreen");
}

// static
QString VBoxProblemReporter::highlight (const QString &str)
{
    QString text = str;
    // mark strings in single quotes with color
    QRegExp rx = QRegExp ("((?:^|\\s)[(]?)'([^']*)'(?=[:.-!);]?(?:\\s|$))");
    rx.setMinimal (true);
    text.replace (rx, "\\1'<font color=#0000CC>\\2</font>'");
    // mark UUIDs with color
    text.replace (QRegExp (
        "((?:^|\\s)[(]?)"
        "(\\{[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}\\})"
        "(?=[:.-!);]?(?:\\s|$))"),
        "\\1<font color=#008000>\\2</font>");
    // split to paragraphs at \n chars
    text.replace ('\n', "</p><p>");
    return text;
}

// static
QString VBoxProblemReporter::formatErrorInfo (const COMErrorInfo &info,
                                              HRESULT wrapperRC)
{
    QString formatted = "<qt>";

    if (info.text())
        formatted += QString ("<table bgcolor=#FFFFFF border=0 cellspacing=0 "
                              "cellpadding=0 width=100%>"
                              "<tr><td><p>%1.</p></td></tr>"
                              "</table><p></p>")
                              .arg (highlight (info.text()));

    formatted += "<table bgcolor=#EEEEEE border=0 cellspacing=0 "
                 "cellpadding=0 width=100%>";

    bool haveResultCode = false;

    if (info.isBasicAvailable())
    {
#if defined (Q_WS_WIN)
        haveResultCode = info.isFullAvailable();
        bool haveComponent = true;
        bool haveInterfaceID = true;
#else // !Q_WS_WIN
        haveResultCode = true;
        bool haveComponent = info.isFullAvailable();
        bool haveInterfaceID = info.isFullAvailable();
#endif

        if (haveResultCode)
        {
#if defined (Q_WS_WIN)
            /* format the error code, masking off the top 16 bits */
            PCRTWINERRMSG msg;
            msg = RTErrWinGet(info.resultCode());
            /* try again with masked off top 16bit if not found */
            if (!msg->iCode)
                msg = RTErrWinGet(info.resultCode() & 0xFFFF);
            formatted += QString ("<tr><td>%1</td><td><tt>%2 (0x%3)</tt></td></tr>")
                .arg (tr ("Result&nbsp;Code: ", "error info"))
                .arg (msg->pszDefine)
                .arg (uint (info.resultCode()), 8, 16);
#else
            formatted += QString ("<tr><td>%1</td><td><tt>0x%2</tt></td></tr>")
                .arg (tr ("Result&nbsp;Code: ", "error info"))
                .arg (uint (info.resultCode()), 8, 16);
#endif
        }

        if (haveComponent)
            formatted += QString ("<tr><td>%1</td><td>%2</td></tr>")
                .arg (tr ("Component: ", "error info"), info.component());

        if (haveInterfaceID)
        {
            QString s = info.interfaceID();
            if (info.interfaceName())
                s = info.interfaceName() + ' ' + s;
            formatted += QString ("<tr><td>%1</td><td>%2</td></tr>")
                .arg (tr ("Interface: ", "error info"), s);
        }

        if (!info.calleeIID().isNull() && info.calleeIID() != info.interfaceID())
        {
            QString s = info.calleeIID();
            if (info.calleeName())
                s = info.calleeName() + ' ' + s;
            formatted += QString ("<tr><td>%1</td><td>%2</td></tr>")
                .arg (tr ("Callee: ", "error info"), s);
        }
    }

    if (FAILED (wrapperRC) &&
        (!haveResultCode || wrapperRC != info.resultCode()))
    {
#if defined (Q_WS_WIN)
        /* format the error code */
        PCRTWINERRMSG msg;
        msg = RTErrWinGet(wrapperRC);
        /* try again with masked off top 16bit if not found */
        if (!msg->iCode)
            msg = RTErrWinGet(wrapperRC & 0xFFFF);
        formatted += QString ("<tr><td>%1</td><td><tt>%2 (0x%3)</tt></td></tr>")
            .arg (tr ("Callee&nbsp;RC: ", "error info"))
            .arg (msg->pszDefine)
            .arg (uint (wrapperRC), 8, 16);
#else
        formatted += QString ("<tr><td>%1</td><td><tt>0x%2</tt></td></tr>")
            .arg (tr ("Callee&nbsp;RC: ", "error info"))
            .arg (uint (wrapperRC), 8, 16);
#endif
    }
    formatted += "</table></font></qt>";

    return formatted;
}

// Public slots
/////////////////////////////////////////////////////////////////////////////

void VBoxProblemReporter::showHelpWebDialog()
{
}

void VBoxProblemReporter::showHelpAboutDialog()
{
    CVirtualBox vbox = vboxGlobal().virtualBox();
    QString COMVersion = vbox.GetVersion();
    AssertWrapperOk (vbox);

    VBoxAboutDlg dlg (mainWindowShown(), "VBoxAboutDlg");
    QString versions = dlg.txVersions->text();
    dlg.txVersions->setText (versions
                             .arg (COMVersion));
    dlg.exec();
}

void VBoxProblemReporter::resetSuppressedMessages()
{
    CVirtualBox vbox = vboxGlobal().virtualBox();
    vbox.SetExtraData (GUI_SuppressMessages, QString::null);
}

/** @fn vboxProblem
 *
 *  Shortcut to the static VBoxProblemReporter::instance() method, for
 *  convenience.
 */

/* $Id: UISoftKeyboard.cpp 79172 2019-06-17 09:09:03Z vboxsync $ */
/** @file
 * VBox Qt GUI - UISoftKeyboard class implementation.
 */

/*
 * Copyright (C) 2016-2019 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/* Qt includes: */
#include <QApplication>
#include <QComboBox>
#include <QFile>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QPainter>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QStackedWidget>
#include <QToolButton>
#include <QXmlStreamReader>
#include <QVBoxLayout>


/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "QIFileDialog.h"
#include "UIExtraDataManager.h"
#include "UIIconPool.h"
#include "UISession.h"
#include "UISoftKeyboard.h"
#include "QIToolButton.h"
#include "VBoxGlobal.h"

/* COM includes: */
#include "CGuest.h"
#include "CEventSource.h"

/* Forward declarations: */
class UISoftKeyboardWidget;
class UISoftKeyboardLayout;
class UISoftKeyboardRow;

enum UIKeyState
{
    UIKeyState_NotPressed,
    UIKeyState_Pressed,
    UIKeyState_Locked,
    UIKeyState_Max
};

enum UIKeyType
{
    /** Can be in UIKeyState_NotPressed and UIKeyState_Pressed states. */
    UIKeyType_Ordinary,
    /** e.g. CapsLock. Can be only in UIKeyState_NotPressed, UIKeyState_Locked */
    UIKeyType_Toggleable,
    /** e.g. Shift Can be in all 3 states*/
    UIKeyType_Modifier,
    UIKeyType_Max
};

struct KeyCaptions
{
    QString m_strBase;
    QString m_strShift;
    QString m_strAltGr;
};


/*********************************************************************************************************************************
*   UISoftKeyboardPhysicalLayout definition.                                                                                     *
*********************************************************************************************************************************/

class UISoftKeyboardPhysicalLayout
{

public:

    void setName(const QString &strName)
    {
        m_strName = strName;
    }

    const QString &name() const
    {
        return m_strName;
    }

    QString  m_strFileName;
    QUuid    m_uId;

    QVector<UISoftKeyboardRow>  m_rows;

private:

    QString  m_strName;
};

/*********************************************************************************************************************************
*   UILayoutEditor definition.                                                                                  *
*********************************************************************************************************************************/

class UILayoutEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    void sigLayoutEdited();
    void sigGoBackButton();

public:

    UILayoutEditor(QWidget *pParent = 0);
    void setKey(UISoftKeyboardKey *pKey);

    void setLayoutToEdit(UISoftKeyboardLayout *pLayout);
    void setPhysicalLayoutList(const QVector<UISoftKeyboardPhysicalLayout> &physicalLayouts);

protected:

    virtual void retranslateUi() /* override */;

private slots:

    void sltKeyBaseCaptionChange();
    void sltKeyShiftCaptionChange();
    void sltKeyAltGrCaptionChange();
    void sltLayoutNameChange();
    void sltHandlePhysicalLayoutChanged();

private:

    void prepareObjects();
    void prepareConnections();
    QWidget *prepareKeyCaptionEditWidgets();

    QGridLayout *m_pEditorLayout;
    QToolButton *m_pGoBackButton;
    QGroupBox *m_pSelectedKeyGroupBox;
    QGroupBox *m_pCaptionEditGroupBox;

    QComboBox *m_pPhysicalLayoutCombo;
    QLabel *m_pPhysicalLayoutLabel;
    QLabel *m_pLayoutNameLabel;
    QLabel *m_pScanCodeLabel;
    QLabel *m_pPositionLabel;
    QLabel *m_pBaseCaptionLabel;
    QLabel *m_pShiftCaptionLabel;
    QLabel *m_pAltGrCaptionLabel;

    QLineEdit *m_pLayoutNameEdit;
    QLineEdit *m_pScanCodeEdit;
    QLineEdit *m_pPositionEdit;
    QLineEdit *m_pBaseCaptionEdit;
    QLineEdit *m_pShiftCaptionEdit;
    QLineEdit *m_pAltGrCaptionEdit;

    /** The key which is being currently edited. Might be Null. */
    UISoftKeyboardKey  *m_pKey;
    /** The layout which is being currently edited. */
    UISoftKeyboardLayout *m_pLayout;
    QVector<UISoftKeyboardPhysicalLayout> m_physicalLayouts;
};

/*********************************************************************************************************************************
*   UILayoutSelector definition.                                                                                  *
*********************************************************************************************************************************/

class UILayoutSelector : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

signals:

    void sigApplySelectedLayout(const QString &strSelectedLayoutName);
    void sigEditSelectedLayout();

public:

    UILayoutSelector(QWidget *pParent = 0);
    void setLayoutList(const QStringList &layoutNames);
    QString selectedLayoutName() const;
    void selectLayoutByName(const QString &strLayoutName);

protected:

    virtual void retranslateUi() /* override */;

private slots:


private:
    void prepareObjects();

    QListWidget *m_pLayoutListWidget;
    QToolButton *m_pApplyLayoutButton;
    QToolButton *m_pEditLayoutButton;
};

/*********************************************************************************************************************************
*   UISoftKeyboardRow definition.                                                                                  *
*********************************************************************************************************************************/

/** UISoftKeyboardRow represents a row in the physical keyboard. */
class UISoftKeyboardRow
{
public:

    UISoftKeyboardRow();

    void setDefaultWidth(int iWidth);
    int defaultWidth() const;

    void setDefaultHeight(int iHeight);
    int defaultHeight() const;

    QVector<UISoftKeyboardKey> &keys();
    const QVector<UISoftKeyboardKey> &keys() const;

    void setSpaceHeightAfter(int iSpace);
    int spaceHeightAfter() const;

private:

    /** Default width and height might be inherited from the layout and overwritten in row settings. */
    int m_iDefaultWidth;
    int m_iDefaultHeight;
    QVector<UISoftKeyboardKey> m_keys;
    int m_iSpaceHeightAfter;
};

/*********************************************************************************************************************************
*   UISoftKeyboardKey definition.                                                                                  *
*********************************************************************************************************************************/

/** UISoftKeyboardKey is a place holder for a keyboard key. Graphical key represantations are drawn according to this class. */
class UISoftKeyboardKey
{
public:

    UISoftKeyboardKey();

    const QRect keyGeometry() const;
    void setKeyGeometry(const QRect &rect);

    const QString baseCaption() const;
    void setBaseCaption(const QString &strBaseCaption);

    const QString shiftCaption() const;
    void setShiftCaption(const QString &strShiftCaption);

    const QString altGrCaption() const;
    void  setAltGrCaption(const QString &strAltGrCaption);

    const QString text() const;

    void setWidth(int iWidth);
    int width() const;

    void setHeight(int iHeight);
    int height() const;

    void setScanCode(LONG scanCode);
    LONG scanCode() const;

    void setScanCodePrefix(LONG scanCode);
    LONG scanCodePrefix() const;

    void setSpaceWidthAfter(int iSpace);
    int spaceWidthAfter() const;

    void setPosition(int iPosition);
    int position() const;

    void setType(UIKeyType enmType);
    UIKeyType type() const;

    void setCutout(int iCorner, int iWidth, int iHeight);

    void setParentWidget(UISoftKeyboardWidget* pParent);
    UIKeyState state() const;
    QVector<LONG> scanCodeWithPrefix() const;

    void release();
    void press();

    void setPolygon(const QPolygon &polygon);
    const QPolygon &polygon() const;

    QPolygon polygonInGlobal() const;

    int cutoutCorner() const;
    int cutoutWidth() const;
    int cutoutHeight() const;

private:

    void updateState(bool fPressed);
    void updateText();

    QRect      m_keyGeometry;

    QString    m_strBaseCaption;
    QString    m_strShiftCaption;
    QString    m_strAltGrCaption;
    /** m_strText is concatenation of base, shift, and altgr captions. */
    QString    m_strText;

    /** Stores the key polygon in local coordinates. */
    QPolygon   m_polygon;
    UIKeyType  m_enmType;
    UIKeyState m_enmState;
    /** Key width as it is read from the xml file. */
    int        m_iWidth;
    /** Key height as it is read from the xml file. */
    int        m_iHeight;
    int        m_iSpaceWidthAfter;
    LONG       m_scanCode;
    LONG       m_scanCodePrefix;
    int        m_iCutoutWidth;
    int        m_iCutoutHeight;
    /** -1 is for no cutout. 0 is the topleft, 2 is the top right and so on. */
    int        m_iCutoutCorner;
    /** Key's position in the layout. */
    int        m_iPosition;
    UISoftKeyboardWidget  *m_pParentWidget;
};


/*********************************************************************************************************************************
*   UISoftKeyboardLayout definition.                                                                                  *
*********************************************************************************************************************************/

class UISoftKeyboardLayout
{

public:

    UISoftKeyboardLayout()
        :m_pPhysicalLayout(0){}

    void setName(const QString &strName)
    {
        m_strName = strName;
    }

    const QString &name() const
    {
        return m_strName;
    }

    /** The physical layout used by this layout. */
    UISoftKeyboardPhysicalLayout *m_pPhysicalLayout;

    /** We cache the key caps here instead of reading the layout files each time layout changes.
      * Map key is the key position and the value is the captions of the key. */
    QMap<int, KeyCaptions> m_keyCapMap;

private:

    QString m_strName;
};

/*********************************************************************************************************************************
*   UISoftKeyboardWidget definition.                                                                                  *
*********************************************************************************************************************************/

/** The container widget for keyboard keys. It also handles all the keyboard related events. */
class UISoftKeyboardWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;
    enum Mode
    {
        Mode_LayoutEdit,
        Mode_Keyboard,
        Mode_Max
    };

signals:

    void sigPutKeyboardSequence(QVector<LONG> sequence);
    void sigLayoutChange(const QString &strLayoutName);
    void sigLayoutListChanged(QStringList layoutNames);

public:

    UISoftKeyboardWidget(QWidget *pParent = 0);

    virtual QSize minimumSizeHint() const /* override */;
    virtual QSize sizeHint() const  /* override */;
    void keyStateChange(UISoftKeyboardKey* pKey);
    void loadLayouts();
    void showContextMenu(const QPoint &globalPoint);
    void applyLayoutByName(const QString &strLayoutName);
    QStringList layoutNameList() const;
    const QVector<UISoftKeyboardPhysicalLayout> &physicalLayouts() const;

protected:

    virtual void paintEvent(QPaintEvent *pEvent) /* override */;
    virtual void mousePressEvent(QMouseEvent *pEvent) /* override */;
    virtual void mouseReleaseEvent(QMouseEvent *pEvent) /* override */;
    virtual void mouseMoveEvent(QMouseEvent *pEvent) /* override */;
    virtual void retranslateUi() /* override */;

private slots:

    void sltHandleContextMenuRequest(const QPoint &point);
    void sltHandleSaveLayout();
    void sltHandleLayoutEditModeToggle(bool fToggle);
    void sltHandleNewLayout();
    void sltPhysicalLayoutForLayoutChanged(int iIndex);

private:

    void               setNewMinimumSize(const QSize &size);
    void               setInitialSize(int iWidth, int iHeight);
    /** Searches for the key which contains the position of the @p pEvent and returns it if found. */
    UISoftKeyboardKey *keyUnderMouse(QMouseEvent *pEvent);
    UISoftKeyboardKey *keyUnderMouse(const QPoint &point);
    void               handleKeyPress(UISoftKeyboardKey *pKey);
    void               handleKeyRelease(UISoftKeyboardKey *pKey);
    bool               loadPhysicalLayout(const QString &strLayoutFileName);
    bool               loadKeyboardLayout(const QString &strLayoutName);
    void               reset();
    void               prepareObjects();
    /** Sets m_pKeyBeingEdited. */
    void               setKeyBeingEdited(UISoftKeyboardKey *pKey);
    void               setCurrentLayout(UISoftKeyboardLayout *pLayout);
    void               emitLayoutNames();
    UISoftKeyboardLayout *findLayoutByName(const QString &strName);

    UISoftKeyboardKey *m_pKeyUnderMouse;
    UISoftKeyboardKey *m_pKeyBeingEdited;

    UISoftKeyboardKey *m_pKeyPressed;
    QColor m_keyDefaultColor;
    QColor m_keyHoverColor;
    QColor m_textDefaultColor;
    QColor m_textPressedColor;
    QColor m_keyEditColor;
    QVector<UISoftKeyboardKey*> m_pressedModifiers;
    //QVector<UISoftKeyboardRow>  m_rows;
    QVector<UISoftKeyboardPhysicalLayout> m_physicalLayouts;
    QVector<UISoftKeyboardLayout>         m_layouts;
    UISoftKeyboardLayout *m_pCurrentKeyboardLayout;

    QSize m_minimumSize;
    float m_fScaleFactorX;
    float m_fScaleFactorY;
    int   m_iInitialHeight;
    int   m_iInitialWidth;
    int   m_iXSpacing;
    int   m_iYSpacing;
    int   m_iLeftMargin;
    int   m_iTopMargin;
    int   m_iRightMargin;
    int   m_iBottomMargin;

    QMenu   *m_pContextMenu;
    QAction *m_pShowPositionsAction;
    QAction *m_pLayoutEditModeToggleAction;
    Mode       m_enmMode;
};

/*********************************************************************************************************************************
*   UIPhysicalLayoutReader definition.                                                                                  *
*********************************************************************************************************************************/

class UIPhysicalLayoutReader
{

public:

    bool parseXMLFile(const QString &strFileName, UISoftKeyboardPhysicalLayout &physicalLayout);
    static QVector<QPoint> computeKeyVertices(const UISoftKeyboardKey &key);
private:

    void  parseKey(UISoftKeyboardRow &row);
    void  parseRow(int iDefaultWidth, int iDefaultHeight, QVector<UISoftKeyboardRow> &rows);
    /**  Parses the verticel space between the rows. */
    void  parseRowSpace(QVector<UISoftKeyboardRow> &rows);
    /** Parses the horizontal space between keys. */
    void  parseKeySpace(UISoftKeyboardRow &row);
    void  parseCutout(UISoftKeyboardKey &key);

    QXmlStreamReader m_xmlReader;
};

/*********************************************************************************************************************************
*   UIKeyboardLayoutReader definition.                                                                                  *
*********************************************************************************************************************************/

class UIKeyboardLayoutReader
{

public:

    bool  parseFile(const QString &strFileName);
    const QUuid &physicalLayoutUUID() const;
    const QString &name() const;
    const QMap<int, KeyCaptions> &keyCapMap() const;

private:

    void  parseKey();
    QXmlStreamReader m_xmlReader;
    /** Map key is the key position and the value is the captions of the key. */
    QMap<int, KeyCaptions> m_keyCapMap;
    QUuid m_physicalLayoutUid;
    QString m_strName;
};

/*********************************************************************************************************************************
*   UILayoutEditor implementation.                                                                                  *
*********************************************************************************************************************************/

UILayoutEditor::UILayoutEditor(QWidget *pParent /* = 0 */)
    :QIWithRetranslateUI<QWidget>(pParent)
    , m_pEditorLayout(0)
    , m_pGoBackButton(0)
    , m_pSelectedKeyGroupBox(0)
    , m_pCaptionEditGroupBox(0)
    , m_pPhysicalLayoutCombo(0)
    , m_pPhysicalLayoutLabel(0)
    , m_pLayoutNameLabel(0)
    , m_pScanCodeLabel(0)
    , m_pPositionLabel(0)
    , m_pBaseCaptionLabel(0)
    , m_pShiftCaptionLabel(0)
    , m_pAltGrCaptionLabel(0)
    , m_pLayoutNameEdit(0)
    , m_pScanCodeEdit(0)
    , m_pPositionEdit(0)
    , m_pBaseCaptionEdit(0)
    , m_pShiftCaptionEdit(0)
    , m_pAltGrCaptionEdit(0)
    , m_pKey(0)
{
    setAutoFillBackground(true);
    prepareObjects();
}

void UILayoutEditor::setKey(UISoftKeyboardKey *pKey)
{
    if (m_pKey == pKey)
        return;
    m_pKey = pKey;
    if (m_pSelectedKeyGroupBox)
        m_pSelectedKeyGroupBox->setEnabled(m_pKey);
    if (!m_pKey)
        return;
    if (m_pScanCodeEdit)
        m_pScanCodeEdit->setText(QString::number(m_pKey->scanCode(), 16));
    if (m_pPositionEdit)
        m_pPositionEdit->setText(QString::number(m_pKey->position()));
    if (m_pBaseCaptionEdit)
        m_pBaseCaptionEdit->setText(m_pKey->baseCaption());
    if (m_pShiftCaptionEdit)
        m_pShiftCaptionEdit->setText(m_pKey->shiftCaption());
    if (m_pAltGrCaptionEdit)
        m_pAltGrCaptionEdit->setText(m_pKey->altGrCaption());
}

void UILayoutEditor::setLayoutToEdit(UISoftKeyboardLayout *pLayout)
{
    if (m_pLayout != pLayout)
        return;
    if (m_pLayoutNameEdit)
        m_pLayoutNameEdit->setText(m_pLayout->name());
    update();
}

void UILayoutEditor::setPhysicalLayoutList(const QVector<UISoftKeyboardPhysicalLayout> &physicalLayouts)
{
    m_physicalLayouts = physicalLayouts;

    if (!m_pPhysicalLayoutCombo)
        return;
    m_pPhysicalLayoutCombo->clear();
    foreach (const UISoftKeyboardPhysicalLayout &physicalLayout, physicalLayouts)
        m_pPhysicalLayoutCombo->addItem(physicalLayout.name(), physicalLayout.m_uId);
}

void UILayoutEditor::retranslateUi()
{
    if (m_pGoBackButton)
        m_pGoBackButton->setToolTip(UISoftKeyboard::tr("Return Back to Layout List"));
    if (m_pPhysicalLayoutLabel)
        m_pPhysicalLayoutLabel->setText(UISoftKeyboard::tr("Physical Layout"));
    if (m_pLayoutNameLabel)
        m_pLayoutNameLabel->setText(UISoftKeyboard::tr("Layout Name"));
    if (m_pScanCodeLabel)
        m_pScanCodeLabel->setText(UISoftKeyboard::tr("Scan Code"));
    if (m_pPositionLabel)
        m_pPositionLabel->setText(UISoftKeyboard::tr("Position"));
    if (m_pBaseCaptionLabel)
        m_pBaseCaptionLabel->setText(UISoftKeyboard::tr("Base"));
    if (m_pShiftCaptionLabel)
        m_pShiftCaptionLabel->setText(UISoftKeyboard::tr("Shift"));
    if (m_pAltGrCaptionLabel)
        m_pAltGrCaptionLabel->setText(UISoftKeyboard::tr("AltGr"));
    if (m_pCaptionEditGroupBox)
        m_pCaptionEditGroupBox->setTitle(UISoftKeyboard::tr("Captions"));
    if (m_pSelectedKeyGroupBox)
        m_pSelectedKeyGroupBox->setTitle(UISoftKeyboard::tr("Selected Key"));
}

void UILayoutEditor::sltKeyBaseCaptionChange()
{
    if (!m_pKey || !m_pBaseCaptionEdit)
        return;
    m_pKey->setBaseCaption(m_pBaseCaptionEdit->text());
    emit sigLayoutEdited();
}

void UILayoutEditor::sltKeyShiftCaptionChange()
{
    if (!m_pKey || !m_pShiftCaptionEdit)
        return;
    m_pKey->setShiftCaption(m_pShiftCaptionEdit->text());
    emit sigLayoutEdited();
}

void UILayoutEditor::sltKeyAltGrCaptionChange()
{
    if (!m_pKey || !m_pAltGrCaptionEdit)
        return;
    m_pKey->setAltGrCaption(m_pAltGrCaptionEdit->text());
    emit sigLayoutEdited();
}

void UILayoutEditor::sltLayoutNameChange()
{
    if (!m_pLayoutNameEdit)
        return;
    emit sigLayoutEdited();
}

void UILayoutEditor::sltHandlePhysicalLayoutChanged()
{
    if (!m_pPhysicalLayoutCombo)
        return;


}

void UILayoutEditor::prepareObjects()
{
    m_pEditorLayout = new QGridLayout;
    if (!m_pEditorLayout)
        return;
    setLayout(m_pEditorLayout);

    m_pGoBackButton = new QToolButton;
    m_pGoBackButton->setIcon(UIIconPool::defaultIcon(UIIconPool::UIDefaultIconType_ArrowBack));
    m_pEditorLayout->addWidget(m_pGoBackButton, 0, 0, 1, 1);
    connect(m_pGoBackButton, &QToolButton::pressed, this, &UILayoutEditor::sigGoBackButton);

    m_pLayoutNameLabel = new QLabel;
    m_pLayoutNameEdit = new QLineEdit;
    m_pLayoutNameLabel->setBuddy(m_pLayoutNameEdit);
    m_pEditorLayout->addWidget(m_pLayoutNameLabel, 1, 0, 1, 1);
    m_pEditorLayout->addWidget(m_pLayoutNameEdit, 1, 1, 1, 1);
    connect(m_pLayoutNameEdit, &QLineEdit::editingFinished, this, &UILayoutEditor::sltLayoutNameChange);


    m_pPhysicalLayoutLabel = new QLabel;
    m_pPhysicalLayoutCombo = new QComboBox;
    m_pPhysicalLayoutLabel->setBuddy(m_pPhysicalLayoutCombo);
    m_pEditorLayout->addWidget(m_pPhysicalLayoutLabel, 2, 0, 1, 1);
    m_pEditorLayout->addWidget(m_pPhysicalLayoutCombo, 2, 1, 1, 1);
    connect(m_pPhysicalLayoutCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &UILayoutEditor::sltHandlePhysicalLayoutChanged);

    m_pSelectedKeyGroupBox = new QGroupBox;
    m_pSelectedKeyGroupBox->setEnabled(false);

    m_pEditorLayout->addWidget(m_pSelectedKeyGroupBox, 3, 0, 1, 2);
    QGridLayout *pSelectedKeyLayout = new QGridLayout(m_pSelectedKeyGroupBox);
    pSelectedKeyLayout->setSpacing(0);
    pSelectedKeyLayout->setContentsMargins(0, 0, 0, 0);

    m_pScanCodeLabel = new QLabel;
    m_pScanCodeEdit = new QLineEdit;
    m_pScanCodeLabel->setBuddy(m_pScanCodeEdit);
    m_pScanCodeEdit->setEnabled(false);
    pSelectedKeyLayout->addWidget(m_pScanCodeLabel, 0, 0);
    pSelectedKeyLayout->addWidget(m_pScanCodeEdit, 0, 1);

    m_pPositionLabel= new QLabel;
    m_pPositionEdit = new QLineEdit;
    m_pPositionEdit->setEnabled(false);
    m_pPositionLabel->setBuddy(m_pPositionEdit);
    pSelectedKeyLayout->addWidget(m_pPositionLabel, 1, 0);
    pSelectedKeyLayout->addWidget(m_pPositionEdit, 1, 1);

    QWidget *pCaptionEditor = prepareKeyCaptionEditWidgets();
    if (pCaptionEditor)
        pSelectedKeyLayout->addWidget(pCaptionEditor, 2, 0, 2, 2);

    QSpacerItem *pSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (pSpacer)
        pSelectedKeyLayout->addItem(pSpacer, 4, 1);

    retranslateUi();
}

QWidget *UILayoutEditor::prepareKeyCaptionEditWidgets()
{
    m_pCaptionEditGroupBox = new QGroupBox;
    if (!m_pCaptionEditGroupBox)
        return 0;
    m_pCaptionEditGroupBox->setFlat(false);
    QGridLayout *pCaptionEditorLayout = new QGridLayout(m_pCaptionEditGroupBox);
    pCaptionEditorLayout->setSpacing(0);
    pCaptionEditorLayout->setContentsMargins(0, 0, 0, 0);

    if (!pCaptionEditorLayout)
        return 0;

    m_pBaseCaptionLabel = new QLabel;
    m_pBaseCaptionEdit = new QLineEdit;
    m_pBaseCaptionLabel->setBuddy(m_pBaseCaptionEdit);
    pCaptionEditorLayout->addWidget(m_pBaseCaptionLabel, 0, 0);
    pCaptionEditorLayout->addWidget(m_pBaseCaptionEdit, 0, 1);
    connect(m_pBaseCaptionEdit, &QLineEdit::editingFinished, this, &UILayoutEditor::sltKeyBaseCaptionChange);

    m_pShiftCaptionLabel = new QLabel;
    m_pShiftCaptionEdit = new QLineEdit;
    m_pShiftCaptionLabel->setBuddy(m_pShiftCaptionEdit);
    pCaptionEditorLayout->addWidget(m_pShiftCaptionLabel, 1, 0);
    pCaptionEditorLayout->addWidget(m_pShiftCaptionEdit, 1, 1);
    connect(m_pShiftCaptionEdit, &QLineEdit::editingFinished, this, &UILayoutEditor::sltKeyShiftCaptionChange);

    m_pAltGrCaptionLabel = new QLabel;
    m_pAltGrCaptionEdit = new QLineEdit;
    m_pAltGrCaptionLabel->setBuddy(m_pAltGrCaptionEdit);
    pCaptionEditorLayout->addWidget(m_pAltGrCaptionLabel, 2, 0);
    pCaptionEditorLayout->addWidget(m_pAltGrCaptionEdit, 2, 1);
    connect(m_pAltGrCaptionEdit, &QLineEdit::editingFinished, this, &UILayoutEditor::sltKeyAltGrCaptionChange);


    QSpacerItem *pSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Expanding);
    if (pSpacer)
        pCaptionEditorLayout->addItem(pSpacer, 4, 1);
    return m_pCaptionEditGroupBox;
}

/*********************************************************************************************************************************
*   UILayoutSelector implementation.                                                                                  *
*********************************************************************************************************************************/

UILayoutSelector::UILayoutSelector(QWidget *pParent /* = 0 */)
    :QIWithRetranslateUI<QWidget>(pParent)
    , m_pLayoutListWidget(0)
    , m_pApplyLayoutButton(0)
    , m_pEditLayoutButton(0)
{
    prepareObjects();
}

QString UILayoutSelector::selectedLayoutName() const
{
    if (!m_pLayoutListWidget || !m_pLayoutListWidget->currentItem())
        return QString();
    return m_pLayoutListWidget->currentItem()->text();
}

void UILayoutSelector::selectLayoutByName(const QString &strLayoutName)
{
    if (!m_pLayoutListWidget)
        return;
    QList<QListWidgetItem *> items = m_pLayoutListWidget->findItems(strLayoutName, Qt::MatchFixedString |Qt::MatchCaseSensitive);
    if (items.isEmpty())
        return;
    QListWidgetItem *pItem = items[0];
    if (!pItem || pItem == m_pLayoutListWidget->currentItem())
        return;
    m_pLayoutListWidget->setCurrentItem(pItem);
}

void UILayoutSelector::setLayoutList(const QStringList &layoutNames)
{
    if (!m_pLayoutListWidget)
        return;
    m_pLayoutListWidget->clear();
    foreach (const QString &strLayoutName, layoutNames)
        m_pLayoutListWidget->addItem(strLayoutName);
}

void UILayoutSelector::retranslateUi()
{
    if (m_pApplyLayoutButton)
        m_pApplyLayoutButton->setToolTip(UISoftKeyboard::tr("Use the selected layout"));
    if (m_pEditLayoutButton)
        m_pEditLayoutButton->setToolTip(UISoftKeyboard::tr("Edit the selected layout"));
}

void UILayoutSelector::prepareObjects()
{
    QVBoxLayout *pLayout = new QVBoxLayout;
    if (!pLayout)
        return;
    pLayout->setSpacing(0);
    setLayout(pLayout);

    m_pLayoutListWidget = new QListWidget;
    pLayout->addWidget(m_pLayoutListWidget);
    connect(m_pLayoutListWidget, &QListWidget::currentTextChanged, this, &UILayoutSelector::sigApplySelectedLayout);

    QHBoxLayout *pButtonsLayout = new QHBoxLayout;
    pLayout->addLayout(pButtonsLayout);

    // m_pApplyLayoutButton = new QToolButton;
    // m_pApplyLayoutButton->setIcon(UIIconPool::iconSet(":/keyboard_16px.png"));
    // pButtonsLayout->addWidget(m_pApplyLayoutButton);


    m_pEditLayoutButton = new QToolButton;
    m_pEditLayoutButton->setIcon(UIIconPool::iconSet(":/keyboard_settings_16px.png"));
    pButtonsLayout->addWidget(m_pEditLayoutButton);
    connect(m_pEditLayoutButton, &QToolButton::pressed, this, &UILayoutSelector::sigEditSelectedLayout);

    pButtonsLayout->addStretch(2);

    retranslateUi();
}

/*********************************************************************************************************************************
*   UISoftKeyboardRow implementation.                                                                                  *
*********************************************************************************************************************************/

UISoftKeyboardRow::UISoftKeyboardRow()
    : m_iDefaultWidth(0)
    , m_iDefaultHeight(0)
    , m_iSpaceHeightAfter(0)
{
}

void UISoftKeyboardRow::setDefaultWidth(int iWidth)
{
    m_iDefaultWidth = iWidth;
}

int UISoftKeyboardRow::defaultWidth() const
{
    return m_iDefaultWidth;
}

void UISoftKeyboardRow::setDefaultHeight(int iHeight)
{
    m_iDefaultHeight = iHeight;
}

int UISoftKeyboardRow::defaultHeight() const
{
    return m_iDefaultHeight;
}

QVector<UISoftKeyboardKey> &UISoftKeyboardRow::keys()
{
    return m_keys;
}

const QVector<UISoftKeyboardKey> &UISoftKeyboardRow::keys() const
{
    return m_keys;
}

void UISoftKeyboardRow::setSpaceHeightAfter(int iSpace)
{
    m_iSpaceHeightAfter = iSpace;
}

int UISoftKeyboardRow::spaceHeightAfter() const
{
    return m_iSpaceHeightAfter;
}

/*********************************************************************************************************************************
*   UISoftKeyboardKey implementation.                                                                                  *
*********************************************************************************************************************************/

UISoftKeyboardKey::UISoftKeyboardKey()
    : m_enmType(UIKeyType_Ordinary)
    , m_enmState(UIKeyState_NotPressed)
    , m_iWidth(0)
    , m_iHeight(0)
    , m_iSpaceWidthAfter(0)
    , m_scanCode(0)
    , m_scanCodePrefix(0)
    , m_iCutoutWidth(0)
    , m_iCutoutHeight(0)
    , m_iCutoutCorner(-1)
    , m_iPosition(0)
    , m_pParentWidget(0)
{
}

const QRect UISoftKeyboardKey::keyGeometry() const
{
    return m_keyGeometry;
}

void UISoftKeyboardKey::setKeyGeometry(const QRect &rect)
{
    m_keyGeometry = rect;
}

const QString UISoftKeyboardKey::baseCaption() const
{
    return m_strBaseCaption;
}

void UISoftKeyboardKey::setBaseCaption(const QString &strBaseCaption)
{
    m_strBaseCaption = strBaseCaption;
    updateText();
}

const QString UISoftKeyboardKey::shiftCaption() const
{
    return m_strShiftCaption;
}

void UISoftKeyboardKey::setShiftCaption(const QString &strShiftCaption)
{
    m_strShiftCaption = strShiftCaption;
    updateText();
}

const QString UISoftKeyboardKey::altGrCaption() const
{
    return m_strAltGrCaption;
}

void UISoftKeyboardKey::setAltGrCaption(const QString &strAltGrCaption)
{
    m_strAltGrCaption = strAltGrCaption;
    updateText();
}

const QString UISoftKeyboardKey::text() const
{
    return m_strText;
}

void UISoftKeyboardKey::setWidth(int iWidth)
{
    m_iWidth = iWidth;
}

int UISoftKeyboardKey::width() const
{
    return m_iWidth;
}

void UISoftKeyboardKey::setHeight(int iHeight)
{
    m_iHeight = iHeight;
}

int UISoftKeyboardKey::height() const
{
    return m_iHeight;
}

void UISoftKeyboardKey::setScanCode(LONG scanCode)
{
    m_scanCode = scanCode;
}

LONG UISoftKeyboardKey::scanCode() const
{
    return m_scanCode;
}

void UISoftKeyboardKey::setScanCodePrefix(LONG scanCodePrefix)
{
    m_scanCodePrefix = scanCodePrefix;
}

LONG UISoftKeyboardKey::scanCodePrefix() const
{
    return m_scanCodePrefix;
}

void UISoftKeyboardKey::setSpaceWidthAfter(int iSpace)
{
    m_iSpaceWidthAfter = iSpace;
}

int UISoftKeyboardKey::spaceWidthAfter() const
{
    return m_iSpaceWidthAfter;
}

void UISoftKeyboardKey::setPosition(int iPosition)
{
    m_iPosition = iPosition;
}

int UISoftKeyboardKey::position() const
{
    return m_iPosition;
}

void UISoftKeyboardKey::setType(UIKeyType enmType)
{
    m_enmType = enmType;
}

UIKeyType UISoftKeyboardKey::type() const
{
    return m_enmType;
}

void UISoftKeyboardKey::setCutout(int iCorner, int iWidth, int iHeight)
{
    m_iCutoutCorner = iCorner;
    m_iCutoutWidth = iWidth;
    m_iCutoutHeight = iHeight;
}

UIKeyState UISoftKeyboardKey::state() const
{
    return m_enmState;
}

void UISoftKeyboardKey::setParentWidget(UISoftKeyboardWidget* pParent)
{
    m_pParentWidget = pParent;
}

void UISoftKeyboardKey::release()
{
    updateState(false);
}

void UISoftKeyboardKey::press()
{
    updateState(true);
}

void UISoftKeyboardKey::setPolygon(const QPolygon &polygon)
{
    m_polygon = polygon;
}

const QPolygon &UISoftKeyboardKey::polygon() const
{
    return m_polygon;
}

QPolygon UISoftKeyboardKey::polygonInGlobal() const
{
    QPolygon globalPolygon(m_polygon);
    globalPolygon.translate(m_keyGeometry.x(), m_keyGeometry.y());
    return globalPolygon;
}

int UISoftKeyboardKey::cutoutCorner() const
{
    return m_iCutoutCorner;
}

int UISoftKeyboardKey::cutoutWidth() const
{
    return m_iCutoutWidth;
}

int UISoftKeyboardKey::cutoutHeight() const
{
    return m_iCutoutHeight;
}

void UISoftKeyboardKey::updateState(bool fPressed)
{
    UIKeyState enmPreviousState = state();
    if (m_enmType == UIKeyType_Modifier)
    {
        if (fPressed)
        {
            if (m_enmState == UIKeyState_NotPressed)
                m_enmState = UIKeyState_Pressed;
            else if(m_enmState == UIKeyState_Pressed)
                m_enmState = UIKeyState_Locked;
            else
                m_enmState = UIKeyState_NotPressed;
        }
        else
        {
            if(m_enmState == UIKeyState_Pressed)
                m_enmState = UIKeyState_NotPressed;
        }
    }
    else if (m_enmType == UIKeyType_Toggleable)
    {
        if (fPressed)
        {
            if (m_enmState == UIKeyState_NotPressed)
                 m_enmState = UIKeyState_Locked;
            else
                m_enmState = UIKeyState_NotPressed;
        }
    }
    else if (m_enmType == UIKeyType_Ordinary)
    {
        if (m_enmState == UIKeyState_NotPressed)
            m_enmState = UIKeyState_Pressed;
        else
            m_enmState = UIKeyState_NotPressed;
    }
    if (enmPreviousState != state() && m_pParentWidget)
        m_pParentWidget->keyStateChange(this);
}

void UISoftKeyboardKey::updateText()
{
    m_strText.clear();
    if (!m_strShiftCaption.isEmpty())
        m_strText += QString("%1\n").arg(m_strShiftCaption);
    if (!m_strBaseCaption.isEmpty())
        m_strText += QString("%1\n").arg(m_strBaseCaption);
    if (!m_strAltGrCaption.isEmpty())
        m_strText += QString("%1\n").arg(m_strAltGrCaption);
}

/*********************************************************************************************************************************
*   UISoftKeyboardWidget implementation.                                                                                  *
*********************************************************************************************************************************/

UISoftKeyboardWidget::UISoftKeyboardWidget(QWidget *pParent /* = 0 */)
    :QIWithRetranslateUI<QWidget>(pParent)
    , m_pKeyUnderMouse(0)
    , m_pKeyBeingEdited(0)
    , m_pKeyPressed(0)
    , m_keyDefaultColor(QColor(103, 128, 159))
    , m_keyHoverColor(QColor(108, 122, 137))
    , m_textDefaultColor(QColor(46, 49, 49))
    , m_textPressedColor(QColor(149, 165, 166))
    , m_keyEditColor(QColor(249, 165, 166))
    , m_pCurrentKeyboardLayout(0)
    , m_iInitialHeight(0)
    , m_iInitialWidth(0)
    , m_iXSpacing(5)
    , m_iYSpacing(5)
    , m_iLeftMargin(10)
    , m_iTopMargin(10)
    , m_iRightMargin(10)
    , m_iBottomMargin(10)
    , m_pContextMenu(0)
    , m_pShowPositionsAction(0)
    , m_pLayoutEditModeToggleAction(0)
    , m_enmMode(Mode_Keyboard)
{
    prepareObjects();
}

QSize UISoftKeyboardWidget::minimumSizeHint() const
{
    return QSize(0.5 * m_minimumSize.width(), 0.5 * m_minimumSize.height());
}

QSize UISoftKeyboardWidget::sizeHint() const
{
    return QSize(0.5 * m_minimumSize.width(), 0.5 * m_minimumSize.height());
    //return m_minimumSize;
}

void UISoftKeyboardWidget::paintEvent(QPaintEvent *pEvent) /* override */
{
    Q_UNUSED(pEvent);

    int iEditDialogWidth = 0;
    if (m_enmMode == Mode_LayoutEdit)
        iEditDialogWidth = 34 * QApplication::fontMetrics().width('x');
    m_fScaleFactorX = (width() - iEditDialogWidth) / (float) m_iInitialWidth;
    m_fScaleFactorY = height() / (float) m_iInitialHeight;

    QPainter painter(this);
    QFont painterFont(font());
    painterFont.setPixelSize(15);
    painterFont.setBold(false);
    painter.setFont(painterFont);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(m_fScaleFactorX, m_fScaleFactorY);
    int unitSize = qApp->style()->pixelMetric(QStyle::PM_LayoutLeftMargin);
    float fLedRadius =  0.8 * unitSize;
    float fLedMargin =  0.6 * unitSize;

    if (!m_pCurrentKeyboardLayout || m_iInitialWidth == 0 || m_iInitialHeight == 0)
        return;
    if (!m_pCurrentKeyboardLayout->m_pPhysicalLayout)
        return;

    QVector<UISoftKeyboardRow> &rows = m_pCurrentKeyboardLayout->m_pPhysicalLayout->m_rows;
    for (int i = 0; i < rows.size(); ++i)
    {
        QVector<UISoftKeyboardKey> &keys = rows[i].keys();
        for (int j = 0; j < keys.size(); ++j)
        {
            UISoftKeyboardKey &key = keys[j];
            painter.translate(key.keyGeometry().x(), key.keyGeometry().y());

            if(&key  == m_pKeyBeingEdited)
                painter.setBrush(QBrush(m_keyEditColor));
            else if (&key  == m_pKeyUnderMouse)
                painter.setBrush(QBrush(m_keyHoverColor));
            else
                painter.setBrush(QBrush(m_keyDefaultColor));

            if (&key  == m_pKeyPressed)
                painter.setPen(QPen(QColor(m_textPressedColor), 2));
            else
                painter.setPen(QPen(QColor(m_textDefaultColor), 2));

            painter.drawPolygon(key.polygon());


            QRect textRect(0.55 * unitSize, 1 * unitSize
                           , key.keyGeometry().width(), key.keyGeometry().height());
            if (m_pShowPositionsAction && m_pShowPositionsAction->isChecked())
                painter.drawText(textRect, Qt::TextWordWrap, QString::number(key.position()));
            else
                painter.drawText(textRect, Qt::TextWordWrap, key.text());

            if (key.type() != UIKeyType_Ordinary)
            {
                QColor ledColor;
                if (key.state() == UIKeyState_NotPressed)
                    ledColor = m_textDefaultColor;
                else if (key.state() == UIKeyState_Pressed)
                    ledColor = QColor(0, 255, 0);
                else
                    ledColor = QColor(255, 0, 0);
                painter.setBrush(ledColor);
                painter.setPen(ledColor);
                QRectF rectangle(key.keyGeometry().width() - 2 * fLedMargin, fLedMargin, fLedRadius, fLedRadius);
                painter.drawEllipse(rectangle);
            }
            painter.translate(-key.keyGeometry().x(), -key.keyGeometry().y());

        }
    }
}

void UISoftKeyboardWidget::mousePressEvent(QMouseEvent *pEvent)
{
    QWidget::mousePressEvent(pEvent);
    if (pEvent->button() != Qt::LeftButton)
        return;
    m_pKeyPressed = keyUnderMouse(pEvent);

    if (m_enmMode == Mode_Keyboard)
        handleKeyPress(m_pKeyPressed);
    else if (m_enmMode == Mode_LayoutEdit)
    {
        /* If the editor is shown already for another key clicking away accepts the entered text: */
        if (m_pKeyBeingEdited && m_pKeyBeingEdited != m_pKeyUnderMouse)
        {
            //printf("farkli %p\n", m_pKeyBeingEdited);
        }
        setKeyBeingEdited(m_pKeyUnderMouse);
        // if (m_pKeyBeingEdited && m_pLayoutEditor)
        //     m_pLayoutEditor->setText(m_pKeyBeingEdited->keyCap());
    }
    update();
}

void UISoftKeyboardWidget::mouseReleaseEvent(QMouseEvent *pEvent)
{
    QWidget::mouseReleaseEvent(pEvent);
    if (pEvent->button() != Qt::LeftButton)
        return;
    if (!m_pKeyPressed)
        return;

    if (m_enmMode == Mode_Keyboard)
        handleKeyRelease(m_pKeyPressed);

    update();
    m_pKeyPressed = 0;
}

void UISoftKeyboardWidget::mouseMoveEvent(QMouseEvent *pEvent)
{
    QWidget::mouseMoveEvent(pEvent);
    keyUnderMouse(pEvent);
}

// bool UISoftKeyboardWidget::eventFilter(QObject *pWatched, QEvent *pEvent)
// {
//     if (pWatched != m_pLayoutEditor)
//         return QIWithRetranslateUI<QWidget>::eventFilter(pWatched, pEvent);

//     switch (pEvent->type())
//     {
//         case QEvent::KeyPress:
//         {
//             QKeyEvent *pKeyEvent = dynamic_cast<QKeyEvent*>(pEvent);
//             if (pKeyEvent && pKeyEvent->key() == Qt::Key_Escape)
//             {
//                 setKeyBeingEdited(0);
//                 update();
//                 return true;
//             }
//             else if (pKeyEvent && (pKeyEvent->key() == Qt::Key_Enter || pKeyEvent->key() == Qt::Key_Return))
//             {
//                 // if (m_pKeyBeingEdited)
//                 //     m_pKeyBeingEdited->setStaticKeyCap(m_pLayoutEditor->text());
//                 setKeyBeingEdited(0);
//                 update();
//                 return true;
//             }
//             break;
//         }
//         default:
//             break;
//     }

//     return QIWithRetranslateUI<QWidget>::eventFilter(pWatched, pEvent);
// }


void UISoftKeyboardWidget::retranslateUi()
{
}

void UISoftKeyboardWidget::sltHandleContextMenuRequest(const QPoint &point)
{
    showContextMenu(mapToGlobal(point));
}

// void UISoftKeyboardWidget::sltHandleLoadLayout(QAction *pSenderAction)
// {
//     if (!pSenderAction)
//         return;
//     if (pSenderAction == m_pLoadLayoutFileAction)
//     {
//         const QString strFileName = QIFileDialog::getOpenFileName(QString(), UISoftKeyboard::tr("XML files (*.xml)"), this,
//                                                                   UISoftKeyboard::tr("Choose file to load physical keyboard layout.."));
//         if (!strFileName.isEmpty() && loadPhysicalLayout(strFileName))
//         {
//             m_pLastSelectedLayout = pSenderAction;
//             m_pLoadLayoutFileAction->setData(strFileName);
//
//             return;
//         }
//     }
//     else
//     {
//         QString strLayout = pSenderAction->data().toString();
//         if (!strLayout.isEmpty() && loadPhysicalLayout(strLayout))
//         {
//             m_pLastSelectedLayout = pSenderAction;
//             emit sigLayoutChange(strLayout);
//             return;
//         }
//     }
//     /* In case something went wrong try to restore the previous layout: */
//     if (m_pLastSelectedLayout)
//     {
//         QString strLayout = m_pLastSelectedLayout->data().toString();
//         loadPhysicalLayout(strLayout);
//         emit sigLayoutChange(strLayout);
//         m_pLastSelectedLayout->setChecked(true);
//     }
// }

void UISoftKeyboardWidget::sltHandleSaveLayout()
{
    QString strFileName = QIFileDialog::getSaveFileName(vboxGlobal().documentsPath(), UISoftKeyboard::tr("XML files (*.xml)"),
                                                        this, tr("Save layout to a file..."));
    if (strFileName.isEmpty())
        return;
    if (!m_pCurrentKeyboardLayout || !m_pCurrentKeyboardLayout->m_pPhysicalLayout)
        return;

    QFileInfo fileInfo(strFileName);
    if (fileInfo.suffix().compare("xml", Qt::CaseInsensitive) != 0)
        strFileName += ".xml";

    QFile xmlFile(strFileName);
    if (!xmlFile.open(QIODevice::WriteOnly))
        return;
    QXmlStreamWriter xmlWriter;
    xmlWriter.setDevice(&xmlFile);

    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeStartDocument("1.0");
    xmlWriter.writeStartElement("layout");
    xmlWriter.writeTextElement("name", m_pCurrentKeyboardLayout->name());
    xmlWriter.writeTextElement("physicallayoutid", m_pCurrentKeyboardLayout->m_pPhysicalLayout->m_uId.toString());

    QVector<UISoftKeyboardRow> &rows = m_pCurrentKeyboardLayout->m_pPhysicalLayout->m_rows;
    for (int i = 0; i < rows.size(); ++i)
    {
        QVector<UISoftKeyboardKey> &keys = rows[i].keys();

       for (int j = 0; j < keys.size(); ++j)
       {
           xmlWriter.writeStartElement("key");

           UISoftKeyboardKey &key = keys[j];
           xmlWriter.writeTextElement("position", QString::number(key.position()));
           xmlWriter.writeTextElement("basecaption", key.baseCaption());
           xmlWriter.writeTextElement("shiftcaption", key.shiftCaption());
           xmlWriter.writeTextElement("altgrcaption", key.altGrCaption());

           // if (!key.keyCap().isEmpty())
           //     xmlWriter.writeTextElement("keycap", key.keyCap());
           // else
           //     xmlWriter.writeTextElement("keycap", key.staticKeyCap());
           xmlWriter.writeEndElement();
       }
   }
   xmlWriter.writeEndElement();
   xmlWriter.writeEndDocument();

   xmlFile.close();
}

// void UISoftKeyboardWidget::sltHandleLoadKeyCapFile()
// {
//     const QString strFileName = QIFileDialog::getOpenFileName(QString(), UISoftKeyboard::tr("XML files (*.xml)"), this,
//                                                               UISoftKeyboard::tr("Choose file to load key captions.."));
//     if (strFileName.isEmpty())
//         return;
//     UIKeyboardLayoutReader keyCapReader(strFileName);
//     //const QMap<int, QString> &keyCapMap = keyCapReader.keyCapMap();

//     //for (int i = 0; i < m_rows.size(); ++i)
//     {
//         //QVector<UISoftKeyboardKey> &keys = m_rows[i].keys();
//         // for (int j = 0; j < keys.size(); ++j)
//         // {
//         //     UISoftKeyboardKey &key = keys[j];
//         //     if (keyCapMap.contains(key.position()))
//         //         key.setKeyCap(keyCapMap[key.position()]);
//         // }
//     }
//     emit sigKeyCapChange(strFileName);
// }

void UISoftKeyboardWidget::sltHandleLayoutEditModeToggle(bool fToggle)
{
    if (fToggle)
        m_enmMode = Mode_LayoutEdit;
    else
    {
        m_enmMode = Mode_Keyboard;
        m_pKeyBeingEdited = 0;
    }
}

void UISoftKeyboardWidget::sltHandleNewLayout()
{
    /* We need at least one physical layout: */
    if (m_physicalLayouts.isEmpty())
        return;
    m_layouts.append(UISoftKeyboardLayout());
    UISoftKeyboardLayout &newLayout = m_layouts.back();
    newLayout.m_pPhysicalLayout = &(m_physicalLayouts[0]);
    newLayout.setName(QString(UISoftKeyboard::tr("Unnamed")));
    setCurrentLayout(&newLayout);
}

void UISoftKeyboardWidget::sltPhysicalLayoutForLayoutChanged(int iIndex)
{
    if (!m_pCurrentKeyboardLayout)
        return;
    if (iIndex < 0 || iIndex >= m_physicalLayouts.size())
        return;

    if (m_pCurrentKeyboardLayout->m_pPhysicalLayout == &(m_physicalLayouts[iIndex]))
        return;
    m_pCurrentKeyboardLayout->m_pPhysicalLayout = &(m_physicalLayouts[iIndex]);
    update();
}

void UISoftKeyboardWidget::setNewMinimumSize(const QSize &size)
{
    m_minimumSize = size;
    updateGeometry();
}

void UISoftKeyboardWidget::setInitialSize(int iWidth, int iHeight)
{
    m_iInitialWidth = iWidth;
    m_iInitialHeight = iHeight;
}

UISoftKeyboardKey *UISoftKeyboardWidget::keyUnderMouse(QMouseEvent *pEvent)
{
    QPoint eventPosition(pEvent->pos().x() / m_fScaleFactorX, pEvent->pos().y() / m_fScaleFactorY);
    return keyUnderMouse(eventPosition);
}

UISoftKeyboardKey *UISoftKeyboardWidget::keyUnderMouse(const QPoint &eventPosition)
{
    if (!m_pCurrentKeyboardLayout || !m_pCurrentKeyboardLayout->m_pPhysicalLayout)
        return 0;
    UISoftKeyboardKey *pKey = 0;
    QVector<UISoftKeyboardRow> &rows = m_pCurrentKeyboardLayout->m_pPhysicalLayout->m_rows;
    for (int i = 0; i < rows.size(); ++i)
    {
        QVector<UISoftKeyboardKey> &keys = rows[i].keys();
        for (int j = 0; j < keys.size(); ++j)
        {
            UISoftKeyboardKey &key = keys[j];
            if (key.polygonInGlobal().containsPoint(eventPosition, Qt::OddEvenFill))
            {
                pKey = &key;
                break;
            }
        }
    }
    if (m_pKeyUnderMouse != pKey)
    {
        m_pKeyUnderMouse = pKey;
        update();
    }
    return pKey;
}

void UISoftKeyboardWidget::handleKeyPress(UISoftKeyboardKey *pKey)
{
    if (!pKey)
        return;
    pKey->press();

    if (pKey->type() == UIKeyType_Modifier)
        return;

    QVector<LONG> sequence;
     /* Add the pressed modifiers first: */
    for (int i = 0; i < m_pressedModifiers.size(); ++i)
    {
        UISoftKeyboardKey *pModifier = m_pressedModifiers[i];
        if (pModifier->scanCodePrefix() != 0)
            sequence << pModifier->scanCodePrefix();
        sequence << pModifier->scanCode();
    }

    if (pKey->scanCodePrefix() != 0)
        sequence << pKey->scanCodePrefix();
    sequence << pKey->scanCode();
    emit sigPutKeyboardSequence(sequence);
}

void UISoftKeyboardWidget::keyStateChange(UISoftKeyboardKey* pKey)
{
    if (!pKey)
        return;
    if (pKey->type() == UIKeyType_Modifier)
    {
        if (pKey->state() == UIKeyState_NotPressed)
            m_pressedModifiers.removeOne(pKey);
        else
            if (!m_pressedModifiers.contains(pKey))
                m_pressedModifiers.append(pKey);
    }
}

void UISoftKeyboardWidget::showContextMenu(const QPoint &globalPoint)
{
    m_pContextMenu->exec(globalPoint);
    update();
}

void UISoftKeyboardWidget::applyLayoutByName(const QString &strLayoutName)
{
    UISoftKeyboardLayout *pLayout = findLayoutByName(strLayoutName);
    if (!pLayout)
        return;
    setCurrentLayout(pLayout);
}

void UISoftKeyboardWidget::handleKeyRelease(UISoftKeyboardKey *pKey)
{
    if (!pKey)
        return;
    if (pKey->type() == UIKeyType_Ordinary)
        pKey->release();
    /* We only send the scan codes of Ordinary keys: */
    if (pKey->type() == UIKeyType_Modifier)
        return;

    QVector<LONG> sequence;
    if (pKey->scanCodePrefix() != 0)
        sequence <<  pKey->scanCodePrefix();
    sequence << (pKey->scanCode() | 0x80);

    /* Add the pressed modifiers in the reverse order: */
    for (int i = m_pressedModifiers.size() - 1; i >= 0; --i)
    {
        UISoftKeyboardKey *pModifier = m_pressedModifiers[i];
        if (pModifier->scanCodePrefix() != 0)
            sequence << pModifier->scanCodePrefix();
        sequence << (pModifier->scanCode() | 0x80);
        /* Release the pressed modifiers (if there are not locked): */
        pModifier->release();
    }
    emit sigPutKeyboardSequence(sequence);
}

bool UISoftKeyboardWidget::loadPhysicalLayout(const QString &strLayoutFileName)
{
    if (strLayoutFileName.isEmpty())
        return false;
    UIPhysicalLayoutReader reader;
    m_physicalLayouts.append(UISoftKeyboardPhysicalLayout());
    UISoftKeyboardPhysicalLayout &newPhysicalLayout = m_physicalLayouts.back();

    if (!reader.parseXMLFile(strLayoutFileName, newPhysicalLayout))
    {
        m_physicalLayouts.removeLast();
        return false;
    }

    int iY = m_iTopMargin;
    int iMaxWidth = 0;
    QVector<UISoftKeyboardRow> &rows = newPhysicalLayout.m_rows;
    for (int i = 0; i < rows.size(); ++i)
    {
        UISoftKeyboardRow &row = rows[i];
        int iX = m_iLeftMargin;
        int iRowHeight = row.defaultHeight();
        for (int j = 0; j < row.keys().size(); ++j)
        {
            UISoftKeyboardKey &key = (row.keys())[j];
            key.setKeyGeometry(QRect(iX, iY, key.width(), key.height()));
            key.setPolygon(QPolygon(UIPhysicalLayoutReader::computeKeyVertices(key)));
            key.setParentWidget(this);
            iX += key.width();
            if (j < row.keys().size() - 1)
                iX += m_iXSpacing;
            if (key.spaceWidthAfter() != 0)
                iX += (m_iXSpacing + key.spaceWidthAfter());
        }
        if (row.spaceHeightAfter() != 0)
            iY += row.spaceHeightAfter() + m_iYSpacing;
        iMaxWidth = qMax(iMaxWidth, iX);
        iY += iRowHeight;
        if (i < rows.size() - 1)
            iY += m_iYSpacing;
    }
    int iInitialWidth = iMaxWidth + m_iRightMargin;
    int iInitialHeight = iY + m_iBottomMargin;

    setNewMinimumSize(QSize(iInitialWidth, iInitialHeight));
    setInitialSize(iInitialWidth, iInitialHeight);
    return true;
}

bool UISoftKeyboardWidget::loadKeyboardLayout(const QString &strLayoutName)
{
    if (strLayoutName.isEmpty())
        return false;

    UIKeyboardLayoutReader keyboardLayoutReader;

    if (!keyboardLayoutReader.parseFile(strLayoutName))
        return false;


    UISoftKeyboardPhysicalLayout *pPhysicalLayout = 0;
    /* Search for the physical layout among the one stored in m_pPhysicalLayout: */
    for (int i = 0; i < m_physicalLayouts.size(); ++i)
    {
        if (keyboardLayoutReader.physicalLayoutUUID() == m_physicalLayouts[i].m_uId)
            pPhysicalLayout = &(m_physicalLayouts[i]);
    }

    /* If no pyhsical layout with the UUID the keyboard layout refers is found then cancel loading the keyboard layout: */
    if (!pPhysicalLayout)
        return false;

    m_layouts.append(UISoftKeyboardLayout());
    UISoftKeyboardLayout &newLayout = m_layouts.back();
    newLayout.m_pPhysicalLayout = pPhysicalLayout;
    newLayout.setName(keyboardLayoutReader.name());
    newLayout.m_keyCapMap = keyboardLayoutReader.keyCapMap();
    return true;
}

void UISoftKeyboardWidget::reset()
{
    m_pressedModifiers.clear();
    //m_rows.clear();
}

void UISoftKeyboardWidget::loadLayouts()
{
    /* Load physical layouts from resources: */
    QStringList physicalLayoutNames;
    physicalLayoutNames << ":/101_ansi.xml" << ":/102_iso.xml";
    foreach (const QString &strName, physicalLayoutNames)
        loadPhysicalLayout(strName);

    /* Load keyboard layouts from resources: */
    QStringList keyboardLayoutNames;
    keyboardLayoutNames << ":/us_international.xml"
                        <<":/german.xml";
    foreach (const QString &strName, keyboardLayoutNames)
        loadKeyboardLayout(strName);
    emitLayoutNames();
    if (m_layouts.isEmpty())
        return;
    setCurrentLayout(&(m_layouts[0]));
}

void UISoftKeyboardWidget::prepareObjects()
{
    m_pContextMenu = new QMenu(this);

    QAction *pNewLayoutAction = m_pContextMenu->addAction(UISoftKeyboard::tr("New Layout..."));
    connect(pNewLayoutAction, &QAction::triggered, this, &UISoftKeyboardWidget::sltHandleNewLayout);

    m_pLayoutEditModeToggleAction = m_pContextMenu->addAction(UISoftKeyboard::tr("Edit the layout"));
    connect(m_pLayoutEditModeToggleAction, &QAction::toggled, this, &UISoftKeyboardWidget::sltHandleLayoutEditModeToggle);
    m_pLayoutEditModeToggleAction->setCheckable(true);
    m_pLayoutEditModeToggleAction->setChecked(false);

    QAction *pSaveLayoutAction = m_pContextMenu->addAction(UISoftKeyboard::tr("Save layout to file..."));
    connect(pSaveLayoutAction, &QAction::triggered, this, &UISoftKeyboardWidget::sltHandleSaveLayout);

#ifdef DEBUG
    m_pShowPositionsAction = m_pContextMenu->addAction(UISoftKeyboard::tr("Show key positions instead of key caps"));
    m_pShowPositionsAction->setCheckable(true);
    m_pShowPositionsAction->setChecked(false);
#endif

    setMouseTracking(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &UISoftKeyboardWidget::customContextMenuRequested,
            this, &UISoftKeyboardWidget::sltHandleContextMenuRequest);
}

void UISoftKeyboardWidget::setKeyBeingEdited(UISoftKeyboardKey* pKey)
{
    if (m_pKeyBeingEdited == pKey)
        return;
    m_pKeyBeingEdited = pKey;
    // if (m_pLayoutEditor)
    //     m_pLayoutEditor->setKey(pKey);
}

void UISoftKeyboardWidget::setCurrentLayout(UISoftKeyboardLayout *pLayout)
{
    if (m_pCurrentKeyboardLayout == pLayout)
        return;
    m_pCurrentKeyboardLayout = pLayout;
    if (!m_pCurrentKeyboardLayout)
    {
        emit sigLayoutChange(QString());
        return;
    }
    emit sigLayoutChange(m_pCurrentKeyboardLayout->name());

    const QMap<int, KeyCaptions> &keyCapMap = m_pCurrentKeyboardLayout->m_keyCapMap;

    /* Update the key captions: */
    QVector<UISoftKeyboardRow> &rows = m_pCurrentKeyboardLayout->m_pPhysicalLayout->m_rows;
    for (int i = 0; i < rows.size(); ++i)
    {
        QVector<UISoftKeyboardKey> &keys = rows[i].keys();
        for (int j = 0; j < keys.size(); ++j)
        {
            UISoftKeyboardKey &key = keys[j];
            if (!keyCapMap.contains(key.position()))
                continue;
            const KeyCaptions &captions = keyCapMap.value(key.position());
            key.setBaseCaption(captions.m_strBase);
            key.setShiftCaption(captions.m_strShift);
            key.setAltGrCaption(captions.m_strAltGr);
        }
    }
}

void UISoftKeyboardWidget::emitLayoutNames()
{
    emit sigLayoutListChanged(layoutNameList());
}

UISoftKeyboardLayout *UISoftKeyboardWidget::findLayoutByName(const QString &strName)
{
    for (int i = 0; i < m_layouts.size(); ++i)
    {
        if (m_layouts[i].name() == strName)
            return &(m_layouts[i]);
    }
    return 0;
}

QStringList UISoftKeyboardWidget::layoutNameList() const
{
    QStringList layoutNames;
    foreach (const UISoftKeyboardLayout &layout, m_layouts)
        layoutNames << layout.name();
    return layoutNames;
}

const QVector<UISoftKeyboardPhysicalLayout> &UISoftKeyboardWidget::physicalLayouts() const
{
    return m_physicalLayouts;
}

/*********************************************************************************************************************************
*   UIPhysicalLayoutReader implementation.                                                                                  *
*********************************************************************************************************************************/

bool UIPhysicalLayoutReader::parseXMLFile(const QString &strFileName, UISoftKeyboardPhysicalLayout &physicalLayout)
{
    QFile xmlFile(strFileName);
    if (!xmlFile.exists())
        return false;

    if (!xmlFile.open(QIODevice::ReadOnly))
        return false;

    m_xmlReader.setDevice(&xmlFile);

    if (!m_xmlReader.readNextStartElement() || m_xmlReader.name() != "physicallayout")
        return false;
    physicalLayout.m_strFileName = strFileName;

    QXmlStreamAttributes attributes = m_xmlReader.attributes();
    int iDefaultWidth = attributes.value("defaultWidth").toInt();
    int iDefaultHeight = attributes.value("defaultHeight").toInt();
    QVector<UISoftKeyboardRow> &rows = physicalLayout.m_rows;
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "row")
            parseRow(iDefaultWidth, iDefaultHeight, rows);
        else if (m_xmlReader.name() == "space")
            parseRowSpace(rows);
        else if (m_xmlReader.name() == "name")
            physicalLayout.setName(m_xmlReader.readElementText());
        else if (m_xmlReader.name() == "id")
            physicalLayout.m_uId = m_xmlReader.readElementText();
        else
            m_xmlReader.skipCurrentElement();
    }

    return true;
}

void UIPhysicalLayoutReader::parseRow(int iDefaultWidth, int iDefaultHeight, QVector<UISoftKeyboardRow> &rows)
{
    rows.append(UISoftKeyboardRow());
    UISoftKeyboardRow &row = rows.back();

    row.setDefaultWidth(iDefaultWidth);
    row.setDefaultHeight(iDefaultHeight);
    row.setSpaceHeightAfter(0);

    /* Override the layout attributes if the row also has them: */
    QXmlStreamAttributes attributes = m_xmlReader.attributes();
    if (attributes.hasAttribute("defaultWidth"))
        row.setDefaultWidth(attributes.value("defaultWidth").toInt());
    if (attributes.hasAttribute("defaultHeight"))
        row.setDefaultHeight(attributes.value("defaultHeight").toInt());
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "key")
            parseKey(row);
        else if (m_xmlReader.name() == "space")
            parseKeySpace(row);
        else
            m_xmlReader.skipCurrentElement();
    }
}

void UIPhysicalLayoutReader::parseRowSpace(QVector<UISoftKeyboardRow> &rows)
{
    int iSpace = 0;
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "height")
            iSpace = m_xmlReader.readElementText().toInt();
        else
            m_xmlReader.skipCurrentElement();
    }
    if (!rows.empty())
        rows.back().setSpaceHeightAfter(iSpace);
}

void UIPhysicalLayoutReader::parseKey(UISoftKeyboardRow &row)
{
    row.keys().append(UISoftKeyboardKey());
    UISoftKeyboardKey &key = row.keys().back();
    key.setWidth(row.defaultWidth());
    key.setHeight(row.defaultHeight());
    QString strKeyCap;
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "width")
            key.setWidth(m_xmlReader.readElementText().toInt());
        else if (m_xmlReader.name() == "height")
            key.setHeight(m_xmlReader.readElementText().toInt());
        else if (m_xmlReader.name() == "scancode")
        {
            QString strCode = m_xmlReader.readElementText();
            bool fOk = false;
            key.setScanCode(strCode.toInt(&fOk, 16));
        }
        else if (m_xmlReader.name() == "scancodeprefix")
        {
            QString strCode = m_xmlReader.readElementText();
            bool fOk = false;
            key.setScanCodePrefix(strCode.toInt(&fOk, 16));
        }
        else if (m_xmlReader.name() == "cutout")
            parseCutout(key);
        else if (m_xmlReader.name() == "position")
            key.setPosition(m_xmlReader.readElementText().toInt());
        else if (m_xmlReader.name() == "type")
        {
            QString strType = m_xmlReader.readElementText();
            if (strType == "modifier")
                key.setType(UIKeyType_Modifier);
            else if (strType == "toggleable")
                key.setType(UIKeyType_Toggleable);
        }
        else
            m_xmlReader.skipCurrentElement();
    }
}

void UIPhysicalLayoutReader::parseKeySpace(UISoftKeyboardRow &row)
{
    int iWidth = row.defaultWidth();
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "width")
            iWidth = m_xmlReader.readElementText().toInt();
        else
            m_xmlReader.skipCurrentElement();
    }
    if (row.keys().size() <= 0)
        return;
    row.keys().back().setSpaceWidthAfter(iWidth);
}

void UIPhysicalLayoutReader::parseCutout(UISoftKeyboardKey &key)
{
    int iWidth = 0;
    int iHeight = 0;
    int iCorner = 0;
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "width")
            iWidth = m_xmlReader.readElementText().toInt();
        else if (m_xmlReader.name() == "height")
            iHeight = m_xmlReader.readElementText().toInt();
        else if (m_xmlReader.name() == "corner")
        {
            QString strCorner = m_xmlReader.readElementText();
            if (strCorner == "topLeft")
                    iCorner = 0;
            else if(strCorner == "topRight")
                    iCorner = 1;
            else if(strCorner == "bottomRight")
                    iCorner = 2;
            else if(strCorner == "bottomLeft")
                    iCorner = 3;
        }
        else
            m_xmlReader.skipCurrentElement();
    }
    key.setCutout(iCorner, iWidth, iHeight);
}

QVector<QPoint> UIPhysicalLayoutReader::computeKeyVertices(const UISoftKeyboardKey &key)
{
    QVector<QPoint> vertices;

    if (key.cutoutCorner() == -1 || key.width() <= key.cutoutWidth() || key.height() <= key.cutoutHeight())
    {
        vertices.append(QPoint(0, 0));
        vertices.append(QPoint(key.width(), 0));
        vertices.append(QPoint(key.width(), key.height()));
        vertices.append(QPoint(0, key.height()));
        return vertices;
    }
    if (key.cutoutCorner() == 0)
    {
        vertices.append(QPoint(key.cutoutWidth(), 0));
        vertices.append(QPoint(key.width(), 0));
        vertices.append(QPoint(key.width(), key.height()));
        vertices.append(QPoint(0, key.height()));
        vertices.append(QPoint(0, key.cutoutHeight()));
        vertices.append(QPoint(key.cutoutWidth(), key.cutoutHeight()));
    }
    else if (key.cutoutCorner() == 1)
    {
        vertices.append(QPoint(0, 0));
        vertices.append(QPoint(key.width() - key.cutoutWidth(), 0));
        vertices.append(QPoint(key.width() - key.cutoutWidth(), key.cutoutHeight()));
        vertices.append(QPoint(key.width(), key.cutoutHeight()));
        vertices.append(QPoint(key.width(), key.height()));
        vertices.append(QPoint(0, key.height()));
    }
    else if (key.cutoutCorner() == 2)
    {
        vertices.append(QPoint(0, 0));
        vertices.append(QPoint(key.width(), 0));
        vertices.append(QPoint(key.width(), key.cutoutHeight()));
        vertices.append(QPoint(key.width() - key.cutoutWidth(), key.cutoutHeight()));
        vertices.append(QPoint(key.width() - key.cutoutWidth(), key.height()));
        vertices.append(QPoint(0, key.height()));
    }
    else if (key.cutoutCorner() == 3)
    {
        vertices.append(QPoint(0, 0));
        vertices.append(QPoint(key.width(), 0));
        vertices.append(QPoint(key.width(), key.height()));
        vertices.append(QPoint(key.cutoutWidth(), key.height()));
        vertices.append(QPoint(key.cutoutWidth(), key.height() - key.cutoutHeight()));
        vertices.append(QPoint(0, key.height() - key.cutoutHeight()));
    }
    return vertices;
}

/*********************************************************************************************************************************
*   UIKeyboardLayoutReader implementation.                                                                                  *
*********************************************************************************************************************************/

bool UIKeyboardLayoutReader::parseFile(const QString &strFileName)
{
    QFile xmlFile(strFileName);
    if (!xmlFile.exists())
        return false;

    if (!xmlFile.open(QIODevice::ReadOnly))
        return false;

    m_xmlReader.setDevice(&xmlFile);

    if (!m_xmlReader.readNextStartElement() || m_xmlReader.name() != "layout")
        return false;

    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "key")
            parseKey();
        else if (m_xmlReader.name() == "name")
            m_strName = m_xmlReader.readElementText();
        else if (m_xmlReader.name() == "physicallayoutid")
            m_physicalLayoutUid = QUuid(m_xmlReader.readElementText());
        else
            m_xmlReader.skipCurrentElement();
    }
    return true;
}

const QUuid &UIKeyboardLayoutReader::physicalLayoutUUID() const
{
    return m_physicalLayoutUid;
}

const QString &UIKeyboardLayoutReader::name() const
{
    return m_strName;
}

const QMap<int, KeyCaptions> &UIKeyboardLayoutReader::keyCapMap() const
{
    return m_keyCapMap;
}

void  UIKeyboardLayoutReader::parseKey()
{
    KeyCaptions keyCaptions;
    int iKeyPosition = 0;
    while (m_xmlReader.readNextStartElement())
    {
        if (m_xmlReader.name() == "basecaption")
            keyCaptions.m_strBase = m_xmlReader.readElementText();
        else if (m_xmlReader.name() == "shiftcaption")
            keyCaptions.m_strShift = m_xmlReader.readElementText();
        else if (m_xmlReader.name() == "altgrcaption")
            keyCaptions.m_strAltGr = m_xmlReader.readElementText();
        else if (m_xmlReader.name() == "position")
            iKeyPosition = m_xmlReader.readElementText().toInt();
        else
            m_xmlReader.skipCurrentElement();
    }
    m_keyCapMap.insert(iKeyPosition, keyCaptions);
}

/*********************************************************************************************************************************
*   UISoftKeyboard implementation.                                                                                  *
*********************************************************************************************************************************/

UISoftKeyboard::UISoftKeyboard(QWidget *pParent,
                               UISession *pSession, QString strMachineName /* = QString()*/)
    :QIWithRetranslateUI<QMainWindow>(pParent)
    , m_pSession(pSession)
    , m_pMainLayout(0)
    , m_pContainerWidget(0)
    , m_strMachineName(strMachineName)
    , m_pSplitter(0)
    , m_pSettingsButton(0)
    , m_pSidePanelContainerWidget(0)
    , m_pLayoutEditor(0)
{
    setWindowTitle(QString("%1 - %2").arg(m_strMachineName).arg(tr("Soft Keyboard")));
    setAttribute(Qt::WA_DeleteOnClose);
    prepareObjects();
    prepareConnections();

    if (m_pContainerWidget)
        m_pContainerWidget->loadLayouts();

    loadSettings();
    retranslateUi();
    updateStatusBarMessage();
}

UISoftKeyboard::~UISoftKeyboard()
{
    saveSettings();
}

void UISoftKeyboard::retranslateUi()
{
    if (m_pSettingsButton)
        m_pSettingsButton->setToolTip(tr("Settings Menu"));
}

bool UISoftKeyboard::eventFilter(QObject *pWatched, QEvent *pEvent)
{
    if (pWatched != m_pSettingsButton)
        return QIWithRetranslateUI<QMainWindow>::eventFilter(pWatched, pEvent);

    if (pEvent->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *pMouseEvent = dynamic_cast<QMouseEvent*>(pEvent);
        if (pMouseEvent && pMouseEvent->button() == Qt::LeftButton)
        {
            if (m_pContainerWidget)
                m_pContainerWidget->showContextMenu(m_pSettingsButton->mapToGlobal(pMouseEvent->pos()));
            return true;
        }
    }

    return QIWithRetranslateUI<QMainWindow>::eventFilter(pWatched, pEvent);
}

void UISoftKeyboard::sltHandleKeyboardLedsChange()
{
    // bool fNumLockLed = m_pSession->isNumLock();
    // bool fCapsLockLed = m_pSession->isCapsLock();
    // bool fScrollLockLed = m_pSession->isScrollLock();
}

void UISoftKeyboard::sltHandlePutKeyboardSequence(QVector<LONG> sequence)
{
    keyboard().PutScancodes(sequence);
}

void UISoftKeyboard::sltHandleStatusBarContextMenuRequest(const QPoint &point)
{
    if (m_pContainerWidget)
        m_pContainerWidget->showContextMenu(statusBar()->mapToGlobal(point));
}

void UISoftKeyboard::prepareObjects()
{
    m_pSplitter = new QSplitter;
    if (!m_pSplitter)
        return;
    setCentralWidget(m_pSplitter);

    m_pSidePanelContainerWidget = new QStackedWidget;
    m_pSplitter->addWidget(m_pSidePanelContainerWidget);

    m_pLayoutSelector = new UILayoutSelector;
    if (m_pLayoutSelector)
        m_pSidePanelContainerWidget->addWidget(m_pLayoutSelector);

    m_pLayoutEditor = new UILayoutEditor;
    if (m_pLayoutEditor)
        m_pSidePanelContainerWidget->addWidget(m_pLayoutEditor);

    m_pContainerWidget = new UISoftKeyboardWidget;
    if (!m_pContainerWidget)
        return;
    m_pContainerWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    m_pContainerWidget->updateGeometry();
    m_pSplitter->addWidget(m_pContainerWidget);

    statusBar()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(statusBar(), &QStatusBar::customContextMenuRequested,
            this, &UISoftKeyboard::sltHandleStatusBarContextMenuRequest);

    statusBar()->setStyleSheet( "QStatusBar::item { border: 0px}" );

    m_pSettingsButton = new QToolButton;
    m_pSettingsButton->setIcon(UIIconPool::iconSet(":/keyboard_settings_16px.png"));
    m_pSettingsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
    m_pSettingsButton->resize(QSize(iIconMetric, iIconMetric));
    m_pSettingsButton->setStyleSheet("QToolButton { border: 0px none black; margin: 0px 0px 0px 0px; } QToolButton::menu-indicator {image: none;}");
    statusBar()->addPermanentWidget(m_pSettingsButton);

    retranslateUi();
}

void UISoftKeyboard::prepareConnections()
{
    connect(m_pSession, &UISession::sigKeyboardLedsChange, this, &UISoftKeyboard::sltHandleKeyboardLedsChange);
    connect(m_pContainerWidget, &UISoftKeyboardWidget::sigPutKeyboardSequence, this, &UISoftKeyboard::sltHandlePutKeyboardSequence);
}

void UISoftKeyboard::saveSettings()
{
}

void UISoftKeyboard::loadSettings()
{
}

void UISoftKeyboard::updateStatusBarMessage()
{
    QString strMessage;
    if (!m_strLayoutName.isEmpty())
        strMessage += QString("%1: %2").arg(tr("Layout")).arg(m_strLayoutName);
    if (!m_strKeyCapFileName.isEmpty())
        strMessage += QString("\t/\t %1: %2").arg(tr("Key Captions File")).arg(m_strKeyCapFileName);
    statusBar()->showMessage(strMessage);
}

CKeyboard& UISoftKeyboard::keyboard() const
{
    return m_pSession->keyboard();
}

#include "UISoftKeyboard.moc"

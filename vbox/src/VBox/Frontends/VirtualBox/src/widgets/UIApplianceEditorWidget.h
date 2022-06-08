/* $Id: UIApplianceEditorWidget.h 64692 2016-11-17 15:22:44Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIApplianceEditorWidget class declaration.
 */

/*
 * Copyright (C) 2009-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIApplianceEditorWidget_h___
#define ___UIApplianceEditorWidget_h___

/* Qt includes: */
#include <QAbstractItemModel>
#include <QItemDelegate>
#include <QSortFilterProxyModel>
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"

/* COM includes: */
#include "COMEnums.h"
#include "CVirtualSystemDescription.h"

/* Forward declarations: */
class UIApplianceModelItem;
class QCheckBox;
class QLabel;
class QTextEdit;
class QTreeView;


/** Appliance tree-view section types. */
enum ApplianceViewSection
{
    ApplianceViewSection_Description = 0,
    ApplianceViewSection_OriginalValue,
    ApplianceViewSection_ConfigValue
};


/** Appliance model item types. */
enum ApplianceModelItemType
{
    ApplianceModelItemType_Root,
    ApplianceModelItemType_VirtualSystem,
    ApplianceModelItemType_VirtualHardware
};


/** QAbstractItemModel subclass used as Appliance model. */
class UIApplianceModel : public QAbstractItemModel
{
public:

    /** Constructs the Appliance model passing @a pParent to the base-class.
      * @param  aVSDs  Brings the Virtual System descriptions. */
    UIApplianceModel(QVector<CVirtualSystemDescription>& aVSDs, QObject *pParent = 0);
    /** Destructs the Appliance model. */
    ~UIApplianceModel();

    /** Returns the index of the item in the model specified by the given @a iRow, @a iColumn and @a parentIdx. */
    QModelIndex index(int iRow, int iColumn, const QModelIndex &parentIdx = QModelIndex()) const;
    /** Returns the parent of the model item with the given @a idx. */
    QModelIndex parent(const QModelIndex &idx) const;

    /** Returns the number of rows for the children of the given @a parentIdx. */
    int rowCount(const QModelIndex &parentIdx = QModelIndex()) const;
    /** Returns the number of columns for the children of the given @a parentIdx. */
    int columnCount(const QModelIndex &parentIdx = QModelIndex()) const;

    /** Returns the item flags for the given @a idx. */
    Qt::ItemFlags flags(const QModelIndex &idx) const;
    /** Returns the data for the given @a iRole and @a iSection in the header with the specified @a enmOrientation. */
    QVariant headerData(int iSection, Qt::Orientation enmOrientation, int iRole) const;

    /** Defines the @a iRole data for the item at @a idx to @a value. */
    bool setData(const QModelIndex &idx, const QVariant &value, int iRole);
    /** Returns the data stored under the given @a iRole for the item referred to by the @a idx. */
    QVariant data(const QModelIndex &idx, int iRole = Qt::DisplayRole) const;

    /** Returns a model index for the buddy of the item represented by @a idx. */
    QModelIndex buddy(const QModelIndex &idx) const;

    /** Restores the default values for the item with the given @a parentIdx. */
    void restoreDefaults(const QModelIndex &parentIdx = QModelIndex());

    /** Cache currently stored values. */
    void putBack();

private:

    /** Holds the root item reference. */
    UIApplianceModelItem *m_pRootItem;
};


/** QItemDelegate subclass used to create various Appliance model editors. */
class UIApplianceDelegate : public QItemDelegate
{
public:

    /** Constructs the Appliance Delegate passing @a pParent to the base-class.
      * @param  pProxy  Brings the proxy model reference used to redirect requests to. */
    UIApplianceDelegate(QAbstractProxyModel *pProxy, QObject *pParent = 0);

    /** Returns the widget used to edit the item specified by @a idx for editing.
      * @param  pParent      Brings the parent to be assigned for newly created editor.
      * @param  styleOption  Bring the style option set for the newly created editor. */
    QWidget *createEditor(QWidget *pParent, const QStyleOptionViewItem &styleOption, const QModelIndex &idx) const;

    /** Defines the contents of the given @a pEditor to the data for the item at the given @a idx. */
    void setEditorData(QWidget *pEditor, const QModelIndex &idx) const;
    /** Defines the data for the item at the given @a idx in the @a pModel to the contents of the given @a pEditor. */
    void setModelData(QWidget *pEditor, QAbstractItemModel *pModel, const QModelIndex &idx) const;

    /** Updates the geometry of the @a pEditor for the item with the given @a idx, according to the rectangle specified in the @a styleOption. */
    void updateEditorGeometry(QWidget *pEditor, const QStyleOptionViewItem &styleOption, const QModelIndex &idx) const;

    /** Returns the size hint for the item at the given @a idx and specified @a styleOption. */
    QSize sizeHint(const QStyleOptionViewItem &styleOption, const QModelIndex &idx) const;

protected:

#ifdef VBOX_WS_MAC
    /** Filters @a pEvent if this object has been installed as an event filter for the watched @a pObject. */
    bool eventFilter(QObject *pObject, QEvent *pEvent);
#endif

private:

    /** Holds the proxy model reference used to redirect requests to. */
    QAbstractProxyModel *m_pProxy;
};


/** QSortFilterProxyModel subclass used as the Appliance Sorting Proxy model. */
class UIApplianceSortProxyModel : public QSortFilterProxyModel
{
public:

    /** Constructs the Appliance Sorting Proxy model passing @a pParent to the base-class. */
    UIApplianceSortProxyModel(QObject *pParent = 0);

protected:

    /** Returns whether item in the row indicated by the given @a iSourceRow and @a srcParenIdx should be included in the model. */
    bool filterAcceptsRow(int iSourceRow, const QModelIndex &srcParenIdx) const;

    /** Returns whether value of the item referred to by the given index @a leftIdx is less
      * than the value of the item referred to by the given index @a rightIdx. */
    bool lessThan(const QModelIndex &leftIdx, const QModelIndex &rightIdx) const;

    /** Holds the array of sorted Virtual System Description types. */
    static KVirtualSystemDescriptionType s_aSortList[];

    /** Holds the filtered list of Virtual System Description types. */
    QList<KVirtualSystemDescriptionType> m_aFilteredList;
};


/** QWidget subclass used as the Appliance Editor widget. */
class UIApplianceEditorWidget : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs the Appliance Editor widget passing @a pParent to the base-class. */
    UIApplianceEditorWidget(QWidget *pParent = 0);

    /** Returns whether the Appliance Editor has valid state. */
    bool isValid() const { return m_pAppliance != 0; }
    /** Returns the currently set appliance reference. */
    CAppliance *appliance() const { return m_pAppliance; }

    /** Returns the minimum guest RAM. */
    static int minGuestRAM() { return m_minGuestRAM; }
    /** Returns the maximum guest RAM. */
    static int maxGuestRAM() { return m_maxGuestRAM; }
    /** Returns the minimum guest CPU count. */
    static int minGuestCPUCount() { return m_minGuestCPUCount; }
    /** Returns the maximum guest CPU count. */
    static int maxGuestCPUCount() { return m_maxGuestCPUCount; }

public slots:

    /** Restores the default values. */
    void restoreDefaults();

protected:

    /** Handles translation event. */
    virtual void retranslateUi();

    /** Holds the currently set appliance reference. */
    CAppliance         *m_pAppliance;
    /** Holds the Appliance model reference. */
    UIApplianceModel *m_pModel;

    /** Holds the information pane instance. */
    QWidget   *m_pPaneInformation;
    /** Holds the settings tree-view instance. */
    QTreeView *m_pTreeViewSettings;
    /** Holds the 'reinit MACs' check-box instance. */
    QCheckBox *m_pCheckBoxReinitMACs;

    /** Holds the warning pane instance. */
    QWidget   *m_pPaneWarning;
    /** Holds the warning label instance. */
    QLabel    *m_pLabelWarning;
    /** Holds the warning browser instance. */
    QTextEdit *m_pTextEditWarning;

private:

    /** Performs Appliance settings initialization. */
    static void initSystemSettings();

    /** Holds the minimum guest RAM. */
    static int m_minGuestRAM;
    /** Holds the maximum guest RAM. */
    static int m_maxGuestRAM;
    /** Holds the minimum guest CPU count. */
    static int m_minGuestCPUCount;
    /** Holds the maximum guest CPU count. */
    static int m_maxGuestCPUCount;
};

#endif /* !___UIApplianceEditorWidget_h___ */


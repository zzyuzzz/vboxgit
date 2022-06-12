/* $Id: UIGlobalSettingsInput.cpp 85979 2020-09-01 14:11:42Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIGlobalSettingsInput class implementation.
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
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
#include <QAbstractItemDelegate>
#include <QCheckBox>
#include <QGridLayout>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QTabWidget>

/* GUI includes: */
#include "QIStyledItemDelegate.h"
#include "QITableView.h"
#include "QIWidgetValidator.h"
#include "UICommon.h"
#include "UIActionPool.h"
#include "UIGlobalSettingsInput.h"
#include "UIHostComboEditor.h"
#include "UIHotKeyEditor.h"
#include "UIShortcutPool.h"
#include "UIExtraDataManager.h"
#include "UIMessageCenter.h"

/* Qt includes: */
#include <QShortcut>


/* Namespaces: */
using namespace UIExtraDataDefs;


/** Column index enumerator. */
enum UIHotKeyColumnIndex
{
    UIHotKeyColumnIndex_Description,
    UIHotKeyColumnIndex_Sequence,
    UIHotKeyColumnIndex_Max
};


/** Global settings: Input page: Shortcut cell data structure. */
class UIDataShortcutCell : public QITableViewCell
{
    Q_OBJECT;

public:

    /** Constructs table cell.
      * @param  pParent  Brings the row this cell belongs too.
      * @param  strText  Brings the text describing this cell. */
    UIDataShortcutCell(QITableViewRow *pParent, const QString &strText)
        : QITableViewCell(pParent)
        , m_strText(strText)
    {}

    /** Returns the cell text. */
    virtual QString text() const /* override */ { return m_strText; }

private:

    /** Holds the cell text. */
    QString m_strText;
};


/** Global settings: Input page: Shortcut data structure. */
class UIDataShortcutRow : public QITableViewRow
{
    Q_OBJECT;

public:

    /** Constructs table row on the basis of passed arguments.
      * @param  pParent             Brings the row this cell belongs too.
      * @param  strKey              Brings the unique key identifying held sequence.
      * @param  strScope            Brings the scope of the held sequence.
      * @param  strDescription      Brings the deescription for the held sequence.
      * @param  strCurrentSequence  Brings the current held sequence.
      * @param  strDefaultSequence  Brings the default held sequence. */
    UIDataShortcutRow(QITableView *pParent,
                      const QString &strKey,
                      const QString &strScope,
                      const QString &strDescription,
                      const QString &strCurrentSequence,
                      const QString &strDefaultSequence)
        : QITableViewRow(pParent)
        , m_strKey(strKey)
        , m_strScope(strScope)
        , m_strDescription(strDescription)
        , m_strCurrentSequence(strCurrentSequence)
        , m_strDefaultSequence(strDefaultSequence)
    {
        /* Create cells: */
        createCells();
    }

    /** Constructs table row on the basis of @a other one. */
    UIDataShortcutRow(const UIDataShortcutRow &other)
        : QITableViewRow(other.table())
        , m_strKey(other.key())
        , m_strScope(other.scope())
        , m_strDescription(other.description())
        , m_strCurrentSequence(other.currentSequence())
        , m_strDefaultSequence(other.defaultSequence())
    {
        /* Create cells: */
        createCells();
    }

    /** Destructs table row. */
    ~UIDataShortcutRow()
    {
        /* Destroy cells: */
        destroyCells();
    }

    /** Copies a table row from @a other one. */
    UIDataShortcutRow &operator=(const UIDataShortcutRow &other)
    {
        /* Reassign variables: */
        setTable(other.table());
        m_strKey = other.key();
        m_strScope = other.scope();
        m_strDescription = other.description();
        m_strCurrentSequence = other.currentSequence();
        m_strDefaultSequence = other.defaultSequence();

        /* Recreate cells: */
        destroyCells();
        createCells();

        /* Return this: */
        return *this;
    }

    /** Returns whether this row equals to @a other. */
    bool operator==(const UIDataShortcutRow &other) const
    {
        /* Compare by the key and the current sequence: */
        return true
               && (m_strKey == other.key())
               && (m_strScope == other.scope())
               && (m_strCurrentSequence == other.currentSequence())
               ;
    }

    /** Returns the key. */
    QString key() const { return m_strKey; }
    /** Returns the scope. */
    QString scope() const { return m_strScope; }
    /** Returns the description. */
    QString description() const { return m_strDescription; }
    /** Returns the current sequence. */
    QString currentSequence() const { return m_strCurrentSequence; }
    /** Returns the default sequence. */
    QString defaultSequence() const { return m_strDefaultSequence; }

    /** Defines @a strCurrentSequence. */
    void setCurrentSequence(const QString &strCurrentSequence) { m_strCurrentSequence = strCurrentSequence; }

protected:

    /** Returns the number of children. */
    virtual int childCount() const /* override */
    {
        return UIHotKeyColumnIndex_Max;
    }

    /** Returns the child item with @a iIndex. */
    virtual QITableViewCell *childItem(int iIndex) const /* override */
    {
        switch (iIndex)
        {
            case UIHotKeyColumnIndex_Description: return m_cells.first;
            case UIHotKeyColumnIndex_Sequence: return m_cells.second;
            default: break;
        }
        return 0;
    }

private:

    /** Creates cells. */
    void createCells()
    {
        /* Create cells on the basis of description and current sequence: */
        m_cells = qMakePair(new UIDataShortcutCell(this, m_strDescription),
                            new UIDataShortcutCell(this, m_strCurrentSequence));
    }

    /** Destroys cells. */
    void destroyCells()
    {
        /* Destroy cells: */
        delete m_cells.first;
        delete m_cells.second;
        m_cells.first = 0;
        m_cells.second = 0;
    }

    /** Holds the key. */
    QString m_strKey;
    /** Holds the scope. */
    QString m_strScope;
    /** Holds the description. */
    QString m_strDescription;
    /** Holds the current sequence. */
    QString m_strCurrentSequence;
    /** Holds the default sequence. */
    QString m_strDefaultSequence;

    /** Holds the cell instances. */
    QPair<UIDataShortcutCell*, UIDataShortcutCell*> m_cells;
};
typedef QList<UIDataShortcutRow> UIShortcutCache;


/** Global settings: Input page data structure. */
class UIDataSettingsGlobalInput
{
public:

    /** Constructs cache. */
    UIDataSettingsGlobalInput()
        : m_fAutoCapture(false)
    {}

    /** Returns the shortcuts cache [full access]. */
    UIShortcutCache &shortcuts() { return m_shortcuts; }
    /** Returns the shortcuts cache [read-only access]. */
    const UIShortcutCache &shortcuts() const { return m_shortcuts; }

    /** Defines whether the keyboard auto-capture is @a fEnabled. */
    void setAutoCapture(bool fEnabled) { m_fAutoCapture = fEnabled; }
    /** Returns whether the keyboard auto-capture is enabled. */
    bool autoCapture() const { return m_fAutoCapture; }

    /** Returns whether the @a other passed data is equal to this one. */
    bool equal(const UIDataSettingsGlobalInput &other) const
    {
        return (m_shortcuts == other.m_shortcuts) &&
               (m_fAutoCapture == other.m_fAutoCapture);
    }

    /** Returns whether the @a other passed data is equal to this one. */
    bool operator==(const UIDataSettingsGlobalInput &other) const { return equal(other); }
    /** Returns whether the @a other passed data is different from this one. */
    bool operator!=(const UIDataSettingsGlobalInput &other) const { return !equal(other); }

private:

    /** Holds the shortcut cache. */
    UIShortcutCache m_shortcuts;

    /** Holds whether the keyboard auto-capture is enabled. */
    bool m_fAutoCapture;
};


/** Global settings: Input page: Shortcut cache sort functor. */
class UIShortcutCacheItemFunctor
{
public:

    /** Constructs cache sorting functor.
      * @param  iColumn  Brings the column sorting should be done according to.
      * @param  m_order  Brings the sorting order to be applied. */
    UIShortcutCacheItemFunctor(int iColumn, Qt::SortOrder order)
        : m_iColumn(iColumn)
        , m_order(order)
    {}

    /** Returns whether the @a item1 is more/less than the @a item2.
      * @note  Order depends on the one set through constructor, stored in m_order. */
    bool operator()(const UIDataShortcutRow &item1, const UIDataShortcutRow &item2)
    {
        switch (m_iColumn)
        {
            case UIHotKeyColumnIndex_Description:
                return m_order == Qt::AscendingOrder ? item1.description() < item2.description() : item1.description() > item2.description();
            case UIHotKeyColumnIndex_Sequence:
                return m_order == Qt::AscendingOrder ? item1.currentSequence() < item2.currentSequence() : item1.currentSequence() > item2.currentSequence();
            default: break;
        }
        return m_order == Qt::AscendingOrder ? item1.key() < item2.key() : item1.key() > item2.key();
    }

private:

    /** Holds the column sorting should be done according to. */
    int m_iColumn;
    /** Holds the sorting order to be applied. */
    Qt::SortOrder m_order;
};


/** Global settings: Input page: Shortcut search functor. */
class UIFunctorFindShortcut
{
public:

    /** Search match level enumerator. */
    enum UIMatchLevel { Base, Full };

    /** Constructs shortcut search functor.
      * @param  matchLevel  Brings the search match level. */
    UIFunctorFindShortcut(UIMatchLevel enmMatchLevel)
        : m_enmMatchLevel(enmMatchLevel)
    {}

    /** Returns the position of the 1st occurrence of the
      * @a shortcut in the @a shortcuts list, or -1 otherwise. */
    int operator()(const UIShortcutCache &shortcuts, const UIDataShortcutRow &shortcut)
    {
        for (int i = 0; i < shortcuts.size(); ++i)
        {
            const UIDataShortcutRow &iteratedShortcut = shortcuts.at(i);
            switch (m_enmMatchLevel)
            {
                case Base:
                {
                    if (iteratedShortcut.key() == shortcut.key())
                        return i;
                    break;
                }
                case Full:
                {
                    if (   iteratedShortcut.key() == shortcut.key()
                        && iteratedShortcut.currentSequence() == shortcut.currentSequence())
                        return i;
                    break;
                }
            }
        }
        return -1;
    }

private:

    /** Holds the search match level. */
    const UIMatchLevel m_enmMatchLevel;
};


/* A model representing hot-key combination table: */
class UIHotKeyTableModel : public QAbstractTableModel
{
    Q_OBJECT;

signals:

    /* Notifier: Readiness stuff: */
    void sigShortcutsLoaded();

    /* Notifier: Validation stuff: */
    void sigRevalidationRequired();

public:

    /* Constructor: */
    UIHotKeyTableModel(QObject *pParent, UIActionPoolType type);

    /** Returns the number of children. */
    int childCount() const;
    /** Returns the child item with @a iIndex. */
    QITableViewRow *childItem(int iIndex);

    /* API: Loading/saving stuff: */
    void load(const UIShortcutCache &shortcuts);
    void save(UIShortcutCache &shortcuts);

    /* API: Validation stuff: */
    bool isAllShortcutsUnique();

public slots:

    /* Handler: Filtering stuff: */
    void sltHandleFilterTextChange(const QString &strText);

private:

    /* Internal API: Size stuff: */
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

    /* Internal API: Data stuff: */
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const;
    QVariant data(const QModelIndex &index, int iRole = Qt::DisplayRole) const;
    bool setData(const QModelIndex &index, const QVariant &value, int iRole = Qt::EditRole);

    /* Internal API: Sorting stuff: */
    void sort(int iColumn, Qt::SortOrder order = Qt::AscendingOrder);

    /* Helper: Filtering stuff: */
    void applyFilter();

    /* Variables: */
    UIActionPoolType m_type;
    QString m_strFilter;
    UIShortcutCache m_shortcuts;
    UIShortcutCache m_filteredShortcuts;
    QSet<QString> m_duplicatedSequences;
};


/* A table reflecting hot-key combinations: */
class UIHotKeyTable : public QITableView
{
    Q_OBJECT;

public:

    /* Constructor: */
    UIHotKeyTable(QWidget *pParent, UIHotKeyTableModel *pModel, const QString &strObjectName);

protected:

    /** Returns the number of children. */
    virtual int childCount() const /* override */;
    /** Returns the child item with @a iIndex. */
    virtual QITableViewRow *childItem(int iIndex) const /* override */;

private slots:

    /* Handler: Readiness stuff: */
    void sltHandleShortcutsLoaded();

private:

    /** Prepares all. */
    void prepare();
};


/*********************************************************************************************************************************
*   Class UIHotKeyTableModel implementation.                                                                                     *
*********************************************************************************************************************************/

UIHotKeyTableModel::UIHotKeyTableModel(QObject *pParent, UIActionPoolType type)
    : QAbstractTableModel(pParent)
    , m_type(type)
{
}

int UIHotKeyTableModel::childCount() const
{
    /* Return row count: */
    return rowCount();
}

QITableViewRow *UIHotKeyTableModel::childItem(int iIndex)
{
    /* Make sure index within the bounds: */
    AssertReturn(iIndex >= 0 && iIndex < m_filteredShortcuts.size(), 0);
    /* Return corresponding filtered row: */
    return &m_filteredShortcuts[iIndex];
}

void UIHotKeyTableModel::load(const UIShortcutCache &shortcuts)
{
    /* Load shortcuts: */
    foreach (const UIDataShortcutRow &item, shortcuts)
    {
        /* Filter out unnecessary shortcuts: */
        if ((m_type == UIActionPoolType_Manager && item.key().startsWith(GUI_Input_MachineShortcuts)) ||
            (m_type == UIActionPoolType_Runtime && item.key().startsWith(GUI_Input_SelectorShortcuts)))
            continue;
        /* Load shortcut cache item into model: */
        m_shortcuts << item;
    }
    /* Apply filter: */
    applyFilter();
    /* Notify table: */
    emit sigShortcutsLoaded();
}

void UIHotKeyTableModel::save(UIShortcutCache &shortcuts)
{
    /* Save model items: */
    foreach (const UIDataShortcutRow &item, m_shortcuts)
    {
        /* Search for corresponding cache item index: */
        int iIndexOfCacheItem = UIFunctorFindShortcut(UIFunctorFindShortcut::Base)(shortcuts, item);
        /* Make sure index is valid: */
        if (iIndexOfCacheItem == -1)
            continue;
        /* Save model item into the cache: */
        shortcuts[iIndexOfCacheItem] = item;
    }
}

bool UIHotKeyTableModel::isAllShortcutsUnique()
{
    /* Enumerate all the sequences: */
    QMap<QString, QString> usedSequences;
    foreach (const UIDataShortcutRow &item, m_shortcuts)
    {
        QString strKey = item.currentSequence();
        if (!strKey.isEmpty())
        {
            const QString strScope = item.scope();
            strKey = strScope.isNull() ? strKey : QString("%1: %2").arg(strScope, strKey);
            usedSequences.insertMulti(strKey, item.key());
        }
    }
    /* Enumerate all the duplicated sequences: */
    QSet<QString> duplicatedSequences;
    foreach (const QString &strKey, usedSequences.keys())
        if (usedSequences.count(strKey) > 1)
            duplicatedSequences.unite(usedSequences.values(strKey).toSet());
    /* Is there something changed? */
    if (m_duplicatedSequences != duplicatedSequences)
    {
        m_duplicatedSequences = duplicatedSequences;
        emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
    }
    /* Are there duplicated shortcuts? */
    if (!m_duplicatedSequences.isEmpty())
        return false;
    /* True by default: */
    return true;
}

void UIHotKeyTableModel::sltHandleFilterTextChange(const QString &strText)
{
    m_strFilter = strText;
    applyFilter();
}

int UIHotKeyTableModel::rowCount(const QModelIndex& /*parent = QModelIndex()*/) const
{
    return m_filteredShortcuts.size();
}

int UIHotKeyTableModel::columnCount(const QModelIndex& /*parent = QModelIndex()*/) const
{
    return 2;
}

Qt::ItemFlags UIHotKeyTableModel::flags(const QModelIndex &index) const
{
    /* No flags for invalid index: */
    if (!index.isValid()) return Qt::NoItemFlags;
    /* Switch for different columns: */
    switch (index.column())
    {
        case UIHotKeyColumnIndex_Description: return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
        case UIHotKeyColumnIndex_Sequence: return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
        default: break;
    }
    /* No flags by default: */
    return Qt::NoItemFlags;
}

QVariant UIHotKeyTableModel::headerData(int iSection, Qt::Orientation orientation, int iRole /* = Qt::DisplayRole*/) const
{
    /* Switch for different roles: */
    switch (iRole)
    {
        case Qt::DisplayRole:
        {
            /* Invalid for vertical header: */
            if (orientation == Qt::Vertical) return QString();
            /* Switch for different columns: */
            switch (iSection)
            {
                case UIHotKeyColumnIndex_Description: return tr("Name");
                case UIHotKeyColumnIndex_Sequence: return tr("Shortcut");
                default: break;
            }
            /* Invalid for other cases: */
            return QString();
        }
        default: break;
    }
    /* Invalid by default: */
    return QVariant();
}

QVariant UIHotKeyTableModel::data(const QModelIndex &index, int iRole /* = Qt::DisplayRole*/) const
{
    /* No data for invalid index: */
    if (!index.isValid()) return QVariant();
    int iIndex = index.row();
    /* Switch for different roles: */
    switch (iRole)
    {
        case Qt::DisplayRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIHotKeyColumnIndex_Description:
                {
                    /* Return shortcut scope and description: */
                    const QString strScope = m_filteredShortcuts[iIndex].scope();
                    const QString strDescription = m_filteredShortcuts[iIndex].description();
                    return strScope.isNull() ? strDescription : tr("%1: %2", "scope: description").arg(strScope, strDescription);
                }
                case UIHotKeyColumnIndex_Sequence:
                {
                    /* If that is host-combo cell: */
                    if (m_filteredShortcuts[iIndex].key() == UIHostCombo::hostComboCacheKey())
                        /* We should return host-combo: */
                        return UIHostCombo::toReadableString(m_filteredShortcuts[iIndex].currentSequence());
                    /* In other cases we should return hot-combo: */
                    QString strHotCombo = m_filteredShortcuts[iIndex].currentSequence();
                    /* But if that is machine table and hot-combo is not empty: */
                    if (m_type == UIActionPoolType_Runtime && !strHotCombo.isEmpty())
                        /* We should prepend it with Host+ prefix: */
                        strHotCombo.prepend(UIHostCombo::hostComboModifierName());
                    /* Return what we've got: */
                    return strHotCombo;
                }
                default: break;
            }
            /* Invalid for other cases: */
            return QString();
        }
        case Qt::EditRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIHotKeyColumnIndex_Sequence: return m_filteredShortcuts[iIndex].key() == UIHostCombo::hostComboCacheKey() ?
                                                          QVariant::fromValue(UIHostComboWrapper(m_filteredShortcuts[iIndex].currentSequence())) :
                                                          QVariant::fromValue(UIHotKey(m_type == UIActionPoolType_Runtime ?
                                                                                       UIHotKeyType_Simple : UIHotKeyType_WithModifiers,
                                                                                       m_filteredShortcuts[iIndex].currentSequence(),
                                                                                       m_filteredShortcuts[iIndex].defaultSequence()));
                default: break;
            }
            /* Invalid for other cases: */
            return QString();
        }
        case Qt::FontRole:
        {
            /* Do we have a default font? */
            QFont font(QApplication::font());
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIHotKeyColumnIndex_Sequence:
                {
                    if (m_filteredShortcuts[iIndex].key() != UIHostCombo::hostComboCacheKey() &&
                        m_filteredShortcuts[iIndex].currentSequence() != m_filteredShortcuts[iIndex].defaultSequence())
                        font.setBold(true);
                    break;
                }
                default: break;
            }
            /* Return resulting font: */
            return font;
        }
        case Qt::ForegroundRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIHotKeyColumnIndex_Sequence:
                {
                    if (m_duplicatedSequences.contains(m_filteredShortcuts[iIndex].key()))
                        return QBrush(Qt::red);
                    break;
                }
                default: break;
            }
            /* Default for other cases: */
            return QString();
        }
        default: break;
    }
    /* Invalid by default: */
    return QVariant();
}

bool UIHotKeyTableModel::setData(const QModelIndex &index, const QVariant &value, int iRole /* = Qt::EditRole*/)
{
    /* Nothing to set for invalid index: */
    if (!index.isValid()) return false;
    /* Switch for different roles: */
    switch (iRole)
    {
        case Qt::EditRole:
        {
            /* Switch for different columns: */
            switch (index.column())
            {
                case UIHotKeyColumnIndex_Sequence:
                {
                    /* Get index: */
                    int iIndex = index.row();
                    /* Set sequence to shortcut: */
                    UIDataShortcutRow &filteredShortcut = m_filteredShortcuts[iIndex];
                    int iShortcutIndex = UIFunctorFindShortcut(UIFunctorFindShortcut::Base)(m_shortcuts, filteredShortcut);
                    if (iShortcutIndex != -1)
                    {
                        filteredShortcut.setCurrentSequence(filteredShortcut.key() == UIHostCombo::hostComboCacheKey() ?
                                                            value.value<UIHostComboWrapper>().toString() :
                                                            value.value<UIHotKey>().sequence());
                        m_shortcuts[iShortcutIndex] = filteredShortcut;
                        emit sigRevalidationRequired();
                        return true;
                    }
                    break;
                }
                default: break;
            }
            break;
        }
        default: break;
    }
    /* Nothing to set by default: */
    return false;
}

void UIHotKeyTableModel::sort(int iColumn, Qt::SortOrder order /* = Qt::AscendingOrder*/)
{
    /* Sort whole the list: */
    qStableSort(m_shortcuts.begin(), m_shortcuts.end(), UIShortcutCacheItemFunctor(iColumn, order));
    /* Make sure host-combo item is always the first one: */
    UIDataShortcutRow fakeHostComboItem(0, UIHostCombo::hostComboCacheKey(), QString(), QString(), QString(), QString());
    int iIndexOfHostComboItem = UIFunctorFindShortcut(UIFunctorFindShortcut::Base)(m_shortcuts, fakeHostComboItem);
    if (iIndexOfHostComboItem != -1)
    {
        UIDataShortcutRow hostComboItem = m_shortcuts.takeAt(iIndexOfHostComboItem);
        m_shortcuts.prepend(hostComboItem);
    }
    /* Apply the filter: */
    applyFilter();
    /* Notify the model: */
    emit dataChanged(index(0, 0), index(rowCount() - 1, columnCount() - 1));
}

void UIHotKeyTableModel::applyFilter()
{
    /* Erase items first if necessary: */
    if (!m_filteredShortcuts.isEmpty())
    {
        beginRemoveRows(QModelIndex(), 0, m_filteredShortcuts.size() - 1);
        m_filteredShortcuts.clear();
        endRemoveRows();
    }

    /* If filter is empty: */
    if (m_strFilter.isEmpty())
    {
        /* Just add all the items: */
        m_filteredShortcuts = m_shortcuts;
    }
    else
    {
        /* Check if the description matches the filter: */
        foreach (const UIDataShortcutRow &item, m_shortcuts)
        {
            /* If neither scope nor description or sequence matches the filter, skip item: */
            if (!item.scope().contains(m_strFilter, Qt::CaseInsensitive) &&
                !item.description().contains(m_strFilter, Qt::CaseInsensitive) &&
                !item.currentSequence().contains(m_strFilter, Qt::CaseInsensitive))
                continue;
            /* Add that item: */
            m_filteredShortcuts << item;
        }
    }

    /* Add items finally if necessary: */
    if (!m_filteredShortcuts.isEmpty())
    {
        beginInsertRows(QModelIndex(), 0, m_filteredShortcuts.size() - 1);
        endInsertRows();
    }
}


/*********************************************************************************************************************************
*   Class UIHotKeyTable implementation.                                                                                          *
*********************************************************************************************************************************/

UIHotKeyTable::UIHotKeyTable(QWidget *pParent, UIHotKeyTableModel *pModel, const QString &strObjectName)
    : QITableView(pParent)
{
    /* Set object name: */
    setObjectName(strObjectName);
    /* Set model: */
    setModel(pModel);

    /* Prepare all: */
    prepare();
}

int UIHotKeyTable::childCount() const
{
    /* Redirect request to table model: */
    return qobject_cast<UIHotKeyTableModel*>(model())->childCount();
}

QITableViewRow *UIHotKeyTable::childItem(int iIndex) const
{
    /* Redirect request to table model: */
    return qobject_cast<UIHotKeyTableModel*>(model())->childItem(iIndex);
}

void UIHotKeyTable::sltHandleShortcutsLoaded()
{
    /* Resize columns to feat contents: */
    resizeColumnsToContents();

    /* Configure sorting: */
    sortByColumn(UIHotKeyColumnIndex_Description, Qt::AscendingOrder);
    setSortingEnabled(true);
}

void UIHotKeyTable::prepare()
{
    /* Configure self: */
    setTabKeyNavigation(false);
    setContextMenuPolicy(Qt::CustomContextMenu);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::CurrentChanged | QAbstractItemView::SelectedClicked);

    /* Configure headers: */
    verticalHeader()->hide();
    verticalHeader()->setDefaultSectionSize((int)(verticalHeader()->minimumSectionSize() * 1.33));
    horizontalHeader()->setStretchLastSection(false);
    horizontalHeader()->setSectionResizeMode(UIHotKeyColumnIndex_Description, QHeaderView::Interactive);
    horizontalHeader()->setSectionResizeMode(UIHotKeyColumnIndex_Sequence, QHeaderView::Stretch);

    /* Connect model: */
    UIHotKeyTableModel *pHotKeyTableModel = qobject_cast<UIHotKeyTableModel*>(model());
    AssertPtrReturnVoid(pHotKeyTableModel);
    {
        connect(pHotKeyTableModel, &UIHotKeyTableModel::sigShortcutsLoaded,
                this, &UIHotKeyTable::sltHandleShortcutsLoaded);
    }

    /* Check if we do have proper item delegate: */
    QIStyledItemDelegate *pStyledItemDelegate = qobject_cast<QIStyledItemDelegate*>(itemDelegate());
    AssertPtrReturnVoid(pStyledItemDelegate);
    {
        /* Configure item delegate: */
        pStyledItemDelegate->setWatchForEditorDataCommits(true);

        /* Create new item editor factory: */
        QItemEditorFactory *pNewItemEditorFactory = new QItemEditorFactory;
        AssertPtrReturnVoid(pNewItemEditorFactory);
        {
            /* Register UIHotKeyEditor as the UIHotKey editor: */
            int iHotKeyTypeId = qRegisterMetaType<UIHotKey>();
            QStandardItemEditorCreator<UIHotKeyEditor> *pHotKeyItemEditorCreator = new QStandardItemEditorCreator<UIHotKeyEditor>();
            pNewItemEditorFactory->registerEditor((QVariant::Type)iHotKeyTypeId, pHotKeyItemEditorCreator);

            /* Register UIHostComboEditor as the UIHostComboWrapper editor: */
            int iHostComboTypeId = qRegisterMetaType<UIHostComboWrapper>();
            QStandardItemEditorCreator<UIHostComboEditor> *pHostComboItemEditorCreator = new QStandardItemEditorCreator<UIHostComboEditor>();
            pNewItemEditorFactory->registerEditor((QVariant::Type)iHostComboTypeId, pHostComboItemEditorCreator);

            /* Assign configured item editor factory to item delegate: */
            pStyledItemDelegate->setItemEditorFactory(pNewItemEditorFactory);
        }
    }
}


/*********************************************************************************************************************************
*   Class UIGlobalSettingsInput implementation.                                                                                  *
*********************************************************************************************************************************/

UIGlobalSettingsInput::UIGlobalSettingsInput()
    : m_pModelSelector(0)
    , m_pModelMachine(0)
    , m_pCache(0)
    , m_pTabWidget(0)
    , m_pEditorSelectorFilter(0), m_pTableSelector(0)
    , m_pMachineFilterEditor(0), m_pTableMachine(0)
    , m_pCheckBoxEnableAutoGrab(0)
{
    /* Prepare: */
    prepare();
}

UIGlobalSettingsInput::~UIGlobalSettingsInput()
{
    /* Cleanup: */
    cleanup();
}

void UIGlobalSettingsInput::loadToCacheFrom(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Clear cache initially: */
    m_pCache->clear();

    /* Prepare old input data: */
    UIDataSettingsGlobalInput oldInputData;

    /* Gather old input data: */
    oldInputData.shortcuts() << UIDataShortcutRow(m_pTableMachine,
                                                  UIHostCombo::hostComboCacheKey(),
                                                  QString(),
                                                  tr("Host Key Combination"),
                                                  gEDataManager->hostKeyCombination(),
                                                  QString());
    const QMap<QString, UIShortcut> &shortcuts = gShortcutPool->shortcuts();
    const QList<QString> shortcutKeys = shortcuts.keys();
    foreach (const QString &strShortcutKey, shortcutKeys)
    {
        const UIShortcut &shortcut = shortcuts[strShortcutKey];
        QITableView *pParent = strShortcutKey.startsWith(GUI_Input_MachineShortcuts) ? m_pTableMachine :
                               strShortcutKey.startsWith(GUI_Input_SelectorShortcuts) ? m_pTableSelector : 0;
        AssertPtr(pParent);
        oldInputData.shortcuts() << UIDataShortcutRow(pParent,
                                                      strShortcutKey,
                                                      shortcut.scope(),
                                                      UICommon::removeAccelMark(shortcut.description()),
                                                      shortcut.primaryToNativeText(),
                                                      shortcut.defaultSequence().toString(QKeySequence::NativeText));
    }
    oldInputData.setAutoCapture(gEDataManager->autoCaptureEnabled());

    /* Cache old input data: */
    m_pCache->cacheInitialData(oldInputData);

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

void UIGlobalSettingsInput::getFromCache()
{
    /* Get old input data from the cache: */
    const UIDataSettingsGlobalInput &oldInputData = m_pCache->base();

    /* Load old input data from the cache: */
    m_pModelSelector->load(oldInputData.shortcuts());
    m_pModelMachine->load(oldInputData.shortcuts());
    m_pCheckBoxEnableAutoGrab->setChecked(oldInputData.autoCapture());

    /* Revalidate: */
    revalidate();
}

void UIGlobalSettingsInput::putToCache()
{
    /* Prepare new input data: */
    UIDataSettingsGlobalInput newInputData = m_pCache->base();

    /* Gather new input data: */
    m_pModelSelector->save(newInputData.shortcuts());
    m_pModelMachine->save(newInputData.shortcuts());
    newInputData.setAutoCapture(m_pCheckBoxEnableAutoGrab->isChecked());

    /* Cache new input data: */
    m_pCache->cacheCurrentData(newInputData);
}

void UIGlobalSettingsInput::saveFromCacheTo(QVariant &data)
{
    /* Fetch data to properties: */
    UISettingsPageGlobal::fetchData(data);

    /* Update input data and failing state: */
    setFailed(!saveInputData());

    /* Upload properties to data: */
    UISettingsPageGlobal::uploadData(data);
}

bool UIGlobalSettingsInput::validate(QList<UIValidationMessage> &messages)
{
    /* Pass by default: */
    bool fPass = true;

    /* Check VirtualBox Manager page for unique shortcuts: */
    if (!m_pModelSelector->isAllShortcutsUnique())
    {
        UIValidationMessage message;
        message.first = UICommon::removeAccelMark(m_pTabWidget->tabText(UIHotKeyTableIndex_Selector));
        message.second << tr("Some items have the same shortcuts assigned.");
        messages << message;
        fPass = false;
    }

    /* Check Virtual Machine page for unique shortcuts: */
    if (!m_pModelMachine->isAllShortcutsUnique())
    {
        UIValidationMessage message;
        message.first = UICommon::removeAccelMark(m_pTabWidget->tabText(UIHotKeyTableIndex_Machine));
        message.second << tr("Some items have the same shortcuts assigned.");
        messages << message;
        fPass = false;
    }

    /* Return result: */
    return fPass;
}

void UIGlobalSettingsInput::setOrderAfter(QWidget *pWidget)
{
    setTabOrder(pWidget, m_pTabWidget);
    setTabOrder(m_pTabWidget, m_pEditorSelectorFilter);
    setTabOrder(m_pEditorSelectorFilter, m_pTableSelector);
    setTabOrder(m_pTableSelector, m_pMachineFilterEditor);
    setTabOrder(m_pMachineFilterEditor, m_pTableMachine);
    setTabOrder(m_pTableMachine, m_pCheckBoxEnableAutoGrab);
}

void UIGlobalSettingsInput::retranslateUi()
{
    m_pCheckBoxEnableAutoGrab->setWhatsThis(tr("When checked, the keyboard is automatically captured every time the VM window "
                                               "is activated. When the keyboard is captured, all keystrokes (including system ones "
                                               "like Alt-Tab) are directed to the VM."));
    m_pCheckBoxEnableAutoGrab->setText(tr("&Auto Capture Keyboard"));


    /* Translate tab-widget labels: */
    m_pTabWidget->setTabText(UIHotKeyTableIndex_Selector, tr("&VirtualBox Manager"));
    m_pTabWidget->setTabText(UIHotKeyTableIndex_Machine, tr("Virtual &Machine"));
    m_pTableSelector->setWhatsThis(tr("Lists all available shortcuts which can be configured."));
    m_pTableMachine->setWhatsThis(tr("Lists all available shortcuts which can be configured."));
    m_pEditorSelectorFilter->setWhatsThis(tr("Holds a sequence to filter the shortcut list."));
    m_pMachineFilterEditor->setWhatsThis(tr("Holds a sequence to filter the shortcut list."));
}

void UIGlobalSettingsInput::prepare()
{
    /* Prepare cache: */
    m_pCache = new UISettingsCacheGlobalInput;
    AssertPtrReturnVoid(m_pCache);

    /* Prepare everything: */
    prepareWidgets();
    prepareConnections();

    /* Apply language settings: */
    retranslateUi();
}

void UIGlobalSettingsInput::prepareWidgets()
{
    /* Prepare main layout: */
    QGridLayout *pMainLayout = new QGridLayout(this);
    if (pMainLayout)
    {
        /* Prepare tab-widget: */
        m_pTabWidget = new QTabWidget(this);
        if (m_pTabWidget)
        {
            m_pTabWidget->setMinimumWidth(400);

            /* Prepare 'Selector UI' tab: */
            prepareTabSelector();
            /* Prepare 'Runtime UI' tab: */
            prepareTabMachine();

            /* Add tab-widget into layout: */
            pMainLayout->addWidget(m_pTabWidget, 0, 0, 1, 2);
        }

        /* Prepare enable auto-grab check-box: */
        m_pCheckBoxEnableAutoGrab = new QCheckBox(this);
        if (m_pCheckBoxEnableAutoGrab)
            pMainLayout->addWidget(m_pCheckBoxEnableAutoGrab, 1, 0);
    }
}

void UIGlobalSettingsInput::prepareTabSelector()
{
    /* Prepare Selector UI tab: */
    QWidget *pSelectorTab = new QWidget;
    if (pSelectorTab)
    {
        /* Prepare Selector UI layout: */
        QVBoxLayout *pSelectorLayout = new QVBoxLayout(pSelectorTab);
        if (pSelectorLayout)
        {
            pSelectorLayout->setSpacing(1);
#ifdef VBOX_WS_MAC
            /* On Mac OS X and X11 we can do a bit of smoothness: */
            pSelectorLayout->setContentsMargins(0, 0, 0, 0);
#endif

            /* Prepare Selector UI filter editor: */
            m_pEditorSelectorFilter = new QLineEdit(pSelectorTab);
            if (m_pEditorSelectorFilter)
                pSelectorLayout->addWidget(m_pEditorSelectorFilter);

            /* Prepare Selector UI model: */
            m_pModelSelector = new UIHotKeyTableModel(this, UIActionPoolType_Manager);

            /* Prepare Selector UI table: */
            m_pTableSelector = new UIHotKeyTable(pSelectorTab, m_pModelSelector, "m_pTableSelector");
            if (m_pTableSelector)
                pSelectorLayout->addWidget(m_pTableSelector);
        }

        m_pTabWidget->insertTab(UIHotKeyTableIndex_Selector, pSelectorTab, QString());
    }
}

void UIGlobalSettingsInput::prepareTabMachine()
{
    /* Create Runtime UI tab: */
    QWidget *pMachineTab = new QWidget;
    if (pMachineTab)
    {
        /* Prepare Runtime UI layout: */
        QVBoxLayout *pMachineLayout = new QVBoxLayout(pMachineTab);
        if (pMachineLayout)
        {
            pMachineLayout->setSpacing(1);
#ifdef VBOX_WS_MAC
            /* On Mac OS X and X11 we can do a bit of smoothness. */
            pMachineLayout->setContentsMargins(0, 0, 0, 0);
#endif

            /* Prepare Runtime UI filter editor: */
            m_pMachineFilterEditor = new QLineEdit(pMachineTab);
            if (m_pMachineFilterEditor)
                pMachineLayout->addWidget(m_pMachineFilterEditor);

            /* Prepare Runtime UI model: */
            m_pModelMachine = new UIHotKeyTableModel(this, UIActionPoolType_Runtime);

            /* Create Runtime UI table: */
            m_pTableMachine = new UIHotKeyTable(pMachineTab, m_pModelMachine, "m_pTableMachine");
            if (m_pTableMachine)
                pMachineLayout->addWidget(m_pTableMachine);
        }

        m_pTabWidget->insertTab(UIHotKeyTableIndex_Machine, pMachineTab, QString());

        /* In the VM process we start by displaying the Runtime UI tab: */
        if (uiCommon().uiType() == UICommon::UIType_RuntimeUI)
            m_pTabWidget->setCurrentWidget(pMachineTab);
    }
}

void UIGlobalSettingsInput::prepareConnections()
{
    /* Configure 'Selector UI' connections: */
    connect(m_pEditorSelectorFilter, &QLineEdit::textChanged,
            m_pModelSelector, &UIHotKeyTableModel::sltHandleFilterTextChange);
    connect(m_pModelSelector, &UIHotKeyTableModel::sigRevalidationRequired, this, &UIGlobalSettingsInput::revalidate);

    /* Configure 'Runtime UI' connections: */
    connect(m_pMachineFilterEditor, &QLineEdit::textChanged,
            m_pModelMachine, &UIHotKeyTableModel::sltHandleFilterTextChange);
    connect(m_pModelMachine, &UIHotKeyTableModel::sigRevalidationRequired, this, &UIGlobalSettingsInput::revalidate);
}

void UIGlobalSettingsInput::cleanup()
{
    /* Cleanup cache: */
    delete m_pCache;
    m_pCache = 0;
}

bool UIGlobalSettingsInput::saveInputData()
{
    /* Prepare result: */
    bool fSuccess = true;
    /* Save input settings from the cache: */
    if (fSuccess && m_pCache->wasChanged())
    {
        /* Get old input data from the cache: */
        const UIDataSettingsGlobalInput &oldInputData = m_pCache->base();
        /* Get new input data from the cache: */
        const UIDataSettingsGlobalInput &newInputData = m_pCache->data();

        /* Save new host-combo shortcut from the cache: */
        const UIDataShortcutRow fakeHostComboItem(0, UIHostCombo::hostComboCacheKey(), QString(), QString(), QString(), QString());
        const int iHostComboItemBase = UIFunctorFindShortcut(UIFunctorFindShortcut::Base)(oldInputData.shortcuts(), fakeHostComboItem);
        const int iHostComboItemData = UIFunctorFindShortcut(UIFunctorFindShortcut::Base)(newInputData.shortcuts(), fakeHostComboItem);
        const QString strHostComboBase = iHostComboItemBase != -1 ? oldInputData.shortcuts().at(iHostComboItemBase).currentSequence() : QString();
        const QString strHostComboData = iHostComboItemData != -1 ? newInputData.shortcuts().at(iHostComboItemData).currentSequence() : QString();
        if (strHostComboData != strHostComboBase)
            gEDataManager->setHostKeyCombination(strHostComboData);

        /* Save other new shortcuts from the cache: */
        QMap<QString, QString> sequencesBase;
        QMap<QString, QString> sequencesData;
        foreach (const UIDataShortcutRow &item, oldInputData.shortcuts())
            sequencesBase.insert(item.key(), item.currentSequence());
        foreach (const UIDataShortcutRow &item, newInputData.shortcuts())
            sequencesData.insert(item.key(), item.currentSequence());
        if (sequencesData != sequencesBase)
            gShortcutPool->setOverrides(sequencesData);

        /* Save other new things from the cache: */
        if (newInputData.autoCapture() != oldInputData.autoCapture())
            gEDataManager->setAutoCaptureEnabled(newInputData.autoCapture());
    }
    /* Return result: */
    return fSuccess;
}

# include "UIGlobalSettingsInput.moc"

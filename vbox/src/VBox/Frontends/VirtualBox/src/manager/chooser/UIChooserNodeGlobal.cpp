/* $Id: UIChooserNodeGlobal.cpp 83315 2020-03-18 12:29:49Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIChooserNodeGlobal class implementation.
 */

/*
 * Copyright (C) 2012-2020 Oracle Corporation
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
#include "UIChooserNodeGlobal.h"

/* Other VBox includes: */
#include "iprt/assert.h"


UIChooserNodeGlobal::UIChooserNodeGlobal(UIChooserNode *pParent,
                                         bool fFavorite,
                                         int iPosition,
                                         const QString &)
    : UIChooserNode(pParent, fFavorite)
{
    /* Add to parent: */
    if (parentNode())
        parentNode()->addNode(this, iPosition);

    /* Apply language settings: */
    retranslateUi();
}

UIChooserNodeGlobal::UIChooserNodeGlobal(UIChooserNode *pParent,
                                         UIChooserNodeGlobal *pCopyFrom,
                                         int iPosition)
    : UIChooserNode(pParent, pCopyFrom->isFavorite())
{
    /* Add to parent: */
    if (parentNode())
        parentNode()->addNode(this, iPosition);

    /* Apply language settings: */
    retranslateUi();
}

UIChooserNodeGlobal::~UIChooserNodeGlobal()
{
    /* Delete item: */
    delete item();

    /* Remove from parent: */
    if (parentNode())
        parentNode()->removeNode(this);
}

QString UIChooserNodeGlobal::name() const
{
    return m_strName;
}

QString UIChooserNodeGlobal::fullName() const
{
    return name();
}

QString UIChooserNodeGlobal::description() const
{
    return name();
}

QString UIChooserNodeGlobal::definition() const
{
    return QString("n=%1").arg("GLOBAL");
}

bool UIChooserNodeGlobal::hasNodes(UIChooserItemType enmType /* = UIChooserItemType_Any */) const
{
    Q_UNUSED(enmType);
    AssertFailedReturn(false);
}

QList<UIChooserNode*> UIChooserNodeGlobal::nodes(UIChooserItemType enmType /* = UIChooserItemType_Any */) const
{
    Q_UNUSED(enmType);
    AssertFailedReturn(QList<UIChooserNode*>());
}

void UIChooserNodeGlobal::addNode(UIChooserNode *pNode, int iPosition)
{
    Q_UNUSED(pNode);
    Q_UNUSED(iPosition);
    AssertFailedReturnVoid();
}

void UIChooserNodeGlobal::removeNode(UIChooserNode *pNode)
{
    Q_UNUSED(pNode);
    AssertFailedReturnVoid();
}

void UIChooserNodeGlobal::removeAllNodes(const QUuid &)
{
    // Nothing to remove for global-node..
}

void UIChooserNodeGlobal::updateAllNodes(const QUuid &)
{
    // Nothing to update for global-node..

    /* Update global-item: */
    item()->updateItem();
}

bool UIChooserNodeGlobal::hasAtLeastOneCloudNode() const
{
    return false;
}

int UIChooserNodeGlobal::positionOf(UIChooserNode *pNode)
{
    Q_UNUSED(pNode);
    AssertFailedReturn(0);
}

void UIChooserNodeGlobal::searchForNodes(const QString &strSearchTerm, int iItemSearchFlags, QList<UIChooserNode*> &matchedItems)
{
    if (!(iItemSearchFlags & UIChooserItemSearchFlag_Global))
        return;

    if (strSearchTerm.isEmpty())
    {
        matchedItems << this;
        return;
    }

    if (iItemSearchFlags & UIChooserItemSearchFlag_ExactName)
    {
        if (name() == strSearchTerm)
            matchedItems << this;
    }
    else
    {
        if (name().contains(strSearchTerm, Qt::CaseInsensitive))
            matchedItems << this;
    }
}

void UIChooserNodeGlobal::sortNodes()
{
    AssertFailedReturnVoid();
}

void UIChooserNodeGlobal::retranslateUi()
{
    /* Translate name: */
    m_strName = tr("Tools");

    /* Update global-item: */
    if (item())
        item()->updateItem();
}

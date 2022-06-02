/* $Id: UISettingsPage.cpp 33656 2010-11-01 14:18:11Z vboxsync $ */
/** @file
 *
 * VBox frontends: Qt4 GUI ("VirtualBox"):
 * UISettingsPage class implementation
 */

/*
 * Copyright (C) 2006-2010 Oracle Corporation
 *
 * Oracle Corporation confidential
 * All rights reserved
 */

/* Local includes */
#include "UISettingsPage.h"

/* Returns settings page type: */
UISettingsPageType UISettingsPage::type() const
{
    return m_type;
}

/* Validation stuff: */
void UISettingsPage::setValidator(QIWidgetValidator *pValidator)
{
    Q_UNUSED(pValidator);
}

/* Validation stuff: */
bool UISettingsPage::revalidate(QString &strWarningText, QString &strTitle)
{
    Q_UNUSED(strWarningText);
    Q_UNUSED(strTitle);
    return true;
}

/* Navigation stuff: */
void UISettingsPage::setOrderAfter(QWidget *pWidget)
{
    m_pFirstWidget = pWidget;
}

/* Page 'ID' stuff: */
int UISettingsPage::id() const
{
    return m_cId;
}

/* Page 'ID' stuff: */
void UISettingsPage::setId(int cId)
{
    m_cId = cId;
}

/* Page 'processed' stuff: */
bool UISettingsPage::processed() const
{
    return m_fProcessed;
}

/* Page 'processed' stuff: */
void UISettingsPage::setProcessed(bool fProcessed)
{
    m_fProcessed = fProcessed;
}

/* Settings page constructor, hidden: */
UISettingsPage::UISettingsPage(UISettingsPageType type, QWidget *pParent)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_type(type)
    , m_cId(-1)
    , m_fProcessed(false)
    , m_pFirstWidget(0)
{
}

/* Fetch data to m_properties & m_settings: */
void UISettingsPageGlobal::fetchData(const QVariant &data)
{
    m_properties = data.value<UISettingsDataGlobal>().m_properties;
    m_settings = data.value<UISettingsDataGlobal>().m_settings;
}

/* Upload m_properties & m_settings to data: */
void UISettingsPageGlobal::uploadData(QVariant &data) const
{
    data = QVariant::fromValue(UISettingsDataGlobal(m_properties, m_settings));
}

/* Global settings page constructor, hidden: */
UISettingsPageGlobal::UISettingsPageGlobal(QWidget *pParent)
    : UISettingsPage(UISettingsPageType_Global, pParent)
{
}

/* Fetch data to m_machine: */
void UISettingsPageMachine::fetchData(const QVariant &data)
{
    m_machine = data.value<UISettingsDataMachine>().m_machine;
}

/* Upload m_machine to data: */
void UISettingsPageMachine::uploadData(QVariant &data) const
{
    data = QVariant::fromValue(UISettingsDataMachine(m_machine));
}

/* Machine settings page constructor, hidden: */
UISettingsPageMachine::UISettingsPageMachine(QWidget *pParent)
    : UISettingsPage(UISettingsPageType_Machine, pParent)
{
}


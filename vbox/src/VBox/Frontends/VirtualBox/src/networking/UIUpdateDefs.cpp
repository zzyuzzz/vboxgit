/* $Id: UIUpdateDefs.cpp 94750 2022-04-28 19:11:26Z vboxsync $ */
/** @file
 * VBox Qt GUI - Update routine related implementations.
 */

/*
 * Copyright (C) 2006-2022 Oracle Corporation
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
#include <QCoreApplication>
#include <QLocale>
#include <QStringList>

/* GUI includes: */
#include "UICommon.h"
#include "UIUpdateDefs.h"


/* static: */
VBoxUpdateDayList VBoxUpdateData::s_days = VBoxUpdateDayList();

/* static */
void VBoxUpdateData::populate()
{
    /* Clear list initially: */
    s_days.clear();

    // WORKAROUND:
    // To avoid re-translation complexity
    // all values will be retranslated separately.

    /* Separately retranslate each day: */
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 day"),   "1 d");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "2 days"),  "2 d");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "3 days"),  "3 d");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "4 days"),  "4 d");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "5 days"),  "5 d");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "6 days"),  "6 d");

    /* Separately retranslate each week: */
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 week"),  "1 w");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "2 weeks"), "2 w");
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "3 weeks"), "3 w");

    /* Separately retranslate each month: */
    s_days << VBoxUpdateDay(QCoreApplication::translate("UIUpdateManager", "1 month"), "1 m");
}

/* static */
QStringList VBoxUpdateData::list()
{
    QStringList result;
    foreach (const VBoxUpdateDay &day, s_days)
        result << day.val;
    return result;
}

VBoxUpdateData::VBoxUpdateData(const QString &strData)
    : m_strData(strData)
    , m_fCheckEnabled(false)
    , m_fCheckRequired(false)
    , m_enmUpdatePeriod(UpdatePeriodType_Never)
    , m_enmUpdateChannel(KUpdateChannel_Invalid)
{
    /* Skip 'never' case: */
    if (m_strData == "never")
        return;

    /* Check is enabled in all cases besides 'never': */
    m_fCheckEnabled = true;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QStringList parser = m_strData.split(", ", Qt::SkipEmptyParts);
#else
    const QStringList parser = m_strData.split(", ", QString::SkipEmptyParts);
#endif

    /* Parse 'period' value: */
    if (parser.size() > 0)
    {
        if (s_days.isEmpty())
            populate();
        m_enmUpdatePeriod = (UpdatePeriodType)s_days.indexOf(VBoxUpdateDay(QString(), parser.at(0)));
        if (m_enmUpdatePeriod == UpdatePeriodType_Never)
            m_enmUpdatePeriod = UpdatePeriodType_1Day;
    }

    /* Parse 'date' value: */
    if (parser.size() > 1)
    {
        QDate date = QDate::fromString(parser.at(1), Qt::ISODate);
        m_date = date.isValid() ? date : QDate::currentDate();
    }

    /* Parse 'update channel' value: */
    if (parser.size() > 2)
    {
        m_enmUpdateChannel = updateChannelFromInternalString(parser.at(2));
    }

    /* Parse 'version' value: */
    if (parser.size() > 3)
    {
        m_version = UIVersion(parser.at(3));
    }

    /* Decide whether we need to check: */
    m_fCheckRequired =    (QDate::currentDate() >= date())
                       && (   !version().isValid()
                           || version() != UIVersion(uiCommon().vboxVersionStringNormalized()));
}

VBoxUpdateData::VBoxUpdateData(bool fCheckEnabled, UpdatePeriodType enmUpdatePeriod, KUpdateChannel enmUpdateChannel)
    : m_strData("never")
    , m_fCheckEnabled(fCheckEnabled)
    , m_fCheckRequired(false)
    , m_enmUpdatePeriod(enmUpdatePeriod)
    , m_enmUpdateChannel(enmUpdateChannel)
{
    /* Skip 'check disabled' case: */
    if (!m_fCheckEnabled)
        return;

    /* Encode 'period' value: */
    if (s_days.isEmpty())
        populate();
    const QString strRemindPeriod = s_days.at(m_enmUpdatePeriod).key;

    /* Encode 'date' value: */
    m_date = QDate::currentDate();
    QStringList parser(strRemindPeriod.split(' '));
    if (parser[1] == "d")
        m_date = m_date.addDays(parser[0].toInt());
    else if (parser[1] == "w")
        m_date = m_date.addDays(parser[0].toInt() * 7);
    else if (parser[1] == "m")
        m_date = m_date.addDays(parser[0].toInt() * 30);
    const QString strRemindDate = m_date.toString(Qt::ISODate);

    /* Encode 'update channel' value: */
    const QString strUpdateChannel = updateChannelName();

    /* Encode 'version' value: */
    m_version = UIVersion(uiCommon().vboxVersionStringNormalized());
    const QString strVersionValue = m_version.toString();

    /* Compose m_strData: */
    m_strData = QString("%1, %2, %3, %4").arg(strRemindPeriod, strRemindDate, strUpdateChannel, strVersionValue);

    /* Decide whether we need to check: */
    m_fCheckRequired =    (QDate::currentDate() >= date())
                       && (   !version().isValid()
                           || version() != UIVersion(uiCommon().vboxVersionStringNormalized()));
}

bool VBoxUpdateData::isCheckEnabled() const
{
    return m_fCheckEnabled;
}

bool VBoxUpdateData::isCheckRequired() const
{
    return m_fCheckRequired;
}

QString VBoxUpdateData::data() const
{
    return m_strData;
}

UpdatePeriodType VBoxUpdateData::updatePeriod() const
{
    return m_enmUpdatePeriod;
}

QDate VBoxUpdateData::date() const
{
    return m_date;
}

QString VBoxUpdateData::dateToString() const
{
    return   isCheckEnabled()
           ? QLocale::system().toString(m_date, QLocale::ShortFormat)
           : QCoreApplication::translate("UIUpdateManager", "Never");
}

KUpdateChannel VBoxUpdateData::updateChannel() const
{
    return m_enmUpdateChannel;
}

QString VBoxUpdateData::updateChannelName() const
{
    return updateChannelToInternalString(m_enmUpdateChannel);
}

UIVersion VBoxUpdateData::version() const
{
    return m_version;
}

bool VBoxUpdateData::isEqual(const VBoxUpdateData &another) const
{
    return    true
           && (m_strData == another.data())
           && (m_enmUpdatePeriod == another.updatePeriod())
           && (m_date == another.date())
           && (m_enmUpdateChannel == another.updateChannel())
           && (m_version == another.version())
              ;
}

bool VBoxUpdateData::operator==(const VBoxUpdateData &another) const
{
    return isEqual(another);
}

bool VBoxUpdateData::operator!=(const VBoxUpdateData &another) const
{
    return !isEqual(another);
}

/* static */
QString VBoxUpdateData::updateChannelToInternalString(KUpdateChannel enmUpdateChannel)
{
    switch (enmUpdateChannel)
    {
        case KUpdateChannel_WithTesting: return "withtesting";
        case KUpdateChannel_WithBetas: return "withbetas";
        case KUpdateChannel_All: return "allrelease";
        default: return "stable";
    }
}

/* static */
KUpdateChannel VBoxUpdateData::updateChannelFromInternalString(const QString &strUpdateChannel)
{
    QMap<QString, KUpdateChannel> pairs;
    pairs["withtesting"] = KUpdateChannel_WithTesting;
    pairs["withbetas"] = KUpdateChannel_WithBetas;
    pairs["allrelease"] = KUpdateChannel_All;
    return pairs.value(strUpdateChannel, KUpdateChannel_Stable);
}

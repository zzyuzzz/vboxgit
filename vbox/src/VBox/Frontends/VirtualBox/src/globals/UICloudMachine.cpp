/* $Id: UICloudMachine.cpp 83160 2020-02-26 12:40:06Z vboxsync $ */
/** @file
 * VBox Qt GUI - UICloudMachine class implementation.
 */

/*
 * Copyright (C) 2020 Oracle Corporation
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
#include <QMap>

/* GUI includes: */
#include "UICloudMachine.h"
#include "UICloudNetworkingStuff.h"


/*********************************************************************************************************************************
*   Class UICloudMachineData implementation.                                                                                     *
*********************************************************************************************************************************/

UICloudMachineData::UICloudMachineData(const CCloudClient &comCloudClient,
                                       const QString &strId,
                                       const QString &strName)
    : m_comCloudClient(comCloudClient)
    , m_strId(strId)
    , m_strName(strName)
    , m_fDataActual(false)
    , m_fAccessible(true)
    , m_strOsType("Other")
    , m_iMemorySize(0)
    , m_iCpuCount(0)
{
    //printf("Data for machine with id = {%s} is created\n", m_strId.toUtf8().constData());
}

UICloudMachineData::UICloudMachineData(const UICloudMachineData &other)
    : QSharedData(other)
    , m_comCloudClient(other.m_comCloudClient)
    , m_strId(other.m_strId)
    , m_strName(other.m_strName)
    , m_fDataActual(other.m_fDataActual)
    , m_fAccessible(other.m_fAccessible)
    , m_strOsType(other.m_strOsType)
    , m_iMemorySize(other.m_iMemorySize)
    , m_iCpuCount(other.m_iCpuCount)
{
    //printf("Data for machine with id = {%s} is copied\n", m_strId.toUtf8().constData());
}

UICloudMachineData::~UICloudMachineData()
{
    //printf("Data for machine with id = {%s} is deleted\n", m_strId.toUtf8().constData());
}

CCloudClient UICloudMachineData::cloudClient() const
{
    return m_comCloudClient;
}

QString UICloudMachineData::id() const
{
    return m_strId;
}

QString UICloudMachineData::name() const
{
    return m_strName;
}

bool UICloudMachineData::isAccessible() const
{
    return m_fAccessible;
}

QString UICloudMachineData::osType()
{
    if (!m_fDataActual)
        refresh();
    return m_strOsType;
}

int UICloudMachineData::memorySize()
{
    if (!m_fDataActual)
        refresh();
    return m_iMemorySize;
}

int UICloudMachineData::cpuCount()
{
    if (!m_fDataActual)
        refresh();
    return m_iCpuCount;
}

void UICloudMachineData::refresh()
{
    /* Acquire instance info sync way, be aware, this is blocking stuff, it takes some time: */
    const QMap<KVirtualSystemDescriptionType, QString> infoMap = getInstanceInfo(m_comCloudClient, m_strId);

    /* Refresh corresponding values: */
    m_strOsType = fetchOsType(infoMap);
    m_iMemorySize = fetchMemorySize(infoMap);
    m_iCpuCount = fetchCpuCount(infoMap);

    /* Mark data actual: */
    m_fDataActual = true;
}


/*********************************************************************************************************************************
*   Class UICloudMachine implementation.                                                                                         *
*********************************************************************************************************************************/

UICloudMachine::UICloudMachine()
{
}

UICloudMachine::UICloudMachine(const CCloudClient &comCloudClient,
                               const QString &strId,
                               const QString &strName)
    : d(new UICloudMachineData(comCloudClient, strId, strName))
{
}

UICloudMachine::UICloudMachine(const UICloudMachine &other)
    : d(other.d)
{
}

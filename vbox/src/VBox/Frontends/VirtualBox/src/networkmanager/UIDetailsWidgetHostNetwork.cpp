/* $Id: UIDetailsWidgetHostNetwork.cpp 87268 2021-01-15 12:50:01Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIDetailsWidgetHostNetwork class implementation.
 */

/*
 * Copyright (C) 2009-2021 Oracle Corporation
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
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QRegExpValidator>
#include <QStyleOption>
#include <QVBoxLayout>

/* GUI includes: */
#include "QIDialogButtonBox.h"
#include "QILineEdit.h"
#include "QITabWidget.h"
#include "UIIconPool.h"
#include "UIDetailsWidgetHostNetwork.h"
#include "UINetworkManagerUtils.h"

/* Other VBox includes: */
#include "iprt/assert.h"
#include "iprt/cidr.h"


UIDetailsWidgetHostNetwork::UIDetailsWidgetHostNetwork(EmbedTo enmEmbedding, QWidget *pParent /* = 0 */)
    : QIWithRetranslateUI<QWidget>(pParent)
    , m_enmEmbedding(enmEmbedding)
    , m_pTabWidget(0)
    , m_pButtonAutomatic(0), m_pErrorPaneAutomatic(0)
    , m_pButtonManual(0), m_pErrorPaneManual(0)
    , m_pLabelIPv4(0), m_pEditorIPv4(0), m_pErrorPaneIPv4(0)
    , m_pLabelNMv4(0), m_pEditorNMv4(0), m_pErrorPaneNMv4(0)
    , m_pLabelIPv6(0), m_pEditorIPv6(0), m_pErrorPaneIPv6(0)
    , m_pLabelNMv6(0), m_pEditorNMv6(0), m_pErrorPaneNMv6(0)
    , m_pButtonBoxInterface(0)
    , m_pCheckBoxDHCP(0)
    , m_pLabelDHCPAddress(0), m_pEditorDHCPAddress(0), m_pErrorPaneDHCPAddress(0)
    , m_pLabelDHCPMask(0), m_pEditorDHCPMask(0), m_pErrorPaneDHCPMask(0)
    , m_pLabelDHCPLowerAddress(0), m_pEditorDHCPLowerAddress(0), m_pErrorPaneDHCPLowerAddress(0)
    , m_pLabelDHCPUpperAddress(0), m_pEditorDHCPUpperAddress(0), m_pErrorPaneDHCPUpperAddress(0)
    , m_pButtonBoxServer(0)
{
    /* Prepare: */
    prepare();
}

void UIDetailsWidgetHostNetwork::setData(const UIDataHostNetwork &data)
{
    /* Cache old/new data: */
    m_oldData = data;
    m_newData = m_oldData;

    /* Load 'Interface' data: */
    loadDataForInterface();
    /* Load 'DHCP server' data: */
    loadDataForDHCPServer();
}

void UIDetailsWidgetHostNetwork::retranslateUi()
{
    /* Translate tab-widget: */
    if (m_pTabWidget)
    {
        m_pTabWidget->setTabText(0, tr("&Adapter"));
        m_pTabWidget->setTabText(1, tr("&DHCP Server"));
    }

    /* Translate 'Interface' tab content: */
    if (m_pButtonAutomatic)
        m_pButtonAutomatic->setText(tr("Configure Adapter &Automatically"));
    if (m_pButtonManual)
        m_pButtonManual->setText(tr("Configure Adapter &Manually"));
    if (m_pLabelIPv4)
        m_pLabelIPv4->setText(tr("&IPv4 Address:"));
    if (m_pEditorIPv4)
        m_pEditorIPv4->setToolTip(tr("Holds the host IPv4 address for this adapter."));
    if (m_pLabelNMv4)
        m_pLabelNMv4->setText(tr("IPv4 Network &Mask:"));
    if (m_pEditorNMv4)
        m_pEditorNMv4->setToolTip(tr("Holds the host IPv4 network mask for this adapter."));
    if (m_pLabelIPv6)
        m_pLabelIPv6->setText(tr("I&Pv6 Address:"));
    if (m_pEditorIPv6)
        m_pEditorIPv6->setToolTip(tr("Holds the host IPv6 address for this adapter if IPv6 is supported."));
    if (m_pLabelNMv6)
        m_pLabelNMv6->setText(tr("IPv6 Prefix &Length:"));
    if (m_pEditorNMv6)
        m_pEditorNMv6->setToolTip(tr("Holds the host IPv6 prefix length for this adapter if IPv6 is supported."));
    if (m_pButtonBoxInterface)
    {
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setText(tr("Reset"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setText(tr("Apply"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setShortcut(Qt::Key_Escape);
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setShortcut(QString("Ctrl+Return"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setStatusTip(tr("Reset changes in current interface details"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setStatusTip(tr("Apply changes in current interface details"));
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->
            setToolTip(tr("Reset Changes (%1)").arg(m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->shortcut().toString()));
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->
            setToolTip(tr("Apply Changes (%1)").arg(m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->shortcut().toString()));
    }

    /* Translate 'DHCP server' tab content: */
    if (m_pCheckBoxDHCP)
    {
        m_pCheckBoxDHCP->setText(tr("&Enable Server"));
        m_pCheckBoxDHCP->setToolTip(tr("When checked, the DHCP Server will be enabled for this network on machine start-up."));
    }
    if (m_pLabelDHCPAddress)
        m_pLabelDHCPAddress->setText(tr("Server Add&ress:"));
    if (m_pEditorDHCPAddress)
        m_pEditorDHCPAddress->setToolTip(tr("Holds the address of the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pLabelDHCPMask)
        m_pLabelDHCPMask->setText(tr("Server &Mask:"));
    if (m_pEditorDHCPMask)
        m_pEditorDHCPMask->setToolTip(tr("Holds the network mask of the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pLabelDHCPLowerAddress)
        m_pLabelDHCPLowerAddress->setText(tr("&Lower Address Bound:"));
    if (m_pEditorDHCPLowerAddress)
        m_pEditorDHCPLowerAddress->setToolTip(tr("Holds the lower address bound offered by the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pLabelDHCPUpperAddress)
        m_pLabelDHCPUpperAddress->setText(tr("&Upper Address Bound:"));
    if (m_pEditorDHCPUpperAddress)
        m_pEditorDHCPUpperAddress->setToolTip(tr("Holds the upper address bound offered by the DHCP server servicing the network associated with this host-only adapter."));
    if (m_pButtonBoxServer)
    {
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setText(tr("Reset"));
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setText(tr("Apply"));
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setShortcut(Qt::Key_Escape);
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setShortcut(QString("Ctrl+Return"));
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setStatusTip(tr("Reset changes in current DHCP server details"));
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setStatusTip(tr("Apply changes in current DHCP server details"));
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->
            setToolTip(tr("Reset Changes (%1)").arg(m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->shortcut().toString()));
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->
            setToolTip(tr("Apply Changes (%1)").arg(m_pButtonBoxServer->button(QDialogButtonBox::Ok)->shortcut().toString()));
    }

    /* Retranslate validation: */
    retranslateValidation();
}

void UIDetailsWidgetHostNetwork::sltToggledButtonAutomatic(bool fChecked)
{
    m_newData.m_interface.m_fDHCPEnabled = fChecked;
    loadDataForInterface();
    revalidate();
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltToggledButtonManual(bool fChecked)
{
    m_newData.m_interface.m_fDHCPEnabled = !fChecked;
    loadDataForInterface();
    revalidate();
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedIPv4(const QString &strText)
{
    m_newData.m_interface.m_strAddress = strText;
    revalidate(m_pErrorPaneIPv4);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedNMv4(const QString &strText)
{
    m_newData.m_interface.m_strMask = strText;
    revalidate(m_pErrorPaneNMv4);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedIPv6(const QString &strText)
{
    m_newData.m_interface.m_strAddress6 = strText;
    revalidate(m_pErrorPaneIPv6);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedNMv6(const QString &strText)
{
    m_newData.m_interface.m_strPrefixLength6 = strText;
    revalidate(m_pErrorPaneNMv6);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltStatusChangedServer(int iChecked)
{
    m_newData.m_dhcpserver.m_fEnabled = (bool)iChecked;
    loadDataForDHCPServer();
    revalidate();
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedAddress(const QString &strText)
{
    m_newData.m_dhcpserver.m_strAddress = strText;
    revalidate(m_pErrorPaneDHCPAddress);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedMask(const QString &strText)
{
    m_newData.m_dhcpserver.m_strMask = strText;
    revalidate(m_pErrorPaneDHCPMask);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedLowerAddress(const QString &strText)
{
    m_newData.m_dhcpserver.m_strLowerAddress = strText;
    revalidate(m_pErrorPaneDHCPLowerAddress);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltTextChangedUpperAddress(const QString &strText)
{
    m_newData.m_dhcpserver.m_strUpperAddress = strText;
    revalidate(m_pErrorPaneDHCPUpperAddress);
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::sltHandleButtonBoxClick(QAbstractButton *pButton)
{
    /* Make sure button-boxes exist: */
    if (!m_pButtonBoxInterface || !m_pButtonBoxServer)
        return;

    /* Disable buttons first of all: */
    m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setEnabled(false);
    m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setEnabled(false);
    m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setEnabled(false);
    m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setEnabled(false);

    /* Compare with known buttons: */
    if (   pButton == m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)
        || pButton == m_pButtonBoxServer->button(QDialogButtonBox::Cancel))
        emit sigDataChangeRejected();
    else
    if (   pButton == m_pButtonBoxInterface->button(QDialogButtonBox::Ok)
        || pButton == m_pButtonBoxServer->button(QDialogButtonBox::Ok))
        emit sigDataChangeAccepted();
}

void UIDetailsWidgetHostNetwork::prepare()
{
    /* Prepare this: */
    prepareThis();

    /* Apply language settings: */
    retranslateUi();

    /* Update button states finally: */
    updateButtonStates();
}

void UIDetailsWidgetHostNetwork::prepareThis()
{
    /* Create layout: */
    QVBoxLayout *pLayout = new QVBoxLayout(this);
    if (pLayout)
    {
        /* Configure layout: */
        pLayout->setContentsMargins(0, 0, 0, 0);

        /* Prepare tab-widget: */
        prepareTabWidget();
    }
}

void UIDetailsWidgetHostNetwork::prepareTabWidget()
{
    /* Create tab-widget: */
    m_pTabWidget = new QITabWidget;
    if (m_pTabWidget)
    {
        /* Prepare 'Interface' tab: */
        prepareTabInterface();
        /* Prepare 'DHCP server' tab: */
        prepareTabDHCPServer();

        /* Add into layout: */
        layout()->addWidget(m_pTabWidget);
    }
}

void UIDetailsWidgetHostNetwork::prepareTabInterface()
{
    /* Create 'Interface' tab: */
    QWidget *pTabInterface = new QWidget;
    if (pTabInterface)
    {
        /* Create 'Interface' layout: */
        QGridLayout *pLayoutInterface = new QGridLayout(pTabInterface);
        if (pLayoutInterface)
        {
#ifdef VBOX_WS_MAC
            /* Configure layout: */
            pLayoutInterface->setSpacing(10);
            pLayoutInterface->setContentsMargins(10, 10, 10, 10);
#endif

            /* Get the required icon metric: */
            const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

            /* Create automatic interface configuration layout: */
            QHBoxLayout *pLayoutAutomatic = new QHBoxLayout;
            if (pLayoutAutomatic)
            {
                /* Configure layout: */
                pLayoutAutomatic->setContentsMargins(0, 0, 0, 0);

                /* Create automatic interface configuration radio-button: */
                m_pButtonAutomatic = new QRadioButton;
                if (m_pButtonAutomatic)
                {
                    /* Configure radio-button: */
                    connect(m_pButtonAutomatic, &QRadioButton::toggled,
                            this, &UIDetailsWidgetHostNetwork::sltToggledButtonAutomatic);
                    /* Add into layout: */
                    pLayoutAutomatic->addWidget(m_pButtonAutomatic);
                }
                /* Create automatic interface configuration error pane: */
                m_pErrorPaneAutomatic = new QLabel;
                if (m_pErrorPaneAutomatic)
                {
                    /* Configure label: */
                    m_pErrorPaneAutomatic->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    m_pErrorPaneAutomatic->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
                    m_pErrorPaneAutomatic->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                     .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutAutomatic->addWidget(m_pErrorPaneAutomatic);
                }
                /* Add into layout: */
                pLayoutInterface->addLayout(pLayoutAutomatic, 0, 0, 1, 3);
#ifdef VBOX_WS_MAC
                pLayoutInterface->setRowMinimumHeight(0, 22);
#endif
            }

            /* Create manual interface configuration layout: */
            QHBoxLayout *pLayoutManual = new QHBoxLayout;
            if (pLayoutManual)
            {
                /* Configure layout: */
                pLayoutManual->setContentsMargins(0, 0, 0, 0);
                /* Create manual interface configuration radio-button: */
                m_pButtonManual = new QRadioButton;
                if (m_pButtonManual)
                {
                    /* Configure radio-button: */
                    connect(m_pButtonManual, &QRadioButton::toggled,
                            this, &UIDetailsWidgetHostNetwork::sltToggledButtonManual);
                    /* Add into layout: */
                    pLayoutManual->addWidget(m_pButtonManual);
                }
                /* Create manual interface configuration error pane: */
                m_pErrorPaneManual = new QLabel;
                if (m_pErrorPaneManual)
                {
                    /* Configure label: */
                    m_pErrorPaneManual->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                    m_pErrorPaneManual->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
                    m_pErrorPaneManual->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                  .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutManual->addWidget(m_pErrorPaneManual);
                }
                /* Add into layout: */
                pLayoutInterface->addLayout(pLayoutManual, 1, 0, 1, 3);
#ifdef VBOX_WS_MAC
                pLayoutInterface->setRowMinimumHeight(1, 22);
#endif
            }

            /* Create IPv4 address label: */
            m_pLabelIPv4 = new QLabel;
            if (m_pLabelIPv4)
            {
                /* Configure label: */
                m_pLabelIPv4->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutInterface->addWidget(m_pLabelIPv4, 2, 1);
            }
            /* Create IPv4 layout: */
            QHBoxLayout *pLayoutIPv4 = new QHBoxLayout;
            if (pLayoutIPv4)
            {
                /* Configure layout: */
                pLayoutIPv4->setContentsMargins(0, 0, 0, 0);
                /* Create IPv4 address editor: */
                m_pEditorIPv4 = new QILineEdit;
                if (m_pEditorIPv4)
                {
                    /* Configure editor: */
                    m_pLabelIPv4->setBuddy(m_pEditorIPv4);
                    connect(m_pEditorIPv4, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedIPv4);
                    /* Add into layout: */
                    pLayoutIPv4->addWidget(m_pEditorIPv4);
                }
                /* Create IPv4 error pane: */
                m_pErrorPaneIPv4 = new QLabel;
                if (m_pErrorPaneIPv4)
                {
                    /* Configure label: */
                    m_pErrorPaneIPv4->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneIPv4->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutIPv4->addWidget(m_pErrorPaneIPv4);
                }
                /* Add into layout: */
                pLayoutInterface->addLayout(pLayoutIPv4, 2, 2);
            }

            /* Create NMv4 network mask label: */
            m_pLabelNMv4 = new QLabel;
            if (m_pLabelNMv4)
            {
                /* Configure label: */
                m_pLabelNMv4->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutInterface->addWidget(m_pLabelNMv4, 3, 1);
            }
            /* Create NMv4 layout: */
            QHBoxLayout *pLayoutNMv4 = new QHBoxLayout;
            if (pLayoutNMv4)
            {
                /* Configure layout: */
                pLayoutNMv4->setContentsMargins(0, 0, 0, 0);
                /* Create NMv4 network mask editor: */
                m_pEditorNMv4 = new QILineEdit;
                if (m_pEditorNMv4)
                {
                    /* Configure editor: */
                    m_pLabelNMv4->setBuddy(m_pEditorNMv4);
                    connect(m_pEditorNMv4, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedNMv4);
                    /* Add into layout: */
                    pLayoutNMv4->addWidget(m_pEditorNMv4);
                }
                /* Create NMv4 error pane: */
                m_pErrorPaneNMv4 = new QLabel;
                if (m_pErrorPaneNMv4)
                {
                    /* Configure label: */
                    m_pErrorPaneNMv4->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneNMv4->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutNMv4->addWidget(m_pErrorPaneNMv4);
                }
                /* Add into layout: */
                pLayoutInterface->addLayout(pLayoutNMv4, 3, 2);
            }

            /* Create IPv6 address label: */
            m_pLabelIPv6 = new QLabel;
            if (m_pLabelIPv6)
            {
                /* Configure label: */
                m_pLabelIPv6->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutInterface->addWidget(m_pLabelIPv6, 4, 1);
            }
            /* Create IPv6 layout: */
            QHBoxLayout *pLayoutIPv6 = new QHBoxLayout;
            if (pLayoutIPv6)
            {
                /* Configure layout: */
                pLayoutIPv6->setContentsMargins(0, 0, 0, 0);
                /* Create IPv6 address editor: */
                m_pEditorIPv6 = new QILineEdit;
                if (m_pEditorIPv6)
                {
                    /* Configure editor: */
                    m_pLabelIPv6->setBuddy(m_pEditorIPv6);
                    connect(m_pEditorIPv6, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedIPv6);
                    /* Add into layout: */
                    pLayoutIPv6->addWidget(m_pEditorIPv6);
                }
                /* Create IPv4 error pane: */
                m_pErrorPaneIPv6 = new QLabel;
                if (m_pErrorPaneIPv6)
                {
                    /* Configure label: */
                    m_pErrorPaneIPv6->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneIPv6->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutIPv6->addWidget(m_pErrorPaneIPv6);
                }
                /* Add into layout: */
                pLayoutInterface->addLayout(pLayoutIPv6, 4, 2);
            }

            /* Create NMv6 network mask label: */
            m_pLabelNMv6 = new QLabel;
            if (m_pLabelNMv6)
            {
                /* Configure label: */
                m_pLabelNMv6->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutInterface->addWidget(m_pLabelNMv6, 5, 1);
            }
            /* Create NMv6 layout: */
            QHBoxLayout *pLayoutNMv6 = new QHBoxLayout;
            if (pLayoutNMv6)
            {
                /* Configure layout: */
                pLayoutNMv6->setContentsMargins(0, 0, 0, 0);
                /* Create NMv6 network mask editor: */
                m_pEditorNMv6 = new QILineEdit;
                if (m_pEditorNMv6)
                {
                    /* Configure editor: */
                    m_pLabelNMv6->setBuddy(m_pEditorNMv6);
                    connect(m_pEditorNMv6, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedNMv6);
                    /* Add into layout: */
                    pLayoutNMv6->addWidget(m_pEditorNMv6);
                }
                /* Create NMv6 error pane: */
                m_pErrorPaneNMv6 = new QLabel;
                if (m_pErrorPaneNMv6)
                {
                    /* Configure label: */
                    m_pErrorPaneNMv6->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneNMv6->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutNMv6->addWidget(m_pErrorPaneNMv6);
                }
                /* Add into layout: */
                pLayoutInterface->addLayout(pLayoutNMv6, 5, 2);
            }

            /* Create indent: */
            QStyleOption options;
            options.initFrom(m_pButtonManual);
            const int iWidth = m_pButtonManual->style()->pixelMetric(QStyle::PM_ExclusiveIndicatorWidth, &options, m_pButtonManual) +
                               m_pButtonManual->style()->pixelMetric(QStyle::PM_RadioButtonLabelSpacing, &options, m_pButtonManual) -
                               pLayoutInterface->spacing() - 1;
            QSpacerItem *pSpacer1 = new QSpacerItem(iWidth, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
            if (pSpacer1)
            {
                /* Add into layout: */
                pLayoutInterface->addItem(pSpacer1, 2, 0, 4);
            }
            /* Create stretch: */
            QSpacerItem *pSpacer2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            if (pSpacer2)
            {
                /* Add into layout: */
                pLayoutInterface->addItem(pSpacer2, 6, 0, 1, 3);
            }

            /* If parent embedded into stack: */
            if (m_enmEmbedding == EmbedTo_Stack)
            {
                /* Create button-box: */
                m_pButtonBoxInterface = new QIDialogButtonBox;
                if (m_pButtonBoxInterface)
                {
                    /* Configure button-box: */
                    m_pButtonBoxInterface->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                    connect(m_pButtonBoxInterface, &QIDialogButtonBox::clicked, this, &UIDetailsWidgetHostNetwork::sltHandleButtonBoxClick);

                    /* Add into layout: */
                    pLayoutInterface->addWidget(m_pButtonBoxInterface, 7, 0, 1, 3);
                }
            }
        }
        /* Add to tab-widget: */
        m_pTabWidget->addTab(pTabInterface, QString());
    }
}

void UIDetailsWidgetHostNetwork::prepareTabDHCPServer()
{
    /* Create 'DHCP server' tab: */
    QWidget *pTabDHCPServer = new QWidget;
    if (pTabDHCPServer)
    {
        /* Create 'DHCP server' layout: */
        QGridLayout *pLayoutDHCPServer = new QGridLayout(pTabDHCPServer);
        if (pLayoutDHCPServer)
        {
#ifdef VBOX_WS_MAC
            /* Configure layout: */
            pLayoutDHCPServer->setSpacing(10);
            pLayoutDHCPServer->setContentsMargins(10, 10, 10, 10);
#endif

            /* Get the required icon metric: */
            const int iIconMetric = QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

            /* Create DHCP server status check-box: */
            m_pCheckBoxDHCP = new QCheckBox;
            if (m_pCheckBoxDHCP)
            {
                /* Configure check-box: */
                connect(m_pCheckBoxDHCP, &QCheckBox::stateChanged,
                        this, &UIDetailsWidgetHostNetwork::sltStatusChangedServer);
                /* Add into layout: */
                pLayoutDHCPServer->addWidget(m_pCheckBoxDHCP, 0, 0, 1, 2);
#ifdef VBOX_WS_MAC
                pLayoutDHCPServer->setRowMinimumHeight(0, 22);
#endif
            }

            /* Create DHCP address label: */
            m_pLabelDHCPAddress = new QLabel;
            if (m_pLabelDHCPAddress)
            {
                /* Configure label: */
                m_pLabelDHCPAddress->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutDHCPServer->addWidget(m_pLabelDHCPAddress, 1, 1);
            }
            /* Create DHCP address layout: */
            QHBoxLayout *pLayoutDHCPAddress = new QHBoxLayout;
            if (pLayoutDHCPAddress)
            {
                /* Configure layout: */
                pLayoutDHCPAddress->setContentsMargins(0, 0, 0, 0);
                /* Create DHCP address editor: */
                m_pEditorDHCPAddress = new QILineEdit;
                if (m_pEditorDHCPAddress)
                {
                    /* Configure editor: */
                    m_pLabelDHCPAddress->setBuddy(m_pEditorDHCPAddress);
                    connect(m_pEditorDHCPAddress, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedAddress);
                    /* Add into layout: */
                    pLayoutDHCPAddress->addWidget(m_pEditorDHCPAddress);
                }
                /* Create DHCP address error pane: */
                m_pErrorPaneDHCPAddress = new QLabel;
                if (m_pErrorPaneDHCPAddress)
                {
                    /* Configure label: */
                    m_pErrorPaneDHCPAddress->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneDHCPAddress->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                       .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutDHCPAddress->addWidget(m_pErrorPaneDHCPAddress);
                }
                /* Add into layout: */
                pLayoutDHCPServer->addLayout(pLayoutDHCPAddress, 1, 2);
            }

            /* Create DHCP network mask label: */
            m_pLabelDHCPMask = new QLabel;
            if (m_pLabelDHCPMask)
            {
                /* Configure label: */
                m_pLabelDHCPMask->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutDHCPServer->addWidget(m_pLabelDHCPMask, 2, 1);
            }
            /* Create DHCP mask layout: */
            QHBoxLayout *pLayoutDHCPMask = new QHBoxLayout;
            if (pLayoutDHCPMask)
            {
                /* Configure layout: */
                pLayoutDHCPMask->setContentsMargins(0, 0, 0, 0);
                /* Create DHCP network mask editor: */
                m_pEditorDHCPMask = new QILineEdit;
                if (m_pEditorDHCPMask)
                {
                    /* Configure editor: */
                    m_pLabelDHCPMask->setBuddy(m_pEditorDHCPMask);
                    connect(m_pEditorDHCPMask, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedMask);
                    /* Add into layout: */
                    pLayoutDHCPMask->addWidget(m_pEditorDHCPMask);
                }
                /* Create DHCP mask error pane: */
                m_pErrorPaneDHCPMask = new QLabel;
                if (m_pErrorPaneDHCPMask)
                {
                    /* Configure label: */
                    m_pErrorPaneDHCPMask->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneDHCPMask->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                    .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutDHCPMask->addWidget(m_pErrorPaneDHCPMask);
                }
                /* Add into layout: */
                pLayoutDHCPServer->addLayout(pLayoutDHCPMask, 2, 2);
            }

            /* Create DHCP lower address label: */
            m_pLabelDHCPLowerAddress = new QLabel;
            if (m_pLabelDHCPLowerAddress)
            {
                /* Configure label: */
                m_pLabelDHCPLowerAddress->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutDHCPServer->addWidget(m_pLabelDHCPLowerAddress, 3, 1);
            }
            /* Create DHCP lower address layout: */
            QHBoxLayout *pLayoutDHCPLowerAddress = new QHBoxLayout;
            if (pLayoutDHCPLowerAddress)
            {
                /* Configure layout: */
                pLayoutDHCPLowerAddress->setContentsMargins(0, 0, 0, 0);
                /* Create DHCP lower address editor: */
                m_pEditorDHCPLowerAddress = new QILineEdit;
                if (m_pEditorDHCPLowerAddress)
                {
                    /* Configure editor: */
                    m_pLabelDHCPLowerAddress->setBuddy(m_pEditorDHCPLowerAddress);
                    connect(m_pEditorDHCPLowerAddress, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedLowerAddress);
                    /* Add into layout: */
                    pLayoutDHCPLowerAddress->addWidget(m_pEditorDHCPLowerAddress);
                }
                /* Create DHCP lower address error pane: */
                m_pErrorPaneDHCPLowerAddress = new QLabel;
                if (m_pErrorPaneDHCPLowerAddress)
                {
                    /* Configure label: */
                    m_pErrorPaneDHCPLowerAddress->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneDHCPLowerAddress->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                            .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutDHCPLowerAddress->addWidget(m_pErrorPaneDHCPLowerAddress);
                }
                /* Add into layout: */
                pLayoutDHCPServer->addLayout(pLayoutDHCPLowerAddress, 3, 2);
            }

            /* Create DHCP upper address label: */
            m_pLabelDHCPUpperAddress = new QLabel;
            if (m_pLabelDHCPUpperAddress)
            {
                /* Configure label: */
                m_pLabelDHCPUpperAddress->setAlignment(Qt::AlignRight | Qt::AlignTrailing | Qt::AlignVCenter);
                /* Add into layout: */
                pLayoutDHCPServer->addWidget(m_pLabelDHCPUpperAddress, 4, 1);
            }
            /* Create DHCP upper address layout: */
            QHBoxLayout *pLayoutDHCPUpperAddress = new QHBoxLayout;
            if (pLayoutDHCPUpperAddress)
            {
                /* Configure layout: */
                pLayoutDHCPUpperAddress->setContentsMargins(0, 0, 0, 0);
                /* Create DHCP upper address editor: */
                m_pEditorDHCPUpperAddress = new QILineEdit;
                if (m_pEditorDHCPUpperAddress)
                {
                    /* Configure editor: */
                    m_pLabelDHCPUpperAddress->setBuddy(m_pEditorDHCPUpperAddress);
                    connect(m_pEditorDHCPUpperAddress, &QLineEdit::textChanged,
                            this, &UIDetailsWidgetHostNetwork::sltTextChangedUpperAddress);
                    /* Add into layout: */
                    pLayoutDHCPUpperAddress->addWidget(m_pEditorDHCPUpperAddress);
                }
                /* Create DHCP upper address error pane: */
                m_pErrorPaneDHCPUpperAddress = new QLabel;
                if (m_pErrorPaneDHCPUpperAddress)
                {
                    /* Configure label: */
                    m_pErrorPaneDHCPUpperAddress->setAlignment(Qt::AlignCenter);
                    m_pErrorPaneDHCPUpperAddress->setPixmap(UIIconPool::iconSet(":/status_error_16px.png")
                                                            .pixmap(QSize(iIconMetric, iIconMetric)));
                    /* Add into layout: */
                    pLayoutDHCPUpperAddress->addWidget(m_pErrorPaneDHCPUpperAddress);
                }
                /* Add into layout: */
                pLayoutDHCPServer->addLayout(pLayoutDHCPUpperAddress, 4, 2);
            }

            /* Create indent: */
            QStyleOption options;
            options.initFrom(m_pCheckBoxDHCP);
            const int iWidth = m_pCheckBoxDHCP->style()->pixelMetric(QStyle::PM_IndicatorWidth, &options, m_pCheckBoxDHCP) +
                               m_pCheckBoxDHCP->style()->pixelMetric(QStyle::PM_CheckBoxLabelSpacing, &options, m_pCheckBoxDHCP) -
                               pLayoutDHCPServer->spacing() - 1;
            QSpacerItem *pSpacer1 = new QSpacerItem(iWidth, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
            if (pSpacer1)
            {
                /* Add into layout: */
                pLayoutDHCPServer->addItem(pSpacer1, 1, 0, 4);
            }
            /* Create stretch: */
            QSpacerItem *pSpacer2 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            if (pSpacer2)
            {
                /* Add into layout: */
                pLayoutDHCPServer->addItem(pSpacer2, 5, 0, 1, 3);
            }

            /* If parent embedded into stack: */
            if (m_enmEmbedding == EmbedTo_Stack)
            {
                /* Create button-box: */
                m_pButtonBoxServer = new QIDialogButtonBox;
                if (m_pButtonBoxServer)
                {
                    /* Configure button-box: */
                    m_pButtonBoxServer->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
                    connect(m_pButtonBoxServer, &QIDialogButtonBox::clicked, this, &UIDetailsWidgetHostNetwork::sltHandleButtonBoxClick);

                    /* Add into layout: */
                    pLayoutDHCPServer->addWidget(m_pButtonBoxServer, 6, 0, 1, 3);
                }
            }
        }
        /* Add to tab-widget: */
        m_pTabWidget->addTab(pTabDHCPServer, QString());
    }
}

void UIDetailsWidgetHostNetwork::loadDataForInterface()
{
    /* Toggle IPv4 & IPv6 interface fields availability: */
    const bool fIsInterfaceConfigurable = !m_newData.m_interface.m_fDHCPEnabled;
    if (m_pLabelIPv4)
        m_pLabelIPv4->setEnabled(fIsInterfaceConfigurable);
    if (m_pLabelNMv4)
        m_pLabelNMv4->setEnabled(fIsInterfaceConfigurable);
    if (m_pEditorIPv4)
        m_pEditorIPv4->setEnabled(fIsInterfaceConfigurable);
    if (m_pEditorNMv4)
        m_pEditorNMv4->setEnabled(fIsInterfaceConfigurable);

    /* Load IPv4 interface fields: */
    if (m_pButtonAutomatic)
        m_pButtonAutomatic->setChecked(!fIsInterfaceConfigurable);
    if (m_pButtonManual)
        m_pButtonManual->setChecked(fIsInterfaceConfigurable);
    if (m_pEditorIPv4)
        m_pEditorIPv4->setText(m_newData.m_interface.m_strAddress);
    if (m_pEditorNMv4)
        m_pEditorNMv4->setText(m_newData.m_interface.m_strMask);

    /* Toggle IPv6 interface fields availability: */
    const bool fIsIpv6Configurable = fIsInterfaceConfigurable && m_newData.m_interface.m_fSupportedIPv6;
    if (m_pLabelIPv6)
        m_pLabelIPv6->setEnabled(fIsIpv6Configurable);
    if (m_pLabelNMv6)
        m_pLabelNMv6->setEnabled(fIsIpv6Configurable);
    if (m_pEditorIPv6)
        m_pEditorIPv6->setEnabled(fIsIpv6Configurable);
    if (m_pEditorNMv6)
        m_pEditorNMv6->setEnabled(fIsIpv6Configurable);

    /* Load IPv6 interface fields: */
    if (m_pEditorIPv6)
        m_pEditorIPv6->setText(m_newData.m_interface.m_strAddress6);
    if (m_pEditorNMv6)
        m_pEditorNMv6->setText(m_newData.m_interface.m_strPrefixLength6);
}

void UIDetailsWidgetHostNetwork::loadDataForDHCPServer()
{
    /* Toggle DHCP server fields availability: */
    const bool fIsDHCPServerEnabled = m_newData.m_dhcpserver.m_fEnabled;
    if (m_pLabelDHCPAddress)
        m_pLabelDHCPAddress->setEnabled(fIsDHCPServerEnabled);
    if (m_pLabelDHCPMask)
        m_pLabelDHCPMask->setEnabled(fIsDHCPServerEnabled);
    if (m_pLabelDHCPLowerAddress)
        m_pLabelDHCPLowerAddress->setEnabled(fIsDHCPServerEnabled);
    if (m_pLabelDHCPUpperAddress)
        m_pLabelDHCPUpperAddress->setEnabled(fIsDHCPServerEnabled);
    if (m_pEditorDHCPAddress)
        m_pEditorDHCPAddress->setEnabled(fIsDHCPServerEnabled);
    if (m_pEditorDHCPMask)
        m_pEditorDHCPMask->setEnabled(fIsDHCPServerEnabled);
    if (m_pEditorDHCPLowerAddress)
        m_pEditorDHCPLowerAddress->setEnabled(fIsDHCPServerEnabled);
    if (m_pEditorDHCPUpperAddress)
        m_pEditorDHCPUpperAddress->setEnabled(fIsDHCPServerEnabled);

    /* Load DHCP server fields: */
    if (m_pCheckBoxDHCP)
        m_pCheckBoxDHCP->setChecked(fIsDHCPServerEnabled);
    if (m_pEditorDHCPAddress)
        m_pEditorDHCPAddress->setText(m_newData.m_dhcpserver.m_strAddress);
    if (m_pEditorDHCPMask)
        m_pEditorDHCPMask->setText(m_newData.m_dhcpserver.m_strMask);
    if (m_pEditorDHCPLowerAddress)
        m_pEditorDHCPLowerAddress->setText(m_newData.m_dhcpserver.m_strLowerAddress);
    if (m_pEditorDHCPUpperAddress)
        m_pEditorDHCPUpperAddress->setText(m_newData.m_dhcpserver.m_strUpperAddress);

    /* Invent default values if server was enabled
     * but at least one current value is invalid: */
    if (   fIsDHCPServerEnabled
        && m_pEditorDHCPAddress
        && m_pEditorDHCPMask
        && m_pEditorDHCPLowerAddress
        && m_pEditorDHCPUpperAddress
        && (   m_pEditorDHCPAddress->text().isEmpty()
            || m_pEditorDHCPAddress->text() == "0.0.0.0"
            || m_pEditorDHCPMask->text().isEmpty()
            || m_pEditorDHCPMask->text() == "0.0.0.0"
            || m_pEditorDHCPLowerAddress->text().isEmpty()
            || m_pEditorDHCPLowerAddress->text() == "0.0.0.0"
            || m_pEditorDHCPUpperAddress->text().isEmpty()
            || m_pEditorDHCPUpperAddress->text() == "0.0.0.0"))
    {
        const QStringList &proposal = makeDhcpServerProposal(m_oldData.m_interface.m_strAddress,
                                                             m_oldData.m_interface.m_strMask);
        m_pEditorDHCPAddress->setText(proposal.at(0));
        m_pEditorDHCPMask->setText(proposal.at(1));
        m_pEditorDHCPLowerAddress->setText(proposal.at(2));
        m_pEditorDHCPUpperAddress->setText(proposal.at(3));
    }
}

void UIDetailsWidgetHostNetwork::revalidate(QWidget *pWidget /* = 0 */)
{
    /* Validate 'Interface' tab content: */
    if (m_pErrorPaneAutomatic && (!pWidget || pWidget == m_pErrorPaneAutomatic))
    {
        const bool fError =    m_newData.m_interface.m_fDHCPEnabled
                            && !m_newData.m_dhcpserver.m_fEnabled;
        m_pErrorPaneAutomatic->setVisible(fError);
    }
    if (m_pErrorPaneManual && (!pWidget || pWidget == m_pErrorPaneManual))
    {
        const bool fError = false;
        m_pErrorPaneManual->setVisible(fError);
    }
    if (m_pErrorPaneIPv4 && (!pWidget || pWidget == m_pErrorPaneIPv4))
    {
        const bool fError =    !m_newData.m_interface.m_fDHCPEnabled
                            && !m_newData.m_interface.m_strAddress.trimmed().isEmpty()
                            && (   !RTNetIsIPv4AddrStr(m_newData.m_interface.m_strAddress.toUtf8().constData())
                                || RTNetStrIsIPv4AddrAny(m_newData.m_interface.m_strAddress.toUtf8().constData()));
        m_pErrorPaneIPv4->setVisible(fError);
    }
    if (m_pErrorPaneNMv4 && (!pWidget || pWidget == m_pErrorPaneNMv4))
    {
        const bool fError =    !m_newData.m_interface.m_fDHCPEnabled
                            && !m_newData.m_interface.m_strMask.trimmed().isEmpty()
                            && (   !RTNetIsIPv4AddrStr(m_newData.m_interface.m_strMask.toUtf8().constData())
                                || RTNetStrIsIPv4AddrAny(m_newData.m_interface.m_strMask.toUtf8().constData()));
        m_pErrorPaneNMv4->setVisible(fError);
    }
    if (m_pErrorPaneIPv6 && (!pWidget || pWidget == m_pErrorPaneIPv6))
    {
        const bool fError =    !m_newData.m_interface.m_fDHCPEnabled
                            && m_newData.m_interface.m_fSupportedIPv6
                            && !m_newData.m_interface.m_strAddress6.trimmed().isEmpty()
                            && (   !RTNetIsIPv6AddrStr(m_newData.m_interface.m_strAddress6.toUtf8().constData())
                                || RTNetStrIsIPv6AddrAny(m_newData.m_interface.m_strAddress6.toUtf8().constData()));
        m_pErrorPaneIPv6->setVisible(fError);
    }
    if (m_pErrorPaneNMv6 && (!pWidget || pWidget == m_pErrorPaneNMv6))
    {
        bool fIsMaskPrefixLengthNumber = false;
        const int iMaskPrefixLength = m_newData.m_interface.m_strPrefixLength6.trimmed().toInt(&fIsMaskPrefixLengthNumber);
        const bool fError =    !m_newData.m_interface.m_fDHCPEnabled
                            && m_newData.m_interface.m_fSupportedIPv6
                            && (   !fIsMaskPrefixLengthNumber
                                || iMaskPrefixLength < 0
                                || iMaskPrefixLength > 128);
        m_pErrorPaneNMv6->setVisible(fError);
    }

    /* Validate 'DHCP server' tab content: */
    if (m_pErrorPaneDHCPAddress && (!pWidget || pWidget == m_pErrorPaneDHCPAddress))
    {
        const bool fError =    m_newData.m_dhcpserver.m_fEnabled
                            && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strAddress.toUtf8().constData())
                                || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strAddress.toUtf8().constData()));
        m_pErrorPaneDHCPAddress->setVisible(fError);
    }
    if (m_pErrorPaneDHCPMask && (!pWidget || pWidget == m_pErrorPaneDHCPMask))
    {
        const bool fError =    m_newData.m_dhcpserver.m_fEnabled
                            && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strMask.toUtf8().constData())
                                || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strMask.toUtf8().constData()));
        m_pErrorPaneDHCPMask->setVisible(fError);
    }
    if (m_pErrorPaneDHCPLowerAddress && (!pWidget || pWidget == m_pErrorPaneDHCPLowerAddress))
    {
        const bool fError =    m_newData.m_dhcpserver.m_fEnabled
                            && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strLowerAddress.toUtf8().constData())
                                || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strLowerAddress.toUtf8().constData()));
        m_pErrorPaneDHCPLowerAddress->setVisible(fError);
    }
    if (m_pErrorPaneDHCPUpperAddress && (!pWidget || pWidget == m_pErrorPaneDHCPUpperAddress))
    {
        const bool fError =    m_newData.m_dhcpserver.m_fEnabled
                            && (   !RTNetIsIPv4AddrStr(m_newData.m_dhcpserver.m_strUpperAddress.toUtf8().constData())
                                || RTNetStrIsIPv4AddrAny(m_newData.m_dhcpserver.m_strUpperAddress.toUtf8().constData()));
        m_pErrorPaneDHCPUpperAddress->setVisible(fError);
    }

    /* Retranslate validation: */
    retranslateValidation(pWidget);
}

void UIDetailsWidgetHostNetwork::retranslateValidation(QWidget *pWidget /* = 0 */)
{
    /* Translate 'Interface' tab content: */
    if (m_pErrorPaneAutomatic && (!pWidget || pWidget == m_pErrorPaneAutomatic))
        m_pErrorPaneAutomatic->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> is set to obtain the address automatically "
                                             "but the corresponding DHCP server is not enabled.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneIPv4 && (!pWidget || pWidget == m_pErrorPaneIPv4))
        m_pErrorPaneIPv4->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                        "IPv4 address.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneNMv4 && (!pWidget || pWidget == m_pErrorPaneNMv4))
        m_pErrorPaneNMv4->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                        "IPv4 network mask.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneIPv6 && (!pWidget || pWidget == m_pErrorPaneIPv6))
        m_pErrorPaneIPv6->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                        "IPv6 address.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneNMv6 && (!pWidget || pWidget == m_pErrorPaneNMv6))
        m_pErrorPaneNMv6->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                        "IPv6 prefix length.").arg(m_newData.m_interface.m_strName));

    /* Translate 'DHCP server' tab content: */
    if (m_pErrorPaneDHCPAddress && (!pWidget || pWidget == m_pErrorPaneDHCPAddress))
        m_pErrorPaneDHCPAddress->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                               "DHCP server address.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneDHCPMask && (!pWidget || pWidget == m_pErrorPaneDHCPMask))
        m_pErrorPaneDHCPMask->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                            "DHCP server mask.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneDHCPLowerAddress && (!pWidget || pWidget == m_pErrorPaneDHCPLowerAddress))
        m_pErrorPaneDHCPLowerAddress->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                                    "DHCP server lower address bound.").arg(m_newData.m_interface.m_strName));
    if (m_pErrorPaneDHCPUpperAddress && (!pWidget || pWidget == m_pErrorPaneDHCPUpperAddress))
        m_pErrorPaneDHCPUpperAddress->setToolTip(tr("Host interface <nobr><b>%1</b></nobr> does not currently have a valid "
                                                    "DHCP server upper address bound.").arg(m_newData.m_interface.m_strName));
}

void UIDetailsWidgetHostNetwork::updateButtonStates()
{
//    if (m_oldData != m_newData)
//        printf("Interface: %s, %s, %s, %s;  DHCP server: %d, %s, %s, %s, %s\n",
//               m_newData.m_interface.m_strAddress.toUtf8().constData(),
//               m_newData.m_interface.m_strMask.toUtf8().constData(),
//               m_newData.m_interface.m_strAddress6.toUtf8().constData(),
//               m_newData.m_interface.m_strPrefixLength6.toUtf8().constData(),
//               (int)m_newData.m_dhcpserver.m_fEnabled,
//               m_newData.m_dhcpserver.m_strAddress.toUtf8().constData(),
//               m_newData.m_dhcpserver.m_strMask.toUtf8().constData(),
//               m_newData.m_dhcpserver.m_strLowerAddress.toUtf8().constData(),
//               m_newData.m_dhcpserver.m_strUpperAddress.toUtf8().constData());

    /* Update 'Apply' / 'Reset' button states: */
    if (m_pButtonBoxInterface)
    {
        m_pButtonBoxInterface->button(QDialogButtonBox::Cancel)->setEnabled(m_oldData != m_newData);
        m_pButtonBoxInterface->button(QDialogButtonBox::Ok)->setEnabled(m_oldData != m_newData);
    }
    if (m_pButtonBoxServer)
    {
        m_pButtonBoxServer->button(QDialogButtonBox::Cancel)->setEnabled(m_oldData != m_newData);
        m_pButtonBoxServer->button(QDialogButtonBox::Ok)->setEnabled(m_oldData != m_newData);
    }

    /* Notify listeners as well: */
    emit sigDataChanged(m_oldData != m_newData);
}

/* $Id: UINetworkRequest.h 58480 2015-10-29 12:58:18Z vboxsync $ */
/** @file
 * VBox Qt GUI - UINetworkRequest class declaration.
 */

/*
 * Copyright (C) 2011-2015 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UINetworkRequest_h___
#define ___UINetworkRequest_h___

/* Qt includes: */
#include <QUuid>
#include <QPointer>

/* GUI inludes: */
#include "UINetworkDefs.h"
#include "UINetworkReply.h"

/* Forward declarations: */
class UINetworkManager;
class UINetworkManagerDialog;
class UINetworkManagerIndicator;
class UINetworkRequestWidget;
class UINetworkCustomer;

/* Network-request contianer: */
class UINetworkRequest : public QObject
{
    Q_OBJECT;

signals:

    /* Notifications to UINetworkManager: */
    void sigProgress(const QUuid &uuid, qint64 iReceived, qint64 iTotal);
    void sigStarted(const QUuid &uuid);
    void sigCanceled(const QUuid &uuid);
    void sigFinished(const QUuid &uuid);
    void sigFailed(const QUuid &uuid, const QString &strError);

    /* Notifications to UINetworkRequestWidget: */
    void sigProgress(qint64 iReceived, qint64 iTotal);
    void sigStarted();
    void sigFinished();
    void sigFailed(const QString &strError);

public:

    /** Constructs network-request of the passed @a type
      * on the basis of the passed @a urls and the @a requestHeaders
      * for the @a pCustomer and @a pNetworkManager specified. */
    UINetworkRequest(UINetworkRequestType type,
                     const QList<QUrl> &urls,
                     const UserDictionary &requestHeaders,
                     UINetworkCustomer *pCustomer,
                     UINetworkManager *pNetworkManager);
    /** Destructs network-request. */
    ~UINetworkRequest();

    /* Getters: */
    const QString description() const;
    UINetworkCustomer* customer() { return m_pCustomer; }
    /** Returns parent network-manager. */
    UINetworkManager* manager() const { return m_pNetworkManager; }
    const QUuid& uuid() const { return m_uuid; }
    UINetworkReply* reply() { return m_pReply; }

private slots:

    /* Network-reply progress handler: */
    void sltHandleNetworkReplyProgress(qint64 iReceived, qint64 iTotal);
    /* Network-reply finish handler: */
    void sltHandleNetworkReplyFinish();

    /* Slot to retry network-request: */
    void sltRetry();
    /* Slot to cancel network-request: */
    void sltCancel();

private:

    /* Prepare: */
    void prepare();
    /* Prepare network-reply: */
    void prepareNetworkReply();

    /* Cleanup network-reply: */
    void cleanupNetworkReply();
    /* Cleanup: */
    void cleanup();

    /* Variables: */
    const UINetworkRequestType m_type;
    const QList<QUrl> m_requests;
    /** Holds the request headers. */
    const UserDictionary m_requestHeaders;
    UINetworkCustomer *m_pCustomer;
    UINetworkManager *m_pNetworkManager;
    const QUuid m_uuid;

    QUrl m_request;
    int m_iCurrentRequestIndex;
    bool m_fRunning;

    QPointer<UINetworkReply> m_pReply;
};

#endif /* !___UINetworkRequest_h___ */


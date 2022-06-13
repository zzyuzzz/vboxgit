/* $Id: UIWizardCloneVD.h 90755 2021-08-19 16:33:03Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardCloneVD class declaration.
 */

/*
 * Copyright (C) 2006-2021 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVD_h
#define FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVD_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CMedium.h"
#include "CMediumFormat.h"


/** UIWizard subclass to clone virtual disk image files. */
class UIWizardCloneVD : public UINativeWizard
{
    Q_OBJECT;

public:

    /** Constructs wizard to clone @a comSourceVirtualDisk passing @a pParent to the base-class. */
    UIWizardCloneVD(QWidget *pParent, const CMedium &comSourceVirtualDisk);

    /** Returns source virtual-disk. */
    const CMedium &sourceVirtualDisk() const { return m_comSourceVirtualDisk; }

    /** Returns the source virtual-disk device type. */
    KDeviceType sourceVirtualDiskDeviceType() const { return m_enmSourceVirtualDiskDeviceType; }

    /** Makes a copy of source virtual-disk. */
    bool copyVirtualDisk();

    const CMediumFormat &mediumFormat() const;
    void setMediumFormat(const CMediumFormat &comMediumFormat);

    qulonglong mediumVariant() const;
    void setMediumVariant(qulonglong uMediumVariant);

protected:

    virtual void populatePages() /* final override */;

private:

    /** Handles translation event. */
    virtual void retranslateUi() /* override */;
    void setMediumVariantPageVisibility();

    CMediumFormat m_comMediumFormat;
    qulonglong m_uMediumVariant;
    /** Holds the source virtual disk wrapper. */
    CMedium m_comSourceVirtualDisk;

    /** Holds the source virtual-disk device type. */
    KDeviceType m_enmSourceVirtualDiskDeviceType;
    int m_iMediumVariantPageIndex;
};

#endif /* !FEQT_INCLUDED_SRC_wizards_clonevd_UIWizardCloneVD_h */

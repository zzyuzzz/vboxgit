/* $Id: UIWizardImportApp.h 91646 2021-10-08 15:22:28Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIWizardImportApp class declaration.
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

#ifndef FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportApp_h
#define FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportApp_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* GUI includes: */
#include "UINativeWizard.h"

/* COM includes: */
#include "COMEnums.h"
#include "CAppliance.h"
#include "CVirtualSystemDescriptionForm.h"

/** MAC address policies. */
enum MACAddressImportPolicy
{
    MACAddressImportPolicy_KeepAllMACs,
    MACAddressImportPolicy_KeepNATMACs,
    MACAddressImportPolicy_StripAllMACs,
    MACAddressImportPolicy_MAX
};
Q_DECLARE_METATYPE(MACAddressImportPolicy);

/** Import Appliance wizard. */
class UIWizardImportApp : public UINativeWizard
{
    Q_OBJECT;

public:

    /** Constructs Import Appliance wizard passing @a pParent to the base-class.
      * @param  fImportFromOCIByDefault  Brings whether wizard should start with OCI target.
      * @param  strFileName              Brings local file name to import OVF/OVA from. */
    UIWizardImportApp(QWidget *pParent,
                      bool fImportFromOCIByDefault,
                      const QString &strFileName);

    /** @name Common fields.
      * @{ */
        /** Returns whether source is cloud one. */
        bool isSourceCloudOne() const { return m_fSourceCloudOne; }
        /** Defines whether source is @a fCloudOne. */
        void setSourceCloudOne(bool fCloudOne) { m_fSourceCloudOne = fCloudOne; }
    /** @} */

    /** @name Local import fields.
      * @{ */
        /** Returns local Appliance object. */
        CAppliance localAppliance() const { return m_comLocalAppliance; }
        /** Defines file @a strName. */
        bool setFile(const QString &strName);

        /** Returns MAC address import policy. */
        MACAddressImportPolicy macAddressImportPolicy() const { return m_enmMacAddressImportPolicy; }
        /** Defines MAC address import @a enmPolicy. */
        void setMACAddressImportPolicy(MACAddressImportPolicy enmPolicy) { m_enmMacAddressImportPolicy = enmPolicy; }

        /** Returns whether hard disks should be imported as VDIs. */
        bool isImportHDsAsVDI() const { return m_fImportHDsAsVDI; }
        /** Defines whether hard disks should be imported @a fAsVDI. */
        void setImportHDsAsVDI(bool fAsVDI) { m_fImportHDsAsVDI = fAsVDI; }
    /** @} */

    /** @name Cloud import fields.
      * @{ */
        /** Returns cloud Appliance object. */
        CAppliance cloudAppliance() const { return m_comCloudAppliance; }
        /** Defines cloud @a comAppliance object. */
        void setCloudAppliance(const CAppliance &comAppliance) { m_comCloudAppliance = comAppliance; }

        /** Returns Virtual System Description import form object. */
        CVirtualSystemDescriptionForm vsdImportForm() const { return m_comVsdImportForm; }
        /** Defines Virtual System Description import @a comForm object. */
        void setVsdImportForm(const CVirtualSystemDescriptionForm &comForm) { m_comVsdImportForm = comForm; }
    /** @} */

    /** @name Auxiliary stuff.
      * @{ */
        /** Imports appliance. */
        bool importAppliance();
    /** @} */

protected:

    /** @name Inherited stuff.
      * @{ */
        /** Populates pages. */
        virtual void populatePages() /* override final */;

        /** Handles translation event. */
        virtual void retranslateUi() /* override final */;
    /** @} */

private:

    /** @name Auxiliary stuff.
      * @{ */
        /** Returns a list of license agreement pairs. */
        QList<QPair<QString, QString> > licenseAgreements() const;
    /** @} */

    /** @name Arguments.
      * @{ */
        /** Holds whether default source should be Import from OCI. */
        bool     m_fImportFromOCIByDefault;
        /** Handles the appliance file name. */
        QString  m_strFileName;
    /** @} */

    /** @name Common fields.
      * @{ */
        /** */
        bool  m_fSourceCloudOne;
    /** @} */

    /** @name Local import fields.
      * @{ */
        /** Holds the local appliance wrapper object. */
        CAppliance  m_comLocalAppliance;

        /** Holds the MAC address import policy. */
        MACAddressImportPolicy  m_enmMacAddressImportPolicy;

        /** Holds whether hard disks should be imported as VDIs. */
        bool  m_fImportHDsAsVDI;
    /** @} */

    /** @name Cloud import fields.
      * @{ */
        /** Holds the cloud appliance wrapper object. */
        CAppliance  m_comCloudAppliance;

        /** Holds the Virtual System Description import form wrapper object. */
        CVirtualSystemDescriptionForm  m_comVsdImportForm;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_wizards_importappliance_UIWizardImportApp_h */

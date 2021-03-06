/* $Id: UIAudioSettingsEditor.h 94709 2022-04-25 16:16:06Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIAudioSettingsEditor class declaration.
 */

/*
 * Copyright (C) 2019-2022 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef FEQT_INCLUDED_SRC_settings_editors_UIAudioSettingsEditor_h
#define FEQT_INCLUDED_SRC_settings_editors_UIAudioSettingsEditor_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QWidget>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"
#include "UIPortForwardingTable.h"

/* COM includes: */
#include "COMEnums.h"

/* Forward declarations: */
class QCheckBox;
class UIAudioControllerEditor;
class UIAudioFeaturesEditor;
class UIAudioHostDriverEditor;

/** QWidget subclass used as a audio settings editor. */
class SHARED_LIBRARY_STUFF UIAudioSettingsEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    /** Constructs editor passing @a pParent to the base-class. */
    UIAudioSettingsEditor(QWidget *pParent = 0);

    /** @name General stuff
     * @{ */
        /** Defines whether feature is @a fEnabled. */
        void setFeatureEnabled(bool fEnabled);
        /** Returns whether feature is enabled. */
        bool isFeatureEnabled() const;

        /** Defines whether feature @a fAvailable. */
        void setFeatureAvailable(bool fAvailable);
    /** @} */

    /** @name Host driver editor stuff
     * @{ */
        /** Defines host driver @a enmType. */
        void setHostDriverType(KAudioDriverType enmType);
        /** Returns host driver type. */
        KAudioDriverType hostDriverType() const;

        /** Defines whether host driver option @a fAvailable. */
        void setHostDriverOptionAvailable(bool fAvailable);
    /** @} */

    /** @name Controller editor stuff
     * @{ */
        /** Defines controller @a enmType. */
        void setControllerType(KAudioControllerType enmValue);
        /** Returns controller type. */
        KAudioControllerType controllerType() const;

        /** Defines whether controller option @a fAvailable. */
        void setControllerOptionAvailable(bool fAvailable);
    /** @} */

    /** @name Features editor stuff
     * @{ */
        /** Defines whether 'enable output' feature in @a fOn. */
        void setEnableOutput(bool fOn);
        /** Returns 'enable output' feature value. */
        bool outputEnabled() const;

        /** Defines whether 'enable input' feature in @a fOn. */
        void setEnableInput(bool fOn);
        /** Returns 'enable input' feature value. */
        bool inputEnabled() const;

        /** Defines whether feature options @a fAvailable. */
        void setFeatureOptionsAvailable(bool fAvailable);
    /** @} */

protected:

    /** Handles translation event. */
    virtual void retranslateUi() RT_OVERRIDE;

private slots:

    /** Handles feature toggling. */
    void sltHandleFeatureToggled();

private:

    /** Prepares all. */
    void prepare();
    /** Prepares widgets. */
    void prepareWidgets();
    /** Prepares connections. */
    void prepareConnections();

    /** Updates feature availability. */
    void updateFeatureAvailability();

    /** @name Values
     * @{ */
        /** Holds whether feature is enabled. */
        bool  m_fFeatureEnabled;
    /** @} */

    /** @name Widgets
     * @{ */
        /** Holds the feature check-box instance. */
        QCheckBox               *m_pCheckboxFeature;
        /** Holds the settings widget instance. */
        QWidget                 *m_pWidgetSettings;
        /** Holds the host driver editor instance. */
        UIAudioHostDriverEditor *m_pEditorAudioHostDriver;
        /** Holds the controller editor instance. */
        UIAudioControllerEditor *m_pEditorAudioController;
        /** Holds the features editor instance. */
        UIAudioFeaturesEditor   *m_pEditorAudioFeatures;
    /** @} */
};

#endif /* !FEQT_INCLUDED_SRC_settings_editors_UIAudioSettingsEditor_h */

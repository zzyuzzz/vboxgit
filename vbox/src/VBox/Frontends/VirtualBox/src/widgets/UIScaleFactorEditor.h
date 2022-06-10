/* $Id: UIScaleFactorEditor.h 75370 2018-11-09 16:41:58Z vboxsync $ */
/** @file
 * VBox Qt GUI - UIScaleFactorEditor class declaration.
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIScaleFactorEditor_h___
#define ___UIScaleFactorEditor_h___

/** GUI includes. */
#include "QIWithRetranslateUI.h"
#include "UILibraryDefs.h"


/** Forward declarations. */
class QComboBox;
class QGridLayout;
class QLabel;
class QSpinBox;
class QWidget;
class QIAdvancedSlider;

/** QWidget reimplementation providing GUI with monitor scale factor editing functionality.
  *  It includes a combo box to select a monitor, a slider, and a spinbox to display/modify values.
  *  The first item in the combo box is used to change the scale factor of all monitors. */
class SHARED_LIBRARY_STUFF UIScaleFactorEditor : public QIWithRetranslateUI<QWidget>
{
    Q_OBJECT;

public:

    UIScaleFactorEditor(QWidget *pParent);
    /** Configures the internal variables (m_pMonitorComboBox items , m_scaleFactors' size, etc)
      * wrt. @p iMonitorCount. */
    void           setMonitorCount(int iMonitorCount);
    void           setScaleFactors(const QList<double> &scaleFactors);
    /** Returns either a single global scale factor or a list of scale factor for each monitor. */
    QList<double>  scaleFactors() const;
    void           setDefaultScaleFactor(double dDefaultScaleFactor);
    /** Defines minimum width @a iHint for internal spin-box. */
    void           setSpinBoxWidthHint(int iHint);

protected:

    virtual void retranslateUi() /* override */;

private slots:

    /** @name Internal slots handling respective widget's value update.
      * @{ */
        void sltScaleSpinBoxValueChanged(int value);
        void sltScaleSliderValueChanged(int value);
        void sltMonitorComboIndexChanged(int index);
    /** @} */

private:

    void               prepare();
    void               setIsGlobalScaleFactor(bool bFlag);
    void               setScaleFactor(int iMonitorIndex, int iScaleFactor);
    /** Blocks slider's signals before settting the slider value. */
    void               setSliderValue(int iValue);
    /** Blocks spinbox's signals before settting the value. */
    void               setSpinBoxValue(int iValue);
    /** Set the spinbox and slider to scale factor of currently selected monitor. */
    void               updateValuesAfterMonitorChange();

    /** @name Member widgets.
      * @{ */
        QSpinBox          *m_pScaleSpinBox;
        QGridLayout       *m_pMainLayout;
        QComboBox         *m_pMonitorComboBox;
        QIAdvancedSlider  *m_pScaleSlider;
        QLabel            *m_pMaxScaleLabel;
        QLabel            *m_pMinScaleLabel;
    /** @} */

    /** Stores the per-monitor scale factors. The 0th item is for all monitors (global). */
    QList<double>      m_scaleFactors;
    double             m_dDefaultScaleFactor;
};

#endif /* !___UIScaleFactorEditor_h___ */

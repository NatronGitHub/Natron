/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef KNOBGUIVALUE_H
#define KNOBGUIVALUE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <vector> // KnobGuiInt
#include <list>

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QObject>
#include <QStyledItemDelegate>
#include <QTextEdit>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EngineFwd.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGui.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

struct KnobGuiValuePrivate;
class KnobGuiValue
    : public KnobGui
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    KnobGuiValue(KnobIPtr knob,
                 KnobGuiContainerI *container);

    virtual ~KnobGuiValue() OVERRIDE;

    virtual void removeSpecificGui() OVERRIDE FINAL;
    virtual KnobIPtr getKnob() const OVERRIDE FINAL;
    virtual bool getAllDimensionsVisible() const OVERRIDE FINAL;

    int getDimensionForSpinBox(const SpinBox* spinbox) const;

public Q_SLOTS:

    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
    void onSliderEditingFinished(bool hasMovedOnce);
    void onMinMaxChanged(double mini, double maxi, int index = 0);
    void onDisplayMinMaxChanged(double mini, double maxi, int index = 0);
    void onIncrementChanged(double incr, int index = 0);
    void onDecimalsChanged(int deci, int index = 0);

    void onDimensionSwitchClicked(bool);

    void onRectangleFormatButtonClicked();

    void onResetToDefaultRequested();

protected:

    void getSpinBox(int dim, SpinBox** spinbox, Label** label = 0) const;

    virtual bool isSliderDisabled() const = 0;
    virtual bool isAutoFoldDimensionsEnabled() const = 0;
    virtual bool isRectangleType() const = 0;
    virtual bool isSpatialType() const = 0;
    virtual ValueIsNormalizedEnum getNormalizationPolicy(int /*dimension*/) const
    {
        return eValueIsNormalizedNone;
    }

    virtual double denormalize(int /*dimension*/,
                               double /*time*/,
                               double value) const
    {
        return value;
    }

    virtual double normalize(int /*dimension*/,
                             double /*time*/,
                             double value) const
    {
        return value;
    }

    virtual void connectKnobSignalSlots() {}

    virtual void disableSlider() = 0;
    virtual void getIncrements(std::vector<double>* increments) const = 0;
    virtual void getDecimals(std::vector<int>* /*decimals*/) const {}

    virtual void addExtraWidgets(QHBoxLayout* /*containerLayout*/) {}

    virtual void _hide() OVERRIDE;
    virtual void _show() OVERRIDE;
    virtual void setEnabledExtraGui(bool /*enabled*/) {}

    virtual void onDimensionsFolded() {}

    virtual void onDimensionsExpanded() {}

    virtual void updateExtraGui(const std::vector<double>& /*values*/)
    {
    }

private:

    /**
     * @brief Normalized parameters handling. It converts from project format
     * to normailzed coords or from project format to normalized coords.
     * @param normalize True if we want to normalize, false otherwise
     * @param dimension Must be either 0 and 1
     * @note If the dimension of the knob is not 1 or 2 this function does nothing.
     **/
    double valueAccordingToType(bool normalize, int dimension, double value) WARN_UNUSED_RETURN;


    void expandAllDimensions();
    void foldAllDimensions();

    void sliderEditingEnd(double d);

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    void setMaximum(int);
    void setMinimum(int);

    virtual bool shouldAddStretch() const OVERRIDE FINAL;
    virtual void setEnabled() OVERRIDE FINAL;
    virtual void setReadOnly(bool readOnly, int dimension) OVERRIDE FINAL;
    virtual void updateGUI(int dimension) OVERRIDE FINAL;
    virtual void setDirty(bool dirty) OVERRIDE FINAL;
    virtual void reflectAnimationLevel(int dimension, AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void reflectExpressionState(int dimension, bool hasExpr) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
    virtual void refreshDimensionName(int dim) OVERRIDE FINAL;

private:

    boost::scoped_ptr<KnobGuiValuePrivate> _imp;
};

class KnobGuiDouble
    : public KnobGuiValue
{
    KnobDoubleWPtr _knob;

public:

    static KnobGui * BuildKnobGui(KnobIPtr knob,
                                  KnobGuiContainerI *container)
    {
        return new KnobGuiDouble(knob, container);
    }

    KnobGuiDouble(KnobIPtr knob,
                  KnobGuiContainerI *container);

    virtual ~KnobGuiDouble() {}

private:


    virtual bool isSliderDisabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isAutoFoldDimensionsEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isRectangleType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSpatialType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ValueIsNormalizedEnum getNormalizationPolicy(int dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double denormalize(int dimension, double time, double value) const OVERRIDE FINAL;
    virtual double normalize(int dimension, double time, double value) const OVERRIDE FINAL;
    virtual void connectKnobSignalSlots() OVERRIDE FINAL;
    virtual void disableSlider() OVERRIDE FINAL;
    virtual void getIncrements(std::vector<double>* increments) const OVERRIDE FINAL;
    virtual void getDecimals(std::vector<int>* decimals) const OVERRIDE FINAL;
};

class KnobGuiInt
    : public KnobGuiValue
{
    KnobIntWPtr _knob;

public:

    static KnobGui * BuildKnobGui(KnobIPtr knob,
                                  KnobGuiContainerI *container)
    {
        return new KnobGuiInt(knob, container);
    }

    KnobGuiInt(KnobIPtr knob,
               KnobGuiContainerI *container);

    virtual ~KnobGuiInt() {}

private:

    virtual bool isSpatialType() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isSliderDisabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isAutoFoldDimensionsEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isRectangleType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void connectKnobSignalSlots() OVERRIDE FINAL;
    virtual void disableSlider() OVERRIDE FINAL;
    virtual void getIncrements(std::vector<double>* increments) const OVERRIDE FINAL;
};

NATRON_NAMESPACE_EXIT


#endif // KNOBGUIVALUE_H

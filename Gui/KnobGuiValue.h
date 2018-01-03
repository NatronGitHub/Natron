/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/Knob.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EngineFwd.h"

#include "Gui/KnobGui.h"
#include "Gui/KnobGuiWidgets.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER


#define kKnobGuiValueSpinBoxDimensionProperty "KnobGuiValueSpinBoxDimensionProperty"


struct KnobGuiValuePrivate;
class KnobGuiValue
: public QObject
, public KnobGuiWidgets
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    KnobGuiValue(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiValue() OVERRIDE;
    
    bool getDimForSpinBox(const SpinBox* spinbox, DimIdx* dimension) const;

    virtual void setAllDimensionsVisible(bool visible) OVERRIDE;

    void disableSpinBoxesBorder();

    virtual void reflectLinkedState(DimIdx dimension, bool linked) OVERRIDE;

public Q_SLOTS:

    void onSpinBoxValueChanged();
    void onSliderValueChanged(double);
    void onSliderEditingFinished(bool hasMovedOnce);
    void onMinMaxChanged(DimSpec dimension);
    void onDisplayMinMaxChanged(DimSpec dimension);
    void onIncrementChanged(double incr, DimIdx dimension);
    void onDecimalsChanged(int deci, DimIdx dimension);

    void onDimensionSwitchClicked(bool);

    void onRectangleFormatButtonClicked();

    void onSliderResetToDefaultRequested();

protected:

    bool getSpinBox(DimIdx dim, SpinBox** spinbox, Label** label = 0) const;

    virtual bool isSliderDisabled() const = 0;
    virtual bool isRectangleType() const = 0;
    virtual bool isSpatialType() const = 0;
    virtual ValueIsNormalizedEnum getNormalizationPolicy(DimIdx /*dimension*/) const
    {
        return eValueIsNormalizedNone;
    }

    virtual double denormalize(DimIdx /*dimension*/,
                               TimeValue /*time*/,
                               double value) const
    {
        return value;
    }

    virtual double normalize(DimIdx /*dimension*/,
                             TimeValue /*time*/,
                             double value) const
    {
        return value;
    }

    virtual void connectKnobSignalSlots() {}

    virtual void disableSlider() = 0;
    virtual void getIncrements(std::vector<double>* increments) const = 0;
    virtual void getDecimals(std::vector<int>* /*decimals*/) const {}

    virtual void addExtraWidgets(QHBoxLayout* /*containerLayout*/) {}

    virtual void setWidgetsVisible(bool visible) OVERRIDE;

    virtual void setEnabledExtraGui(bool /*enabled*/) {}

    virtual void onDimensionsMadeVisible(bool visible) { Q_UNUSED(visible); }

    virtual void updateExtraGui(const std::vector<double>& /*values*/)
    {
    }

    virtual Qt::Alignment getSpinboxAlignment() const
    {
        return Qt::AlignLeft | Qt::AlignVCenter;
    }

protected:

    void setVisibleSlider(bool visible);

    void setWidgetsVisibleInternal(bool visible);

    /**
     * @brief Normalized parameters handling. It converts from project format
     * to normailzed coords or from project format to normalized coords.
     * @param normalize True if we want to normalize, false otherwise
     * @param dimension Must be either 0 and 1
     * @note If the dimension of the knob is not 1 or 2 this function does nothing.
     **/
    double valueAccordingToType(bool normalize, DimIdx dimension, double value) WARN_UNUSED_RETURN;



    void sliderEditingEnd(double d);

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE;

    virtual bool shouldAddStretch() const OVERRIDE ;
    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE ;
    virtual void updateGUI() OVERRIDE ;
    virtual void reflectMultipleSelection(bool dirty) OVERRIDE ;
    virtual void reflectSelectionState(bool selected) OVERRIDE ;
    virtual void reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level) OVERRIDE ;
    virtual void updateToolTip() OVERRIDE ;
    virtual void reflectModificationsState() OVERRIDE ;
    virtual void refreshDimensionName(DimIdx dim) OVERRIDE ;

private:

    friend struct KnobGuiValuePrivate;
    boost::scoped_ptr<KnobGuiValuePrivate> _imp;
};

class KnobGuiDouble
    : public KnobGuiValue
{
    KnobDoubleWPtr _knob;

public:

    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiDouble(knob, view);
    }

    KnobGuiDouble(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiDouble() {}

private:


    virtual bool isSliderDisabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isRectangleType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isSpatialType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual ValueIsNormalizedEnum getNormalizationPolicy(DimIdx dimension) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual double denormalize(DimIdx dimension, TimeValue time, double value) const OVERRIDE FINAL;
    virtual double normalize(DimIdx dimension, TimeValue time, double value) const OVERRIDE FINAL;
    virtual void connectKnobSignalSlots() OVERRIDE FINAL;
    virtual void disableSlider() OVERRIDE FINAL;
    virtual void getIncrements(std::vector<double>* increments) const OVERRIDE FINAL;
    virtual void getDecimals(std::vector<int>* decimals) const OVERRIDE FINAL;
};

class KnobGuiInt
    : public KnobGuiValue
{

    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON


    KnobIntWPtr _knob;

    KeybindRecorder* _shortcutRecorder;

public:

    static KnobGuiWidgets * BuildKnobGui(const KnobGuiPtr& knob, ViewIdx view)
    {
        return new KnobGuiInt(knob, view);
    }

    KnobGuiInt(const KnobGuiPtr& knob, ViewIdx view);

    virtual ~KnobGuiInt() {}

public Q_SLOTS:

    void onKeybindRecorderEditingFinished();

private:

    virtual bool isSpatialType() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isSliderDisabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool isRectangleType() const OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void connectKnobSignalSlots() OVERRIDE FINAL;
    virtual void disableSlider() OVERRIDE FINAL;
    virtual void getIncrements(std::vector<double>* increments) const OVERRIDE FINAL;

    virtual Qt::Alignment getSpinboxAlignment() const OVERRIDE FINAL;

    virtual void createWidget(QHBoxLayout* layout) OVERRIDE FINAL;

    virtual void updateGUI() OVERRIDE FINAL ;

    virtual void setEnabled(const std::vector<bool>& perDimEnabled) OVERRIDE FINAL;
    virtual void reflectMultipleSelection(bool dirty) OVERRIDE FINAL;
    virtual void reflectSelectionState(bool selected) OVERRIDE ;
    virtual void reflectAnimationLevel(DimIdx dimension, AnimationLevelEnum level) OVERRIDE FINAL;
    virtual void updateToolTip() OVERRIDE FINAL;
    virtual void reflectModificationsState() OVERRIDE FINAL;
    virtual void refreshDimensionName(DimIdx dim) OVERRIDE FINAL;
};


inline KnobGuiDoublePtr
toKnobGuiDouble(const KnobGuiWidgetsPtr& knobGui)
{
    return boost::dynamic_pointer_cast<KnobGuiDouble>(knobGui);
}

inline KnobGuiIntPtr
toKnobGuiInt(const KnobGuiWidgetsPtr& knobGui)
{
    return boost::dynamic_pointer_cast<KnobGuiInt>(knobGui);
}


NATRON_NAMESPACE_EXIT

#endif // KNOBGUIVALUE_H

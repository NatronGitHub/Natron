/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef Gui_KnobGuiColor_h
#define Gui_KnobGuiColor_h

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
#include <QToolButton>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Global/GlobalDefines.h"

#include "Engine/Singleton.h"
#include "Engine/Knob.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EngineFwd.h"

#include "Gui/CurveSelection.h"
#include "Gui/KnobGuiValue.h"
#include "Gui/ColorSelectorWidget.h"
#include "Gui/AnimatedCheckBox.h"
#include "Gui/Label.h"
#include "Gui/GuiFwd.h"

NATRON_NAMESPACE_ENTER

//================================

class KnobGuiColor;
class ColorPickerLabel
    : public Label
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    ColorPickerLabel(KnobGuiColor* knob,
                     QWidget* parent = NULL);

    virtual ~ColorPickerLabel() OVERRIDE
    {
    }

    bool isPickingEnabled() const
    {
        return _pickingEnabled;
    }

    void setColor(const QColor & color);

    const QColor& getCurrentColor() const
    {
        return _currentColor;
    }

    void setPickingEnabled(bool enabled);

    void setEnabledMode(bool enabled);

Q_SIGNALS:

    void pickingEnabled(bool);

private:

    virtual void enterEvent(QEvent*) OVERRIDE FINAL;
    virtual void leaveEvent(QEvent*) OVERRIDE FINAL;
    virtual void mousePressEvent(QMouseEvent*) OVERRIDE FINAL;

private:

    bool _pickingEnabled;
    QColor _currentColor;
    KnobGuiColor* _knob;
};


class KnobGuiColor
    : public KnobGuiValue
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    static KnobGui * BuildKnobGui(KnobIPtr knob,
                                  KnobGuiContainerI *container)
    {
        return new KnobGuiColor(knob, container);
    }

    KnobGuiColor(KnobIPtr knob,
                 KnobGuiContainerI *container);

    virtual ~KnobGuiColor() {}

public Q_SLOTS:

    void updateColorSelector();

    void setPickingEnabled(bool enabled);

    void onPickingEnabled(bool enabled);

    void onMustShowAllDimension();

    void onColorSelectorChanged(float r, float g, float b, float a);

Q_SIGNALS:

    void dimensionSwitchToggled(bool b);

private:

    virtual bool isSpatialType() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isSliderDisabled() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual bool isAutoFoldDimensionsEnabled() const OVERRIDE FINAL WARN_UNUSED_RETURN;

    virtual bool isRectangleType() const OVERRIDE FINAL WARN_UNUSED_RETURN
    {
        return false;
    }

    virtual void disableSlider() OVERRIDE FINAL
    {
    }

    virtual void connectKnobSignalSlots() OVERRIDE FINAL;
    virtual void getIncrements(std::vector<double>* increments) const OVERRIDE FINAL;
    virtual void getDecimals(std::vector<int>* decimals) const OVERRIDE FINAL;
    virtual void addExtraWidgets(QHBoxLayout* containerLayout) OVERRIDE FINAL;
    virtual void updateExtraGui(const std::vector<double>& values) OVERRIDE FINAL;

    void updateLabel(double r, double g, double b, double a);

    virtual void _show() OVERRIDE FINAL;
    virtual void _hide() OVERRIDE FINAL;
    virtual void setEnabledExtraGui(bool enabled) OVERRIDE FINAL;
    virtual void onDimensionsFolded() OVERRIDE FINAL;
    virtual void onDimensionsExpanded() OVERRIDE FINAL;

private:

    KnobColorWPtr _knob;
    ColorPickerLabel *_colorLabel;
    ColorSelectorWidget *_colorSelector;
    QToolButton *_colorSelectorButton;
    std::vector<double> _lastColor;
    bool _useSimplifiedUI;
};

NATRON_NAMESPACE_EXIT

#endif // Gui_KnobGuiColor_h

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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "SpinBoxValidator.h"

#include <stdexcept>

#include "Engine/KnobUndoCommand.h"
#include "Engine/KnobTypes.h"

#include "Gui/KnobGuiValue.h"
#include "Gui/KnobGuiColor.h"
#include "Gui/SpinBox.h"

NATRON_NAMESPACE_ENTER

struct NumericKnobValidatorPrivate
{
    const SpinBox* spinbox;
    KnobGuiWidgetsWPtr knobGui;
    ViewIdx view;

    ///Only these knobs have spinboxes
    KnobGuiDoublePtr isDoubleGui;
    KnobGuiColorPtr isColorGui;
    KnobGuiIntPtr isIntGui;
    KnobDoubleBasePtr isDouble;
    KnobIntBasePtr isInt;

    NumericKnobValidatorPrivate(const SpinBox* spinbox,
                                const KnobGuiWidgetsPtr& knob,
                                ViewIdx view)
        : spinbox(spinbox)
        , knobGui(knob)
        , view(view)

    {

        isDoubleGui = toKnobGuiDouble(knob);
        isColorGui = toKnobGuiColor(knob);
        isIntGui = toKnobGuiInt(knob);

        KnobIPtr internalKnob = knob->getKnobGui()->getKnob();

        isDouble = toKnobDoubleBase(internalKnob);
        isInt = toKnobIntBase(internalKnob);
        assert( isDouble || isInt );
    }
};

NumericKnobValidator::NumericKnobValidator(const SpinBox* spinbox,
                                           const KnobGuiWidgetsPtr& knob,
                                           ViewIdx view)
    : _imp( new NumericKnobValidatorPrivate(spinbox, knob, view) )
{
}

NumericKnobValidator::~NumericKnobValidator()
{
}

bool
NumericKnobValidator::validateInput(const QString& userText,
                                    double* valueToDisplay) const
{
    DimSpec dimension;
    KnobGuiWidgetsPtr knobWidgets = _imp->knobGui.lock();
    bool allDimsVisible = knobWidgets->getKnobGui()->getKnob()->getAllDimensionsVisible(_imp->view);

    if (!allDimsVisible) {
        dimension = DimSpec::all();
    } else {
        dimension = DimSpec(_imp->spinbox->property(kKnobGuiValueSpinBoxDimensionProperty).toInt());
    }

    *valueToDisplay = 0;
    std::string ret;
    QString simplifiedUserText = userText.simplified();
    bool isPersistentExpression = simplifiedUserText.startsWith( QLatin1Char('=') );
    if (isPersistentExpression) {
        simplifiedUserText.remove(0, 1);
    }
    std::string expr = simplifiedUserText.toStdString();
    if (!expr.empty() && knobWidgets) {
        KnobIPtr internalKnob = knobWidgets->getKnobGui()->getKnob();
        try {
            internalKnob->validateExpression(expr, eExpressionLanguageExprTk, DimIdx(0), _imp->view, false, &ret);
        } catch (...) {
            return false;
        }

        if (isPersistentExpression) {
            //Only set the expression if it starts with '='
            knobWidgets->getKnobGui()->pushUndoCommand( new SetExpressionCommand(internalKnob,
                                                                                 eExpressionLanguageExprTk,
                                                                                 false,
                                                                                 dimension,
                                                                                 _imp->view,
                                                                                 expr) );
        }
        

        bool ok = false;
        *valueToDisplay = QString::fromUtf8( ret.c_str() ).toDouble(&ok);
        if (!ok) {
            *valueToDisplay = QString::fromUtf8( ret.c_str() ).toInt(&ok);
        }
        assert(ok);
    } else {
        *valueToDisplay = _imp->spinbox->getLastValidValueBeforeValidation();
    }

    return true;
} // NumericKnobValidator::validateInput

NATRON_NAMESPACE_EXIT

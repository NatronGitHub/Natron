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

#include "Engine/KnobTypes.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobGuiValue.h"
#include "Gui/KnobGuiColor.h"
#include "Gui/SpinBox.h"

NATRON_NAMESPACE_ENTER

struct NumericKnobValidatorPrivate
{
    const SpinBox* spinbox;
    KnobGuiWPtr knobUi;

    ///Only these knobs have spinboxes
    KnobGuiDouble* isDoubleGui;
    KnobGuiColor* isColorGui;
    KnobGuiInt* isIntGui;
    KnobDoubleBaseWPtr isDouble;
    KnobIntBaseWPtr isInt;

    NumericKnobValidatorPrivate(const SpinBox* spinbox,
                                const KnobGuiPtr& knob)
        : spinbox(spinbox)
        , knobUi(knob)
        , isDoubleGui( dynamic_cast<KnobGuiDouble*>( knob.get() ) )
        , isColorGui( dynamic_cast<KnobGuiColor*>( knob.get() ) )
        , isIntGui( dynamic_cast<KnobGuiInt*>( knob.get() ) )
    {
        KnobIPtr internalKnob = knob->getKnob();

        isDouble = boost::dynamic_pointer_cast<KnobDoubleBase>(internalKnob);
        isInt = boost::dynamic_pointer_cast<KnobIntBase>(internalKnob);
        assert( isDouble.lock() || isInt.lock() );
    }
};

NumericKnobValidator::NumericKnobValidator(const SpinBox* spinbox,
                                           const KnobGuiPtr& knob)
    : _imp( new NumericKnobValidatorPrivate(spinbox, knob) )
{
}

NumericKnobValidator::~NumericKnobValidator()
{
}

bool
NumericKnobValidator::validateInput(const QString& userText,
                                    double* valueToDisplay) const
{
    int dimension;
    bool allDimsVisible = true;

    if (_imp->isDoubleGui) {
        allDimsVisible = _imp->isDoubleGui->getAllDimensionsVisible();
    } else if (_imp->isIntGui) {
        allDimsVisible = _imp->isIntGui->getAllDimensionsVisible();
    } else if (_imp->isColorGui) {
        allDimsVisible = _imp->isColorGui->getAllDimensionsVisible();
    } else {
        assert(0);
    }
    if (!allDimsVisible) {
        dimension = -1;
    } else {
        if (_imp->isDoubleGui) {
            dimension = _imp->isDoubleGui->getDimensionForSpinBox(_imp->spinbox);
        } else if (_imp->isIntGui) {
            dimension = _imp->isIntGui->getDimensionForSpinBox(_imp->spinbox);
        } else if (_imp->isColorGui) {
            dimension = _imp->isColorGui->getDimensionForSpinBox(_imp->spinbox);
        } else {
            dimension = 0;
        }
    }

    *valueToDisplay = 0;
    std::string ret;
    QString simplifiedUserText = userText.simplified();
    bool isPersistentExpression = simplifiedUserText.startsWith( QLatin1Char('=') );
    if (isPersistentExpression) {
        simplifiedUserText.remove(0, 1);
    }
    std::string expr = simplifiedUserText.toStdString();
    KnobGuiPtr knob = _imp->knobUi.lock();

    if (!expr.empty() && knob) {
        try {
            knob->getKnob()->validateExpression(expr, 0, false, &ret);
        } catch (...) {
            return false;
        }

        if (isPersistentExpression) {
            //Only set the expression if it starts with '='
            knob->pushUndoCommand( new SetExpressionCommand(knob->getKnob(),
                                                            false,
                                                            dimension,
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

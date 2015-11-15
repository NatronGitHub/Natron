/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/KnobTypes.h"
#include "Gui/KnobUndoCommand.h"
#include "Gui/KnobGuiDouble.h"
#include "Gui/KnobGuiInt.h"
#include "Gui/KnobGuiColor.h"
#include "Gui/SpinBox.h"

struct NumericKnobValidatorPrivate
{
    
    const SpinBox* spinbox;
    KnobGui* knobUi;
    
    ///Only these knobs have spinboxes
    KnobGuiDouble* isDoubleGui;
    KnobGuiColor* isColorGui;
    KnobGuiInt* isIntGui;
    
    
    boost::weak_ptr<Knob<double> > isDouble;
    boost::weak_ptr<Knob<int> > isInt;
    
    NumericKnobValidatorPrivate(const SpinBox* spinbox,KnobGui* knob)
    : spinbox(spinbox)
    , knobUi(knob)
    , isDoubleGui(dynamic_cast<KnobGuiDouble*>(knob))
    , isColorGui(dynamic_cast<KnobGuiColor*>(knob))
    , isIntGui(dynamic_cast<KnobGuiInt*>(knob))
    {
        boost::shared_ptr<KnobI> internalKnob = knob->getKnob();
        isDouble = boost::dynamic_pointer_cast<Knob<double> >(internalKnob);
        isInt = boost::dynamic_pointer_cast<Knob<int> >(internalKnob);
        assert(isDouble.lock() || isInt.lock());
    }
};

NumericKnobValidator::NumericKnobValidator(const SpinBox* spinbox,KnobGui* knob)
: _imp(new NumericKnobValidatorPrivate(spinbox,knob))
{
    
}

NumericKnobValidator::~NumericKnobValidator()
{
    
}

bool
NumericKnobValidator::validateInput(const QString& userText, double* valueToDisplay) const
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
    std::string expr = userText.toStdString();
    if (!expr.empty()) {
        try {
            _imp->knobUi->getKnob()->validateExpression(expr, 0, false, &ret);
        } catch (...) {
            return false;
        }
    }
    
    _imp->knobUi->pushUndoCommand(new SetExpressionCommand(_imp->knobUi->getKnob(),
                                                           false,
                                                           dimension,
                                                           expr));
    if (!expr.empty()) {
        bool ok = false;
        *valueToDisplay = QString(ret.c_str()).toDouble(&ok);
        if (!ok) {
            *valueToDisplay = QString(ret.c_str()).toInt(&ok);
        }
        assert(ok);
    } else {
        *valueToDisplay = _imp->spinbox->getLastValidValueBeforeValidation();
    }
    return true;
}

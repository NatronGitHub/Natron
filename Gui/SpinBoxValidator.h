/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#ifndef GUI_SPINBOXVALIDATOR_H
#define GUI_SPINBOXVALIDATOR_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#endif

#include "Gui/GuiFwd.h"


class SpinBoxValidator
{
public:
    
    SpinBoxValidator()
    {
        
    }
    
    virtual ~SpinBoxValidator()
    {
        
    }
    
    virtual bool validateInput(const QString& userText, double* valueToDisplay) const = 0;
};


struct NumericKnobValidatorPrivate;
class NumericKnobValidator : public SpinBoxValidator
{
    
public:
    
    NumericKnobValidator(const SpinBox* spinbox,KnobGui* knob);
    
    virtual ~NumericKnobValidator();
    
    virtual bool validateInput(const QString& userText, double* valueToDisplay) const OVERRIDE FINAL WARN_UNUSED_RETURN;
    
private:
    
    boost::scoped_ptr<NumericKnobValidatorPrivate> _imp;
};

#endif // GUI_SPINBOXVALIDATOR_H

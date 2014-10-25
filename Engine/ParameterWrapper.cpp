//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#include "ParameterWrapper.h"

#include "Engine/KnobTypes.h"

Param::Param(const boost::shared_ptr<KnobI>& knob)
: _knob(knob)
{
    
}

Param::~Param()
{
    
}

int
Param::getNumDimensions() const
{
    return _knob->getDimension();
}

std::string
Param::getScriptName() const
{
    return _knob->getName();
}


///////////// IntParam

IntParam::IntParam(const boost::shared_ptr<Int_Knob>& knob)
: Param(boost::dynamic_pointer_cast<KnobI>(knob))
, _intKnob(knob)
{
    
}

IntParam::~IntParam()
{

}

int
IntParam::getValue(int dimension) const
{
    return _intKnob->getValue(dimension);
}
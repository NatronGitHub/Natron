/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier
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

#include "Global/Mock.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/Rect.h"

#undef OfxEffectInstance
#include "OfxEffectInstance_Mock.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Invoke;

OfxEffectInstance_Mock::OfxEffectInstance_Mock(Natron::Node* node)
    : OfxEffectInstance(node)
{
    ON_CALL( *this,isInputOptional(_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::isInputOptional_delegate) );

    ON_CALL( *this,getRegionOfDefinition(_,_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::getRegionOfDefinition_delegate) );

    ON_CALL( *this,getRegionOfInterest(_,_,_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::getRegionOfInterest_delegate) );

    ON_CALL( *this,getFrameRange(_,_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::getFrameRange_delegate) );

    ON_CALL( *this,beginKnobsValuesChanged(_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::beginKnobsValuesChanged_delegate) );

    ON_CALL( *this,endKnobsValuesChanged(_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::endKnobsValuesChanged_delegate) );

    ON_CALL( *this,onKnobValueChanged(_,_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::onKnobValueChanged_delegate) );

    ON_CALL( *this,render(_,_,_,_,_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::render_delegate) );

    ON_CALL( *this,isIdentity(_,_,_,_,_,_) )
    .WillByDefault( Invoke(this,&OfxEffectInstance_Mock::isIdentity_delegate) );
}


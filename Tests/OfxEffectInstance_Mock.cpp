//  Natron
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "OfxEffectInstance_Mock.h"
#include "Engine/Rect.h"

using ::testing::_;
using ::testing::AtLeast;
using ::testing::Invoke;

OfxEffectInstance_Mock::OfxEffectInstance_Mock(Natron::Node* node)
    : OfxEffectInstance(node)
{
    
    
    ON_CALL(*this,isInputOptional(_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::isInputOptional_delegate));

    ON_CALL(*this,getRegionOfDefinition(_,_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::getRegionOfDefinition_delegate));

    ON_CALL(*this,getRegionOfInterest(_,_,_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::getRegionOfInterest_delegate));

    ON_CALL(*this,getFrameRange(_,_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::getFrameRange_delegate));

    ON_CALL(*this,beginKnobsValuesChanged(_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::beginKnobsValuesChanged_delegate));

    ON_CALL(*this,endKnobsValuesChanged(_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::endKnobsValuesChanged_delegate));

    ON_CALL(*this,onKnobValueChanged(_,_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::onKnobValueChanged_delegate));

    ON_CALL(*this,render(_,_,_,_,_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::render_delegate));

    ON_CALL(*this,isIdentity(_,_,_,_,_,_))
            .WillByDefault(Invoke(this,&OfxEffectInstance_Mock::isIdentity_delegate));

}

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

#ifndef OFXEFFECTINSTANCE_MOCK_H
#define OFXEFFECTINSTANCE_MOCK_H

#include <gmock/gmock.h>

#include "Engine/OfxEffectInstance.h"

///https://code.google.com/p/googlemock/wiki/V1_7_CookBook#Delegating_Calls_to_a_Parent_Class

class OfxEffectInstance_Mock : public OfxEffectInstance
{
public:

    OfxEffectInstance_Mock(Natron::Node* node);

    virtual ~OfxEffectInstance_Mock() {}


    MOCK_CONST_METHOD1(isInputOptional,bool(int inputNb));
    bool isInputOptional_delegate(int inputNb) const { return OfxEffectInstance::isInputOptional(inputNb); }
    
    
    
    MOCK_METHOD2(getRegionOfDefinition,Natron::Status(SequenceTime time,RectI* rod));
    Natron::Status getRegionOfDefinition_delegate(SequenceTime time,RectI* rod) { return OfxEffectInstance::getRegionOfDefinition(time,rod); }
    
    
    
    MOCK_METHOD3(getRegionOfInterest,Natron::EffectInstance::RoIMap(SequenceTime time,RenderScale scale,const RectI& renderWindow));
    Natron::EffectInstance::RoIMap getRegionOfInterest_delegate(SequenceTime time,RenderScale scale,const RectI& renderWindow) {
        return OfxEffectInstance::getRegionOfInterest(time,scale,renderWindow);
    }
    
    
    
    MOCK_METHOD2(getFrameRange,void(SequenceTime* first,SequenceTime* last));
    void getFrameRange_delegate(SequenceTime* first,SequenceTime* last) {
        OfxEffectInstance::getFrameRange(first,last);
    }
    
    
    
    MOCK_METHOD1(beginKnobsValuesChanged,void(Natron::ValueChangedReason reason));
    void beginKnobsValuesChanged_delegate(Natron::ValueChangedReason reason) {
        OfxEffectInstance::beginKnobsValuesChanged(reason);
    }
    
    
    
    MOCK_METHOD1(endKnobsValuesChanged,void(Natron::ValueChangedReason reason));
    void endKnobsValuesChanged_delegate(Natron::ValueChangedReason reason) {
        OfxEffectInstance::endKnobsValuesChanged(reason);
    }
    
    
    
    MOCK_METHOD2(onKnobValueChanged,void(Knob* k,Natron::ValueChangedReason reason));
    void onKnobValueChanged_delegate(Knob* k,Natron::ValueChangedReason reason) {
        OfxEffectInstance::onKnobValueChanged(k,reason);
    }
    
    
    
    MOCK_METHOD5(render,Natron::Status(SequenceTime time,RenderScale scale,
                                       const RectI& roi,int view,boost::shared_ptr<Natron::Image> output));
    Natron::Status render_delegate(SequenceTime time,RenderScale scale,
                         const RectI& roi,int view,boost::shared_ptr<Natron::Image> output) {
        return OfxEffectInstance::render(time,scale,roi,view,output);
    }
    
    
    
    MOCK_METHOD6(isIdentity,bool(SequenceTime time,RenderScale scale,const RectI& roi,
                                 int view,SequenceTime* inputTime,int* inputNb));
    bool isIdentity_delegate(SequenceTime time,RenderScale scale,const RectI& roi,
                             int view,SequenceTime* inputTime,int* inputNb) {
        return OfxEffectInstance::isIdentity(time,scale,roi,view,inputTime,inputNb);
    }
    
    
};

#endif // OFXEFFECTINSTANCE_MOCK_H

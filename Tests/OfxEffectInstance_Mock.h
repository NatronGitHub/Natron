/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier
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

#ifndef OFXEFFECTINSTANCE_MOCK_H
#define OFXEFFECTINSTANCE_MOCK_H

#include <gmock/gmock.h>

///https://code.google.com/p/googlemock/wiki/V1_7_CookBook#Delegating_Calls_to_a_Parent_Class

class OfxEffectInstance_Mock
    : public OfxEffectInstance
{
public:

    OfxEffectInstance_Mock(Natron::Node* node);

    virtual ~OfxEffectInstance_Mock()
    {
    }

    MOCK_CONST_METHOD1( isInputOptional,bool(int inputNb) );
    bool isInputOptional_delegate(int inputNb) const
    {
        return OfxEffectInstance::isInputOptional(inputNb);
    }

    MOCK_METHOD2( getRegionOfDefinition,Natron::Status(SequenceTime time,RectI * rod) );
    Natron::Status getRegionOfDefinition_delegate(SequenceTime time,
                                                  RectI* rod)
    {
        return OfxEffectInstance::getRegionOfDefinition(time,rod);
    }

    MOCK_METHOD3( getRegionOfInterest,Natron::EffectInstance::RoIMap(SequenceTime time,RenderScale scale,const RectI &renderWindow) );
    Natron::EffectInstance::RoIMap getRegionOfInterest_delegate(SequenceTime time,
                                                                RenderScale scale,
                                                                const RectI & renderWindow)
    {
        return OfxEffectInstance::getRegionOfInterest(time,scale,renderWindow);
    }

    MOCK_METHOD2( getFrameRange,void(SequenceTime * first,SequenceTime * last) );
    void getFrameRange_delegate(SequenceTime* first,
                                SequenceTime* last)
    {
        OfxEffectInstance::getFrameRange(first,last);
    }

    MOCK_METHOD1( beginKnobsValuesChanged,void(Natron::ValueChangedReason reason) );
    void beginKnobsValuesChanged_delegate(Natron::ValueChangedReason reason)
    {
        OfxEffectInstance::beginKnobsValuesChanged(reason);
    }

    MOCK_METHOD1( endKnobsValuesChanged,void(Natron::ValueChangedReason reason) );
    void endKnobsValuesChanged_delegate(Natron::ValueChangedReason reason)
    {
        OfxEffectInstance::endKnobsValuesChanged(reason);
    }

    MOCK_METHOD2( onKnobValueChanged,void(Knob * k,Natron::ValueChangedReason reason) );
    void onKnobValueChanged_delegate(Knob* k,
                                     Natron::ValueChangedReason reason)
    {
        OfxEffectInstance::onKnobValueChanged(k,reason);
    }

    MOCK_METHOD5( render,Natron::Status(SequenceTime time,RenderScale scale,
                                        const RectI &roi,int view,boost::shared_ptr<Natron::Image> output) );
    Natron::Status render_delegate(SequenceTime time,
                                   RenderScale scale,
                                   const RectI & roi,
                                   int view,
                                   boost::shared_ptr<Natron::Image> output)
    {
        return OfxEffectInstance::render(time,scale,roi,view,output);
    }

    MOCK_METHOD6( isIdentity,bool(SequenceTime time,RenderScale scale,const RectI &roi,
                                  int view,SequenceTime * inputTime,int* inputNb) );
    bool isIdentity_delegate(SequenceTime time,
                             RenderScale scale,
                             const RectI & roi,
                             int view,
                             SequenceTime* inputTime,
                             int* inputNb)
    {
        return OfxEffectInstance::isIdentity(time,scale,roi,view,inputTime,inputNb);
    }
};

#endif // OFXEFFECTINSTANCE_MOCK_H

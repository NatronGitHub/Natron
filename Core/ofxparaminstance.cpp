//  Powiter
//
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 *Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 *contact: immarespond at gmail dot com
 *
 */

#include "ofxparaminstance.h"
#include "Gui/knob.h"
#include "Gui/knob_callback.h"
#include "Core/ofxnode.h"

OfxPushButtonInstance::OfxPushButtonInstance(OfxNode* effect,
                                           const std::string& name,
                                           OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::PushbuttonInstance(descriptor)
{
    Knob_Callback* cb = _effect->getKnobCallBack();
    Button_Knob* knob = dynamic_cast<Button_Knob*>(KnobFactory::createKnob("Button", cb, name, Knob::NONE));
    _effect->createKnobDynamically();
}

OfxIntegerInstance::OfxIntegerInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::IntegerInstance(descriptor){
    
}
OfxStatus OfxIntegerInstance::get(int&){
    
}
OfxStatus OfxIntegerInstance::get(OfxTime time, int&){
    
}
OfxStatus OfxIntegerInstance::set(int){
    
}
OfxStatus OfxIntegerInstance::set(OfxTime time, int){
    
}


OfxDoubleInstance::OfxDoubleInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::DoubleInstance(descriptor){
    
}
OfxStatus OfxDoubleInstance::get(double&){
    
}
OfxStatus OfxDoubleInstance::get(OfxTime time, double&){
    
}
OfxStatus OfxDoubleInstance::set(double){
    
}
OfxStatus OfxDoubleInstance::set(OfxTime time, double){
    
}
OfxStatus OfxDoubleInstance::derive(OfxTime time, double&){
    
}
OfxStatus OfxDoubleInstance::integrate(OfxTime time1, OfxTime time2, double&){
    
}

OfxBooleanInstance::OfxBooleanInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::BooleanInstance(descriptor){
    
}
OfxStatus OfxBooleanInstance::get(bool&){
    
}
OfxStatus OfxBooleanInstance::get(OfxTime time, bool&){
    
}
OfxStatus OfxBooleanInstance::set(bool){
    
}
OfxStatus OfxBooleanInstance::set(OfxTime time, bool){
    
}


OfxChoiceInstance::OfxChoiceInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::ChoiceInstance(descriptor){
    
}
OfxStatus OfxChoiceInstance::get(int&){
    
}
OfxStatus OfxChoiceInstance::get(OfxTime time, int&){
    
}
OfxStatus OfxChoiceInstance::set(int){
    
}
OfxStatus OfxChoiceInstance::set(OfxTime time, int){
    
}


OfxRGBAInstance::OfxRGBAInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::RGBAInstance(descriptor){
    
}
OfxStatus OfxRGBAInstance::get(double&,double&,double&,double&){
    
}
OfxStatus OfxRGBAInstance::get(OfxTime time, double&,double&,double&,double&){
    
}
OfxStatus OfxRGBAInstance::set(double,double,double,double){
    
}
OfxStatus OfxRGBAInstance::set(OfxTime time, double,double,double,double){
    
}


OfxRGBInstance::OfxRGBInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::RGBInstance(descriptor){
    
}
OfxStatus OfxRGBInstance::get(double&,double&,double&){
    
}
OfxStatus OfxRGBInstance::get(OfxTime time, double&,double&,double&){
    
}
OfxStatus OfxRGBInstance::set(double,double,double){
    
}
OfxStatus OfxRGBInstance::set(OfxTime time, double,double,double){
    
}


OfxDouble2DInstance::OfxDouble2DInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::Double2DInstance(descriptor){
    
}
OfxStatus OfxDouble2DInstance::get(double&,double&){
    
}
OfxStatus OfxDouble2DInstance::get(OfxTime time,double&,double&){
    
}
OfxStatus OfxDouble2DInstance::set(double,double){
    
}
OfxStatus OfxDouble2DInstance::set(OfxTime time,double,double){
    
}


OfxInteger2DInstance::OfxInteger2DInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: _effect(effect), _descriptor(descriptor), OFX::Host::Param::Integer2DInstance(descriptor){
    
}
OfxStatus OfxInteger2DInstance::get(int&,int&){
    
}
OfxStatus OfxInteger2DInstance::get(OfxTime time,int&,int&){
    
}
OfxStatus OfxInteger2DInstance::set(int,int){
    
}
OfxStatus OfxInteger2DInstance::set(OfxTime time,int,int){
    
}

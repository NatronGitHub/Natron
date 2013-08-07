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
:  OFX::Host::Param::PushbuttonInstance(descriptor),_effect(effect), _descriptor(descriptor)
{
    Knob_Callback* cb = _effect->getKnobCallBack();
    Button_Knob* knob = dynamic_cast<Button_Knob*>(KnobFactory::createKnob("Button", cb, name, Knob::NONE));
    knob->connectButtonToSlot(this, SLOT(emitInstanceChanged()));
    QObject::connect(this, SIGNAL(buttonPressed(QString)), _effect, SLOT(onInstanceChangedAction(QString)));
}

void OfxPushButtonInstance::emitInstanceChanged(){
    emit buttonPressed(getName().c_str());
}

OfxIntegerInstance::OfxIntegerInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::IntegerInstance(descriptor),_effect(effect), _descriptor(descriptor){
    Knob_Callback* cb = _effect->getKnobCallBack();
    Int_Knob* knob = dynamic_cast<Int_Knob*>(KnobFactory::createKnob("Int", cb, name, Knob::NONE));
    knob->setPointer(&_value);
}
OfxStatus OfxIntegerInstance::get(int& v){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::get(OfxTime time, int& v){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(int v){
    _value = v;
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(OfxTime time, int v){
    _value = v;
    return kOfxStatOK;
}


OfxDoubleInstance::OfxDoubleInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::DoubleInstance(descriptor), _effect(effect), _descriptor(descriptor){
    Knob_Callback* cb = _effect->getKnobCallBack();
    Double_Knob* knob = dynamic_cast<Double_Knob*>(KnobFactory::createKnob("Double", cb, name, Knob::NONE));
    knob->setPointer(&_value);

}
OfxStatus OfxDoubleInstance::get(double& v){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::get(OfxTime time, double& v){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(double v ){
    v = _value;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(OfxTime time, double v){
    _value = v;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::derive(OfxTime time, double& v){
    return kOfxStatErrMissingHostFeature;
}
OfxStatus OfxDoubleInstance::integrate(OfxTime time1, OfxTime time2, double& v){
    return kOfxStatErrMissingHostFeature;
}

OfxBooleanInstance::OfxBooleanInstance(OfxNode* effect, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::BooleanInstance(descriptor), _effect(effect), _descriptor(descriptor){
    Knob_Callback* cb = _effect->getKnobCallBack();
    Bool_Knob* knob = dynamic_cast<Bool_Knob*>(KnobFactory::createKnob("Bool", cb, name, Knob::NONE));
    knob->setPointer(&_value);
}
OfxStatus OfxBooleanInstance::get(bool& b){
    b = _value;
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::get(OfxTime time, bool& b){
    b = _value;
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::set(bool b){
    _value = b;
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::set(OfxTime time, bool b){
    _value = b;
    return kOfxStatOK;
}


OfxChoiceInstance::OfxChoiceInstance(OfxNode* effect,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::ChoiceInstance(descriptor), _effect(effect), _descriptor(descriptor) {
    Knob_Callback* cb = _effect->getKnobCallBack();
    ComboBox_Knob* knob = dynamic_cast<ComboBox_Knob*>(KnobFactory::createKnob("ComboBox", cb, name, Knob::NONE));
    
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
:OFX::Host::Param::RGBAInstance(descriptor), _effect(effect), _descriptor(descriptor){
    
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
:OFX::Host::Param::RGBInstance(descriptor), _effect(effect), _descriptor(descriptor){
    
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
:OFX::Host::Param::Double2DInstance(descriptor), _effect(effect), _descriptor(descriptor){
    
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
:OFX::Host::Param::Integer2DInstance(descriptor), _effect(effect), _descriptor(descriptor){
    
}
OfxStatus OfxInteger2DInstance::get(int&,int&){
    
}
OfxStatus OfxInteger2DInstance::get(OfxTime time,int&,int&){
    
}
OfxStatus OfxInteger2DInstance::set(int,int){
    
}
OfxStatus OfxInteger2DInstance::set(OfxTime time,int,int){
    
}

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

#include "OfxParamInstance.h"

#include <iostream>
#include <QColor>
#include <QHBoxLayout>
//ofx extension
#include <nuke/fnPublicOfxExtensions.h>

#include "Engine/Knob.h"
#include "Engine/OfxNode.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerNode.h"
#include "Engine/Model.h"

using namespace std;
using namespace Powiter;

OfxPushButtonInstance::OfxPushButtonInstance(OfxNode* node,
                                             const std::string& name,
                                             OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::PushbuttonInstance(descriptor, node->effectInstance())
, _node(node)
, _descriptor(descriptor)
{
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    _knob = dynamic_cast<Button_Knob*>(appPTR->getKnobFactory()->createKnob("Button", node, name,1, Knob::NONE));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
}


// callback which should set enabled state as appropriate
void OfxPushButtonInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxPushButtonInstance::setSecret(){
    _knob->setVisible(!getSecret());
}

Knob* OfxPushButtonInstance::getKnob() const {
    return _knob;
}

void OfxPushButtonInstance::triggerInstanceChanged() {
    _node->onInstanceChanged(_paramName);
    ViewerNode* viewer = Node::hasViewerConnected(_node);
    if(viewer){
        _node->getModel()->updateDAG(viewer,false);
    }else if(_node->isOutputNode()){
        OutputNode* n = dynamic_cast<OutputNode*>(_node);
        n->updateDAG(false);
        n->getVideoEngine()->startEngine(-1);
    }
}

OfxIntegerInstance::OfxIntegerInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::IntegerInstance(descriptor, node->effectInstance())
, _node(node)
, _descriptor(descriptor)
, _paramName(name)
{
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    _knob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory()->createKnob("Int", node, name,1, Knob::NONE));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    int min = getProperties().getIntProperty(kOfxParamPropDisplayMin);
    int max = getProperties().getIntProperty(kOfxParamPropDisplayMax);
    int incr = getProperties().getIntProperty(kOfxParamPropIncrement);
    int def = getProperties().getIntProperty(kOfxParamPropDefault);
    _knob->setMinimum(min);
    if(incr > 0)
        _knob->setIncrement(incr);
    _knob->setMaximum(max);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    set(def);
}
OfxStatus OfxIntegerInstance::get(int& v) {
    v = _knob->getValues()[0];
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::get(OfxTime /*time*/, int& v) {
    v = _knob->getValues()[0];
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(int v){
    _knob->setValue(v);
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(OfxTime /*time*/, int v){
    _knob->setValue(v);
    return kOfxStatOK;
}
void OfxIntegerInstance::triggerInstanceChanged(){
    _node->onInstanceChanged(_paramName);
}
// callback which should set enabled state as appropriate
void OfxIntegerInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxIntegerInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
Knob* OfxIntegerInstance::getKnob() const{
    return _knob;
}

OfxDoubleInstance::OfxDoubleInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::DoubleInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    _knob = dynamic_cast<Double_Knob*>(appPTR->getKnobFactory()->createKnob("Double", node, name,1, Knob::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    double min = getProperties().getDoubleProperty(kOfxParamPropDisplayMin);
    double max = getProperties().getDoubleProperty(kOfxParamPropDisplayMax);
    double incr = getProperties().getDoubleProperty(kOfxParamPropIncrement);
    double def = getProperties().getDoubleProperty(kOfxParamPropDefault);
    int decimals = getProperties().getIntProperty(kOfxParamPropDigits);
    _knob->setMinimum(min);
    _knob->setMaximum(max);
    if(incr > 0)
        _knob->setIncrement(incr);
    if(decimals > 0)
        _knob->setDecimals(decimals);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    set(def);
    
}
OfxStatus OfxDoubleInstance::get(double& v){
    v = _knob->getValues()[0];
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::get(OfxTime /*time*/, double& v){
    v = _knob->getValues()[0];
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(double v) {
    _knob->setValue(v);
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(OfxTime /*time*/, double v){
    _knob->setValue(v);
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::derive(OfxTime /*time*/, double& /*v*/){
    return kOfxStatErrMissingHostFeature;
}
OfxStatus OfxDoubleInstance::integrate(OfxTime /*time1*/, OfxTime /*time2*/, double& /*v*/){
    return kOfxStatErrMissingHostFeature;
}
void OfxDoubleInstance::triggerInstanceChanged(){
    _node->onInstanceChanged(_paramName);
}
// callback which should set enabled state as appropriate
void OfxDoubleInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDoubleInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
Knob* OfxDoubleInstance::getKnob() const{
    return _knob;
}

OfxBooleanInstance::OfxBooleanInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::BooleanInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    _knob = dynamic_cast<Bool_Knob*>(appPTR->getKnobFactory()->createKnob("Bool", node, name, 1,Knob::NONE));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    int def = getProperties().getIntProperty(kOfxParamPropDefault);
    set((bool)def);
    
}
OfxStatus OfxBooleanInstance::get(bool& b){
    b = _knob->getValue();
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::get(OfxTime /*time*/, bool& b) {
    b = _knob->getValue();
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::set(bool b){
    _knob->setValue(b);
    return kOfxStatOK;
}

OfxStatus OfxBooleanInstance::set(OfxTime /*time*/, bool b){
    _knob->setValue(b);
    return kOfxStatOK;
}

void OfxBooleanInstance::triggerInstanceChanged() {
   _node->onInstanceChanged(_paramName);
}

// callback which should set enabled state as appropriate
void OfxBooleanInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxBooleanInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
Knob* OfxBooleanInstance::getKnob() const{
    return _knob;
}


OfxChoiceInstance::OfxChoiceInstance(OfxNode *node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::ChoiceInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name) {
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    _knob = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory()->createKnob("ComboBox", node, name,1, Knob::NONE));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    OFX::Host::Property::Set& pSet = getProperties();
    for (int i = 0 ; i < pSet.getDimension(kOfxParamPropChoiceOption) ; ++i) {
        std::string str = pSet.getStringProperty(kOfxParamPropChoiceOption,i);
        _entries.push_back(str);
    }
    _knob->populate(_entries);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    int def = pSet.getIntProperty(kOfxParamPropDefault);    
    set(def);
}
OfxStatus OfxChoiceInstance::get(int& v){
    v = _knob->getActiveEntry();
    return kOfxStatOK;
}
OfxStatus OfxChoiceInstance::get(OfxTime /*time*/, int& v){
    v = _knob->getActiveEntry();
    return kOfxStatOK;
}
OfxStatus OfxChoiceInstance::set(int v){
    if(v < (int)_entries.size()){
        _knob->setValue(v);
        return kOfxStatOK;
    }else{
        return kOfxStatErrBadIndex;
    }
}
OfxStatus OfxChoiceInstance::set(OfxTime /*time*/, int v){
    if(v < (int)_entries.size()){
        _knob->setValue(v);
        return kOfxStatOK;
    }else{
        return kOfxStatErrBadIndex;
    }
}

void OfxChoiceInstance::triggerInstanceChanged() {
   _node->onInstanceChanged(_paramName);
}


// callback which should set enabled state as appropriate
void OfxChoiceInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxChoiceInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
Knob* OfxChoiceInstance::getKnob() const{
    return _knob;
}



OfxRGBAInstance::OfxRGBAInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::RGBAInstance(descriptor,node->effectInstance()),
_node(node),
_descriptor(descriptor),
_paramName(name){
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name, 1,Knob::NONE);
    }
    _knob = dynamic_cast<RGBA_Knob*>(appPTR->getKnobFactory()->createKnob("RGBA", node, name, 1,Knob::NONE));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    double defR = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    double defG = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    double defB = getProperties().getDoubleProperty(kOfxParamPropDefault,2);
    double defA = getProperties().getDoubleProperty(kOfxParamPropDefault,3);
    set(defR, defG, defB, defA);
}
OfxStatus OfxRGBAInstance::get(double& r, double& g, double& b, double& a) {
  
    QVector4D _color = _knob->getValues();
    r = _color.x();
    g = _color.y();
    b = _color.z();
    a = _color.w();
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::get(OfxTime /*time*/, double&r ,double& g, double& b, double& a) {
    QVector4D _color = _knob->getValues();
    r = _color.x();
    g = _color.y();
    b = _color.z();
    a = _color.w();
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::set(double r,double g , double b ,double a){
    QVector4D _color;
    _color.setX(r);
    _color.setY(g);
    _color.setZ(b);
    _color.setW(a);
    _knob->setValue(_color);
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::set(OfxTime /*time*/, double r ,double g,double b,double a){
    QVector4D _color;
    _color.setX(r);
    _color.setY(g);
    _color.setZ(b);
    _color.setW(a);
    _knob->setValue(_color);
    return kOfxStatOK;
}

void OfxRGBAInstance::triggerInstanceChanged() {
    _node->onInstanceChanged(_paramName);
}

// callback which should set enabled state as appropriate
void OfxRGBAInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxRGBAInstance::setSecret(){
    _knob->setVisible(!getSecret());
}


Knob* OfxRGBAInstance::getKnob() const{
    return _knob;
}
OfxRGBInstance::OfxRGBInstance(OfxNode *node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::RGBInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name, 1,Knob::NONE);
    }
    _knob = dynamic_cast<RGBA_Knob*>(appPTR->getKnobFactory()->createKnob("RGBA", node, name,1, Knob::NO_ALPHA));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    double defR = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    double defG = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    double defB = getProperties().getDoubleProperty(kOfxParamPropDefault,2);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    set(defR, defG, defB);
}
OfxStatus OfxRGBInstance::get(double& r, double& g, double& b) {
    QVector4D _color = _knob->getValues();
    r = _color.x();
    g = _color.y();
    b = _color.z();
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::get(OfxTime /*time*/, double& r, double& g, double& b) {
    QVector4D _color = _knob->getValues();
    r = _color.x();
    g = _color.y();
    b = _color.z();
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::set(double r,double g,double b){
    QVector4D _color;
	_color.setX(r);
    _color.setY(g);
    _color.setZ(b);
    _color.setW(1.0);
    _knob->setValue(_color);
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::set(OfxTime /*time*/, double r,double g,double b){
    QVector4D _color;
	_color.setX(r);
    _color.setY(g);
    _color.setZ(b);
    _color.setW(1.0);
    _knob->setValue(_color);
    return kOfxStatOK;
}

void OfxRGBInstance::triggerInstanceChanged() {
    _node->onInstanceChanged(_paramName);
}
// callback which should set enabled state as appropriate
void OfxRGBInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxRGBInstance::setSecret(){
    _knob->setVisible(!getSecret());
}

Knob* OfxRGBInstance::getKnob() const{
    return _knob;
}

OfxDouble2DInstance::OfxDouble2DInstance(OfxNode *node, const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::Double2DInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name, 1,Knob::NONE);
    }
    _knob = dynamic_cast<Double_Knob*>(appPTR->getKnobFactory()->createKnob("Double", node, name,2, Knob::NONE));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    vector<double> minimum;
    vector<double> maximum;
    vector<double> increment;
    vector<int> decimals;
    double def[2];
    minimum.push_back(getProperties().getDoubleProperty(kOfxParamPropDisplayMin,0));
    maximum.push_back(getProperties().getDoubleProperty(kOfxParamPropDisplayMax,0));
    increment.push_back(getProperties().getDoubleProperty(kOfxParamPropIncrement,0));
    decimals.push_back(getProperties().getIntProperty(kOfxParamPropDigits,0));
    def[0] = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    
    minimum.push_back(getProperties().getDoubleProperty(kOfxParamPropDisplayMin,1));
    maximum.push_back(getProperties().getDoubleProperty(kOfxParamPropDisplayMax,1));
    increment.push_back(getProperties().getDoubleProperty(kOfxParamPropIncrement,1));
    decimals.push_back(getProperties().getIntProperty(kOfxParamPropDigits,1));
    def[1] = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    _knob->setMinimumsAndMaximums(minimum, maximum);
    _knob->setIncrement(increment);
    _knob->setDecimals(decimals);
    _knob->setValue<double>(def,2);
    
}
OfxStatus OfxDouble2DInstance::get(double& x1, double& x2) {
    const vector<double>& _values = _knob->getValues();
    x1 = _values[0];
    x2 = _values[1];
    return kOfxStatOK;
}
OfxStatus OfxDouble2DInstance::get(OfxTime /*time*/, double& x1, double& x2) {
    const vector<double>& _values = _knob->getValues();
    x1 = _values[0];
    x2 = _values[1];
    return kOfxStatOK;
}
OfxStatus OfxDouble2DInstance::set(double x1,double x2){
    double _values[2];
	_values[0] = x1;
    _values[1] = x2;
    _knob->setValue<double>(_values,2);
	return kOfxStatOK;
}
OfxStatus OfxDouble2DInstance::set(OfxTime /*time*/,double x1,double x2){
    double _values[2];
	_values[0] = x1;
    _values[1] = x2;
    _knob->setValue<double>(_values,2);
	return kOfxStatOK;
}

void OfxDouble2DInstance::triggerInstanceChanged() {
   _node->onInstanceChanged(_paramName);
}

// callback which should set enabled state as appropriate
void OfxDouble2DInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDouble2DInstance::setSecret(){
    _knob->setVisible(!getSecret());
}

Knob* OfxDouble2DInstance::getKnob() const{
    return _knob;
}

OfxInteger2DInstance::OfxInteger2DInstance(OfxNode *node,  const std::string& name, OFX::Host::Param::Descriptor& descriptor)
:OFX::Host::Param::Integer2DInstance(descriptor,node->effectInstance()), _node(node), _descriptor(descriptor),_paramName(name){
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    _knob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory()->createKnob("Int", node, name, 2,Knob::NONE));
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
        _knob->turnOffUndoRedo();
    }
    const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
    _knob->setHintToolTip(hint);
    _knob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    vector<int> minimum;
    vector<int> maximum;
    vector<int> increment;
    int def[2];
    minimum.push_back(getProperties().getIntProperty(kOfxParamPropDisplayMin,0));
    maximum.push_back(getProperties().getIntProperty(kOfxParamPropDisplayMax,0));
    increment.push_back(getProperties().getIntProperty(kOfxParamPropIncrement,0));
    def[0] = getProperties().getIntProperty(kOfxParamPropDefault,0);
    
    minimum.push_back(getProperties().getIntProperty(kOfxParamPropDisplayMin,1));
    maximum.push_back(getProperties().getIntProperty(kOfxParamPropDisplayMax,1));
    increment.push_back(getProperties().getIntProperty(kOfxParamPropIncrement,1));
    def[1] = getProperties().getIntProperty(kOfxParamPropDefault,1);
    _knob->setMinimumsAndMaximums(minimum, maximum);
    _knob->setIncrement(increment);
    _knob->setValue<int>(def,2);
}
OfxStatus OfxInteger2DInstance::get(int& x1, int& x2) {
    const vector<int>& _values = _knob->getValues();
    x1 = _values[0];
    x2 = _values[1];
    return kOfxStatOK;
}
OfxStatus OfxInteger2DInstance::get(OfxTime /*time*/, int& x1, int& x2) {
    const vector<int>& _values = _knob->getValues();
    x1 = _values[0];
    x2 = _values[1];
    return kOfxStatOK;
}
OfxStatus OfxInteger2DInstance::set(int x1,int x2){
    int _values[2];
	_values[0] = x1;
    _values[1] = x2;
   _knob->setValue<int>(_values,2);
	return kOfxStatOK;
}
OfxStatus OfxInteger2DInstance::set(OfxTime /*time*/, int x1, int x2) {
    int _values[2];
	_values[0] = x1;
    _values[1] = x2;
    _knob->setValue<int>(_values,2);
	return kOfxStatOK;
}

void OfxInteger2DInstance::triggerInstanceChanged() {
    _node->onInstanceChanged(_paramName);
}

// callback which should set enabled state as appropriate
void OfxInteger2DInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxInteger2DInstance::setSecret(){
    _knob->setVisible(!getSecret());
}
Knob* OfxInteger2DInstance::getKnob() const{
    return _knob;
}


/***********/
OfxGroupInstance::OfxGroupInstance(OfxNode *node,const std::string& name,OFX::Host::Param::Descriptor& descriptor):
OFX::Host::Param::GroupInstance(descriptor,node->effectInstance()),_node(node),_descriptor(descriptor),_paramName(name){
    int isTab = getProperties().getIntProperty(kFnOfxParamPropGroupIsTab);
    if(isTab){
        Tab_Knob* _tabKnob = _node->getTabKnob();
        if(!_tabKnob){
            _tabKnob = dynamic_cast<Tab_Knob*>(appPTR->getKnobFactory()->createKnob("Tab", node, name,1, Knob::NONE));
            _node->setTabKnob(_tabKnob);
        }
        _groupKnob = 0;
        _tabKnob->addTab(name);
        if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
            _tabKnob->turnOffUndoRedo();
        }
        const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
        _tabKnob->setHintToolTip(hint);
        _tabKnob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    }else{
        _groupKnob = dynamic_cast<Group_Knob*>(appPTR->getKnobFactory()->createKnob("Group", node, name,1, Knob::NONE));
        int opened = getProperties().getIntProperty(kOfxParamPropGroupOpen);
        _groupKnob->setValue((bool)opened);
        if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
            _groupKnob->turnOffUndoRedo();
        }
        const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
        _groupKnob->setHintToolTip(hint);
        _groupKnob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
    }
    
    
}
void OfxGroupInstance::addKnob(Knob *k) {
    if(_groupKnob){
        k->setParentKnob(_groupKnob);
        _groupKnob->addKnob(k);
    }else{
        k->setParentKnob(_node->getTabKnob());
        _node->getTabKnob()->addKnob(_paramName, k);
    }
}

Knob* OfxGroupInstance::getKnob() const{
    if(_groupKnob){
        return _groupKnob;
    }else{
        return _node->getTabKnob();
    }
}

OfxStringInstance::OfxStringInstance(OfxNode *node,const std::string& name,OFX::Host::Param::Descriptor& descriptor):
OFX::Host::Param::StringInstance(descriptor,node->effectInstance()),_node(node),_descriptor(descriptor),_paramName(name),
_fileKnob(0),_outputFileKnob(0){
    std::string mode = getProperties().getStringProperty(kOfxParamPropStringMode);
    int layoutHint = getProperties().getIntProperty(kOfxParamPropLayoutHint);
    if(layoutHint == 1){
        appPTR->getKnobFactory()->createKnob("Separator", node, name,1, Knob::NONE);
    }
    if(mode == kOfxParamStringIsFilePath){
        if(_node->isInputNode()){
            _fileKnob = dynamic_cast<File_Knob*>(appPTR->getKnobFactory()->createKnob("InputFile", node, name, 1,Knob::NONE));
            QObject::connect(_fileKnob, SIGNAL(filesSelected()), this, SLOT(triggerInstanceChanged()));
            QObject::connect(_fileKnob, SIGNAL(frameRangeChanged(int,int)), _node, SLOT(onFrameRangeChanged(int,int)));
            if(layoutHint == 2){
                _fileKnob->turnOffNewLine();
            }
            _fileKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
            if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
                _fileKnob->turnOffUndoRedo();
            }
            const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
            _fileKnob->setHintToolTip(hint);
            _fileKnob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
        }else{
            _node->setAsOutputNode(); // IMPORTANT ! 
            _outputFileKnob = dynamic_cast<OutputFile_Knob*>(appPTR->getKnobFactory()->createKnob("OutputFile", node, name,1, Knob::NONE));
            QObject::connect(_outputFileKnob, SIGNAL(filesSelected()), this, SLOT(triggerInstanceChanged()));
            if(layoutHint == 2){
                _outputFileKnob->turnOffNewLine();
            }
            _outputFileKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
            if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
                _outputFileKnob->turnOffUndoRedo();
            }
            const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
            _outputFileKnob->setHintToolTip(hint);
            _outputFileKnob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));

        }
    }else if(mode == kOfxParamStringIsSingleLine || mode == kOfxParamStringIsLabel){
        
        _stringKnob = dynamic_cast<String_Knob*>(appPTR->getKnobFactory()->createKnob("String", node, name,1, Knob::NONE));
        if(mode == kOfxParamStringIsLabel){
            _stringKnob->setEnabled(false);
        }
        QObject::connect(_stringKnob, SIGNAL(valueChangedByUser()), this, SLOT(triggerInstanceChanged()));
        if(layoutHint == 2){
            _stringKnob->turnOffNewLine();
        }
        _stringKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
        if(!getProperties().getIntProperty(kOfxParamPropCanUndo)){
            _stringKnob->turnOffUndoRedo();
        }
        const std::string& hint = getProperties().getStringProperty(kOfxParamPropHint);
        _stringKnob->setHintToolTip(hint);
        _stringKnob->setEnabled((bool)getProperties().getIntProperty(kOfxParamPropEnabled));
        _returnValue = getProperties().getStringProperty(kOfxParamPropDefault,1);
        set(_returnValue.c_str());
    }
    
    
}
OfxStatus OfxStringInstance::get(std::string &str) {
    assert(_node->effectInstance());
    if(_fileKnob){
        int currentFrame = _fileKnob->clampToRange((int)_node->effectInstance()->timeLineGetTime());
        str = _fileKnob->getRandomFrameName(currentFrame).toStdString();
    }else if(_outputFileKnob){
        str = filenameFromPattern((int)_node->effectInstance()->timeLineGetTime());
    }else{
        str = _stringKnob->getString();
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::get(OfxTime /*time*/, std::string& str) {
    assert(_node->effectInstance());
    if(_fileKnob){
        int currentFrame = _fileKnob->clampToRange((int)_node->effectInstance()->timeLineGetTime());
        str = _fileKnob->getRandomFrameName(currentFrame).toStdString();
    }else if(_outputFileKnob){
        str = filenameFromPattern((int)_node->effectInstance()->timeLineGetTime());
    }else{
        str = _stringKnob->getString();
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::set(const char* str) {
    _returnValue = str;
    if(_fileKnob){
        _fileKnob->setValue(_returnValue);
    }
    if(_outputFileKnob){
        _outputFileKnob->setValue(str);
    }
    if(_stringKnob){
        _stringKnob->setValue(str);
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::set(OfxTime /*time*/, const char* str) {
    _returnValue = str;
    if(_fileKnob){
        _fileKnob->setValue(_returnValue);
    }
    if(_outputFileKnob){
         _outputFileKnob->setValue(str);
    }
    if(_stringKnob){
        _stringKnob->setValue(str);
    }
    return kOfxStatOK;
}

Knob* OfxStringInstance::getKnob() const{
    
    if(_fileKnob){
        return _fileKnob;
    }
    if(_outputFileKnob){
        return _outputFileKnob;
    }
    if(_stringKnob){
        return _stringKnob;
    }
    return NULL;
}
// callback which should set enabled state as appropriate
void OfxStringInstance::setEnabled(){
    if(_fileKnob){
        _fileKnob->setEnabled(getEnabled());
    }
    if (_outputFileKnob) {
        _outputFileKnob->setEnabled(getEnabled());
    }
    if (_stringKnob) {
        _stringKnob->setEnabled(getEnabled());
    }
}

// callback which should set secret state as appropriate
void OfxStringInstance::setSecret(){
    if(_fileKnob){
        _fileKnob->setVisible(!getSecret());
    }
    if (_outputFileKnob) {
        _outputFileKnob->setVisible(getSecret());
    }
    if (_stringKnob) {
        _stringKnob->setVisible(getSecret());
    }
    
}

void OfxStringInstance::triggerInstanceChanged() {
    if (_fileKnob) {
         _node->computePreviewImage();
    }
    _node->onInstanceChanged(_paramName);
}

const QString OfxStringInstance::getRandomFrameName(int f) const{
    return _fileKnob ? _fileKnob->getRandomFrameName(f) : "";
}
bool OfxStringInstance::isValid() const{
    if(_fileKnob){
        return !_fileKnob->value<QStringList>().isEmpty();
    }
    if(_outputFileKnob){
        return !_outputFileKnob->value<QString>().toStdString().empty();
    }
    return true;
}
std::string OfxStringInstance::filenameFromPattern(int frameIndex) const{
    if(_outputFileKnob){
        std::string pattern = _outputFileKnob->value<QString>().toStdString();
        if(isValid()){
            QString p(pattern.c_str());
            return p.replace("#", QString::number(frameIndex)).toStdString();
        }
    }
    return "";
}
void OfxStringInstance::ifFileKnobPopDialog(){
    if(_fileKnob){
        _fileKnob->openFile();
    }else if(_outputFileKnob){
        _outputFileKnob->openFile();
    }
}

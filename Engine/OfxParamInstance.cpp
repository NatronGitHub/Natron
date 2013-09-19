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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
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

void OfxPushButtonInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    int min = getProperties().getIntProperty(kOfxParamPropDisplayMin);
    int max = getProperties().getIntProperty(kOfxParamPropDisplayMax);
    int def = getProperties().getIntProperty(kOfxParamPropDefault);
    _knob->setMinimum(min);
    _knob->setMaximum(max);
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
void OfxIntegerInstance::onInstanceChanged(){
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    double min = getProperties().getDoubleProperty(kOfxParamPropDisplayMin);
    double max = getProperties().getDoubleProperty(kOfxParamPropDisplayMax);
    double incr = getProperties().getDoubleProperty(kOfxParamPropIncrement);
    double def = getProperties().getDoubleProperty(kOfxParamPropDefault);
    _knob->setMinimum(min);
    _knob->setMaximum(max);
    _knob->setIncrement(incr);
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
void OfxDoubleInstance::onInstanceChanged(){
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
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

void OfxBooleanInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    OFX::Host::Property::Set& pSet = getProperties();
    for (int i = 0 ; i < pSet.getDimension(kOfxParamPropChoiceOption) ; ++i) {
        std::string str = pSet.getStringProperty(kOfxParamPropChoiceOption,i);
        _entries.push_back(str);
    }
    _knob->populate(_entries);
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

void OfxChoiceInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
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

void OfxRGBAInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    double defR = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    double defG = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    double defB = getProperties().getDoubleProperty(kOfxParamPropDefault,2);
    
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

void OfxRGBInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    double minimum[2];
    double maximum[2];
    double increment[2];
    double def[2];
    minimum[0] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,0);
    maximum[0] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,0);
    increment[0] = getProperties().getDoubleProperty(kOfxParamPropIncrement,0);
    def[0] = getProperties().getDoubleProperty(kOfxParamPropDefault,0);
    
    minimum[1] = getProperties().getDoubleProperty(kOfxParamPropDisplayMin,1);
    maximum[1] = getProperties().getDoubleProperty(kOfxParamPropDisplayMax,1);
    increment[1] = getProperties().getDoubleProperty(kOfxParamPropIncrement,1);
    def[1] = getProperties().getDoubleProperty(kOfxParamPropDefault,1);
    _knob->setMinimum<double>(minimum,2);
    _knob->setMaximum<double>(maximum,2);
    _knob->setIncrement<double>(increment,2);
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

void OfxDouble2DInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    QObject::connect(_knob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
    if(layoutHint == 2){
        _knob->turnOffNewLine();
    }
    _knob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
    
    int minimum[2];
    int maximum[2];
    int increment[2];
    int def[2];
    minimum[0] = getProperties().getIntProperty(kOfxParamPropDisplayMin,0);
    maximum[0] = getProperties().getIntProperty(kOfxParamPropDisplayMax,0);
    increment[0] = getProperties().getIntProperty(kOfxParamPropIncrement,0);
    def[0] = getProperties().getIntProperty(kOfxParamPropDefault,0);
    
    minimum[1] = getProperties().getIntProperty(kOfxParamPropDisplayMin,1);
    maximum[1] = getProperties().getIntProperty(kOfxParamPropDisplayMax,1);
    increment[1] = getProperties().getIntProperty(kOfxParamPropIncrement,1);
    def[1] = getProperties().getIntProperty(kOfxParamPropDefault,1);
    _knob->setMinimum<int>(minimum,2);
    _knob->setMaximum<int>(maximum,2);
    _knob->setIncrement<int>(increment,2);
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

void OfxInteger2DInstance::onInstanceChanged() {
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
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
    }else{
        _groupKnob = dynamic_cast<Group_Knob*>(appPTR->getKnobFactory()->createKnob("Group", node, name,1, Knob::NONE));
        int opened = getProperties().getIntProperty(kOfxParamPropGroupOpen);
        _groupKnob->setValue((bool)opened);
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
            QObject::connect(_fileKnob, SIGNAL(filesSelected()), this, SLOT(onInstanceChanged()));
            if(layoutHint == 2){
                _fileKnob->turnOffNewLine();
            }
            _fileKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
        }else{
            _node->setAsOutputNode(); // IMPORTANT ! 
            _outputFileKnob = dynamic_cast<OutputFile_Knob*>(appPTR->getKnobFactory()->createKnob("OutputFile", node, name,1, Knob::NONE));
            QObject::connect(_outputFileKnob, SIGNAL(filesSelected()), this, SLOT(onInstanceChanged()));
            if(layoutHint == 2){
                _outputFileKnob->turnOffNewLine();
            }
            _outputFileKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
        }
    }else if(mode == kOfxParamStringIsSingleLine || mode == kOfxParamStringIsLabel){
        
        _stringKnob = dynamic_cast<String_Knob*>(appPTR->getKnobFactory()->createKnob("String", node, name,1, Knob::NONE));
        if(mode == kOfxParamStringIsLabel){
            _stringKnob->setEnabled(false);
        }
        QObject::connect(_stringKnob, SIGNAL(valueChangedByUser()), this, SLOT(onInstanceChanged()));
        if(layoutHint == 2){
            _stringKnob->turnOffNewLine();
        }
        _stringKnob->setSpacingBetweenItems(getProperties().getIntProperty(kOfxParamPropLayoutPadWidth));
        _returnValue = getProperties().getStringProperty(kOfxParamPropDefault,1);
        set(_returnValue.c_str());
    }
    
}
OfxStatus OfxStringInstance::get(std::string &str) {
    assert(_node->effectInstance());
    if(_fileKnob){
        int currentFrame = clampToRange((int)_node->effectInstance()->timeLineGetTime());
        if(currentFrame != INT_MAX && currentFrame != INT_MIN){
            map<int,QString>::iterator it = _files.find(currentFrame);
            if(it!=_files.end()){
                str = it->second.toStdString();
            }else{
                str = "";
            }
        }else{
            str = "";
        }
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
        int currentFrame = clampToRange((int)_node->effectInstance()->timeLineGetTime());
        if(currentFrame != INT_MAX && currentFrame != INT_MIN){
            map<int,QString>::iterator it = _files.find(currentFrame);
            if(it!=_files.end()){
                str = it->second.toStdString();
            }else{
                str = "";
            }
        }else{
            str = "";
        }
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

void OfxStringInstance::onInstanceChanged() {
    if (_fileKnob) {
        getVideoSequenceFromFilesList();
        _returnValue = _fileKnob->value<QString>().toStdString();
         _node->computePreviewImage();

    }
    assert(_node->effectInstance());
    OfxStatus stat;
    stat = _node->effectInstance()->beginInstanceChangedAction(kOfxChangeUserEdited);
    if(stat == kOfxStatOK){
        OfxPointD renderScale;
        renderScale.x = renderScale.y = 1.0;
        stat = _node->effectInstance()->paramInstanceChangedAction(_paramName, kOfxChangeUserEdited, 1.0,renderScale);
        assert(stat == kOfxStatOK);
        stat = _node->effectInstance()->endInstanceChangedAction(kOfxChangeUserEdited);
        assert(stat == kOfxStatOK);
    }
}

void OfxStringInstance::getVideoSequenceFromFilesList(){
    _files.clear();
    QStringList _filesList = _fileKnob->value<QStringList>();
    bool first_time=true;
    QString originalName;
    foreach(QString Qfilename,_filesList)
    {	if(Qfilename.at(0) == QChar('.')) continue;
        QString const_qfilename=Qfilename;
        if(first_time){
            Qfilename=Qfilename.remove(Qfilename.length()-4,4);
            int j=Qfilename.length()-1;
            QString frameIndex;
            while(j>0 && Qfilename.at(j).isDigit()){
                frameIndex.push_front(Qfilename.at(j));
                --j;
            }
            if(j>0){
				int number=0;
                if(_filesList.size() > 1){
                    number = frameIndex.toInt();
                }
				_files.insert(make_pair(number,const_qfilename));
                originalName=Qfilename.remove(j+1,frameIndex.length());
                
            }else{
                _files[0]=const_qfilename;
            }
            first_time=false;
        }else{
            if(Qfilename.contains(originalName) /*&& (extension)*/){
                Qfilename.remove(Qfilename.length()-4,4);
                int j=Qfilename.length()-1;
                QString frameIndex;
                while(j>0 && Qfilename.at(j).isDigit()){
                    frameIndex.push_front(Qfilename.at(j));
                    --j;
                }
                if(j>0){
                    int number = frameIndex.toInt();
                    _files[number]=const_qfilename;
                }else{
                    cout << " Read handle : WARNING !! several frames read but no frame count found in their name " << endl;
                }
            }
        }
    }
    _node->set_firstFrame(firstFrame());
    _node->set_lastFrame(lastFrame());
    
}

int OfxStringInstance::firstFrame(){
    std::map<int,QString>::iterator it=_files.begin();
    if(it == _files.end()) return INT_MIN;
    return it->first;
}
int OfxStringInstance::lastFrame(){
    std::map<int,QString>::iterator it=_files.end();
    if(it == _files.begin()) return INT_MAX;
    --it;
    return it->first;
}
int OfxStringInstance::clampToRange(int f){
    int first = firstFrame();
    int last = lastFrame();
    if(f < first) return first;
    if(f > last) return last;
    return f;
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

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

#include "OfxParamInstance.h"
#include <iostream>
#include <QColor>
#include <QHBoxLayout>

//ofx extension
#include <nuke/fnPublicOfxExtensions.h>


#include "Global/AppManager.h"

#include "Engine/Knob.h"
#include "Engine/KnobFactory.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/OfxEffectInstance.h"
#include "Engine/OfxClipInstance.h"
#include "Engine/OfxImageEffectInstance.h"
#include "Engine/VideoEngine.h"
#include "Engine/ViewerInstance.h"

using namespace Natron;


static std::string getParamLabel(OFX::Host::Param::Instance* param){
    std::string label = param->getProperties().getStringProperty(kOfxPropLabel);
    if(label.empty()){
        label = param->getProperties().getStringProperty(kOfxPropShortLabel);
    }
    if(label.empty()){
        label = param->getProperties().getStringProperty(kOfxPropLongLabel);
    }
    if(label.empty()){
        label = param->getName();
    }
    return label;
}

OfxPushButtonInstance::OfxPushButtonInstance(OfxEffectInstance* node,
                                             OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::PushbuttonInstance(descriptor, node->effectInstance())
{
    _knob = dynamic_cast<Button_Knob*>(appPTR->getKnobFactory().createKnob("Button", node, getParamLabel(this)));
}


// callback which should set enabled state as appropriate
void OfxPushButtonInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxPushButtonInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxPushButtonInstance::getKnob() const {
    return _knob;
}



OfxIntegerInstance::OfxIntegerInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::IntegerInstance(descriptor, node->effectInstance())
{
    const OFX::Host::Property::Set &properties = getProperties();
    _knob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", node, getParamLabel(this)));

    int min = properties.getIntProperty(kOfxParamPropMin);
    int max = properties.getIntProperty(kOfxParamPropMax);
    int def = properties.getIntProperty(kOfxParamPropDefault);
    int displayMin = properties.getIntProperty(kOfxParamPropDisplayMin);
    int displayMax = properties.getIntProperty(kOfxParamPropDisplayMax);
    _knob->setDisplayMinimum(displayMin);
    _knob->setDisplayMaximum(displayMax);

    _knob->setMinimum(min);
    _knob->setIncrement(1); // kOfxParamPropIncrement only exists for Double
    _knob->setMaximum(max);
    set(def);
}
OfxStatus OfxIntegerInstance::get(int& v) {
    v = _knob->getValue<int>();
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::get(OfxTime time, int& v) {
    v = _knob->getValueAtTime<int>(time);
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(int v){
    _knob->setValue<int>(v);
    return kOfxStatOK;
}
OfxStatus OfxIntegerInstance::set(OfxTime time, int v){
    _knob->setValueAtTime<int>(time,v);
    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxIntegerInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxIntegerInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxIntegerInstance::getKnob() const{
    return _knob;
}

OfxDoubleInstance::OfxDoubleInstance(OfxEffectInstance* node,  OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::DoubleInstance(descriptor,node->effectInstance())
{
    const OFX::Host::Property::Set &properties = getProperties();

    _knob = dynamic_cast<Double_Knob*>(appPTR->getKnobFactory().createKnob("Double", node, getParamLabel(this)));

    double min = properties.getDoubleProperty(kOfxParamPropMin);
    double max = properties.getDoubleProperty(kOfxParamPropMax);
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    double def = properties.getDoubleProperty(kOfxParamPropDefault);
    int decimals = properties.getIntProperty(kOfxParamPropDigits);
    double displayMin = properties.getDoubleProperty(kOfxParamPropDisplayMin);
    double displayMax = properties.getDoubleProperty(kOfxParamPropDisplayMax);
    _knob->setDisplayMinimum(displayMin);
    _knob->setDisplayMaximum(displayMax);
    _knob->setMinimum(min);
    _knob->setMaximum(max);
    if(incr > 0) {
        _knob->setIncrement(incr);
    }
    if(decimals > 0) {
        _knob->setDecimals(decimals);
    }

    set(def);
    
}
OfxStatus OfxDoubleInstance::get(double& v){
    v = _knob->getValue<double>();
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::get(OfxTime time, double& v){
    v = _knob->getValueAtTime<double>(time);
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(double v) {
    _knob->setValue<double>(v);
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::set(OfxTime time, double v){
    _knob->setValueAtTime<double>(time,v);
    return kOfxStatOK;
}

OfxStatus OfxDoubleInstance::derive(OfxTime /*time*/, double& v) {
    v = 0;
    return kOfxStatOK;
}
OfxStatus OfxDoubleInstance::integrate(OfxTime time1, OfxTime time2, double& v) {
    v = _knob->getValue<double>() * (time2 - time1);
    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxDoubleInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDoubleInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxDoubleInstance::getKnob() const{
    return _knob;
}

OfxBooleanInstance::OfxBooleanInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::BooleanInstance(descriptor,node->effectInstance())
{
    const OFX::Host::Property::Set &properties = getProperties();

    _knob = dynamic_cast<Bool_Knob*>(appPTR->getKnobFactory().createKnob("Bool", node, getParamLabel(this)));
    int def = properties.getIntProperty(kOfxParamPropDefault);
    set((bool)def);
    
}
OfxStatus OfxBooleanInstance::get(bool& b){
    b = _knob->getValue<bool>();
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::get(OfxTime /*time*/, bool& b) {
    b = _knob->getValue<bool>();
    return kOfxStatOK;
}
OfxStatus OfxBooleanInstance::set(bool b){
    _knob->setValue<bool>(b);
    return kOfxStatOK;
}

OfxStatus OfxBooleanInstance::set(OfxTime /*time*/, bool b){
    _knob->setValue<bool>(b);
    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxBooleanInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxBooleanInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxBooleanInstance::getKnob() const{
    return _knob;
}


OfxChoiceInstance::OfxChoiceInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::ChoiceInstance(descriptor,node->effectInstance())
{
    const OFX::Host::Property::Set &properties = getProperties();


    _knob = dynamic_cast<ComboBox_Knob*>(appPTR->getKnobFactory().createKnob("ComboBox", node, getParamLabel(this)));

    std::vector<std::string> helpStrings;
    for (int i = 0 ; i < properties.getDimension(kOfxParamPropChoiceOption) ; ++i) {
        std::string str = properties.getStringProperty(kOfxParamPropChoiceOption,i);
        std::string help = properties.getStringProperty(kOfxParamPropChoiceLabelOption,i);

        _entries.push_back(str);
        helpStrings.push_back(help);
    }
    _knob->populate(_entries,helpStrings);

    int def = properties.getIntProperty(kOfxParamPropDefault);
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
        _knob->setValue<int>(v);
        return kOfxStatOK;
    }else{
        return kOfxStatErrBadIndex;
    }
}
OfxStatus OfxChoiceInstance::set(OfxTime /*time*/, int v){
    if(v < (int)_entries.size()){
        _knob->setValue<int>(v);
        return kOfxStatOK;
    }else{
        return kOfxStatErrBadIndex;
    }
}


// callback which should set enabled state as appropriate
void OfxChoiceInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxChoiceInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxChoiceInstance::getKnob() const{
    return _knob;
}



OfxRGBAInstance::OfxRGBAInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::RGBAInstance(descriptor,node->effectInstance())
{
    const OFX::Host::Property::Set &properties = getProperties();  
    _knob = dynamic_cast<Color_Knob*>(appPTR->getKnobFactory().createKnob("Color", node, getParamLabel(this),4));
    double defR = properties.getDoubleProperty(kOfxParamPropDefault,0);
    double defG = properties.getDoubleProperty(kOfxParamPropDefault,1);
    double defB = properties.getDoubleProperty(kOfxParamPropDefault,2);
    double defA = properties.getDoubleProperty(kOfxParamPropDefault,3);
    set(defR, defG, defB, defA);
}
OfxStatus OfxRGBAInstance::get(double& r, double& g, double& b, double& a) {
  
    r = _knob->getValue<double>(0);
    g = _knob->getValue<double>(1);
    b = _knob->getValue<double>(2);
    a = _knob->getValue<double>(3);
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::get(OfxTime time, double&r ,double& g, double& b, double& a) {
    r = _knob->getValueAtTime<double>(time,0);
    g = _knob->getValueAtTime<double>(time,1);
    b = _knob->getValueAtTime<double>(time,2);
    a = _knob->getValueAtTime<double>(time,3);
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::set(double r,double g , double b ,double a){
    _knob->setValue<double>(r,0);
    _knob->setValue<double>(g,1);
    _knob->setValue<double>(b,2);
    _knob->setValue<double>(a,3);
    return kOfxStatOK;
}
OfxStatus OfxRGBAInstance::set(OfxTime time, double r ,double g,double b,double a){
    _knob->setValueAtTime<double>(time,r,0);
    _knob->setValueAtTime<double>(time,g,1);
    _knob->setValueAtTime<double>(time,b,2);
    _knob->setValueAtTime<double>(time,a,3);
    return kOfxStatOK;
}


// callback which should set enabled state as appropriate
void OfxRGBAInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxRGBAInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxRGBAInstance::getKnob() const{
    return _knob;
}


OfxRGBInstance::OfxRGBInstance(OfxEffectInstance* node,  OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::RGBInstance(descriptor,node->effectInstance())
{
    const OFX::Host::Property::Set &properties = getProperties();

    _knob = dynamic_cast<Color_Knob*>(appPTR->getKnobFactory().createKnob("Color", node, getParamLabel(this),3));
    double defR = properties.getDoubleProperty(kOfxParamPropDefault,0);
    double defG = properties.getDoubleProperty(kOfxParamPropDefault,1);
    double defB = properties.getDoubleProperty(kOfxParamPropDefault,2);   
    set(defR, defG, defB);
}
OfxStatus OfxRGBInstance::get(double& r, double& g, double& b) {
    r = _knob->getValue<double>(0);
    g = _knob->getValue<double>(1);
    b = _knob->getValue<double>(2);
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::get(OfxTime time, double& r, double& g, double& b) {
    r = _knob->getValueAtTime<double>(time,0);
    g = _knob->getValueAtTime<double>(time,1);
    b = _knob->getValueAtTime<double>(time,2);
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::set(double r,double g,double b){
    _knob->setValue<double>(r,0);
    _knob->setValue<double>(g,1);
    _knob->setValue<double>(b,2);
    return kOfxStatOK;
}
OfxStatus OfxRGBInstance::set(OfxTime time, double r,double g,double b){
    _knob->setValueAtTime<double>(time,r,0);
    _knob->setValueAtTime<double>(time,g,1);
    _knob->setValueAtTime<double>(time,b,2);
    return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxRGBInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxRGBInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxRGBInstance::getKnob() const{
    return _knob;
}


OfxDouble2DInstance::OfxDouble2DInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::Double2DInstance(descriptor,node->effectInstance())
{
    const int dims = 2;
    const OFX::Host::Property::Set &properties = getProperties();


    _knob = dynamic_cast<Double_Knob*>(appPTR->getKnobFactory().createKnob("Double", node, getParamLabel(this),dims));

    std::vector<double> minimum(dims);
    std::vector<double> maximum(dims);
    std::vector<double> increment(dims);
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);
    std::vector<int> decimals(dims);
    boost::scoped_array<double> def(new double[dims]);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    int dig = properties.getIntProperty(kOfxParamPropDigits);
    for (int i=0; i < dims; ++i) {
        minimum[i] = properties.getDoubleProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getDoubleProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getDoubleProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getDoubleProperty(kOfxParamPropMax,i);
        increment[i] = incr;
        decimals[i] = dig;
        def[i] = properties.getDoubleProperty(kOfxParamPropDefault,i);
    }

    _knob->setMinimumsAndMaximums(minimum, maximum);
    _knob->setIncrement(increment);
    _knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    _knob->setDecimals(decimals);
    _knob->setValue<double>(def.get(),dims);
}

OfxStatus OfxDouble2DInstance::get(double& x1, double& x2) {
    x1 = _knob->getValue<double>(0);
    x2 = _knob->getValue<double>(1);
    return kOfxStatOK;
}

OfxStatus OfxDouble2DInstance::get(OfxTime time, double& x1, double& x2) {
    x1 = _knob->getValueAtTime<double>(time,0);
    x2 = _knob->getValueAtTime<double>(time,1);
    return kOfxStatOK;
}

OfxStatus OfxDouble2DInstance::set(double x1,double x2){
    _knob->setValue<double>(x1,0);
    _knob->setValue<double>(x2,1);
	return kOfxStatOK;
}

OfxStatus OfxDouble2DInstance::set(OfxTime time,double x1,double x2){
    _knob->setValueAtTime<double>(time,x1,0);
    _knob->setValueAtTime<double>(time,x2,1);
	return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxDouble2DInstance::setEnabled(){
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDouble2DInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxDouble2DInstance::getKnob() const{
    return _knob;
}


OfxInteger2DInstance::OfxInteger2DInstance(OfxEffectInstance *node, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::Integer2DInstance(descriptor,node->effectInstance())
{
    const int dims = 2;
    const OFX::Host::Property::Set &properties = getProperties();


    _knob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", node, getParamLabel(this), dims));

    std::vector<int> minimum(dims);
    std::vector<int> maximum(dims);
    std::vector<int> increment(dims);
    std::vector<int> displayMins(dims);
    std::vector<int> displayMaxs(dims);
    boost::scoped_array<int> def(new int[dims]);

    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getIntProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getIntProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getIntProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getIntProperty(kOfxParamPropMax,i);
        increment[i] = 1; // kOfxParamPropIncrement only exists for Double
        def[i] = properties.getIntProperty(kOfxParamPropDefault,i);
    }
    
    _knob->setMinimumsAndMaximums(minimum, maximum);
    _knob->setIncrement(increment);
    _knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    _knob->setValue<int>(def.get(),dims);
}

OfxStatus OfxInteger2DInstance::get(int& x1, int& x2) {
    x1 = _knob->getValue<int>(0);
    x2 = _knob->getValue<int>(1);
    return kOfxStatOK;
}

OfxStatus OfxInteger2DInstance::get(OfxTime time, int& x1, int& x2) {
    x1 = _knob->getValueAtTime<int>(time,0);
    x2 = _knob->getValueAtTime<int>(time,1);
    return kOfxStatOK;
}

OfxStatus OfxInteger2DInstance::set(int x1,int x2) {
    _knob->setValue<int>(x1,0);
    _knob->setValue<int>(x2,1);
	return kOfxStatOK;
}

OfxStatus OfxInteger2DInstance::set(OfxTime time, int x1, int x2) {
    _knob->setValueAtTime<int>(time,x1,0);
    _knob->setValueAtTime<int>(time,x2,1);
	return kOfxStatOK;
}


// callback which should set enabled state as appropriate
void OfxInteger2DInstance::setEnabled() {
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxInteger2DInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxInteger2DInstance::getKnob() const {
    return _knob;
}

OfxDouble3DInstance::OfxDouble3DInstance(OfxEffectInstance* node, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::Double3DInstance(descriptor,node->effectInstance())
{
    const int dims = 3;
    const OFX::Host::Property::Set &properties = getProperties();


    _knob = dynamic_cast<Double_Knob*>(appPTR->getKnobFactory().createKnob("Double", node, getParamLabel(this),dims));

    std::vector<double> minimum(dims);
    std::vector<double> maximum(dims);
    std::vector<double> increment(dims);
    std::vector<double> displayMins(dims);
    std::vector<double> displayMaxs(dims);
    std::vector<int> decimals(dims);
    boost::scoped_array<double> def(new double[dims]);

    // kOfxParamPropIncrement and kOfxParamPropDigits only have one dimension,
    // @see Descriptor::addNumericParamProps() in ofxhParam.cpp
    // @see gDoubleParamProps in ofxsPropertyValidation.cpp
    double incr = properties.getDoubleProperty(kOfxParamPropIncrement);
    int dig = properties.getIntProperty(kOfxParamPropDigits);
    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getDoubleProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getDoubleProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getDoubleProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getDoubleProperty(kOfxParamPropMax,i);
        increment[i] = incr;
        decimals[i] = dig;
        def[i] = properties.getDoubleProperty(kOfxParamPropDefault,i);
    }

    _knob->setMinimumsAndMaximums(minimum, maximum);
    _knob->setIncrement(increment);
    _knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    _knob->setDecimals(decimals);
    _knob->setValue<double>(def.get(),dims);
}

OfxStatus OfxDouble3DInstance::get(double& x1, double& x2, double& x3) {
    x1 = _knob->getValue<double>(0);
    x2 = _knob->getValue<double>(1);
    x3 = _knob->getValue<double>(2);
    return kOfxStatOK;
}

OfxStatus OfxDouble3DInstance::get(OfxTime time, double& x1, double& x2, double& x3) {
    x1 = _knob->getValueAtTime<double>(time,0);
    x2 = _knob->getValueAtTime<double>(time,1);
    x3 = _knob->getValueAtTime<double>(time,2);
    return kOfxStatOK;
}

OfxStatus OfxDouble3DInstance::set(double x1,double x2,double x3){
    _knob->setValue<double>(x1,0);
    _knob->setValue<double>(x2,1);
    _knob->setValue<double>(x3,2);
	return kOfxStatOK;
}

OfxStatus OfxDouble3DInstance::set(OfxTime time, double x1, double x2, double x3) {
    _knob->setValueAtTime<double>(time,x1,0);
    _knob->setValueAtTime<double>(time,x2,1);
    _knob->setValueAtTime<double>(time,x3,2);
	return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxDouble3DInstance::setEnabled() {
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxDouble3DInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxDouble3DInstance::getKnob() const {
    return _knob;
}


OfxInteger3DInstance::OfxInteger3DInstance(OfxEffectInstance *node, OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::Integer3DInstance(descriptor,node->effectInstance())
{
    const int dims = 3;
    const OFX::Host::Property::Set &properties = getProperties();


    _knob = dynamic_cast<Int_Knob*>(appPTR->getKnobFactory().createKnob("Int", node, getParamLabel(this), dims));

    std::vector<int> minimum(dims);
    std::vector<int> maximum(dims);
    std::vector<int> increment(dims);
    std::vector<int> displayMins(dims);
    std::vector<int> displayMaxs(dims);
    boost::scoped_array<int> def(new int[dims]);

    for (int i = 0; i < dims; ++i) {
        minimum[i] = properties.getIntProperty(kOfxParamPropMin,i);
        displayMins[i] = properties.getIntProperty(kOfxParamPropDisplayMin,i);
        displayMaxs[i] = properties.getIntProperty(kOfxParamPropDisplayMax,i);
        maximum[i] = properties.getIntProperty(kOfxParamPropMax,i);
        int incr = properties.getIntProperty(kOfxParamPropIncrement,i);
        increment[i] = incr != 0 ?  incr : 1;
        def[i] = properties.getIntProperty(kOfxParamPropDefault,i);
    }

    _knob->setMinimumsAndMaximums(minimum, maximum);
    _knob->setIncrement(increment);
    _knob->setDisplayMinimumsAndMaximums(displayMins, displayMaxs);
    _knob->setValue<int>(def.get(),dims);
}

OfxStatus OfxInteger3DInstance::get(int& x1, int& x2, int& x3) {
    x1 = _knob->getValue<int>(0);
    x2 = _knob->getValue<int>(1);
    x3 = _knob->getValue<int>(2);
    return kOfxStatOK;
}

OfxStatus OfxInteger3DInstance::get(OfxTime time, int& x1, int& x2, int& x3) {
    x1 = _knob->getValueAtTime<int>(time,0);
    x2 = _knob->getValueAtTime<int>(time,1);
    x3 = _knob->getValueAtTime<int>(time,2);
    return kOfxStatOK;
}

OfxStatus OfxInteger3DInstance::set(int x1, int x2, int x3) {
    _knob->setValue<int>(x1,0);
    _knob->setValue<int>(x2,1);
    _knob->setValue<int>(x3,2);
	return kOfxStatOK;
}

OfxStatus OfxInteger3DInstance::set(OfxTime time, int x1, int x2, int x3) {
    _knob->setValueAtTime<int>(time,x1,0);
    _knob->setValueAtTime<int>(time,x2,1);
    _knob->setValueAtTime<int>(time,x3,2);
	return kOfxStatOK;
}

// callback which should set enabled state as appropriate
void OfxInteger3DInstance::setEnabled() {
    _knob->setEnabled(getEnabled());
}

// callback which should set secret state as appropriate
void OfxInteger3DInstance::setSecret() {
    _knob->setSecret(getSecret());
}

Knob* OfxInteger3DInstance::getKnob() const {
    return _knob;
}


/***********/
OfxGroupInstance::OfxGroupInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::GroupInstance(descriptor,node->effectInstance())
, _node(node)
{
    const OFX::Host::Property::Set &properties = getProperties();

    int isTab = properties.getIntProperty(kFnOfxParamPropGroupIsTab);
    if(isTab){
        Tab_Knob* _tabKnob = _node->getTabKnob();
        std::string name = getParamLabel(this);
        if(!_tabKnob){
            _tabKnob = dynamic_cast<Tab_Knob*>(appPTR->getKnobFactory().createKnob("Tab", node, name));
            _node->setTabKnob(_tabKnob);
        }
        _groupKnob = 0;

    }else{
        _groupKnob = dynamic_cast<Group_Knob*>(appPTR->getKnobFactory().createKnob("Group", node, getParamLabel(this)));
        int opened = properties.getIntProperty(kOfxParamPropGroupOpen);
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


OfxStringInstance::OfxStringInstance(OfxEffectInstance* node,OFX::Host::Param::Descriptor& descriptor)
: OFX::Host::Param::StringInstance(descriptor,node->effectInstance())
, _node(node)
, _fileKnob(0)
, _outputFileKnob(0)
, _stringKnob(0)
, _multiLineKnob(0)
{
    const OFX::Host::Property::Set &properties = getProperties();
    std::string mode = properties.getStringProperty(kOfxParamPropStringMode);

    if(mode == kOfxParamStringIsFilePath){
        if(_node->isGenerator()){
            _fileKnob = dynamic_cast<File_Knob*>(appPTR->getKnobFactory().createKnob("InputFile", node, getParamLabel(this)));
            QObject::connect(_fileKnob, SIGNAL(frameRangeChanged(int,int)), this, SLOT(onFrameRangeChanged(int,int)));

        }else{
            _node->setAsOutputNode(); // IMPORTANT ! 
            _outputFileKnob = dynamic_cast<OutputFile_Knob*>(appPTR->getKnobFactory().createKnob("OutputFile", node, getParamLabel(this)));

        }
    }else if(mode == kOfxParamStringIsSingleLine || mode == kOfxParamStringIsLabel){
        
        _stringKnob = dynamic_cast<String_Knob*>(appPTR->getKnobFactory().createKnob("String", node, getParamLabel(this)));
        if(mode == kOfxParamStringIsLabel){
            _stringKnob->setEnabled(false);
        }

        set(properties.getStringProperty(kOfxParamPropDefault,1).c_str());
    }else if(mode == kOfxParamStringIsMultiLine){
        _multiLineKnob = dynamic_cast<RichText_Knob*>(appPTR->getKnobFactory().createKnob("RichText", node, getParamLabel(this)));

        set(properties.getStringProperty(kOfxParamPropDefault,1).c_str());

    }
    
    
}

 void OfxStringInstance::onFrameRangeChanged(int f,int l){
     _node->notifyFrameRangeChanged(f,l);
 }

OfxStatus OfxStringInstance::get(std::string &str) {
    assert(_node->effectInstance());
    if(_fileKnob){
        int currentFrame = (int)_node->effectInstance()->timeLineGetTime();
        QString fileName =  _fileKnob->getRandomFrameName(currentFrame,true);
        str = fileName.toStdString();
    }else if(_outputFileKnob){
        str = filenameFromPattern((int)_node->getCurrentFrame());
    }else if(_stringKnob){
        str = _stringKnob->getString();
    }else if(_multiLineKnob){
        str = _multiLineKnob->getString();
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::get(OfxTime time, std::string& str) {
    assert(_node->effectInstance());
    if(_fileKnob){
        str = _fileKnob->getRandomFrameName(time,true).toStdString();
    }else if(_outputFileKnob){
        str = filenameFromPattern((int)_node->getCurrentFrame());
    }else if(_stringKnob){
        str = _stringKnob->getString();
    }else if(_multiLineKnob){
        str = _multiLineKnob->getString();
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::set(const char* str) {
    if(_fileKnob){
        _fileKnob->setValue(str);
    }
    if(_outputFileKnob){
        _outputFileKnob->setValue(str);
    }
    if(_stringKnob){
        _stringKnob->setValue(str);
    }
    if(_multiLineKnob){
        _multiLineKnob->setValue(str);
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::set(OfxTime /*time*/, const char* str) {
    if(_fileKnob){
        _fileKnob->setValue(str);
    }
    if(_outputFileKnob){
         _outputFileKnob->setValue(str);
    }
    if(_stringKnob){
        _stringKnob->setValue(str);
    }
    if(_multiLineKnob){
        _multiLineKnob->setValue(str);
    }
    return kOfxStatOK;
}
OfxStatus OfxStringInstance::getV(va_list arg){
    const char **value = va_arg(arg, const char **);
    
    OfxStatus stat = get(_localString.localData());
    *value = _localString.localData().c_str();
    return stat;

}
OfxStatus OfxStringInstance::getV(OfxTime time, va_list arg){
    const char **value = va_arg(arg, const char **);
    
    OfxStatus stat = get(time,_localString.localData());
    *value = _localString.localData().c_str();
    return stat;
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
    if(_multiLineKnob){
        return _multiLineKnob;
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
    if(_multiLineKnob){
        _multiLineKnob->setEnabled(getEnabled());
    }
}

// callback which should set secret state as appropriate
void OfxStringInstance::setSecret(){
    if(_fileKnob){
        _fileKnob->setSecret(getSecret());
    }
    if (_outputFileKnob) {
        _outputFileKnob->setSecret(getSecret());
    }
    if (_stringKnob) {
        _stringKnob->setSecret(getSecret());
    }
    if (_multiLineKnob) {
        _multiLineKnob->setSecret(getSecret());
    }
}


const QString OfxStringInstance::getRandomFrameName(int f) const{
    return _fileKnob ? _fileKnob->getRandomFrameName(f,true) : "";
}
bool OfxStringInstance::isValid() const{
    if(_fileKnob){
        return !_fileKnob->getValue<QStringList>().isEmpty();
    }
    if(_outputFileKnob){
        return !_outputFileKnob->getValue<QString>().toStdString().empty();
    }
    return true;
}
std::string OfxStringInstance::filenameFromPattern(int frameIndex) const{
    if(_outputFileKnob){
        std::string pattern = _outputFileKnob->getValue<QString>().toStdString();
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

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
#include "Knob.h"

#include <QtCore/QDataStream>
#include <QtCore/QByteArray>

#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"


#include "Readers/Reader.h"

using namespace Natron;
using std::make_pair; using std::pair;


/***********************************KNOB BASE******************************************/

struct Knob::KnobPrivate {
    KnobHolder*  _holder;
    std::vector<U64> _hashVector;
    std::string _description;//< the text label that will be displayed  on the GUI
    QString _name;//< the knob can have a name different than the label displayed on GUI.
    //By default this is the same as _description but can be set by calling setName().
    bool _newLine;
    int _itemSpacing;

    Knob* _parentKnob;
    bool _secret;
    bool _enabled;
    bool _canUndo;
    bool _isInsignificant; //< if true, a value change will never trigger an evaluation
    bool _isPersistent;//will it be serialized?
    std::string _tooltipHint;
    bool _isAnimationEnabled;

    KnobPrivate(KnobHolder*  holder,const std::string& description)
        : _holder(holder)
        , _hashVector()
        , _description(description)
        , _name(description.c_str())
        , _newLine(true)
        , _itemSpacing(0)
        , _parentKnob(NULL)
        , _secret(false)
        , _enabled(true)
        , _canUndo(true)
        , _isInsignificant(false)
        , _isPersistent(true)
        , _tooltipHint()
        , _isAnimationEnabled(true)
    {

    }

    void updateHash(const std::map<int,Variant>& value){

        _hashVector.clear();

        for(std::map<int,Variant>::const_iterator it = value.begin();it!=value.end();++it){
            QByteArray data;
            QDataStream ds(&data,QIODevice::WriteOnly);
            ds << it->second;
            data = data.toBase64();
            QString str(data);
            for (int i = 0; i < str.size(); ++i) {
                _hashVector.push_back(str.at(i).unicode());
            }
        }

        _holder->invalidateHash();
    }

};

Knob::Knob(KnobHolder* holder,const std::string& description,int dimension)
    : AnimatingParam(dimension)
    , _imp(new KnobPrivate(holder,description))
{
    
    if(_imp->_holder){
        _imp->_holder->addKnob(boost::shared_ptr<Knob>(this));
    }
}


Knob::~Knob(){    
    remove();
}

void Knob::remove(){
    emit deleted();
    if(_imp->_holder){
        _imp->_holder->removeKnob(this);
    }
}

//void Knob::onKnobUndoneChange(){
//    _imp->_holder->triggerAutoSave();
//}

//void Knob::onKnobRedoneChange(){
//    _imp->_holder->triggerAutoSave();
//}


void Knob::onStartupRestoration(const AnimatingParam& other){
    clone(other); //set the value
    bool wasBeginCalled = true;
    if(!_imp->_holder->wasBeginCalled()){
        wasBeginCalled = false;
        _imp->_holder->beginValuesChanged(AnimatingParam::PLUGIN_EDITED,true);
    }
    for(int i = 0 ; i < getDimension();++i){
        evaluateValueChange(i,AnimatingParam::PLUGIN_EDITED);
    }
    if(!wasBeginCalled){
        _imp->_holder->endValuesChanged(AnimatingParam::PLUGIN_EDITED);
    }
    emit restorationComplete();
}



void Knob::onValueChanged(int dimension,const Variant& variant){
    setValue(variant, dimension,AnimatingParam::USER_EDITED);
}


void Knob::evaluateAnimationChange(){
    
    SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();
    bool wasBeginCalled = true;
    if(!_imp->_holder->wasBeginCalled()){
        wasBeginCalled = false;
        beginValueChange(AnimatingParam::PLUGIN_EDITED);
    }
    for(int i = 0; i < getDimension();++i){
        boost::shared_ptr<Curve> curve = getCurve(i);
        if(curve && curve->isAnimated()){
            Variant v = getValueAtTime(time,i);
            setValue(v,i);
        }
    }
    if(!wasBeginCalled){
        endValueChange(AnimatingParam::PLUGIN_EDITED);
    }
}

void Knob::beginValueChange(AnimatingParam::ValueChangedReason reason) {
    _imp->_holder->beginValuesChanged(reason,!_imp->_isInsignificant);
}

void Knob::endValueChange(AnimatingParam::ValueChangedReason reason) {
    _imp->_holder->endValuesChanged(reason);
}

void Knob::evaluateValueChange(int dimension,AnimatingParam::ValueChangedReason reason){
    if(!_imp->_isInsignificant)
        _imp->updateHash(getValueForEachDimension());
    processNewValue();
    if(reason != AnimatingParam::USER_EDITED){
        emit valueChanged(dimension);
    }
    _imp->_holder->onValueChanged(this,reason,!_imp->_isInsignificant);
}

void Knob::onTimeChanged(SequenceTime time){
    if(isAnimationEnabled()){
        _imp->_holder->beginValuesChanged(AnimatingParam::TIME_CHANGED,false); // we do not want to force a re-evaluation
        for(int i = 0; i < getDimension();++i){
            boost::shared_ptr<Curve> curve = getCurve(i);
            if(curve && curve->isAnimated()){
                Variant v = getValueAtTime(time,i);
                setValue(v,i);
            }
        }
        _imp->_holder->endValuesChanged(AnimatingParam::TIME_CHANGED);

    }
}



void Knob::cloneValue(const Knob& other){
    assert(_imp->_name == other._imp->_name);
    _imp->_hashVector = other._imp->_hashVector;
    clone(dynamic_cast<const AnimatingParam&>(other));
    cloneExtraData(other);
}

void Knob::turnOffNewLine(){
    _imp->_newLine = false;
}

void Knob::setSpacingBetweenItems(int spacing){
    _imp->_itemSpacing = spacing;
}
void Knob::setEnabled(bool b){
    _imp->_enabled = b;
    emit enabledChanged();
}

void Knob::setSecret(bool b){
    _imp->_secret = b;
    emit secretChanged();
}

int Knob::determineHierarchySize() const{
    int ret = 0;
    Knob* current = getParentKnob();
    while(current){
        ++ret;
        current = current->getParentKnob();
    }
    return ret;
}


const std::string& Knob::getDescription() const { return _imp->_description; }

const std::vector<U64>& Knob::getHashVector() const { return _imp->_hashVector; }

KnobHolder*  Knob::getHolder() const { return _imp->_holder; }

void Knob::turnOffAnimation() { _imp->_isAnimationEnabled = false; }

bool Knob::isAnimationEnabled() const { return canAnimate() && _imp->_isAnimationEnabled; }

void Knob::setName(const std::string& name) {_imp->_name = QString(name.c_str());}

std::string Knob::getName() const {return _imp->_name.toStdString();}

void Knob::setParentKnob(Knob* knob){ _imp->_parentKnob = knob;}

Knob* Knob::getParentKnob() const {return _imp->_parentKnob;}

bool Knob::isSecret() const {return _imp->_secret;}

bool Knob::isEnabled() const {return _imp->_enabled;}

void Knob::setInsignificant(bool b) {_imp->_isInsignificant = b;}

bool Knob::isPersistent() const { return _imp->_isPersistent; }

void Knob::setPersistent(bool b) { _imp->_isPersistent = b; }

void Knob::turnOffUndoRedo() {_imp->_canUndo = false;}

bool Knob::canBeUndone() const {return _imp->_canUndo;}

bool Knob::isInsignificant() const {return _imp->_isInsignificant;}

void Knob::setHintToolTip(const std::string& hint) {_imp->_tooltipHint = hint;}

const std::string& Knob::getHintToolTip() const {return _imp->_tooltipHint;}

/***************************KNOB HOLDER******************************************/

KnobHolder::KnobHolder(AppInstance* appInstance):
    _app(appInstance)
  , _knobs()
  , _betweenBeginEndParamChanged(false)
  , _insignificantChange(false)
  , _knobsChanged()
{}

KnobHolder::~KnobHolder(){
    _knobs.clear();
}

void KnobHolder::invalidateHash(){
    _app->incrementKnobsAge();
}
int KnobHolder::getAppAge() const{
    return _app->getKnobsAge();
}


void KnobHolder::beginValuesChanged(AnimatingParam::ValueChangedReason reason, bool isSignificant){
    if(_betweenBeginEndParamChanged)
        return;
    _betweenBeginEndParamChanged = true ;
    _insignificantChange = !isSignificant;
    beginKnobsValuesChanged(reason);
}

void KnobHolder::endValuesChanged(AnimatingParam::ValueChangedReason reason){
    if(!_betweenBeginEndParamChanged){
        return;
    }

    if(reason == AnimatingParam::USER_EDITED){
        triggerAutoSave();
    }
    _betweenBeginEndParamChanged = false ;
    if(!_knobsChanged.empty() && reason != AnimatingParam::TIME_CHANGED){
        evaluate(_knobsChanged.back(),!_insignificantChange);
    }
    _knobsChanged.clear();
    endKnobsValuesChanged(reason);
}

void KnobHolder::onValueChanged(Knob* k, AnimatingParam::ValueChangedReason reason, bool isSignificant){
    bool wasBeginCalled = true;
    if(!_betweenBeginEndParamChanged){
        beginValuesChanged(reason,isSignificant);
        wasBeginCalled = false;
    }
    _knobsChanged.push_back(k);
    onKnobValueChanged(k,reason);
    if(!wasBeginCalled){
        endValuesChanged(reason);
    }

}
void KnobHolder::cloneKnobs(const KnobHolder& other){
    assert(_knobs.size() == other._knobs.size());
    for(U32 i = 0 ; i < other._knobs.size();++i){
        _knobs[i]->cloneValue(*(other._knobs[i]));
    }
}



void KnobHolder::removeKnob(Knob* knob){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        if (_knobs[i].get() == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}

void KnobHolder::triggerAutoSave(){
    _app->triggerAutoSave();
}

void KnobHolder::refreshAfterTimeChange(SequenceTime time){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        _knobs[i]->onTimeChanged(time);
    }
}

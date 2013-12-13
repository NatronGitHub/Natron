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
#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/KnobSerialization.h"

#include "Global/AppManager.h"
#include "Global/LibraryBinary.h"
#include "Global/GlobalDefines.h"


#include "Readers/Reader.h"

using namespace Natron;
using std::make_pair; using std::pair;


/***********************************KNOB BASE******************************************/
typedef std::vector< boost::shared_ptr<Knob> > MastersMap;
typedef std::vector< boost::shared_ptr<Curve> > CurvesMap;

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

    /* A variant storing all the values in any dimension. <dimension,value>*/
   std::vector<Variant> _values;
   int _dimension;
   /* the keys for a specific dimension*/
   CurvesMap _curves;

    ////curve links
    ///A slave link CANNOT be master at the same time (i.e: if _slaveLinks[i] != NULL  then _masterLinks[i] == NULL )
    MastersMap _masters; //from what knob is slaved each curve if any

    KnobPrivate(KnobHolder*  holder,int dimension,const std::string& description)
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
        , _values(dimension)
        , _dimension(dimension)
        , _curves(dimension)
        , _masters(dimension)
    {

    }

    void updateHash(const std::vector<Variant>& value){

        _hashVector.clear();

        for(U32 i = 0 ; i < value.size();++i){
            QByteArray data;
            QDataStream ds(&data,QIODevice::WriteOnly);
            ds <<  value[i];
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
    :_imp(new KnobPrivate(holder,dimension,description))
{
    
    if(_imp->_holder){
        _imp->_holder->addKnob(boost::shared_ptr<Knob>(this));
    }

    for(int i = 0; i < dimension ; ++i){
        _imp->_values[i] = Variant();
        _imp->_curves[i] = boost::shared_ptr<Curve>(new Curve(this));

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


const Variant& Knob::getValue(int dimension) const {
    if(dimension > (int)_imp->_values.size()){
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return isSlave->getValue(dimension);
    }

    return _imp->_values[dimension];
}


Variant Knob::getValueAtTime(double time,int dimension) const{

    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return isSlave->getValueAtTime(time,dimension);
    }


    boost::shared_ptr<Curve> curve  = _imp->_curves[dimension];
    if (curve->isAnimated()) {
#warning "We should query the variant's type of the curve to construct  an appropriate return value"
        //FIXME: clamp to min/max
        return Variant(curve->getValueAt(time));
    } else {
        /*if the knob as no keys at this dimension, return the value
        at the requested dimension.*/
        return _imp->_values[dimension];
    }
}

void Knob::setValue(const Variant& v, int dimension, Natron::ValueChangedReason reason){

    if(dimension > (int)_imp->_values.size()){
        throw std::invalid_argument("Knob::setValue(): Dimension out of range");
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return;
    }

    _imp->_values[dimension] = v;


    evaluateValueChange(dimension,reason);
}

void Knob::setValueAtTime(int time, const Variant& v, int dimension, Natron::ValueChangedReason reason){

    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::setValueAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return;
    }


    boost::shared_ptr<Curve> curve = _imp->_curves[dimension];
#warning "We should query the variant's type passed in parameter to construct a keyframe with an appropriate value"
    curve->addKeyFrame(KeyFrame(time,v.toDouble()));


     if(reason != Natron::USER_EDITED){
        emit keyFrameSet(time,dimension);
     }

}

void Knob::deleteValueAtTime(int time,int dimension,Natron::ValueChangedReason reason){

    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }


    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return;
    }
    _imp->_curves[dimension]->removeKeyFrame(time);

    if(reason != Natron::USER_EDITED){
        emit keyFrameRemoved(time,dimension);
    }
}


void Knob::setValue(const Variant& value,int dimension){
    setValue(value,dimension,Natron::PLUGIN_EDITED);
}

void Knob::setValueAtTime(int time, const Variant& v, int dimension){
    setValueAtTime(time,v,dimension,Natron::PLUGIN_EDITED);
}



void Knob::deleteValueAtTime(int time,int dimension){
    deleteValueAtTime(time,dimension,Natron::PLUGIN_EDITED);
}

boost::shared_ptr<Curve> Knob::getCurve(int dimension) const {
    assert(dimension < (int)_imp->_curves.size());
    return _imp->_curves[dimension];
}

bool Knob::isAnimated(int dimension) const {
    return getCurve(dimension)->isAnimated();
}

const std::vector<boost::shared_ptr<Curve> >& Knob::getCurves() const{
    return _imp->_curves;
}

const std::vector<Variant>& Knob::getValueForEachDimension() const { return _imp->_values; }

int Knob::getDimension() const { return _imp->_dimension; }

void Knob::load(const KnobSerialization& serializationObj){

    assert(_imp->_dimension == serializationObj.getDimension());

    ///restore masters
    const std::vector< std::string >& serializedMasters = serializationObj.getMasters();
    for(U32 i = 0 ; i < serializedMasters.size();++i){
        const std::vector< boost::shared_ptr<Knob> >& otherKnobs = _imp->_holder->getKnobs();
        for(U32 j = 0 ; j < otherKnobs.size();++j)
        {
            if(otherKnobs[j]->getDescription() == serializedMasters[i]){
                _imp->_masters[i] = otherKnobs[j];
                break;
            }
        }
    }

    ///bracket value changes
    beginValueChange(Natron::PLUGIN_EDITED);

    const std::vector< boost::shared_ptr<Curve> >& serializedCurves = serializationObj.getCurves();
    for(U32 i = 0 ; i< serializedCurves.size();++i){
        assert(serializedCurves[i]);
        _imp->_curves[i]->clone(*serializedCurves[i]);
    }

    const std::vector<Variant>& serializedValues = serializationObj.getValues();
    for(U32 i = 0 ; i < serializedValues.size();++i){
        setValue(serializedValues[i],i,Natron::PLUGIN_EDITED);
    }

    ///end bracket
    endValueChange(Natron::PLUGIN_EDITED);
    emit restorationComplete();
}

void Knob::save(KnobSerialization* serializationObj) const {
    serializationObj->initialize(this);
}

void Knob::onValueChanged(int dimension,const Variant& variant){
    setValue(variant, dimension,Natron::USER_EDITED);
}

void Knob::onKeyFrameSet(SequenceTime time,int dimension){
    setValueAtTime(time,getValue(dimension),dimension,Natron::USER_EDITED);
}

void Knob::onKeyFrameRemoved(SequenceTime time,int dimension){
    deleteValueAtTime(time,dimension,Natron::USER_EDITED);
}

void Knob::evaluateAnimationChange(){
    
    SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();

    beginValueChange(Natron::PLUGIN_EDITED);

    for(int i = 0; i < getDimension();++i){
        boost::shared_ptr<Curve> curve = getCurve(i);
        if(curve && curve->isAnimated()){
            Variant v = getValueAtTime(time,i);
            setValue(v,i);
        }
    }
    endValueChange(Natron::PLUGIN_EDITED);

}

void Knob::beginValueChange(Natron::ValueChangedReason reason) {
    _imp->_holder->notifyProjectBeginKnobsValuesChanged(reason);
}

void Knob::endValueChange(Natron::ValueChangedReason reason) {
    _imp->_holder->notifyProjectEndKnobsValuesChanged(reason);
}


void Knob::evaluateValueChange(int dimension,Natron::ValueChangedReason reason){
    if(!_imp->_isInsignificant)
        _imp->updateHash(getValueForEachDimension());
    processNewValue();
    if(reason != Natron::USER_EDITED){
        emit valueChanged(dimension);
    }

    bool significant = reason == Natron::TIME_CHANGED ? false : !_imp->_isInsignificant;

    _imp->_holder->notifyProjectEvaluationRequested(reason, this, significant);
}

void Knob::onTimeChanged(SequenceTime time){
    //setValue's calls compression is taken care of above.
    for(U32 i = 0 ; i < _imp->_curves.size();++i){
        if(_imp->_curves[i]->keyFramesCount() > 0){
            Variant v = getValueAtTime(time,i);
            setValue(v,i,Natron::TIME_CHANGED);
        }
    }

}



void Knob::cloneValue(const Knob& other){
    assert(_imp->_name == other._imp->_name);
    _imp->_hashVector = other._imp->_hashVector;

    _imp->_values = other._imp->_values;

    assert(_imp->_curves.size() == other._imp->_curves.size());

    //we cannot copy directly the map of curves because the curves hold a pointer to the knob
    //we must explicitly call clone() on them
    for(U32 i = 0 ; i < _imp->_curves.size();++i){
        assert(_imp->_curves[i] && other._imp->_curves[i]);
        _imp->_curves[i]->clone(*(other._imp->_curves[i]));
    }
    
    //same for masters : the knobs are not refered to the same KnobHolder (i.e the same effect instance)
    //so we need to copy with the good pointers
    const MastersMap& otherMasters = other.getMasters();
    for(U32 j = 0 ; j < otherMasters.size();++j){
        if(otherMasters[j]){
            const std::vector< boost::shared_ptr<Knob> >& holderKnobs = _imp->_holder->getKnobs();
            for(U32 i = 0 ; i < holderKnobs.size();++i)
            {
                if(holderKnobs[i]->getDescription() == otherMasters[i]->getDescription()){
                    _imp->_masters[j] = holderKnobs[i];
                    break;
                }
            }
        }
    }
    
    
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

bool Knob::slaveTo(int dimension,boost::shared_ptr<Knob> other){
    assert(dimension < (int)_imp->_masters.size());
    assert(!other->isCurveSlave(dimension));
    if(_imp->_masters[dimension]){
        return false;
    }
    _imp->_masters[dimension] = other;
        //copy values and add keyframes
    return true;
}


void Knob::unSlave(int dimension){
    assert(isCurveSlave(dimension));
    _imp->_masters[dimension].reset();
}


boost::shared_ptr<Knob>  Knob::isCurveSlave(int dimension) const {
    return _imp->_masters[dimension];
}

const std::vector<boost::shared_ptr<Knob> >& Knob::getMasters() const{
    return _imp->_masters;
}


/***************************KNOB HOLDER******************************************/

KnobHolder::KnobHolder(AppInstance* appInstance):
    _app(appInstance)
  , _knobs()
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

void KnobHolder::refreshAfterTimeChange(SequenceTime time){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        _knobs[i]->onTimeChanged(time);
    }
}

void KnobHolder::notifyProjectBeginKnobsValuesChanged(Natron::ValueChangedReason reason){
    getApp()->getProject()->beginProjectWideValueChanges(reason, this);
}

void KnobHolder::notifyProjectEndKnobsValuesChanged(Natron::ValueChangedReason reason){
    getApp()->getProject()->endProjectWideValueChanges(reason,this);
}

void KnobHolder::notifyProjectEvaluationRequested(Natron::ValueChangedReason reason,Knob* k,bool significant){
    getApp()->getProject()->stackEvaluateRequest(reason,this,k,significant);
}

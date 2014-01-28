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


using namespace Natron;
using std::make_pair; using std::pair;


/***********************************KNOB BASE******************************************/
typedef std::vector< boost::shared_ptr<Knob> > MastersMap;
typedef std::vector< boost::shared_ptr<Curve> > CurvesMap;

struct Knob::KnobPrivate {
    Knob* _publicInterface;
    KnobHolder*  _holder;
    mutable QMutex _hashMutex; //< protects hashVector
    std::vector<U64> _hashVector;
    std::string _description;//< the text label that will be displayed  on the GUI
    QString _name;//< the knob can have a name different than the label displayed on GUI.
                  //By default this is the same as _description but can be set by calling setName().
    bool _newLine;
    int _itemSpacing;
    
    boost::shared_ptr<Knob> _parentKnob;
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
    
    std::vector<Natron::AnimationLevel> _animationLevel;//< indicates for each dimension whether it is static/interpolated/onkeyframe
    
    KnobPrivate(Knob* publicInterface,KnobHolder*  holder,int dimension,const std::string& description)
    : _publicInterface(publicInterface)
    , _holder(holder)
    , _hashMutex()
    , _hashVector()
    , _description(description)
    , _name(description.c_str())
    , _newLine(true)
    , _itemSpacing(0)
    , _parentKnob()
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
    , _animationLevel(dimension)
    {
        
    }
    
    void updateHash(const std::vector<Variant>& value){
        {
            QMutexLocker l(&_hashMutex);
            _hashVector.clear();
            
            _publicInterface->appendExtraDataToHash(&_hashVector);
            
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
        }
        
        _holder->invalidateHash();
    }
    
};

Knob::Knob(KnobHolder* holder,const std::string& description,int dimension)
:_imp(new KnobPrivate(this,holder,dimension,description))
{
    
    
    for(int i = 0; i < dimension ; ++i){
        _imp->_values[i] = Variant();
        _imp->_curves[i] = boost::shared_ptr<Curve>(new Curve(this));
        _imp->_animationLevel[i] = Natron::NO_ANIMATION;
    }
}


Knob::~Knob(){
    remove();
    
}

void Knob::remove(){
    emit deleted(this);
    if(_imp->_holder){
         _imp->_holder->removeKnob(this);
    }
}


Variant Knob::getValue(int dimension) const {
    
    if (isAnimated(dimension)) {
        return getValueAtTime(getHolder()->getApp()->getTimeLine()->currentFrame(), dimension);
    }
    
    if(dimension > (int)_imp->_values.size()){
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    boost::shared_ptr<Knob> master = getMaster(dimension);
    if (master) {
        return master->getValue(dimension);
    }
    
    return _imp->_values[dimension];
}


Variant Knob::getValueAtTime(double time,int dimension) const{
    
    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }
    
    ///if the knob is slaved to another knob, returns the other knob value
    boost::shared_ptr<Knob> master = getMaster(dimension);
    if (master) {
        return master->getValueAtTime(time,dimension);
    }
    
    
    boost::shared_ptr<Curve> curve  = _imp->_curves[dimension];
    if (curve->isAnimated()) {
        Variant ret;
        variantFromInterpolatedValue(curve->getValueAt(time), &ret);
        return ret;
    } else {
        /*if the knob as no keys at this dimension, return the value
         at the requested dimension.*/
        return _imp->_values[dimension];
    }
}

Knob::ValueChangedReturnCode Knob::setValue(const Variant& v, int dimension, Natron::ValueChangedReason reason,KeyFrame* newKey){
   
    if(dimension > (int)_imp->_values.size()){
        throw std::invalid_argument("Knob::setValue(): Dimension out of range");
    }
    
    Knob::ValueChangedReturnCode  ret = NO_KEYFRAME_ADDED;
    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (!isSlave(dimension)) {
        ///locking project if it is saving
        if(_imp->_holder->getApp()){
            _imp->_holder->getApp()->lockProject();
        }
        
        _imp->_values[dimension] = v;
        
        ///unlocking project if it is saving
        if(_imp->_holder->getApp()){
            _imp->_holder->getApp()->unlockProject();
        }
        
        ///Add automatically a new keyframe
        if(getAnimationLevel(dimension) != Natron::NO_ANIMATION && //< if the knob is animated
           _imp->_holder->getApp() && //< the app pointer is not NULL
           !_imp->_holder->getApp()->isLoadingProject() && //< we're not loading the project
           (reason == Natron::USER_EDITED || reason == Natron::PLUGIN_EDITED) && //< the change was made by the user or plugin
           newKey != NULL){ //< the keyframe to set is not null
            
            SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();
            bool addedKeyFrame = setValueAtTime(time, v, dimension,reason,newKey);
            if(addedKeyFrame){
                ret = KEYFRAME_ADDED;
            }else{
                ret = KEYFRAME_MODIFIED;
            }
            
        }
    }
    if(ret == NO_KEYFRAME_ADDED){ //the other cases already called this in setValueAtTime()
        evaluateValueChange(dimension,reason);
    }
    return ret;
}

bool Knob::setValueAtTime(int time, const Variant& v, int dimension, Natron::ValueChangedReason reason,KeyFrame* newKey){
    
    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::setValueAtTime(): Dimension out of range");
    }
    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return false;
    }
    
    
    boost::shared_ptr<Curve> curve = _imp->_curves[dimension];
    double keyFrameValue;
    Natron::Status stat = variantToKeyFrameValue(time,v, &keyFrameValue);
    if (stat == Natron::StatReplyDefault) {
        if(curve->areKeyFramesValuesClampedToIntegers()){
            keyFrameValue = v.toInt();
        }else if(curve->areKeyFramesValuesClampedToBooleans()){
            keyFrameValue = (int)v.toBool();
        }else{
            keyFrameValue = v.toDouble();
        }
    }

    *newKey = KeyFrame((double)time,keyFrameValue);
    bool ret = curve->addKeyFrame(*newKey);
    
    if (reason == Natron::PLUGIN_EDITED) {
        setValue(v, dimension,Natron::OTHER_REASON,NULL);
    }
    
    if(reason != Natron::USER_EDITED){
        emit keyFrameSet(time,dimension);
    }
    
    emit updateSlaves(dimension);
    return ret;
}

void Knob::deleteValueAtTime(int time,int dimension,Natron::ValueChangedReason reason){
    
    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }
    
    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return;
    }
    _imp->_curves[dimension]->removeKeyFrame((double)time);
    
    if(reason != Natron::USER_EDITED){
        emit keyFrameRemoved(time,dimension);
    }
    emit updateSlaves(dimension);
}

void Knob::removeAnimation(int dimension,Natron::ValueChangedReason reason){
    if(dimension > (int)_imp->_curves.size()){
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }

    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return;
    }
    
    _imp->_curves[dimension]->clearKeyFrames();
    
    if(reason != Natron::USER_EDITED){
        emit animationRemoved(dimension);
    }
}

void Knob::setValue(const Variant& value,int dimension,bool turnOffAutoKeying){
    if(turnOffAutoKeying){
        setValue(value,dimension,Natron::PLUGIN_EDITED,NULL);
    }else{
        KeyFrame k;
        setValue(value,dimension,Natron::PLUGIN_EDITED,&k);
    }
}

void Knob::setValueAtTime(int time, const Variant& v, int dimension){
    KeyFrame k;
    setValueAtTime(time,v,dimension,Natron::PLUGIN_EDITED,&k);
}



void Knob::deleteValueAtTime(int time,int dimension){
    deleteValueAtTime(time,dimension,Natron::PLUGIN_EDITED);
}

void Knob::removeAnimation(int dimension){
    removeAnimation(dimension,Natron::PLUGIN_EDITED);
}

boost::shared_ptr<Curve> Knob::getCurve(int dimension) const {
    assert(dimension < (int)_imp->_curves.size());
    
    boost::shared_ptr<Knob> master = getMaster(dimension);
    if (master) {
        return master->getCurve(dimension);
    }
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
                setEnabled(false);
                break;
            }
        }
    }
    
    ///bracket value changes
    beginValueChange(Natron::PLUGIN_EDITED);
    
    const std::string& extraData = serializationObj.getExtraData();
    if(!extraData.empty()){
        loadExtraData(extraData.c_str());
    }
    
    const std::vector< boost::shared_ptr<Curve> >& serializedCurves = serializationObj.getCurves();
    for(U32 i = 0 ; i< serializedCurves.size();++i){
        assert(serializedCurves[i]);
        _imp->_curves[i]->clone(*serializedCurves[i]);
    }
    
    const std::vector<Variant>& serializedValues = serializationObj.getValues();
    for(U32 i = 0 ; i < serializedValues.size();++i){
        setValue(serializedValues[i],i,Natron::PLUGIN_EDITED,NULL);
    }
    
    
    ///end bracket
    endValueChange();
    emit restorationComplete();
}

void Knob::save(KnobSerialization* serializationObj) const {
    serializationObj->initialize(this);
}

Knob::ValueChangedReturnCode Knob::onValueChanged(int dimension,const Variant& variant,KeyFrame* newKey){
    return setValue(variant, dimension,Natron::USER_EDITED,newKey);
}

void Knob::onKeyFrameSet(SequenceTime time,int dimension){
    KeyFrame k;
    setValueAtTime(time,getValue(dimension),dimension,Natron::USER_EDITED,&k);
}

void Knob::onKeyFrameRemoved(SequenceTime time,int dimension){
    deleteValueAtTime(time,dimension,Natron::USER_EDITED);
}

void Knob::onAnimationRemoved(int dimension){
    removeAnimation(dimension, Natron::USER_EDITED);
}

void Knob::evaluateAnimationChange(){
    
    //the holder cannot be a global holder(i.e: it cannot be tied application wide)
    assert(_imp->_holder->getApp());
    SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();
    
    beginValueChange(Natron::PLUGIN_EDITED);
    bool hasEvaluatedOnce = false;
    for(int i = 0; i < getDimension();++i){
        boost::shared_ptr<Curve> curve = getCurve(i);
        if(curve && curve->isAnimated()){
            Variant v = getValueAtTime(time,i);
            setValue(v,i,Natron::OTHER_REASON,NULL);
            hasEvaluatedOnce = true;
        }
    }
    if(!hasEvaluatedOnce && !_imp->_holder->isClone()){
        evaluateValueChange(0, Natron::PLUGIN_EDITED);
    }
    
    endValueChange();
    
}

void Knob::beginValueChange(Natron::ValueChangedReason reason) {
    _imp->_holder->notifyProjectBeginKnobsValuesChanged(reason);
}

void Knob::endValueChange() {
    _imp->_holder->notifyProjectEndKnobsValuesChanged();
}


void Knob::evaluateValueChange(int dimension,Natron::ValueChangedReason reason){
    if(!_imp->_isInsignificant)
        _imp->updateHash(getValueForEachDimension());
    processNewValue();
    if(reason != Natron::USER_EDITED && !_imp->_holder->isClone()){
        emit valueChanged(dimension);
    }
    emit updateSlaves(dimension);
    
    bool significant = reason == Natron::TIME_CHANGED ? false : !_imp->_isInsignificant;
    if(!_imp->_holder->isClone()){
        _imp->_holder->notifyProjectEvaluationRequested(reason, this, significant);
    }
}

void Knob::onTimeChanged(SequenceTime time){
    //setValue's calls compression is taken care of above.
    for (int i = 0; i < getDimension(); ++i) {
        boost::shared_ptr<Curve> c = getCurve(i);
        if(c->keyFramesCount() > 0) {
            Variant v = getValueAtTime(time,i);
            setValue(v,i,Natron::TIME_CHANGED,NULL);
        }
    }
}



void Knob::cloneValue(const Knob& other){

    assert(_imp->_name == other._imp->_name);
    
    //thread-safe
    _imp->_hashVector = other.getHashVector();
    
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
                if(holderKnobs[i]->getDescription() == otherMasters[j]->getDescription()){
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
    boost::shared_ptr<Knob> current = getParentKnob();
    while(current){
        ++ret;
        current = current->getParentKnob();
    }
    return ret;
}


const std::string& Knob::getDescription() const { return _imp->_description; }

void Knob::appendHashVectorToHash(Hash64* hash) const {
    QMutexLocker l(&_imp->_hashMutex);
    for(U32 i=0;i< _imp->_hashVector.size();++i) {
        hash->append(_imp->_hashVector[i]);
    }
}

KnobHolder*  Knob::getHolder() const { return _imp->_holder; }

void Knob::turnOffAnimation() { _imp->_isAnimationEnabled = false; }

bool Knob::isAnimationEnabled() const { return canAnimate() && _imp->_isAnimationEnabled; }

void Knob::setName(const std::string& name) {_imp->_name = QString(name.c_str());}

std::string Knob::getName() const {return _imp->_name.toStdString();}

void Knob::setParentKnob(boost::shared_ptr<Knob> knob){ _imp->_parentKnob = knob;}

boost::shared_ptr<Knob> Knob::getParentKnob() const {return _imp->_parentKnob;}

bool Knob::isSecret() const {return _imp->_secret;}

bool Knob::isEnabled() const {return _imp->_enabled;}

void Knob::setInsignificant(bool b) {_imp->_isInsignificant = b;}

bool Knob::isPersistent() const { return _imp->_isPersistent; }

void Knob::setPersistent(bool b) { _imp->_isPersistent = b; }

void Knob::turnOffUndoRedo() {_imp->_canUndo = false;}

bool Knob::canBeUndone() const {return _imp->_canUndo;}

bool Knob::isInsignificant() const {return _imp->_isInsignificant;}

const std::vector<U64>& Knob::getHashVector() const {
    QMutexLocker l(&_imp->_hashMutex);
    return _imp->_hashVector;
}

void Knob::setHintToolTip(const std::string& hint) {
    QString tooltip(hint.c_str());
    int countSinceLastNewLine = 0;
    for (int i = 0; i < tooltip.size();++i) {
        if(tooltip.at(i) != QChar('\n')){
            ++countSinceLastNewLine;
        }else{
            countSinceLastNewLine = 0;
        }
        if (countSinceLastNewLine%80 == 0 && i!=0) {
            /*Find closest word end and insert a new line*/
            while(i < tooltip.size() && tooltip.at(i)!=QChar(' ')){
                ++i;
            }
            tooltip.insert(i, QChar('\n'));
        }
    }
    _imp->_tooltipHint = tooltip.toStdString();
}

const std::string& Knob::getHintToolTip() const {return _imp->_tooltipHint;}

bool Knob::slaveTo(int dimension,boost::shared_ptr<Knob> other){
    assert(dimension < (int)_imp->_masters.size());
    assert(!other->isSlave(dimension));
    if(_imp->_masters[dimension]){
        return false;
    }
    _imp->_masters[dimension] = other;
    
    //copy values and add keyframes
    //    _imp->_values[dimension] = other->getValue(dimension);
    //    _imp->_curves[dimension]->clone(*(other->getCurve(dimension)));
    return true;
}


void Knob::unSlave(int dimension){
    assert(isSlave(dimension));
    //copy the state before cloning
    _imp->_values[dimension] =  _imp->_masters[dimension]->getValue(dimension);
    _imp->_curves[dimension]->clone(*( _imp->_masters[dimension]->getCurve(dimension)));
    
    _imp->_masters[dimension].reset();
}


boost::shared_ptr<Knob> Knob::getMaster(int dimension) const {
    return _imp->_masters[dimension];
}

bool Knob::isSlave(int dimension) const {
    return bool(_imp->_masters[dimension]);
}

const std::vector<boost::shared_ptr<Knob> >& Knob::getMasters() const{
    return _imp->_masters;
}

void Knob::setAnimationLevel(int dimension,Natron::AnimationLevel level){
    assert(dimension < (int)_imp->_animationLevel.size());
    
    _imp->_animationLevel[dimension] = level;
    animationLevelChanged((int)level);
}

Natron::AnimationLevel Knob::getAnimationLevel(int dimension) const{
    if(dimension > (int)_imp->_animationLevel.size()){
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }
    
    return _imp->_animationLevel[dimension];
}

void Knob::variantFromInterpolatedValue(double interpolated,Variant* returnValue) const {
    returnValue->setValue<double>(interpolated);
}

/***************************KNOB HOLDER******************************************/

KnobHolder::KnobHolder(AppInstance* appInstance):
_app(appInstance)
, _knobs()
, _isClone(false)
{}

KnobHolder::~KnobHolder(){
    for (U32 i = 0; i < _knobs.size(); ++i) {
        _knobs[i]->_imp->_holder = NULL;
    }
}

void KnobHolder::invalidateHash(){
    if(_app){
        _app->incrementKnobsAge();
    }
}
int KnobHolder::getAppAge() const{
    if(_app){
        return _app->getKnobsAge();
    }else{
        return -1;
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

void KnobHolder::refreshAfterTimeChange(SequenceTime time){
    for(U32 i = 0; i < _knobs.size() ; ++i){
        _knobs[i]->onTimeChanged(time);
    }
}

void KnobHolder::notifyProjectBeginKnobsValuesChanged(Natron::ValueChangedReason reason){
    if(_app){
        getApp()->beginProjectWideValueChanges(reason, this);
    }
}

void KnobHolder::notifyProjectEndKnobsValuesChanged(){
    if(_app){
        getApp()->endProjectWideValueChanges(this);
    }
}

void KnobHolder::notifyProjectEvaluationRequested(Natron::ValueChangedReason reason,Knob* k,bool significant){
    if(_app){
        getApp()->stackEvaluateRequest(reason,this,k,significant);
    }else{
        onKnobValueChanged(k, reason);
    }
}



boost::shared_ptr<Knob> KnobHolder::getKnobByDescription(const std::string& desc) const {
    
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for(U32 i = 0; i < knobs.size() ; ++i){
        if (knobs[i]->getDescription() == desc) {
            return knobs[i];
        }
    }
    return boost::shared_ptr<Knob>();
}

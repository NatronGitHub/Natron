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
typedef std::map<int, boost::shared_ptr<Knob> > MastersMap;
typedef std::multimap<int, Knob* > SlavesMap;
typedef std::map<int, boost::shared_ptr<Curve> > CurvesMap;

struct Knob::KnobPrivate {
    Knob* _publicInterface;
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
   std::map<int,Variant> _value;
   int _dimension;
   /* the keys for a specific dimension*/
   std::map<int, boost::shared_ptr<Curve> > _curves;

    ////curve links
    ///A slave link CANNOT be master at the same time (i.e: if _slaveLinks[i] != NULL  then _masterLinks[i] == NULL )
    MastersMap _masters; //from what knob is slaved each curve if any
    SlavesMap _slaves; //what knobs each curve will modify as a result of a link

    KnobPrivate(Knob* publicInterface,KnobHolder*  holder,int dimension,const std::string& description)
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
        , _value()
        , _dimension(dimension)
        , _curves()
        , _masters()
        , _slaves()
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

    /**
     * @brief Sets the curve at the given dimension to be the master of the
     * curve at the same dimension for the knob other. This is called internally
     * by slaveTo
    **/
    void setAsMasterOf(int dimension, Knob *other);

    /**
     * @brief Called internally by unSlave
    **/
    void unMaster(Knob* other);



};

Knob::Knob(KnobHolder* holder,const std::string& description,int dimension)
    :_imp(new KnobPrivate(this,holder,dimension,description))
{
    
    if(_imp->_holder){
        _imp->_holder->addKnob(boost::shared_ptr<Knob>(this));
    }

    for(int i = 0; i < dimension ; ++i){
        _imp->_value.insert(std::make_pair(i,Variant()));
        boost::shared_ptr<Curve> c (new Curve(this));
        _imp->_curves.insert(std::make_pair(i,c));
    }
}


Knob::~Knob(){    
    remove();
    _imp->_curves.clear();
    _imp->_value.clear();
}

void Knob::remove(){
    emit deleted();
    if(_imp->_holder){
        _imp->_holder->removeKnob(this);
    }
}


const Variant& Knob::getValue(int dimension) const{

    ///if the knob is slaved to another knob, returns the other knob value
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return isSlave->getValue(dimension);
    }

    std::map<int,Variant>::const_iterator it = _imp->_value.find(dimension);
    assert(it != _imp->_value.end());
    return it->second;
}


Variant Knob::getValueAtTime(double time,int dimension) const{

    ///if the knob is slaved to another knob, returns the other knob value
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave){
        return isSlave->getValueAtTime(time,dimension);
    }


    CurvesMap::const_iterator foundDimension = _imp->_curves.find(dimension);
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    if (curve->isAnimated()) {
        return foundDimension->second->getValueAt(time);
    } else {
        /*if the knob as no keys at this dimension, return the value
        at the requested dimension.*/
        std::map<int,Variant>::const_iterator it = _imp->_value.find(dimension);
        if(it != _imp->_value.end()){
            return it->second;
        }else{
            return Variant();
        }
    }
}

void Knob::setValue(const Variant& v, int dimension, Natron::ValueChangedReason reason){

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave && reason == Natron::PLUGIN_EDITED){
        return;
    }

    std::map<int,Variant>::iterator it = _imp->_value.find(dimension);
    assert(it != _imp->_value.end());
    it->second = v;

    ///also update slaves
    std::pair<SlavesMap::iterator,SlavesMap::iterator> range = _imp->_slaves.equal_range(dimension);
    for(SlavesMap::iterator it = range.first;it != range.second;++it){

        ///for a slave, find out what is the dimension that is slaved to this knob
        const MastersMap& slaveMasters = it->second->getMasters();
        for(MastersMap::const_iterator it2 = slaveMasters.begin();it2 != slaveMasters.end();++it2){
            if(it2->second.get() == this){
                it->second->setValue(v,it2->first,Natron::OTHER_REASON);
                break;
            }
        }
    }

    evaluateValueChange(dimension,reason);
}

boost::shared_ptr<KeyFrame> Knob::setValueAtTime(double time, const Variant& v, int dimension, Natron::ValueChangedReason reason){
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave && reason == Natron::PLUGIN_EDITED){
        return boost::shared_ptr<KeyFrame>();
    }


    CurvesMap::iterator foundDimension = _imp->_curves.find(dimension);
    assert(foundDimension != _imp->_curves.end());
    boost::shared_ptr<KeyFrame> k(new KeyFrame(time,v,foundDimension->second.get()));
    foundDimension->second->addKeyFrame(k);


    ///also update slaves
    std::pair<SlavesMap::iterator,SlavesMap::iterator> range = _imp->_slaves.equal_range(dimension);
    for(SlavesMap::iterator it = range.first;it != range.second;++it){
        ///for a slave, find out what is the dimension that is slaved to this knob
        const MastersMap& slaveMasters = it->second->getMasters();
        for(MastersMap::const_iterator it2 = slaveMasters.begin();it2 != slaveMasters.end();++it2){
            if(it2->second.get() == this){
                it->second->setValueAtTime(time,v,it2->first,Natron::OTHER_REASON);
                break;
            }
        }
    }
     if(reason != Natron::USER_EDITED){
        emit keyFrameSet(time,dimension);
     }

    return k;
}

void Knob::deleteValueAtTime(double time,int dimension,Natron::ValueChangedReason reason){
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    boost::shared_ptr<Knob> isSlave = isCurveSlave(dimension);
    if(isSlave && reason == Natron::PLUGIN_EDITED){
        return;
    }

    CurvesMap::iterator foundDimension = _imp->_curves.find(dimension);
    assert(foundDimension != _imp->_curves.end());
    foundDimension->second->removeKeyFrame(time);

    ///also update slaves
    std::pair<SlavesMap::iterator,SlavesMap::iterator> range = _imp->_slaves.equal_range(dimension);
    for(SlavesMap::iterator it = range.first;it != range.second;++it){
        ///for a slave, find out what is the dimension that is slaved to this knob
        const MastersMap& slaveMasters = it->second->getMasters();
        for(MastersMap::const_iterator it2 = slaveMasters.begin();it2 != slaveMasters.end();++it2){
            if(it2->second.get() == this){
                it->second->deleteValueAtTime(time,it2->first,Natron::OTHER_REASON);
                break;
            }
        }
    }
    if(reason != Natron::USER_EDITED){
    emit keyFrameRemoved(time,dimension);
    }
}


void Knob::setValue(const Variant& value,int dimension){
    setValue(value,dimension,Natron::PLUGIN_EDITED);
}

boost::shared_ptr<KeyFrame> Knob::setValueAtTime(double time, const Variant& v, int dimension){
   setValueAtTime(time,v,dimension,Natron::PLUGIN_EDITED);
}



void Knob::deleteValueAtTime(double time,int dimension){
    deleteValueAtTime(time,dimension,Natron::PLUGIN_EDITED);
}

boost::shared_ptr<Curve> Knob::getCurve(int dimension) const {
    CurvesMap::const_iterator foundDimension = _imp->_curves.find(dimension);
    assert(foundDimension != _imp->_curves.end());
    return foundDimension->second;
}

const std::map<int, boost::shared_ptr<Curve>  >& Knob::getCurves() const{
    return _imp->_curves;
}

const std::map<int,Variant>& Knob::getValueForEachDimension() const { return _imp->_value; }

int Knob::getDimension() const { return _imp->_dimension; }

void Knob::load(const KnobSerialization& serializationObj){

    assert(_imp->_dimension == serializationObj.getDimension());

    ///restore masters
    const std::map<int,std::string >& serializedMasters = serializationObj.getMasters();
    for(std::map<int,std::string >::const_iterator it = serializedMasters.begin();it!=serializedMasters.end();++it){
        assert(!isCurveSlave(it->first));
        const std::vector< boost::shared_ptr<Knob> >& otherKnobs = _imp->_holder->getKnobs();
        for(U32 i = 0 ; i < otherKnobs.size();++i)
        {
            if(otherKnobs[i]->getDescription() == it->second){
                _imp->_masters.insert(std::make_pair(it->first,otherKnobs[i]));
                break;
            }
        }
    }

    ///restore slaves
    const std::multimap<int, std::string >& serializedSlaves = serializationObj.getSlaves();
    for(std::multimap<int, std::string >::const_iterator it = serializedSlaves.begin();it!=serializedSlaves.end();++it){
        const std::vector< boost::shared_ptr<Knob> >& otherKnobs = _imp->_holder->getKnobs();
        for(U32 i = 0 ; i < otherKnobs.size();++i)
        {
            if(otherKnobs[i]->getDescription() == it->second){
                _imp->_slaves.insert(std::make_pair(it->first,otherKnobs[i].get()));
                break;
            }
        }
    }


    ///bracket value changes
    beginValueChange(Natron::PLUGIN_EDITED);

    const std::map<int,boost::shared_ptr<Curve> >& serializedCurves = serializationObj.getCurves();
    for(std::map<int,boost::shared_ptr<Curve> >::const_iterator it = serializedCurves.begin(); it!=serializedCurves.end();++it){
        assert(it->second);
        std::map<int,boost::shared_ptr<Curve> >::const_iterator found = _imp->_curves.find(it->first);
        if(found != _imp->_curves.end()){
            assert(found->second);
            found->second->clone(*it->second);
        }
    }

    const std::map<int,Variant>& serializedValues = serializationObj.getValues();
    for(std::map<int,Variant>::const_iterator it = serializedValues.begin();it!=serializedValues.end();++it){
        setValue(it->second,it->first,Natron::PLUGIN_EDITED);
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
    _imp->_holder->getApp()->getProject()->beginProjectWideValueChanges(reason,_imp->_holder);
}

void Knob::endValueChange(Natron::ValueChangedReason reason) {
    _imp->_holder->getApp()->getProject()->endProjectWideValueChanges(reason,_imp->_holder);
}


void Knob::evaluateValueChange(int dimension,Natron::ValueChangedReason reason){
    if(!_imp->_isInsignificant)
        _imp->updateHash(getValueForEachDimension());
    processNewValue();
    if(reason != Natron::USER_EDITED){
        emit valueChanged(dimension);
    }
    _imp->_holder->getApp()->getProject()->stackEvaluateRequest(reason,_imp->_holder,this,!_imp->_isInsignificant);
}

void Knob::onTimeChanged(SequenceTime time){
    if(isAnimationEnabled()){
        beginValueChange(Natron::TIME_CHANGED); // we do not want to force a re-evaluation
        for(int i = 0; i < getDimension();++i){
            boost::shared_ptr<Curve> curve = getCurve(i);
            if(curve && curve->isAnimated()){
                Variant v = getValueAtTime(time,i);
                setValue(v,i);
            }
        }
        endValueChange(Natron::TIME_CHANGED);

    }
}



void Knob::cloneValue(const Knob& other){
    assert(_imp->_name == other._imp->_name);
    _imp->_hashVector = other._imp->_hashVector;

    _imp->_value = other._imp->_value;

    assert(_imp->_curves.size() == other._imp->_curves.size());

    //we cannot copy directly the map of curves because the curves hold a pointer to the knob
    //we must explicitly call clone() on them
    for(CurvesMap::iterator it = _imp->_curves.begin(), itOther = other._imp->_curves.begin() ;
        it!=_imp->_curves.end();++it,++itOther){
        it->second->clone(*(itOther->second));
    }

    //same for masters & slaves map: the knobs are not refered to the same KnobHolder (i.e the same effect instance)
    //so we need to copy with the good pointers
    _imp->_masters.clear();
    const MastersMap& otherMasters = other.getMasters();
    for(MastersMap::const_iterator it = otherMasters.begin();it!=otherMasters.end();++it){
        const std::vector< boost::shared_ptr<Knob> >& holderKnobs = _imp->_holder->getKnobs();
        for(U32 i = 0 ; i < holderKnobs.size();++i)
        {
            if(holderKnobs[i]->getDescription() == it->second->getDescription()){
                _imp->_masters.insert(std::make_pair(it->first,holderKnobs[i]));
                break;
            }
        }
    }

    ///restore slaves
   _imp->_slaves.clear();
    const SlavesMap& otherSlaves = other.getSlaves();
    for(SlavesMap::const_iterator it = otherSlaves.begin();it!=otherSlaves.end();++it){
        const std::vector< boost::shared_ptr<Knob> >& holderKnobs = _imp->_holder->getKnobs();
        for(U32 i = 0 ; i < holderKnobs.size();++i)
        {
            if(holderKnobs[i]->getDescription() == it->second->getDescription()){
                _imp->_slaves.insert(std::make_pair(it->first,holderKnobs[i].get()));
                break;
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
    MastersMap::iterator itLinked = _imp->_masters.find(dimension);
    if(itLinked != _imp->_masters.end()){
        //the dimension is already linked! you must unSlave it before
        return false;
    }else{
        _imp->_masters.insert(std::make_pair(dimension,other));
    }
    other->_imp->setAsMasterOf(dimension,this);
    return true;
}

void Knob::KnobPrivate::setAsMasterOf(int dimension,Knob* other){

    std::pair<SlavesMap::iterator,SlavesMap::iterator> range = _slaves.equal_range(dimension);
    for(SlavesMap::iterator it = range.first; it!= range.second ;++it){
        if(it->second == other){
            // this knob is already a master of other
            return;
        }
    }

    ///registering other to be slaved to this knob.
    _slaves.insert(std::make_pair(dimension,other));

}

void Knob::unSlave(int dimension){
     MastersMap::iterator itLinked = _imp->_masters.find(dimension);
     if(itLinked == _imp->_masters.end()){
         //the dimension is out of range...
         return ;
     }
     itLinked->second->_imp->unMaster(this);
     _imp->_masters.erase(itLinked);

}

void Knob::KnobPrivate::unMaster(Knob* other){
    for(SlavesMap::iterator it = _slaves.begin(); it!= _slaves.end() ;++it){
        if(it->second == other){
            _slaves.erase(it);
            break;
        }
    }
}

boost::shared_ptr<Knob>  Knob::isCurveSlave(int dimension) const {
    MastersMap::iterator itLinked = _imp->_masters.find(dimension);
    if(itLinked == _imp->_masters.end()){
        //the dimension is out of range...
        return boost::shared_ptr<Knob>();
    }

    return itLinked->second;
}

const std::map<int,boost::shared_ptr<Knob> >& Knob::getMasters() const{
    return _imp->_masters;
}

const std::multimap<int,Knob*>& Knob::getSlaves() const{
    return _imp->_slaves;
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

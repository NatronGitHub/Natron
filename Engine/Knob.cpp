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
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QReadWriteLock>

#include "Global/GlobalDefines.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/KnobSerialization.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/AppInstance.h"
#include "Engine/Hash64.h"


using namespace Natron;
using std::make_pair; using std::pair;

/***************** KNOBI**********************/

bool KnobI::slaveTo(int dimension,const boost::shared_ptr<KnobI>& other,int otherDimension)
{
    return slaveTo(dimension, other, otherDimension, Natron::PLUGIN_EDITED);
}

void KnobI::onKnobSlavedTo(int dimension,const boost::shared_ptr<KnobI>&  other,int otherDimension)
{
    slaveTo(dimension, other, otherDimension, Natron::USER_EDITED);
}

void KnobI::unSlave(int dimension)
{
    unSlave(dimension, Natron::PLUGIN_EDITED);
}

void KnobI::onKnobUnSlaved(int dimension)
{
    unSlave(dimension, Natron::USER_EDITED);
}


void KnobI::deleteValueAtTime(int time,int dimension)
{
    deleteValueAtTime(time, dimension, Natron::PLUGIN_EDITED);
}

void KnobI::removeAnimation(int dimension)
{
    removeAnimation(dimension, Natron::PLUGIN_EDITED);
}

void KnobI::onKeyFrameRemoved(SequenceTime time,int dimension)
{
    deleteValueAtTime(time, dimension, Natron::USER_EDITED);
}

void KnobI::onAnimationRemoved(int dimension)
{
    removeAnimation(dimension, Natron::USER_EDITED);
}

/***********************************KNOB BASE******************************************/
typedef std::vector< std::pair< int,boost::shared_ptr<Knob> > > MastersMap;
typedef std::vector< boost::shared_ptr<Curve> > CurvesMap;

struct Knob::KnobPrivate {
    Knob* _publicInterface;
    KnobHolder*  _holder;
    std::string _description;//< the text label that will be displayed  on the GUI
    QString _name;//< the knob can have a name different than the label displayed on GUI.
                  //By default this is the same as _description but can be set by calling setName().
    bool _newLine;
    int _itemSpacing;
    
    boost::shared_ptr<Knob> _parentKnob;
    bool _IsSecret;
    std::vector<bool> _enabled;
    bool _CanUndo;
    bool _EvaluateOnChange; //< if true, a value change will never trigger an evaluation
    bool _IsPersistant;//will it be serialized?
    std::string _tooltipHint;
    bool _isAnimationEnabled;
    
    /* A variant storing all the values in any dimension. <dimension,value>*/
    mutable QReadWriteLock _valueMutex; //< protects _values
    std::vector<Variant> _values;
    std::vector<Variant> _defaultValues;
    
    int _dimension;
    /* the keys for a specific dimension*/
    CurvesMap _curves;
    
    ////curve links
    ///A slave link CANNOT be master at the same time (i.e: if _slaveLinks[i] != NULL  then _masterLinks[i] == NULL )
    mutable QReadWriteLock _mastersMutex; //< protects _masters
    MastersMap _masters; //from what knob is slaved each curve if any
    
    std::vector<Natron::AnimationLevel> _animationLevel;//< indicates for each dimension whether it is static/interpolated/onkeyframe
    
    
    mutable QMutex _betweenBeginEndMutex;
    int _betweenBeginEndCount; //< between begin/end value change count
    Natron::ValueChangedReason _beginEndReason;
    std::vector<int> _dimensionChanged; //< all the dimension changed during the begin end
    
    KnobPrivate(Knob* publicInterface,KnobHolder*  holder,int dimension,const std::string& description)
    : _publicInterface(publicInterface)
    , _holder(holder)
    , _description(description)
    , _name(description.c_str())
    , _newLine(true)
    , _itemSpacing(0)
    , _parentKnob()
    , _IsSecret(false)
    , _enabled(dimension)
    , _CanUndo(true)
    , _EvaluateOnChange(false)
    , _IsPersistant(true)
    , _tooltipHint()
    , _isAnimationEnabled(true)
    , _valueMutex(QReadWriteLock::Recursive)
    , _values(dimension)
    , _defaultValues(dimension)
    , _dimension(dimension)
    , _curves(dimension)
    , _mastersMutex()
    , _masters(dimension)
    , _animationLevel(dimension)
    , _betweenBeginEndMutex()
    , _betweenBeginEndCount(0)
    , _beginEndReason(Natron::OTHER_REASON)
    , _dimensionChanged()
    {
    }
    
};

Knob::Knob(KnobHolder* holder,const std::string& description,int dimension)
:_imp(new KnobPrivate(this,holder,dimension,description))
{
    QObject::connect(this, SIGNAL(evaluateValueChangedInMainThread(int,int)), this, SLOT(onEvaluateValueChangedInOtherThread(int,int)));
}


Knob::~Knob()
{
    emit deleted();
    if (_imp->_holder) {
        _imp->_holder->removeKnob(this);
    }
}

void Knob::populate() {
    for (int i = 0; i < _imp->_dimension ; ++i) {
        _imp->_enabled[i] = true;
        _imp->_values[i] = Variant();
        _imp->_defaultValues[i] = Variant();
        _imp->_curves[i] = boost::shared_ptr<Curve>(new Curve(this));
        _imp->_animationLevel[i] = Natron::NO_ANIMATION;
    }
}


Variant Knob::getValue(int dimension) const
{
    if (isAnimated(dimension)) {
        return getValueAtTime(getHolder()->getApp()->getTimeLine()->currentFrame(), dimension);
    }
    
    if (dimension > (int)_imp->_values.size()) {
        throw std::invalid_argument("Knob::getValue(): Dimension out of range");
    }
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getValue(master.first);
    }
    
    QReadLocker l(&_imp->_valueMutex);
    return _imp->_values[dimension];
}


Variant Knob::getValueAtTime(double time,int dimension) const
{
    if (dimension > (int)_imp->_curves.size()) {
        throw std::invalid_argument("Knob::getValueAtTime(): Dimension out of range");
    }
    
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getValueAtTime(time,master.first);
    }
    
    boost::shared_ptr<Curve> curve  = _imp->_curves[dimension];
    if (curve->getKeyFramesCount() > 0) {
        Variant ret;
        variantFromInterpolatedValue(curve->getValueAt(time), &ret);
        return ret;
    } else {
        /*if the knob as no keys at this dimension, return the value
         at the requested dimension.*/
        QReadLocker l(&_imp->_valueMutex);
        return _imp->_values[dimension];
    }
}

double Knob::getDerivativeAtTime(double time,int dimension) const
{
    if (dimension > (int)_imp->_curves.size()) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getDerivativeAtTime(time,master.first);
    }

    boost::shared_ptr<Curve> curve  = _imp->_curves[dimension];
    if (curve->getKeyFramesCount() > 0) {
        return curve->getDerivativeAt(time);
    } else {
        /*if the knob as no keys at this dimension, the derivative is 0.*/
        return 0.;
    }
}

double Knob::getIntegrateFromTimeToTime(double time1, double time2, int dimension) const
{
    if (dimension > (int)_imp->_curves.size()) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getIntegrateFromTimeToTime(time1, time2, master.first);
    }

    boost::shared_ptr<Curve> curve  = _imp->_curves[dimension];
    if (curve->getKeyFramesCount() > 0) {
        return curve->getIntegrateFromTo(time1, time2);
    } else {
        // if the knob as no keys at this dimension, the integral is trivial
        return _imp->_values[dimension].toDouble() * (time2 - time1);
    }
}

Knob::ValueChangedReturnCode Knob::setValue(const Variant& v, int dimension, Natron::ValueChangedReason reason,KeyFrame* newKey)
{
    if (0 > dimension || dimension > (int)_imp->_values.size()) {
        throw std::invalid_argument("Knob::setValue(): Dimension out of range");
    }
    
    Knob::ValueChangedReturnCode  ret = NO_KEYFRAME_ADDED;
    
    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (!isSlave(dimension)) {
        
        {
            QWriteLocker l(&_imp->_valueMutex);
            _imp->_values[dimension] = v;
        }
        
        ///Add automatically a new keyframe
        if (getAnimationLevel(dimension) != Natron::NO_ANIMATION && //< if the knob is animated
           _imp->_holder->getApp() && //< the app pointer is not NULL
           !_imp->_holder->getApp()->getProject()->isLoadingProject() && //< we're not loading the project
           (reason == Natron::USER_EDITED || reason == Natron::PLUGIN_EDITED) && //< the change was made by the user or plugin
           newKey != NULL) { //< the keyframe to set is not null
            
            SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();
            bool addedKeyFrame = setValueAtTime(time, v, dimension,reason,newKey);
            if (addedKeyFrame) {
                ret = KEYFRAME_ADDED;
            } else {
                ret = KEYFRAME_MODIFIED;
            }
            
        }
    }
    if (ret == NO_KEYFRAME_ADDED) { //the other cases already called this in setValueAtTime()
        evaluateValueChange(dimension,reason);
    }
    return ret;
}

bool Knob::setValueAtTime(int time, const Variant& v, int dimension, Natron::ValueChangedReason reason,KeyFrame* newKey)
{
    if (dimension > (int)_imp->_curves.size()) {
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
        if (curve->areKeyFramesValuesClampedToIntegers()) {
            keyFrameValue = v.toInt();
        } else if (curve->areKeyFramesValuesClampedToBooleans()) {
            keyFrameValue = (int)v.toBool();
        } else {
            keyFrameValue = v.toDouble();
        }
    }

    *newKey = KeyFrame((double)time,keyFrameValue);
    
    bool ret = curve->addKeyFrame(*newKey);
    if (reason == Natron::PLUGIN_EDITED) {
        setValue(v, dimension,Natron::OTHER_REASON,NULL);
    }
    
    if (reason != Natron::USER_EDITED) {
        emit keyFrameSet(time,dimension);
    }
    
    emit updateSlaves(dimension);
    return ret;
}

void Knob::deleteValueAtTime(int time,int dimension,Natron::ValueChangedReason reason)
{
    if (dimension > (int)_imp->_curves.size()) {
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return;
    }
    
    _imp->_curves[dimension]->removeKeyFrameWithTime((double)time);
    
    //virtual portion
    keyframeRemoved_virtual(dimension, time);
    
    if (reason != Natron::USER_EDITED) {
        emit keyFrameRemoved(time,dimension);
    }
    emit updateSlaves(dimension);
}

void Knob::removeAnimation(int dimension,Natron::ValueChangedReason reason)
{
    if (dimension > (int)_imp->_curves.size()) {
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return;
    }
    
    _imp->_curves[dimension]->clearKeyFrames();
    
    //virtual portion
    animationRemoved_virtual(dimension);
        
    if (reason != Natron::USER_EDITED) {
        emit animationRemoved(dimension);
    }
    
}

void Knob::setValue(const Variant& value,int dimension,bool turnOffAutoKeying)
{
    if (turnOffAutoKeying) {
        setValue(value,dimension,Natron::PLUGIN_EDITED,NULL);
    } else {
        KeyFrame k;
        setValue(value,dimension,Natron::PLUGIN_EDITED,&k);
    }
}

void Knob::setValueAtTime(int time, const Variant& v, int dimension)
{
    KeyFrame k;
    setValueAtTime(time,v,dimension,Natron::PLUGIN_EDITED,&k);
}



void Knob::deleteValueAtTime(int time,int dimension)
{
    deleteValueAtTime(time,dimension,Natron::PLUGIN_EDITED);
}

void Knob::removeAnimation(int dimension)
{
    removeAnimation(dimension,Natron::PLUGIN_EDITED);
}

boost::shared_ptr<Curve> Knob::getCurve(int dimension) const
{
    assert(dimension < (int)_imp->_curves.size());
    
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getCurve(master.first);
    }
    return _imp->_curves[dimension];
}

bool Knob::isAnimated(int dimension) const
{
    return getCurve(dimension)->isAnimated();
}

const std::vector<boost::shared_ptr<Curve> >& Knob::getCurves() const
{
    return _imp->_curves;
}

const std::vector<Variant>& Knob::getValueForEachDimension() const
{
    return _imp->_values;
}

std::vector<Variant> Knob::getValueForEachDimension_mt_safe() const {
    QReadLocker l(&_imp->_valueMutex);
    return _imp->_values;
}

int Knob::getDimension() const
{
    return _imp->_dimension;
}

void Knob::restoreSlaveMasterState(const KnobSerialization& serializationObj) {
    ///restore masters
    
    const std::list<ValueSerialization>& serializedValues = serializationObj.getValuesSerialized();
    int i = 0;
    for (std::list<ValueSerialization>::const_iterator it = serializedValues.begin();  it != serializedValues.end(); ++it) {
        if (it->hasMaster) {
            ///we need to cycle through all the nodes of the project to find the real master
            std::vector<Natron::Node*> allNodes;
            getHolder()->getApp()->getActiveNodes(&allNodes);
            Natron::Node* masterNode = 0;
            for (U32 k = 0; k < allNodes.size(); ++k) {
                if (allNodes[k]->getName() == it->master.masterNodeName) {
                    masterNode = allNodes[k];
                    break;
                }
            }
            if (!masterNode) {
                Natron::errorDialog("Link slave/master", getDescription() + " failed to restore the following linkage: "
                                    + it->master.masterNodeName + ". Please submit a bug report.");
                continue;
                
            }
            
            ///now that we have the master node, find the corresponding knob
            const std::vector< boost::shared_ptr<Knob> >& otherKnobs = masterNode->getKnobs();
            bool found = false;
            for (U32 j = 0 ; j < otherKnobs.size();++j) {
                if (otherKnobs[j]->getName() == it->master.masterKnobName) {
                    _imp->_masters[i].second = otherKnobs[j];
                    _imp->_masters[i].first = it->master.masterDimension;
                    emit readOnlyChanged(true,_imp->_masters[i].first);
                    found = true;
                    break;
                }
            }
            if (!found) {
                Natron::errorDialog("Link slave/master", getDescription() + " failed to restore the following linkage: "
                                    + it->master.masterKnobName + ". Please submit a bug report.");
                
            }
            

        }
        ++i;
    }
}

void Knob::load(const KnobSerialization& serializationObj)
{
    assert(_imp->_dimension == serializationObj.getDimension());
    assert(getIsPersistant()); // a non-persistent Knob should never be loaded!
    
        ///bracket value changes
    beginValueChange(Natron::OTHER_REASON);
    
    const std::list<ValueSerialization>& serializedValues = serializationObj.getValuesSerialized();
    int i = 0;
    for (std::list<ValueSerialization>::const_iterator it = serializedValues.begin();  it != serializedValues.end(); ++it) {
        _imp->_curves[i]->clone(it->curve);
        setValue(it->value,i,Natron::OTHER_REASON,NULL);
        ++i;
    }
        
    const std::string& extraData = serializationObj.getExtraData();
    if (!extraData.empty()) {
        loadExtraData(extraData.c_str());
    }
    
    ///end bracket
    endValueChange();
    emit restorationComplete();
}

void Knob::save(KnobSerialization* serializationObj) const
{
    assert(getIsPersistant()); // a non-persistent Knob should never be saved!
    serializationObj->initialize(this);
}

Knob::ValueChangedReturnCode Knob::onValueChanged(int dimension,const Variant& variant,KeyFrame* newKey)
{
    return setValue(variant, dimension,Natron::USER_EDITED,newKey);
}

void Knob::onKeyFrameSet(SequenceTime time,int dimension)
{
    KeyFrame k;
    setValueAtTime(time,getValue(dimension),dimension,Natron::USER_EDITED,&k);
}

void Knob::onKeyFrameRemoved(SequenceTime time,int dimension)
{
    deleteValueAtTime(time,dimension,Natron::USER_EDITED);
}

void Knob::onAnimationRemoved(int dimension)
{
    removeAnimation(dimension, Natron::USER_EDITED);
}

void Knob::evaluateAnimationChange()
{
    //the holder cannot be a global holder(i.e: it cannot be tied application wide)
    assert(_imp->_holder->getApp());
    SequenceTime time = _imp->_holder->getApp()->getTimeLine()->currentFrame();
    
    beginValueChange(Natron::PLUGIN_EDITED);
    bool hasEvaluatedOnce = false;
    for (int i = 0; i < getDimension();++i) {
        if (isAnimated(i)) {
            Variant v = getValueAtTime(time,i);
            setValue(v,i,Natron::PLUGIN_EDITED,NULL);
            hasEvaluatedOnce = true;
        }
    }
    if (!hasEvaluatedOnce) {
        evaluateValueChange(0, Natron::PLUGIN_EDITED);
    }
    
    endValueChange();
}

void Knob::beginValueChange(Natron::ValueChangedReason reason)
{
    {
        QMutexLocker l(&_imp->_betweenBeginEndMutex);
        _imp->_beginEndReason = reason;
        ++_imp->_betweenBeginEndCount;
    }
    _imp->_holder->notifyProjectBeginKnobsValuesChanged(reason);
}

void Knob::endValueChange()
{
    {
        QMutexLocker l(&_imp->_betweenBeginEndMutex);
        assert(_imp->_betweenBeginEndCount > 0);
        --_imp->_betweenBeginEndCount;
        
        if (_imp->_betweenBeginEndCount == 0) {
            
            processNewValue(_imp->_beginEndReason);
            if ((_imp->_beginEndReason != Natron::USER_EDITED)) {
                for (U32 i = 0; i < _imp->_dimensionChanged.size(); ++i) {
                    emit valueChanged(_imp->_dimensionChanged[i]);
                }
            }
            for (U32 i = 0; i < _imp->_dimensionChanged.size(); ++i) {
                emit updateSlaves(_imp->_dimensionChanged[i]);
            }
            _imp->_dimensionChanged.clear();
        }
    }
    _imp->_holder->notifyProjectEndKnobsValuesChanged();
}

void Knob::onEvaluateValueChangedInOtherThread(int dimension, int reason)
{
    evaluateValueChange(dimension,(Natron::ValueChangedReason)reason);
}


void Knob::evaluateValueChange(int dimension,Natron::ValueChangedReason reason)
{
    if (QThread::currentThread() != qApp->thread()) {
        emit evaluateValueChangedInMainThread(dimension, reason);
        return;
    }
    
    bool beginCalled = false;
    {
        QMutexLocker l(&_imp->_betweenBeginEndMutex);
        if (_imp->_betweenBeginEndCount == 0) {
            l.unlock();
            beginValueChange(reason);
            l.relock();
            beginCalled = true;
        }
        
        std::vector<int>::iterator foundDimensionChanged = std::find(_imp->_dimensionChanged.begin(),
                                                                     _imp->_dimensionChanged.end(), dimension);
        if (foundDimensionChanged == _imp->_dimensionChanged.end()) {
            _imp->_dimensionChanged.push_back(dimension);
        }
        
    }
    
    ///Basically just call onKnobChange on the plugin
    bool significant = (reason != Natron::TIME_CHANGED) && _imp->_EvaluateOnChange;
    _imp->_holder->notifyProjectEvaluationRequested(reason, this, significant);

    
    if (beginCalled) {
        endValueChange();
    }
}

void Knob::onMasterChanged(int dimension)
{
    evaluateValueChange(dimension, Natron::PLUGIN_EDITED);
}

void Knob::onTimeChanged(SequenceTime time)
{
    //setValue's calls compression is taken care of above.
    for (int i = 0; i < getDimension(); ++i) {
        boost::shared_ptr<Curve> c = getCurve(i);
        if (c->getKeyFramesCount() > 0) {
            Variant v = getValueAtTime(time,i);
            setValue(v,i,Natron::TIME_CHANGED,NULL);
        }
    }
}



void Knob::cloneValue(const Knob& other)
{
    assert(_imp->_name == other._imp->_name);
    
    {
        QWriteLocker l(&_imp->_valueMutex);
        _imp->_values = other._imp->_values;
    }
    
    assert(_imp->_curves.size() == other._imp->_curves.size());
    
    //we cannot copy directly the map of curves because the curves hold a pointer to the knob
    //we must explicitly call clone() on them
    for (U32 i = 0 ; i < _imp->_curves.size();++i) {
        assert(_imp->_curves[i] && other._imp->_curves[i]);
        _imp->_curves[i]->clone(*(other._imp->_curves[i]));
    }
    
    //same for masters : the knobs are not refered to the same KnobHolder (i.e the same effect instance)
    //so we need to copy with the good pointers
    {
        QWriteLocker l(&_imp->_mastersMutex);
        MastersMap otherMasters = other.getMasters_mt_safe();
        for (U32 j = 0 ; j < otherMasters.size();++j) {
            if (otherMasters[j].second) {
                _imp->_masters[j].second = otherMasters[j].second;
                _imp->_masters[j].first = otherMasters[j].first;
            }
        }
    }
    cloneExtraData(other);
}

void Knob::turnOffNewLine()
{
    _imp->_newLine = false;
}

bool Knob::isNewLineTurnedOff() const
{
    return !_imp->_newLine;
}

void Knob::setSpacingBetweenItems(int spacing)
{
    _imp->_itemSpacing = spacing;
}

void Knob::setEnabled(int dimension,bool b)
{
    _imp->_enabled[dimension] = b;
    emit enabledChanged();
}

void Knob::setAllDimensionsEnabled(bool b)
{
    for (U32 i = 0; i < _imp->_enabled.size(); ++i) {
        _imp->_enabled[i] = b;
    }
    emit enabledChanged();
}

void Knob::setSecret(bool b)
{
    _imp->_IsSecret = b;
    emit secretChanged();
}

int Knob::determineHierarchySize() const
{
    int ret = 0;
    boost::shared_ptr<Knob> current = getParentKnob();
    while(current) {
        ++ret;
        current = current->getParentKnob();
    }
    return ret;
}


const std::string& Knob::getDescription() const
{
    return _imp->_description;
}


bool Knob::hasAnimation() const
{
    for (int i = 0; i < getDimension(); ++i) {
        if (getKeyFramesCount(i) > 0) {
            return true;
        }
    }
    return false;
}

KnobHolder*  Knob::getHolder() const
{
    return _imp->_holder;
}

void Knob::setAnimationEnabled(bool val)
{
    _imp->_isAnimationEnabled = val;
}

bool Knob::isAnimationEnabled() const
{
    return canAnimate() && _imp->_isAnimationEnabled;
}

void Knob::setName(const std::string& name)
{
    _imp->_name = QString(name.c_str());
}

std::string Knob::getName() const
{
    return _imp->_name.toStdString();
}

void Knob::setParentKnob(boost::shared_ptr<Knob> knob)
{
    _imp->_parentKnob = knob;
}

boost::shared_ptr<Knob> Knob::getParentKnob() const
{
    return _imp->_parentKnob;
}

bool Knob::getIsSecret() const
{
    return _imp->_IsSecret;
}

bool Knob::isEnabled(int dimension) const
{
    assert(dimension < getDimension());
    return _imp->_enabled[dimension];
}

void Knob::setEvaluateOnChange(bool b)
{
    _imp->_EvaluateOnChange = b;
}

bool Knob::getIsPersistant() const
{
    return _imp->_IsPersistant;
}

void Knob::setIsPersistant(bool b)
{
    _imp->_IsPersistant = b;
}

void Knob::setCanUndo(bool val)
{
    _imp->_CanUndo = val;
}

bool Knob::getCanUndo() const
{
    return _imp->_CanUndo;
}

bool Knob::getEvaluateOnChange()const
{
    return _imp->_EvaluateOnChange;
}

void Knob::setHintToolTip(const std::string& hint)
{
    _imp->_tooltipHint = hint;
}

const std::string& Knob::getHintToolTip() const
{
    return _imp->_tooltipHint;
}

void Knob::onKnobSlavedTo(int dimension,const boost::shared_ptr<Knob>&  other,int otherDimension) {
    slaveTo(dimension, other, otherDimension,Natron::USER_EDITED);
}

bool Knob::slaveTo(int dimension,const boost::shared_ptr<Knob>& other,int otherDimension,Natron::ValueChangedReason reason) {
    assert(dimension < (int)_imp->_masters.size());
    assert(!other->isSlave(otherDimension));
    
    {
        QWriteLocker l(&_imp->_mastersMutex);
        if (_imp->_masters[dimension].second) {
            return false;
        }
        _imp->_masters[dimension].second = other;
        _imp->_masters[dimension].first = otherDimension;
    }
    QObject::connect(other.get(), SIGNAL(updateSlaves(int)), this, SLOT(onMasterChanged(int)));
    emit valueChanged(dimension);
    if (reason == Natron::PLUGIN_EDITED) {
        emit knobSlaved(dimension,true);
    }
    return true;

}

bool Knob::slaveTo(int dimension,const boost::shared_ptr<Knob>& other,int otherDimension)
{
    return slaveTo(dimension, other, otherDimension,Natron::PLUGIN_EDITED);
}

void Knob::onKnobUnSlaved(int dimension) {
    unSlave(dimension,Natron::USER_EDITED);
}

void Knob::unSlave(int dimension)
{
    unSlave(dimension,Natron::PLUGIN_EDITED);
}

void Knob::unSlave(int dimension,Natron::ValueChangedReason reason) {
    assert(isSlave(dimension));
    //copy the state before cloning
    {
        
        QWriteLocker l2(&_imp->_mastersMutex);
        {
            QWriteLocker l1(&_imp->_valueMutex);
            _imp->_values[dimension] =  _imp->_masters[_imp->_masters[dimension].first].second->getValue(dimension);
        }
        _imp->_curves[dimension]->clone(*( _imp->_masters[_imp->_masters[dimension].first].second->getCurve(dimension)));
        
        cloneExtraData(*_imp->_masters[dimension].second);
        
        QObject::disconnect(_imp->_masters[dimension].second.get(), SIGNAL(updateSlaves(int)), this, SLOT(onMasterChanged(int)));
        _imp->_masters[dimension].second.reset();
        _imp->_masters[dimension].first = -1;
    }
    emit valueChanged(dimension);
    if (reason == Natron::PLUGIN_EDITED) {
        emit knobSlaved(dimension, false);
    }
}


std::pair<int,boost::shared_ptr<Knob> > Knob::getMaster(int dimension) const
{
    QReadLocker l(&_imp->_mastersMutex);
    return _imp->_masters[dimension];
}

bool Knob::isSlave(int dimension) const
{
    QReadLocker l(&_imp->_mastersMutex);
    return bool(_imp->_masters[dimension].second);
}

std::vector< std::pair<int,boost::shared_ptr<Knob> > > Knob::getMasters_mt_safe() const
{
    QReadLocker l(&_imp->_mastersMutex);
    return _imp->_masters;
}

void Knob::setAnimationLevel(int dimension,Natron::AnimationLevel level)
{
    assert(dimension < (int)_imp->_animationLevel.size());
    
    _imp->_animationLevel[dimension] = level;
    animationLevelChanged((int)level);
}

Natron::AnimationLevel Knob::getAnimationLevel(int dimension) const
{
    if (dimension > (int)_imp->_animationLevel.size()) {
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }
    
    return _imp->_animationLevel[dimension];
}

void Knob::variantFromInterpolatedValue(double interpolated,Variant* returnValue) const
{
    returnValue->setValue<double>(interpolated);
}


void Knob::setDefaultValues(const std::vector<Variant>& values)
{
    assert(values.size() == _imp->_defaultValues.size());
    _imp->_defaultValues = values;
    _imp->_values = _imp->_defaultValues;
    processNewValue(Natron::PLUGIN_EDITED);
}

void Knob::setDefaultValue(const Variant& v,int dimension)
{
    assert(dimension < getDimension());
    _imp->_defaultValues[dimension] = v;
    _imp->_values[dimension] = v;
    processNewValue(Natron::PLUGIN_EDITED);
}

void Knob::resetToDefaultValue(int dimension)
{
    if (typeName() == Button_Knob::typeNameStatic()) {
        return;
    }
    removeAnimation(dimension);
    setValue(_imp->_defaultValues[dimension], dimension,true);
}


bool Knob::getKeyFrameTime(int index,int dimension,double* time) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getKeyFrameTime(index,master.first,time);
    }
    
    assert(dimension < getDimension());
    if (!isAnimated(dimension)) {
        return false;
    }
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getKeyFrameWithIndex(index, &kf);
    if (ret) {
        *time = kf.getTime();
    }
    return ret;
}


bool Knob::getLastKeyFrameTime(int dimension,double* time) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getLastKeyFrameTime(master.first,time);
    }
    
    assert(dimension < getDimension());
    if (!isAnimated(dimension)) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    *time = curve->getMaximumTimeCovered();
    return true;
}

bool Knob::getFirstKeyFrameTime(int dimension,double* time) const
{
    return getKeyFrameTime(0, dimension, time);
}

int Knob::getKeyFramesCount(int dimension) const
{
    //get curve forwards it to the master
    return getCurve(dimension)->getKeyFramesCount();
}

bool Knob::getNearestKeyFrameTime(int dimension,double time,double* nearestTime) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getNearestKeyFrameTime(master.first,time,nearestTime);
    }
    
    assert(dimension < getDimension());
    if (!isAnimated(dimension)) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    KeyFrame kf;
    bool ret = curve->getNearestKeyFrameWithTime(time, &kf);
    if (ret) {
        *nearestTime = kf.getTime();
    }
    return ret;
}

int Knob::getKeyFrameIndex(int dimension, double time) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getKeyFrameIndex(master.first,time);
    }
    
    assert(dimension < getDimension());
    if (!isAnimated(dimension)) {
        return -1;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    return curve->keyFrameIndex(time);
}


bool Knob::getKeyFrameValueByIndex(int dimension,int index,Variant* value) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<Knob> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getKeyFrameValueByIndex(master.first,index,value);
    }
    
    assert(dimension < getDimension());
    if (!getKeyFramesCount(dimension)) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    KeyFrame kf;
    bool found =  curve->getKeyFrameWithIndex(index, &kf);
    if (found) {
        variantFromInterpolatedValue(kf.getValue(),value);
    }
    return found;
}

/***************************KNOB HOLDER******************************************/

KnobHolder::KnobHolder(AppInstance* appInstance):
_app(appInstance)
, _knobs()
, _knobsInitialized(false)
, _isSlave(false)
{
}

KnobHolder::~KnobHolder()
{
    for (U32 i = 0; i < _knobs.size(); ++i) {
        _knobs[i]->_imp->_holder = NULL;
    }
}

void KnobHolder::initializeKnobsPublic()
{
    initializeKnobs();
    _knobsInitialized = true;
}

void KnobHolder::cloneKnobs(const KnobHolder& other)
{
    assert(_knobs.size() == other._knobs.size());
    for (U32 i = 0 ; i < other._knobs.size();++i) {
        _knobs[i]->cloneValue(*(other._knobs[i]));
    }
}

void KnobHolder::removeKnob(Knob* knob)
{
    for (U32 i = 0; i < _knobs.size() ; ++i) {
        if (_knobs[i].get() == knob) {
            _knobs.erase(_knobs.begin()+i);
            break;
        }
    }
}

void KnobHolder::refreshAfterTimeChange(SequenceTime time)
{
    for (U32 i = 0; i < _knobs.size() ; ++i) {
        _knobs[i]->onTimeChanged(time);
    }
}

void KnobHolder::notifyProjectBeginKnobsValuesChanged(Natron::ValueChangedReason reason)
{
    if (!_knobsInitialized) {
        return;
    }
    
    if (_app) {
        getApp()->getProject()->beginProjectWideValueChanges(reason, this);
    }
}

void KnobHolder::notifyProjectEndKnobsValuesChanged()
{
    if (!_knobsInitialized) {
        return;
    }
    
    if (_app) {
        getApp()->getProject()->endProjectWideValueChanges(this);
    }
}

void KnobHolder::notifyProjectEvaluationRequested(Natron::ValueChangedReason reason,Knob* k,bool significant)
{
    if (!_knobsInitialized) {
        return;
    }
    
    if (_app) {
        getApp()->getProject()->stackEvaluateRequest(reason,this,k,significant);
    } else {
        onKnobValueChanged(k, reason);
    }
}



boost::shared_ptr<Knob> KnobHolder::getKnobByName(const std::string& name) const
{
    const std::vector<boost::shared_ptr<Knob> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size() ; ++i) {
        if (knobs[i]->getName() == name) {
            return knobs[i];
        }
    }
    return boost::shared_ptr<Knob>();
}

const std::vector< boost::shared_ptr<Knob> >& KnobHolder::getKnobs() const {
    ///MT-safe since it never changes
    return _knobs;
}


void KnobHolder::slaveAllKnobs(KnobHolder* other) {
    
    if (_isSlave) {
        return;
    }
    const std::vector<boost::shared_ptr<Knob> >& otherKnobs = other->getKnobs();
    const std::vector<boost::shared_ptr<Knob> >& thisKnobs = getKnobs();
    for (U32 i = 0; i < otherKnobs.size(); ++i) {
        boost::shared_ptr<Knob> foundKnob;
        for (U32 j = 0; j < thisKnobs.size(); ++j) {
            if (thisKnobs[j]->getName() == otherKnobs[i]->getName()) {
                foundKnob = thisKnobs[j];
                break;
            }
        }
        assert(foundKnob);
        for (int j = 0; j < foundKnob->getDimension(); ++j) {
            foundKnob->slaveTo(j, otherKnobs[i], j);
        }
    }
    _isSlave = true;
    onSlaveStateChanged(true,other);

}

bool KnobHolder::isSlave() const  {
    return _isSlave;
}

void KnobHolder::unslaveAllKnobs() {
    if (!_isSlave) {
        return;
    }
    const std::vector<boost::shared_ptr<Knob> >& thisKnobs = getKnobs();
    for (U32 i = 0; i < thisKnobs.size(); ++i) {
        for (int j = 0; j < thisKnobs[i]->getDimension(); ++j) {
            if (thisKnobs[i]->isSlave(j)) {
                thisKnobs[i]->unSlave(j);
            }
        }
    }
    _isSlave = false;
    onSlaveStateChanged(false,NULL);
}

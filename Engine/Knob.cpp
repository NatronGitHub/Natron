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
#include "KnobImpl.h"

#include <QtCore/QDataStream>
#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include "Global/GlobalDefines.h"
#include "Engine/Node.h"
#include "Engine/ViewerInstance.h"
#include "Engine/TimeLine.h"
#include "Engine/Curve.h"
#include "Engine/KnobFile.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/KnobSerialization.h"
#include "Engine/ThreadStorage.h"

#include "Engine/AppManager.h"
#include "Engine/LibraryBinary.h"
#include "Engine/AppInstance.h"
#include "Engine/Hash64.h"
#include "Engine/StringAnimationManager.h"


using namespace Natron;
using std::make_pair; using std::pair;

KnobSignalSlotHandler::KnobSignalSlotHandler(boost::shared_ptr<KnobI> knob)
: QObject()
, k(knob)
{
    QObject::connect(this, SIGNAL(evaluateValueChangedInMainThread(int,int)), this, SLOT(onEvaluateValueChangedInOtherThread(int,int)));
}

void KnobSignalSlotHandler::onKeyFrameSet(SequenceTime time,int dimension)
{
    k->onKeyFrameSet(time, dimension);
}

void KnobSignalSlotHandler::onKeyFrameRemoved(SequenceTime time,int dimension)
{
    k->onKeyFrameRemoved(time, dimension);
}

void KnobSignalSlotHandler::onTimeChanged(SequenceTime t)
{
    k->onTimeChanged(t);
}

void KnobSignalSlotHandler::onAnimationRemoved(int dimension)
{
    k->onAnimationRemoved(dimension);
}

void KnobSignalSlotHandler::onMasterChanged(int dimension)
{
    k->evaluateValueChange(dimension, Natron::SLAVE_REFRESH);
}

void KnobSignalSlotHandler::onEvaluateValueChangedInOtherThread(int dimension, int reason)
{
    assert(QThread::currentThread() == qApp->thread());
    k->evaluateValueChange(dimension,(Natron::ValueChangedReason)reason);
}

/***************** KNOBI**********************/

bool KnobI::slaveTo(int dimension,const boost::shared_ptr<KnobI>& other,int otherDimension,bool ignoreMasterPersistence)
{
    return slaveTo(dimension, other, otherDimension, Natron::PLUGIN_EDITED,ignoreMasterPersistence);
}

void KnobI::onKnobSlavedTo(int dimension,const boost::shared_ptr<KnobI>&  other,int otherDimension)
{
    slaveTo(dimension, other, otherDimension, Natron::USER_EDITED);
}

void KnobI::unSlave(int dimension,bool copyState)
{
    unSlave(dimension, Natron::PLUGIN_EDITED,copyState);
}

void KnobI::onKnobUnSlaved(int dimension)
{
    unSlave(dimension, Natron::USER_EDITED,true);
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

/***********************************KNOB HELPER******************************************/

///for each dimension, the dimension of the master this dimension is linked to, and a pointer to the master
typedef std::vector< std::pair< int,boost::shared_ptr<KnobI> > > MastersMap;

///a curve for each dimension
typedef std::vector< boost::shared_ptr<Curve> > CurvesMap;

struct KnobHelper::KnobHelperPrivate {
    KnobHelper* publicInterface;
    KnobHolder* holder;
    std::string description;//< the text label that will be displayed  on the GUI
    std::string name;//< the knob can have a name different than the label displayed on GUI.
                  //By default this is the same as _description but can be set by calling setName().
    bool newLine;
    int itemSpacing;
    
    boost::shared_ptr<KnobI> parentKnob;
    bool IsSecret;
    std::vector<bool> enabled;
    bool CanUndo;
    bool EvaluateOnChange; //< if true, a value change will never trigger an evaluation
    bool IsPersistant;//will it be serialized?
    std::string tooltipHint;
    bool isAnimationEnabled;
    
    int dimension;
    /* the keys for a specific dimension*/
    CurvesMap curves;
    
    ////curve links
    ///A slave link CANNOT be master at the same time (i.e: if _slaveLinks[i] != NULL  then _masterLinks[i] == NULL )
    mutable QReadWriteLock mastersMutex; //< protects _masters & ignoreMasterPersistence
    MastersMap masters; //from what knob is slaved each curve if any
    bool ignoreMasterPersistence; //< when true masters will not be serialized
    
    mutable QMutex animationLevelMutex;
    std::vector<Natron::AnimationLevel> animationLevel;//< indicates for each dimension whether it is static/interpolated/onkeyframe
    
    
    mutable QMutex betweenBeginEndMutex;
    int betweenBeginEndCount; //< between begin/end value change count
    Natron::ValueChangedReason beginEndReason;
    std::vector<int> dimensionChanged; //< all the dimension changed during the begin end
    
    bool declaredByPlugin; //< was the knob declared by a plug-in or added by Natron
    
    boost::shared_ptr<OfxParamOverlayInteract> customInteract;
    KnobGuiI* gui;
    void* ofxParamHandle;
    
    bool isInstanceSpecific;
    

    KnobHelperPrivate(KnobHelper* publicInterface_,
                      KnobHolder*  holder_,
                      int dimension_,
                      const std::string& description_,
                      bool declaredByPlugin_)
    : publicInterface(publicInterface_)
    , holder(holder_)
    , description(description_)
    , name(description_.c_str())
    , newLine(true)
    , itemSpacing(0)
    , parentKnob()
    , IsSecret(false)
    , enabled(dimension_)
    , CanUndo(true)
    , EvaluateOnChange(true)
    , IsPersistant(true)
    , tooltipHint()
    , isAnimationEnabled(true)
    , dimension(dimension_)
    , curves(dimension_)
    , mastersMutex()
    , masters(dimension_)
    , ignoreMasterPersistence(false)
    , animationLevelMutex()
    , animationLevel(dimension_)
    , betweenBeginEndMutex(QMutex::Recursive)
    , betweenBeginEndCount(0)
    , beginEndReason(Natron::PROJECT_LOADING)
    , dimensionChanged()
    , declaredByPlugin(declaredByPlugin_)
    , customInteract()
    , gui(0)
    , ofxParamHandle(0)
    , isInstanceSpecific(false)
    {
    }
    
};

KnobHelper::KnobHelper(KnobHolder* holder,const std::string& description,int dimension,bool declaredByPlugin)
: _signalSlotHandler()
, _imp(new KnobHelperPrivate(this,holder,dimension,description,declaredByPlugin))
{
}


KnobHelper::~KnobHelper()
{
    if (_signalSlotHandler) {
        _signalSlotHandler->s_deleted();
    }
    if (_imp->holder) {
        _imp->holder->removeKnob(this);
    }
}

void KnobHelper::setKnobGuiPointer(KnobGuiI* ptr)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->gui = ptr;
}

KnobGuiI* KnobHelper::getKnobGuiPointer() const
{
    return _imp->gui;
}

bool KnobHelper::isDeclaredByPlugin() const
{
    return _imp->declaredByPlugin;
}

void KnobHelper::setAsInstanceSpecific()
{
    _imp->isInstanceSpecific = true;
}

bool KnobHelper::isInstanceSpecific() const
{
    return _imp->isInstanceSpecific;
}

void KnobHelper::populate() {
    for (int i = 0; i < _imp->dimension ; ++i) {
        _imp->enabled[i] = true;
        _imp->curves[i] = boost::shared_ptr<Curve>(new Curve(this));
        _imp->animationLevel[i] = Natron::NO_ANIMATION;
    }
}

void KnobHelper::setSignalSlotHandler(const boost::shared_ptr<KnobSignalSlotHandler>& handler)
{
    _signalSlotHandler = handler;
}



double KnobHelper::getDerivativeAtTime(double time,int dimension) const
{
    if (dimension > (int)_imp->curves.size()) {
        throw std::invalid_argument("Knob::getDerivativeAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getDerivativeAtTime(time,master.first);
    }

    boost::shared_ptr<Curve> curve  = _imp->curves[dimension];
    if (curve->getKeyFramesCount() > 0) {
        return curve->getDerivativeAt(time);
    } else {
        /*if the knob as no keys at this dimension, the derivative is 0.*/
        return 0.;
    }
}



void KnobHelper::deleteValueAtTime(int time,int dimension,Natron::ValueChangedReason reason)
{
    if (dimension > (int)_imp->curves.size()) {
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return;
    }
    
    try {
        _imp->curves[dimension]->removeKeyFrameWithTime((double)time);
    } catch (const std::exception& e) {
        qDebug() << e.what();
    }
    
    //virtual portion
    keyframeRemoved_virtual(dimension, time);
    
    if (reason != Natron::USER_EDITED) {
        _signalSlotHandler->s_keyFrameRemoved(time,dimension);
    }
    _signalSlotHandler->s_updateSlaves(dimension);
    checkAnimationLevel(dimension);
}

void KnobHelper::removeAnimation(int dimension, Natron::ValueChangedReason reason)
{
    assert(0 <= dimension);
    if (dimension < 0 || (int)_imp->curves.size() <= dimension) {
        throw std::invalid_argument("Knob::deleteValueAtTime(): Dimension out of range");
    }

    ///if the knob is slaved to another knob,return, because we don't want the
    ///gui to be unsynchronized with what lies internally.
    if (isSlave(dimension)) {
        return;
    }
    
    _imp->curves[dimension]->clearKeyFrames();
    
    //virtual portion
    animationRemoved_virtual(dimension);
        
    if (reason != Natron::USER_EDITED) {
        _signalSlotHandler->s_animationRemoved(dimension);
    }
    
}

boost::shared_ptr<Curve> KnobHelper::getCurve(int dimension) const
{
    assert(0 <= dimension && dimension < (int)_imp->curves.size());
    
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getCurve(master.first);
    }
    return _imp->curves[dimension];
}

bool KnobHelper::isAnimated(int dimension) const
{
    return getCurve(dimension)->isAnimated();
}

const std::vector<boost::shared_ptr<Curve> >& KnobHelper::getCurves() const
{
    return _imp->curves;
}


int KnobHelper::getDimension() const
{
    return _imp->dimension;
}

void KnobHelper::beginValueChange(Natron::ValueChangedReason reason)
{
    {
        QMutexLocker l(&_imp->betweenBeginEndMutex);
        _imp->beginEndReason = reason;
        ++_imp->betweenBeginEndCount;
    }
    if (getHolder()) {
        _imp->holder->notifyProjectBeginKnobsValuesChanged(reason);
    }
}

void KnobHelper::endValueChange()
{
    {
        QMutexLocker l(&_imp->betweenBeginEndMutex);
        assert(_imp->betweenBeginEndCount > 0);
        --_imp->betweenBeginEndCount;
        
        if (_imp->betweenBeginEndCount == 0) {
            
            processNewValue(_imp->beginEndReason);
            
            if (_signalSlotHandler && _imp->beginEndReason != Natron::SLAVE_REFRESH) {
                if ((_imp->beginEndReason != Natron::USER_EDITED)) {
                    for (U32 i = 0; i < _imp->dimensionChanged.size(); ++i) {
                        _signalSlotHandler->s_valueChanged(_imp->dimensionChanged[i]);
                    }
                }
                for (U32 i = 0; i < _imp->dimensionChanged.size(); ++i) {
                    _signalSlotHandler->s_updateSlaves(_imp->dimensionChanged[i]);
                    checkAnimationLevel(i);
                }

            }
            _imp->dimensionChanged.clear();
        }
    }
    if (getHolder()) {
        _imp->holder->notifyProjectEndKnobsValuesChanged();
    }
}


void KnobHelper::evaluateValueChange(int dimension,Natron::ValueChangedReason reason)
{
    if (QThread::currentThread() != qApp->thread()) {
        _signalSlotHandler->s_evaluateValueChangedInMainThread(dimension, reason);
        return;
    }
    
    bool beginCalled = false;
    {
        QMutexLocker l(&_imp->betweenBeginEndMutex);
        if (_imp->betweenBeginEndCount == 0) {
            beginValueChange(reason);
            beginCalled = true;
        }
        
        std::vector<int>::iterator foundDimensionChanged = std::find(_imp->dimensionChanged.begin(),
                                                                     _imp->dimensionChanged.end(), dimension);
        if (foundDimensionChanged == _imp->dimensionChanged.end()) {
            _imp->dimensionChanged.push_back(dimension);
        }
        
    }
    
    if (getHolder()) {
        ///Basically just call onKnobChange on the plugin
        bool significant = (reason != Natron::TIME_CHANGED) && _imp->EvaluateOnChange;
        _imp->holder->notifyProjectEvaluationRequested(reason, this, significant);
    }
    
    if (beginCalled) {
        endValueChange();
    }
}

void KnobHelper::turnOffNewLine()
{
    _imp->newLine = false;
}

bool KnobHelper::isNewLineTurnedOff() const
{
    return !_imp->newLine;
}

void KnobHelper::setSpacingBetweenItems(int spacing)
{
    _imp->itemSpacing = spacing;
}

void KnobHelper::setEnabled(int dimension,bool b)
{
    _imp->enabled[dimension] = b;
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void KnobHelper::setAllDimensionsEnabled(bool b)
{
    for (U32 i = 0; i < _imp->enabled.size(); ++i) {
        _imp->enabled[i] = b;
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_enabledChanged();
    }
}

void KnobHelper::setSecret(bool b)
{
    _imp->IsSecret = b;
    
    ///the knob was revealed , refresh its gui to the current time
    if (!b && _imp->holder && _imp->holder->getApp()) {
        onTimeChanged(_imp->holder->getApp()->getTimeLine()->currentFrame());
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_secretChanged();
    }
}

int KnobHelper::determineHierarchySize() const
{
    int ret = 0;
    boost::shared_ptr<KnobI> current = getParentKnob();
    while(current) {
        ++ret;
        current = current->getParentKnob();
    }
    return ret;
}


const std::string& KnobHelper::getDescription() const
{
    return _imp->description;
}


bool KnobHelper::hasAnimation() const
{
    for (int i = 0; i < getDimension(); ++i) {
        if (getKeyFramesCount(i) > 0) {
            return true;
        }
    }
    return false;
}

KnobHolder*  KnobHelper::getHolder() const
{
    return _imp->holder;
}

void KnobHelper::setAnimationEnabled(bool val)
{
    _imp->isAnimationEnabled = val;
}

bool KnobHelper::isAnimationEnabled() const
{
    return canAnimate() && _imp->isAnimationEnabled;
}

void KnobHelper::setName(const std::string& name)
{
    _imp->name = name;
}

std::string KnobHelper::getName() const
{
    return _imp->name;
}

void KnobHelper::setParentKnob(boost::shared_ptr<KnobI> knob)
{
    _imp->parentKnob = knob;
}

boost::shared_ptr<KnobI> KnobHelper::getParentKnob() const
{
    return _imp->parentKnob;
}

bool KnobHelper::getIsSecret() const
{
    return _imp->IsSecret;
}

bool KnobHelper::isEnabled(int dimension) const
{
    assert(0 <= dimension && dimension < getDimension());
    return _imp->enabled[dimension];
}

void KnobHelper::setDirty(bool d)
{
    _signalSlotHandler->s_setDirty(d);
}

void KnobHelper::setEvaluateOnChange(bool b)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->EvaluateOnChange = b;
}

bool KnobHelper::getIsPersistant() const
{
    return _imp->IsPersistant;
}

void KnobHelper::setIsPersistant(bool b)
{
    _imp->IsPersistant = b;
}

void KnobHelper::setCanUndo(bool val)
{
    _imp->CanUndo = val;
}

bool KnobHelper::getCanUndo() const
{
    return _imp->CanUndo;
}

bool KnobHelper::getEvaluateOnChange()const
{
    return _imp->EvaluateOnChange;
}

void KnobHelper::setHintToolTip(const std::string& hint)
{
    _imp->tooltipHint = hint;
}

const std::string& KnobHelper::getHintToolTip() const
{
    return _imp->tooltipHint;
}

void KnobHelper::setCustomInteract(const boost::shared_ptr<OfxParamOverlayInteract>& interactDesc)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->customInteract = interactDesc;
}

boost::shared_ptr<OfxParamOverlayInteract> KnobHelper::getCustomInteract() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->customInteract;
}

void KnobHelper::swapOpenGLBuffers()
{
    
}

void KnobHelper::redraw()
{
    if (_imp->gui) {
        _imp->gui->redraw();
    }
}

void KnobHelper::getViewportSize(double &width, double &height) const
{
    if (_imp->gui) {
        _imp->gui->getViewportSize(width, height);
    } else {
        width = 0;
        height = 0;
    }
}

void KnobHelper::getPixelScale(double& xScale, double& yScale) const
{
    if (_imp->gui) {
        _imp->gui->getPixelScale(xScale, yScale);
    } else {
        xScale = 0;
        yScale = 0;
    }
}

void KnobHelper::getBackgroundColour(double &r, double &g, double &b) const
{
    if (_imp->gui) {
        _imp->gui->getBackgroundColour(r, g, b);
    } else {
        r = 0;
        g = 0;
        b = 0;
    }
}

void KnobHelper::setOfxParamHandle(void* ofxParamHandle)
{
    assert(QThread::currentThread() == qApp->thread());
    _imp->ofxParamHandle = ofxParamHandle;
}

void* KnobHelper::getOfxParamHandle() const
{
    assert(QThread::currentThread() == qApp->thread());
    return _imp->ofxParamHandle;
}

bool KnobHelper::isMastersPersistenceIgnored() const
{
    QReadLocker l(&_imp->mastersMutex);
    return _imp->ignoreMasterPersistence;
}

bool KnobHelper::slaveTo(int dimension,
                         const boost::shared_ptr<KnobI>& other,
                         int otherDimension,
                         Natron::ValueChangedReason reason,
                         bool ignoreMasterPersistence) {
    assert(0 <= dimension && dimension < (int)_imp->masters.size());
    assert(!other->isSlave(otherDimension));
    
    {
        QWriteLocker l(&_imp->mastersMutex);
        if (_imp->masters[dimension].second) {
            return false;
        }
        _imp->ignoreMasterPersistence = ignoreMasterPersistence;
        _imp->masters[dimension].second = other;
        _imp->masters[dimension].first = otherDimension;
    }
    
    boost::shared_ptr<KnobHelper> helper = boost::dynamic_pointer_cast<KnobHelper>(other);
    assert(helper);
    
    if (helper->_signalSlotHandler && _signalSlotHandler) {
        QObject::connect(helper->_signalSlotHandler.get(), SIGNAL(updateSlaves(int)), _signalSlotHandler.get(), SLOT(onMasterChanged(int)));
    }
    if (_signalSlotHandler) {
        _signalSlotHandler->s_valueChanged(dimension);
        if (reason == Natron::PLUGIN_EDITED) {
            _signalSlotHandler->s_knobSlaved(dimension,true);
        }
        checkAnimationLevel(dimension);
    }
    return true;

}


std::pair<int,boost::shared_ptr<KnobI> > KnobHelper::getMaster(int dimension) const
{
    assert(dimension >= 0);
    QReadLocker l(&_imp->mastersMutex);
    return _imp->masters[dimension];
}

void KnobHelper::resetMaster(int dimension)
{
    assert(dimension >= 0);
    _imp->masters[dimension].second.reset();
    _imp->masters[dimension].first = -1;
    _imp->ignoreMasterPersistence = false;
}

bool KnobHelper::isSlave(int dimension) const
{
    assert(dimension >= 0);
    QReadLocker l(&_imp->mastersMutex);
    return bool(_imp->masters[dimension].second);
}

std::vector< std::pair<int,boost::shared_ptr<KnobI> > > KnobHelper::getMasters_mt_safe() const
{
    QReadLocker l(&_imp->mastersMutex);
    return _imp->masters;
}

void KnobHelper::checkAnimationLevel(int dimension)
{
    AnimationLevel level = Natron::NO_ANIMATION;
    if (getHolder() && getHolder()->getApp()) {
        
        boost::shared_ptr<Curve> c = getCurve(dimension);
        SequenceTime time = getHolder()->getApp()->getTimeLine()->currentFrame();
        if (c->getKeyFramesCount() > 0) {
            KeyFrame kf;
            bool found = c->getKeyFrameWithTime(time, &kf);;
            if (found) {
                level = Natron::ON_KEYFRAME;
            } else {
                level = Natron::INTERPOLATED_VALUE;
            }
        } else {
            level = Natron::NO_ANIMATION;
        }
    }
    if (level != getAnimationLevel(dimension)) {
        setAnimationLevel(dimension,level);
    }
}


void KnobHelper::setAnimationLevel(int dimension,Natron::AnimationLevel level)
{
    {
        QMutexLocker l(&_imp->animationLevelMutex);
        assert(dimension < (int)_imp->animationLevel.size());
        _imp->animationLevel[dimension] = level;
    }
    _signalSlotHandler->s_animationLevelChanged((int)level);
}

Natron::AnimationLevel KnobHelper::getAnimationLevel(int dimension) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getAnimationLevel(master.first);
    }
    
    QMutexLocker l(&_imp->animationLevelMutex);
    if (dimension > (int)_imp->animationLevel.size()) {
        throw std::invalid_argument("Knob::getAnimationLevel(): Dimension out of range");
    }
    
    return _imp->animationLevel[dimension];
}


bool KnobHelper::getKeyFrameTime(int index, int dimension, double* time) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getKeyFrameTime(index,master.first,time);
    }
    
    assert(0 <= dimension && dimension < getDimension());
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


bool KnobHelper::getLastKeyFrameTime(int dimension,double* time) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getLastKeyFrameTime(master.first,time);
    }
    
    assert(0 <= dimension && dimension < getDimension());
    if (!isAnimated(dimension)) {
        return false;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    *time = curve->getMaximumTimeCovered();
    return true;
}

bool KnobHelper::getFirstKeyFrameTime(int dimension,double* time) const
{
    return getKeyFrameTime(0, dimension, time);
}

int KnobHelper::getKeyFramesCount(int dimension) const
{
    //get curve forwards it to the master
    return getCurve(dimension)->getKeyFramesCount();
}

bool KnobHelper::getNearestKeyFrameTime(int dimension,double time,double* nearestTime) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getNearestKeyFrameTime(master.first,time,nearestTime);
    }
    
    assert(0 <= dimension && dimension < getDimension());
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

int KnobHelper::getKeyFrameIndex(int dimension, double time) const
{
    ///if the knob is slaved to another knob, returns the other knob value
    std::pair<int,boost::shared_ptr<KnobI> > master = getMaster(dimension);
    if (master.second) {
        return master.second->getKeyFrameIndex(master.first,time);
    }
    
    assert(0 <= dimension && dimension < getDimension());
    if (!isAnimated(dimension)) {
        return -1;
    }
    
    boost::shared_ptr<Curve> curve = getCurve(dimension);
    assert(curve);
    
    return curve->keyFrameIndex(time);
}



/***************************KNOB HOLDER******************************************/

struct KnobHolder::KnobHolderPrivate
{
    AppInstance* app;
    std::vector< boost::shared_ptr<KnobI> > knobs;
    bool knobsInitialized;
    bool isSlave;
    
    ///Use to count the recursion in the function calls
    /* The image effect actions which may trigger a recursive action call on a single instance are...
     
     kOfxActionBeginInstanceChanged
     kOfxActionInstanceChanged
     kOfxActionEndInstanceChanged
     The interact actions which may trigger a recursive action to be called on the associated plugin instance are...
     
     kOfxInteractActionGainFocus
     kOfxInteractActionKeyDown
     kOfxInteractActionKeyRepeat
     kOfxInteractActionKeyUp
     kOfxInteractActionLoseFocus
     kOfxInteractActionPenDown
     kOfxInteractActionPenMotion
     kOfxInteractActionPenUp
     
     The image effect actions which may be called recursively are...
     
     kOfxActionBeginInstanceChanged
     kOfxActionInstanceChanged
     kOfxActionEndInstanceChanged
     kOfxImageEffectActionGetClipPreferences
     The interact actions which may be called recursively are...
     
     kOfxInteractActionDraw
     
     */
    ThreadStorage<int> actionsRecursionLevel;
    
    ///If true, when the actionsRecursionLevel hit 0, it will trigger an evaluation.
    struct EvaluationRequest {
        KnobI* requester; //< the last requester
        bool isSignificant; //< is it a significant evaluation ?
        
        EvaluationRequest()
        : requester(0) , isSignificant(false)
        {
            
        }
    };
    
    EvaluationRequest evaluateQueue;
    
    mutable QMutex paramsEditLevelMutex;
    KnobHolder::MultipleParamsEditLevel paramsEditLevel;
    
    KnobHolderPrivate(AppInstance* appInstance_)
    : app(appInstance_)
    , knobs()
    , knobsInitialized(false)
    , isSlave(false)
    , actionsRecursionLevel()
    , evaluateQueue()
    , paramsEditLevel(PARAM_EDIT_OFF)
    {
        // Initialize local data on the main-thread
        ///Don't remove the if condition otherwise this will crash because QApp is not initialized yet for Natron settings.
        if (appInstance_) {
            actionsRecursionLevel.setLocalData(0);
        }
    }
};

KnobHolder::KnobHolder(AppInstance* appInstance)
: _imp(new KnobHolderPrivate(appInstance))
{
}

KnobHolder::~KnobHolder()
{
    for (U32 i = 0; i < _imp->knobs.size(); ++i) {
        KnobHelper* helper = dynamic_cast<KnobHelper*>(_imp->knobs[i].get());
        helper->_imp->holder = 0;
    }
}

KnobHolder::MultipleParamsEditLevel KnobHolder::getMultipleParamsEditLevel() const
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    return _imp->paramsEditLevel;
}

void KnobHolder::setMultipleParamsEditLevel(KnobHolder::MultipleParamsEditLevel level)
{
    QMutexLocker l(&_imp->paramsEditLevelMutex);
    _imp->paramsEditLevel = level;
}

AppInstance* KnobHolder::getApp() const {return _imp->app;}

void KnobHolder::initializeKnobsPublic()
{
    initializeKnobs();
    _imp->knobsInitialized = true;
}

void KnobHolder::addKnob(boost::shared_ptr<KnobI> k){ _imp->knobs.push_back(k); }

void KnobHolder::removeKnob(KnobI* knob)
{
    for (U32 i = 0; i < _imp->knobs.size() ; ++i) {
        if (_imp->knobs[i].get() == knob) {
            _imp->knobs.erase(_imp->knobs.begin()+i);
            break;
        }
    }
}

void KnobHolder::refreshAfterTimeChange(SequenceTime time)
{
    for (U32 i = 0; i < _imp->knobs.size() ; ++i) {
        _imp->knobs[i]->onTimeChanged(time);
    }
}

void KnobHolder::notifyProjectBeginKnobsValuesChanged(Natron::ValueChangedReason reason)
{
    if (!_imp->knobsInitialized) {
        return;
    }
    
    if (_imp->app) {
        getApp()->getProject()->beginProjectWideValueChanges(reason, this);
    }
}

void KnobHolder::notifyProjectEndKnobsValuesChanged()
{
    if (!_imp->knobsInitialized) {
        return;
    }
    
    if (_imp->app) {
        getApp()->getProject()->endProjectWideValueChanges(this);
    }
}

void KnobHolder::notifyProjectEvaluationRequested(Natron::ValueChangedReason reason,KnobI* k,bool significant)
{
    if (!_imp->knobsInitialized) {
        return;
    }
    
    if (_imp->app) {
        getApp()->getProject()->stackEvaluateRequest(reason,this,k,significant);
    } else {
        onKnobValueChanged(k, reason);
    }
}



boost::shared_ptr<KnobI> KnobHolder::getKnobByName(const std::string& name) const
{
    const std::vector<boost::shared_ptr<KnobI> >& knobs = getKnobs();
    for (U32 i = 0; i < knobs.size() ; ++i) {
        if (knobs[i]->getName() == name) {
            return knobs[i];
        }
    }
    return boost::shared_ptr<KnobI>();
}

const std::vector< boost::shared_ptr<KnobI> >& KnobHolder::getKnobs() const {
    ///MT-safe since it never changes
    return _imp->knobs;
}


void KnobHolder::slaveAllKnobs(KnobHolder* other) {
    
    if (_imp->isSlave) {
        return;
    }
    const std::vector<boost::shared_ptr<KnobI> >& otherKnobs = other->getKnobs();
    const std::vector<boost::shared_ptr<KnobI> >& thisKnobs = getKnobs();
    for (U32 i = 0; i < otherKnobs.size(); ++i) {
        boost::shared_ptr<KnobI> foundKnob;
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
    _imp->isSlave = true;
    onSlaveStateChanged(true,other);

}

bool KnobHolder::isSlave() const  {
    return _imp->isSlave;
}

void KnobHolder::unslaveAllKnobs() {
    if (!_imp->isSlave) {
        return;
    }
    const std::vector<boost::shared_ptr<KnobI> >& thisKnobs = getKnobs();
    for (U32 i = 0; i < thisKnobs.size(); ++i) {
        for (int j = 0; j < thisKnobs[i]->getDimension(); ++j) {
            if (thisKnobs[i]->isSlave(j)) {
                thisKnobs[i]->unSlave(j,true);
            }
        }
    }
    _imp->isSlave = false;
    onSlaveStateChanged(false,(KnobHolder*)NULL);
}

void KnobHolder::beginKnobsValuesChanged_public(Natron::ValueChangedReason reason)
{
    ///cannot run in another thread.
    assert(QThread::currentThread() == qApp->thread());
    
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    beginKnobsValuesChanged(reason);
    decrementRecursionLevel();
}

void KnobHolder::endKnobsValuesChanged_public(Natron::ValueChangedReason reason)
{
    ///cannot run in another thread.
    assert(QThread::currentThread() == qApp->thread());
    
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    endKnobsValuesChanged(reason);
    decrementRecursionLevel();
}


void KnobHolder::onKnobValueChanged_public(KnobI* k,Natron::ValueChangedReason reason)
{
    ///cannot run in another thread.
    assert(QThread::currentThread() == qApp->thread());
    
    ///Recursive action, must not call assertActionIsNotRecursive()
    incrementRecursionLevel();
    onKnobValueChanged(k, reason);
    decrementRecursionLevel();
}

void KnobHolder::evaluate_public(KnobI* knob,bool isSignificant,Natron::ValueChangedReason reason)
{
    ///cannot run in another thread.
    assert(QThread::currentThread() == qApp->thread());
    _imp->evaluateQueue.isSignificant |= isSignificant;
    _imp->evaluateQueue.requester = knob;
    if (getRecursionLevel() == 0) {
        evaluate(knob, _imp->evaluateQueue.isSignificant,reason);
        _imp->evaluateQueue.requester = NULL;
        _imp->evaluateQueue.isSignificant = false;
    }
}

void KnobHolder::checkIfRenderNeeded()
{
    ///cannot run in another thread.
    assert(QThread::currentThread() == qApp->thread());
    if (getRecursionLevel() == 0 && _imp->evaluateQueue.requester != NULL) {
        evaluate(_imp->evaluateQueue.requester, _imp->evaluateQueue.isSignificant,Natron::USER_EDITED);
        _imp->evaluateQueue.requester = NULL;
        _imp->evaluateQueue.isSignificant = false;
    }
}

void KnobHolder::assertActionIsNotRecursive() const
{
#ifdef NATRON_DEBUG
    int recursionLvl = getRecursionLevel();
    
    if (getApp() && getApp()->isShowingDialog()) {
        return;
    }
    if (recursionLvl != 0) {
        qDebug() << "A non-recursive action has been called recursively.";
    }
#endif
    
}

void KnobHolder::incrementRecursionLevel()
{
    if (!_imp->actionsRecursionLevel.hasLocalData()) {
        _imp->actionsRecursionLevel.setLocalData(1);
    } else {
        _imp->actionsRecursionLevel.setLocalData(_imp->actionsRecursionLevel.localData() + 1);
    }
}


void KnobHolder::decrementRecursionLevel()
{
    assert(_imp->actionsRecursionLevel.hasLocalData());
    _imp->actionsRecursionLevel.setLocalData(_imp->actionsRecursionLevel.localData() - 1);
}

int KnobHolder::getRecursionLevel() const
{
    if (_imp->actionsRecursionLevel.hasLocalData()) {
        return _imp->actionsRecursionLevel.localData();
    } else {
        return 0;
    }
}

/***************************STRING ANIMATION******************************************/
void AnimatingString_KnobHelper::cloneExtraData(const boost::shared_ptr<KnobI>& other)
{
    AnimatingString_KnobHelper* isAnimatedString = dynamic_cast<AnimatingString_KnobHelper*>(other.get());
    if (isAnimatedString) {
        _animation->clone(isAnimatedString->getAnimation());
    }
}

void AnimatingString_KnobHelper::cloneExtraData(const boost::shared_ptr<KnobI> &other, SequenceTime offset, const RangeD* range)
{
    AnimatingString_KnobHelper* isAnimatedString = dynamic_cast<AnimatingString_KnobHelper*>(other.get());
    if (isAnimatedString) {
        _animation->clone(isAnimatedString->getAnimation(), offset, range);
    }
}

AnimatingString_KnobHelper::AnimatingString_KnobHelper(KnobHolder* holder,
                                                       const std::string &description, int dimension,bool declaredByPlugin)
: Knob<std::string>(holder,description,dimension,declaredByPlugin)
, _animation(new StringAnimationManager(this))
{
    
}

AnimatingString_KnobHelper::~AnimatingString_KnobHelper()
{
    delete _animation;
}

void AnimatingString_KnobHelper::stringToKeyFrameValue(int time,const std::string& v,double* returnValue)
{
    _animation->insertKeyFrame(time, v, returnValue);
}

void AnimatingString_KnobHelper::stringFromInterpolatedValue(double interpolated,std::string* returnValue) const
{
    _animation->stringFromInterpolatedIndex(interpolated, returnValue);
}

void AnimatingString_KnobHelper::animationRemoved_virtual(int /*dimension*/) {
    _animation->clearKeyFrames();
}

void AnimatingString_KnobHelper::keyframeRemoved_virtual(int /*dimension*/, double time) {
    _animation->removeKeyFrame(time);
}


std::string AnimatingString_KnobHelper::getStringAtTime(double time, int dimension) const {
    std::string ret;
    
    if (_animation->hasCustomInterp()) {
        bool succeeded = _animation->customInterpolation(time, &ret);
        if (!succeeded) {
            return getValue(dimension);
        } else {
            return ret;
        }
        
    }
    return ret;
}

void AnimatingString_KnobHelper::setCustomInterpolation(customParamInterpolationV1Entry_t func,void* ofxParamHandle) {
    _animation->setCustomInterpolation(func, ofxParamHandle);
}

void AnimatingString_KnobHelper::loadAnimation(const std::map<int,std::string>& keyframes)
{
    _animation->load(keyframes);
    processNewValue(Natron::PROJECT_LOADING);
}

void AnimatingString_KnobHelper::saveAnimation(std::map<int,std::string>* keyframes) const
{
    _animation->save(keyframes);
}

/***************************KNOB EXPLICIT TEMPLATE INSTANTIATION******************************************/


template class Knob<int>;
template class Knob<double>;
template class Knob<bool>;
template class Knob<std::string>;


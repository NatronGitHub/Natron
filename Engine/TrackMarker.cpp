/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TrackMarker.h"

#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/TrackerContext.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackerSerialization.h"


NATRON_NAMESPACE_ENTER;


struct TrackMarkerPrivate
{
    boost::weak_ptr<TrackerContext> context;
    
    // Defines the rectangle of the search window, this is in coordinates relative to the marker center point
    boost::shared_ptr<KnobDouble> searchWindowBtmLeft,searchWindowTopRight;
    
    // The pattern Quad defined by 4 corners relative to the center
    boost::shared_ptr<KnobDouble> patternTopLeft,patternTopRight,patternBtmRight,patternBtmLeft;
    boost::shared_ptr<KnobDouble> center,offset,weight,correlation;
    boost::shared_ptr<KnobChoice> motionModel;
    
    std::list<boost::shared_ptr<KnobI> > knobs;
    
    mutable QMutex trackMutex;
    std::set<int> userKeyframes;
    std::string trackScriptName,trackLabel;
    bool enabled;
    
    TrackMarkerPrivate(TrackMarker* publicInterface, const boost::shared_ptr<TrackerContext>& context)
    : context(context)
    , searchWindowBtmLeft()
    , searchWindowTopRight()
    , patternTopLeft()
    , patternTopRight()
    , patternBtmRight()
    , patternBtmLeft()
    , center()
    , offset()
    , weight()
    , correlation()
    , motionModel()
    , knobs()
    , trackMutex()
    , userKeyframes()
    , trackScriptName()
    , trackLabel()
    , enabled(true)
    {
        searchWindowBtmLeft.reset(new KnobDouble(publicInterface,kTrackerParamSearchWndBtmLeftLabel, 2, false));
        searchWindowBtmLeft->setName(kTrackerParamSearchWndBtmLeft);
        searchWindowBtmLeft->populate();
        searchWindowBtmLeft->setDefaultValue(-25, 0);
        searchWindowBtmLeft->setDefaultValue(-25, 1);
        knobs.push_back(searchWindowBtmLeft);
        
        searchWindowTopRight.reset(new KnobDouble(publicInterface,kTrackerParamSearchWndTopRightLabel, 2, false));
        searchWindowTopRight->setName(kTrackerParamSearchWndTopRight);
        searchWindowTopRight->populate();
        searchWindowTopRight->setDefaultValue(25, 0);
        searchWindowTopRight->setDefaultValue(25, 1);
        knobs.push_back(searchWindowTopRight);
        
        
        patternTopLeft.reset(new KnobDouble(publicInterface,kTrackerParamPatternTopLeftLabel, 2, false));
        patternTopLeft->setName(kTrackerParamPatternTopLeft);
        patternTopLeft->populate();
        patternTopLeft->setDefaultValue(-15, 0);
        patternTopLeft->setDefaultValue(15, 1);
        knobs.push_back(patternTopLeft);
        
        patternTopRight.reset(new KnobDouble(publicInterface,kTrackerParamPatternTopRightLabel, 2, false));
        patternTopRight->setName(kTrackerParamPatternTopRight);
        patternTopRight->populate();
        patternTopRight->setDefaultValue(15, 0);
        patternTopRight->setDefaultValue(15, 1);
        knobs.push_back(patternTopRight);
        
        patternBtmRight.reset(new KnobDouble(publicInterface,kTrackerParamPatternBtmRightLabel, 2, false));
        patternBtmRight->setName(kTrackerParamPatternBtmRight);
        patternBtmRight->populate();
        patternBtmRight->setDefaultValue(15, 0);
        patternBtmRight->setDefaultValue(-15, 1);
        knobs.push_back(patternBtmRight);
        
        patternBtmLeft.reset(new KnobDouble(publicInterface,kTrackerParamPatternBtmLeftLabel, 2, false));
        patternBtmLeft->setName(kTrackerParamPatternBtmLeft);
        patternBtmLeft->populate();
        patternBtmLeft->setDefaultValue(-15, 0);
        patternBtmLeft->setDefaultValue(-15, 1);
        knobs.push_back(patternBtmLeft);
        
        center.reset(new KnobDouble(publicInterface,kTrackerParamCenterLabel, 2, false));
        center->setName(kTrackerParamCenter);
        center->populate();
        knobs.push_back(center);
        
        offset.reset(new KnobDouble(publicInterface,kTrackerParamOffsetLabel, 2, false));
        offset->setName(kTrackerParamOffset);
        offset->populate();
        knobs.push_back(offset);
        
        weight.reset(new KnobDouble(publicInterface,kTrackerParamTrackWeightLabel, 1, false));
        weight->setName(kTrackerParamTrackWeight);
        weight->populate();
        weight->setDefaultValue(1.);
        weight->setAnimationEnabled(false);
        weight->setMinimum(0.);
        weight->setMaximum(1.);
        knobs.push_back(weight);
        
        motionModel.reset(new KnobChoice(publicInterface,kTrackerParamMotionModelLabel, 1, false));
        motionModel->setName(kTrackerParamMotionModel);
        motionModel->populate();
        {
            std::vector<std::string> choices,helps;
            TrackerContext::getMotionModelsAndHelps(&choices,&helps);
            motionModel->populateChoices(choices,helps);
        }
        motionModel->setDefaultValue(4);
        knobs.push_back(motionModel);
        
        correlation.reset(new KnobDouble(publicInterface,kTrackerParamCorrelationLabel, 1, false));
        correlation->setName(kTrackerParamCorrelation);
        correlation->populate();
        knobs.push_back(correlation);
    }
};

TrackMarker::TrackMarker(const boost::shared_ptr<TrackerContext>& context)
: KnobHolder(context->getNode()->getApp())
, boost::enable_shared_from_this<TrackMarker>()
, _imp(new TrackMarkerPrivate(this, context))
{
    boost::shared_ptr<KnobSignalSlotHandler> handler = _imp->center->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(keyFrameSet(double,int,int,bool)), this , SLOT(onCenterKeyframeSet(double,int,int,bool)));
    QObject::connect(handler.get(), SIGNAL(keyFrameRemoved(double,int,int)), this , SLOT(onCenterKeyframeRemoved(double,int,int)));
    QObject::connect(handler.get(), SIGNAL(keyFrameMoved(int,double,double)), this , SLOT(onCenterKeyframeMoved(int,double,double)));
    QObject::connect(handler.get(), SIGNAL(multipleKeyFramesSet(std::list<double>, int, int)), this ,
                     SLOT(onCenterKeyframesSet(std::list<double>, int, int)));
    QObject::connect(handler.get(), SIGNAL(animationRemoved(int)), this , SLOT(onCenterAnimationRemoved(int)));
    
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onCenterKnobValueChanged(int, int)));
    handler = _imp->offset->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onOffsetKnobValueChanged(int, int)));
    
    handler = _imp->correlation->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onCorrelationKnobValueChanged(int, int)));
    
    handler = _imp->weight->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onWeightKnobValueChanged(int, int)));
    
    handler = _imp->motionModel->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onMotionModelKnobValueChanged(int, int)));
    
    /*handler = _imp->patternBtmLeft->getSignalSlotHandler();
     QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onPatternBtmLeftKnobValueChanged(int, int)));
     
     handler = _imp->patternTopLeft->getSignalSlotHandler();
     QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onPatternTopLeftKnobValueChanged(int, int)));
     
     handler = _imp->patternTopRight->getSignalSlotHandler();
     QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onPatternTopRightKnobValueChanged(int, int)));
     
     handler = _imp->patternBtmRight->getSignalSlotHandler();
     QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onPatternBtmRightKnobValueChanged(int, int)));
     */
    
    handler = _imp->searchWindowBtmLeft->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onSearchBtmLeftKnobValueChanged(int, int)));
    
    handler = _imp->searchWindowTopRight->getSignalSlotHandler();
    QObject::connect(handler.get(), SIGNAL(valueChanged(int,int)), this, SLOT(onSearchTopRightKnobValueChanged(int, int)));
}


TrackMarker::~TrackMarker()
{
    
}

void
TrackMarker::clone(const TrackMarker& other)
{
    boost::shared_ptr<TrackMarker> thisShared = shared_from_this();
    boost::shared_ptr<TrackerContext> context = getContext();
    context->s_trackAboutToClone(thisShared);
    
    {
        QMutexLocker k(&_imp->trackMutex);
        _imp->trackLabel = other._imp->trackLabel;
        _imp->trackScriptName = other._imp->trackScriptName;
        _imp->userKeyframes = other._imp->userKeyframes;
        _imp->enabled = other._imp->enabled;
        
        assert(_imp->knobs.size() == other._imp->knobs.size());
        
        std::list<boost::shared_ptr<KnobI> >::iterator itOther = other._imp->knobs.begin();
        for (std::list<boost::shared_ptr<KnobI> >::iterator it =_imp->knobs.begin() ; it!= _imp->knobs.end(); ++it,++itOther) {
            (*it)->clone(*itOther);
        }
    }
    
    context->s_trackCloned(thisShared);
}

void
TrackMarker::load(const TrackSerialization& serialization)
{
    QMutexLocker k(&_imp->trackMutex);
    _imp->enabled = serialization._enabled;
    _imp->trackLabel = serialization._label;
    _imp->trackScriptName = serialization._scriptName;
    for (std::list<boost::shared_ptr<KnobSerialization> >::const_iterator it = serialization._knobs.begin(); it!=serialization._knobs.end(); ++it) {
        for (std::list<boost::shared_ptr<KnobI> >::iterator it2 = _imp->knobs.begin(); it2!=_imp->knobs.end(); ++it2) {
            if ((*it2)->getName() == (*it)->getName()) {
                (*it2)->clone((*it)->getKnob());
                break;
            }
        }
    }
    for (std::list<int>::const_iterator it = serialization._userKeys.begin(); it!= serialization._userKeys.end(); ++it) {
        _imp->userKeyframes.insert(*it);
    }
}

void
TrackMarker::save(TrackSerialization* serialization) const
{
    QMutexLocker k(&_imp->trackMutex);
    serialization->_enabled = _imp->enabled;
    serialization->_label = _imp->trackLabel;
    serialization->_scriptName = _imp->trackScriptName;
    for (std::list<boost::shared_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it!=_imp->knobs.end(); ++it) {
        boost::shared_ptr<KnobSerialization> s(new KnobSerialization(*it));
        serialization->_knobs.push_back(s);
    }
    for (std::set<int>::const_iterator it = _imp->userKeyframes.begin(); it!= _imp->userKeyframes.end(); ++it) {
        serialization->_userKeys.push_back(*it);
    }
}

boost::shared_ptr<TrackerContext>
TrackMarker::getContext() const
{
    return _imp->context.lock();
}

bool
TrackMarker::setScriptName(const std::string& name)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );
    
    if (name.empty()) {
        return false;
    }
    
    
    std::string cpy = Python::makeNameScriptFriendly(name);
    
    if (cpy.empty()) {
        return false;
    }
    
    boost::shared_ptr<TrackMarker> existingItem = getContext()->getMarkerByName(name);
    if ( existingItem && (existingItem.get() != this) ) {
        return false;
    }
    
    {
        QMutexLocker l(&_imp->trackMutex);
        _imp->trackScriptName = cpy;
    }
    return true;
}

std::string
TrackMarker::getScriptName() const
{
    QMutexLocker l(&_imp->trackMutex);
    return _imp->trackScriptName;
}

void
TrackMarker::setLabel(const std::string& label)
{
    QMutexLocker l(&_imp->trackMutex);
    _imp->trackLabel = label;
}

std::string
TrackMarker::getLabel() const
{
    QMutexLocker l(&_imp->trackMutex);
    return _imp->trackLabel;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getSearchWindowBottomLeftKnob() const
{
    return _imp->searchWindowBtmLeft;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getSearchWindowTopRightKnob() const
{
    return _imp->searchWindowTopRight;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getPatternTopLeftKnob() const
{
    return _imp->patternTopLeft;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getPatternTopRightKnob() const
{
    return _imp->patternTopRight;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getPatternBtmRightKnob() const
{
    return _imp->patternBtmRight;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getPatternBtmLeftKnob() const
{
    return _imp->patternBtmLeft;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getWeightKnob() const
{
    return _imp->weight;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getCenterKnob() const
{
    return _imp->center;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getOffsetKnob() const
{
    return _imp->offset;
}

boost::shared_ptr<KnobDouble>
TrackMarker::getCorrelationKnob() const
{
    return _imp->correlation;
}

boost::shared_ptr<KnobChoice>
TrackMarker::getMotionModelKnob() const
{
    return _imp->motionModel;
}

const std::list<boost::shared_ptr<KnobI> >&
TrackMarker::getKnobs() const
{
    return _imp->knobs;
}

bool
TrackMarker::isUserKeyframe(int time) const
{
    QMutexLocker k(&_imp->trackMutex);
    return _imp->userKeyframes.find(time) != _imp->userKeyframes.end();
}

int
TrackMarker::getPreviousKeyframe(int time) const
{
    QMutexLocker k(&_imp->trackMutex);
    for (std::set<int>::const_reverse_iterator it = _imp->userKeyframes.rbegin(); it != _imp->userKeyframes.rend(); ++it) {
        if (*it < time) {
            return *it;
        }
    }
    return INT_MIN;
}

int
TrackMarker::getNextKeyframe( int time) const
{
    QMutexLocker k(&_imp->trackMutex);
    for (std::set<int>::const_iterator it = _imp->userKeyframes.begin(); it != _imp->userKeyframes.end(); ++it) {
        if (*it > time) {
            return *it;
        }
    }
    return INT_MAX;
}

void
TrackMarker::getUserKeyframes(std::set<int>* keyframes) const
{
    QMutexLocker k(&_imp->trackMutex);
    *keyframes = _imp->userKeyframes;
}

void
TrackMarker::getCenterKeyframes(std::set<double>* keyframes) const
{
    boost::shared_ptr<Curve> curve = _imp->center->getCurve(ViewSpec(0), 0);
    assert(curve);
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
        keyframes->insert(it->getTime());
    }
}

bool
TrackMarker::isEnabled() const
{
    QMutexLocker k(&_imp->trackMutex);
    return _imp->enabled;
}

void
TrackMarker::setEnabled(bool enabled, int reason)
{
    {
        QMutexLocker k(&_imp->trackMutex);
        _imp->enabled = enabled;
    }
    getContext()->s_enabledChanged(shared_from_this(), reason);
}

int
TrackMarker::getReferenceFrame(int time, bool forward) const
{
    QMutexLocker k(&_imp->trackMutex);
    std::set<int>::iterator upper = _imp->userKeyframes.upper_bound(time);
    if (upper == _imp->userKeyframes.end()) {
        //all keys are lower than time, pick the last one
        if (!_imp->userKeyframes.empty()) {
            return *_imp->userKeyframes.rbegin();
        }
        
        // no keyframe - use the previous/next as reference
        return forward ? time - 1 : time + 1;
    } else {
        if (upper == _imp->userKeyframes.begin()) {
            ///all keys are greater than time
            return *upper;
        }
        
        int upperKeyFrame = *upper;
        ///If we find "time" as a keyframe, then use it
        --upper;
        
        int lowerKeyFrame = *upper;
        
        if (lowerKeyFrame == time) {
            return time;
        }
        
        /// return the nearest from time
        return (time - lowerKeyFrame) < (upperKeyFrame - time) ? lowerKeyFrame : upperKeyFrame;
    }
}

void
TrackMarker::resetCenter()
{
    boost::shared_ptr<TrackerContext> context = getContext();
    assert(context);
    NodePtr input = context->getNode()->getInput(0);
    if (input) {
        SequenceTime time = input->getApp()->getTimeLine()->currentFrame();
        RenderScale scale;
        scale.x = scale.y = 1;
        RectD rod;
        bool isProjectFormat;
        Natron::StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(), time, scale, ViewIdx(0), &rod, &isProjectFormat);
        Natron::Point center;
        center.x = 0;
        center.y = 0;
        if (stat == Natron::eStatusOK) {
            center.x = (rod.x1 + rod.x2) / 2.;
            center.y = (rod.y1 + rod.y2) / 2.;
        }
        _imp->center->setValue(center.x, ViewSpec::current(), 0);
        _imp->center->setValue(center.y, ViewSpec::current(), 1);
    }
}

void
TrackMarker::resetOffset()
{
    for (int i = 0; i < _imp->offset->getDimension(); ++i) {
        _imp->offset->resetToDefaultValue(i);
    }
}


void
TrackMarker::resetTrack()
{
    
    Natron::Point curCenter;
    curCenter.x = _imp->center->getValue(0);
    curCenter.y = _imp->center->getValue(1);
    
    EffectInstPtr effect = getContext()->getNode()->getEffectInstance();
    effect->beginChanges();
    for (std::list<boost::shared_ptr<KnobI> >::iterator it = _imp->knobs.begin(); it!=_imp->knobs.end(); ++it) {
        if (*it != _imp->center) {
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                (*it)->resetToDefaultValue(i);
            }
        } else {
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                (*it)->removeAnimation(ViewSpec::current(), i);
            }
            _imp->center->setValue(curCenter.x, ViewSpec::current(), 0);
            _imp->center->setValue(curCenter.y, ViewSpec::current(), 1);
        }
    }
    effect->endChanges();
    removeAllKeyframes();
    
}

void
TrackMarker::removeAllKeyframes()
{
    {
        QMutexLocker k(&_imp->trackMutex);
        _imp->userKeyframes.clear();
    }
    getContext()->s_allKeyframesRemovedOnTrack(shared_from_this());
}

void
TrackMarker::setUserKeyframe(int time)
{
    {
        QMutexLocker k(&_imp->trackMutex);
        _imp->userKeyframes.insert(time);
    }
    getContext()->s_keyframeSetOnTrack(shared_from_this(), time);
    
}

void
TrackMarker::removeUserKeyframe(int time)
{
    bool emitSignal = false;
    {
        QMutexLocker k(&_imp->trackMutex);
        std::set<int>::iterator found = _imp->userKeyframes.find(time);
        if (found != _imp->userKeyframes.end()) {
            _imp->userKeyframes.erase(found);
            emitSignal = true;
        }
    }
    if (emitSignal) {
        getContext()->s_keyframeRemovedOnTrack(shared_from_this(), time);
    }
}

void
TrackMarker::onCenterKeyframeSet(double time,int /*dimension*/,int /*reason*/,bool added)
{
    if (added) {
        getContext()->s_keyframeSetOnTrackCenter(shared_from_this(),time);
    }
}

void
TrackMarker::onCenterKeyframeRemoved(double time,int /*dimension*/,int /*reason*/)
{
    getContext()->s_keyframeRemovedOnTrackCenter(shared_from_this(), time);
}

void
TrackMarker::onCenterKeyframeMoved(int /*dimension*/,double oldTime,double newTime)
{
    getContext()->s_keyframeRemovedOnTrackCenter(shared_from_this(), oldTime);
    getContext()->s_keyframeSetOnTrackCenter(shared_from_this(),newTime);
}

void
TrackMarker::onCenterKeyframesSet(const std::list<double>& keys, int /*dimension*/, int /*reason*/)
{
    getContext()->s_multipleKeyframesSetOnTrackCenter(shared_from_this(), keys);
}

void
TrackMarker::onCenterAnimationRemoved(int /*dimension*/)
{
    getContext()->s_allKeyframesRemovedOnTrackCenter(shared_from_this());
}

void
TrackMarker::onCenterKnobValueChanged(int dimension,int reason)
{
    getContext()->s_centerKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onOffsetKnobValueChanged(int dimension,int reason)
{
    getContext()->s_offsetKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onCorrelationKnobValueChanged(int dimension,int reason)
{
    getContext()->s_correlationKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onWeightKnobValueChanged(int dimension,int reason)
{
    getContext()->s_weightKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onMotionModelKnobValueChanged(int dimension,int reason)
{
    getContext()->s_motionModelKnobValueChanged(shared_from_this(), dimension, reason);
}

/*void
 TrackMarker::onPatternTopLeftKnobValueChanged(int dimension,int reason)
 {
 getContext()->s_patternTopLeftKnobValueChanged(shared_from_this(), dimension, reason);
 }
 
 void
 TrackMarker::onPatternTopRightKnobValueChanged(int dimension,int reason)
 {
 getContext()->s_patternTopRightKnobValueChanged(shared_from_this(), dimension, reason);
 }
 
 void
 TrackMarker::onPatternBtmRightKnobValueChanged(int dimension,int reason)
 {
 getContext()->s_patternBtmRightKnobValueChanged(shared_from_this(), dimension, reason);
 }
 
 void
 TrackMarker::onPatternBtmLeftKnobValueChanged(int dimension,int reason)
 {
 getContext()->s_patternBtmLeftKnobValueChanged(shared_from_this(), dimension, reason);
 }
 */
void
TrackMarker::onSearchBtmLeftKnobValueChanged(int dimension,int reason)
{
    getContext()->s_searchBtmLeftKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onSearchTopRightKnobValueChanged(int dimension,int reason)
{
    getContext()->s_searchTopRightKnobValueChanged(shared_from_this(), dimension, reason);
}

RectI
TrackMarker::getMarkerImageRoI(int time) const
{
    const unsigned int mipmapLevel = 0;
    
    Natron::Point center,offset;
    center.x = _imp->center->getValueAtTime(time, 0);
    center.y = _imp->center->getValueAtTime(time, 1);
    
    offset.x = _imp->offset->getValueAtTime(time, 0);
    offset.y = _imp->offset->getValueAtTime(time, 1);
    
    RectD roiCanonical;
    roiCanonical.x1 = _imp->searchWindowBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
    roiCanonical.y1 = _imp->searchWindowBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
    roiCanonical.x2 = _imp->searchWindowTopRight->getValueAtTime(time, 0) + center.x + offset.x;
    roiCanonical.y2 = _imp->searchWindowTopRight->getValueAtTime(time, 1) + center.y + offset.y;
    
    RectI roi;
    NodePtr node = getContext()->getNode();
    NodePtr input = node->getInput(0);
    if (!input) {
        return RectI();
    }
    roiCanonical.toPixelEnclosing(mipmapLevel, input ? input->getEffectInstance()->getAspectRatio(-1) : 1., &roi);
    
    return roi;
}

std::pair<boost::shared_ptr<Natron::Image>,RectI>
TrackMarker::getMarkerImage(int time, const RectI& roi) const
{
    std::list<ImageComponents> components;
    components.push_back(ImageComponents::getRGBComponents());
    
    const unsigned int mipmapLevel = 0;
    assert(!roi.isNull());
    
    NodePtr node = getContext()->getNode();
    NodePtr input = node->getInput(0);
    if (!input) {
        return std::make_pair(ImagePtr(),roi);
    }
    
    AbortableRenderInfoPtr abortInfo(new AbortableRenderInfo(false, 0));
    ParallelRenderArgsSetter frameRenderArgs(time,
                                             ViewIdx(0), //<  view 0 (left)
                                             true, //<isRenderUserInteraction
                                             false, //isSequential
                                             abortInfo, //abort info
                                             getContext()->getNode(), // viewer requester
                                             0, //texture index
                                             node->getApp()->getTimeLine().get(),
                                             NodePtr(),
                                             true, //isAnalysis
                                             false,  //draftMode
                                             false, // viewerprogress
                                             boost::shared_ptr<RenderStats>());
    
    RenderScale scale;
    scale.x = scale.y = 1.;
    EffectInstance::RenderRoIArgs args(time,
                                       scale,
                                       mipmapLevel, //mipmaplevel
                                       ViewIdx(0),
                                       false,
                                       roi,
                                       RectD(),
                                       components,
                                       Natron::eImageBitDepthFloat,
                                       false,
                                       node->getEffectInstance().get());
    std::map<ImageComponents,ImagePtr> planes;
    EffectInstance::RenderRoIRetCode stat = input->getEffectInstance()->renderRoI(args, &planes);
    if (stat != EffectInstance::eRenderRoIRetCodeOk || planes.empty()) {
        return std::make_pair(ImagePtr(),roi);
    }
    return std::make_pair(planes.begin()->second,roi);
}



NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_TrackMarker.cpp"
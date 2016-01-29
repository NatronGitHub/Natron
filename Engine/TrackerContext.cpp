/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerContext.h"

#include <set>
#include <sstream>

#include <QWaitCondition>
#include <QThread>
#include <QCoreApplication>
#include <QtConcurrentMap>

#include <libmv/autotrack/autotrack.h>
#include <libmv/autotrack/frame_accessor.h>
#include <libmv/image/array_nd.h>

#include <boost/bind.hpp>

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/NodeGroup.h"
#include "Engine/KnobTypes.h"
#include "Engine/Project.h"
#include "Engine/Curve.h"
#include "Engine/TrackerSerialization.h"
#include "Engine/ViewerInstance.h"

#define kTrackBaseName "track"
#define TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE 8

/// Parameters definitions

//////// Global to all tracks
#define kTrackerParamTrackRed "trackRed"
#define kTrackerParamTrackRedLabel "Track Red"
#define kTrackerParamTrackRedHint "Enable tracking on the red channel"

#define kTrackerParamTrackGreen "trackGreen"
#define kTrackerParamTrackGreenLabel "Track Green"
#define kTrackerParamTrackGreenHint "Enable tracking on the green channel"

#define kTrackerParamTrackBlue "trackBlue"
#define kTrackerParamTrackBlueLabel "Track Blue"
#define kTrackerParamTrackBlueHint "Enable tracking on the blue channel"

#define kTrackerParamMinimumCorrelation "minCorrelation"
#define kTrackerParamMinimumCorrelationLabel "Minimum correlation"
#define kTrackerParamMinimumCorrelationHint "Minimum normalized cross-correlation necessary between the final tracked " \
"position of the patch on the destination image and the reference patch needed to declare tracking success."

#define kTrackerParamMaximumIteration "maxIterations"
#define kTrackerParamMaximumIterationLabel "Maximum iterations"
#define kTrackerParamMaximumIterationHint "Maximum number of iterations the algorithm will run for the inner minimization " \
"before it gives up."

#define kTrackerParamBruteForcePreTrack "bruteForcePretTrack"
#define kTrackerParamBruteForcePreTrackLabel "Use brute-force pre-track"
#define kTrackerParamBruteForcePreTrackHint "Use a brute-force translation-only pre-track before refinement"

#define kTrackerParamNormalizeIntensities "normalizeIntensities"
#define kTrackerParamNormalizeIntensitiesLabel "Normalize Intensities"
#define kTrackerParamNormalizeIntensitiesHint "Normalize the image patches by their mean before doing the sum of squared" \
" error calculation. Slower."

#define kTrackerParamPreBlurSigma "preBlurSigma"
#define kTrackerParamPreBlurSigmaLabel "Pre-blur sigma"
#define kTrackerParamPreBlurSigmaHint "The size in pixels of the blur kernel used to both smooth the image and take the image derivative."

#define kTrackerParamReferenceFrame "referenceFrame"
#define kTrackerParamReferenceFrameLabel "Reference frame"
#define kTrackerParamReferenceFrameHint "When exporting tracks to a CornerPin or Transform, this will be the frame number at which the transform will be an identity."

//////// Per-track parameters
#define kTrackerParamSearchWndBtmLeft "searchWndBtmLeft"
#define kTrackerParamSearchWndBtmLeftLabel "Search Window Bottom Left"
#define kTrackerParamSearchWndBtmLeftHint "The bottom left point of the search window, relative to the center point."

#define kTrackerParamSearchWndTopRight "searchWndTopRight"
#define kTrackerParamSearchWndTopRightLabel "Search Window Top Right"
#define kTrackerParamSearchWndTopRightHint "The top right point of the search window, relative to the center point."

#define kTrackerParamPatternTopLeft "patternTopLeft"
#define kTrackerParamPatternTopLeftLabel "Pattern Top Left"
#define kTrackerParamPatternTopLeftHint "The top left point of the quad defining the pattern to track"

#define kTrackerParamPatternTopRight "patternTopRight"
#define kTrackerParamPatternTopRightLabel "Pattern Top Right"
#define kTrackerParamPatternTopRightHint "The top right point of the quad defining the pattern to track"

#define kTrackerParamPatternBtmRight "patternBtmRight"
#define kTrackerParamPatternBtmRightLabel "Pattern Bottom Right"
#define kTrackerParamPatternBtmRightHint "The bottom right point of the quad defining the pattern to track"

#define kTrackerParamPatternBtmLeft "patternBtmLeft"
#define kTrackerParamPatternBtmLeftLabel "Pattern Bottom Left"
#define kTrackerParamPatternBtmLeftHint "The bottom left point of the quad defining the pattern to track"

#define kTrackerParamCenter "centerPoint"
#define kTrackerParamCenterLabel "Center"
#define kTrackerParamCenterHint "The point to track"

#define kTrackerParamOffset "offset"
#define kTrackerParamOffsetLabel "Offset"
#define kTrackerParamOffsetHint "The offset applied to the center point relative to the real tracked position"

#define kTrackerParamTrackWeight "trackWeight"
#define kTrackerParamTrackWeightLabel "Weight"
#define kTrackerParamTrackWeightHint "The weight determines the amount this marker contributes to the final solution"

#define kTrackerParamMotionModel "motionModel"
#define kTrackerParamMotionModelLabel "Motion model"
#define kTrackerParamMotionModelHint "The motion model to use for tracking."

#define kTrackerParamMotionModelTranslation "Search for markers that are only translated between frames."
#define kTrackerParamMotionModelTransRot "Search for markers that are translated and rotated between frames."
#define kTrackerParamMotionModelTransScale "Search for markers that are translated and scaled between frames."
#define kTrackerParamMotionModelTransRotScale "Search for markers that are translated, rotated and scaled between frames."
#define kTrackerParamMotionModelAffine "Search for markers that are affine transformed (t,r,k and skew) between frames."
#define kTrackerParamMotionModelPerspective "Search for markers that are perspectively deformed (homography) between frames."

#define kTrackerParamCorrelation "correlation"
#define kTrackerParamCorrelationLabel "Correlation"
#define kTrackerParamCorrelationHint "The correlation score obtained after tracking each frame"

#ifdef DEBUG
#define TRACE_LIB_MV 1
#endif

NATRON_NAMESPACE_ENTER;

namespace  {
    
enum libmv_MarkerChannel {
    LIBMV_MARKER_CHANNEL_R = (1 << 0),
    LIBMV_MARKER_CHANNEL_G = (1 << 1),
    LIBMV_MARKER_CHANNEL_B = (1 << 2),
};

} // anon namespace

void
TrackerContext::getMotionModelsAndHelps(std::vector<std::string>* models,std::vector<std::string>* tooltips)
{
    models->push_back("Trans.");
    tooltips->push_back(kTrackerParamMotionModelTranslation);
    models->push_back("Trans.+Rot.");
    tooltips->push_back(kTrackerParamMotionModelTransRot);
    models->push_back("Trans.+Scale");
    tooltips->push_back(kTrackerParamMotionModelTransScale);
    models->push_back("Trans.+Rot.+Scale");
    tooltips->push_back(kTrackerParamMotionModelTransRotScale);
    models->push_back("Affine");
    tooltips->push_back(kTrackerParamMotionModelAffine);
    models->push_back("Perspective");
    tooltips->push_back(kTrackerParamMotionModelPerspective);
}


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
    
    TrackMarkerPrivate(const boost::shared_ptr<TrackerContext>& context)
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
        searchWindowBtmLeft.reset(new KnobDouble(0, kTrackerParamSearchWndBtmLeftLabel, 2, false));
        searchWindowBtmLeft->setName(kTrackerParamSearchWndBtmLeft);
        searchWindowBtmLeft->populate();
        searchWindowBtmLeft->setDefaultValue(-25, 0);
        searchWindowBtmLeft->setDefaultValue(-25, 1);
        knobs.push_back(searchWindowBtmLeft);
        
        searchWindowTopRight.reset(new KnobDouble(0, kTrackerParamSearchWndTopRightLabel, 2, false));
        searchWindowTopRight->setName(kTrackerParamSearchWndTopRight);
        searchWindowTopRight->populate();
        searchWindowTopRight->setDefaultValue(25, 0);
        searchWindowTopRight->setDefaultValue(25, 1);
        knobs.push_back(searchWindowTopRight);
        
        
        patternTopLeft.reset(new KnobDouble(0, kTrackerParamPatternTopLeftLabel, 2, false));
        patternTopLeft->setName(kTrackerParamPatternTopLeft);
        patternTopLeft->populate();
        patternTopLeft->setDefaultValue(-15, 0);
        patternTopLeft->setDefaultValue(15, 1);
        knobs.push_back(patternTopLeft);
        
        patternTopRight.reset(new KnobDouble(0, kTrackerParamPatternTopRightLabel, 2, false));
        patternTopRight->setName(kTrackerParamPatternTopRight);
        patternTopRight->populate();
        patternTopRight->setDefaultValue(15, 0);
        patternTopRight->setDefaultValue(15, 1);
        knobs.push_back(patternTopRight);
        
        patternBtmRight.reset(new KnobDouble(0, kTrackerParamPatternBtmRightLabel, 2, false));
        patternBtmRight->setName(kTrackerParamPatternBtmRight);
        patternBtmRight->populate();
        patternBtmRight->setDefaultValue(15, 0);
        patternBtmRight->setDefaultValue(-15, 1);
        knobs.push_back(patternBtmRight);
        
        patternBtmLeft.reset(new KnobDouble(0, kTrackerParamPatternBtmLeftLabel, 2, false));
        patternBtmLeft->setName(kTrackerParamPatternBtmLeft);
        patternBtmLeft->populate();
        patternBtmLeft->setDefaultValue(-15, 0);
        patternBtmLeft->setDefaultValue(-15, 1);
        knobs.push_back(patternBtmLeft);
        
        center.reset(new KnobDouble(0, kTrackerParamCenterLabel, 2, false));
        center->setName(kTrackerParamCenter);
        center->populate();
        knobs.push_back(center);
        
        offset.reset(new KnobDouble(0, kTrackerParamOffsetLabel, 2, false));
        offset->setName(kTrackerParamOffset);
        offset->populate();
        knobs.push_back(offset);
        
        weight.reset(new KnobDouble(0, kTrackerParamTrackWeightLabel, 1, false));
        weight->setName(kTrackerParamTrackWeight);
        weight->populate();
        weight->setDefaultValue(1.);
        weight->setAnimationEnabled(false);
        weight->setMinimum(0.);
        weight->setMaximum(1.);
        knobs.push_back(weight);
        
        motionModel.reset(new KnobChoice(0, kTrackerParamMotionModelLabel, 1, false));
        motionModel->setName(kTrackerParamMotionModel);
        motionModel->populate();
        {
            std::vector<std::string> choices,helps;
            TrackerContext::getMotionModelsAndHelps(&choices,&helps);
            motionModel->populateChoices(choices,helps);
        }
        motionModel->setDefaultValue(4);
        knobs.push_back(motionModel);

        correlation.reset(new KnobDouble(0, kTrackerParamCorrelationLabel, 1, false));
        correlation->setName(kTrackerParamCorrelation);
        correlation->populate();
        knobs.push_back(correlation);
    }
};

TrackMarker::TrackMarker(const boost::shared_ptr<TrackerContext>& context)
: QObject()
, boost::enable_shared_from_this<TrackMarker>()
, _imp(new TrackMarkerPrivate(context))
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
    boost::shared_ptr<Curve> curve = _imp->center->getCurve(0);
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
        Natron::StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(), time, scale, 0, &rod, &isProjectFormat);
        Natron::Point center;
        center.x = 0;
        center.y = 0;
        if (stat == Natron::eStatusOK) {
            center.x = (rod.x1 + rod.x2) / 2.;
            center.y = (rod.y1 + rod.y2) / 2.;
        }
        _imp->center->setValue(center.x, 0);
        _imp->center->setValue(center.y, 1);
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
                (*it)->removeAnimation(i);
            }
            _imp->center->setValue(curCenter.x, 0);
            _imp->center->setValue(curCenter.y, 1);
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
    roiCanonical.toPixelEnclosing(mipmapLevel, input ? input->getEffectInstance()->getPreferredAspectRatio() : 1., &roi);
    
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
    
    ParallelRenderArgsSetter frameRenderArgs(time,
                                             0, //<  view 0 (left)
                                             true, //<isRenderUserInteraction
                                             false, //isSequential
                                             false, //can abort
                                             0, //render Age
                                             getContext()->getNode(), // viewer requester
                                             0,
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
                                       0,
                                       false,
                                       roi,
                                       RectD(),
                                       components,
                                       Natron::eImageBitDepthFloat,
                                       false,
                                       node->getEffectInstance().get());
    ImageList planes;
    EffectInstance::RenderRoIRetCode stat = input->getEffectInstance()->renderRoI(args, &planes);
    if (stat != EffectInstance::eRenderRoIRetCodeOk || planes.empty()) {
        return std::make_pair(ImagePtr(),roi);
    }
    return std::make_pair(planes.front(),roi);
}

struct TrackMarkerAndOptions
{
    boost::shared_ptr<TrackMarker> natronMarker;
    mv::Marker mvMarker;
    mv::TrackRegionOptions mvOptions;
};

class FrameAccessorImpl;

class TrackArgsLibMV
{
    int _start, _end;
    bool _isForward;
    boost::shared_ptr<TimeLine> _timeline;
    bool _isUpdateViewerEnabled;
    bool _centerViewer;
    ViewerInstance* _viewer;
    boost::shared_ptr<mv::AutoTrack> _libmvAutotrack;
    boost::shared_ptr<FrameAccessorImpl> _fa;
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > _tracks;
    
    //Store the format size because LibMV internally has a top-down Y axis
    double _formatWidth,_formatHeight;
    mutable QMutex _autoTrackMutex;
    
public:
    
    TrackArgsLibMV()
    : _start(0)
    , _end(0)
    , _isForward(false)
    , _timeline()
    , _isUpdateViewerEnabled(false)
    , _centerViewer(false)
    , _viewer(0)
    , _libmvAutotrack()
    , _fa()
    , _tracks()
    , _formatWidth(0)
    , _formatHeight(0)
    {
        
    }
    
    TrackArgsLibMV(int start,
                   int end,
                   bool isForward,
                   const boost::shared_ptr<TimeLine>& timeline,
                   bool isUpdateViewerEnabled,
                   bool centerViewer,
                   ViewerInstance* viewer,
                   const boost::shared_ptr<mv::AutoTrack>& autoTrack,
                   const boost::shared_ptr<FrameAccessorImpl>& fa,
                   const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks,
                   double formatWidth,
                   double formatHeight)
    : _start(start)
    , _end(end)
    , _isForward(isForward)
    , _timeline(timeline)
    , _isUpdateViewerEnabled(isUpdateViewerEnabled)
    , _centerViewer(centerViewer)
    , _viewer(viewer)
    , _libmvAutotrack(autoTrack)
    , _fa(fa)
    , _tracks(tracks)
    , _formatWidth(formatWidth)
    , _formatHeight(formatHeight)
    {
        
    }
    
    TrackArgsLibMV(const TrackArgsLibMV& other)
    {
        *this = other;
    }
    
    void operator=(const TrackArgsLibMV& other)
    {
        _start = other._start;
        _end = other._end;
        _isForward = other._isForward;
        _timeline = other._timeline;
        _isUpdateViewerEnabled = other._isUpdateViewerEnabled;
        _viewer = other._viewer;
        _libmvAutotrack = other._libmvAutotrack;
        _fa = other._fa;
        _centerViewer = other._centerViewer;
        _tracks = other._tracks;
        _formatWidth = other._formatWidth;
        _formatHeight = other._formatHeight;
    }
    
    double getFormatHeight() const
    {
        return _formatHeight;
    }
    
    double getFormatWidth() const
    {
        return _formatWidth;
    }
    
    QMutex* getAutoTrackMutex() const
    {
        return &_autoTrackMutex;
    }
    
    int getStart() const
    {
        return _start;
    }
    
    int getEnd() const
    {
        return _end;
    }
    
    bool getForward() const
    {
        return _isForward;
    }
    
    boost::shared_ptr<TimeLine> getTimeLine() const
    {
        return _timeline;
    }
    
    ViewerInstance* getViewer() const
    {
        return _viewer;
    }
    
    bool isCenterViewerEnabled() const
    {
        return _centerViewer;
    }

    
    bool isUpdateViewerEnabled() const
    {
        return _isUpdateViewerEnabled;
    }
    
    int getNumTracks() const
    {
        return (int)_tracks.size();
    }
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& getTracks() const
    {
        return _tracks;
    }
    
    boost::shared_ptr<mv::AutoTrack> getLibMVAutoTrack() const
    {
        return _libmvAutotrack;
    }
    
    void getEnabledChannels(bool* r, bool* g, bool* b) const;
    
    void getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const
    {
        for (std::vector<boost::shared_ptr<TrackMarkerAndOptions> >::const_iterator it = _tracks.begin(); it!=_tracks.end(); ++it) {
            boost::shared_ptr<KnobDouble> searchBtmLeft = (*it)->natronMarker->getSearchWindowBottomLeftKnob();
            boost::shared_ptr<KnobDouble> searchTopRight = (*it)->natronMarker->getSearchWindowTopRightKnob();
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->natronMarker->getCenterKnob();
            boost::shared_ptr<KnobDouble> offsetKnob = (*it)->natronMarker->getOffsetKnob();
            
            Natron::Point offset,center,btmLeft,topRight;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);
            
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            
            btmLeft.x = searchBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = searchBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
            
            topRight.x = searchTopRight->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = searchTopRight->getValueAtTime(time, 1) + center.y + offset.y;
            
            RectD rect;
            rect.x1 = btmLeft.x;
            rect.y1 = btmLeft.y;
            rect.x2 = topRight.x;
            rect.y2 = topRight.y;
            canonicalRects->push_back(rect);
        }
    }
};

void
TrackArgsV1::getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const
{
    for (std::vector<KnobButton*>::const_iterator it = _buttonInstances.begin(); it!=_buttonInstances.end(); ++it) {
        EffectInstance* effect = dynamic_cast<EffectInstance*>((*it)->getHolder());
        assert(effect);
    
        boost::shared_ptr<KnobDouble> searchBtmLeft = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("searchBoxBtmLeft"));
        boost::shared_ptr<KnobDouble> searchTopRight = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("searchBoxTopRight"));
        boost::shared_ptr<KnobDouble> centerKnob = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("center"));
        boost::shared_ptr<KnobDouble> offsetKnob = boost::dynamic_pointer_cast<KnobDouble>(effect->getKnobByName("offset"));
        assert(searchBtmLeft  && searchTopRight && centerKnob && offsetKnob);
        Natron::Point offset,center,btmLeft,topRight;
        offset.x = offsetKnob->getValueAtTime(time, 0);
        offset.y = offsetKnob->getValueAtTime(time, 1);
        
        center.x = centerKnob->getValueAtTime(time, 0);
        center.y = centerKnob->getValueAtTime(time, 1);
        
        btmLeft.x = searchBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
        btmLeft.y = searchBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
        
        topRight.x = searchTopRight->getValueAtTime(time, 0) + center.x + offset.x;
        topRight.y = searchTopRight->getValueAtTime(time, 1) + center.y + offset.y;
        
        RectD rect;
        rect.x1 = btmLeft.x;
        rect.y1 = btmLeft.y;
        rect.x2 = topRight.x;
        rect.y2 = topRight.y;
        canonicalRects->push_back(rect);
    }
}
/*
static void updateBbox(const Natron::Point& p, RectD* bbox)
{
    bbox->x1 = std::min(p.x, bbox->x1);
    bbox->x2 = std::max(p.x, bbox->x2);
    bbox->y1 = std::min(p.x, bbox->y1);
    bbox->y2 = std::max(p.x, bbox->y2);
}
*/
static double invertYCoordinate(double yIn, double formatHeight)
{
    return formatHeight - 1 - yIn;
}

static void convertLibMVRegionToRectI(const mv::Region& region, int formatHeight, RectI* roi)
{
    roi->x1 = region.min(0);
    roi->x2 = region.max(0);
    roi->y1 = invertYCoordinate(region.max(1),formatHeight);
    roi->y2 = invertYCoordinate(region.min(1),formatHeight);
}


/**
 * @brief Set keyframes on knobs from Marker data
 **/
static void setKnobKeyframesFromMarker(const mv::Marker& mvMarker,
                                       int formatHeight,
                                       const libmv::TrackRegionResult* result,
                                       const boost::shared_ptr<TrackMarker>& natronMarker)
{
    int time = mvMarker.frame;
    boost::shared_ptr<KnobDouble> correlationKnob = natronMarker->getCorrelationKnob();
    if (result) {
        correlationKnob->setValueAtTime(time, result->correlation, 0);
    } else {
        correlationKnob->setValueAtTime(time, 0., 0);
    }
    
    Natron::Point center;
    center.x = (double)mvMarker.center(0);
    center.y = (double)invertYCoordinate(mvMarker.center(1), formatHeight);
    
    boost::shared_ptr<KnobDouble> offsetKnob = natronMarker->getOffsetKnob();
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    // Set the center
    boost::shared_ptr<KnobDouble> centerKnob = natronMarker->getCenterKnob();
    centerKnob->setValuesAtTime(time, center.x, center.y, Natron::eValueChangedReasonNatronInternalEdited);
    
    Natron::Point topLeftCorner,topRightCorner,btmRightCorner,btmLeftCorner;
    topLeftCorner.x = mvMarker.patch.coordinates(0,0) - offset.x - center.x;
    topLeftCorner.y = invertYCoordinate(mvMarker.patch.coordinates(0,1),formatHeight) - offset.y - center.y;
    
    topRightCorner.x = mvMarker.patch.coordinates(1,0) - offset.x - center.x;
    topRightCorner.y = invertYCoordinate(mvMarker.patch.coordinates(1,1),formatHeight) - offset.y - center.y;
    
    btmRightCorner.x = mvMarker.patch.coordinates(2,0) - offset.x - center.x;
    btmRightCorner.y = invertYCoordinate(mvMarker.patch.coordinates(2,1),formatHeight) - offset.y - center.y;
    
    btmLeftCorner.x = mvMarker.patch.coordinates(3,0) - offset.x - center.x;
    btmLeftCorner.y = invertYCoordinate(mvMarker.patch.coordinates(3,1),formatHeight) - offset.y - center.y;
    
    boost::shared_ptr<KnobDouble> pntTopLeftKnob = natronMarker->getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> pntTopRightKnob = natronMarker->getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> pntBtmLeftKnob = natronMarker->getPatternBtmLeftKnob();
    boost::shared_ptr<KnobDouble> pntBtmRightKnob = natronMarker->getPatternBtmRightKnob();
    
    // Set the pattern Quad
    pntTopLeftKnob->setValuesAtTime(time, topLeftCorner.x, topLeftCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
    pntTopRightKnob->setValuesAtTime(time, topRightCorner.x, topRightCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
    pntBtmLeftKnob->setValuesAtTime(time, btmLeftCorner.x, btmLeftCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
    pntBtmRightKnob->setValuesAtTime(time, btmRightCorner.x, btmRightCorner.y, Natron::eValueChangedReasonNatronInternalEdited);
}

static void updateLibMvTrackMinimal(const TrackMarker& marker,
                                    int time,
                                    bool forward,
                                    int formatHeight,
                                    mv::Marker* mvMarker)
{
    boost::shared_ptr<KnobDouble> searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    boost::shared_ptr<KnobDouble> centerKnob = marker.getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker.getOffsetKnob();
    mvMarker->reference_frame = marker.getReferenceFrame(time, forward);
    if (marker.isUserKeyframe(time)) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }
    mvMarker->frame = time;
    
    Natron::Point nCenter;
    nCenter.x = centerKnob->getValueAtTime(time, 0);
    nCenter.y = centerKnob->getValueAtTime(time, 1);
    mvMarker->center(0) = nCenter.x;
    mvMarker->center(1) = invertYCoordinate(nCenter.y, formatHeight);
    
    Natron::Point searchWndBtmLeft,searchWndTopRight;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(mvMarker->reference_frame, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(mvMarker->reference_frame, 1);
    
    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(mvMarker->reference_frame, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(mvMarker->reference_frame, 1);
    
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    //See natronTrackerToLibMVTracker below for coordinates system
    mvMarker->search_region.min(0) = searchWndBtmLeft.x + mvMarker->center(0) + offset.x;
    mvMarker->search_region.min(1) = invertYCoordinate(searchWndTopRight.y + nCenter.x + offset.y,formatHeight);
    mvMarker->search_region.max(0) = searchWndTopRight.x + mvMarker->center(0) + offset.x;
    mvMarker->search_region.max(1) = invertYCoordinate(searchWndBtmLeft.y + nCenter.y + offset.y,formatHeight);
}

/// Converts a Natron track marker to the one used in LibMV. This is expensive: many calls to getValue are made
static void natronTrackerToLibMVTracker(bool useRefFrameForSearchWindow,
                                        bool trackChannels[3],
                                        const TrackMarker& marker,
                                        int trackIndex,
                                        int time,
                                        bool forward,
                                        double formatHeight,
                                        mv::Marker* mvMarker)
{
    boost::shared_ptr<KnobDouble> searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    boost::shared_ptr<KnobDouble> searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    boost::shared_ptr<KnobDouble> patternTopLeftKnob = marker.getPatternTopLeftKnob();
    boost::shared_ptr<KnobDouble> patternTopRightKnob = marker.getPatternTopRightKnob();
    boost::shared_ptr<KnobDouble> patternBtmRightKnob = marker.getPatternBtmRightKnob();
    boost::shared_ptr<KnobDouble> patternBtmLeftKnob = marker.getPatternBtmLeftKnob();
    boost::shared_ptr<KnobDouble> weightKnob = marker.getWeightKnob();
    boost::shared_ptr<KnobDouble> centerKnob = marker.getCenterKnob();
    boost::shared_ptr<KnobDouble> offsetKnob = marker.getOffsetKnob();
    
    // We don't use the clip in Natron
    mvMarker->clip = 0;
    mvMarker->reference_clip = 0;
    mvMarker->weight = (float)weightKnob->getValue();
    mvMarker->frame = time;
    mvMarker->reference_frame = marker.getReferenceFrame(time, forward);
    if (marker.isUserKeyframe(time)) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }
    Natron::Point nCenter;
    nCenter.x = centerKnob->getValueAtTime(time, 0);
    nCenter.y = centerKnob->getValueAtTime(time, 1);
    mvMarker->center(0) = nCenter.x;
    mvMarker->center(1) = invertYCoordinate(nCenter.y, formatHeight);
    mvMarker->model_type = mv::Marker::POINT;
    mvMarker->model_id = 0;
    mvMarker->track = trackIndex;
    
    mvMarker->disabled_channels =
    (trackChannels[0] ? LIBMV_MARKER_CHANNEL_R : 0) |
    (trackChannels[1] ? LIBMV_MARKER_CHANNEL_G : 0) |
    (trackChannels[2] ? LIBMV_MARKER_CHANNEL_B : 0);
    
    Natron::Point searchWndBtmLeft,searchWndTopRight;
    int searchWinTime = useRefFrameForSearchWindow ? mvMarker->reference_frame : time;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 0);
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, 1);
    
    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 0);
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(searchWinTime, 1);
    
    Natron::Point offset;
    offset.x = offsetKnob->getValueAtTime(time,0);
    offset.y = offsetKnob->getValueAtTime(time,1);
    
    
    Natron::Point tl,tr,br,bl;
    tl.x = patternTopLeftKnob->getValueAtTime(time, 0);
    tl.y = patternTopLeftKnob->getValueAtTime(time, 1);
    
    tr.x = patternTopRightKnob->getValueAtTime(time, 0);
    tr.y = patternTopRightKnob->getValueAtTime(time, 1);
    
    br.x = patternBtmRightKnob->getValueAtTime(time, 0);
    br.y = patternBtmRightKnob->getValueAtTime(time, 1);
    
    bl.x = patternBtmLeftKnob->getValueAtTime(time, 0);
    bl.y = patternBtmLeftKnob->getValueAtTime(time, 1);
    
    /*RectD patternBbox;
    patternBbox.setupInfinity();
    updateBbox(tl, &patternBbox);
    updateBbox(tr, &patternBbox);
    updateBbox(br, &patternBbox);
    updateBbox(bl, &patternBbox);*/
    
    // The search-region is laid out as such:
    //
    //    +----------> x
    //    |
    //    |   (min.x, min.y)           (max.x, min.y)
    //    |        +-------------------------+
    //    |        |                         |
    //    |        |                         |
    //    |        |                         |
    //    |        +-------------------------+
    //    v   (min.x, max.y)           (max.x, max.y)
    //
    mvMarker->search_region.min(0) = searchWndBtmLeft.x + nCenter.x + offset.x;
    mvMarker->search_region.min(1) = invertYCoordinate(searchWndTopRight.y + nCenter.y + offset.y,formatHeight);
    mvMarker->search_region.max(0) = searchWndTopRight.x + nCenter.x + offset.x;
    mvMarker->search_region.max(1) = invertYCoordinate(searchWndBtmLeft.y + nCenter.y + offset.y,formatHeight);
    
    
    // The patch is a quad (4 points); generally in 2D or 3D (here 2D)
    //
    //    +----------> x
    //    |\.
    //    | \.
    //    |  z (z goes into screen)
    //    |
    //    |     r0----->r1
    //    |      ^       |
    //    |      |   .   |
    //    |      |       V
    //    |     r3<-----r2
    //    |              \.
    //    |               \.
    //    v                normal goes away (right handed).
    //    y
    //
    // Each row is one of the corners coordinates; either (x, y) or (x, y, z).
    // TrackMarker extracts the patch coordinates as such:
    /*
     Quad2Df offset_quad = marker.patch;
     Vec2f origin = marker.search_region.Rounded().min;
     offset_quad.coordinates.rowwise() -= origin.transpose();
     QuadToArrays(offset_quad, x, y);
     x[4] = marker.center.x() - origin(0);
     y[4] = marker.center.y() - origin(1);
     */
    // The patch coordinates should be canonical
    
    mvMarker->patch.coordinates(0,0) = tl.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(0,1) = invertYCoordinate(tl.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(1,0) = tr.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(1,1) = invertYCoordinate(tr.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(2,0) = br.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(2,1) = invertYCoordinate(br.y + nCenter.y + offset.y,formatHeight);
    
    mvMarker->patch.coordinates(3,0) = bl.x + nCenter.x + offset.x;
    mvMarker->patch.coordinates(3,1) = invertYCoordinate(bl.y + nCenter.y + offset.y,formatHeight);
}

/*
 * @brief This is the internal tracking function that makes use of LivMV to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param time The search frame time, that is, the frame to track
 */
static bool trackStepLibMV(int trackIndex, const TrackArgsLibMV& args, int time)
{
    assert(trackIndex >= 0 && trackIndex < args.getNumTracks());
    
    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks = args.getTracks();
    const boost::shared_ptr<TrackMarkerAndOptions>& track = tracks[trackIndex];
    
    boost::shared_ptr<mv::AutoTrack> autoTrack = args.getLibMVAutoTrack();
    QMutex* autoTrackMutex = args.getAutoTrackMutex();
    
    bool enabledChans[3];
    args.getEnabledChannels(&enabledChans[0], &enabledChans[1], &enabledChans[2]);
    

    {
        //Add the tracked marker
        QMutexLocker k(autoTrackMutex);
        natronTrackerToLibMVTracker(true, enabledChans, *track->natronMarker, trackIndex, time, args.getForward(), args.getFormatHeight(), &track->mvMarker);
        autoTrack->AddMarker(track->mvMarker);
    }
    
    
    //The frame on the mvMarker should have been set accordingly
    assert(track->mvMarker.frame == time);

    if (track->mvMarker.source == mv::Marker::MANUAL) {
        // This is a user keyframe or the first frame, we do not track it
        assert(time == args.getStart() || track->natronMarker->isUserKeyframe(track->mvMarker.frame));
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "TrackStep:" << time << "is a keyframe";
#endif
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), 0, track->natronMarker);
        
    } else {
        
        // Set the reference rame
        track->mvMarker.reference_frame = track->natronMarker->getReferenceFrame(time, args.getForward());
        assert(track->mvMarker.reference_frame != track->mvMarker.frame);

        //Make sure the reference frame as the same search window as the tracked frame and exists in the AutoTrack
        {
            QMutexLocker k(autoTrackMutex);
            mv::Marker m;
            natronTrackerToLibMVTracker(false, enabledChans, *track->natronMarker, track->mvMarker.track, track->mvMarker.reference_frame, args.getForward(), args.getFormatHeight(), &m);
            autoTrack->AddMarker(m);
        }

        
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << ">>>> Tracking marker" << trackIndex << "at frame" << time <<
        "with reference frame" << track->mvMarker.reference_frame;
#endif
        
        // Do the actual tracking
        libmv::TrackRegionResult result;
        if (!autoTrack->TrackMarker(&track->mvMarker, &result, &track->mvOptions) || !result.is_usable()) {
#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "Tracking FAILED (" << (int)result.termination <<  ") for track" << trackIndex << "at frame" << time;
#endif
            return false;
        }
        
        //Ok the tracking has succeeded, now the marker is in this state:
        //the source is TRACKED, the search_window has been offset by the center delta,  the center has been offset
        
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "Tracking SUCCESS for track" << trackIndex << "at frame" << time;
#endif
        
        //Extract the marker to the knob keyframes
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), &result, track->natronMarker);
        
        //Add the marker to the autotrack
        /*{
            QMutexLocker k(autoTrackMutex);
            autoTrack->AddMarker(track->mvMarker);
        }*/
    } // if (track->mvMarker.source == mv::Marker::MANUAL) {
    
    //Refresh the marker for next iteration
    //int nextFrame = args.getForward() ? time + 1 : time - 1;
    //updateLibMvTrackMinimal(*track->natronMarker, nextFrame, args.getForward(), args.getFormatHeight(), &track->mvMarker);
    
    return true;
}

bool
TrackerContext::trackStepV1(int trackIndex, const TrackArgsV1& args, int time)
{
    assert(trackIndex >= 0 && trackIndex < (int)args.getInstances().size());
    KnobButton* selectedInstance = args.getInstances()[trackIndex];
    selectedInstance->getHolder()->onKnobValueChanged_public(selectedInstance,eValueChangedReasonNatronInternalEdited,time,
                                                             true);
    return true;
}

struct TrackerContextPrivate
{
    
    TrackerContext* _publicInterface;
    boost::weak_ptr<Natron::Node> node;
    
    std::list<boost::weak_ptr<KnobI> > knobs,perTrackKnobs;
    boost::weak_ptr<KnobBool> enableTrackRed,enableTrackGreen,enableTrackBlue;
    boost::weak_ptr<KnobDouble> minCorrelation,maxIterations;
    boost::weak_ptr<KnobBool> bruteForcePreTrack,useNormalizedIntensities;
    boost::weak_ptr<KnobDouble> preBlurSigma;
    boost::weak_ptr<KnobInt> referenceFrame;
    
    mutable QMutex trackerContextMutex;
    std::vector<boost::shared_ptr<TrackMarker> > markers;
    std::list<boost::shared_ptr<TrackMarker> > selectedMarkers,markersToSlave,markersToUnslave;
    int beginSelectionCounter;
    int selectionRecursion;
    
    TrackScheduler<TrackArgsLibMV> scheduler;
    
    TrackerContextPrivate(TrackerContext* publicInterface, const boost::shared_ptr<Natron::Node> &node)
    : _publicInterface(publicInterface)
    , node(node)
    , knobs()
    , perTrackKnobs()
    , enableTrackRed()
    , enableTrackGreen()
    , enableTrackBlue()
    , minCorrelation()
    , maxIterations()
    , bruteForcePreTrack()
    , useNormalizedIntensities()
    , preBlurSigma()
    , referenceFrame()
    , trackerContextMutex()
    , markers()
    , selectedMarkers()
    , markersToSlave()
    , markersToUnslave()
    , beginSelectionCounter(0)
    , selectionRecursion(0)
    , scheduler(trackStepLibMV)
    {
        EffectInstPtr effect = node->getEffectInstance();
        QObject::connect(&scheduler, SIGNAL(trackingStarted()), _publicInterface, SIGNAL(trackingStarted()));
        QObject::connect(&scheduler, SIGNAL(trackingFinished()), _publicInterface, SIGNAL(trackingFinished()));
        QObject::connect(&scheduler, SIGNAL(progressUpdate(double)), _publicInterface, SIGNAL(trackingProgress(double)));
        
        boost::shared_ptr<KnobPage> settingsPage = AppManager::createKnob<KnobPage>(effect.get(), "Controls", 1 , false);
        boost::shared_ptr<KnobPage> transformPage = AppManager::createKnob<KnobPage>(effect.get(), "Transform", 1 , false);
        
        boost::shared_ptr<KnobBool> enableTrackRedKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackRedLabel, 1, false);
        enableTrackRedKnob->setName(kTrackerParamTrackRed);
        enableTrackRedKnob->setHintToolTip(kTrackerParamTrackRedHint);
        enableTrackRedKnob->setDefaultValue(true);
        enableTrackRedKnob->setAnimationEnabled(false);
        enableTrackRedKnob->setAddNewLine(false);
        enableTrackRedKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(enableTrackRedKnob);
        enableTrackRed = enableTrackRedKnob;
        knobs.push_back(enableTrackRedKnob);
        
        boost::shared_ptr<KnobBool> enableTrackGreenKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackGreenLabel, 1, false);
        enableTrackGreenKnob->setName(kTrackerParamTrackGreen);
        enableTrackGreenKnob->setHintToolTip(kTrackerParamTrackGreenHint);
        enableTrackGreenKnob->setDefaultValue(true);
        enableTrackGreenKnob->setAnimationEnabled(false);
        enableTrackGreenKnob->setAddNewLine(false);
        enableTrackGreenKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(enableTrackGreenKnob);
        enableTrackGreen = enableTrackGreenKnob;
        knobs.push_back(enableTrackGreenKnob);
        
        boost::shared_ptr<KnobBool> enableTrackBlueKnob = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamTrackBlueLabel, 1, false);
        enableTrackBlueKnob->setName(kTrackerParamTrackBlue);
        enableTrackBlueKnob->setHintToolTip(kTrackerParamTrackBlueHint);
        enableTrackBlueKnob->setDefaultValue(true);
        enableTrackBlueKnob->setAnimationEnabled(false);
        enableTrackBlueKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(enableTrackBlueKnob);
        enableTrackBlue = enableTrackBlueKnob;
        knobs.push_back(enableTrackBlueKnob);
        
        boost::shared_ptr<KnobDouble> minCorelKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamMinimumCorrelationLabel, 1, false);
        minCorelKnob->setName(kTrackerParamMinimumCorrelation);
        minCorelKnob->setHintToolTip(kTrackerParamMinimumCorrelationHint);
        minCorelKnob->setAnimationEnabled(false);
        minCorelKnob->setMinimum(0.);
        minCorelKnob->setMaximum(1.);
        minCorelKnob->setDefaultValue(0.75);
        minCorelKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(minCorelKnob);
        minCorrelation = minCorelKnob;
        knobs.push_back(minCorelKnob);
        
        boost::shared_ptr<KnobDouble> maxItKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamMaximumIterationLabel, 1, false);
        maxItKnob->setName(kTrackerParamMaximumIteration);
        maxItKnob->setHintToolTip(kTrackerParamMaximumIterationHint);
        maxItKnob->setAnimationEnabled(false);
        maxItKnob->setMinimum(0);
        maxItKnob->setMaximum(150);
        maxItKnob->setEvaluateOnChange(false);
        maxItKnob->setDefaultValue(50);
        settingsPage->addKnob(maxItKnob);
        maxIterations = maxItKnob;
        knobs.push_back(maxItKnob);
        
        boost::shared_ptr<KnobBool> usePretTrackBF = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamBruteForcePreTrackLabel, 1, false);
        usePretTrackBF->setName(kTrackerParamBruteForcePreTrack);
        usePretTrackBF->setHintToolTip(kTrackerParamBruteForcePreTrackHint);
        usePretTrackBF->setDefaultValue(true);
        usePretTrackBF->setAnimationEnabled(false);
        usePretTrackBF->setEvaluateOnChange(false);
        usePretTrackBF->setAddNewLine(false);
        settingsPage->addKnob(usePretTrackBF);
        bruteForcePreTrack = usePretTrackBF;
        knobs.push_back(usePretTrackBF);
        
        boost::shared_ptr<KnobBool> useNormalizedInt = AppManager::createKnob<KnobBool>(effect.get(), kTrackerParamNormalizeIntensitiesLabel, 1, false);
        useNormalizedInt->setName(kTrackerParamNormalizeIntensities);
        useNormalizedInt->setHintToolTip(kTrackerParamNormalizeIntensitiesHint);
        useNormalizedInt->setDefaultValue(false);
        useNormalizedInt->setAnimationEnabled(false);
        useNormalizedInt->setEvaluateOnChange(false);
        settingsPage->addKnob(useNormalizedInt);
        useNormalizedIntensities = useNormalizedInt;
        knobs.push_back(useNormalizedInt);

        boost::shared_ptr<KnobDouble> preBlurSigmaKnob = AppManager::createKnob<KnobDouble>(effect.get(), kTrackerParamPreBlurSigmaLabel, 1, false);
        preBlurSigmaKnob->setName(kTrackerParamPreBlurSigma);
        preBlurSigmaKnob->setHintToolTip(kTrackerParamPreBlurSigmaHint);
        preBlurSigmaKnob->setAnimationEnabled(false);
        preBlurSigmaKnob->setMinimum(0);
        preBlurSigmaKnob->setMaximum(10.);
        preBlurSigmaKnob->setDefaultValue(0.9);
        preBlurSigmaKnob->setEvaluateOnChange(false);
        settingsPage->addKnob(preBlurSigmaKnob);
        preBlurSigma = preBlurSigmaKnob;
        knobs.push_back(preBlurSigmaKnob);
        
        boost::shared_ptr<KnobInt> referenceFrameKnob = AppManager::createKnob<KnobInt>(effect.get(), kTrackerParamReferenceFrameLabel, 1, false);
        referenceFrameKnob->setName(kTrackerParamReferenceFrame);
        referenceFrameKnob->setHintToolTip(kTrackerParamReferenceFrameHint);
        referenceFrameKnob->setAnimationEnabled(false);
        referenceFrameKnob->setDefaultValue(0.9);
        referenceFrameKnob->setEvaluateOnChange(false);
        transformPage->addKnob(referenceFrameKnob);
        referenceFrame = referenceFrameKnob;
        knobs.push_back(referenceFrameKnob);
 
        
    }
    

    
    /// Make all calls to getValue() that are global to the tracker context in here
    void beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const;
    
    /// Make all calls to getValue() that are local to the track in here
    void endLibMVOptionsForTrack(const TrackMarker& marker,
                                  mv::TrackRegionOptions* options) const;
    
    void addToSelectionList(const boost::shared_ptr<TrackMarker>& marker)
    {
        if (std::find(selectedMarkers.begin(), selectedMarkers.end(), marker) != selectedMarkers.end()) {
            return;
        }
        selectedMarkers.push_back(marker);
        markersToSlave.push_back(marker);
    }
    
    void removeFromSelectionList(const boost::shared_ptr<TrackMarker>& marker)
    {
        std::list<boost::shared_ptr<TrackMarker> >::iterator found = std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);
        if (found == selectedMarkers.end()) {
            return;
        }
        selectedMarkers.erase(found);
        markersToUnslave.push_back(marker);
    }
    
    void incrementSelectionCounter()
    {
        ++beginSelectionCounter;
    }
    
    void decrementSelectionCounter()
    {
        if (beginSelectionCounter > 0) {
            --beginSelectionCounter;
        }
    }
    
    void linkMarkerKnobsToGuiKnobs(const std::list<boost::shared_ptr<TrackMarker> >& markers,
                                   bool multipleTrackSelected,
                                   bool slave);
    
    
};

TrackerContext::TrackerContext(const boost::shared_ptr<Natron::Node> &node)
: boost::enable_shared_from_this<TrackerContext>()
, _imp(new TrackerContextPrivate(this, node))
{
    
}

TrackerContext::~TrackerContext()
{
    
}

void
TrackerContext::load(const TrackerContextSerialization& serialization)
{
    
    boost::shared_ptr<TrackerContext> thisShared = shared_from_this();
    QMutexLocker k(&_imp->trackerContextMutex);

    for (std::list<TrackSerialization>::const_iterator it = serialization._tracks.begin(); it != serialization._tracks.end(); ++it) {
        boost::shared_ptr<TrackMarker> marker(new TrackMarker(thisShared));
        marker->load(*it);
        _imp->markers.push_back(marker);
    }
}

void
TrackerContext::save(TrackerContextSerialization* serialization) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        TrackSerialization s;
        _imp->markers[i]->save(&s);
        serialization->_tracks.push_back(s);
    }
}

int
TrackerContext::getTransformReferenceFrame() const
{
    return _imp->referenceFrame.lock()->getValue();
}


void
TrackerContext::goToPreviousKeyFrame(int time)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getSelectedMarkers(&markers);
    
    int minimum = INT_MIN;
    for (std::list<boost::shared_ptr<TrackMarker> > ::iterator it = markers.begin(); it != markers.end(); ++it) {
        int t = (*it)->getPreviousKeyframe(time);
        if ( (t != INT_MIN) && (t > minimum) ) {
            minimum = t;
        }
    }
    if (minimum != INT_MIN) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(minimum, false,  NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}

void
TrackerContext::goToNextKeyFrame(int time)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getSelectedMarkers(&markers);
    
    int maximum = INT_MAX;
    for (std::list<boost::shared_ptr<TrackMarker> > ::iterator it = markers.begin(); it != markers.end(); ++it) {
        int t = (*it)->getNextKeyframe(time);
        if ( (t != INT_MAX) && (t < maximum) ) {
            maximum = t;
        }
    }
    if (maximum != INT_MAX) {
        getNode()->getApp()->setLastViewerUsingTimeline(boost::shared_ptr<Natron::Node>());
        getNode()->getApp()->getTimeLine()->seekFrame(maximum, false,  NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}

/*boost::shared_ptr<KnobDouble>
TrackerContext::getSearchWindowBottomLeftKnob() const
{
    return _imp->searchWindowBtmLeft.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getSearchWindowTopRightKnob() const
{
    return _imp->searchWindowTopRight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternTopLeftKnob() const
{
    return _imp->patternTopLeft.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternTopRightKnob() const
{
    return _imp->patternTopRight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternBtmRightKnob() const
{
    return _imp->patternBtmRight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getPatternBtmLeftKnob() const
{
    return _imp->patternBtmLeft.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getWeightKnob() const
{
    return _imp->weight.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getCenterKnob() const
{
    return _imp->center.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getOffsetKnob() const
{
    return _imp->offset.lock();
}

boost::shared_ptr<KnobDouble>
TrackerContext::getCorrelationKnob() const
{
    return _imp->correlation.lock();
}

boost::shared_ptr<KnobChoice>
TrackerContext::getMotionModelKnob() const
{
    return _imp->motionModel.lock();
}*/

boost::shared_ptr<TrackMarker>
TrackerContext::getMarkerByName(const std::string & name) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::vector<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
        if ((*it)->getScriptName() == name) {
            return *it;
        }
    }
    return boost::shared_ptr<TrackMarker>();
}

std::string
TrackerContext::generateUniqueTrackName(const std::string& baseName)
{
    int no = 1;
    
    bool foundItem;
    std::string name;
    do {
        std::stringstream ss;
        ss << baseName;
        ss << no;
        name = ss.str();
        if (getMarkerByName(name)) {
            foundItem = true;
        } else {
            foundItem = false;
        }
        ++no;
    } while (foundItem);
    return name;
}

boost::shared_ptr<TrackMarker>
TrackerContext::createMarker()
{
    boost::shared_ptr<TrackMarker> track(new TrackMarker(shared_from_this()));
    std::string name = generateUniqueTrackName(kTrackBaseName);
    track->setScriptName(name);
    track->setLabel(name);
    track->resetCenter();
    int index;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        index  = _imp->markers.size();
        _imp->markers.push_back(track);
    }
    Q_EMIT trackInserted(track, index);
    return track;
    
}

int
TrackerContext::getMarkerIndex(const boost::shared_ptr<TrackMarker>& marker) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            return (int)i;
        }
    }
    return -1;
}

boost::shared_ptr<TrackMarker>
TrackerContext::getPrevMarker(const boost::shared_ptr<TrackMarker>& marker, bool loop) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            if (i > 0) {
                return _imp->markers[i - 1];
            }
        }
    }
    return (_imp->markers.size() == 0 || !loop) ? boost::shared_ptr<TrackMarker>() : _imp->markers[_imp->markers.size() - 1];
}

boost::shared_ptr<TrackMarker>
TrackerContext::getNextMarker(const boost::shared_ptr<TrackMarker>& marker, bool loop) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::size_t i = 0; i < _imp->markers.size(); ++i) {
        if (_imp->markers[i] == marker) {
            if (i < (_imp->markers.size() - 1)) {
                return _imp->markers[i + 1];
            } else if (!loop) {
                return boost::shared_ptr<TrackMarker>();
            }
        }
    }
    return (_imp->markers.size() == 0 || !loop || _imp->markers[0] == marker) ? boost::shared_ptr<TrackMarker>() : _imp->markers[0];
}

void
TrackerContext::appendMarker(const boost::shared_ptr<TrackMarker>& marker)
{
    int index;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        index = _imp->markers.size();
        _imp->markers.push_back(marker);
    }
    Q_EMIT trackInserted(marker, index);
}

void
TrackerContext::insertMarker(const boost::shared_ptr<TrackMarker>& marker, int index)
{
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        assert(index >= 0);
        if (index >= (int)_imp->markers.size()) {
            _imp->markers.push_back(marker);
        } else {
            std::vector<boost::shared_ptr<TrackMarker> >::iterator it = _imp->markers.begin();
            std::advance(it, index);
            _imp->markers.insert(it, marker);
        }
    }
    Q_EMIT trackInserted(marker,index);
}

void
TrackerContext::removeMarker(const boost::shared_ptr<TrackMarker>& marker)
{
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = _imp->markers.begin(); it != _imp->markers.end(); ++it) {
            if (*it == marker) {
                _imp->markers.erase(it);
                break;
            }
        }
    }
    beginEditSelection();
    removeTrackFromSelection(marker, TrackerContext::eTrackSelectionInternal);
    endEditSelection(TrackerContext::eTrackSelectionInternal);
    Q_EMIT trackRemoved(marker);
}

boost::shared_ptr<Natron::Node>
TrackerContext::getNode() const
{
    return _imp->node.lock();
}

int
TrackerContext::getTimeLineFirstFrame() const
{
    boost::shared_ptr<Natron::Node> node = getNode();
    if (!node) {
        return -1;
    }
    double first,last;
    node->getApp()->getProject()->getFrameRange(&first, &last);
    return (int)first;
}

int
TrackerContext::getTimeLineLastFrame() const
{
    boost::shared_ptr<Natron::Node> node = getNode();
    if (!node) {
        return -1;
    }
    double first,last;
    node->getApp()->getProject()->getFrameRange(&first, &last);
    return (int)last;
}

void
TrackerContextPrivate::beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const
{
    options->minimum_correlation = minCorrelation.lock()->getValue();
    options->max_iterations = maxIterations.lock()->getValue();
    options->use_brute_initialization = bruteForcePreTrack.lock()->getValue();
    options->use_normalized_intensities = useNormalizedIntensities.lock()->getValue();
    options->sigma = preBlurSigma.lock()->getValue();

}

void
TrackerContextPrivate::endLibMVOptionsForTrack(const TrackMarker& marker,
                              mv::TrackRegionOptions* options) const
{
    int mode_i = marker.getMotionModelKnob()->getValue();
    switch (mode_i) {
        case 0:
            options->mode = mv::TrackRegionOptions::TRANSLATION;
            break;
        case 1:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION;
            break;
        case 2:
            options->mode = mv::TrackRegionOptions::TRANSLATION_SCALE;
            break;
        case 3:
            options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE;
            break;
        case 4:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
        case 5:
            options->mode = mv::TrackRegionOptions::HOMOGRAPHY;
            break;
        default:
            options->mode = mv::TrackRegionOptions::AFFINE;
            break;
    }
}

struct FrameAccessorCacheKey
{
    int frame;
    int mipMapLevel;
    mv::FrameAccessor::InputMode mode;
};

struct CacheKey_compare_less
{
    bool operator() (const FrameAccessorCacheKey & lhs,
                     const FrameAccessorCacheKey & rhs) const
    {
        if (lhs.frame < rhs.frame) {
            return true;
        } else if (lhs.frame > rhs.frame) {
            return false;
        } else {
            if (lhs.mipMapLevel < rhs.mipMapLevel) {
                return true;
            } else if (lhs.mipMapLevel > rhs.mipMapLevel) {
                return false;
            } else {
                if ((int)lhs.mode < (int)rhs.mode) {
                    return true;
                } else if ((int)lhs.mode > (int)rhs.mode) {
                    return false;
                } else {
                    return false;
                }
            }
                
        }
    }
};


class MvFloatImage : public libmv::Array3D<float>
{
    
public:
    
    MvFloatImage()
    : libmv::Array3D<float>()
    {
        
    }
    
    MvFloatImage(int height, int width)
    : libmv::Array3D<float>(height, width)
    {
        
    }
    
    MvFloatImage(float* data, int height, int width)
    : libmv::Array3D<float>(data, height, width)
    {
        
    }
    
    virtual ~MvFloatImage()
    {
        
    }
};

struct FrameAccessorCacheEntry
{
    boost::shared_ptr<MvFloatImage> image;
    
    // If null, this is the full image
    RectI bounds;
    
    unsigned int referenceCount;
};

class FrameAccessorImpl : public mv::FrameAccessor
{
    const TrackerContext* _context;
    boost::shared_ptr<Natron::Node> _trackerInput;
    typedef std::multimap<FrameAccessorCacheKey, FrameAccessorCacheEntry, CacheKey_compare_less > FrameAccessorCache;
    
    mutable QMutex _cacheMutex;
    FrameAccessorCache _cache;
    bool _enabledChannels[3];
    int _formatHeight;
    
public:
    
    FrameAccessorImpl(const TrackerContext* context,
                      bool enabledChannels[3],
                      int formatHeight)
    : _context(context)
    , _trackerInput()
    , _cacheMutex()
    , _cache()
    , _enabledChannels()
    , _formatHeight(formatHeight)
    {
        _trackerInput = context->getNode()->getInput(0);
        assert(_trackerInput);
        for (int i = 0; i < 3; ++i) {
            _enabledChannels[i] = enabledChannels[i];
        }
    }
    
    virtual ~FrameAccessorImpl()
    {
        
    }
    
    void getEnabledChannels(bool* r, bool* g, bool* b) const
    {
        *r = _enabledChannels[0];
        *g = _enabledChannels[1];
        *b = _enabledChannels[2];
    }
    
    // Get a possibly-filtered version of a frame of a video. Downscale will
    // cause the input image to get downscaled by 2^downscale for pyramid access.
    // Region is always in original-image coordinates, and describes the
    // requested area. The transform describes an (optional) transform to apply
    // to the image before it is returned.
    //
    // When done with an image, you must call ReleaseImage with the returned key.
    virtual mv::FrameAccessor::Key GetImage(int clip,
                                            int frame,
                                            mv::FrameAccessor::InputMode input_mode,
                                            int downscale,               // Downscale by 2^downscale.
                                            const mv::Region* region,        // Get full image if NULL.
                                            const mv::FrameAccessor::Transform* transform,  // May be NULL.
                                            mv::FloatImage* destination) OVERRIDE FINAL;
    
    // Releases an image from the frame accessor. Non-caching implementations may
    // free the image immediately; others may hold onto the image.
    virtual void ReleaseImage(Key) OVERRIDE FINAL;
    
    virtual bool GetClipDimensions(int clip, int* width, int* height) OVERRIDE FINAL;
    virtual int NumClips() OVERRIDE FINAL;
    virtual int NumFrames(int clip) OVERRIDE FINAL;
    
private:
    
    
};

void
TrackArgsLibMV::getEnabledChannels(bool* r, bool* g, bool* b) const
{
    _fa->getEnabledChannels(r,g,b);
}


template <bool doR, bool doG, bool doB>
void natronImageToLibMvFloatImageForChannels(const Natron::Image* source,
                                             const RectI& roi,
                                             MvFloatImage& mvImg)
{
    //mvImg is expected to have its bounds equal to roi
    
    Natron::Image::ReadAccess racc(source);
    
    unsigned int compsCount = source->getComponentsCount();
    assert(compsCount == 3);
    unsigned int srcRowElements = source->getRowElements();
    
    assert(source->getBounds().contains(roi));
    const float* src_pixels = (const float*)racc.pixelAt(roi.x1, roi.y2 - 1);
    assert(src_pixels);
    float* dst_pixels = mvImg.Data();
    assert(dst_pixels);
    //LibMV images have their origin in the top left hand corner
    
    // It's important to rescale the resultappropriately so that e.g. if only
    // blue is selected, it's not zeroed out.
    float scale = (doR ? 0.2126f : 0.0f) +
    (doG ? 0.7152f : 0.0f) +
    (doB ? 0.0722f : 0.0f);
    
    int h = roi.height();
    int w = roi.width();
    for (int y = 0; y < h ; ++y,
         src_pixels -= (srcRowElements + compsCount * w)) {
        
        for (int x = 0; x < w; ++x,
             src_pixels += compsCount,
             ++dst_pixels) {
            
            /// Apply luminance conversion while we copy the image
            /// This code is taken from DisableChannelsTransform::run in libmv/autotrack/autotrack.cc
            *dst_pixels = (0.2126f * (doR ? src_pixels[0] : 0.0f) +
                           0.7152f * (doG ? src_pixels[1] : 0.0f) +
                           0.0722f * (doB ? src_pixels[2] : 0.0f)) / scale;
        }
    }
}


void natronImageToLibMvFloatImage(bool enabledChannels[3],
                                  const Natron::Image* source,
                                  const RectI& roi,
                                  MvFloatImage& mvImg)
{
    
    if (enabledChannels[0]) {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true,true,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true,true,false>(source,roi,mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<true,false,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<true,false,false>(source,roi,mvImg);
            }
        }
        
    } else {
        if (enabledChannels[1]) {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false,true,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false,true,false>(source,roi,mvImg);
            }
        } else {
            if (enabledChannels[2]) {
                natronImageToLibMvFloatImageForChannels<false,false,true>(source,roi,mvImg);
            } else {
                natronImageToLibMvFloatImageForChannels<false,false,false>(source,roi,mvImg);
            }
        }
    }
}

/*
 * @brief This is called by LibMV to retrieve an image either for reference or as search frame.
 */
mv::FrameAccessor::Key
FrameAccessorImpl::GetImage(int /*clip*/,
                            int frame,
                            mv::FrameAccessor::InputMode input_mode,
                            int downscale,               // Downscale by 2^downscale.
                            const mv::Region* region,        // Get full image if NULL.
                            const mv::FrameAccessor::Transform* /*transform*/,  // May be NULL.
                            mv::FloatImage* destination)
{
    
    // Since libmv only uses MONO images for now we have only optimized for this case, remove and handle properly
    // other case(s) when they get integrated into libmv.
    assert(input_mode == mv::FrameAccessor::MONO);
    
    
    
    FrameAccessorCacheKey key;
    key.frame = frame;
    key.mipMapLevel = downscale;
    key.mode = input_mode;
    
    /*
     Check if a frame exists in the cache with matching key and bounds enclosing the given region
     */
    RectI roi;
    if (region) {
        convertLibMVRegionToRectI(*region,_formatHeight, &roi);

        QMutexLocker k(&_cacheMutex);
        std::pair<FrameAccessorCache::iterator,FrameAccessorCache::iterator> range = _cache.equal_range(key);
        for (FrameAccessorCache::iterator it = range.first; it != range.second; ++it) {
            if (roi.x1 >= it->second.bounds.x1 && roi.x2 <= it->second.bounds.x2 &&
                roi.y1 >= it->second.bounds.y1 && roi.y2 <= it->second.bounds.y2) {
                // LibMV is kinda dumb on this we must necessarily copy the data either via CopyFrom or the
                // assignment constructor
#ifdef TRACE_LIB_MV
                qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Found cached image at frame" << frame << "with RoI x1="
                << region->min(0) << "y1=" << region->max(1) << "x2=" << region->max(0) << "y2=" << region->min(1);
#endif
                destination->CopyFrom<float>(*it->second.image);
                ++it->second.referenceCount;
                return (mv::FrameAccessor::Key)it->second.image.get();
            }
            
        }
    }
    
    // Not in accessor cache, call renderRoI
    RenderScale scale;
    scale.y = scale.x = Image::getScaleFromMipMapLevel((unsigned int)downscale);
    
    
    RectD precomputedRoD;
    if (!region) {
        bool isProjectFormat;
        Natron::StatusEnum stat = _trackerInput->getEffectInstance()->getRegionOfDefinition_public(_trackerInput->getHashValue(), frame, scale, 0, &precomputedRoD, &isProjectFormat);
        if (stat == eStatusFailed) {
            return (mv::FrameAccessor::Key)0;
        }
        double par = _trackerInput->getEffectInstance()->getPreferredAspectRatio();
        precomputedRoD.toPixelEnclosing((unsigned int)downscale, par, &roi);
    }
    
    std::list<ImageComponents> components;
    components.push_back(ImageComponents::getRGBComponents());
    
    NodePtr node = _context->getNode();
    
    ParallelRenderArgsSetter frameRenderArgs(frame,
                                             0, //<  view 0 (left)
                                             true, //<isRenderUserInteraction
                                             false, //isSequential
                                             false, //can abort
                                             0, //render Age
                                             node, //  requester
                                             0, // request
                                             0, //texture index
                                             node->getApp()->getTimeLine().get(), //Timeline
                                             NodePtr(), // rotoPaintNode
                                             true, //isAnalysis
                                             false,//draftMode
                                             false, //viewer progress
                                             boost::shared_ptr<RenderStats>()); // Stats

    
    EffectInstance::RenderRoIArgs args(frame,
                                       scale,
                                       downscale,
                                       0,
                                       false,
                                       roi,
                                       precomputedRoD,
                                       components,
                                       Natron::eImageBitDepthFloat,
                                       true,
                                       _context->getNode()->getEffectInstance().get());
    ImageList planes;
    EffectInstance::RenderRoIRetCode stat = _trackerInput->getEffectInstance()->renderRoI(args, &planes);
    if (stat != EffectInstance::eRenderRoIRetCodeOk || planes.empty()) {
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Failed to call renderRoI on input at frame" << frame << "with RoI x1="
        << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2;
#endif
        return (mv::FrameAccessor::Key)0;
    }
    
    assert(!planes.empty());
    const ImagePtr& sourceImage = planes.front();
    RectI sourceBounds = sourceImage->getBounds();
    RectI intersectedRoI;
    if (!roi.intersect(sourceBounds, &intersectedRoI)) {
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "RoI does not intersect the source image bounds (RoI x1="
        << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif
        return (mv::FrameAccessor::Key)0;

    }
    
#ifdef TRACE_LIB_MV
    qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "renderRoi (frame" << frame << ") OK  (BOUNDS= x1="
    << sourceBounds.x1 << "y1=" << sourceBounds.y1 << "x2=" << sourceBounds.x2 << "y2=" << sourceBounds.y2 << ") (ROI = " << roi.x1 << "y1=" << roi.y1 << "x2=" << roi.x2 << "y2=" << roi.y2 << ")";
#endif
    
    /*
     Copy the Natron image to the LivMV float image
     */
    FrameAccessorCacheEntry entry;
    entry.image.reset(new MvFloatImage(intersectedRoI.height(), intersectedRoI.width()));
    entry.bounds = intersectedRoI;
    entry.referenceCount = 1;
    natronImageToLibMvFloatImage(_enabledChannels,
                                 sourceImage.get(),
                                 intersectedRoI,
                                 *entry.image);
    // we ignore the transform parameter and do it in natronImageToLibMvFloatImage instead
    
    // LibMV is kinda dumb on this we must necessarily copy the data...
    destination->CopyFrom<float>(*entry.image);
    
    //insert into the cache
    {
        QMutexLocker k(&_cacheMutex);
        _cache.insert(std::make_pair(key,entry));
    }
#ifdef TRACE_LIB_MV
    qDebug() << QThread::currentThread() << "FrameAccessor::GetImage():" << "Rendered frame" << frame << "with RoI x1="
    << intersectedRoI.x1 << "y1=" << intersectedRoI.y1 << "x2=" << intersectedRoI.x2 << "y2=" << intersectedRoI.y2;
#endif
    return (mv::FrameAccessor::Key)entry.image.get();
}

void
FrameAccessorImpl::ReleaseImage(Key key)
{
    MvFloatImage* imgKey = (MvFloatImage*)key;
    QMutexLocker k(&_cacheMutex);
    for (FrameAccessorCache::iterator it = _cache.begin(); it != _cache.end(); ++it) {
        if (it->second.image.get() == imgKey) {
            --it->second.referenceCount;
            if (!it->second.referenceCount) {
                _cache.erase(it);
                return;
            }
        }
    }
}

// Not used in LibMV
bool
FrameAccessorImpl::GetClipDimensions(int /*clip*/, int* /*width*/, int* /*height*/)
{
    return false;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
FrameAccessorImpl::NumClips()
{
    return 1;
}

// Only used in AutoTrack::DetectAndTrack which is w.i.p in LibMV so don't bother implementing it
int
FrameAccessorImpl::NumFrames(int /*clip*/)
{
    return 0;
}

void
TrackerContext::trackMarkers(const std::list<boost::shared_ptr<TrackMarker> >& markers,
                             int start,
                             int end,
                             bool forward,
                             bool updateViewer,
                             bool centerViewer,
                             ViewerInstance* viewer)
{
    
    if (markers.empty()) {
        return;
    }
    
    /// The channels we are going to use for tracking
    bool enabledChannels[3];
    enabledChannels[0] = _imp->enableTrackRed.lock()->getValue();
    enabledChannels[1] = _imp->enableTrackGreen.lock()->getValue();
    enabledChannels[2] = _imp->enableTrackBlue.lock()->getValue();
    
    double formatWidth,formatHeight;
    Format f;
    getNode()->getApp()->getProject()->getProjectDefaultFormat(&f);
    formatWidth = f.width();
    formatHeight = f.height();
    
    /// The accessor and its cache is local to a track operation, it is wiped once the whole sequence track is finished.
    boost::shared_ptr<FrameAccessorImpl> accessor(new FrameAccessorImpl(this, enabledChannels, formatHeight));
    boost::shared_ptr<mv::AutoTrack> trackContext(new mv::AutoTrack(accessor.get()));
    
    
    
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > trackAndOptions;
    
    mv::TrackRegionOptions mvOptions;
    /*
     Get the global parameters for the LivMV track: pre-blur sigma, No iterations, normalized intensities, etc...
     */
    _imp->beginLibMVOptionsForTrack(&mvOptions);
    
    /*
     For the given markers, do the following:
     - Get the "User" keyframes that have been set and create a LibMV marker for each keyframe as well as for the "start" time
     - Set the "per-track" parameters that were given by the user, that is the mv::TrackRegionOptions
     - t->mvMarker will contain the marker that evolves throughout the tracking
     */
    int trackIndex = 0;
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers.begin(); it!= markers.end(); ++it, ++trackIndex) {
        boost::shared_ptr<TrackMarkerAndOptions> t(new TrackMarkerAndOptions);
        t->natronMarker = *it;
        
        std::set<int> userKeys;
        t->natronMarker->getUserKeyframes(&userKeys);
        
        // Add a libmv marker for all keyframes
        for (std::set<int>::iterator it2 = userKeys.begin(); it2 != userKeys.end(); ++it2) {
            
            if (*it2 == start) {
                //Will be added in the track step
                continue;
            } else {
                mv::Marker marker;
                natronTrackerToLibMVTracker(false, enabledChannels, *t->natronMarker, trackIndex, *it2, forward, formatHeight, &marker);
                assert(marker.source == mv::Marker::MANUAL);
                trackContext->AddMarker(marker);
            }
            
            
        }
        
        
        //For all already tracked frames which are not keyframes, add them to the AutoTrack too
        std::set<double> centerKeys;
        t->natronMarker->getCenterKeyframes(&centerKeys);
        for (std::set<double>::iterator it2 = centerKeys.begin(); it2 != centerKeys.end(); ++it2) {
            if (userKeys.find(*it2) != userKeys.end()) {
                continue;
            }
            if (*it2 == start) {
                //Will be added in the track step
                continue;
            } else {
                mv::Marker marker;
                natronTrackerToLibMVTracker(false, enabledChannels, *t->natronMarker, trackIndex, *it2, forward, formatHeight, &marker);
                assert(marker.source == mv::Marker::TRACKED);
                trackContext->AddMarker(marker);
            }
        }
        
        
        
        
        t->mvOptions = mvOptions;
        _imp->endLibMVOptionsForTrack(*t->natronMarker, &t->mvOptions);
        trackAndOptions.push_back(t);
    }
    
    
    /*
     Launch tracking in the scheduler thread.
     */
    TrackArgsLibMV args(start, end, forward, getNode()->getApp()->getTimeLine(), updateViewer, centerViewer, viewer, trackContext, accessor, trackAndOptions,formatWidth,formatHeight);
    _imp->scheduler.track(args);
}

void
TrackerContext::trackSelectedMarkers(int start, int end, bool forward, bool updateViewer, bool centerViewer, ViewerInstance* viewer)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        for (std::list<boost::shared_ptr<TrackMarker> >::iterator it = _imp->selectedMarkers.begin();
             it != _imp->selectedMarkers.end(); ++it) {
            if ((*it)->isEnabled()) {
                markers.push_back(*it);
            }
        }
    }
    trackMarkers(markers,start,end,forward,updateViewer, centerViewer, viewer);
}

bool
TrackerContext::isCurrentlyTracking() const
{
    return _imp->scheduler.isWorking();
}

void
TrackerContext::abortTracking()
{
    _imp->scheduler.abortTracking();
}

void
TrackerContext::beginEditSelection()
{
    QMutexLocker k(&_imp->trackerContextMutex);
    _imp->incrementSelectionCounter();
}

void
TrackerContext::endEditSelection(TrackSelectionReason reason)
{
    bool doEnd = false;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        _imp->decrementSelectionCounter();
        
        if (_imp->beginSelectionCounter == 0) {
            doEnd = true;
            
        }
    }
    if (doEnd) {
        endSelection(reason);
    }
}

void
TrackerContext::addTrackToSelection(const boost::shared_ptr<TrackMarker>& mark, TrackSelectionReason reason)
{
    std::list<boost::shared_ptr<TrackMarker> > marks;
    marks.push_back(mark);
    addTracksToSelection(marks, reason);
}

void
TrackerContext::addTracksToSelection(const std::list<boost::shared_ptr<TrackMarker> >& marks, TrackSelectionReason reason)
{
    bool hasCalledBegin = false;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        
        if (!_imp->beginSelectionCounter) {
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }
        
        for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = marks.begin() ; it!=marks.end(); ++it) {
            _imp->addToSelectionList(*it);
        }
        
        if (hasCalledBegin) {
            _imp->decrementSelectionCounter();
        }
    }
    if (hasCalledBegin) {
        endSelection(reason);
        
    }
}

void
TrackerContext::removeTrackFromSelection(const boost::shared_ptr<TrackMarker>& mark, TrackSelectionReason reason)
{
    std::list<boost::shared_ptr<TrackMarker> > marks;
    marks.push_back(mark);
    removeTracksFromSelection(marks, reason);
}

void
TrackerContext::removeTracksFromSelection(const std::list<boost::shared_ptr<TrackMarker> >& marks, TrackSelectionReason reason)
{
    bool hasCalledBegin = false;
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        
        if (!_imp->beginSelectionCounter) {
            _imp->incrementSelectionCounter();
            hasCalledBegin = true;
        }
        
        for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = marks.begin() ; it!=marks.end(); ++it) {
            _imp->removeFromSelectionList(*it);
        }
        
        if (hasCalledBegin) {
            _imp->decrementSelectionCounter();
        }
    }
    if (hasCalledBegin) {
        endSelection(reason);
    }
}

void
TrackerContext::clearSelection(TrackSelectionReason reason)
{
    std::list<boost::shared_ptr<TrackMarker> > markers;
    getSelectedMarkers(&markers);
    removeTracksFromSelection(markers, reason);
}

void
TrackerContext::selectAll(TrackSelectionReason reason)
{
    beginEditSelection();
    std::vector<boost::shared_ptr<TrackMarker> > markers;
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        markers = _imp->markers;
    }
    for (std::vector<boost::shared_ptr<TrackMarker> >::iterator it = markers.begin(); it!= markers.end(); ++it) {
        addTrackToSelection(*it, reason);
    }
    endEditSelection(reason);
    
}

void
TrackerContext::getAllMarkers(std::vector<boost::shared_ptr<TrackMarker> >* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    *markers = _imp->markers;
}

void
TrackerContext::getSelectedMarkers(std::list<boost::shared_ptr<TrackMarker> >* markers) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    *markers = _imp->selectedMarkers;
}

bool
TrackerContext::isMarkerSelected(const boost::shared_ptr<TrackMarker>& marker) const
{
    QMutexLocker k(&_imp->trackerContextMutex);
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = _imp->selectedMarkers.begin(); it!=_imp->selectedMarkers.end(); ++it) {
        if (*it == marker) {
            return true;
        }
    }
    return false;
}

void
TrackerContextPrivate::linkMarkerKnobsToGuiKnobs(const std::list<boost::shared_ptr<TrackMarker> >& markers,
                                                 bool multipleTrackSelected,
                                                 bool slave)
{
    std::list<boost::shared_ptr<TrackMarker> >::const_iterator next = markers.begin();
    if (!markers.empty()) {
        ++next;
    }
    for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers.begin() ; it!= markers.end(); ++it) {
        const std::list<boost::shared_ptr<KnobI> >& trackKnobs = (*it)->getKnobs();
        for (std::list<boost::shared_ptr<KnobI> >::const_iterator it2 = trackKnobs.begin(); it2 != trackKnobs.end(); ++it2) {
            
            // Find the knob in the TrackerContext knobs
            boost::shared_ptr<KnobI> found;
            for (std::list<boost::weak_ptr<KnobI> >::iterator it3 = perTrackKnobs.begin(); it3 != perTrackKnobs.end(); ++it3) {
                boost::shared_ptr<KnobI> k = it3->lock();
                if (k->getName() == (*it2)->getName()) {
                    found = k;
                    break;
                }
            }
            
            if (!found) {
                continue;
            }
            
            //Clone current state only for the last marker
            if (next == markers.end()) {
                found->cloneAndUpdateGui(it2->get());
            }
            
            //Slave internal knobs
            assert((*it2)->getDimension() == found->getDimension());
            for (int i = 0; i < (*it2)->getDimension(); ++i) {
                if (slave) {
                    (*it2)->slaveTo(i, found, i);
                } else {
                    (*it2)->unSlave(i, !multipleTrackSelected);
                }
            }
            
            if (!slave) {
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,int,int,bool)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,int,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,double,double)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::disconnect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,int)),
                                    _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
            } else {
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameSet(double,int,int,bool)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameRemoved(double,int,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameMoved(int,double,double)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(animationRemoved(int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(derivativeMoved(double,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
                
                QObject::connect((*it2)->getSignalSlotHandler().get(), SIGNAL(keyFrameInterpolationChanged(double,int)),
                                 _publicInterface, SLOT(onSelectedKnobCurveChanged()));
            }
            
        }
        if (next != markers.end()) {
            ++next;
        }
    } // for (std::list<boost::shared_ptr<TrackMarker> >::const_iterator it = markers() ; it!=markers(); ++it)
}

void
TrackerContext::endSelection(TrackSelectionReason reason)
{
    assert(QThread::currentThread() == qApp->thread());
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        if (_imp->selectionRecursion > 0) {
            _imp->markersToSlave.clear();
            _imp->markersToUnslave.clear();
            return;
        }
        if (_imp->markersToSlave.empty() && _imp->markersToUnslave.empty()) {
            return;
        }

    }
    ++_imp->selectionRecursion;
    
    Q_EMIT selectionAboutToChange((int)reason);
    
    {
        QMutexLocker k(&_imp->trackerContextMutex);
        
        // Slave newly selected knobs
        bool selectionIsDirty = _imp->selectedMarkers.size() > 1;
        bool selectionEmpty = _imp->selectedMarkers.empty();
        
        
        //_imp->linkMarkerKnobsToGuiKnobs(_imp->markersToUnslave, selectionIsDirty, false);
        _imp->markersToUnslave.clear();
        
        
        
        //_imp->linkMarkerKnobsToGuiKnobs(_imp->markersToSlave, selectionIsDirty, true);
        _imp->markersToSlave.clear();
        
        
        
        
        for (std::list<boost::weak_ptr<KnobI> >::iterator it = _imp->perTrackKnobs.begin(); it != _imp->perTrackKnobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (!k) {
                continue;
            }
            k->setAllDimensionsEnabled(!selectionEmpty);
            k->setDirty(selectionIsDirty);
        }
    } //  QMutexLocker k(&_imp->trackerContextMutex);
    Q_EMIT selectionChanged((int)reason);
    
    --_imp->selectionRecursion;
}

void
TrackerContext::onSelectedKnobCurveChanged()
{
    KnobSignalSlotHandler* handler = qobject_cast<KnobSignalSlotHandler*>(sender());
    if (handler) {
        boost::shared_ptr<KnobI> knob = handler->getKnob();
        for (std::list<boost::weak_ptr<KnobI> >::const_iterator it = _imp->knobs.begin(); it != _imp->knobs.end(); ++it) {
            boost::shared_ptr<KnobI> k = it->lock();
            if (k->getName() == knob->getName()) {
                k->clone(knob.get());
                break;
            }
        }
    }
}

//////////////////////// TrackScheduler

struct TrackSchedulerPrivate
{
    
    mutable QMutex mustQuitMutex;
    bool mustQuit;
    QWaitCondition mustQuitCond;
    
    mutable QMutex abortRequestedMutex;
    int abortRequested;
    QWaitCondition abortRequestedCond;
    
    QMutex startRequesstMutex;
    int startRequests;
    QWaitCondition startRequestsCond;
    
    mutable QMutex isWorkingMutex;
    bool isWorking;
    
    
    TrackSchedulerPrivate()
    : mustQuitMutex()
    , mustQuit(false)
    , mustQuitCond()
    , abortRequestedMutex()
    , abortRequested(0)
    , abortRequestedCond()
    , startRequesstMutex()
    , startRequests(0)
    , startRequestsCond()
    , isWorkingMutex()
    , isWorking(false)
    {
        
    }
    
    bool checkForExit()
    {
        QMutexLocker k(&mustQuitMutex);
        if (mustQuit) {
            mustQuit = false;
            mustQuitCond.wakeAll();
            return true;
        }
        return false;
    }
    
};

TrackSchedulerBase::TrackSchedulerBase()
: QThread()
{
    QObject::connect(this, SIGNAL(renderCurrentFrameForViewer(ViewerInstance*)), this, SLOT(doRenderCurrentFrameForViewer(ViewerInstance*)));
}

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::TrackScheduler(TrackStepFunctor functor)
: TrackSchedulerBase()
, _imp(new TrackSchedulerPrivate())
, argsMutex()
, curArgs()
, requestedArgs()
, _functor(functor)
{
    setObjectName("TrackScheduler");
}

template <class TrackArgsType>
TrackScheduler<TrackArgsType>::~TrackScheduler()
{
    
}

template <class TrackArgsType>
bool
TrackScheduler<TrackArgsType>::isWorking() const
{
    QMutexLocker k(&_imp->isWorkingMutex);
    return _imp->isWorking;
}

class IsTrackingFlagSetter_RAII
{
    ViewerInstance* _v;
    TrackSchedulerBase* _base;
    bool _reportProgress;
    
public:
    
    IsTrackingFlagSetter_RAII(TrackSchedulerBase* base, bool reportProgress, ViewerInstance* viewer)
    : _v(viewer)
    , _base(base)
    , _reportProgress(reportProgress)
    {
        
        if (_reportProgress) {
            base->emit_trackingStarted();
        }
        viewer->setDoingPartialUpdates(true);
    }
    
    ~IsTrackingFlagSetter_RAII()
    {
        _v->setDoingPartialUpdates(false);
        if (_reportProgress) {
            _base->emit_trackingFinished();
        }

    }
};

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::run()
{
    for (;;) {
        
        ///Check for exit of the thread
        if (_imp->checkForExit()) {
            return;
        }
        
        ///Flag that we're working
        {
            QMutexLocker k(&_imp->isWorkingMutex);
            _imp->isWorking = true;
        }
        
        ///Copy the requested args to the args used for processing
        {
            QMutexLocker k(&argsMutex);
            curArgs = requestedArgs;
        }
        
        boost::shared_ptr<TimeLine> timeline = curArgs.getTimeLine();
        
        ViewerInstance* viewer =  curArgs.getViewer();
        assert(viewer);
        
        bool isUpdateViewerOnTrackingEnabled = curArgs.isUpdateViewerEnabled();
        bool isCenterViewerEnabled = curArgs.isCenterViewerEnabled();
        
        
        int end = curArgs.getEnd();
        int start = curArgs.getStart();
        int cur = start;
        bool isForward = curArgs.getForward();
        int framesCount = isForward ? (end - start) : (start - end);
        int numTracks = curArgs.getNumTracks();
        
        std::vector<int> trackIndexes;
        for (std::size_t i = 0; i < (std::size_t)numTracks; ++i) {
            trackIndexes.push_back(i);
        }
        
        int lastValidFrame = isForward ? start - 1 : start + 1;
        bool reportProgress = numTracks > 1 || framesCount > 1;

        {
            ///Use RAII style for setting the isDoingPartialUpdates flag so we're sure it gets removed
            IsTrackingFlagSetter_RAII __istrackingflag__(this, reportProgress, viewer);

            while (cur != end) {
                
                
                
                ///Launch parallel thread for each track using the global thread pool
                QFuture<bool> future = QtConcurrent::mapped(trackIndexes,
                                                            boost::bind(_functor,
                                                                        _1,
                                                                        curArgs,
                                                                        cur));
                future.waitForFinished();
                
                bool failure = false;
                for (QFuture<bool>::const_iterator it = future.begin(); it != future.end(); ++it) {
                    if (!(*it)) {
                        failure = true;
                        break;
                    }
                }
                if (failure) {
                    break;
                }
                
                lastValidFrame = cur;
                
                
                double progress;
                if (isForward) {
                    ++cur;
                    progress = (double)(cur - start) / framesCount;
                } else {
                    --cur;
                    progress = (double)(start - cur) / framesCount;
                }
                
                
                ///Ok all tracks are finished now for this frame, refresh viewer if needed
                if (isUpdateViewerOnTrackingEnabled && viewer) {
                    
                    //This will not refresh the viewer since when tracking, renderCurrentFrame()
                    //is not called on viewers, see Gui::onTimeChanged
                    timeline->seekFrame(cur, true, 0, Natron::eTimelineChangeReasonOtherSeek);
                    
                    ///Beyond TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE it becomes more expensive to render all partial rectangles
                    ///than just render the whole viewer RoI
                    if (numTracks < TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE) {
                        std::list<RectD> updateRects;
                        curArgs.getRedrawAreasNeeded(cur, &updateRects);
                        viewer->setPartialUpdateParams(updateRects, isCenterViewerEnabled);
                    } else {
                        viewer->clearPartialUpdateParams();
                    }
                    Q_EMIT renderCurrentFrameForViewer(viewer);
                }
                
                if (reportProgress) {
                    ///Notify we progressed of 1 frame
                    emit_progressUpdate(progress);
                }
                
                ///Check for abortion
                {
                    QMutexLocker k(&_imp->abortRequestedMutex);
                    if (_imp->abortRequested > 0) {
                        _imp->abortRequested = 0;
                        _imp->abortRequestedCond.wakeAll();
                        break;
                    }
                }
                
            } // while (cur != end) {
            
        
        } // IsTrackingFlagSetter_RAII
        
        
        //Now that tracking is done update viewer once to refresh the whole visible portion
        
        if (isUpdateViewerOnTrackingEnabled) {
            //Refresh all viewers to the current frame
            timeline->seekFrame(lastValidFrame, true, 0, Natron::eTimelineChangeReasonOtherSeek);
        }
        
        ///Flag that we're no longer working
        {
            QMutexLocker k(&_imp->isWorkingMutex);
            _imp->isWorking = false;
        }
        
        ///Make sure we really reset the abort flag
        {
            QMutexLocker k(&_imp->abortRequestedMutex);
            if (_imp->abortRequested > 0) {
                _imp->abortRequested = 0;
                
            }
        }
        
        ///Sleep or restart if we've got requests in the queue
        {
            QMutexLocker k(&_imp->startRequesstMutex);
            while (_imp->startRequests <= 0) {
                _imp->startRequestsCond.wait(&_imp->startRequesstMutex);
            }
            _imp->startRequests = 0;
        }
        
    } // for (;;) {
}

void
TrackSchedulerBase::doRenderCurrentFrameForViewer(ViewerInstance* viewer)
{
    assert(QThread::currentThread() == qApp->thread());
    viewer->renderCurrentFrame(true);
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::track(const TrackArgsType& args)
{
    if ((args.getForward() && args.getStart() >= args.getEnd()) || (!args.getForward() && args.getStart() <= args.getEnd())) {
        Q_EMIT trackingFinished();
        return;
    }
    {
        QMutexLocker k(&argsMutex);
        requestedArgs = args;
    }
    if (isRunning()) {
        QMutexLocker k(&_imp->startRequesstMutex);
        ++_imp->startRequests;
        _imp->startRequestsCond.wakeAll();
    } else {
        start();
    }
}


template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::abortTracking()
{
    if (!isRunning() || !isWorking()) {
        return;
    }
    
    
    {
        QMutexLocker k(&_imp->abortRequestedMutex);
        ++_imp->abortRequested;
        _imp->abortRequestedCond.wakeAll();
    }
    
}

template <class TrackArgsType>
void
TrackScheduler<TrackArgsType>::quitThread()
{
    if (!isRunning()) {
        return;
    }
    
    abortTracking();
    
    {
        QMutexLocker k(&_imp->mustQuitMutex);
        _imp->mustQuit = true;
        
        {
            QMutexLocker k(&_imp->startRequesstMutex);
            ++_imp->startRequests;
            _imp->startRequestsCond.wakeAll();
        }
        
        while (_imp->mustQuit) {
            _imp->mustQuitCond.wait(&_imp->mustQuitMutex);
        }
        
    }
    
    
    wait();
    
}

///Explicit template instantiation for TrackScheduler
template class TrackScheduler<TrackArgsV1>;
template class TrackScheduler<TrackArgsLibMV>;

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_TrackerContext.cpp"

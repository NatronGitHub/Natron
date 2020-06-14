/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include <QtCore/QCoreApplication>

#include "Engine/AbortableRenderInfo.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/NodeSerialization.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/Node.h"
#include "Engine/TrackerContext.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackerSerialization.h"
#include "Engine/TLSHolder.h"

#include <ofxNatron.h>

#define kTrackerPMParamScore "score"
#define kTrackerPMParamTrackingNext kNatronParamTrackingNext
#define kTrackerPMParamTrackingPrevious kNatronParamTrackingPrevious
#define kTrackerPMParamTrackingSearchBoxTopRight "searchBoxTopRight"
#define kTrackerPMParamTrackingSearchBoxBtmLeft "searchBoxBtmLeft"
#define kTrackerPMParamTrackingPatternBoxTopRight "patternBoxTopRight"
#define kTrackerPMParamTrackingPatternBoxBtmLeft "patternBoxBtmLeft"
#define kTrackerPMParamTrackingCorrelationScore "correlation"
#define kTrackerPMParamTrackingReferenceFrame "refFrame"
#define kTrackerPMParamTrackingEnableReferenceFrame "enableRefFrame"
#define kTrackerPMParamTrackingOffset "offset"
#define kTrackerPMParamTrackingCenterPoint "center"


NATRON_NAMESPACE_ENTER


NATRON_NAMESPACE_ANONYMOUS_ENTER

class MetaTypesRegistration
{
public:
    inline MetaTypesRegistration()
    {
        qRegisterMetaType<TrackMarkerPtr>("TrackMarkerPtr");
    }
};

NATRON_NAMESPACE_ANONYMOUS_EXIT

static MetaTypesRegistration registration;

struct TrackMarkerPrivate
{
    TrackMarker* _publicInterface;
    TrackerContextWPtr context;

    // Defines the rectangle of the search window, this is in coordinates relative to the marker center point
    KnobDoubleWPtr searchWindowBtmLeft, searchWindowTopRight;

    // The pattern Quad defined by 4 corners relative to the center
    KnobDoubleWPtr patternTopLeft, patternTopRight, patternBtmRight, patternBtmLeft;
    KnobDoubleWPtr center, offset, error;
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    KnobDoubleWPtr weight;
#endif
    KnobChoiceWPtr motionModel;
    mutable QMutex trackMutex;
    std::set<int> userKeyframes;
    std::string trackScriptName, trackLabel;
    KnobBoolWPtr enabled;

    // Only used by the TrackScheduler thread
    int trackingStartedCount;
    std::list<double> keyframesAddedWhileTracking;

    TrackMarkerPrivate(TrackMarker* publicInterface,
                       const TrackerContextPtr& context)
        : _publicInterface(publicInterface)
        , context(context)
        , searchWindowBtmLeft()
        , searchWindowTopRight()
        , patternTopLeft()
        , patternTopRight()
        , patternBtmRight()
        , patternBtmLeft()
        , center()
        , offset()
        , error()
#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
        , weight()
#endif
        , motionModel()
        , trackMutex()
        , userKeyframes()
        , trackScriptName()
        , trackLabel()
        , enabled()
        , trackingStartedCount(0)
        , keyframesAddedWhileTracking()
    {
    }
};

TrackMarker::TrackMarker(const TrackerContextPtr& context)
    : NamedKnobHolder( context->getNode()->getApp() )
    , boost::enable_shared_from_this<TrackMarker>()
    , _imp( new TrackMarkerPrivate(this, context) )
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TrackMarker::MakeSharedEnabler: public TrackMarker
{
    MakeSharedEnabler(const TrackerContextPtr& context) : TrackMarker(context) {
    }
};


TrackMarkerPtr
TrackMarker::create(const TrackerContextPtr& context)
{
    return boost::make_shared<TrackMarker::MakeSharedEnabler>(context);
}

TrackMarker::~TrackMarker()
{
}

void
TrackMarker::initializeKnobs()
{
    TrackerContextPtr context = _imp->context.lock();
    KnobIntPtr defPatternSizeKnob = context->getDefaultMarkerPatternWinSizeKnob();
    KnobIntPtr defSearchSizeKnob = context->getDefaultMarkerSearchWinSizeKnob();
    KnobChoicePtr defMotionModelKnob = context->getDefaultMotionModelKnob();

    double patternHalfSize = defPatternSizeKnob->getValue() / 2.;
    double searchHalfSize = defSearchSizeKnob->getValue() / 2.;

    int defMotionModel_i = defMotionModelKnob->getValue();

    KnobDoublePtr swbbtmLeft = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamSearchWndBtmLeftLabel), 2, false);

    swbbtmLeft->setName(kTrackerParamSearchWndBtmLeft);
    swbbtmLeft->setDefaultValue(-searchHalfSize, 0);
    swbbtmLeft->setDefaultValue(-searchHalfSize, 1);
    swbbtmLeft->setHintToolTip( tr(kTrackerParamSearchWndBtmLeftHint) );
    _imp->searchWindowBtmLeft = swbbtmLeft;

    KnobDoublePtr swbtRight = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamSearchWndTopRightLabel), 2, false);
    swbtRight->setName(kTrackerParamSearchWndTopRight);
    swbtRight->setDefaultValue(searchHalfSize, 0);
    swbtRight->setDefaultValue(searchHalfSize, 1);
    swbtRight->setHintToolTip( tr(kTrackerParamSearchWndTopRightHint) );
    _imp->searchWindowTopRight = swbtRight;


    KnobDoublePtr ptLeft = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamPatternTopLeftLabel), 2, false);
    ptLeft->setName(kTrackerParamPatternTopLeft);
    ptLeft->setDefaultValue(-patternHalfSize, 0);
    ptLeft->setDefaultValue(patternHalfSize, 1);
    ptLeft->setHintToolTip( tr(kTrackerParamPatternTopLeftHint) );
    _imp->patternTopLeft = ptLeft;

    KnobDoublePtr ptRight = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamPatternTopRightLabel), 2, false);
    ptRight->setName(kTrackerParamPatternTopRight);
    ptRight->setDefaultValue(patternHalfSize, 0);
    ptRight->setDefaultValue(patternHalfSize, 1);
    ptRight->setHintToolTip( tr(kTrackerParamPatternTopRightHint) );
    _imp->patternTopRight = ptRight;

    KnobDoublePtr pBRight = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamPatternBtmRightLabel), 2, false);
    pBRight->setName(kTrackerParamPatternBtmRight);
    pBRight->setDefaultValue(patternHalfSize, 0);
    pBRight->setDefaultValue(-patternHalfSize, 1);
    pBRight->setHintToolTip( tr(kTrackerParamPatternBtmRightHint) );
    _imp->patternBtmRight = pBRight;

    KnobDoublePtr pBLeft = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamPatternBtmLeftLabel), 2, false);
    pBLeft->setName(kTrackerParamPatternBtmLeft);
    pBLeft->setDefaultValue(-patternHalfSize, 0);
    pBLeft->setDefaultValue(-patternHalfSize, 1);
    pBLeft->setHintToolTip( tr(kTrackerParamPatternBtmLeftHint) );
    _imp->patternBtmLeft = pBLeft;

    KnobDoublePtr centerKnob = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamCenterLabel), 2, false);
    centerKnob->setName(kTrackerParamCenter);
    centerKnob->setHintToolTip( tr(kTrackerParamCenterHint) );
    _imp->center = centerKnob;

    KnobDoublePtr offsetKnob = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamOffsetLabel), 2, false);
    offsetKnob->setName(kTrackerParamOffset);
    offsetKnob->setHintToolTip( tr(kTrackerParamOffsetHint) );
    _imp->offset = offsetKnob;

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    KnobDoublePtr weightKnob = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamTrackWeightLabel), 1, false);
    weightKnob->setName(kTrackerParamTrackWeight);
    weightKnob->setHintToolTip( tr(kTrackerParamTrackWeightHint) );
    weightKnob->setDefaultValue(1.);
    weightKnob->setAnimationEnabled(false);
    weightKnob->setMinimum(0.);
    weightKnob->setMaximum(1.);
    _imp->weight = weightKnob;
#endif

    KnobChoicePtr mmodelKnob = AppManager::createKnob<KnobChoice>(this, tr(kTrackerParamMotionModelLabel), 1, false);
    mmodelKnob->setHintToolTip( tr(kTrackerParamMotionModelHint) );
    mmodelKnob->setName(kTrackerParamMotionModel);
    {
        std::vector<ChoiceOption> choices;
        TrackerContext::getMotionModelsAndHelps(true, &choices);
        mmodelKnob->populateChoices(choices);
    }

    mmodelKnob->setDefaultValue(defMotionModel_i);
    _imp->motionModel = mmodelKnob;

    KnobDoublePtr errKnob = AppManager::createKnob<KnobDouble>(this, tr(kTrackerParamErrorLabel), 1, false);
    errKnob->setName(kTrackerParamError);
    _imp->error = errKnob;

    KnobBoolPtr enableKnob = AppManager::createKnob<KnobBool>(this, tr(kTrackerParamEnabledLabel), 1, false);
    enableKnob->setName(kTrackerParamEnabled);
    enableKnob->setHintToolTip( tr(kTrackerParamEnabledHint) );
    enableKnob->setAnimationEnabled(true);
    enableKnob->setDefaultValue(true);
    _imp->enabled = enableKnob;


    QObject::connect( this, SIGNAL(enabledChanged(int)), _imp->context.lock().get(), SLOT(onMarkerEnabledChanged(int)) );
    KnobSignalSlotHandlerPtr handler = _imp->center.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(keyFrameSet(double,ViewSpec,int,int,bool)), this, SLOT(onCenterKeyframeSet(double,ViewSpec,int,int,bool)) );
    QObject::connect( handler.get(), SIGNAL(keyFrameRemoved(double,ViewSpec,int,int)), this, SLOT(onCenterKeyframeRemoved(double,ViewSpec,int,int)) );
    QObject::connect( handler.get(), SIGNAL(keyFrameMoved(ViewSpec,int,double,double)), this, SLOT(onCenterKeyframeMoved(ViewSpec,int,double,double)) );
    QObject::connect( handler.get(), SIGNAL(multipleKeyFramesSet(std::list<double>,ViewSpec,int,int)), this,
                      SLOT(onCenterKeyframesSet(std::list<double>,ViewSpec,int,int)) );
    QObject::connect( handler.get(), SIGNAL(multipleKeyFramesRemoved(std::list<double>,ViewSpec,int,int)), this,
                      SLOT(onCenterMultipleKeysRemoved(std::list<double>,ViewSpec,int,int)) );
    QObject::connect( handler.get(), SIGNAL(animationRemoved(ViewSpec,int)), this, SLOT(onCenterAnimationRemoved(ViewSpec,int)) );
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onCenterKnobValueChanged(ViewSpec,int,int)) );

    handler = _imp->offset.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onOffsetKnobValueChanged(ViewSpec,int,int)) );

    handler = _imp->error.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onErrorKnobValueChanged(ViewSpec,int,int)) );

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    handler = _imp->weight.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onWeightKnobValueChanged(ViewSpec,int,int)) );
#endif

    handler = _imp->motionModel.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onMotionModelKnobValueChanged(ViewSpec,int,int)) );

    /*handler = _imp->patternBtmLeft->getSignalSlotHandler();
       QObject::connect(handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onPatternBtmLeftKnobValueChanged(int,int)));

       handler = _imp->patternTopLeft->getSignalSlotHandler();
       QObject::connect(handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onPatternTopLeftKnobValueChanged(int,int)));

       handler = _imp->patternTopRight->getSignalSlotHandler();
       QObject::connect(handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onPatternTopRightKnobValueChanged(int,int)));

       handler = _imp->patternBtmRight->getSignalSlotHandler();
       QObject::connect(handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onPatternBtmRightKnobValueChanged(int,int)));
     */

    handler = _imp->searchWindowBtmLeft.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onSearchBtmLeftKnobValueChanged(ViewSpec,int,int)) );

    handler = _imp->searchWindowTopRight.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onSearchTopRightKnobValueChanged(ViewSpec,int,int)) );

    handler = _imp->enabled.lock()->getSignalSlotHandler();
    QObject::connect( handler.get(), SIGNAL(valueChanged(ViewSpec,int,int)), this, SLOT(onEnabledValueChanged(ViewSpec,int,int)) );
} // TrackMarker::initializeKnobs

void
TrackMarker::clone(const TrackMarker& other)
{
    TrackMarkerPtr thisShared = shared_from_this();
    TrackerContextPtr context = getContext();

    context->s_trackAboutToClone(thisShared);

    {
        QMutexLocker k(&_imp->trackMutex);
        _imp->trackLabel = other._imp->trackLabel;
        _imp->trackScriptName = other._imp->trackScriptName;
        _imp->userKeyframes = other._imp->userKeyframes;
        _imp->enabled = other._imp->enabled;

        const KnobsVec& knobs = getKnobs();
        KnobsVec::const_iterator itOther = other.getKnobs().begin();
        for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it, ++itOther) {
            (*it)->clone(*itOther);
        }
    }

    context->s_trackCloned(thisShared);
}

void
TrackMarker::load(const TrackSerialization& serialization)
{
    QMutexLocker k(&_imp->trackMutex);

    _imp->trackLabel = serialization._label;
    _imp->trackScriptName = serialization._scriptName;
    const KnobsVec& knobs = getKnobs();

    for (std::list<KnobSerializationPtr>::const_iterator it = serialization._knobs.begin(); it != serialization._knobs.end(); ++it) {
        for (KnobsVec::const_iterator it2 = knobs.begin(); it2 != knobs.end(); ++it2) {
            if ( (*it2)->getName() == (*it)->getName() ) {
                (*it2)->blockValueChanges();
                (*it2)->clone( (*it)->getKnob() );
                (*it2)->unblockValueChanges();
                break;
            }
        }
    }
    for (std::list<int>::const_iterator it = serialization._userKeys.begin(); it != serialization._userKeys.end(); ++it) {
        _imp->userKeyframes.insert(*it);
    }
}

void
TrackMarker::save(TrackSerialization* serialization) const
{
    QMutexLocker k(&_imp->trackMutex);

    serialization->_isPM = (dynamic_cast<const TrackMarkerPM*>(this) != 0);
    serialization->_label = _imp->trackLabel;
    serialization->_scriptName = _imp->trackScriptName;
    KnobsVec knobs = getKnobs_mt_safe();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        KnobSerializationPtr s = boost::make_shared<KnobSerialization>(*it);
        serialization->_knobs.push_back(s);
    }
    for (std::set<int>::const_iterator it = _imp->userKeyframes.begin(); it != _imp->userKeyframes.end(); ++it) {
        serialization->_userKeys.push_back(*it);
    }
}

TrackerContextPtr
TrackMarker::getContext() const
{
    return _imp->context.lock();
}

bool
TrackMarker::setScriptName(const std::string& name)
{
    ///called on the main-thread only
    assert( QThread::currentThread() == qApp->thread() );

    if ( name.empty() ) {
        return false;
    }

    std::string currentName;
    {
        QMutexLocker l(&_imp->trackMutex);
        currentName = _imp->trackScriptName;
    }


    std::string cpy = NATRON_PYTHON_NAMESPACE::makeNameScriptFriendly(name);

    if ( cpy.empty() || cpy == currentName) {
        return false;
    }

    TrackMarkerPtr existingItem = getContext()->getMarkerByName(name);
    if ( existingItem && (existingItem.get() != this) ) {
        return false;
    }

    TrackMarkerPtr thisShared = shared_from_this();
    if (!currentName.empty()) {
        getContext()->removeItemAsPythonField(thisShared);
    }

    {
        QMutexLocker l(&_imp->trackMutex);
        _imp->trackScriptName = cpy;
    }

    getContext()->declareItemAsPythonField(thisShared);

    return true;
}

std::string
TrackMarker::getScriptName_mt_safe() const
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

KnobDoublePtr
TrackMarker::getSearchWindowBottomLeftKnob() const
{
    return _imp->searchWindowBtmLeft.lock();
}

KnobDoublePtr
TrackMarker::getSearchWindowTopRightKnob() const
{
    return _imp->searchWindowTopRight.lock();
}

KnobDoublePtr
TrackMarker::getPatternTopLeftKnob() const
{
    return _imp->patternTopLeft.lock();
}

KnobDoublePtr
TrackMarker::getPatternTopRightKnob() const
{
    return _imp->patternTopRight.lock();
}

KnobDoublePtr
TrackMarker::getPatternBtmRightKnob() const
{
    return _imp->patternBtmRight.lock();
}

KnobDoublePtr
TrackMarker::getPatternBtmLeftKnob() const
{
    return _imp->patternBtmLeft.lock();
}

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
KnobDoublePtr
TrackMarker::getWeightKnob() const
{
    return _imp->weight.lock();
}

#endif

KnobDoublePtr
TrackMarker::getCenterKnob() const
{
    return _imp->center.lock();
}

KnobDoublePtr
TrackMarker::getOffsetKnob() const
{
    return _imp->offset.lock();
}

KnobDoublePtr
TrackMarker::getErrorKnob() const
{
    return _imp->error.lock();
}

KnobChoicePtr
TrackMarker::getMotionModelKnob() const
{
    return _imp->motionModel.lock();
}

KnobBoolPtr
TrackMarker::getEnabledKnob() const
{
    return _imp->enabled.lock();
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
    CurvePtr curve = _imp->center.lock()->getCurve(ViewSpec(0), 0);

    assert(curve);
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
        keyframes->insert( it->getTime() );
    }
}

bool
TrackMarker::isEnabled(double time) const
{
    return _imp->enabled.lock()->getValueAtTime(time, 0, ViewSpec::current(), true, true /*byPassMaster*/);
}

void
TrackMarker::setEnabledAtTime(double time,
                              bool enabled)
{
    KnobBoolPtr knob = _imp->enabled.lock();

    if (!knob) {
        return;
    }
    knob->setValueAtTime(time, enabled, ViewSpec::all(), 0);
}

AnimationLevelEnum
TrackMarker::getEnabledNessAnimationLevel() const
{
    return _imp->enabled.lock()->getAnimationLevel(0);
}

void
TrackMarker::setMotionModelFromGui(int index)
{
    KnobChoicePtr knob = _imp->motionModel.lock();

    if (!knob) {
        return;
    }


    KeyFrame k;
    std::pair<int, KnobIPtr> master = knob->getMaster(0);
    if (master.second) {
        knob->unSlave(0, true);
    }
    knob->onValueChanged(index, ViewSpec::all(), 0, eValueChangedReasonNatronGuiEdited, &k);
    if (master.second) {
        master.second->cloneAndUpdateGui( knob.get() );
        knob->slaveTo(0, master.second, master.first);
    }
}

void
TrackMarker::setEnabledFromGui(double /*time*/,
                               bool enabled)
{
    KnobBoolPtr knob = _imp->enabled.lock();

    if (!knob) {
        return;
    }


    KeyFrame k;
    std::pair<int, KnobIPtr> master = knob->getMaster(0);
    if (master.second) {
        knob->unSlave(0, true);
    }
    knob->onValueChanged(enabled, ViewSpec::all(), 0, eValueChangedReasonNatronGuiEdited, &k);
    if (master.second) {
        master.second->cloneAndUpdateGui( knob.get() );
        knob->slaveTo(0, master.second, master.first);
    }

    getContext()->solveTransformParamsIfAutomatic();
}

void
TrackMarker::onEnabledValueChanged(ViewSpec,
                                   int /*dimension*/,
                                   int reason)
{
    Q_EMIT enabledChanged(reason);
}

int
TrackMarker::getReferenceFrame(int time,
                               int frameStep) const
{
    QMutexLocker k(&_imp->trackMutex);
    std::set<int>::iterator upper = _imp->userKeyframes.upper_bound(time);

    if ( upper == _imp->userKeyframes.end() ) {
        //all keys are lower than time, pick the last one
        if ( !_imp->userKeyframes.empty() ) {
            return *_imp->userKeyframes.rbegin();
        }

        // no keyframe - use the previous/next as reference
        return time - frameStep;
    } else {
        if ( upper == _imp->userKeyframes.begin() ) {
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
    TrackerContextPtr context = getContext();

    assert(context);
    NodePtr input = context->getNode()->getInput(0);
    if (input) {
        SequenceTime time = input->getApp()->getTimeLine()->currentFrame();
        RenderScale scale;
        scale.x = scale.y = 1;
        RectD rod;
        bool isProjectFormat;
        StatusEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(input->getHashValue(), time, scale, ViewIdx(0), &rod, &isProjectFormat);
        Point center;
        center.x = 0;
        center.y = 0;
        if (stat == eStatusOK) {
            center.x = (rod.x1 + rod.x2) / 2.;
            center.y = (rod.y1 + rod.y2) / 2.;
        }

        KnobDoublePtr centerKnob = getCenterKnob();
        centerKnob->setValue(center.x, ViewSpec::current(), 0);
        centerKnob->setValue(center.y, ViewSpec::current(), 1);
    }
}

enum DeleteKnobAnimationEnum
{
    eDeleteKnobAnimationAll,
    eDeleteKnobAnimationBeforeTime,
    eDeleteKnobAnimationAfterTime
};

static void
deleteKnobAnimation(const std::set<int>& userKeyframes,
                    const KnobIPtr& knob,
                    DeleteKnobAnimationEnum type,
                    int currentTime)
{
    for (int i = 0; i < knob->getDimension(); ++i) {
        CurvePtr curve = knob->getCurve(ViewSpec(0), i);
        assert(curve);
        KeyFrameSet keys = curve->getKeyFrames_mt_safe();
        std::list<double> toRemove;
        switch (type) {
        case eDeleteKnobAnimationAll: {
            for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
                std::set<int>::iterator found = userKeyframes.find( (int)it->getTime() );
                if ( found == userKeyframes.end() ) {
                    toRemove.push_back( it->getTime() );
                }
            }
            break;
        }
        case eDeleteKnobAnimationBeforeTime: {
            for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
                if (it->getTime() >= currentTime) {
                    break;
                }
                std::set<int>::iterator found = userKeyframes.find( (int)it->getTime() );
                if ( found == userKeyframes.end() ) {
                    toRemove.push_back( it->getTime() );
                }
            }
            break;
        }
        case eDeleteKnobAnimationAfterTime: {
            for (KeyFrameSet::reverse_iterator it = keys.rbegin(); it != keys.rend(); ++it) {
                if (it->getTime() <= currentTime) {
                    break;
                }
                std::set<int>::iterator found = userKeyframes.find( (int)it->getTime() );
                if ( found == userKeyframes.end() ) {
                    toRemove.push_back( it->getTime() );
                }
            }
            break;
        }
        }
        knob->deleteValuesAtTime(eCurveChangeReasonInternal, toRemove, ViewSpec::all(), i, true);
    }
}

void
TrackMarker::clearAnimation()
{
    std::set<int> userKeyframes;

    getUserKeyframes(&userKeyframes);

    KnobIPtr offsetKnob = getOffsetKnob();
    assert(offsetKnob);
    deleteKnobAnimation(userKeyframes, offsetKnob, eDeleteKnobAnimationAll, 0);

    KnobIPtr centerKnob = getCenterKnob();
    assert(centerKnob);
    deleteKnobAnimation(userKeyframes, centerKnob, eDeleteKnobAnimationAll, 0);

    KnobIPtr errorKnob = getErrorKnob();
    assert(errorKnob);
    deleteKnobAnimation(userKeyframes, errorKnob, eDeleteKnobAnimationAll, 0);
}

void
TrackMarker::clearAnimationBeforeTime(int time)
{
    std::set<int> userKeyframes;

    getUserKeyframes(&userKeyframes);

    KnobIPtr offsetKnob = getOffsetKnob();
    assert(offsetKnob);
    deleteKnobAnimation(userKeyframes, offsetKnob, eDeleteKnobAnimationBeforeTime, time);

    KnobIPtr centerKnob = getCenterKnob();
    assert(centerKnob);
    deleteKnobAnimation(userKeyframes, centerKnob, eDeleteKnobAnimationBeforeTime, time);

    KnobIPtr errorKnob = getErrorKnob();
    assert(errorKnob);
    deleteKnobAnimation(userKeyframes, errorKnob, eDeleteKnobAnimationBeforeTime, time);
}

void
TrackMarker::clearAnimationAfterTime(int time)
{
    std::set<int> userKeyframes;

    getUserKeyframes(&userKeyframes);

    KnobIPtr offsetKnob = getOffsetKnob();
    assert(offsetKnob);
    deleteKnobAnimation(userKeyframes, offsetKnob, eDeleteKnobAnimationAfterTime, time);

    KnobIPtr centerKnob = getCenterKnob();
    assert(centerKnob);
    deleteKnobAnimation(userKeyframes, centerKnob, eDeleteKnobAnimationAfterTime, time);

    KnobIPtr errorKnob = getErrorKnob();
    assert(errorKnob);
    deleteKnobAnimation(userKeyframes, errorKnob, eDeleteKnobAnimationAfterTime, time);
}

void
TrackMarker::resetOffset()
{
    KnobDoublePtr knob = getOffsetKnob();

    for (int i = 0; i < knob->getDimension(); ++i) {
        knob->resetToDefaultValue(i);
    }
}

void
TrackMarker::resetTrack()
{
    Point curCenter;
    KnobDoublePtr knob = getCenterKnob();

    curCenter.x = knob->getValue(0);
    curCenter.y = knob->getValue(1);

    EffectInstancePtr effect = getContext()->getNode()->getEffectInstance();
    effect->beginChanges();

    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (*it != knob) {
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                (*it)->resetToDefaultValue(i);
            }
        } else {
            for (int i = 0; i < (*it)->getDimension(); ++i) {
                (*it)->removeAnimation(ViewSpec::current(), i);
            }
            knob->setValue(curCenter.x, ViewSpec::current(), 0);
            knob->setValue(curCenter.y, ViewSpec::current(), 1);
        }
    }
    effect->endChanges();
    removeAllUserKeyframes();
}

void
TrackMarker::removeAllUserKeyframes()
{
    {
        QMutexLocker k(&_imp->trackMutex);
        _imp->userKeyframes.clear();
    }
    getContext()->s_allKeyframesRemovedOnTrack( shared_from_this() );
}

void
TrackMarker::setKeyFrameOnCenterAndPatternAtTime(int time)
{
    KnobDoublePtr center = _imp->center.lock();

    for (int i = 0; i < center->getDimension(); ++i) {
        double v = center->getValueAtTime(time, i);
        center->setValueAtTime(time, v, ViewSpec::all(), i);
    }

    KnobDoublePtr patternCorners[4] = {_imp->patternBtmLeft.lock(), _imp->patternTopLeft.lock(), _imp->patternTopRight.lock(), _imp->patternBtmRight.lock()};
    for (int c = 0; c < 4; ++c) {
        KnobDoublePtr k = patternCorners[c];
        for (int i = 0; i < k->getDimension(); ++i) {
            double v = k->getValueAtTime(time, i);
            k->setValueAtTime(time, v, ViewSpec::all(), i);
        }
    }
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
        if ( found != _imp->userKeyframes.end() ) {
            _imp->userKeyframes.erase(found);
            emitSignal = true;
        }
    }

    if (emitSignal) {
        getContext()->s_keyframeRemovedOnTrack(shared_from_this(), time);
    }
}

void
TrackMarker::onCenterKeyframeSet(double time,
                                 ViewSpec /*view*/,
                                 int /*dimension*/,
                                 int /*reason*/,
                                 bool added)
{
    if (!_imp->trackingStartedCount) {
        if (added) {
            getContext()->s_keyframeSetOnTrackCenter(shared_from_this(), time);
        }
    } else {
        if (added) {
            _imp->keyframesAddedWhileTracking.push_back(time);
        }
    }
}

void
TrackMarker::onCenterKeyframeRemoved(double time,
                                     ViewSpec /*view*/,
                                     int /*dimension*/,
                                     int /*reason*/)
{
    getContext()->s_keyframeRemovedOnTrackCenter(shared_from_this(), time);
}

void
TrackMarker::onCenterMultipleKeysRemoved(const std::list<double>& times,
                                         ViewSpec /*view*/,
                                         int /*dimension*/,
                                         int /*reason*/)
{
    getContext()->s_multipleKeyframesRemovedOnTrackCenter(shared_from_this(), times);
}

void
TrackMarker::onCenterKeyframeMoved(ViewSpec /*view*/,
                                   int /*dimension*/,
                                   double oldTime,
                                   double newTime)
{
    getContext()->s_keyframeRemovedOnTrackCenter(shared_from_this(), oldTime);
    getContext()->s_keyframeSetOnTrackCenter(shared_from_this(), newTime);
}

void
TrackMarker::onCenterKeyframesSet(const std::list<double>& keys,
                                  ViewSpec /*view*/,
                                  int /*dimension*/,
                                  int /*reason*/)
{
    getContext()->s_multipleKeyframesSetOnTrackCenter(shared_from_this(), keys);
}

void
TrackMarker::onCenterAnimationRemoved(ViewSpec /*view*/,
                                      int /*dimension*/)
{
    getContext()->s_allKeyframesRemovedOnTrackCenter( shared_from_this() );
}

void
TrackMarker::onCenterKnobValueChanged(ViewSpec /*view*/,
                                      int dimension,
                                      int reason)
{
    if (!_imp->trackingStartedCount) {
        getContext()->s_centerKnobValueChanged(shared_from_this(), dimension, reason);
    }
}

void
TrackMarker::onOffsetKnobValueChanged(ViewSpec /*view*/,
                                      int dimension,
                                      int reason)
{
    getContext()->s_offsetKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onErrorKnobValueChanged(ViewSpec /*view*/,
                                     int dimension,
                                     int reason)
{
    if (!_imp->trackingStartedCount) {
        getContext()->s_errorKnobValueChanged(shared_from_this(), dimension, reason);
    }
}

void
TrackMarker::onWeightKnobValueChanged(ViewSpec /*view*/,
                                      int dimension,
                                      int reason)
{
    getContext()->s_weightKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onMotionModelKnobValueChanged(ViewSpec /*view*/,
                                           int dimension,
                                           int reason)
{
    getContext()->s_motionModelKnobValueChanged(shared_from_this(), dimension, reason);
}


void
TrackMarker::onSearchBtmLeftKnobValueChanged(ViewSpec /*view*/,
                                             int dimension,
                                             int reason)
{
    getContext()->s_searchBtmLeftKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::onSearchTopRightKnobValueChanged(ViewSpec /*view*/,
                                              int dimension,
                                              int reason)
{
    getContext()->s_searchTopRightKnobValueChanged(shared_from_this(), dimension, reason);
}

void
TrackMarker::notifyTrackingStarted()
{
    ++_imp->trackingStartedCount;
    _imp->keyframesAddedWhileTracking.clear();
}

void
TrackMarker::notifyTrackingEnded()
{
    if (_imp->trackingStartedCount) {
        --_imp->trackingStartedCount;
    }

    // Refresh knobs once finished
    if (!_imp->trackingStartedCount) {
        TrackMarkerPtr marker = shared_from_this();
        getContext()->s_errorKnobValueChanged(marker, 0, eValueChangedReasonNatronInternalEdited);
        for (int i = 0; i < 2; ++i) {
            getContext()->s_centerKnobValueChanged(marker, i, eValueChangedReasonNatronInternalEdited);
        }
        if (!_imp->keyframesAddedWhileTracking.empty()) {
            getContext()->s_multipleKeyframesSetOnTrackCenter(marker, _imp->keyframesAddedWhileTracking);
            _imp->keyframesAddedWhileTracking.clear();
        }
    }
}


RectI
TrackMarker::getMarkerImageRoI(int time) const
{
    const unsigned int mipmapLevel = 0;
    Point center, offset;
    KnobDoublePtr centerKnob = getCenterKnob();
    KnobDoublePtr offsetKnob = getOffsetKnob();

    center.x = centerKnob->getValueAtTime(time, 0);
    center.y = centerKnob->getValueAtTime(time, 1);

    offset.x = offsetKnob->getValueAtTime(time, 0);
    offset.y = offsetKnob->getValueAtTime(time, 1);

    RectD roiCanonical;
    KnobDoublePtr swBl = getSearchWindowBottomLeftKnob();
    KnobDoublePtr swTr = getSearchWindowTopRightKnob();

    roiCanonical.x1 = swBl->getValueAtTime(time, 0) + center.x + offset.x;
    roiCanonical.y1 = swBl->getValueAtTime(time, 1) + center.y + offset.y;
    roiCanonical.x2 = swTr->getValueAtTime(time, 0) + center.x + offset.x;
    roiCanonical.y2 = swTr->getValueAtTime(time, 1) + center.y + offset.y;

    RectI roi;
    NodePtr node = getContext()->getNode();
    NodePtr input = node->getInput(0);
    if (!input) {
        return RectI();
    }
    roiCanonical.toPixelEnclosing(mipmapLevel, input ? input->getEffectInstance()->getAspectRatio(-1) : 1., &roi);

    return roi;
}

std::pair<ImagePtr, RectI>
TrackMarker::getMarkerImage(int time,
                            const RectI& roi) const
{
    std::list<ImagePlaneDesc> components;

    components.push_back( ImagePlaneDesc::getRGBComponents() );

    const unsigned int mipmapLevel = 0;
    assert( !roi.isNull() );

    NodePtr node = getContext()->getNode();
    NodePtr input = node->getInput(0);
    if (!input) {
        return std::make_pair(ImagePtr(), roi);
    }

    AbortableRenderInfoPtr abortInfo = AbortableRenderInfo::create(false, 0);
    const bool isRenderUserInteraction = true;
    const bool isSequentialRender = false;
    AbortableThread* isAbortable = dynamic_cast<AbortableThread*>( QThread::currentThread() );
    if (isAbortable) {
        isAbortable->setAbortInfo( isRenderUserInteraction, abortInfo, node->getEffectInstance() );
    }

    ParallelRenderArgsSetter frameRenderArgs( time,
                                              ViewIdx(0), //<  view 0 (left)
                                              isRenderUserInteraction, //<isRenderUserInteraction
                                              isSequentialRender, //isSequential
                                              abortInfo, //abort info
                                              getContext()->getNode(), // viewer requester
                                              0, //texture index
                                              node->getApp()->getTimeLine().get(),
                                              NodePtr(),
                                              true, //isAnalysis
                                              false, //draftMode
                                              RenderStatsPtr() );
    RenderScale scale;
    scale.x = scale.y = 1.;
    EffectInstance::RenderRoIArgs args( time,
                                        scale,
                                        mipmapLevel, //mipmaplevel
                                        ViewIdx(0),
                                        false,
                                        roi,
                                        RectD(),
                                        components,
                                        eImageBitDepthFloat,
                                        false,
                                        node->getEffectInstance().get(),
                                        eStorageModeRAM /*returnOpenGlTex*/,
                                        time);
    std::map<ImagePlaneDesc, ImagePtr> planes;
    EffectInstance::RenderRoIRetCode stat = input->getEffectInstance()->renderRoI(args, &planes);

    appPTR->getAppTLS()->cleanupTLSForThread();

    if ( (stat != EffectInstance::eRenderRoIRetCodeOk) || planes.empty() ) {
        return std::make_pair(ImagePtr(), roi);
    }

    return std::make_pair(planes.begin()->second, roi);
} // TrackMarker::getMarkerImage

void
TrackMarker::onKnobSlaved(const KnobIPtr& slave,
                          const KnobIPtr& master,
                          int dimension,
                          bool isSlave)
{
    EffectInstancePtr effect = getContext()->getNode()->getEffectInstance();

    if (effect) {
        effect->onKnobSlaved(slave, master, dimension, isSlave);
    }
}

TrackMarkerPM::TrackMarkerPM(const TrackerContextPtr& context)
    : TrackMarker(context)
{
}


// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TrackMarkerPM::MakeSharedEnabler: public TrackMarkerPM
{
    MakeSharedEnabler(const TrackerContextPtr& context) : TrackMarkerPM(context) {
    }
};

TrackMarkerPtr
TrackMarkerPM::create(const TrackerContextPtr& context)
{
    return boost::make_shared<TrackMarkerPM::MakeSharedEnabler>(context);
}


TrackMarkerPM::~TrackMarkerPM()
{
}

void
TrackMarkerPM::onTrackerNodeInputChanged(int /*inputNb*/)
{
    NodePtr thisNode = getContext()->getNode();

    if (thisNode) {
        NodePtr inputNode = thisNode->getInput(0);
        if (inputNode) {
            trackerNode->connectInput(inputNode, 0);
        }
    }
}

bool
TrackMarkerPM::trackMarker(bool forward,
                           int refFrame,
                           int frame)
{
    KnobButtonPtr button;

    if (forward) {
        button = trackNextButton.lock();
    } else {
        button = trackPrevButton.lock();
    }
    KnobIntPtr refFrameK = refFrameKnob.lock();
    refFrameK->setValue(refFrame);

    // Unslave the center knob since the trackerNode will update it, then update the marker center
    KnobDoublePtr center = centerKnob.lock();
    for (int i = 0; i < center->getDimension(); ++i) {
        center->unSlave(i, true);
    }

    trackerNode->getEffectInstance()->onKnobValueChanged_public(button.get(), eValueChangedReasonNatronInternalEdited, frame, ViewIdx(0),
                                                                true);

    KnobDoublePtr markerCenter = getCenterKnob();
    // The TrackerPM plug-in has set a keyframe at the refFrame and frame, copy them
    bool ret = true;
    double centerPoint[2];
    for (int i = 0; i < center->getDimension(); ++i) {
        {
            int index = center->getKeyFrameIndex(ViewSpec::current(), i, frame);
            if (index != -1) {
                centerPoint[i] = center->getValueAtTime(frame, i);
                markerCenter->setValueAtTime(frame, centerPoint[i], ViewSpec::current(), i);
            } else {
                // No keyframe at this time: tracking failed
                ret = false;
                break;
            }
        }
        {
            int index = center->getKeyFrameIndex(ViewSpec::current(), i, refFrame);
            if (index != -1) {
                double value = center->getValueAtTime(refFrame, i);
                markerCenter->setValueAtTime(refFrame, value, ViewSpec::current(), i);
            }
        }
    }

    // Convert the correlation score of the TrackerPM to the error
    if (ret) {
        KnobDoublePtr markerError = getErrorKnob();
        KnobDoublePtr correlation = correlationScoreKnob.lock();
        {
            int index = correlation->getKeyFrameIndex(ViewSpec::current(), 0, frame);
            if (index != -1) {
                // The error is estimated as a percentage of the correlation across the number of pixels in the pattern window
                KnobDoublePtr  pBtmLeft = patternBtmLeftKnob.lock();
                KnobDoublePtr  pTopRight = patternTopRightKnob.lock();
                Point btmLeft, topRight;

                btmLeft.x = pBtmLeft->getValueAtTime(frame, 0);
                btmLeft.y = pBtmLeft->getValueAtTime(frame, 1);

                topRight.x = pTopRight->getValueAtTime(frame, 0);
                topRight.y = pTopRight->getValueAtTime(frame, 1);


                double areaPixels = (topRight.x - btmLeft.x) * (topRight.y - btmLeft.y);
                NodePtr trackerInput = trackerNode->getInput(0);
                if (trackerInput) {
                    ImagePlaneDesc comps, paireComps;
                    trackerInput->getEffectInstance()->getMetadataComponents(-1, &comps, &paireComps);
                    areaPixels *= comps.getNumComponents();
                }

                double value = correlation->getValueAtTime(frame, 0);

                // Convert to a percentage
                value /= areaPixels;

                markerError->setValueAtTime(frame, value, ViewSpec::current(), 0);
            }
        }
    }

    for (int i = 0; i < center->getDimension(); ++i) {
        center->slaveTo(i, markerCenter, i);
    }

    return ret;
} // TrackMarkerPM::trackMarker

template <typename T>
boost::shared_ptr<T>
getNodeKnob(const NodePtr& node,
            const std::string& scriptName)
{
    KnobIPtr knob = node->getKnobByName(scriptName);

    assert(knob);
    if (!knob) {
        return boost::shared_ptr<T>();
    }
    boost::shared_ptr<T> ret = boost::dynamic_pointer_cast<T>(knob);
    assert(ret);

    return ret;
}

void
TrackMarkerPM::initializeKnobs()
{
    TrackMarker::initializeKnobs();
    NodePtr thisNode = getContext()->getNode();
    QObject::connect( getContext().get(), SIGNAL(onNodeInputChanged(int)), this, SLOT(onTrackerNodeInputChanged(int)) );
    NodePtr node;
    {
        CreateNodeArgs args( PLUGINID_OFX_TRACKERPM, NodeCollectionPtr() );
        args.setProperty<bool>(kCreateNodeArgsPropOutOfProject, true);
        args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "TrackerPMNode");

        node = getContext()->getNode()->getApp()->createNode(args);
        if (!node) {
            throw std::runtime_error("Couldn't create plug-in " PLUGINID_OFX_TRACKERPM);
        }
        if (thisNode) {
            NodePtr inputNode = thisNode->getInput(0);
            if (inputNode) {
                node->connectInput(inputNode, 0);
            }
        }
        trackerNode = node;
    }

    trackPrevButton = getNodeKnob<KnobButton>(node, kTrackerPMParamTrackingPrevious);
    trackNextButton = getNodeKnob<KnobButton>(node, kTrackerPMParamTrackingNext);
    KnobDoublePtr center = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingCenterPoint);
    centerKnob = center;
    // Slave the center knob and unslave when tracking
    for (int i = 0; i < center->getDimension(); ++i) {
        center->slaveTo(i, getCenterKnob(), i);
    }

    KnobDoublePtr offset = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingOffset);

    // Slave the offset knob
    for (int i = 0; i < offset->getDimension(); ++i) {
        offset->slaveTo(i, getOffsetKnob(), i);
    }
    offsetKnob = offset;

    // Ref frame is set for each
    refFrameKnob = getNodeKnob<KnobInt>(node, kTrackerPMParamTrackingReferenceFrame);

    // Enable reference frame
    KnobBoolPtr enableRefFrameKnob = getNodeKnob<KnobBool>(node, kTrackerPMParamTrackingEnableReferenceFrame);
    enableRefFrameKnob->setValue(true);

    KnobChoicePtr scoreType = getNodeKnob<KnobChoice>(node, kTrackerPMParamScore);
    scoreType->slaveTo(0, getContext()->getCorrelationScoreTypeKnob(), 0);
    scoreTypeKnob = scoreType;

    KnobDoublePtr correlationScore = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingCorrelationScore);
    correlationScoreKnob = correlationScore;

    KnobDoublePtr patternBtmLeft = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingPatternBoxBtmLeft);
    patternBtmLeftKnob = patternBtmLeft;

    // Slave the search window and pattern of the node to the parameters of the marker
    for (int i = 0; i < patternBtmLeft->getDimension(); ++i) {
        patternBtmLeft->slaveTo(i, getPatternBtmLeftKnob(), i);
    }
    KnobDoublePtr patternTopRight = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingPatternBoxTopRight);
    patternTopRightKnob = patternTopRight;
    for (int i = 0; i < patternTopRight->getDimension(); ++i) {
        patternTopRight->slaveTo(i, getPatternTopRightKnob(), i);
    }

    KnobDoublePtr searchWindowBtmLeft = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingSearchBoxBtmLeft);
    searchWindowBtmLeftKnob = searchWindowBtmLeft;
    for (int i = 0; i < searchWindowBtmLeft->getDimension(); ++i) {
        searchWindowBtmLeft->slaveTo(i, getSearchWindowBottomLeftKnob(), i);
    }

    KnobDoublePtr searchWindowTopRight = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingSearchBoxTopRight);
    searchWindowTopRightKnob = searchWindowTopRight;
    for (int i = 0; i < searchWindowTopRight->getDimension(); ++i) {
        searchWindowTopRight->slaveTo(i, getSearchWindowTopRightKnob(), i);
    }
} // TrackMarkerPM::initializeKnobs


NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TrackMarker.cpp"

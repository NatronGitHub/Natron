/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#include "Engine/Curve.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/AppManager.h"
#include "Engine/AppInstance.h"
#include "Engine/KnobTypes.h"
#include "Engine/FrameViewRequest.h"
#include "Engine/EffectInstance.h"
#include "Engine/Image.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/TrackerNode.h"
#include "Engine/TrackerNodePrivate.h"
#include "Engine/TimeLine.h"
#include "Engine/TreeRender.h"
#include "Engine/TLSHolder.h"

#include "Serialization/KnobTableItemSerialization.h"


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
    TrackMarker* _publicInterface; // can not be a smart ptr

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
    KnobBoolWPtr enabled;

    KnobKeyFrameMarkersWPtr shapeKeys;


    // Only used by the TrackScheduler thread
    int trackingStartedCount;

    TrackMarkerPrivate(TrackMarker* publicInterface)
        : _publicInterface(publicInterface)
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
        , enabled()
        , shapeKeys()
        , trackingStartedCount(0)
    {
    }
};

TrackMarker::TrackMarker(const KnobItemsTablePtr& model)
    : KnobTableItem(model)
    , _imp( new TrackMarkerPrivate(this) )
{
}

TrackMarker::~TrackMarker()
{
}

std::string
TrackMarker::getBaseItemName() const
{
    return KnobTableItem::tr("Track").toStdString();
}

std::string
TrackMarker::getSerializationClassName() const
{
    return kSerializationTrackTag;
}

void
TrackMarker::initializeKnobs()
{
    KnobItemsTablePtr model = getModel();
    EffectInstancePtr effect;
    if (model) {
        effect = model->getNode()->getEffectInstance();
    }
    KnobIntPtr defPatternSizeKnob, defSearchSizeKnob;
    KnobChoicePtr defMotionModelKnob;
    if (effect) {
        defPatternSizeKnob = toKnobInt(effect->getKnobByName(kTrackerUIParamDefaultMarkerPatternWinSize));
        defSearchSizeKnob = toKnobInt(effect->getKnobByName(kTrackerUIParamDefaultMarkerSearchWinSize));
        defMotionModelKnob = toKnobChoice(effect->getKnobByName(kTrackerUIParamDefaultMotionModel));
    }

    double patternHalfSize = defPatternSizeKnob ? defPatternSizeKnob->getValue() / 2. : 21;
    double searchHalfSize = defSearchSizeKnob ? defSearchSizeKnob->getValue() / 2. : 71;

    int defMotionModel_i = defMotionModelKnob ? defMotionModelKnob->getValue() : 0;

    KnobDoublePtr swbbtmLeft = createKnob<KnobDouble>(kTrackerParamSearchWndBtmLeft, 2);

    swbbtmLeft->setLabel(tr(kTrackerParamSearchWndBtmLeftLabel));
    swbbtmLeft->setDefaultValue(-searchHalfSize, DimIdx(0));
    swbbtmLeft->setDefaultValue(-searchHalfSize, DimIdx(1));
    swbbtmLeft->setHintToolTip( tr(kTrackerParamSearchWndBtmLeftHint) );
    _imp->searchWindowBtmLeft = swbbtmLeft;

    KnobDoublePtr swbtRight = createKnob<KnobDouble>(kTrackerParamSearchWndTopRight, 2);
    swbtRight->setLabel(tr(kTrackerParamSearchWndTopRightLabel));
    swbtRight->setDefaultValue(searchHalfSize, DimIdx(0));
    swbtRight->setDefaultValue(searchHalfSize, DimIdx(1));
    swbtRight->setHintToolTip( tr(kTrackerParamSearchWndTopRightHint) );
    _imp->searchWindowTopRight = swbtRight;


    KnobDoublePtr ptLeft = createKnob<KnobDouble>(kTrackerParamPatternTopLeft, 2);
    ptLeft->setLabel(tr(kTrackerParamPatternTopLeftLabel));
    ptLeft->setDefaultValue(-patternHalfSize, DimIdx(0));
    ptLeft->setDefaultValue(patternHalfSize, DimIdx(1));
    ptLeft->setHintToolTip( tr(kTrackerParamPatternTopLeftHint) );
    _imp->patternTopLeft = ptLeft;

    KnobDoublePtr ptRight = createKnob<KnobDouble>(kTrackerParamPatternTopRight, 2);
    ptRight->setLabel(tr(kTrackerParamPatternTopRightLabel));
    ptRight->setDefaultValue(patternHalfSize, DimIdx(0));
    ptRight->setDefaultValue(patternHalfSize, DimIdx(1));
    ptRight->setHintToolTip( tr(kTrackerParamPatternTopRightHint) );
    _imp->patternTopRight = ptRight;

    KnobDoublePtr pBRight = createKnob<KnobDouble>(kTrackerParamPatternBtmRight, 2);
    pBRight->setLabel(tr(kTrackerParamPatternBtmRightLabel));
    pBRight->setDefaultValue(patternHalfSize, DimIdx(0));
    pBRight->setDefaultValue(-patternHalfSize, DimIdx(1));
    pBRight->setHintToolTip( tr(kTrackerParamPatternBtmRightHint) );
    _imp->patternBtmRight = pBRight;

    KnobDoublePtr pBLeft = createKnob<KnobDouble>(kTrackerParamPatternBtmLeft, 2);
    pBLeft->setLabel(tr(kTrackerParamPatternBtmLeftLabel));
    pBLeft->setDefaultValue(-patternHalfSize, DimIdx(0));
    pBLeft->setDefaultValue(-patternHalfSize, DimIdx(1));
    pBLeft->setHintToolTip( tr(kTrackerParamPatternBtmLeftHint) );
    _imp->patternBtmLeft = pBLeft;

    KnobDoublePtr centerKnob = createKnob<KnobDouble>(kTrackerParamCenter, 2);
    centerKnob->setLabel(tr(kTrackerParamCenterLabel));
    centerKnob->setHintToolTip( tr(kTrackerParamCenterHint) );
    _imp->center = centerKnob;

    KnobDoublePtr offsetKnob = createKnob<KnobDouble>(kTrackerParamOffset, 2);
    offsetKnob->setLabel(tr(kTrackerParamOffsetLabel));
    offsetKnob->setHintToolTip( tr(kTrackerParamOffsetHint) );
    _imp->offset = offsetKnob;

    {
        KnobKeyFrameMarkersPtr param = createKnob<KnobKeyFrameMarkers>(kTrackerParamManualKeyframes);
        param->setLabel(tr(kTrackerParamManualKeyframesLabel));
        param->setHintToolTip(tr(kTrackerParamManualKeyframesHint));
        param->getAnimationCurve(ViewIdx(0), DimIdx(0))->registerListener(shared_from_this());
        _imp->shapeKeys = param;
    }

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    KnobDoublePtr weightKnob = createKnob<KnobDouble>(kTrackerParamTrackWeight, 1);
    weightKnob->setLabel(tr(kTrackerParamTrackWeightLabel));
    weightKnob->setHintToolTip( tr(kTrackerParamTrackWeightHint) );
    weightKnob->setDefaultValue(1.);
    weightKnob->setAnimationEnabled(false);
    weightKnob->setRange(0., 1.);
    _imp->weight = weightKnob;
#endif

    KnobChoicePtr mmodelKnob = createKnob<KnobChoice>(kTrackerParamMotionModel, 1);
    mmodelKnob->setHintToolTip( tr(kTrackerParamMotionModelHint) );
    mmodelKnob->setLabel(tr(kTrackerParamMotionModelLabel));
    {
        std::vector<ChoiceOption> choices, helps;
        std::map<int, std::string> icons;
        TrackerNodePrivate::getMotionModelsAndHelps(true, &choices, &icons);
        mmodelKnob->populateChoices(choices);
        mmodelKnob->setIcons(icons);
    }

    mmodelKnob->setDefaultValue(defMotionModel_i);
    _imp->motionModel = mmodelKnob;

    KnobDoublePtr errKnob = createKnob<KnobDouble>(kTrackerParamError, 1);
    errKnob->setLabel(tr(kTrackerParamErrorLabel));
    _imp->error = errKnob;

    KnobBoolPtr enableKnob = createKnob<KnobBool>(kTrackerParamEnabled, 1);
    enableKnob->setLabel(tr(kTrackerParamEnabledLabel));
    enableKnob->setHintToolTip( tr(kTrackerParamEnabledHint) );
    enableKnob->setAnimationEnabled(true);
    enableKnob->setDefaultValue(true);
    _imp->enabled = enableKnob;

    addColumn(kKnobTableItemColumnLabel, DimIdx(0));
    addColumn(kTrackerParamEnabled, DimIdx(0));
    addColumn(kTrackerParamMotionModel, DimIdx(0));
    addColumn(kTrackerParamCenter, DimIdx(0));
    addColumn(kTrackerParamCenter, DimIdx(1));
    addColumn(kTrackerParamOffset, DimIdx(0));
    addColumn(kTrackerParamOffset, DimIdx(1));
    addColumn(kTrackerParamError, DimIdx(0));

    KnobTableItem::initializeKnobs();

} // TrackMarker::initializeKnobs

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


void
TrackMarker::getCenterKeyframes(std::set<double>* keyframes) const
{
    CurvePtr curve = _imp->center.lock()->getAnimationCurve(ViewIdx(0), DimIdx(0));

    assert(curve);
    KeyFrameSet keys = curve->getKeyFrames_mt_safe();
    for (KeyFrameSet::iterator it = keys.begin(); it != keys.end(); ++it) {
        keyframes->insert( it->getTime() );
    }
}

bool
TrackMarker::isEnabled(TimeValue time) const
{
    return _imp->enabled.lock()->getValueAtTime(time, DimIdx(0), ViewIdx(0), true);
}

void
TrackMarker::setEnabledAtTime(TimeValue time,
                              bool enabled)
{
    KnobBoolPtr knob = _imp->enabled.lock();

    if (!knob) {
        return;
    }
    knob->setValueAtTime(time, enabled, ViewSetSpec::all(), DimIdx(0));
}

int
TrackMarker::getReferenceFrame(TimeValue time,
                               int frameStep) const
{
    QMutexLocker k(&_imp->trackMutex);

    KeyFrameSet userKeyframes = _imp->shapeKeys.lock()->getAnimationCurve(ViewIdx(0), DimIdx(0))->getKeyFrames_mt_safe();

    KeyFrame tmp(time,0);
    KeyFrameSet::iterator upper = userKeyframes.upper_bound(tmp);

    if ( upper == userKeyframes.end() ) {
        //all keys are lower than time, pick the last one
        if ( !userKeyframes.empty() ) {
            return userKeyframes.rbegin()->getTime();
        }

        // no keyframe - use the previous/next as reference
        return time - frameStep;
    } else {
        if ( upper == userKeyframes.begin() ) {
            ///all keys are greater than time
            return upper->getTime();
        }

        TimeValue upperKeyFrame = upper->getTime();
        ///If we find "time" as a keyframe, then use it
        --upper;

        TimeValue lowerKeyFrame = upper->getTime();

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
    KnobItemsTablePtr model = getModel();
    if (!model) {
        return;
    }
    RectD rod;
    NodePtr input = model->getNode()->getInput(0);
    if (!input) {
        Format f;
        getApp()->getProject()->getProjectDefaultFormat(&f);
        rod = f.toCanonicalFormat();
    } else {
        TimeValue time(input->getApp()->getTimeLine()->currentFrame());
        RenderScale scale(1.);
        RectD rod;
        {
            GetRegionOfDefinitionResultsPtr results;
            ActionRetCodeEnum stat = input->getEffectInstance()->getRegionOfDefinition_public(time, scale, ViewIdx(0), &results);
            if (!isFailureRetCode(stat)) {
                rod = results->getRoD();
            }
        }
        Point center;
        center.x = 0;
        center.y = 0;
        center.x = (rod.x1 + rod.x2) / 2.;
        center.y = (rod.y1 + rod.y2) / 2.;


        KnobDoublePtr centerKnob = getCenterKnob();
        centerKnob->setValue(center.x, ViewSetSpec::all(), DimIdx(0));
        centerKnob->setValue(center.y, ViewSetSpec::all(), DimIdx(1));
    }
}

KeyFrameSet
TrackMarker::getKeyFrames() const
{
    return _imp->shapeKeys.lock()->getAnimationCurve(ViewIdx(0), DimIdx(0))->getKeyFrames_mt_safe();
}

void
TrackMarker::removeKeyFrame(TimeValue time)
{
    _imp->shapeKeys.lock()->deleteValueAtTime(time, ViewSetSpec(0), DimSpec(0), eValueChangedReasonUserEdited);
}

void
TrackMarker::removeAnimation()
{
    _imp->shapeKeys.lock()->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
}

void
TrackMarker::setKeyFrame(TimeValue time)
{
    KeyFrame k(time, 0);
    AnimatingObjectI::SetKeyFrameArgs args;
    args.dimension = DimSpec(0);
    args.view = ViewSetSpec(0);
    _imp->shapeKeys.lock()->setKeyFrame(args, k);
}

bool
TrackMarker::hasKeyFrameAtTime(TimeValue time) const
{
    int found = _imp->shapeKeys.lock()->getKeyFrameIndex(ViewIdx(0), DimIdx(0), time);
    return found != -1;
}

void
TrackMarker::clearAnimation()
{

    KnobIPtr offsetKnob = getOffsetKnob();
    assert(offsetKnob);
    offsetKnob->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    KnobIPtr centerKnob = getCenterKnob();
    assert(centerKnob);
    centerKnob->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    KnobIPtr errorKnob = getErrorKnob();
    assert(errorKnob);
    errorKnob->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
}

static std::set<double> keyframesToDoubleSet(const KeyFrameSet& keys)
{
    std::set<double> ret;
    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        ret.insert(it->getTime());
    }
    return ret;
}


void
TrackMarker::clearAnimationBeforeTime(TimeValue time)
{
    std::set<double> userKeyframes = keyframesToDoubleSet(getKeyFrames());

    KnobIPtr offsetKnob = getOffsetKnob();
    assert(offsetKnob);
    offsetKnob->deleteValuesBeforeTime(userKeyframes, time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    KnobIPtr centerKnob = getCenterKnob();
    assert(centerKnob);
    centerKnob->deleteValuesBeforeTime(userKeyframes, time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    KnobIPtr errorKnob = getErrorKnob();
    assert(errorKnob);
    errorKnob->deleteValuesBeforeTime(userKeyframes, time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
}

void
TrackMarker::clearAnimationAfterTime(TimeValue time)
{
    std::set<double> userKeyframes = keyframesToDoubleSet(getKeyFrames());

    KnobIPtr offsetKnob = getOffsetKnob();
    assert(offsetKnob);
    offsetKnob->deleteValuesAfterTime(userKeyframes, time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    KnobIPtr centerKnob = getCenterKnob();
    assert(centerKnob);
    centerKnob->deleteValuesAfterTime(userKeyframes, time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

    KnobIPtr errorKnob = getErrorKnob();
    assert(errorKnob);
    errorKnob->deleteValuesAfterTime(userKeyframes, time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
}

void
TrackMarker::resetOffset()
{
    KnobDoublePtr knob = getOffsetKnob();
    knob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
}

void
TrackMarker::resetTrack()
{
    Point curCenter;
    KnobDoublePtr centerKnob = getCenterKnob();

    curCenter.x = centerKnob->getValue();
    curCenter.y = centerKnob->getValue(DimIdx(1));

    const KnobsVec& knobs = getKnobs();
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if (*it != centerKnob) {
            (*it)->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
        } else {
            (*it)->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

            std::vector<double> values(2);
            values[0] = curCenter.x;
            values[1] = curCenter.y;
            centerKnob->setValueAcrossDimensions(values);
        }
    }

    _imp->shapeKeys.lock()->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);
}


void
TrackMarker::setKeyFrameOnCenterAndPatternAtTime(TimeValue time)
{
    KnobDoublePtr center = _imp->center.lock();

    {
        std::vector<double> values(2);
        values[0] = center->getValueAtTime(time);
        values[1] = center->getValueAtTime(time, DimIdx(1));
        center->setValueAtTimeAcrossDimensions(time, values);
    }

    KnobDoublePtr patternCorners[4] = {_imp->patternBtmLeft.lock(), _imp->patternTopLeft.lock(), _imp->patternTopRight.lock(), _imp->patternBtmRight.lock()};
    for (int c = 0; c < 4; ++c) {
        KnobDoublePtr k = patternCorners[c];
        std::vector<double> values(2);
        values[0] = k->getValueAtTime(time, DimIdx(0));
        values[1] = k->getValueAtTime(time, DimIdx(1));
        k->setValueAcrossDimensions(values);
    }
}
void
TrackMarker::notifyTrackingStarted()
{
    if (!_imp->trackingStartedCount) {
        // When tracking disable keyframes tracking on knobs that get updated at each track
        // step to keep the UI from refreshing at each frame.
        _imp->center.lock()->setKeyFrameTrackingEnabled(false);
        _imp->error.lock()->setKeyFrameTrackingEnabled(false);
        _imp->patternBtmLeft.lock()->setKeyFrameTrackingEnabled(false);
        _imp->patternBtmRight.lock()->setKeyFrameTrackingEnabled(false);
        _imp->patternTopLeft.lock()->setKeyFrameTrackingEnabled(false);
        _imp->patternTopRight.lock()->setKeyFrameTrackingEnabled(false);
    }
    ++_imp->trackingStartedCount;
}

void
TrackMarker::notifyTrackingEnded()
{
    if (_imp->trackingStartedCount) {
        --_imp->trackingStartedCount;
    }

    // Refresh knobs once finished
    if (!_imp->trackingStartedCount) {
        _imp->center.lock()->setKeyFrameTrackingEnabled(true);
        _imp->error.lock()->setKeyFrameTrackingEnabled(true);
        _imp->patternBtmLeft.lock()->setKeyFrameTrackingEnabled(true);
        _imp->patternBtmRight.lock()->setKeyFrameTrackingEnabled(true);
        _imp->patternTopLeft.lock()->setKeyFrameTrackingEnabled(true);
        _imp->patternTopRight.lock()->setKeyFrameTrackingEnabled(true);
    }
}

void
TrackMarker::onKeyFrameRemoved(const Curve* /*curve*/, const KeyFrame& key)
{
    Q_EMIT keyframeRemoved(key.getTime());
}

void
TrackMarker::onKeyFrameSet(const Curve* /*curve*/, const KeyFrame& key, bool /*added*/)
{
    Q_EMIT keyframeAdded(key.getTime());
}

void
TrackMarker::onKeyFrameMoved(const Curve* /*curve*/, const KeyFrame& from, const KeyFrame& to)
{
    Q_EMIT keyframeMoved(from.getTime(), to.getTime());
}

RectD
TrackMarker::getMarkerImageRoI(TimeValue time) const
{
    Point center, offset;
    KnobDoublePtr centerKnob = getCenterKnob();
    KnobDoublePtr offsetKnob = getOffsetKnob();

    center.x = centerKnob->getValueAtTime(time, DimIdx(0));
    center.y = centerKnob->getValueAtTime(time, DimIdx(1));

    offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
    offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

    RectD roiCanonical;
    KnobDoublePtr swBl = getSearchWindowBottomLeftKnob();
    KnobDoublePtr swTr = getSearchWindowTopRightKnob();

    roiCanonical.x1 = swBl->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
    roiCanonical.y1 = swBl->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;
    roiCanonical.x2 = swTr->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
    roiCanonical.y2 = swTr->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

    return roiCanonical;
}

std::pair<ImagePtr, RectD>
TrackMarker::getMarkerImage(TimeValue time,
                            const RectD& roi)
{

    assert( !roi.isNull() );

    NodePtr node = getModel()->getNode();
    NodePtr input = node->getInput(0);
    if (!input) {
        return std::make_pair(ImagePtr(), roi);
    }

    TreeRender::CtorArgsPtr args(new TreeRender::CtorArgs);
    {
        args->treeRootEffect = input->getEffectInstance();
        args->provider = args->treeRootEffect;
        args->time = time;
        args->view = ViewIdx(0);

        args->mipMapLevel = 0;
        args->proxyScale = RenderScale(1.);
        args->canonicalRoI = roi;
        args->draftMode = false;
        args->playback = false;
        args->byPassCache = false;
    }

    TreeRenderPtr render = TreeRender::create(args);

    FrameViewRequestPtr outputRequest;

    args->treeRootEffect->launchRender(render);
    ActionRetCodeEnum stat = args->treeRootEffect->waitForRenderFinished(render);
    if (isFailureRetCode(stat)) {
        return std::make_pair(ImagePtr(), roi);
    }
    outputRequest = render->getOutputRequest();
    ImagePtr sourceImage = outputRequest->getRequestedScaleImagePlane();

    // Make sure the Natron image rendered is RGBA full rect and on CPU, we don't support other formats
    if (sourceImage->getStorageMode() != eStorageModeRAM) {
        Image::InitStorageArgs initArgs;
        initArgs.bounds = sourceImage->getBounds();
        initArgs.plane = sourceImage->getLayer();
        initArgs.bufferFormat = eImageBufferLayoutRGBAPackedFullRect;
        initArgs.storage = eStorageModeRAM;
        initArgs.bitdepth = sourceImage->getBitDepth();
        ImagePtr tmpImage = Image::create(initArgs);
        if (!tmpImage) {
            return std::make_pair(ImagePtr(), roi);
        }
        Image::CopyPixelsArgs cpyArgs;
        cpyArgs.roi = initArgs.bounds;
        tmpImage->copyPixels(*sourceImage, cpyArgs);
        sourceImage = tmpImage;
        
    }

    return std::make_pair(sourceImage, roi);
} // TrackMarker::getMarkerImage



TrackMarkerPM::TrackMarkerPM(const KnobItemsTablePtr& context)
    : TrackMarker(context)
{
}

TrackMarkerPM::~TrackMarkerPM()
{
}

void
TrackMarkerPM::onTrackerNodeInputChanged(int /*inputNb*/)
{
    NodePtr thisNode = getModel()->getNode();

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
    center->unlink(DimSpec::all(), ViewSetSpec::all(), true);

    trackerNode->getEffectInstance()->onKnobValueChanged_public(button, eValueChangedReasonUserEdited, TimeValue(frame), ViewIdx(0));

    KnobDoublePtr markerCenter = getCenterKnob();
    // The TrackerPM plug-in has set a keyframe at the refFrame and frame, copy them
    bool ret = true;
    double centerPoint[2];
    for (int i = 0; i < center->getNDimensions(); ++i) {
        {
            int index = center->getKeyFrameIndex(ViewIdx(0), DimIdx(i), TimeValue(frame));
            if (index != -1) {
                centerPoint[i] = center->getValueAtTime(TimeValue(frame), DimIdx(i));
                markerCenter->setValueAtTime(TimeValue(frame), centerPoint[i], ViewSetSpec::all(), DimIdx(i));
            } else {
                // No keyframe at this time: tracking failed
                ret = false;
                break;
            }
        }
        {
            int index = center->getKeyFrameIndex(ViewIdx(0), DimIdx(i), TimeValue(refFrame));
            if (index != -1) {
                double value = center->getValueAtTime(TimeValue(refFrame), DimIdx(i));
                markerCenter->setValueAtTime(TimeValue(refFrame), value, ViewSetSpec::all(), DimIdx(i));
            }
        }
    }

    // Convert the correlation score of the TrackerPM to the error
    if (ret) {
        KnobDoublePtr markerError = getErrorKnob();
        KnobDoublePtr correlation = correlationScoreKnob.lock();
        {
            int index = correlation->getKeyFrameIndex(ViewIdx(0), DimIdx(0), TimeValue(frame));
            if (index != -1) {
                // The error is estimated as a percentage of the correlation across the number of pixels in the pattern window
                KnobDoublePtr  pBtmLeft = patternBtmLeftKnob.lock();
                KnobDoublePtr  pTopRight = patternTopRightKnob.lock();
                Point btmLeft, topRight;

                btmLeft.x = pBtmLeft->getValueAtTime(TimeValue(frame), DimIdx(0));
                btmLeft.y = pBtmLeft->getValueAtTime(TimeValue(frame), DimIdx(1));

                topRight.x = pTopRight->getValueAtTime(TimeValue(frame), DimIdx(0));
                topRight.y = pTopRight->getValueAtTime(TimeValue(frame), DimIdx(1));


                double areaPixels = (topRight.x - btmLeft.x) * (topRight.y - btmLeft.y);
                NodePtr trackerInput = trackerNode->getInput(0);
                if (trackerInput) {
                    ImagePlaneDesc comps, paireComps;
                    trackerInput->getEffectInstance()->getMetadataComponents(-1, &comps, &paireComps);
                    areaPixels *= comps.getNumComponents();
                }

                double value = correlation->getValueAtTime(TimeValue(frame), DimIdx(0));

                // Convert to a percentage
                if (areaPixels > 0) {
                    value /= areaPixels;
                }

                markerError->setValueAtTime(TimeValue(frame), value, ViewSetSpec::all(), DimIdx(0));
            }
        }
    }

    if ( !center->linkTo(markerCenter) ) {
        throw std::runtime_error("Could not link center");
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
    NodePtr thisNode = getModel()->getNode();
    NodePtr node;
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create( PLUGINID_OFX_TRACKERPM, NodeCollectionPtr() ));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "TrackerPMNode");

        node = getApp()->createNode(args);
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

    KnobItemsTablePtr model = getModel();
    EffectInstancePtr effect;
    if (model) {
        effect = model->getNode()->getEffectInstance();
    }

    trackPrevButton = getNodeKnob<KnobButton>(node, kTrackerPMParamTrackingPrevious);
    trackNextButton = getNodeKnob<KnobButton>(node, kTrackerPMParamTrackingNext);
    KnobDoublePtr center = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingCenterPoint);
    centerKnob = center;

    // Slave the center knob and unslave when tracking
    if ( !center->linkTo(getCenterKnob()) ) {
        throw std::runtime_error("Could not link center");
    }

    KnobDoublePtr offset = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingOffset);

    // Slave the offset knob
    if ( !offset->linkTo( getOffsetKnob() ) ) {
        throw std::runtime_error("Could not link offset");
    }

    offsetKnob = offset;

    // Ref frame is set for each
    refFrameKnob = getNodeKnob<KnobInt>(node, kTrackerPMParamTrackingReferenceFrame);

    // Enable reference frame
    KnobBoolPtr enableRefFrameKnob = getNodeKnob<KnobBool>(node, kTrackerPMParamTrackingEnableReferenceFrame);
    enableRefFrameKnob->setValue(true);

    KnobChoicePtr scoreType = getNodeKnob<KnobChoice>(node, kTrackerPMParamScore);
    if (effect) {
#ifdef kTrackerParamPatternMatchingScoreType
        KnobIPtr modelKnob = effect->getKnobByName(kTrackerParamPatternMatchingScoreType);
        if (modelKnob) {
            if ( !scoreType->linkTo(modelKnob) ) {
                throw std::runtime_error("Could not link scoreType");
            }
        }
#endif
    }

    scoreTypeKnob = scoreType;

    KnobDoublePtr correlationScore = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingCorrelationScore);
    correlationScoreKnob = correlationScore;

    KnobDoublePtr patternBtmLeft = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingPatternBoxBtmLeft);
    patternBtmLeftKnob = patternBtmLeft;

    // Slave the search window and pattern of the node to the parameters of the marker
    ignore_result(patternBtmLeft->linkTo(getPatternBtmLeftKnob()));

    KnobDoublePtr patternTopRight = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingPatternBoxTopRight);
    patternTopRightKnob = patternTopRight;
    ignore_result(patternTopRight->linkTo(getPatternTopRightKnob()));

    KnobDoublePtr searchWindowBtmLeft = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingSearchBoxBtmLeft);
    searchWindowBtmLeftKnob = searchWindowBtmLeft;
    ignore_result(searchWindowBtmLeft->linkTo(getSearchWindowBottomLeftKnob()));

    KnobDoublePtr searchWindowTopRight = getNodeKnob<KnobDouble>(node, kTrackerPMParamTrackingSearchBoxTopRight);
    searchWindowTopRightKnob = searchWindowTopRight;
    ignore_result(searchWindowTopRight->linkTo(getSearchWindowTopRightKnob()));

} // TrackMarkerPM::initializeKnobs



NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TrackMarker.cpp"

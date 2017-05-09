/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2013-2017 INRIA and Alexandre Gauthier-Foichat
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


#include "RotoPaintPrivate.h"

#include <QtCore/QLineF>

#include "Global/GLIncludes.h"
#include "Engine/Color.h"
#include "Engine/KnobTypes.h"
#include "Engine/RotoPaint.h"
#include "Engine/MergingEnum.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/Format.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/OverlaySupport.h"
#include "Engine/RotoPoint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/RotoUndoCommand.h"
#include "Engine/RotoLayer.h"
#include "Engine/Bezier.h"
#include "Engine/Transform.h"
#include "Engine/TimeLine.h"
#include "Engine/AppInstance.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER;


RotoPaintPrivate::RotoPaintPrivate(RotoPaint* publicInterface,
                                   RotoPaint::RotoPaintTypeEnum type)
    : TrackerParamsProvider()
    , publicInterface(publicInterface)
    , nodeType(type)
    , premultKnob()
    , enabledKnobs()
    , ui()
    , inputNodes()
    , premultNode()
    , knobsTable()
    , globalMergeNodes()
    , treeRefreshBlocked(0)
{
}



NodePtr
RotoPaintPrivate::getTrackerNode() const
{
    return publicInterface->getNode();
}

NodePtr
RotoPaintPrivate::getSourceImageNode() const
{
    NodePtr node =  publicInterface->getNode();
    if (!node) {
        return NodePtr();
    }
    return node->getInput(0);
}

ImagePlaneDesc
RotoPaintPrivate::getMaskImagePlane(int *channelIndex) const
{
    *channelIndex = 0;
    std::vector<std::string> channels(1);
    channels[0] = "A";
    ImagePlaneDesc rotoMaskPlane("RotoMask", "", "Alpha", channels);
    return rotoMaskPlane;
}

NodePtr
RotoPaintPrivate::getMaskImageNode() const
{
    // The bottom of the rotopaint tree
    return globalTimeBlurNode;
}

TrackerHelperPtr
RotoPaintPrivate::getTracker() const
{
    return tracker;
}

bool
RotoPaintPrivate::getCenterOnTrack() const
{
    return centerViewerButton.lock()->getValue();
}

bool
RotoPaintPrivate::getUpdateViewer() const
{
    return updateViewerButton.lock()->getValue();
}

void
RotoPaintPrivate::getTrackChannels(bool* doRed, bool* doGreen, bool* doBlue) const
{
    *doRed = enableTrackRed.lock()->getValue();
    *doGreen = enableTrackGreen.lock()->getValue();
    *doBlue = enableTrackBlue.lock()->getValue();
}

bool
RotoPaintPrivate::canDisableMarkersAutomatically() const
{
    return false;
}

double
RotoPaintPrivate::getMaxError() const
{
    return maxError.lock()->getValue();
}

int
RotoPaintPrivate::getMaxNIterations() const
{
    return maxIterations.lock()->getValue();
}

bool
RotoPaintPrivate::isBruteForcePreTrackEnabled() const
{
    return bruteForcePreTrack.lock()->getValue();
}

bool
RotoPaintPrivate::isNormalizeIntensitiesEnabled() const
{
    return useNormalizedIntensities.lock()->getValue();
}

double
RotoPaintPrivate::getPreBlurSigma() const
{
    return preBlurSigma.lock()->getValue();
}

RectD
RotoPaintPrivate::getInputRoD(TimeValue time, ViewIdx view) const
{
    EffectInstancePtr inputEffect = publicInterface->getInputRenderEffect(0, time, view);
    RectD ret;

    if (inputEffect) {
        GetRegionOfDefinitionResultsPtr results;
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(time, RenderScale(1.), view, &results);
        if (!isFailureRetCode(stat)) {
            return results->getRoD();
        }
    }
    Format f;
    publicInterface->getApp()->getProject()->getProjectDefaultFormat(&f);
    ret.x1 = f.x1;
    ret.x2 = f.x2;
    ret.y1 = f.y1;
    ret.y2 = f.y2;


    return ret;
} // getInputRoD

void
RotoPaintPrivate::setFromPointsToInputRod()
{
    RectD inputRod = getInputRoD(publicInterface->getTimelineCurrentTime(), ViewIdx(0));
    KnobDoublePtr fromPointsKnob[4];

    for (int i = 0; i < 4; ++i) {
        fromPointsKnob[i] = fromPoints[i].lock();
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x1;
        values[1] = inputRod.y1;
        fromPointsKnob[0]->setValueAcrossDimensions(values);
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x2;
        values[1] = inputRod.y1;
        fromPointsKnob[1]->setValueAcrossDimensions(values);
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x2;
        values[1] = inputRod.y2;
        fromPointsKnob[2]->setValueAcrossDimensions(values);
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x1;
        values[1] = inputRod.y2;
        fromPointsKnob[3]->setValueAcrossDimensions(values);
    }
}


RectD
RotoPaintPrivate::getNormalizationRoD(TimeValue time, ViewIdx view) const
{
    return getInputRoD(time, view);
}


TrackMarkerPtr
RotoPaintPrivate::createTrackForTracking(TimeValue startingFrame)
{


    KnobDoublePtr fromPointsKnob[4], toPointsKnob[4];
    for (int i = 0; i < 4; ++i) {
        fromPointsKnob[i] = fromPoints[i].lock();
        toPointsKnob[i] = toPoints[i].lock();
    }

    // If one of the toPoint is not animated, we assume
    // that the user cleared the animation or never launched a track.
    // We initialize the corner pin from points and to points as the bounding box
    // of the shapes to track.
    // To do so, we ensure no input of the RotoPaint node is connected, then we get
    // the region of definition of the bottom node in the RotoPaint nodegraph.

    TimeValue referenceFrame = TimeValue(trackerReferenceFrame.lock()->getValue());

    bool mustInitCornerPin = false;
    for (int i = 0; i < 4; ++i) {
        if (!toPointsKnob[i]->hasAnimation()) {
            mustInitCornerPin = true;
            break;
        }
    }
    if (mustInitCornerPin) {

        referenceFrame = startingFrame;
        trackerReferenceFrame.lock()->setValue(referenceFrame);

        // Disconnect the RotoPaint inputs so that we are sure the RoD in input is the RoD of the shapes to track
        const int maxInputs = publicInterface->getMaxInputCount();
        std::vector<NodePtr> inputs(maxInputs);
        NodePtr rotoPaintNode = publicInterface->getNode();


        {
            // Block any tree refresh and do it manually once
            BlockTreeRefreshRAII(this);
            for (int i = 0; i < maxInputs; ++i) {
                inputs[i] = rotoPaintNode->getInput(i);
                if (inputs[i]) {
                    rotoPaintNode->swapInput(NodePtr(), i);
                }
            }

            publicInterface->refreshRotoPaintTree();
        }

        RectD rod;
        {
            GetRegionOfDefinitionResultsPtr results;
            ActionRetCodeEnum stat = globalTimeBlurNode->getEffectInstance()->getRegionOfDefinition_public(referenceFrame, RenderScale(1.), ViewIdx(0), &results);
            if (isFailureRetCode(stat)) {
                QString error = publicInterface->tr("Failure to initialize planar surface to the RotoMask bounding box");
                throw std::runtime_error(error.toStdString());
            }
            rod = results->getRoD();
        }

        {
            std::vector<double> val(2);
            val[0] = rod.x1;
            val[1] = rod.y1;
            fromPointsKnob[0]->setValueAcrossDimensions(val);
            toPointsKnob[0]->setValueAtTimeAcrossDimensions(referenceFrame, val);
        }
        {
            std::vector<double> val(2);
            val[0] = rod.x2;
            val[1] = rod.y1;
            fromPointsKnob[1]->setValueAcrossDimensions(val);
            toPointsKnob[1]->setValueAtTimeAcrossDimensions(referenceFrame, val);
        }

        {
            std::vector<double> val(2);
            val[0] = rod.x2;
            val[1] = rod.y2;
            fromPointsKnob[2]->setValueAcrossDimensions(val);
            toPointsKnob[2]->setValueAtTimeAcrossDimensions(referenceFrame, val);
        }

        {
            std::vector<double> val(2);
            val[0] = rod.x1;
            val[1] = rod.y2;
            fromPointsKnob[3]->setValueAcrossDimensions(val);
            toPointsKnob[3]->setValueAtTimeAcrossDimensions(referenceFrame, val);
        }

        // Restore the tree as it was
        {
            // Block any tree refresh and do it manually once
            BlockTreeRefreshRAII(this);
            for (int i = 0; i < maxInputs; ++i) {
                if (inputs[i]) {
                    rotoPaintNode->swapInput(inputs[i], i);
                }
            }

            publicInterface->refreshRotoPaintTree();
        }

    } // mustInitCornerPin


    // Create the track
    TrackMarkerPtr track = TrackMarker::create(KnobItemsTablePtr());
    track->initializeKnobsPublic();

    KeyFrameSet keys = toPointsKnob[0]->getAnimationCurve(ViewIdx(0), DimIdx(0))->getKeyFrames_mt_safe();

    // Clone the toPoints animation to the pattern corners
    KnobDoublePtr patternPointsKnob[4];
    patternPointsKnob[0] = track->getPatternBtmLeftKnob();
    patternPointsKnob[1] = track->getPatternBtmRightKnob();
    patternPointsKnob[2] = track->getPatternTopRightKnob();
    patternPointsKnob[3] = track->getPatternTopLeftKnob();

    KnobDoublePtr centerPointKnob = track->getCenterKnob();

    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {


        // Corner points are relative to the center point
        Point toPoints[4];

        // The center point is at the average of the 4 pattern corners
        Point centerPoint = {0, 0};
        for (int i = 0; i < 4; ++i) {
            toPoints[i].x = toPointsKnob[i]->getValueAtTime(it->getTime(), DimIdx(0));
            toPoints[i].y = toPointsKnob[i]->getValueAtTime(it->getTime(), DimIdx(1));
            centerPoint.x += toPoints[i].x;
            centerPoint.y += toPoints[i].y;
        }

        centerPoint.x /= 4;
        centerPoint.y /= 4;

        for (int i = 0; i < 4; ++i) {
            toPoints[i].x -= centerPoint.x;
            toPoints[i].y -= centerPoint.y;
        }
        for (int i = 0; i < 4; ++i) {
            patternPointsKnob[i]->setValueAtTime(it->getTime(), toPoints[i].x, ViewIdx(0), DimIdx(0));
            patternPointsKnob[i]->setValueAtTime(it->getTime(), toPoints[i].y, ViewIdx(0), DimIdx(1));
        }
        centerPointKnob->setValueAtTime(it->getTime(), centerPoint.x, ViewIdx(0), DimIdx(0));
        centerPointKnob->setValueAtTime(it->getTime(), centerPoint.y, ViewIdx(0), DimIdx(1));
    } // for each keyframe  

    // Set the search window as the bounding box of the reference frame fromPoints plus an extent
    Point referencePoints[4];
    for (int i = 0; i < 4; ++i) {
        referencePoints[i].x = fromPointsKnob[i]->getValueAtTime(referenceFrame, DimIdx(0));
        referencePoints[i].y = fromPointsKnob[i]->getValueAtTime(referenceFrame, DimIdx(1));
    }

    RectD referencePointsBbox;
    {
        referencePointsBbox.x1 = referencePoints[0].x;
        referencePointsBbox.x2 = referencePoints[0].x;
        referencePointsBbox.y1 = referencePoints[0].y;
        referencePointsBbox.y2 = referencePoints[0].y;
        for (int i = 1; i < 4; ++i) {
            referencePointsBbox.x1 = std::min(referencePointsBbox.x1, referencePoints[i].x);
            referencePointsBbox.y1 = std::min(referencePointsBbox.y1, referencePoints[i].x);
            referencePointsBbox.x2 = std::max(referencePointsBbox.x2, referencePoints[i].y);
            referencePointsBbox.y2 = std::max(referencePointsBbox.y2, referencePoints[i].y);
        }
    }



    KnobDoublePtr searchPointBottomLeft, searchPointTopRight;
    searchPointBottomLeft = track->getSearchWindowBottomLeftKnob();
    searchPointTopRight = track->getSearchWindowTopRightKnob();

    // make the search window 70px larger than the pattern. This should maybe set using some other sort
    // of initialization
    const int searchWindowOffset = 70;
    referencePointsBbox.addPadding(searchWindowOffset, searchWindowOffset);


    {

        Point centerPoint = {0, 0};
        Point toPoints[4];
        for (int i = 0; i < 4; ++i) {
            toPoints[i].x = toPointsKnob[i]->getValueAtTime(referenceFrame, DimIdx(0));
            toPoints[i].y = toPointsKnob[i]->getValueAtTime(referenceFrame, DimIdx(1));
            centerPoint.x += toPoints[i].x;
            centerPoint.y += toPoints[i].y;
        }
        centerPoint.x /= 4;
        centerPoint.y /= 4;


        Point searchBottomLeftPoint, searchTopRightPoint;

        searchBottomLeftPoint.x = referencePointsBbox.x1 - centerPoint.x;
        searchBottomLeftPoint.y = referencePointsBbox.y1 - centerPoint.y;

        searchTopRightPoint.x = referencePointsBbox.x2 - centerPoint.x;
        searchTopRightPoint.y = referencePointsBbox.y2 - centerPoint.y;

        searchPointBottomLeft->setValue(searchBottomLeftPoint.x, ViewIdx(0), DimIdx(0));
        searchPointBottomLeft->setValue(searchBottomLeftPoint.y, ViewIdx(0), DimIdx(1));

        searchPointTopRight->setValue(searchTopRightPoint.x, ViewIdx(0), DimIdx(0));
        searchPointTopRight->setValue(searchTopRightPoint.y, ViewIdx(0), DimIdx(1));
    }


    // Set the motion model from the ui motion model
    track->getMotionModelKnob()->setValue(motionModel.lock()->getValue());

    return track;
} // createTrackForTracking

void
RotoPaint::onTrackingStarted(int step)
{
    if (step > 0) {
        _imp->trackFwButton.lock()->setValue(true, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    } else {
        _imp->trackBwButton.lock()->setValue(true, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    }
}

void
RotoPaint::onTrackingEnded()
{
    _imp->activePlanarTrack.reset();
    _imp->trackBwButton.lock()->setValue(false, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);
    _imp->trackFwButton.lock()->setValue(false, ViewIdx(0), DimIdx(0), eValueChangedReasonPluginEdited);

}

void
RotoPaintPrivate::onTrackRangeClicked()
{
    OverlaySupport* overlay = ui->getLastCallingViewport();

    assert(overlay);
    RangeD range = overlay->getFrameRange();

    int first = trackRangeDialogFirstFrame.lock()->getValue();
    int last = trackRangeDialogLastFrame.lock()->getValue();
    int step = trackRangeDialogStep.lock()->getValue();
    if (first == INT_MIN) {
        trackRangeDialogFirstFrame.lock()->setValue(range.min);
    }
    if (last == INT_MIN) {
        trackRangeDialogLastFrame.lock()->setValue(range.max);
    }
    if (step == INT_MIN) {
        trackRangeDialogStep.lock()->setValue(1);
    }
    KnobGroupPtr k = trackRangeDialogGroup.lock();
    if ( k->getValue() ) {
        k->setValue(false);
    } else {
        k->setValue(true);
    }
}

void
RotoPaintPrivate::startTrackingInternal(TimeValue startFrame, TimeValue lastFrame, TimeValue step, OverlaySupport* overlay)
{
    assert(activePlanarTrack);
    TrackMarkerPtr track = createTrackForTracking(startFrame);

    // Make sure the RotoPaint tree only contains nodes that are part of the PlanarTrackLayer to track
    publicInterface->refreshRotoPaintTree();

    // Launch tracking
    tracker->trackMarker(track, startFrame, lastFrame, step, overlay->getViewerNode());
}

void
RotoPaintPrivate::onTrackRangeOkClicked()
{
    int first = trackRangeDialogFirstFrame.lock()->getValue();
    int last = trackRangeDialogLastFrame.lock()->getValue();
    int step = trackRangeDialogStep.lock()->getValue();

    OverlaySupport* overlay = ui->getLastCallingViewport();
    assert(overlay);

    if ( tracker->isCurrentlyTracking() ) {
        tracker->abortTracking();
        return;
    }

    if (activePlanarTrack) {
        return;
    }

    if (step == 0) {
        publicInterface->message( eMessageTypeError, publicInterface->tr("The Step cannot be 0").toStdString() );
        return ;
    }


    int startFrame = step > 0 ? first : last;
    int lastFrame = step > 0 ? last + 1 : first - 1;

    if ( ( (step > 0) && (startFrame >= lastFrame) ) || ( (step < 0) && (startFrame <= lastFrame) ) ) {
        return;
    }


    activePlanarTrack = knobsTable->getSelectedPlanarTrack();

    if (!activePlanarTrack) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select a PlanarTrackGroup to track first. To create a PlanarTrackGroup right click on a shape in the settings panel").toStdString());
        return;
    }

    startTrackingInternal(TimeValue(startFrame), TimeValue(lastFrame), TimeValue(step), overlay);

    trackRangeDialogGroup.lock()->setValue(false);
}

void
RotoPaintPrivate::onTrackBwClicked()
{
    OverlaySupport* overlay = ui->getLastCallingViewport();
    assert(overlay);

    RangeD range = overlay->getFrameRange();

    TimeValue startFrame = publicInterface->getTimelineCurrentTime();

    if ( tracker->isCurrentlyTracking() ) {
        tracker->abortTracking();
        return;
    }

    if (activePlanarTrack) {
        return;
    }

    activePlanarTrack = knobsTable->getSelectedPlanarTrack();

    if (!activePlanarTrack) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select a PlanarTrackGroup to track first. To create a PlanarTrackGroup right click on a shape in the settings panel").toStdString());
        return;
    }


    startTrackingInternal(startFrame, TimeValue(range.min - 1), TimeValue(-1), overlay);

}

void
RotoPaintPrivate::onTrackPrevClicked()
{
    OverlaySupport* overlay = ui->getLastCallingViewport();
    assert(overlay);

    TimeValue startFrame = publicInterface->getTimelineCurrentTime();

    if ( tracker->isCurrentlyTracking() ) {
        tracker->abortTracking();
        return;
    }

    if (activePlanarTrack) {
        return;
    }

    activePlanarTrack = knobsTable->getSelectedPlanarTrack();

    if (!activePlanarTrack) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select a PlanarTrackGroup to track first. To create a PlanarTrackGroup right click on a shape in the settings panel").toStdString());
        return;
    }

    startTrackingInternal(startFrame, TimeValue(startFrame - 2), TimeValue(-1), overlay);
}

void
RotoPaintPrivate::onStopButtonClicked()
{
    tracker->abortTracking();
}

void
RotoPaintPrivate::onTrackNextClicked()
{
    OverlaySupport* overlay = ui->getLastCallingViewport();
    assert(overlay);

    TimeValue startFrame = publicInterface->getTimelineCurrentTime();

    if ( tracker->isCurrentlyTracking() ) {
        tracker->abortTracking();
        return;
    }

    if (activePlanarTrack) {
        return;
    }

    activePlanarTrack = knobsTable->getSelectedPlanarTrack();

    if (!activePlanarTrack) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select a PlanarTrackGroup to track first. To create a PlanarTrackGroup right click on a shape in the settings panel").toStdString());
        return;
    }

    startTrackingInternal(startFrame, TimeValue(startFrame - 2), TimeValue(1), overlay);
}

void
RotoPaintPrivate::onTrackFwClicked()
{
    OverlaySupport* overlay = ui->getLastCallingViewport();
    assert(overlay);

    RangeD range = overlay->getFrameRange();

    TimeValue startFrame = publicInterface->getTimelineCurrentTime();

    if ( tracker->isCurrentlyTracking() ) {
        tracker->abortTracking();
        return;
    }

    if (activePlanarTrack) {
        return;
    }

    activePlanarTrack = knobsTable->getSelectedPlanarTrack();

    if (!activePlanarTrack) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select a PlanarTrackGroup to track first. To create a PlanarTrackGroup right click on a shape in the settings panel").toStdString());
        return;
    }

    startTrackingInternal(startFrame, TimeValue(range.max +1), TimeValue(1), overlay);
}

void
RotoPaintPrivate::onClearAllAnimationClicked()
{

    PlanarTrackLayerPtr activePlanarTrack = knobsTable->getSelectedPlanarTrack();
    if (activePlanarTrack) {
        activePlanarTrack->clearTransformAnimation();
    }

    RectD inputRod = getInputRoD(publicInterface->getTimelineCurrentTime(), ViewIdx(0));

    Point rodPoints[4] = {{inputRod.x1,inputRod.y1}, {inputRod.x2,inputRod.y1}, {inputRod.x2,inputRod.y2}, {inputRod.x2,inputRod.y2}};

    for (int i = 0; i < 4; ++i) {
        KnobDoublePtr to = toPoints[i].lock();
        to->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        KnobDoublePtr from = fromPoints[i].lock();
        from->removeAnimation(ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

         // Reset from & to to the input Rod
        from->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
        from->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));

        to->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
        to->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));
    }

}

void
RotoPaintPrivate::onClearBwAnimationClicked()
{
    TimeValue time = publicInterface->getTimelineCurrentTime();
    PlanarTrackLayerPtr activePlanarTrack = knobsTable->getSelectedPlanarTrack();
    if (activePlanarTrack) {
        activePlanarTrack->clearTransformAnimationBeforeTime(time);
    }

    RectD inputRod = getInputRoD(time, ViewIdx(0));

    Point rodPoints[4] = {{inputRod.x1,inputRod.y1}, {inputRod.x2,inputRod.y1}, {inputRod.x2,inputRod.y2}, {inputRod.x2,inputRod.y2}};

    for (int i = 0; i < 4; ++i) {
        KnobDoublePtr to = toPoints[i].lock();
        to->deleteValuesBeforeTime(std::set<double>(), time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        KnobDoublePtr from = fromPoints[i].lock();
        from->deleteValuesBeforeTime(std::set<double>(), time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        if (!to->hasAnimation()) {

            // Reset from & to to the input Rod if no animation anymore
            from->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
            from->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));
            
            to->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
            to->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));
        }
    }

}

void
RotoPaintPrivate::onClearFwAnimationClicked()
{
    TimeValue time = publicInterface->getTimelineCurrentTime();
    PlanarTrackLayerPtr activePlanarTrack = knobsTable->getSelectedPlanarTrack();
    if (activePlanarTrack) {
        activePlanarTrack->clearTransformAnimationBeforeTime(time);
    }

    RectD inputRod = getInputRoD(time, ViewIdx(0));

    Point rodPoints[4] = {{inputRod.x1,inputRod.y1}, {inputRod.x2,inputRod.y1}, {inputRod.x2,inputRod.y2}, {inputRod.x2,inputRod.y2}};

    for (int i = 0; i < 4; ++i) {
        KnobDoublePtr to = toPoints[i].lock();
        to->deleteValuesAfterTime(std::set<double>(), time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        KnobDoublePtr from = fromPoints[i].lock();
        from->deleteValuesAfterTime(std::set<double>(), time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        if (!to->hasAnimation()) {

            // Reset from & to to the input Rod if no animation anymore
            from->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
            from->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));

            to->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
            to->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));
        }
    }
}

void
RotoPaintPrivate::onRemoveKeyframeButtonClicked()
{
    TimeValue time = publicInterface->getTimelineCurrentTime();
    PlanarTrackLayerPtr activePlanarTrack = knobsTable->getSelectedPlanarTrack();
    if (activePlanarTrack) {
        activePlanarTrack->deleteTransformKeyframe(time);
    }

    RectD inputRod = getInputRoD(time, ViewIdx(0));

    Point rodPoints[4] = {{inputRod.x1,inputRod.y1}, {inputRod.x2,inputRod.y1}, {inputRod.x2,inputRod.y2}, {inputRod.x2,inputRod.y2}};

    for (int i = 0; i < 4; ++i) {
        KnobDoublePtr to = toPoints[i].lock();
        to->deleteValueAtTime(time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        KnobDoublePtr from = fromPoints[i].lock();
        from->deleteValueAtTime(time, ViewSetSpec::all(), DimSpec::all(), eValueChangedReasonUserEdited);

        if (!to->hasAnimation()) {

            // Reset from & to to the input Rod if no animation anymore
            from->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
            from->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));

            to->setValue(rodPoints[i].x, ViewIdx(0), DimIdx(0));
            to->setValue(rodPoints[i].y, ViewIdx(0), DimIdx(1));
        }
    }
}


bool
RotoPaintPrivate::trackStepFunctor(int trackIndex, const TrackArgsBasePtr& args, int frame)
{

    TrackArgs* trackerArgs = dynamic_cast<TrackArgs*>(args.get());
    assert(trackerArgs);

    assert( trackIndex >= 0 && trackIndex < trackerArgs->getNumTracks() );
    const std::vector<TrackMarkerAndOptionsPtr >& tracks = trackerArgs->getTracks();
    const TrackMarkerAndOptionsPtr& track = tracks[trackIndex];
    bool ret =  TrackerHelperPrivate::trackStepLibMV(trackIndex, *trackerArgs, frame);
    updateCornerPinFromTrack(track->natronMarker, TimeValue(frame));
    return ret;

} // trackStepFunctor

void
RotoPaintPrivate::updateCornerPinFromTrack(const TrackMarkerPtr& track, TimeValue time)
{

    KnobDoublePtr trackPointKnobs[4];
    trackPointKnobs[0] = track->getPatternBtmLeftKnob();
    trackPointKnobs[1] = track->getPatternBtmRightKnob();
    trackPointKnobs[2] = track->getPatternTopRightKnob();
    trackPointKnobs[3] = track->getPatternTopLeftKnob();

    KnobDoublePtr centerPointKnob = track->getCenterKnob();

    Point center;
    Point trackPoints[4];

    center.x = centerPointKnob->getValueAtTime(time, DimIdx(0));
    center.y = centerPointKnob->getValueAtTime(time, DimIdx(1));

    for (int i = 0; i < 4; ++i) {
        trackPoints[i].x = trackPointKnobs[i]->getValueAtTime(time, DimIdx(0));
        trackPoints[i].y = trackPointKnobs[i]->getValueAtTime(time, DimIdx(1));

        // The track points are relative to the center, make them absolute
        trackPoints[i].x += center.x;
        trackPoints[i].y += center.y;
    }

    // Update the corner pin to points
    for (int i = 0; i < 4; ++i) {
        KnobDoublePtr knob = toPoints[i].lock();
        knob->setValueAtTime(time, trackPoints[i].x, ViewIdx(0), DimIdx(0));
        knob->setValueAtTime(time, trackPoints[i].y, ViewIdx(0), DimIdx(1));
    }


    // Update the transform so that the PlanarTrack group shapes follow
    updatePlanarTrackExtraMatrix(time, activePlanarTrack);
}

void
RotoPaintPrivate::updatePlanarTrackExtraMatrixForAllKeyframes()
{
    PlanarTrackLayerPtr selectedTrack = knobsTable->getSelectedPlanarTrack();
    if (!selectedTrack) {
        return;
    }
    
    selectedTrack->clearTransformAnimation();

    KeyFrameSet keys = toPoints[0].lock()->getAnimationCurve(ViewIdx(0), DimIdx(0))->getKeyFrames_mt_safe();
    for (KeyFrameSet::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        updatePlanarTrackExtraMatrix(it->getTime(), selectedTrack);
    }
}

void
RotoPaintPrivate::updatePlanarTrackExtraMatrix(TimeValue time, const PlanarTrackLayerPtr& planarTrack)
{
    openMVG::Vec3 to[4], from[4];

    for (int i = 0; i < 4; ++i) {
        KnobDoublePtr fromKnob = fromPoints[i].lock();
        KnobDoublePtr toKnob = toPoints[i].lock();

        to[i][0] = toKnob->getValueAtTime(time, DimIdx(0));
        to[i][1] = toKnob->getValueAtTime(time, DimIdx(1));
        to[i][2] = 1;
        from[i][0] = fromKnob->getValueAtTime(time, DimIdx(0));
        from[i][1] = fromKnob->getValueAtTime(time, DimIdx(1));
        from[i][2] = 1;
    }


    openMVG::Mat3 H;
    if (!openMVG::robust::homography_from_four_points(from[0], from[1], from[2], from[3], to[0], to[1], to[2], to[3], &H)) {
        return;
    }

    Transform::Matrix3x3 mat( H(0, 0), H(0, 1), H(0, 2),
                         H(1, 0), H(1, 1), H(1, 2),
                         H(2, 0), H(2, 1), H(2, 2) );

    if (planarTrack) {
        planarTrack->setExtraMatrix(true, time, ViewIdx(0), mat);
    }
} // updatePlanarTrackExtraMatrix

void
RotoPaintPrivate::createPlanarTrackForSelectedShapes()
{
    SelectedItems selection = knobsTable->getSelectedDrawableItems();
    if (selection.empty()) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select a shape to plane track first").toStdString());
        return;
    }

    std::list< RotoDrawableItemPtr > drawables;
    for (SelectedItems::iterator it = selection.begin(); it != selection.end(); ++it) {
        RotoDrawableItemPtr isDrawable = boost::dynamic_pointer_cast<RotoDrawableItem>(*it);
        if (!isDrawable) {
            continue;
        }
        drawables.push_back(isDrawable);
    }

    if (drawables.size() != 1) {
        publicInterface->message(eMessageTypeWarning, publicInterface->tr("You must select exactly 1 shape to plane track").toStdString());
        return;
    }
    publicInterface->createPlanarTrackForShape(drawables.front());
}

void
RotoPaint::createPlanarTrackForShape(const RotoDrawableItemPtr& item)
{

    PlanarTrackLayerPtr layer(new PlanarTrackLayer(_imp->knobsTable));

    RotoLayerPtr parentLayer = getLayerForNewItem();
    _imp->knobsTable->addItem(layer, parentLayer, eTableChangeReasonInternal);

    _imp->knobsTable->removeItem(item, eTableChangeReasonInternal);
    _imp->knobsTable->addItem(item, layer, eTableChangeReasonInternal);

    _imp->refreshTrackingControlsVisiblity();

} // createPlanarTrackForSelectedShapes

void
RotoPaintPrivate::refreshTrackingControlsVisiblity()
{
    bool hasPlanarTrack = false;
    std::vector<KnobTableItemPtr> items = knobsTable->getAllItems();
    for (std::vector<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        PlanarTrackLayerPtr isPlaneTrack = toPlanarTrackLayer(*it);
        if (!isPlaneTrack) {
            hasPlanarTrack = true;
            break;
        }
    }


    trackBwButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    trackPrevButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    trackNextButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    trackFwButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    trackRangeButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    removeKeyframeButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    clearAllAnimationButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    clearBwAnimationButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    clearFwAnimationButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    updateViewerButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    centerViewerButton.lock()->setInViewerContextSecret(!hasPlanarTrack);
    showCornerPinOverlay.lock()->setInViewerContextSecret(!hasPlanarTrack);
    motionModel.lock()->setInViewerContextSecret(!hasPlanarTrack);

} // refreshTrackingControlsVisiblity

void
RotoPaintPrivate::exportTrackDataFromExportOptions()
{

} // exportTrackDataFromExportOptions

NATRON_NAMESPACE_EXIT;

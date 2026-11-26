/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2026 The Natron developers
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

// The Tracker node without a viewer, as in NatronRenderer or a Python script (#1256):
// - its buttons (e.g. Compute, Track Next) did nothing: TrackerNode::knobChanged() returned at once
//   without a viewer;
// - the transform solve ended in a slot of a QFutureWatcher, which no event loop called in background mode:
//   the solved parameters were never set.
// The Tracker node needs openfx-misc (its internal Transform and CornerPin nodes, and CheckerBoard for the
// image to track): without it, the tests are skipped.

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <cmath>
#include <cstdio>
#include <iostream>
#include <list>
#include <set>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"

#include "TestUtils.h"

#define PLUGINID_OFX_CHECKERBOARD "net.sf.openfx.CheckerBoardPlugin"

// From Engine/TrackerContextPrivate.h and Engine/TrackerNodeInteract.h, which need headers that the tests
// don't have (libmv, QtConcurrent)
#define kTrackerUIParamTrackNext "trackNext"
#define kTrackerParamAutoGenerateTransform "autoComputeransform"
#define kTrackerParamGenerateTransform "computeTransform"
#define kTrackerParamMotionType "motionType"
#define kTrackerParamTransformType "transformType"
#define kTrackerParamReferenceFrame "referenceFrame"
#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamCenter "center"
#define kTrackerParamMotionModel "motionModel" // Engine/TrackMarker.h
const int kMotionTypeMatchMove = 2; // eTrackerMotionTypeMatchMove
const int kTransformTypeTransform = 0; // eTrackerTransformNodeTransform
const int kTransformTypeCornerPin = 1; // eTrackerTransformNodeCornerPin

NATRON_NAMESPACE_USING

namespace {
using NatronTest::createNode;

class TrackerHeadlessTest
    : public ::testing::Test
{
protected:
    NodePtr _tracker;
    TrackerContextPtr _context;
    TrackMarkerPtr _track;

    virtual void SetUp() OVERRIDE
    {
        if ( !NatronTest::isPluginAvailable(PLUGINID_OFX_TRANSFORM) || !NatronTest::isPluginAvailable(PLUGINID_OFX_CORNERPIN) ) {
            return;
        }
        _tracker = createNode(PLUGINID_NATRON_TRACKER);
        if (!_tracker) {
            return;
        }
        _context = _tracker->getTrackerContext();
        // No automatic solve: the tests ask for it
        KnobBoolPtr autoSolve = knob<KnobBool>(kTrackerParamAutoGenerateTransform);
        if (autoSolve) {
            autoSolve->setValue(false);
        }
        _track = _context->createMarker();
    }

    virtual void TearDown() OVERRIDE
    {
        if (_context) {
            _context->abortTracking_blocking();
        }
        _track.reset();
        _context.reset();
        if (_tracker) {
            _tracker->destroyNode(true, false);
            _tracker.reset();
        }
    }

    // true if the test must return: the plug-ins are missing (skipped), or the tracker was not created (failed)
    bool skipped()
    {
        if ( !NatronTest::requirePlugins({ PLUGINID_OFX_TRANSFORM, PLUGINID_OFX_CORNERPIN }, "openfx-misc") ) {
            return true;
        }
        EXPECT_TRUE( bool(_track) ) << "the Tracker node or its track was not created";

        return !_track;
    }

    template <typename K>
    std::shared_ptr<K> knob(const std::string& name) const
    {
        return std::dynamic_pointer_cast<K>( _tracker->getKnobByName(name) );
    }

    // A track moving by (+10, 0) per frame from frame 1 to 5, and a match-move transform solve
    void animateTrack()
    {
        KnobDoublePtr center = _track->getCenterKnob();

        for (int t = 1; t <= 5; ++t) {
            center->setValueAtTime(t, 100. + 10. * t, ViewSpec::all(), 0);
            center->setValueAtTime(t, 200., ViewSpec::all(), 1);
        }
        knob<KnobChoice>(kTrackerParamMotionType)->setValue( kMotionTypeMatchMove );
        knob<KnobChoice>(kTrackerParamTransformType)->setValue( kTransformTypeTransform );
        knob<KnobInt>(kTrackerParamReferenceFrame)->setValue(1);
    }

    // The translation solved at frame 5 (the track moved by 40 since the reference frame 1)
    double solvedTranslateX() const
    {
        return knob<KnobDouble>(kTransformParamTranslate)->getValueAtTime(5, 0);
    }
};
} // anonymous namespace

// The transform solve sets the solved parameters before it returns, without an event loop
TEST_F(TrackerHeadlessTest, SolveSetsTheParametersAtOnce)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    ASSERT_TRUE( knob<KnobBool>(kTrackerParamAutoGenerateTransform) && knob<KnobChoice>(kTrackerParamMotionType) &&
                 knob<KnobChoice>(kTrackerParamTransformType) && knob<KnobInt>(kTrackerParamReferenceFrame) &&
                 knob<KnobDouble>(kTransformParamTranslate) );
    animateTrack();
    ASSERT_DOUBLE_EQ( 0., solvedTranslateX() );
    _context->solveTransformParams();
    EXPECT_NEAR(40., solvedTranslateX(), 1e-3) << "the solved translation was not set when the solve returned";
}

// The Compute button solves the transform without a viewer
TEST_F(TrackerHeadlessTest, ComputeButtonWithoutViewer)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    ASSERT_TRUE( knob<KnobBool>(kTrackerParamAutoGenerateTransform) && knob<KnobChoice>(kTrackerParamMotionType) &&
                 knob<KnobChoice>(kTrackerParamTransformType) && knob<KnobInt>(kTrackerParamReferenceFrame) &&
                 knob<KnobDouble>(kTransformParamTranslate) );
    animateTrack();
    ASSERT_DOUBLE_EQ( 0., solvedTranslateX() );
    KnobButtonPtr compute = knob<KnobButton>(kTrackerParamGenerateTransform);
    ASSERT_TRUE( bool(compute) );
    compute->trigger(); // as ButtonParam.trigger() in Python
    EXPECT_NEAR(40., solvedTranslateX(), 1e-3) << "the Compute button did nothing without a viewer";
}

// Tracks the track on a CheckerBoard from frame 1 to 2 with the Track Next button, and waits for the end
// of the tracking (and for the solve that follows it, which runs on the main thread)
static void
trackNextOnCheckerBoard(const NodePtr& tracker,
                        const TrackerContextPtr& context,
                        const TrackMarkerPtr& track,
                        const KnobButtonPtr& trackNext)
{
    NodePtr checkerBoard = createNode(PLUGINID_OFX_CHECKERBOARD);
    ASSERT_TRUE( bool(checkerBoard) );
    ASSERT_TRUE( tracker->connectInput(checkerBoard, 0) );

    // a corner of the checkerboard (64x64 boxes)
    KnobDoublePtr center = track->getCenterKnob();
    center->setValueAtTime(1, 256., ViewSpec::all(), 0);
    center->setValueAtTime(1, 256., ViewSpec::all(), 1);
    context->addTrackToSelection(track, TrackerContext::eTrackSelectionInternal);
    tracker->getApp()->getTimeLine()->seekFrame(1, false, 0, eTimelineChangeReasonOtherSeek);

    ASSERT_TRUE( bool(trackNext) );
    trackNext->trigger(); // as ButtonParam.trigger() in Python
    // the tracking sets a user keyframe at its start frame before its thread starts
    std::set<int> userKeys;
    track->getUserKeyframes(&userKeys);
    ASSERT_EQ( 1u, userKeys.count(1) ) << "the Track Next button did nothing without a viewer";

    // the tracking runs in its own thread
    QElapsedTimer timer;
    timer.start();
    while ( ( (center->getKeyFramesCount(ViewSpec::all(), 0) < 2) || context->isCurrentlyTracking() ) && (timer.elapsed() < 10000) ) {
        QCoreApplication::processEvents();
        QThread::msleep(10);
    }
    // the solve queued at the end of the tracking
    QCoreApplication::processEvents();
    double time = 0.;
    EXPECT_EQ( 2, center->getKeyFramesCount(ViewSpec::all(), 0) ) << "the Track Next button did not track without a viewer";
    EXPECT_TRUE( center->getKeyFrameTime(ViewSpec::all(), 1, 0, &time) && (time == 2.) );

    context->abortTracking_blocking();
    checkerBoard->destroyNode(true, false);
}

// The Track Next button tracks the selected tracks without a viewer, from the timeline's current frame
TEST_F(TrackerHeadlessTest, TrackNextButtonWithoutViewer)
{
    if ( skipped() || !NatronTest::requirePlugins({ PLUGINID_OFX_CHECKERBOARD }, "openfx-misc") ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    ASSERT_NO_FATAL_FAILURE( trackNextOnCheckerBoard( _tracker, _context, _track, knob<KnobButton>(kTrackerUIParamTrackNext) ) );
}

// The transform is solved after a tracking (the motion type is not None), without a viewer
TEST_F(TrackerHeadlessTest, SolveAfterTrackingWithoutViewer)
{
    if ( skipped() || !NatronTest::requirePlugins({ PLUGINID_OFX_CHECKERBOARD }, "openfx-misc") ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    KnobChoicePtr motionType = knob<KnobChoice>(kTrackerParamMotionType);
    ASSERT_TRUE( bool(motionType) );
    motionType->setValue(kMotionTypeMatchMove);
    knob<KnobInt>(kTrackerParamReferenceFrame)->setValue(1);
    KnobDoublePtr transformCenter = knob<KnobDouble>(kTransformParamCenter);
    ASSERT_TRUE( bool(transformCenter) );
    ASSERT_NO_FATAL_FAILURE( trackNextOnCheckerBoard( _tracker, _context, _track, knob<KnobButton>(kTrackerUIParamTrackNext) ) );
    // the solve sets the center of the transform to the center of the tracks at the reference frame
    EXPECT_NEAR( 256., transformCenter->getValue(0), 1e-6 ) << "the transform was not solved after the tracking";
    EXPECT_NEAR( 256., transformCenter->getValue(1), 1e-6 ) << "the transform was not solved after the tracking";
    // and the solve ended: it enables the solver parameters again
    EXPECT_TRUE( motionType->isEnabled(0) ) << "the solve after the tracking did not end";
}

namespace {
// The motion of the synthetic sequence of TracksAKnownMotion: a Transform centered on the image, which moves it by
// (3, -2) pixels, rotates it by 1 degree and scales it by 1% per frame from frame 1, where it is the identity.
const int kKnownMotionFirstFrame = 1;
const int kKnownMotionLastFrame = 6;

void
knownMotion(int frame,
            double* tx,
            double* ty,
            double* rotateDegrees,
            double* scale)
{
    const int t = frame - kKnownMotionFirstFrame;

    *tx = 3. * t;
    *ty = -2. * t;
    *rotateDegrees = 1. * t;
    *scale = 1. + 0.01 * t;
}

// Where the known motion around (centerX, centerY) takes the point (x, y) of the first frame
void
knownMotionPoint(int frame,
                 double centerX,
                 double centerY,
                 double x,
                 double y,
                 double* xOut,
                 double* yOut)
{
    double tx, ty, rotateDegrees, scale;

    knownMotion(frame, &tx, &ty, &rotateDegrees, &scale);
    const double a = rotateDegrees * std::atan(1.) / 45.;
    const double dx = scale * (x - centerX);
    const double dy = scale * (y - centerY);
    *xOut = centerX + tx + std::cos(a) * dx - std::sin(a) * dy;
    *yOut = centerY + ty + std::sin(a) * dx + std::cos(a) * dy;
}

// Records a value with all its digits in the test's XML output (--gtest_output=xml), to compare two builds exactly
void
recordValue(const std::string& key,
            double value)
{
    char buf[64];

    std::snprintf(buf, sizeof(buf), "%.17g", value);
    ::testing::Test::RecordProperty(key, buf);
}
} // anonymous namespace

// Tracks four corners of a CheckerBoard moved by a known Transform animation with each motion model, then solves a
// Transform and a CornerPin from the tracks. The tracks and the solved parameters must follow the known motion.
TEST_F(TrackerHeadlessTest, TracksAKnownMotion)
{
    if ( skipped() || !NatronTest::requirePlugins({ PLUGINID_OFX_CHECKERBOARD }, "openfx-misc") ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    // only the tracks created below are solved
    _context->removeMarker(_track);
    _track.reset();

    NodePtr checkerBoard = createNode(PLUGINID_OFX_CHECKERBOARD);
    ASSERT_TRUE( bool(checkerBoard) );
    NodePtr transform = createNode(PLUGINID_OFX_TRANSFORM);
    ASSERT_TRUE( bool(transform) );
    ASSERT_TRUE( transform->connectInput(checkerBoard, 0) );
    ASSERT_TRUE( _tracker->connectInput(transform, 0) );
    KnobDoublePtr translate = std::dynamic_pointer_cast<KnobDouble>( transform->getKnobByName(kTransformParamTranslate) );
    KnobDoublePtr rotate = std::dynamic_pointer_cast<KnobDouble>( transform->getKnobByName(kTransformParamRotate) );
    KnobDoublePtr scale = std::dynamic_pointer_cast<KnobDouble>( transform->getKnobByName(kTransformParamScale) );
    KnobDoublePtr center = std::dynamic_pointer_cast<KnobDouble>( transform->getKnobByName(kTransformParamCenter) );
    ASSERT_TRUE(translate && rotate && scale && center);
    // the CheckerBoard covers the project format, and its 64x64 boxes start from its center
    Format format;
    _tracker->getApp()->getProject()->getProjectDefaultFormat(&format);
    const double centerX = (format.x1 + format.x2) / 2.;
    const double centerY = (format.y1 + format.y2) / 2.;
    center->setValue(centerX, ViewSpec::all(), 0);
    center->setValue(centerY, ViewSpec::all(), 1);
    for (int f = kKnownMotionFirstFrame; f <= kKnownMotionLastFrame; ++f) {
        double tx, ty, rotateDegrees, s;
        knownMotion(f, &tx, &ty, &rotateDegrees, &s);
        translate->setValueAtTime(f, tx, ViewSpec::all(), 0);
        translate->setValueAtTime(f, ty, ViewSpec::all(), 1);
        rotate->setValueAtTime(f, rotateDegrees, ViewSpec::all(), 0);
        scale->setValueAtTime(f, s, ViewSpec::all(), 0);
        scale->setValueAtTime(f, s, ViewSpec::all(), 1);
    }

    // corners of boxes of the CheckerBoard, around the center of the Transform (also the center of the tracks)
    const double corners[4][2] = {
        { centerX - 192., centerY - 128. }, { centerX + 192., centerY - 128. },
        { centerX + 192., centerY + 128. }, { centerX - 192., centerY + 128. }
    };
    std::vector<ChoiceOption> models;
    TrackerContext::getMotionModelsAndHelps(true, &models);
    ASSERT_EQ( 6u, models.size() );
    std::list<TrackMarkerPtr> tracks;
    for (std::size_t m = 0; m < models.size(); ++m) {
        SCOPED_TRACE(models[m].id);
        for (TrackMarkerPtr& track : tracks) {
            _context->removeMarker(track);
        }
        tracks.clear();
        for (int i = 0; i < 4; ++i) {
            TrackMarkerPtr track = _context->createMarker();
            ASSERT_TRUE( bool(track) );
            KnobChoicePtr motionModel = std::dynamic_pointer_cast<KnobChoice>( track->getKnobByName(kTrackerParamMotionModel) );
            ASSERT_TRUE( bool(motionModel) );
            motionModel->setValue( (int)m );
            track->getCenterKnob()->setValueAtTime(kKnownMotionFirstFrame, corners[i][0], ViewSpec::all(), 0);
            track->getCenterKnob()->setValueAtTime(kKnownMotionFirstFrame, corners[i][1], ViewSpec::all(), 1);
            tracks.push_back(track);
        }
        _tracker->getApp()->getTimeLine()->seekFrame(kKnownMotionFirstFrame, false, 0, eTimelineChangeReasonOtherSeek);
        // the last frame is excluded
        _context->trackMarkers(tracks, kKnownMotionFirstFrame, kKnownMotionLastFrame + 1, 1, 0);
        // the tracking runs in its own thread, which may not have started yet
        const int frameCount = kKnownMotionLastFrame - kKnownMotionFirstFrame + 1;
        KnobDoublePtr lastTrackCenter = tracks.back()->getCenterKnob();
        QElapsedTimer timer;
        timer.start();
        while ( ( (lastTrackCenter->getKeyFramesCount(ViewSpec::all(), 0) < frameCount) || _context->isCurrentlyTracking() ) &&
                (timer.elapsed() < 60000) ) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        QCoreApplication::processEvents();
        ASSERT_FALSE( _context->isCurrentlyTracking() ) << "the tracking did not end";

        int i = 0;
        for (const TrackMarkerPtr& track : tracks) {
            KnobDoublePtr trackCenter = track->getCenterKnob();
            EXPECT_EQ( frameCount, trackCenter->getKeyFramesCount(ViewSpec::all(), 0) );
            for (int f = kKnownMotionFirstFrame + 1; f <= kKnownMotionLastFrame; ++f) {
                double x, y;
                knownMotionPoint(f, centerX, centerY, corners[i][0], corners[i][1], &x, &y);
                const double trackedX = trackCenter->getValueAtTime(f, 0);
                const double trackedY = trackCenter->getValueAtTime(f, 1);
                const std::string key = "model" + std::to_string(m) + "_track" + std::to_string(i) + "_frame" + std::to_string(f);
                recordValue(key + "_x", trackedX);
                recordValue(key + "_y", trackedY);
                EXPECT_NEAR(x, trackedX, 0.25) << key;
                EXPECT_NEAR(y, trackedY, 0.25) << key;
            }
            ++i;
        }

        // solve a Transform, then a CornerPin, from the tracks
        knob<KnobChoice>(kTrackerParamMotionType)->setValue(kMotionTypeMatchMove);
        knob<KnobInt>(kTrackerParamReferenceFrame)->setValue(kKnownMotionFirstFrame);
        knob<KnobChoice>(kTrackerParamTransformType)->setValue(kTransformTypeTransform);
        _context->solveTransformParams();
        KnobDoublePtr solvedTranslate = knob<KnobDouble>(kTransformParamTranslate);
        KnobDoublePtr solvedRotate = knob<KnobDouble>(kTransformParamRotate);
        KnobDoublePtr solvedScale = knob<KnobDouble>(kTransformParamScale);
        ASSERT_TRUE(solvedTranslate && solvedRotate && solvedScale);
        for (int f = kKnownMotionFirstFrame + 1; f <= kKnownMotionLastFrame; ++f) {
            double tx, ty, rotateDegrees, s;
            knownMotion(f, &tx, &ty, &rotateDegrees, &s);
            const std::string key = "model" + std::to_string(m) + "_transform_frame" + std::to_string(f);
            recordValue( key + "_tx", solvedTranslate->getValueAtTime(f, 0) );
            recordValue( key + "_ty", solvedTranslate->getValueAtTime(f, 1) );
            recordValue( key + "_rotate", solvedRotate->getValueAtTime(f, 0) );
            recordValue( key + "_scale", solvedScale->getValueAtTime(f, 0) );
            EXPECT_NEAR(tx, solvedTranslate->getValueAtTime(f, 0), 0.25) << key;
            EXPECT_NEAR(ty, solvedTranslate->getValueAtTime(f, 1), 0.25) << key;
            EXPECT_NEAR(rotateDegrees, solvedRotate->getValueAtTime(f, 0), 0.05) << key;
            EXPECT_NEAR(s, solvedScale->getValueAtTime(f, 0), 0.001) << key;
        }

        knob<KnobChoice>(kTrackerParamTransformType)->setValue(kTransformTypeCornerPin);
        _context->solveTransformParams();
        for (int c = 0; c < 4; ++c) {
            KnobDoublePtr to = knob<KnobDouble>( "to" + std::to_string(c + 1) );
            ASSERT_TRUE( bool(to) );
            for (int f = kKnownMotionFirstFrame + 1; f <= kKnownMotionLastFrame; ++f) {
                const std::string key = "model" + std::to_string(m) + "_cornerpin_to" + std::to_string(c + 1) + "_frame" + std::to_string(f);
                recordValue( key + "_x", to->getValueAtTime(f, 0) );
                recordValue( key + "_y", to->getValueAtTime(f, 1) );
            }
        }
    }
    for (TrackMarkerPtr& track : tracks) {
        _context->removeMarker(track);
    }
    _context->abortTracking_blocking();
    transform->destroyNode(true, false);
    checkerBoard->destroyNode(true, false);
}

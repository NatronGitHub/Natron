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
#define kTrackerParamSmooth "smooth"
#define kTrackerParamSmoothCornerPin "smoothCornerPin"
#define kCornerPinParamFrom1 "from1"
#define kCornerPinParamTo1 "to1"
#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamCenter "center"
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

    // A track moving by (+10, 0) per frame from frame 1 to lastFrame, and a match-move transform solve
    void animateTrack(int lastFrame = 5)
    {
        KnobDoublePtr center = _track->getCenterKnob();

        for (int t = 1; t <= lastFrame; ++t) {
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

// The solve gives one result per keyframe of the track. With Smooth > 1, each solved value is the average of
// the results around its time, the first result included: it was never sampled, and with a single result the
// average asserted (Debug) or was 0 (#1941).
// Smooth 3 averages the result at a time with the one before and the one after it. The translations solved
// from frame 1 to 5 are 0, 10, 20, 30 and 40.
TEST_F(TrackerHeadlessTest, SmoothedTranslationSamplesTheFirstResult)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( knob<KnobInt>(kTrackerParamSmooth) && knob<KnobDouble>(kTransformParamTranslate) );
    const std::vector<std::vector<double> > expected = {
        { 0. },                     // 1 result
        { 5., 5. },                 // 2 results
        { 5., 10., 20., 30., 35. }, // N = 5 results
    };
    for (const std::vector<double>& translations : expected) {
        const int nResults = (int)translations.size();
        SCOPED_TRACE( std::to_string(nResults) + " result(s)" );
        _track->getCenterKnob()->removeAnimation(ViewSpec::all(), 0);
        _track->getCenterKnob()->removeAnimation(ViewSpec::all(), 1);
        animateTrack(nResults);
        knob<KnobInt>(kTrackerParamSmooth)->setValue(3, ViewSpec::all(), 0);
        _context->solveTransformParams();
        KnobDoublePtr translate = knob<KnobDouble>(kTransformParamTranslate);
        EXPECT_EQ( nResults, translate->getKeyFramesCount(ViewSpec::all(), 0) );
        for (int t = 1; t <= nResults; ++t) {
            EXPECT_NEAR(translations[t - 1], translate->getValueAtTime(t, 0), 1e-3) << "frame " << t;
        }
    }
}

// A CornerPin smoothed from a single result: the "to" points are the "from" points (the track doesn't move), not
// (0, 0) (#1941)
TEST_F(TrackerHeadlessTest, SmoothedCornerPinFromASingleResult)
{
    if ( skipped() ) {
        return;
    }
    ASSERT_TRUE( knob<KnobInt>(kTrackerParamSmoothCornerPin) && knob<KnobDouble>(kCornerPinParamFrom1) &&
                 knob<KnobDouble>(kCornerPinParamTo1) );
    animateTrack(1);
    knob<KnobChoice>(kTrackerParamTransformType)->setValue( kTransformTypeCornerPin );
    KnobDoublePtr from1 = knob<KnobDouble>(kCornerPinParamFrom1);
    from1->setValue(30., ViewSpec::all(), 0);
    from1->setValue(70., ViewSpec::all(), 1);
    knob<KnobInt>(kTrackerParamSmoothCornerPin)->setValue(3);
    _context->solveTransformParams();
    KnobDoublePtr to1 = knob<KnobDouble>(kCornerPinParamTo1);
    EXPECT_NEAR(30., to1->getValueAtTime(1, 0), 1e-3);
    EXPECT_NEAR(70., to1->getValueAtTime(1, 1), 1e-3);
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
// The CornerPin points are the corners of the image, far from the tracks: the tracking errors grow there
const double kCornerPinTolerance = 2.;

struct KnownMotion
{
    double tx, ty, rotateDegrees, scale;
};

KnownMotion
knownMotion(int frame)
{
    const int t = frame - kKnownMotionFirstFrame;

    return KnownMotion{ 3. * t, -2. * t, 1. * t, 1. + 0.01 * t };
}

struct Point2D
{
    double x, y;
};

// Where the known motion around center takes the point p of the first frame
Point2D
knownMotionPoint(int frame,
                 const Point2D& center,
                 const Point2D& p)
{
    const KnownMotion m = knownMotion(frame);
    const double a = m.rotateDegrees * std::atan(1.) / 45.;
    const double dx = m.scale * (p.x - center.x);
    const double dy = m.scale * (p.y - center.y);

    return Point2D{ center.x + m.tx + std::cos(a) * dx - std::sin(a) * dy,
                    center.y + m.ty + std::sin(a) * dx + std::cos(a) * dy };
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

    // destroyed at the end of the test, also when an assertion fails
    NatronTest::NodeScope nodes;
    NodePtr checkerBoard = nodes.create(PLUGINID_OFX_CHECKERBOARD);
    ASSERT_TRUE( bool(checkerBoard) );
    NodePtr transform = nodes.create(PLUGINID_OFX_TRANSFORM);
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
    const Point2D motionCenter{ (format.x1 + format.x2) / 2., (format.y1 + format.y2) / 2. };
    center->setValue(motionCenter.x, ViewSpec::all(), 0);
    center->setValue(motionCenter.y, ViewSpec::all(), 1);
    for (int f = kKnownMotionFirstFrame; f <= kKnownMotionLastFrame; ++f) {
        const KnownMotion m = knownMotion(f);
        translate->setValueAtTime(f, m.tx, ViewSpec::all(), 0);
        translate->setValueAtTime(f, m.ty, ViewSpec::all(), 1);
        rotate->setValueAtTime(f, m.rotateDegrees, ViewSpec::all(), 0);
        scale->setValueAtTime(f, m.scale, ViewSpec::all(), 0);
        scale->setValueAtTime(f, m.scale, ViewSpec::all(), 1);
    }

    // corners of boxes of the CheckerBoard, around the center of the Transform (also the center of the tracks)
    const Point2D corners[4] = {
        { motionCenter.x - 192., motionCenter.y - 128. }, { motionCenter.x + 192., motionCenter.y - 128. },
        { motionCenter.x + 192., motionCenter.y + 128. }, { motionCenter.x - 192., motionCenter.y + 128. }
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
            track->getCenterKnob()->setValueAtTime(kKnownMotionFirstFrame, corners[i].x, ViewSpec::all(), 0);
            track->getCenterKnob()->setValueAtTime(kKnownMotionFirstFrame, corners[i].y, ViewSpec::all(), 1);
            tracks.push_back(track);
        }
        _tracker->getApp()->getTimeLine()->seekFrame(kKnownMotionFirstFrame, false, 0, eTimelineChangeReasonOtherSeek);
        // the last frame is excluded
        _context->trackMarkers(tracks, kKnownMotionFirstFrame, kKnownMotionLastFrame + 1, 1, 0);
        // the tracking runs in its own thread, which may not have started yet
        const int frameCount = kKnownMotionLastFrame - kKnownMotionFirstFrame + 1;
        KnobDoublePtr lastTrackCenter = tracks.back()->getCenterKnob();
        // (it may also end before every frame is tracked, if a track is lost)
        QElapsedTimer timer;
        timer.start();
        bool started = false;
        while (timer.elapsed() < 60000) {
            const bool tracking = _context->isCurrentlyTracking();
            started = started || tracking;
            if ( !tracking && ( started || (lastTrackCenter->getKeyFramesCount(ViewSpec::all(), 0) >= frameCount) ) ) {
                break;
            }
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
                const Point2D expected = knownMotionPoint(f, motionCenter, corners[i]);
                const double trackedX = trackCenter->getValueAtTime(f, 0);
                const double trackedY = trackCenter->getValueAtTime(f, 1);
                const std::string key = "model" + std::to_string(m) + "_track" + std::to_string(i) + "_frame" + std::to_string(f);
                recordValue(key + "_x", trackedX);
                recordValue(key + "_y", trackedY);
                EXPECT_NEAR(expected.x, trackedX, 0.25) << key;
                EXPECT_NEAR(expected.y, trackedY, 0.25) << key;
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
            const KnownMotion expected = knownMotion(f);
            const std::string key = "model" + std::to_string(m) + "_transform_frame" + std::to_string(f);
            recordValue( key + "_tx", solvedTranslate->getValueAtTime(f, 0) );
            recordValue( key + "_ty", solvedTranslate->getValueAtTime(f, 1) );
            recordValue( key + "_rotate", solvedRotate->getValueAtTime(f, 0) );
            recordValue( key + "_scale", solvedScale->getValueAtTime(f, 0) );
            EXPECT_NEAR(expected.tx, solvedTranslate->getValueAtTime(f, 0), 0.25) << key;
            EXPECT_NEAR(expected.ty, solvedTranslate->getValueAtTime(f, 1), 0.25) << key;
            EXPECT_NEAR(expected.rotateDegrees, solvedRotate->getValueAtTime(f, 0), 0.05) << key;
            EXPECT_NEAR(expected.scale, solvedScale->getValueAtTime(f, 0), 0.001) << key;
        }

        knob<KnobChoice>(kTrackerParamTransformType)->setValue(kTransformTypeCornerPin);
        _context->solveTransformParams();
        // the CornerPin takes its "from" points (at the reference frame) where the known motion takes them
        for (int c = 0; c < 4; ++c) {
            KnobDoublePtr from = knob<KnobDouble>( "from" + std::to_string(c + 1) );
            KnobDoublePtr to = knob<KnobDouble>( "to" + std::to_string(c + 1) );
            ASSERT_TRUE(from && to);
            const Point2D fromPoint{ from->getValueAtTime(kKnownMotionFirstFrame, 0), from->getValueAtTime(kKnownMotionFirstFrame, 1) };
            for (int f = kKnownMotionFirstFrame + 1; f <= kKnownMotionLastFrame; ++f) {
                const std::string key = "model" + std::to_string(m) + "_cornerpin_to" + std::to_string(c + 1) + "_frame" + std::to_string(f);
                const Point2D expected = knownMotionPoint(f, motionCenter, fromPoint);
                recordValue( key + "_x", to->getValueAtTime(f, 0) );
                recordValue( key + "_y", to->getValueAtTime(f, 1) );
                EXPECT_NEAR(expected.x, to->getValueAtTime(f, 0), kCornerPinTolerance) << key;
                EXPECT_NEAR(expected.y, to->getValueAtTime(f, 1), kCornerPinTolerance) << key;
            }
        }
    }
    for (TrackMarkerPtr& track : tracks) {
        _context->removeMarker(track);
    }
    _context->abortTracking_blocking();
}

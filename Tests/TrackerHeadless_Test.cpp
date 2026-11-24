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

#include <iostream>
#include <set>
#include <string>

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QThread>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Plugin.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"

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
#define kTransformParamCenter "center"
const int kMotionTypeMatchMove = 2; // eTrackerMotionTypeMatchMove
const int kTransformTypeTransform = 0; // eTrackerTransformNodeTransform

NATRON_NAMESPACE_USING

namespace {
bool
isPluginAvailable(const char* pluginID)
{
    try {
        Plugin* p = appPTR->getPluginBinary(QString::fromUtf8(pluginID), -1, -1, false);

        return p && p->getLibraryBinary();
    } catch (...) {
        return false;
    }
}

// The bundled google-test predates GTEST_SKIP(): say clearly that the test did not run
bool
skipIfMissing(const char* pluginID)
{
    if ( isPluginAvailable(pluginID) ) {
        return false;
    }
    const testing::TestInfo* info = testing::UnitTest::GetInstance()->current_test_info();
    std::cerr << "[  SKIPPED ] " << info->test_case_name() << '.' << info->name()
              << ": plug-in " << pluginID << " not found (needs openfx-misc in OFX_PLUGIN_PATH)" << std::endl;

    return true;
}

NodePtr
createNode(const char* pluginID)
{
    AppInstancePtr app = appPTR->getTopLevelInstance();
    CreateNodeArgs args( pluginID, app->getProject() );

    args.setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
    args.setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);

    return app->createNode(args);
}

class TrackerHeadlessTest
    : public ::testing::Test
{
protected:
    NodePtr _tracker;
    TrackerContextPtr _context;
    TrackMarkerPtr _track;

    virtual void SetUp() OVERRIDE
    {
        if ( !isPluginAvailable(PLUGINID_OFX_TRANSFORM) || !isPluginAvailable(PLUGINID_OFX_CORNERPIN) ) {
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

    bool skipped()
    {
        if ( skipIfMissing(PLUGINID_OFX_TRANSFORM) || skipIfMissing(PLUGINID_OFX_CORNERPIN) ) {
            return true;
        }

        return false;
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
    if ( skipped() || skipIfMissing(PLUGINID_OFX_CHECKERBOARD) ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    trackNextOnCheckerBoard( _tracker, _context, _track, knob<KnobButton>(kTrackerUIParamTrackNext) );
}

// The transform is solved after a tracking (the motion type is not None), without a viewer
TEST_F(TrackerHeadlessTest, SolveAfterTrackingWithoutViewer)
{
    if ( skipped() || skipIfMissing(PLUGINID_OFX_CHECKERBOARD) ) {
        return;
    }
    ASSERT_TRUE(_tracker && _context && _track);
    KnobChoicePtr motionType = knob<KnobChoice>(kTrackerParamMotionType);
    ASSERT_TRUE( bool(motionType) );
    motionType->setValue(kMotionTypeMatchMove);
    knob<KnobInt>(kTrackerParamReferenceFrame)->setValue(1);
    KnobDoublePtr transformCenter = knob<KnobDouble>(kTransformParamCenter);
    ASSERT_TRUE( bool(transformCenter) );
    trackNextOnCheckerBoard( _tracker, _context, _track, knob<KnobButton>(kTrackerUIParamTrackNext) );
    // the solve sets the center of the transform to the center of the tracks at the reference frame
    EXPECT_NEAR( 256., transformCenter->getValue(0), 1e-6 ) << "the transform was not solved after the tracking";
    EXPECT_NEAR( 256., transformCenter->getValue(1), 1e-6 ) << "the transform was not solved after the tracking";
}

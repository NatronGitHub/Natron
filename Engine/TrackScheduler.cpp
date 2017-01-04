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

#include "TrackScheduler.h"

#define TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE 8
#define NATRON_TRACKER_REPORT_PROGRESS_DELTA_MS 200

#include <boost/bind.hpp>


CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QWaitCondition>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/KnobTypes.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackArgs.h"
#include "Engine/TrackerHelperPrivate.h"
#include "Engine/TLSHolder.h"
#include "Engine/Timer.h"
#include "Engine/TrackerParamsProvider.h"
#include "Engine/ViewerInstance.h"

NATRON_NAMESPACE_ENTER;


struct TrackSchedulerPrivate
{
    TrackerParamsProviderWPtr paramsProvider;

    TrackSchedulerPrivate(const TrackerParamsProviderPtr& paramsProvider)
    : paramsProvider(paramsProvider)
    {
    }

    /*
     * @brief Function that will be called concurrently for each Track marker to track.
     * @param trackIndex Identifies the track in args, which is supposed to hold the tracks vector.
     * @param time The time at which to track. The reference frame is held in the args and can be different for each track
     */
    static bool trackStepFunctor(int trackIndex, const TrackArgs& args, int time);
};

TrackScheduler::TrackScheduler(const TrackerParamsProviderPtr& paramsProvider)
: GenericSchedulerThread()
, _imp( new TrackSchedulerPrivate(paramsProvider) )
{
    QObject::connect( this, SIGNAL(renderCurrentFrameForViewer(ViewerInstancePtr)), this, SLOT(doRenderCurrentFrameForViewer(ViewerInstancePtr)) );

    setThreadName("TrackScheduler");
}

TrackScheduler::~TrackScheduler()
{
}

bool
TrackSchedulerPrivate::trackStepFunctor(int trackIndex,
                                        const TrackArgs& args,
                                        int time)
{
    assert( trackIndex >= 0 && trackIndex < args.getNumTracks() );
    const std::vector<TrackMarkerAndOptionsPtr >& tracks = args.getTracks();
    const TrackMarkerAndOptionsPtr& track = tracks[trackIndex];

    if ( !track->natronMarker->isEnabled(time) ) {
        return false;
    }

    TrackMarkerPMPtr isTrackerPM = toTrackMarkerPM( track->natronMarker );
    bool ret;
    if (isTrackerPM) {
        ret = TrackerHelperPrivate::trackStepTrackerPM(isTrackerPM, args, time);
    } else {
        ret = TrackerHelperPrivate::trackStepLibMV(trackIndex, args, time);
    }

    // Disable the marker since it failed to track
    if (!ret && args.isAutoKeyingEnabledParamEnabled()) {
        track->natronMarker->setEnabledAtTime(time, false);
    }

    appPTR->getAppTLS()->cleanupTLSForThread();

    return ret;
}


class IsTrackingFlagSetter_RAII
{
    Q_DECLARE_TR_FUNCTIONS(TrackScheduler)

private:
    ViewerInstancePtr _v;
    EffectInstancePtr _effect;
    TrackScheduler* _base;
    bool _reportProgress;
    bool _doPartialUpdates;

public:

    IsTrackingFlagSetter_RAII(TrackScheduler* base,
                              int step,
                              bool reportProgress,
                              const ViewerInstancePtr& viewer,
                              bool doPartialUpdates)
    : _v(viewer)
    , _base(base)
    , _reportProgress(reportProgress)
    , _doPartialUpdates(doPartialUpdates)
    {
        if (_reportProgress) {
            _base->emit_trackingStarted(step);
        }

        if (viewer && doPartialUpdates) {
            viewer->setDoingPartialUpdates(doPartialUpdates);
        }
    }

    ~IsTrackingFlagSetter_RAII()
    {
        if (_v && _doPartialUpdates) {
            _v->setDoingPartialUpdates(false);
        }
        if (_reportProgress) {
            _base->emit_trackingFinished();
        }
    }
};


GenericSchedulerThread::ThreadStateEnum
TrackScheduler::threadLoopOnce(const ThreadStartArgsPtr& inArgs)
{
    boost::shared_ptr<TrackArgs> args = boost::dynamic_pointer_cast<TrackArgs>(inArgs);

    assert(args);

    ThreadStateEnum state = eThreadStateActive;
    TimeLinePtr timeline = args->getTimeLine();
    ViewerInstancePtr viewer =  args->getViewer();
    int end = args->getEnd();
    int start = args->getStart();
    int cur = start;
    int frameStep = args->getStep();
    int framesCount = 0;
    if (frameStep != 0) {
        framesCount = frameStep > 0 ? (end - start) / frameStep : (start - end) / std::abs(frameStep);
    }

    TrackerParamsProviderPtr paramsProvider = _imp->paramsProvider.lock();

    const std::vector<TrackMarkerAndOptionsPtr >& tracks = args->getTracks();
    const int numTracks = (int)tracks.size();
    std::vector<int> trackIndexes( tracks.size() );



    // For all tracks, notify tracking is starting and unslave the 'enabled' knob if it is
    // slaved to the UI "enabled" knob.
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        trackIndexes[i] = i;
        tracks[i]->natronMarker->notifyTrackingStarted();
    }

    // Beyond TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE it becomes more expensive to render all partial rectangles
    // than just render the whole viewer RoI
    const bool doPartialUpdates = numTracks < TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE;
    int lastValidFrame = frameStep > 0 ? start - 1 : start + 1;
    bool reportProgress = numTracks > 1 || framesCount > 1;
    timeval lastProgressUpdateTime;
    gettimeofday(&lastProgressUpdateTime, 0);

    bool allTrackFailed = false;
    {
        ///Use RAII style for setting the isDoingPartialUpdates flag so we're sure it gets removed
        IsTrackingFlagSetter_RAII __istrackingflag__(this, frameStep, reportProgress, viewer, doPartialUpdates);

        if ( (frameStep == 0) || ( (frameStep > 0) && (start >= end) ) || ( (frameStep < 0) && (start <= end) ) ) {
            // Invalid range
            cur = end;
        }


        while (cur != end) {
            ///Launch parallel thread for each track using the global thread pool
            QFuture<bool> future = QtConcurrent::mapped( trackIndexes,
                                                        boost::bind(&TrackSchedulerPrivate::trackStepFunctor,
                                                                    _1,
                                                                    *args,
                                                                    cur) );
            future.waitForFinished();

            allTrackFailed = true;
            for (QFuture<bool>::const_iterator it = future.begin(); it != future.end(); ++it) {
                if ( (*it) ) {
                    allTrackFailed = false;
                    break;
                }
            }



            lastValidFrame = cur;



            // We don't have any successful track, stop
            if (allTrackFailed) {
                break;
            }



            cur += frameStep;

            double progress;
            if (frameStep > 0) {
                progress = (double)(cur - start) / framesCount;
            } else {
                progress = (double)(start - cur) / framesCount;
            }

            bool isUpdateViewerOnTrackingEnabled = paramsProvider->getUpdateViewer();
            bool isCenterViewerEnabled = paramsProvider->getCenterOnTrack();
            bool enoughTimePassedToReportProgress;
            {
                timeval now;
                gettimeofday(&now, 0);
                double dt =  now.tv_sec  - lastProgressUpdateTime.tv_sec +
                (now.tv_usec - lastProgressUpdateTime.tv_usec) * 1e-6f;
                dt *= 1000; // switch to MS
                enoughTimePassedToReportProgress = dt > NATRON_TRACKER_REPORT_PROGRESS_DELTA_MS;
                if (enoughTimePassedToReportProgress) {
                    lastProgressUpdateTime = now;
                }
            }


            ///Ok all tracks are finished now for this frame, refresh viewer if needed
            if (isUpdateViewerOnTrackingEnabled && viewer) {
                //This will not refresh the viewer since when tracking, renderCurrentFrame()
                //is not called on viewers, see Gui::onTimeChanged
                timeline->seekFrame(cur, true, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);

                if (enoughTimePassedToReportProgress) {
                    if (doPartialUpdates) {
                        std::list<RectD> updateRects;
                        args->getRedrawAreasNeeded(cur, &updateRects);
                        viewer->setPartialUpdateParams(updateRects, isCenterViewerEnabled);
                    } else {
                        viewer->clearPartialUpdateParams();
                    }
                    Q_EMIT renderCurrentFrameForViewer(viewer);
                }
            }

            if (enoughTimePassedToReportProgress && reportProgress) {
                ///Notify we progressed of 1 frame
                Q_EMIT trackingProgress(progress);
            }

            // Check for abortion
            state = resolveState();
            if ( (state == eThreadStateAborted) || (state == eThreadStateStopped) ) {
                break;
            }
        } // while (cur != end) {
    } // IsTrackingFlagSetter_RAII

    appPTR->getAppTLS()->cleanupTLSForThread();

    for (std::size_t i = 0; i < tracks.size(); ++i) {
        tracks[i]->natronMarker->notifyTrackingEnded();
    }



    //Now that tracking is done update viewer once to refresh the whole visible portion

    if ( paramsProvider->getUpdateViewer() ) {
        //Refresh all viewers to the current frame
        timeline->seekFrame(lastValidFrame, true, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
    
    return state;
} // >::threadLoopOnce

void
TrackScheduler::doRenderCurrentFrameForViewer(const ViewerInstancePtr& viewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    viewer->renderCurrentFrame(true);
}

void
TrackScheduler::track(const boost::shared_ptr<TrackArgs>& args)
{
    startTask(args);
}


NATRON_NAMESPACE_EXIT;
NATRON_NAMESPACE_USING;
#include "moc_TrackScheduler.cpp"

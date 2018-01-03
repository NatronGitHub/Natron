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
#include "Engine/TLSHolder.h"
#include "Engine/Timer.h"
#include "Engine/Node.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/TrackerParamsProvider.h"
#include "Engine/RenderEngine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

NATRON_NAMESPACE_ENTER


struct TrackSchedulerPrivate
{
    TrackerParamsProviderBaseWPtr paramsProvider;

    TrackSchedulerPrivate(const TrackerParamsProviderBasePtr& paramsProvider)
    : paramsProvider(paramsProvider)
    {
    }

};

TrackScheduler::TrackScheduler(const TrackerParamsProviderBasePtr& paramsProvider)
: GenericSchedulerThread()
, _imp( new TrackSchedulerPrivate(paramsProvider) )
{
    QObject::connect( this, SIGNAL(renderCurrentFrameForViewer(ViewerNodePtr)), this, SLOT(doRenderCurrentFrameForViewer(ViewerNodePtr)) );

    setThreadName("TrackScheduler");
}

TrackScheduler::~TrackScheduler()
{
}


class IsTrackingFlagSetter_RAII
{
    Q_DECLARE_TR_FUNCTIONS(TrackScheduler)

private:
    ViewerNodePtr _v;
    EffectInstancePtr _effect;
    TrackScheduler* _base;
    bool _doPartialUpdates;

public:

    IsTrackingFlagSetter_RAII(TrackScheduler* base,
                              int step,
                              const ViewerNodePtr& viewer,
                              bool doPartialUpdates)
    : _v(viewer)
    , _base(base)
    , _doPartialUpdates(doPartialUpdates)
    {
        _base->emit_trackingStarted(step);


        if (viewer && doPartialUpdates) {
            viewer->setDoingPartialUpdates(doPartialUpdates);
        }
    }

    ~IsTrackingFlagSetter_RAII()
    {
        if (_v && _doPartialUpdates) {
            _v->setDoingPartialUpdates(false);
        }
        _base->emit_trackingFinished();
    }
};


GenericSchedulerThread::ThreadStateEnum
TrackScheduler::threadLoopOnce(const GenericThreadStartArgsPtr& inArgs)
{
    boost::shared_ptr<TrackArgsBase> args = boost::dynamic_pointer_cast<TrackArgs>(inArgs);

    assert(args);

    ThreadStateEnum state = eThreadStateActive;
    TimeLinePtr timeline = args->getTimeLine();
    ViewerNodePtr viewer =  args->getViewer();
    int end = args->getEnd();
    int start = args->getStart();
    int cur = start;
    int frameStep = args->getStep();
    int framesCount = 0;
    if (frameStep != 0) {
        framesCount = frameStep > 0 ? (end - start) / frameStep : (start - end) / std::abs(frameStep);
    }

    TrackerParamsProviderBasePtr paramsProvider = _imp->paramsProvider.lock();

    const int numTracks = args->getNumTracks();
    std::vector<int> trackIndexes(numTracks);
    for (std::size_t i = 0; i < trackIndexes.size(); ++i) {
        trackIndexes[i] = i;
    }

    paramsProvider->beginTrackSequence(args);

    // Beyond TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE it becomes more expensive to render all partial rectangles
    // than just render the whole viewer RoI
    const bool doPartialUpdates = numTracks < TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE;
    int lastValidFrame = frameStep > 0 ? start - 1 : start + 1;
    timeval lastProgressUpdateTime;
    gettimeofday(&lastProgressUpdateTime, 0);

    bool allTrackFailed = false;
    {
        ///Use RAII style for setting the isDoingPartialUpdates flag so we're sure it gets removed
        IsTrackingFlagSetter_RAII __istrackingflag__(this, frameStep, viewer, doPartialUpdates);

        if ( (frameStep == 0) || ( (frameStep > 0) && (start >= end) ) || ( (frameStep < 0) && (start <= end) ) ) {
            // Invalid range
            cur = end;
        }


        while (cur != end) {
            ///Launch parallel thread for each track using the global thread pool
            QFuture<bool> future = QtConcurrent::mapped( trackIndexes,
                                                        boost::bind(&TrackerParamsProviderBase::trackStepFunctor,
                                                                    paramsProvider.get(),
                                                                    _1,
                                                                    args,
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

                        double unionPx = 0;
                        args->getRedrawAreasNeeded(TimeValue(cur), &updateRects);
                        for (std::list<RectD>::const_iterator it = updateRects.begin(); it != updateRects.end(); ++it) {
                            unionPx += it->area();
                        }
                        double totalPx = args->getFormatHeight() * args->getFormatWidth();
                        if (totalPx > 0 && unionPx / totalPx >= 0.5) {
                            // If the update areas cover more than half the image, redraw it all as usual
                            viewer->clearPartialUpdateParams();
                        } else {
                            viewer->setPartialUpdateParams(updateRects, isCenterViewerEnabled);
                        }
                    } else {
                        viewer->clearPartialUpdateParams();
                    }
                    Q_EMIT renderCurrentFrameForViewer(viewer);
                }
            }

            if (enoughTimePassedToReportProgress) {
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

    paramsProvider->endTrackSequence(args);


    //Now that tracking is done update viewer once to refresh the whole visible portion

    if ( paramsProvider->getUpdateViewer() ) {
        //Refresh all viewers to the current frame
        timeline->seekFrame(lastValidFrame, true, EffectInstancePtr(), eTimelineChangeReasonOtherSeek);
    }
    
    return state;
} // >::threadLoopOnce

void
TrackScheduler::doRenderCurrentFrameForViewer(const ViewerNodePtr& viewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    viewer->getNode()->getRenderEngine()->renderCurrentFrame();
}

void
TrackScheduler::track(const TrackArgsBasePtr& args)
{
    startTask(args);
}


NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_TrackScheduler.cpp"

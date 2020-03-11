/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
 * Copyright (C) 2018-2020 The Natron developers
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

#include "TrackerHelper.h"
#include "TrackerHelperPrivate.h"

#include <set>
#include <sstream>

#include <boost/make_shared.hpp>

#include "Engine/AppInstance.h"
#include "Engine/EffectInstance.h"
#include "Engine/Node.h"
#include "Engine/Project.h"
#include "Engine/TrackArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/TrackMarker.h"

NATRON_NAMESPACE_ENTER


TrackerHelper::TrackerHelper(const TrackerParamsProviderPtr &provider)
: QObject()
, _imp(new TrackerHelperPrivate(this, provider))
{

}

TrackerHelper::~TrackerHelper()
{
}


bool
TrackerHelper::isCurrentlyTracking() const
{
    return _imp->scheduler->isWorking();
}

void
TrackerHelper::abortTracking()
{
    _imp->scheduler->abortThreadedTask();
}

void
TrackerHelper::abortTracking_blocking()
{
    _imp->scheduler->abortThreadedTask();
    _imp->scheduler->waitForAbortToComplete_enforce_blocking();
}

void
TrackerHelper::quitTrackerThread_non_blocking()
{
    _imp->scheduler->quitThread(true);
}

bool
TrackerHelper::hasTrackerThreadQuit() const
{
    return !_imp->scheduler->isRunning();
}

void
TrackerHelper::quitTrackerThread_blocking(bool allowRestart)
{
    _imp->scheduler->quitThread(allowRestart);
    _imp->scheduler->waitForThreadToQuit_enforce_blocking();
}


void
TrackerHelper::onSchedulerTrackingStarted(int frameStep)
{
    TrackerParamsProviderPtr provider = _imp->provider.lock();
    if (!provider) {
        return;
    }
    NodePtr node = provider->getTrackerNode();
    if (!node) {
        return;
    }
    node->getApp()->progressStart(node, tr("Tracking...").toStdString(), "");
    Q_EMIT trackingStarted(frameStep);
}

void
TrackerHelper::onSchedulerTrackingFinished()
{
    TrackerParamsProviderPtr provider = _imp->provider.lock();
    if (!provider) {
        return;
    }
    NodePtr node = provider->getTrackerNode();
    if (!node) {
        return;
    }
    node->getApp()->progressEnd( node );
    Q_EMIT trackingFinished();
}

void
TrackerHelper::onSchedulerTrackingProgress(double progress)
{
    TrackerParamsProviderPtr provider = _imp->provider.lock();
    if (!provider) {
        return;
    }
    NodePtr node = provider->getTrackerNode();
    if (!node) {
        return;
    }
    if ( node->getApp() && !node->getApp()->progressUpdate(node, progress) ) {
        _imp->scheduler->abortThreadedTask();
    }
}

static mv::TrackRegionOptions::Mode
motionModelIndexToLivMVMode(int mode_i)
{

    switch (mode_i) {
        case 0:
            return mv::TrackRegionOptions::TRANSLATION;
        case 1:
            return mv::TrackRegionOptions::TRANSLATION_ROTATION;
        case 2:
            return mv::TrackRegionOptions::TRANSLATION_SCALE;
        case 3:
            return mv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE;
        case 4:
            return mv::TrackRegionOptions::AFFINE;
        case 5:
            return mv::TrackRegionOptions::HOMOGRAPHY;
        default:
            return mv::TrackRegionOptions::AFFINE;
    }
}

void
TrackerHelper::trackMarker(const TrackMarkerPtr& mark,
                 TimeValue start,
                 TimeValue end,
                 TimeValue frameStep,
                 const ViewerNodePtr& viewer)
{
    std::list<TrackMarkerPtr> markers;
    markers.push_back(mark);
    trackMarkers(markers, start, end, frameStep, viewer);
}


void
TrackerHelper::trackMarkers(const std::list<TrackMarkerPtr>& markers,
                             TimeValue start,
                             TimeValue end,
                             TimeValue frameStep,
                            const ViewerNodePtr& viewer)
{
    if ( markers.empty() ) {
        Q_EMIT trackingFinished();
        return;
    }

    TrackerParamsProviderPtr provider = _imp->provider.lock();
    if (!provider) {
        Q_EMIT trackingFinished();
        return;
    }

    NodePtr trackerNodeSource = provider->getSourceImageNode();
    NodePtr trackerNodeMask = provider->getMaskImageNode();
    if (!trackerNodeSource) {
        Q_EMIT trackingFinished();
        return;
    }

    ImagePlaneDesc maskImagePlane;
    int maskPlaneIndex = 0;
    if (trackerNodeMask) {
        maskImagePlane = provider->getMaskImagePlane(&maskPlaneIndex);
    }

    // The channels we are going to use for tracking
    bool enabledChannels[3];
    provider->getTrackChannels(&enabledChannels[0], &enabledChannels[1], &enabledChannels[2]);


    double formatWidth, formatHeight;
    Format f;
    trackerNodeSource->getApp()->getProject()->getProjectDefaultFormat(&f);
    formatWidth = f.width();
    formatHeight = f.height();

    bool autoKeyingOnEnabledParamEnabled = provider->canDisableMarkersAutomatically();

    /// The accessor and its cache is local to a track operation, it is wiped once the whole sequence track is finished.
    TrackerFrameAccessorPtr accessor = TrackerFrameAccessor::create(trackerNodeSource, trackerNodeMask, maskImagePlane, maskPlaneIndex, enabledChannels, formatHeight);
    boost::shared_ptr<mv::AutoTrack> trackContext = boost::make_shared<mv::AutoTrack>( accessor.get() );
    std::vector<TrackMarkerAndOptionsPtr> trackAndOptions;
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
    for (std::list<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it, ++trackIndex) {

        if (autoKeyingOnEnabledParamEnabled) {
            (*it)->setEnabledAtTime(start, true);
        }

        TrackMarkerAndOptionsPtr t(new TrackMarkerAndOptions);
        t->natronMarker = *it;

        int mode_i = (*it)->getMotionModelKnob()->getValue();
        mvOptions.mode = motionModelIndexToLivMVMode(mode_i);

        // Set a keyframe on the marker to initialize its position
        {
            KnobDoublePtr centerKnob = (*it)->getCenterKnob();
            std::vector<double> values(2);
            values[0] = centerKnob->getValueAtTime(start, DimIdx(0));
            values[1] = centerKnob->getValueAtTime(start, DimIdx(1));
            centerKnob->setValueAtTimeAcrossDimensions(start, values);
        }

        // For a translation warp, we do not need to add an animation curve for the pattern which stays constant.
        if (mvOptions.mode != libmv::TrackRegionOptions::TRANSLATION) {
            KnobDoublePtr patternCorners[4];
            patternCorners[0] = (*it)->getPatternBtmLeftKnob();
            patternCorners[1] = (*it)->getPatternBtmRightKnob();
            patternCorners[2] = (*it)->getPatternTopRightKnob();
            patternCorners[3] = (*it)->getPatternTopLeftKnob();
            for (int c = 0; c < 4; ++c) {
                KnobDoublePtr k = patternCorners[c];
                std::vector<double> values(2);
                values[0] = k->getValueAtTime(start, DimIdx(0));
                values[1] = k->getValueAtTime(start, DimIdx(1));
                k->setValueAcrossDimensions(values);
            }
        }

        KeyFrameSet userKeys = t->natronMarker->getKeyFrames();


        if ( userKeys.empty() ) {
            // Set a user keyframe on tracking start if the marker does not have any user keys
            t->natronMarker->setKeyFrame(start);
        }

        PreviouslyTrackedFrameSet keyframesOrdered;

        // Make sure to create a marker at the start time
        userKeys.insert(KeyFrame(start,0.));


        // Add a libmv marker for all keyframes
        for (KeyFrameSet::iterator it2 = userKeys.begin(); it2 != userKeys.end(); ++it2) {
            keyframesOrdered.insert( PreviouslyComputedTrackFrame(it2->getTime(), true) );
        }


        //For all already tracked frames which are not keyframes, add them to the AutoTrack too
        std::set<double> centerKeys;
        t->natronMarker->getCenterKeyframes(&centerKeys);

        for (std::set<double>::iterator it2 = centerKeys.begin(); it2 != centerKeys.end(); ++it2) {
            if ( userKeys.find(KeyFrame(TimeValue(*it2), 0)) != userKeys.end() ) {
                continue;
            }

            keyframesOrdered.insert( PreviouslyComputedTrackFrame(TimeValue(*it2), false) );
        }


        // Taken from libmv, only initialize the filter to this amount of frames (max)
        const int max_frames_to_predict_from = 20;
        std::list<mv::Marker> previouslyComputedMarkersOrdered;

        // Find the first keyframe that's not considered to go before start or end
        PreviouslyTrackedFrameSet::iterator prevFramesIt = keyframesOrdered.lower_bound(PreviouslyComputedTrackFrame(start, false));
        if (frameStep < 0) {
            if (prevFramesIt != keyframesOrdered.end()) {

                while (prevFramesIt != keyframesOrdered.end()) {


                    mv::Marker mvMarker;
                    TrackerHelperPrivate::natronTrackerToLibMVTracker(true, enabledChannels, *t->natronMarker, trackIndex, TimeValue(prevFramesIt->frame), frameStep, formatHeight, &mvMarker);
                    

                    if ( ( ( (frameStep > 0) && (prevFramesIt->frame <= start) ) || ( (frameStep < 0) && (prevFramesIt->frame >= start) ) ) ) {

                        if ((int)previouslyComputedMarkersOrdered.size() != max_frames_to_predict_from) {
                            // insert in the front of the list so that the order is reversed
                            previouslyComputedMarkersOrdered.push_front(mvMarker);
                        }

                    }

                    trackContext->AddMarker(mvMarker);
                    ++prevFramesIt;
                }
            }
            // previouslyComputedMarkersOrdered is now ordererd by decreasing order
        } else {

            if (prevFramesIt != keyframesOrdered.end()) {



                while (prevFramesIt != keyframesOrdered.begin()) {

                    mv::Marker mvMarker;
                    TrackerHelperPrivate::natronTrackerToLibMVTracker(true, enabledChannels, *t->natronMarker, trackIndex, TimeValue(prevFramesIt->frame), frameStep, formatHeight, &mvMarker);
                    if ( ( ( (frameStep > 0) && (prevFramesIt->frame <= start) ) || ( (frameStep < 0) && (prevFramesIt->frame >= start) ) ) ) {

                        if ((int)previouslyComputedMarkersOrdered.size() != max_frames_to_predict_from) {
                            // insert in the front of the list so that the order is reversed
                            previouslyComputedMarkersOrdered.push_front(mvMarker);
                        }
                        
                    }

                    trackContext->AddMarker(mvMarker);
                    --prevFramesIt;
                }



                mv::Marker mvMarker;
                TrackerHelperPrivate::natronTrackerToLibMVTracker(true, enabledChannels, *t->natronMarker, trackIndex, TimeValue(prevFramesIt->frame), frameStep, formatHeight, &mvMarker);

                if ( ( ( (frameStep > 0) && (prevFramesIt->frame <= start) ) || ( (frameStep < 0) && (prevFramesIt->frame >= start) ) ) ) {

                    if (prevFramesIt == keyframesOrdered.begin()) {

                        if ((int)previouslyComputedMarkersOrdered.size() != max_frames_to_predict_from) {
                            // insert in the front of the list so that the order is reversed
                            previouslyComputedMarkersOrdered.push_front(mvMarker);
                        }

                    }
                }

                trackContext->AddMarker(mvMarker);


            }
            // previouslyComputedMarkersOrdered is now ordererd by increasing order
        } // frameStep < 0
        
        
        // There must be at least 1 marker at the start time
        assert( !previouslyComputedMarkersOrdered.empty() );

        // Initialise the kalman state with the marker at the position

        std::list<mv::Marker>::iterator mIt = previouslyComputedMarkersOrdered.begin();
        t->mvState.Init(*mIt, frameStep);
        ++mIt;
        for (; mIt != previouslyComputedMarkersOrdered.end(); ++mIt) {
            mv::Marker predictedMarker;
            if ( !t->mvState.PredictForward(mIt->frame, &predictedMarker) ) {
                break;
            } else {
                t->mvState.Update(*mIt);
            }
        }


        t->mvOptions = mvOptions;
        trackAndOptions.push_back(t);
    }
    
    
    /*
     Launch tracking in the scheduler thread.
     */

#ifdef BOOST_NO_CXX11_VARIADIC_TEMPLATES
    boost::shared_ptr<TrackArgs> args( new TrackArgs(start, end, frameStep, trackerNodeSource->getApp()->getTimeLine(), viewer, trackContext, accessor, trackAndOptions, formatWidth, formatHeight, autoKeyingOnEnabledParamEnabled) );
#else
    boost::shared_ptr<TrackArgs> args = boost::make_shared<TrackArgs>(start, end, frameStep, trackerNodeSource->getApp()->getTimeLine(), viewer, trackContext, accessor, trackAndOptions, formatWidth, formatHeight, autoKeyingOnEnabledParamEnabled);
#endif
    _imp->scheduler->track(args);
} // TrackerHelper::trackMarkers



NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TrackerHelper.cpp"

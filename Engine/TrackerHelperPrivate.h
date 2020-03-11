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

#ifndef Natron_Engine_TrackerHelperPrivate_H
#define Natron_Engine_TrackerHelperPrivate_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#include "Global/Macros.h"

GCC_DIAG_OFF(unused-function)
GCC_DIAG_OFF(unused-parameter)
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_OFF(maybe-uninitialized)
#endif
#include <libmv/autotrack/autotrack.h>
#include <libmv/autotrack/predict_tracks.h>
#include <libmv/logging/logging.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <openMVG/robust_estimation/robust_estimator_Prosac.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_ON(maybe-uninitialized)
#endif

#include "Engine/TrackMarker.h"
#include "Engine/TrackerParamsProvider.h"
#include "Engine/TrackerFrameAccessor.h"
#include "Engine/TrackScheduler.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

enum libmv_MarkerChannelEnum
{
    LIBMV_MARKER_CHANNEL_R = (1 << 0),
    LIBMV_MARKER_CHANNEL_G = (1 << 1),
    LIBMV_MARKER_CHANNEL_B = (1 << 2),
};

class TrackMarkerAndOptions
{
public:
    TrackMarkerPtr natronMarker;
    mv::Marker mvMarker;
    mv::TrackRegionOptions mvOptions;
    mv::KalmanFilterState mvState;
};



struct PreviouslyComputedTrackFrame
{
    TimeValue frame;
    bool isUserKey;

    PreviouslyComputedTrackFrame()
    : frame(0), isUserKey(false) {}

    PreviouslyComputedTrackFrame(TimeValue f,
                                 bool b)
    : frame(f), isUserKey(b) {}
};

struct PreviouslyComputedTrackFrameCompareLess
{
    bool operator() (const PreviouslyComputedTrackFrame& lhs,
                     const PreviouslyComputedTrackFrame& rhs) const
    {
        return lhs.frame < rhs.frame;
    }
};

typedef std::set<PreviouslyComputedTrackFrame, PreviouslyComputedTrackFrameCompareLess> PreviouslyTrackedFrameSet;


class TrackerHelperPrivate
{

public:


    TrackerHelper* _publicInterface;
    TrackerParamsProviderWPtr provider;
    boost::scoped_ptr<TrackScheduler> scheduler;
   


    TrackerHelperPrivate(TrackerHelper* publicInterface,
                          const TrackerParamsProviderPtr &provider);


    /// Make all calls to getValue() that are global to the tracker context in here
    void beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const;

    static void natronTrackerToLibMVTracker(bool isReferenceMarker,
                                            bool trackChannels[3],
                                            const TrackMarker &marker,
                                            int trackIndex,
                                            TimeValue time,
                                            TimeValue frameStep,
                                            double formatHeight,
                                            mv::Marker * mvMarker);
    static void setKnobKeyframesFromMarker(int formatHeight,
                                           const libmv::TrackRegionResult* result,
                                           const TrackMarkerAndOptionsPtr& options);

    static bool trackStepLibMV(int trackIndex, const TrackArgs& args, int time);
    static bool trackStepTrackerPM(const TrackMarkerPMPtr& tracker, const TrackArgs& args, int time);

};

NATRON_NAMESPACE_EXIT

#endif // Natron_Engine_TrackerHelperPrivate_H

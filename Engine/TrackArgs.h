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


#ifndef TRACKARGS_H
#define TRACKARGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/EngineFwd.h"
#include "Engine/GenericSchedulerThread.h"

namespace mv
{
class AutoTrack;
}

NATRON_NAMESPACE_ENTER;

struct TrackArgsPrivate;
class TrackArgs
: public GenericThreadStartArgs
{
public:

    TrackArgs();

    TrackArgs(int start,
              int end,
              int step,
              const TimeLinePtr& timeline,
              const ViewerInstancePtr& viewer,
              const boost::shared_ptr<mv::AutoTrack>& autoTrack,
              const boost::shared_ptr<TrackerFrameAccessor>& fa,
              const std::vector<TrackMarkerAndOptionsPtr >& tracks,
              double formatWidth,
              double formatHeight,
              bool autoKeyEnabled);

    TrackArgs(const TrackArgs& other);
    void operator=(const TrackArgs& other);

    virtual ~TrackArgs();

    bool isAutoKeyingEnabledParamEnabled() const;

    double getFormatHeight() const;
    double getFormatWidth() const;

    QMutex* getAutoTrackMutex() const;
    int getStart() const;

    int getEnd() const;

    int getStep() const;

    TimeLinePtr getTimeLine() const;
    ViewerInstancePtr getViewer() const;

    int getNumTracks() const;
    const std::vector<TrackMarkerAndOptionsPtr >& getTracks() const;
    boost::shared_ptr<mv::AutoTrack> getLibMVAutoTrack() const;

    void getEnabledChannels(bool* r, bool* g, bool* b) const;

    void getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const;

private:
    
    boost::scoped_ptr<TrackArgsPrivate> _imp;
};


NATRON_NAMESPACE_EXIT;

#endif // TRACKARGS_H

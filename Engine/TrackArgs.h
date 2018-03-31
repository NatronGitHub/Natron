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

#ifndef TRACKARGS_H
#define TRACKARGS_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/GenericSchedulerThread.h"

#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

struct TrackArgsPrivate;

class TrackArgsBase
: public GenericThreadStartArgs
{
public:

    TrackArgsBase()
    : GenericThreadStartArgs()
    {
        
    }

    virtual ~TrackArgsBase()
    {

    }

    /**
     * @brief Returns the first frame to track
     **/
    virtual int getStart() const = 0;

    /**
     * @brief Returns the last frame to track
     **/
    virtual int getEnd() const = 0;

    /**
     * @brief Returns the increment between each frame to track
     **/
    virtual int getStep() const = 0;

    /**
     * @brief Returns the number of tracks to track
     **/
    virtual int getNumTracks() const = 0;

    /**
     * @brief Returns in canonicalRects the portions to redraw on the viewer at the given frame
     **/
    virtual void getRedrawAreasNeeded(TimeValue time, std::list<RectD>* canonicalRects) const = 0;

    /**
     * @brief Returns a pointer to the timeline used by the tracker (generally the main application's one)
     **/
    virtual TimeLinePtr getTimeLine() const = 0;

    /**
     * @brief Returns the viewer node from which the tracking operation was launched
     **/
    virtual ViewerNodePtr getViewer() const = 0;

    virtual double getFormatHeight() const = 0;
    virtual double getFormatWidth() const = 0;

};

class TrackArgs
: public TrackArgsBase
{
public:

    TrackArgs();

    TrackArgs(int start,
              int end,
              int step,
              const TimeLinePtr& timeline,
              const ViewerNodePtr& viewer,
              const boost::shared_ptr<mv::AutoTrack>& autoTrack,
              const boost::shared_ptr<TrackerFrameAccessor>& fa,
              const std::vector<TrackMarkerAndOptionsPtr>& tracks,
              double formatWidth,
              double formatHeight,
              bool autoKeyEnabled);

    TrackArgs(const TrackArgs& other);
    void operator=(const TrackArgs& other);

    virtual ~TrackArgs();

    bool isAutoKeyingEnabledParamEnabled() const;

    virtual double getFormatHeight() const OVERRIDE FINAL;
    virtual double getFormatWidth() const OVERRIDE FINAL;

    QMutex* getAutoTrackMutex() const;

    const std::vector<TrackMarkerAndOptionsPtr>& getTracks() const;
    boost::shared_ptr<mv::AutoTrack> getLibMVAutoTrack() const;

    void getEnabledChannels(bool* r, bool* g, bool* b) const;

    ///// Overriden from TrackArgsBase
    virtual int getStart() const OVERRIDE FINAL;
    virtual int getEnd() const OVERRIDE FINAL;
    virtual int getStep() const OVERRIDE FINAL;
    virtual TimeLinePtr getTimeLine() const OVERRIDE FINAL;
    virtual ViewerNodePtr getViewer() const OVERRIDE FINAL;
    virtual void getRedrawAreasNeeded(TimeValue time, std::list<RectD>* canonicalRects) const OVERRIDE FINAL;
    virtual int getNumTracks() const OVERRIDE FINAL;


private:
    
    boost::scoped_ptr<TrackArgsPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // TRACKARGS_H

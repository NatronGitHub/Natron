/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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


#include "TrackArgs.h"

#include "Engine/KnobTypes.h"
#include "Engine/TrackerFrameAccessor.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerHelperPrivate.h"
#include "Global/GlobalDefines.h"

NATRON_NAMESPACE_ENTER

struct TrackArgsPrivate
{
    int start, end;
    int step;
    TimeLinePtr timeline;
    ViewerNodePtr viewer;
    boost::shared_ptr<mv::AutoTrack> libmvAutotrack;
    boost::shared_ptr<TrackerFrameAccessor> fa;
    std::vector<TrackMarkerAndOptionsPtr> tracks;

    //Store the format size because LibMV internally has a top-down Y axis
    double formatWidth, formatHeight;
    mutable QMutex autoTrackMutex;

    bool autoKeyingOnEnabledParamEnabled;

    TrackArgsPrivate()
    : start(0)
    , end(0)
    , step(1)
    , timeline()
    , viewer()
    , libmvAutotrack()
    , fa()
    , tracks()
    , formatWidth(0)
    , formatHeight(0)
    , autoKeyingOnEnabledParamEnabled(false)
    {
    }
};

TrackArgs::TrackArgs()
: _imp( new TrackArgsPrivate() )
{
}

TrackArgs::TrackArgs(int start,
                     int end,
                     int step,
                     const TimeLinePtr& timeline,
                     const ViewerNodePtr& viewer,
                     const boost::shared_ptr<mv::AutoTrack>& autoTrack,
                     const boost::shared_ptr<TrackerFrameAccessor>& fa,
                     const std::vector<TrackMarkerAndOptionsPtr>& tracks,
                     double formatWidth,
                     double formatHeight,
                     bool autoKeyEnabled)
: TrackArgsBase()
, _imp( new TrackArgsPrivate() )
{
    _imp->start = start;
    _imp->end = end;
    _imp->step = step;
    _imp->timeline = timeline;
    _imp->viewer = viewer;
    _imp->libmvAutotrack = autoTrack;
    _imp->fa = fa;
    _imp->tracks = tracks;
    _imp->formatWidth = formatWidth;
    _imp->formatHeight = formatHeight;
    _imp->autoKeyingOnEnabledParamEnabled = autoKeyEnabled;
}

TrackArgs::~TrackArgs()
{
}

TrackArgs::TrackArgs(const TrackArgs& other)
: TrackArgsBase()
, _imp( new TrackArgsPrivate() )
{
    *this = other;
}

void
TrackArgs::operator=(const TrackArgs& other)
{
    _imp->start = other._imp->start;
    _imp->end = other._imp->end;
    _imp->step = other._imp->step;
    _imp->timeline = other._imp->timeline;
    _imp->viewer = other._imp->viewer;
    _imp->libmvAutotrack = other._imp->libmvAutotrack;
    _imp->fa = other._imp->fa;
    _imp->tracks = other._imp->tracks;
    _imp->formatWidth = other._imp->formatWidth;
    _imp->formatHeight = other._imp->formatHeight;
    _imp->autoKeyingOnEnabledParamEnabled = other._imp->autoKeyingOnEnabledParamEnabled;
}

bool
TrackArgs::isAutoKeyingEnabledParamEnabled() const
{
    return _imp->autoKeyingOnEnabledParamEnabled;
}

double
TrackArgs::getFormatHeight() const
{
    return _imp->formatHeight;
}

double
TrackArgs::getFormatWidth() const
{
    return _imp->formatWidth;
}

QMutex*
TrackArgs::getAutoTrackMutex() const
{
    return &_imp->autoTrackMutex;
}

int
TrackArgs::getStart() const
{
    return _imp->start;
}

int
TrackArgs::getEnd() const
{
    return _imp->end;
}

int
TrackArgs::getStep() const
{
    return _imp->step;
}

TimeLinePtr
TrackArgs::getTimeLine() const
{
    return _imp->timeline;
}

ViewerNodePtr
TrackArgs::getViewer() const
{
    return _imp->viewer;
}

int
TrackArgs::getNumTracks() const
{
    return (int)_imp->tracks.size();
}

const std::vector<TrackMarkerAndOptionsPtr>&
TrackArgs::getTracks() const
{
    return _imp->tracks;
}

boost::shared_ptr<mv::AutoTrack>
TrackArgs::getLibMVAutoTrack() const
{
    return _imp->libmvAutotrack;
}

void
TrackArgs::getEnabledChannels(bool* r,
                              bool* g,
                              bool* b) const
{
    _imp->fa->getEnabledChannels(r, g, b);
}

void
TrackArgs::getRedrawAreasNeeded(TimeValue time,
                                std::list<RectD>* canonicalRects) const
{
    for (std::vector<TrackMarkerAndOptionsPtr>::const_iterator it = _imp->tracks.begin(); it != _imp->tracks.end(); ++it) {
        if ( !(*it)->natronMarker->isEnabled(time) ) {
            continue;
        }
        KnobDoublePtr searchBtmLeft = (*it)->natronMarker->getSearchWindowBottomLeftKnob();
        KnobDoublePtr searchTopRight = (*it)->natronMarker->getSearchWindowTopRightKnob();
        KnobDoublePtr centerKnob = (*it)->natronMarker->getCenterKnob();
        KnobDoublePtr offsetKnob = (*it)->natronMarker->getOffsetKnob();
        Point offset, center, btmLeft, topRight;
        offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
        offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

        center.x = centerKnob->getValueAtTime(time, DimIdx(0));
        center.y = centerKnob->getValueAtTime(time, DimIdx(1));

        btmLeft.x = searchBtmLeft->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
        btmLeft.y = searchBtmLeft->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

        topRight.x = searchTopRight->getValueAtTime(time, DimIdx(0)) + center.x + offset.x;
        topRight.y = searchTopRight->getValueAtTime(time, DimIdx(1)) + center.y + offset.y;

        RectD rect;
        rect.x1 = btmLeft.x;
        rect.y1 = btmLeft.y;
        rect.x2 = topRight.x;
        rect.y2 = topRight.y;
        canonicalRects->push_back(rect);
    }
}

NATRON_NAMESPACE_EXIT

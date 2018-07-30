/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://natrongithub.github.io/>,
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

#include "PyTracker.h"

#include "Engine/PyNode.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TrackerParamsProvider.h"

NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

Track::Track(const TrackMarkerPtr& marker)
: ItemBase(marker)
, _marker(marker)
{
}

Track::~Track()
{
}

void
Track::reset()
{
    TrackMarkerPtr marker = getInternalMarker();
    if (!marker) {
        PythonSetNullError();
        return;
    }
    marker->resetTrack();
}

Tracker::Tracker(const KnobItemsTablePtr& ctx, const TrackerHelperPtr& tracker)
: ItemsTable(ctx)
, _tracker(tracker)
{
}

Tracker::~Tracker()
{
}

void
Tracker::startTracking(const std::list<Track*>& marks,
                       int start,
                       int end,
                       bool forward)
{
    TrackerHelperPtr tracker = _tracker.lock();
    if (!tracker) {
        PythonSetNullError();
        return;
    }

    std::list<TrackMarkerPtr> markers;
    for (std::list<Track*>::const_iterator it = marks.begin(); it != marks.end(); ++it) {
        markers.push_back( (*it)->getInternalMarker() );
    }

    tracker->trackMarkers(markers, TimeValue(start), TimeValue(end), TimeValue(forward), ViewerNodePtr());
}

void
Tracker::stopTracking()
{
    TrackerHelperPtr tracker = _tracker.lock();
    if (!tracker) {
        PythonSetNullError();
        return;
    }
    tracker->abortTracking();
}

Track*
Tracker::createTrack()
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    TrackMarkerPtr track = TrackMarker::create(model);
    track->resetCenter();
    model->addItem(track, KnobTableItemPtr(), eTableChangeReasonInternal);
    Track* ret = dynamic_cast<Track*>( ItemsTable::createPyItemWrapper(track) );
    assert(ret);
    return ret;
}


NATRON_PYTHON_NAMESPACE_EXIT


NATRON_NAMESPACE_EXIT

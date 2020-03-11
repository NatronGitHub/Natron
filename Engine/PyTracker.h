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

#ifndef Engine_PyTracker_H
#define Engine_PyTracker_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include "Engine/PyItemsTable.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class Track : public ItemBase
{
public:

    Track(const TrackMarkerPtr& marker);

    ~Track();

    TrackMarkerPtr getInternalMarker() const
    {
        return _marker.lock();
    }

    void reset();

private:


    TrackMarkerWPtr _marker;
};

class Tracker : public ItemsTable
{
public:

    Tracker(const KnobItemsTablePtr& ctx, const TrackerHelperPtr& tracker);

    virtual ~Tracker();


    Track* createTrack();

    void startTracking(const std::list<Track*>& marks,
                       int start,
                       int end,
                       bool forward);

    void stopTracking();

private:

    TrackerHelperWPtr _tracker;

};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // Engine_PyTracker_H

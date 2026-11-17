/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2023 The Natron developers
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

#ifndef Engine_PyTracker_H
#define Engine_PyTracker_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>

#include "Engine/PyParameter.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

class Track
{
public:

    Track(const TrackMarkerPtr& marker);

    ~Track();

    TrackMarkerPtr getInternalMarker() const
    {
        return _marker.lock();
    }

    void setScriptName(const QString& scriptName);
    QString getScriptName() const;

    Param* getParam(const QString& scriptName) const;

    std::list<Param*> getParams() const;

    void reset();

private:

    TrackMarkerWPtr _marker;
};

class Tracker
{
public:

    Tracker(const TrackerContextPtr& ctx);

    ~Tracker();

    TrackerContextPtr getInternalContext() const
    {
        return _ctx.lock();
    }

    Track* getTrackByName(const QString& name) const;

    void getAllTracks(std::list<Track*>* markers) const;

    void getSelectedTracks(std::list<Track*>* markers) const;

    Track* createTrack();

    void startTracking(const std::list<Track*>& marks,
                       int start,
                       int end,
                       bool forward);

    void stopTracking();

private:

    TrackerContextWPtr _ctx;
};

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;

#endif // Engine_PyTracker_H

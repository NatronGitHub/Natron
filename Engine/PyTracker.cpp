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

#include "PyTracker.h"

#include "Engine/PyNode.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerContext.h"


NATRON_NAMESPACE_ENTER
NATRON_PYTHON_NAMESPACE_ENTER

Track::Track(const TrackMarkerPtr& marker)
    : _marker(marker)
{
}

Track::~Track()
{
}

void
Track::setScriptName(const QString& scriptName)
{
    TrackMarkerPtr marker = getInternalMarker();
    if (!marker) {
        return;
    }
    marker->setScriptName( scriptName.toStdString() );
}

QString
Track::getScriptName() const
{
    TrackMarkerPtr marker = getInternalMarker();
    if (!marker) {
        return QString();
    }
    return QString::fromUtf8( marker->getScriptName_mt_safe().c_str() );
}

Param*
Track::getParam(const QString& scriptName) const
{
    TrackMarkerPtr marker = getInternalMarker();
    if (!marker) {
        return 0;
    }
    KnobIPtr knob = marker->getKnobByName( scriptName.toStdString() );

    if (!knob) {
        return 0;
    }
    Param* ret = Effect::createParamWrapperForKnob(knob);

    return ret;
}

std::list<Param*>
Track::getParams() const
{
    
    std::list<Param*> ret;
    TrackMarkerPtr marker = getInternalMarker();
    if (!marker) {
        return ret;
    }
    const KnobsVec& knobs = marker->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = Effect::createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }

    return ret;
}

void
Track::reset()
{
    TrackMarkerPtr marker = getInternalMarker();
    if (!marker) {
        return;
    }
    marker->resetTrack();
}

Tracker::Tracker(const TrackerContextPtr& ctx)
    : _ctx(ctx)
{
}

Tracker::~Tracker()
{
}

Track*
Tracker::getTrackByName(const QString& name) const
{
    TrackerContextPtr ctx = getInternalContext();
    if (!ctx) {
        return 0;
    }
    TrackMarkerPtr t = ctx->getMarkerByName( name.toStdString() );

    if (t) {
        return new Track(t);
    } else {
        return 0;
    }
}

void
Tracker::startTracking(const std::list<Track*>& marks,
                       int start,
                       int end,
                       bool forward)
{
    TrackerContextPtr ctx = getInternalContext();
    if (!ctx) {
        return;
    }
    std::list<TrackMarkerPtr> markers;

    for (std::list<Track*>::const_iterator it = marks.begin(); it != marks.end(); ++it) {
        markers.push_back( (*it)->getInternalMarker() );
    }
    ctx->trackMarkers(markers, start, end, forward, 0);
}

void
Tracker::stopTracking()
{
    TrackerContextPtr ctx = getInternalContext();
    if (!ctx) {
        return;
    }
    ctx->abortTracking();
}

void
Tracker::getAllTracks(std::list<Track*>* tracks) const
{
    TrackerContextPtr ctx = getInternalContext();
    if (!ctx) {
        return;
    }
    std::vector<TrackMarkerPtr> markers;

    ctx->getAllMarkers(&markers);
    for (std::vector<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it) {
        tracks->push_back( new Track(*it) );
    }
}

void
Tracker::getSelectedTracks(std::list<Track*>* tracks) const
{
    TrackerContextPtr ctx = getInternalContext();
    if (!ctx) {
        return;
    }
    std::list<TrackMarkerPtr> markers;

    ctx->getSelectedMarkers(&markers);
    for (std::list<TrackMarkerPtr>::const_iterator it = markers.begin(); it != markers.end(); ++it) {
        tracks->push_back( new Track(*it) );
    }
}

Track*
Tracker::createTrack()
{
    TrackerContextPtr ctx = getInternalContext();
    if (!ctx) {
        return 0;
    }
    TrackMarkerPtr track = ctx->createMarker();
    if (!track) {
        return 0;
    }
    return new Track(track);
}

NATRON_PYTHON_NAMESPACE_EXIT
NATRON_NAMESPACE_EXIT

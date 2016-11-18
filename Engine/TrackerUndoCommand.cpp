/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

#include "TrackerUndoCommand.h"

#include "Engine/KnobItemsTable.h"
#include "Engine/Node.h"
#include "Engine/AppInstance.h"
#include "Engine/TrackMarker.h"


NATRON_NAMESPACE_ENTER;

AddTrackCommand::AddTrackCommand(const TrackMarkerPtr &marker)
    : UndoCommand()
    , _isFirstRedo(true)
{
    int index = marker->getIndexInParent();
    _markers[index] = marker;
    setText( tr("Add Track(s)").toStdString() );
}

void
AddTrackCommand::undo()
{

    assert(!_markers.empty());
    KnobItemsTablePtr model = _markers.begin()->second->getModel();
    model->beginEditSelection();
    for (std::map<int, TrackMarkerPtr >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        model->removeItem(it->second, eTableChangeReasonInternal);
    }
    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
}

void
AddTrackCommand::redo()
{

    KnobItemsTablePtr model = _markers.begin()->second->getModel();

    model->beginEditSelection();
    model->clearSelection(eTableChangeReasonInternal);

    if (!_isFirstRedo) {
        for (std::map<int, TrackMarkerPtr >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
            model->insertItem(it->first, it->second, KnobTableItemPtr(), eTableChangeReasonInternal);
        }
    }
    for (std::map<int, TrackMarkerPtr >::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        model->addToSelection(it->second, eTableChangeReasonInternal);
    }

    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
    _isFirstRedo = false;
}

RemoveTracksCommand::RemoveTracksCommand(const std::list<TrackMarkerPtr > &markers)
    : UndoCommand()
    , _markers()
{
    assert( !markers.empty() );
    for (std::list<TrackMarkerPtr >::const_iterator it = markers.begin(); it != markers.end(); ++it) {
        TrackToRemove t;
        t.track = *it;
        KnobItemsTablePtr model = (*it)->getModel();
        t.prevTrack = toTrackMarker(model->getTopLevelItem((*it)->getIndexInParent() - 1));
        _markers.push_back(t);
    }
    setText( tr("Remove Track(s)").toStdString() );
}

void
RemoveTracksCommand::undo()
{
    KnobItemsTablePtr model = _markers.begin()->track->getModel();
    model->beginEditSelection();
    model->clearSelection(eTableChangeReasonInternal);
    for (std::list<TrackToRemove>::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        int prevIndex = -1;
        TrackMarkerPtr prevMarker = it->prevTrack.lock();
        if (prevMarker) {
            prevIndex = prevMarker->getIndexInParent();
        }
        if (prevIndex != -1) {
            model->insertItem(prevIndex, it->track, KnobTableItemPtr(), eTableChangeReasonInternal);
        } else {
            model->addItem(it->track, KnobTableItemPtr(), eTableChangeReasonInternal);
        }
        model->addToSelection(it->track, eTableChangeReasonInternal);
    }
    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
}

void
RemoveTracksCommand::redo()
{
    KnobItemsTablePtr model = _markers.begin()->track->getModel();

    KnobTableItemPtr nextMarker;
    {
        KnobTableItemPtr marker = _markers.back().track;
        assert(marker);
        int index = marker->getIndexInParent();
        std::vector<KnobTableItemPtr> topLevel = model->getTopLevelItems();
        if (!topLevel.empty()) {
            if (index + 1 < (int)topLevel.size()) {
                nextMarker = topLevel[index + 1];
            } else {
                if (topLevel[0] != marker) {
                    nextMarker = topLevel[0];
                }
            }
        }
    }

    model->beginEditSelection();
    for (std::list<TrackToRemove>::const_iterator it = _markers.begin(); it != _markers.end(); ++it) {
        model->removeItem(it->track, eTableChangeReasonInternal);
    }
    if (nextMarker) {
        model->addToSelection(nextMarker, eTableChangeReasonInternal);
    }
    model->endEditSelection(eTableChangeReasonInternal);
    model->getNode()->getApp()->triggerAutoSave();
}

NATRON_NAMESPACE_EXIT;

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2021 The Natron developers
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

#ifndef TRACKERUNDOCOMMAND_H
#define TRACKERUNDOCOMMAND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <list>
#include <map>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

CLANG_DIAG_OFF(deprecated)
CLANG_DIAG_OFF(uninitialized)
#include <QtCore/QCoreApplication>
CLANG_DIAG_ON(deprecated)
CLANG_DIAG_ON(uninitialized)

#include "Engine/EngineFwd.h"
#include "Engine/UndoCommand.h"


NATRON_NAMESPACE_ENTER

class AddTrackCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(AddTrackCommand)

public:

    AddTrackCommand(const TrackMarkerPtr &marker,
                    const TrackerContextPtr& context);

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
    bool _isFirstRedo;
    std::map<int, TrackMarkerPtr> _markers;
    TrackerContextWPtr _context;
};


class RemoveTracksCommand
    : public UndoCommand
{
    Q_DECLARE_TR_FUNCTIONS(RemoveTracksCommand)

public:

    RemoveTracksCommand(const std::list<TrackMarkerPtr> &markers,
                        const TrackerContextPtr& context);

    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;

private:

    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
    struct TrackToRemove
    {
        TrackMarkerPtr track;
        TrackMarkerWPtr prevTrack;
    };

    std::list<TrackToRemove> _markers;
    TrackerContextWPtr _context;
};

NATRON_NAMESPACE_EXIT


#endif // TRACKERUNDOCOMMAND_H

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



#ifndef TRACKERUNDOCOMMAND_H
#define TRACKERUNDOCOMMAND_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#endif

#include <list>
#include <QUndoCommand>

#include "Global/Macros.h"
#include "Gui/GuiFwd.h"


NATRON_NAMESPACE_ENTER;

class AddTrackCommand
: public QUndoCommand
{
    
public:
    
    AddTrackCommand(const boost::shared_ptr<TrackMarker> &marker, const boost::shared_ptr<TrackerContext>& context);
    
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    
    
private:
    
    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
   
    std::list<boost::shared_ptr<TrackMarker> > _markers;
    boost::weak_ptr<TrackerContext> _context;
    
};


class RemoveTracksCommand
: public QUndoCommand
{

public:
    
    RemoveTracksCommand(const std::list<boost::shared_ptr<TrackMarker> > &markers, const boost::shared_ptr<TrackerContext>& context);
    
    virtual void undo() OVERRIDE FINAL;
    virtual void redo() OVERRIDE FINAL;
    

private:
    
    //Hold shared_ptrs otherwise no one is holding a shared_ptr while items are removed from the context
    struct TrackToRemove
    {
        boost::shared_ptr<TrackMarker> track;
        boost::weak_ptr<TrackMarker> prevTrack;
    };
    
    std::list<TrackToRemove> _markers;
    boost::weak_ptr<TrackerContext> _context;
    
};

NATRON_NAMESPACE_EXIT;


#endif // TRACKERUNDOCOMMAND_H

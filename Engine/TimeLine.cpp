/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
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

#include "TimeLine.h"

#ifndef NDEBUG
#include <QThread>
#include <QCoreApplication>
#endif

#include <cassert>
#include "Engine/Project.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"

TimeLine::TimeLine(Natron::Project* project)
: _project(project)
, _currentFrame(1)
{
}


SequenceTime
TimeLine::currentFrame() const
{
    QMutexLocker l(&_lock);

    return _currentFrame;
}


void
TimeLine::seekFrame(SequenceTime frame,
                    bool updateLastCaller,
                    Natron::OutputEffectInstance* caller,
                    Natron::TimelineChangeReasonEnum reason)
{
    if (reason != Natron::eTimelineChangeReasonPlaybackSeek) {
        Q_EMIT frameAboutToChange();
    }
    bool changed = false;
    {
        QMutexLocker l(&_lock);
        if (_currentFrame != frame) {
            _currentFrame = frame;
            changed = true;
        }
    }

    if (_project && updateLastCaller) {
        _project->getApp()->setLastViewerUsingTimeline(caller ? caller->getNode() : boost::shared_ptr<Natron::Node>());
    }
    if (changed) {
        Q_EMIT frameChanged(frame, (int)reason);
    }
}

void
TimeLine::incrementCurrentFrame()
{
    SequenceTime frame;
    {
        QMutexLocker l(&_lock);
        ++_currentFrame;
        frame = _currentFrame;
    }

    Q_EMIT frameChanged(frame, (int)Natron::eTimelineChangeReasonPlaybackSeek);
}

void
TimeLine::decrementCurrentFrame()
{
    SequenceTime frame;
    {
        QMutexLocker l(&_lock);
        --_currentFrame;
        frame = _currentFrame;
    }

    Q_EMIT frameChanged(frame, (int)Natron::eTimelineChangeReasonPlaybackSeek);
}

void
TimeLine::onFrameChanged(SequenceTime frame)
{
   
    Q_EMIT frameAboutToChange();
    
    bool changed = false;
    {
        QMutexLocker l(&_lock);
        if (_currentFrame != frame) {
            _currentFrame = frame;
            changed = true;
        }
    }

    if (changed) {
        /*This function is called in response to a signal emitted by a single timeline gui, but we also
           need to sync all the other timelines potentially existing.*/
        Q_EMIT frameChanged(frame, (int)Natron::eTimelineChangeReasonUserSeek);
    }
}



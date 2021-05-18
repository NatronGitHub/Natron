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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TimeLine.h"

#include <cassert>
#include <stdexcept>

#ifndef NDEBUG
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#endif

#include "Engine/Project.h"
#include "Engine/AppInstance.h"
#include "Engine/Node.h"
#include "Engine/EffectInstance.h"

NATRON_NAMESPACE_ENTER

TimeLine::TimeLine(Project* project)
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
                    OutputEffectInstance* caller,
                    TimelineChangeReasonEnum reason)
{
    if (reason != eTimelineChangeReasonPlaybackSeek) {
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
        _project->getApp()->setLastViewerUsingTimeline( caller ? caller->getNode() : NodePtr() );
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
    Q_EMIT frameChanged(frame, (int)eTimelineChangeReasonPlaybackSeek);
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
    Q_EMIT frameChanged(frame, (int)eTimelineChangeReasonPlaybackSeek);
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
        Q_EMIT frameChanged(frame, (int)eTimelineChangeReasonUserSeek);
    }
}

NATRON_NAMESPACE_EXIT

NATRON_NAMESPACE_USING
#include "moc_TimeLine.cpp"

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
: _currentFrame(1)
, _keyframes()
, _project(project)
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

void
TimeLine::removeAllKeyframesIndicators()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    bool wasEmpty = _keyframes.empty();
    _keyframes.clear();
    if (!wasEmpty) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
TimeLine::addKeyframeIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _keyframes.push_back(time);
    Q_EMIT keyframeIndicatorsChanged();
}

void
TimeLine::addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys,
                                             bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _keyframes.insert( _keyframes.begin(),keys.begin(),keys.end() );
    if (!keys.empty() && emitSignal) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
TimeLine::removeKeyFrameIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<SequenceTime>::iterator it = std::find(_keyframes.begin(), _keyframes.end(), time);
    if ( it != _keyframes.end() ) {
        _keyframes.erase(it);
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
TimeLine::removeMultipleKeyframeIndicator(const std::list<SequenceTime> & keys,
                                          bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    for (std::list<SequenceTime>::const_iterator it = keys.begin(); it != keys.end(); ++it) {
        std::list<SequenceTime>::iterator it2 = std::find(_keyframes.begin(), _keyframes.end(), *it);
        if ( it2 != _keyframes.end() ) {
            _keyframes.erase(it2);
        }
    }
    if (!keys.empty() && emitSignal) {
        Q_EMIT keyframeIndicatorsChanged();
    }
}

void
TimeLine::addNodesKeyframesToTimeline(const std::list<Natron::Node*> & nodes)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<Natron::Node*>::const_iterator next = nodes.begin();
    if (next != nodes.end()) {
        ++next;
    }
    for (std::list<Natron::Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->showKeyframesOnTimeline( next == nodes.end() );

        // increment for next iteration
        if (next != nodes.end()) {
            ++next;
        }
    } // for()
}

void
TimeLine::addNodeKeyframesToTimeline(Natron::Node* node)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    node->showKeyframesOnTimeline(true);
}

void
TimeLine::removeNodesKeyframesFromTimeline(const std::list<Natron::Node*> & nodes)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<Natron::Node*>::const_iterator next = nodes.begin();
    if (next != nodes.end()) {
        ++next;
    }
    for (std::list<Natron::Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        (*it)->hideKeyframesFromTimeline( next == nodes.end() );

        // increment for next iteration
        if (next != nodes.end()) {
            ++next;
        }
    } // for(it)
}

void
TimeLine::removeNodeKeyframesFromTimeline(Natron::Node* node)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    node->hideKeyframesFromTimeline(true);
}

void
TimeLine::getKeyframes(std::list<SequenceTime>* keys) const
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    *keys = _keyframes;
}

void
TimeLine::goToPreviousKeyframe()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _keyframes.sort();
    std::list<SequenceTime>::iterator lowerBound = std::lower_bound(_keyframes.begin(), _keyframes.end(), _currentFrame);
    if ( lowerBound != _keyframes.begin() ) {
        --lowerBound;
        seekFrame(*lowerBound, true, NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}

void
TimeLine::goToNextKeyframe()
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _keyframes.sort();
    std::list<SequenceTime>::iterator upperBound = std::upper_bound(_keyframes.begin(), _keyframes.end(), _currentFrame);
    if ( upperBound != _keyframes.end() ) {
        seekFrame(*upperBound, true, NULL, Natron::eTimelineChangeReasonPlaybackSeek);
    }
}


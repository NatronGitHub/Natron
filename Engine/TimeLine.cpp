//
//  TimeLine.cpp
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#include "TimeLine.h"

#ifndef NDEBUG
#include <QThread>
#include <QCoreApplication>
#endif

#include <cassert>
#include "Engine/Project.h"
#include "Engine/Node.h"

TimeLine::TimeLine(Natron::Project* project)
    : _firstFrame(1)
      , _lastFrame(1)
      , _currentFrame(1)
      , _leftBoundary(_firstFrame)
      , _rightBoundary(_lastFrame)
      , _keyframes()
      , _project(project)
{
}

SequenceTime
TimeLine::firstFrame() const
{
    QMutexLocker l(&_lock);

    return _firstFrame;
}

SequenceTime
TimeLine::lastFrame() const
{
    QMutexLocker l(&_lock);

    return _lastFrame;
}

SequenceTime
TimeLine::currentFrame() const
{
    QMutexLocker l(&_lock);

    return _currentFrame;
}

SequenceTime
TimeLine::leftBound() const
{
    QMutexLocker l(&_lock);

    return _leftBoundary;
}

SequenceTime
TimeLine::rightBound() const
{
    QMutexLocker l(&_lock);

    return _rightBoundary;
}

void
TimeLine::setFrameRange(SequenceTime first,
                        SequenceTime last)
{
    bool changed = false;
    {
        QMutexLocker l(&_lock);
        if ( (_firstFrame != first) || (_lastFrame != last) ) {
            _firstFrame = first;
            _lastFrame = last;
            changed = true;
        }
    }

    if (changed) {
        emit frameRangeChanged(first, last);
        setBoundaries(first,last);
    }
}

void
TimeLine::seekFrame(SequenceTime frame,
                    Natron::OutputEffectInstance* caller)
{
    bool changed = false;
    {
        QMutexLocker l(&_lock);
        if (_currentFrame != frame) {
            _currentFrame = frame;
            changed = true;
        }
    }

    if (changed) {
        if (_project) {
            _project->setLastTimelineSeekCaller(caller);
        }
        emit frameChanged(frame, (int)Natron::PLAYBACK_SEEK);
    }
}

void
TimeLine::incrementCurrentFrame(Natron::OutputEffectInstance* caller)
{
    SequenceTime frame;
    {
        QMutexLocker l(&_lock);
        ++_currentFrame;
        frame = _currentFrame;
    }
    if (_project) {
        _project->setLastTimelineSeekCaller(caller);
    }
    emit frameChanged(frame, (int)Natron::PLAYBACK_SEEK);
}

void
TimeLine::decrementCurrentFrame(Natron::OutputEffectInstance* caller)
{
    SequenceTime frame;
    {
        QMutexLocker l(&_lock);
        --_currentFrame;
        frame = _currentFrame;
    }
    if (_project) {
        _project->setLastTimelineSeekCaller(caller);
    }
    emit frameChanged(frame, (int)Natron::PLAYBACK_SEEK);
}

void
TimeLine::onFrameChanged(SequenceTime frame)
{
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
        emit frameChanged(frame, (int)Natron::USER_SEEK);
    }
}

void
TimeLine::setBoundaries(SequenceTime leftBound,
                        SequenceTime rightBound)
{
    bool changed = false;
    {
        QMutexLocker l(&_lock);
        if ( (_leftBoundary != leftBound) || (_rightBoundary != rightBound) ) {
            _leftBoundary = leftBound;
            _rightBoundary = rightBound;
            changed = true;
        }
    }

    if (changed) {
        emit boundariesChanged(leftBound, rightBound, Natron::PLUGIN_EDITED);
    }
}

// the reason (last line) differs in this version
void
TimeLine::onBoundariesChanged(SequenceTime leftBound,
                              SequenceTime rightBound)
{
    bool changed = false;
    {
        QMutexLocker l(&_lock);
        if ( (_leftBoundary != leftBound) || (_rightBoundary != rightBound) ) {
            _leftBoundary = leftBound;
            _rightBoundary = rightBound;
            changed = true;
        }
    }

    if (changed) {
        emit boundariesChanged(leftBound, rightBound, Natron::USER_EDITED);
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
        emit keyframeIndicatorsChanged();
    }
}

void
TimeLine::addKeyframeIndicator(SequenceTime time)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _keyframes.push_back(time);
    emit keyframeIndicatorsChanged();
}

void
TimeLine::addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & keys,
                                             bool emitSignal)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    _keyframes.insert( _keyframes.begin(),keys.begin(),keys.end() );
    if (!keys.empty() && emitSignal) {
        emit keyframeIndicatorsChanged();
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
        emit keyframeIndicatorsChanged();
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
        emit keyframeIndicatorsChanged();
    }
}

void
TimeLine::addNodesKeyframesToTimeline(const std::list<Natron::Node*> & nodes)
{
    ///runs only in the main thread
    assert( QThread::currentThread() == qApp->thread() );

    std::list<Natron::Node*>::const_iterator next = nodes.begin();
    ++next;
    for (std::list<Natron::Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it,++next) {
        (*it)->showKeyframesOnTimeline( next == nodes.end() );
    }
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
    ++next;
    for (std::list<Natron::Node*>::const_iterator it = nodes.begin(); it != nodes.end(); ++it,++next) {
        (*it)->hideKeyframesFromTimeline( next == nodes.end() );
    }
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
        seekFrame(*lowerBound,NULL);
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
        seekFrame(*upperBound,NULL);
    }
}


//
//  TimeLine.cpp
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#include "TimeLine.h"
#include "Engine/Project.h"
#include <cassert>

TimeLine::TimeLine(Natron::Project* project):
_firstFrame(0)
, _lastFrame(0)
, _currentFrame(0)
, _leftBoundary(_firstFrame)
, _rightBoundary(_lastFrame)
, _project(project)
{}

void TimeLine::setFrameRange(SequenceTime first,SequenceTime last){
    QMutexLocker l(&_lock);
    SequenceTime oldFirst = _firstFrame;
    SequenceTime oldLast = _lastFrame;
    _firstFrame = first;
    _lastFrame = last;
    if(first != oldFirst || last != oldLast){
        emit frameRangeChanged(first, last);
        setBoundaries(first,last);
    }

}

void TimeLine::seekFrame(SequenceTime frame,Natron::OutputEffectInstance* caller){
    QMutexLocker l(&_lock);
    _currentFrame = frame;
    _project->setLastTimelineSeekCaller(caller);
    emit frameChanged(_currentFrame,(int)Natron::PLAYBACK_SEEK);
}

void TimeLine::incrementCurrentFrame(Natron::OutputEffectInstance* caller) { seekFrame(_currentFrame+1,caller); }

void TimeLine::decrementCurrentFrame(Natron::OutputEffectInstance* caller) { seekFrame(_currentFrame-1,caller); }

void TimeLine::onFrameChanged(SequenceTime frame){
    _currentFrame = frame;
    /*This function is called in response to a signal emitted by a single timeline gui, but we also
     need to sync all the other timelines potentially existing.*/
    emit frameChanged(_currentFrame,(int)Natron::USER_SEEK);
}



void TimeLine::setBoundaries(SequenceTime leftBound,SequenceTime rightBound){
    _leftBoundary = leftBound;
    _rightBoundary = rightBound;
    emit boundariesChanged(_leftBoundary,_rightBoundary,Natron::PLUGIN_EDITED);

}

void TimeLine::onBoundariesChanged(SequenceTime left,SequenceTime right){
    _leftBoundary = left;
    _rightBoundary = right;
     emit boundariesChanged(_leftBoundary,_rightBoundary,Natron::USER_EDITED);
}

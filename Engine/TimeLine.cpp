//
//  TimeLine.cpp
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#include "TimeLine.h"

#include <cassert>

TimeLine::TimeLine():
_firstFrame(0)
, _lastFrame(0)
, _currentFrame(0)
, _leftBoundary(_firstFrame)
, _rightBoundary(_lastFrame)
{}

void TimeLine::setFrameRange(SequenceTime first,SequenceTime last){
    SequenceTime oldFirst = _firstFrame;
    SequenceTime oldLast = _lastFrame;
    _firstFrame = first;
    _lastFrame = last;
    if(first != oldFirst || last != oldLast){
        emit frameRangeChanged(first, last);
        setBoundaries(first,last);
    }

}

void TimeLine::seekFrame(SequenceTime frame){
    _currentFrame = frame;
    emit frameChanged(_currentFrame);
}

void TimeLine::onFrameChanged(SequenceTime frame){
    _currentFrame = frame;
    
    /*This function is called in response to a signal emitted by a single timeline gui, but we also
     need to sync all the other timelines potentially existing.*/
    emit frameChanged(_currentFrame);
}

void TimeLine::setBoundaries(SequenceTime leftBound,SequenceTime rightBound){
    _leftBoundary = leftBound;
    _rightBoundary = rightBound;
    emit boundariesChanged(_leftBoundary,_rightBoundary);

}

void TimeLine::onBoundariesChanged(SequenceTime left,SequenceTime right){
    _leftBoundary = left;
    _rightBoundary = right;
}

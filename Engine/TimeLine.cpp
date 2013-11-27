//
//  TimeLine.cpp
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#include "TimeLine.h"

#include <cassert>


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

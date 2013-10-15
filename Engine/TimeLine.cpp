//
//  TimeLine.cpp
//  Powiter
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#include "TimeLine.h"

#include <cassert>

void TimeLine::seekFrame(SequenceTime frame){
    assert(frame <= _lastFrame && frame >= _firstFrame);
    _currentFrame = frame;
    emit frameChanged(_currentFrame);
}

void TimeLine::seekFrame_noEmit(SequenceTime frame){
    assert(frame <= _lastFrame && frame >= _firstFrame);
    _currentFrame = frame;
}

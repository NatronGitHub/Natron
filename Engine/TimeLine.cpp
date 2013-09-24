//
//  TimeLine.cpp
//  Powiter
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#include "TimeLine.h"

#include <cassert>

void TimeLine::seekFrame(int frame){
    assert(frame <= _lastFrame && frame >= _firstFrame);
    _currentFrame = frame;
    emit frameChanged(_currentFrame);
}

void TimeLine::seekFrame_noEmit(int frame){
    assert(frame <= _lastFrame && frame >= _firstFrame);
    _currentFrame = frame;
}

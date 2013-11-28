//
//  TimeLine.h
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#ifndef NATRON_ENGINE_TIMELINE_H_
#define NATRON_ENGINE_TIMELINE_H_

#include <QtCore/QObject>

#include "Global/GlobalDefines.h"

/**
  * @brief A simple TimeLine representing the time for image sequences.
  * The interval [_firstFrame,_lastFrame] represents where images exist in the time space.
  * The interval [_leftBoundary,_rightBoundary] represents what the user interval of interest within the time space.
  * The _currentFrame represents the current time in the time space. It doesn't have to be within any aforementioned interval.
**/
class TimeLine: public QObject {

    Q_OBJECT

public:

    TimeLine();
    
    virtual ~TimeLine(){}

    SequenceTime firstFrame() const {return _firstFrame;}

    SequenceTime lastFrame() const {return _lastFrame;}

    SequenceTime currentFrame() const {return _currentFrame;}

    SequenceTime leftBound() const {return _leftBoundary;}

    SequenceTime rightBound() const {return _rightBoundary;}

    void setFrameRange(SequenceTime first,SequenceTime last);

    void setBoundaries(SequenceTime leftBound,SequenceTime rightBound);

    void seekFrame(SequenceTime frame);

    void incrementCurrentFrame() {++_currentFrame; emit frameChanged(_currentFrame);}

    void decrementCurrentFrame() {--_currentFrame; emit frameChanged(_currentFrame);}

public slots:

    void onFrameChanged(SequenceTime frame);

    void onBoundariesChanged(SequenceTime left,SequenceTime right);

signals:

    void frameRangeChanged(SequenceTime,SequenceTime);
    void boundariesChanged(SequenceTime,SequenceTime);
    void frameChanged(SequenceTime);

private:
    SequenceTime _firstFrame;
    SequenceTime _lastFrame;
    SequenceTime _currentFrame;
    SequenceTime _leftBoundary,_rightBoundary; //these boundaries are within the interval [firstFrame,lastFrame]
};
#endif /* defined(NATRON_ENGINE_TIMELINE_H_) */

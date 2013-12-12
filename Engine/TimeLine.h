//
//  TimeLine.h
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#ifndef NATRON_ENGINE_TIMELINE_H_
#define NATRON_ENGINE_TIMELINE_H_

#include <QtCore/QMutex>
#include <QtCore/QObject>

#include "Global/GlobalDefines.h"

/**
  * @brief A simple TimeLine representing the time for image sequences.
  * The interval [_firstFrame,_lastFrame] represents where images exist in the time space.
  * The interval [_leftBoundary,_rightBoundary] represents what the user interval of interest within the time space.
  * The _currentFrame represents the current time in the time space. It doesn't have to be within any aforementioned interval.
**/
namespace Natron {
class OutputEffectInstance;
class Project;
}

class TimeLine: public QObject {

    Q_OBJECT

public:

    TimeLine(Natron::Project* project);
    
    virtual ~TimeLine(){}

    SequenceTime firstFrame() const {return _firstFrame;}

    SequenceTime lastFrame() const {return _lastFrame;}

    SequenceTime currentFrame() const {return _currentFrame;}

    SequenceTime leftBound() const {return _leftBoundary;}

    SequenceTime rightBound() const {return _rightBoundary;}

    void setFrameRange(SequenceTime first,SequenceTime last);

    void setBoundaries(SequenceTime leftBound,SequenceTime rightBound);

    void seekFrame(SequenceTime frame,Natron::OutputEffectInstance* caller);

    void incrementCurrentFrame(Natron::OutputEffectInstance* caller);

    void decrementCurrentFrame(Natron::OutputEffectInstance* caller) ;

public slots:
    void onFrameChanged(SequenceTime frame);

    void onBoundariesChanged(SequenceTime left,SequenceTime right);

signals:

    void frameRangeChanged(SequenceTime,SequenceTime);
    void boundariesChanged(SequenceTime,SequenceTime,int reason);
    //reason being a Natron::TIMELINE_CHANGE_REASON
    void frameChanged(SequenceTime,int reason);

private:
    SequenceTime _firstFrame;
    SequenceTime _lastFrame;
    SequenceTime _currentFrame;
    SequenceTime _leftBoundary,_rightBoundary; //these boundaries are within the interval [firstFrame,lastFrame]
    mutable QMutex _lock;
    Natron::Project* _project;
};
#endif /* defined(NATRON_ENGINE_TIMELINE_H_) */

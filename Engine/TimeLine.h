//
//  TimeLine.h
//  Powiter
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#ifndef POWITER_ENGINE_TIMELINE_H_
#define POWITER_ENGINE_TIMELINE_H_

#include <QtCore/QObject>


#include "Global/GlobalDefines.h"
class TimeLine: public QObject {
    Q_OBJECT
public:

    TimeLine():
    _firstFrame(0),
    _lastFrame(100),
    _currentFrame(0)
    {}

    virtual ~TimeLine(){}

    SequenceTime firstFrame() const {return _firstFrame;}

    SequenceTime lastFrame() const {return _lastFrame;}

    void setFrameRange(SequenceTime first,SequenceTime last){
        SequenceTime oldFirst = _firstFrame;
        SequenceTime oldLast = _lastFrame;
        _firstFrame = first;
        _lastFrame = last;
        if(first != oldFirst || last != oldLast){
            emit frameRangeChanged(first, last);
        }
       
    }
    
    SequenceTime currentFrame() const {return _currentFrame;}

    void incrementCurrentFrame(){++_currentFrame; emit frameChanged(_currentFrame);}

    void decrementCurrentFrame(){--_currentFrame; emit frameChanged(_currentFrame);}

public slots:

    void seekFrame(SequenceTime frame);

    void seekFrame_noEmit(SequenceTime frame);


signals:
    void frameRangeChanged(int,int);
    void frameChanged(int);

private:
    SequenceTime _firstFrame;
    SequenceTime _lastFrame;
    SequenceTime _currentFrame;
};
#endif /* defined(POWITER_ENGINE_TIMELINE_H_) */

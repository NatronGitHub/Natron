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

#include "Global/Macros.h"

class TimeLine: public QObject {
    Q_OBJECT
public:

    TimeLine():
    _firstFrame(0),
    _lastFrame(100),
    _currentFrame(0)
    {}

    virtual ~TimeLine(){}

    int firstFrame() const {return _firstFrame;}

    void setFirstFrame(int f) {_firstFrame = f;}

    int lastFrame() const {return _lastFrame;}

    void setLastFrame(int f){_lastFrame = f;}

    int currentFrame() const {return _currentFrame;}

    void incrementCurrentFrame(){++_currentFrame; emit frameChanged(_currentFrame);}

    void decrementCurrentFrame(){--_currentFrame; emit frameChanged(_currentFrame);}

    public slots:

    void seekFrame(int frame);

    void seekFrame_noEmit(int frame);


signals:

    void frameChanged(int);

private:
    int _firstFrame;
    int _lastFrame;
    int _currentFrame;
};
#endif /* defined(POWITER_ENGINE_TIMELINE_H_) */

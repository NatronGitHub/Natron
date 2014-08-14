//
//  TimeLine.h
//  Natron
//
//  Created by Frédéric Devernay on 24/09/13.
//
//

#ifndef NATRON_ENGINE_TIMELINE_H_
#define NATRON_ENGINE_TIMELINE_H_
#include <list>
#include "Global/Macros.h"
CLANG_DIAG_OFF(deprecated)
#include <QtCore/QMutex>
#include <QtCore/QObject>
CLANG_DIAG_ON(deprecated)

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
class Node;
}

class TimeLine: public QObject {

    Q_OBJECT

public:

    TimeLine(Natron::Project* project);
    
    virtual ~TimeLine(){}

    SequenceTime firstFrame() const;

    SequenceTime lastFrame() const;

    SequenceTime currentFrame() const;

    SequenceTime leftBound() const;

    SequenceTime rightBound() const;

    void setFrameRange(SequenceTime first, SequenceTime last);

    void setBoundaries(SequenceTime leftBound,SequenceTime rightBound);

    void seekFrame(SequenceTime frame,Natron::OutputEffectInstance* caller);

    void incrementCurrentFrame(Natron::OutputEffectInstance* caller);

    void decrementCurrentFrame(Natron::OutputEffectInstance* caller) ;
    
    void removeAllKeyframesIndicators();
    
    void addKeyframeIndicator(SequenceTime time);
    
    void addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime>& keys,bool emitSignal) ;
    
    void removeKeyFrameIndicator(SequenceTime time);
     
    void removeMultipleKeyframeIndicator(const std::list<SequenceTime>& keys,bool emitSignal);
    
    /**
     * @brief Show keyframe markers for the given nodes on the timeline. The signal to refresh the gui
     * will be emitted only once.
     **/
    void addNodesKeyframesToTimeline(const std::list<Natron::Node*>& nodes);
    
    /**
     * @brief Provided for convenience for a single node
     **/
    void addNodeKeyframesToTimeline(Natron::Node* node);
    
    /**
     * @brief Hide keyframe markers for the given nodes on the timeline. The signal to refresh the gui
     * will be emitted only once.
     **/
    void removeNodesKeyframesFromTimeline(const std::list<Natron::Node*>& nodes);
    
    /**
     * @brief Provided for convenience for a single node
     **/
    void removeNodeKeyframesFromTimeline(Natron::Node* node);
    
    void getKeyframes(std::list<SequenceTime>* keys) const;

  
public slots:
    void onFrameChanged(SequenceTime frame);

    void onBoundariesChanged(SequenceTime left,SequenceTime right);
    
    void goToPreviousKeyframe();
    
    void goToNextKeyframe();

signals:

    void frameRangeChanged(SequenceTime,SequenceTime);
    void boundariesChanged(SequenceTime,SequenceTime,int reason);
    //reason being a Natron::TIMELINE_CHANGE_REASON
    void frameChanged(SequenceTime,int reason);
    
    void keyframeIndicatorsChanged();

private:
    mutable QMutex _lock; // protects the following SequenceTime members
    SequenceTime _firstFrame;
    SequenceTime _lastFrame;
    SequenceTime _currentFrame;
    SequenceTime _leftBoundary, _rightBoundary; //these boundaries are within the interval [firstFrame,lastFrame]

    // not MT-safe
    std::list<SequenceTime> _keyframes;
    Natron::Project* _project;
};
#endif /* defined(NATRON_ENGINE_TIMELINE_H_) */

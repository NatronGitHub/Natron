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
, _keyframes()
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

void TimeLine::removeAllKeyframesIndicators() {
    bool wasEmpty = _keyframes.empty();
    _keyframes.clear();
    if (!wasEmpty) {
        emit keyframeIndicatorsChanged();
    }
}

void TimeLine::addKeyframeIndicator(SequenceTime time) {
    _keyframes.push_back(time);
    emit keyframeIndicatorsChanged();
}

void TimeLine::addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime>& keys) {
    _keyframes.insert(_keyframes.begin(),keys.begin(),keys.end());
    if (!keys.empty()) {
        emit keyframeIndicatorsChanged();
    }
}

void TimeLine::removeKeyFrameIndicator(SequenceTime time) {
    std::list<SequenceTime>::iterator it = std::find(_keyframes.begin(), _keyframes.end(), time);
    if (it != _keyframes.end()) {
        _keyframes.erase(it);
        emit keyframeIndicatorsChanged();
    }
    
}

void TimeLine::removeMultipleKeyframeIndicator(const std::list<SequenceTime>& keys) {
    for (std::list<SequenceTime>::const_iterator it = keys.begin(); it!=keys.end(); ++it) {
        std::list<SequenceTime>::iterator it2 = std::find(_keyframes.begin(), _keyframes.end(), *it);
        if (it2 != _keyframes.end()) {
            _keyframes.erase(it2);
        }
    }
    if (!keys.empty()) {
        emit keyframeIndicatorsChanged();
    }
}

void TimeLine::getKeyframes(std::list<SequenceTime>* keys) const
{
    *keys = _keyframes;
}

void TimeLine::goToPreviousKeyframe()
{
    _keyframes.sort();
    std::list<SequenceTime>::iterator lowerBound = std::lower_bound(_keyframes.begin(), _keyframes.end(), _currentFrame);
    if (lowerBound != _keyframes.begin()) {
        --lowerBound;
         seekFrame(*lowerBound,NULL);
    }
}

void TimeLine::goToNextKeyframe()
{
    _keyframes.sort();
    std::list<SequenceTime>::iterator upperBound = std::upper_bound(_keyframes.begin(), _keyframes.end(), _currentFrame);
    if (upperBound != _keyframes.end()) {
        seekFrame(*upperBound,NULL);
    }
}

/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

#ifndef TIMELINEKEYFRAMES_H
#define TIMELINEKEYFRAMES_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/GlobalDefines.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER;

/**
 * @brief An interface to implement to manage keyframes that are to be displayed on the timeline
 * Timeline keyframes indicators are integers
 **/
class TimeLineKeyFrames
{
    
public:
    
    TimeLineKeyFrames()
    {
        
    }
    
    /*
     * @brief Clear all indicators
     */
    virtual void removeAllKeyframesIndicators() {}
    
    /*
     * @brief Add the indicator at the given time. If it already existed it will be duplicated so that we can keep 
     * track of multiple keyframes at a given time.
     */
    virtual void addKeyframeIndicator(SequenceTime /*time*/) {}
    
    /*
     * @brief Same as addKeyframeIndicator() but for multiple keyframes
     * @param emitSignal If true, a signal should be emitted to refresh all GUI representing the timeline keyframes
     */
    virtual void addMultipleKeyframeIndicatorsAdded(const std::list<SequenceTime> & /*keys*/,bool /*emitSignal*/) {}
    
    /*
     * @brief Remove the indicator at the given time if the number of keyframes at this time is equal to 1 otherwise
     * the count of keyframes at this time is decreased by 1.
     */
    virtual void removeKeyFrameIndicator(SequenceTime /*time*/) {}
    
    /*
     * @brief Same as removeKeyFrameIndicator() but for multiple keyframes
     * @param emitSignal If true, a signal should be emitted to refresh all GUI representing the timeline keyframes
     */
    virtual void removeMultipleKeyframeIndicator(const std::list<SequenceTime> & /*keys*/,bool /*emitSignal*/) {}
    
    /**
     * @brief Show keyframe markers for the given nodes on the timeline. The signal to refresh the gui
     * will be emitted only once.
     **/
    virtual void addNodesKeyframesToTimeline(const std::list<Natron::Node*> & /*nodes*/) {}
    
    /**
     * @brief Provided for convenience for a single node
     **/
    virtual void addNodeKeyframesToTimeline(Natron::Node* /*node*/) {}
    
    /**
     * @brief Hide keyframe markers for the given nodes on the timeline. The signal to refresh the gui
     * will be emitted only once.
     **/
    virtual void removeNodesKeyframesFromTimeline(const std::list<Natron::Node*> & /*nodes*/) {}
    
    /**
     * @brief Provided for convenience for a single node
     **/
    virtual void removeNodeKeyframesFromTimeline(Natron::Node* /*node*/) {}
    
    /**
     * @brief Get all keyframes, there may be duplicates.
     **/
    virtual void getKeyframes(std::list<SequenceTime>* /*keys*/) const { }

    /**
     * @brief Go to the nearest keyframe before the application's timeline's current time.
     **/
    virtual void goToPreviousKeyframe() {}
    
    /**
     * @brief Go to the nearest keyframe after the application's timeline's current time.
     **/
    virtual void goToNextKeyframe() {}
};

NATRON_NAMESPACE_EXIT;

#endif // TIMELINEKEYFRAMES_H

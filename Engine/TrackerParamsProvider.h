/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * Copyright (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef TRACKERPARAMSPROVIDER_H
#define TRACKERPARAMSPROVIDER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "Engine/RectD.h"
#include "Engine/ImagePlaneDesc.h"
#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @brief A generic interface for all nodes that which to use the TrackScheduler
 **/
class TrackerParamsProviderBase
{
public:


    TrackerParamsProviderBase()
    {
    }

    virtual ~TrackerParamsProviderBase()
    {
    }

    /**
     * @brief Returns the node associated to this params provider
     **/
    virtual NodePtr getTrackerNode() const = 0;

    /**
     * @brief Returns the node from which to pull the source image stream to track
     **/
    virtual NodePtr getSourceImageNode() const = 0;

    /**
     * @brief Returns the node from which to pull a mask for the pattern of the reference image
     **/
    virtual NodePtr getMaskImageNode() const = 0;

    /**
     * @brief Returns the image plane to fetch for the mask image node
     **/
    virtual ImagePlaneDesc getMaskImagePlane(int *channelIndex) const = 0;

    /**
     * @brief Returns true if the viewer should center on markers when during tracking
     **/
    virtual bool getCenterOnTrack() const = 0;

    /**
     * @brief Returns true if the viewer should update during tracking
     **/
    virtual bool getUpdateViewer() const = 0;

    /**
     * @brief The function that performs a tracking step at the given frame for the track at the given trackIndex
     **/
    virtual bool trackStepFunctor(int trackIndex, const TrackArgsBasePtr& args, int frame) = 0;

    /**
     * @brief Called prior to tracking a sequence
     **/
    virtual void beginTrackSequence(const TrackArgsBasePtr& /*args*/) {}

    /**
     * @brief Called when the tracking ends for the sequence
     **/
    virtual void endTrackSequence(const TrackArgsBasePtr& /*args*/) {}
};

class TrackerParamsProvider : public TrackerParamsProviderBase
{


public:

    TrackerParamsProvider()
    : TrackerParamsProviderBase()
    {
    }

    virtual ~TrackerParamsProvider()
    {
    }

    /**
     * @brief Returns a pointer to the tracker object used
     **/
    virtual TrackerHelperPtr getTracker() const = 0;

    /**
     * @brief Returns channels to track
     **/
    virtual void getTrackChannels(bool* doRed, bool* doGreen, bool* doBlue) const = 0;

    /**
     * @brief Returns true if markers can be disabled automatically when tracking fails.
     **/
    virtual bool canDisableMarkersAutomatically() const = 0;

    /**
     * @brief If above this value, the tracking operation should stop. Must be between 0 and 1, 
     * 0 indicating no error.
     **/
    virtual double getMaxError() const = 0;

    /**
     * @brief The maximum number of iterations the internal solver should attempt before failing
     * the tracking operation.
     **/
    virtual int getMaxNIterations() const = 0;

    /**
     * @brief Should the solver use brute force translation only track before refining ?
     **/
    virtual bool isBruteForcePreTrackEnabled() const = 0;

    /**
     * @brief Should all computations be normalized internally ?
     **/
    virtual bool isNormalizeIntensitiesEnabled() const = 0;

    /**
     * @brief Returns the sigma of the pre-blur used on images to track
     **/
    virtual double getPreBlurSigma() const = 0;

    /**
     * @brief Returns the rectangle used to normalize coordinates for the given time/view
     **/
    virtual RectD getNormalizationRoD(TimeValue time, ViewIdx view) const = 0;

};


NATRON_NAMESPACE_EXIT

#endif // TRACKERPARAMSPROVIDER_H

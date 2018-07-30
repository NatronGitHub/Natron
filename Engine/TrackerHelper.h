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

#ifndef NATRON_ENGINE_TRACKER_HELPER_H
#define NATRON_ENGINE_TRACKER_HELPER_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <set>
#include <list>

#include "Global/GlobalDefines.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#endif

#include <QObject>

#include "Engine/Transform.h"
#include "Engine/TimeValue.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

/**
 * @struct Structure returned by the computeCornerPinParamsFromTracksAtTime function
 **/
class CornerPinData
{
public:

    CornerPinData()
    : h()
    , nbEnabledPoints(0)
    , time(-1.)
    , valid(false)
    , rms(-1.)
    {
    }

    Transform::Matrix3x3 h;
    int nbEnabledPoints;
    TimeValue time;
    bool valid;
    double rms;
};

/**
 * @struct Structure returned by the computeTransformParamsFromTracksAtTime function
 **/
class TransformData
{
public:

    TransformData()
    : rotation(0.)
    , scale(0.)
    , hasRotationAndScale(false)
    , time(-1.)
    , valid(false)
    , rms(-1.)
    {
        translation.x = translation.y = 0.;
    }

    Point translation;
    double rotation;
    double scale;
    bool hasRotationAndScale;
    TimeValue time;
    bool valid;
    double rms;
};


class TrackerHelperPrivate;
class TrackerHelper
: public QObject
{
    GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
    GCC_DIAG_SUGGEST_OVERRIDE_ON

public:
    
    TrackerHelper(const TrackerParamsProviderPtr &provider);

    virtual ~TrackerHelper();

    /**
     * @brief Tracks the given markers over the range defined by [start,end[ (end pointing to the frame
     * after the last one, a la STL).
     * @param frameStep How many frames the scheduler should step before tracking the next frame. Cannot be 0,
     * but may be negative to track backward.
     * @param viewer A pointer to the viewer that should be updated throughout the tracking operation.
     **/
    void trackMarkers(const std::list<TrackMarkerPtr>& marks,
                      TimeValue start,
                      TimeValue end,
                      TimeValue frameStep,
                      const ViewerNodePtr& viewer);

    void trackMarker(const TrackMarkerPtr& mark,
                      TimeValue start,
                      TimeValue end,
                      TimeValue frameStep,
                      const ViewerNodePtr& viewer);

    /**
     * @brief Abort any ongoing tracking. Non blocking: it is not guaranteed the tracking is finished
     * once returning this function returns.
     **/
    void abortTracking();

    /**
     * @brief Abort any ongoing tracking. Blocking: it is guaranteed the tracking is finished
     * once returning this function returns.
     **/
    void abortTracking_blocking();

    /**
     * @brief Returns true if this object is currently tracking.
     **/
    bool isCurrentlyTracking() const;

    /**
     * @brief Quit the tracker thread. Non blocking: it is not guaranteed the tracking is finished
     * once returning this function.
     **/
    void quitTrackerThread_non_blocking();

    /**
     * @brief Quit the tracker thread. Blocking: it is guaranteed the tracking is finished
     * once returning this function.
     **/
    void quitTrackerThread_blocking(bool allowRestart);

    /**
     * @brief Returns true if the threads of the tracker are stopped.
     **/
    bool hasTrackerThreadQuit() const;


    /**
     * @brief Computes the translation that best fit the set of correspondences x1 and x2.
     * Requires at least 1 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeTranslationFromNPoints(const bool dataSetIsManual,
                                              const bool robustModel,
                                              const std::vector<Point>& x1,
                                              const std::vector<Point>& x2,
                                              int w1, int h1, int w2, int h2,
                                              Point* translation,
                                              double *RMS = 0);

    /**
     * @brief Computes the translation, rotation and scale that best fit the set of correspondences x1 and x2.
     * Requires at least 2 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeSimilarityFromNPoints(const bool dataSetIsManual,
                                             const bool robustModel,
                                             const std::vector<Point>& x1,
                                             const std::vector<Point>& x2,
                                             int w1, int h1, int w2, int h2,
                                             Point* translation,
                                             double* rotate,
                                             double* scale,
                                             double *RMS = 0);
    /**
     * @brief Computes the homography that best fit the set of correspondences x1 and x2.
     * Requires at least 4 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeHomographyFromNPoints(const bool dataSetIsManual,
                                             const bool robustModel,
                                             const std::vector<Point>& x1,
                                             const std::vector<Point>& x2,
                                             int w1, int h1, int w2, int h2,
                                             Transform::Matrix3x3* homog,
                                             double *RMS = 0);

    /**
     * @brief Computes the fundamental matrix that best fit the set of correspondences x1 and x2.
     * Requires at least 7 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeFundamentalFromNPoints(const bool dataSetIsManual,
                                              const bool robustModel,
                                              const std::vector<Point>& x1,
                                              const std::vector<Point>& x2,
                                              int w1, int h1, int w2, int h2,
                                              Transform::Matrix3x3* fundamental,
                                              double *RMS = 0);

    /**
     * @brief Extracts the values of the center point of the given markers at x1Time and x2Time.
     * @param jitterPeriod If jitterPeriod > 1 this is the amount of frames that will be averaged together to add
     * jitter or remove jitter.
     * @param jitterAdd If jitterPeriod > 1 this parameter is disregarded. Otherwise, if jitterAdd is false, then
     * the values of the center points are smoothed with high frequencies removed (using average over jitterPeriod)
     * If jitterAdd is true, then we compute the smoothed points (using average over jitterPeriod), and substract it
     * from the original points to get the high frequencies. We then add those high frequencies back to the original
     * points to increase shaking/motion
     **/
    static void extractSortedPointsFromMarkers(TimeValue x1Time, TimeValue x2Time,
                                               const std::vector<TrackMarkerPtr>& markers,
                                               int jitterPeriod,
                                               bool jitterAdd,
                                               const KnobDoublePtr& center,
                                               std::vector<Point>* x1,
                                               std::vector<Point>* x2);


    /**
     * @brief Given the markers that have been tracked, computes the affine transform mapping from refTime
     * to time.
     **/
    static TransformData computeTransformParamsFromTracksAtTime(TimeValue refTime,
                                                                TimeValue time,
                                                                int jitterPeriod,
                                                                bool jitterAdd,
                                                                bool robustModel,
                                                                const TrackerParamsProviderPtr& params,
                                                                const KnobDoublePtr& center,
                                                                const std::vector<TrackMarkerPtr>& allMarkers);



    /**
     * @brief Given the markers that have been tracked, computes the CornerPin mapping from refTime
     * to time.
     **/
    static CornerPinData computeCornerPinParamsFromTracksAtTime(TimeValue refTime,
                                                                TimeValue time,
                                                                int jitterPeriod,
                                                                bool jitterAdd,
                                                                bool robustModel,
                                                                const TrackerParamsProviderPtr& params,
                                                                const std::vector<TrackMarkerPtr>& allMarkers);

    static Point applyHomography(const Point& p, const Transform::Matrix3x3& h);
    
private Q_SLOTS:
    
    void onSchedulerTrackingStarted(int frameStep);
    
    void onSchedulerTrackingFinished();
    
    void onSchedulerTrackingProgress(double progress);
    
Q_SIGNALS:
    
    void trackingStarted(int);
    void trackingFinished();
    
private:
    
    
    boost::scoped_ptr<TrackerHelperPrivate> _imp;
};


NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_TRACKER_HELPER_H

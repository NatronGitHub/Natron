/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2015 INRIA and Alexandre Gauthier-Foichat
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "TrackerHelperPrivate.h"

#include <QtCore/QThreadPool>

#include "Engine/AppInstance.h"
#include "Engine/Curve.h"
#include "Engine/Project.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/TrackArgs.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerHelper.h"



NATRON_NAMESPACE_ENTER;



TrackerHelperPrivate::TrackerHelperPrivate(TrackerHelper* publicInterface,
                                            const TrackerParamsProviderPtr &provider)
    : _publicInterface(publicInterface)
    , scheduler()
{
    scheduler.reset(new TrackScheduler(provider));
    //needs to be blocking, otherwise the progressUpdate() call could be made before startProgress
    QObject::connect( scheduler.get(), SIGNAL(trackingStarted(int)), publicInterface, SLOT(onSchedulerTrackingStarted(int)) );
    QObject::connect( scheduler.get(), SIGNAL(trackingFinished()), _publicInterface, SLOT(onSchedulerTrackingFinished()) );
    QObject::connect( scheduler.get(), SIGNAL(trackingProgress(double)), _publicInterface, SLOT(onSchedulerTrackingProgress(double)) );
}


/**
 * @brief Set keyframes on knobs from Marker data
 **/
void
TrackerHelperPrivate::setKnobKeyframesFromMarker(const mv::Marker& mvMarker,
                                                  int /*formatHeight*/,
                                                  const libmv::TrackRegionResult* result,
                                                  const TrackMarkerPtr& natronMarker)
{
    int time = mvMarker.frame;
    KnobDoublePtr errorKnob = natronMarker->getErrorKnob();

    if (result) {
        double corr = result->correlation;
        if (corr != corr) {
            corr = 1.;
        }
        errorKnob->setValueAtTime(time, 1. - corr);
    } else {
        errorKnob->setValueAtTime(time, 0.);
    }

    // Blender also adds 0.5 to coordinates
    Point center;
    center.x = (double)mvMarker.center(0) + 0.5;
    center.y = mvMarker.center(1) + 0.5; //(double)TrackerFrameAccessor::invertYCoordinate(mvMarker.center(1), formatHeight);

    KnobDoublePtr offsetKnob = natronMarker->getOffsetKnob();
    Point offset;
    offset.x = offsetKnob->getValueAtTime(time, DimIdx(0));
    offset.y = offsetKnob->getValueAtTime(time, DimIdx(1));

    // Set the center
    KnobDoublePtr centerKnob = natronMarker->getCenterKnob();
    {
        std::vector<double> values(2);
        values[0] = center.x;
        values[1] = center.y;
        centerKnob->setValueAtTimeAcrossDimensions(time, values);
    }


    Point topLeftCorner, topRightCorner, btmRightCorner, btmLeftCorner;
    topLeftCorner.x = mvMarker.patch.coordinates(3, 0) - offset.x - center.x;//mvMarker.patch.coordinates(0, 0) - offset.x - center.x;
    topLeftCorner.y = mvMarker.patch.coordinates(3, 1) - offset.y - center.y; //TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(0, 1), formatHeight) - offset.y - center.y;

    topRightCorner.x = mvMarker.patch.coordinates(2, 0) - offset.x - center.x;//mvMarker.patch.coordinates(1, 0) - offset.x - center.x;
    topRightCorner.y = mvMarker.patch.coordinates(2, 1) - offset.y - center.y;//TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(1, 1), formatHeight) - offset.y - center.y;

    btmRightCorner.x = mvMarker.patch.coordinates(1, 0) - offset.x - center.x;// mvMarker.patch.coordinates(2, 0) - offset.x - center.x;
    btmRightCorner.y = mvMarker.patch.coordinates(1, 1) - offset.y - center.y;//TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(2, 1), formatHeight) - offset.y - center.y;

    btmLeftCorner.x = mvMarker.patch.coordinates(0, 0) - offset.x - center.x; //mvMarker.patch.coordinates(3, 0) - offset.x - center.x;
    btmLeftCorner.y = mvMarker.patch.coordinates(0, 1) - offset.y - center.y;//TrackerFrameAccessor::invertYCoordinate(mvMarker.patch.coordinates(3, 1), formatHeight) - offset.y - center.y;

    KnobDoublePtr pntTopLeftKnob = natronMarker->getPatternTopLeftKnob();
    KnobDoublePtr pntTopRightKnob = natronMarker->getPatternTopRightKnob();
    KnobDoublePtr pntBtmLeftKnob = natronMarker->getPatternBtmLeftKnob();
    KnobDoublePtr pntBtmRightKnob = natronMarker->getPatternBtmRightKnob();

    // Set the pattern Quad
    std::vector<double> values(2);
    values[0] = topLeftCorner.x;
    values[1] = topLeftCorner.y;
    pntTopLeftKnob->setValueAtTimeAcrossDimensions(time, values);
    values[0] = topRightCorner.x;
    values[1] = topRightCorner.y;
    pntTopRightKnob->setValueAtTimeAcrossDimensions(time, values);
    values[0] = btmLeftCorner.x;
    values[1] = btmLeftCorner.y;
    pntBtmLeftKnob->setValueAtTimeAcrossDimensions(time, values);
    values[0] = btmRightCorner.x;
    values[1] = btmRightCorner.y;
    pntBtmRightKnob->setValueAtTimeAcrossDimensions(time, values);
} // TrackerHelperPrivate::setKnobKeyframesFromMarker

/// Converts a Natron track marker to the one used in LibMV. This is expensive: many calls to getValue are made
void
TrackerHelperPrivate::natronTrackerToLibMVTracker(bool isReferenceMarker,
                                                   bool trackChannels[3],
                                                   const TrackMarker& marker,
                                                   int trackIndex,
                                                   int trackedTime,
                                                   int frameStep,
                                                   double /*formatHeight*/,
                                                   mv::Marker* mvMarker)
{
    KnobDoublePtr searchWindowBtmLeftKnob = marker.getSearchWindowBottomLeftKnob();
    KnobDoublePtr searchWindowTopRightKnob = marker.getSearchWindowTopRightKnob();
    KnobDoublePtr patternTopLeftKnob = marker.getPatternTopLeftKnob();
    KnobDoublePtr patternTopRightKnob = marker.getPatternTopRightKnob();
    KnobDoublePtr patternBtmRightKnob = marker.getPatternBtmRightKnob();
    KnobDoublePtr patternBtmLeftKnob = marker.getPatternBtmLeftKnob();

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    KnobDoublePtr weightKnob = marker.getWeightKnob();
#endif
    KnobDoublePtr centerKnob = marker.getCenterKnob();
    KnobDoublePtr offsetKnob = marker.getOffsetKnob();

    // We don't use the clip in Natron
    mvMarker->clip = 0;
    mvMarker->reference_clip = 0;

#ifdef NATRON_TRACK_MARKER_USE_WEIGHT
    mvMarker->weight = (float)weightKnob->getValue();
#else
    mvMarker->weight = 1.;
#endif

    mvMarker->frame = trackedTime;
    const int referenceTime = marker.getReferenceFrame(trackedTime, frameStep);
    mvMarker->reference_frame = referenceTime;
    if ( marker.isUserKeyframe(trackedTime) ) {
        mvMarker->source = mv::Marker::MANUAL;
    } else {
        mvMarker->source = mv::Marker::TRACKED;
    }

    // This doesn't seem to be used at all by libmv TrackRegion
    mvMarker->model_type = mv::Marker::POINT;
    mvMarker->model_id = 0;
    mvMarker->track = trackIndex;

    mvMarker->disabled_channels =
        (trackChannels[0] ? LIBMV_MARKER_CHANNEL_R : 0) |
        (trackChannels[1] ? LIBMV_MARKER_CHANNEL_G : 0) |
        (trackChannels[2] ? LIBMV_MARKER_CHANNEL_B : 0);


    //int searchWinTime = isReferenceMarker ? trackedTime : mvMarker->reference_frame;

    // The patch is a quad (4 points); generally in 2D or 3D (here 2D)
    //
    //    +----------> x
    //    |\.
    //    | \.
    //    |  z (z goes into screen)
    //    |
    //    |     r0----->r1
    //    |      ^       |
    //    |      |   .   |
    //    |      |       V
    //    |     r3<-----r2
    //    |              \.
    //    |               \.
    //    v                normal goes away (right handed).
    //    y
    //
    // Each row is one of the corners coordinates; either (x, y) or (x, y, z).
    // TrackMarker extracts the patch coordinates as such:
    /*
       Quad2Df offset_quad = marker.patch;
       Vec2f origin = marker.search_region.Rounded().min;
       offset_quad.coordinates.rowwise() -= origin.transpose();
       QuadToArrays(offset_quad, x, y);
       x[4] = marker.center.x() - origin(0);
       y[4] = marker.center.y() - origin(1);
     */
    // The patch coordinates should be canonical


    Point tl, tr, br, bl;
    tl.x = patternTopLeftKnob->getValueAtTime(trackedTime, DimIdx(0));
    tl.y = patternTopLeftKnob->getValueAtTime(trackedTime, DimIdx(1));

    tr.x = patternTopRightKnob->getValueAtTime(trackedTime, DimIdx(0));
    tr.y = patternTopRightKnob->getValueAtTime(trackedTime, DimIdx(1));

    br.x = patternBtmRightKnob->getValueAtTime(trackedTime, DimIdx(0));
    br.y = patternBtmRightKnob->getValueAtTime(trackedTime, DimIdx(1));

    bl.x = patternBtmLeftKnob->getValueAtTime(trackedTime, DimIdx(0));
    bl.y = patternBtmLeftKnob->getValueAtTime(trackedTime, DimIdx(1));

    /*RectD patternBbox;
       patternBbox.setupInfinity();
       updateBbox(tl, &patternBbox);
       updateBbox(tr, &patternBbox);
       updateBbox(br, &patternBbox);
       updateBbox(bl, &patternBbox);*/

    // The search-region is laid out as such:
    //
    //    +----------> x
    //    |
    //    |   (min.x, min.y)           (max.x, min.y)
    //    |        +-------------------------+
    //    |        |                         |
    //    |        |                         |
    //    |        |                         |
    //    |        +-------------------------+
    //    v   (min.x, max.y)           (max.x, max.y)
    //

    int searchWinTime = isReferenceMarker ? trackedTime : referenceTime;
    Point searchWndBtmLeft, searchWndTopRight;
    searchWndBtmLeft.x = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, DimIdx(0));
    searchWndBtmLeft.y = searchWindowBtmLeftKnob->getValueAtTime(searchWinTime, DimIdx(1));

    searchWndTopRight.x = searchWindowTopRightKnob->getValueAtTime(searchWinTime, DimIdx(0));
    searchWndTopRight.y = searchWindowTopRightKnob->getValueAtTime(searchWinTime, DimIdx(1));

    /*
       Center and offset are always sampled at the reference time
     */
    Point centerAtTrackedTime;
    centerAtTrackedTime.x = centerKnob->getValueAtTime(trackedTime, DimIdx(0));
    centerAtTrackedTime.y = centerKnob->getValueAtTime(trackedTime, DimIdx(1));

    Point offsetAtTrackedTime;
    offsetAtTrackedTime.x = offsetKnob->getValueAtTime(trackedTime, DimIdx(0));
    offsetAtTrackedTime.y = offsetKnob->getValueAtTime(trackedTime, DimIdx(1));

    // Blender also substracts 0.5 to coordinates
    mvMarker->center(0) = centerAtTrackedTime.x - 0.5;
    mvMarker->center(1) = centerAtTrackedTime.y - 0.5; //TrackerFrameAccessor::invertYCoordinate(centerAtTrackedTime.y, formatHeight);

    Point centerPlusOffset;
    centerPlusOffset.x = centerAtTrackedTime.x + offsetAtTrackedTime.x;
    centerPlusOffset.y = centerAtTrackedTime.y + offsetAtTrackedTime.y;

    searchWndBtmLeft.x += centerPlusOffset.x;
    searchWndBtmLeft.y += centerPlusOffset.y;

    searchWndTopRight.x += centerPlusOffset.x;
    searchWndTopRight.y += centerPlusOffset.y;

    tl.x += centerPlusOffset.x;
    tl.y += centerPlusOffset.y;

    tr.x += centerPlusOffset.x;
    tr.y += centerPlusOffset.y;

    br.x += centerPlusOffset.x;
    br.y += centerPlusOffset.y;

    bl.x += centerPlusOffset.x;
    bl.y += centerPlusOffset.y;

    mvMarker->search_region.min(0) = searchWndBtmLeft.x - 0.5;
    mvMarker->search_region.min(1) = searchWndBtmLeft.y - 0.5; //TrackerFrameAccessor::invertYCoordinate(searchWndTopRight.y, formatHeight);
    mvMarker->search_region.max(0) = searchWndTopRight.x - 0.5;
    mvMarker->search_region.max(1) = searchWndTopRight.y - 0.5; //TrackerFrameAccessor::invertYCoordinate(searchWndBtmLeft.y, formatHeight);


    mvMarker->patch.coordinates(0, 0) = bl.x - 0.5; //tl.x;
    mvMarker->patch.coordinates(0, 1) = bl.y - 0.5; //TrackerFrameAccessor::invertYCoordinate(tl.y, formatHeight);

    mvMarker->patch.coordinates(1, 0) = br.x - 0.5; //tr.x;
    mvMarker->patch.coordinates(1, 1) = br.y - 0.5; //TrackerFrameAccessor::invertYCoordinate(tr.y, formatHeight);

    mvMarker->patch.coordinates(2, 0) = tr.x - 0.5; //br.x;
    mvMarker->patch.coordinates(2, 1) = tr.y - 0.5 ;//TrackerFrameAccessor::invertYCoordinate(br.y, formatHeight);

    mvMarker->patch.coordinates(3, 0) = tl.x - 0.5; //bl.x;
    mvMarker->patch.coordinates(3, 1) = tl.y - 0.5; //TrackerFrameAccessor::invertYCoordinate(bl.y, formatHeight);
} // TrackerHelperPrivate::natronTrackerToLibMVTracker

void
TrackerHelperPrivate::beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const
{
    TrackerParamsProviderPtr params = provider.lock();
    options->minimum_correlation = 1. - params->getMaxError();
    assert(options->minimum_correlation >= 0. && options->minimum_correlation <= 1.);
    options->max_iterations = params->getMaxNIterations();
    options->use_brute_initialization = params->isBruteForcePreTrackEnabled();
    options->use_normalized_intensities = params->isNormalizeIntensitiesEnabled();
    options->sigma = params->getPreBlurSigma();

#pragma message WARN("LibMV: TrackRegionOptions use_esm flag set to false because it is buggy")
    options->use_esm = false;
}

void
TrackerHelperPrivate::endLibMVOptionsForTrack(const TrackMarker& marker,
                                               mv::TrackRegionOptions* options) const
{
    int mode_i = marker.getMotionModelKnob()->getValue();

    switch (mode_i) {
    case 0:
        options->mode = mv::TrackRegionOptions::TRANSLATION;
        break;
    case 1:
        options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION;
        break;
    case 2:
        options->mode = mv::TrackRegionOptions::TRANSLATION_SCALE;
        break;
    case 3:
        options->mode = mv::TrackRegionOptions::TRANSLATION_ROTATION_SCALE;
        break;
    case 4:
        options->mode = mv::TrackRegionOptions::AFFINE;
        break;
    case 5:
        options->mode = mv::TrackRegionOptions::HOMOGRAPHY;
        break;
    default:
        options->mode = mv::TrackRegionOptions::AFFINE;
        break;
    }
}

/*
 * @brief This is the internal tracking function that makes use of TrackerPM to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param trackTime The search frame time, that is, the frame to track
 */
bool
TrackerHelperPrivate::trackStepTrackerPM(const TrackMarkerPMPtr& track,
                                          const TrackArgs& args,
                                          int trackTime)
{
    int frameStep = args.getStep();
    int refTime = track->getReferenceFrame(trackTime, frameStep);

    return track->trackMarker(frameStep > 0, refTime, trackTime);
}

/*
 * @brief This is the internal tracking function that makes use of LivMV to do 1 track step
 * @param trackingIndex This is the index of the Marker we should track in the args
 * @param args Multiple arguments global to the whole track, not just this step
 * @param trackTime The search frame time, that is, the frame to track
 */
bool
TrackerHelperPrivate::trackStepLibMV(int trackIndex,
                                      const TrackArgs& args,
                                      int trackTime)
{
    assert( trackIndex >= 0 && trackIndex < args.getNumTracks() );

#ifdef CERES_USE_OPENMP
    // Set the number of threads Ceres may use
    QThreadPool* tp = QThreadPool::globalInstance();
    omp_set_num_threads(tp->maxThreadCount() - tp->activeThreadCount() - 1);
#endif

    const std::vector<TrackMarkerAndOptionsPtr >& tracks = args.getTracks();
    const TrackMarkerAndOptionsPtr& track = tracks[trackIndex];
    boost::shared_ptr<mv::AutoTrack> autoTrack = args.getLibMVAutoTrack();
    QMutex* autoTrackMutex = args.getAutoTrackMutex();
    bool enabledChans[3];
    args.getEnabledChannels(&enabledChans[0], &enabledChans[1], &enabledChans[2]);


    {
        // Add a marker to the auto-track at the tracked time: the mv::Marker struct is filled with the values of the Natron TrackMarker at the trackTime
        QMutexLocker k(autoTrackMutex);
        if ( trackTime == args.getStart() ) {
            bool foundStartMarker = autoTrack->GetMarker(0, trackTime, trackIndex, &track->mvMarker);
            assert(foundStartMarker);
            Q_UNUSED(foundStartMarker);
            track->mvMarker.source = mv::Marker::MANUAL;
        } else {
            natronTrackerToLibMVTracker(false, enabledChans, *track->natronMarker, trackIndex, trackTime, args.getStep(), args.getFormatHeight(), &track->mvMarker);
            autoTrack->AddMarker(track->mvMarker);
        }
    }

    if (track->mvMarker.source == mv::Marker::MANUAL) {
        // This is a user keyframe or the first frame, we do not track it
        assert( trackTime == args.getStart() || track->natronMarker->isUserKeyframe(track->mvMarker.frame) );
#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "TrackStep:" << trackTime << "is a keyframe";
#endif
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), 0, track->natronMarker);
    } else {
        // Make sure the reference frame is in the auto-track: the mv::Marker struct is filled with the values of the Natron TrackMarker at the reference_frame
        {
            QMutexLocker k(autoTrackMutex);
            mv::Marker m;
            if ( !autoTrack->GetMarker(0, track->mvMarker.reference_frame, trackIndex, &m) ) {
                natronTrackerToLibMVTracker(true, enabledChans, *track->natronMarker, track->mvMarker.track, track->mvMarker.reference_frame, args.getStep(), args.getFormatHeight(), &m);
                autoTrack->AddMarker(m);
            }
        }


        assert(track->mvMarker.reference_frame != track->mvMarker.frame);


#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << ">>>> Tracking marker" << trackIndex << "at frame" << trackTime <<
            "with reference frame" << track->mvMarker.reference_frame;
#endif

        // Do the actual tracking
        libmv::TrackRegionResult result;
        if ( !autoTrack->TrackMarker(&track->mvMarker, &result,  &track->mvState, &track->mvOptions) || !result.is_usable() ) {
#ifdef TRACE_LIB_MV
            qDebug() << QThread::currentThread() << "Tracking FAILED (" << (int)result.termination <<  ") for track" << trackIndex << "at frame" << trackTime;
#endif

            // Todo: report error to user
            return false;
        }

        //Ok the tracking has succeeded, now the marker is in this state:
        //the source is TRACKED, the search_window has been offset by the center delta,  the center has been offset

#ifdef TRACE_LIB_MV
        qDebug() << QThread::currentThread() << "Tracking SUCCESS for track" << trackIndex << "at frame" << trackTime;
#endif

        //Extract the marker to the knob keyframes
        setKnobKeyframesFromMarker(track->mvMarker, args.getFormatHeight(), &result, track->natronMarker);

        //Add the marker to the autotrack
        /*{
           QMutexLocker k(autoTrackMutex);
           autoTrack->AddMarker(track->mvMarker);
           }*/
    } // if (track->mvMarker.source == mv::Marker::MANUAL) {


    return true;
} // TrackerHelperPrivate::trackStepLibMV



static Transform::Point3D
euclideanToHomogenous(const Point& p)
{
    Transform::Point3D r;

    r.x = p.x;
    r.y = p.y;
    r.z = 1;

    return r;
}

static Point
applyHomography(const Point& p,
                const Transform::Matrix3x3& h)
{
    Transform::Point3D a = euclideanToHomogenous(p);
    Transform::Point3D r = Transform::matApply(h, a);
    Point ret;

    ret.x = r.x / r.z;
    ret.y = r.y / r.z;

    return ret;
}

using namespace openMVG::robust;
static void
throwProsacError(ProsacReturnCodeEnum c,
                 int nMinSamples)
{
    switch (c) {
    case openMVG::robust::eProsacReturnCodeFoundModel:
        break;
    case openMVG::robust::eProsacReturnCodeInliersIsMinSamples:
        break;
    case openMVG::robust::eProsacReturnCodeNoModelFound:
        throw std::runtime_error("Could not find a model for the given correspondences.");
        break;
    case openMVG::robust::eProsacReturnCodeNotEnoughPoints: {
        std::stringstream ss;
        ss << "This model requires a minimum of ";
        ss << nMinSamples;
        ss << " correspondences.";
        throw std::runtime_error( ss.str() );
        break;
    }
    case openMVG::robust::eProsacReturnCodeMaxIterationsFromProportionParamReached:
        throw std::runtime_error("Maximum iterations computed from outliers proportion reached");
        break;
    case openMVG::robust::eProsacReturnCodeMaxIterationsParamReached:
        throw std::runtime_error("Maximum solver iterations reached");
        break;
    }
}

/*
 * @brief Search for the best model that fits the correspondences x1 to x2.
 * @param dataSetIsManual If true, this indicates that the x1 points were placed manually by the user
 * in which case we expect the vector to have less than 10% outliers
 * @param robustModel When dataSetIsManual is true, if this parameter is true then the solver will run a MEsimator on the data
 * assuming the model searched is the correct model. Otherwise if false, only a least-square pass is done to compute a model that fits
 * all correspondences (but which may be incorrect)
 */
template <typename MODELTYPE>
void
searchForModel(const bool dataSetIsManual,
               const bool robustModel,
               const std::vector<Point>& x1,
               const std::vector<Point>& x2,
               int w1,
               int h1,
               int w2,
               int h2,
               typename MODELTYPE::Model* foundModel,
               double *RMS = 0
#ifdef DEBUG
               ,
               std::vector<bool>* inliers = 0
#endif
               )
{
    typedef ProsacKernelAdaptor<MODELTYPE> KernelType;

    assert( x1.size() == x2.size() );
    openMVG::Mat M1( 2, x1.size() ), M2( 2, x2.size() );
    for (std::size_t i = 0; i < x1.size(); ++i) {
        M1(0, i) = x1[i].x;
        M1(1, i) = x1[i].y;

        M2(0, i) = x2[i].x;
        M2(1, i) = x2[i].y;
    }

    KernelType kernel(M1, w1, h1, M2, w2, h2);

    if (dataSetIsManual) {
        if (robustModel) {
            double sigmaMAD;
            if ( !searchModelWithMEstimator(kernel, 3, foundModel, RMS, &sigmaMAD) ) {
                throw std::runtime_error("MEstimator failed to run a successful iteration");
            }
        } else {
            if ( !searchModelLS(kernel, foundModel, RMS) ) {
                throw std::runtime_error("Least-squares solver failed to find a model");
            }
        }
    } else {
        ProsacReturnCodeEnum ret = prosac(kernel, foundModel
#ifdef DEBUG
                                          , inliers
#else
                                          , 0
#endif
                                          , RMS);
        throwProsacError( ret, KernelType::MinimumSamples() );
    }
} // searchForModel

void
TrackerHelper::computeTranslationFromNPoints(const bool dataSetIsManual,
                                                     const bool robustModel,
                                                     const std::vector<Point>& x1,
                                                     const std::vector<Point>& x2,
                                                     int w1,
                                                     int h1,
                                                     int w2,
                                                     int h2,
                                                     Point* translation,
                                                     double *RMS)
{
    openMVG::Vec2 model;

    searchForModel<openMVG::robust::Translation2DSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS);
    translation->x = model(0);
    translation->y = model(1);
}

void
TrackerHelper::computeSimilarityFromNPoints(const bool dataSetIsManual,
                                                    const bool robustModel,
                                                    const std::vector<Point>& x1,
                                                    const std::vector<Point>& x2,
                                                    int w1,
                                                    int h1,
                                                    int w2,
                                                    int h2,
                                                    Point* translation,
                                                    double* rotate,
                                                    double* scale,
                                                    double *RMS)
{
    openMVG::Vec4 model;

    searchForModel<openMVG::robust::Similarity2DSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS);
    openMVG::robust::Similarity2DSolver::rtsFromVec4(model, &translation->x, &translation->y, scale, rotate);
    *rotate = Transform::toDegrees(*rotate);
}

void
TrackerHelper::computeHomographyFromNPoints(const bool dataSetIsManual,
                                                    const bool robustModel,
                                                    const std::vector<Point>& x1,
                                                    const std::vector<Point>& x2,
                                                    int w1,
                                                    int h1,
                                                    int w2,
                                                    int h2,
                                                    Transform::Matrix3x3* homog,
                                                    double *RMS)
{
    openMVG::Mat3 model;

#ifdef DEBUG
    std::vector<bool> inliers;
#endif

    searchForModel<openMVG::robust::Homography2DSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS
#ifdef DEBUG
                                                        , &inliers
#endif
                                                        );

    *homog = Transform::Matrix3x3( model(0, 0), model(0, 1), model(0, 2),
                                   model(1, 0), model(1, 1), model(1, 2),
                                   model(2, 0), model(2, 1), model(2, 2) );

#ifdef DEBUG
    // Check that the warped x1 points match x2
    assert( x1.size() == x2.size() );
    for (std::size_t i = 0; i < x1.size(); ++i) {
        if (!dataSetIsManual && inliers[i]) {
            Point p2 = applyHomography(x1[i], *homog);
            if ( (std::abs(p2.x - x2[i].x) >  0.02) ||
                 ( std::abs(p2.y - x2[i].y) > 0.02) ) {
                qDebug() << "[BUG]: Inlier for Homography2DSolver failed to fit the found model: X1 (" << x1[i].x << ',' << x1[i].y << ')' << "X2 (" << x2[i].x << ',' << x2[i].y << ')'  << "P2 (" << p2.x << ',' << p2.y << ')';
            }
        }
    }
#endif
}

void
TrackerHelper::computeFundamentalFromNPoints(const bool dataSetIsManual,
                                                     const bool robustModel,
                                                     const std::vector<Point>& x1,
                                                     const std::vector<Point>& x2,
                                                     int w1,
                                                     int h1,
                                                     int w2,
                                                     int h2,
                                                     Transform::Matrix3x3* fundamental,
                                                     double *RMS)
{
    openMVG::Mat3 model;

    searchForModel<openMVG::robust::FundamentalSolver>(dataSetIsManual, robustModel, x1, x2, w1, h1, w2, h2, &model, RMS);

    *fundamental = Transform::Matrix3x3( model(0, 0), model(0, 1), model(0, 2),
                                         model(1, 0), model(1, 1), model(1, 2),
                                         model(2, 0), model(2, 1), model(2, 2) );
}

struct PointWithError
{
    Point p1, p2;
    double error;
};

static bool
PointWithErrorCompareLess(const PointWithError& lhs,
                          const PointWithError& rhs)
{
    return lhs.error < rhs.error;
}

void
TrackerHelper::extractSortedPointsFromMarkers(double refTime,
                                                      double time,
                                                      const std::vector<TrackMarkerPtr>& markers,
                                                      int jitterPeriod,
                                                      bool jitterAdd,
                                                      std::vector<Point>* x1,
                                                      std::vector<Point>* x2)
{
    assert( !markers.empty() );

    std::vector<PointWithError> pointsWithErrors;
    bool useJitter = (jitterPeriod > 1);
    int halfJitter = std::max(0, jitterPeriod / 2);
    // Prosac expects the points to be sorted by decreasing correlation score (increasing error)
    int pIndex = 0;
    for (std::size_t i = 0; i < markers.size(); ++i) {
        KnobDoublePtr centerKnob = markers[i]->getCenterKnob();
        KnobDoublePtr errorKnob = markers[i]->getErrorKnob();

        if (centerKnob->getKeyFrameIndex(ViewGetSpec::current(), DimIdx(0), time) < 0) {
            continue;
        }
        pointsWithErrors.resize(pointsWithErrors.size() + 1);

        PointWithError& perr = pointsWithErrors[pIndex];

        if (!useJitter) {
            perr.p1.x = centerKnob->getValueAtTime(refTime, DimIdx(0));
            perr.p1.y = centerKnob->getValueAtTime(refTime, DimIdx(1));
            perr.p2.x = centerKnob->getValueAtTime(time, DimIdx(0));
            perr.p2.y = centerKnob->getValueAtTime(time, DimIdx(1));
        } else {
            // Average halfJitter frames before and after refTime and time together to smooth the center
            std::vector<Point> x2PointJitter;

            for (double t = time - halfJitter; t <= time + halfJitter; t += 1.) {
                Point p;
                p.x = centerKnob->getValueAtTime(t, DimIdx(0));
                p.y = centerKnob->getValueAtTime(t, DimIdx(1));
                x2PointJitter.push_back(p);
            }
            Point x2avg = {0, 0};
            for (std::size_t i = 0; i < x2PointJitter.size(); ++i) {
                x2avg.x += x2PointJitter[i].x;
                x2avg.y += x2PointJitter[i].y;
            }
            if ( !x2PointJitter.empty() ) {
                x2avg.x /= x2PointJitter.size();
                x2avg.y /= x2PointJitter.size();
            }
            if (!jitterAdd) {
                perr.p1.x = centerKnob->getValueAtTime(time, DimIdx(0));
                perr.p1.y = centerKnob->getValueAtTime(time, DimIdx(1));
                perr.p2.x = x2avg.x;
                perr.p2.y = x2avg.y;
            } else {
                Point highFreqX2;

                Point x2;
                x2.x = centerKnob->getValueAtTime(time, DimIdx(0));
                x2.y = centerKnob->getValueAtTime(time, DimIdx(1));
                highFreqX2.x = x2.x - x2avg.x;
                highFreqX2.y = x2.y - x2avg.y;

                perr.p1.x = x2.x;
                perr.p1.y = x2.y;
                perr.p2.x = x2.x + highFreqX2.x;
                perr.p2.y = x2.y + highFreqX2.y;
            }
        }

        perr.error = errorKnob->getValueAtTime(time, DimIdx(0));
        ++pIndex;
    }

    std::sort(pointsWithErrors.begin(), pointsWithErrors.end(), PointWithErrorCompareLess);

    x1->resize( pointsWithErrors.size() );
    x2->resize( pointsWithErrors.size() );

    for (std::size_t i =  0; i < pointsWithErrors.size(); ++i) {
        assert(i == 0 || pointsWithErrors[i].error >= pointsWithErrors[i - 1].error);
        (*x1)[i] = pointsWithErrors[i].p1;
        (*x2)[i] = pointsWithErrors[i].p2;
    }
} // TrackerContext::extractSortedPointsFromMarkers

TrackerHelper::TransformData
TrackerHelper::computeTransformParamsFromTracksAtTime(double refTime,
                                                      double time,
                                                      int jitterPeriod,
                                                      bool jitterAdd,
                                                      bool robustModel,
                                                      const TrackerParamsProviderPtr& params,
                                                      const std::vector<TrackMarkerPtr>& allMarkers)
{
    RectD rodRef = params->getNormalizationRoD(refTime, ViewIdx(0));
    RectD rodTime = params->getNormalizationRoD(time, ViewIdx(0));
    int w1 = rodRef.width();
    int h1 = rodRef.height();
    int w2 = rodTime.width();
    int h2 = rodTime.height();

    std::vector<TrackMarkerPtr> markers;

    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        if ( allMarkers[i]->isEnabled(time) ) {
            markers.push_back(allMarkers[i]);
        }
    }
    TrackerHelper::TransformData data;
    data.rms = 0.;
    data.time = time;
    data.valid = true;
    assert( !markers.empty() );
    std::vector<Point> x1, x2;
    extractSortedPointsFromMarkers(refTime, time, markers, jitterPeriod, jitterAdd, &x1, &x2);
    assert( x1.size() == x2.size() );
    if ( x1.empty() ) {
        data.valid = false;

        return data;
    }
    if (refTime == time) {
        data.hasRotationAndScale = x1.size() > 1;
        data.translation.x = data.translation.y = data.rotation = 0;
        data.scale = 1.;

        return data;
    }


    const bool dataSetIsUserManual = true;

    try {
        if (x1.size() == 1) {
            data.hasRotationAndScale = false;
            computeTranslationFromNPoints(dataSetIsUserManual, robustModel, x1, x2, w1, h1, w2, h2, &data.translation);
        } else {
            data.hasRotationAndScale = true;
            computeSimilarityFromNPoints(dataSetIsUserManual, robustModel, x1, x2, w1, h1, w2, h2, &data.translation, &data.rotation, &data.scale, &data.rms);
        }
    } catch (...) {
        data.valid = false;
    }

    return data;
} // TrackerHelperPrivate::computeTransformParamsFromTracksAtTime

TrackerHelper::CornerPinData
TrackerHelper::computeCornerPinParamsFromTracksAtTime(double refTime,
                                                      double time,
                                                      int jitterPeriod,
                                                      bool jitterAdd,
                                                      bool robustModel,
                                                      const TrackerParamsProviderPtr& params,
                                                      const std::vector<TrackMarkerPtr>& allMarkers)
{
    RectD rodRef = params->getNormalizationRoD(refTime, ViewIdx(0));
    RectD rodTime = params->getNormalizationRoD(time, ViewIdx(0));
    int w1 = rodRef.width();
    int h1 = rodRef.height();
    int w2 = rodTime.width();
    int h2 = rodTime.height();


    std::vector<TrackMarkerPtr> markers;

    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        if ( allMarkers[i]->isEnabled(time) ) {
            markers.push_back(allMarkers[i]);
        }
    }
    TrackerHelper::CornerPinData data;
    data.rms = 0.;
    data.time = time;
    data.valid = true;
    assert( !markers.empty() );
    std::vector<Point> x1, x2;
    extractSortedPointsFromMarkers(refTime, time, markers, jitterPeriod, jitterAdd, &x1, &x2);
    assert( x1.size() == x2.size() );
    if ( x1.empty() ) {
        data.valid = false;

        return data;
    }
    if (refTime == time) {
        data.h.setIdentity();
        data.nbEnabledPoints = 4;

        return data;
    }


    if (x1.size() == 1) {
        data.h.setTranslationFromOnePoint( euclideanToHomogenous(x1[0]), euclideanToHomogenous(x2[0]) );
        data.nbEnabledPoints = 1;
    } else if (x1.size() == 2) {
        data.h.setSimilarityFromTwoPoints( euclideanToHomogenous(x1[0]), euclideanToHomogenous(x1[1]), euclideanToHomogenous(x2[0]), euclideanToHomogenous(x2[1]) );
        data.nbEnabledPoints = 2;
    } else if (x1.size() == 3) {
        data.h.setAffineFromThreePoints( euclideanToHomogenous(x1[0]), euclideanToHomogenous(x1[1]), euclideanToHomogenous(x1[2]), euclideanToHomogenous(x2[0]), euclideanToHomogenous(x2[1]), euclideanToHomogenous(x2[2]) );
        data.nbEnabledPoints = 3;
    } else {
        const bool dataSetIsUserManual = true;
        try {
            computeHomographyFromNPoints(dataSetIsUserManual, robustModel, x1, x2, w1, h1, w2, h2, &data.h, &data.rms);
            data.nbEnabledPoints = 4;
        } catch (...) {
            data.valid = false;
        }
    }

    return data;
} // TrackerHelperPrivate::computeCornerPinParamsFromTracksAtTime





NATRON_NAMESPACE_EXIT;

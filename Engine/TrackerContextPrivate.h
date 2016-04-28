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

#ifndef TRACKERCONTEXTPRIVATE_H
#define TRACKERCONTEXTPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Engine/TrackerContext.h"

#include <list>

#include "Global/Macros.h"

GCC_DIAG_OFF(unused-function)
GCC_DIAG_OFF(unused-parameter)
#include <libmv/autotrack/autotrack.h>
#include <libmv/logging/logging.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)

#include <openMVG/robust_estimation/robust_estimator_Prosac.hpp>

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>

#include <boost/bind.hpp>

#include "Engine/RectD.h"
#include "Engine/EngineFwd.h"


#define kTrackBaseName "track"
#define TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE 8

/// Parameters definitions

//////// Global to all tracks
#define kTrackerParamTrackRed "trackRed"
#define kTrackerParamTrackRedLabel "Track Red"
#define kTrackerParamTrackRedHint "Enable tracking on the red channel"

#define kTrackerParamTrackGreen "trackGreen"
#define kTrackerParamTrackGreenLabel "Track Green"
#define kTrackerParamTrackGreenHint "Enable tracking on the green channel"

#define kTrackerParamTrackBlue "trackBlue"
#define kTrackerParamTrackBlueLabel "Track Blue"
#define kTrackerParamTrackBlueHint "Enable tracking on the blue channel"

#define kTrackerParamMaxError "maxError"
#define kTrackerParamMaxErrorLabel "Max. Error"
#define kTrackerParamMaxErrorHint "This is the minimum necessary error between the final tracked " \
    "position of the patch on the destination image and the reference patch needed to declare tracking success." \
    "The error is 1 minus the normalized cross-correlation score."

#define kTrackerParamMaximumIteration "maxIterations"
#define kTrackerParamMaximumIterationLabel "Maximum iterations"
#define kTrackerParamMaximumIterationHint "Maximum number of iterations the algorithm will run for the inner minimization " \
    "before it gives up."

#define kTrackerParamBruteForcePreTrack "bruteForcePretTrack"
#define kTrackerParamBruteForcePreTrackLabel "Use brute-force pre-track"
#define kTrackerParamBruteForcePreTrackHint "Use a brute-force translation-only pre-track before refinement"

#define kTrackerParamNormalizeIntensities "normalizeIntensities"
#define kTrackerParamNormalizeIntensitiesLabel "Normalize Intensities"
#define kTrackerParamNormalizeIntensitiesHint "Normalize the image patches by their mean before doing the sum of squared" \
    " error calculation. Slower."

#define kTrackerParamPreBlurSigma "preBlurSigma"
#define kTrackerParamPreBlurSigmaLabel "Pre-blur sigma"
#define kTrackerParamPreBlurSigmaHint "The size in pixels of the blur kernel used to both smooth the image and take the image derivative."

#define kTrackerParamExportDataSeparator "exportDataSection"
#define kTrackerParamExportDataSeparatorLabel "Export"

#define kTrackerParamExportLink "exportLink"
#define kTrackerParamExportLinkLabel "Link"
#define kTrackerParamExportLinkHint "When checked, the node created will be linked to the parameters from this tab. When unchecked, the node created will copy the animation of all the parameters in this tab but will not be updated if any modification is made to this tab's parameters."

#define kTrackerParamExportUseCurrentFrame "exportUseCurrentFrame"
#define kTrackerParamExportUseCurrentFrameLabel "Use Current Frame"
#define kTrackerParamExportUseCurrentFrameHint "When checked, the exported Transform or CornerPin node will be identity at the current timeline's time. When unchecked, it will be identity at the frame indicated by the Reference Frame parameter."

#define kTrackerParamExportButton "export"
#define kTrackerParamExportButtonLabel "Export"
#define kTrackerParamExportButtonHint "Creates a node referencing the tracked data. The node type depends on the node selected by the Transform Type parameter. " \
    "The type of transformation applied by the created node depends on the Motion Type parameter. To activate this button you must select set the Motion Type to something other than None"

#define kCornerPinInvertParamName "invert"

#define kTrackerParamTransformType "transformType"
#define kTrackerParamTransformTypeLabel "Transform Type"
#define kTrackerParamTransformTypeHint "The type of transform used to produce the results."

#define kTrackerParamTransformTypeTransform "Transform"
#define kTrackerParamTransformTypeTransformHelp "The tracks motion will be used to compute the translation, scale and rotation parameter " \
    "of a Transform node. At least 1 track is required to compute the translation and 2 for scale and rotation. The more tracks you use, " \
    "the more stable and precise the resulting transform will be."

#define kTrackerParamTransformTypeCornerPin "CornerPin"
#define kTrackerParamTransformTypeCornerPinHelp "The tracks motion will be used to compute a CornerPin. " \
    "A CornerPin is useful if you are tracking an image portion that has a perspective distortion. " \
    "At least 1 track is required to compute the homography transforming the \"From\" points to the \"To\" points, and 4 required to " \
    "track a perspective transformation." \
    "The more points you add, the more stable and precise the resulting CornerPin will be."

#define kTrackerParamMotionType "motionType"
#define kTrackerParamMotionTypeLabel "Motion Type"
#define kTrackerParamMotionTypeHint "The type of motion in output of this node."

#define kTrackerParamMotionTypeNone "None"
#define kTrackerParamMotionTypeNoneHelp "No transformation applied in output to the image: this node is a pass-through. Set it to this mode when tracking to correclty see the input image on the viewer"

#define kTrackerParamMotionTypeStabilize "Stabilize"
#define kTrackerParamMotionTypeStabilizeHelp "Transforms the image so that the tracked points do not move"

#define kTrackerParamMotionTypeMatchMove "Match-Move"
#define kTrackerParamMotionTypeMatchMoveHelp "Transforms a different image so that it moves to match the tracked points"

#define kTrackerParamMotionTypeRemoveJitter "Remove Jitter"
#define kTrackerParamMotionTypeRemoveJitterHelp "Transforms the image so that the tracked points move smoothly with high frequencies removed"

#define kTrackerParamMotionTypeAddJitter "Add Jitter"
#define kTrackerParamMotionTypeAddJitterHelp "Transforms the image by the high frequencies of the animation of the tracks to increase the shake or apply it on another image"

#define kTrackerParamReferenceFrame "referenceFrame"
#define kTrackerParamReferenceFrameLabel "Reference frame"
#define kTrackerParamReferenceFrameHint "When exporting tracks to a CornerPin or Transform, this will be the frame number at which the transform will be an identity."

#define kTrackerParamSetReferenceFrame "setReferenceButton"
#define kTrackerParamSetReferenceFrameLabel "Set To Current Frame"
#define kTrackerParamSetReferenceFrameHint "Set the reference frame to the timeline's current frame"

#define kTrackerParamJitterPeriod "jitterPeriod"
#define kTrackerParamJitterPeriodLabel "Jitter Period"
#define kTrackerParamJitterPeriodHint "Number of frames to average together to remove high frequencies for the add/remove jitter transform type"

#define kTrackerParamSmooth "smooth"
#define kTrackerParamSmoothLabel "Smooth"
#define kTrackerParamSmoothHint "Smooth the translation/rotation/scale by averaging this number of frames together"

#define kTrackerParamSmoothCornerPin "smoothCornerPin"
#define kTrackerParamSmoothCornerPinLabel "Smooth"
#define kTrackerParamSmoothCornerPinHint "Smooth the Corner Pin by averaging this number of frames together"


#define kTransformParamTranslate "translate"
#define kTransformParamRotate "rotate"
#define kTransformParamScale "scale"
#define kTransformParamUniform "uniform"
#define kTransformParamSkewX "skewX"
#define kTransformParamSkewY "skewY"
#define kTransformParamSkewOrder "skewOrder"
#define kTransformParamCenter "center"
#define kTransformParamInvert "invert"
#define kTransformParamFilter "filter"
#define kTransformParamClamp "clamp"
#define kTransformParamBlackOutside "black_outside"
#define kTransformParamMotionBlur "motionBlur"
#define kTransformParamShutter "shutter"
#define kTransformParamShutterOffset "shutterOffset"
#define kTransformParamCustomShutterOffset "shutterCustomOffset"

#define kCornerPinParamFrom "from"
#define kCornerPinParamTo "to"

#define kCornerPinParamFrom1 "from1"
#define kCornerPinParamFrom2 "from2"
#define kCornerPinParamFrom3 "from3"
#define kCornerPinParamFrom4 "from4"

#define kCornerPinParamTo1 "to1"
#define kCornerPinParamTo2 "to2"
#define kCornerPinParamTo3 "to3"
#define kCornerPinParamTo4 "to4"

#define kCornerPinParamEnable1 "enable1"
#define kCornerPinParamEnable2 "enable2"
#define kCornerPinParamEnable3 "enable3"
#define kCornerPinParamEnable4 "enable4"

#define kCornerPinParamOverlayPoints "overlayPoints"

#define kCornerPinParamMatrix "transform"


NATRON_NAMESPACE_ENTER;


enum TrackerMotionTypeEnum
{
    eTrackerMotionTypeNone,
    eTrackerMotionTypeStabilize,
    eTrackerMotionTypeMatchMove,
    eTrackerMotionTypeRemoveJitter,
    eTrackerMotionTypeAddJitter
};

enum TrackerTransformNodeEnum
{
    eTrackerTransformNodeTransform,
    eTrackerTransformNodeCornerPin
};

enum libmv_MarkerChannelEnum
{
    LIBMV_MARKER_CHANNEL_R = (1 << 0),
    LIBMV_MARKER_CHANNEL_G = (1 << 1),
    LIBMV_MARKER_CHANNEL_B = (1 << 2),
};

struct TrackMarkerAndOptions
{
    TrackMarkerPtr natronMarker;
    mv::Marker mvMarker;
    mv::TrackRegionOptions mvOptions;
};


class TrackArgsLibMV
{
    int _start, _end;
    int _step;
    boost::shared_ptr<TimeLine> _timeline;
    ViewerInstance* _viewer;
    boost::shared_ptr<mv::AutoTrack> _libmvAutotrack;
    boost::shared_ptr<TrackerFrameAccessor> _fa;
    std::vector<boost::shared_ptr<TrackMarkerAndOptions> > _tracks;

    //Store the format size because LibMV internally has a top-down Y axis
    double _formatWidth, _formatHeight;
    mutable QMutex _autoTrackMutex;

public:

    TrackArgsLibMV()
        : _start(0)
        , _end(0)
        , _step(1)
        , _timeline()
        , _viewer(0)
        , _libmvAutotrack()
        , _fa()
        , _tracks()
        , _formatWidth(0)
        , _formatHeight(0)
    {
    }

    TrackArgsLibMV(int start,
                   int end,
                   int step,
                   const boost::shared_ptr<TimeLine>& timeline,
                   ViewerInstance* viewer,
                   const boost::shared_ptr<mv::AutoTrack>& autoTrack,
                   const boost::shared_ptr<TrackerFrameAccessor>& fa,
                   const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& tracks,
                   double formatWidth,
                   double formatHeight)
        : _start(start)
        , _end(end)
        , _step(step)
        , _timeline(timeline)
        , _viewer(viewer)
        , _libmvAutotrack(autoTrack)
        , _fa(fa)
        , _tracks(tracks)
        , _formatWidth(formatWidth)
        , _formatHeight(formatHeight)
    {
    }

    TrackArgsLibMV(const TrackArgsLibMV& other)
    {
        *this = other;
    }

    void operator=(const TrackArgsLibMV& other)
    {
        _start = other._start;
        _end = other._end;
        _step = other._step;
        _timeline = other._timeline;
        _viewer = other._viewer;
        _libmvAutotrack = other._libmvAutotrack;
        _fa = other._fa;
        _tracks = other._tracks;
        _formatWidth = other._formatWidth;
        _formatHeight = other._formatHeight;
    }

    double getFormatHeight() const
    {
        return _formatHeight;
    }

    double getFormatWidth() const
    {
        return _formatWidth;
    }

    QMutex* getAutoTrackMutex() const
    {
        return &_autoTrackMutex;
    }

    int getStart() const
    {
        return _start;
    }

    int getEnd() const
    {
        return _end;
    }

    int getStep() const
    {
        return _step;
    }

    boost::shared_ptr<TimeLine> getTimeLine() const
    {
        return _timeline;
    }

    ViewerInstance* getViewer() const
    {
        return _viewer;
    }

    int getNumTracks() const
    {
        return (int)_tracks.size();
    }

    const std::vector<boost::shared_ptr<TrackMarkerAndOptions> >& getTracks() const
    {
        return _tracks;
    }

    boost::shared_ptr<mv::AutoTrack> getLibMVAutoTrack() const
    {
        return _libmvAutotrack;
    }

    void getEnabledChannels(bool* r, bool* g, bool* b) const;

    void getRedrawAreasNeeded(int time, std::list<RectD>* canonicalRects) const;
};


class TrackerContextPrivate
    : public QObject
{
    Q_OBJECT

public:


    TrackerContext * _publicInterface;
    boost::weak_ptr<Node> node;
    std::list<boost::weak_ptr<KnobI> > perTrackKnobs;
    boost::weak_ptr<KnobBool> enableTrackRed, enableTrackGreen, enableTrackBlue;
    boost::weak_ptr<KnobDouble> maxError;
    boost::weak_ptr<KnobInt> maxIterations;
    boost::weak_ptr<KnobBool> bruteForcePreTrack, useNormalizedIntensities;
    boost::weak_ptr<KnobDouble> preBlurSigma;
    boost::weak_ptr<KnobBool> activateTrack;
    boost::weak_ptr<KnobSeparator> exportDataSep;
    boost::weak_ptr<KnobBool> exportLink;
    boost::weak_ptr<KnobButton> exportButton;
    NodeWPtr transformNode, cornerPinNode;
    boost::weak_ptr<KnobPage> transformPageKnob;
    boost::weak_ptr<KnobSeparator> transformGenerationSeparator;
    boost::weak_ptr<KnobChoice> transformType, motionType;
    boost::weak_ptr<KnobInt> referenceFrame;
    boost::weak_ptr<KnobButton> setCurrentFrameButton;
    boost::weak_ptr<KnobInt> jitterPeriod;
    boost::weak_ptr<KnobInt> smoothTransform;
    boost::weak_ptr<KnobInt> smoothCornerPin;
    boost::weak_ptr<KnobSeparator> transformControlsSeparator;
    boost::weak_ptr<KnobDouble> translate;
    boost::weak_ptr<KnobDouble> rotate;
    boost::weak_ptr<KnobDouble> scale;
    boost::weak_ptr<KnobBool> scaleUniform;
    boost::weak_ptr<KnobDouble> skewX;
    boost::weak_ptr<KnobDouble> skewY;
    boost::weak_ptr<KnobChoice> skewOrder;
    boost::weak_ptr<KnobDouble> center;
    boost::weak_ptr<KnobBool> invertTransform;
    boost::weak_ptr<KnobChoice> filter;
    boost::weak_ptr<KnobBool> clamp;
    boost::weak_ptr<KnobBool> blackOutside;
    boost::weak_ptr<KnobDouble> motionBlur;
    boost::weak_ptr<KnobDouble> shutter;
    boost::weak_ptr<KnobChoice> shutterOffset;
    boost::weak_ptr<KnobDouble> customShutterOffset;
    boost::weak_ptr<KnobGroup> fromGroup, toGroup;
    boost::weak_ptr<KnobDouble> fromPoints[4], toPoints[4];
    boost::weak_ptr<KnobBool> enableToPoint[4];
    boost::weak_ptr<KnobDouble> cornerPinMatrix;
    boost::weak_ptr<KnobChoice> cornerPinOverlayPoints;
    mutable QMutex trackerContextMutex;
    std::vector<TrackMarkerPtr > markers;
    std::list<TrackMarkerPtr > selectedMarkers, markersToSlave, markersToUnslave;
    int beginSelectionCounter;
    int selectionRecursion;

    TrackScheduler<TrackArgsLibMV> scheduler;


    struct TransformData
    {
        Point translation;
        double rotation;
        double scale;
        bool hasRotationAndScale;
        double time;
        bool valid;
    };

    struct CornerPinData
    {
        Transform::Matrix3x3 h;
        int nbEnabledPoints;
        double time;
        bool valid;
    };

    typedef boost::shared_ptr<QFutureWatcher<CornerPinData> > CornerPinSolverWatcher;
    typedef boost::shared_ptr<QFutureWatcher<TransformData> > TransformSolverWatcher;

    struct SolveRequest
    {
        CornerPinSolverWatcher cpWatcher;
        TransformSolverWatcher tWatcher;
        double refTime;
        std::set<double> keyframes;
        int jitterPeriod;
        bool jitterAdd;
        std::vector<TrackMarkerPtr> allMarkers;
    };

    SolveRequest lastSolveRequest;


    TrackerContextPrivate(TrackerContext* publicInterface, const boost::shared_ptr<Node> &node);

    virtual ~TrackerContextPrivate()
    {
    }

    /// Make all calls to getValue() that are global to the tracker context in here
    void beginLibMVOptionsForTrack(mv::TrackRegionOptions* options) const;

    /// Make all calls to getValue() that are local to the track in here
    void endLibMVOptionsForTrack(const TrackMarker& marker,
                                 mv::TrackRegionOptions* options) const;

    void addToSelectionList(const TrackMarkerPtr& marker)
    {
        if ( std::find(selectedMarkers.begin(), selectedMarkers.end(), marker) != selectedMarkers.end() ) {
            return;
        }
        selectedMarkers.push_back(marker);
        markersToSlave.push_back(marker);
    }

    void removeFromSelectionList(const TrackMarkerPtr& marker)
    {
        std::list<TrackMarkerPtr >::iterator found = std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);

        if ( found == selectedMarkers.end() ) {
            return;
        }
        selectedMarkers.erase(found);
        markersToUnslave.push_back(marker);
    }

    void incrementSelectionCounter()
    {
        ++beginSelectionCounter;
    }

    void decrementSelectionCounter()
    {
        if (beginSelectionCounter > 0) {
            --beginSelectionCounter;
        }
    }

    void linkMarkerKnobsToGuiKnobs(const std::list<TrackMarkerPtr >& markers,
                                   bool multipleTrackSelected,
                                   bool slave);

    void createTransformFromSelection(bool linked);


    void refreshVisibilityFromTransformType();
    void refreshVisibilityFromTransformTypeInternal(TrackerTransformNodeEnum transformType);

    RectD getInputRoDAtTime(double time) const;


    static void natronTrackerToLibMVTracker(bool isReferenceMarker,
                                            bool trackChannels[3],
                                            const TrackMarker &marker,
                                            int trackIndex,
                                            int time,
                                            int frameStep,
                                            double formatHeight,
                                            mv::Marker * mvMarker);
    static void setKnobKeyframesFromMarker(const mv::Marker& mvMarker,
                                           int formatHeight,
                                           const libmv::TrackRegionResult* result,
                                           const TrackMarkerPtr& natronMarker);
    static bool trackStepLibMV(int trackIndex, const TrackArgsLibMV& args, int time);


    /**
     * @brief Computes the translation that best fit the set of correspondences x1 and x2.
     * Requires at least 1 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeTranslationFromNPoints(const std::vector<Point>& x1,
                                              const std::vector<Point>& x2,
                                              int w1, int h1, int w2, int h2,
                                              Point* translation);

    /**
     * @brief Computes the translation, rotation and scale that best fit the set of correspondences x1 and x2.
     * Requires at least 2 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeSimilarityFromNPoints(const std::vector<Point>& x1,
                                             const std::vector<Point>& x2,
                                             int w1, int h1, int w2, int h2,
                                             Point* translation,
                                             double* rotate,
                                             double* scale);
    /**
     * @brief Computes the homography that best fit the set of correspondences x1 and x2.
     * Requires at least 4 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeHomographyFromNPoints(const std::vector<Point>& x1,
                                             const std::vector<Point>& x2,
                                             int w1, int h1, int w2, int h2,
                                             Transform::Matrix3x3* homog);

    /**
     * @brief Computes the fundamental matrix that best fit the set of correspondences x1 and x2.
     * Requires at least 7 point. x1 and x2 must have the same size.
     * This function throws an exception with an error message upon failure.
     **/
    static void computeFundamentalFromNPoints(const std::vector<Point>& x1,
                                              const std::vector<Point>& x2,
                                              int w1, int h1, int w2, int h2,
                                              Transform::Matrix3x3* fundamental);

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
    static void extractSortedPointsFromMarkers(double x1Time, double x2Time,
                                               const std::vector<TrackMarkerPtr>& markers,
                                               int jitterPeriod,
                                               bool jitterAdd,
                                               std::vector<Point>* x1,
                                               std::vector<Point>* x2);


    TransformData computeTransformParamsFromTracksAtTime(double refTime,
                                                         double time,
                                                         int jitterPeriod,
                                                         bool jitterAdd,
                                                         const std::vector<TrackMarkerPtr>& allMarkers);

    CornerPinData computeCornerPinParamsFromTracksAtTime(double refTime,
                                                         double time,
                                                         int jitterPeriod,
                                                         bool jitterAdd,
                                                         const std::vector<TrackMarkerPtr>& allMarkers);


    void resetTransformParamsAnimation();

    void computeTransformParamsFromTracks();

    void computeTransformParamsFromTracksEnd(double refTime,
                                             const QList<TransformData>& results);

    void computeCornerParamsFromTracks();

    void computeCornerParamsFromTracksEnd(double refTime,
                                          const QList<CornerPinData>& results);

    void setSolverParamsEnabled(bool enabled);

    void endSolve();

public Q_SLOTS:

    void onCornerPinSolverWatcherFinished();
    void onTransformSolverWatcherFinished();

    void onCornerPinSolverWatcherProgress(int progress);
    void onTransformSolverWatcherProgress(int progress);
};

NATRON_NAMESPACE_EXIT;

#endif // TRACKERCONTEXTPRIVATE_H

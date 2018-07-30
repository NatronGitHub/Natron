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

#ifndef TRACKERCONTEXTPRIVATE_H
#define TRACKERCONTEXTPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include "TrackerContext.h"

#include <list>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/utility.hpp>
#endif

GCC_DIAG_OFF(unused-function)
GCC_DIAG_OFF(unused-parameter)
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_OFF(maybe-uninitialized)
#endif
#include <libmv/autotrack/autotrack.h>
#include <libmv/autotrack/predict_tracks.h>
#include <libmv/logging/logging.h>
GCC_DIAG_ON(unused-function)
GCC_DIAG_ON(unused-parameter)

GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <openMVG/robust_estimation/robust_estimator_Prosac.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#if ( ( __GNUC__ * 100) + __GNUC_MINOR__) >= 408
GCC_DIAG_ON(maybe-uninitialized)
#endif

#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>

#include <boost/bind.hpp>

#include "Engine/CreateNodeArgs.h"
#include "Engine/NodeSerialization.h"
#include "Engine/RectD.h"
#include "Engine/EngineFwd.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerFrameAccessor.h"


#define kTrackBaseName "track"


#ifdef DEBUG
// Enable usage of markers that track using TrackerPM internally
#define NATRON_TRACKER_ENABLE_TRACKER_PM
#endif

#define TRACKER_MAX_TRACKS_FOR_PARTIAL_VIEWER_UPDATE 8

/// Parameters definitions

//////// Global to all tracks
#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM

#define kTrackerParamUsePatternMatching "usePatternMatching"
#define kTrackerParamUsePatternMatchingLabel "Use Pattern Matching"
#define kTrackerParamUsePatternMatchingHint "When enabled, the tracker will internally make use of the TrackerPM OpenFX plug-in to track the marker instead of LibMV. TrackerPM is using a pattern-matching method. " \
    "Note that this is only applied to markers created after changing this parameter. Markers that existed prior to any change will continue using the method they were using when created"

#define kTrackerParamPatternMatchingScoreType "pmScoreType"
#define kTrackerParamPatternMatchingScoreTypeLabel "Score Type"
#define kTrackerParamPatternMatchingScoreTypeHint "Correlation score computation method"

#define kTrackerParamPatternMatchingScoreOptionSSD "SSD"
#define kTrackerParamPatternMatchingScoreOptionSSDHint "Sum of Squared Differences"
#define kTrackerParamPatternMatchingScoreOptionSAD "SAD"
#define kTrackerParamPatternMatchingScoreOptionSADHint "Sum of Absolute Differences, more robust to occlusions"
#define kTrackerParamPatternMatchingScoreOptionNCC "NCC"
#define kTrackerParamPatternMatchingScoreOptionNCCHint "Normalized Cross-Correlation"
#define kTrackerParamPatternMatchingScoreOptionZNCC "ZNCC"
#define kTrackerParamPatternMatchingScoreOptionZNCCHint "Zero-mean Normalized Cross-Correlation, less sensitive to illumination changes"

#endif // NATRON_TRACKER_ENABLE_TRACKER_PM


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

#define kTrackerParamDefaultMarkerPatternWinSize "defPatternWinSize"
#define kTrackerParamDefaultMarkerPatternWinSizeLabel "Default Pattern Size"
#define kTrackerParamDefaultMarkerPatternWinSizeHint "The size in pixels of the pattern that created markers will have by default"

#define kTrackerParamDefaultMarkerSearchWinSize "defSearchWinSize"
#define kTrackerParamDefaultMarkerSearchWinSizeLabel "Default Search Area Size"
#define kTrackerParamDefaultMarkerSearchWinSizeHint "The size in pixels of the search window that created markers will have by default"

#define kTrackerParamDefaultMotionModel "defMotionModel"
#define kTrackerParamDefaultMotionModelLabel "Default Motion Model"
#define kTrackerParamDefaultMotionModelHint "The motion model that new tracks have by default"

#define kTrackerParamMaximumIteration "maxIterations"
#define kTrackerParamMaximumIterationLabel "Maximum Iterations"
#define kTrackerParamMaximumIterationHint "Maximum number of iterations the algorithm will run for the inner minimization " \
    "before it gives up."

#define kTrackerParamBruteForcePreTrack "bruteForcePretTrack"
#define kTrackerParamBruteForcePreTrackLabel "Use Brute-Force Pre-track"
#define kTrackerParamBruteForcePreTrackHint "Use a brute-force translation-only pre-track before refinement"

#define kTrackerParamNormalizeIntensities "normalizeIntensities"
#define kTrackerParamNormalizeIntensitiesLabel "Normalize Intensities"
#define kTrackerParamNormalizeIntensitiesHint "Normalize the image patches by their mean before doing the sum of squared" \
    " error calculation. Slower."

#define kTrackerParamPreBlurSigma "preBlurSigma"
#define kTrackerParamPreBlurSigmaLabel "Pre-blur Sigma"
#define kTrackerParamPreBlurSigmaHint "The size in pixels of the blur kernel used to both smooth the image and take the image derivative."


#define kTrackerParamAutoKeyEnabled "autoKeyEnabled"
#define kTrackerParamAutoKeyEnabledLabel "Animate Enabled"
#define kTrackerParamAutoKeyEnabledHint "When checked, the \"Enabled\" parameter will be keyframed automatically when a track fails. " \
"This is useful for example if you have a scene with a moving camera that you want to stabilize: You place track markers, and when " \
"the tracker looses them, they get disabled automatically and you can place new ones. A disabled marker will not be taken into account"\
" when computing the resulting Transform to stabilize/match-move."

#define kTrackerParamPerTrackParamsSeparator "perTrackParams"
#define kTrackerParamPerTrackParamsSeparatorLabel "Per-track Parameters"

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

#define kTrackerParamRobustModel "robustModel"
#define kTrackerParamRobustModelLabel "Robust Model"
#define kTrackerParamRobustModelHint "When checked, the solver will assume that the model generated (i.e: the Transform or the CornerPin) is possible given the motion of the video and will eliminate points that do not match the model to compute the resulting parameters. " \
    "When unchecked, the solver assumes that all points that are enabled and have a keyframe are valid and fit the model: this may in some situations work better if you are trying to find a model that is just not correct for the given motion of the video."

#define kTrackerParamFittingError "fittingError"
#define kTrackerParamFittingErrorLabel "Fitting Error (px)"
#define kTrackerParamFittingErrorHint "This parameter indicates the error for each frame of the fitting of the model (i.e: Transform / CornerPin) to the tracks data. This value is in pixels and represents the rooted weighted sum of squared errors for each track. The error is " \
    "essentially the difference between the point position computed from the original point onto which is applied the fitted model and the original tracked point. "

#define kTrackerParamFittingErrorWarnValue "fittingErrorWarnAbove"
#define kTrackerParamFittingErrorWarnValueLabel "Warn If Error Is Above"
#define kTrackerParamFittingErrorWarnValueHint "A warning will appear if the model fitting error reaches this value (or higher). The warning indicates that " \
    "the calculated model is probably poorly suited for the stabilization/match-move you want to achieve and you should either refine your tracking data or pick " \
    "another model"

#define kTrackerParamFittingErrorWarning "fittingErrorWarning"
#define kTrackerParamFittingErrorWarningLabel "Incorrect model"
#define kTrackerParamFittingErrorWarningHint "This warning indicates that " \
    "the calculated model is probably poorly suited for the stabilization/match-move you want to achieve and you should either refine your tracking data or pick " \
    "another model"

#define kTrackerParamReferenceFrame "referenceFrame"
#define kTrackerParamReferenceFrameLabel "Reference Frame"
#define kTrackerParamReferenceFrameHint "When exporting tracks to a CornerPin or Transform, this will be the frame number at which the transform will be an identity."

#define kTrackerParamSetReferenceFrame "setReferenceButton"
#define kTrackerParamSetReferenceFrameLabel "Set to Current Frame"
#define kTrackerParamSetReferenceFrameHint "Set the reference frame to the timeline's current frame"

#define kTrackerParamJitterPeriod "jitterPeriod"
#define kTrackerParamJitterPeriodLabel "Jitter Period"
#define kTrackerParamJitterPeriodHint "Number of frames to average together to remove high frequencies for the add/remove jitter transform type"

#define kTrackerParamSmooth "smooth"
#define kTrackerParamSmoothLabel "Smooth"
#define kTrackerParamSmoothHint "Smooth the translation/rotation/scale by averaging this number of frames together"

#define kTrackerParamSmoothCornerPin "smoothCornerPin"
#define kTrackerParamSmoothCornerPinLabel "Smooth"
#define kTrackerParamSmoothCornerPinHint "Smooth the CornerPin by averaging this number of frames together"

#define kTrackerParamAutoGenerateTransform "autoComputeransform"
#define kTrackerParamAutoGenerateTransformLabel "Compute Transform Automatically"
#define kTrackerParamAutoGenerateTransformHint "When checked, whenever changing a parameter controlling the Transform Generation (such as Motion Type, Transform Type, Reference Frame, etc...) or changing the Enabled parameter of a track, the transform parameters will be re-computed automatically. " \
    "When unchecked, you must press the Compute button to compute it."

#define kTrackerParamGenerateTransform "computeTransform"
#define kTrackerParamGenerateTransformLabel "Compute"
#define kTrackerParamGenerateTransformHint "Click to compute the parameters of the Transform Controls or CornerPin Controls (depending on the Transform Type) from the data acquired on the tracks during the tracking. This should be done after the tracking is finished and when you feel the results are satisfying. " \
    "For each frame, the resulting parameter is computed from the tracks that are enabled at this frame and that have a keyframe on the center point (e.g: are valid)."

#define kTrackerParamTransformOutOfDate "transformOutOfDate"
#define kTrackerParamTransformOutOfDateHint "The Transform parameters are out of date because parameters that control their generation have been changed, please click the Compute button to refresh them"

#define kTrackerParamDisableTransform "disableProcess"
#define kTrackerParamDisableTransformLabel "Disable Transform"
#define kTrackerParamDisableTransformHint "When checked, the CornerPin/Transform applied by the parameters is disabled temporarily. This is useful if " \
    "you are using a CornerPin and you need to edit the From or To points. " \
    "For example, in match-move mode to replace a portion of the image by another one. To achieve such effect, you would need to place " \
    "the From points of the CornerPin controls to the desired 4 corners in the image. Similarly, you may want to stabilize the image onto a moving vehicule, in " \
    "which case you would want to set the CornerPin points to enclose the vehicule."

#define kTrackerParamCornerPinFromPointsSetOnce "fromPointsSet"

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

#define kCornerPinParamSetToInputRoD "setToInputRod"
#define kCornerPinParamSetToInputRoDLabel "Set to Input Rod"
#define kCornerPinParamSetToInputRoDHint "Set the 4 from points to the image rectangle in input of the tracker node"

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


NATRON_NAMESPACE_ENTER


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

class TrackMarkerAndOptions
{
public:
    TrackMarkerPtr natronMarker;
    mv::Marker mvMarker;
    mv::TrackRegionOptions mvOptions;
    mv::KalmanFilterState mvState;
};


class TrackerContextPrivate
    : public QObject
{
    Q_OBJECT

public:


    TrackerContext * _publicInterface;
    NodeWPtr node;
    std::list<KnobIWPtr> perTrackKnobs;

#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    KnobBoolWPtr usePatternMatching;
    KnobChoiceWPtr patternMatchingScore;
#endif

    KnobPageWPtr trackingPageKnob;
    KnobBoolWPtr enableTrackRed, enableTrackGreen, enableTrackBlue;
    KnobDoubleWPtr maxError;
    KnobIntWPtr maxIterations;
    KnobIntWPtr defaultSearchWinSize, defaultPatternWinSize;
    KnobChoiceWPtr defaultMotionModel;
    KnobBoolWPtr bruteForcePreTrack, useNormalizedIntensities;
    KnobDoubleWPtr preBlurSigma;
    KnobSeparatorWPtr perTrackParamsSeparator;
    KnobBoolWPtr activateTrack;
    KnobBoolWPtr autoKeyEnabled;
    KnobChoiceWPtr motionModel;
    KnobSeparatorWPtr exportDataSep;
    KnobBoolWPtr exportLink;
    KnobButtonWPtr exportButton;
    NodeWPtr transformNode, cornerPinNode;
    KnobPageWPtr transformPageKnob;
    KnobSeparatorWPtr transformGenerationSeparator;
    KnobChoiceWPtr transformType, motionType;
    KnobBoolWPtr robustModel;
    KnobDoubleWPtr fittingError;
    KnobDoubleWPtr fittingErrorWarnIfAbove;
    KnobStringWPtr fittingErrorWarning;
    KnobIntWPtr referenceFrame;
    KnobButtonWPtr setCurrentFrameButton;
    KnobIntWPtr jitterPeriod;
    KnobIntWPtr smoothTransform;
    KnobIntWPtr smoothCornerPin;
    KnobBoolWPtr autoGenerateTransform;
    KnobButtonWPtr generateTransformButton;
    KnobStringWPtr transformOutOfDateLabel;
    KnobSeparatorWPtr transformControlsSeparator;
    KnobBoolWPtr disableTransform;
    KnobDoubleWPtr translate;
    KnobDoubleWPtr rotate;
    KnobDoubleWPtr scale;
    KnobBoolWPtr scaleUniform;
    KnobDoubleWPtr skewX;
    KnobDoubleWPtr skewY;
    KnobChoiceWPtr skewOrder;
    KnobDoubleWPtr center;
    KnobBoolWPtr invertTransform;
    KnobChoiceWPtr filter;
    KnobBoolWPtr clamp;
    KnobBoolWPtr blackOutside;
    KnobDoubleWPtr motionBlur;
    KnobDoubleWPtr shutter;
    KnobChoiceWPtr shutterOffset;
    KnobDoubleWPtr customShutterOffset;
    KnobGroupWPtr fromGroup, toGroup;
    KnobDoubleWPtr fromPoints[4], toPoints[4];
    KnobButtonWPtr setFromPointsToInputRod;
    KnobBoolWPtr cornerPinFromPointsSetOnceAutomatically;
    KnobBoolWPtr enableToPoint[4];
    KnobBoolWPtr enableTransform;
    KnobDoubleWPtr cornerPinMatrix;
    KnobChoiceWPtr cornerPinOverlayPoints;
    mutable QMutex trackerContextMutex;
    std::vector<TrackMarkerPtr> markers;
    std::list<TrackMarkerPtr> selectedMarkers, markersToSlave, markersToUnslave;
    int beginSelectionCounter;
    int selectionRecursion;
    TrackScheduler scheduler;
    struct TransformData
    {
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
        double time;
        bool valid;
        double rms;
    };

    struct CornerPinData
    {
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
        double time;
        bool valid;
        double rms;
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
        bool robustModel;
        double maxFittingError;
        std::vector<TrackMarkerPtr> allMarkers;
    };

    SolveRequest lastSolveRequest;


    TrackerContextPrivate(TrackerContext* publicInterface,
                          const NodePtr &node);

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
        std::list<TrackMarkerPtr>::iterator found = std::find(selectedMarkers.begin(), selectedMarkers.end(), marker);

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

    void linkMarkerKnobsToGuiKnobs(const std::list<TrackMarkerPtr>& markers,
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
    static bool trackStepLibMV(int trackIndex, const TrackArgs& args, int time);
    static bool trackStepTrackerPM(TrackMarkerPM* tracker, const TrackArgs& args, int time);


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
    static void extractSortedPointsFromMarkers(double x1Time, double x2Time,
                                               const std::vector<TrackMarkerPtr>& markers,
                                               int jitterPeriod,
                                               bool jitterAdd,
                                               const KnobDoublePtr& center,
                                               std::vector<Point>* x1,
                                               std::vector<Point>* x2);


    TransformData computeTransformParamsFromTracksAtTime(double refTime,
                                                         double time,
                                                         int jitterPeriod,
                                                         bool jitterAdd,
                                                         bool robustModel,
                                                         const std::vector<TrackMarkerPtr>& allMarkers);

    CornerPinData computeCornerPinParamsFromTracksAtTime(double refTime,
                                                         double time,
                                                         int jitterPeriod,
                                                         bool jitterAdd,
                                                         bool robustModel,
                                                         const std::vector<TrackMarkerPtr>& allMarkers);


    void resetTransformParamsAnimation();

    void computeTransformParamsFromTracks();

    void computeTransformParamsFromTracksEnd(double refTime,
                                             double maxFittingError,
                                             const QList<TransformData>& results);

    void computeCornerParamsFromTracks();

    void computeCornerParamsFromTracksEnd(double refTime,
                                          double maxFittingError,
                                          const QList<CornerPinData>& results);

    void setSolverParamsEnabled(bool enabled);

    void endSolve();

    bool isTransformAutoGenerationEnabled() const;

    void setTransformOutOfDate(bool outdated);

public Q_SLOTS:

    void onCornerPinSolverWatcherFinished();
    void onTransformSolverWatcherFinished();

    void onCornerPinSolverWatcherProgress(int progress);
    void onTransformSolverWatcherProgress(int progress);
};

NATRON_NAMESPACE_EXIT

#endif // TRACKERCONTEXTPRIVATE_H

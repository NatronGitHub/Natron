/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2020 The Natron developers
 * (C) 2013-2018 INRIA and Alexandre Gauthier-Foichat
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

#ifndef NATRON_ENGINE_TRACKERNODEPRIVATE_H
#define NATRON_ENGINE_TRACKERNODEPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <map>
#include <set>

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
#include <boost/scoped_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#endif

#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <ofxNatron.h>

#include "Engine/KnobTypes.h"
#include "Engine/RectI.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/TrackerNode.h"
#include "Engine/Texture.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TrackerParamsProvider.h"
#include "Engine/KnobItemsTableUndoCommand.h"
#include "Engine/TransformOverlayInteract.h"
#include "Engine/CornerPinOverlayInteract.h"

#include "Global/GLIncludes.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#ifdef DEBUG
// Enable usage of markers that track using TrackerPM internally
#define NATRON_TRACKER_ENABLE_TRACKER_PM
#endif



#define POINT_SIZE 5
#define CROSS_SIZE 6
#define POINT_TOLERANCE 6
#define ADDTRACK_SIZE 5
#define HANDLE_SIZE 6

//Controls how many center keyframes should be displayed before and after the time displayed
#define MAX_CENTER_POINTS_DISPLAYED 50

#define SELECTED_MARKER_KEYFRAME_WIDTH_SCREEN_PX 75
#define MAX_TRACK_KEYS_TO_DISPLAY 10

#define CORRELATION_ERROR_MAX_DISPLAY 0.2

#define kTrackerUIParamAddTrack "addTrack"
#define kTrackerUIParamAddTrackLabel "Add Track"
#define kTrackerUIParamAddTrackHint "When enabled you can add new tracks " \
    "by clicking on the Viewer. " \
    "Holding the Control + Alt keys is the " \
    "same as pressing this button."

#define kTrackerUIParamTrackBW "trackBW"
#define kTrackerUIParamTrackBWLabel "Track Backward"
#define kTrackerUIParamTrackBWHint "Track selected tracks backward until left bound of the timeline"

#define kTrackerUIParamTrackPrevious "trackPrevious"
#define kTrackerUIParamTrackPreviousLabel "Track Previous"
#define kTrackerUIParamTrackPreviousHint "Track selected tracks on the previous frame"

#define kTrackerUIParamStopTracking "stopTracking"
#define kTrackerUIParamStopTrackingLabel "Stop Tracking"
#define kTrackerUIParamStopTrackingHint "Stop any ongoing tracking operation"

#define kTrackerUIParamTrackFW "trackFW"
#define kTrackerUIParamTrackFWLabel "Track Forward"
#define kTrackerUIParamTrackFWHint "Track selected tracks forward until right bound of the timeline"

#define kTrackerUIParamTrackNext "trackNext"
#define kTrackerUIParamTrackNextLabel "Track Next"
#define kTrackerUIParamTrackNextHint "Track selected tracks on the next frame"

#define kTrackerUIParamTrackRange "trackRange"
#define kTrackerUIParamTrackRangeLabel "Track Range"
#define kTrackerUIParamTrackRangeHint "Track selected tracks over the range and with the step input by a dialog"

#define kTrackerUIParamTrackAllKeyframes "trackAllKeys"
#define kTrackerUIParamTrackAllKeyframesLabel "Track All Keyframes"
#define kTrackerUIParamTrackAllKeyframesHint "Track selected tracks across all pattern keyframes"

#define kTrackerUIParamTrackCurrentKeyframe "trackCurrentKey"
#define kTrackerUIParamTrackCurrentKeyframeLabel "Track Current Keyframe"
#define kTrackerUIParamTrackCurrentKeyframeHint "Track selected tracks over only the pattern keyframes related to the current track"

#define kTrackerUIParamClearAllAnimation "clearAnimation"
#define kTrackerUIParamClearAllAnimationLabel "Clear Animation"
#define kTrackerUIParamClearAllAnimationHint "Reset animation on the selected tracks"

#define kTrackerUIParamClearAnimationBw "clearAnimationBw"
#define kTrackerUIParamClearAnimationBwLabel "Clear Animation Backward"
#define kTrackerUIParamClearAnimationBwHint "Reset animation on the selected tracks backward from the current frame"

#define kTrackerUIParamClearAnimationFw "clearAnimationFw"
#define kTrackerUIParamClearAnimationFwLabel "Clear Animation Forward"
#define kTrackerUIParamClearAnimationFwHint "Reset animation on the selected tracks forward from the current frame"

#define kTrackerUIParamRefreshViewer "refreshViewer"
#define kTrackerUIParamRefreshViewerLabel "Refresh Viewer"
#define kTrackerUIParamRefreshViewerHint "Update viewer during tracking for each frame instead of just the tracks"

#define kTrackerUIParamCenterViewer "centerViewer"
#define kTrackerUIParamCenterViewerLabel "Center Viewer"
#define kTrackerUIParamCenterViewerHint "Center the viewer on selected tracks during tracking"

#define kTrackerUIParamCreateKeyOnMove "createKeyOnMove"
#define kTrackerUIParamCreateKeyOnMoveLabel "Create Keyframe On Move"
#define kTrackerUIParamCreateKeyOnMoveHint "When enabled, adjusting a track on the viewer will create a new keyframe"

#define kTrackerUIParamShowError "showError"
#define kTrackerUIParamShowErrorLabel "Show Error"
#define kTrackerUIParamShowErrorHint "When enabled, the error of the track for each frame will be displayed on " \
    "the viewer, with good tracks close to green and bad tracks close to red"

#define kTrackerUIParamSetPatternKeyFrame "setPatternKey"
#define kTrackerUIParamSetPatternKeyFrameLabel "Set Keyframe On Pattern"
#define kTrackerUIParamSetPatternKeyFrameHint "Set a keyframe for the pattern for the selected tracks"

#define kTrackerUIParamRemovePatternKeyFrame "removePatternKey"
#define kTrackerUIParamRemovePatternKeyFrameLabel "Remove Keyframe On Pattern"
#define kTrackerUIParamRemovePatternKeyFrameHint "Remove a keyframe for the pattern for the selected tracks at the current timeline's time, if any"

#define kTrackerUIParamResetOffset "resetTrackOffset"
#define kTrackerUIParamResetOffsetLabel "Reset Offset"
#define kTrackerUIParamResetOffsetHint "Resets the offset for the selected tracks"

#define kTrackerUIParamResetTrack "resetTrack"
#define kTrackerUIParamResetTrackLabel "Reset Track"
#define kTrackerUIParamResetTrackHint "Reset pattern, search window and track animation for the selected tracks"

#define kTrackerUIParamMagWindowSize "magWindowSize"
#define kTrackerUIParamMagWindowSizeLabel "Mag. Window Size"
#define kTrackerUIParamMagWindowSizeHint "The size of the selected track magnification winow in pixels"




// Right click menu
#define kTrackerUIParamRightClickMenu kNatronOfxParamRightClickMenu

#define kTrackerUIParamRightClickMenuActionSelectAllTracks "selectAllMenuAction"
#define kTrackerUIParamRightClickMenuActionSelectAllTracksLabel "Select All"

#define kTrackerUIParamRightClickMenuActionRemoveTracks "removeTracks"
#define kTrackerUIParamRightClickMenuActionRemoveTracksLabel "Remove Track(s)"

#define kTrackerUIParamRightClickMenuActionNudgeLeft "nudgeLeftAction"
#define kTrackerUIParamRightClickMenuActionNudgeLeftLabel "Nudge Left"

#define kTrackerUIParamRightClickMenuActionNudgeRight "nudgeRightAction"
#define kTrackerUIParamRightClickMenuActionNudgeRightLabel "Nudge Right"

#define kTrackerUIParamRightClickMenuActionNudgeBottom "nudgeBottomAction"
#define kTrackerUIParamRightClickMenuActionNudgeBottomLabel "Nudge Bottom"

#define kTrackerUIParamRightClickMenuActionNudgeTop "nudgeTopAction"
#define kTrackerUIParamRightClickMenuActionNudgeTopLabel "Nudge Top"

// Track range dialog
#define kTrackerUIParamTrackRangeDialog "trackRangeDialog"
#define kTrackerUIParamTrackRangeDialogLabel "Track Range"

#define kTrackerUIParamTrackRangeDialogFirstFrame "firstFrame"
#define kTrackerUIParamTrackRangeDialogFirstFrameLabel "First Frame"
#define kTrackerUIParamTrackRangeDialogFirstFrameHint "The starting frame from which to track"

#define kTrackerUIParamTrackRangeDialogLastFrame "lastFrame"
#define kTrackerUIParamTrackRangeDialogLastFrameLabel "Last Frame"
#define kTrackerUIParamTrackRangeDialogLastFrameHint "The last frame to track"

#define kTrackerUIParamTrackRangeDialogStep "frameStep"
#define kTrackerUIParamTrackRangeDialogStepLabel "Step"
#define kTrackerUIParamTrackRangeDialogStepHint "The number of frames to increment on the timeline after each successful tracks. If Last Frame is lesser than First Frame, you may specify a negative frame step."

#define kTrackerUIParamTrackRangeDialogOkButton "okButton"
#define kTrackerUIParamTrackRangeDialogOkButtonLabel "Ok"
#define kTrackerUIParamTrackRangeDialogOkButtonHint "Start Tracking"

#define kTrackerUIParamTrackRangeDialogCancelButton "cancelButton"
#define kTrackerUIParamTrackRangeDialogCancelButtonLabel "Cancel"
#define kTrackerUIParamTrackRangeDialogCancelButtonHint "Close this window and do not start tracking"


// Tracking knobs

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

#define kTrackerAddTrackParam "addTrackButton"
#define kTrackerAddTrackParamLabel "New Track"
#define kTrackerAddTrackParamHint "Creates a new track marker"

#define kTrackerRemoveTracksParam "removeTracksButton"
#define kTrackerRemoveTracksParamLabel "Remove"
#define kTrackerRemoveTracksParamHint "Remove the selected track(s)"

#define kTrackerAverageTracksParam "averageTracksButton"
#define kTrackerAverageTracksParamLabel "Average"
#define kTrackerAverageTracksParamHint "Average the selected tracks into a single track"

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

#define kCornerPinParamEnableHint "Enables the point on the left."

#define kCornerPinParamInteractive "interactive"

#define kCornerPinParamOverlayPoints "overlayPoints"
#define kCornerPinParamOverlayPointsLabel "Overlay Points"
#define kCornerPinParamOverlayPointsHint "Whether to display the \"from\" or the \"to\" points in the overlay"

#define kCornerPinParamMatrix "transform"

enum TrackerTransformNodeEnum
{
    eTrackerTransformNodeTransform,
    eTrackerTransformNodeCornerPin
};


enum TrackerMotionTypeEnum
{
    eTrackerMotionTypeNone,
    eTrackerMotionTypeStabilize,
    eTrackerMotionTypeMatchMove,
    eTrackerMotionTypeRemoveJitter,
    eTrackerMotionTypeAddJitter
};


enum TrackerMouseStateEnum
{
    eMouseStateIdle = 0,
    eMouseStateDraggingCenter,
    eMouseStateDraggingOffset,

    eMouseStateDraggingInnerTopLeft,
    eMouseStateDraggingInnerTopRight,
    eMouseStateDraggingInnerBtmLeft,
    eMouseStateDraggingInnerBtmRight,
    eMouseStateDraggingInnerTopMid,
    eMouseStateDraggingInnerMidRight,
    eMouseStateDraggingInnerBtmMid,
    eMouseStateDraggingInnerMidLeft,

    eMouseStateDraggingOuterTopLeft,
    eMouseStateDraggingOuterTopRight,
    eMouseStateDraggingOuterBtmLeft,
    eMouseStateDraggingOuterBtmRight,
    eMouseStateDraggingOuterTopMid,
    eMouseStateDraggingOuterMidRight,
    eMouseStateDraggingOuterBtmMid,
    eMouseStateDraggingOuterMidLeft,

    eMouseStateDraggingSelectedMarkerResizeAnchor,
    eMouseStateScalingSelectedMarker,
    eMouseStateDraggingSelectedMarker,
};

enum TrackerDrawStateEnum
{
    eDrawStateInactive = 0,
    eDrawStateHoveringCenter,

    eDrawStateHoveringInnerTopLeft,
    eDrawStateHoveringInnerTopRight,
    eDrawStateHoveringInnerBtmLeft,
    eDrawStateHoveringInnerBtmRight,
    eDrawStateHoveringInnerTopMid,
    eDrawStateHoveringInnerMidRight,
    eDrawStateHoveringInnerBtmMid,
    eDrawStateHoveringInnerMidLeft,

    eDrawStateHoveringOuterTopLeft,
    eDrawStateHoveringOuterTopRight,
    eDrawStateHoveringOuterBtmLeft,
    eDrawStateHoveringOuterBtmRight,
    eDrawStateHoveringOuterTopMid,
    eDrawStateHoveringOuterMidRight,
    eDrawStateHoveringOuterBtmMid,
    eDrawStateHoveringOuterMidLeft,

    eDrawStateShowScalingHint,
};

typedef QFutureWatcher<std::pair<ImagePtr, RectD> > TrackWatcher;
typedef boost::shared_ptr<TrackWatcher> TrackWatcherPtr;

struct TrackRequestKey
{
    TimeValue time;
    boost::weak_ptr<TrackMarker> track;
    RectD roi;
};

struct TrackRequestKey_compareLess
{
    bool operator()(const TrackRequestKey& lhs,
                    const TrackRequestKey& rhs) const
    {
        if (lhs.time < rhs.time) {
            return true;
        } else if (lhs.time > rhs.time) {
            return false;
        } else {
            TrackMarkerPtr lptr = lhs.track.lock();
            TrackMarkerPtr rptr = rhs.track.lock();
            if ( lptr.get() < rptr.get() ) {
                return true;
            } else if ( lptr.get() > rptr.get() ) {
                return false;
            } else {
                double la = lhs.roi.area();
                double ra = rhs.roi.area();
                if (la < ra) {
                    return true;
                } else if (la > ra) {
                    return false;
                } else {
                    return false;
                }
            }
        }
    }
};

typedef std::map<TrackRequestKey, TrackWatcherPtr, TrackRequestKey_compareLess> TrackKeyframeRequests;

typedef boost::shared_ptr<QFutureWatcher<CornerPinData> > CornerPinSolverWatcher;
typedef boost::shared_ptr<QFutureWatcher<TransformData> > TransformSolverWatcher;

struct SolveRequest
{
    CornerPinSolverWatcher cpWatcher;
    TransformSolverWatcher tWatcher;
    TimeValue refTime;
    std::set<TimeValue> keyframes;
    int jitterPeriod;
    bool jitterAdd;
    bool robustModel;
    double maxFittingError;
    std::vector<TrackMarkerPtr> allMarkers;
};

class TrackerKnobItemsTable;
class TrackerNode;
class TrackerNodeInteract;

class TrackerNodePrivate
    : public TrackerParamsProvider
    , public boost::enable_shared_from_this<TrackerNodePrivate>
{
    Q_DECLARE_TR_FUNCTIONS(TrackerNodePrivate)

public:
    
    TrackerNode* publicInterface; // can not be a smart ptr
    boost::shared_ptr<TrackerNodeInteract> ui;


#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    KnobBoolWPtr usePatternMatching;
    KnobChoiceWPtr patternMatchingScore;
#endif

    KnobPageWPtr trackingPageKnob;
    KnobBoolWPtr enableTrackRed, enableTrackGreen, enableTrackBlue;
    KnobDoubleWPtr maxError;
    KnobIntWPtr maxIterations;
    KnobBoolWPtr bruteForcePreTrack, useNormalizedIntensities;
    KnobDoubleWPtr preBlurSigma;
    KnobSeparatorWPtr perTrackParamsSeparator;
    KnobBoolWPtr activateTrack;
    KnobBoolWPtr autoKeyEnabled;
    KnobChoiceWPtr motionModel;
    KnobKeyFrameMarkersWPtr trackKeyframesKnob;
    KnobSeparatorWPtr exportDataSep;
    KnobBoolWPtr exportLink;
    KnobButtonWPtr exportButton;
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
    KnobButtonWPtr setFromPointsToInputRodButton;
    KnobBoolWPtr cornerPinFromPointsSetOnceAutomatically;
    KnobBoolWPtr enableToPoint[4];
    KnobBoolWPtr enableTransform;
    KnobDoubleWPtr cornerPinMatrix;
    KnobChoiceWPtr cornerPinOverlayPoints;

    KnobButtonWPtr addTrackFromPanelButton;
    KnobButtonWPtr averageTracksButton;
    KnobButtonWPtr removeSelectedTracksButton;

    NodeWPtr cornerPinNode, transformNode, maskNode;

    boost::shared_ptr<TrackerKnobItemsTable> knobsTable;
    TrackerHelperPtr tracker;

    SolveRequest lastSolveRequest;

    struct MakeSharedEnabler;

    // used by boost::make_shared<>
    TrackerNodePrivate(TrackerNode* publicInterface);

public:
    static boost::shared_ptr<TrackerNodePrivate> create(TrackerNode* publicInterface);

    virtual ~TrackerNodePrivate();

    TrackMarkerPtr createMarker();

    static void getMotionModelsAndHelps(bool addPerspective,
                                        std::vector<ChoiceOption>* models,
                                        std::map<int, std::string> *icons);

    bool
    isTrackerPMEnabled() const
    {
#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM

        return usePatternMatching.lock()->getValue();
#else

        return false;
#endif
    }

    void setFromPointsToInputRod();

    RectD getInputRoD(TimeValue time, ViewIdx view) const;

    void resetTransformCenter();

    void refreshVisibilityFromTransformType();

    void refreshVisibilityFromTransformTypeInternal(TrackerTransformNodeEnum transformType);

    void trackSelectedMarkers(TimeValue start, TimeValue end, TimeValue frameStep, OverlaySupport* viewer);

    void exportTrackDataFromExportOptions();

    bool isTransformAutoGenerationEnabled() const
    {
        return autoGenerateTransform.lock()->getValue();
    }


    void solveTransformParams();

    void solveTransformParamsIfAutomatic();

    void setSolverParamsEnabled(bool enabled);

    void setTransformOutOfDate(bool outdated);

    void resetTransformParamsAnimation();

    void computeTransformParamsFromTracks();

    void computeTransformParamsFromTracksEnd(double refTime,
                                             double maxFittingError,
                                             const QList<TransformData>& results);

    void computeCornerParamsFromTracks();

    void computeCornerParamsFromTracksEnd(double refTime,
                                          double maxFittingError,
                                          const QList<CornerPinData>& results);


    void endSolve();

    //////////////////// Overriden from TrackerParamsProviderBase
    virtual bool trackStepFunctor(int trackIndex, const TrackArgsBasePtr& args, int frame) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void beginTrackSequence(const TrackArgsBasePtr& args) OVERRIDE FINAL;
    virtual void endTrackSequence(const TrackArgsBasePtr& args) OVERRIDE FINAL;
    ////////////////////

    //////////////////// Overriden from TrackerParamsProvider
    virtual NodePtr getTrackerNode() const OVERRIDE FINAL;
    virtual NodePtr getSourceImageNode() const OVERRIDE FINAL;
    virtual ImagePlaneDesc getMaskImagePlane(int *channelIndex) const OVERRIDE FINAL;
    virtual NodePtr getMaskImageNode() const OVERRIDE FINAL;
    virtual TrackerHelperPtr getTracker() const OVERRIDE FINAL;
    virtual bool getCenterOnTrack() const OVERRIDE FINAL;
    virtual bool getUpdateViewer() const OVERRIDE FINAL;
    virtual void getTrackChannels(bool* doRed, bool* doGreen, bool* doBlue) const OVERRIDE FINAL;
    virtual bool canDisableMarkersAutomatically() const OVERRIDE FINAL;
    virtual double getMaxError() const OVERRIDE FINAL;
    virtual int getMaxNIterations() const OVERRIDE FINAL;
    virtual bool isBruteForcePreTrackEnabled() const OVERRIDE FINAL;
    virtual bool isNormalizeIntensitiesEnabled() const OVERRIDE FINAL;
    virtual double getPreBlurSigma() const OVERRIDE FINAL;
    virtual RectD getNormalizationRoD(TimeValue time, ViewIdx view) const OVERRIDE FINAL;
    ////////////////////

    void averageSelectedTracks();
};


class TrackerKnobItemsTable : public KnobItemsTable
{

public:

    TrackerNodePrivate* _imp;

    TrackerKnobItemsTable(TrackerNodePrivate* imp, KnobItemsTableTypeEnum type);

    virtual ~TrackerKnobItemsTable()
    {

    }

    virtual std::string getTableIdentifier() const OVERRIDE FINAL
    {
        return kTrackerParamTracksTableName;
    }

    virtual std::string getTablePythonPrefix() const OVERRIDE FINAL
    {
        return "tracker";
    }

    virtual KnobTableItemPtr createItemFromSerialization(const SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr& data) OVERRIDE FINAL;
    
    virtual SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr createSerializationFromItem(const KnobTableItemPtr& item) OVERRIDE FINAL;

    void getAllMarkers(std::vector<TrackMarkerPtr>* markers) const;

    void getSelectedMarkers(std::list<TrackMarkerPtr>* markers) const;

    void getAllEnabledMarkers(std::list<TrackMarkerPtr>* markers) const;

    bool isMarkerSelected(const TrackMarkerPtr& marker) const;

};


class TrackerNodeInteract
    : public OverlayInteractBase
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    TrackerNodePrivate* _imp;
    KnobButtonWPtr addTrackButton;
    KnobButtonWPtr trackRangeButton;
    KnobButtonWPtr trackBwButton;
    KnobButtonWPtr trackPrevButton;
    KnobButtonWPtr stopTrackingButton; //< invisible
    KnobButtonWPtr trackNextButton;
    KnobButtonWPtr trackFwButton;
    KnobButtonWPtr trackAllKeyframesButton;
    KnobButtonWPtr trackCurrentKeyframeButton;
    KnobButtonWPtr clearAllAnimationButton;
    KnobButtonWPtr clearBwAnimationButton;
    KnobButtonWPtr clearFwAnimationButton;
    KnobButtonWPtr updateViewerButton;
    KnobButtonWPtr centerViewerButton;
    KnobButtonWPtr createKeyOnMoveButton;
    KnobButtonWPtr setKeyFrameButton;
    KnobButtonWPtr removeKeyFrameButton;
    KnobButtonWPtr resetOffsetButton;
    KnobButtonWPtr resetTrackButton;
    KnobButtonWPtr showCorrelationButton;
    KnobIntWPtr magWindowPxSizeKnob;
    KnobIntWPtr defaultSearchWinSize, defaultPatternWinSize;
    KnobChoiceWPtr defaultMotionModel;

    // Track range dialog
    KnobGroupWPtr trackRangeDialogGroup;
    KnobIntWPtr trackRangeDialogFirstFrame;
    KnobIntWPtr trackRangeDialogLastFrame;
    KnobIntWPtr trackRangeDialogStep;
    KnobButtonWPtr trackRangeDialogOkButton;
    KnobButtonWPtr trackRangeDialogCancelButton;


    // Right click menu
    KnobChoiceWPtr rightClickMenuKnob;
    KnobButtonWPtr selectAllTracksMenuAction;
    KnobButtonWPtr removeTracksMenuAction;
    KnobButtonWPtr nudgeTracksOnRightMenuAction;
    KnobButtonWPtr nudgeTracksOnLeftMenuAction;
    KnobButtonWPtr nudgeTracksOnTopMenuAction;
    KnobButtonWPtr nudgeTracksOnBottomMenuAction;
    bool clickToAddTrackEnabled;
    QPointF lastMousePos;
    QRectF selectionRectangle;
    int controlDown;
    int shiftDown;
    int altDown;
    TrackerMouseStateEnum eventState;
    TrackerDrawStateEnum hoverState;
    TrackMarkerPtr interactMarker, hoverMarker;

    typedef std::map<TimeValue, GLTexturePtr> KeyFrameTexIDs;
    typedef std::map<boost::weak_ptr<TrackMarker>, KeyFrameTexIDs> TrackKeysMap;
    TrackKeysMap trackTextures;
    TrackKeyframeRequests trackRequestsMap;
    GLTexturePtr selectedMarkerTexture;
    //If theres a single selection, this points to it
    boost::weak_ptr<TrackMarker> selectedMarker;
    GLuint pboID;
    TrackWatcherPtr imageGetterWatcher;
    bool showMarkerTexture;
    RenderScale selectedMarkerScale;
    boost::weak_ptr<Image> selectedMarkerImg;
    bool isTracking;
    int nSelectedMarkerTextureRefreshRequests;


    TrackerNodeInteract(TrackerNodePrivate* p);

    virtual ~TrackerNodeInteract();
    
    void onAddTrackClicked(bool clicked);

    void onTrackRangeClicked();

    void onTrackAllKeyframesClicked();

    void onTrackCurrentKeyframeClicked();

    void onTrackBwClicked();

    void onTrackPrevClicked();

    void onStopButtonClicked();

    void onTrackNextClicked();

    void onTrackFwClicked();

    void onClearAllAnimationClicked();

    void onClearBwAnimationClicked();

    void onClearFwAnimationClicked();

    void onSetKeyframeButtonClicked();

    void onRemoveKeyframeButtonClicked();

    void onResetOffsetButtonClicked();

    void onResetTrackButtonClicked();


    /**
     *@brief Moves of the given pixel the selected tracks.
     * This takes into account the zoom factor.
     **/
    bool nudgeSelectedTracks(int x, int y);


    void transformPattern(TimeValue time, TrackerMouseStateEnum state, const Point& delta);

    void refreshSelectedMarkerTextureLater();

    void refreshSelectedMarkerTextureNow();

    void convertImageTosRGBOpenGLTexture(const ImagePtr& image, const boost::shared_ptr<Texture>& tex, const RectI& renderWindow);

    void makeMarkerKeyTexture(TimeValue time, const TrackMarkerPtr& track);

    void drawSelectedMarkerTexture(const std::pair<double, double>& pixelScale,
                                   TimeValue currentTime,
                                   const Point& selectedCenter,
                                   const Point& offset,
                                   const Point& selectedPtnTopLeft,
                                   const Point& selectedPtnTopRight,
                                   const Point& selectedPtnBtmRight,
                                   const Point& selectedPtnBtmLeft,
                                   const Point& selectedSearchWndBtmLeft,
                                   const Point& selectedSearchWndTopRight);

    void drawSelectedMarkerKeyframes(const std::pair<double, double>& pixelScale, TimeValue currentTime);

    ///Filter allkeys so only that only the MAX_TRACK_KEYS_TO_DISPLAY surrounding are displayed
    static KeyFrameTexIDs getKeysToRenderForMarker(TimeValue currentTime, const KeyFrameTexIDs& allKeys);

    void computeTextureCanonicalRect(const Texture& tex, int xOffset, int texWidthPx, RectD* rect) const;
    void computeSelectedMarkerCanonicalRect(RectD* rect) const;
    bool isNearbySelectedMarkerTextureResizeAnchor(const QPointF& pos) const;
    bool isInsideSelectedMarkerTexture(const QPointF& pos) const;

    ///Returns the keyframe time if the mouse is inside a keyframe texture
    int isInsideKeyFrameTexture(TimeValue currentTime, const QPointF& pos, const QPointF& viewportPos) const;

    static Point toMagWindowPoint(const Point& ptnPoint,
                                  const RectD& canonicalSearchWindow,
                                  const RectD& textureRectCanonical);
    static QPointF computeMidPointExtent(const QPointF& prev,
                                         const QPointF& next,
                                         const QPointF& point,
                                         const QPointF& handleSize);

    bool isNearbyPoint(const KnobDoublePtr& knob,
                       double xWidget,
                       double yWidget,
                       double toleranceWidget,
                       TimeValue time);

    bool isNearbyPoint(const QPointF& p,
                       double xWidget,
                       double yWidget,
                       double toleranceWidget);

    static void drawEllipse(double x,
                            double y,
                            double radiusX,
                            double radiusY,
                            int l,
                            double r,
                            double g,
                            double b,
                            double a);
    static void findLineIntersection(const Point& p,
                                     const Point& l1,
                                     const Point& l2,
                                     Point* inter);

    virtual void onViewportSelectionCleared() OVERRIDE FINAL;

    virtual void onViewportSelectionUpdated(const RectD& rectangle, bool onRelease) OVERRIDE FINAL;


    virtual void drawOverlay(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayPenDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenMotion(TimeValue time, const RenderScale & renderScale, ViewIdx view,
                                    const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayPenDoubleClicked(TimeValue time,
                                           const RenderScale & renderScale,
                                           ViewIdx view,
                                           const QPointF & viewportPos,
                                           const QPointF & pos) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual bool onOverlayKeyDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyUp(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayKeyRepeat(TimeValue time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers) OVERRIDE FINAL;
    virtual bool onOverlayFocusGained(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
    virtual bool onOverlayFocusLost(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;


Q_SIGNALS:

    void requestRefreshSelectedMarkerTexture();

public Q_SLOTS:

    void onRequestRefreshSelectedMarkerReceived();

    void onTrackingStarted(int step);

    void onTrackingEnded();


    void onModelSelectionChanged(const std::list<KnobTableItemPtr>& addedToSelection, const std::list<KnobTableItemPtr>& removedFromSelection, TableChangeReasonEnum reason);
    void onKeyframeSetOnTrack(TimeValue key);
    void onKeyframeRemovedOnTrack(TimeValue key);
    void onKeyframeMovedOnTrack(TimeValue from, TimeValue to);

    void rebuildMarkerTextures();
    void onTrackImageRenderingFinished();
    void onKeyFrameImageRenderingFinished();
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_TRACKERNODEPRIVATE_H

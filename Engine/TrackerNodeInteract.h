/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
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

#ifndef TRACKERNODEINTERACT_H
#define TRACKERNODEINTERACT_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QFutureWatcher>
#include <QtConcurrentRun>
#include <QtCore/QPointF>
#include <QtCore/QRectF>

#include <ofxNatron.h>

#include "Engine/EngineFwd.h"
#include "Engine/KnobTypes.h"
#include "Engine/RectI.h"
#include "Engine/Texture.h"
#include "Engine/TrackerUndoCommand.h"


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

NATRON_NAMESPACE_ENTER


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

typedef QFutureWatcher<std::pair<boost::shared_ptr<Image>, RectI> > TrackWatcher;
typedef boost::shared_ptr<TrackWatcher> TrackWatcherPtr;

struct TrackRequestKey
{
    int time;
    TrackMarkerWPtr track;
    RectI roi;
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

class TrackerNode;
class TrackerNodeInteract;
struct TrackerNodePrivate
{
    TrackerNode* publicInterface;
    boost::shared_ptr<TrackerNodeInteract> ui;

    TrackerNodePrivate(TrackerNode* publicInterface);

    ~TrackerNodePrivate();
};

class TrackerNodeInteract
    : public QObject
{
GCC_DIAG_SUGGEST_OVERRIDE_OFF
    Q_OBJECT
GCC_DIAG_SUGGEST_OVERRIDE_ON

public:

    TrackerNodePrivate* _p;
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

    typedef std::map<int, GLTexturePtr> KeyFrameTexIDs;
    typedef std::map<TrackMarkerWPtr, KeyFrameTexIDs> TrackKeysMap;
    TrackKeysMap trackTextures;
    TrackKeyframeRequests trackRequestsMap;
    GLTexturePtr selectedMarkerTexture;
    int selectedMarkerTextureTime;
    RectI selectedMarkerTextureRoI;
    //If theres a single selection, this points to it
    TrackMarkerWPtr selectedMarker;
    GLuint pboID;
    TrackWatcherPtr imageGetterWatcher;
    bool showMarkerTexture;
    RenderScale selectedMarkerScale;
    ImageWPtr selectedMarkerImg;
    bool isTracking;


    TrackerNodeInteract(TrackerNodePrivate* p);

    ~TrackerNodeInteract();

    boost::shared_ptr<TrackerContext> getContext() const;

    void onAddTrackClicked(bool clicked);

    void onTrackRangeClicked();

    void onTrackAllKeyframesClicked();

    void onTrackCurrentKeyframeClicked();

    void onTrackBwClicked();

    void onTrackPrevClicked();

    void onStopButtonClicked();

    void onTrackNextClicked();

    void onTrackFwClicked();

    void onUpdateViewerClicked(bool clicked);

    void onClearAllAnimationClicked();

    void onClearBwAnimationClicked();

    void onClearFwAnimationClicked();

    void onCenterViewerButtonClicked(bool clicked);

    void onSetKeyframeButtonClicked();

    void onRemoveKeyframeButtonClicked();

    void onResetOffsetButtonClicked();

    void onResetTrackButtonClicked();


    /**
     *@brief Moves of the given pixel the selected tracks.
     * This takes into account the zoom factor.
     **/
    bool nudgeSelectedTracks(int x, int y);


    void transformPattern(double time, TrackerMouseStateEnum state, const Point& delta);

    void refreshSelectedMarkerTexture();

    void convertImageTosRGBOpenGLTexture(const boost::shared_ptr<Image>& image, const boost::shared_ptr<Texture>& tex, const RectI& renderWindow);

    void makeMarkerKeyTexture(int time, const TrackMarkerPtr& track);

    void drawSelectedMarkerTexture(const std::pair<double, double>& pixelScale,
                                   int currentTime,
                                   const Point& selectedCenter,
                                   const Point& offset,
                                   const Point& selectedPtnTopLeft,
                                   const Point& selectedPtnTopRight,
                                   const Point& selectedPtnBtmRight,
                                   const Point& selectedPtnBtmLeft,
                                   const Point& selectedSearchWndBtmLeft,
                                   const Point& selectedSearchWndTopRight);

    void drawSelectedMarkerKeyframes(const std::pair<double, double>& pixelScale, int currentTime);

    ///Filter allkeys so only that only the MAX_TRACK_KEYS_TO_DISPLAY surrounding are displayed
    static KeyFrameTexIDs getKeysToRenderForMarker(double currentTime, const KeyFrameTexIDs& allKeys);

    void computeTextureCanonicalRect(const Texture& tex, int xOffset, int texWidthPx, RectD* rect) const;
    void computeSelectedMarkerCanonicalRect(RectD* rect) const;
    bool isNearbySelectedMarkerTextureResizeAnchor(const QPointF& pos) const;
    bool isInsideSelectedMarkerTexture(const QPointF& pos) const;

    ///Returns the keyframe time if the mouse is inside a keyframe texture
    int isInsideKeyFrameTexture(double currentTime, const QPointF& pos, const QPointF& viewportPos) const;

    static Point toMagWindowPoint(const Point& ptnPoint,
                                  const RectD& canonicalSearchWindow,
                                  const RectD& textureRectCanonical);
    static QPointF computeMidPointExtent(const QPointF& prev,
                                         const QPointF& next,
                                         const QPointF& point,
                                         const QPointF& handleSize);

    bool isNearbyPoint(const boost::shared_ptr<KnobDouble>& knob,
                       double xWidget,
                       double yWidget,
                       double toleranceWidget,
                       double time);

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

public Q_SLOTS:

    void onTrackingStarted(int step);

    void onTrackingEnded();


    void onContextSelectionChanged(int reason);
    void onKeyframeSetOnTrack(const TrackMarkerPtr &marker, int key);
    void onKeyframeRemovedOnTrack(const TrackMarkerPtr &marker, int key);
    void onAllKeyframesRemovedOnTrack(const TrackMarkerPtr& marker);

    void rebuildMarkerTextures();
    void onTrackImageRenderingFinished();
    void onKeyFrameImageRenderingFinished();
};

NATRON_NAMESPACE_EXIT

#endif // TRACKERNODEINTERACT_H

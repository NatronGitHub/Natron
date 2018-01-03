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

#ifndef NATRON_ENGINE_VIEWERNODEPRIVATE_H
#define NATRON_ENGINE_VIEWERNODEPRIVATE_H

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "Global/Macros.h"

#include <QtCore/QThread>
#include <QCoreApplication>
#include <QTimer>

#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Node.h"
#include "Engine/GroupInput.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/OutputSchedulerThread.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/Project.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"
#include "Engine/ViewIdx.h"

#include "Serialization/NodeSerialization.h"

#include "Engine/EngineFwd.h"

NATRON_NAMESPACE_ENTER

#define kViewerNodeParamLayers "layer"
#define kViewerNodeParamLayersLabel "Layer"
#define kViewerNodeParamLayersHint "The layer that the Viewer node will fetch upstream in the tree. " \
"The channels of the layer will be mapped to the RGBA channels of the viewer according to the Display Channels parameter. " \
"its number of channels. (e.g: UV would be mapped to RG)"

#define kViewerNodeParamAlphaChannel "alphaChannel"
#define kViewerNodeParamAlphaChannelLabel "Alpha Channel"
#define kViewerNodeParamAlphaChannelHint "Select here a channel of any layer used when to display the " \
"alpha channel when Display Channels is set to Alpha or Matte"

#define kViewerNodeParamDisplayChannels "displayChannels"
#define kViewerNodeParamDisplayChannelsB "displayChannelsB"
#define kViewerNodeParamDisplayChannelsLabel "Display Channels"
#define kViewerNodeParamDisplayChannelsHint "The channels to display on the viewer from the selected layer"

#define kViewerNodeParamFullFrame "fullFrame"
#define kViewerNodeParamFullFrameLabel "Full Frame"
#define kViewerNodeParamFullFrameHint "When checked, the viewer will render the image in its entirety (at full resolution) not just the visible portion. The \"Downscale Level\" parameter is meaningless when this is checked. This may be useful when panning/zooming during playback"

#define kViewerNodeParamProxyLevel "downscaleLevel"
#define kViewerNodeParamProxyLevelLabel "Downscale Level"
#define kViewerNodeParamProxyLevelHint "The image will be downscaled by the indicated factor in both dimensions to speed up rendering. " \
"When set to Auto, the image is downscaled according to the current zoom factor in the viewport." \

#define kViewerNodeParamRefreshViewport "refreshViewport"
#define kViewerNodeParamRefreshViewportLabel "Refresh Viewport"
#define kViewerNodeParamRefreshViewportHint "Forces a new render of the current frame. Press %2 to activate in-depth render statistics useful " \
"for debugging the composition"

#define kViewerNodeParamPauseRender "pauseUpdates"
#define kViewerNodeParamPauseRenderB "pauseUpdatesB"
#define kViewerNodeParamPauseRenderLabel "Pause Updates"
#define kViewerNodeParamPauseRenderHint "When activated the viewer will not update after any change that would modify the image " \
"displayed in the viewport. Use %2 to pause both input A and B"

#define kViewerNodeParamAInput "aInput"
#define kViewerNodeParamAInputLabel "A"
#define kViewerNodeParamAInputHint "What node to display in the viewer input A"

#define kViewerNodeParamBInput "bInput"
#define kViewerNodeParamBInputLabel "B"
#define kViewerNodeParamBInputHint "What node to display in the viewer input B"

#define kViewerNodeParamOperation "operation"
#define kViewerNodeParamOperationLabel "Operation"
#define kViewerNodeParamOperationHint "Operation applied between viewer inputs A and B. a and b are the alpha components of each input. d is the wipe dissolve factor, controlled by the arc handle"

#define kViewerNodeParamOperationWipeUnder "Wipe Under"
#define kViewerNodeParamOperationWipeUnderHint "A(1 - d) + Bd"

#define kViewerNodeParamOperationWipeOver "Wipe Over"
#define kViewerNodeParamOperationWipeOverHint "A + B(1 - a)d"

#define kViewerNodeParamOperationWipeMinus "Wipe Minus"
#define kViewerNodeParamOperationWipeMinusHint "A - B"

#define kViewerNodeParamOperationWipeOnionSkin "Wipe Onion skin"
#define kViewerNodeParamOperationWipeOnionSkinHint "A + B"

#define kViewerNodeParamOperationStackUnder "Stack Under"
#define kViewerNodeParamOperationStackUnderHint "B"

#define kViewerNodeParamOperationStackOver "Stack Over"
#define kViewerNodeParamOperationStackOverHint "A + B(1 - a)"

#define kViewerNodeParamOperationStackMinus "Stack Minus"
#define kViewerNodeParamOperationStackMinusHint "A - B"

#define kViewerNodeParamOperationStackOnionSkin "Stack Onion skin"
#define kViewerNodeParamOperationStackOnionSkinHint "A + B"

#define kViewerNodeParamEnableGain "enableGain"
#define kViewerNodeParamEnableGainLabel "Enable Gain"
#define kViewerNodeParamEnableGainHint "Switch between \"neutral\" 1.0 gain f-stop and the previous setting"

#define kViewerNodeParamGain "gain"
#define kViewerNodeParamGainLabel "Gain"
#define kViewerNodeParamGainHint "Gain is shown as f-stops. The image is multipled by pow(2,value) before display"

#define kViewerNodeParamEnableAutoContrast "autoContrast"
#define kViewerNodeParamEnableAutoContrastLabel "Auto Contrast"
#define kViewerNodeParamEnableAutoContrastHint "Automatically adjusts the gain and the offset applied " \
"to the colors of the visible image portion on the viewer"

#define kViewerNodeParamEnableGamma "enableGamma"
#define kViewerNodeParamEnableGammaLabel "Enable Gamma"
#define kViewerNodeParamEnableGammaHint "Gamma correction: Switch between gamma=1.0 and user setting"

#define kViewerNodeParamGamma "gamma"
#define kViewerNodeParamGammaLabel "Gamma"
#define kViewerNodeParamGammaHint "Viewer gamma correction level (applied after gain and before colorspace correction)"

#define kViewerNodeParamColorspace "deviceColorspace"
#define kViewerNodeParamColorspaceLabel "Device Colorspace"
#define kViewerNodeParamColorspaceHint "The operation applied to the image before it is displayed " \
"on screen. The image is converted to this colorspace before being displayed on the monitor"

#define kViewerNodeParamView "activeView"
#define kViewerNodeParamViewLabel "Active View"
#define kViewerNodeParamViewHint "The view displayed on the viewer"

#define kViewerNodeParamZoom "zoom"
#define kViewerNodeParamZoomLabel "Zoom"
#define kViewerNodeParamZoomHint "The zoom applied to the image on the viewer"

#define kViewerNodeParamSyncViewports "syncViewports"
#define kViewerNodeParamSyncViewportsLabel "Sync Viewports"
#define kViewerNodeParamSyncViewportsHint "When enabled, all viewers will be synchronized to the same portion of the image in the viewport"

#define kViewerNodeParamFitViewport "fitViewport"
#define kViewerNodeParamFitViewportLabel "Fit Viewport"
#define kViewerNodeParamFitViewportHint "Scales the image so it doesn't exceed the size of the viewport and centers it"

#define kViewerNodeParamCheckerBoard "enableCheckerBoard"
#define kViewerNodeParamCheckerBoardLabel "Enable Checkerboard"
#define kViewerNodeParamCheckerBoardHint "If checked, the viewer draws a checkerboard under input A instead of black (disabled under the wipe area and in stack modes)"

#define kViewerNodeParamEnableColorPicker "enableInfoBar"
#define kViewerNodeParamEnableColorPickerLabel "Show Info Bar"
#define kViewerNodeParamEnableColorPickerHint "Show/Hide information bar in the bottom of the viewer. If unchecked it also deactivates any active color picker"


// Right-click menu actions

// The right click menu
#define kViewerNodeParamRightClickMenu kNatronOfxParamRightClickMenu

#define kViewerNodeParamRightClickMenuToggleWipe "enableWipeAction"
#define kViewerNodeParamRightClickMenuToggleWipeLabel "Enable Wipe"

#define kViewerNodeParamRightClickMenuCenterWipe "centerWipeAction"
#define kViewerNodeParamRightClickMenuCenterWipeLabel "Center Wipe"

#define kViewerNodeParamRightClickMenuPreviousLayer "previousLayerAction"
#define kViewerNodeParamRightClickMenuPreviousLayerLabel "Previous Layer"

#define kViewerNodeParamRightClickMenuNextLayer "nextLayerAction"
#define kViewerNodeParamRightClickMenuNextLayerLabel "Next Layer"

#define kViewerNodeParamRightClickMenuPreviousView "previousViewAction"
#define kViewerNodeParamRightClickMenuPreviousViewLabel "Previous View"

#define kViewerNodeParamRightClickMenuNextView "nextViewAction"
#define kViewerNodeParamRightClickMenuNextViewLabel "Next View"

#define kViewerNodeParamRightClickMenuSwitchAB "switchABAction"
#define kViewerNodeParamRightClickMenuSwitchABLabel "Switch Input A and B"

#define kViewerNodeParamRightClickMenuShowHideOverlays "showHideOverlays"
#define kViewerNodeParamRightClickMenuShowHideOverlaysLabel "Show/Hide Overlays"

#define kViewerNodeParamRightClickMenuShowHideSubMenu "showHideSubMenu"
#define kViewerNodeParamRightClickMenuShowHideSubMenuLabel "Show/Hide"

#define kViewerNodeParamRightClickMenuHideAll "hideAll"
#define kViewerNodeParamRightClickMenuHideAllLabel "Hide All"

#define kViewerNodeParamRightClickMenuHideAllTop "hideAllTop"
#define kViewerNodeParamRightClickMenuHideAllTopLabel "Hide All Toolbars + Header"

#define kViewerNodeParamRightClickMenuHideAllBottom "hideAllBottom"
#define kViewerNodeParamRightClickMenuHideAllBottomLabel "Hide Player + Timeline"

#define kViewerNodeParamRightClickMenuShowHidePlayer "showHidePlayer"
#define kViewerNodeParamRightClickMenuShowHidePlayerLabel "Show/Hide Player"

#define kViewerNodeParamRightClickMenuShowHideTimeline "showHideTimeline"
#define kViewerNodeParamRightClickMenuShowHideTimelineLabel "Show/Hide Timeline"

#define kViewerNodeParamRightClickMenuShowHideLeftToolbar "showHideLeftToolbar"
#define kViewerNodeParamRightClickMenuShowHideLeftToolbarLabel "Show/Hide Left Toolbar"

#define kViewerNodeParamRightClickMenuShowHideTopToolbar "showHideTopToolbar"
#define kViewerNodeParamRightClickMenuShowHideTopToolbarLabel "Show/Hide Top Toolbar"

#define kViewerNodeParamRightClickMenuShowHideTabHeader "showHideTabHeader"
#define kViewerNodeParamRightClickMenuShowHideTabHeaderLabel "Show/Hide Tab Header"

// Viewer Actions
#define kViewerNodeParamActionLuminance "displayLuminance"
#define kViewerNodeParamActionLuminanceA "displayLuminanceA"
#define kViewerNodeParamActionLuminanceALabel "Display Luminance For A Input Only"
#define kViewerNodeParamActionLuminanceLabel "Display Luminance"

#define kViewerNodeParamActionRed "displayRed"
#define kViewerNodeParamActionRedLabel "Display Red"
#define kViewerNodeParamActionRedA "displayRedA"
#define kViewerNodeParamActionRedALabel "Display Red For A Input Only"

#define kViewerNodeParamActionGreen "displayGreen"
#define kViewerNodeParamActionGreenLabel "Display Green"
#define kViewerNodeParamActionGreenA "displayGreenA"
#define kViewerNodeParamActionGreenALabel "Display Green For A Input Only"

#define kViewerNodeParamActionBlue "displayBlue"
#define kViewerNodeParamActionBlueLabel "Display Blue"
#define kViewerNodeParamActionBlueA "displayBlueA"
#define kViewerNodeParamActionBlueALabel "Display Blue For A Input Only"

#define kViewerNodeParamActionAlpha "displayAlpha"
#define kViewerNodeParamActionAlphaLabel "Display Alpha"
#define kViewerNodeParamActionAlphaA "displayAlphaA"
#define kViewerNodeParamActionAlphaALabel "Display Alpha For A Input Only"

#define kViewerNodeParamActionMatte "displayMatte"
#define kViewerNodeParamActionMatteLabel "Display Matte"
#define kViewerNodeParamActionMatteA "displayMatteA"
#define kViewerNodeParamActionMatteALabel "Display Matte For A Input Only"

#define kViewerNodeParamActionZoomIn "zoomInAction"
#define kViewerNodeParamActionZoomInLabel "Zoom In"

#define kViewerNodeParamActionZoomOut "zoomOut"
#define kViewerNodeParamActionZoomOutLabel "Zoom Out"

#define kViewerNodeParamActionScaleOne "scaleOne"
#define kViewerNodeParamActionScaleOneLabel "Zoom 100%"

#define kViewerNodeParamActionProxy2 "proxy2"
#define kViewerNodeParamActionProxy2Label "Proxy Level 2"

#define kViewerNodeParamActionProxy4 "proxy4"
#define kViewerNodeParamActionProxy4Label "Proxy Level 4"

#define kViewerNodeParamActionProxy8 "proxy8"
#define kViewerNodeParamActionProxy8Label "Proxy Level 8"

#define kViewerNodeParamActionProxy16 "proxy16"
#define kViewerNodeParamActionProxy16Label "Proxy Level 16"

#define kViewerNodeParamActionProxy32 "proxy32"
#define kViewerNodeParamActionProxy32Label "Proxy Level 32"

#define kViewerNodeParamActionLeftView "leftView"
#define kViewerNodeParamActionLeftViewLabel "Left View"

#define kViewerNodeParamActionRightView "rightView"
#define kViewerNodeParamActionRightViewLabel "Right View"

#define kViewerNodeParamActionPauseAB "pauseAB"
#define kViewerNodeParamActionPauseABLabel "Pause input A and B"

#define kViewerNodeParamActionRefreshWithStats "enableStats"
#define kViewerNodeParamActionRefreshWithStatsLabel "Enable Render Stats"

#define kViewerNodeParamActionCreateNewRoI "createNewRoI"
#define kViewerNodeParamActionCreateNewRoILabel "Create New Region Of Interest"

#define kViewerNodeParamActionAbortRender "aboortRender"
#define kViewerNodeParamActionAbortRenderLabel "Abort Rendering"
#define kViewerNodeParamActionAbortRenderHint "Abort any ongoing playback or render"

// Viewer overlay
#define kViewerNodeParamWipeCenter "wipeCenter"
#define kViewerNodeParamWipeAmount "wipeAmount"
#define kViewerNodeParamWipeAngle "wipeAngle"

// Player buttons
#define kViewerNodeParamInPoint "inPoint"
#define kViewerNodeParamInPointLabel "In Point"
#define kViewerNodeParamInPointHint "The playback in point"

#define kViewerNodeParamOutPoint "outPoint"
#define kViewerNodeParamOutPointLabel "Out Point"
#define kViewerNodeParamOutPointHint "The playback out point"

#define kViewerNodeParamEnableFps "enableFps"
#define kViewerNodeParamEnableFpsLabel "Enable FPS"
#define kViewerNodeParamEnableFpsHint "When unchecked, the playback frame rate is automatically set from " \
"the Viewer A input. When checked, the user setting is used"

#define kViewerNodeParamFps "desiredFps"
#define kViewerNodeParamFpsLabel "Fps"
#define kViewerNodeParamFpsHint "Viewer playback framerate, in frames per second"

#define kViewerNodeParamEnableTurboMode "enableTurboMode"
#define kViewerNodeParamEnableTurboModeLabel "Turbo Mode"
#define kViewerNodeParamEnableTurboModeHint "When checked, only the viewer is redrawn during playback, " \
"for maximum efficiency"

#define kViewerNodeParamPlaybackMode "playbackMode"
#define kViewerNodeParamPlaybackModeLabel "Playback Mode"
#define kViewerNodeParamPlaybackModeHint "Behavior to adopt when the playback hit the end of the range: loop,bounce or stop"

#define kViewerNodeParamSyncTimelines "syncTimelines"
#define kViewerNodeParamSyncTimelinesLabel "Sync Timelines"
#define kViewerNodeParamSyncTimelinesHint "When activated, the timeline frame-range is synchronized with the Dope Sheet and the Curve Editor"



#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923132169163975144   /* pi/2           */
#endif
#ifndef M_PI_4
#define M_PI_4      0.785398163397448309615660845819875721  /* pi/4           */
#endif

#define VIEWER_INITIAL_N_INPUTS 10


enum ViewerNodeInteractMouseStateEnum
{
    eViewerNodeInteractMouseStateIdle,
    eViewerNodeInteractMouseStateBuildingUserRoI,
    eViewerNodeInteractMouseStateDraggingRoiLeftEdge,
    eViewerNodeInteractMouseStateDraggingRoiRightEdge,
    eViewerNodeInteractMouseStateDraggingRoiTopEdge,
    eViewerNodeInteractMouseStateDraggingRoiBottomEdge,
    eViewerNodeInteractMouseStateDraggingRoiTopLeft,
    eViewerNodeInteractMouseStateDraggingRoiTopRight,
    eViewerNodeInteractMouseStateDraggingRoiBottomRight,
    eViewerNodeInteractMouseStateDraggingRoiBottomLeft,
    eViewerNodeInteractMouseStateDraggingRoiCross,
    eViewerNodeInteractMouseStateDraggingWipeCenter,
    eViewerNodeInteractMouseStateDraggingWipeMixHandle,
    eViewerNodeInteractMouseStateRotatingWipeHandle
};

enum HoverStateEnum
{
    eHoverStateNothing = 0,
    eHoverStateWipeMix,
    eHoverStateWipeRotateHandle
};

class ViewerNodeOverlay;
struct ViewerNodePrivate
{
    ViewerNode* _publicInterface;

    // Pointer to ViewerGL (interface)
    OpenGLViewerI* uiContext;

    NodeWPtr internalViewerProcessNode[2];

    boost::weak_ptr<KnobChoice> layersKnob;
    boost::weak_ptr<KnobChoice> alphaChannelKnob;
    boost::weak_ptr<KnobChoice> displayChannelsKnob[2];
    boost::weak_ptr<KnobChoice> zoomChoiceKnob;
    boost::weak_ptr<KnobButton> syncViewersButtonKnob;
    boost::weak_ptr<KnobButton> centerViewerButtonKnob;
    boost::weak_ptr<KnobButton> clipToFormatButtonKnob;
    boost::weak_ptr<KnobButton> fullFrameButtonKnob;
    boost::weak_ptr<KnobButton> toggleUserRoIButtonKnob;
    boost::weak_ptr<KnobDouble> userRoIBtmLeftKnob;
    boost::weak_ptr<KnobDouble> userRoISizeKnob;
    boost::weak_ptr<KnobChoice> downscaleChoiceKnob;
    boost::weak_ptr<KnobButton> refreshButtonKnob;
    boost::weak_ptr<KnobButton> pauseButtonKnob[2];
    boost::weak_ptr<KnobChoice> aInputNodeChoiceKnob;
    boost::weak_ptr<KnobChoice> blendingModeChoiceKnob;
    boost::weak_ptr<KnobChoice> bInputNodeChoiceKnob;

    boost::weak_ptr<KnobButton> enableGainButtonKnob;
    boost::weak_ptr<KnobDouble> gainSliderKnob;
    boost::weak_ptr<KnobButton> enableAutoContrastButtonKnob;
    boost::weak_ptr<KnobButton> enableGammaButtonKnob;
    boost::weak_ptr<KnobDouble> gammaSliderKnob;
    boost::weak_ptr<KnobButton> enableCheckerboardButtonKnob;
    boost::weak_ptr<KnobChoice> colorspaceKnob;
    boost::weak_ptr<KnobChoice> activeViewKnob;
    boost::weak_ptr<KnobButton> enableInfoBarButtonKnob;

    // Player
    boost::weak_ptr<KnobButton> setInPointButtonKnob;
    boost::weak_ptr<KnobButton> setOutPointButtonKnob;
    boost::weak_ptr<KnobInt> inPointKnob;
    boost::weak_ptr<KnobInt> outPointKnob;
    boost::weak_ptr<KnobInt> curFrameKnob;
    boost::weak_ptr<KnobBool> enableFpsKnob;
    boost::weak_ptr<KnobDouble> fpsKnob;
    boost::weak_ptr<KnobButton> enableTurboModeButtonKnob;
    boost::weak_ptr<KnobChoice> playbackModeKnob;
    boost::weak_ptr<KnobButton> syncTimelinesButtonKnob;
    boost::weak_ptr<KnobButton> firstFrameButtonKnob;
    boost::weak_ptr<KnobButton> playBackwardButtonKnob;
    boost::weak_ptr<KnobButton> playForwardButtonKnob;
    boost::weak_ptr<KnobButton> lastFrameButtonKnob;
    boost::weak_ptr<KnobButton> prevFrameButtonKnob;
    boost::weak_ptr<KnobButton> nextFrameButtonKnob;
    boost::weak_ptr<KnobButton> prevKeyFrameButtonKnob;
    boost::weak_ptr<KnobButton> nextKeyFrameButtonKnob;
    boost::weak_ptr<KnobButton> prevIncrButtonKnob;
    boost::weak_ptr<KnobInt> incrFrameKnob;
    boost::weak_ptr<KnobButton> nextIncrButtonKnob;


    // Overlays
    boost::weak_ptr<KnobDouble> wipeCenter;
    boost::weak_ptr<KnobDouble> wipeAmount;
    boost::weak_ptr<KnobDouble> wipeAngle;

    // Right click menu
    boost::weak_ptr<KnobChoice> rightClickMenu;
    boost::weak_ptr<KnobButton> rightClickToggleWipe;
    boost::weak_ptr<KnobButton> rightClickCenterWipe;
    boost::weak_ptr<KnobButton> rightClickPreviousLayer;
    boost::weak_ptr<KnobButton> rightClickNextLayer;
    boost::weak_ptr<KnobButton> rightClickPreviousView;
    boost::weak_ptr<KnobButton> rightClickNextView;
    boost::weak_ptr<KnobButton> rightClickSwitchAB;
    boost::weak_ptr<KnobButton> rightClickShowHideOverlays;
    boost::weak_ptr<KnobChoice> rightClickShowHideSubMenu;
    boost::weak_ptr<KnobButton> rightClickHideAll;
    boost::weak_ptr<KnobButton> rightClickHideAllTop;
    boost::weak_ptr<KnobButton> rightClickHideAllBottom;
    boost::weak_ptr<KnobButton> rightClickShowHidePlayer;
    boost::weak_ptr<KnobButton> rightClickShowHideTimeline;
    boost::weak_ptr<KnobButton> rightClickShowHideLeftToolbar;
    boost::weak_ptr<KnobButton> rightClickShowHideTopToolbar;
    boost::weak_ptr<KnobButton> rightClickShowHideTabHeader;

    // Viewer actions
    boost::weak_ptr<KnobButton> displayLuminanceAction[2];
    boost::weak_ptr<KnobButton> displayRedAction[2];
    boost::weak_ptr<KnobButton> displayGreenAction[2];
    boost::weak_ptr<KnobButton> displayBlueAction[2];
    boost::weak_ptr<KnobButton> displayAlphaAction[2];
    boost::weak_ptr<KnobButton> displayMatteAction[2];
    boost::weak_ptr<KnobButton> zoomInAction;
    boost::weak_ptr<KnobButton> zoomOutAction;
    boost::weak_ptr<KnobButton> zoomScaleOneAction;
    boost::weak_ptr<KnobButton> proxyLevelAction[5];
    boost::weak_ptr<KnobButton> leftViewAction;
    boost::weak_ptr<KnobButton> rightViewAction;
    boost::weak_ptr<KnobButton> pauseABAction;
    boost::weak_ptr<KnobButton> enableStatsAction;
    boost::weak_ptr<KnobButton> createUserRoIAction;
    boost::weak_ptr<KnobButton> abortRenderingAction;

    double lastFstopValue;
    double lastGammaValue;
    int lastWipeIndex;

    QTimer mustSetUpPlaybackButtonsTimer;


    QMutex forceRenderMutex;
    bool forceRender;


    QMutex partialUpdatesMutex;

    // If this list is not empty, this is the list of canonical rectangles we should update on the viewer, completly
    //disregarding the RoI. This is protected by partialUpdatesMutex
    std::list<RectD> partialUpdateRects;

    // Overlay
    boost::shared_ptr<ViewerNodeOverlay> ui;

    // If set, the viewport center will be updated to this point upon the next update of the texture, this is protected by
    // partialUpdatesMutex
    Point viewportCenter;
    bool viewportCenterSet;

    // True if during tracking, protected by partialUpdatesMutex
    bool isDoingPartialUpdates;

    ViewerNodePrivate(ViewerNode* publicInterface)
    : _publicInterface(publicInterface)
    , uiContext(0)
    , lastFstopValue(0)
    , lastGammaValue(1)
    , lastWipeIndex(0)
    , mustSetUpPlaybackButtonsTimer()
    , forceRenderMutex()
    , forceRender(false)
    , partialUpdateRects()
    , ui()
    , viewportCenter()
    , viewportCenterSet(false)
    , isDoingPartialUpdates(false)
    {

    }

    NodePtr getViewerProcessNode(int index) const
    {
        return internalViewerProcessNode[index].lock();
    }

    void refreshInputChoices(bool resetChoiceIfNotFound);

    void scaleZoomFactor(double scale) {
        double factor = uiContext->getZoomFactor();
        factor *= scale;
        uiContext->zoomViewport(factor);
    }

    void getAllViewerNodes(bool inGroupOnly, std::list<ViewerNodePtr>& viewerNodes) const;

    void abortAllViewersRendering();

    void startPlayback(RenderDirectionEnum direction);

    void timelineGoTo(TimeValue time);
    
    void refreshInputChoiceMenu(int internalIndex, int groupInputIndex);
    
    void toggleDownscaleLevel(int index);
    
    NodePtr getInputRecursive(int inputIndex) const;
    
    NodePtr getMainInputRecursiveInternal(const NodePtr& inputParam) const;

    void setAlphaChannelFromLayerIfRGBA();

    void swapViewerProcessInputs();

    void setDisplayChannels(int index, bool setBoth);
    
};

class ViewerNodeOverlay : public OverlayInteractBase
{
public:

    ViewerNodeOverlay(ViewerNodePrivate* imp);

    virtual ~ViewerNodeOverlay()
    {

    }
    
private:


    void drawWipeControl();

    void drawUserRoI();

    void drawArcOfCircle(const QPointF & center, double radiusX, double radiusY, double startAngle, double endAngle);

    void showRightClickMenu();

    static bool isNearbyWipeCenter(const QPointF& wipeCenter, const QPointF & pos, double zoomScreenPixelWidth, double zoomScreenPixelHeight ) ;

    static bool isNearbyWipeRotateBar(const QPointF& wipeCenter, double wipeAngle, const QPointF & pos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);

    static bool isNearbyWipeMixHandle(const QPointF& wipeCenter, double wipeAngle, double mixAmount, const QPointF & pos, double zoomScreenPixelWidth, double zoomScreenPixelHeight);

    static bool isNearbyUserRoITopEdge(const RectD & roi,
                                       const QPointF & zoomPos,
                                       double zoomScreenPixelWidth,
                                       double zoomScreenPixelHeight);

    static bool isNearbyUserRoIRightEdge(const RectD & roi,
                                         const QPointF & zoomPos,
                                         double zoomScreenPixelWidth,
                                         double zoomScreenPixelHeight);

    static bool isNearbyUserRoILeftEdge(const RectD & roi,
                                        const QPointF & zoomPos,
                                        double zoomScreenPixelWidth,
                                        double zoomScreenPixelHeight);


    static bool isNearbyUserRoIBottomEdge(const RectD & roi,
                                          const QPointF & zoomPos,
                                          double zoomScreenPixelWidth,
                                          double zoomScreenPixelHeight);

    static bool isNearbyUserRoI(double x,
                                double y,
                                const QPointF & zoomPos,
                                double zoomScreenPixelWidth,
                                double zoomScreenPixelHeight);

    virtual bool onOverlayPenDown(TimeValue time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, TimeValue timestamp, PenType pen) OVERRIDE FINAL WARN_UNUSED_RETURN;
    virtual void drawOverlay(TimeValue time, const RenderScale & renderScale, ViewIdx view) OVERRIDE FINAL;
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

public:
    
    ViewerNodePrivate* _imp;
    RectD draggedUserRoI;
    bool buildUserRoIOnNextPress;
    ViewerNodeInteractMouseStateEnum uiState;
    HoverStateEnum hoverState;
    QPointF lastMousePos;
};

NATRON_NAMESPACE_EXIT

#endif // NATRON_ENGINE_VIEWERNODEPRIVATE_H

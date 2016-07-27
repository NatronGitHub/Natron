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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerNode.h"

#include <ofxNatron.h>

#include "Engine/AppInstance.h"
#include "Engine/AppManager.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/KnobTypes.h"
#include "Engine/Image.h"
#include "Engine/Lut.h"
#include "Engine/MemoryFile.h"
#include "Engine/Node.h"
#include "Engine/OpenGLViewerI.h"
#include "Engine/RenderStats.h"
#include "Engine/RotoPaint.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewIdx.h"


/*
 Below are knobs definitions that are not used by the internal ViewerInstance and just used by the Gui
 */
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

#define kViewerNodeParamRightClickMenuToggleWipe "toggleWipeAction"
#define kViewerNodeParamRightClickMenuToggleWipeLabel "Toggle Wipe"

#define kViewerNodeParamRightClickMenuCenterWipe "centerWipeAction"
#define kViewerNodeParamRightClickMenuCenterWipeLabel "Center Wipe"

#define kViewerNodeParamRightClickMenuPreviousLayer "previousLayerAction"
#define kViewerNodeParamRightClickMenuPreviousLayerLabel "Previous Layer"

#define kViewerNodeParamRightClickMenuNextLayer "nextLayerAction"
#define kViewerNodeParamRightClickMenuNextLayerLabel "Next Layer"

#define kViewerNodeParamRightClickMenuSwitchAB "switchABAction"
#define kViewerNodeParamRightClickMenuSwitchABLabel "Switch Input A and B"

#define kViewerNodeParamRightClickMenuShowHideOverlays "showHideOverlays"
#define kViewerNodeParamRightClickMenuShowHideOverlaysLabel "Show/Hide Overlays"

#define kViewerNodeParamRightClickMenuShowHideSubMenu "showHideSubMenu"

#define kViewerNodeParamRightClickMenuHideAll "hideAll"
#define kViewerNodeParamRightClickMenuHideAllLabel "Hide All"

#define kViewerNodeParamRightClickMenuShowHidePlayer "showHidePlayer"
#define kViewerNodeParamRightClickMenuShowHidePlayerLabel "Show/Hide Player"

#define kViewerNodeParamRightClickMenuShowHideTimeline "showHideTimeline"
#define kViewerNodeParamRightClickMenuShowHideTimelineLabel "Show/Hide Timeline"

#define kViewerNodeParamRightClickMenuShowHideInfoBar "showHideInfoBar"
#define kViewerNodeParamRightClickMenuShowHideInfoBarLabel "Show/Hide Info-bar"

#define kViewerNodeParamRightClickMenuShowHideLeftToolbar "showHideLeftToolbar"
#define kViewerNodeParamRightClickMenuShowHideLeftToolbarLabel "Show/Hide Left Toolbar"

#define kViewerNodeParamRightClickMenuShowHideTopToolbar "showHideTopToolbar"
#define kViewerNodeParamRightClickMenuShowHideTopToolbarLabel "Show/Hide Top Toolbar"


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

#define VIEWER_UI_SECTIONS_SPACING_PX 5

NATRON_NAMESPACE_ENTER;

struct ViewerNodePrivate
{
    ViewerNode* _publicInterface;

    // Pointer to ViewerGL (interface)
    OpenGLViewerI* uiContext;

    NodeWPtr internalViewerProcessNode;

    boost::weak_ptr<KnobChoice> layersKnob;
    boost::weak_ptr<KnobChoice> alphaChannelKnob;
    boost::weak_ptr<KnobChoice> displayChannelsKnob;
    boost::weak_ptr<KnobChoice> zoomChoiceKnob;
    boost::weak_ptr<KnobButton> syncViewersButtonKnob;
    boost::weak_ptr<KnobButton> centerViewerButtonKnob;
    boost::weak_ptr<KnobButton> clipToProjectButtonKnob;
    boost::weak_ptr<KnobButton> fullFrameButtonKnob;
    boost::weak_ptr<KnobButton> toggleUserRoIButtonKnob;
    boost::weak_ptr<KnobButton> toggleProxyModeButtonKnob;
    boost::weak_ptr<KnobChoice> proxyChoiceKnob;
    boost::weak_ptr<KnobButton> refreshButtonKnob;
    boost::weak_ptr<KnobButton> pauseButtonKnob;
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

    // Right click menu
    boost::weak_ptr<KnobChoice> rightClickMenu;
    boost::weak_ptr<KnobButton> rightClickToggleWipe;
    boost::weak_ptr<KnobButton> rightClickCenterWipe;
    boost::weak_ptr<KnobButton> rightClickPreviousLayer;
    boost::weak_ptr<KnobButton> rightClickNextLayer;
    boost::weak_ptr<KnobButton> rightClickSwitchAB;
    boost::weak_ptr<KnobButton> rightClickShowHideOverlays;
    boost::weak_ptr<KnobChoice> rightClickShowHideSubMenu;
    boost::weak_ptr<KnobButton> rightClickHideAll;
    boost::weak_ptr<KnobButton> rightClickShowHidePlayer;
    boost::weak_ptr<KnobButton> rightClickShowHideTimeline;
    boost::weak_ptr<KnobButton> rightClickShowHideLeftToolbar;
    boost::weak_ptr<KnobButton> rightClickShowHideTopToolbar;
    boost::weak_ptr<KnobButton> rightClickShowHideInfoBar;

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

    double lastFstopValue;
    double lastGammaValue;
    
    ViewerNodePrivate(ViewerNode* publicInterface)
    : _publicInterface(publicInterface)
    , uiContext(0)
    , lastFstopValue(0)
    , lastGammaValue(1)
    {

    }

    NodePtr getInternalViewerNode() const
    {
        return internalViewerProcessNode.lock();
    }
};

EffectInstancePtr
ViewerNode::create(const NodePtr& node)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return EffectInstancePtr( new ViewerNode(node) );
}

ViewerNode::ViewerNode(const NodePtr& node)
    : NodeGroup(node)
    , _imp( new ViewerNodePrivate(this) )
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    setSupportsRenderScaleMaybe(EffectInstance::eSupportsYes);

    QObject::connect( this, SIGNAL(disconnectTextureRequest(int,bool)), this, SLOT(executeDisconnectTextureRequestOnMainThread(int,bool)) );
    QObject::connect( this, SIGNAL(s_callRedrawOnMainThread()), this, SLOT(redrawViewer()) );
}

ViewerNode::~ViewerNode()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );


    // If _imp->updateViewerRunning is true, that means that the next updateViewer call was
    // not yet processed. Since we're in the main thread and it is processed in the main thread,
    // there is no way to wait for it (locking the mutex would cause a deadlock).
    // We don't care, after all.
    //{
    //    QMutexLocker locker(&_imp->updateViewerMutex);
    //    assert(!_imp->updateViewerRunning);
    //}
    if (_imp->uiContext) {
        _imp->uiContext->removeGUI();
    }
}

void
ViewerNode::getPluginGrouping(std::list<std::string>* grouping) const
{
    grouping->push_back(PLUGIN_GROUP_IMAGE);
}

void
ViewerNode::getPluginShortcuts(std::list<PluginActionShortcut>* shortcuts) const
{
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamClipToProject, kViewerNodeParamClipToProjectLabel, Key_C, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamFullFrame, kViewerNodeParamFullFrameLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamEnableUserRoI, kViewerNodeParamEnableUserRoILabel, Key_W, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamEnableProxyMode, kViewerNodeParamEnableProxyModeLabel, Key_P, eKeyboardModifierControl) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamPauseRender, kViewerNodeParamPauseRenderLabel, Key_P) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamEnableGain, kViewerNodeParamEnableGainLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamEnableAutoContrast, kViewerNodeParamEnableAutoContrastLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamEnableGamma, kViewerNodeParamEnableGammaLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRefreshViewport, kViewerNodeParamRefreshViewportLabel, Key_U) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamFitViewport, kViewerNodeParamFitViewportLabel, Key_F) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamSyncViewports, kViewerNodeParamSyncViewportsLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamCheckerBoard, kViewerNodeParamCheckerBoardLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamEnableColorPicker, kViewerNodeParamEnableColorPickerLabel) );

    // Right-click actions
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuToggleWipe, kViewerNodeParamRightClickMenuToggleWipeLabel, Key_W) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuCenterWipe, kViewerNodeParamRightClickMenuCenterWipeLabel, Key_F, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuPreviousLayer, kViewerNodeParamRightClickMenuPreviousLayerLabel, Key_Page_Up) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuNextLayer, kViewerNodeParamRightClickMenuNextLayerLabel, Key_Page_Down) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuSwitchAB, kViewerNodeParamRightClickMenuSwitchABLabel, Key_Return) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideOverlays, kViewerNodeParamRightClickMenuShowHideOverlaysLabel, Key_O) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuHideAll, kViewerNodeParamRightClickMenuHideAllLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHidePlayer, kViewerNodeParamRightClickMenuShowHidePlayerLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideTimeline, kViewerNodeParamRightClickMenuShowHideTimelineLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideInfoBar, kViewerNodeParamRightClickMenuShowHideInfoBarLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideLeftToolbar, kViewerNodeParamRightClickMenuShowHideLeftToolbarLabel) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideTopToolbar, kViewerNodeParamRightClickMenuShowHideTopToolbarLabel) );

    // Viewer actions
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionLuminance, kViewerNodeParamActionLuminanceLabel, Key_Y) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionRed, kViewerNodeParamActionRedLabel, Key_R) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionRedA, kViewerNodeParamActionRedALabel, Key_R, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionGreen, kViewerNodeParamActionGreenLabel, Key_G) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionGreenA, kViewerNodeParamActionGreenALabel, Key_G, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionBlue, kViewerNodeParamActionBlueLabel, Key_B) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionBlueA, kViewerNodeParamActionBlueALabel, Key_B, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionAlpha, kViewerNodeParamActionAlphaLabel, Key_A) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionAlphaA, kViewerNodeParamActionAlphaALabel, Key_A, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionMatte, kViewerNodeParamActionMatteLabel, Key_M) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionMatteA, kViewerNodeParamActionMatteALabel, Key_M, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionZoomIn, kViewerNodeParamActionZoomInLabel, Key_plus) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionZoomOut, kViewerNodeParamActionZoomOutLabel, Key_minus) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionScaleOne, kViewerNodeParamActionScaleOneLabel, Key_1, eKeyboardModifierControl) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionProxy2, kViewerNodeParamActionProxy2Label, Key_1, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionProxy4, kViewerNodeParamActionProxy4Label, Key_2, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionProxy8, kViewerNodeParamActionProxy8Label, Key_3, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionProxy16, kViewerNodeParamActionProxy16Label, Key_4, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionProxy32, kViewerNodeParamActionProxy32Label, Key_5, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionLeftView, kViewerNodeParamActionLeftViewLabel, Key_Left, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionRightView, kViewerNodeParamActionRightViewLabel, Key_Right, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionCreateNewRoI, kViewerNodeParamActionCreateNewRoILabel, Key_W, eKeyboardModifierAlt) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionPauseAB, kViewerNodeParamActionPauseABLabel, Key_P, eKeyboardModifierShift) );
    shortcuts->push_back( PluginActionShortcut(kViewerNodeParamActionRefreshWithStats, kViewerNodeParamActionRefreshWithStatsLabel, Key_U, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierControl)) );
}

/**
 * @brief Creates a duplicate of the knob identified by knobName which is a knob in the internalNode onto the effect and add it to the given page.
 **/
template <typename KNOBTYPE>
boost::shared_ptr<KNOBTYPE>
createDuplicateKnob( const std::string& knobName,
                    const NodePtr& internalNode,
                    const EffectInstancePtr& effect,
                    const KnobPagePtr& page = KnobPagePtr(),
                    const KnobGroupPtr& group = KnobGroupPtr() )
{
    KnobIPtr internalNodeKnob = internalNode->getKnobByName(knobName);
    KnobIPtr duplicateKnob = internalNodeKnob->createDuplicateOnHolder(effect, page, group, -1, true, internalNodeKnob->getName(), internalNodeKnob->getLabel(), internalNodeKnob->getHintToolTip(), false, false);
    return boost::dynamic_pointer_cast<KNOBTYPE>(duplicateKnob);
}

void
ViewerNode::initializeKnobs()
{

    ViewerNodePtr thisShared = shared_from_this();
    NodePtr internalViewerNode;
    {
        QString fixedNamePrefix = QString::fromUtf8( getNode()->getScriptName_mt_safe().c_str() );
        fixedNamePrefix.append( QLatin1Char('_') );
        QString nodeName = fixedNamePrefix + QLatin1String("internalViewer");
        CreateNodeArgs args(PLUGINID_NATRON_VIEWER_INTERNAL, thisShared);
        args.setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args.setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, nodeName.toStdString());
        internalViewerNode = getApp()->createNode(args);
        _imp->internalViewerProcessNode = internalViewerNode;
        assert(internalViewerNode);
    }

    KnobPagePtr page = AppManager::createKnob<KnobPage>( thisShared, tr("Controls") );

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamLayers, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->layersKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamAlphaChannel, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->alphaChannelKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamDisplayChannels, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->displayChannelsKnob = param;
    }


    {
        KnobChoicePtr param = AppManager::createKnob<KnobChoice>( thisShared, tr(kViewerNodeParamZoomLabel) );
        param->setName(kViewerNodeParamZoom);
        param->setHintToolTip(tr(kViewerNodeParamZoomHint));
        param->setSecretByDefault(true);
        {
            std::vector<std::string> entries;
            entries.push_back("Fit");
            entries.push_back("+");
            entries.push_back("-");
            entries.push_back(KnobChoice::getSeparatorOption());
            entries.push_back("10%");
            entries.push_back("25%");
            entries.push_back("50%");
            entries.push_back("75%");
            entries.push_back("100%");
            entries.push_back("125%");
            entries.push_back("150%");
            entries.push_back("200%");
            entries.push_back("400%");
            entries.push_back("800%");
            entries.push_back("1600%");
            entries.push_back("2400%");
            entries.push_back("3200%");
            entries.push_back("6400%");
            param->populateChoices(entries);
        }
        page->addKnob(param);
        _imp->zoomChoiceKnob = param;
    }


    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamClipToProject, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "cliptoprojectEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "cliptoprojectDisable.png", false);
        _imp->clipToProjectButtonKnob = param;
    }

    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamFullFrame, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "fullFrameOn.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "fullFrameOff.png", false);
        _imp->fullFrameButtonKnob = param;
    }

    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamEnableUserRoI, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "viewer_roiEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "viewer_roiDisabled.png", false);
        _imp->toggleUserRoIButtonKnob = param;
    }

    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamEnableProxyMode, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "renderScale_checked.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "renderScale.png", false);
        page->addKnob(param);
        _imp->toggleProxyModeButtonKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamProxyLevel, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->proxyChoiceKnob = param;
    }


    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamPauseRender, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseEnabled.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "pauseDisabled.png", false);
        _imp->pauseButtonKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamAInput, internalViewerNode, thisShared, page);
        param->setName(kViewerNodeParamAInput);
        param->setSecretByDefault(true);
        _imp->aInputNodeChoiceKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamOperation, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setIconLabel(NATRON_IMAGES_PATH "roto_merge.png");
        _imp->blendingModeChoiceKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamBInput, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->bInputNodeChoiceKnob = param;
    }

    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamEnableGain, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "expoON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "expoOFF.png", false);
        _imp->enableGainButtonKnob = param;
    }

    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kViewerNodeParamGain, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setDisplayMinimum(-6.);
        param->setDisplayMaximum(6.);
        _imp->gainSliderKnob = param;
    }

    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamEnableAutoContrast, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "AutoContrastON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "AutoContrast.png", false);
        _imp->enableAutoContrastButtonKnob = param;
    }

    {
        KnobButtonPtr param = createDuplicateKnob<KnobButton>(kViewerNodeParamEnableGamma, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "gammaON.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "gammaOFF.png", false);
        _imp->enableGammaButtonKnob = param;
    }

    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kViewerNodeParamGamma, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        param->setDisplayMinimum(0.);
        param->setDisplayMaximum(4.);
        param->setDefaultValue(1.);
        _imp->gammaSliderKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamColorspace, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->colorspaceKnob = param;
    }

    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kViewerNodeParamView, internalViewerNode, thisShared, page);
        param->setSecretByDefault(true);
        _imp->activeViewKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamRefreshViewportLabel) );
        param->setName(kViewerNodeParamRefreshViewport);
        param->setHintToolTip(tr(kViewerNodeParamRefreshViewportHint));
        param->setSecretByDefault(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "refreshActive.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "refresh.png", false);
        page->addKnob(param);
        _imp->refreshButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamSyncViewportsLabel) );
        param->setName(kViewerNodeParamSyncViewports);
        param->setHintToolTip(tr(kViewerNodeParamSyncViewportsHint));
        param->setInViewerContextCanHaveShortcut(true);
        param->setSecretByDefault(true);
        param->setCheckable(true);
        param->setEvaluateOnChange(false);
        param->setIconLabel(NATRON_IMAGES_PATH "locked.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "unlocked.png", false);
        page->addKnob(param);
        _imp->syncViewersButtonKnob = param;
    }

    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamFitViewportLabel) );
        param->setName(kViewerNodeParamFitViewport);
        param->setHintToolTip(tr(kViewerNodeParamFitViewportHint));
        param->setInViewerContextCanHaveShortcut(true);
        param->setSecretByDefault(true);
        param->setIconLabel(NATRON_IMAGES_PATH "centerViewer.png", true);
        page->addKnob(param);
        _imp->centerViewerButtonKnob = param;
    }


    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamCheckerBoardLabel) );
        param->setName(kViewerNodeParamCheckerBoard);
        param->setHintToolTip(tr(kViewerNodeParamCheckerBoardHint));
        param->setSecretByDefault(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "checkerboard_on.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "checkerboard_off.png", false);
        page->addKnob(param);
        _imp->enableCheckerboardButtonKnob = param;
    }


    {
        KnobButtonPtr param = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamEnableColorPickerLabel) );
        param->setName(kViewerNodeParamEnableColorPicker);
        param->setHintToolTip(tr(kViewerNodeParamEnableColorPickerHint));
        param->setSecretByDefault(true);
        param->setEvaluateOnChange(false);
        param->setInViewerContextCanHaveShortcut(true);
        param->setCheckable(true);
        param->setIconLabel(NATRON_IMAGES_PATH "color_picker.png", true);
        param->setIconLabel(NATRON_IMAGES_PATH "color_picker.png", false);
        page->addKnob(param);
        _imp->enableInfoBarButtonKnob = param;
    }

    addKnobToViewerUI(_imp->layersKnob.lock());
    addKnobToViewerUI(_imp->alphaChannelKnob.lock());
    addKnobToViewerUI(_imp->displayChannelsKnob.lock());
    _imp->displayChannelsKnob.lock()->setInViewerContextStretch(eStretchAfter);
    addKnobToViewerUI(_imp->aInputNodeChoiceKnob.lock());
    addKnobToViewerUI(_imp->blendingModeChoiceKnob.lock());
    addKnobToViewerUI(_imp->bInputNodeChoiceKnob.lock());
    _imp->bInputNodeChoiceKnob.lock()->setInViewerContextStretch(eStretchAfter);

    addKnobToViewerUI(_imp->clipToProjectButtonKnob.lock());
    addKnobToViewerUI(_imp->toggleProxyModeButtonKnob.lock());
    addKnobToViewerUI(_imp->proxyChoiceKnob.lock());
    addKnobToViewerUI(_imp->fullFrameButtonKnob.lock());
    addKnobToViewerUI(_imp->toggleUserRoIButtonKnob.lock());
    _imp->toggleUserRoIButtonKnob.lock()->setInViewerContextAddSeparator(true);
    addKnobToViewerUI(_imp->refreshButtonKnob.lock());
    addKnobToViewerUI(_imp->pauseButtonKnob.lock());
    _imp->pauseButtonKnob.lock()->setInViewerContextAddSeparator(true);

    addKnobToViewerUI(_imp->centerViewerButtonKnob.lock());
    addKnobToViewerUI(_imp->syncViewersButtonKnob.lock());
    addKnobToViewerUI(_imp->zoomChoiceKnob.lock());
    _imp->zoomChoiceKnob.lock()->setInViewerContextNewLineActivated(true);


    addKnobToViewerUI(_imp->enableGainButtonKnob.lock());
    addKnobToViewerUI(_imp->gainSliderKnob.lock());
    addKnobToViewerUI(_imp->enableAutoContrastButtonKnob.lock());
    addKnobToViewerUI(_imp->enableGammaButtonKnob.lock());
    addKnobToViewerUI(_imp->gammaSliderKnob.lock());
    addKnobToViewerUI(_imp->colorspaceKnob.lock());
    addKnobToViewerUI(_imp->enableCheckerboardButtonKnob.lock());
    addKnobToViewerUI(_imp->activeViewKnob.lock());
    _imp->activeViewKnob.lock()->setInViewerContextStretch(eStretchAfter);
    addKnobToViewerUI(_imp->enableInfoBarButtonKnob.lock());

    // Right click menu
    KnobChoicePtr rightClickMenu = AppManager::createKnob<KnobChoice>( thisShared, std::string(kViewerNodeParamRightClickMenu) );
    rightClickMenu->setSecretByDefault(true);
    rightClickMenu->setEvaluateOnChange(false);
    page->addKnob(rightClickMenu);
    _imp->rightClickMenu = rightClickMenu;
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuToggleWipeLabel) );
        action->setName(kViewerNodeParamRightClickMenuToggleWipe);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickToggleWipe = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuCenterWipeLabel) );
        action->setName(kViewerNodeParamRightClickMenuCenterWipe);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickCenterWipe = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuPreviousLayerLabel) );
        action->setName(kViewerNodeParamRightClickMenuPreviousLayer);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickPreviousLayer = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuNextLayerLabel) );
        action->setName(kViewerNodeParamRightClickMenuNextLayer);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickNextLayer = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuSwitchABLabel) );
        action->setName(kViewerNodeParamRightClickMenuSwitchAB);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickSwitchAB = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuShowHideOverlaysLabel) );
        action->setName(kViewerNodeParamRightClickMenuShowHideOverlays);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightClickShowHideOverlays = action;
    }

    KnobChoicePtr showHideSubMenu = AppManager::createKnob<KnobChoice>( thisShared, std::string(kViewerNodeParamRightClickMenuShowHideSubMenu) );
    showHideSubMenu->setSecretByDefault(true);
    showHideSubMenu->setEvaluateOnChange(false);
    page->addKnob(showHideSubMenu);
    _imp->rightClickShowHideSubMenu = showHideSubMenu;

    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuHideAllLabel) );
        action->setName(kViewerNodeParamRightClickMenuHideAll);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickHideAll = action;
    }

    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuShowHidePlayerLabel) );
        action->setName(kViewerNodeParamRightClickMenuShowHidePlayer);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickShowHidePlayer = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuShowHideTimelineLabel) );
        action->setName(kViewerNodeParamRightClickMenuShowHideTimeline);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickShowHideTimeline = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuShowHideLeftToolbarLabel) );
        action->setName(kViewerNodeParamRightClickMenuShowHideLeftToolbar);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickShowHideLeftToolbar = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuShowHideTopToolbarLabel) );
        action->setName(kViewerNodeParamRightClickMenuShowHideTopToolbar);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickShowHideTopToolbar = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( thisShared, tr(kViewerNodeParamRightClickMenuShowHideInfoBarLabel) );
        action->setName(kViewerNodeParamRightClickMenuShowHideInfoBar);
        action->setSecretByDefault(true);
        action->setInViewerContextCanHaveShortcut(true);
        action->setCheckable(true);
        page->addKnob(action);
        _imp->rightClickShowHideInfoBar = action;
    }

    // Viewer actions
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionLuminanceLabel) );
        action->setName(kViewerNodeParamActionLuminance);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayLuminanceAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionLuminanceALabel) );
        action->setName(kViewerNodeParamActionLuminanceA);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayLuminanceAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionRedLabel) );
        action->setName(kViewerNodeParamActionRed);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayRedAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionRedALabel) );
        action->setName(kViewerNodeParamActionRedA);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayRedAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionGreenLabel) );
        action->setName(kViewerNodeParamActionGreen);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayGreenAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionGreenALabel) );
        action->setName(kViewerNodeParamActionGreenA);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayGreenAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionBlueLabel) );
        action->setName(kViewerNodeParamActionBlue);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayBlueAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionBlueALabel) );
        action->setName(kViewerNodeParamActionBlueA);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayBlueAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionAlphaLabel) );
        action->setName(kViewerNodeParamActionAlpha);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayAlphaAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionAlphaALabel) );
        action->setName(kViewerNodeParamActionAlphaA);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayAlphaAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionMatteLabel) );
        action->setName(kViewerNodeParamActionMatte);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayMatteAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionMatteALabel) );
        action->setName(kViewerNodeParamActionMatteA);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->displayMatteAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionZoomInLabel) );
        action->setName(kViewerNodeParamActionZoomIn);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->zoomInAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionZoomOutLabel) );
        action->setName(kViewerNodeParamActionZoomOut);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->zoomOutAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionScaleOneLabel) );
        action->setName(kViewerNodeParamActionScaleOne);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->zoomScaleOneAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionProxy2Label) );
        action->setName(kViewerNodeParamActionProxy2);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[0] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionProxy4Label) );
        action->setName(kViewerNodeParamActionProxy4);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[1] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionProxy8Label) );
        action->setName(kViewerNodeParamActionProxy8);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[2] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionProxy16Label) );
        action->setName(kViewerNodeParamActionProxy16);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[3] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionProxy32Label) );
        action->setName(kViewerNodeParamActionProxy32);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->proxyLevelAction[4] = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionLeftViewLabel) );
        action->setName(kViewerNodeParamActionLeftView);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->leftViewAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionRightViewLabel) );
        action->setName(kViewerNodeParamActionRightView);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->rightViewAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionPauseABLabel) );
        action->setName(kViewerNodeParamActionPauseAB);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->pauseABAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionCreateNewRoILabel) );
        action->setName(kViewerNodeParamActionCreateNewRoI);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->createUserRoIAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( shared_from_this(), tr(kViewerNodeParamActionRefreshWithStatsLabel) );
        action->setName(kViewerNodeParamActionRefreshWithStats);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        page->addKnob(action);
        _imp->enableStatsAction = action;
    }

} // initializeKnobs

bool
ViewerNode::knobChanged(const KnobIPtr& k, ValueChangedReasonEnum reason,
                        ViewSpec /*view*/,
                        double /*time*/,
                        bool /*originatedFromMainThread*/)
{
    if (!k) {
        return false;
    }

    bool caught = true;
    if (k == _imp->enableGainButtonKnob.lock()) {
        double value;
        bool down = _imp->enableGainButtonKnob.lock()->getValue();
        if (down) {
            value = _imp->lastFstopValue;
        } else {
            value = 0;
        }
        _imp->gainSliderKnob.lock()->setValue(value);
    } else if (k == _imp->gainSliderKnob.lock()) {
        KnobButtonPtr enableKnob = _imp->enableGainButtonKnob.lock();
        bool down = enableKnob->getValue();
        if (!down) {
            enableKnob->setValue(true);
        }
        _imp->lastFstopValue =  _imp->gainSliderKnob.lock()->getValue();
    } else if (k == _imp->enableGammaButtonKnob.lock()) {
        double value;
        bool down = _imp->enableGammaButtonKnob.lock()->getValue();
        if (down) {
            value = _imp->lastGammaValue;
        } else {
            value = 0;
        }
        _imp->gammaSliderKnob.lock()->setValue(value);

    } else if (k == _imp->gammaSliderKnob.lock()) {
        KnobButtonPtr enableKnob = _imp->enableGammaButtonKnob.lock();
        bool down = enableKnob->getValue();
        if (!down) {
            enableKnob->setValue(true);
        }
        _imp->lastGammaValue =  _imp->gammaSliderKnob.lock()->getValue();

    } else if (k == _imp->enableAutoContrastButtonKnob.lock()) {
        bool enable = _imp->enableAutoContrastButtonKnob.lock()->getValue();
        _imp->enableGammaButtonKnob.lock()->setAllDimensionsEnabled(!enable);
        _imp->gammaSliderKnob.lock()->setAllDimensionsEnabled(!enable);
        _imp->enableGainButtonKnob.lock()->setAllDimensionsEnabled(!enable);
        _imp->gainSliderKnob.lock()->setAllDimensionsEnabled(!enable);
        
    } else if (k == _imp->refreshButtonKnob.lock()) {

    } else if (k == _imp->syncViewersButtonKnob.lock()) {

    } else if (k == _imp->centerViewerButtonKnob.lock()) {

    } else if (k == _imp->enableInfoBarButtonKnob.lock()) {

    } else if (k == _imp->rightClickToggleWipe.lock()) {

    } else if (k == _imp->rightClickCenterWipe.lock()) {

    } else if (k == _imp->rightClickNextLayer.lock()) {

    } else if (k == _imp->rightClickPreviousLayer.lock()) {

    } else if (k == _imp->rightClickShowHideOverlays.lock()) {

    } else if (k == _imp->rightClickSwitchAB.lock()) {

    } else if (k == _imp->rightClickHideAll.lock()) {

    } else if (k == _imp->rightClickShowHideTopToolbar.lock()) {

    } else if (k == _imp->rightClickShowHideLeftToolbar.lock()) {

    } else if (k == _imp->rightClickShowHidePlayer.lock()) {

    } else if (k == _imp->rightClickShowHideTimeline.lock()) {

    } else if (k == _imp->rightClickShowHideInfoBar.lock()) {

    } else if (k == _imp->displayRedAction[0].lock()) {

    } else if (k == _imp->displayRedAction[1].lock()) {

    } else if (k == _imp->displayGreenAction[0].lock()) {

    } else if (k == _imp->displayGreenAction[1].lock()) {

    } else if (k == _imp->displayBlueAction[0].lock()) {

    } else if (k == _imp->displayBlueAction[1].lock()) {

    } else if (k == _imp->displayAlphaAction[0].lock()) {

    } else if (k == _imp->displayAlphaAction[1].lock()) {

    } else if (k == _imp->displayMatteAction[0].lock()) {

    } else if (k == _imp->displayMatteAction[1].lock()) {

    } else if (k == _imp->displayLuminanceAction[0].lock()) {

    } else if (k == _imp->displayLuminanceAction[1].lock()) {

    } else if (k == _imp->zoomInAction.lock()) {

    } else if (k == _imp->zoomOutAction.lock()) {

    } else if (k == _imp->zoomScaleOneAction.lock()) {

    } else if (k == _imp->proxyLevelAction[0].lock()) {

    } else if (k == _imp->proxyLevelAction[1].lock()) {

    } else if (k == _imp->proxyLevelAction[2].lock()) {

    } else if (k == _imp->proxyLevelAction[3].lock()) {

    } else if (k == _imp->proxyLevelAction[4].lock()) {

    } else if (k == _imp->leftViewAction.lock()) {

    } else if (k == _imp->rightViewAction.lock()) {

    } else if (k == _imp->pauseABAction.lock()) {

    } else if (k == _imp->createUserRoIAction.lock()) {

    } else if (k == _imp->enableStatsAction.lock()) {

    } else {
        caught = false;
    }
    return caught;

} // knobChanged


bool
ViewerNode::onOverlayPenDown(double time,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            const QPointF & /*viewportPos*/,
                            const QPointF & pos,
                            double pressure,
                            double timestamp,
                            PenType pen)
{


} // onOverlayPenDown

OpenGLViewerI*
ViewerNode::getUiContext() const
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->uiContext;
}

void
ViewerNode::setUiContext(OpenGLViewerI* viewer)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    _imp->uiContext = viewer;
}


void
ViewerNode::forceFullComputationOnNextFrame()
{
    // this is called by the GUI when the user presses the "Refresh" button.
    // It set the flag forceRender to true, meaning no cache will be used.

    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

}

void
ViewerNode::clearLastRenderedImage()
{
    NodeGroup::clearLastRenderedImage();

    if (_imp->uiContext) {
        _imp->uiContext->clearLastRenderedImage();
    }

}

void
ViewerNode::invalidateUiContext()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext = NULL;
}


void
ViewerNode::executeDisconnectTextureRequestOnMainThread(int index,bool clearRoD)
{
    assert( QThread::currentThread() == qApp->thread() );
    if (_imp->uiContext) {
        _imp->uiContext->disconnectInputTexture(index, clearRoD);
    }
}
void
ViewerNode::aboutToUpdateTextures()
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext->clearPartialUpdateTextures();
}

bool
ViewerNode::isViewerUIVisible() const
{
    assert( qApp && qApp->thread() == QThread::currentThread() );

    return _imp->uiContext ? _imp->uiContext->isViewerUIVisible() : false;
}

void
ViewerInstance::onGammaChanged(double value)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);

        if (_imp->viewerParamsGamma != value) {
            _imp->viewerParamsGamma = value;
            changed = true;
        }
    }
    if (changed) {
        QWriteLocker k(&_imp->gammaLookupMutex);
        _imp->fillGammaLut(1. / value);
    }
    assert(_imp->uiContext);
    if (changed) {
        if ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte)
             && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

double
ViewerInstance::getGamma() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsGamma;
}

void
ViewerInstance::onGainChanged(double exp)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsGain != exp) {
            _imp->viewerParamsGain = exp;
            changed = true;
        }
    }
    if (changed) {
        assert(_imp->uiContext);
        if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) )
             && !getApp()->getProject()->isLoadingProject() ) {
            renderCurrentFrame(true);
        } else {
            _imp->uiContext->redraw();
        }
    }
}

unsigned int
ViewerInstance::getViewerMipMapLevel() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerMipMapLevel;
}

void
ViewerInstance::onMipMapLevelChanged(int level)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerMipMapLevel == (unsigned int)level) {
            return;
        }
        _imp->viewerMipMapLevel = level;
    }
}

void
ViewerInstance::onAutoContrastChanged(bool autoContrast,
                                      bool refresh)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsAutoContrast != autoContrast) {
            _imp->viewerParamsAutoContrast = autoContrast;
            changed = true;
        }
    }
    if ( changed && refresh && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

bool
ViewerInstance::isAutoContrastEnabled() const
{
    // MT-safe
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsAutoContrast;
}

void
ViewerInstance::onColorSpaceChanged(ViewerColorSpaceEnum colorspace)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsLut == colorspace) {
            return;
        }
        _imp->viewerParamsLut = colorspace;
    }
    assert(_imp->uiContext);
    if ( ( (_imp->uiContext->getBitDepth() == eImageBitDepthByte) )
         && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    } else {
        _imp->uiContext->redraw();
    }
}

void
ViewerInstance::setDisplayChannels(DisplayChannelsEnum channels,
                                   bool bothInputs)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (!bothInputs) {
            if (_imp->viewerParamsChannels[0] != channels) {
                _imp->viewerParamsChannels[0] = channels;
                changed = true;
            }
        } else {
            if (_imp->viewerParamsChannels[0] != channels) {
                _imp->viewerParamsChannels[0] = channels;
                changed = true;
            }
            if (_imp->viewerParamsChannels[1] != channels) {
                _imp->viewerParamsChannels[1] = channels;
                changed = true;
            }
        }
    }
    if ( changed && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::setActiveLayer(const ImageComponents& layer,
                               bool doRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsLayer != layer) {
            _imp->viewerParamsLayer = layer;
            changed = true;
        }
    }
    if ( doRender && changed && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::setAlphaChannel(const ImageComponents& layer,
                                const std::string& channelName,
                                bool doRender)
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    bool changed = false;
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->viewerParamsAlphaLayer != layer) {
            _imp->viewerParamsAlphaLayer = layer;
            changed = true;
        }
        if (_imp->viewerParamsAlphaChannelName != channelName) {
            _imp->viewerParamsAlphaChannelName = channelName;
            changed = true;
        }
    }
    if ( changed && doRender && !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

void
ViewerInstance::disconnectViewer()
{
    // always running in the render thread
    if (_imp->uiContext) {
        Q_EMIT viewerDisconnected();
    }
}

void
ViewerInstance::disconnectTexture(int index,bool clearRod)
{
    if (_imp->uiContext) {
        Q_EMIT disconnectTextureRequest(index, clearRod);
    }
}

void
ViewerInstance::redrawViewer()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    if (!_imp->uiContext) {
        return;
    }
    _imp->uiContext->redraw();
}

void
ViewerInstance::redrawViewerNow()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    assert(_imp->uiContext);
    _imp->uiContext->redrawNow();
}

int
ViewerInstance::getLutType() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsLut;
}

double
ViewerInstance::getGain() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsGain;
}

int
ViewerInstance::getMipMapLevel() const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerMipMapLevel;
}

DisplayChannelsEnum
ViewerInstance::getChannels(int texIndex) const
{
    // MT-SAFE: called from main thread and Serialization (pooled) thread

    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->viewerParamsChannels[texIndex];
}

void
ViewerInstance::setFullFrameProcessingEnabled(bool fullFrame)
{
    {
        QMutexLocker l(&_imp->viewerParamsMutex);
        if (_imp->fullFrameProcessingEnabled == fullFrame) {
            return;
        }
        _imp->fullFrameProcessingEnabled = fullFrame;
    }
    if ( !getApp()->getProject()->isLoadingProject() ) {
        renderCurrentFrame(true);
    }
}

bool
ViewerInstance::isFullFrameProcessingEnabled() const
{
    QMutexLocker l(&_imp->viewerParamsMutex);

    return _imp->fullFrameProcessingEnabled;
}

void
ViewerInstance::addAcceptedComponents(int /*inputNb*/,
                                      std::list<ImageComponents>* comps)
{
    ///Viewer only supports RGBA for now.
    comps->push_back( ImageComponents::getRGBAComponents() );
    comps->push_back( ImageComponents::getRGBComponents() );
    comps->push_back( ImageComponents::getAlphaComponents() );
}

ViewIdx
ViewerInstance::getViewerCurrentView() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentView() : ViewIdx(0);
}

void
ViewerInstance::setActivateInputChangeRequestedFromViewer(bool fromViewer)
{
    assert( QThread::currentThread() == qApp->thread() );
    _imp->activateInputChangedFromViewer = fromViewer;
}

bool
ViewerInstance::isInputChangeRequestedFromViewer() const
{
    assert( QThread::currentThread() == qApp->thread() );

    return _imp->activateInputChangedFromViewer;
}

void
ViewerInstance::setViewerPaused(bool paused,
                                bool allInputs)
{
    QMutexLocker k(&_imp->isViewerPausedMutex);

    _imp->isViewerPaused[0] = paused;
    if (allInputs) {
        _imp->isViewerPaused[1] = paused;
    }
}

bool
ViewerInstance::isViewerPaused(int texIndex) const
{
    QMutexLocker k(&_imp->isViewerPausedMutex);

    return _imp->isViewerPaused[texIndex];
}

void
ViewerInstance::onInputChanged(int /*inputNb*/)
{
    Q_EMIT clipPreferencesChanged();
}

void
ViewerInstance::onMetaDatasRefreshed(const NodeMetadata& /*metadata*/)
{
    Q_EMIT clipPreferencesChanged();
}

void
ViewerInstance::onChannelsSelectorRefreshed()
{
    Q_EMIT availableComponentsChanged();
}

void
ViewerInstance::addSupportedBitDepth(std::list<ImageBitDepthEnum>* depths) const
{
    depths->push_back(eImageBitDepthFloat);
    depths->push_back(eImageBitDepthShort);
    depths->push_back(eImageBitDepthByte);
}

void
ViewerInstance::getActiveInputs(int & a,
                                int &b) const
{
    NodePtr n = getNode();
    InspectorNodePtr isInspector = toInspectorNode(n);

    assert(isInspector);
    if (isInspector) {
        isInspector->getActiveInputs(a, b);
    }
}

void
ViewerInstance::setInputA(int inputNb)
{
    NodePtr n = getNode();
    InspectorNodePtr isInspector = toInspectorNode(n);

    assert(isInspector);
    if (isInspector) {
        isInspector->setInputA(inputNb);
    }

    Q_EMIT availableComponentsChanged();
}

void
ViewerInstance::setInputB(int inputNb)
{
    NodePtr n = getNode();
    InspectorNodePtr isInspector = toInspectorNode(n);

    assert(isInspector);
    if (isInspector) {
        isInspector->setInputB(inputNb);
    }

    Q_EMIT availableComponentsChanged();
}

int
ViewerInstance::getLastRenderedTime() const
{
    return _imp->uiContext ? _imp->uiContext->getCurrentlyDisplayedTime() : getApp()->getTimeLine()->currentFrame();
}

TimeLinePtr
ViewerInstance::getTimeline() const
{
    return _imp->uiContext ? _imp->uiContext->getTimeline() : getApp()->getTimeLine();
}

void
ViewerInstance::getTimelineBounds(int* first,
                                  int* last) const
{
    if (_imp->uiContext) {
        _imp->uiContext->getViewerFrameRange(first, last);
    } else {
        *first = 0;
        *last = 0;
    }
}

int
ViewerInstance::getMipMapLevelFromZoomFactor() const
{
    double zoomFactor = _imp->uiContext->getZoomFactor();
    double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );

    return std::log(closestPowerOf2) / M_LN2;
}

double
ViewerInstance::getCurrentTime() const
{
    return getFrameRenderArgsCurrentTime();
}

ViewIdx
ViewerInstance::getCurrentView() const
{
    return getFrameRenderArgsCurrentView();
}

bool
ViewerInstance::isLatestRender(int textureIndex,
                               U64 renderAge) const
{
    return _imp->isLatestRender(textureIndex, renderAge);
}

void
ViewerInstance::setPartialUpdateParams(const std::list<RectD>& rois,
                                       bool recenterViewer)
{
    double viewerCenterX = 0;
    double viewerCenterY = 0;

    if (recenterViewer) {
        RectD bbox;
        bool bboxSet = false;
        for (std::list<RectD>::const_iterator it = rois.begin(); it != rois.end(); ++it) {
            if (!bboxSet) {
                bboxSet = true;
                bbox = *it;
            } else {
                bbox.merge(*it);
            }
        }
        viewerCenterX = (bbox.x1 + bbox.x2) / 2.;
        viewerCenterY = (bbox.y1 + bbox.y2) / 2.;
    }
    QMutexLocker k(&_imp->viewerParamsMutex);
    _imp->partialUpdateRects = rois;
    _imp->viewportCenterSet = recenterViewer;
    _imp->viewportCenter.x = viewerCenterX;
    _imp->viewportCenter.y = viewerCenterY;
}

void
ViewerInstance::clearPartialUpdateParams()
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    _imp->partialUpdateRects.clear();
    _imp->viewportCenterSet = false;
}

void
ViewerInstance::setDoingPartialUpdates(bool doing)
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    _imp->isDoingPartialUpdates = doing;
}

bool
ViewerInstance::isDoingPartialUpdates() const
{
    QMutexLocker k(&_imp->viewerParamsMutex);

    return _imp->isDoingPartialUpdates;
}

void
ViewerInstance::reportStats(int time,
                            ViewIdx view,
                            double wallTime,
                            const RenderStatsMap& stats)
{
    Q_EMIT renderStatsAvailable(time, view, wallTime, stats);
}

NATRON_NAMESPACE_EXIT;

NATRON_NAMESPACE_USING;
#include "moc_ViewerInstance.cpp"
#include "moc_ViewerInstancePrivate.cpp"

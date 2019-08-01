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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerNode.h"
#include "ViewerNodePrivate.h"

#include "Engine/RenderEngine.h"

NATRON_NAMESPACE_ENTER


PluginPtr
ViewerNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_IMAGE);
    PluginPtr ret = Plugin::create(ViewerNode::create, ViewerNode::createRenderClone, PLUGINID_NATRON_VIEWER_GROUP, "Viewer", 1, 0, grouping);

    ret->setProperty<std::string>(kNatronPluginPropIconFilePath, "Images/viewer_icon.png");
    QString desc = tr("The Viewer node can display the output of a node graph. Shift + double click on the viewer node to customize the viewer display process with a custom node tree");

    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));


    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)Key_I, 0);
    ret->setProperty<int>(kNatronPluginPropShortcut, (int)eKeyboardModifierControl, 1
                          );
    ret->addActionShortcut( PluginActionShortcut(kViewerInstanceParamClipToFormat, kViewerInstanceParamClipToFormatLabel, kViewerInstanceParamClipToFormatHint, Key_C, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamFullFrame, kViewerNodeParamFullFrameLabel, kViewerNodeParamFullFrameHint) );
    ret->addActionShortcut( PluginActionShortcut(kViewerInstanceParamEnableUserRoI, kViewerInstanceParamEnableUserRoILabel, kViewerInstanceParamEnableUserRoIHint, Key_W, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamPauseRender, kViewerNodeParamPauseRenderLabel, kViewerNodeParamPauseRenderHint, Key_P) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamEnableGain, kViewerNodeParamEnableGainLabel, kViewerNodeParamEnableGainHint) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamEnableAutoContrast, kViewerNodeParamEnableAutoContrastLabel, kViewerNodeParamEnableAutoContrastHint) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamEnableGamma, kViewerNodeParamEnableGammaLabel, kViewerNodeParamEnableGammaHint) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRefreshViewport, kViewerNodeParamRefreshViewportLabel, kViewerNodeParamRefreshViewportHint, Key_U) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamFitViewport, kViewerNodeParamFitViewportLabel, kViewerNodeParamFitViewportHint, Key_F) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamSyncViewports, kViewerNodeParamSyncViewportsLabel, kViewerNodeParamSyncViewportsHint) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamCheckerBoard, kViewerNodeParamCheckerBoardLabel, kViewerNodeParamCheckerBoardHint) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamEnableColorPicker, kViewerNodeParamEnableColorPickerLabel, kViewerNodeParamEnableColorPickerHint) );

    // Right-click actions
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuToggleWipe, kViewerNodeParamRightClickMenuToggleWipeLabel, "", Key_W) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuCenterWipe, kViewerNodeParamRightClickMenuCenterWipeLabel, "", Key_F, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuPreviousLayer, kViewerNodeParamRightClickMenuPreviousLayerLabel, "", Key_Page_Up) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuNextLayer, kViewerNodeParamRightClickMenuNextLayerLabel, "", Key_Page_Down) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuSwitchAB, kViewerNodeParamRightClickMenuSwitchABLabel, "", Key_Return) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuPreviousView, kViewerNodeParamRightClickMenuPreviousViewLabel, "", Key_Page_Up, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuNextView, kViewerNodeParamRightClickMenuNextViewLabel, "", Key_Page_Down, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideOverlays, kViewerNodeParamRightClickMenuShowHideOverlaysLabel, "", Key_O) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuHideAll, kViewerNodeParamRightClickMenuHideAllLabel, "", Key_space, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierAlt) ));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuHideAllTop, kViewerNodeParamRightClickMenuHideAllTopLabel, "", Key_space, eKeyboardModifierShift ));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuHideAllBottom, kViewerNodeParamRightClickMenuHideAllBottomLabel, "", Key_space, eKeyboardModifierAlt ));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHidePlayer, kViewerNodeParamRightClickMenuShowHidePlayerLabel, "") );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideTimeline, kViewerNodeParamRightClickMenuShowHideTimelineLabel, "") );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideLeftToolbar, kViewerNodeParamRightClickMenuShowHideLeftToolbarLabel, "") );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideTopToolbar, kViewerNodeParamRightClickMenuShowHideTopToolbarLabel, "") );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamRightClickMenuShowHideTabHeader, kViewerNodeParamRightClickMenuShowHideTabHeaderLabel, "") );

    // Viewer actions
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionLuminance, kViewerNodeParamActionLuminanceLabel, "", Key_Y) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionRed, kViewerNodeParamActionRedLabel, "", Key_R) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionRedA, kViewerNodeParamActionRedALabel, "", Key_R, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionGreen, kViewerNodeParamActionGreenLabel, "", Key_G) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionGreenA, kViewerNodeParamActionGreenALabel, "", Key_G, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionBlue, kViewerNodeParamActionBlueLabel, "", Key_B) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionBlueA, kViewerNodeParamActionBlueALabel, "", Key_B, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionAlpha, kViewerNodeParamActionAlphaLabel, "", Key_A) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionAlphaA, kViewerNodeParamActionAlphaALabel, "", Key_A, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionMatte, kViewerNodeParamActionMatteLabel, "", Key_M) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionMatteA, kViewerNodeParamActionMatteALabel, "", Key_M, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionZoomIn, kViewerNodeParamActionZoomInLabel,"",  Key_plus) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionZoomOut, kViewerNodeParamActionZoomOutLabel, "", Key_minus) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionScaleOne, kViewerNodeParamActionScaleOneLabel, "", Key_1, eKeyboardModifierControl) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionProxy2, kViewerNodeParamActionProxy2Label, "", Key_1, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionProxy4, kViewerNodeParamActionProxy4Label, "", Key_2, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionProxy8, kViewerNodeParamActionProxy8Label, "", Key_3, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionProxy16, kViewerNodeParamActionProxy16Label, "", Key_4, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionProxy32, kViewerNodeParamActionProxy32Label, "", Key_5, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionLeftView, kViewerNodeParamActionLeftViewLabel, "", Key_Left, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionRightView, kViewerNodeParamActionRightViewLabel, "", Key_Right, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionCreateNewRoI, kViewerNodeParamActionCreateNewRoILabel, "", Key_W, eKeyboardModifierAlt) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionPauseAB, kViewerNodeParamActionPauseABLabel, "", Key_P, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionRefreshWithStats, kViewerNodeParamActionRefreshWithStatsLabel, "", Key_U, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierControl)) );


    // Player
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamPreviousFrame, kViewerNodeParamPreviousFrameLabel, kViewerNodeParamPreviousFrameHint, Key_Left, eKeyboardModifierNone));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamNextFrame, kViewerNodeParamNextFrameLabel, kViewerNodeParamNextFrameHint, Key_Right, eKeyboardModifierNone));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamPlayBackward, kViewerNodeParamPlayBackwardLabel, kViewerNodeParamPlayBackwardHint, Key_J, eKeyboardModifierNone));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamPlayForward, kViewerNodeParamPlayForwardLabel, kViewerNodeParamPlayForwardHint, Key_L, eKeyboardModifierNone));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamActionAbortRender, kViewerNodeParamActionAbortRenderLabel, kViewerNodeParamActionAbortRenderHint, Key_K, eKeyboardModifierNone));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamPreviousIncr, kViewerNodeParamPreviousIncrLabel, kViewerNodeParamPreviousIncrHint, Key_Left, eKeyboardModifierShift));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamNextIncr, kViewerNodeParamNextIncrLabel, kViewerNodeParamNextIncrHint, Key_Right, eKeyboardModifierShift));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamPreviousKeyFrame, kViewerNodeParamPreviousKeyFrameLabel, kViewerNodeParamPreviousKeyFrameHint, Key_Left, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierControl)));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamNextKeyFrame, kViewerNodeParamNextKeyFrameLabel, kViewerNodeParamNextKeyFrameHint, Key_Right, KeyboardModifiers(eKeyboardModifierShift | eKeyboardModifierControl)));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamFirstFrame, kViewerNodeParamFirstFrameLabel, kViewerNodeParamFirstFrameHint, Key_Left, eKeyboardModifierControl));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamLastFrame, kViewerNodeParamLastFrameLabel, kViewerNodeParamLastFrameHint, Key_Right, eKeyboardModifierControl));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamSetInPoint, kViewerNodeParamSetInPointLabel, kViewerNodeParamSetInPointHint, Key_I, eKeyboardModifierAlt));
    ret->addActionShortcut( PluginActionShortcut(kViewerNodeParamSetOutPoint, kViewerNodeParamSetOutPointLabel, kViewerNodeParamSetOutPointHint, Key_O, eKeyboardModifierAlt));


    return ret;
}

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


    QObject::connect( this, SIGNAL(disconnectTextureRequest(int,bool)), this, SLOT(executeDisconnectTextureRequestOnMainThread(int,bool)) );
    QObject::connect( this, SIGNAL(redrawOnMainThread()), this, SLOT(redrawViewer()) );

    if (node) {
        QObject::connect( node.get(), SIGNAL(inputLabelChanged(int,QString)), this, SLOT(onInputNameChanged(int,QString)) );
    }

}

ViewerNode::~ViewerNode()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );

    if (_imp->uiContext) {
        _imp->uiContext->removeGUI();
    }
}

void
ViewerNode::initializeOverlayInteract()
{
    // Do not enable draft render during the viewer interacts: we don't want a low res image to show while moving
    // the wipe cursor
    getNode()->setDraftEnabledForOverlayActions(false);

    _imp->ui.reset(new ViewerNodeOverlay(_imp.get()));
    registerOverlay(eOverlayViewportTypeViewer, _imp->ui, std::map<std::string, std::string>());
}

RenderEnginePtr
ViewerNode::createRenderEngine()
{

    return ViewerRenderEngine::create(getNode());
}



ViewerInstancePtr
ViewerNode::getViewerProcessNode(int index) const
{
    if (index >= 2 || index < 0) {
        return ViewerInstancePtr();
    }
    NodePtr node = _imp->getViewerProcessNode(index);
    if (!node) {
        return ViewerInstancePtr();
    }
    return toViewerInstance(node->getEffectInstance());
}


void
ViewerNode::setPartialUpdateParams(const std::list<RectD>& rois,
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
    QMutexLocker k(&_imp->partialUpdatesMutex);
    _imp->partialUpdateRects = rois;
    _imp->viewportCenterSet = recenterViewer;
    _imp->viewportCenter.x = viewerCenterX;
    _imp->viewportCenter.y = viewerCenterY;
}

std::list<RectD>
ViewerNode::getPartialUpdateRects() const
{
    QMutexLocker k(&_imp->partialUpdatesMutex);
    return _imp->partialUpdateRects;
}

void
ViewerNode::clearPartialUpdateParams()
{
    QMutexLocker k(&_imp->partialUpdatesMutex);

    _imp->partialUpdateRects.clear();
    _imp->viewportCenterSet = false;
}

bool
ViewerNode::getViewerCenterPoint(Point* center) const
{
    QMutexLocker k(&_imp->partialUpdatesMutex);
    if (_imp->viewportCenterSet) {
        *center = _imp->viewportCenter;
        return true;
    }
    return false;
}

void
ViewerNode::setDoingPartialUpdates(bool doing)
{
    QMutexLocker k(&_imp->partialUpdatesMutex);

    _imp->isDoingPartialUpdates = doing;
}

bool
ViewerNode::isDoingPartialUpdates() const
{
    QMutexLocker k(&_imp->partialUpdatesMutex);

    return _imp->isDoingPartialUpdates;
}

void
ViewerNode::onViewerProcessNodeMetadataRefreshed(const NodePtr& viewerProcessNode, const NodeMetadata& metadata)
{
    int viewerProcess_i = -1;
    for (int i = 0; i < 2; ++i) {
        if (_imp->internalViewerProcessNode[i].lock() == viewerProcessNode) {
            viewerProcess_i = i;
            break;
        }
    }
    if (viewerProcess_i == -1) {
        // The _imp->internalViewerProcessNode may not be yet set if we are still in the createNode() call in the
        // ViewerNode::createViewerProcessNode() call.
        return;
    }

    refreshFps();
    refreshViewsKnobVisibility();
    
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->refreshMetadata(viewerProcess_i, metadata);
    }

}

void
ViewerNode::onInputChanged(int /*inputNb*/)
{
    _imp->refreshInputChoices(true);
    refreshInputFromChoiceMenu(0);
    refreshInputFromChoiceMenu(1);

}

void
ViewerNode::onInputNameChanged(int,QString)
{
    _imp->refreshInputChoices(false);
}


void
ViewerNode::createViewerProcessNode()
{
    NodePtr internalViewerNode[2];

    for (int i = 0; i < 2; ++ i) {
        ViewerNodePtr thisShared = shared_from_this();

        QString nodeName = QString::fromUtf8("ViewerProcess");
        nodeName += QString::number(i + 1);
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_VIEWER_INTERNAL, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty<bool>(kCreateNodeArgsPropAllowNonUserCreatablePlugins, true);
        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, nodeName.toStdString());
        internalViewerNode[i] = getApp()->createNode(args);
        assert(internalViewerNode[i]);
        if (!internalViewerNode[i]) {
            throw std::invalid_argument("ViewerNode::setupGraph: No internal viewer process!");
        }


        if (i == 1) {
            Point position;
            internalViewerNode[0]->getPosition(&position.x, &position.y);
            double w,h;
            internalViewerNode[0]->getSize(&w, &h);
            internalViewerNode[0]->setPosition(position.x - w /2. - w/2, position.y);

            internalViewerNode[1]->setPosition(position.x + w /2., position.y);

        }

        _imp->internalViewerProcessNode[i] = internalViewerNode[i];

        // Link output layer and alpha channel to the viewer process choices
        KnobIPtr viewerProcessOutputLayerChoice = internalViewerNode[i]->getKnobByName(kViewerInstanceParamOutputLayer);
        viewerProcessOutputLayerChoice->linkTo(_imp->layersKnob.lock());

        KnobIPtr viewerProcessAlphaChannelChoice = internalViewerNode[i]->getKnobByName(kViewerInstanceParamAlphaChannel);
        viewerProcessAlphaChannelChoice->linkTo(_imp->alphaChannelKnob.lock());

        KnobIPtr viewerProcessDisplayChanelsChoice = internalViewerNode[i]->getKnobByName(kViewerInstanceParamDisplayChannels);
        viewerProcessDisplayChanelsChoice->linkTo(_imp->displayChannelsKnob[i].lock());

        KnobIPtr viewerProcessGammaKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamGamma);
        viewerProcessGammaKnob->linkTo(_imp->gammaSliderKnob.lock());

        KnobIPtr viewerProcessGainKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceNodeParamGain);
        viewerProcessGainKnob->linkTo(_imp->gainSliderKnob.lock());

        KnobIPtr viewerProcessAutoContrastKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamEnableAutoContrast);
        viewerProcessAutoContrastKnob->linkTo(_imp->enableAutoContrastButtonKnob.lock());

        KnobIPtr viewerProcessAutoColorspaceKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamColorspace);
        viewerProcessAutoColorspaceKnob->linkTo(_imp->colorspaceKnob.lock());

        KnobIPtr userRoiEnabledKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamEnableUserRoI);
        userRoiEnabledKnob->linkTo(_imp->toggleUserRoIButtonKnob.lock());

        KnobIPtr userRoiBtmLeftKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamUserRoIBottomLeft);
        userRoiBtmLeftKnob->linkTo(_imp->userRoIBtmLeftKnob.lock());

        KnobIPtr userRoiSizeKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamUserRoISize);
        userRoiSizeKnob->linkTo(_imp->userRoISizeKnob.lock());

        KnobIPtr clipToFormatKnob = internalViewerNode[i]->getKnobByName(kViewerInstanceParamClipToFormat);
        clipToFormatKnob->linkTo(_imp->clipToFormatButtonKnob.lock());


        // A ViewerNode is composed of 2 ViewerProcess nodes but it only has 1 layer and 1 alpha channel choices.
        // We thus disable the refreshing of the menu from the 2nd ViewerProcess node.
        if (i == 1) {
            ViewerInstancePtr instance = internalViewerNode[i]->isEffectViewerInstance();
            instance->setRefreshLayerAndAlphaChoiceEnabled(false);
        }
    }

    Q_EMIT internalViewerCreated();
} // createViewerProcessNode

void
ViewerNode::setupGraph(bool createViewerProcess)
{
    // Viewers are not considered edited by default
    setSubGraphEditedByUser(false);

    ViewerNodePtr thisShared = shared_from_this();

    NodePtr internalViewerNode0 = _imp->internalViewerProcessNode[0].lock();
    assert(createViewerProcess || internalViewerNode0);
    if (createViewerProcess) {
        createViewerProcessNode();
        internalViewerNode0 = _imp->internalViewerProcessNode[0].lock();
    }


    double inputWidth = 1, inputHeight = 1;
    if (internalViewerNode0) {
        internalViewerNode0->getSize(&inputWidth, &inputHeight);
    }
    double inputX = 0, inputY = 0;
    if (internalViewerNode0) {
        internalViewerNode0->getPosition(&inputX, &inputY);

        // Offset so that we start in the middle inbetween the 2 viewer process nodes
        inputX += inputWidth + inputWidth / 2;
    }

    double startOffset = - (VIEWER_INITIAL_N_INPUTS / 2) * inputWidth - inputWidth / 2. - (VIEWER_INITIAL_N_INPUTS / 2 - 1) * inputWidth / 2;

    std::vector<NodePtr> inputNodes(VIEWER_INITIAL_N_INPUTS);
    // Create input nodes
    for (int i = 0; i < VIEWER_INITIAL_N_INPUTS; ++i) {
        QString inputName = QString::fromUtf8("Input%1").arg(i + 1);
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropAutoConnect, false);
        args->setProperty<bool>(kCreateNodeArgsPropAddUndoRedoCommand, false);
        args->setProperty<bool>(kCreateNodeArgsPropSettingsOpened, false);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, inputName.toStdString());
        args->setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, inputX + startOffset, 0);
        args->setProperty<double>(kCreateNodeArgsPropNodeInitialPosition, inputY - inputHeight * 10, 1);
        //args.addParamDefaultValue<bool>(kNatronGroupInputIsOptionalParamName, true);
        inputNodes[i] = getApp()->createNode(args);
        assert(inputNodes[i]);
        startOffset += inputWidth * 1.5;
    }

}

void
ViewerNode::setupInitialSubGraphState()
{
    setupGraph(true);
}

void
ViewerNode::clearGroupWithoutViewerProcess()
{
    // When we load the internal node-graph we don't want to kill the viewer process node, hence we remove it temporarily from the group so it doesn't get killed
    // and the re-add it back.
    if (getNodes().empty()) {
        return;
    }
    NodePtr viewerProcessNode[2];
    for (int i = 0; i < 2; ++i) {
        viewerProcessNode[i] = _imp->internalViewerProcessNode[i].lock();
        assert(viewerProcessNode[i]);
        removeNode(viewerProcessNode[i]);
    }

    clearNodesBlocking();

    for (int i = 0; i < 2; ++i) {
        addNode(viewerProcessNode[i]);
    }
}

void
ViewerNode::loadSubGraph(const SERIALIZATION_NAMESPACE::NodeSerialization* projectSerialization,
                         const SERIALIZATION_NAMESPACE::NodeSerialization* pyPlugSerialization)
{


    if (getNode()->isPyPlug()) {
        assert(pyPlugSerialization);
        // Handle the pyplug case on NodeGroup implementation
        NodeGroup::loadSubGraph(projectSerialization, pyPlugSerialization);
    } else if (projectSerialization) {
        // If there's a project serialization load it. There will be children only if the user edited the Viewer group
        if (!projectSerialization->_children.empty()) {

            // The subgraph was not initialized in this case
            assert(getNodes().empty());

            EffectInstancePtr thisShared = shared_from_this();

            // Clear nodes that were created if any
            clearGroupWithoutViewerProcess();

            // Restore group
            createNodesFromSerialization(projectSerialization->_children, eCreateNodesFromSerializationFlagsNone, 0);

            setSubGraphEditedByUser(true);
        } else {

            // We are loading a non edited default viewer, just ensure the initial setup was done
            if (!_imp->internalViewerProcessNode[0].lock()) {
                setupGraph(true);
            }

            setSubGraphEditedByUser(false);
        }
    }



    // Ensure the internal viewer process node exists
    if (!_imp->internalViewerProcessNode[0].lock()) {
        NodePtr internalViewerNode[2];
        NodesList nodes = getNodes();
        int viewerProcessNode_i = 0;
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if ((*it)->isEffectViewerInstance()) {
                internalViewerNode[viewerProcessNode_i] = *it;
                ++viewerProcessNode_i;
                if (viewerProcessNode_i == 2) {
                    break;
                }
            }
        }
        assert(internalViewerNode[0] && internalViewerNode[1]);
        if (!internalViewerNode[0] || !internalViewerNode[1]) {
            throw std::invalid_argument("Viewer: No internal viewer process!");
        }
        _imp->internalViewerProcessNode[0] = internalViewerNode[0];
        _imp->internalViewerProcessNode[1] = internalViewerNode[1];
        Q_EMIT internalViewerCreated();
        

    }
    assert(getViewerProcessNode(0));
    assert(getViewerProcessNode(1));

    _imp->refreshInputChoices(true);
    refreshInputFromChoiceMenu(0);
    refreshInputFromChoiceMenu(1);
}



void
ViewerNode::refreshViewsKnobVisibility()
{
    KnobChoicePtr knob = _imp->activeViewKnob.lock();
    if (knob) {
        const std::vector<std::string>& views = getApp()->getProject()->getProjectViewNames();
        std::vector<ChoiceOption> entries(views.size());
        for (std::size_t i = 0; i < views.size(); ++i) {
            entries[i] = ChoiceOption(views[i], "", "");
        }
        knob->populateChoices(entries);
        knob->setInViewerContextSecret(views.size() <= 1);
    }
}



void
ViewerNode::connectInputToIndex(int groupInputIndex, int internalInputIndex)
{

    // We want to connect the node upstream of the internal viewer process node (or this node if there's nothing else upstream)
    // to the appropriate GroupInput node inside the group
    NodePtr internalNodeToConnect = _imp->getInputRecursive(internalInputIndex);
    assert(internalNodeToConnect);

    std::vector<NodePtr> inputNodes;
    getInputs(&inputNodes);
    if (groupInputIndex >= (int)inputNodes.size() || groupInputIndex < 0) {
        // Invalid input index
        return;
    }

    // This is the GroupInput node inside the group to connect to
    NodePtr groupInput = inputNodes[groupInputIndex];

    // Update the input choice
    _imp->refreshInputChoiceMenu(internalInputIndex, groupInputIndex);


    // Connect the node recursive upstream of the internal viewer process to the corresponding GroupInput node
    if (internalNodeToConnect == _imp->internalViewerProcessNode[internalInputIndex].lock()) {
        internalNodeToConnect->disconnectInput(0);
        internalNodeToConnect->connectInput(groupInput, 0);
    } else {
        int prefInput = internalNodeToConnect->getPreferredInputForConnection();
        if (prefInput == -1) {
            // Preferred input might be connected, disconnect it first
            prefInput = internalNodeToConnect->getPreferredInput();
            if (prefInput != -1) {
                internalNodeToConnect->disconnectInput(prefInput);
            }
            internalNodeToConnect->connectInput(groupInput, prefInput);
        }
    }
}

void
ViewerNode::setZoomComboBoxText(const std::string& text)
{
    ChoiceOption opt(text, "", "");
    _imp->zoomChoiceKnob.lock()->setActiveEntry(opt, ViewSetSpec(0), eValueChangedReasonPluginEdited);
}

bool
ViewerNode::isLeftToolbarVisible() const
{
    return _imp->rightClickShowHideLeftToolbar.lock()->getValue();
}

bool
ViewerNode::isTopToolbarVisible() const
{
    return _imp->rightClickShowHideTopToolbar.lock()->getValue();
}

bool
ViewerNode::isTimelineVisible() const
{
    return _imp->rightClickShowHideTimeline.lock()->getValue();
}

bool
ViewerNode::isPlayerVisible() const
{
    return _imp->rightClickShowHidePlayer.lock()->getValue();
}

bool
ViewerNode::isInfoBarVisible() const
{
    return _imp->enableInfoBarButtonKnob.lock()->getValue();
}

void
ViewerNode::forceNextRenderWithoutCacheRead()
{
    QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
    _imp->forceRender = true;
}

bool
ViewerNode::isRenderWithoutCacheEnabledAndTurnOff()
{
    QMutexLocker forceRenderLocker(&_imp->forceRenderMutex);
    if (_imp->forceRender) {
        _imp->forceRender = false;
        return true;
    }
    return false;
}


DisplayChannelsEnum
ViewerNode::getDisplayChannels(int index) const
{
    KnobChoicePtr displayChoice = _imp->displayChannelsKnob[index].lock();
    return (DisplayChannelsEnum)displayChoice->getValue();
}

bool
ViewerNode::isAutoContrastEnabled() const
{
    return _imp->enableAutoContrastButtonKnob.lock()->getValue();
}

ViewerColorSpaceEnum
ViewerNode::getColorspace() const
{
    return (ViewerColorSpaceEnum)_imp->colorspaceKnob.lock()->getValue();
}

OpenGLViewerI*
ViewerNode::getUiContext() const
{
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
ViewerNode::invalidateUiContext()
{
    // always running in the main thread
    assert( qApp && qApp->thread() == QThread::currentThread() );
    _imp->uiContext = NULL;
}


NodePtr
ViewerNode::getCurrentAInput() const
{
    ChoiceOption curLabel = _imp->aInputNodeChoiceKnob.lock()->getCurrentEntry();
    if (curLabel.id == "-") {
        return NodePtr();
    }
    int inputIndex = QString::fromUtf8(curLabel.id.c_str()).toInt();
    return getNode()->getInput(inputIndex);
}

NodePtr
ViewerNode::getCurrentBInput() const
{
    ChoiceOption curLabel = _imp->bInputNodeChoiceKnob.lock()->getCurrentEntry();
    if (curLabel.id == "-") {
        return NodePtr();
    }
    int inputIndex = QString::fromUtf8(curLabel.id.c_str()).toInt();
    return getNode()->getInput(inputIndex);

}

void
ViewerNode::refreshInputFromChoiceMenu(int internalInputIdx)
{
    assert(internalInputIdx == 0 || internalInputIdx == 1);

    // Get the "Input" nodes
    std::vector<NodePtr> groupInputNodes;
    getInputs(&groupInputNodes);

    KnobChoicePtr knob = internalInputIdx == 0 ? _imp->aInputNodeChoiceKnob.lock() : _imp->bInputNodeChoiceKnob.lock();

    // Read from the choice menu the current selected node
    ChoiceOption curLabel = internalInputIdx == 0 ? _imp->aInputNodeChoiceKnob.lock()->getCurrentEntry() : _imp->bInputNodeChoiceKnob.lock()->getCurrentEntry();

    // Find recursively the node that we should connect to the "Input" node
    NodePtr nodeToConnect = _imp->getInputRecursive(internalInputIdx);
    if (curLabel.id == "-") {
        if (nodeToConnect->getEffectInstance().get() == this) {
            nodeToConnect->disconnectInput(internalInputIdx);
        } else {
            int prefInput = nodeToConnect->getPreferredInput();
            if (prefInput != -1) {
                nodeToConnect->disconnectInput(prefInput);
            }
        }

    } else {

        int groupInputIndex = QString::fromUtf8(curLabel.id.c_str()).toInt();
        if (groupInputIndex < (int)groupInputNodes.size() && groupInputIndex >= 0) {

            if (nodeToConnect == _imp->internalViewerProcessNode[internalInputIdx].lock()) {
                nodeToConnect->swapInput(groupInputNodes[groupInputIndex], 0);
            } else {
                int prefInput = nodeToConnect->getPreferredInputForConnection();
                if (prefInput != -1) {
                    nodeToConnect->swapInput(groupInputNodes[groupInputIndex], prefInput);
                }
            }
        }
    }
} // refreshInputFromChoiceMenu

ViewerCompositingOperatorEnum
ViewerNode::getCurrentOperator() const
{
    return (ViewerCompositingOperatorEnum)_imp->blendingModeChoiceKnob.lock()->getValue();
}

void
ViewerNode::setRefreshButtonDown(bool down)
{
    KnobButtonPtr knob = _imp->refreshButtonKnob.lock();
    // Ignore evaluation

    ScopedChanges_RAII changes(this, true);
    knob->setValue(down, ViewIdx(0), DimIdx(0), eValueChangedReasonTimeChanged);

}

bool
ViewerNode::isViewersSynchroEnabled() const
{
    return _imp->syncViewersButtonKnob.lock()->getValue();
}

void
ViewerNode::setViewersSynchroEnabled(bool enabled)
{
    _imp->syncViewersButtonKnob.lock()->setValue(enabled);
}

void
ViewerNode::setPickerEnabled(bool enabled)
{
    _imp->enableInfoBarButtonKnob.lock()->setValue(enabled);
}

ViewIdx
ViewerNode::getCurrentRenderView() const
{
    return ViewIdx(_imp->activeViewKnob.lock()->getValue());
}

void
ViewerNode::setCurrentView(ViewIdx view)
{
    _imp->activeViewKnob.lock()->setValue(view.value());
}

bool
ViewerNode::isClipToFormatEnabled() const
{
    return _imp->clipToFormatButtonKnob.lock()->getValue();
}

double
ViewerNode::getWipeAmount() const
{
    return _imp->wipeAmount.lock()->getValue();
}

double
ViewerNode::getWipeAngle() const
{
    return _imp->wipeAngle.lock()->getValue();
}

QPointF
ViewerNode::getWipeCenter() const
{
    KnobDoublePtr wipeCenter = _imp->wipeCenter.lock();
    QPointF r;
    r.rx() = wipeCenter->getValue();
    r.ry() = wipeCenter->getValue(DimIdx(1));
    return r;
}

bool
ViewerNode::isCheckerboardEnabled() const
{
    return _imp->enableCheckerboardButtonKnob.lock()->getValue();
}

bool
ViewerNode::isUserRoIEnabled() const
{
    return _imp->toggleUserRoIButtonKnob.lock()->getValue();
}

bool
ViewerNode::isOverlayEnabled() const
{
    return _imp->rightClickShowHideOverlays.lock()->getValue();
}

bool
ViewerNode::isFullFrameProcessingEnabled() const
{
    return _imp->fullFrameButtonKnob.lock()->getValue();
}

void
ViewerNode::resetWipe()
{
    ScopedChanges_RAII changes(this);
    _imp->wipeCenter.lock()->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    _imp->wipeAngle.lock()->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    _imp->wipeAmount.lock()->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
}

KnobChoicePtr
ViewerNode::getLayerKnob() const
{
    return _imp->layersKnob.lock();
}

KnobChoicePtr
ViewerNode::getAlphaChannelKnob() const
{
    return _imp->alphaChannelKnob.lock();
}

KnobIntPtr
ViewerNode::getPlaybackInPointKnob() const
{
    return _imp->inPointKnob.lock();
}

KnobIntPtr
ViewerNode::getPlaybackOutPointKnob() const
{
    return _imp->outPointKnob.lock();
}

KnobIntPtr
ViewerNode::getCurrentFrameKnob() const
{
    return _imp->curFrameKnob.lock();
}

KnobButtonPtr
ViewerNode::getTurboModeButtonKnob() const
{
    return _imp->enableTurboModeButtonKnob.lock();
}

bool
ViewerNode::isViewerPaused(int index) const
{
    return _imp->pauseButtonKnob[index].lock()->getValue();
}

RectD
ViewerNode::getUserRoI() const
{
    RectD ret;
    KnobDoublePtr btmLeft = _imp->userRoIBtmLeftKnob.lock();
    KnobDoublePtr size = _imp->userRoISizeKnob.lock();
    ret.x1 = btmLeft->getValue();
    ret.y1 = btmLeft->getValue(DimIdx(1));
    ret.x2 = ret.x1 + size->getValue();
    ret.y2 = ret.y1 + size->getValue(DimIdx(1));
    return ret;
}

void
ViewerNode::setUserRoI(const RectD& rect)
{
    KnobDoublePtr btmLeft = _imp->userRoIBtmLeftKnob.lock();
    KnobDoublePtr size = _imp->userRoISizeKnob.lock();
    std::vector<double> values(2);
    values[0] = rect.x1;
    values[1] = rect.y1;
    btmLeft->setValueAcrossDimensions(values, DimIdx(0), ViewSetSpec::all(), eValueChangedReasonUserEdited);
    values[0] = rect.x2 - rect.x1;
    values[1] = rect.y2 - rect.y1;
    size->setValueAcrossDimensions(values, DimIdx(0), ViewSetSpec::all(), eValueChangedReasonUserEdited);
}

void
ViewerNode::reportStats(int time, double wallTime, const RenderStatsMap& stats)
{
    Q_EMIT renderStatsAvailable(time, wallTime, stats);
}

void
ViewerNode::executeDisconnectTextureRequestOnMainThread(int index,bool clearRoD)
{
    assert( QThread::currentThread() == qApp->thread() );
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->disconnectInputTexture(index, clearRoD);
    }
}


int
ViewerNode::getDownscaleMipMapLevelKnobIndex() const
{
    int index =  _imp->downscaleChoiceKnob.lock()->getValue();
    return index;
}

void
ViewerNode::redrawViewer()
{
    // always running in the main thread
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->redraw();
    }
}


void
ViewerNode::redrawViewerNow()
{
    // always running in the main thread
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->redrawNow();
    }
}

void
ViewerNode::disconnectViewer()
{
    // always running in the render thread
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        s_viewerDisconnected();
    }
}

bool
ViewerNode::isViewerUIVisible() const
{
    assert( qApp && qApp->thread() == QThread::currentThread() );
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        return uiContext->isViewerUIVisible();
    }
    return false;
}

void
ViewerNode::disconnectTexture(int index,bool clearRod)
{
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        s_disconnectTextureRequest(index, clearRod);
    }
}

void
ViewerNode::clearLastRenderedImage()
{
    NodeGroup::clearLastRenderedImage();

    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->clearLastRenderedImage();
    }
}

double
ViewerNode::getUIZoomFactor() const
{
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        return uiContext->getZoomFactor();
    }
    return 1.;

}

unsigned int
ViewerNode::getMipMapLevelFromZoomFactor() const
{
    OpenGLViewerI* uiContext = getUiContext();
    if (!uiContext) {
        return 0;
    }
    double zoomFactor = uiContext->getZoomFactor();
    // Downscale level is set to Auto, compute it from the zoom factor.
    // This is the current zoom factor (1. == 100%) currently set by the user in the viewport. This is thread-safe.
    
    // If we were to render at 48% zoom factor, we would render at 50% which is mipmapLevel=1
    // If on the other hand the zoom factor would be at 51%, then we would render at 100% which is mipmapLevel=0
    
    // Adjust the mipmap level (without taking draft into account yet) as the max of the closest mipmap level of the viewer zoom
    // and the requested user proxy mipmap level
    
    //double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );
    //return std::log(closestPowerOf2) / M_LN2;
    //double closestPowerOf2 = zoomFactor >= 1 ? 1 : std::pow( 2, -std::ceil(std::log(zoomFactor) / M_LN2) );
    return zoomFactor >= 1 ? 0 : (int)-std::ceil(std::log(zoomFactor) / M_LN2);
}

void
ViewerNode::updateViewer(const UpdateViewerArgs& args)
{

    OpenGLViewerI* uiContext = getUiContext();
    assert(uiContext);

    uiContext->clearPartialUpdateTextures();

    for (int i = 0; i < 2; ++i) {
        RectD rod;
        NodePtr viewerProcessNode = _imp->internalViewerProcessNode[i].lock();
        GetRegionOfDefinitionResultsPtr actionResults;
        ActionRetCodeEnum stat = viewerProcessNode->getEffectInstance()->getRegionOfDefinition_public(args.time, RenderScale(1.), args.view,  &actionResults);
        if (!isFailureRetCode(stat)) {
            rod = actionResults->getRoD();
        }

        for (std::list<UpdateViewerArgs::TextureUpload>::const_iterator it = args.viewerUploads[i].begin(); it!= args.viewerUploads[i].end(); ++it) {

            OpenGLViewerI::TextureTransferArgs transferArgs;
            transferArgs.image = it->image;
            transferArgs.colorPickerImage = it->colorPickerImage;
            transferArgs.colorPickerInputImage = it->colorPickerInputImage;
            transferArgs.textureIndex = i;
            transferArgs.time = args.time;
            transferArgs.view = args.view;
            transferArgs.rod = rod;
            transferArgs.recenterViewer = args.recenterViewer;
            transferArgs.viewportCenter = args.viewerCenter;
            transferArgs.viewerProcessNodeKey = it->viewerProcessImageKey;
            transferArgs.type = args.type;
            uiContext->transferBufferFromRAMtoGPU(transferArgs);
        }

    }
}

TimeValue
ViewerNode::getTimelineCurrentTime() const
{
    TimeLinePtr timeline = getTimeline();
    return TimeValue(timeline->currentFrame());
}

TimeValue
ViewerNode::getLastRenderedTime() const
{
    OpenGLViewerI* uiContext = getUiContext();

    return uiContext ? uiContext->getCurrentlyDisplayedTime() : TimeValue(getApp()->getTimeLine()->currentFrame());
}

TimeLinePtr
ViewerNode::getTimeline() const
{
    OpenGLViewerI* uiContext = getUiContext();

    return uiContext ? uiContext->getTimeline() : getApp()->getTimeLine();
}

void
ViewerNode::getTimelineBounds(int* first,
                                  int* last) const
{
    OpenGLViewerI* uiContext = getUiContext();
    if (uiContext) {
        uiContext->getViewerFrameRange(first, last);
    } else {
        *first = 0;
        *last = 0;
    }
}



void
ViewerNodePrivate::startPlayback(RenderDirectionEnum direction)
{
    abortAllViewersRendering();

    if ( _publicInterface->getApp()->checkAllReadersModificationDate(true) ) {
        return;
    }
    _publicInterface->getApp()->setLastViewerUsingTimeline(_publicInterface->getNode());

    // Render selected view
    std::vector<ViewIdx> viewsToRender;
    viewsToRender.push_back(_publicInterface->getCurrentRenderView());

    _publicInterface->getNode()->getRenderEngine()->renderFromCurrentFrame(_publicInterface->getApp()->isRenderStatsActionChecked(), viewsToRender, direction);


}

void
ViewerNode::refreshFps()
{
    bool fpsEnabled = _imp->enableFpsKnob.lock()->getValue();
    double fps;
    if (fpsEnabled) {
        fps = _imp->fpsKnob.lock()->getValue();
    } else {
        NodePtr input0 = getCurrentAInput();
        NodePtr input1 = getCurrentBInput();

        if (input0) {
            fps = input0->getEffectInstance()->getFrameRate();
        } else {
            if (input1) {
                fps = input1->getEffectInstance()->getFrameRate();
            } else {
                fps = getApp()->getProjectFrameRate();
            }
        }
        _imp->fpsKnob.lock()->setValue(fps, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    }

    getNode()->getRenderEngine()->setDesiredFPS(fps);

}


void
ViewerNode::onEngineStarted(bool forward)
{

    std::list<ViewerNodePtr> viewers;
    _imp->getAllViewerNodes(false, viewers);
    for (std::list<ViewerNodePtr>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
        (*it)->_imp->playForwardButtonKnob.lock()->setValue(forward, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        (*it)->_imp->playBackwardButtonKnob.lock()->setValue(!forward, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
    }

    if (!getApp()->isGuiFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled() ) {
        getApp()->setGuiFrozen(true);
    }
}


void
ViewerNode::onEngineStopped()
{
 

    // Don't set the playback buttons up now, do it a bit later, maybe the user will restart playback  just aftewards
    _imp->mustSetUpPlaybackButtonsTimer.start(200);

    std::list<ViewerNodePtr> viewers;
    _imp->getAllViewerNodes(false, viewers);

    _imp->curFrameKnob.lock()->setValue( getTimelineCurrentTime(), ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);

    if (!getApp()->isGuiFrozen() && appPTR->getCurrentSettings()->isAutoTurboEnabled() ) {
        getApp()->setGuiFrozen(false);
    } else {
        getApp()->refreshAllTimeEvaluationParams(true);
    }
}


void
ViewerNode::onSetDownPlaybackButtonsTimeout()
{
    if ( getNode()->getRenderEngine() && !getNode()->getRenderEngine()->isDoingSequentialRender() ) {
        std::list<ViewerNodePtr> viewers;
        _imp->getAllViewerNodes(false, viewers);
        for (std::list<ViewerNodePtr>::const_iterator it = viewers.begin(); it != viewers.end(); ++it) {
            (*it)->_imp->playForwardButtonKnob.lock()->setValue(false, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
            (*it)->_imp->playBackwardButtonKnob.lock()->setValue(false, ViewSetSpec::all(), DimIdx(0), eValueChangedReasonPluginEdited);
        }
    }
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING
#include "moc_ViewerNode.cpp"

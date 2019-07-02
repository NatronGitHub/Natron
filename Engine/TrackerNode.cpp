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

#include <boost/algorithm/clamp.hpp>

#include "TrackerNode.h"

#include "Engine/AppInstance.h"
#include "Engine/CreateNodeArgs.h"
#include "Engine/Curve.h"
#include "Engine/Format.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Lut.h"
#include "Engine/Project.h"
#include "Engine/OverlaySupport.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackArgs.h"
#include "Engine/TrackerHelper.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerNodePrivate.h"
#include "Engine/TrackerHelperPrivate.h"
#include "Engine/KnobItemsTableUndoCommand.h"
#include "Engine/ViewerInstance.h"

#include "Global/GLIncludes.h"

#include "Serialization/KnobTableItemSerialization.h"


NATRON_NAMESPACE_ENTER


#define NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING 10


PluginPtr
TrackerNode::createPlugin()
{
    std::vector<std::string> grouping;
    grouping.push_back(PLUGIN_GROUP_TRANSFORM);
    PluginPtr ret = Plugin::create(TrackerNode::create, TrackerNode::createRenderClone, PLUGINID_NATRON_TRACKER, "Tracker", 1, 0, grouping);

    QString desc = tr(
                      "Track one or more 2D point(s) using LibMV from the Blender open-source software.\n\n"
                      "Goal\n"
                      "----\n\n"
                      "Track one or more 2D point and use them to either make another object/image match-move their motion or to stabilize the input image.\n\n"
                      "Tracking\n"
                      "--------\n\n"
                      "* Connect a Tracker node to the image containing the item you need to track\n"
                      "* Place tracking markers with CTRL+ALT+Click on the Viewer or by clicking the **+** button below the track table in the settings panel\n"
                      "* Setup the motion model to match the motion type of the item you need to track. By default the tracker will only assume the item is underoing a translation. Other motion models can be used for complex tracks but may be slower.\n"
                      "* Select in the settings panel or on the Viewer the markers you want to track and then start tracking with the player buttons on the top of the Viewer.\n"
                      "* If a track is getting lost or fails at some point, you may refine it by moving the marker at its correct position, this will force a new keyframe on the pattern which will be visible in the Viewer and on the timeline.\n\n"
                      "Using the tracks data\n"
                      "---------------------\n\n"
                      "You can either use the Tracker node itself to use the track data or you may export it to another node.\n\n"
                      "Using the Transform within the Tracker node\n"
                      "-------------------------------------------\n\n"
                      "Go to the Transform tab in the settings panel, and set the Transform Type to the operation you want to achieve. During tracking, the Transform Type should always been set to None if you want to correctly see the tracks on the Viewer.\n\n"
                      "You will notice that the transform parameters will be set automatically when the tracking is finished. Depending on the Transform Type, the values will be computed either to match-move the motion of the tracked points or to stabilize the image.\n\n"
                      "Exporting the tracking data\n"
                      "---------------------------\n\n"
                      "You may export the tracking data either to a CornerPin node or to a Transform node. The CornerPin node performs a warp that may be more stable than a Transform node when using 4 or more tracks: it retains more information than the Transform node.");
    ret->setProperty<std::string>(kNatronPluginPropDescription, desc.toStdString());
    ret->setProperty<bool>(kNatronPluginPropDescriptionIsMarkdown, true);

    EffectDescriptionPtr effectDesc = ret->getEffectDescriptor();
    effectDesc->setProperty<RenderSafetyEnum>(kEffectPropRenderThreadSafety, eRenderSafetyFullySafe);
    effectDesc->setProperty<bool>(kEffectPropSupportsTiles, true);
    ret->setProperty<std::string>(kNatronPluginPropIconFilePath,  "Images/trackerNodeIcon.png");

    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthFloat, 0);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthShort, 1);
    ret->setProperty<ImageBitDepthEnum>(kNatronPluginPropOutputSupportedBitDepths, eImageBitDepthByte, 2);
    ret->setProperty<std::bitset<4> >(kNatronPluginPropOutputSupportedComponents, std::bitset<4>(std::string("1111")));

    // Viewer buttons
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackBW, kTrackerUIParamTrackBWLabel, kTrackerUIParamTrackBWHint, Key_Z) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackPrevious, kTrackerUIParamTrackPreviousLabel, kTrackerUIParamTrackPreviousHint, Key_X) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackNext, kTrackerUIParamTrackNextLabel, kTrackerUIParamTrackNextHint, Key_C) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamStopTracking, kTrackerUIParamStopTrackingLabel, kTrackerUIParamStopTrackingHint, Key_Escape) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackFW, kTrackerUIParamTrackFWLabel, kTrackerUIParamTrackFWHint, Key_V) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackRange, kTrackerUIParamTrackRangeLabel, kTrackerUIParamTrackRangeHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackAllKeyframes, kTrackerUIParamTrackAllKeyframesLabel, kTrackerUIParamTrackAllKeyframesHint, Key_V, eKeyboardModifierControl) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamTrackCurrentKeyframe, kTrackerUIParamTrackCurrentKeyframeLabel, kTrackerUIParamTrackCurrentKeyframeHint, Key_C, eKeyboardModifierControl) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamClearAllAnimation, kTrackerUIParamClearAllAnimationLabel, kTrackerUIParamClearAllAnimationHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamClearAnimationBw, kTrackerUIParamClearAnimationBwLabel, kTrackerUIParamClearAnimationBwHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamClearAnimationFw, kTrackerUIParamClearAnimationFwLabel, kTrackerUIParamClearAnimationFwHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRefreshViewer, kTrackerUIParamRefreshViewerLabel, kTrackerUIParamRefreshViewerHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamCenterViewer, kTrackerUIParamCenterViewerLabel, kTrackerUIParamCenterViewerHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamCreateKeyOnMove, kTrackerUIParamCreateKeyOnMoveLabel, kTrackerUIParamCreateKeyOnMoveHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamShowError, kTrackerUIParamShowErrorLabel, kTrackerUIParamShowErrorHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamSetPatternKeyFrame, kTrackerUIParamSetPatternKeyFrameLabel, kTrackerUIParamSetPatternKeyFrameHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRemovePatternKeyFrame, kTrackerUIParamRemovePatternKeyFrameLabel, kTrackerUIParamRemovePatternKeyFrameHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamResetOffset, kTrackerUIParamResetOffsetLabel, kTrackerUIParamResetOffsetHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamResetTrack, kTrackerUIParamResetTrackLabel, kTrackerUIParamResetTrackHint) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRightClickMenuActionSelectAllTracks, kTrackerUIParamRightClickMenuActionSelectAllTracksLabel, "", Key_A, eKeyboardModifierControl) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRightClickMenuActionRemoveTracks, kTrackerUIParamRightClickMenuActionRemoveTracksLabel, "", Key_BackSpace) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeBottom, kTrackerUIParamRightClickMenuActionNudgeBottomLabel, "", Key_Down, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeTop, kTrackerUIParamRightClickMenuActionNudgeTopLabel, "", Key_Up, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeRight, kTrackerUIParamRightClickMenuActionNudgeRightLabel, "", Key_Right, eKeyboardModifierShift) );
    ret->addActionShortcut( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeLeft, kTrackerUIParamRightClickMenuActionNudgeLeftLabel, "", Key_Left, eKeyboardModifierShift) );


    return ret;
} // createPlugin

// make_shared enabler (because make_shared needs access to the private constructor)
// see https://stackoverflow.com/a/20961251/2607517
struct TrackerNodePrivate::MakeSharedEnabler: public TrackerNodePrivate
{
    MakeSharedEnabler(TrackerNode* publicInterface) : TrackerNodePrivate(publicInterface) {
    }
};


boost::shared_ptr<TrackerNodePrivate>
TrackerNodePrivate::create(TrackerNode* publicInterface)
{
    return boost::make_shared<TrackerNodePrivate::MakeSharedEnabler>(publicInterface);
}


TrackerNode::TrackerNode(const NodePtr& node)
    : NodeGroup(node)
    , _imp( TrackerNodePrivate::create(this) )
{
    _imp->ui.reset(new TrackerNodeInteract(_imp.get()));
}

TrackerNode::~TrackerNode()
{
    if (_imp->tracker) {
        _imp->tracker->quitTrackerThread_blocking(false);
    }
}


void
TrackerNode::initializeOverlayInteract()
{
    registerOverlay(eOverlayViewportTypeViewer, _imp->ui, std::map<std::string, std::string>());
}


void
TrackerNode::setupInitialSubGraphState()
{
    QString fixedNamePrefix = QString::fromUtf8( getScriptName_mt_safe().c_str() );

    fixedNamePrefix.append( QLatin1Char('_') );
    NodePtr input, output;

    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_OUTPUT, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);

        output = getApp()->createNode(args);
        if (!output) {
            throw std::runtime_error(tr("The Tracker node requires the plug-in %1 to be installed.").arg(QLatin1String(PLUGINID_NATRON_OUTPUT)).toStdString());
        }
    }
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "Source");
        input = getApp()->createNode(args);
        if (!input) {
            throw std::runtime_error(tr("The Tracker node requires the plug-in %1 to be installed.").arg(QLatin1String(PLUGINID_NATRON_INPUT)).toStdString());
        }

    }
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    {
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_NATRON_INPUT, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, "Mask");
        args->addParamDefaultValue<bool>(kNatronGroupInputIsOptionalParamName, true);
        args->addParamDefaultValue<bool>(kNatronGroupInputIsMaskParamName, true);
        _imp->maskNode = getApp()->createNode(args);
        if (!_imp->maskNode.lock()) {
            throw std::runtime_error(tr("The Tracker node requires the plug-in %1 to be installed.").arg(QLatin1String(PLUGINID_NATRON_INPUT)).toStdString());
        }

    }
#endif
    {
        QString cornerPinName = fixedNamePrefix + QLatin1String("CornerPin");
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_OFX_CORNERPIN, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, cornerPinName.toStdString());
        NodePtr cpNode = getApp()->createNode(args);
        if (!cpNode) {
            throw std::runtime_error(tr("The Tracker node requires the plug-in %1 to be installed.").arg(QLatin1String(PLUGINID_OFX_CORNERPIN)).toStdString());
        }
        cpNode->getEffectInstance()->setNodeDisabled(true);
        _imp->cornerPinNode = cpNode;
    }

    {
        QString transformName = fixedNamePrefix + QLatin1String("Transform");
        CreateNodeArgsPtr args(CreateNodeArgs::create(PLUGINID_OFX_TRANSFORM, thisShared));
        args->setProperty<bool>(kCreateNodeArgsPropVolatile, true);
        args->setProperty<bool>(kCreateNodeArgsPropNoNodeGUI, true);
        args->setProperty<std::string>(kCreateNodeArgsPropNodeInitialName, transformName.toStdString());
        NodePtr tNode = getApp()->createNode(args);
        if (!tNode) {
            throw std::runtime_error(tr("The Tracker node requires the plug-in %1 to be installed.").arg(QLatin1String(PLUGINID_OFX_TRANSFORM)).toStdString());
        }
        tNode->getEffectInstance()->setNodeDisabled(true);
        _imp->transformNode = tNode;

        output->connectInput(tNode, 0);
        NodePtr cpNode = _imp->cornerPinNode.lock();
        tNode->connectInput(cpNode, 0);
        cpNode->connectInput(input, 0);
    }

    // Initialize transform nodes now because they need to link to some of the Transform/CornerPin node
    // knobs.
    initializeTransformPageKnobs(_imp->transformPageKnob.lock());
} // setupInitialSubGraphState

void
TrackerNode::initializeViewerUIKnobs(const KnobPagePtr& trackingPage)
{

    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamAddTrack);

        param->setLabel(tr(kTrackerUIParamAddTrackLabel));
        param->setHintToolTip( tr(kTrackerUIParamAddTrackHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setSecret(true);
        param->setIconLabel("Images/addTrack.png");
        addOverlaySlaveParam(param);
        trackingPage->addKnob(param);
        _imp->ui->addTrackButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackBW);
        param->setLabel(tr(kTrackerUIParamTrackBWLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackBWHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackBackwardOn.png", true);
        param->setIconLabel("Images/trackBackwardOff.png", false);
        trackingPage->addKnob(param);
        _imp->ui->trackBwButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackPrevious);
        param->setLabel(tr(kTrackerUIParamTrackPreviousLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackPreviousHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackPrev.png");
        trackingPage->addKnob(param);
        _imp->ui->trackPrevButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamStopTracking );
        param->setLabel(tr(kTrackerUIParamStopTrackingLabel));
        param->setHintToolTip( tr(kTrackerUIParamStopTrackingHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/pauseDisabled.png");
        trackingPage->addKnob(param);
        _imp->ui->stopTrackingButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackNext);
        param->setLabel(tr(kTrackerUIParamTrackNextLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackNextHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackNext.png");
        trackingPage->addKnob(param);
        _imp->ui->trackNextButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackFW);
        param->setLabel(tr(kTrackerUIParamTrackFWLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackFWHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackForwardOn.png", true);
        param->setIconLabel("Images/trackForwardOff.png", false);
        trackingPage->addKnob(param);
        _imp->ui->trackFwButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackRange);
        param->setLabel(tr(kTrackerUIParamTrackRangeLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackRangeHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackRange.png");
        trackingPage->addKnob(param);
        _imp->ui->trackRangeButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackAllKeyframes);
        param->setLabel(tr(kTrackerUIParamTrackAllKeyframesLabel) );
        param->setHintToolTip( tr(kTrackerUIParamTrackAllKeyframesHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackAllKeyframes.png");
        trackingPage->addKnob(param);
        _imp->ui->trackAllKeyframesButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackCurrentKeyframe);
        param->setLabel(tr(kTrackerUIParamTrackCurrentKeyframeLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackCurrentKeyframeHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/trackCurrentKeyframe.png");
        trackingPage->addKnob(param);
        _imp->ui->trackCurrentKeyframeButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamSetPatternKeyFrame);
        param->setLabel(tr(kTrackerUIParamSetPatternKeyFrameLabel));
        param->setHintToolTip( tr(kTrackerUIParamSetPatternKeyFrameHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/addUserKey.png");
        trackingPage->addKnob(param);
        _imp->ui->setKeyFrameButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamRemovePatternKeyFrame);
        param->setLabel(tr(kTrackerUIParamRemovePatternKeyFrameLabel) );
        param->setHintToolTip( tr(kTrackerUIParamRemovePatternKeyFrameHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/removeUserKey.png");
        trackingPage->addKnob(param);
        _imp->ui->removeKeyFrameButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamClearAllAnimation);
        param->setLabel(tr(kTrackerUIParamClearAllAnimationLabel) );
        param->setHintToolTip( tr(kTrackerUIParamClearAllAnimationHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/clearAnimation.png");
        trackingPage->addKnob(param);
        _imp->ui->clearAllAnimationButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamClearAnimationBw );
        param->setLabel(tr(kTrackerUIParamClearAnimationBwLabel));
        param->setHintToolTip( tr(kTrackerUIParamClearAnimationBwHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/clearAnimationBw.png");
        trackingPage->addKnob(param);
        _imp->ui->clearBwAnimationButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamClearAnimationFw);
        param->setLabel(tr(kTrackerUIParamClearAnimationFwLabel) );
        param->setHintToolTip( tr(kTrackerUIParamClearAnimationFwHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/clearAnimationFw.png");
        trackingPage->addKnob(param);
        _imp->ui->clearFwAnimationButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamRefreshViewer);
        param->setLabel(tr(kTrackerUIParamRefreshViewerLabel) );
        param->setHintToolTip( tr(kTrackerUIParamRefreshViewerHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(true);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/refreshActive.png", true);
        param->setIconLabel("Images/refresh.png", false);
        trackingPage->addKnob(param);
        _imp->ui->updateViewerButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamCenterViewer);
        param->setLabel(tr(kTrackerUIParamCenterViewerLabel) );
        param->setHintToolTip( tr(kTrackerUIParamCenterViewerHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/centerOnTrack.png");
        trackingPage->addKnob(param);
        _imp->ui->centerViewerButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamCreateKeyOnMove );
        param->setLabel(tr(kTrackerUIParamCreateKeyOnMoveLabel));
        param->setHintToolTip( tr(kTrackerUIParamCreateKeyOnMoveHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(true);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        param->setIconLabel("Images/createKeyOnMoveOn.png", true);
        param->setIconLabel("Images/createKeyOnMoveOff.png", false);
        trackingPage->addKnob(param);
        _imp->ui->createKeyOnMoveButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamShowError );
        param->setLabel(tr(kTrackerUIParamShowErrorLabel));
        param->setHintToolTip( tr(kTrackerUIParamShowErrorHint) );
        param->setEvaluateOnChange(false);
        param->setCheckable(true);
        param->setDefaultValue(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(param);
        param->setIconLabel("Images/showTrackError.png", true);
        param->setIconLabel("Images/hideTrackError.png", false);
        trackingPage->addKnob(param);
        _imp->ui->showCorrelationButton = param;
    }


    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamResetOffset);
        param->setLabel(tr(kTrackerUIParamResetOffsetLabel) );
        param->setHintToolTip( tr(kTrackerUIParamResetOffsetHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(param);
        param->setIconLabel("Images/resetTrackOffset.png");
        trackingPage->addKnob(param);
        _imp->ui->resetOffsetButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamResetTrack);
        param->setLabel(tr(kTrackerUIParamResetTrackLabel) );
        param->setHintToolTip( tr(kTrackerUIParamResetTrackHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(param);
        param->setIconLabel("Images/restoreDefaultEnabled.png");
        trackingPage->addKnob(param);
        _imp->ui->resetTrackButton = param;
    }

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerUIParamMagWindowSize);
        param->setInViewerContextLabel(tr(kTrackerUIParamMagWindowSizeLabel));
        param->setLabel(tr(kTrackerUIParamMagWindowSizeLabel) );
        param->setHintToolTip( tr(kTrackerUIParamMagWindowSizeHint) );
        param->setEvaluateOnChange(false);
        param->setDefaultValue(200);
        param->setRange(10, 10000);
        param->disableSlider();
        addOverlaySlaveParam(param);
        trackingPage->addKnob(param);
        _imp->ui->magWindowPxSizeKnob = param;
    }

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerUIParamDefaultMarkerPatternWinSize);
        param->setLabel(tr(kTrackerUIParamDefaultMarkerPatternWinSizeLabel));
        param->setInViewerContextLabel(tr(kTrackerUIParamDefaultMarkerPatternWinSizeLabel));
        param->setHintToolTip( tr(kTrackerUIParamDefaultMarkerPatternWinSizeHint) );
        param->setInViewerContextIconFilePath("Images/patternSize.png");
        param->setAnimationEnabled(false);
        param->setRange(1, INT_MAX);
        param->disableSlider();
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(21);
        param->setSecret(true);
        trackingPage->addKnob(param);
        _imp->ui->defaultPatternWinSize = param;
    }

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerUIParamDefaultMarkerSearchWinSize);
        param->setLabel(tr(kTrackerUIParamDefaultMarkerSearchWinSizeLabel));
        param->setInViewerContextLabel(tr(kTrackerUIParamDefaultMarkerSearchWinSizeLabel));
        param->setHintToolTip( tr(kTrackerUIParamDefaultMarkerSearchWinSizeHint) );
        param->setInViewerContextIconFilePath("Images/searchSize.png");
        param->setAnimationEnabled(false);
        param->setRange(1, INT_MAX);
        param->disableSlider();
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setDefaultValue(71);
        trackingPage->addKnob(param);
        _imp->ui->defaultSearchWinSize = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kTrackerUIParamDefaultMotionModel);
        param->setLabel(QString());
        param->setAnimationEnabled(false);
        {
            std::vector<ChoiceOption> choices;
            std::map<int, std::string> icons;
            TrackerNodePrivate::getMotionModelsAndHelps(true, &choices, &icons);
            param->populateChoices(choices);
            param->setIcons(icons);
        }
        param->setSecret(true);
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->ui->defaultMotionModel = param;
    }

    addKnobToViewerUI(_imp->ui->addTrackButton.lock());
    _imp->ui->addTrackButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->trackBwButton.lock());
    _imp->ui->trackBwButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->trackPrevButton.lock());
    _imp->ui->trackPrevButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->trackNextButton.lock());
    _imp->ui->trackNextButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->trackFwButton.lock());
    _imp->ui->trackFwButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->trackRangeButton.lock());
    _imp->ui->trackRangeButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->trackAllKeyframesButton.lock());
    _imp->ui->trackAllKeyframesButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->trackCurrentKeyframeButton.lock());
    _imp->ui->trackCurrentKeyframeButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->setKeyFrameButton.lock());
    _imp->ui->setKeyFrameButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->removeKeyFrameButton.lock());
    _imp->ui->removeKeyFrameButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->clearAllAnimationButton.lock());
    _imp->ui->clearAllAnimationButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->clearBwAnimationButton.lock());
    _imp->ui->clearBwAnimationButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->clearFwAnimationButton.lock());
    _imp->ui->clearFwAnimationButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->updateViewerButton.lock());
    _imp->ui->updateViewerButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->centerViewerButton.lock());
    _imp->ui->centerViewerButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->createKeyOnMoveButton.lock());
    addKnobToViewerUI(_imp->ui->showCorrelationButton.lock());
    _imp->ui->showCorrelationButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->resetOffsetButton.lock());
    _imp->ui->resetOffsetButton.lock()->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(_imp->ui->resetTrackButton.lock());
    _imp->ui->resetTrackButton.lock()->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(_imp->ui->defaultPatternWinSize.lock());
    addKnobToViewerUI(_imp->ui->defaultSearchWinSize.lock());
    addKnobToViewerUI(_imp->ui->defaultMotionModel.lock());
    _imp->ui->defaultMotionModel.lock()->setInViewerContextLayoutType(eViewerContextLayoutTypeStretchAfter);


} // initializeViewerUIKnobs

void
TrackerNode::initializeTrackRangeDialogKnobs(const KnobPagePtr& trackingPage)
{
    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());


    // Track range dialog
    KnobGroupPtr dialog = createKnob<KnobGroup>(kTrackerUIParamTrackRangeDialog);
    dialog->setLabel(tr(kTrackerUIParamTrackRangeDialogLabel));
    dialog->setSecret(true);
    dialog->setEvaluateOnChange(false);
    dialog->setDefaultValue(false);
    dialog->setIsPersistent(false);
    dialog->setAsDialog(true);
    trackingPage->addKnob(dialog);
    _imp->ui->trackRangeDialogGroup = dialog;

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerUIParamTrackRangeDialogFirstFrame );
        param->setLabel(tr(kTrackerUIParamTrackRangeDialogFirstFrameLabel));
        param->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogFirstFrameHint) );
        param->setEvaluateOnChange(false);
        param->setAnimationEnabled(false);
        param->setIsPersistent(false);
        param->setDefaultValue(INT_MIN);
        dialog->addKnob(param);
        _imp->ui->trackRangeDialogFirstFrame = param;
    }

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerUIParamTrackRangeDialogLastFrame);
        param->setLabel(tr(kTrackerUIParamTrackRangeDialogLastFrameLabel) );
        param->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogLastFrameHint) );
        param->setEvaluateOnChange(false);
        param->setAnimationEnabled(false);
        param->setIsPersistent(false);
        param->setDefaultValue(INT_MIN);
        dialog->addKnob(param);
        _imp->ui->trackRangeDialogLastFrame = param;
    }

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerUIParamTrackRangeDialogStep);
        param->setLabel(tr(kTrackerUIParamTrackRangeDialogStepLabel) );
        param->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogStepHint) );
        param->setEvaluateOnChange(false);
        param->setAnimationEnabled(false);
        param->setIsPersistent(false);
        param->setDefaultValue(INT_MIN);
        dialog->addKnob(param);
        _imp->ui->trackRangeDialogStep = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackRangeDialogOkButton);
        param->setLabel(tr(kTrackerUIParamTrackRangeDialogOkButtonLabel) );
        param->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogOkButtonHint) );
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        param->setSpacingBetweenItems(3);
        param->setIsPersistent(false);
        dialog->addKnob(param);
        _imp->ui->trackRangeDialogOkButton = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerUIParamTrackRangeDialogCancelButton);
        param->setLabel(tr(kTrackerUIParamTrackRangeDialogCancelButtonLabel) );
        param->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogCancelButtonHint) );
        param->setEvaluateOnChange(false);
        dialog->addKnob(param);
        _imp->ui->trackRangeDialogCancelButton = param;
    }
} // initializeTrackRangeDialogKnobs

void
TrackerNode::initializeRightClickMenuKnobs(const KnobPagePtr& trackingPage)
{
    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());
    // Right click menu
    KnobChoicePtr rightClickMenu = createKnob<KnobChoice>(std::string(kTrackerUIParamRightClickMenu) );
    rightClickMenu->setSecret(true);
    rightClickMenu->setEvaluateOnChange(false);
    trackingPage->addKnob(rightClickMenu);
    _imp->ui->rightClickMenuKnob = rightClickMenu;

    {
        KnobButtonPtr action = createKnob<KnobButton>(kTrackerUIParamRightClickMenuActionSelectAllTracks);
        action->setLabel(tr(kTrackerUIParamRightClickMenuActionSelectAllTracksLabel) );
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->selectAllTracksMenuAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kTrackerUIParamRightClickMenuActionRemoveTracks );
        action->setLabel(tr(kTrackerUIParamRightClickMenuActionRemoveTracksLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->removeTracksMenuAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kTrackerUIParamRightClickMenuActionNudgeBottom);
        action->setLabel(tr(kTrackerUIParamRightClickMenuActionNudgeBottomLabel) );
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnBottomMenuAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kTrackerUIParamRightClickMenuActionNudgeLeft );
        action->setLabel(tr(kTrackerUIParamRightClickMenuActionNudgeLeftLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnLeftMenuAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kTrackerUIParamRightClickMenuActionNudgeRight );
        action->setLabel(tr(kTrackerUIParamRightClickMenuActionNudgeRightLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnRightMenuAction = action;
    }
    {
        KnobButtonPtr action = createKnob<KnobButton>(kTrackerUIParamRightClickMenuActionNudgeTop );
        action->setLabel(tr(kTrackerUIParamRightClickMenuActionNudgeTopLabel));
        action->setSecret(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnTopMenuAction = action;
    }

} // initializeRightClickMenuKnobs


/**
 * @brief Creates a duplicate of the knob identified by knobName which is a knob in the internalNode onto the effect and add it to the given page.
 * If otherNode is set, also fetch a knob of the given name on the otherNode and link it to the newly created knob.
 **/

template <typename KNOBTYPE>
boost::shared_ptr<KNOBTYPE>
createDuplicateKnob( const std::string& knobName,
                    const NodePtr& internalNode,
                    const EffectInstancePtr& effect,
                    const KnobPagePtr& page = KnobPagePtr(),
                    const KnobGroupPtr& group = KnobGroupPtr(),
                    const NodePtr& otherNode = NodePtr() )
{
    KnobIPtr internalNodeKnob = internalNode->getKnobByName(knobName);

    if (!internalNodeKnob) {
        return boost::shared_ptr<KNOBTYPE>();
    }
    assert(internalNodeKnob);
    KnobIPtr duplicateKnob = internalNodeKnob->createDuplicateOnHolder(effect, page, group, -1, KnobI::eDuplicateKnobTypeAlias, internalNodeKnob->getName(), internalNodeKnob->getLabel(), internalNodeKnob->getHintToolTip(), false /*refreshParamsGui*/, KnobI::eKnobDeclarationTypePlugin);

    if (otherNode) {
        KnobIPtr otherNodeKnob = otherNode->getKnobByName(knobName);
        assert(otherNodeKnob);
        otherNodeKnob->linkTo(duplicateKnob);
    }

    return boost::dynamic_pointer_cast<KNOBTYPE>(duplicateKnob);
} // createDuplicateKnob

void
TrackerNode::initializeTrackingPageKnobs(const KnobPagePtr& trackingPage)
{

    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());

#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamUsePatternMatching);
        param->setLabel(tr(kTrackerParamUsePatternMatchingLabel));
        param->setHintToolTip( tr(kTrackerParamUsePatternMatchingHint) );
        param->setDefaultValue(false);
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->usePatternMatching = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kTrackerParamPatternMatchingScoreType);
        param->setLabel(tr(kTrackerParamPatternMatchingScoreTypeLabel));
        param->setHintToolTip( tr(kTrackerParamPatternMatchingScoreTypeHint) );
        {
            std::vector<ChoiceOption> choices;
            choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionSSD, "", tr(kTrackerParamPatternMatchingScoreOptionSSDHint).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionSAD, "", tr(kTrackerParamPatternMatchingScoreOptionSADHint).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionNCC, "", tr(kTrackerParamPatternMatchingScoreOptionNCCHint).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamPatternMatchingScoreOptionZNCC, "", tr(kTrackerParamPatternMatchingScoreOptionZNCCHint).toStdString()));
            param->populateChoices(choices);
        }
        param->setDefaultValue(1); // SAD
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->patternMatchingScore = param;
    }
#endif // NATRON_TRACKER_ENABLE_TRACKER_PM

    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamTrackRed);
        param->setLabel(tr(kTrackerParamTrackRedLabel));
        param->setHintToolTip( tr(kTrackerParamTrackRedHint) );
        param->setDefaultValue(true);
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->enableTrackRed = param;
    }

    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamTrackGreen);
        param->setLabel(tr(kTrackerParamTrackGreenLabel));
        param->setHintToolTip( tr(kTrackerParamTrackGreenHint) );
        param->setDefaultValue(true);
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->enableTrackGreen = param;
    }

    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamTrackBlue);
        param->setLabel(tr(kTrackerParamTrackBlueLabel));
        param->setHintToolTip( tr(kTrackerParamTrackBlueHint) );
        param->setDefaultValue(true);
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->enableTrackBlue = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>(kTrackerParamMaxError);
        param->setLabel(tr(kTrackerParamMaxErrorLabel));
        param->setHintToolTip( tr(kTrackerParamMaxErrorHint) );
        param->setAnimationEnabled(false);
        param->setRange(0., 1.);
        param->setDefaultValue(0.25);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->maxError = param;
    }
    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerParamMaximumIteration);
        param->setLabel(tr(kTrackerParamMaximumIterationLabel));
        param->setHintToolTip( tr(kTrackerParamMaximumIterationHint) );
        param->setAnimationEnabled(false);
        param->setRange(0, 150);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(50);
        trackingPage->addKnob(param);
        _imp->maxIterations = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamBruteForcePreTrack);
        param->setLabel(tr(kTrackerParamBruteForcePreTrackLabel));
        param->setHintToolTip( tr(kTrackerParamBruteForcePreTrackHint) );
        param->setDefaultValue(true);
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        param->setAddNewLine(false);
        trackingPage->addKnob(param);
        _imp->bruteForcePreTrack = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamNormalizeIntensities);
        param->setLabel(tr(kTrackerParamNormalizeIntensitiesLabel));
        param->setHintToolTip( tr(kTrackerParamNormalizeIntensitiesHint) );
        param->setDefaultValue(false);
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->useNormalizedIntensities = param;
    }
    {
        KnobDoublePtr param = createKnob<KnobDouble>(kTrackerParamPreBlurSigma);
        param->setLabel(tr(kTrackerParamPreBlurSigmaLabel));
        param->setHintToolTip( tr(kTrackerParamPreBlurSigmaHint) );
        param->setAnimationEnabled(false);
        param->setRange(0, 10.);
        param->setDefaultValue(0.9);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->preBlurSigma = param;
    }

    {
        KnobSeparatorPtr  param = createKnob<KnobSeparator>(kTrackerParamPerTrackParamsSeparator, 3);
        param->setLabel(tr(kTrackerParamPerTrackParamsSeparatorLabel));
        trackingPage->addKnob(param);
        _imp->perTrackParamsSeparator = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamEnabled);
        param->setLabel(tr(kTrackerParamEnabledLabel));
        param->setHintToolTip( tr(kTrackerParamEnabledHint) );
        param->setAnimationEnabled(true);
        param->setDefaultValue(true);
        param->setEvaluateOnChange(false);
        param->setEnabled(false);
        param->setAddNewLine(false);
        trackingPage->addKnob(param);
        _imp->activateTrack = param;
        _imp->knobsTable->addPerItemKnobMaster(param);
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamAutoKeyEnabled);
        param->setLabel(tr(kTrackerParamAutoKeyEnabledLabel));
        param->setHintToolTip( tr(kTrackerParamAutoKeyEnabledHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(false);
        param->setEvaluateOnChange(false);
        trackingPage->addKnob(param);
        _imp->autoKeyEnabled = param;
    }


    {
        KnobChoicePtr param = createKnob<KnobChoice>(kTrackerParamMotionModel);
        param->setLabel(tr(kTrackerParamMotionModelLabel));
        param->setHintToolTip( tr(kTrackerParamMotionModelHint) );
        {
            std::vector<ChoiceOption> choices;
            std::map<int, std::string> icons;
            TrackerNodePrivate::getMotionModelsAndHelps(true, &choices, &icons);
            param->populateChoices(choices);
            param->setIcons(icons);
        }
        param->setEnabled(false);
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        _imp->knobsTable->addPerItemKnobMaster(param);
        _imp->motionModel = param;
        trackingPage->addKnob(param);
    }

    {
        KnobKeyFrameMarkersPtr param = createKnob<KnobKeyFrameMarkers>(kTrackerParamManualKeyframes);
        param->setLabel(tr(kTrackerParamManualKeyframesLabel));
        param->setHintToolTip(tr(kTrackerParamManualKeyframesHint));
        trackingPage->addKnob(param);
        _imp->trackKeyframesKnob = param;
        _imp->knobsTable->addPerItemKnobMaster(param);
    }

    // Add a separator before the table
    {
        KnobSeparatorPtr param = createKnob<KnobSeparator>("trackTableSep");
        param->setLabel(QString());
        trackingPage->addKnob(param);
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerAddTrackParam);
        param->setLabel(tr(kTrackerAddTrackParamLabel));
        param->setHintToolTip( tr(kTrackerAddTrackParamHint) );
        param->setEvaluateOnChange(false);
        param->setIconLabel("Images/addTrack.png");
        param->setAddNewLine(false);
        param->setSpacingBetweenItems(5);
        _imp->knobsTable->addTableControlKnob(param);
        _imp->addTrackFromPanelButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerRemoveTracksParam);
        param->setLabel(tr(kTrackerRemoveTracksParamLabel));
        param->setHintToolTip( tr(kTrackerRemoveTracksParamHint) );
        param->setEvaluateOnChange(false);
        param->setIconLabel("Images/rotoPaintRemoveItemIcon.png");
        param->setAddNewLine(false);
        param->setSpacingBetweenItems(5);
        _imp->knobsTable->addTableControlKnob(param);
        _imp->removeSelectedTracksButton = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerAverageTracksParam);
        param->setLabel(tr(kTrackerAverageTracksParamLabel));
        param->setHintToolTip( tr(kTrackerAverageTracksParamHint) );
        param->setEvaluateOnChange(false);
        _imp->knobsTable->addTableControlKnob(param);
        _imp->averageTracksButton = param;

    }

} // initializeTrackingPageKnobs

void
TrackerNode::initializeTransformPageKnobs(const KnobPagePtr& transformPage)
{
    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());

    {
        KnobSeparatorPtr param = createKnob<KnobSeparator>("transformSep");
        param->setLabel(tr("Transform Generation"));
        transformPage->addKnob(param);
        _imp->transformGenerationSeparator = param;
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kTrackerParamMotionType);
        param->setLabel(tr(kTrackerParamMotionTypeLabel));
        param->setHintToolTip( tr(kTrackerParamMotionTypeHint) );
        {
            std::vector<ChoiceOption> choices;
            choices.push_back(ChoiceOption(kTrackerParamMotionTypeNone, "", tr(kTrackerParamMotionTypeNoneHelp).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamMotionTypeStabilize, "", tr(kTrackerParamMotionTypeStabilizeHelp).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamMotionTypeMatchMove, "", tr(kTrackerParamMotionTypeMatchMoveHelp).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamMotionTypeRemoveJitter, "", tr(kTrackerParamMotionTypeRemoveJitterHelp).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamMotionTypeAddJitter, "", tr(kTrackerParamMotionTypeAddJitterHelp).toStdString()));

            param->populateChoices(choices);
        }
        param->setAddNewLine(false);
        _imp->motionType = param;
        transformPage->addKnob(param);
    }

    {
        KnobChoicePtr param = createKnob<KnobChoice>(kTrackerParamTransformType);
        param->setLabel(tr(kTrackerParamTransformTypeLabel));
        param->setHintToolTip( tr(kTrackerParamTransformTypeHint) );
        {
            std::vector<ChoiceOption> choices;
            choices.push_back(ChoiceOption(kTrackerParamTransformTypeTransform, "", tr(kTrackerParamTransformTypeTransformHelp).toStdString()));
            choices.push_back(ChoiceOption(kTrackerParamTransformTypeCornerPin, "", tr(kTrackerParamTransformTypeCornerPinHelp).toStdString()));
            param->populateChoices(choices);
        }
        param->setDefaultValue(1);
        _imp->transformType = param;
        transformPage->addKnob(param);
    }

    {
        KnobIntPtr param = createKnob<KnobInt>(kTrackerParamReferenceFrame);
        param->setLabel(tr(kTrackerParamReferenceFrameLabel));
        param->setHintToolTip( tr(kTrackerParamReferenceFrameHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(1);
        param->setAddNewLine(false);
        param->setEvaluateOnChange(false);
        transformPage->addKnob(param);
        _imp->referenceFrame = param;
    }

    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerParamSetReferenceFrame);
        param->setLabel(tr(kTrackerParamSetReferenceFrameLabel));
        param->setHintToolTip( tr(kTrackerParamSetReferenceFrameHint) );
        transformPage->addKnob(param);
        _imp->setCurrentFrameButton = param;
    }
    {
        KnobIntPtr  param = createKnob<KnobInt>(kTrackerParamJitterPeriod);
        param->setLabel(tr(kTrackerParamJitterPeriodLabel));
        param->setHintToolTip( tr(kTrackerParamJitterPeriodHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue(10);
        param->setRange(0, INT_MAX);
        param->setEvaluateOnChange(false);
        transformPage->addKnob(param);
        _imp->jitterPeriod = param;
    }
    {
        KnobIntPtr  param = createKnob<KnobInt>(kTrackerParamSmooth, 3);
        param->setLabel(tr(kTrackerParamSmoothLabel));
        param->setHintToolTip( tr(kTrackerParamSmoothHint) );
        param->setAnimationEnabled(false);
        param->disableSlider();
        param->setDimensionName(DimIdx(0), "t");
        param->setDimensionName(DimIdx(1), "r");
        param->setDimensionName(DimIdx(2), "s");
        for (int i = 0; i < 3; ++i) {
            param->setRange(0, INT_MAX, DimIdx(i));
        }
        param->setEvaluateOnChange(false);
        transformPage->addKnob(param);
        _imp->smoothTransform = param;
    }
    {
        KnobIntPtr  param = createKnob<KnobInt>(kTrackerParamSmoothCornerPin);
        param->setLabel(tr(kTrackerParamSmoothCornerPinLabel));
        param->setHintToolTip( tr(kTrackerParamSmoothCornerPinHint) );
        param->setAnimationEnabled(false);
        param->disableSlider();
        param->setRange(0, INT_MAX);
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        transformPage->addKnob(param);
        _imp->smoothCornerPin = param;
    }
    {
        KnobBoolPtr  param = createKnob<KnobBool>(kTrackerParamAutoGenerateTransform);
        param->setLabel(tr(kTrackerParamAutoGenerateTransformLabel));
        param->setHintToolTip( tr(kTrackerParamAutoGenerateTransformHint) );
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(true);
        param->setAddNewLine(false);
        transformPage->addKnob(param);
        _imp->autoGenerateTransform = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerParamGenerateTransform);
        param->setLabel(tr(kTrackerParamGenerateTransformLabel));
        param->setHintToolTip( tr(kTrackerParamGenerateTransformHint) );
        param->setEvaluateOnChange(false);
        transformPage->addKnob(param);
        _imp->generateTransformButton = param;
    }
    {
        KnobBoolPtr  param = createKnob<KnobBool>(kTrackerParamRobustModel);
        param->setLabel(tr(kTrackerParamRobustModelLabel));
        param->setHintToolTip( tr(kTrackerParamRobustModelHint) );
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(true);
        param->setAddNewLine(false);
        transformPage->addKnob(param);
        _imp->robustModel = param;
    }
    {
        KnobStringPtr  param = createKnob<KnobString>(kTrackerParamFittingErrorWarning);
        param->setLabel(tr(kTrackerParamFittingErrorWarningLabel));
        param->setHintToolTip( tr(kTrackerParamFittingErrorWarningHint) );
        param->setAnimationEnabled(false);
        param->setDefaultValue( tr(kTrackerParamFittingErrorWarningLabel).toStdString() );
        param->setIconLabel("dialog-warning");
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        param->setAsLabel();
        transformPage->addKnob(param);
        _imp->fittingErrorWarning = param;
    }
    {
        KnobDoublePtr  param = createKnob<KnobDouble>(kTrackerParamFittingError);
        param->setLabel(tr(kTrackerParamFittingErrorLabel));
        param->setHintToolTip( tr(kTrackerParamFittingErrorHint) );
        param->setEvaluateOnChange(false);
        param->setAddNewLine(false);
        param->setEnabled(false);
        transformPage->addKnob(param);
        _imp->fittingError = param;
    }
    {
        KnobDoublePtr  param = createKnob<KnobDouble>(kTrackerParamFittingErrorWarnValue);
        param->setLabel(tr(kTrackerParamFittingErrorWarnValueLabel));
        param->setHintToolTip( tr(kTrackerParamFittingErrorWarnValueHint) );
        param->setAnimationEnabled(false);
        param->setEvaluateOnChange(false);
        param->setDefaultValue(1);
        transformPage->addKnob(param);
        _imp->fittingErrorWarnIfAbove = param;
    }
    {
        KnobStringPtr param = createKnob<KnobString>(kTrackerParamTransformOutOfDate);
        param->setLabel(QString());
        param->setHintToolTip( tr(kTrackerParamTransformOutOfDateHint) );
        param->setIconLabel("dialog-warning");
        param->setAsLabel();
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        transformPage->addKnob(param);
        _imp->transformOutOfDateLabel = param;
    }
    {
        KnobSeparatorPtr  param = createKnob<KnobSeparator>("transformControlsSep");
        param->setLabel(tr("Transform Controls"));
        transformPage->addKnob(param);
        param->setSecret(true);
        _imp->transformControlsSeparator = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamDisableTransform);
        param->setLabel(tr(kTrackerParamDisableTransformLabel));
        param->setHintToolTip( tr(kTrackerParamDisableTransformHint) );
        param->setEvaluateOnChange(false);
        param->setSecret(true);
        transformPage->addKnob(param);
        _imp->disableTransform = param;
    }


    NodePtr tNode = _imp->transformNode.lock();
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamTranslate, tNode, thisShared, transformPage);
        param->setSecret(true);
        _imp->translate = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamRotate, tNode,thisShared, transformPage);
        param->setSecret(true);
        _imp->rotate = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamScale, tNode,thisShared, transformPage);
        param->setAddNewLine(false);
        param->setSecret(true);
        _imp->scale = param;
    }
    {
        KnobBoolPtr param = createDuplicateKnob<KnobBool>(kTransformParamUniform, tNode,thisShared, transformPage);
        param->setSecret(true);
        _imp->scaleUniform = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamSkewX, tNode,thisShared, transformPage);
        param->setSecret(true);
        _imp->skewX = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamSkewY, tNode,thisShared, transformPage);
        param->setSecret(true);
        _imp->skewY = param;
    }
    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kTransformParamSkewOrder, tNode,thisShared, transformPage);
        param->setSecret(true);
        _imp->skewOrder = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamCenter, tNode,thisShared, transformPage);
        param->setSecret(true);
        _imp->center = param;
    }
    NodePtr cNode = _imp->cornerPinNode.lock();
    KnobGroupPtr  toGroupKnob = createKnob<KnobGroup>(kCornerPinParamTo);
    toGroupKnob->setLabel(tr(kCornerPinParamTo));
    toGroupKnob->setAsTab();
    toGroupKnob->setSecret(true);
    transformPage->addKnob(toGroupKnob);
    _imp->toGroup = toGroupKnob;

    KnobGroupPtr  fromGroupKnob = createKnob<KnobGroup>(kCornerPinParamFrom);
    fromGroupKnob->setLabel(tr(kCornerPinParamFrom));
    fromGroupKnob->setAsTab();
    fromGroupKnob->setSecret(true);
    transformPage->addKnob(fromGroupKnob);
    _imp->fromGroup = fromGroupKnob;

    const char* fromPointNames[4] = {kCornerPinParamFrom1, kCornerPinParamFrom2, kCornerPinParamFrom3, kCornerPinParamFrom4};
    const char* toPointNames[4] = {kCornerPinParamTo1, kCornerPinParamTo2, kCornerPinParamTo3, kCornerPinParamTo4};
    const char* enablePointNames[4] = {kCornerPinParamEnable1, kCornerPinParamEnable2, kCornerPinParamEnable3, kCornerPinParamEnable4};

    for (int i = 0; i < 4; ++i) {
        _imp->fromPoints[i] = createDuplicateKnob<KnobDouble>(fromPointNames[i], cNode, thisShared, transformPage, fromGroupKnob);

        _imp->toPoints[i] = createDuplicateKnob<KnobDouble>(toPointNames[i], cNode,thisShared, transformPage, toGroupKnob);
        _imp->toPoints[i].lock()->setAddNewLine(false);
        _imp->enableToPoint[i] = createDuplicateKnob<KnobBool>(enablePointNames[i], cNode,thisShared, transformPage, toGroupKnob);
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kCornerPinParamSetToInputRoD);
        param->setLabel(tr(kCornerPinParamSetToInputRoDLabel));
        param->setHintToolTip( tr(kCornerPinParamSetToInputRoDHint) );
        fromGroupKnob->addKnob(param);
        _imp->setFromPointsToInputRodButton = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(std::string(kTrackerParamCornerPinFromPointsSetOnce) );
        param->setSecret(true);
        fromGroupKnob->addKnob(param);
        _imp->cornerPinFromPointsSetOnceAutomatically = param;
    }
    _imp->cornerPinOverlayPoints = createDuplicateKnob<KnobChoice>(kCornerPinParamOverlayPoints, cNode, thisShared, transformPage);
    _imp->cornerPinOverlayPoints.lock()->setSecret(true);

    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kCornerPinParamMatrix, cNode, thisShared, transformPage);
        _imp->cornerPinMatrix = param;
        param->setSecret(true);

    }

    // Add filtering & motion blur knobs
    {
        KnobBoolPtr param = createDuplicateKnob<KnobBool>(kTransformParamInvert, tNode, thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        _imp->invertTransform = param;
    }
    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kTransformParamFilter, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        param->setAddNewLine(false);
        _imp->filter = param;
    }
    {
        KnobBoolPtr param = createDuplicateKnob<KnobBool>(kTransformParamClamp, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        param->setAddNewLine(false);
        _imp->clamp = param;
    }
    {
        KnobBoolPtr param = createDuplicateKnob<KnobBool>(kTransformParamBlackOutside, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        _imp->blackOutside = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamMotionBlur, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        _imp->motionBlur = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamShutter, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        _imp->shutter = param;
    }
    {
        KnobChoicePtr param = createDuplicateKnob<KnobChoice>(kTransformParamShutterOffset, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        _imp->shutterOffset = param;
    }
    {
        KnobDoublePtr param = createDuplicateKnob<KnobDouble>(kTransformParamCustomShutterOffset, tNode,thisShared, transformPage, KnobGroupPtr(), cNode);
        param->setSecret(true);
        _imp->customShutterOffset = param;
    }

    std::map<std::string, std::string> transformKnobs;
    transformKnobs["translate"] = kTransformParamTranslate;
    transformKnobs["scale"] = kTransformParamScale;
    transformKnobs["scaleUniform"] = kTransformParamUniform;
    transformKnobs["rotate"] = kTransformParamRotate;
    transformKnobs["center"] = kTransformParamCenter;
    transformKnobs["skewX"] = kTransformParamSkewX;
    transformKnobs["skewY"] = kTransformParamSkewY;
    transformKnobs["skewOrder"] = kTransformParamSkewOrder;
    transformKnobs["invert"] = kTransformParamInvert;

    boost::shared_ptr<TransformOverlayInteract> transformInteract(new TransformOverlayInteract());
    registerOverlay(eOverlayViewportTypeViewer, transformInteract, transformKnobs);


    std::map<std::string, std::string> cpKnobs;
    cpKnobs["from1"] = kCornerPinParamFrom1;
    cpKnobs["from2"] = kCornerPinParamFrom2;
    cpKnobs["from3"] = kCornerPinParamFrom3;
    cpKnobs["from4"] = kCornerPinParamFrom4;
    cpKnobs["to1"] = kCornerPinParamTo1;
    cpKnobs["to2"] = kCornerPinParamTo2;
    cpKnobs["to3"] = kCornerPinParamTo3;
    cpKnobs["to4"] = kCornerPinParamTo4;
    cpKnobs["enable1"] = kCornerPinParamEnable1;
    cpKnobs["enable2"] = kCornerPinParamEnable2;
    cpKnobs["enable3"] = kCornerPinParamEnable3;
    cpKnobs["enable4"] = kCornerPinParamEnable4;
    cpKnobs["overlayPoints"] = kCornerPinParamOverlayPoints;
    cpKnobs["invert"] = kCornerPinInvertParamName;

    boost::shared_ptr<CornerPinOverlayInteract> cornerPinInteract(new CornerPinOverlayInteract());
    registerOverlay(eOverlayViewportTypeViewer, cornerPinInteract, cpKnobs);

    {
        KnobSeparatorPtr param = createKnob<KnobSeparator>(kTrackerParamExportDataSeparator);
        param->setLabel(tr(kTrackerParamExportDataSeparatorLabel));
        transformPage->addKnob(param);
        _imp->exportDataSep = param;
    }
    {
        KnobBoolPtr param = createKnob<KnobBool>(kTrackerParamExportLink);
        param->setLabel(tr(kTrackerParamExportLinkLabel));
        param->setHintToolTip( tr(kTrackerParamExportLinkHint) );
        param->setAnimationEnabled(false);
        param->setAddNewLine(false);
        param->setDefaultValue(true);
        transformPage->addKnob(param);
        _imp->exportLink = param;
    }
    {
        KnobButtonPtr param = createKnob<KnobButton>(kTrackerParamExportButton);
        param->setLabel(tr(kTrackerParamExportButtonLabel));
        param->setHintToolTip( tr(kTrackerParamExportButtonHint) );
        param->setEnabled(false);
        transformPage->addKnob(param);
        _imp->exportButton = param;
    }
} // initializeTransformPageKnobs

void
TrackerNode::initializeKnobs()
{
    TrackerNodePtr thisShared = toTrackerNode(shared_from_this());

    _imp->knobsTable.reset(new TrackerKnobItemsTable(_imp.get(), KnobItemsTable::eKnobItemsTableTypeTable));
    QObject::connect(_imp->knobsTable.get(), SIGNAL(selectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)),
                     _imp->ui.get(), SLOT(onModelSelectionChanged(std::list<KnobTableItemPtr>,std::list<KnobTableItemPtr>,TableChangeReasonEnum)));


    _imp->knobsTable->setIconsPath(NATRON_IMAGES_PATH);
    _imp->knobsTable->setColumnText(0, tr("Label").toStdString());
    _imp->knobsTable->setColumnText(1, tr(kTrackerParamEnabledLabel).toStdString());
    _imp->knobsTable->setColumnText(2, tr(kTrackerParamMotionModelLabel).toStdString());
    _imp->knobsTable->setColumnText(3, tr("%1 X").arg(QString::fromUtf8(kTrackerParamCenterLabel)).toStdString());
    _imp->knobsTable->setColumnText(4, tr("%1 Y").arg(QString::fromUtf8(kTrackerParamCenterLabel)).toStdString());
    _imp->knobsTable->setColumnText(5, tr("%1 X").arg(QString::fromUtf8(kTrackerParamOffsetLabel)).toStdString());
    _imp->knobsTable->setColumnText(6, tr("%1 Y").arg(QString::fromUtf8(kTrackerParamOffsetLabel)).toStdString());
    _imp->knobsTable->setColumnText(7, tr(kTrackerParamErrorLabel).toStdString());

    _imp->knobsTable->setColumnIcon(2, "motionTypeAffine.png");

    _imp->tracker.reset(new TrackerHelper(_imp));

    KnobPagePtr trackingPage = createKnob<KnobPage>("trackingPage");
    trackingPage->setLabel(tr("Tracking"));
    _imp->trackingPageKnob = trackingPage;
    KnobPagePtr transformPage = createKnob<KnobPage>("transformPage");
    transformPage->setLabel(tr("Transform"));
    _imp->transformPageKnob = transformPage;

    initializeTrackingPageKnobs(trackingPage);
    initializeTrackRangeDialogKnobs(trackingPage);
    initializeRightClickMenuKnobs(trackingPage);
    initializeViewerUIKnobs(trackingPage);

    // Add the table at the bottom of the tracking page
    addItemsTable(_imp->knobsTable, KnobHolder::eKnobItemsTablePositionAfterKnob, trackingPage->getName());


    QObject::connect( getNode().get(), SIGNAL(s_refreshPreviewsAfterProjectLoadRequested()), _imp->ui.get(), SLOT(rebuildMarkerTextures()) );
    QObject::connect( _imp->tracker.get(), SIGNAL(trackingFinished()), _imp->ui.get(), SLOT(onTrackingEnded()) );
    QObject::connect( _imp->tracker.get(), SIGNAL(trackingStarted(int)), _imp->ui.get(), SLOT(onTrackingStarted(int)) );


} // TrackerNode::initializeKnobs

bool
TrackerNode::knobChanged(const KnobIPtr& k,
                         ValueChangedReasonEnum reason,
                         ViewSetSpec /*view*/,
                         TimeValue time)
{


    bool ret = true;
    if ( k == _imp->ui->trackRangeDialogOkButton.lock() ) {
        int first = _imp->ui->trackRangeDialogFirstFrame.lock()->getValue();
        int last = _imp->ui->trackRangeDialogLastFrame.lock()->getValue();
        int step = _imp->ui->trackRangeDialogStep.lock()->getValue();
        if ( _imp->tracker->isCurrentlyTracking() ) {
            _imp->tracker->abortTracking();
        }

        if (step == 0) {
            message( eMessageTypeError, tr("The Step cannot be 0").toStdString() );

            return false;
        }

        int startFrame = step > 0 ? first : last;
        int lastFrame = step > 0 ? last + 1 : first - 1;

        if ( ( (step > 0) && (startFrame >= lastFrame) ) || ( (step < 0) && (startFrame <= lastFrame) ) ) {
            return false;
        }

        OverlaySupport* overlay = _imp->ui->getLastCallingViewport();
        assert(overlay);
        _imp->trackSelectedMarkers( TimeValue(startFrame), TimeValue(lastFrame), TimeValue(step),  overlay);
        _imp->ui->trackRangeDialogGroup.lock()->setValue(false);
    } else if ( k == _imp->ui->trackRangeDialogCancelButton.lock() ) {
        _imp->ui->trackRangeDialogGroup.lock()->setValue(false);
    } else if ( k == _imp->ui->selectAllTracksMenuAction.lock() ) {
        _imp->knobsTable->selectAll(eTableChangeReasonInternal);
    } else if ( k == _imp->ui->removeTracksMenuAction.lock() ) {
        std::list<KnobTableItemPtr> items = _imp->knobsTable->getSelectedItems();
        if ( !items.empty() ) {
            pushUndoCommand( new RemoveItemsCommand(items) );
        } else {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnTopMenuAction.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(0, 1) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnRightMenuAction.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(1, 0) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnLeftMenuAction.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(-1, 0) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnBottomMenuAction.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(0, -1) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->stopTrackingButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onStopButtonClicked();
    } else if ( ( k == _imp->ui->trackBwButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackBwClicked();
    } else if ( ( k == _imp->ui->trackPrevButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackPrevClicked();
    } else if ( ( k == _imp->ui->trackFwButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackFwClicked();
    } else if ( ( k == _imp->ui->trackNextButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackNextClicked();
    } else if ( ( k == _imp->ui->trackRangeButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackRangeClicked();
    } else if ( ( k == _imp->ui->trackAllKeyframesButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackAllKeyframesClicked();
    } else if ( ( k == _imp->ui->trackCurrentKeyframeButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackCurrentKeyframeClicked();
    } else if ( ( k == _imp->ui->clearAllAnimationButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onClearAllAnimationClicked();
    } else if ( ( k == _imp->ui->clearBwAnimationButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onClearBwAnimationClicked();
    } else if ( ( k == _imp->ui->clearFwAnimationButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onClearFwAnimationClicked();
    } else if ( ( k == _imp->ui->setKeyFrameButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onSetKeyframeButtonClicked();
    } else if ( ( k == _imp->ui->removeKeyFrameButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onRemoveKeyframeButtonClicked();
    } else if ( ( k == _imp->ui->resetOffsetButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onResetOffsetButtonClicked();
    } else if ( ( k == _imp->ui->resetTrackButton.lock() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onResetTrackButtonClicked();
    } else if ( k == _imp->ui->addTrackButton.lock() ) {
        _imp->ui->clickToAddTrackEnabled = _imp->ui->addTrackButton.lock()->getValue();
    } else if ( k == _imp->exportButton.lock() ) {
        _imp->exportTrackDataFromExportOptions();
    } else if ( k == _imp->setCurrentFrameButton.lock() ) {
        KnobIntPtr refFrame = _imp->referenceFrame.lock();
        refFrame->setValue(time);
    } else if ( k == _imp->transformType.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
        _imp->refreshVisibilityFromTransformType();
    } else if ( k == _imp->motionType.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
        _imp->refreshVisibilityFromTransformType();
    } else if ( k == _imp->jitterPeriod.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
    } else if ( k == _imp->smoothCornerPin.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
    } else if ( k == _imp->smoothTransform.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
    } else if ( k == _imp->referenceFrame.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
    } else if ( k == _imp->robustModel.lock() ) {
        _imp->solveTransformParamsIfAutomatic();
    } else if ( k == _imp->generateTransformButton.lock() ) {
        _imp->solveTransformParams();
    } else if ( k == _imp->setFromPointsToInputRodButton.lock() ) {
        _imp->setFromPointsToInputRod();
        _imp->solveTransformParamsIfAutomatic();
    } else if ( k == _imp->autoGenerateTransform.lock() ) {
        _imp->solveTransformParams();
        _imp->refreshVisibilityFromTransformType();
    }
#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    else if ( k == _imp->usePatternMatching.lock() ) {
        _imp->refreshVisibilityFromTransformType();
    }
#endif
    else if ( k == _imp->disableTransform.lock() ) {
        _imp->refreshVisibilityFromTransformType();
    } else if ( k == _imp->addTrackFromPanelButton.lock() ) {
        TrackMarkerPtr marker = _imp->createMarker();
        pushUndoCommand( new AddItemsCommand(marker) );
    } else if ( k == _imp->removeSelectedTracksButton.lock() ) {
        std::list<KnobTableItemPtr> items = _imp->knobsTable->getSelectedItems();
        if ( !items.empty() ) {
            pushUndoCommand( new RemoveItemsCommand(items) );
        } else {
            return false;
        }

    } else if ( k == _imp->averageTracksButton.lock() ) {
        _imp->averageSelectedTracks();
    } else {
        ret = false;
    }


    return ret;
} // TrackerNode::knobChanged

void
TrackerNode::onKnobsLoaded()
{


    _imp->setSolverParamsEnabled(true);
    _imp->refreshVisibilityFromTransformType();

}

void
TrackerNode::evaluate(bool isSignificant, bool refreshMetadata)
{
    NodeGroup::evaluate(isSignificant, refreshMetadata);
    _imp->ui->refreshSelectedMarkerTextureLater();
}

RectD
TrackerNodePrivate::getInputRoD(TimeValue time, ViewIdx view) const
{
    EffectInstancePtr inputEffect = publicInterface->getInputRenderEffect(0, time, view);
    RectD ret;

    if (inputEffect) {
        GetRegionOfDefinitionResultsPtr results;
        ActionRetCodeEnum stat = inputEffect->getRegionOfDefinition_public(time, RenderScale(1.), view, &results);
        if (!isFailureRetCode(stat)) {
            return results->getRoD();
        }
    }
    Format f;
    publicInterface->getApp()->getProject()->getProjectDefaultFormat(&f);
    ret.x1 = f.x1;
    ret.x2 = f.x2;
    ret.y1 = f.y1;
    ret.y2 = f.y2;


    return ret;
} // getInputRoD

void
TrackerNodePrivate::setFromPointsToInputRod()
{
    RectD inputRod = getInputRoD(publicInterface->getTimelineCurrentTime(), ViewIdx(0));
    KnobDoublePtr fromPointsKnob[4];

    for (int i = 0; i < 4; ++i) {
        fromPointsKnob[i] = fromPoints[i].lock();
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x1;
        values[1] = inputRod.y1;
        fromPointsKnob[0]->setValueAcrossDimensions(values);
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x2;
        values[1] = inputRod.y1;
        fromPointsKnob[1]->setValueAcrossDimensions(values);
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x2;
        values[1] = inputRod.y2;
        fromPointsKnob[2]->setValueAcrossDimensions(values);
    }
    {
        std::vector<double> values(2);
        values[0] = inputRod.x1;
        values[1] = inputRod.y2;
        fromPointsKnob[3]->setValueAcrossDimensions(values);
    }
}

void
TrackerNode::onInputChanged(int inputNb)
{
    KnobBoolPtr fromPointsSetOnceKnob = _imp->cornerPinFromPointsSetOnceAutomatically.lock();
    if ( !fromPointsSetOnceKnob->getValue() ) {
        _imp->setFromPointsToInputRod();
        fromPointsSetOnceKnob->setValue(true);
    }

    std::vector<TrackMarkerPtr> allMarkers;
    _imp->knobsTable->getAllMarkers(&allMarkers);
    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        TrackMarkerPM* isPM = dynamic_cast<TrackMarkerPM*>(allMarkers[i].get());
        if (isPM) {
            isPM->onTrackerNodeInputChanged(inputNb);
        }
    }


    _imp->ui->refreshSelectedMarkerTextureLater();
}

void
TrackerNode::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                               TimeValue /*time*/)
{
    if (_imp->ui->showMarkerTexture && !isPlayback && !getApp()->isDraftRenderEnabled()) {
        _imp->ui->refreshSelectedMarkerTextureLater();
    }
}

SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr
TrackerKnobItemsTable::createSerializationFromItem(const KnobTableItemPtr& item)
{
    TrackMarkerPtr isTrack = toTrackMarker(item);
    assert(isTrack);
    SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr ret(new SERIALIZATION_NAMESPACE::KnobTableItemSerialization);
    isTrack->toSerialization(ret.get());
    return ret;
}

KnobTableItemPtr
TrackerKnobItemsTable::createItemFromSerialization(const SERIALIZATION_NAMESPACE::KnobTableItemSerializationPtr& data)
{
    const SERIALIZATION_NAMESPACE::KnobTableItemSerialization* serialization = dynamic_cast<const SERIALIZATION_NAMESPACE::KnobTableItemSerialization*>(data.get());
    if (!serialization) {
        return KnobTableItemPtr();
    }
    TrackMarkerPtr marker;
    {
        marker = TrackMarker::create(shared_from_this());
        QObject::connect( marker.get(), SIGNAL(keyframeRemoved(TimeValue)), _imp->ui.get(), SLOT(onKeyframeRemovedOnTrack(TimeValue)) );
        QObject::connect( marker.get(), SIGNAL(keyframeAdded(TimeValue)), _imp->ui.get(), SLOT(onKeyframeSetOnTrack(TimeValue)) );
        QObject::connect( marker.get(), SIGNAL(keyframeMoved(TimeValue,TimeValue)), _imp->ui.get(), SLOT(onKeyframeMovedOnTrack(TimeValue, TimeValue)) );
    }
    marker->initializeKnobsPublic();
    marker->fromSerialization(*serialization);
    return marker;
}

void
TrackerKnobItemsTable::getAllMarkers(std::vector<TrackMarkerPtr>* markers) const
{
    if (!markers) {
        return;
    }
    std::vector<KnobTableItemPtr> items = getTopLevelItems();
    for (std::vector<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        TrackMarkerPtr marker = toTrackMarker(*it);
        assert(marker);
        markers->push_back(marker);
    }
}

void
TrackerKnobItemsTable::getAllEnabledMarkers(std::list<TrackMarkerPtr>* markers) const
{
    if (!markers) {
        return;
    }
    std::vector<KnobTableItemPtr> items = getTopLevelItems();
    for (std::vector<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        TrackMarkerPtr marker = toTrackMarker(*it);
        assert(marker);
        if (marker->isEnabled(marker->getTimelineCurrentTime())) {
            markers->push_back(marker);
        }
    }
}

void
TrackerKnobItemsTable::getSelectedMarkers(std::list<TrackMarkerPtr>* markers) const
{
    if (!markers) {
        return;
    }
    std::list<KnobTableItemPtr> items = getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        markers->push_back(toTrackMarker(*it));
    }
}

bool
TrackerKnobItemsTable::isMarkerSelected(const TrackMarkerPtr& marker) const
{
    std::list<KnobTableItemPtr> items = getSelectedItems();

    for (std::list<KnobTableItemPtr>::const_iterator it = items.begin(); it != items.end(); ++it) {
        if (*it == marker) {
            return true;
        }
    }

    return false;
}

void
TrackerNodePrivate::resetTransformCenter()
{
    std::list<TrackMarkerPtr> tracks;
    knobsTable->getAllEnabledMarkers(&tracks);

    TimeValue time(referenceFrame.lock()->getValue());
    std::vector<double> p(2);
    if ( tracks.empty() ) {
        RectD rod = getInputRoD(time, ViewIdx(0));
        p[0] = (rod.x1 + rod.x2) / 2.;
        p[1] = (rod.y1 + rod.y2) / 2.;
    } else {
        p[0] = p[1] = 0.;
        for (std::list<TrackMarkerPtr>::const_iterator it = tracks.begin(); it != tracks.end(); ++it) {
            KnobDoublePtr centerKnob = (*it)->getCenterKnob();
            p[0] += centerKnob->getValueAtTime(time, DimIdx(0));
            p[1] += centerKnob->getValueAtTime(time, DimIdx(1));

        }
        p[0] /= tracks.size();
        p[1] /= tracks.size();
    }

    KnobDoublePtr centerKnob = center.lock();
    centerKnob->resetToDefaultValue(DimSpec::all(), ViewSetSpec::all());
    centerKnob->setValueAcrossDimensions(p);
}

void
TrackerNodePrivate::refreshVisibilityFromTransformTypeInternal(TrackerTransformNodeEnum transformType)
{
    if ( !transformNode.lock() ) {
        return;
    }

    KnobChoicePtr motionTypeKnob = motionType.lock();
    if (!motionTypeKnob) {
        return;
    }
    int motionType_i = motionTypeKnob->getValue();
    TrackerMotionTypeEnum motionType = (TrackerMotionTypeEnum)motionType_i;
    KnobBoolPtr disableTransformKnob = disableTransform.lock();
    bool disableNodes = disableTransformKnob->getValue();

    transformNode.lock()->getEffectInstance()->setNodeDisabled(disableNodes || transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    cornerPinNode.lock()->getEffectInstance()->setNodeDisabled(disableNodes || transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);

    transformControlsSeparator.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    disableTransformKnob->setSecret(motionType == eTrackerMotionTypeNone);
    if (transformType == eTrackerTransformNodeTransform) {
        transformControlsSeparator.lock()->setLabel( tr("Transform Controls") );
        disableTransformKnob->setLabel( tr("Disable Transform") );
    } else if (transformType == eTrackerTransformNodeCornerPin) {
        transformControlsSeparator.lock()->setLabel( tr("CornerPin Controls") );
        disableTransformKnob->setLabel( tr("Disable CornerPin") );
    }


    smoothTransform.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    smoothCornerPin.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);

    toGroup.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    fromGroup.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    cornerPinOverlayPoints.lock()->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    KnobDoublePtr matrix = cornerPinMatrix.lock();
    if (matrix) {
        matrix->setSecret(transformType == eTrackerTransformNodeTransform || motionType == eTrackerMotionTypeNone);
    }


    translate.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    scale.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    scaleUniform.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    rotate.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    center.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    skewX.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    skewY.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    skewOrder.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    filter.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    clamp.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);
    blackOutside.lock()->setSecret(transformType == eTrackerTransformNodeCornerPin || motionType == eTrackerMotionTypeNone);

    invertTransform.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    motionBlur.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    shutter.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    shutterOffset.lock()->setSecret(motionType == eTrackerMotionTypeNone);
    customShutterOffset.lock()->setSecret(motionType == eTrackerMotionTypeNone);

    exportLink.lock()->setEnabled(motionType != eTrackerMotionTypeNone);
    exportButton.lock()->setEnabled(motionType != eTrackerMotionTypeNone);

#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    bool usePM = usePatternMatching.lock()->getValue();
    enableTrackRed.lock()->setSecret(usePM);
    enableTrackGreen.lock()->setSecret(usePM);
    enableTrackBlue.lock()->setSecret(usePM);
    maxError.lock()->setSecret(usePM);
    maxIterations.lock()->setSecret(usePM);
    bruteForcePreTrack.lock()->setSecret(usePM);
    useNormalizedIntensities.lock()->setSecret(usePM);
    preBlurSigma.lock()->setSecret(usePM);

    patternMatchingScore.lock()->setSecret(!usePM);

#endif
} // TrackerNodePrivate::refreshVisibilityFromTransformTypeInternal

void
TrackerNodePrivate::refreshVisibilityFromTransformType()
{
    KnobChoicePtr transformTypeKnob = transformType.lock();

    // The transform page may not be created yet
    if (!transformTypeKnob) {
        return;
    }

    int transformType_i = transformTypeKnob->getValue();
    TrackerTransformNodeEnum transformType = (TrackerTransformNodeEnum)transformType_i;
    refreshVisibilityFromTransformTypeInternal(transformType);
}

void
TrackerNodePrivate::setSolverParamsEnabled(bool enabled)
{
    KnobChoicePtr motionTypeKnob = motionType.lock();

    // The transform page may not be created yet
    if (!motionTypeKnob) {
        return;
    }
    motionTypeKnob->setEnabled(enabled);
    setCurrentFrameButton.lock()->setEnabled(enabled);
    robustModel.lock()->setEnabled(enabled);
    referenceFrame.lock()->setEnabled(enabled);
    transformType.lock()->setEnabled(enabled);
    jitterPeriod.lock()->setEnabled(enabled);
    smoothTransform.lock()->setEnabled(enabled);
    smoothCornerPin.lock()->setEnabled(enabled);
}

void
TrackerNodePrivate::setTransformOutOfDate(bool outdated)
{
    transformOutOfDateLabel.lock()->setSecret(!outdated);
}

bool
TrackerNodePrivate::trackStepFunctor(int trackIndex, const TrackArgsBasePtr& args, int frame)
{

    TrackArgs* trackerArgs = dynamic_cast<TrackArgs*>(args.get());
    assert(trackerArgs);

    assert( trackIndex >= 0 && trackIndex < trackerArgs->getNumTracks() );
    const std::vector<TrackMarkerAndOptionsPtr>& tracks = trackerArgs->getTracks();
    const TrackMarkerAndOptionsPtr& track = tracks[trackIndex];

    if ( !track->natronMarker->isEnabled(TimeValue(frame)) ) {
        return false;
    }

    TrackMarkerPMPtr isTrackerPM = toTrackMarkerPM( track->natronMarker );
    bool ret;
    if (isTrackerPM) {
        ret = TrackerHelperPrivate::trackStepTrackerPM(isTrackerPM, *trackerArgs, frame);
    } else {
        ret = TrackerHelperPrivate::trackStepLibMV(trackIndex, *trackerArgs, frame);
    }

    // Disable the marker since it failed to track
    if (!ret && trackerArgs->isAutoKeyingEnabledParamEnabled()) {
        track->natronMarker->setEnabledAtTime(TimeValue(frame), false);
    }


    return ret;
} // trackStepFunctor

void
TrackerNodePrivate::beginTrackSequence(const TrackArgsBasePtr& args)
{
    TrackArgs* trackerArgs = dynamic_cast<TrackArgs*>(args.get());

    const std::vector<TrackMarkerAndOptionsPtr>& tracks = trackerArgs->getTracks();
    // For all tracks, notify tracking is starting and unslave the 'enabled' knob if it is
    // slaved to the UI "enabled" knob.
    for (std::size_t i = 0; i < tracks.size(); ++i) {
        tracks[i]->natronMarker->notifyTrackingStarted();
    }
}

void
TrackerNodePrivate::endTrackSequence(const TrackArgsBasePtr& args)
{
    TrackArgs* trackerArgs = dynamic_cast<TrackArgs*>(args.get());
    const std::vector<TrackMarkerAndOptionsPtr>& tracks = trackerArgs->getTracks();

    for (std::size_t i = 0; i < tracks.size(); ++i) {
        tracks[i]->natronMarker->notifyTrackingEnded();
    }

}

NodePtr
TrackerNodePrivate::getTrackerNode() const
{
    return publicInterface->getNode();
}

NodePtr
TrackerNodePrivate::getSourceImageNode() const
{
    NodePtr node =  publicInterface->getNode();
    if (!node) {
        return NodePtr();
    }
    return node->getInput(0);
}

ImagePlaneDesc
TrackerNodePrivate::getMaskImagePlane(int *channelIndex) const
{
    *channelIndex = 0;
    return ImagePlaneDesc::getAlphaComponents();
}

NodePtr
TrackerNodePrivate::getMaskImageNode() const
{
#ifdef ROTOPAINT_ENABLE_PLANARTRACKER
    return maskNode.lock();
#else
    return NodePtr();
#endif
}

TrackerHelperPtr
TrackerNodePrivate::getTracker() const
{
    return tracker;
}

TrackerHelperPtr
TrackerNode::getTracker() const
{
    return _imp->tracker;
}

TrackMarkerPtr
TrackerNode::createMarker()
{
    return _imp->createMarker();
}


bool
TrackerNodePrivate::getCenterOnTrack() const
{
    return ui->centerViewerButton.lock()->getValue();
}

bool
TrackerNodePrivate::getUpdateViewer() const
{
    return ui->updateViewerButton.lock()->getValue();
}

void
TrackerNodePrivate::getTrackChannels(bool* doRed, bool* doGreen, bool* doBlue) const
{
    *doRed = enableTrackRed.lock()->getValue();
    *doGreen = enableTrackGreen.lock()->getValue();
    *doBlue = enableTrackBlue.lock()->getValue();
}

bool
TrackerNodePrivate::canDisableMarkersAutomatically() const
{
    return autoKeyEnabled.lock()->getValue();
}

double
TrackerNodePrivate::getMaxError() const
{
    return maxError.lock()->getValue();
}

int
TrackerNodePrivate::getMaxNIterations() const
{
    return maxIterations.lock()->getValue();
}

bool
TrackerNodePrivate::isBruteForcePreTrackEnabled() const
{
    return bruteForcePreTrack.lock()->getValue();
}

bool
TrackerNodePrivate::isNormalizeIntensitiesEnabled() const
{
    return useNormalizedIntensities.lock()->getValue();
}

double
TrackerNodePrivate::getPreBlurSigma() const
{
    return preBlurSigma.lock()->getValue();
}

RectD
TrackerNodePrivate::getNormalizationRoD(TimeValue time, ViewIdx view) const
{
    return getInputRoD(time, view);
}

TrackMarkerPtr
TrackerNodePrivate::createMarker()
{
    TrackMarkerPtr track;

#ifdef NATRON_TRACKER_ENABLE_TRACKER_PM
    bool usePM = isTrackerPMEnabled();
    if (!usePM) {
        track = TrackMarker::create( knobsTable );
    } else {
        track = TrackMarkerPM::create( knobsTable );
    }
#else
    track = TrackMarker::create(knobsTable);
#endif

    knobsTable->addItem(track, KnobTableItemPtr(), eTableChangeReasonInternal);

    track->resetCenter();

    QObject::connect( track.get(), SIGNAL(keyframeRemoved(TimeValue)), ui.get(), SLOT(onKeyframeRemovedOnTrack(TimeValue)) );
    QObject::connect( track.get(), SIGNAL(keyframeAdded(TimeValue)), ui.get(), SLOT(onKeyframeSetOnTrack(TimeValue)) );
    QObject::connect( track.get(), SIGNAL(keyframeMoved(TimeValue,TimeValue)), ui.get(), SLOT(onKeyframeMovedOnTrack(TimeValue, TimeValue)) );

    return track;
}

void
TrackerNodePrivate::trackSelectedMarkers(TimeValue start, TimeValue end, TimeValue frameStep, OverlaySupport* viewer)
{
    std::list<TrackMarkerPtr> markers;
    knobsTable->getAllEnabledMarkers(&markers);

    ViewerNodePtr viewerNode;
    if (viewer) {
        viewerNode = viewer->getViewerNode();
    }
    tracker->trackMarkers(markers, start, end, frameStep, viewerNode);
}

void
TrackerNodePrivate::averageSelectedTracks()
{

    std::list<TrackMarkerPtr> markers;
    knobsTable->getSelectedMarkers(&markers);
    if ( markers.empty() ) {
        Dialogs::warningDialog( tr("Average").toStdString(), tr("No tracks selected").toStdString() );

        return;
    }

    TrackMarkerPtr marker = createMarker();
    publicInterface->pushUndoCommand(new AddItemsCommand(marker));

    KnobDoublePtr centerKnob = marker->getCenterKnob();

#ifdef AVERAGE_ALSO_PATTERN_QUAD
    KnobDoublePtr topLeftKnob = marker->getPatternTopLeftKnob();
    KnobDoublePtr topRightKnob = marker->getPatternTopRightKnob();
    KnobDoublePtr btmRightKnob = marker->getPatternBtmRightKnob();
    KnobDoublePtr btmLeftKnob = marker->getPatternBtmLeftKnob();
#endif

    RangeD keyframesRange;
    keyframesRange.min = INT_MAX;
    keyframesRange.max = INT_MIN;
    for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
        KnobDoublePtr markCenter = (*it)->getCenterKnob();
        TimeValue mini, maxi;
        bool hasKey = markCenter->getFirstKeyFrameTime(ViewIdx(0), DimIdx(0), &mini);
        if (!hasKey) {
            continue;
        }
        if (mini < keyframesRange.min) {
            keyframesRange.min = mini;
        }
        hasKey = markCenter->getLastKeyFrameTime(ViewIdx(0), DimIdx(0), &maxi);

        ///both dimensions must have keyframes
        assert(hasKey);
        if (maxi > keyframesRange.max) {
            keyframesRange.max = maxi;
        }
    }

    bool hasKeyFrame = keyframesRange.min != INT_MIN && keyframesRange.max != INT_MAX;
    for (double t = keyframesRange.min; t <= keyframesRange.max; t += 1) {
        Point avgCenter;
        avgCenter.x = avgCenter.y = 0.;

#ifdef AVERAGE_ALSO_PATTERN_QUAD
        Point avgTopLeft, avgTopRight, avgBtmRight, avgBtmLeft;
        avgTopLeft.x = avgTopLeft.y = avgTopRight.x = avgTopRight.y = avgBtmRight.x = avgBtmRight.y = avgBtmLeft.x = avgBtmLeft.y = 0;
#endif


        for (std::list<TrackMarkerPtr>::iterator it = markers.begin(); it != markers.end(); ++it) {
            KnobDoublePtr markCenter = (*it)->getCenterKnob();

#ifdef AVERAGE_ALSO_PATTERN_QUAD
            KnobDoublePtr markTopLeft = (*it)->getPatternTopLeftKnob();
            KnobDoublePtr markTopRight = (*it)->getPatternTopRightKnob();
            KnobDoublePtr markBtmRight = (*it)->getPatternBtmRightKnob();
            KnobDoublePtr markBtmLeft = (*it)->getPatternBtmLeftKnob();
#endif

            avgCenter.x += markCenter->getValueAtTime(TimeValue(t), DimIdx(0));
            avgCenter.y += markCenter->getValueAtTime(TimeValue(t), DimIdx(1));

#        ifdef AVERAGE_ALSO_PATTERN_QUAD
            avgTopLeft.x += markTopLeft->getValueAtTime(TimeValue(t), DimIdx(0));
            avgTopLeft.y += markTopLeft->getValueAtTime(TimeValue(t), DimIdx(1));

            avgTopRight.x += markTopRight->getValueAtTime(TimeValue(t), DimIdx(0));
            avgTopRight.y += markTopRight->getValueAtTime(TimeValue(t), DimIdx(1));

            avgBtmRight.x += markBtmRight->getValueAtTime(TimeValue(t), DimIdx(0));
            avgBtmRight.y += markBtmRight->getValueAtTime(TimeValue(t), DimIdx(1));

            avgBtmLeft.x += markBtmLeft->getValueAtTime(TimeValue(t), DimIdx(0));
            avgBtmLeft.y += markBtmLeft->getValueAtTime(TimeValue(t), DimIdx(1));
#         endif
        }

        int n = markers.size();
        if (n) {
            avgCenter.x /= n;
            avgCenter.y /= n;

#         ifdef AVERAGE_ALSO_PATTERN_QUAD
            avgTopLeft.x /= n;
            avgTopLeft.y /= n;

            avgTopRight.x /= n;
            avgTopRight.y /= n;

            avgBtmRight.x /= n;
            avgBtmRight.y /= n;

            avgBtmLeft.x /= n;
            avgBtmLeft.y /= n;
#         endif
        }

        if (!hasKeyFrame) {
            centerKnob->setValue(avgCenter.x, ViewSetSpec::all(), DimIdx(0));
            centerKnob->setValue(avgCenter.y, ViewSetSpec::all(), DimIdx(1));
#         ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValue(avgTopLeft.x, ViewSetSpec::all(), DimIdx(0));
            topLeftKnob->setValue(avgTopLeft.y, ViewSetSpec::all(), DimIdx(1));
            topRightKnob->setValue(avgTopRight.x, ViewSetSpec::all(), DimIdx(0));
            topRightKnob->setValue(avgTopRight.y, ViewSetSpec::all(), DimIdx(1));
            btmRightKnob->setValue(avgBtmRight.x, ViewSetSpec::all(), DimIdx(0));
            btmRightKnob->setValue(avgBtmRight.y, ViewSetSpec::all(), DimIdx(1));
            btmLeftKnob->setValue(avgBtmLeft.x, ViewSetSpec::all(), DimIdx(0));
            btmLeftKnob->setValue(avgBtmLeft.y, ViewSetSpec::all(), DimIdx(1));
#         endif
            break;
        } else {
            centerKnob->setValueAtTime(TimeValue(t), avgCenter.x, ViewSetSpec::all(), DimIdx(0));
            centerKnob->setValueAtTime(TimeValue(t), avgCenter.y, ViewSetSpec::all(), DimIdx(1));
#         ifdef AVERAGE_ALSO_PATTERN_QUAD
            topLeftKnob->setValueAtTime(TimeValue(t), avgTopLeft.x, ViewSetSpec::all(), DimIdx(0));
            topLeftKnob->setValueAtTime(TimeValue(t), avgTopLeft.y, ViewSetSpec::all(), DimIdx(1));
            topRightKnob->setValueAtTime(TimeValue(t), avgTopRight.x, ViewSetSpec::all(), DimIdx(0));
            topRightKnob->setValueAtTime(TimeValue(t), avgTopRight.y, ViewSetSpec::all(), DimIdx(1));
            btmRightKnob->setValueAtTime(TimeValue(t), avgBtmRight.x, ViewSetSpec::all(), DimIdx(0));
            btmRightKnob->setValueAtTime(TimeValue(t), avgBtmRight.y, ViewSetSpec::all(), DimIdx(1));
            btmLeftKnob->setValueAtTime(TimeValue(t), avgBtmLeft.x, ViewSetSpec::all(), DimIdx(0));
            btmLeftKnob->setValueAtTime(TimeValue(t), avgBtmLeft.y, ViewSetSpec::all(), DimIdx(1));
#         endif
        }
    }

} // averageSelectedTracks



NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING

#include "moc_TrackerNode.cpp"

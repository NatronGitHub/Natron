/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <https://natrongithub.github.io/>,
 * (C) 2018-2022 The Natron developers
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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include <boost/algorithm/clamp.hpp>

#include "TrackerNode.h"

#include "Engine/AppInstance.h"
#include "Engine/Curve.h"
#include "Engine/KnobTypes.h"
#include "Engine/Node.h"
#include "Engine/Lut.h"
#include "Engine/TimeLine.h"
#include "Engine/TrackerContext.h"
#include "Engine/TrackMarker.h"
#include "Engine/TrackerNodeInteract.h"
#include "Engine/TrackerUndoCommand.h"
#include "Engine/ViewerInstance.h"


#define NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING 10

NATRON_NAMESPACE_ENTER


TrackerNode::TrackerNode(Natron::NodePtr node)
    : NodeGroup(node)
    , _imp( new TrackerNodePrivate(this) )
{
}

TrackerNode::~TrackerNode()
{
}

std::string
TrackerNode::getPluginID() const
{
    return PLUGINID_NATRON_TRACKER;
}

std::string
TrackerNode::getPluginLabel() const
{
    return "Tracker";
}

std::string
TrackerNode::getPluginDescription() const
{
    return "Track one or more 2D point(s) using LibMV from the Blender open-source software.\n\n"
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
           "You may export the tracking data either to a CornerPin node or to a Transform node. The CornerPin node performs a warp that may be more stable than a Transform node when using 4 or more tracks: it retains more information than the Transform node.";
}

void
TrackerNode::getPluginShortcuts(std::list<PluginActionShortcut>* shortcuts)
{
    // Viewer buttons
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackBW, kTrackerUIParamTrackBWLabel, Key_Z) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackPrevious, kTrackerUIParamTrackPreviousLabel, Key_X) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackNext, kTrackerUIParamTrackNextLabel, Key_C) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamStopTracking, kTrackerUIParamStopTrackingLabel, Key_Escape) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackFW, kTrackerUIParamTrackFWLabel, Key_V) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackRange, kTrackerUIParamTrackRangeLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackAllKeyframes, kTrackerUIParamTrackAllKeyframesLabel, Key_V, eKeyboardModifierControl) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamTrackCurrentKeyframe, kTrackerUIParamTrackCurrentKeyframeLabel, Key_C, eKeyboardModifierControl) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamClearAllAnimation, kTrackerUIParamClearAllAnimationLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamClearAnimationBw, kTrackerUIParamClearAnimationBwLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamClearAnimationFw, kTrackerUIParamClearAnimationFwLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRefreshViewer, kTrackerUIParamRefreshViewerLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamCenterViewer, kTrackerUIParamCenterViewerLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamCreateKeyOnMove, kTrackerUIParamCreateKeyOnMoveLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamShowError, kTrackerUIParamShowErrorLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamSetPatternKeyFrame, kTrackerUIParamSetPatternKeyFrameLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRemovePatternKeyFrame, kTrackerUIParamRemovePatternKeyFrameLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamResetOffset, kTrackerUIParamResetOffsetLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamResetTrack, kTrackerUIParamResetTrackLabel) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRightClickMenuActionSelectAllTracks, kTrackerUIParamRightClickMenuActionSelectAllTracksLabel, Key_A, eKeyboardModifierControl) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRightClickMenuActionRemoveTracks, kTrackerUIParamRightClickMenuActionRemoveTracksLabel, Key_BackSpace) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeBottom, kTrackerUIParamRightClickMenuActionNudgeBottomLabel, Key_Down) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeTop, kTrackerUIParamRightClickMenuActionNudgeTopLabel, Key_Up) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeRight, kTrackerUIParamRightClickMenuActionNudgeRightLabel, Key_Right) );
    shortcuts->push_back( PluginActionShortcut(kTrackerUIParamRightClickMenuActionNudgeLeft, kTrackerUIParamRightClickMenuActionNudgeLeftLabel, Key_Left) );

    // Right click menu
}

void
TrackerNode::initializeKnobs()
{
    TrackerContextPtr context = getNode()->getTrackerContext();
    KnobPagePtr trackingPage = context->getTrackingPageKnob();

    KnobButtonPtr addMarker = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamAddTrackLabel) );
    addMarker->setName(kTrackerUIParamAddTrack);
    addMarker->setHintToolTip( tr(kTrackerUIParamAddTrackHint) );
    addMarker->setEvaluateOnChange(false);
    addMarker->setCheckable(true);
    addMarker->setDefaultValue(false);
    addMarker->setSecretByDefault(true);
    addMarker->setIconLabel(NATRON_IMAGES_PATH "addTrack.png");
    addOverlaySlaveParam(addMarker);
    trackingPage->addKnob(addMarker);
    _imp->ui->addTrackButton = addMarker;

    KnobButtonPtr trackBw = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackBWLabel) );
    trackBw->setName(kTrackerUIParamTrackBW);
    trackBw->setHintToolTip( tr(kTrackerUIParamTrackBWHint) );
    trackBw->setEvaluateOnChange(false);
    trackBw->setCheckable(true);
    trackBw->setDefaultValue(false);
    trackBw->setSecretByDefault(true);
    trackBw->setInViewerContextCanHaveShortcut(true);
    trackBw->setIconLabel(NATRON_IMAGES_PATH "trackBackwardOn.png", true);
    trackBw->setIconLabel(NATRON_IMAGES_PATH "trackBackwardOff.png", false);
    trackingPage->addKnob(trackBw);
    _imp->ui->trackBwButton = trackBw;

    KnobButtonPtr trackPrev = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackPreviousLabel) );
    trackPrev->setName(kTrackerUIParamTrackPrevious);
    trackPrev->setHintToolTip( tr(kTrackerUIParamTrackPreviousHint) );
    trackPrev->setEvaluateOnChange(false);
    trackPrev->setSecretByDefault(true);
    trackPrev->setInViewerContextCanHaveShortcut(true);
    trackPrev->setIconLabel(NATRON_IMAGES_PATH "trackPrev.png");
    trackingPage->addKnob(trackPrev);
    _imp->ui->trackPrevButton = trackPrev;

    KnobButtonPtr stopTracking = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamStopTrackingLabel) );
    stopTracking->setName(kTrackerUIParamStopTracking);
    stopTracking->setHintToolTip( tr(kTrackerUIParamStopTrackingHint) );
    stopTracking->setEvaluateOnChange(false);
    stopTracking->setSecretByDefault(true);
    stopTracking->setInViewerContextCanHaveShortcut(true);
    stopTracking->setIconLabel(NATRON_IMAGES_PATH "pauseDisabled.png");
    trackingPage->addKnob(stopTracking);
    _imp->ui->stopTrackingButton = stopTracking;

    KnobButtonPtr trackNext = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackNextLabel) );
    trackNext->setName(kTrackerUIParamTrackNext);
    trackNext->setHintToolTip( tr(kTrackerUIParamTrackNextHint) );
    trackNext->setEvaluateOnChange(false);
    trackNext->setSecretByDefault(true);
    trackNext->setInViewerContextCanHaveShortcut(true);
    trackNext->setIconLabel(NATRON_IMAGES_PATH "trackNext.png");
    trackingPage->addKnob(trackNext);
    _imp->ui->trackNextButton = trackNext;

    KnobButtonPtr trackFw = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackFWLabel) );
    trackFw->setName(kTrackerUIParamTrackFW);
    trackFw->setHintToolTip( tr(kTrackerUIParamTrackFWHint) );
    trackFw->setEvaluateOnChange(false);
    trackFw->setCheckable(true);
    trackFw->setDefaultValue(false);
    trackFw->setSecretByDefault(true);
    trackFw->setInViewerContextCanHaveShortcut(true);
    trackFw->setIconLabel(NATRON_IMAGES_PATH "trackForwardOn.png", true);
    trackFw->setIconLabel(NATRON_IMAGES_PATH "trackForwardOff.png", false);
    trackingPage->addKnob(trackFw);
    _imp->ui->trackFwButton = trackFw;

    KnobButtonPtr trackRange = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackRangeLabel) );
    trackRange->setName(kTrackerUIParamTrackRange);
    trackRange->setHintToolTip( tr(kTrackerUIParamTrackRangeHint) );
    trackRange->setEvaluateOnChange(false);
    trackRange->setSecretByDefault(true);
    trackRange->setInViewerContextCanHaveShortcut(true);
    trackRange->setIconLabel(NATRON_IMAGES_PATH "trackRange.png");
    trackingPage->addKnob(trackRange);
    _imp->ui->trackRangeButton = trackRange;

    KnobButtonPtr trackAllKeys = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackAllKeyframesLabel) );
    trackAllKeys->setName(kTrackerUIParamTrackAllKeyframes);
    trackAllKeys->setHintToolTip( tr(kTrackerUIParamTrackAllKeyframesHint) );
    trackAllKeys->setEvaluateOnChange(false);
    trackAllKeys->setSecretByDefault(true);
    trackAllKeys->setInViewerContextCanHaveShortcut(true);
    trackAllKeys->setIconLabel(NATRON_IMAGES_PATH "trackAllKeyframes.png");
    trackingPage->addKnob(trackAllKeys);
    _imp->ui->trackAllKeyframesButton = trackAllKeys;


    KnobButtonPtr trackCurKey = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackCurrentKeyframeLabel) );
    trackCurKey->setName(kTrackerUIParamTrackCurrentKeyframe);
    trackCurKey->setHintToolTip( tr(kTrackerUIParamTrackCurrentKeyframeHint) );
    trackCurKey->setEvaluateOnChange(false);
    trackCurKey->setSecretByDefault(true);
    trackCurKey->setInViewerContextCanHaveShortcut(true);
    trackCurKey->setIconLabel(NATRON_IMAGES_PATH "trackCurrentKeyframe.png");
    trackingPage->addKnob(trackCurKey);
    _imp->ui->trackCurrentKeyframeButton = trackCurKey;


    KnobButtonPtr addKeyframe = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamSetPatternKeyFrameLabel) );
    addKeyframe->setName(kTrackerUIParamSetPatternKeyFrame);
    addKeyframe->setHintToolTip( tr(kTrackerUIParamSetPatternKeyFrameHint) );
    addKeyframe->setEvaluateOnChange(false);
    addKeyframe->setSecretByDefault(true);
    addKeyframe->setInViewerContextCanHaveShortcut(true);
    addKeyframe->setIconLabel(NATRON_IMAGES_PATH "addUserKey.png");
    trackingPage->addKnob(addKeyframe);
    _imp->ui->setKeyFrameButton = addKeyframe;

    KnobButtonPtr removeKeyframe = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRemovePatternKeyFrameLabel) );
    removeKeyframe->setName(kTrackerUIParamRemovePatternKeyFrame);
    removeKeyframe->setHintToolTip( tr(kTrackerUIParamRemovePatternKeyFrameHint) );
    removeKeyframe->setEvaluateOnChange(false);
    removeKeyframe->setSecretByDefault(true);
    removeKeyframe->setInViewerContextCanHaveShortcut(true);
    removeKeyframe->setIconLabel(NATRON_IMAGES_PATH "removeUserKey.png");
    trackingPage->addKnob(removeKeyframe);
    _imp->ui->removeKeyFrameButton = removeKeyframe;

    KnobButtonPtr clearAllAnimation = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamClearAllAnimationLabel) );
    clearAllAnimation->setName(kTrackerUIParamClearAllAnimation);
    clearAllAnimation->setHintToolTip( tr(kTrackerUIParamClearAllAnimationHint) );
    clearAllAnimation->setEvaluateOnChange(false);
    clearAllAnimation->setSecretByDefault(true);
    clearAllAnimation->setInViewerContextCanHaveShortcut(true);
    clearAllAnimation->setIconLabel(NATRON_IMAGES_PATH "clearAnimation.png");
    trackingPage->addKnob(clearAllAnimation);
    _imp->ui->clearAllAnimationButton = clearAllAnimation;

    KnobButtonPtr clearBackwardAnim = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamClearAnimationBwLabel) );
    clearBackwardAnim->setName(kTrackerUIParamClearAnimationBw);
    clearBackwardAnim->setHintToolTip( tr(kTrackerUIParamClearAnimationBwHint) );
    clearBackwardAnim->setEvaluateOnChange(false);
    clearBackwardAnim->setSecretByDefault(true);
    clearBackwardAnim->setInViewerContextCanHaveShortcut(true);
    clearBackwardAnim->setIconLabel(NATRON_IMAGES_PATH "clearAnimationBw.png");
    trackingPage->addKnob(clearBackwardAnim);
    _imp->ui->clearBwAnimationButton = clearBackwardAnim;

    KnobButtonPtr clearForwardAnim = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamClearAnimationFwLabel) );
    clearForwardAnim->setName(kTrackerUIParamClearAnimationFw);
    clearForwardAnim->setHintToolTip( tr(kTrackerUIParamClearAnimationFwHint) );
    clearForwardAnim->setEvaluateOnChange(false);
    clearForwardAnim->setSecretByDefault(true);
    clearForwardAnim->setInViewerContextCanHaveShortcut(true);
    clearForwardAnim->setIconLabel(NATRON_IMAGES_PATH "clearAnimationFw.png");
    trackingPage->addKnob(clearForwardAnim);
    _imp->ui->clearFwAnimationButton = clearForwardAnim;

    KnobButtonPtr updateViewer = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRefreshViewerLabel) );
    updateViewer->setName(kTrackerUIParamRefreshViewer);
    updateViewer->setHintToolTip( tr(kTrackerUIParamRefreshViewerHint) );
    updateViewer->setEvaluateOnChange(false);
    updateViewer->setCheckable(true);
    updateViewer->setDefaultValue(true);
    updateViewer->setSecretByDefault(true);
    updateViewer->setInViewerContextCanHaveShortcut(true);
    updateViewer->setIconLabel(NATRON_IMAGES_PATH "refreshActive.png", true);
    updateViewer->setIconLabel(NATRON_IMAGES_PATH "refresh.png", false);
    trackingPage->addKnob(updateViewer);
    _imp->ui->updateViewerButton = updateViewer;

    KnobButtonPtr centerViewer = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamCenterViewerLabel) );
    centerViewer->setName(kTrackerUIParamCenterViewer);
    centerViewer->setHintToolTip( tr(kTrackerUIParamCenterViewerHint) );
    centerViewer->setEvaluateOnChange(false);
    centerViewer->setCheckable(true);
    centerViewer->setDefaultValue(false);
    centerViewer->setSecretByDefault(true);
    centerViewer->setInViewerContextCanHaveShortcut(true);
    centerViewer->setIconLabel(NATRON_IMAGES_PATH "centerOnTrack.png");
    trackingPage->addKnob(centerViewer);
    _imp->ui->centerViewerButton = centerViewer;

    KnobButtonPtr createKeyOnMove = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamCreateKeyOnMoveLabel) );
    createKeyOnMove->setName(kTrackerUIParamCreateKeyOnMove);
    createKeyOnMove->setHintToolTip( tr(kTrackerUIParamCreateKeyOnMoveHint) );
    createKeyOnMove->setEvaluateOnChange(false);
    createKeyOnMove->setCheckable(true);
    createKeyOnMove->setDefaultValue(true);
    createKeyOnMove->setSecretByDefault(true);
    createKeyOnMove->setInViewerContextCanHaveShortcut(true);
    createKeyOnMove->setIconLabel(NATRON_IMAGES_PATH "createKeyOnMoveOn.png", true);
    createKeyOnMove->setIconLabel(NATRON_IMAGES_PATH "createKeyOnMoveOff.png", false);
    trackingPage->addKnob(createKeyOnMove);
    _imp->ui->createKeyOnMoveButton = createKeyOnMove;

    KnobButtonPtr showError = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamShowErrorLabel) );
    showError->setName(kTrackerUIParamShowError);
    showError->setHintToolTip( tr(kTrackerUIParamShowErrorHint) );
    showError->setEvaluateOnChange(false);
    showError->setCheckable(true);
    showError->setDefaultValue(false);
    showError->setSecretByDefault(true);
    showError->setInViewerContextCanHaveShortcut(true);
    addOverlaySlaveParam(showError);
    showError->setIconLabel(NATRON_IMAGES_PATH "showTrackError.png", true);
    showError->setIconLabel(NATRON_IMAGES_PATH "hideTrackError.png", false);
    trackingPage->addKnob(showError);
    _imp->ui->showCorrelationButton = showError;


    KnobButtonPtr resetOffset = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamResetOffsetLabel) );
    resetOffset->setName(kTrackerUIParamResetOffset);
    resetOffset->setHintToolTip( tr(kTrackerUIParamResetOffsetHint) );
    resetOffset->setEvaluateOnChange(false);
    resetOffset->setSecretByDefault(true);
    resetOffset->setInViewerContextCanHaveShortcut(true);
    addOverlaySlaveParam(resetOffset);
    resetOffset->setIconLabel(NATRON_IMAGES_PATH "resetTrackOffset.png");
    trackingPage->addKnob(resetOffset);
    _imp->ui->resetOffsetButton = resetOffset;

    KnobButtonPtr resetTrack = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamResetTrackLabel) );
    resetTrack->setName(kTrackerUIParamResetTrack);
    resetTrack->setHintToolTip( tr(kTrackerUIParamResetTrackHint) );
    resetTrack->setEvaluateOnChange(false);
    resetTrack->setSecretByDefault(true);
    resetTrack->setInViewerContextCanHaveShortcut(true);
    addOverlaySlaveParam(resetTrack);
    resetTrack->setIconLabel(NATRON_IMAGES_PATH "restoreDefaultEnabled.png");
    trackingPage->addKnob(resetTrack);
    _imp->ui->resetTrackButton = resetTrack;

    KnobIntPtr magWindow = AppManager::createKnob<KnobInt>( this, tr(kTrackerUIParamMagWindowSizeLabel) );
    magWindow->setInViewerContextLabel(tr(kTrackerUIParamMagWindowSizeLabel));
    magWindow->setName(kTrackerUIParamMagWindowSize);
    magWindow->setHintToolTip( tr(kTrackerUIParamMagWindowSizeHint) );
    magWindow->setEvaluateOnChange(false);
    magWindow->setDefaultValue(200);
    magWindow->setMinimum(10);
    magWindow->setMaximum(10000);
    magWindow->disableSlider();
    addOverlaySlaveParam(magWindow);
    trackingPage->addKnob(magWindow);
    _imp->ui->magWindowPxSizeKnob = magWindow;


    addKnobToViewerUI(addMarker);
    addMarker->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(trackBw);
    trackBw->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(trackPrev);
    trackPrev->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(trackNext);
    trackNext->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(trackFw);
    trackFw->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(trackRange);
    trackRange->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(trackAllKeys);
    trackAllKeys->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(trackCurKey);
    trackCurKey->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(addKeyframe);
    addKeyframe->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(removeKeyframe);
    removeKeyframe->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(clearAllAnimation);
    clearAllAnimation->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(clearBackwardAnim);
    clearBackwardAnim->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(clearForwardAnim);
    clearForwardAnim->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(updateViewer);
    updateViewer->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(centerViewer);
    centerViewer->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(createKeyOnMove);
    addKnobToViewerUI(showError);
    showError->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(resetOffset);
    resetOffset->setInViewerContextItemSpacing(0);
    addKnobToViewerUI(resetTrack);
    resetTrack->setInViewerContextItemSpacing(NATRON_TRACKER_UI_BUTTONS_CATEGORIES_SPACING);
    addKnobToViewerUI(context->getDefaultMarkerPatternWinSizeKnob());
    addKnobToViewerUI(context->getDefaultMarkerSearchWinSizeKnob());
    addKnobToViewerUI(context->getDefaultMotionModelKnob());

    context->setUpdateViewer( updateViewer->getValue() );
    context->setCenterOnTrack( centerViewer->getValue() );

    QObject::connect( getNode().get(), SIGNAL(s_refreshPreviewsAfterProjectLoadRequested()), _imp->ui.get(), SLOT(rebuildMarkerTextures()) );
    QObject::connect( context.get(), SIGNAL(selectionChanged(int)), _imp->ui.get(), SLOT(onContextSelectionChanged(int)) );
    QObject::connect( context.get(), SIGNAL(keyframeSetOnTrack(TrackMarkerPtr,int)), _imp->ui.get(), SLOT(onKeyframeSetOnTrack(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(keyframeRemovedOnTrack(TrackMarkerPtr,int)), _imp->ui.get(), SLOT(onKeyframeRemovedOnTrack(TrackMarkerPtr,int)) );
    QObject::connect( context.get(), SIGNAL(allKeyframesRemovedOnTrack(TrackMarkerPtr)), _imp->ui.get(), SLOT(onAllKeyframesRemovedOnTrack(TrackMarkerPtr)) );
    QObject::connect( context.get(), SIGNAL(trackingFinished()), _imp->ui.get(), SLOT(onTrackingEnded()) );
    QObject::connect( context.get(), SIGNAL(trackingStarted(int)), _imp->ui.get(), SLOT(onTrackingStarted(int)) );

    // Right click menu
    KnobChoicePtr rightClickMenu = AppManager::createKnob<KnobChoice>( this, std::string(kTrackerUIParamRightClickMenu) );
    rightClickMenu->setSecretByDefault(true);
    rightClickMenu->setEvaluateOnChange(false);
    trackingPage->addKnob(rightClickMenu);
    _imp->ui->rightClickMenuKnob = rightClickMenu;
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRightClickMenuActionSelectAllTracksLabel) );
        action->setName(kTrackerUIParamRightClickMenuActionSelectAllTracks);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->selectAllTracksMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRightClickMenuActionRemoveTracksLabel) );
        action->setName(kTrackerUIParamRightClickMenuActionRemoveTracks);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->removeTracksMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRightClickMenuActionNudgeBottomLabel) );
        action->setName(kTrackerUIParamRightClickMenuActionNudgeBottom);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnBottomMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRightClickMenuActionNudgeLeftLabel) );
        action->setName(kTrackerUIParamRightClickMenuActionNudgeLeft);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnLeftMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRightClickMenuActionNudgeRightLabel) );
        action->setName(kTrackerUIParamRightClickMenuActionNudgeRight);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnRightMenuAction = action;
    }
    {
        KnobButtonPtr action = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamRightClickMenuActionNudgeTopLabel) );
        action->setName(kTrackerUIParamRightClickMenuActionNudgeTop);
        action->setSecretByDefault(true);
        action->setEvaluateOnChange(false);
        action->setInViewerContextCanHaveShortcut(true);
        addOverlaySlaveParam(action);
        trackingPage->addKnob(action);
        _imp->ui->nudgeTracksOnTopMenuAction = action;
    }


    // Track range dialog
    KnobGroupPtr trackRangeDialog = AppManager::createKnob<KnobGroup>( this, tr(kTrackerUIParamTrackRangeDialogLabel) );
    trackRangeDialog->setName(kTrackerUIParamTrackRangeDialog);
    trackRangeDialog->setSecretByDefault(true);
    trackRangeDialog->setEvaluateOnChange(false);
    trackRangeDialog->setDefaultValue(false);
    trackRangeDialog->setIsPersistent(false);
    trackRangeDialog->setAsDialog(true);
    trackingPage->addKnob(trackRangeDialog);
    _imp->ui->trackRangeDialogGroup = trackRangeDialog;

    KnobIntPtr trackRangeDialogFirstFrame = AppManager::createKnob<KnobInt>( this, tr(kTrackerUIParamTrackRangeDialogFirstFrameLabel) );
    trackRangeDialogFirstFrame->setName(kTrackerUIParamTrackRangeDialogFirstFrame);
    trackRangeDialogFirstFrame->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogFirstFrameHint) );
    trackRangeDialogFirstFrame->setSecretByDefault(true);
    trackRangeDialogFirstFrame->setEvaluateOnChange(false);
    trackRangeDialogFirstFrame->setAnimationEnabled(false);
    trackRangeDialogFirstFrame->setIsPersistent(false);
    trackRangeDialogFirstFrame->setDefaultValue(INT_MIN);
    trackRangeDialog->addKnob(trackRangeDialogFirstFrame);
    _imp->ui->trackRangeDialogFirstFrame = trackRangeDialogFirstFrame;

    KnobIntPtr trackRangeDialogLastFrame = AppManager::createKnob<KnobInt>( this, tr(kTrackerUIParamTrackRangeDialogLastFrameLabel) );
    trackRangeDialogLastFrame->setName(kTrackerUIParamTrackRangeDialogLastFrame);
    trackRangeDialogLastFrame->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogLastFrameHint) );
    trackRangeDialogLastFrame->setSecretByDefault(true);
    trackRangeDialogLastFrame->setEvaluateOnChange(false);
    trackRangeDialogLastFrame->setAnimationEnabled(false);
    trackRangeDialogLastFrame->setIsPersistent(false);
    trackRangeDialogLastFrame->setDefaultValue(INT_MIN);
    trackRangeDialog->addKnob(trackRangeDialogLastFrame);
    _imp->ui->trackRangeDialogLastFrame = trackRangeDialogLastFrame;

    KnobIntPtr trackRangeDialogFrameStep = AppManager::createKnob<KnobInt>( this, tr(kTrackerUIParamTrackRangeDialogStepLabel) );
    trackRangeDialogFrameStep->setName(kTrackerUIParamTrackRangeDialogStep);
    trackRangeDialogFrameStep->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogStepHint) );
    trackRangeDialogFrameStep->setSecretByDefault(true);
    trackRangeDialogFrameStep->setEvaluateOnChange(false);
    trackRangeDialogFrameStep->setAnimationEnabled(false);
    trackRangeDialogFrameStep->setIsPersistent(false);
    trackRangeDialogFrameStep->setDefaultValue(INT_MIN);
    trackRangeDialog->addKnob(trackRangeDialogFrameStep);
    _imp->ui->trackRangeDialogStep = trackRangeDialogFrameStep;

    KnobButtonPtr trackRangeDialogOkButton = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackRangeDialogOkButtonLabel) );
    trackRangeDialogOkButton->setName(kTrackerUIParamTrackRangeDialogOkButton);
    trackRangeDialogOkButton->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogOkButtonHint) );
    trackRangeDialogOkButton->setSecretByDefault(true);
    trackRangeDialogOkButton->setAddNewLine(false);
    trackRangeDialogOkButton->setEvaluateOnChange(false);
    trackRangeDialogOkButton->setSpacingBetweenItems(3);
    trackRangeDialogOkButton->setIsPersistent(false);
    trackRangeDialog->addKnob(trackRangeDialogOkButton);
    _imp->ui->trackRangeDialogOkButton = trackRangeDialogOkButton;

    KnobButtonPtr trackRangeDialogCancelButton = AppManager::createKnob<KnobButton>( this, tr(kTrackerUIParamTrackRangeDialogCancelButtonLabel) );
    trackRangeDialogCancelButton->setName(kTrackerUIParamTrackRangeDialogCancelButton);
    trackRangeDialogCancelButton->setHintToolTip( tr(kTrackerUIParamTrackRangeDialogCancelButtonHint) );
    trackRangeDialogCancelButton->setSecretByDefault(true);
    trackRangeDialogCancelButton->setEvaluateOnChange(false);
    trackRangeDialog->addKnob(trackRangeDialogCancelButton);
    _imp->ui->trackRangeDialogCancelButton = trackRangeDialogCancelButton;
} // TrackerNode::initializeKnobs

bool
TrackerNode::knobChanged(KnobI* k,
                         ValueChangedReasonEnum reason,
                         ViewSpec view,
                         double time,
                         bool originatedFromMainThread)
{
    TrackerContextPtr ctx = getNode()->getTrackerContext();

    if (!ctx) {
        return false;
    }

    ctx->onKnobsLoaded();
    
    bool ret = true;
    if ( k == _imp->ui->trackRangeDialogOkButton.lock().get() ) {
        int first = _imp->ui->trackRangeDialogFirstFrame.lock()->getValue();
        int last = _imp->ui->trackRangeDialogLastFrame.lock()->getValue();
        int step = _imp->ui->trackRangeDialogStep.lock()->getValue();
        TrackerContextPtr ctx = getNode()->getTrackerContext();
        if ( ctx->isCurrentlyTracking() ) {
            ctx->abortTracking();
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

        OverlaySupport* overlay = getCurrentViewportForOverlays();
        ctx->trackSelectedMarkers( startFrame, lastFrame, step,  overlay);
        _imp->ui->trackRangeDialogGroup.lock()->setValue(false);
    } else if ( k == _imp->ui->trackRangeDialogCancelButton.lock().get() ) {
        _imp->ui->trackRangeDialogGroup.lock()->setValue(false);
    } else if ( k == _imp->ui->selectAllTracksMenuAction.lock().get() ) {
        getNode()->getTrackerContext()->selectAll(TrackerContext::eTrackSelectionInternal);
    } else if ( k == _imp->ui->removeTracksMenuAction.lock().get() ) {
        std::list<TrackMarkerPtr> markers;
        TrackerContextPtr context = getNode()->getTrackerContext();
        context->getSelectedMarkers(&markers);
        if ( !markers.empty() ) {
            pushUndoCommand( new RemoveTracksCommand( markers, context ) );
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnTopMenuAction.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(0, 1) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnRightMenuAction.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(1, 0) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnLeftMenuAction.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(-1, 0) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->nudgeTracksOnBottomMenuAction.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        if ( !_imp->ui->nudgeSelectedTracks(0, -1) ) {
            return false;
        }
    } else if ( ( k == _imp->ui->stopTrackingButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onStopButtonClicked();
    } else if ( ( k == _imp->ui->trackBwButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackBwClicked();
    } else if ( ( k == _imp->ui->trackPrevButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackPrevClicked();
    } else if ( ( k == _imp->ui->trackFwButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackFwClicked();
    } else if ( ( k == _imp->ui->trackNextButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackNextClicked();
    } else if ( ( k == _imp->ui->trackRangeButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackRangeClicked();
    } else if ( ( k == _imp->ui->trackAllKeyframesButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackAllKeyframesClicked();
    } else if ( ( k == _imp->ui->trackCurrentKeyframeButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onTrackCurrentKeyframeClicked();
    } else if ( ( k == _imp->ui->clearAllAnimationButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onClearAllAnimationClicked();
    } else if ( ( k == _imp->ui->clearBwAnimationButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onClearBwAnimationClicked();
    } else if ( ( k == _imp->ui->clearFwAnimationButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onClearFwAnimationClicked();
    } else if ( ( k == _imp->ui->updateViewerButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onUpdateViewerClicked( _imp->ui->updateViewerButton.lock()->getValue() );
    } else if ( ( k == _imp->ui->centerViewerButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onCenterViewerButtonClicked( _imp->ui->centerViewerButton.lock()->getValue() );
    } else if ( ( k == _imp->ui->setKeyFrameButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onSetKeyframeButtonClicked();
    } else if ( ( k == _imp->ui->removeKeyFrameButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onRemoveKeyframeButtonClicked();
    } else if ( ( k == _imp->ui->resetOffsetButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onResetOffsetButtonClicked();
    } else if ( ( k == _imp->ui->resetTrackButton.lock().get() ) && (reason == eValueChangedReasonUserEdited) ) {
        _imp->ui->onResetTrackButtonClicked();
    } else if ( k == _imp->ui->addTrackButton.lock().get() ) {
        _imp->ui->clickToAddTrackEnabled = _imp->ui->addTrackButton.lock()->getValue();
    } else {
        ret = false;
    }
    if (!ret) {
        ret |= ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
    }

    return ret;
} // TrackerNode::knobChanged

void
TrackerNode::onKnobsLoaded()
{
    TrackerContextPtr ctx = getNode()->getTrackerContext();

    if (!ctx) {
        return;
    }
    ctx->onKnobsLoaded();

    ctx->setUpdateViewer( _imp->ui->updateViewerButton.lock()->getValue() );
    ctx->setCenterOnTrack( _imp->ui->centerViewerButton.lock()->getValue() );
}

void
TrackerNode::evaluate(bool isSignificant, bool refreshMetadata)
{
    NodeGroup::evaluate(isSignificant, refreshMetadata);
    _imp->ui->refreshSelectedMarkerTexture();
}

void
TrackerNode::onInputChanged(int inputNb)
{
    TrackerContextPtr ctx = getNode()->getTrackerContext();

    ctx->inputChanged(inputNb);

    _imp->ui->refreshSelectedMarkerTexture();
}

struct CenterPointDisplayInfo
{
    double x;
    double y;
    double err;
    bool isValid;

    CenterPointDisplayInfo()
    : x(0)
    , y(0)
    , err(0)
    , isValid(false)
    {

    }
};

typedef std::map<double, CenterPointDisplayInfo> CenterPointsMap;

void
TrackerNode::drawOverlay(double time,
                         const RenderScale & /*renderScale*/,
                         ViewIdx /*view*/)
{
    double pixelScaleX, pixelScaleY;
    OverlaySupport* overlay = getCurrentViewportForOverlays();

    assert(overlay);
    overlay->getPixelScale(pixelScaleX, pixelScaleY);
    double viewportSize[2];
    overlay->getViewportSize(viewportSize[0], viewportSize[1]);
    double screenPixelRatio = overlay->getScreenPixelRatio();

    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);
        double markerColor[3];
        if ( !getNode()->getOverlayColor(&markerColor[0], &markerColor[1], &markerColor[2]) ) {
            markerColor[0] = markerColor[1] = markerColor[2] = 0.8;
        }

        std::vector<TrackMarkerPtr> allMarkers;
        std::list<TrackMarkerPtr> selectedMarkers;
        TrackerContextPtr context = getNode()->getTrackerContext();
        context->getSelectedMarkers(&selectedMarkers);
        context->getAllMarkers(&allMarkers);

        bool trackingPageSecret = context->getTrackingPageKnob()->getIsSecret();
        bool showErrorColor = _imp->ui->showCorrelationButton.lock()->getValue();
        TrackMarkerPtr selectedMarker = _imp->ui->selectedMarker.lock();
        bool selectedFound = false;
        Point selectedCenter;
        Point selectedPtnTopLeft;
        Point selectedPtnTopRight;
        Point selectedPtnBtmRight;
        Point selectedPtnBtmLeft;
        Point selectedOffset;
        Point selectedSearchBtmLeft;
        Point selectedSearchTopRight;

        for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
            bool isEnabled = (*it)->isEnabled(time);

            double thisMarkerColor[3];
            if (!isEnabled) {
                for (int i = 0; i < 3; ++i) {
                    thisMarkerColor[i] = markerColor[i] / 2.;
                }
            } else {
                for (int i = 0; i < 3; ++i) {
                    thisMarkerColor[i] = markerColor[i];
                }
            }

            bool isHoverMarker = *it == _imp->ui->hoverMarker;
            bool isDraggedMarker = *it == _imp->ui->interactMarker;
            bool isHoverOrDraggedMarker = isHoverMarker || isDraggedMarker;
            std::list<TrackMarkerPtr>::iterator foundSelected = std::find(selectedMarkers.begin(), selectedMarkers.end(), *it);
            bool isSelected = foundSelected != selectedMarkers.end();
            KnobDoublePtr centerKnob = (*it)->getCenterKnob();
            KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
            KnobDoublePtr errorKnob = (*it)->getErrorKnob();
            KnobDoublePtr ptnTopLeft = (*it)->getPatternTopLeftKnob();
            KnobDoublePtr ptnTopRight = (*it)->getPatternTopRightKnob();
            KnobDoublePtr ptnBtmRight = (*it)->getPatternBtmRightKnob();
            KnobDoublePtr ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            KnobDoublePtr searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            KnobDoublePtr searchWndTopRight = (*it)->getSearchWindowTopRightKnob();

            // When the tracking page is secret, still show markers, but as if deselected
            if (!isSelected || trackingPageSecret) {
                ///Draw a custom interact, indicating the track isn't selected
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);

                for (int l = 0; l < 2; ++l) {
                    // shadow (uses GL_PROJECTION)
                    glMatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                    glMatrixMode(GL_MODELVIEW);

                    if (l == 0) {
                        glColor4d(0., 0., 0., 1.);
                    } else {
                        glColor4f(thisMarkerColor[0], thisMarkerColor[1], thisMarkerColor[2], 1.);
                    }


                    double x = centerKnob->getValueAtTime(time, 0);
                    double y = centerKnob->getValueAtTime(time, 1);

                    glPointSize(POINT_SIZE * screenPixelRatio);
                    glBegin(GL_POINTS);
                    glVertex2d(x, y);
                    glEnd();

                    glLineWidth(1.5f * screenPixelRatio);
                    glBegin(GL_LINES);
                    glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                    glVertex2d(x + CROSS_SIZE * pixelScaleX, y);


                    glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                    glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                    glEnd();
                }
                glPointSize(1. * screenPixelRatio);
            } else { // if (isSelected) {
                glEnable(GL_LINE_SMOOTH);
                glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                GLdouble projection[16];
                glGetDoublev( GL_PROJECTION_MATRIX, projection);
                OfxPointD shadow; // how much to translate GL_PROJECTION to get exactly one pixel on screen
                shadow.x = 2. / (projection[0] * viewportSize[0]);
                shadow.y = 2. / (projection[5] * viewportSize[1]);

                const QPointF center( centerKnob->getValueAtTime(time, 0),
                                      centerKnob->getValueAtTime(time, 1) );
                const QPointF offset( offsetKnob->getValueAtTime(time, 0),
                                      offsetKnob->getValueAtTime(time, 1) );
                const QPointF topLeft( ptnTopLeft->getValueAtTime(time, 0) + offset.x() + center.x(),
                                       ptnTopLeft->getValueAtTime(time, 1) + offset.y() + center.y() );
                const QPointF topRight( ptnTopRight->getValueAtTime(time, 0) + offset.x() + center.x(),
                                        ptnTopRight->getValueAtTime(time, 1) + offset.y() + center.y() );
                const QPointF btmRight( ptnBtmRight->getValueAtTime(time, 0) + offset.x() + center.x(),
                                        ptnBtmRight->getValueAtTime(time, 1) + offset.y() + center.y() );
                const QPointF btmLeft( ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + center.x(),
                                       ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + center.y() );
                const double searchLeft   = searchWndBtmLeft->getValueAtTime(time, 0)  + offset.x() + center.x();
                const double searchBottom = searchWndBtmLeft->getValueAtTime(time, 1)  + offset.y() + center.y();
                const double searchRight  = searchWndTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
                const double searchTop    = searchWndTopRight->getValueAtTime(time, 1) + offset.y() + center.y();
                const double searchMidX   = (searchLeft + searchRight) / 2;
                const double searchMidY   = (searchTop + searchBottom) / 2;

                if (selectedMarker == *it) {
                    selectedCenter.x = center.x();
                    selectedCenter.y = center.y();
                    selectedOffset.x = offset.x();
                    selectedOffset.y = offset.y();
                    selectedPtnBtmLeft.x = btmLeft.x();
                    selectedPtnBtmLeft.y = btmLeft.y();
                    selectedPtnBtmRight.x = btmRight.x();
                    selectedPtnBtmRight.y = btmRight.y();
                    selectedPtnTopRight.x = topRight.x();
                    selectedPtnTopRight.y = topRight.y();
                    selectedPtnTopLeft.x = topLeft.x();
                    selectedPtnTopLeft.y = topLeft.y();
                    selectedSearchBtmLeft.x = searchLeft;
                    selectedSearchBtmLeft.y = searchBottom;

                    selectedSearchTopRight.x = searchRight;
                    selectedSearchTopRight.y = searchTop;
                    selectedFound = true;
                }

                const QPointF innerMidLeft( (btmLeft + topLeft) / 2 );
                const QPointF innerMidTop( (topLeft + topRight) / 2 );
                const QPointF innerMidRight( (btmRight + topRight) / 2 );
                const QPointF innerMidBtm( (btmLeft + btmRight) / 2 );
                const QPointF outterMidLeft(searchLeft, searchMidY);
                const QPointF outterMidTop(searchMidX, searchTop);
                const QPointF outterMidRight(searchRight, searchMidY);
                const QPointF outterMidBtm(searchMidX, searchBottom);
                const QPointF handleSize( HANDLE_SIZE * pixelScaleX, HANDLE_SIZE * pixelScaleY );
                const QPointF innerMidLeftExt = TrackerNodeInteract::computeMidPointExtent(topLeft, btmLeft, innerMidLeft, handleSize);
                const QPointF innerMidRightExt = TrackerNodeInteract::computeMidPointExtent(btmRight, topRight, innerMidRight, handleSize);
                const QPointF innerMidTopExt = TrackerNodeInteract::computeMidPointExtent(topRight, topLeft, innerMidTop, handleSize);
                const QPointF innerMidBtmExt = TrackerNodeInteract::computeMidPointExtent(btmLeft, btmRight, innerMidBtm, handleSize);
                const QPointF searchTopLeft(searchLeft, searchTop);
                const QPointF searchTopRight(searchRight, searchTop);
                const QPointF searchBtmRight(searchRight, searchBottom);
                const QPointF searchBtmLeft(searchLeft, searchBottom);
                const QPointF searchTopMid(searchMidX, searchTop);
                const QPointF searchRightMid(searchRight, searchMidY);
                const QPointF searchLeftMid(searchLeft, searchMidY);
                const QPointF searchBtmMid(searchMidX, searchBottom);
                const QPointF outterMidLeftExt  = TrackerNodeInteract::computeMidPointExtent(searchTopLeft,  searchBtmLeft,  outterMidLeft,  handleSize);
                const QPointF outterMidRightExt = TrackerNodeInteract::computeMidPointExtent(searchBtmRight, searchTopRight, outterMidRight, handleSize);
                const QPointF outterMidTopExt   = TrackerNodeInteract::computeMidPointExtent(searchTopRight, searchTopLeft,  outterMidTop,   handleSize);
                const QPointF outterMidBtmExt   = TrackerNodeInteract::computeMidPointExtent(searchBtmLeft,  searchBtmRight, outterMidBtm,   handleSize);
                std::string name = (*it)->getLabel();
                if (!isEnabled) {
                    name += ' ';
                    name += tr("(disabled)").toStdString();
                }
                CenterPointsMap centerPoints;
                CurvePtr xCurve = centerKnob->getCurve(ViewSpec::current(), 0);
                CurvePtr yCurve = centerKnob->getCurve(ViewSpec::current(), 1);
                CurvePtr errorCurve = errorKnob->getCurve(ViewSpec::current(), 0);

                {
                    KeyFrameSet xKeyframes = xCurve->getKeyFrames_mt_safe();
                    KeyFrameSet yKeyframes = yCurve->getKeyFrames_mt_safe();
                    KeyFrameSet errKeyframes;
                    if (showErrorColor) {
                        errKeyframes = errorCurve->getKeyFrames_mt_safe();
                    }

                    // Try first to do an optimized case in O(N) where we assume that all 3 curves have the same keyframes
                    // at the same time
                    KeyFrameSet remainingXKeys,remainingYKeys, remainingErrKeys;
                    if (xKeyframes.size() == yKeyframes.size() && (!showErrorColor || xKeyframes.size() == errKeyframes.size())) {
                        KeyFrameSet::iterator errIt = errKeyframes.begin();
                        KeyFrameSet::iterator xIt = xKeyframes.begin();
                        KeyFrameSet::iterator yIt = yKeyframes.begin();

                        bool setsHaveDifferentKeyTimes = false;
                        while (xIt!=xKeyframes.end()) {
                            if (xIt->getTime() != yIt->getTime() || (showErrorColor && xIt->getTime() != errIt->getTime())) {
                                setsHaveDifferentKeyTimes = true;
                                break;
                            }
                            CenterPointDisplayInfo& p = centerPoints[xIt->getTime()];
                            p.x = xIt->getValue();
                            p.y = yIt->getValue();
                            if ( showErrorColor ) {
                                p.err = errIt->getValue();
                            }
                            p.isValid = true;

                            ++xIt;
                            ++yIt;
                            if (showErrorColor) {
                                ++errIt;
                            }

                        }
                        if (setsHaveDifferentKeyTimes) {
                            remainingXKeys.insert(xIt, xKeyframes.end());
                            remainingYKeys.insert(yIt, yKeyframes.end());
                            if (showErrorColor) {
                                remainingErrKeys.insert(errIt, errKeyframes.end());
                            }
                        }
                    } else {
                        remainingXKeys = xKeyframes;
                        remainingYKeys = yKeyframes;
                        if (showErrorColor) {
                            remainingErrKeys = errKeyframes;
                        }
                    }
                    for (KeyFrameSet::iterator xIt = remainingXKeys.begin(); xIt != remainingXKeys.end(); ++xIt) {
                        CenterPointDisplayInfo& p = centerPoints[xIt->getTime()];
                        p.x = xIt->getValue();
                        p.isValid = false;
                    }
                    for (KeyFrameSet::iterator yIt = remainingYKeys.begin(); yIt != remainingYKeys.end(); ++yIt) {
                        CenterPointsMap::iterator foundPoint = centerPoints.find(yIt->getTime());
                        if (foundPoint == centerPoints.end()) {
                            continue;
                        }
                        foundPoint->second.y = yIt->getValue();
                        if (!showErrorColor) {
                            foundPoint->second.isValid = true;
                        }
                    }
                    for (KeyFrameSet::iterator errIt = remainingErrKeys.begin(); errIt != remainingErrKeys.end(); ++errIt) {
                        CenterPointsMap::iterator foundPoint = centerPoints.find(errIt->getTime());
                        if (foundPoint == centerPoints.end()) {
                            continue;
                        }
                        foundPoint->second.err = errIt->getValue();
                        foundPoint->second.isValid = true;
                    }

                }



                for (int l = 0; l < 2; ++l) {
                    // shadow (uses GL_PROJECTION)
                    glMatrixMode(GL_PROJECTION);
                    int direction = (l == 0) ? 1 : -1;
                    // translate (1,-1) pixels
                    glTranslated(direction * shadow.x, -direction * shadow.y, 0);
                    glMatrixMode(GL_MODELVIEW);

                    ///Draw center position
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                    glLineWidth(1.5 * screenPixelRatio);
                    glBegin(GL_LINE_STRIP);
                    glColor3f(0.5 * l, 0.5 * l, 0.5 * l);
                    for (CenterPointsMap::iterator it = centerPoints.begin(); it != centerPoints.end(); ++it) {
                        if (it->second.isValid) {
                            glVertex2d(it->second.x, it->second.y);
                        }
                    }
                    glEnd();
                    glDisable(GL_LINE_SMOOTH);

                    glEnable(GL_POINT_SMOOTH);
                    glPointSize(POINT_SIZE * screenPixelRatio);
                    glBegin(GL_POINTS);
                    if (!showErrorColor) {
                        glColor3f(0.5 * l, 0.5 * l, 0.5 * l);
                    }

                    for (CenterPointsMap::iterator it2 = centerPoints.begin(); it2 != centerPoints.end(); ++it2) {
                        if (!it2->second.isValid) {
                            continue;
                        }
                        if (showErrorColor) {
                            if (l != 0) {
                                /*
                                   Clamp the correlation to [CORRELATION_ERROR_MIN, 1] then
                                   Map the correlation to [0, 0.33] since 0 is Red for HSV and 0.33 is Green.
                                   Also clamp to the interval if the correlation is higher, and reverse.
                                 */

                                double error = boost::algorithm::clamp(it2->second.err, 0., CORRELATION_ERROR_MAX_DISPLAY);
                                double mappedError = 0.33 - 0.33 * error / CORRELATION_ERROR_MAX_DISPLAY;
                                float r, g, b;
                                Color::hsv_to_rgb(mappedError, 1, 1, &r, &g, &b);
                                glColor3f(r, g, b);
                            } else {
                                glColor3f(0., 0., 0.);
                            }
                        }
                        glVertex2d(it2->second.x, it2->second.y);
                    }
                    glEnd();
                    glDisable(GL_POINT_SMOOTH);


                    glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    glLineWidth(1.5 * screenPixelRatio);
                    glBegin(GL_LINE_LOOP);
                    glVertex2d( topLeft.x(), topLeft.y() );
                    glVertex2d( topRight.x(), topRight.y() );
                    glVertex2d( btmRight.x(), btmRight.y() );
                    glVertex2d( btmLeft.x(), btmLeft.y() );
                    glEnd();

                    glLineWidth(1.5f * screenPixelRatio);
                    glBegin(GL_LINE_LOOP);
                    glVertex2d( searchTopLeft.x(), searchTopLeft.y() );
                    glVertex2d( searchTopRight.x(), searchTopRight.y() );
                    glVertex2d( searchBtmRight.x(), searchBtmRight.y() );
                    glVertex2d( searchBtmLeft.x(), searchBtmRight.y() );
                    glEnd();

                    glPointSize(POINT_SIZE * screenPixelRatio);
                    glBegin(GL_POINTS);

                    ///draw center
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringCenter) || (_imp->ui->eventState == eMouseStateDraggingCenter) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( center.x(), center.y() );

                    if ( (offset.x() != 0) || (offset.y() != 0) ) {
                        glVertex2d( center.x() + offset.x(), center.y() + offset.y() );
                    }

                    //////DRAWING INNER POINTS
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerBtmLeft) || (_imp->ui->eventState == eMouseStateDraggingInnerBtmLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( btmLeft.x(), btmLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->ui->eventState == eMouseStateDraggingInnerBtmMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( innerMidBtm.x(), innerMidBtm.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerBtmRight) || (_imp->ui->eventState == eMouseStateDraggingInnerBtmRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( btmRight.x(), btmRight.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->ui->eventState == eMouseStateDraggingInnerMidLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( innerMidLeft.x(), innerMidLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->ui->eventState == eMouseStateDraggingInnerMidRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( innerMidRight.x(), innerMidRight.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerTopLeft) || (_imp->ui->eventState == eMouseStateDraggingInnerTopLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( topLeft.x(), topLeft.y() );
                    }

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->ui->eventState == eMouseStateDraggingInnerTopMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( innerMidTop.x(), innerMidTop.y() );
                    }

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerTopRight) || (_imp->ui->eventState == eMouseStateDraggingInnerTopRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( topRight.x(), topRight.y() );
                    }


                    //////DRAWING OUTER POINTS

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterBtmLeft) || (_imp->ui->eventState == eMouseStateDraggingOuterBtmLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( searchBtmLeft.x(), searchBtmLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->ui->eventState == eMouseStateDraggingOuterBtmMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( outterMidBtm.x(), outterMidBtm.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterBtmRight) || (_imp->ui->eventState == eMouseStateDraggingOuterBtmRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( searchBtmRight.x(), searchBtmRight.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->ui->eventState == eMouseStateDraggingOuterMidLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( outterMidLeft.x(), outterMidLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->ui->eventState == eMouseStateDraggingOuterMidRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( outterMidRight.x(), outterMidRight.y() );
                    }

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterTopLeft) || (_imp->ui->eventState == eMouseStateDraggingOuterTopLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( searchTopLeft.x(), searchTopLeft.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->ui->eventState == eMouseStateDraggingOuterTopMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( outterMidTop.x(), outterMidTop.y() );
                    }
                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterTopRight) || (_imp->ui->eventState == eMouseStateDraggingOuterTopRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                        glVertex2d( searchTopRight.x(), searchTopRight.y() );
                    }

                    glEnd();

                    if ( (offset.x() != 0) || (offset.y() != 0) ) {
                        glLineWidth(1.5f * screenPixelRatio);
                        glBegin(GL_LINES);
                        glColor3f( (float)thisMarkerColor[0] * l * 0.5, (float)thisMarkerColor[1] * l * 0.5, (float)thisMarkerColor[2] * l * 0.5 );
                        glVertex2d( center.x(), center.y() );
                        glVertex2d( center.x() + offset.x(), center.y() + offset.y() );
                        glEnd();
                    }

                    ///now show small lines at handle positions
                    glLineWidth(1.5f * screenPixelRatio);
                    glBegin(GL_LINES);

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->ui->eventState == eMouseStateDraggingInnerMidLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( innerMidLeft.x(), innerMidLeft.y() );
                    glVertex2d( innerMidLeftExt.x(), innerMidLeftExt.y() );

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->ui->eventState == eMouseStateDraggingInnerTopMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( innerMidTop.x(), innerMidTop.y() );
                    glVertex2d( innerMidTopExt.x(), innerMidTopExt.y() );

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->ui->eventState == eMouseStateDraggingInnerMidRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( innerMidRight.x(), innerMidRight.y() );
                    glVertex2d( innerMidRightExt.x(), innerMidRightExt.y() );

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->ui->eventState == eMouseStateDraggingInnerBtmMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( innerMidBtm.x(), innerMidBtm.y() );
                    glVertex2d( innerMidBtmExt.x(), innerMidBtmExt.y() );

                    //////DRAWING OUTER HANDLES

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->ui->eventState == eMouseStateDraggingOuterMidLeft) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( outterMidLeft.x(), outterMidLeft.y() );
                    glVertex2d( outterMidLeftExt.x(), outterMidLeftExt.y() );

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->ui->eventState == eMouseStateDraggingOuterTopMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( outterMidTop.x(), outterMidTop.y() );
                    glVertex2d( outterMidTopExt.x(), outterMidTopExt.y() );

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->ui->eventState == eMouseStateDraggingOuterMidRight) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( outterMidRight.x(), outterMidRight.y() );
                    glVertex2d( outterMidRightExt.x(), outterMidRightExt.y() );

                    if ( isHoverOrDraggedMarker && ( (_imp->ui->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->ui->eventState == eMouseStateDraggingOuterBtmMid) ) ) {
                        glColor3f(0.f * l, 1.f * l, 0.f * l);
                    } else {
                        glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );
                    }
                    glVertex2d( outterMidBtm.x(), outterMidBtm.y() );
                    glVertex2d( outterMidBtmExt.x(), outterMidBtmExt.y() );
                    glEnd();

                    glColor3f( (float)thisMarkerColor[0] * l, (float)thisMarkerColor[1] * l, (float)thisMarkerColor[2] * l );

                    overlay->renderText( center.x(), center.y(), name, markerColor[0], markerColor[1], markerColor[2]);
                } // for (int l = 0; l < 2; ++l) {
            } // if (!isSelected) {
        } // for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

        if (_imp->ui->showMarkerTexture && selectedFound) {
            _imp->ui->drawSelectedMarkerTexture(std::make_pair(pixelScaleX, pixelScaleY), _imp->ui->selectedMarkerTextureTime, selectedCenter, selectedOffset, selectedPtnTopLeft, selectedPtnTopRight, selectedPtnBtmRight, selectedPtnBtmLeft, selectedSearchBtmLeft, selectedSearchTopRight);
        }
        // context->drawInternalNodesOverlay( time, renderScale, view, overlay);


        if (_imp->ui->clickToAddTrackEnabled) {
            ///draw a square of 20px around the mouse cursor
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(1.5 * screenPixelRatio);
            //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)

            const double addTrackSize = TO_DPIX(ADDTRACK_SIZE);

            for (int l = 0; l < 2; ++l) {
                // shadow (uses GL_PROJECTION)
                glMatrixMode(GL_PROJECTION);
                int direction = (l == 0) ? 1 : -1;
                // translate (1,-1) pixels
                glTranslated(direction * pixelScaleX / 256, -direction * pixelScaleY / 256, 0);
                glMatrixMode(GL_MODELVIEW);

                if (l == 0) {
                    glColor4d(0., 0., 0., 0.8);
                } else {
                    glColor4d(0., 1., 0., 0.8);
                }

                glLineWidth(1.5 * screenPixelRatio);
                glBegin(GL_LINE_LOOP);
                glVertex2d(_imp->ui->lastMousePos.x() - addTrackSize * 2 * pixelScaleX, _imp->ui->lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->ui->lastMousePos.x() - addTrackSize * 2 * pixelScaleX, _imp->ui->lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->ui->lastMousePos.x() + addTrackSize * 2 * pixelScaleX, _imp->ui->lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->ui->lastMousePos.x() + addTrackSize * 2 * pixelScaleX, _imp->ui->lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                glEnd();

                ///draw a cross at the cursor position
                glLineWidth(1.5f * screenPixelRatio);
                glBegin(GL_LINES);
                glVertex2d( _imp->ui->lastMousePos.x() - addTrackSize * pixelScaleX, _imp->ui->lastMousePos.y() );
                glVertex2d( _imp->ui->lastMousePos.x() + addTrackSize * pixelScaleX, _imp->ui->lastMousePos.y() );
                glVertex2d(_imp->ui->lastMousePos.x(), _imp->ui->lastMousePos.y() - addTrackSize * pixelScaleY);
                glVertex2d(_imp->ui->lastMousePos.x(), _imp->ui->lastMousePos.y() + addTrackSize * pixelScaleY);
                glEnd();
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
} // drawOverlay

bool
TrackerNode::onOverlayPenDown(double time,
                              const RenderScale & /*renderScale*/,
                              ViewIdx view,
                              const QPointF & viewportPos,
                              const QPointF & pos,
                              double /*pressure*/,
                              double /*timestamp*/,
                              PenType /*pen*/)
{
    std::pair<double, double> pixelScale;
    OverlaySupport* overlay = getCurrentViewportForOverlays();

    assert(overlay);
    overlay->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;
    TrackerContextPtr context = getNode()->getTrackerContext();
    /*if ( context->onOverlayPenDownInternalNodes( time, renderScale, view, viewportPos, pos, pressure, timestamp, pen, overlay ) ) {
        return true;
       }*/
    std::vector<TrackMarkerPtr> allMarkers;
    context->getAllMarkers(&allMarkers);

    bool trackingPageSecret = context->getTrackingPageKnob()->getIsSecret();
    for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
        if (!(*it)->isEnabled(time) || trackingPageSecret) {
            continue;
        }

        bool isSelected = context->isMarkerSelected( (*it) );
        KnobDoublePtr centerKnob = (*it)->getCenterKnob();
        KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
        KnobDoublePtr ptnTopLeft = (*it)->getPatternTopLeftKnob();
        KnobDoublePtr ptnTopRight = (*it)->getPatternTopRightKnob();
        KnobDoublePtr ptnBtmRight = (*it)->getPatternBtmRightKnob();
        KnobDoublePtr ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
        KnobDoublePtr searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
        KnobDoublePtr searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
        QPointF centerPoint;
        centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
        centerPoint.ry() = centerKnob->getValueAtTime(time, 1);

        QPointF offset;
        offset.rx() = offsetKnob->getValueAtTime(time, 0);
        offset.ry() = offsetKnob->getValueAtTime(time, 1);

        if ( _imp->ui->isNearbyPoint(centerKnob, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time) ) {
            if (_imp->ui->controlDown > 0) {
                _imp->ui->eventState = eMouseStateDraggingOffset;
            } else {
                _imp->ui->eventState = eMouseStateDraggingCenter;
            }
            _imp->ui->interactMarker = *it;
            didSomething = true;
        } else if ( ( (offset.x() != 0) || (offset.y() != 0) ) && _imp->ui->isNearbyPoint(QPointF( centerPoint.x() + offset.x(), centerPoint.y() + offset.y() ), viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
            _imp->ui->eventState = eMouseStateDraggingOffset;
            _imp->ui->interactMarker = *it;
            didSomething = true;
        }

        if (!didSomething && isSelected) {
            QPointF topLeft, topRight, btmRight, btmLeft;
            topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
            topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();

            topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
            topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();

            btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
            btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + centerPoint.y();

            btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + centerPoint.x();
            btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + centerPoint.y();

            QPointF midTop, midRight, midBtm, midLeft;
            midTop.rx() = ( topLeft.x() + topRight.x() ) / 2.;
            midTop.ry() = ( topLeft.y() + topRight.y() ) / 2.;

            midRight.rx() = ( btmRight.x() + topRight.x() ) / 2.;
            midRight.ry() = ( btmRight.y() + topRight.y() ) / 2.;

            midBtm.rx() = ( btmRight.x() + btmLeft.x() ) / 2.;
            midBtm.ry() = ( btmRight.y() + btmLeft.y() ) / 2.;

            midLeft.rx() = ( topLeft.x() + btmLeft.x() ) / 2.;
            midLeft.ry() = ( topLeft.y() + btmLeft.y() ) / 2.;

            if ( isSelected && _imp->ui->isNearbyPoint(topLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerTopLeft;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(topRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerTopRight;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(btmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerBtmRight;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(btmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerBtmLeft;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midTop, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerTopMid;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerMidRight;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerMidLeft;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midBtm, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingInnerBtmMid;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            }
        }

        if (!didSomething && isSelected) {
            ///Test search window
            const double searchLeft = searchWndBtmLeft->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
            const double searchRight = searchWndTopRight->getValueAtTime(time, 0) + centerPoint.x() + offset.x();
            const double searchTop = searchWndTopRight->getValueAtTime(time, 1) + centerPoint.y() + offset.y();
            const double searchBottom = searchWndBtmLeft->getValueAtTime(time, 1) + +centerPoint.y() + offset.y();
            const double searchMidX = (searchLeft + searchRight) / 2.;
            const double searchMidY = (searchTop + searchBottom) / 2.;
            const QPointF searchTopLeft(searchLeft, searchTop);
            const QPointF searchTopRight(searchRight, searchTop);
            const QPointF searchBtmRight(searchRight, searchBottom);
            const QPointF searchBtmLeft(searchLeft, searchBottom);
            const QPointF searchTopMid(searchMidX, searchTop);
            const QPointF searchRightMid(searchRight, searchMidY);
            const QPointF searchLeftMid(searchLeft, searchMidY);
            const QPointF searchBtmMid(searchMidX, searchBottom);

            if ( _imp->ui->isNearbyPoint(searchTopLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterTopLeft;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchTopRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterTopRight;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchBtmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterBtmRight;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchBtmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterBtmLeft;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchTopMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterTopMid;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchBtmMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterBtmMid;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchLeftMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterMidLeft;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            } else if ( _imp->ui->isNearbyPoint(searchRightMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->eventState = eMouseStateDraggingOuterMidRight;
                _imp->ui->interactMarker = *it;
                didSomething = true;
            }
        }

        //If we hit the interact, make sure it is selected
        if (_imp->ui->interactMarker) {
            if (!isSelected) {
                context->beginEditSelection(TrackerContext::eTrackSelectionViewer);
                if (!_imp->ui->shiftDown) {
                    context->clearSelection(TrackerContext::eTrackSelectionViewer);
                }
                context->addTrackToSelection(_imp->ui->interactMarker, TrackerContext::eTrackSelectionViewer);
                context->endEditSelection(TrackerContext::eTrackSelectionViewer);
            }
        }

        if (didSomething) {
            break;
        }
    } // for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

    if (_imp->ui->clickToAddTrackEnabled && !didSomething && !trackingPageSecret) {
        TrackMarkerPtr marker = context->createMarker();
        KnobDoublePtr centerKnob = marker->getCenterKnob();
        centerKnob->setValuesAtTime(time, pos.x(), pos.y(), view, eValueChangedReasonNatronInternalEdited);
        if ( _imp->ui->createKeyOnMoveButton.lock()->getValue() ) {
            marker->setUserKeyframe(time);
        }
        pushUndoCommand( new AddTrackCommand(marker, context) );
        _imp->ui->refreshSelectedMarkerTexture();
        didSomething = true;
    }

    if ( !didSomething && _imp->ui->showMarkerTexture && _imp->ui->selectedMarkerTexture && _imp->ui->isNearbySelectedMarkerTextureResizeAnchor(pos) ) {
        _imp->ui->eventState = eMouseStateDraggingSelectedMarkerResizeAnchor;
        didSomething = true;
    }

    if ( !didSomething && _imp->ui->showMarkerTexture && _imp->ui->selectedMarkerTexture  && _imp->ui->isInsideSelectedMarkerTexture(pos) ) {
        if (_imp->ui->shiftDown) {
            _imp->ui->eventState = eMouseStateScalingSelectedMarker;
        } else {
            _imp->ui->eventState = eMouseStateDraggingSelectedMarker;
        }
        _imp->ui->interactMarker = _imp->ui->selectedMarker.lock();
        didSomething = true;
    }

    if (!didSomething) {
        int keyTime = _imp->ui->isInsideKeyFrameTexture(time, pos, viewportPos);
        if (keyTime != INT_MAX) {
            ViewerInstance* viewer = overlay->getInternalViewerNode();
            if (viewer) {
                viewer->getTimeline()->seekFrame(keyTime, true, viewer, eTimelineChangeReasonOtherSeek);
            }
            didSomething = true;
        }
    }
    if (!didSomething && !trackingPageSecret) {
        std::list<TrackMarkerPtr> selectedMarkers;
        context->getSelectedMarkers(&selectedMarkers);
        if ( !selectedMarkers.empty() ) {
            context->clearSelection(TrackerContext::eTrackSelectionViewer);

            didSomething = true;
        }
    }

    _imp->ui->lastMousePos = pos;

    return didSomething;
} // penDown

bool
TrackerNode::onOverlayPenMotion(double time,
                                const RenderScale & /*renderScale*/,
                                ViewIdx view,
                                const QPointF & viewportPos,
                                const QPointF & pos,
                                double /*pressure*/,
                                double /*timestamp*/)
{
    std::pair<double, double> pixelScale;
    OverlaySupport* overlay = getCurrentViewportForOverlays();

    assert(overlay);
    overlay->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false; // Set if an actual action was performed based on mouse motion.
    bool redraw = false; // Set if we just need a redraw (e.g., hover state changed).
    TrackerContextPtr context = getNode()->getTrackerContext();
    Point delta;
    delta.x = pos.x() - _imp->ui->lastMousePos.x();
    delta.y = pos.y() - _imp->ui->lastMousePos.y();


    if (_imp->ui->hoverState != eDrawStateInactive) {
        _imp->ui->hoverState = eDrawStateInactive;
        _imp->ui->hoverMarker.reset();
        redraw = true;
    }

    std::vector<TrackMarkerPtr> allMarkers;
    context->getAllMarkers(&allMarkers);
    bool trackingPageSecret = context->getTrackingPageKnob()->getIsSecret();
    bool hoverProcess = false;
    for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
        if (!(*it)->isEnabled(time) || trackingPageSecret) {
            continue;
        }

        bool isSelected = context->isMarkerSelected( (*it) );
        KnobDoublePtr centerKnob = (*it)->getCenterKnob();
        KnobDoublePtr offsetKnob = (*it)->getOffsetKnob();
        KnobDoublePtr ptnTopLeft = (*it)->getPatternTopLeftKnob();
        KnobDoublePtr ptnTopRight = (*it)->getPatternTopRightKnob();
        KnobDoublePtr ptnBtmRight = (*it)->getPatternBtmRightKnob();
        KnobDoublePtr ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
        KnobDoublePtr searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
        KnobDoublePtr searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
        QPointF center;
        center.rx() = centerKnob->getValueAtTime(time, 0);
        center.ry() = centerKnob->getValueAtTime(time, 1);

        QPointF offset;
        offset.rx() = offsetKnob->getValueAtTime(time, 0);
        offset.ry() = offsetKnob->getValueAtTime(time, 1);
        if ( _imp->ui->isNearbyPoint(centerKnob, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time) ) {
            _imp->ui->hoverState = eDrawStateHoveringCenter;
            _imp->ui->hoverMarker = *it;
            hoverProcess = true;
        } else if ( ( (offset.x() != 0) || (offset.y() != 0) ) && _imp->ui->isNearbyPoint(QPointF( center.x() + offset.x(), center.y() + offset.y() ), viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
            redraw = _imp->ui->hoverState != eDrawStateHoveringCenter;
            _imp->ui->hoverState = eDrawStateHoveringCenter;
            _imp->ui->hoverMarker = *it;
        }


        if (!hoverProcess) {
            QPointF topLeft, topRight, btmRight, btmLeft;
            topLeft.rx() = ptnTopLeft->getValueAtTime(time, 0) + offset.x() + center.x();
            topLeft.ry() = ptnTopLeft->getValueAtTime(time, 1) + offset.y() + center.y();

            topRight.rx() = ptnTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
            topRight.ry() = ptnTopRight->getValueAtTime(time, 1) + offset.y() + center.y();

            btmRight.rx() = ptnBtmRight->getValueAtTime(time, 0) + offset.x() + center.x();
            btmRight.ry() = ptnBtmRight->getValueAtTime(time, 1) + offset.y() + center.y();

            btmLeft.rx() = ptnBtmLeft->getValueAtTime(time, 0) + offset.x() + center.x();
            btmLeft.ry() = ptnBtmLeft->getValueAtTime(time, 1) + offset.y() + center.y();

            QPointF midTop, midRight, midBtm, midLeft;
            midTop.rx() = ( topLeft.x() + topRight.x() ) / 2.;
            midTop.ry() = ( topLeft.y() + topRight.y() ) / 2.;

            midRight.rx() = ( btmRight.x() + topRight.x() ) / 2.;
            midRight.ry() = ( btmRight.y() + topRight.y() ) / 2.;

            midBtm.rx() = ( btmRight.x() + btmLeft.x() ) / 2.;
            midBtm.ry() = ( btmRight.y() + btmLeft.y() ) / 2.;

            midLeft.rx() = ( topLeft.x() + btmLeft.x() ) / 2.;
            midLeft.ry() = ( topLeft.y() + btmLeft.y() ) / 2.;


            if ( isSelected && _imp->ui->isNearbyPoint(topLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerTopLeft;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(topRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerTopRight;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(btmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerBtmRight;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(btmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerBtmLeft;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midTop, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerTopMid;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerMidRight;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerMidLeft;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( isSelected && _imp->ui->isNearbyPoint(midBtm, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringInnerBtmMid;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            }
        }

        if (!hoverProcess && isSelected) {
            ///Test search window
            const double searchLeft   = searchWndBtmLeft->getValueAtTime(time, 0)  + offset.x() + center.x();
            const double searchBottom = searchWndBtmLeft->getValueAtTime(time, 1)  + offset.y() + center.y();
            const double searchRight  = searchWndTopRight->getValueAtTime(time, 0) + offset.x() + center.x();
            const double searchTop    = searchWndTopRight->getValueAtTime(time, 1) + offset.y() + center.y();
            const double searchMidX   = (searchLeft + searchRight) / 2;
            const double searchMidY   = (searchTop + searchBottom) / 2;
            const QPointF searchTopLeft(searchLeft, searchTop);
            const QPointF searchTopRight(searchRight, searchTop);
            const QPointF searchBtmRight(searchRight, searchBottom);
            const QPointF searchBtmLeft(searchLeft, searchBottom);
            const QPointF searchTopMid(searchMidX, searchTop);
            const QPointF searchRightMid(searchRight, searchMidY);
            const QPointF searchLeftMid(searchLeft, searchMidY);
            const QPointF searchBtmMid(searchMidX, searchBottom);

            if ( _imp->ui->isNearbyPoint(searchTopLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterTopLeft;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchTopRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterTopRight;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchBtmRight, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterBtmRight;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchBtmLeft, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterBtmLeft;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchTopMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterTopMid;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchBtmMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterBtmMid;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchLeftMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterMidLeft;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            } else if ( _imp->ui->isNearbyPoint(searchRightMid, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->ui->hoverState = eDrawStateHoveringOuterMidRight;
                _imp->ui->hoverMarker = *it;
                hoverProcess = true;
            }
        }

        if (hoverProcess) {
            break;
        }
    } // for (std::vector<TrackMarkerPtr>::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

    if ( _imp->ui->showMarkerTexture && _imp->ui->selectedMarkerTexture && _imp->ui->isNearbySelectedMarkerTextureResizeAnchor(pos) ) {
        setCurrentCursor(eCursorFDiag);
        hoverProcess = true;
    } else if ( _imp->ui->showMarkerTexture && _imp->ui->selectedMarkerTexture && _imp->ui->isInsideSelectedMarkerTexture(pos) ) {
        setCurrentCursor(eCursorSizeAll);
        hoverProcess = true;
    } else if ( _imp->ui->showMarkerTexture && (_imp->ui->isInsideKeyFrameTexture(time, pos, viewportPos) != INT_MAX) ) {
        setCurrentCursor(eCursorPointingHand);
        hoverProcess = true;
    } else {
        setCurrentCursor(eCursorDefault);
    }

    if ( _imp->ui->showMarkerTexture && _imp->ui->selectedMarkerTexture && _imp->ui->shiftDown && _imp->ui->isInsideSelectedMarkerTexture(pos) ) {
        _imp->ui->hoverState = eDrawStateShowScalingHint;
        hoverProcess = true;
    }

    if (hoverProcess) {
        redraw = true;
    }

    KnobDoublePtr centerKnob, offsetKnob, searchWndTopRight, searchWndBtmLeft;
    KnobDoublePtr patternCorners[4];
    if (_imp->ui->interactMarker) {
        centerKnob = _imp->ui->interactMarker->getCenterKnob();
        offsetKnob = _imp->ui->interactMarker->getOffsetKnob();

        /*

           TopLeft(0) ------------- Top right(3)
         |                        |
         |                        |
         |                        |
           Btm left (1) ------------ Btm right (2)

         */
        patternCorners[0] = _imp->ui->interactMarker->getPatternTopLeftKnob();
        patternCorners[1] = _imp->ui->interactMarker->getPatternBtmLeftKnob();
        patternCorners[2] = _imp->ui->interactMarker->getPatternBtmRightKnob();
        patternCorners[3] = _imp->ui->interactMarker->getPatternTopRightKnob();
        searchWndTopRight = _imp->ui->interactMarker->getSearchWindowTopRightKnob();
        searchWndBtmLeft = _imp->ui->interactMarker->getSearchWindowBottomLeftKnob();
    }
    if (!trackingPageSecret) {
        switch (_imp->ui->eventState) {
        case eMouseStateDraggingCenter:
        case eMouseStateDraggingOffset: {
            assert(_imp->ui->interactMarker);
            if (!centerKnob || !offsetKnob) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }

            if (_imp->ui->eventState == eMouseStateDraggingOffset) {
                offsetKnob->setValues(offsetKnob->getValueAtTime(time, 0) + delta.x,
                                      offsetKnob->getValueAtTime(time, 1) + delta.y,
                                      view,
                                      eValueChangedReasonPluginEdited);
            } else {
                centerKnob->setValuesAtTime(time, centerKnob->getValueAtTime(time, 0) + delta.x,
                                            centerKnob->getValueAtTime(time, 1) + delta.y,
                                            view,
                                            eValueChangedReasonPluginEdited);
                for (int i = 0; i < 4; ++i) {
                    for (int d = 0; d < patternCorners[i]->getDimension(); ++d) {
                        patternCorners[i]->setValueAtTime(time, patternCorners[i]->getValueAtTime(time, d), view, d);
                    }
                }
            }
            _imp->ui->refreshSelectedMarkerTexture();
            if ( _imp->ui->createKeyOnMoveButton.lock()->getValue() ) {
                _imp->ui->interactMarker->setUserKeyframe(time);
            }
            didSomething = true;
            break;
        }
        case eMouseStateDraggingInnerBtmLeft:
        case eMouseStateDraggingInnerTopRight:
        case eMouseStateDraggingInnerTopLeft:
        case eMouseStateDraggingInnerBtmRight: {
            if (_imp->ui->controlDown == 0) {
                _imp->ui->transformPattern(time, _imp->ui->eventState, delta);
                didSomething = true;
                break;
            }
            if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }
            int index = 0;
            if (_imp->ui->eventState == eMouseStateDraggingInnerBtmLeft) {
                index = 1;
            } else if (_imp->ui->eventState == eMouseStateDraggingInnerBtmRight) {
                index = 2;
            } else if (_imp->ui->eventState == eMouseStateDraggingInnerTopRight) {
                index = 3;
            } else if (_imp->ui->eventState == eMouseStateDraggingInnerTopLeft) {
                index = 0;
            }

            int nextIndex = (index + 1) % 4;
            int prevIndex = (index + 3) % 4;
            int diagIndex = (index + 2) % 4;
            Point center;
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            Point offset;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);

            Point cur, prev, next, diag;
            cur.x = patternCorners[index]->getValueAtTime(time, 0) + delta.x  + center.x + offset.x;;
            cur.y = patternCorners[index]->getValueAtTime(time, 1) + delta.y  + center.y + offset.y;

            prev.x = patternCorners[prevIndex]->getValueAtTime(time, 0)  + center.x + offset.x;;
            prev.y = patternCorners[prevIndex]->getValueAtTime(time, 1) + center.y + offset.y;

            next.x = patternCorners[nextIndex]->getValueAtTime(time, 0)  + center.x + offset.x;;
            next.y = patternCorners[nextIndex]->getValueAtTime(time, 1)  + center.y + offset.y;

            diag.x = patternCorners[diagIndex]->getValueAtTime(time, 0)  + center.x + offset.x;;
            diag.y = patternCorners[diagIndex]->getValueAtTime(time, 1) + center.y + offset.y;

            Point nextVec;
            nextVec.x = next.x - cur.x;
            nextVec.y = next.y - cur.y;

            Point prevVec;
            prevVec.x = cur.x - prev.x;
            prevVec.y = cur.y - prev.y;

            Point nextDiagVec, prevDiagVec;
            prevDiagVec.x = diag.x - next.x;
            prevDiagVec.y = diag.y - next.y;

            nextDiagVec.x = prev.x - diag.x;
            nextDiagVec.y = prev.y - diag.y;

            //Clamp so the 4 points remaining the same in the homography
            if (prevVec.x * nextVec.y - prevVec.y * nextVec.x < 0.) {         // cross-product
                TrackerNodeInteract::findLineIntersection(cur, prev, next, &cur);
            }
            if (nextDiagVec.x * prevVec.y - nextDiagVec.y * prevVec.x < 0.) {         // cross-product
                TrackerNodeInteract::findLineIntersection(cur, prev, diag, &cur);
            }
            if (nextVec.x * prevDiagVec.y - nextVec.y * prevDiagVec.x < 0.) {         // cross-product
                TrackerNodeInteract::findLineIntersection(cur, next, diag, &cur);
            }


            Point searchWindowCorners[2];
            searchWindowCorners[0].x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
            searchWindowCorners[0].y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;

            searchWindowCorners[1].x = searchWndTopRight->getValueAtTime(time, 0)  + center.x + offset.x;
            searchWindowCorners[1].y = searchWndTopRight->getValueAtTime(time, 1)  + center.y + offset.y;

            cur.x = boost::algorithm::clamp(cur.x, searchWindowCorners[0].x, searchWindowCorners[1].x);
            cur.y = boost::algorithm::clamp(cur.y, searchWindowCorners[0].y, searchWindowCorners[1].y);

            cur.x -= (center.x + offset.x);
            cur.y -= (center.y + offset.y);

            patternCorners[index]->setValuesAtTime(time, cur.x, cur.y, view, eValueChangedReasonNatronInternalEdited);

            if ( _imp->ui->createKeyOnMoveButton.lock()->getValue() ) {
                _imp->ui->interactMarker->setUserKeyframe(time);
            }
            didSomething = true;
            break;
        }
        case eMouseStateDraggingOuterBtmLeft: {
            if (_imp->ui->controlDown == 0) {
                _imp->ui->transformPattern(time, _imp->ui->eventState, delta);
                didSomething = true;
                break;
            }
            if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }
            Point center;
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            Point offset;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);

            Point p = {0, 0.};
            p.x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
            p.y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y + delta.y;
            Point topLeft;
            topLeft.x = patternCorners[0]->getValueAtTime(time, 0) + center.x + offset.x;
            topLeft.y = patternCorners[0]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmLeft;
            btmLeft.x = patternCorners[1]->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = patternCorners[1]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmRight;
            btmRight.x = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
            btmRight.y = patternCorners[2]->getValueAtTime(time, 1) + center.y + offset.y;
            Point topRight;
            topRight.x = patternCorners[3]->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = patternCorners[3]->getValueAtTime(time, 1) + center.y + offset.y;

            // test every point: even topRight pattern corner may be on the left of topLeft
            p.x = std::min(p.x, topLeft.x);
            p.x = std::min(p.x, btmLeft.x);
            p.x = std::min(p.x, btmRight.x);
            p.x = std::min(p.x, topRight.x);

            p.y = std::min(p.y, topLeft.y);
            p.y = std::min(p.y, btmLeft.y);
            p.y = std::min(p.y, btmRight.y);
            p.y = std::min(p.y, topRight.y);

            p.x -= (center.x + offset.x);
            p.y -= (center.y + offset.y);
            if ( searchWndBtmLeft->hasAnimation() ) {
                searchWndBtmLeft->setValuesAtTime(time, p.x, p.y, view, eValueChangedReasonNatronInternalEdited);
            } else {
                searchWndBtmLeft->setValues(p.x, p.y, view, eValueChangedReasonNatronInternalEdited);
            }

            _imp->ui->refreshSelectedMarkerTexture();
            didSomething = true;
            break;
        }
        case eMouseStateDraggingOuterBtmRight: {
            if (_imp->ui->controlDown == 0) {
                _imp->ui->transformPattern(time, _imp->ui->eventState, delta);
                didSomething = true;
                break;
            }
            if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }
            Point center;
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            Point offset;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);

            Point p;
            p.x = searchWndTopRight->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
            p.y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y + delta.y;

            Point topLeft;
            topLeft.x = patternCorners[0]->getValueAtTime(time, 0) + center.x + offset.x;
            topLeft.y = patternCorners[0]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmLeft;
            btmLeft.x = patternCorners[1]->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = patternCorners[1]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmRight;
            btmRight.x = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
            btmRight.y = patternCorners[2]->getValueAtTime(time, 1) + center.y + offset.y;
            Point topRight;
            topRight.x = patternCorners[3]->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = patternCorners[3]->getValueAtTime(time, 1) + center.y + offset.y;

            // test every point: even topRight pattern corner may be on the left of topLeft
            p.x = std::max(p.x, topLeft.x);
            p.x = std::max(p.x, btmLeft.x);
            p.x = std::max(p.x, btmRight.x);
            p.x = std::max(p.x, topRight.x);

            p.y = std::min(p.y, topLeft.y);
            p.y = std::min(p.y, btmLeft.y);
            p.y = std::min(p.y, btmRight.y);
            p.y = std::min(p.y, topRight.y);

            p.x -= (center.x + offset.x);
            p.y -= (center.y + offset.y);
            if ( searchWndBtmLeft->hasAnimation() ) {
                searchWndBtmLeft->setValueAtTime(time, p.y, view, 1);
            } else {
                searchWndBtmLeft->setValue(p.y, view, 1);
            }
            if ( searchWndTopRight->hasAnimation() ) {
                searchWndTopRight->setValueAtTime(time, p.x, view, 0);
            } else {
                searchWndTopRight->setValue(p.x, view, 0);
            }

            _imp->ui->refreshSelectedMarkerTexture();
            didSomething = true;
            break;
        }
        case eMouseStateDraggingOuterTopRight: {
            if (_imp->ui->controlDown == 0) {
                _imp->ui->transformPattern(time, _imp->ui->eventState, delta);
                didSomething = true;
                break;
            }
            if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }
            Point center;
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            Point offset;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);

            Point p = {0, 0};
            if (searchWndTopRight) {
                p.x = searchWndTopRight->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
                p.y = searchWndTopRight->getValueAtTime(time, 1) + center.y + offset.y + delta.y;
            }

            Point topLeft;
            topLeft.x = patternCorners[0]->getValueAtTime(time, 0) + center.x + offset.x;
            topLeft.y = patternCorners[0]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmLeft;
            btmLeft.x = patternCorners[1]->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = patternCorners[1]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmRight;
            btmRight.x = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
            btmRight.y = patternCorners[2]->getValueAtTime(time, 1) + center.y + offset.y;
            Point topRight;
            topRight.x = patternCorners[3]->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = patternCorners[3]->getValueAtTime(time, 1) + center.y + offset.y;

            // test every point: even topRight pattern corner may be on the left of topLeft
            p.x = std::max(p.x, topLeft.x);
            p.x = std::max(p.x, btmLeft.x);
            p.x = std::max(p.x, btmRight.x);
            p.x = std::max(p.x, topRight.x);

            p.y = std::max(p.y, topLeft.y);
            p.y = std::max(p.y, btmLeft.y);
            p.y = std::max(p.y, btmRight.y);
            p.y = std::max(p.y, topRight.y);

            p.x -= (center.x + offset.x);
            p.y -= (center.y + offset.y);
            if ( searchWndTopRight->hasAnimation() ) {
                searchWndTopRight->setValuesAtTime(time, p.x, p.y, view, eValueChangedReasonNatronInternalEdited);
            } else {
                searchWndTopRight->setValues(p.x, p.y, view, eValueChangedReasonNatronInternalEdited);
            }

            _imp->ui->refreshSelectedMarkerTexture();
            didSomething = true;
            break;
        }
        case eMouseStateDraggingOuterTopLeft: {
            if (_imp->ui->controlDown == 0) {
                _imp->ui->transformPattern(time, _imp->ui->eventState, delta);
                didSomething = true;
                break;
            }
            if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }
            Point center;
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            Point offset;
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);

            Point p;
            p.x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x + delta.x;
            p.y = searchWndTopRight->getValueAtTime(time, 1) + center.y + offset.y + delta.y;

            Point topLeft;
            topLeft.x = patternCorners[0]->getValueAtTime(time, 0) + center.x + offset.x;
            topLeft.y = patternCorners[0]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmLeft;
            btmLeft.x = patternCorners[1]->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = patternCorners[1]->getValueAtTime(time, 1) + center.y + offset.y;
            Point btmRight;
            btmRight.x = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
            btmRight.y = patternCorners[2]->getValueAtTime(time, 1) + center.y + offset.y;
            Point topRight;
            topRight.x = patternCorners[3]->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = patternCorners[3]->getValueAtTime(time, 1) + center.y + offset.y;

            // test every point: even topRight pattern corner may be on the left of topLeft
            p.x = std::min(p.x, topLeft.x);
            p.x = std::min(p.x, btmLeft.x);
            p.x = std::min(p.x, btmRight.x);
            p.x = std::min(p.x, topRight.x);

            p.y = std::max(p.y, topLeft.y);
            p.y = std::max(p.y, btmLeft.y);
            p.y = std::max(p.y, btmRight.y);
            p.y = std::max(p.y, topRight.y);

            p.x -= (center.x + offset.x);
            p.y -= (center.y + offset.y);
            if ( searchWndBtmLeft->hasAnimation() ) {
                searchWndBtmLeft->setValueAtTime(time, p.x, view, 0);
            } else {
                searchWndBtmLeft->setValue(p.x, view, 0);
            }
            if ( searchWndTopRight->hasAnimation() ) {
                searchWndTopRight->setValueAtTime(time, p.y, view, 1);
            } else {
                searchWndTopRight->setValue(p.y, view, 1);
            }

            _imp->ui->refreshSelectedMarkerTexture();
            didSomething = true;
            break;
        }
        case eMouseStateDraggingInnerBtmMid:
        case eMouseStateDraggingInnerTopMid:
        case eMouseStateDraggingInnerMidLeft:
        case eMouseStateDraggingInnerMidRight:
        case eMouseStateDraggingOuterBtmMid:
        case eMouseStateDraggingOuterTopMid:
        case eMouseStateDraggingOuterMidLeft:
        case eMouseStateDraggingOuterMidRight: {
            _imp->ui->transformPattern(time, _imp->ui->eventState, delta);
            didSomething = true;
            break;
        }
        case eMouseStateDraggingSelectedMarkerResizeAnchor: {
            QPointF lastPosWidget = overlay->toWidgetCoordinates(_imp->ui->lastMousePos);
            double dx = viewportPos.x() - lastPosWidget.x();
            KnobIntPtr knob = _imp->ui->magWindowPxSizeKnob.lock();
            int value = knob->getValue();
            value += dx;
            value = std::max(value, 10);
            knob->setValue(value);
            didSomething = true;
            break;
        }
        case eMouseStateScalingSelectedMarker: {
            TrackMarkerPtr marker = _imp->ui->selectedMarker.lock();
            assert(marker);
            RectD markerMagRect;
            _imp->ui->computeSelectedMarkerCanonicalRect(&markerMagRect);
            KnobDoublePtr centerKnob = marker->getCenterKnob();
            KnobDoublePtr offsetKnob = marker->getOffsetKnob();
            KnobDoublePtr searchBtmLeft = marker->getSearchWindowBottomLeftKnob();
            KnobDoublePtr searchTopRight = marker->getSearchWindowTopRightKnob();
            if (!centerKnob || !offsetKnob || !searchBtmLeft || !searchTopRight) {
                didSomething = false;
                break;
            }

            Point center, offset, btmLeft, topRight;
            center.x = centerKnob->getValueAtTime(time, 0);
            center.y = centerKnob->getValueAtTime(time, 1);
            offset.x = offsetKnob->getValueAtTime(time, 0);
            offset.y = offsetKnob->getValueAtTime(time, 1);
            btmLeft.x = searchBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
            btmLeft.y = searchBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;
            topRight.x = searchTopRight->getValueAtTime(time, 0) + center.x + offset.x;
            topRight.y = searchTopRight->getValueAtTime(time, 1) + center.y + offset.y;

            //Remove any offset to the center to see the marker in the magnification window
            double xCenterPercent = (center.x - btmLeft.x + offset.x) / (topRight.x - btmLeft.x);
            double yCenterPercent = (center.y - btmLeft.y + offset.y) / (topRight.y - btmLeft.y);
            Point centerPoint;
            centerPoint.x = markerMagRect.x1 + xCenterPercent * (markerMagRect.x2 - markerMagRect.x1);
            centerPoint.y = markerMagRect.y1 + yCenterPercent * (markerMagRect.y2 - markerMagRect.y1);

            double prevDist = std::sqrt( (_imp->ui->lastMousePos.x() - centerPoint.x ) * ( _imp->ui->lastMousePos.x() - centerPoint.x) + ( _imp->ui->lastMousePos.y() - centerPoint.y) * ( _imp->ui->lastMousePos.y() - centerPoint.y) );
            if (prevDist != 0) {
                double dist = std::sqrt( ( pos.x() - centerPoint.x) * ( pos.x() - centerPoint.x) + ( pos.y() - centerPoint.y) * ( pos.y() - centerPoint.y) );
                double ratio = dist / prevDist;
                _imp->ui->selectedMarkerScale.x *= ratio;
                _imp->ui->selectedMarkerScale.x = boost::algorithm::clamp(_imp->ui->selectedMarkerScale.x, 0.05, 1.);
                _imp->ui->selectedMarkerScale.y = _imp->ui->selectedMarkerScale.x;
                didSomething = true;
            }
            break;
        }
        case eMouseStateDraggingSelectedMarker: {
            if (!centerKnob || !offsetKnob || !searchWndBtmLeft || !searchWndTopRight) {
                didSomething = false;
                break;
            }
            if (!patternCorners[0] || !patternCorners[1] || !patternCorners[2] || !patternCorners[3]) {
                didSomething = false;
                break;
            }
            double x = centerKnob->getValueAtTime(time, 0);
            double y = centerKnob->getValueAtTime(time, 1);
            double dx = delta.x *  _imp->ui->selectedMarkerScale.x;
            double dy = delta.y *  _imp->ui->selectedMarkerScale.y;
            x += dx;
            y += dy;
            centerKnob->setValuesAtTime(time, x, y, view, eValueChangedReasonPluginEdited);
            for (int i = 0; i < 4; ++i) {
                for (int d = 0; d < patternCorners[i]->getDimension(); ++d) {
                    patternCorners[i]->setValueAtTime(time, patternCorners[i]->getValueAtTime(time, d), view, d);
                }
            }
            if ( _imp->ui->createKeyOnMoveButton.lock()->getValue() ) {
                _imp->ui->interactMarker->setUserKeyframe(time);
            }
            _imp->ui->refreshSelectedMarkerTexture();
            didSomething = true;
            break;
        }
        default:
            break;
        } // switch
    } // !trackingPageSecret
    if (_imp->ui->clickToAddTrackEnabled) {
        ///Refresh the overlay
        didSomething = true;
    }
    _imp->ui->lastMousePos = pos;

    if (redraw) {
        redrawOverlayInteract();
    }

    return didSomething;
} //penMotion

bool
TrackerNode::onOverlayPenDoubleClicked(double /*time*/,
                                       const RenderScale & /*renderScale*/,
                                       ViewIdx /*view*/,
                                       const QPointF & /*viewportPos*/,
                                       const QPointF & /*pos*/)
{
    return false;
}

bool
TrackerNode::onOverlayPenUp(double /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            const QPointF & /*viewportPos*/,
                            const QPointF & /*pos*/,
                            double /*pressure*/,
                            double /*timestamp*/)
{
    bool didSomething = false;
    TrackerMouseStateEnum state = _imp->ui->eventState;

    if (state != eMouseStateIdle) {
        _imp->ui->eventState = eMouseStateIdle;
        didSomething = true;
    }
    if (_imp->ui->interactMarker) {
        _imp->ui->interactMarker.reset();
        didSomething = true;
    }

    return didSomething;
} // penUp

bool
TrackerNode::onOverlayKeyDown(double /*time*/,
                              const RenderScale & /*renderScale*/,
                              ViewIdx /*view*/,
                              Key key,
                              KeyboardModifiers /*modifiers*/)
{
    bool didSomething = false;
    bool isCtrl = false;
    bool isAlt = false;

    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        ++_imp->ui->shiftDown;
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        ++_imp->ui->controlDown;
        isCtrl = true;
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        ++_imp->ui->altDown;
        isAlt = true;
    }

    bool trackingPageSecret = getNode()->getTrackerContext()->getTrackingPageKnob()->getIsSecret();

    if ( !trackingPageSecret && _imp->ui->controlDown && _imp->ui->altDown && !_imp->ui->shiftDown && (isCtrl || isAlt) ) {
        _imp->ui->clickToAddTrackEnabled = true;
        _imp->ui->addTrackButton.lock()->setValue(true);
        didSomething = true;
    }


    return didSomething;
} // keydown

bool
TrackerNode::onOverlayKeyUp(double /*time*/,
                            const RenderScale & /*renderScale*/,
                            ViewIdx /*view*/,
                            Key key,
                            KeyboardModifiers /*modifiers*/)
{
    bool didSomething = false;
    bool isAlt = false;
    bool isControl = false;

    if ( (key == Key_Shift_L) || (key == Key_Shift_R) ) {
        if (_imp->ui->shiftDown) {
            --_imp->ui->shiftDown;
        }
        if (_imp->ui->eventState == eMouseStateScalingSelectedMarker) {
            _imp->ui->eventState = eMouseStateIdle;
            didSomething = true;
        }
    } else if ( (key == Key_Control_L) || (key == Key_Control_R) ) {
        if (_imp->ui->controlDown) {
            --_imp->ui->controlDown;
        }
        isControl = true;
    } else if ( (key == Key_Alt_L) || (key == Key_Alt_R) ) {
        if (_imp->ui->altDown) {
            --_imp->ui->altDown;
        }
        isAlt = true;
    }


    if ( _imp->ui->clickToAddTrackEnabled && (isControl || isAlt) ) {
        _imp->ui->clickToAddTrackEnabled = false;
        _imp->ui->addTrackButton.lock()->setValue(false);
        didSomething = true;
    }


    return didSomething;
} // KeyUp

bool
TrackerNode::onOverlayKeyRepeat(double /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/,
                                Key /*key*/,
                                KeyboardModifiers /* modifiers*/)
{
    return false;
} // keyrepeat

bool
TrackerNode::onOverlayFocusGained(double /*time*/,
                                  const RenderScale & /*renderScale*/,
                                  ViewIdx /* view*/)
{
    return false;
} // gainFocus

bool
TrackerNode::onOverlayFocusLost(double /*time*/,
                                const RenderScale & /*renderScale*/,
                                ViewIdx /*view*/)
{
    _imp->ui->altDown = 0;
    _imp->ui->controlDown = 0;
    _imp->ui->shiftDown = 0;


    return true;
} // loseFocus

void
TrackerNode::onInteractViewportSelectionCleared()
{
    bool trackingPageSecret = getNode()->getTrackerContext()->getTrackingPageKnob()->getIsSecret();

    if (trackingPageSecret) {
        return;
    }

    getNode()->getTrackerContext()->clearSelection(TrackerContext::eTrackSelectionViewer);
}

void
TrackerNode::onInteractViewportSelectionUpdated(const RectD& rectangle,
                                                bool onRelease)
{
    if (!onRelease) {
        return;
    }


    bool trackingPageSecret = getNode()->getTrackerContext()->getTrackingPageKnob()->getIsSecret();
    if (trackingPageSecret) {
        return;
    }

    std::vector<TrackMarkerPtr> allMarkers;
    std::list<TrackMarkerPtr> selectedMarkers;
    TrackerContextPtr context = getNode()->getTrackerContext();
    context->getAllMarkers(&allMarkers);
    for (std::size_t i = 0; i < allMarkers.size(); ++i) {
        if ( !allMarkers[i]->isEnabled( allMarkers[i]->getCurrentTime() ) ) {
            continue;
        }
        KnobDoublePtr center = allMarkers[i]->getCenterKnob();
        double x, y;
        x = center->getValue(0);
        y = center->getValue(1);
        if ( (x >= rectangle.x1) && (x <= rectangle.x2) && (y >= rectangle.y1) && (y <= rectangle.y2) ) {
            selectedMarkers.push_back(allMarkers[i]);
        }
    }

    context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
    context->clearSelection(TrackerContext::eTrackSelectionInternal);
    context->addTracksToSelection(selectedMarkers, TrackerContext::eTrackSelectionInternal);
    context->endEditSelection(TrackerContext::eTrackSelectionInternal);
}

void
TrackerNode::refreshExtraStateAfterTimeChanged(bool isPlayback,
                                               double /*time*/)
{
    if (_imp->ui->showMarkerTexture && !isPlayback && !getApp()->isDraftRenderEnabled()) {
        _imp->ui->refreshSelectedMarkerTexture();
    }
}

NATRON_NAMESPACE_EXIT
NATRON_NAMESPACE_USING

#include "moc_TrackerNode.cpp"

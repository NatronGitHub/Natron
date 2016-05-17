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

#include "TrackerNode.h"
#include "Engine/Node.h"
#include "Engine/TrackerContext.h"
#include "Engine/TrackerNodeInteract.h"

NATRON_NAMESPACE_ENTER;


TrackerNode::TrackerNode(boost::shared_ptr<Natron::Node> node)
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
    return "<p>Track one or more 2D point(s) using LibMV from the Blender open-source software.</p>"
           "<br/>"
           "<p><b>Goal:</b></p>"
           "<ul>"
           "<li> Track one or more 2D point and use them to either make another object/image match-move their motion or to stabilize the input image.</li>"
           "</ul>"
           "<br/>"
           "<p><b>Tracking:</b></p>"
           "<ul>"
           "<li> Connect a Tracker node to the image containing the item you need to track </li>"
           "<li> Place tracking markers with CTRL+ALT+Click on the Viewer or by clicking the \"+\" button below the track table in the settings panel </li>"
           "<li> Setup the motion model to match the motion type of the item you need to track. By default the tracker will only assume the item is underoing a translation. Other motion models can be used for complex tracks but may be slower. </li>"
           "<li> Select in the settings panel or on the Viewer the markers you want to track and then start tracking with the player buttons on the top of the Viewer. </li>"
           "<li> If a track is getting lost or fails at some point, you may refine it by moving the marker at its correct position, this will force a new keyframe on the pattern which will be visible in the Viewer and on the timeline. </li>"
           "</ul>"
           "<p><b>Using the tracks data:</b></p>"
           "<p>You can either use the Tracker node itself to use the track data or you may export it to another node.</p>"
           "<p><b>Using the Transform within the Tracker node:</b></p>"
           "<p>Go to the Transform tab in the settings panel, and set the Transform Type to the operation you want to achieve. During tracking, the Transform Type should always been set to None if you want to correctly see the tracks on the Viewer."
           "You will notice that the transform parameters will be set automatically when the tracking is finished. Depending on the Transform Type, the values will be computed either to match-move the motion of the tracked points or to stabilize the image.</p>"
           "<p><b>Exporting the tracking data:</b></p>"
           "<p>You may export the tracking data either to a CornerPin node or to a Transform node. The CornerPin node performs a warp that may be more stable than a Transform node when using 4 or more tracks: it retains more information than the Transform node.<p>";
}

void
TrackerNode::getPluginShortcuts(std::list<PluginActionShortcut>* shortcuts)
{

}

void
TrackerNode::initializeKnobs()
{
    boost::shared_ptr<KnobPage> trackingPage;
    {
        KnobPtr page = getKnobByName("Tracking");
        assert(page);
        trackingPage = boost::dynamic_pointer_cast<KnobPage>(page);
    }

    boost::shared_ptr<KnobButton> addMarker = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamAddTrackLabel));
    addMarker->setName(kTrackerUIParamAddTrack);
    addMarker->setHintToolTip(tr(kTrackerUIParamAddTrackHint));
    addMarker->setEvaluateOnChange(false);
    addMarker->setCheckable(true);
    addMarker->setDefaultValue(false);
    addMarker->setSecretByDefault(true);
    addMarker->setInViewerContextCanHaveShortcut(true);
    addMarker->setIconLabel(NATRON_IMAGES_PATH "addTrack.png");
    trackingPage->addKnob(addMarker);
    _imp->ui->addTrackButton = addMarker;

    boost::shared_ptr<KnobButton> trackBw = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackBWLabel));
    trackBw->setName(kTrackerUIParamTrackBW);
    trackBw->setHintToolTip(tr(kTrackerUIParamTrackBWHint));
    trackBw->setEvaluateOnChange(false);
    trackBw->setCheckable(true);
    trackBw->setDefaultValue(false);
    trackBw->setSecretByDefault(true);
    trackBw->setInViewerContextCanHaveShortcut(true);
    trackBw->setIconLabel(NATRON_IMAGES_PATH "trackBackwardOn.png", true);
    trackBw->setIconLabel(NATRON_IMAGES_PATH "trackBackwardOff.png", false);
    trackingPage->addKnob(trackBw);
    _imp->ui->trackBwButton = trackBw;

    boost::shared_ptr<KnobButton> trackPrev = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackPreviousLabel));
    trackPrev->setName(kTrackerUIParamTrackPrevious);
    trackPrev->setHintToolTip(tr(kTrackerUIParamTrackPreviousHint));
    trackPrev->setEvaluateOnChange(false);
    trackPrev->setSecretByDefault(true);
    trackPrev->setInViewerContextCanHaveShortcut(true);
    trackPrev->setIconLabel(NATRON_IMAGES_PATH "trackPrev.png");
    trackingPage->addKnob(trackPrev);
    _imp->ui->trackPrevButton = trackPrev;

    boost::shared_ptr<KnobButton> trackNext = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackNextLabel));
    trackNext->setName(kTrackerUIParamTrackNext);
    trackNext->setHintToolTip(tr(kTrackerUIParamTrackNextHint));
    trackNext->setEvaluateOnChange(false);
    trackNext->setSecretByDefault(true);
    trackNext->setInViewerContextCanHaveShortcut(true);
    trackNext->setIconLabel(NATRON_IMAGES_PATH "trackNext.png");
    trackingPage->addKnob(trackNext);
    _imp->ui->trackNextButton = trackNext;

    boost::shared_ptr<KnobButton> trackFw = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackFWLabel));
    trackFw->setName(kTrackerUIParamTrackFW);
    trackFw->setHintToolTip(tr(kTrackerUIParamTrackFWHint));
    trackFw->setEvaluateOnChange(false);
    trackFw->setCheckable(true);
    trackFw->setDefaultValue(false);
    trackFw->setSecretByDefault(true);
    trackFw->setInViewerContextCanHaveShortcut(true);
    trackFw->setIconLabel(NATRON_IMAGES_PATH "trackForwardOn.png", true);
    trackFw->setIconLabel(NATRON_IMAGES_PATH "trackForwardOff.png", false);
    trackingPage->addKnob(trackFw);
    _imp->ui->trackFwButton = trackFw;

    boost::shared_ptr<KnobButton> trackRange = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackRangeLabel));
    trackRange->setName(kTrackerUIParamTrackRange);
    trackRange->setHintToolTip(tr(kTrackerUIParamTrackRangeHint));
    trackRange->setEvaluateOnChange(false);
    trackRange->setSecretByDefault(true);
    trackRange->setInViewerContextCanHaveShortcut(true);
    trackRange->setIconLabel(NATRON_IMAGES_PATH "trackRange.png");
    trackingPage->addKnob(trackRange);
    _imp->ui->trackRangeButton = trackRange;

    boost::shared_ptr<KnobButton> trackAllKeys = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackAllKeyframesLabel));
    trackAllKeys->setName(kTrackerUIParamTrackAllKeyframes);
    trackAllKeys->setHintToolTip(tr(kTrackerUIParamTrackAllKeyframesHint));
    trackAllKeys->setEvaluateOnChange(false);
    trackAllKeys->setSecretByDefault(true);
    trackAllKeys->setInViewerContextCanHaveShortcut(true);
    trackAllKeys->setIconLabel(NATRON_IMAGES_PATH "trackAllKeyframes.png");
    trackingPage->addKnob(trackAllKeys);
    _imp->ui->trackAllKeyframesButton = trackAllKeys;


    boost::shared_ptr<KnobButton> trackCurKey = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamTrackCurrentKeyframeLabel));
    trackCurKey->setName(kTrackerUIParamTrackCurrentKeyframe);
    trackCurKey->setHintToolTip(tr(kTrackerUIParamTrackCurrentKeyframeHint));
    trackCurKey->setEvaluateOnChange(false);
    trackCurKey->setSecretByDefault(true);
    trackCurKey->setInViewerContextCanHaveShortcut(true);
    trackCurKey->setIconLabel(NATRON_IMAGES_PATH "trackCurrentKeyframe.png");
    trackingPage->addKnob(trackCurKey);
    _imp->ui->trackCurrentKeyframeButton = trackCurKey;

    boost::shared_ptr<KnobButton> clearAllAnimation = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamClearAllAnimationLabel));
    clearAllAnimation->setName(kTrackerUIParamClearAllAnimation);
    clearAllAnimation->setHintToolTip(tr(kTrackerUIParamClearAllAnimationHint));
    clearAllAnimation->setEvaluateOnChange(false);
    clearAllAnimation->setSecretByDefault(true);
    clearAllAnimation->setInViewerContextCanHaveShortcut(true);
    clearAllAnimation->setIconLabel(NATRON_IMAGES_PATH "clearAnimation.png");
    trackingPage->addKnob(clearAllAnimation);
    _imp->ui->clearAllAnimationButton = clearAllAnimation;

    boost::shared_ptr<KnobButton> clearBackwardAnim = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamClearAnimationBwLabel));
    clearBackwardAnim->setName(kTrackerUIParamClearAnimationBw);
    clearBackwardAnim->setHintToolTip(tr(kTrackerUIParamClearAnimationBwHint));
    clearBackwardAnim->setEvaluateOnChange(false);
    clearBackwardAnim->setSecretByDefault(true);
    clearBackwardAnim->setInViewerContextCanHaveShortcut(true);
    clearBackwardAnim->setIconLabel(NATRON_IMAGES_PATH "clearAnimationBw.png");
    trackingPage->addKnob(clearBackwardAnim);
    _imp->ui->clearBwAnimationButton = clearBackwardAnim;

    boost::shared_ptr<KnobButton> clearForwardAnim = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamClearAnimationFwLabel));
    clearForwardAnim->setName(kTrackerUIParamClearAnimationFw);
    clearForwardAnim->setHintToolTip(tr(kTrackerUIParamClearAnimationFwHint));
    clearForwardAnim->setEvaluateOnChange(false);
    clearForwardAnim->setSecretByDefault(true);
    clearForwardAnim->setInViewerContextCanHaveShortcut(true);
    clearForwardAnim->setIconLabel(NATRON_IMAGES_PATH "clearAnimationFw.png");
    trackingPage->addKnob(clearForwardAnim);
    _imp->ui->clearFwAnimationButton = clearForwardAnim;

    boost::shared_ptr<KnobButton> updateViewer = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamRefreshViewerLabel));
    updateViewer->setName(kTrackerUIParamRefreshViewer);
    updateViewer->setHintToolTip(tr(kTrackerUIParamRefreshViewerHint));
    updateViewer->setEvaluateOnChange(false);
    updateViewer->setCheckable(true);
    updateViewer->setDefaultValue(true);
    updateViewer->setSecretByDefault(true);
    updateViewer->setInViewerContextCanHaveShortcut(true);
    updateViewer->setIconLabel(NATRON_IMAGES_PATH "updateViewerEnabled.png", true);
    updateViewer->setIconLabel(NATRON_IMAGES_PATH "updateViewerDisabled.png", false);
    trackingPage->addKnob(updateViewer);
    _imp->ui->updateViewerButton = updateViewer;

    boost::shared_ptr<KnobButton> centerViewer = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamCenterViewerLabel));
    centerViewer->setName(kTrackerUIParamCenterViewer);
    centerViewer->setHintToolTip(tr(kTrackerUIParamCenterViewerHint));
    centerViewer->setEvaluateOnChange(false);
    centerViewer->setCheckable(true);
    centerViewer->setDefaultValue(false);
    centerViewer->setSecretByDefault(true);
    centerViewer->setInViewerContextCanHaveShortcut(true);
    centerViewer->setIconLabel(NATRON_IMAGES_PATH "centerOnTrack.png");
    trackingPage->addKnob(centerViewer);
    _imp->ui->centerViewerButton = centerViewer;

    boost::shared_ptr<KnobButton> createKeyOnMove = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamCreateKeyOnMoveLabel));
    createKeyOnMove->setName(kTrackerUIParamCreateKeyOnMove);
    createKeyOnMove->setHintToolTip(tr(kTrackerUIParamCreateKeyOnMoveHint));
    createKeyOnMove->setEvaluateOnChange(false);
    createKeyOnMove->setCheckable(true);
    createKeyOnMove->setDefaultValue(true);
    createKeyOnMove->setSecretByDefault(true);
    createKeyOnMove->setInViewerContextCanHaveShortcut(true);
    createKeyOnMove->setIconLabel(NATRON_IMAGES_PATH "createKeyOnMoveOn.png", true);
    createKeyOnMove->setIconLabel(NATRON_IMAGES_PATH "createKeyOnMoveOff.png", false);
    trackingPage->addKnob(createKeyOnMove);
    _imp->ui->createKeyOnMoveButton = createKeyOnMove;

    boost::shared_ptr<KnobButton> showError = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamShowErrorLabel));
    showError->setName(kTrackerUIParamShowError);
    showError->setHintToolTip(tr(kTrackerUIParamShowErrorHint));
    showError->setEvaluateOnChange(false);
    showError->setCheckable(true);
    showError->setDefaultValue(false);
    showError->setSecretByDefault(true);
    showError->setInViewerContextCanHaveShortcut(true);
    showError->setIconLabel(NATRON_IMAGES_PATH "showTrackError.png", true);
    showError->setIconLabel(NATRON_IMAGES_PATH "hideTrackError.png", false);
    trackingPage->addKnob(showError);
    _imp->ui->showCorrelationButton = showError;


    boost::shared_ptr<KnobButton> addKeyframe = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamSetPatternKeyFrameLabel));
    addKeyframe->setName(kTrackerUIParamSetPatternKeyFrame);
    addKeyframe->setHintToolTip(tr(kTrackerUIParamSetPatternKeyFrameHint));
    addKeyframe->setEvaluateOnChange(false);
    addKeyframe->setSecretByDefault(true);
    addKeyframe->setInViewerContextCanHaveShortcut(true);
    addKeyframe->setIconLabel(NATRON_IMAGES_PATH "addUserKey.png");
    trackingPage->addKnob(addKeyframe);
    _imp->ui->setKeyFrameButton = addKeyframe;

    boost::shared_ptr<KnobButton> removeKeyframe = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamRemovePatternKeyFrameLabel));
    removeKeyframe->setName(kTrackerUIParamRemovePatternKeyFrame);
    removeKeyframe->setHintToolTip(tr(kTrackerUIParamRemovePatternKeyFrameHint));
    removeKeyframe->setEvaluateOnChange(false);
    removeKeyframe->setSecretByDefault(true);
    removeKeyframe->setInViewerContextCanHaveShortcut(true);
    removeKeyframe->setIconLabel(NATRON_IMAGES_PATH "removeUserKey.png");
    trackingPage->addKnob(removeKeyframe);
    _imp->ui->removeKeyFrameButton = removeKeyframe;

    boost::shared_ptr<KnobButton> resetOffset = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamResetOffsetLabel));
    resetOffset->setName(kTrackerUIParamResetOffset);
    resetOffset->setHintToolTip(tr(kTrackerUIParamResetOffsetHint));
    resetOffset->setEvaluateOnChange(false);
    resetOffset->setSecretByDefault(true);
    resetOffset->setInViewerContextCanHaveShortcut(true);
    resetOffset->setIconLabel(NATRON_IMAGES_PATH "resetTrackOffset.png");
    trackingPage->addKnob(resetOffset);
    _imp->ui->resetOffsetButton = resetOffset;

    boost::shared_ptr<KnobButton> resetTrack = AppManager::createKnob<KnobButton>(this, tr(kTrackerUIParamResetTrackLabel));
    resetTrack->setName(kTrackerUIParamResetTrack);
    resetTrack->setHintToolTip(tr(kTrackerUIParamResetTrackHint));
    resetTrack->setEvaluateOnChange(false);
    resetTrack->setSecretByDefault(true);
    resetTrack->setInViewerContextCanHaveShortcut(true);
    resetTrack->setIconLabel(NATRON_IMAGES_PATH "restoreDefaultEnabled.png");
    trackingPage->addKnob(resetTrack);
    _imp->ui->resetTrackButton = resetTrack;


    addKnobToViewerUI(addMarker);
    addMarker->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(trackBw);
    addKnobToViewerUI(trackPrev);
    addKnobToViewerUI(trackNext);
    addKnobToViewerUI(trackFw);
    trackFw->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(trackRange);
    trackRange->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(trackAllKeys);
    addKnobToViewerUI(trackCurKey);
    trackCurKey->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(clearAllAnimation);
    addKnobToViewerUI(clearBackwardAnim);
    addKnobToViewerUI(clearForwardAnim);
    clearForwardAnim->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(updateViewer);
    addKnobToViewerUI(centerViewer);
    centerViewer->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(createKeyOnMove);
    addKnobToViewerUI(showError);
    showError->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(addKeyframe);
    addKnobToViewerUI(removeKeyframe);
    removeKeyframe->setInViewerContextItemSpacing(5);
    addKnobToViewerUI(resetOffset);
    addKnobToViewerUI(resetTrack);

    QObject::connect( getNode().get(), SIGNAL(s_refreshPreviewsAfterProjectLoadRequested()), _imp->ui.get(), SLOT(rebuildMarkerTextures()) );
}

void
TrackerNode::knobChanged(KnobI* k,
                         ValueChangedReasonEnum reason,
                         ViewSpec view,
                         double time,
                         bool originatedFromMainThread)
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();

    if (!ctx) {
        return;
    }
    ctx->knobChanged(k, reason, view, time, originatedFromMainThread);
}

void
TrackerNode::onKnobsLoaded()
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();

    if (!ctx) {
        return;
    }
    ctx->onKnobsLoaded();

    ctx->setUpdateViewer( _imp->ui->updateViewerButton.lock()->getValue() );
    ctx->setCenterOnTrack( _imp->ui->centerViewerButton.lock()->getValue() );
}

void
TrackerNode::onInputChanged(int inputNb)
{
    boost::shared_ptr<TrackerContext> ctx = getNode()->getTrackerContext();

    ctx->inputChanged(inputNb);

    _imp->ui->refreshSelectedMarkerTexture();
}


void
TrackerNode::drawOverlay(double time, const RenderScale & renderScale, ViewIdx view)
{
#if 0
    double pixelScaleX, pixelScaleY;
    ViewerGL* viewer = _imp->viewer->getViewer();

    viewer->getPixelScale(pixelScaleX, pixelScaleY);
    double viewportSize[2];
    viewer->getViewportSize(viewportSize[0], viewportSize[1]);

    {
        GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_POINT_BIT | GL_ENABLE_BIT | GL_HINT_BIT | GL_TRANSFORM_BIT);

        if (_imp->panelv1) {
            ///For each instance: <pointer,selected ? >
            const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();
            for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
                boost::shared_ptr<Node> instance = it->first.lock();

                if ( instance->isNodeDisabled() ) {
                    continue;
                }
                if (it->second) {
                    ///The track is selected, use the plug-ins interact
                    EffectInstPtr effect = instance->getEffectInstance();
                    assert(effect);
                    effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                    effect->drawOverlay_public(time, renderScale, view);
                } else {
                    ///Draw a custom interact, indicating the track isn't selected
                    boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
                    assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
                    KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
                    assert(dblKnob);

                    //GLProtectMatrix p(GL_PROJECTION); // useless (we do two glTranslate in opposite directions)
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
                            glColor4f(1., 1., 1., 1.);
                        }

                        double x = dblKnob->getValue(0);
                        double y = dblKnob->getValue(1);
                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        glVertex2d(x, y);
                        glEnd();

                        glBegin(GL_LINES);
                        glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                        glVertex2d(x + CROSS_SIZE * pixelScaleX, y);

                        glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                        glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                        glEnd();
                    }
                    glPointSize(1.);
                }
            }
        } // if (_imp->panelv1) {
        else {
            assert(_imp->panel);
            double markerColor[3];
            if ( !_imp->panel->getNode()->getOverlayColor(&markerColor[0], &markerColor[1], &markerColor[2]) ) {
                markerColor[0] = markerColor[1] = markerColor[2] = 0.8;
            }

            std::vector<TrackMarkerPtr > allMarkers;
            std::list<TrackMarkerPtr > selectedMarkers;
            boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
            context->getSelectedMarkers(&selectedMarkers);
            context->getAllMarkers(&allMarkers);

            bool showErrorColor = _imp->showCorrelationButton->isChecked();
            TrackMarkerPtr selectedMarker = _imp->selectedMarker.lock();
            Point selectedCenter, selectedPtnTopLeft, selectedPtnTopRight, selectedPtnBtmRight, selectedPtnBtmLeft, selectedOffset, selectedSearchBtmLeft, selectedSearchTopRight;

            for (std::vector<TrackMarkerPtr >::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
                if ( !(*it)->isEnabled( (*it)->getCurrentTime() ) ) {
                    continue;
                }

                bool isHoverMarker = *it == _imp->hoverMarker;
                bool isDraggedMarker = *it == _imp->interactMarker;
                bool isHoverOrDraggedMarker = isHoverMarker || isDraggedMarker;
                std::list<TrackMarkerPtr >::iterator foundSelected = std::find(selectedMarkers.begin(), selectedMarkers.end(), *it);
                bool isSelected = foundSelected != selectedMarkers.end();
                boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
                boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
                boost::shared_ptr<KnobDouble> errorKnob = (*it)->getErrorKnob();
                boost::shared_ptr<KnobDouble> ptnTopLeft = (*it)->getPatternTopLeftKnob();
                boost::shared_ptr<KnobDouble> ptnTopRight = (*it)->getPatternTopRightKnob();
                boost::shared_ptr<KnobDouble> ptnBtmRight = (*it)->getPatternBtmRightKnob();
                boost::shared_ptr<KnobDouble> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
                boost::shared_ptr<KnobDouble> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
                boost::shared_ptr<KnobDouble> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();

                if (!isSelected) {
                    ///Draw a custom interact, indicating the track isn't selected
                    glEnable(GL_LINE_SMOOTH);
                    glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
                    glLineWidth(1.5f);

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
                            glColor4f(markerColor[0], markerColor[1], markerColor[2], 1.);
                        }


                        double x = centerKnob->getValueAtTime(time, 0);
                        double y = centerKnob->getValueAtTime(time, 1);

                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);
                        glVertex2d(x, y);
                        glEnd();

                        glBegin(GL_LINES);
                        glVertex2d(x - CROSS_SIZE * pixelScaleX, y);
                        glVertex2d(x + CROSS_SIZE * pixelScaleX, y);


                        glVertex2d(x, y - CROSS_SIZE * pixelScaleY);
                        glVertex2d(x, y + CROSS_SIZE * pixelScaleY);
                        glEnd();
                    }
                    glPointSize(1.);
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
                    }

                    const QPointF innerMidLeft( (btmLeft + topLeft) / 2 );
                    const QPointF innerMidTop( (topLeft + topRight) / 2 );
                    const QPointF innerMidRight( (btmRight + topRight) / 2 );
                    const QPointF innerMidBtm( (btmLeft + btmRight) / 2 );
                    const QPointF outterMidLeft(searchLeft, searchMidY);
                    const QPointF outterMidTop(searchMidX, searchTop);
                    const QPointF outterMidRight(searchRight, searchMidY);
                    const QPointF outterMidBtm(searchMidX, searchBottom);
                    const QPointF handleSize( HANDLE_SIZE * pixelScaleX, handleSize.x() );
                    const QPointF innerMidLeftExt = computeMidPointExtent(topLeft, btmLeft, innerMidLeft, handleSize);
                    const QPointF innerMidRightExt = computeMidPointExtent(btmRight, topRight, innerMidRight, handleSize);
                    const QPointF innerMidTopExt = computeMidPointExtent(topRight, topLeft, innerMidTop, handleSize);
                    const QPointF innerMidBtmExt = computeMidPointExtent(btmLeft, btmRight, innerMidBtm, handleSize);
                    const QPointF searchTopLeft(searchLeft, searchTop);
                    const QPointF searchTopRight(searchRight, searchTop);
                    const QPointF searchBtmRight(searchRight, searchBottom);
                    const QPointF searchBtmLeft(searchLeft, searchBottom);
                    const QPointF searchTopMid(searchMidX, searchTop);
                    const QPointF searchRightMid(searchRight, searchMidY);
                    const QPointF searchLeftMid(searchLeft, searchMidY);
                    const QPointF searchBtmMid(searchMidX, searchBottom);
                    const QPointF outterMidLeftExt  = computeMidPointExtent(searchTopLeft,  searchBtmLeft,  outterMidLeft,  handleSize);
                    const QPointF outterMidRightExt = computeMidPointExtent(searchBtmRight, searchTopRight, outterMidRight, handleSize);
                    const QPointF outterMidTopExt   = computeMidPointExtent(searchTopRight, searchTopLeft,  outterMidTop,   handleSize);
                    const QPointF outterMidBtmExt   = computeMidPointExtent(searchBtmLeft,  searchBtmRight, outterMidBtm,   handleSize);
                    std::string name = (*it)->getLabel();
                    std::map<int, std::pair<Point, double> > centerPoints;
                    int floorTime = std::floor(time + 0.5);
                    boost::shared_ptr<Curve> xCurve = centerKnob->getCurve(ViewSpec::current(), 0);
                    boost::shared_ptr<Curve> yCurve = centerKnob->getCurve(ViewSpec::current(), 1);
                    boost::shared_ptr<Curve> errorCurve = errorKnob->getCurve(ViewSpec::current(), 0);

                    for (int i = 0; i < MAX_CENTER_POINTS_DISPLAYED / 2; ++i) {
                        KeyFrame k;
                        int keyTimes[2] = {floorTime + i, floorTime - i};

                        for (int j = 0; j < 2; ++j) {
                            if ( xCurve->getKeyFrameWithTime(keyTimes[j], &k) ) {
                                std::pair<Point, double>& p = centerPoints[k.getTime()];
                                p.first.x = k.getValue();
                                p.first.y = INT_MIN;

                                if ( yCurve->getKeyFrameWithTime(keyTimes[j], &k) ) {
                                    p.first.y = k.getValue();
                                }
                                if ( showErrorColor && errorCurve->getKeyFrameWithTime(keyTimes[j], &k) ) {
                                    p.second = k.getValue();
                                }
                            }
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
                        glBegin(GL_LINE_STRIP);
                        glColor3f(0.5 * l, 0.5 * l, 0.5 * l);
                        for (std::map<int, std::pair<Point, double> >::iterator it = centerPoints.begin(); it != centerPoints.end(); ++it) {
                            glVertex2d(it->second.first.x, it->second.first.y);
                        }
                        glEnd();
                        glDisable(GL_LINE_SMOOTH);

                        glEnable(GL_POINT_SMOOTH);
                        glBegin(GL_POINTS);
                        if (!showErrorColor) {
                            glColor3f(0.5 * l, 0.5 * l, 0.5 * l);
                        }

                        for (std::map<int, std::pair<Point, double> >::iterator it2 = centerPoints.begin(); it2 != centerPoints.end(); ++it2) {
                            if (showErrorColor) {
                                if (l != 0) {
                                    /*
                                     Clamp the correlation to [CORRELATION_ERROR_MIN, 1] then
                                     Map the correlation to [0, 0.33] since 0 is Red for HSV and 0.33 is Green.
                                     Also clamp to the interval if the correlation is higher, and reverse.
                                     */

                                    double error = std::min(std::max(it2->second.second, 0.), CORRELATION_ERROR_MAX_DISPLAY);
                                    double mappedError = 0.33 - 0.33 * error / CORRELATION_ERROR_MAX_DISPLAY;
                                    QColor c;
                                    c.setHsvF(mappedError, 1., 1.);
                                    glColor3f( c.redF(), c.greenF(), c.blueF() );
                                } else {
                                    glColor3f(0., 0., 0.);
                                }
                            }
                            glVertex2d(it2->second.first.x, it2->second.first.y);
                        }
                        glEnd();
                        glDisable(GL_POINT_SMOOTH);



                        glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        glBegin(GL_LINE_LOOP);
                        glVertex2d( topLeft.x(), topLeft.y() );
                        glVertex2d( topRight.x(), topRight.y() );
                        glVertex2d( btmRight.x(), btmRight.y() );
                        glVertex2d( btmLeft.x(), btmLeft.y() );
                        glEnd();

                        glBegin(GL_LINE_LOOP);
                        glVertex2d( searchTopLeft.x(), searchTopLeft.y() );
                        glVertex2d( searchTopRight.x(), searchTopRight.y() );
                        glVertex2d( searchBtmRight.x(), searchBtmRight.y() );
                        glVertex2d( searchBtmLeft.x(), searchBtmRight.y() );
                        glEnd();

                        glPointSize(POINT_SIZE);
                        glBegin(GL_POINTS);

                        ///draw center
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringCenter) || (_imp->eventState == eMouseStateDraggingCenter) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( center.x(), center.y() );

                        if ( (offset.x() != 0) || (offset.y() != 0) ) {
                            glVertex2d( center.x() + offset.x(), center.y() + offset.y() );
                        }

                        //////DRAWING INNER POINTS
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerBtmLeft) || (_imp->eventState == eMouseStateDraggingInnerBtmLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( btmLeft.x(), btmLeft.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->eventState == eMouseStateDraggingInnerBtmMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( innerMidBtm.x(), innerMidBtm.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerBtmRight) || (_imp->eventState == eMouseStateDraggingInnerBtmRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( btmRight.x(), btmRight.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->eventState == eMouseStateDraggingInnerMidLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( innerMidLeft.x(), innerMidLeft.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->eventState == eMouseStateDraggingInnerMidRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( innerMidRight.x(), innerMidRight.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerTopLeft) || (_imp->eventState == eMouseStateDraggingInnerTopLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( topLeft.x(), topLeft.y() );
                        }

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->eventState == eMouseStateDraggingInnerTopMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( innerMidTop.x(), innerMidTop.y() );
                        }

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerTopRight) || (_imp->eventState == eMouseStateDraggingInnerTopRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( topRight.x(), topRight.y() );
                        }


                        //////DRAWING OUTTER POINTS

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterBtmLeft) || (_imp->eventState == eMouseStateDraggingOuterBtmLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( searchBtmLeft.x(), searchBtmLeft.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->eventState == eMouseStateDraggingOuterBtmMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( outterMidBtm.x(), outterMidBtm.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterBtmRight) || (_imp->eventState == eMouseStateDraggingOuterBtmRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( searchBtmRight.x(), searchBtmRight.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->eventState == eMouseStateDraggingOuterMidLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( outterMidLeft.x(), outterMidLeft.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->eventState == eMouseStateDraggingOuterMidRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( outterMidRight.x(), outterMidRight.y() );
                        }

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterTopLeft) || (_imp->eventState == eMouseStateDraggingOuterTopLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( searchTopLeft.x(), searchTopLeft.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->eventState == eMouseStateDraggingOuterTopMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( outterMidTop.x(), outterMidTop.y() );
                        }
                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterTopRight) || (_imp->eventState == eMouseStateDraggingOuterTopRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                            glVertex2d( searchTopRight.x(), searchTopRight.y() );
                        }

                        glEnd();

                        if ( (offset.x() != 0) || (offset.y() != 0) ) {
                            glBegin(GL_LINES);
                            glColor3f( (float)markerColor[0] * l * 0.5, (float)markerColor[1] * l * 0.5, (float)markerColor[2] * l * 0.5 );
                            glVertex2d( center.x(), center.y() );
                            glVertex2d( center.x() + offset.x(), center.y() + offset.y() );
                            glEnd();
                        }

                        ///now show small lines at handle positions
                        glBegin(GL_LINES);

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerMidLeft) || (_imp->eventState == eMouseStateDraggingInnerMidLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( innerMidLeft.x(), innerMidLeft.y() );
                        glVertex2d( innerMidLeftExt.x(), innerMidLeftExt.y() );

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerTopMid) || (_imp->eventState == eMouseStateDraggingInnerTopMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( innerMidTop.x(), innerMidTop.y() );
                        glVertex2d( innerMidTopExt.x(), innerMidTopExt.y() );

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerMidRight) || (_imp->eventState == eMouseStateDraggingInnerMidRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( innerMidRight.x(), innerMidRight.y() );
                        glVertex2d( innerMidRightExt.x(), innerMidRightExt.y() );

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringInnerBtmMid) || (_imp->eventState == eMouseStateDraggingInnerBtmMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( innerMidBtm.x(), innerMidBtm.y() );
                        glVertex2d( innerMidBtmExt.x(), innerMidBtmExt.y() );

                        //////DRAWING OUTTER HANDLES

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterMidLeft) || (_imp->eventState == eMouseStateDraggingOuterMidLeft) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( outterMidLeft.x(), outterMidLeft.y() );
                        glVertex2d( outterMidLeftExt.x(), outterMidLeftExt.y() );

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterTopMid) || (_imp->eventState == eMouseStateDraggingOuterTopMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( outterMidTop.x(), outterMidTop.y() );
                        glVertex2d( outterMidTopExt.x(), outterMidTopExt.y() );

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterMidRight) || (_imp->eventState == eMouseStateDraggingOuterMidRight) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( outterMidRight.x(), outterMidRight.y() );
                        glVertex2d( outterMidRightExt.x(), outterMidRightExt.y() );

                        if ( isHoverOrDraggedMarker && ( (_imp->hoverState == eDrawStateHoveringOuterBtmMid) || (_imp->eventState == eMouseStateDraggingOuterBtmMid) ) ) {
                            glColor3f(0.f * l, 1.f * l, 0.f * l);
                        } else {
                            glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );
                        }
                        glVertex2d( outterMidBtm.x(), outterMidBtm.y() );
                        glVertex2d( outterMidBtmExt.x(), outterMidBtmExt.y() );
                        glEnd();

                        glColor3f( (float)markerColor[0] * l, (float)markerColor[1] * l, (float)markerColor[2] * l );

                        QColor c;
                        c.setRgbF(markerColor[0], markerColor[1], markerColor[2]);
                        viewer->renderText( center.x(), center.y(), QString::fromUtf8( name.c_str() ), c, viewer->font() );
                    } // for (int l = 0; l < 2; ++l) {
                } // if (!isSelected) {
            } // for (std::vector<TrackMarkerPtr >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

            if (_imp->showMarkerTexture) {
                _imp->drawSelectedMarkerTexture(std::make_pair(pixelScaleX, pixelScaleY), _imp->selectedMarkerTextureTime, selectedCenter, selectedOffset,  selectedPtnTopLeft, selectedPtnTopRight, selectedPtnBtmRight, selectedPtnBtmLeft, selectedSearchBtmLeft, selectedSearchTopRight);
            }
            _imp->panel->getContext()->drawInternalNodesOverlay( time, renderScale, view, _imp->viewer->getViewer() );
        } // // if (_imp->panelv1) {


        if (_imp->clickToAddTrackEnabled) {
            ///draw a square of 20px around the mouse cursor
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_LINE_SMOOTH);
            glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
            glLineWidth(1.5);
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
                
                glBegin(GL_LINE_LOOP);
                glVertex2d(_imp->lastMousePos.x() - addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() - addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() + addTrackSize * 2 * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x() + addTrackSize * 2 * pixelScaleX, _imp->lastMousePos.y() - addTrackSize * 2 * pixelScaleY);
                glEnd();
                
                ///draw a cross at the cursor position
                glBegin(GL_LINES);
                glVertex2d( _imp->lastMousePos.x() - addTrackSize * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d( _imp->lastMousePos.x() + addTrackSize * pixelScaleX, _imp->lastMousePos.y() );
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() - addTrackSize * pixelScaleY);
                glVertex2d(_imp->lastMousePos.x(), _imp->lastMousePos.y() + addTrackSize * pixelScaleY);
                glEnd();
            }
        }
    } // GLProtectAttrib a(GL_CURRENT_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT | GL_ENABLE_BIT | GL_HINT_BIT);
#endif
} // drawOverlay

bool
TrackerNode::onOverlayPenDown(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp, PenType pen)
{
#if 0
    std::pair<double, double> pixelScale;
    ViewerGL* viewer = _imp->viewer->getViewer();

    viewer->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;

    if (_imp->panel) {
        if ( _imp->panel->getContext()->onOverlayPenDownInternalNodes( time, renderScale, view, viewportPos, pos, pressure, _imp->viewer->getViewer() ) ) {
            return true;
        }
    }


    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayPenDown_public(time, renderScale, view, viewportPos, pos, pressure);
            }
        }

        double selectionTol = pixelScale.first * 10.;
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
            assert(dblKnob);
            double x, y;
            x = dblKnob->getValueAtTime(time, 0);
            y = dblKnob->getValueAtTime(time, 1);

            if ( ( pos.x() >= (x - selectionTol) ) && ( pos.x() <= (x + selectionTol) ) &&
                ( pos.y() >= (y - selectionTol) ) && ( pos.y() <= (y + selectionTol) ) ) {
                if (!it->second) {
                    _imp->panelv1->selectNode( instance, modCASIsShift(e) );
                }
                didSomething = true;
            }
        }

        if (_imp->clickToAddTrackEnabled && !didSomething) {
            NodePtr newInstance = _imp->panelv1->createNewInstance(true);
            boost::shared_ptr<KnobI> newInstanceKnob = newInstance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
            assert(dblKnob);
            dblKnob->beginChanges();
            dblKnob->blockValueChanges();
            dblKnob->setValueAtTime(time, pos.x(), view, 0);
            dblKnob->setValueAtTime(time, pos.y(), view, 1);
            dblKnob->unblockValueChanges();
            dblKnob->endChanges();
            didSomething = true;
        }

        if ( !didSomething && !modCASIsShift(e) ) {
            _imp->panelv1->clearSelection();
        }
    } else { // if (_imp->panelv1) {
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        std::vector<TrackMarkerPtr > allMarkers;
        context->getAllMarkers(&allMarkers);
        for (std::vector<TrackMarkerPtr >::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
            if ( !(*it)->isEnabled(time) ) {
                continue;
            }

            bool isSelected = context->isMarkerSelected( (*it) );
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
            boost::shared_ptr<KnobDouble> ptnTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<KnobDouble> ptnTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            boost::shared_ptr<KnobDouble> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
            boost::shared_ptr<KnobDouble> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            QPointF centerPoint;
            centerPoint.rx() = centerKnob->getValueAtTime(time, 0);
            centerPoint.ry() = centerKnob->getValueAtTime(time, 1);

            QPointF offset;
            offset.rx() = offsetKnob->getValueAtTime(time, 0);
            offset.ry() = offsetKnob->getValueAtTime(time, 1);

            if ( isNearbyPoint(centerKnob, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time) ) {
                if (_imp->controlDown > 0) {
                    _imp->eventState = eMouseStateDraggingOffset;
                } else {
                    _imp->eventState = eMouseStateDraggingCenter;
                }
                _imp->interactMarker = *it;
                didSomething = true;
            } else if ( ( (offset.x() != 0) || (offset.y() != 0) ) && isNearbyPoint(QPointF( centerPoint.x() + offset.x(), centerPoint.y() + offset.y() ), viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->eventState = eMouseStateDraggingOffset;
                _imp->interactMarker = *it;
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

                if ( isSelected && isNearbyPoint(topLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerTopLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(topRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerTopRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(btmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerBtmRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(btmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerBtmLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(midTop, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerTopMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(midRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerMidRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(midLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerMidLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isSelected && isNearbyPoint(midBtm, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingInnerBtmMid;
                    _imp->interactMarker = *it;
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

                if ( isNearbyPoint(searchTopLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterTopLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchTopRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterTopRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchBtmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterBtmRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchBtmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterBtmLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchTopMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterTopMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchBtmMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterBtmMid;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchLeftMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterMidLeft;
                    _imp->interactMarker = *it;
                    didSomething = true;
                } else if ( isNearbyPoint(searchRightMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->eventState = eMouseStateDraggingOuterMidRight;
                    _imp->interactMarker = *it;
                    didSomething = true;
                }
            }

            //If we hit the interact, make sure it is selected
            if (_imp->interactMarker) {
                if (!isSelected) {
                    context->beginEditSelection(TrackerContext::eTrackSelectionViewer);
                    if ( !modCASIsShift(e) ) {
                        context->clearSelection(TrackerContext::eTrackSelectionViewer);
                    }
                    context->addTrackToSelection(_imp->interactMarker, TrackerContext::eTrackSelectionViewer);
                    context->endEditSelection(TrackerContext::eTrackSelectionViewer);
                }
            }

            if (didSomething) {
                break;
            }
        } // for (std::vector<TrackMarkerPtr >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

        if (_imp->clickToAddTrackEnabled && !didSomething) {
            TrackMarkerPtr marker = context->createMarker();
            boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
            centerKnob->setValuesAtTime(time, pos.x(), pos.y(), view, eValueChangedReasonNatronInternalEdited);
            if ( _imp->createKeyOnMoveButton->isChecked() ) {
                marker->setUserKeyframe(time);
            }
            _imp->panel->pushUndoCommand( new AddTrackCommand(marker, context) );
            updateSelectedMarkerTexture();
            didSomething = true;
        }

        if ( !didSomething && _imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->isNearbySelectedMarkerTextureResizeAnchor(pos) ) {
            _imp->eventState = eMouseStateDraggingSelectedMarkerResizeAnchor;
            didSomething = true;
        }

        if ( !didSomething && _imp->showMarkerTexture && _imp->selectedMarkerTexture  && _imp->isInsideSelectedMarkerTextureResizeAnchor(pos) ) {
            if (_imp->shiftDown) {
                _imp->eventState = eMouseStateScalingSelectedMarker;
            } else {
                _imp->eventState = eMouseStateDraggingSelectedMarker;
            }
            _imp->interactMarker = _imp->selectedMarker.lock();
            didSomething = true;
        }

        if (!didSomething) {
            int keyTime = _imp->isInsideKeyFrameTexture(time, pos, viewportPos);
            if (keyTime != INT_MAX) {
                _imp->viewer->seek(keyTime);
                didSomething = true;
            }
        }
        if (!didSomething) {
            std::list<TrackMarkerPtr > selectedMarkers;
            context->getSelectedMarkers(&selectedMarkers);
            if ( !selectedMarkers.empty() ) {
                context->clearSelection(TrackerContext::eTrackSelectionViewer);
                
                didSomething = true;
            }
        }
    }
    _imp->lastMousePos = pos;
    
    return didSomething;
#endif
} // penDown

bool
TrackerNode::onOverlayPenMotion(double time, const RenderScale & renderScale, ViewIdx view,
                              const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp)
{
#if 0
    std::pair<double, double> pixelScale;
    ViewerGL* viewer = _imp->viewer->getViewer();

    viewer->getPixelScale(pixelScale.first, pixelScale.second);
    bool didSomething = false;

    if (_imp->panel) {
        if ( _imp->panel->getContext()->onOverlayPenMotionInternalNodes( time, renderScale, view, viewportPos, pos, pressure, _imp->viewer->getViewer() ) ) {
            return true;
        }
    }

    Point delta;
    delta.x = pos.x() - _imp->lastMousePos.x();
    delta.y = pos.y() - _imp->lastMousePos.y();

    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();

        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                if ( effect->onOverlayPenMotion_public(time, renderScale, view, viewportPos, pos, pressure) ) {
                    didSomething = true;
                }
            }
        }
    } else {
        if (_imp->hoverState != eDrawStateInactive) {
            _imp->hoverState = eDrawStateInactive;
            _imp->hoverMarker.reset();
            didSomething = true;
        }

        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        std::vector<TrackMarkerPtr > allMarkers;
        context->getAllMarkers(&allMarkers);

        bool hoverProcess = false;
        for (std::vector<TrackMarkerPtr >::iterator it = allMarkers.begin(); it != allMarkers.end(); ++it) {
            if ( !(*it)->isEnabled(time) ) {
                continue;
            }

            bool isSelected = context->isMarkerSelected( (*it) );
            boost::shared_ptr<KnobDouble> centerKnob = (*it)->getCenterKnob();
            boost::shared_ptr<KnobDouble> offsetKnob = (*it)->getOffsetKnob();
            boost::shared_ptr<KnobDouble> ptnTopLeft = (*it)->getPatternTopLeftKnob();
            boost::shared_ptr<KnobDouble> ptnTopRight = (*it)->getPatternTopRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmRight = (*it)->getPatternBtmRightKnob();
            boost::shared_ptr<KnobDouble> ptnBtmLeft = (*it)->getPatternBtmLeftKnob();
            boost::shared_ptr<KnobDouble> searchWndTopRight = (*it)->getSearchWindowTopRightKnob();
            boost::shared_ptr<KnobDouble> searchWndBtmLeft = (*it)->getSearchWindowBottomLeftKnob();
            QPointF center;
            center.rx() = centerKnob->getValueAtTime(time, 0);
            center.ry() = centerKnob->getValueAtTime(time, 1);

            QPointF offset;
            offset.rx() = offsetKnob->getValueAtTime(time, 0);
            offset.ry() = offsetKnob->getValueAtTime(time, 1);
            if ( isNearbyPoint(centerKnob, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE, time) ) {
                _imp->hoverState = eDrawStateHoveringCenter;
                _imp->hoverMarker = *it;
                hoverProcess = true;
            } else if ( ( (offset.x() != 0) || (offset.y() != 0) ) && isNearbyPoint(QPointF( center.x() + offset.x(), center.y() + offset.y() ), viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                _imp->hoverState = eDrawStateHoveringCenter;
                _imp->hoverMarker = *it;
                didSomething = true;
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


                if ( isSelected && isNearbyPoint(topLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerTopLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(topRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerTopRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(btmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(btmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(midTop, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerTopMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(midRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerMidRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(midLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerMidLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isSelected && isNearbyPoint(midBtm, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringInnerBtmMid;
                    _imp->hoverMarker = *it;
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

                if ( isNearbyPoint(searchTopLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterTopLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchTopRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterTopRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchBtmRight, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchBtmLeft, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchTopMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterTopMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchBtmMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterBtmMid;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchLeftMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterMidLeft;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                } else if ( isNearbyPoint(searchRightMid, viewer, viewportPos.x(), viewportPos.y(), POINT_TOLERANCE) ) {
                    _imp->hoverState = eDrawStateHoveringOuterMidRight;
                    _imp->hoverMarker = *it;
                    hoverProcess = true;
                }
            }

            if (hoverProcess) {
                break;
            }
        } // for (std::vector<TrackMarkerPtr >::iterator it = allMarkers.begin(); it!=allMarkers.end(); ++it) {

        if ( _imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->isNearbySelectedMarkerTextureResizeAnchor(pos) ) {
            _imp->viewer->getViewer()->setCursor(Qt::SizeFDiagCursor);
            hoverProcess = true;
        } else if ( _imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->isInsideSelectedMarkerTextureResizeAnchor(pos) ) {
            _imp->viewer->getViewer()->setCursor(Qt::SizeAllCursor);
            hoverProcess = true;
        } else if ( _imp->showMarkerTexture && (_imp->isInsideKeyFrameTexture(time, pos, viewportPos) != INT_MAX) ) {
            _imp->viewer->getViewer()->setCursor(Qt::PointingHandCursor);
            hoverProcess = true;
        } else {
            _imp->viewer->getViewer()->unsetCursor();
        }

        if ( _imp->showMarkerTexture && _imp->selectedMarkerTexture && _imp->shiftDown && _imp->isInsideSelectedMarkerTextureResizeAnchor(pos) ) {
            _imp->hoverState = eDrawStateShowScalingHint;
            hoverProcess = true;
        }

        if (hoverProcess) {
            didSomething = true;
        }

        boost::shared_ptr<KnobDouble> centerKnob, offsetKnob, searchWndTopRight, searchWndBtmLeft;
        boost::shared_ptr<KnobDouble> patternCorners[4];
        if (_imp->interactMarker) {
            centerKnob = _imp->interactMarker->getCenterKnob();
            offsetKnob = _imp->interactMarker->getOffsetKnob();

            /*

             TopLeft(0) ------------- Top right(3)
             |                        |
             |                        |
             |                        |
             Btm left (1) ------------ Btm right (2)

             */
            patternCorners[0] = _imp->interactMarker->getPatternTopLeftKnob();
            patternCorners[1] = _imp->interactMarker->getPatternBtmLeftKnob();
            patternCorners[2] = _imp->interactMarker->getPatternBtmRightKnob();
            patternCorners[3] = _imp->interactMarker->getPatternTopRightKnob();
            searchWndTopRight = _imp->interactMarker->getSearchWindowTopRightKnob();
            searchWndBtmLeft = _imp->interactMarker->getSearchWindowBottomLeftKnob();
        }


        switch (_imp->eventState) {
            case eMouseStateDraggingCenter:
            case eMouseStateDraggingOffset: {
                assert(_imp->interactMarker);
                if (_imp->eventState == eMouseStateDraggingOffset) {
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
                updateSelectedMarkerTexture();
                if ( _imp->createKeyOnMoveButton->isChecked() ) {
                    _imp->interactMarker->setUserKeyframe(time);
                }
                didSomething = true;
                break;
            }
            case eMouseStateDraggingInnerBtmLeft:
            case eMouseStateDraggingInnerTopRight:
            case eMouseStateDraggingInnerTopLeft:
            case eMouseStateDraggingInnerBtmRight: {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
                    break;
                }

                int index;
                if (_imp->eventState == eMouseStateDraggingInnerBtmLeft) {
                    index = 1;
                } else if (_imp->eventState == eMouseStateDraggingInnerBtmRight) {
                    index = 2;
                } else if (_imp->eventState == eMouseStateDraggingInnerTopRight) {
                    index = 3;
                } else if (_imp->eventState == eMouseStateDraggingInnerTopLeft) {
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

                //Clamp so the 4 points remaing the same in the homography
                if (prevVec.x * nextVec.y - prevVec.y * nextVec.x < 0.) {     // cross-product
                    findLineIntersection(cur, prev, next, &cur);
                }
                if (nextDiagVec.x * prevVec.y - nextDiagVec.y * prevVec.x < 0.) {     // cross-product
                    findLineIntersection(cur, prev, diag, &cur);
                }
                if (nextVec.x * prevDiagVec.y - nextVec.y * prevDiagVec.x < 0.) {     // cross-product
                    findLineIntersection(cur, next, diag, &cur);
                }


                Point searchWindowCorners[2];
                searchWindowCorners[0].x = searchWndBtmLeft->getValueAtTime(time, 0) + center.x + offset.x;
                searchWindowCorners[0].y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y;

                searchWindowCorners[1].x = searchWndTopRight->getValueAtTime(time, 0)  + center.x + offset.x;
                searchWindowCorners[1].y = searchWndTopRight->getValueAtTime(time, 1)  + center.y + offset.y;

                cur.x = std::max(std::min(cur.x, searchWindowCorners[1].x), searchWindowCorners[0].x);
                cur.y = std::max(std::min(cur.y, searchWindowCorners[1].y), searchWindowCorners[0].y);

                cur.x -= (center.x + offset.x);
                cur.y -= (center.y + offset.y);

                patternCorners[index]->setValuesAtTime(time, cur.x, cur.y, view, eValueChangedReasonNatronInternalEdited);

                if ( _imp->createKeyOnMoveButton->isChecked() ) {
                    _imp->interactMarker->setUserKeyframe(time);
                }
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterBtmLeft: {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
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
                p.y = searchWndBtmLeft->getValueAtTime(time, 1) + center.y + offset.y + delta.y;

                Point topLeft;
                topLeft.x = patternCorners[0]->getValueAtTime(time, 0) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time, 1) + center.y + offset.y;
                Point btmLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time, 0) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time, 1) + center.y + offset.y;
                Point btmRight;
                btmRight.y = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
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

                updateSelectedMarkerTexture();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterBtmRight: {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
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
                btmRight.y = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
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

                updateSelectedMarkerTexture();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterTopRight: {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
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
                p.y = searchWndTopRight->getValueAtTime(time, 1) + center.y + offset.y + delta.y;

                Point topLeft;
                topLeft.x = patternCorners[0]->getValueAtTime(time, 0) + center.x + offset.x;
                topLeft.y = patternCorners[0]->getValueAtTime(time, 1) + center.y + offset.y;
                Point btmLeft;
                btmLeft.x = patternCorners[1]->getValueAtTime(time, 0) + center.x + offset.x;
                btmLeft.y = patternCorners[1]->getValueAtTime(time, 1) + center.y + offset.y;
                Point btmRight;
                btmRight.y = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
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

                updateSelectedMarkerTexture();
                didSomething = true;
                break;
            }
            case eMouseStateDraggingOuterTopLeft: {
                if (_imp->controlDown == 0) {
                    _imp->transformPattern(time, _imp->eventState, delta);
                    didSomething = true;
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
                btmRight.y = patternCorners[2]->getValueAtTime(time, 0) + center.x + offset.x;
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

                updateSelectedMarkerTexture();
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
                _imp->transformPattern(time, _imp->eventState, delta);
                didSomething = true;
                break;
            }
            case eMouseStateDraggingSelectedMarkerResizeAnchor: {
                QPointF lastPosWidget = viewer->toWidgetCoordinates(_imp->lastMousePos);
                double dx = viewportPos.x() - lastPosWidget.x();
                _imp->selectedMarkerWidth += dx;
                _imp->selectedMarkerWidth = std::max(_imp->selectedMarkerWidth, 10);
                didSomething = true;
                break;
            }
            case eMouseStateScalingSelectedMarker: {
                TrackMarkerPtr marker = _imp->selectedMarker.lock();
                assert(marker);
                RectD markerMagRect;
                _imp->computeSelectedMarkerCanonicalRect(&markerMagRect);
                boost::shared_ptr<KnobDouble> centerKnob = marker->getCenterKnob();
                boost::shared_ptr<KnobDouble> offsetKnob = marker->getOffsetKnob();
                boost::shared_ptr<KnobDouble> searchBtmLeft = marker->getSearchWindowBottomLeftKnob();
                boost::shared_ptr<KnobDouble> searchTopRight = marker->getSearchWindowTopRightKnob();
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
                
                double prevDist = std::sqrt( (_imp->lastMousePos.x() - centerPoint.x ) * ( _imp->lastMousePos.x() - centerPoint.x) + ( _imp->lastMousePos.y() - centerPoint.y) * ( _imp->lastMousePos.y() - centerPoint.y) );
                if (prevDist != 0) {
                    double dist = std::sqrt( ( pos.x() - centerPoint.x) * ( pos.x() - centerPoint.x) + ( pos.y() - centerPoint.y) * ( pos.y() - centerPoint.y) );
                    double ratio = dist / prevDist;
                    _imp->selectedMarkerScale.x *= ratio;
                    _imp->selectedMarkerScale.x = std::max( 0.05, std::min(1., _imp->selectedMarkerScale.x) );
                    _imp->selectedMarkerScale.y = _imp->selectedMarkerScale.x;
                    didSomething = true;
                }
                break;
            }
            case eMouseStateDraggingSelectedMarker: {
                double x = centerKnob->getValueAtTime(time, 0);
                double y = centerKnob->getValueAtTime(time, 1);
                double dx = delta.x *  _imp->selectedMarkerScale.x;
                double dy = delta.y *  _imp->selectedMarkerScale.y;
                x -= dx;
                y -= dy;
                centerKnob->setValuesAtTime(time, x, y, view, eValueChangedReasonPluginEdited);
                for (int i = 0; i < 4; ++i) {
                    for (int d = 0; d < patternCorners[i]->getDimension(); ++d) {
                        patternCorners[i]->setValueAtTime(time, patternCorners[i]->getValueAtTime(time, d), view, d);
                    }
                }
                if ( _imp->createKeyOnMoveButton->isChecked() ) {
                    _imp->interactMarker->setUserKeyframe(time);
                }
                updateSelectedMarkerTexture();
                didSomething = true;
                break;
            }
            default:
                break;
        } // switch
    }
    if (_imp->clickToAddTrackEnabled) {
        ///Refresh the overlay
        didSomething = true;
    }
    _imp->lastMousePos = pos;
    
    return didSomething;
#endif
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
TrackerNode::onOverlayPenUp(double time, const RenderScale & renderScale, ViewIdx view, const QPointF & viewportPos, const QPointF & pos, double pressure, double timestamp)
{
#if 0
    bool didSomething = false;

    if (_imp->panel) {
        if ( _imp->panel->getContext()->onOverlayPenUpInternalNodes( time, renderScale, view, viewportPos, pos, pressure, _imp->viewer->getViewer() ) ) {
            return true;
        }
    }


    TrackerMouseStateEnum state = _imp->eventState;
    _imp->eventState = eMouseStateIdle;
    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();

        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayPenUp_public(time, renderScale, view, viewportPos, pos, pressure);
                if (didSomething) {
                    return true;
                }
            }
        }
    } else { // if (_imp->panelv1) {
        _imp->interactMarker.reset();
        (void)state;
    } // if (_imp->panelv1) {

    return didSomething;
#endif

} // penUp

bool
TrackerNode::onOverlayKeyDown(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers)
{
#if 0
    bool didSomething = false;
    Qt::KeyboardModifiers modifiers = e->modifiers();
    Qt::Key key = (Qt::Key)e->key();
    Key natronKey = QtEnumConvert::fromQtKey(key);
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(modifiers);

    if (_imp->panel) {
        if ( _imp->panel->getContext()->onOverlayKeyDownInternalNodes( time, renderScale, view, natronKey, natronMod,  _imp->viewer->getViewer() ) ) {
            return true;
        }
    }

    if (e->key() == Qt::Key_Control) {
        ++_imp->controlDown;
    } else if (e->key() == Qt::Key_Shift) {
        ++_imp->shiftDown;
    }


    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayKeyDown_public(time, renderScale, view, natronKey, natronMod);
                if (didSomething) {
                    return true;
                }
            }
        }
    }

    if ( modCASIsControlAlt(e) && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = true;
        _imp->addTrackButton->setDown(true);
        _imp->addTrackButton->setChecked(true);
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingSelectAll, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->onSelectAllButtonClicked();
            std::list<Node*> selectedInstances;
            _imp->panelv1->getSelectedInstances(&selectedInstances);
            didSomething = !selectedInstances.empty();
        } else {
            _imp->panel->getContext()->selectAll(TrackerContext::eTrackSelectionInternal);
            didSomething = false; //viewer is refreshed already
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingDelete, modifiers, key) ) {
        if (_imp->panelv1) {
            _imp->panelv1->onDeleteKeyPressed();
            std::list<Node*> selectedInstances;
            _imp->panelv1->getSelectedInstances(&selectedInstances);
            didSomething = !selectedInstances.empty();
        } else {
            _imp->panel->onRemoveButtonClicked();
            didSomething = true;
        }
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingBackward, modifiers, key) ) {
        onTrackBwClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingPrevious, modifiers, key) ) {
        onTrackPrevClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingNext, modifiers, key) ) {
        onTrackNextClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingForward, modifiers, key) ) {
        onTrackFwClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingStop, modifiers, key) ) {
        onStopButtonClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingRange, modifiers, key) ) {
        onTrackRangeClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingAllKeyframes, modifiers, key) ) {
        onTrackAllKeyframesClicked();
        didSomething = true;
    } else if ( isKeybind(kShortcutGroupTracking, kShortcutIDActionTrackingCurrentKeyframes, modifiers, key) ) {
        onTrackCurrentKeyframeClicked();
        didSomething = true;
    }

    
    return didSomething;
#endif
} // keydown

bool
TrackerNode::onOverlayKeyUp(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers)
{
#if 0
    Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );

    if (_imp->panel) {
        if ( _imp->panel->getContext()->onOverlayKeyUpInternalNodes( time, renderScale, view, natronKey, natronMod,  _imp->viewer->getViewer() ) ) {
            return true;
        }
    }

    bool didSomething = false;

    if (e->key() == Qt::Key_Control) {
        if (_imp->controlDown > 0) {
            --_imp->controlDown;
        }
    } else if (e->key() == Qt::Key_Shift) {
        if (_imp->shiftDown > 0) {
            --_imp->shiftDown;
        }
        if (_imp->eventState == eMouseStateScalingSelectedMarker) {
            _imp->eventState = eMouseStateIdle;
            didSomething = true;
        }
    }

    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething = effect->onOverlayKeyUp_public(time, renderScale, view, natronKey, natronMod);
                if (didSomething) {
                    return true;
                }
            }
        }
    }
    if ( _imp->clickToAddTrackEnabled && ( (e->key() == Qt::Key_Control) || (e->key() == Qt::Key_Alt) ) ) {
        _imp->clickToAddTrackEnabled = false;
        _imp->addTrackButton->setDown(false);
        _imp->addTrackButton->setChecked(false);
        didSomething = true;
    }

    return didSomething;
#endif
} // KeyUp

bool
TrackerNode::onOverlayKeyRepeat(double time, const RenderScale & renderScale, ViewIdx view, Key key, KeyboardModifiers modifiers)
{

} // keyrepeat

bool
TrackerNode::onOverlayFocusGained(double time, const RenderScale & renderScale, ViewIdx view)
{

} // gainFocus

bool
TrackerNode::onOverlayFocusLost(double time, const RenderScale & renderScale, ViewIdx view)
{
#if 0
    _imp->controlDown = 0;
    _imp->shiftDown = 0;

    if (_imp->panelv1) {
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            if ( it->second && !instance->isNodeDisabled() ) {
                EffectInstPtr effect = instance->getEffectInstance();
                assert(effect);
                effect->setCurrentViewportForOverlays_public( _imp->viewer->getViewer() );
                didSomething |= effect->onOverlayFocusLost_public(time, renderScale, view);
            }
        }
    }

    return didSomething;
#endif
} // loseFocus

void
TrackerNode::onInteractViewportSelectionCleared()
{
#if 0
    if (_imp->panelv1) {
        _imp->panelv1->clearSelection();
    } else {
        _imp->panel->getContext()->clearSelection(TrackerContext::eTrackSelectionViewer);
    }
#endif
}

void
TrackerNode::onInteractViewportSelectionUpdated(const RectD& rectangle, bool onRelease)
{
#if 0
    if (!onRelease) {
        return;
    }


    double l, r, b, t;
    _imp->viewer->getViewer()->getSelectionRectangle(l, r, b, t);

    if (_imp->panelv1) {
        std::list<Node*> currentSelection;
        const std::list<std::pair<NodeWPtr, bool> > & instances = _imp->panelv1->getInstances();
        for (std::list<std::pair<NodeWPtr, bool> >::const_iterator it = instances.begin(); it != instances.end(); ++it) {
            NodePtr instance = it->first.lock();
            boost::shared_ptr<KnobI> newInstanceKnob = instance->getKnobByName("center");
            assert(newInstanceKnob); //< if it crashes here that means the parameter's name changed in the OpenFX plug-in.
            KnobDouble* dblKnob = dynamic_cast<KnobDouble*>( newInstanceKnob.get() );
            assert(dblKnob);
            double x, y;
            x = dblKnob->getValue(0);
            y = dblKnob->getValue(1);
            if ( (x >= l) && (x <= r) && (y >= b) && (y <= t) ) {
                ///assert that the node is really not part of the selection
                assert( std::find( currentSelection.begin(), currentSelection.end(), instance.get() ) == currentSelection.end() );
                currentSelection.push_back( instance.get() );
            }
        }
        _imp->panelv1->selectNodes( currentSelection, (_imp->controlDown > 0) );
    } else {
        std::vector<TrackMarkerPtr > allMarkers;
        std::list<TrackMarkerPtr > selectedMarkers;
        boost::shared_ptr<TrackerContext> context = _imp->panel->getContext();
        context->getAllMarkers(&allMarkers);
        for (std::size_t i = 0; i < allMarkers.size(); ++i) {
            if ( !allMarkers[i]->isEnabled( allMarkers[i]->getCurrentTime() ) ) {
                continue;
            }
            boost::shared_ptr<KnobDouble> center = allMarkers[i]->getCenterKnob();
            double x, y;
            x = center->getValue(0);
            y = center->getValue(1);
            if ( (x >= l) && (x <= r) && (y >= b) && (y <= t) ) {
                selectedMarkers.push_back(allMarkers[i]);
            }
        }

        context->beginEditSelection(TrackerContext::eTrackSelectionInternal);
        context->clearSelection(TrackerContext::eTrackSelectionInternal);
        context->addTracksToSelection(selectedMarkers, TrackerContext::eTrackSelectionInternal);
        context->endEditSelection(TrackerContext::eTrackSelectionInternal);
    }
#endif
}

void
TrackerNode::refreshExtraStateAfterTimeChanged(double time)
{
#if 0
    if ( _imp->showMarkerTexture && (reason != eTimelineChangeReasonPlaybackSeek) ) {
        _imp->refreshSelectedMarkerTexture();
    }
#endif
}

NATRON_NAMESPACE_EXIT;

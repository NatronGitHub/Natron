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

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "ViewerTab.h"
#include "ViewerTabPrivate.h"

#include <cassert>
#include <stdexcept>

#include <QtCore/QDebug>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/KnobTypes.h"
#include "Engine/OSGLFunctions.h"
#include "Engine/OverlayInteractBase.h"
#include "Engine/Transform.h"
#include "Engine/ViewIdx.h"
#include "Engine/ViewerInstance.h"
#include "Engine/ViewerNode.h"

#include "Gui/NodeSettingsPanel.h"
#include "Gui/Gui.h"
#include "Gui/GuiApplicationManager.h" // isKeybind
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGui.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/ViewerGL.h"


NATRON_NAMESPACE_ENTER

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
//OpenGL is column-major for matrixes
static void
transformToOpenGLMatrix(const Transform::Matrix3x3& mat,
                        GLdouble* oglMat)
{
    oglMat[0] = mat(0,0); oglMat[4] = mat(0,1); oglMat[8] = 0; oglMat[12] = mat(0,2);
    oglMat[1] = mat(1,0); oglMat[5] = mat(1,1); oglMat[9]  = 0; oglMat[13] = mat(1,2);
    oglMat[2] = 0;     oglMat[6] = 0;     oglMat[10] = 1; oglMat[14] = 0;
    oglMat[3] = mat(2,0); oglMat[7] = mat(2,1); oglMat[11] = 0; oglMat[15] = mat(2,2);
}

#endif


void
ViewerTab::getNodesEntitledForOverlays(TimeValue time, ViewIdx view, NodesList & nodes) const
{
    assert(QThread::currentThread() == qApp->thread());

    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }

    NodesList nodesWithPanelOpened;
    std::list<DockablePanelI*> panels = app->getOpenedSettingsPanels();

    std::set<ViewerNodePtr> viewerNodesWithPanelOpened;
    for (std::list<DockablePanelI*>::const_iterator it = panels.begin();
         it != panels.end(); ++it) {
        NodeSettingsPanel* panel = dynamic_cast<NodeSettingsPanel*>(*it);
        if (!panel) {
            continue;
        }
        NodeGuiPtr node = panel->getNodeGui();
        NodePtr internalNode = node->getNode();
        if (node && internalNode) {
            if ( internalNode->shouldDrawOverlay(time, view, eOverlayViewportTypeViewer) ) {
                ViewerNodePtr isViewer = internalNode->isEffectViewerNode();
                if (!isViewer) {
                    // Do not add viewers, add them afterwards
                    nodesWithPanelOpened.push_back( node->getNode() );
                }
            }
        }
    }

    // Now remove from the nodesWithPanelOpened list nodes that are not upstream of this viewer node
    NodePtr thisNode = getInternalNode()->getNode();
    for (NodesList::const_iterator it = nodesWithPanelOpened.begin(); it != nodesWithPanelOpened.end(); ++it) {
        if (thisNode->isNodeUpstream(*it)) {
            nodes.push_back(*it);
        }
    }

    // Also add the viewer itself
    nodes.push_back(thisNode);

} // getNodesEntitledForOverlays


void
ViewerTab::drawOverlays(TimeValue time,
                        const RenderScale & renderScale) const
{
    Gui* gui = getGui();
    if (!gui) {
        return;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return;
    }

    bool isDrawing = bool( app->getActiveRotoDrawingStroke() );

    if ( !_imp->viewer ||
         app->isClosing() ||
         isFileDialogViewer() ||
         (gui->isGUIFrozen() && !isDrawing) ) {
        return;
    }

    if ( app->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return;
    }

    ViewIdx view = getInternalNode()->getCurrentRenderView();
    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);

    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        NodeGuiPtr nodeUi = boost::dynamic_pointer_cast<NodeGui>( (*it)->getNodeGui() );
        ViewerNodePtr isViewerNode = (*it)->isEffectViewerNode();
        if (isViewerNode && isViewerNode != getInternalNode()) {
            continue;
        }
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        TimeValue transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        if (!nodeUi) {
            continue;
        }
        bool overlayDeemed = false;
        if (ok) {
            if (time != transformedTime) {
                //when retimed, modify the overlay color so it looks deemed to indicate that the user
                //cannot modify it
                overlayDeemed = true;
            }
            time = transformedTime;
        }
        nodeUi->setOverlayLocked(overlayDeemed);

        Transform::Matrix3x3 mat(1, 0, 0, 0, 1, 0, 0, 0, 1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        GLfloat oldMat[16];
        if (ok) {
            //Ok we've got a transform here, apply it to the OpenGL model view matrix

            GLdouble oglMat[16];
            transformToOpenGLMatrix(mat, oglMat);
            GL_GPU::MatrixMode(GL_MODELVIEW);
            GL_GPU::GetFloatv(GL_MODELVIEW_MATRIX, oldMat);
            GL_GPU::MultMatrixd(oglMat);
        }

#endif
        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            EffectInstancePtr effect = (*it)->getEffectInstance();
            assert(effect);
            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
                (*it)->drawOverlay_public(_imp->viewer, time, renderScale, view);
            }
        }

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        if (ok) {
            GL_GPU::MatrixMode(GL_MODELVIEW);
            GL_GPU::LoadMatrixf(oldMat);
        }
#endif
    }
} // ViewerTab::drawOverlays

bool
ViewerTab::notifyOverlaysPenDown_internal(const NodePtr& node,
                                          const RenderScale & renderScale,
                                          PenType pen,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure,
                                          TimeValue timestamp)
{
    ViewerNodePtr isViewerNode = node->isEffectViewerNode();
    if (isViewerNode && isViewerNode != getInternalNode()) {
        return false;
    }
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

    QPointF transformPos;
#ifndef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    transformPos = pos;
#else
    TimeValue transformedTime;
    bool ok = _imp->getTimeTransform(time, view, node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }

    Transform::Matrix3x3 mat(1, 0, 0, 0, 1, 0, 0, 0, 1);
    ok = _imp->getOverlayTransform(time, view, node, getInternalNode(), &mat);
    if (!ok) {
        transformPos = pos;
    } else {
        Transform::Matrix3x3 inverse;
        if (!mat.inverse(&inverse)) {
            inverse.setIdentity();
        }
        mat = inverse;
        {
            Transform::Point3D p;
            p.x = pos.x();
            p.y = pos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            if (p.z < 0) {
                // the transformed position has a negative z, don't send it
                return false;
            }
            transformPos.rx() = p.x / p.z;
            transformPos.ry() = p.y / p.z;
        }
    }
#endif

    bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(node);
    if (!isInActiveViewerUI) {
        EffectInstancePtr effect = node->getEffectInstance();
        assert(effect);
        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it = overlays.begin(); it != overlays.end(); ++it) {
            bool didSmthing = (*it)->onOverlayPenDown_public(_imp->viewer, time, renderScale, view, viewportPos, transformPos, pressure, timestamp, pen);
            if (didSmthing) {
                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion

                // to any other interactive object it may own that shares the same view.
                _imp->lastOverlayNode = node;

                // Only enable draft render for subsequent penMotion events if the node allows it
                _imp->canEnableDraftOnPenMotion = node->isDraftEnabledForOverlayActions();
                return true;
            }
        }


    }


    return false;
} // ViewerTab::notifyOverlaysPenDown_internal

bool
ViewerTab::notifyOverlaysPenDown(const RenderScale & renderScale,
                                 PenType pen,
                                 const QPointF & viewportPos,
                                 const QPointF & pos,
                                 double pressure,
                                 TimeValue timestamp)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }

    if ( app->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return false;
    }


    _imp->hasPenDown = true;
    _imp->hasCaughtPenMotionWhileDragging = false;

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);


    NodePtr lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if ( notifyOverlaysPenDown_internal(*it, renderScale, pen, viewportPos, pos, pressure, timestamp) ) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }

    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if ( notifyOverlaysPenDown_internal(*it, renderScale, pen, viewportPos, pos, pressure, timestamp) ) {
            return true;
        }
    }


    return false;
}

bool
ViewerTab::notifyOverlaysPenDoubleClick(const RenderScale & renderScale,
                                        const QPointF & viewportPos,
                                        const QPointF & pos)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);


    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        ViewerNodePtr isViewerNode = (*it)->isEffectViewerNode();
        if (isViewerNode && isViewerNode != getInternalNode()) {
            continue;
        }

        QPointF transformPos;
#ifndef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        transformPos = pos;
#else
        TimeValue transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }

        Transform::Matrix3x3 mat(1, 0, 0, 0, 1, 0, 0, 0, 1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        if (!ok) {
            transformPos = pos;
        } else {
            Transform::Matrix3x3 inverse;
            if (!mat.inverse(&inverse)) {
                inverse.setIdentity();
            }
            mat = inverse;

            {
                Transform::Point3D p;
                p.x = pos.x();
                p.y = pos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                if (p.z < 0) {
                    // the transformed position has a negative z, don't send it
                    return false;
                }
                transformPos.rx() = p.x / p.z;
                transformPos.ry() = p.y / p.z;
            }
        }
#endif

        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            EffectInstancePtr effect = (*it)->getEffectInstance();
            assert(effect);

            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                bool didSmthing = (*it2)->onOverlayPenDoubleClicked_public(_imp->viewer, time, renderScale, view, viewportPos, transformPos);
                if (didSmthing) {
                    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                    // if the instance returns kOfxStatOK, the host should not pass the pen motion

                    // to any other interactive object it may own that shares the same view.
                    _imp->lastOverlayNode = *it;
                    
                    return true;
                }
            }

        }
    }

    return false;
} // ViewerTab::notifyOverlaysPenDoubleClick

bool
ViewerTab::notifyOverlaysPenMotion_internal(const NodePtr& node,
                                            const RenderScale & renderScale,
                                            const QPointF & viewportPos,
                                            const QPointF & pos,
                                            double pressure,
                                            TimeValue timestamp)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    ViewerNodePtr isViewerNode = node->isEffectViewerNode();
    if (isViewerNode && isViewerNode != getInternalNode()) {
        return false;
    }

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

    QPointF transformPos;
#ifndef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    transformPos = pos;
#else
    TimeValue transformedTime;
    bool ok = _imp->getTimeTransform(time, view, node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }


    Transform::Matrix3x3 mat(1, 0, 0, 0, 1, 0, 0, 0, 1);
    ok = _imp->getOverlayTransform(time, view, node, getInternalNode(), &mat);
    if (!ok) {
        transformPos = pos;
    } else {
        Transform::Matrix3x3 inverse;
        if (!mat.inverse(&inverse)) {
            inverse.setIdentity();
        }
        mat = inverse;

        {
            Transform::Point3D p;
            p.x = pos.x();
            p.y = pos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            if (p.z < 0) {
                // the transformed position has a negative z, don't send it
                return false;
            }
            transformPos.rx() = p.x / p.z;
            transformPos.ry() = p.y / p.z;
        }
    }
#endif

    bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(node);
    if (!isInActiveViewerUI) {
        ///If we are dragging with mouse, set draft mode (not for roto though)
        if ( _imp->hasPenDown && _imp->canEnableDraftOnPenMotion && !getGui()->isDraftRenderEnabled() ) {
            getGui()->setDraftRenderEnabled(true);
        }

        EffectInstancePtr effect = node->getEffectInstance();
        assert(effect);

        std::list<OverlayInteractBasePtr> overlays;
        effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
        for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
            bool didSmthing = (*it2)->onOverlayPenMotion_public(_imp->viewer, time, renderScale, view, viewportPos, transformPos, pressure, timestamp);
            if (didSmthing) {
                if (_imp->hasPenDown) {
                    _imp->hasCaughtPenMotionWhileDragging = true;
                }

                //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                // if the instance returns kOfxStatOK, the host should not pass the pen motion

                // to any other interactive object it may own that shares the same view.
                _imp->lastOverlayNode = node;
                
                return true;
            }
        }

    }


    return false;
} // ViewerTab::notifyOverlaysPenMotion_internal

bool
ViewerTab::notifyOverlaysPenMotion(const RenderScale & renderScale,
                                   const QPointF & viewportPos,
                                   const QPointF & pos,
                                   double pressure,
                                   TimeValue timestamp)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    bool didSomething = false;


    if ( app->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return false;
    }

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);


    NodePtr lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if ( notifyOverlaysPenMotion_internal(*it, renderScale, viewportPos, pos, pressure, timestamp) ) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }


    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if ( notifyOverlaysPenMotion_internal(*it, renderScale, viewportPos, pos, pressure, timestamp) ) {
            return true;
        }
    }


    return didSomething;
}

bool
ViewerTab::notifyOverlaysPenUp(const RenderScale & renderScale,
                               const QPointF & viewportPos,
                               const QPointF & pos,
                               double pressure,
                               TimeValue timestamp)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    bool didSomething = false;

    if ( app->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return false;
    }


    ///Reset draft
    bool mustTriggerRender = false;
    if ( getGui()->isDraftRenderEnabled() ) {
        getGui()->setDraftRenderEnabled(false);
        mustTriggerRender = _imp->hasCaughtPenMotionWhileDragging;
    }

    _imp->hasPenDown = false;
    _imp->hasCaughtPenMotionWhileDragging = false;


    _imp->lastOverlayNode.reset();

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();


    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);


    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {

        ViewerNodePtr isViewerNode = (*it)->isEffectViewerNode();
        if (isViewerNode && isViewerNode != getInternalNode()) {
            continue;
        }
        QPointF transformPos;
#ifndef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        transformPos = pos;
#else
        TimeValue transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        if (ok) {
            /*
             * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
             * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
             * that interact is not editable when it is retimed.
             */
            if (time != transformedTime) {
                return false;
            }
            time = transformedTime;
        }

        Transform::Matrix3x3 mat(1, 0, 0, 0, 1, 0, 0, 0, 1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        if (!ok) {
            transformPos = pos;
        } else {
            Transform::Matrix3x3 inverse;
            if (!mat.inverse(&inverse)) {
                inverse.setIdentity();
            }
            mat = inverse;

            {
                Transform::Point3D p;
                p.x = pos.x();
                p.y = pos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                if (p.z < 0) {
                    // the transformed position has a negative z, send it (because a penup may be necessary for a good execution of the plugin) but reverse z
                    p.z = - p.z;
                }
                transformPos.rx() = p.x / p.z;
                transformPos.ry() = p.y / p.z;
            }
        }
#endif

        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            EffectInstancePtr effect = (*it)->getEffectInstance();
            assert(effect);
            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                didSomething |= (*it2)->onOverlayPenUp_public(_imp->viewer, time, renderScale, view, viewportPos, transformPos, pressure, timestamp);

            }

        }
    }


    if (mustTriggerRender) {
        //We had draft enabled but penRelease didn't trigger any render, trigger one to refresh the viewer
        app->renderAllViewers();
    }


    return didSomething;
} // ViewerTab::notifyOverlaysPenUp


bool
ViewerTab::checkForTimelinePlayerGlobalShortcut(Qt::Key qKey,
                                                const Qt::KeyboardModifiers& mods)
{
    const char* knobsToCheck[] = {
        kViewerNodeParamSetInPoint,
        kViewerNodeParamSetOutPoint,
        kViewerNodeParamFirstFrame,
        kViewerNodeParamPlayBackward,
        kViewerNodeParamPlayForward,
        kViewerNodeParamLastFrame,
        kViewerNodeParamPreviousFrame,
        kViewerNodeParamNextFrame,
        kViewerNodeParamPreviousKeyFrame,
        kViewerNodeParamNextKeyFrame,
        kViewerNodeParamPreviousIncr,
        kViewerNodeParamNextIncr,
        NULL
    };

    ViewerNodePtr node = getInternalNode();
    const KnobsVec& knobs = node->getKnobs();
    int i = 0;
    std::string pluginShortcutGroup;
    while (knobsToCheck[i]) {
        for (KnobsVec::const_iterator it = knobs.begin(); it!=knobs.end(); ++it) {
            if ((*it)->getName() != knobsToCheck[i]) {
                continue;
            }
            if ( (*it)->getInViewerContextHasShortcut() && !(*it)->getInViewerContextSecret() ) {
                if ( pluginShortcutGroup.empty() ) {
                    pluginShortcutGroup = node->getNode()->getOriginalPlugin()->getPluginShortcutGroup();
                }
                if ( isKeybind(pluginShortcutGroup, (*it)->getName(), mods, qKey) ) {
                    // This only works for groups and buttons, as defined in the spec
                    KnobButtonPtr isButton = toKnobButton(*it);
                    bool ret = false;

                    if (isButton) {
                        ret = isButton->trigger();
                    }
                    return ret;
                }
            }
        }
        ++i;
    }
    return false;
}


bool
ViewerTab::checkNodeViewerContextShortcuts(const NodePtr& node,
                                           Qt::Key qKey,
                                           const Qt::KeyboardModifiers& mods)
{
    // Intercept plug-in defined shortcuts
    NodeViewerContextPtr context;

    for (std::list<ViewerTabPrivate::PluginViewerContext>::const_iterator it = _imp->currentNodeContext.begin(); it != _imp->currentNodeContext.end(); ++it) {
        NodeGuiPtr n = it->currentNode.lock();
        if (!n) {
            continue;
        }
        if (n->getNode() == node) {
            context = it->currentContext;
            break;
        }
    }

    // This is not an active node on the viewer ui, don't trigger any shortcuts
    if (!context) {
        return false;
    }

    EffectInstancePtr effect = node->getEffectInstance();
    assert(effect);
    const KnobsVec& knobs = effect->getKnobs();
    std::string pluginShortcutGroup;
    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        if ( (*it)->getInViewerContextHasShortcut() && !(*it)->getInViewerContextSecret() ) {
            if ( pluginShortcutGroup.empty() ) {
                pluginShortcutGroup = effect->getNode()->getOriginalPlugin()->getPluginShortcutGroup();
            }
            if ( isKeybind(pluginShortcutGroup, (*it)->getName(), mods, qKey) ) {
                // This only works for groups and buttons, as defined in the spec
                KnobButtonPtr isButton = toKnobButton(*it);
                KnobGroupPtr isGrp = toKnobGroup(*it);
                bool ret = false;

                if (isButton) {
                    if ( isButton->getIsCheckable() ) {

                        // Refresh the button state
                        isButton->setValue(!isButton->getValue(), ViewSetSpec::all(), DimIdx(0), eValueChangedReasonUserEdited, true /*forceHandlerEvenIfNoChange*/);

                        ret = true;
                    } else {
                        ret = isButton->trigger();
                    }
                } else if (isGrp) {
                    // This can only be a toolbutton, notify the NodeViewerContext
                    context->onToolButtonShortcutPressed( QString::fromUtf8( isGrp->getName().c_str() ) );
                    ret = true;
                }

                return ret;
            }
        }
    }

    return false;
} // ViewerTab::checkNodeViewerContextShortcuts

bool
ViewerTab::notifyOverlaysKeyDown_internal(const NodePtr& node,
                                          const RenderScale & renderScale,
                                          Key k,
                                          KeyboardModifiers km,
                                          Qt::Key qKey,
                                          const Qt::KeyboardModifiers& mods)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    ViewerNodePtr isViewerNode = node->isEffectViewerNode();
    if (isViewerNode && isViewerNode != getInternalNode()) {
        return false;
    }

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    TimeValue transformedTime;
    bool ok = _imp->getTimeTransform(time, ViewIdx(0), node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }
#endif


    bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(node);
    if (!isInActiveViewerUI) {
        EffectInstancePtr effect = node->getEffectInstance();
        assert(effect);

        bool didSmthing = checkNodeViewerContextShortcuts(node, qKey, mods);

        if (didSmthing) {
            return true;
        } else {
            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                didSmthing = (*it2)->onOverlayKeyDown_public(_imp->viewer, time, renderScale, view, k, km);
                if (didSmthing) {
                    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                    // if the instance returns kOfxStatOK, the host should not pass the pen motion

                    // to any other interactive object it may own that shares the same view.
                    _imp->lastOverlayNode = node;
                    
                    return true;
                }
            }
        }

    }


    return false;
} // ViewerTab::notifyOverlaysKeyDown_internal

bool
ViewerTab::notifyOverlaysKeyDown(const RenderScale & renderScale,
                                 QKeyEvent* e)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    bool didSomething = false;

    /*
       Modifiers key down/up should be passed to all active interacts always so that they can properly figure out
       whether they are up or down
     */
    bool isModifier = e->key() == Qt::Key_Control || e->key() == Qt::Key_Shift || e->key() == Qt::Key_Alt ||
                      e->key() == Qt::Key_Meta;
    Qt::Key qKey = (Qt::Key)e->key();
    Qt::KeyboardModifiers qMods = e->modifiers();
    Key natronKey = QtEnumConvert::fromQtKey(qKey );
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers(qMods);

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();

    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);


    NodePtr lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if ( notifyOverlaysKeyDown_internal(*it, renderScale, natronKey, natronMod, qKey, qMods) ) {
                    didSomething = true;
                    if (isModifier) {
                        nodes.erase(it);
                        break;
                    }
                } else {
                    nodes.erase(it);
                }
                break;

            }
        }
    }


    for (NodesList::reverse_iterator it = nodes.rbegin();
         it != nodes.rend();
         ++it) {
        if ( notifyOverlaysKeyDown_internal(*it, renderScale, natronKey, natronMod, qKey, qMods) ) {
            if (isModifier) {
                continue;
            }

            didSomething = true;
            break;
        }
    }


    if (isModifier) {
        // Modifiers may not necessarily return true for plug-ins but may require a redraw
        app->redrawAllViewers();
    }




    return didSomething;
} // ViewerTab::notifyOverlaysKeyDown

bool
ViewerTab::notifyOverlaysKeyUp(const RenderScale & renderScale,
                               QKeyEvent* e)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    bool didSomething = false;

    _imp->lastOverlayNode.reset();

    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();
    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);

    /*
     Do not catch the event if this is a modifier, let it propagate to the Gui
     */
    bool isModifier = e->key() == Qt::Key_Control || e->key() == Qt::Key_Shift || e->key() == Qt::Key_Alt ||
    e->key() == Qt::Key_Meta;

    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {

        ViewerNodePtr isViewerNode = (*it)->isEffectViewerNode();
        if (isViewerNode && isViewerNode != getInternalNode()) {
            continue;
        }
        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        TimeValue transformedTime;
        bool ok = _imp->getTimeTransform(time, ViewIdx(0), *it, getInternalNode(), &transformedTime);
        if (ok) {
            /*
             * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
             * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
             * that interact is not editable when it is retimed.
             */
            if (time != transformedTime) {
                return false;
            }
            time = transformedTime;
        }
#endif


        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                didSomething |= (*it2)->onOverlayKeyUp_public(_imp->viewer, time, renderScale, view,
                                                              QtEnumConvert::fromQtKey( (Qt::Key)e->key() ), QtEnumConvert::fromQtModifiers( e->modifiers() ) );

            }
        }
    }


    if (isModifier) {
        //Modifiers may not necessarily return true for plug-ins but may require a redraw
        app->redrawAllViewers();
    }


    return didSomething;
} // ViewerTab::notifyOverlaysKeyUp

bool
ViewerTab::notifyOverlaysKeyRepeat_internal(const NodePtr& node,
                                            const RenderScale & renderScale,
                                            Key k,
                                            KeyboardModifiers km,
                                            Qt::Key qKey,
                                            const Qt::KeyboardModifiers& mods)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    ViewerNodePtr isViewerNode = node->isEffectViewerNode();
    if (isViewerNode && isViewerNode != getInternalNode()) {
        return false;
    }

    ViewIdx view = getInternalNode()->getCurrentRenderView();
    TimeValue time(app->getTimeLine()->currentFrame());

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    TimeValue transformedTime;
    bool ok = _imp->getTimeTransform(time, ViewIdx(0), node, getInternalNode(), &transformedTime);
    if (ok) {
        /*
         * Do not allow interaction with retimed interacts otherwise the user may end up modifying keyframes at unexpected
         * (or invalid for floating point) frames, which may be confusing. Rather we indicate with the overlay color hint
         * that interact is not editable when it is retimed.
         */
        if (time != transformedTime) {
            return false;
        }
        time = transformedTime;
    }
#endif

    bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(node);
    if (!isInActiveViewerUI) {
        EffectInstancePtr effect = node->getEffectInstance();
        assert(effect);


        bool didSmthing = checkNodeViewerContextShortcuts(node, qKey, mods);

        if (!didSmthing) {
            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                didSmthing = (*it2)->onOverlayKeyRepeat_public(_imp->viewer, time, renderScale, view, k, km);
                if (didSmthing) {
                    //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
                    // if the instance returns kOfxStatOK, the host should not pass the pen motion

                    // to any other interactive object it may own that shares the same view.
                    _imp->lastOverlayNode = node;
                    
                    return true;
                }
            }
        }

    }


    return false;
}

bool
ViewerTab::notifyOverlaysKeyRepeat(const RenderScale & renderScale,
                                   QKeyEvent* e)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }

    Qt::Key qKey = (Qt::Key)e->key();
    Qt::KeyboardModifiers qMods = e->modifiers();
    Key natronKey = QtEnumConvert::fromQtKey( qKey);
    KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( qMods );
    NodesList nodes;
    NodePtr lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (NodesList::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if ( notifyOverlaysKeyRepeat_internal(*it, renderScale, natronKey, natronMod, qKey, qMods) ) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }


    for (NodesList::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if ( notifyOverlaysKeyRepeat_internal(*it, renderScale, natronKey, natronMod, qKey, qMods) ) {
            return true;
        }
    }



    return false;
}

bool
ViewerTab::notifyOverlaysFocusGained(const RenderScale & renderScale)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }

    if ( app->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return false;
    }


    TimeValue time(app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();
    bool ret = false;
    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {

        ViewerNodePtr isViewerNode = (*it)->isEffectViewerNode();
        if (isViewerNode && isViewerNode != getInternalNode()) {
            continue;
        }

        EffectInstancePtr effect = (*it)->getEffectInstance();
        assert(effect);

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        TimeValue transformedTime;
        bool ok = _imp->getTimeTransform(time, ViewIdx(0), *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
#endif

        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                ret |= (*it2)->onOverlayFocusGained_public(_imp->viewer, time, renderScale, view);

            }

        }
    }

    return ret;
} // ViewerTab::notifyOverlaysFocusGained

bool
ViewerTab::notifyOverlaysFocusLost(const RenderScale & renderScale)
{
    Gui* gui = getGui();
    if (!gui) {
        return false;
    }
    GuiAppInstancePtr app = gui->getApp();
    if (!app) {
        return false;
    }
    if ( app->isClosing() ) {
        return false;
    }
    if ( !getInternalNode() ) {
        return false;
    }

    if ( app->isShowingDialog() ) {
        /*
           We may enter a situation where a plug-in called EffectInstance::message to show a dialog
           and would block the main thread until the user would click OK but Qt would request a paintGL() on the viewer
           because of focus changes. This would end-up in the interact draw action being called whilst the message() function
           did not yet return and may in some plug-ins cause deadlocks (happens in all Genarts Sapphire plug-ins).
         */
        return false;
    }


    TimeValue time (app->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();
    bool ret = false;
    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {

        ViewerNodePtr isViewerNode = (*it)->isEffectViewerNode();
        if (isViewerNode && isViewerNode != getInternalNode()) {
            continue;
        }

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        TimeValue transformedTime;
        bool ok = _imp->getTimeTransform(time, ViewIdx(0), *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
#endif
        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            EffectInstancePtr effect = (*it)->getEffectInstance();
            assert(effect);

            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                ret |= (*it2)->onOverlayFocusLost_public(_imp->viewer, time, renderScale, view);

            }
        }
    }


    return ret;
} // ViewerTab::notifyOverlaysFocusLost


void
ViewerTab::updateSelectionFromViewerSelectionRectangle(bool onRelease)
{

    TimeValue time(getGui()->getApp()->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();


    RectD rect;
    _imp->viewer->getSelectionRectangle(rect.x1, rect.x2, rect.y1, rect.y2);

    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            EffectInstancePtr effect = (*it)->getEffectInstance();
            assert(effect);

            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                (*it2)->onViewportSelectionUpdated(rect, onRelease);
            }
        }
    }
} // updateSelectionFromViewerSelectionRectangle

void
ViewerTab::onViewerSelectionCleared()
{
    TimeValue time(getGui()->getApp()->getTimeLine()->currentFrame());
    ViewIdx view = getInternalNode()->getCurrentRenderView();


    NodesList nodes;
    getNodesEntitledForOverlays(time, view, nodes);
    for (NodesList::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        bool isInActiveViewerUI = _imp->hasInactiveNodeViewerContext(*it);
        if (!isInActiveViewerUI) {
            EffectInstancePtr effect = (*it)->getEffectInstance();
            assert(effect);

            std::list<OverlayInteractBasePtr> overlays;
            effect->getOverlays(eOverlayViewportTypeViewer, &overlays);
            for (std::list<OverlayInteractBasePtr>::const_iterator it2 = overlays.begin(); it2 != overlays.end(); ++it2) {
                (*it2)->onViewportSelectionCleared();
            }
        }
    }
} // onViewerSelectionCleared


NATRON_NAMESPACE_EXIT

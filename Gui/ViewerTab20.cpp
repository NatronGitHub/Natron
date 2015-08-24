//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>

#include "ViewerTab.h"
#include "ViewerTabPrivate.h"

#include <cassert>

#include "Global/GLIncludes.h" //!<must be included before QGlWidget because of gl.h and glew.h
#include "Global/Macros.h"

#include <QtCore/QDebug>

GCC_DIAG_UNUSED_PRIVATE_FIELD_OFF
// /opt/local/include/QtGui/qmime.h:119:10: warning: private field 'type' is not used [-Wunused-private-field]
#include <QtGui/QKeyEvent>
GCC_DIAG_UNUSED_PRIVATE_FIELD_ON

#include "Engine/Node.h"
#include "Engine/Settings.h"
#include "Engine/TimeLine.h"
#include "Engine/Transform.h"
#include "Engine/ViewerInstance.h"

#include "Gui/Gui.h"
#include "Gui/GuiAppInstance.h"
#include "Gui/NodeGui.h"
#include "Gui/QtEnumConvert.h"
#include "Gui/RotoGui.h"
#include "Gui/SpinBox.h"
#include "Gui/TimeLineGui.h"
#include "Gui/TrackerGui.h"
#include "Gui/ViewerGL.h"


using namespace Natron;

#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
//OpenGL is column-major for matrixes
static void transformToOpenGLMatrix(const Transform::Matrix3x3& mat,GLdouble* oglMat)
{
    oglMat[0] = mat.a; oglMat[4] = mat.b; oglMat[8]  = 0; oglMat[12] = mat.c;
    oglMat[1] = mat.d; oglMat[5] = mat.e; oglMat[9]  = 0; oglMat[13] = mat.f;
    oglMat[2] = 0;     oglMat[6] = 0;     oglMat[10] = 1; oglMat[14] = 0;
    oglMat[3] = mat.g; oglMat[7] = mat.h; oglMat[11] = 0; oglMat[15] = mat.i;
}
#endif

void
ViewerTab::drawOverlays(double time,
                        double scaleX,
                        double scaleY) const
{

    if ( !_imp->app ||
        !_imp->viewer ||
        _imp->app->isClosing() ||
        isFileDialogViewer() ||
        !_imp->gui ||
        (_imp->gui->isGUIFrozen() && !_imp->app->getIsUserPainting())) {
        return;
    }
    
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    int view = getCurrentView();
#endif
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    ///Draw overlays in reverse order of appearance so that the first (top) panel is drawn on top of everything else
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        
        boost::shared_ptr<NodeGui> nodeUi = boost::dynamic_pointer_cast<NodeGui>((*it)->getNodeGui());
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
        
        Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        GLfloat oldMat[16];
        if (ok) {
            //Ok we've got a transform here, apply it to the OpenGL model view matrix
            
            GLdouble oglMat[16];
            transformToOpenGLMatrix(mat,oglMat);
            glMatrixMode(GL_MODELVIEW);
            glGetFloatv(GL_MODELVIEW_MATRIX, oldMat);
            glMultMatrixd(oglMat);
        }
        
#endif
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                _imp->currentRoto.second->drawOverlays(time, scaleX, scaleY);
            }
        } else if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                _imp->currentTracker.second->drawOverlays(time, scaleX, scaleY);
            }
        } else {
            
            Natron::EffectInstance* effect = (*it)->getLiveInstance();
            assert(effect);
            effect->setCurrentViewportForOverlays_public(_imp->viewer);
            effect->drawOverlay_public(time, scaleX,scaleY);
        }
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        if (ok) {
            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf(oldMat);
        }
#endif
    }
}

bool
ViewerTab::notifyOverlaysPenDown_internal(const boost::shared_ptr<Natron::Node>& node,
                                          double scaleX,
                                          double scaleY,
                                          Natron::PenType pen,
                                          bool isTabletEvent,
                                          const QPointF & viewportPos,
                                          const QPointF & pos,
                                          double pressure,
                                          double timestamp,
                                          QMouseEvent* e)
{

    QPointF transformViewportPos;
    QPointF transformPos;
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    
    
    int view = getCurrentView();
    
    double transformedTime;
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
    
    Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
    ok = _imp->getOverlayTransform(time, view, node, getInternalNode(), &mat);
    if (!ok) {
        transformViewportPos = viewportPos;
        transformPos = pos;
    } else {
        mat = Transform::matInverse(mat);
        {
            Transform::Point3D p;
            p.x = viewportPos.x();
            p.y = viewportPos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformViewportPos.rx() = p.x / p.z;
            transformViewportPos.ry() = p.y / p.z;
        }
        {
            Transform::Point3D p;
            p.x = pos.x();
            p.y = pos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformPos.rx() = p.x / p.z;
            transformPos.ry() = p.y / p.z;
        }
    }
    
   
#else
    transformViewportPos = viewportPos;
    transformPos = pos;
#endif
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->penDown(time, scaleX, scaleY, pen, isTabletEvent, transformViewportPos, transformPos, pressure, timestamp, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else if (_imp->currentTracker.first && node == _imp->currentTracker.first->getNode()) {
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->penDown(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else {
        
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayPenDown_public(time, scaleX, scaleY, transformViewportPos, transformPos, pressure);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    
    return false;
}

bool
ViewerTab::notifyOverlaysPenDown(double scaleX,
                                 double scaleY,
                                 Natron::PenType pen,
                                 bool isTabletEvent,
                                 const QPointF & viewportPos,
                                 const QPointF & pos,
                                 double pressure,
                                 double timestamp,
                                 QMouseEvent* e)
{

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }

    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                
                if (notifyOverlaysPenDown_internal(*it, scaleX, scaleY, pen, isTabletEvent, viewportPos, pos, pressure, timestamp, e)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }
    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (notifyOverlaysPenDown_internal(*it, scaleX, scaleY, pen, isTabletEvent, viewportPos, pos, pressure, timestamp, e)) {
            return true;
        }
    }

    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
    return false;
}


bool
ViewerTab::notifyOverlaysPenDoubleClick(double scaleX,
                                        double scaleY,
                                        const QPointF & viewportPos,
                                        const QPointF & pos,
                                        QMouseEvent* e)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    

    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        QPointF transformViewportPos;
        QPointF transformPos;
        double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        int view = getCurrentView();
        
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, view, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
        
        Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        if (!ok) {
            transformViewportPos = viewportPos;
            transformPos = pos;
        } else {
            mat = Transform::matInverse(mat);
            {
                Transform::Point3D p;
                p.x = viewportPos.x();
                p.y = viewportPos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformViewportPos.rx() = p.x / p.z;
                transformViewportPos.ry() = p.y / p.z;
            }
            {
                Transform::Point3D p;
                p.x = pos.x();
                p.y = pos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformPos.rx() = p.x / p.z;
                transformPos.ry() = p.y / p.z;
            }
        }
        
        

#else
        transformViewportPos = viewportPos;
        transformPos = pos;
#endif
        
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->penDoubleClicked(time, scaleX, scaleY, transformViewportPos, transformPos, e) ) {
                _imp->lastOverlayNode = _imp->currentRoto.first->getNode();
                return true;
            }
        }
        
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->penDoubleClicked(time, scaleX, scaleY, transformViewportPos, transformPos, e) ) {
                _imp->lastOverlayNode = _imp->currentRoto.first->getNode();
                return true;
            }
        }
    }

    return false;
}

bool
ViewerTab::notifyOverlaysPenMotion_internal(const boost::shared_ptr<Natron::Node>& node,
                                            double scaleX,
                                            double scaleY,
                                            const QPointF & viewportPos,
                                            const QPointF & pos,
                                            double pressure,
                                            double timestamp,
                                            QInputEvent* e)
{
    
    QPointF transformViewportPos;
    QPointF transformPos;
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    int view = getCurrentView();
    
    double transformedTime;
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
    

    
    Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
    ok = _imp->getOverlayTransform(time, view, node, getInternalNode(), &mat);
    if (!ok) {
        transformViewportPos = viewportPos;
        transformPos = pos;
    } else {
        mat = Transform::matInverse(mat);
        {
            Transform::Point3D p;
            p.x = viewportPos.x();
            p.y = viewportPos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformViewportPos.rx() = p.x / p.z;
            transformViewportPos.ry() = p.y / p.z;
        }
        {
            Transform::Point3D p;
            p.x = pos.x();
            p.y = pos.y();
            p.z = 1;
            p = Transform::matApply(mat, p);
            transformPos.rx() = p.x / p.z;
            transformPos.ry() = p.y / p.z;
        }
    }
    
#else
    transformViewportPos = viewportPos;
    transformPos = pos;
#endif
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->penMotion(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, timestamp, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else if (_imp->currentTracker.first && node == _imp->currentTracker.first->getNode()) {
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->penMotion(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else {
        
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayPenMotion_public(time, scaleX, scaleY, transformViewportPos, transformPos, pressure);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    return false;
}

bool
ViewerTab::notifyOverlaysPenMotion(double scaleX,
                                   double scaleY,
                                   const QPointF & viewportPos,
                                   const QPointF & pos,
                                   double pressure,
                                   double timestamp,
                                   QInputEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if (notifyOverlaysPenMotion_internal(*it, scaleX, scaleY, viewportPos, pos, pressure, timestamp, e)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }

    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (notifyOverlaysPenMotion_internal(*it, scaleX, scaleY, viewportPos, pos, pressure, timestamp, e)) {
            return true;
        }
    }

   
    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
   

    return didSomething;
}

bool
ViewerTab::notifyOverlaysPenUp(double scaleX,
                               double scaleY,
                               const QPointF & viewportPos,
                               const QPointF & pos,
                               double pressure,
                               double timestamp,
                               QMouseEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    _imp->lastOverlayNode.reset();
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        
        
        QPointF transformViewportPos;
        QPointF transformPos;
        double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        int view = getCurrentView();
        
        
        double transformedTime;
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
        
        Transform::Matrix3x3 mat(1,0,0,0,1,0,0,0,1);
        ok = _imp->getOverlayTransform(time, view, *it, getInternalNode(), &mat);
        if (!ok) {
            transformViewportPos = viewportPos;
            transformPos = pos;
        } else {
            mat = Transform::matInverse(mat);
            {
                Transform::Point3D p;
                p.x = viewportPos.x();
                p.y = viewportPos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformViewportPos.rx() = p.x / p.z;
                transformViewportPos.ry() = p.y / p.z;
            }
            {
                Transform::Point3D p;
                p.x = pos.x();
                p.y = pos.y();
                p.z = 1;
                p = Transform::matApply(mat, p);
                transformPos.rx() = p.x / p.z;
                transformPos.ry() = p.y / p.z;
            }
        }
        
        

#else
        transformViewportPos = viewportPos;
        transformPos = pos;
#endif
        
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                didSomething |= _imp->currentRoto.second->penUp(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, timestamp, e);
            }
        }
        if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                didSomething |=  _imp->currentTracker.second->penUp(time, scaleX, scaleY, transformViewportPos, transformPos, pressure, e)  ;
            }
        }
        
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        didSomething |= effect->onOverlayPenUp_public(time, scaleX, scaleY, transformViewportPos, transformPos, pressure);
        
        
    }

   
    if (!didSomething && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
    

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyDown_internal(const boost::shared_ptr<Natron::Node>& node,double scaleX,double scaleY,QKeyEvent* e,
                                          Natron::Key k,
                                          Natron::KeyboardModifiers km)
{
    
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    double transformedTime;
    bool ok = _imp->getTimeTransform(time, 0, node, getInternalNode(), &transformedTime);
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
    
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->keyDown(time, scaleX, scaleY, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else if (_imp->currentTracker.first && node == _imp->currentTracker.first->getNode()) {
        if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
            if ( _imp->currentTracker.second->keyDown(time, scaleX, scaleY, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
        
    } else {
   
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayKeyDown_public(time, scaleX,scaleY,k,km);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    return false;
}

bool
ViewerTab::notifyOverlaysKeyDown(double scaleX,
                                 double scaleY,
                                 QKeyEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }

    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if (notifyOverlaysKeyDown_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }

    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin();
         it != nodes.rend();
         ++it) {
        if (notifyOverlaysKeyDown_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
            return true;
        }
    }

    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyUp(double scaleX,
                               double scaleY,
                               QKeyEvent* e)
{
    bool didSomething = false;

    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    _imp->lastOverlayNode.reset();


    double time = _imp->app->getTimeLine()->currentFrame();

    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, 0, *it, getInternalNode(), &transformedTime);
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
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                didSomething |= _imp->currentRoto.second->keyUp(time, scaleX, scaleY, e);
            }
        }
        if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                didSomething |= _imp->currentTracker.second->keyUp(time, scaleX, scaleY, e);
            }
        }
        
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        didSomething |= effect->onOverlayKeyUp_public( time, scaleX,scaleY,
                                            QtEnumConvert::fromQtKey( (Qt::Key)e->key() ),QtEnumConvert::fromQtModifiers( e->modifiers() ) );
        
    }
    
   
    if (!didSomething && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();
    

    return didSomething;
}

bool
ViewerTab::notifyOverlaysKeyRepeat_internal(const boost::shared_ptr<Natron::Node>& node,double scaleX,double scaleY,QKeyEvent* e,Natron::Key k,
                                      Natron::KeyboardModifiers km)
{
    
    double time = _imp->app->getTimeLine()->currentFrame();
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
    double transformedTime;
    bool ok = _imp->getTimeTransform(time, 0, node, getInternalNode(), &transformedTime);
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
    
    if (_imp->currentRoto.first && node == _imp->currentRoto.first->getNode()) {
        
        if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
            if ( _imp->currentRoto.second->keyRepeat(time, scaleX, scaleY, e) ) {
                _imp->lastOverlayNode = node;
                return true;
            }
        }
    } else {
        //if (_imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible()) {
        //    if (_imp->currentTracker.second->loseFocus(scaleX, scaleY,e)) {
        //        return true;
        //    }
        //}
        Natron::EffectInstance* effect = node->getLiveInstance();
        assert(effect);
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayKeyRepeat_public(time, scaleX,scaleY,k,km);
        if (didSmthing) {
            //http://openfx.sourceforge.net/Documentation/1.3/ofxProgrammingReference.html
            // if the instance returns kOfxStatOK, the host should not pass the pen motion
            
            // to any other interactive object it may own that shares the same view.
            _imp->lastOverlayNode = node;
            return true;
        }
    }
    return false;
}

bool
ViewerTab::notifyOverlaysKeyRepeat(double scaleX,
                                   double scaleY,
                                   QKeyEvent* e)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    Natron::Key natronKey = QtEnumConvert::fromQtKey( (Qt::Key)e->key() );
    Natron::KeyboardModifiers natronMod = QtEnumConvert::fromQtModifiers( e->modifiers() );
    
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    
    boost::shared_ptr<Natron::Node> lastOverlay = _imp->lastOverlayNode.lock();
    if (lastOverlay) {
        for (std::list<boost::shared_ptr<Natron::Node> >::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            if (*it == lastOverlay) {
                if (notifyOverlaysKeyRepeat_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
                    return true;
                } else {
                    nodes.erase(it);
                    break;
                }
            }
        }
    }
    

    
    for (std::list<boost::shared_ptr<Natron::Node> >::reverse_iterator it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (notifyOverlaysKeyRepeat_internal(*it, scaleX, scaleY, e, natronKey, natronMod)) {
            return true;
        }
    }

    if (getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();


    return false;
}

bool
ViewerTab::notifyOverlaysFocusGained(double scaleX,
                                     double scaleY)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    double time = _imp->app->getTimeLine()->currentFrame();

    
    bool ret = false;
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, 0, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
#endif
        
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayFocusGained_public(time, scaleX,scaleY);
        if (didSmthing) {
            ret = true;
        }
        
    }
    
    if (!ret && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();

    return ret;
}

bool
ViewerTab::notifyOverlaysFocusLost(double scaleX,
                                   double scaleY)
{
    if ( !_imp->app || _imp->app->isClosing() ) {
        return false;
    }
    
    double time = _imp->app->getTimeLine()->currentFrame();
    
    bool ret = false;
    std::list<boost::shared_ptr<Natron::Node> >  nodes;
    getGui()->getNodesEntitledForOverlays(nodes);
    for (std::list<boost::shared_ptr<Natron::Node> >::const_iterator it = nodes.begin(); it != nodes.end(); ++it) {
        
#ifdef NATRON_TRANSFORM_AFFECTS_OVERLAYS
        double transformedTime;
        bool ok = _imp->getTimeTransform(time, 0, *it, getInternalNode(), &transformedTime);
        if (ok) {
            time = transformedTime;
        }
#endif
        
        if (_imp->currentRoto.first && (*it) == _imp->currentRoto.first->getNode()) {
            
            if ( _imp->currentRoto.second && _imp->currentRoto.first->isSettingsPanelVisible() ) {
                _imp->currentRoto.second->focusOut(time);
            }
        } else if (_imp->currentTracker.first && (*it) == _imp->currentTracker.first->getNode()) {
            if ( _imp->currentTracker.second && _imp->currentTracker.first->isSettingsPanelVisible() ) {
                if ( _imp->currentTracker.second->loseFocus(time, scaleX, scaleY) ) {
                    return true;
                }
            }
        }
        
        Natron::EffectInstance* effect = (*it)->getLiveInstance();
        assert(effect);
        
        effect->setCurrentViewportForOverlays_public(_imp->viewer);
        bool didSmthing = effect->onOverlayFocusLost_public(time, scaleX,scaleY);
        if (didSmthing) {
            ret = true;
        }
    }
    
    
    if (!ret && getGui()->getApp()->getOverlayRedrawRequestsCount() > 0) {
        getGui()->getApp()->redrawAllViewers();
    }
    getGui()->getApp()->clearOverlayRedrawRequests();

    return ret;
}
